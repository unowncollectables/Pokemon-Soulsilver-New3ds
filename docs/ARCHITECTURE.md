# Architecture and porting plan

This is how the SoulSilver port is put together, why, and in what order the work goes. It is based on a survey of
the two earlier Platinum ports (October 2026).

## Where the code comes from

| Source | What it gives us | How we use it |
|---|---|---|
| [pspoke](https://github.com/IbrahimIrfan/pspoke) (GPL-3.0) | SoulSilver (US) already compiled natively for the PSP and **playable**: pret/pokeheartgold + pokeheartgold-slop sources, the ARM assembly that is not decompiled yet translated to C, data converted from your ROM, the shared NitroSDK replacement (libntr), sound engine, melonDS-derived 2D/3D cores | Pinned in `upstream.lock`, fetched by `scripts/bootstrap.sh`, patched by `pspoke-patches/`, compiled with `toolchain-3ds/` |
| Platinum 3DS port (`unowncollectables/3ds-platinum`) | The proven recipe for turning pspoke into a 3DS program: `psp-gcc` wrappers that emit devkitARM objects, PSP-SDK header shims, a libctru platform layer, GPU 3D path, GPU 2D compositor, 1.25x / wide view / stereo 3D, ROM cache, settings menu, HOME Menu CIA | `toolchain-3ds/` and `pspoke-patches/` are carried over as-is (all 11 patches apply cleanly to the pinned pspoke commit). The platform layer comes next (`soulsilver3ds/platform/`) |
| pokeplatinum-3ds fork (GabeSud0, `unowncollectables/pokeplatinum-3ds`) | A cleaner second architecture (the pret decomp compiled directly against a Nitro shim, no PSP layer), plus tooling: data-pack builder, 3D HOME Menu banner from your ROM, CI that never uploads game code, wide-screen UI layouts | Tooling and UI ideas now (`tools/check_ci_artifacts.py` is already here). Its architecture later, see below |

## Why pspoke and not the fork's approach

The fork builds pret/pokeplatinum directly, which works because Platinum is fully decompiled. pret/pokeheartgold
is not: at commit `6988158` it still has about **18,900 functions in ARM/Thumb assembly** (`asm/`, 22 MB) against
roughly 10,000 in C, and its README warns that changing function sizes can crash the game. A direct build would
mean translating all of that first.

pspoke has already done that translation for SoulSilver (`soulsilver-native-codegen/`, `soulsilver-native-play/`)
and the result plays on a PSP. The Platinum 3DS port shows that pspoke's output can be retargeted to the 3DS by
swapping the toolchain and the platform layer. So the shortest road to SoulSilver booting on a New 3DS is the same
road Platinum took.

The fork's approach stays the long-term goal: once pokeheartgold is fully decompiled, a direct build drops the PSP
layer and the translated assembly. Keep 3DS code behind small interfaces so it can move over.

## Build pipeline

```
scripts/bootstrap.sh       pspoke @ upstream.lock  +  pspoke-patches/*.patch  ->  ./pspoke (branch ss3ds)
scripts/ss3ds_build.sh ROM PATH=toolchain-3ds/bin:... ; pspoke/scripts/ss3ds_game.sh ROM
                           = pspoke's soulsilver.sh minus the PSP renderer and EBOOT link
                           -> devkitARM objects/archives under pspoke/.work/tree/test_out/soulsilver-native-core/
soulsilver3ds/full/        (next) link those + soulsilver3ds/platform/ + citro3d/libctru -> soulsilver3ds.3dsx
```

The ROM is read in place. Nothing built from it goes into this repository.

## Order of work

1. **Game objects compile with devkitARM** (`scripts/ss3ds_build.sh`). Expect a first round of fixes like
   Platinum's patch 0001 (ARM type and asm fixes) in SoulSilver-only pspoke code.
2. **Platform layer**: bring over `platinum3ds/platform/` (main loop, PSP/SDL shims on libctru, logging, ROM cache,
   `render3ds.cpp`, `g3gpu.c`, `gpucomp.c`, `gpuscale.c`, shaders); drop the Platinum-only tables.
3. **Link** (`soulsilver3ds/full/Makefile`, from `platinum3ds/full/Makefile`): SoulSilver's app objects come from
   `nitromain-perf/` instead of `native-audio-app/`, and its renderer from `render-fastcompare/`
   (`window_adapter`, `g3_handler`, `frontend`) instead of `native-stack-render/`. The `--wrap=` list needs
   checking against SoulSilver's symbol names.
4. **Boot on hardware**: title screen, new game, New Bark Town. Then the overworld in 3D.
5. **Features from Platinum**: 1.25x scale, wide view, stereo, settings menu, FPS work, CIA. Following Pokémon are
   native in SoulSilver, so Platinum's follower system is not needed; its draw-order fixes may still be.
6. **SoulSilver-specific**: Pokégear (bottom screen), Pokéwalker (likely disabled), Pokéathlon, Battle Frontier,
   day/night and weekly events.

## Platinum lessons to keep

- int enums for game code, short enums only for libctru/citro3d-facing files (`-fno-short-enums` in the wrappers).
- PSP-only address windows in `MIi_NativeRangeOk` (patch 0005) and the missing link wraps (`CTRDG_Init` etc.) were
  the first two boot crashes on Platinum; check both early.
- The 3D readback race was the root cause of tearing (Platinum run 37).
- Keep `log.txt` logging from the first build; it found the Snowpoint crash.
