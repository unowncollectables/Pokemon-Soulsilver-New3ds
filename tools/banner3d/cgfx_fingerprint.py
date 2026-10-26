"""Structural fingerprint of a CGFX banner, and a comparison against a known-good one.

The fingerprint is made from pycgfx's in-memory CGFX tree just before it is written (so it describes exactly the bytes that
are written): for every object class, how many there are, how many bytes they take, and for every integer / enum / flag
slot of its record, which values occur and how often. Floats, strings, offsets and vertex / texel data are not recorded
(only their lengths), so a fingerprint holds no game data and is safe to commit.

`reference/platinum_banner.json` is the fingerprint of the Platinum 3DS port's banner (CGFX SHA-1 61f6c04a..., the one
that runs on a real New 3DS XL HOME Menu). `check(candidate)` lists every way a banner differs from it that is not justified
by the rules in `JUSTIFIED` below; make_banner.py fails the 3D build (the Makefile then uses the flat banner) on any.

    python3 tools/banner3d/cgfx_fingerprint.py FINGERPRINT.json [REFERENCE.json]      # diff two fingerprints
"""
import collections
import enum
import json
import os
import struct
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
REFERENCE = os.path.join(HERE, "reference", "platinum_banner.json")
MAX_DISTINCT = 12          # a slot with more distinct values than this is data (an index, a size), not a setting

# Slots whose values are content-dependent even though they are small integers (counts and indexes). Everything else that is
# an integer must be a value the Platinum banner also uses.
IGNORE_SLOTS = {"DICT", "Node", "CGFXHeader", "CGFXData", "List", "DictInfo", "ListData"}


def _walk(obj, seen, out):
    """Visit every object reachable from obj through values(). `seen` maps id -> object: values() builds temporary
    objects whose ids would be reused after garbage collection if they were not kept alive."""
    from cgfx.shared import BaseObject, Reference
    if isinstance(obj, Reference):
        obj = obj.obj
    if obj is None or not isinstance(obj, BaseObject) or id(obj) in seen:
        return
    seen[id(obj)] = obj
    cls = type(obj).__name__
    try:
        obj.refresh_struct()
        vals = obj.values()
        size = obj.size()
    except Exception:
        return
    rec = out.setdefault(cls, {"count": 0, "bytes": 0, "slots": {}})
    rec["count"] += 1
    rec["bytes"] += 0 if _is_inline(obj) else size
    for i, v in enumerate(vals):
        slot = rec["slots"].setdefault(str(i), collections.Counter())
        if isinstance(v, BaseObject):
            _walk(v, seen, out)
        elif isinstance(v, Reference):
            _walk(v.obj, seen, out)
        elif isinstance(v, bool) or isinstance(v, enum.Enum) or isinstance(v, int):
            slot[str(int(v))] += 1
        elif isinstance(v, float):
            slot["f"] += 1
        elif isinstance(v, (bytes, bytearray)):
            slot[f"bytes"] += 1
            rec.setdefault("data_bytes", 0)
            rec["data_bytes"] += len(v)
        elif isinstance(v, str):
            slot["s"] += 1
        elif v is None:
            slot["null"] += 1
        else:
            slot[type(v).__name__] += 1


def _is_inline(obj):
    from cgfx.shared import InlineObject
    return isinstance(obj, InlineObject)


def fingerprint(root, raw):
    """root = pycgfx's CGFX object, raw = the bytes pycgfx wrote from it."""
    out = {}
    _walk(root, {}, out)
    classes = {}
    for cls, rec in out.items():
        slots = {}
        for k, c in rec["slots"].items():
            slots[k] = dict(sorted(c.items()))
        classes[cls] = {"count": rec["count"], "bytes": rec["bytes"], "slots": slots}
        if "data_bytes" in rec:
            classes[cls]["data_bytes"] = rec["data_bytes"]
    sig, bom, hsize, ver, fsize, blocks = struct.unpack_from("<4sHhiii", raw, 0)
    dsig, dsize = struct.unpack_from("<4si", raw, 0x14)
    dicts = [struct.unpack_from("<ii", raw, 0x1C + 8 * i)[0] for i in range(15)]
    names = ("models textures luts materials shaders cameras lights fogs scenes skeletal_anims material_anims "
             "visibility_anims camera_anims light_anims emitters").split()
    return {
        "container": {"signature": sig.decode(), "bom": bom, "header_size": hsize, "version": ver, "blocks": blocks,
                      "data_signature": dsig.decode(), "dict_counts": dict(zip(names, dicts)),
                      "has_imag": _imag_len(raw) > 0},
        "sizes": {"file": len(raw), "data_section": dsize, "imag": _imag_len(raw)},
        "classes": classes,
    }


def _imag_len(raw):
    """Size of the trailing IMAG block (texel data), 0 if none."""
    pos = raw.rfind(b"IMAG")
    if pos < 0:
        return 0
    size = struct.unpack_from("<i", raw, pos + 4)[0]
    return size if pos + 8 + size <= len(raw) + 0x40 else 0


# ---- comparison -------------------------------------------------------------------------------------------------------
# Every way a banner may differ from the Platinum one without being a risk, and why. Anything else is a mismatch.
COUNT_SLACK = 1.10         # object counts and data sizes may exceed Platinum's by this factor (+2 objects) ...
COUNT_EXEMPT = {"DICT", "Node", "List", "ListData", "DictInfo", "Matrix", "Vector3", "Vector4", "ColorByte", "ColorFloat",
                "PicaCommand", "AnimationGroupMember", "GraphicsAnimationGroup"}   # ... except these follow other counts
# slot values that are indexes or counts, not settings: (class, slot) -> why
INDEX_SLOTS = {
    ("AnimationGroupMember", "9"): "index of the animated bone / material",
    ("SOBJMesh", "5"): "index of the mesh's shape",
    ("SOBJMesh", "6"): "index of the mesh's material",
    ("FloatSegment", "3"): "number of keys in the curve (limited below)",
}
MAX_KEYS = 64
# bone flag bits that only say what a bone's own transform happens to be (identity, zero translation, ...): 1 2 4 8 16
BONE_TRANSFORM_BITS = 1 | 2 | 4 | 8 | 16
# width / height slots: any power of two from 8 to 256 (Platinum's banner has 32 to 256; the format slot must still match)
DIMENSION_SLOTS = {("ImageTexture", "5"), ("ImageTexture", "6"), ("PixelBasedImage", "0"), ("PixelBasedImage", "1")}


def _pot(n):
    return 8 <= n <= 256 and n & (n - 1) == 0


def check(cand, ref=None, ref_path=REFERENCE):
    """Differences between a candidate fingerprint and the Platinum reference. Returns (problems, notes)."""
    if ref is None:
        with open(ref_path, encoding="utf-8") as f:
            ref = json.load(f)
    problems, notes = [], []
    rc, cc = ref["container"], cand["container"]
    for key in ("signature", "bom", "header_size", "version", "blocks", "data_signature", "has_imag"):
        if rc[key] != cc[key]:
            problems.append(f"container {key}: {cc[key]!r} (Platinum {rc[key]!r})")
    for name, n in cc["dict_counts"].items():
        r = rc["dict_counts"].get(name, 0)
        if n and not r:
            problems.append(f"container has {n} {name} but the Platinum banner has none")
        elif n > r * COUNT_SLACK + 2:
            problems.append(f"container has {n} {name}, Platinum {r}")
    for cls, crec in cand["classes"].items():
        rrec = ref["classes"].get(cls)
        if rrec is None:
            problems.append(f"object class {cls} x{crec['count']} is not in the Platinum banner")
            continue
        if cls not in COUNT_EXEMPT and crec["count"] > rrec["count"] * COUNT_SLACK + 2:
            problems.append(f"{cls}: {crec['count']} objects, Platinum has {rrec['count']}")
        if crec.get("data_bytes", 0) > rrec.get("data_bytes", 0) * COUNT_SLACK + 64 and cls != "StepLinear64Key":
            problems.append(f"{cls}: {crec['data_bytes']} data bytes, Platinum has {rrec.get('data_bytes', 0)}")
        if cls in IGNORE_SLOTS:
            continue
        for slot, vals in crec["slots"].items():
            rvals = rrec["slots"].get(slot)
            if rvals is None:
                problems.append(f"{cls} slot {slot}: not in the Platinum banner's record")
                continue
            if (cls, slot) in DIMENSION_SLOTS:
                bad = [v for v in vals if not (v.isdigit() and _pot(int(v)))]
                if bad:
                    problems.append(f"{cls} slot {slot}: size(s) {bad} are not a power of two from 8 to 256")
                continue
            if (cls, slot) in INDEX_SLOTS:
                if (cls, slot) == ("FloatSegment", "3"):
                    big = [v for v in vals if v.lstrip("-").isdigit() and int(v) > MAX_KEYS]
                    if big:
                        problems.append(f"{cls} slot {slot}: curve with {big} keys (limit {MAX_KEYS})")
                continue
            if len(rvals) > MAX_DISTINCT:
                continue                      # data, not a setting
            if cls == "Bone" and slot == "1":
                allowed = {int(v) & ~BONE_TRANSFORM_BITS for v in rvals if v.isdigit()}
                new = sorted(v for v in vals if v.isdigit() and int(v) & ~BONE_TRANSFORM_BITS not in allowed)
            else:
                new = sorted(set(vals) - set(rvals))
            if new:
                problems.append(f"{cls} slot {slot}: value(s) {new} (Platinum uses only {sorted(rvals)})")
    for cls in ref["classes"]:
        if cls not in cand["classes"] and cls not in IGNORE_SLOTS:
            notes.append(f"Platinum has {cls} x{ref['classes'][cls]['count']}, this banner has none")
    return problems, notes


def main():
    cand = json.load(open(sys.argv[1], encoding="utf-8"))
    problems, notes = check(cand, ref_path=sys.argv[2] if len(sys.argv) > 2 else REFERENCE)
    for n in notes:
        print("note:", n)
    for p in problems:
        print("MISMATCH:", p)
    print("OK" if not problems else f"{len(problems)} mismatch(es)")
    sys.exit(1 if problems else 0)


if __name__ == "__main__":
    main()
