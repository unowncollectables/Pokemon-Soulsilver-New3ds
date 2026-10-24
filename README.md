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

A native New Nintendo 3DS homebrew port of Pokémon SoulSilver built from the pret/pokeheartgold decompilation. The goal is to bring the game to original New 3DS hardware with improved rendering, stereoscopic 3D support, and a maintainable decompilation-based codebase.

This project is not a commercial or retail release. It is an experimental, fan-driven homebrew port built for learning, testing, and continued development.

## Why this project exists

- Native 3DS rendering on New 3DS hardware
- Stereoscopic 3D support
- Better visual output than stock hardware limits
- Modularity for future portability work
- Community-driven homebrew porting and testing
- A clean decompilation-based foundation for long-term work

## Current project status

This repo is best described as an early-to-mid stage homebrew/native port project.

### Milestone roadmap

- Phase 1: boot, title screen, and overworld in stereo 3D
- Phase 2: battles, menus, and Pokégear flow
- Phase 3: followers, day/night cycle, and weekly events
- Phase 4: Pokéathlon, Battle Frontier, and side systems
- Phase 5: polish, packaging, and release readiness

## Legal notice

This repository does not include any copyrighted game ROMs, copyrighted assets, or Nintendo-owned extracted content.

By design, this project follows the BYO-ROM rule:

- You must provide your own legal copy of Pokémon SoulSilver
- You must dump or extract the ROM on your own machine
- The repo does not bundle game assets or Nintendo data

If you are building or distributing this project, make sure you comply with local laws and the rights of the original copyright holders.

## Repository layout

```text
.
├── README.md
├── LICENSE
├── CONTRIBUTING.md
├── .gitignore
├── SoulSilver banner 1024.png
├── docs/
│   ├── BUILDING.md
│   ├── ROADMAP.md
│   └── FAQ.md
├── .github/
│   └── ISSUE_TEMPLATE/
│       ├── bug_report.md
│       ├── feature_request.md
│       └── config.yml
└── ...
```

## Quick start

1. Read the docs in `docs/`.
2. Prepare your local 3DS toolchain and legal ROM dump.
3. Build the project using the instructions in `docs/BUILDING.md`.
4. Test on a New 3DS or compatible device.
5. Open an issue or PR with clear reproduction and environment details.

## Requirements

To build and test this project, you will need:

- A New Nintendo 3DS or compatible 3DS-family device
- A working homebrew environment
- A legal copy of Pokémon SoulSilver ROM/data for local builds
- A 3DS-compatible toolchain
- Familiarity with decompilation and native homebrew workflows

## Build guide

The project depends on the upstream decompilation and native 3DS build flow.

For the current workflow, see:

- `docs/BUILDING.md`
- `docs/ROADMAP.md`
- `docs/FAQ.md`

## Contributing

Contributions are welcome, especially for:

- build fixes
- documentation improvements
- compatibility patches
- rendering work
- input/system bug fixes
- porting and feature experiments

Before contributing:

- read the README and docs
- check open issues
- keep changes focused and testable
- never add ROMs, dumps, or copyrighted Nintendo data to the repo

See `CONTRIBUTING.md` for contribution guidelines.

## Credits

Built on the pret/pokeheartgold decompilation.

Project references and inspirations:

- Pokémon Platinum — New 3DS Edition
- pokeplatinum-3ds by GabeSud0
- pret/pokeheartgold
- pret/pokeplatinum
- emerald-3ds

## Support and issues

Use the GitHub Issues tracker to report:

- build failures
- runtime bugs
- compatibility problems
- missing documentation
- feature requests

Please keep reports specific and include both environment details and reproduction steps.

## License

This project is licensed under the GNU General Public License v3.0.

See `LICENSE` for the full text.

## Important note for publishing

If you plan to share this project publicly:

- keep the README honest about what is complete and what is not
- document all prerequisites clearly
- avoid committing ROM dumps, save files, or copyrighted assets
- make the legal status explicit
- label the project as a homebrew/decompilation effort, not a commercial release

This is a fan-driven homebrew project and should be presented as such.
