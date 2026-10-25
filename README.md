# Pokémon SoulSilver — New 3DS Edition

<p align="center">
  <img src="./SoulSilver%20banner%201024.png" alt="Pokémon SoulSilver New 3DS banner" width="1024" />
</p>

<p align="center">
  <a href="https://github.com/unowncollectables/Pokemon-Soulsilver-New3ds/actions"><img alt="Build status" src="https://img.shields.io/badge/build-experimental-orange" /></a>
  <a href="LICENSE"><img alt="License" src="https://img.shields.io/badge/license-GPLv3-blue" /></a>
  <a href="https://github.com/unowncollectables/Pokemon-Soulsilver-New3ds/issues"><img alt="Issues" src="https://img.shields.io/badge/issues-open-ff6b6b" /></a>
  <a href="https://github.com/unowncollectables/Pokemon-Soulsilver-New3ds/discussions"><img alt="Discussions" src="https://img.shields.io/badge/discussions-welcome-6c5ce7" /></a>
</p>

A native New Nintendo 3DS homebrew port of Pokémon SoulSilver. It is built on [pspoke](https://github.com/IbrahimIrfan/pspoke), which compiles the pret/pokeheartgold decompilation natively, retargeted from PSP to the 3DS with devkitARM and a 3DS platform layer reused from the Platinum 3DS port. It is a native port, not an emulator. Game data is read from your own ROM at runtime.

This is an experimental fan project, not a commercial release. See [RELEASE_NOTES.md](RELEASE_NOTES.md) for what is in the current build.

## Legal

No ROM, save or Nintendo asset is in this repo and none must ever be committed. You supply your own legal US SoulSilver dump (SHA1 in `upstream.lock`).

## Requirements

- New Nintendo 3DS / New 2DS XL (the build uses the New 3DS CPU speed and memory mode) with custom firmware
- Your own `SoulSilver.nds` (US)
- To build: Linux or WSL2, devkitPro with `3ds-dev` (`DEVKITPRO=/opt/devkitpro`), Python 3 with `pip install ndspy`, git, make, patch, rsync, curl, about 3 GB of disk

## Build

```sh
bash scripts/bootstrap.sh                          # pspoke at upstream.lock + pspoke-patches/
bash scripts/ss3ds_build.sh ~/roms/SoulSilver.nds  # compiles the game; the ROM is read in place
cd soulsilver3ds/full && make check && make        # links soulsilver3ds.3dsx
make cia                                           # optional HOME Menu build (soulsilver3ds.cia)
```

Keep the ROM outside the repo. Details: `docs/BUILDING.md`.

## Install

1. Copy `soulsilver3ds.3dsx` to `sdmc:/3ds/` and launch it from the Homebrew Launcher, or install `soulsilver3ds.cia` with FBI.
2. Put your ROM at `sdmc:/3ds/soulsilver3ds/soulsilver.nds`.
3. Launch. The game creates `soulsilver.sav` there if none exists.

SD layout, all under `sdmc:/3ds/soulsilver3ds/`:

| File | Purpose |
|---|---|
| `soulsilver.nds` | your ROM (required) |
| `soulsilver.sav` | the save, a 512 KiB raw `.sav` |
| `soulsilver.sav.bak1..5` | automatic copies made at launch and clean exit (`save_backup=1`); only valid saves that differ from bak1 are rotated in, so a blank or damaged file never pushes good copies out |
| `import.sav` | optional: drop a raw 512 KiB `.sav` from a real cart / R4 / DeSmuME here; if valid it replaces `soulsilver.sav` at next launch (old one kept as `soulsilver.sav.preimport`, file renamed `import.sav.done`) |
| `config.txt` | `key=value` settings, written by the settings menu |
| `log.txt` | log of the run, useful for bug reports |
| `hunt.txt` | encounter counter (never touches the save) |

## Saves

The save is a plain 512 KiB `.sav`, intended to be interchangeable with a real cart or an R4-style flashcart. **Back up your save before every new build or experiment**, copy it somewhere off the SD card, and never test on the only copy of a save you care about. The port keeps three rolling backups, but a copy you made yourself is safer.

## Settings menu

Press **L+R+Y** (or hold the top-left of the touch screen for 0.3 s) in the game. L/R or a tap switches tab, left/right/A changes a value, B/START/L+R+Y closes. Changes apply on close and are saved to `config.txt`. SELECT shows a hidden Debug tab.

| Tab | Toggles |
|---|---|
| Display | Screen scale, Wide view, Wide FX (slow), Wide menus, Wide haze, Weather in strips, Edge fill, Camera zoom, FPS counter |
| 3D | 3D strength, Battle 3D depth, Split top (mono) |
| Controls | Circle pad D-pad, C-stick D-pad, ZR = Y button, ZL run toggle, IV/EV on summary (press Y on the summary) |
| Battle | Faster battles (skip move animations, optional fast HP bar), Catch info (Y at the wild battle menu), Encounter counter |
| System | Menu music, Save backups (3), Log size limit, Log detail, Fast text, Hold B skips text, Auto-advance text |
| Play | Reusable TMs, Field moves w/o HM, Repel prompt, No wild battles |
| Debug (SELECT) | Fast upload, Fast 3D convert, GPU compose, GPU compose 3D. For testing only. |

**Untested on device:** the Play tab, Battle tab, ZL run, IV/EV on summary and the other gameplay toggles were merged without a hardware test. They change game behaviour, so try them on a copy of your save first. The `cheat_*` keys (infinite money, always catch, shiny wild, fast eggs, exp boost, rare candy) exist only in `config.txt`, are off by default and are not in the menu.

The GPU 2D compositor also composes alpha-blend and brightness-fade frames (title, menus) by default. To turn that off set `gpu_blend=0` in `config.txt`; the `[GPUC]` log lines show how many frames used it.

## Known issues

- Poké Ball opening shows a double image with the 3D slider up.
- Wide FX is slow (about 29 ms per frame, overworld drops to roughly 15 fps). Keep it off.
- Audio can clip on loud sounds; menu frame rate is below 60 fps in places.
- Retail cart and flashcart save compatibility has not been verified end to end.
- First visit to a map can hitch while it loads from the SD card.

## Docs

`docs/ARCHITECTURE.md`, `docs/BUILDING.md`, `docs/SETUP.md`, `docs/DEVELOPMENT.md`, `docs/ROADMAP.md`, `docs/FAQ.md`, `CONTRIBUTING.md`, `SECURITY.md`, `CODE_OF_CONDUCT.md`.

## Credits

- [IbrahimIrfan/pspoke](https://github.com/IbrahimIrfan/pspoke) (GPL-3.0): the native SoulSilver build this port is based on
- [pret/pokeheartgold](https://github.com/pret/pokeheartgold): the SoulSilver/HeartGold decompilation
- [pret/pokeplatinum](https://github.com/pret/pokeplatinum) and the Platinum 3DS port ([3ds-platinum](https://github.com/unowncollectables/3ds-platinum), [GabeSud0/pokeplatinum-3ds](https://github.com/GabeSud0/pokeplatinum-3ds)): the 3DS platform layer, patches and CIA packaging
- devkitPro, libctru and citro3d

## License

GPL-3.0, see `LICENSE`. pspoke and the pret decompilations carry their own licenses; game data and assets belong to Nintendo, Game Freak and The Pokémon Company and are not distributed here. Pokémon is a trademark of its owners; this project is unaffiliated.
