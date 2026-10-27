<p align="center">
  <img src="./SoulSilver%20banner%201024.png" alt="Pokémon SoulSilver New 3DS Edition" width="720" />
</p>

<p align="center">
  <strong>Back to Johto, natively on the New 3DS.</strong><br>
  A real 3DS port of Pokémon SoulSilver: both screens filled, stereoscopic 3D, a Johto map on the touch screen.
</p>

<p align="center">
  <a href="https://github.com/unowncollectables/patch-builder"><img src="https://img.shields.io/badge/Builder-One--window_installer-9466FF?style=for-the-badge&logo=windows&logoColor=white" alt="One-window builder"></a>
  <a href="https://github.com/unowncollectables/Pokemon-Soulsilver-New3ds/commits/main"><img src="https://img.shields.io/github/last-commit/unowncollectables/Pokemon-Soulsilver-New3ds?style=for-the-badge&logo=github" alt="Last commit"></a>
  <a href="https://github.com/unowncollectables/Pokemon-Soulsilver-New3ds/issues"><img src="https://img.shields.io/github/issues/unowncollectables/Pokemon-Soulsilver-New3ds?style=for-the-badge" alt="Open issues"></a>
  <a href="LICENSE"><img src="https://img.shields.io/badge/License-GPL--3.0-4B5563?style=for-the-badge" alt="GPL-3.0 licence"></a>
</p>

<p align="center">
  <a href="#features">Features</a> &nbsp;·&nbsp;
  <a href="#status">Status</a> &nbsp;·&nbsp;
  <a href="#getting-started">Getting started</a> &nbsp;·&nbsp;
  <a href="#settings-menu">Settings</a> &nbsp;·&nbsp;
  <a href="#for-developers">For developers</a> &nbsp;·&nbsp;
  <a href="#credits">Credits</a>
</p>

---

Pokémon SoulSilver New 3DS Edition is built from the [pret/pokeheartgold](https://github.com/pret/pokeheartgold)
decompilation. It is **not an emulator**: the game's own code runs natively on the 3DS CPU, drawn the way a 3DS game
would draw it. It builds on [pspoke](https://github.com/IbrahimIrfan/pspoke), retargeted from PSP to the 3DS with
devkitARM, and reuses the platform layer, patches and lessons from the Platinum 3DS port
([3ds-platinum](https://github.com/unowncollectables/3ds-platinum),
[GabeSud0/pokeplatinum-3ds](https://github.com/GabeSud0/pokeplatinum-3ds)).

> [!IMPORTANT]
> **This repository contains no game content.** No ROM, save, graphics, music or text from the game is here or in any
> release. You supply your own legal US SoulSilver dump; the game reads its data from it at runtime. Never commit a ROM
> or a save.

## Features

On a New 3DS / New 2DS XL:

| | What to expect |
| :--- | :--- |
| **Native, not emulated** | The decompiled game code is compiled for the 3DS. Real `.sav` saves, real DS-cart and flashcart save compatibility. |
| **Both screens used** | The DS picture is drawn 1.25x bigger on the 3DS screens, with an optional wide view that extends battles, menus and the party screen across the full 400 px. |
| **Stereoscopic 3D** | Depth from the 3D slider in the overworld, battles and on the title screen. |
| **A Johto map on the touch screen** | The bottom screen is the Johto map with an icon column for Pokémon, Bag, Pokédex, Trainer card, Save and Options. It steps aside whenever the game needs its own touch screen (YES/NO, starter Poké Balls, Pokégear). |
| **GPU rendering** | 3D scenes and 2D compositing run on the 3DS GPU. |
| **Settings app** | Press L+R+Y in game for tabs of display, 3D, input, battle and play options. |
| **Quality of life** | Fast text (2x, 4x, instant), faster battles, hold B to skip text, ZL run toggle, reusable TMs, catch info and IV/EV on the summary, an encounter counter, SELECT soft reset. |
| **Shiny hunting suite** | Per-species encounter, streak and reset counters, odds options and a shiny chime (opt-in). |
| **Safe saves** | Rolling `.sav` backups at launch and exit, plus `import.sav` to bring a cartridge save in. |
| **On the HOME Menu** | Installs as a `.cia` with a 3D banner, or run the `.3dsx` from the Homebrew Launcher. |

## Status

| ✅ Works | 🛠️ In progress | 💭 Wishlist |
| :--- | :--- | :--- |
| Boot, title, overworld, battles, menus, saving | 60 fps (the game runs at its own 30; frame cost is being cut) | Real Wi-Fi trading with a real DS |
| 3D depth, wide view, GPU 3D and GPU 2D compositing | Union Room / local wireless (crashes, being diagnosed) | Sharper-than-DS rendering |
| Johto map on the touch screen | Visible wild Pokémon in the overworld | |
| Retail `.sav` round trip with real carts | HMs usable without the Pokémon | |
| Shiny suite, Dark Cave, trainer foe screen | Settings redesign with tooltips | |

See [Known issues](#known-issues) and [`docs/ROADMAP.md`](docs/ROADMAP.md) for details, and [`CHANGELOG.md`](CHANGELOG.md)
for every change.

## Getting started

### What you need

- A **New Nintendo 3DS, New 3DS XL or New 2DS XL** with custom firmware (Luma3DS) and the Homebrew Launcher
  (plus **FBI** for the HOME Menu version). Setup guide: <https://3ds.hacks.guide>
- Your own dump of **Pokémon SoulSilver (USA)** as `SoulSilver.nds`.
- A Windows, Linux or WSL2 computer to build it. To build by hand: devkitPro with `3ds-dev`
  (`DEVKITPRO=/opt/devkitpro`), Python 3 with `pip install ndspy`, git, make, patch, rsync, curl, about 3 GB of disk.

### 1. Easiest: the Pokemon 3DS Builder

Use the one-window [patch-builder](https://github.com/unowncollectables/patch-builder): tick SoulSilver (and/or
Platinum), pick the game file you own and your SD card, press **Build and install**. It downloads this project,
installs the build tools, compiles, and copies the `.3dsx` (and optionally the `.cia`) to the SD card, backing up your
save first. Step-by-step instructions are in its README.

### 2. Or build it by hand

```sh
bash scripts/bootstrap.sh                          # pspoke at upstream.lock + pspoke-patches/
bash scripts/ss3ds_build.sh ~/roms/SoulSilver.nds  # compiles the game; the ROM is read in place
cd soulsilver3ds/full && make check && make        # links soulsilver3ds.3dsx
make cia                                           # optional HOME Menu build (soulsilver3ds.cia)
```

Keep the ROM outside the repo. Details: [`docs/BUILDING.md`](docs/BUILDING.md).

### 3. Copy to the SD card and play

1. Copy `soulsilver3ds.3dsx` to `sdmc:/3ds/` and launch it from the Homebrew Launcher, or install `soulsilver3ds.cia`
   with FBI.
2. Put your ROM at `sdmc:/3ds/soulsilver3ds/soulsilver.nds`.
3. Launch. The game creates `soulsilver.sav` there if none exists.

## SD card layout

All under `sdmc:/3ds/soulsilver3ds/`:

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

The save is a plain 512 KiB `.sav`, interchangeable with a real cart or an R4-style flashcart (a retail save has been loaded and round-tripped successfully). **Back up your save before every new build or experiment**, copy it somewhere off the SD card, and never test on the only copy of a save you care about. The port keeps three rolling backups, but a copy you made yourself is safer.

## Settings menu

Press **L+R+Y** (or hold the top-left of the touch screen for 0.3 s) in the game. L/R or a tap switches tab, left/right/A changes a value, B/START/L+R+Y closes. Changes apply on close and are saved to `config.txt`. SELECT shows a hidden Debug tab.

| Tab | Toggles |
|---|---|
| Display | Screen scale, Wide view, Camera zoom, Bottom menu HD |
| Wide | Wide battles, Wide party/bag, Wide Pokédex, Wide PC boxes, Wide menus, Wide FX (slow), Wide haze, Weather in strips, Edge fill |
| 3D | 3D strength, Battle 3D depth, Split top (mono), Smooth title 3D |
| Input | Bottom menu ON/OFF, Bottom layout (JOHTO map or 4x2 tiles), Bottom map, Kanto on map, Menu button, Circle pad D-pad, C-stick D-pad, ZR = Y button, ZL run toggle, IV/EV on summary (press Y on the summary) |
| Battle | Faster battles, Catch info (Y at the wild battle menu), Encounter counter, SELECT = reset (wild battles), Skip intro on reset |
| Setup | Volume, Menu music, Save backups (3), Log size limit, Log detail, Fast text, Hold B skips text, Auto-advance text, 3D frame reuse, FPS counter |
| Play | Reusable TMs, Field moves w/o HM, Repel prompt, No wild battles, Local wireless (test) |
| Debug (SELECT) | Fast upload, Fast 3D convert, GPU compose, GPU compose 3D (+ menus), GPU 3D direct, Audio limiter, Audio stall fade, Speed tweaks. For testing only. |

The Bottom menu (on by default) turns the bottom screen into the JOHTO map with an icon column for Pokémon, Bag, Pokédex, Trainer card, Save and Options. Turn it off for the stock touch screen. `soulsilver3ds/full/README_settings.txt` lists every `config.txt` key.

**Untested on device:** the Play tab, Battle tab, ZL run, IV/EV on summary and the other gameplay toggles were merged without a hardware test. They change game behaviour, so try them on a copy of your save first. The `cheat_*` keys (infinite money, always catch, shiny wild, fast eggs, exp boost, rare candy) exist only in `config.txt`, are off by default and are not in the menu.

The GPU 2D compositor also composes alpha-blend and brightness-fade frames (title, menus) by default. To turn that off set `gpu_blend=0` in `config.txt`; the `[GPUC]` log lines show how many frames used it.

## Known issues

- Poké Ball opening shows a double image with the 3D slider up.
- Wide FX is slow (about 29 ms per frame, overworld drops to roughly 15 fps). Keep it off.
- The game runs at about 30 fps (its own rate); 60 fps work is in progress, see [`docs/FPS60.md`](docs/FPS60.md).
- Local wireless and the Union Room can crash the console; leave Local wireless off for now.
- Visible wild Pokémon, battle sprites in 3D, and a brief old-menu flash after a bottom-menu tap are being worked on.
- Speed tweaks and some gameplay toggles are merged but not yet tested on hardware.
- First visit to a map can hitch while it loads from the SD card.

Found a bug? Open an [issue](https://github.com/unowncollectables/Pokemon-Soulsilver-New3ds/issues) and attach
`sdmc:/3ds/soulsilver3ds/log.txt` plus your console model.

## For developers

How it works:

```
your SoulSilver.nds (read at runtime, never copied into the repo)

pret/pokeheartgold -> pspoke @ upstream.lock -> + pspoke-patches/ -> + soulsilver3ds/ platform -> .3dsx / .cia
```

- The decompiled game C is compiled with GCC for the 3DS CPU. Changes to pspoke live as numbered patches in
  `pspoke-patches/`; never edit an applied patch in place, add a new numbered one.
- `soulsilver3ds/` is the 3DS platform layer: renderer (GPU 3D, GPU compose), audio, input, the settings app, the Bottom
  menu and the log.
- Experimental features get a toggle in the settings menu, which is removed once tested.

Contributing: [`CONTRIBUTING.md`](CONTRIBUTING.md). New to the code or picking it up as an AI assistant: start with
[`AGENTS.md`](AGENTS.md) and [`docs/HANDOFF.md`](docs/HANDOFF.md).

| Doc | What it covers |
| :--- | :--- |
| [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md) | how the port is put together |
| [`docs/BUILDING.md`](docs/BUILDING.md), [`docs/SETUP.md`](docs/SETUP.md) | toolchain and build steps |
| [`docs/DEVELOPMENT.md`](docs/DEVELOPMENT.md) | working on the code |
| [`docs/ROADMAP.md`](docs/ROADMAP.md), [`docs/FAQ.md`](docs/FAQ.md) | plans and common questions |
| [`docs/FPS60.md`](docs/FPS60.md), [`docs/SPEEDUPS.md`](docs/SPEEDUPS.md) | frame rate and speed work |
| [`docs/WIRELESS.md`](docs/WIRELESS.md) | local wireless notes |
| [`docs/PLATINUM_SYNC.md`](docs/PLATINUM_SYNC.md) | what was shared with the Platinum port |
| [`RELEASE_NOTES.md`](RELEASE_NOTES.md), [`CHANGELOG.md`](CHANGELOG.md), [`SECURITY.md`](SECURITY.md), [`THIRD_PARTY.md`](THIRD_PARTY.md) | releases, history, security, licences |

## Credits

- [@unowncollectables](https://github.com/unowncollectables): project lead, testing on real hardware, direction.
- **Claude** (Anthropic): AI pair programmer that wrote much of the port's code, patches, tooling and docs with Nathan.
- **[GabeSud0](https://github.com/GabeSud0)** and [pokeplatinum-3ds](https://github.com/GabeSud0/pokeplatinum-3ds): the Platinum 3DS port whose platform layer, bottom-screen frame and many ideas this project reuses.
- **[IbrahimIrfan/pspoke](https://github.com/IbrahimIrfan/pspoke)** (GPL-3.0): the native SoulSilver build this port is based on.
- **[pret/pokeheartgold](https://github.com/pret/pokeheartgold)** and **[pret/pokeplatinum](https://github.com/pret/pokeplatinum)**: the decompilations.
- [devkitPro](https://devkitpro.org), libctru and citro3d; [Azahar](https://azahar-emu.org) for emulator testing.
- Thanks to everyone who tests, reports bugs and shares logs. Want your name here? Open a PR.

## Legal

GPL-3.0, see [`LICENSE`](LICENSE). pspoke and the pret decompilations carry their own licenses; game data and assets
belong to Nintendo, Game Freak and The Pokémon Company and are not distributed here. Pokémon is a trademark of its
owners; this is an unofficial fan project and is not affiliated with or endorsed by them.
