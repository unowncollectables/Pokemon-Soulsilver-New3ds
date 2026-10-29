# Plan: 3D and 2D composed at the final screen size (2026-10-29)

PLANNING ONLY. Nothing here is built or started. Waits for Nathan's "start".

## Goal
In normal full-screen mode (1.25x, 320x240, or 400x240 wide) the 3D layer is rendered at the screen's own pixels and
the 2D is laid around it, so the picture is as crisp as 1:1 mode. Today the 3D is drawn at 256x192 (320x256 wide),
composed with the 2D into one 256x192 picture, then stretched 1.25x by gpuscale.c (2D gets a 2x nearest prescale
first, so 2D is fine; 3D is the soft part).

## What exists (read: gpuscale.c, g3gpu.c, gpucomp.c, FPS60_STEP2_DESIGN.md, Gabe's REMASTER.md)
- Gabe (Platinum 3DS): 3D drawn straight into the screen-sized target; 2D split into "behind 3D" and "in front of 3D"
  layers with per-pixel alpha, published as textures (2x2 texels per DS pixel + bilinear); one GPU pass per screen:
  behind, rear plane, polygons, front, brightness. About 24 MB linear RAM. Works because his 2D renderer is his own.
- SoulSilver: 3D and 2D are composed by the DS-engine emulation (gpucomp.c for a "simple subset" of frames, otherwise
  the CPU path) into ONE 256x192 picture with the 3D baked in. There is no behind/in-front split, so Gabe's layer
  model cannot be copied directly.
- res3d (2x 3D + hardware 2x2 downscale) already exists and ships as Settings > 3D > SHARP 3D. It improves edges but
  still ends in a 256x192 picture that is then stretched 1.25x, so the 1.25x softness stays.

## Design: "mask" approach (recommended over a full layer split)
Keep today's composite exactly as the base. Add:
1. 3D drawn at final size (320x240, or 400x240 wide; 240-wide by 320/400 tall in the rotated framebuffer) into a VRAM
   target that gpuscale can sample as a texture. No readback, no CPU pass.
2. A 1-bit "this DS pixel is plain 3D (nothing 2D over it, no blend)" mask, produced where the composite is made.
3. In gpuscale's final quad: draw the 2x-prescaled composite as now, then draw the hi-res 3D texture over it only
   where the mask is set (mask sampled 2x nearest, so the cut-out is DS-pixel exact). Text boxes, sprites, menus over
   the 3D keep their 2D look; open 3D (most of the overworld and battle background) is crisp.
Pixels where 2D is blended over 3D, fades and brightness stay on the old soft path (still correct, just not crisp).
Why not Gabe's full split: it means rewriting how every DS 2D effect over BG0 is composed (alpha blend, windows,
fades, mosaic) in a layer we do not own. Much bigger and riskier; mask gets ~90% of the visible win.

## Stages (each ends on hardware; all behind one settings toggle)
S0 (cheap, do first): add an SD screenshot dump of the final top framebuffer (raw to PNG-ish file on a hotkey).
   Lets me judge the real picture from files instead of photos, since Azahar cannot draw the 2x picture correctly.
S1 3D at final size, mono, no mask yet: title and field, mask = whole 3D rectangle only when nothing is on top.
   Log: [CRISP] target size, VRAM free, frame time. Test: Is the grass/trees/building crisp, any stripes/garbage.
S2 The mask from gpucomp (simple-subset frames) plus the CPU path flag; text boxes/menus over 3D look right.
S3 Stereo (slider up, one target shared by both eyes) and wide view (400 wide; side strips drawn at final size too).
S4 Fog frames, fades, battle, cutscenes, Union Room; fallbacks per frame to today's path; make default if good.
Toggle per TOGGLE RULE: Settings (L+R+Y) > 3D > "CRISP 3D" (default off until tested), removed once tested.

## Memory budget
- Linear RAM: stays near today's. Only new linear item is the mask, 256x192 at 8 bit = 48 KB. About 18 MB free was
  measured on run 48; Gabe's design needed 24 MB, which is why we do not copy it whole.
- VRAM (6 MB total, gpuscale holds about 3 MB; scale targets 1 MB each at RGBA8, half as RGB565): target 512x256 colour
  (as texture) 0.5 MB + 24-bit depth/stencil 0.5 MB = 1 MB for ONE eye target shared between the eyes (draw L, composite
  L, clear, draw R, composite R). Wide: 512x256 still fits (400 wide). That is less than the 2x res3d target
  (about 1.9 MB wide), so this should fit where SHARP 3D already does; SHARP 3D and CRISP 3D are not meant to run together.
- LESSON kept: render targets (colour AND depth) live in VRAM only, never linear RAM (hardware run 49: garbage, 14 fps).
  Depth16 gave stripes; depth stays 24-bit, no 16-bit fallback (if 24 does not fit: stay at 1x and toast).

## Risks, and what failed before
- res3d garbage in field (run 48/49): depth/targets in linear RAM. Fixed by VRAM-only targets (#172, #175, #178, #180).
- Horizontal stripes on ground/battle bg at 2x (run 51/52): fixed by #192 (per Nathan's coordinator note; confirm the
  exact cause in the log before reusing that path, the earlier guess in #185 did not help).
- Azahar draws the 2x 3D shrunk into a corner (likely the SCALE_XY transfer). This plan avoids the HW scaled transfer
  entirely (3D is rendered at final size and sampled), so Azahar may actually be usable for S1 layout checks; do not
  count on it, hardware is the judge (hence S0).
- Fog bit is set often in HGSS overworld; SHARP 3D drops to 1x on those frames. Plan: fog is applied by the GPU fog
  unit at the final size from the start, so fog frames stay crisp. Needs checking in S1, not assumed.
- Mask edges where 3D meets sprites/HUD may show a one-pixel soft seam. Accepted risk; S2 tunes it.
- Frame time: 3D at 320x240 is 1.56x the pixels of 256x192, far less than res3d's 4x. Overworld is already about 28 ms
  per frame (30 fps goal is solid); must not go over. If stereo cost is too high, S3 falls back to 1x per frame.

## Shared with the later 60 fps work
About 60%. The same pieces serve both: compose output stays on the GPU as a texture (no readback, no CPU byte swap,
FPS60_STEP2_DESIGN steps 1 and 2), per-eye ring of targets, per-frame fallback to the old path. CRISP 3D S1 and S3 are
effectively FPS60 step 2 with a bigger target. Doing crisp first makes 60 fps cheaper later. The in-between-frame and
threading ideas are separate.

## Cost, in hardware test rounds for Nathan
S0+S1 one build, one round. S2 one round. S3 one round. S4 one to two rounds. Total 4 to 6 rounds, each a normal
"run NN" build; Nathan only needs a field walk, one battle, a text box, slider up once (a few minutes each).
Cloud work per round is moderate (gpuscale/g3gpu/gpucomp are about 1,500 lines in total; no patch to the pspoke tree
expected, except possibly the CPU-path mask flag, which would be a NEW numbered patch).

## Recommendation
Do it, as the mask approach, S0 first. It fixes the one remaining visible softness at the lowest risk, the work is
reusable for 60 fps, and nothing touches saves. Skip it if Nathan would rather spend the credits on Union Room
battle/other features; the game is already fine at 1:1 and with SHARP 3D. Suggest starting S0+S1 only and deciding on
S2 to S4 after seeing S1 on hardware.
