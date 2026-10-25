# soulsilver3ds/

The 3DS side of the port.

- `platform/` - the 3DS platform layer, copied from the Platinum 3DS port's `platinum3ds/platform/` (run 80):
  main loop, PSP/SDL API shims on libctru (`psp_ctru.c`, `sdl_ctru.c`), renderer glue and settings app
  (`render3ds.cpp`), GPU 3D (`g3gpu.c`), GPU 2D compositor (`gpucomp.c`), scaling/stereo (`gpuscale.c`), ROM cache,
  PICA shaders. Left out as Platinum-only: `ov5crumbs.S`, `p3ds_foltalk.c`, `p3ds_areas.h`, the area-name generator.
  `p3ds_areanames.h` is an empty stand-in until it is generated from pokeheartgold's map headers.
- `full/` - the full-game link (draft): `make check` shows what it found from `scripts/ss3ds_build.sh`'s output;
  `make` links `soulsilver3ds.3dsx`. Object, archive and `--wrap` lists come from pspoke's SoulSilver Makefile.

Known gaps for the first link: `render3ds.cpp` and `main3ds.c` still reference Platinum hooks that lived in the
Platinum game patches (map id, follower state, cheats and gameplay settings). Each needs a SoulSilver hook or a stub.
