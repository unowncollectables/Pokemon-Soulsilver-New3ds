# AGENTS.md: read this first (for any AI assistant or new contributor)

This file is the entry point for Claude, Grok, Codex, Copilot or a human picking up the project cold. Keep it short and true. The detailed state and next steps are in [docs/HANDOFF.md](docs/HANDOFF.md).

## What this is
A native New 3DS port of Pokemon SoulSilver (US). It is built from [pspoke](https://github.com/IbrahimIrfan/pspoke) (SoulSilver compiled natively), retargeted to the 3DS with devkitARM and a libctru/citro3d platform layer. It reuses the recipe from the Platinum 3DS port.

## Hard rules
1. **Never commit a ROM, save, extracted game data or any Nintendo asset.** The ROM is read in place from outside the repo.
2. **Never modify or overwrite a user's live `soulsilver.sav`.** Copy it somewhere off the SD card first, every time.
3. Work on a branch and open a PR; do not push to `main` directly. Keep PRs small.
4. Do not claim something works on hardware unless a `log.txt` or the owner confirmed it. Say "builds" or "untested on device" honestly.
5. If a change touches game behaviour, put it behind a `config.txt` toggle, default off unless proven.

## Map of the repo
| Path | What |
|---|---|
| `soulsilver3ds/platform/` | 3DS platform layer: `main3ds.c`, `render3ds.cpp` (screens, wide view), `g3gpu.c` (GPU 3D), `gpucomp.c` (GPU 2D compositor), `ss3ds_glue.c` (game hooks, config), `ss3ds_settings_ui.h` (settings app), `romcache3ds.c`, `speedcheck.c` |
| `soulsilver3ds/full/` | Link Makefile, CIA rsf/art, `README_settings.txt` (every config key) |
| `pspoke-patches/` | Patches applied to pspoke by `scripts/bootstrap.sh`. Game-source changes live here, not in the repo tree |
| `scripts/` | `bootstrap.sh`, `ss3ds_build.sh`, `ss3ds_game.sh` |
| `toolchain-3ds/` | `psp-*` wrappers that emit devkitARM objects |
| `tools/` | `ss_savecheck.py` (read-only save validator), CI artifact checker, CIA tool build |
| `upstream.lock` | pinned pspoke commit and expected ROM SHA1 |
| `docs/` | `HANDOFF.md` (state), `ARCHITECTURE.md`, `BUILDING.md`, `ROADMAP.md`, `PLATINUM_SYNC.md` |

## Build and test (WSL2/Linux, devkitPro `3ds-dev`)
```sh
export DEVKITPRO=/opt/devkitpro DEVKITARM=/opt/devkitpro/devkitARM
bash scripts/bootstrap.sh
bash scripts/ss3ds_build.sh /path/to/SoulSilver.nds
cd soulsilver3ds/full && make check && make && make cia
```
Outputs: `soulsilver3ds.3dsx` and `soulsilver3ds.cia` in `soulsilver3ds/full/`. Install both under `sdmc:/3ds/soulsilver3ds/` and `cias/` as in the README. There is no automated hardware test: verification is playing on a New 3DS and reading `sdmc:/3ds/soulsilver3ds/log.txt`.

## Log markers worth grepping in log.txt
`[3DS] battle 1` (battle path ran), `[G3REUSE]` (3D frame reuse), `[BTN]` (touch menu button), `[GPUC]` (GPU 2D compositor), `[SPIKE]`/`[PERF3]`/`[ZONE]` (frame time diagnostics), `[SPEED]` (CPU speed check).

## Conventions
- Match the surrounding C/C++ style; comments are sparse and explain why.
- New settings: add the key in `ss3ds_glue.c`, a row in `ss3ds_settings_ui.h`, a line in `soulsilver3ds/full/README_settings.txt`, and a line in `CHANGELOG.md`.
- Update `docs/HANDOFF.md` ("Current state" and "Next steps") in every PR that changes what works or what is untested.
- Commit messages: short imperative subject. PR body: what a player would see before and after, then how.

## Not in this repo
The Platinum port lives in [3ds-platinum](https://github.com/unowncollectables/3ds-platinum). Its GUI patch builder is Platinum-only; there is no SoulSilver patcher yet (see HANDOFF).
