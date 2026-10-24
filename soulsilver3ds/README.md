# soulsilver3ds/

The 3DS side of the port lives here (it does not exist yet):

- `platform/` - the 3DS platform layer: main loop, input, threads, PSP-API shims on libctru (`psp_ctru.c`,
  `sdl_ctru.c`), renderer glue, GPU 3D (`g3gpu.c`), GPU 2D compositor (`gpucomp.c`), scaling/stereo
  (`gpuscale.c`), ROM cache, PICA shaders. Starting point: the Platinum 3DS port's `platinum3ds/platform/`,
  minus its Platinum-only files (area/ability name tables, follower talk, Platinum ov5 crumbs).
- `full/` - the full-game link: pspoke's SoulSilver objects (from `scripts/ss3ds_build.sh`) + `platform/` ->
  `soulsilver3ds.3dsx`, then the HOME Menu `.cia`.

See `docs/ARCHITECTURE.md` for the plan and the order of the work.
