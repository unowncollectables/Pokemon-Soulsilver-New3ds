"""The SoulSilver HOME Menu banner as a glTF scene: a silver/gold portal glow at the back, a few drifting
sparkles, and the SoulSilver logo in front, all this repo's own art and all screen-facing billboards at the root (so
the HOME Menu's spin never turns or moves them), after Pokemon Y's layout. With `lugia` (the title screen's Lugia
model and wing animation, read from the player's own ROM at build time, never committed) Lugia flies in the middle.

    scene.build(art_dir, out_dir, sparkles=True, lugia=None)    # lugia = (nsbmd bytes, nsbca bytes) or None

The HOME Menu spins the whole model one turn every 10 s. Like Pokemon Y's "y_autorotate" bone, Lugia sits under a
bone that turns the other way at the same rate (to_cgfx.py writes its exact curve) with its pivot on the spin axis,
so a spin turns it in place. Lugia and the sparkles are kept inside the band the HOME Menu draws (about y 40 to 212).

Coordinates are in the HOME Menu banner camera's space: camera at (0, 1, 44.786) looking down -Z, 30 degree vertical
field of view (pycgfx's banner-camera.gltf). Only the part between the HOME Menu's bars (about y 40 to 212 of the
240-pixel screen) is drawn; the logo is drawn 1 texel = 1 screen pixel (224 px wide). Standard library only.
"""
import json
import math
import os
import struct

import decimate
import linalg
import lugia_parts
import pngio

FPS = 30
LOOP_SECONDS = 4.0
PORTAL = ("portal.png", 65.0, (0.0, 0.44, -30.0))     # picture, width, centre
LOGO = ("logo.png", 14.9, (0.0, -2.46, 15.0))         # 224 px wide at z = 15

LUGIA_KEEP = ("lugia8:lambert9", "lugia8:lug_arm", "lugia8:lug_eye")   # the title model's Lugia; the rest is sea and sky
MAX_JOINTS = 7                # joints per part, as in Pokemon Y's banner
LUGIA_WIDTH = 20.0            # model width in banner units (wings spread)
LUGIA_POS = (0.0, 3.2, 0.0)   # its pivot: on the spin axis (x = z = 0)
PITCH, YAW = 20.0, -70.0      # degrees: seen a little from above, flying to the viewer's left
FLAP_FRAMES = 30              # the title animation is one wing beat per 30 frames (1 s), repeated; the last frame = the first
AUTOROTATE_SECONDS = 10.0     # the HOME Menu's spin: one turn per 10 s
HOVER_UNITS = 0.7             # the gentle bob (banner units), as the Platinum banner's Giratina has; its own 4 s loop
HOVER_SECONDS = 4.0
PLATINUM_TRIS = 1000          # the Platinum banner (runs on the HOME Menu) has 1000 triangles in all, portal and logo cards included; the title Lugia has ~2040
CARD_TRIS = 4                 # portal + logo


def build(art_dir, out_dir, sparkles=False, lugia=None, max_tris=PLATINUM_TRIS):
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

    def add_lugia(nsbmd, nsbca):
        """Lugia's skinned parts under the spin-cancelling bone; returns what the animation needs."""
        model, inst, tris, frames, images = lugia_parts.build(nsbmd, nsbca, LUGIA_KEEP)
        budget = max_tris - CARD_TRIS - (8 if sparkles else 0)
        if max_tris and len(tris) > budget:
            before = len(tris)
            tris, got = decimate.decimate(tris, budget)
            print(f"[BANNER] Lugia triangles {before} -> {got} (budget {budget} + {max_tris - budget} for the cards = {max_tris}, the Platinum banner's total)")
        mats = model["materials"]
        face = node(0, name="autorotate")
        world = lambda v: linalg.point(inst[v[0]], v[1])
        # size and centre from the beat's poses (the bind pose has the wings spread flat), not the bind pose
        pts = [linalg.point(frames[k][v[0]], v[1]) for k in range(0, FLAP_FRAMES, 5) for _, t in tris for v in t]
        lo = [min(p[k] for p in pts) for k in range(3)]
        hi = [max(p[k] for p in pts) for k in range(3)]
        scale = LUGIA_WIDTH / (hi[0] - lo[0])
        pitch, yaw = math.radians(PITCH), math.radians(YAW)
        q_turn = linalg.quat_mul([math.sin(pitch / 2), 0, 0, math.cos(pitch / 2)], [0, math.sin(yaw / 2), 0, math.cos(yaw / 2)])
        # pivot = the centre of what is seen after the turn
        rt = linalg.quat_matrix(q_turn)
        rp = [linalg.apply(rt, p) for p in pts]
        c = [(min(p[k] for p in rp) + max(p[k] for p in rp)) / 2 for k in range(3)]
        mid = linalg.apply(linalg.transpose(rt), c)
        root = node(face, name="LugiaRoot", translation=list(LUGIA_POS), rotation=q_turn, scale=[scale] * 3)
        center = node(root, name="LugiaCenter", translation=[-mid[0], -mid[1], -mid[2]])
        hover = node(center, name="LugiaHover")

        gtex, gmat = {}, {}
        for m in mats:
            if m["name"] in LUGIA_KEEP and m["texture"] and m["texture"] not in gtex:
                w, h, rgba = images[m["texture"]]
                opaque = all(rgba[i] == 255 for i in range(3, len(rgba), 4))
                gtex[m["texture"]] = (texture(w, h, rgba, "lugia_" + m["texture"], clamp=False), opaque)
        for i, m in enumerate(mats):
            if m["name"] in LUGIA_KEEP:
                t, opaque = gtex[m["texture"]]
                gmat[i] = material("lugia_" + m["name"].replace(":", "_"), t, "OPAQUE" if opaque else "MASK")

        # joints: one node per matrix instance, posed in model space
        jnodes = []
        for i, M in enumerate(inst):
            t, q, sc = linalg.decompose(M)
            jnodes.append(node(hover, name=f"lj{i}", translation=t, rotation=q, scale=sc))
        # parts using at most MAX_JOINTS joints each; every vertex follows exactly one joint
        groups = []
        for mat, t in sorted(tris, key=lambda x: sorted({v[0] for v in x[1]})):
            js = {v[0] for v in t}
            for g in groups:
                if len(g[0] | js) <= MAX_JOINTS:
                    g[0] |= js
                    g[1].setdefault(mat, []).append(t)
                    break
            else:
                groups.append([set(js), {mat: [t]}])
        for gi, (js, bymat) in enumerate(groups):
            joints = sorted(js)
            jidx = {j: k for k, j in enumerate(joints)}
            prims = []
            for mat, ts in bymat.items():
                P, N, UV, J, Wt, IDX, seen = [], [], [], [], [], [], {}
                for t in ts:
                    for v in t:
                        w = world(v)
                        n = linalg.normalize(linalg.apply(linalg.upper3(inst[v[0]]), v[4]))
                        uv = (v[2][0], v[2][1])
                        key = (v[0], tuple(round(x, 4) for x in w), tuple(round(x, 5) for x in uv), tuple(round(x, 3) for x in n))
                        if key not in seen:
                            seen[key] = len(seen)
                            P += w; N += n; UV += [uv[0], uv[1]]
                            J += [jidx[v[0]], 0, 0, 0]; Wt += [1.0, 0.0, 0.0, 0.0]
                        IDX.append(seen[key])
                prims.append({"attributes": {"POSITION": acc(P, "VEC3"), "NORMAL": acc(N, "VEC3"), "TEXCOORD_0": acc(UV, "VEC2"),
                                             "JOINTS_0": acc(J, "VEC4", 5121, False), "WEIGHTS_0": acc(Wt, "VEC4", 5126, False)},
                              "indices": acc(IDX, "SCALAR", 5123), "material": gmat[mat]})
            ibm = []
            for j in joints:
                inv = linalg.inverse(inst[j])
                ibm += [inv[r][c] for c in range(4) for r in range(4)]        # column-major
            gl["skins"].append({"name": f"part{gi}", "joints": [jnodes[j] for j in joints], "inverseBindMatrices": acc(ibm, "MAT4", minmax=False)})
            gl["meshes"].append({"name": f"lpart{gi}", "primitives": prims})
            node(hover, name=f"lpart{gi}", mesh=len(gl["meshes"]) - 1, skin=len(gl["skins"]) - 1)
        return face, hover, jnodes, frames, scale

    # ---- middle: Lugia (from the player's ROM), the sparkles, then the logo in front ----
    lug = None
    if lugia:
        lug = add_lugia(*lugia)
    sparks = []
    if sparkles:
        # (x, y, z, size): around the portal, outside the logo, inside the visible band
        for i, (x, y, z, w) in enumerate(((-22.0, 5.5, -8.0, 2.6), (21.0, 7.5, -12.0, 3.2), (-9.0, 8.5, -16.0, 2.0), (12.0, -6.0, -6.0, 2.2))):
            sparks.append((card(f"sparkle{i}", "sparkle.png", w, (x, y, z), "BLEND"), y, i))
    card("logo", LOGO[0], LOGO[1], LOGO[2], "BLEND")

    samplers, channels = [], []

    def animate(nd, path, values, kind, t_acc):
        samplers.append({"input": t_acc, "output": acc(values, kind, minmax=False), "interpolation": "LINEAR"})
        channels.append({"sampler": len(samplers) - 1, "target": {"node": nd, "path": path}})

    if lug:
        face, hover, jnodes, frames, scale = lug
        frames = frames[:FLAP_FRAMES] + [frames[0]]            # one beat, closed so it loops without a jump
        keys = list(range(len(frames)))
        t_acc = acc([k / FPS for k in keys], "SCALAR")
        for i, jn in enumerate(jnodes):
            trs = [linalg.decompose(frames[k][i]) for k in keys]
            first = trs[0]
            if all(linalg.close(x[0] + x[1] + x[2], first[0] + first[1] + first[2], atol=1e-4) for x in trs):
                continue                                       # this joint never moves
            if any(not linalg.close(x[0], first[0], atol=1e-3) for x in trs):
                animate(jn, "translation", [c for x in trs for c in x[0]], "VEC3", t_acc)
            if any(not linalg.close(x[1], first[1], atol=1e-5) for x in trs):
                animate(jn, "rotation", [c for x in trs for c in x[1]], "VEC4", t_acc)
            if any(not linalg.close(x[2], first[2], atol=1e-4) for x in trs):
                animate(jn, "scale", [c for x in trs for c in x[2]], "VEC3", t_acc)
        # the title screen's gentle bob, on its own 4 s loop like the Platinum banner's Giratina (25 keys)
        n = 24
        th = acc([HOVER_SECONDS * i / n for i in range(n + 1)], "SCALAR")
        amp = HOVER_UNITS / scale
        animate(hover, "translation", [c for i in range(n + 1) for c in (0.0, amp * math.sin(2 * math.pi * i / n), 0.0)], "VEC3", th)
        # placeholder: to_cgfx.py writes the real counter-spin curve over AUTOROTATE_SECONDS
        animate(face, "rotation", [0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0], "VEC4", acc([0.0, AUTOROTATE_SECONDS], "SCALAR"))
    if sparks:
        n = 12
        loop = AUTOROTATE_SECONDS / 2 if lug else LOOP_SECONDS     # a whole number of loops per spin
        t_acc = acc([loop * k / n for k in range(n + 1)], "SCALAR")
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
