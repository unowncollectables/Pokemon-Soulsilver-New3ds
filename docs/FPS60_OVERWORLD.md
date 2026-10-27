# Overworld frame cost: what can and cannot reach 60 fps (2026-10-27)

Numbers are from run 30 (Speed tweaks 4, overworld, 3D slider up, Wide on, GPU 3D direct on): 33.3 ms per frame, about 5 ms idle, so about 28 ms of work: game ~13 ms (includes the CPU 3D transform and clip), composite ~9 ms (left eye waits ~4 ms for the direct 3D draw, then two compose + readback round trips), present ~6 ms (strips and overlays ~2.7 ms, GPU present ~3 ms).

## Why 60 is not a tuning job
The game only advances 30 times a second. A 60 fps picture therefore means drawing an extra in-between frame (interpolated 3D, like pokeplatinum-3ds). An in-between frame is not free: it needs its own 3D draw, compose and present, roughly 15 ms. Today the real frame already costs 28 ms, so real + in-between would be ~43 ms in a 33 ms tick. Shaving 1 to 3 ms does not change that.

## What could realistically get there (in order of size)
1. **Do the CPU and GPU work at the same time (pipelining).** Today everything is in a row: game logic, 3D submit, GPU draw, compose, readback, present. Running compose and present of frame N on another core while the game computes frame N+1 would make a tick cost about max(13, 15) instead of 28. This is the only change big enough, and it is large: the 2D state (OAM, palettes, registers, VRAM) must be copied at the frame boundary because the game keeps writing it (the log already counts "changed DURING composite"). Needs a design pass and device testing; not a blind patch.
2. **Skip the compose round trip.** Each eye is composed on the GPU, read back to the CPU (blocking sync), byte-swapped, then uploaded again by present. Composing straight into the texture present samples removes two syncs and two full-frame CPU passes (roughly 3 to 5 ms). The CPU overlays (toast, save box, hunt counter) read those pixels, so they would move to the GPU too. Medium-large, GPU path, hardware needed.
3. **Stereo vertex shift on the GPU.** Both eyes are transformed once but clipped and queued twice on the CPU. A GPU vertex shader shift would halve the clip/queue cost. Medium-large.
4. **Small CPU trims (this change, Speed tweaks level 5).** Under a millisecond together. Worth it only because the [PERF5] line now shows exactly where the strips and present time goes.

## What cannot work
- Wide FX (29 ms), more overlays, or anything that adds per-frame CPU passes.
- Blind GPU queue changes: the lean-flush experiment hung the game.
- Dropping the 3D slider is the only instant win: with it at 0 there is one eye, so roughly half the compose and present cost.

## Realistic outcome
Without pipelining: stay at a steady 30 with slightly more headroom (a few ms, mostly in battles and menus). With 1 + 2 (and the interpolation from FPS60.md): a 60 fps overworld is plausible with stereo off or Wide off, uncertain with both on.
