# SoulSilver 3DS: more speed, and how the other Platinum port gets 60 fps

Written 2026-10-26. Based on the repos' own notes and code (the web had nothing useful). Nothing was compiled or run for this; the numbers are from Nathan's logs.

## Who is the "other guy"
**GabeSud0/pokeplatinum-3ds** (the `pokeplatinum-3ds` repo here is our copy of that design). It is a different kind of port from ours:
- Ours (SoulSilver and 3ds-platinum): pspoke, the decomp running with a DS-hardware emulation layer. The DS 2D engine and 3D layer are rebuilt in software and the 3D is drawn on the GPU, then **read back to the CPU, converted, uploaded again to the GPU compositor, composed, read back again**, then shown.
- His: decomp with a native port layer. Each 3D scene's polygons go straight to the PICA200 GPU and the 2D layers are uploaded as textures. **Everything is composed on the GPU straight into the screen framebuffers. No readback.** Frames are built while the GPU draws the previous one (two command lists and queues), the game, display and 2D threads are pinned to chosen cores, and a V-blank thread flips frames at exactly the right time.

## How he gets 60 fps (`docs/REMASTER.md`, "60 frames a second in the 3D")
The game itself still only advances 30 times a second (it waits two V-blanks per tick, same as HeartGold). His 60 is **interpolated**: he keeps every polygon of the last scene, pairs each polygon of the new scene with its partner (draw order first, then nearest polygon of the same kind within a few pixels), and draws a scene with every polygon halfway between, shown one V-blank before the real one. Costs one frame of latency. The 2D (text, HUD) stays at 30. It steps itself down (2x2, 2x1, then 30 fps) when a frame does not fit in 16.7 ms. His frame costs 9-11 ms on a New 3DS in the field, which is why it fits. It is off for title, intro, cutscenes.

## Can we do it?
Honest answer: **not as a quick change, and not until the readback is gone.**
- Pairing polygons is the easy part. `g3gpu.c` already holds clip-space triangles per eye, and `docs/FPS60.md` plans it. Nathan never ran the `fps60_probe=1` log, so we do not know the same-kind match rate.
- The blocker is cost. Our frame is GPU draw 5.5 ms + wait and readback 7-9 ms + convert ~4 ms + upload + compose + second readback + byte swap. Roughly 19 ms or more per presented frame in busy scenes. An in-between frame needs a full extra frame in the idle half of each tick, so it must fit in about 16 ms and we do not. Wide FX alone is 29 ms, so it could never coexist.
- Flat menus and the field at 30 are fine today (present about 2 ms with the GPU compositor). The field is capped at 30 by the game, not by us.

## Ranked speedups
1. **Build and measure what is already merged but never compiled (free).** PR #67 (bottom menu overlay cache; ON cost +10 ms present), #68 (stereo GPU compose outside the overworld: battle composite 14-22 ms to about 5 ms expected), #69 (reuse converted 3D layer per eye). These target exactly the menu and battle fps loss. Check the next log lines `[BMENU] overlay cache`, `[GPUC] stereo outside the overworld`, `[PERF]`/`[PERF3]` per scene.
2. **Skip the first CPU round trip (biggest real win, medium-large, needs device).** When the GPU compositor takes the frame and there is no CPU fog, copy the GPU 3D target to the compositor's 3D texture with a GPU transfer, not readback + convert + upload. Saves about 12 ms of stall and convert per frame in 3D scenes. Needs: a toggle in the settings menu, fallback to today's path for fog, wide strips, G3REUSE, and CPU-composed frames, and side-by-side screenshot check (6-bit DS colour precision goes away). This is also the prerequisite for 60 fps.
3. **Skip the second round trip.** Compose straight into the display framebuffer (gpuscale.c already scales on the GPU) instead of reading the compose target back and byte swapping. Saves a few ms and one full-frame CPU pass.
4. **Compiler flags (cheap to try, builder only).** The game compiles with `-O2` and no LTO (`full/Makefile`). `-O3` and `-flto` may give 5-10% on the game-side CPU part (about 20 ms of the battle frame). Needs a full rebuild and a normal smoke test; revert if anything changes.
5. **Finer battle profiling.** Battle "rest" is about 20 ms of unexplained game/pspoke time. PR #68's scene-tagged `[PERF3]` should show which scene; only then guess.
6. **Not recommended:** `gpu3d_async=1` (hidden config, default 0). It shows the 3D one frame behind the 2D and turns off wide view, hi-res and frame reuse.
7. **60 fps interpolated field:** only after 2 and 3, and only after the probe log says pairing is stable. Realistic as a toggle, default off, with auto step-down like his.

## Status update (2026-10-26, 60 fps thread)
Stage 1 (item 2) is written and merged behind the Debug-tab toggle `GPU 3D DIRECT` (`gpu_direct3d`, default OFF, not compiled, not tested): see CHANGELOG. To measure: build, turn it on in Debug, walk the overworld and a battle, then compare `[PERF]` present/g3soft times and the `[GPU3D] direct 600 3D frames` line (direct vs normal avg End us, how many frames had to be resolved to the CPU and why others were not direct) with the toggle OFF. Stage 2 (compose straight into the screen framebuffer) and stage 3 (in-between frame) follow only if stage 1 is clean on a device.

## What was and was not done in the first (docs-only) thread
No code was changed: the cloud session has no devkitARM, and all of 2-4 sit in the render path every frame, so they should not be merged without a compile and a device test. This note is the deliverable. To build the already merged PRs, the builder thread does a single build on request.
