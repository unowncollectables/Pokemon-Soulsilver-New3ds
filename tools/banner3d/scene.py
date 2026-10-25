"""The SoulSilver HOME Menu banner as a glTF scene, built only from this repo's own art (no game assets):
a silver/gold portal glow at the back, a few drifting sparkles, and the SoulSilver logo in front, all
screen-facing billboards at the root (so the HOME Menu's spin never turns or moves them), after Pokemon Y's layout.

    scene.build(art_dir, out_dir, sparkles=True)

Coordinates are in the HOME Menu banner camera's space: camera at (0, 1, 44.786) looking down -Z, 30 degree vertical
field of view (pycgfx's banner-camera.gltf). Only the part between the HOME Menu's bars (about y 40 to 212 of the
240-pixel screen) is drawn; the logo is drawn 1 texel = 1 screen pixel (224 px wide). Standard library only.
"""
import json
import math
import os
import struct

import pngio

FPS = 30
LOOP_SECONDS = 4.0
PORTAL = ("portal.png", 65.0, (0.0, 0.44, -30.0))     # picture, width, centre
LOGO = ("logo.png", 14.9, (0.0, -2.46, 15.0))         # 224 px wide at z = 15


def build(art_dir, out_dir, sparkles=True):
    os.makedirs(out_dir, exist_ok=True)
    blob, views, accs = bytearray(), [], []

    def acc(vals, kind, comp=5126, minmax=True):
        n = {"SCALAR": 1, "VEC2": 2, "VEC3": 3, "VEC4": 4, "MAT4": 16}[kind]
        while len(blob) % 4:
            blob.append(0)
        data = struct.pack("<%d%s" % (len(vals), {5126: "f", 5123: "H", 5121: "B"}[comp]), *vals)
        views.append({"buffer": 0, "byteOffset": len(blob), "byteLength": len(data)})
        blob.extend(data)
        a = {"bufferView": len(views) - 1, "componentType": comp, "count": len(vals) // n, "type": kind}
        if minmax and kind in ("VEC3", "SCALAR"):
            a["min"] = [float(min(vals[i::n])) for i in range(n)]
            a["max"] = [float(max(vals[i::n])) for i in range(n)]
        accs.append(a)
        return len(accs) - 1

    gl = {"asset": {"version": "2.0", "generator": "pokemon-soulsilver-new3ds banner"}, "scene": 0, "scenes": [{"nodes": [0]}],
          "nodes": [{"name": "COMMON", "children": []}], "meshes": [], "materials": [], "textures": [], "images": [],
          "samplers": [{"magFilter": 9729, "minFilter": 9729, "wrapS": 10497, "wrapT": 10497},
                       {"magFilter": 9729, "minFilter": 9729, "wrapS": 33071, "wrapT": 33071}], "skins": []}

    def node(parent, **kw):
        gl["nodes"].append(kw)
        i = len(gl["nodes"]) - 1
        gl["nodes"][parent].setdefault("children", []).append(i)
        return i

    def texture(w, h, rgba, name, clamp=True):
        pngio.write(os.path.join(out_dir, f"{name}.png"), w, h, rgba)
        gl["images"].append({"uri": f"{name}.png", "name": name})
        gl["textures"].append({"sampler": 1 if clamp else 0, "source": len(gl["images"]) - 1})
        return len(gl["textures"]) - 1

    def material(name, tex, mode):
        m = {"name": name, "alphaMode": mode, "pbrMetallicRoughness": {"baseColorFactor": [1, 1, 1, 1], "metallicFactor": 0, "roughnessFactor": 1}}
        if tex is not None:
            m["pbrMetallicRoughness"]["baseColorTexture"] = {"index": tex}
        if mode == "MASK":
            m["alphaCutoff"] = 0.5
        gl["materials"].append(m)
        return len(gl["materials"]) - 1

    def pow2(n):
        return 1 << max(3, math.ceil(math.log2(n)))

    def card(name, picture, width, center, mode):
        w, h, rgba = pngio.read(os.path.join(art_dir, picture))
        tw, th = pow2(w), pow2(h)
        canvas = bytearray(tw * th * 4)
        for y in range(h):
            canvas[y * tw * 4:y * tw * 4 + w * 4] = rgba[y * w * 4:(y + 1) * w * 4]
        mat = material(name, texture(tw, th, canvas, name), mode)
        height = width * h / w
        cx, cy, cz = center
        x0, x1, y0, y1 = cx - width / 2, cx + width / 2, cy - height / 2, cy + height / 2
        u1, v1 = w / tw, h / th
        gl["meshes"].append({"name": name, "primitives": [{"attributes": {
            "POSITION": acc([x0, y0, cz, x1, y0, cz, x1, y1, cz, x0, y1, cz], "VEC3"),
            "NORMAL": acc([0, 0, 1] * 4, "VEC3"), "TEXCOORD_0": acc([0, v1, u1, v1, u1, 0, 0, 0], "VEC2")},
            "indices": acc([0, 1, 2, 0, 2, 3], "SCALAR", 5123), "material": mat}]})
        return node(0, name=name, mesh=len(gl["meshes"]) - 1)

    # ---- back: the portal glow ----
    card("portal", PORTAL[0], PORTAL[1], PORTAL[2], "BLEND")

    # ---- middle: sparkles, then the logo in front ----
    sparks = []
    if sparkles:
        # (x, y, z, size): around the portal, outside the logo, inside the visible band
        for i, (x, y, z, w) in enumerate(((-22.0, 5.5, -8.0, 2.6), (21.0, 7.5, -12.0, 3.2), (-9.0, 8.5, -16.0, 2.0), (12.0, -6.0, -6.0, 2.2))):
            sparks.append((card(f"sparkle{i}", "sparkle.png", w, (x, y, z), "BLEND"), y, i))
    card("logo", LOGO[0], LOGO[1], LOGO[2], "BLEND")

    samplers, channels = [], []
    if sparks:
        n = 12
        t_acc = acc([LOOP_SECONDS * k / n for k in range(n + 1)], "SCALAR")
        for nd, y, i in sparks:
            vals = []
            for k in range(n + 1):
                vals += [0.0, 1.2 * math.sin(2 * math.pi * k / n + i * 1.7), 0.0]
            samplers.append({"input": t_acc, "output": acc(vals, "VEC3", minmax=False), "interpolation": "LINEAR"})
            channels.append({"sampler": len(samplers) - 1, "target": {"node": nd, "path": "translation"}})
            gl["nodes"][nd]["translation"] = [0.0, 0.0, 0.0]
    if channels:
        gl["animations"] = [{"name": "COMMON", "samplers": samplers, "channels": channels}]
    gl["bufferViews"], gl["accessors"] = views, accs
    gl["buffers"] = [{"uri": "soulsilver-banner.bin", "byteLength": len(blob)}]
    with open(os.path.join(out_dir, "soulsilver-banner.bin"), "wb") as f:
        f.write(blob)
    with open(os.path.join(out_dir, "soulsilver-banner.gltf"), "w", encoding="utf-8") as f:
        json.dump(gl, f)
    return os.path.join(out_dir, "soulsilver-banner.gltf")
