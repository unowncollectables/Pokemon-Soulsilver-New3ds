# Pokémon SoulSilver — New 3DS Edition

<p align="center">
  <img src="./SoulSilver%20banner%201024.png" alt="Pokémon SoulSilver New 3DS banner" width="1024" />
</p>

A native New Nintendo 3DS homebrew port of Pokémon SoulSilver built from the pret/pokeheartgold decompilation. The goal is to bring the game to original New 3DS hardware with stereo 3D, better rendering, and a stable decompilation-based foundation for future work.

This project is still in active development and should be treated as an experimental, homebrew-focused port rather than a finished retail build.

## Overview

- Native 3DS rendering for New 3DS hardware
- Stereo 3D support
- Higher native output fidelity than stock hardware limits
- Gameplay and quality-of-life improvements
- Modular decompilation-based structure for future porting work
- Real hardware testing and compatibility validation

## Current status

This repo is best described as an early-to-mid stage homebrew/native port project.

Planned milestones:

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

## Repository structure

```text
.
├── README.md
├── LICENSE
├── CONTRIBUTING.md
├── .gitignore
├── SoulSilver banner 1024.png
├── docs/
│   └── BUILDING.md
├── .github/
│   └── ISSUE_TEMPLATE/
└── ...
```

### Main folders

- `docs/` — documentation and setup instructions
- `.github/` — GitHub templates and repo metadata
- `SoulSilver banner 1024.png` — project banner image used for the GitHub landing page

## Requirements

To build and test this project, you will need:

- A New Nintendo 3DS or compatible 3DS-family device
- A working homebrew environment
- A legal copy of Pokémon SoulSilver ROM/data for local builds
- A compatible 3DS toolchain
- Familiarity with decompilation and homebrew build workflows

## Building

This project depends on the upstream decompilation and native 3DS build flow.

At minimum, the workflow is:

1. Obtain the upstream decompilation base or source tree
2. Set up the local 3DS toolchain
3. Place your legal ROM dump or extracted data in a safe local location
4. Run the project build steps
5. Package the build as a homebrew artifact
6. Load it onto your New 3DS or compatible hardware

See `docs/BUILDING.md` for the current build guidance.

## Contributing

Contributions are welcome, especially for:

- build fixes
- documentation
- porting work
- compatibility patches
- rendering improvements
- input/system bug fixes

Before contributing:

- read the project docs
- check open issues
- keep changes focused
- never add ROMs, extracted Nintendo assets, or copyrighted data to the repo

See `CONTRIBUTING.md` for more details.

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

Please keep reports specific and include environment details.

## License

This project is licensed under the GNU General Public License v3.0.

See `LICENSE` for the full license text.

## Important note for publishing

If you plan to share this project publicly:

- keep the README honest about what is complete and what is not
- document all prerequisites clearly
- avoid committing ROMs, dumps, or copyrighted assets
- make the legal status explicit
- label the project as homebrew/decompilation work, not a commercial release

This is a fan-driven homebrew project and should be presented as such.
