# Building guide

This project is a native 3DS port based on the upstream decompilation workflow. The exact build process may evolve as the project matures, so this guide is intentionally practical and modular.

## Prerequisites

Before you build anything, make sure you have:

- A working 3DS homebrew toolchain
- A local copy of the upstream decompilation or project source tree
- A legal ROM dump of Pokémon SoulSilver generated on your own system
- A compatible build environment for the target platform
- A clean local workspace outside the repository for any extracted or generated data

## Current workflow (step 1: game objects)

The port builds on [pspoke](https://github.com/IbrahimIrfan/pspoke), which already compiles SoulSilver natively
(see `docs/ARCHITECTURE.md`). Today the scripts compile the game for the 3DS; the 3DS link comes next.

Needs: Linux or WSL2, devkitPro with `3ds-dev` (`DEVKITPRO=/opt/devkitpro`), Python 3 with `pip install ndspy`,
git, make, patch, rsync, curl, about 3 GB of disk.

```sh
bash scripts/bootstrap.sh                          # pspoke at upstream.lock + pspoke-patches/
bash scripts/ss3ds_build.sh ~/roms/SoulSilver.nds  # your own dump, read in place
```

pspoke is tested with the SoulSilver (US) dump whose SHA1 is in `upstream.lock`; the build warns on another hash.
Logs for each step are in `pspoke/.work/tree/logs/`. Keep your ROM outside this folder, for example:

```text
~/3ds/
├── pokemon-soulsilver-new3ds/
└── roms/SoulSilver.nds
```

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
