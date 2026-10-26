"""The banner's glTF scene to CGFX, with skyfloogle/pycgfx (fetched at build time
at a pinned commit, never vendored: it has no licence that allows sharing it)
and the fixes it needs to run on a real HOME Menu, applied here at run time so
pycgfx's own files stay untouched.

Fixes found on hardware by daaaaavia/CBC-for-YANBF (MIT licence):
  1. every mesh records its node's bone (mesh_node_name): some banners crash
     without it;
  2. the bone lookup tree is rebuilt after pycgfx moves the bones of BLEND
     materials to the end (otherwise looking up a bone finds the wrong one and
     the HOME Menu crashes);
  3. BLEND materials are marked translucent (pycgfx leaves them "opaque").
Ours, also from hardware tests:
  4. materials are unlit (texture x material colour), whatever the HOME Menu's
     lights do;
  5. the portal, sparkle and logo bones are screen-facing billboards (as Pokemon Y's
     logo is) and "autorotate" (Lugia's bone, when there is a Lugia) turns -360 degrees over the animation's
     10 seconds against the HOME Menu's own spin (as Pokemon Y's y_autorotate);
  6. each curve loops over its own keys (Lugia's 1-second wing beat) inside the
     10-second animation.
pycgfx and its two libraries are swapped for standard-library stand-ins
(shims/: Pillow and gltflib, just the parts pycgfx calls).
"""
import importlib.util
import math
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
PYCGFX_COMMIT = "1f78850086f3a77c41e07162e842f97a5bf3c18a"
CGFX_LIMIT = 0x80000          # the HOME Menu's limit for a banner's CGFX
# Conservative limits, from what is known to run on Nathan's New 3DS XL HOME Menu (the Platinum port's banner: 349 KB,
# 2 textures, 1 model of ~7-joint parts; the largest scene the Platinum port ever got to run was ~431 KB). The Lugia banner
# (483,788 bytes) froze the HOME Menu when the cursor reached the icon, so anything past these fails and the build falls
# back to the flat banner.
SAFE_CGFX_BYTES = 400_000
SAFE_MESHES = 12
SAFE_TEXTURES = 12
SAFE_TEXTURE_SIDE = 256       # power of two, at most this
SAFE_BONES = 40
SAFE_VERTS = 6000
SAFE_TRIS = 6000
SAFE_BLEND = 8


class BannerLimitError(ValueError):
    """The banner is over a limit known (or suspected) to hang the HOME Menu: use the flat banner."""


def banner_stats(gltf, scene, cmdl, raw):
    """Numbers worth comparing with the Platinum port's banner (printed by every 3D banner build)."""
    model = gltf.model
    verts = tris = 0
    for mesh in model.meshes:
        for prim in mesh.primitives:
            verts += model.accessors[prim.attributes.POSITION].count
            tris += (model.accessors[prim.indices].count // 3) if prim.indices is not None else 0
    tex = scene.data.textures
    return {
        "cgfx_bytes": len(raw), "meshes": len(cmdl.meshes.data.contents), "materials": len(list(cmdl.materials)),
        "blend_materials": sum(1 for m in (model.materials or []) if m.alphaMode == "BLEND"),
        "bones": len(cmdl.skeleton.bones.dict.nodes) - 1, "vertices": verts, "triangles": tris,
        "textures": [(n, tex[n].width, tex[n].height) for n in tex],
        "animations": len(list(scene.data.skeletal_animations)),
    }


def check_limits(st):
    bad = []
    def over(what, have, limit):
        if have > limit:
            bad.append(f"{what} {have} > safe limit {limit}")
    over("CGFX bytes", st["cgfx_bytes"], SAFE_CGFX_BYTES)
    over("meshes", st["meshes"], SAFE_MESHES)
    over("textures", len(st["textures"]), SAFE_TEXTURES)
    over("bones", st["bones"], SAFE_BONES)
    over("vertices", st["vertices"], SAFE_VERTS)
    over("triangles", st["triangles"], SAFE_TRIS)
    over("translucent materials", st["blend_materials"], SAFE_BLEND)
    for name, w, h in st["textures"]:
        if w & (w - 1) or h & (h - 1) or max(w, h) > SAFE_TEXTURE_SIDE:
            bad.append(f"texture {name} is {w}x{h} (must be power of two, at most {SAFE_TEXTURE_SIDE})")
    return bad
BILLBOARDS = ("portal", "logo", "sparkle0", "sparkle1", "sparkle2", "sparkle3")


def load_pycgfx(pycgfx_dir):
    shims = os.path.join(HERE, "shims")
    for p in (pycgfx_dir, shims):
        if p not in sys.path:
            sys.path.insert(0, p)
    spec = importlib.util.spec_from_file_location("pycgfx_main", os.path.join(pycgfx_dir, "main.py"))
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def convert(pycgfx_dir, gltf_path):
    pycgfx = load_pycgfx(pycgfx_dir)
    import gltflib
    from cgfx.canm import (FloatAnimationCurve, FloatSegment, InterpolationType, QuantizationType,
                           RepeatMethod, StepLinear64Key)
    from cgfx.mtob import ConstantColorSource, MTOBFlag
    from cgfx.sobj import BillboardMode

    gltf = gltflib.GLTF.load(gltf_path, load_file_resources=True)
    scene = pycgfx.convert_gltf(gltf)
    cmdl = scene.data.models["COMMON"]
    bones = cmdl.skeleton.bones

    # 1. each mesh's node bone (our meshes are named and used by one node each)
    mesh_node = {gltf.model.meshes[n.mesh].name: n.name for n in gltf.model.nodes if n.mesh is not None}
    for sobj_mesh in cmdl.meshes.data.contents:
        mesh_name = next(m for m in mesh_node if sobj_mesh.name.startswith(m + "_"))
        sobj_mesh.mesh_node_name = bones[mesh_node[mesh_name]]
    # 2. the bone lookup tree
    bones.dict.regenerate()
    # 3. translucent BLEND materials, 4. unlit materials
    blend = {m.name for m in (gltf.model.materials or []) if m.alphaMode == "BLEND"}
    for name in cmdl.materials:
        material = cmdl.materials[name]
        if name in blend:
            material.transluscency_kind = 1
        material.flags = MTOBFlag(0)
        stage = material.fragment_shader.texture_combiners[0]
        stage.constant = ConstantColorSource.Diffuse
        stage.src_rgb = stage.src_alpha = 0x0E3
        stage.combine_rgb = stage.combine_alpha = 1
        for stage in material.fragment_shader.texture_combiners[1:]:
            stage.src_rgb = stage.src_alpha = 0x00F
            stage.combine_rgb = stage.combine_alpha = 0
    if list(scene.data.material_animations):
        raise ValueError("material animations are not safe in a HOME Menu banner")
    # 5. billboards
    for n in bones.dict.nodes[1:]:
        if n.content.name in BILLBOARDS:
            n.content.billboard_mode = BillboardMode.Screen
    # 5./6. the animation
    for name in scene.data.skeletal_animations:
        animation = scene.data.skeletal_animations[name]
        animation.looping = True
        for member in animation.member_animations_data:
            bone = animation.member_animations_data[member]
            for curve in vars(bone).values():
                if hasattr(curve, "segments") and curve.segments:
                    curve.start_frame = min(sg.start_frame for sg in curve.segments)
                    curve.end_frame = max(sg.end_frame for sg in curve.segments)
                    curve.pre_repeat_method = curve.post_repeat_method = RepeatMethod.Repeat
            if member == "autorotate":
                frames = float(animation.frame_size)
                bone.rot_x = bone.rot_z = None
                bone.scale_x = bone.scale_y = bone.scale_z = None
                bone.pos_x = bone.pos_y = bone.pos_z = None
                curve, seg = FloatAnimationCurve(), FloatSegment()
                curve.start_frame, curve.end_frame = 0.0, frames
                curve.pre_repeat_method = curve.post_repeat_method = RepeatMethod.Repeat
                seg.start_frame, seg.end_frame = 0.0, frames
                seg.interpolation, seg.quantization = InterpolationType.Linear, QuantizationType.StepLinear64
                seg.keys = [StepLinear64Key(0.0, 0.0), StepLinear64Key(frames, 2 * math.pi)]
                curve.segments.append(seg)
                bone.rot_y = curve
    raw = pycgfx.write(scene)
    if len(raw) > CGFX_LIMIT:
        raise BannerLimitError(f"the banner's CGFX is {len(raw)} bytes, over the HOME Menu's {CGFX_LIMIT}")
    convert.stats = banner_stats(gltf, scene, cmdl, raw)
    print("[BANNER] stats:", convert.stats)
    bad = check_limits(convert.stats)
    if bad:
        raise BannerLimitError("; ".join(bad))
    return raw
