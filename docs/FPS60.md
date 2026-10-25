# 60 fps overworld: feasibility (2026-10-25)

Verdict: **not safe to implement blind** (no build, no device, no decomp work tree in the session that looked at it). This PR adds the measurement that decides it (`fps60_probe`, log only) and the plan. Nothing here changes behaviour.

## Why the field is 30 fps
`pret/pokeheartgold` `src/main.c` (pinned 0985e87): each loop runs one `Main_RunOverlayManager` tick, and if no VBlank has passed (`gSystem.frameCounter == 0`, bumped in the VBlank callback) it waits for one VBlank, then `Handle3DBufferSwapRequest()` and a second `OS_WaitIrq(VBLANK)`. A tick that finishes inside one VBlank therefore always takes two: 30 game ticks per second, one `G3_SwapBuffers` per tick (`fieldmap.c` `RequestSwap3DBuffers`). Walking, camera and animation all advance per tick. Running the logic at 60 would just make the game run twice as fast, so 60 fps has to be **in-between frames**: a frame shown one VBlank before each real one, with every moving polygon halfway between the previous and the new scene.

## How the Platinum repos do it
- `3ds-platinum`: no 60 fps field (its own notes only mention 60 fps menus).
- `pokeplatinum-3ds` (`3ds_port/nitro/src/port_gx3d.c`, `docs/REMASTER.md` "60 frames a second in the 3D"): keeps each polygon as a clip-space record, pairs the new scene's polygons with the previous scene's (draw order first, then nearest polygon of the same kind within 16 DS px), draws a scene of the halfway polygons, and shows it a VBlank before the real scene. Costs one frame of latency. Auto-steps down (2x2, 2x1, 30 fps) when a frame does not fit in 16.7 ms. Too few pairs (cut, new map) = no in-between frame. Slider-up numbers on a New 3DS: 9-11 ms per frame, so only part of the busy time fits.

## What it needs here
1. Hook where `g3soft` hands triangles to `g3gpu.c` (`G3G_TriClip`, per eye) and keep the previous frame's clip-space triangles (`ruV` in `g3gpu.c` already holds a copy for G3REUSE).
2. Pair triangles (index + UV signature like the probe below, with a lookahead for inserted/removed polygons, falling back to "where the new scene has it").
3. At Present: draw/readback/composite the in-between 3D layer, present, wait one VBlank, then do the real frame. The 2D layers (text boxes, weather, bottom screen) are reused from the new frame, so the HUD would run at 30.
4. Pacing: the game thread currently has two VBlanks per tick; the in-between frame must fit in the idle half. Needs a step-down path like Platinum's `auto_fps60`.
5. Toggle in the L+R+Y menu, default off, `[FPS]` log lines, off automatically in battle, title, intro and with Wide FX (already ~29 ms/frame, 15 fps) or GPU compose rejecting a frame.

## Cost estimate (from logs, not measured for this feature)
- 3D draw + readback + convert is the GPU path's main cost; the composite is 9-16 ms on the CPU path and much less when `[GPUC]` composes on the GPU. A second composite+present per tick must fit in the idle time. Wide FX alone is 29 ms/frame, so Wide FX and 60 fps cannot coexist.
- Latency +1 tick (33 ms), as in Platinum.
- Memory: a second triangle copy (~1 MB for 24k vertices, only when on).

## Risks
Mismatched pairs show as polygons flying across the screen for one frame (Platinum needed a motion limit and a "too few pairs" cutoff). Stereo needs both eyes. Any bug is in the render path every frame, so it cannot be tried without a device or Azahar loop. That is why this PR stops at measurement.

## What this PR adds: `fps60_probe=1` in config.txt (log only, default 0, no menu row since it changes nothing visible)
- `[FPS] field 10 s: ...` (main3ds.c, `OS_WaitIrq` wrap): busy vs idle ms in the overworld. If busy per 33 ms tick is well under ~20 ms, a second frame can fit.
- `[FPS] probe N frames: ...` (g3gpu.c, start of `G3G_End`): share of triangles whose same-index triangle in the previous frame has the same texture coords ("same kind"), how many of those moved under 16 / 4 px or not at all, mean move, and in how many frames the triangle count changed. High "same kind" (above ~90%) means Platinum's pairing will work; low means it needs the lookahead matcher.

## Next step
Nathan walks around a town for a minute with `fps60_probe=1`, sends `log.txt`. If busy is under ~60% and same-kind is high, build the interpolating frame (steps 1-5) as its own PR behind a menu toggle; otherwise the nearest options are (a) leave the field at 30, (b) 60 for menus only (already the case where GPU compose accepts the frame).
