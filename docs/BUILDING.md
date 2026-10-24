# Building guide

This project is a native 3DS port based on the upstream decompilation workflow. The exact build process may change as the project evolves.

## Prerequisites

- A working 3DS homebrew toolchain
- A local copy of the upstream decompilation or project source tree
- A legal ROM dump of Pokémon SoulSilver generated on your own system
- A compatible build environment for the targeted platform

## General workflow

1. Acquire the source tree for the project or upstream decompilation
2. Set up your local 3DS toolchain
3. Place your local ROM dump or extracted data in a safe, non-committed location
4. Run the project build command(s)
5. Package the output as a homebrew artifact
6. Load it onto your New 3DS or compatible hardware

## Safety requirements

- Never commit ROM files, extracted dumps, or Nintendo-owned assets to the repo
- Keep your local game data outside the repository
- Use the repo's .gitignore rules to avoid accidental uploads

## Notes

Because this project is still evolving, exact commands and setup instructions may change. This file should be updated as the build flow becomes more stable.
