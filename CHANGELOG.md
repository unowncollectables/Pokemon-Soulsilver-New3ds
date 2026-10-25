# Changelog

All notable changes to this project will be documented here.

## Unreleased

### Added

- build pipeline step 1: pspoke pinned in `upstream.lock`, `scripts/bootstrap.sh`, `scripts/ss3ds_build.sh`
- `toolchain-3ds/` (psp-* wrappers for devkitARM) and `pspoke-patches/` carried over from the Platinum 3DS port
- `docs/ARCHITECTURE.md`: where the code comes from, why pspoke, order of work
- improved GitHub landing page structure
- project banner image and badges
- documentation for build setup, roadmap, and FAQ
- contribution workflow and issue templates
- community standards and security policy
- clearer legal and safety guidance for homebrew work

### Changed

- project description refined to better reflect the experimental homebrew scope
- repository structure reorganized for easier contributor onboarding
- docs updated to emphasize legal and build safety practices

### Security

- added a dedicated private reporting path for vulnerabilities

## Audio and load-speed pass (2026-10-25)
- Zone prefetch is now wired to SoulSilver's map loader (wrap of `MapHeader_GetAreaDataBank`); it never ran before, so every map entry read the ROM cold from the SD card (spikes of 100-400 ms and audio underfeed holds). Watch for `[PREFETCH] zone A -> B` lines in log.txt.
- `audio_gain` default 10 -> 7: the log showed up to 1362 of 1720 grains soft-clipped in 10 s.
- Known remaining cost: title/intro/menu frames use alpha blending, which the GPU 2D compositor rejects (215 of 291 non-field frames), so they fall back to the 9-16 ms CPU composite and run below 60 fps.
