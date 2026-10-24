# Project setup guide

This document covers the local environment and workflow needed to work on the project in a clean and reproducible way.

## Recommended structure

A simple local layout is the easiest way to keep extracted data and source separate from the Git repository:

```text
~/3ds/
├── pokemon-soulsilver-new3ds/
├── pokeheartgold-src/
├── roms/
├── extracted/
├── tools/
└── build/
```

This keeps the repository focused on code and documentation while leaving legally sensitive or generated files outside the Git tree.

## Prerequisites

Before starting development, make sure you have:

- a legal copy of Pokémon SoulSilver
- a working 3DS homebrew toolchain
- a local compiler and build environment
- source access to upstream decompilation work used as the base
- enough disk space for generated build artifacts

## Local workflow

1. Clone this repository.
2. Prepare a legal local ROM dump or extracted game data in a non-committed folder.
3. Set up your toolchain and environment variables if required by your build system.
4. Keep generated files out of the repo using `.gitignore` and local folder conventions.
5. Test changes on a real New 3DS or compatible environment when possible.

## Safety and hygiene

- Never commit ROM files, extracted dumps, or Nintendo-owned assets
- Keep local game data separate from the repository working tree
- Avoid uploading generated binaries or temporary build artifacts
- Keep your build and test output clearly organized

## Good development habits

- make small, reviewable commits
- document any setup changes
- write clear PR summaries
- record environment assumptions when a build is only reproducible under specific conditions

## Troubleshooting

If build setup fails, check:

- missing toolchain dependencies
- source tree path issues
- generated file conflicts
- incorrect local ROM/data path setup
- stale build caches or old output files

## Long-term goal

The goal is to move from a mostly documentation-centered project toward a reproducible build flow that can be shared, tested, and refined by contributors.
