# Pokémon SoulSilver — New 3DS Edition

A native New Nintendo 3DS homebrew port of Pokémon SoulSilver built from the pret/pokeheartgold decompilation. The goal is to bring the game to original New 3DS hardware with stereo 3D, higher native rendering, and a usable real-hardware experience.

This project is still in active development. It is not a full retail build and should be treated as an experimental decompilation-based port.

## Important legal notice

This repository does not include any copyrighted game ROMs, copyrighted assets, or Nintendo-owned extracted content.

By design, this project follows the BYO-ROM rule:
- You must provide your own legal copy of Pokémon SoulSilver
- You must dump or extract the ROM on your own machine
- The repo does not bundle game assets or Nintendo data

If you are building or distributing this project, make sure you comply with local laws and the rights of the original copyright holders.

## What this project aims to do

- Native 3DS rendering for New 3DS hardware
- Stereoscopic 3D support
- Improved resolution compared to stock hardware rendering
- Follower systems and gameplay enhancements
- Real hardware testing and compatibility work
- A modular, decompilation-based codebase for future portability

## Current status

This repo is best described as an early-to-mid stage homebrew/native port project.

Planned phases:
- Phase 1: boot, title screen, and overworld in stereo 3D
- Phase 2: battles, menus, and Pokégear flow
- Phase 3: followers, day/night cycle, and weekly events
- Phase 4: Pokéathlon, Battle Frontier, and side systems
- Phase 5: polish, packaging, and release readiness

## Requirements

To build and test this project, you will need:

- A New Nintendo 3DS or compatible 3DS family device
- A working homebrew environment
- A legal copy of Pokémon SoulSilver ROM/data for your local build
- A build toolchain compatible with the 3DS decompilation setup
- A working knowledge of 3DS homebrew and decompilation workflows

## Repository layout

This repo currently includes the project metadata and documentation, but does not yet contain the complete source tree or generated build outputs.

Expected structure for a more complete project:

- src/ — game source code
- include/ — shared headers
- build/ — generated build outputs
- assets/ — non-ROM project assets
- docs/ — setup and troubleshooting docs
- tools/ — helper scripts

## Building

This project depends on the upstream decompilation and native 3DS build flow.

At minimum, the general process will involve:

1. Obtaining a local copy of the upstream decompilation base
2. Preparing the ROM dump or extracted game content locally
3. Running the 3DS toolchain build steps
4. Packaging the output for homebrew use
5. Running it on real hardware or a compatible emulator setup

See the project docs for the current build guidance as it evolves.

## Contributing

Contributions are welcome, especially for:

- build fixes
- documentation
- porting work
- compatibility patches
- rendering improvements
- input and system bug fixes

Before contributing:

- Read the project docs
- Check open issues
- Keep changes focused
- Do not add ROMs, extracted Nintendo assets, or copyrighted data to the repo

See CONTRIBUTING.md for contribution guidelines.

## Credits

Built on the pret/pokeheartgold decompilation.

Project references and inspirations:

- Pokémon Platinum — New 3DS Edition — prior native 3DS work from the same developer
- pokeplatinum-3ds by GabeSud0 — public reference implementation for a native 3DS port
- pret/pokeheartgold — the decompilation this port builds from
- pret/pokeplatinum — related decompilation and reference work
- emerald-3ds — a native 3DS port that helped validate the approach

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

See LICENSE for the full license text.

## Important note for publishing

If you plan to share this project publicly:

- keep the README honest about what is complete and what is not
- document all prerequisites clearly
- avoid committing game dumps, ROMs, or copyrighted assets
- make the legal status explicit
- label the project as homebrew/decompilation work, not a commercial release

This is a fan-driven homebrew project and should be presented as such.
