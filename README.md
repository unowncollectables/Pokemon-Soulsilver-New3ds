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

This project is not a commercial or retail release. It is an experimental, fan-driven homebrew project built for learning, testing, and continued development.

## Table of contents

- [Project highlights](#project-highlights)
- [Current project status](#current-project-status)
- [Why this project exists](#why-this-project-exists)
- [Legal notice](#legal-notice)
- [Repository layout](#repository-layout)
- [Quick start](#quick-start)
- [Requirements](#requirements)
- [Build guide](#build-guide)
- [Contributing](#contributing)
- [Community standards](#community-standards)
- [Credits](#credits)
- [Support and issues](#support-and-issues)
- [License](#license)
- [Publishing note](#important-note-for-publishing)

## Project highlights

- Native 3DS rendering on New 3DS hardware
- Stereo 3D support and enhanced visual output
- Community-driven homebrew and porting work
- A modular decompilation-based structure for future work
- Real hardware validation and compatibility testing
- Clear legal and safety boundaries around ROM usage

## Current project status

This repo is best described as an early-to-mid stage homebrew/native port project.

The core aim is to move from a local experimental build toward a stable, documented, and maintainable native 3DS port workflow. The project is still evolving, and some systems are intentionally treated as work in progress.

### Milestone roadmap

- Phase 1: boot, title screen, and overworld in stereo 3D
- Phase 2: battles, menus, and Pokégear flow
- Phase 3: followers, day/night cycle, and weekly events
- Phase 4: Pokéathlon, Battle Frontier, and side systems
- Phase 5: polish, packaging, and release readiness

## Why this project exists

This project exists to push a native homebrew port of Pokémon SoulSilver onto original New 3DS hardware while keeping the work transparent, modular, and respectful of upstream decompilation work.

It is a long-term technical project focused on compatibility, rendering, and careful porting rather than a quick hack or unofficial commercial release. The goal is not just to run the game, but to do so in a way that is understandable, maintainable, and reproducible.

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
├── CODE_OF_CONDUCT.md
├── SECURITY.md
├── .gitignore
├── SoulSilver banner 1024.png
├── docs/
│   ├── BUILDING.md
│   ├── ROADMAP.md
│   ├── FAQ.md
│   └── SETUP.md
├── .github/
│   ├── ISSUE_TEMPLATE/
│   │   ├── bug_report.md
│   │   ├── feature_request.md
│   │   └── config.yml
│   └── PULL_REQUEST_TEMPLATE.md
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
- `docs/SETUP.md`
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

## Community standards

This project aims to remain respectful, constructive, and focused on technical progress.

- Please read `CODE_OF_CONDUCT.md` before participating.
- Please report sensitive issues privately through `SECURITY.md`.
- Please keep discussion productive and on-topic.

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
