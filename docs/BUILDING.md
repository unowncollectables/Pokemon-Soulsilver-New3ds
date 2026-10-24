# Building guide

This project is a native 3DS port based on the upstream decompilation workflow. The exact build process may evolve as the project matures, so this guide is intentionally practical and modular.

## Prerequisites

Before you build anything, make sure you have:

- A working 3DS homebrew toolchain
- A local copy of the upstream decompilation or project source tree
- A legal ROM dump of Pokémon SoulSilver generated on your own system
- A compatible build environment for the target platform
- A clean local workspace outside the repository for any extracted or generated data

## Recommended folder layout

A simple local setup is usually easiest:

```text
~/3ds/
├── pokeheartgold-src/
├── pokemon-soulsilver-new3ds/
├── roms/
├── extracted/
└── build/
```

This keeps ROMs and extracted assets away from the Git working tree so they do not accidentally get committed.

## General workflow

1. Acquire the upstream decompilation source or project source tree.
2. Set up your local 3DS toolchain and required dependencies.
3. Place your legal ROM dump or extracted game data in a safe local directory.
4. Configure the project/build scripts for your local environment.
5. Run the project build steps.
6. Package the resulting artifact as a homebrew-compatible format.
7. Load it onto a New 3DS or compatible device for testing.

## Safety requirements

- Never commit ROM files, extracted dumps, or Nintendo-owned assets to the repo
- Keep your local game data outside the repository
- Use the repo's `.gitignore` rules to avoid accidental uploads
- Treat all copyrighted assets as local-only unless you are legally authorized to distribute them

## Notes for developers

Because this project is still evolving, exact commands and configuration may change. Keep the build process documented and update this file whenever a new stable workflow is established.

If you are contributing code or build fixes, document the exact toolchain requirements and any environment assumptions so others can reproduce them.

## Troubleshooting checklist

If the build fails, check:

- toolchain installation and PATH configuration
- local source tree integrity
- whether ROM/extracted files are placed outside the repo
- generated asset or output directories
- whether the build script is targeting the correct platform and architecture

## Long-term goal

The goal is to move from a local experimental build to a cleaner, more documented, and more reproducible homebrew porting workflow.
