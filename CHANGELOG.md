# Changelog

All notable changes to this project will be documented here.

## Unreleased

### run 3 (2026-10-25)
- Fix: crash on soft reset (L+R+START+SELECT): `[SS-MISSING] sub_02038D90` trap aborted (hardware run 2, map 33). sub_02038D90 and sub_02039AA4 hand-ported (offline comm checks), OS_ResetSystem now goes to the 3DS relaunch (P3DS_SoftReset). pspoke patch 0061.
- Title lag: back to the fast GPU path. Settings > 3D > W-buffer depth is now OFF / PICA W (default) / GPU Z (config `g3_wmode`; the old `g3_wdepth` key is ignored). CPU 3D for W-buffer frames moved to the Debug tab (`g3_wcpu`, slow). CPU 3D did NOT fix Lugia on hardware in run 2, so depth is not the cause; Lugia is still open.
- Diagnostics: `[WSUM]` vertex/matrix checksums for W-buffer frames every 60 frames; `g3_shot=N` frame dumps work on hardware.
- 3dsx: homebrew-menu icon is the SoulSilver art and the version string follows the build (was stuck at "run 1"). CIA icon/banner checked against run 1 (identical).
### Save and log flushed to the SD card (2026-10-25, untested; not in any build yet)
- Each in-game save now ends with an fsync, so the SD card commits it before the game goes on; closing from HOME or losing power right after "Saved" can no longer lose it. HOME, sleep and APT exit also sync any unsynced save bytes. pspoke patch 0070 (`backup.c`); logs `[SAVE] sync (save done): N bytes in M writes, fflush ok, fsync ok`.
- `log.txt` is fsynced in batches (at most every 2 s, right after any `[SAVE]` line, on HOME/sleep, crash, hold and exit) so the last lines survive a forced close. First sync logs `[LOG] log.txt fsync active`.
- Rolling save backups (`.bak1`, `.preimport`) are fsynced before the rename.
- No change to the save format or bytes; `soulsilver.sav` stays a plain 512 KiB image. No toggle: it only changes when data reaches the card. Idea from GabeSud0's pokeplatinum-3ds.
### Shiny-hunt reset helpers (2026-10-25, untested; not in any build yet)
- Settings > Battle > **SELECT = reset** (`select_reset=1`, default OFF): tap SELECT in a wild battle (at the FIGHT/BAG menu) for the same soft reset as L+R+START+SELECT. Never in the overworld, menus or trainer battles, and ignored when the wild Pokemon is shiny. Ported from 3ds-platinum runs 96/98.
- Settings > Battle > **Skip intro on reset** (`reset_skip_intro=1`, default OFF): after a soft reset the game goes straight to Continue / New Game (no logos, intro, title or credits splash). New game hook in NitroMain (pspoke patch 0070, starts the game's own save check like START on the title screen; no key injection). If the hook can't be applied the build still links and log.txt says `[BOOT] skip intro hook NOT applied`. Ported from 3ds-platinum runs 95/97.
### Wide party, bag and summary (2026-10-25, untested; not in any build yet)
- Settings > Display > Wide party/bag (`wide_menu_screens=1`, default OFF): the party menu, bag and Pokemon summary (also when opened from a battle) continue their backgrounds into the 400 px sides with the cheap wide_menus strip renderer (no Wide FX). `ss3ds_glue.c` detects them from the overlay manager templates `gOverlayTemplate_PartyMenu`, `gOverlayManagerTemplate_Bag`, `gOverlayTemplate_PokemonSummary`. Needs Wide view on and Screen scale 1.25x. Logs `[WIDE] menu screen none -> party ...` and `[WIDE] wide_menus avg ...`. FPS counter moved from the Display to the System tab to make room.
### 60 fps overworld study (2026-10-25, log only, untested on device)
- `docs/FPS60.md`: why the field is 30 fps (the game always spends two VBlanks per tick), how pokeplatinum-3ds makes in-between frames, what it would take here, and why it was not implemented blind.
- `fps60_probe=1` in config.txt (default 0): `[FPS] field 10 s` busy/idle per tick and `[FPS] probe` triangle-order stability. No behaviour change.

### Bottom menu (2026-10-25, untested; not in any build yet)
- Settings > Controls > Bottom menu (`bottom_menu=1`, default OFF): in the overworld the touch screen shows tiles for the Start-menu entries (Pokedex, Pokemon, Bag, Pokegear, Trainer card, Save, Options). A tap presses X and then selects that entry through the game's own Start menu (pspoke patch 0060 hook in `start_menu.c`), so the real Pokemon / Bag / Save code runs. While the tiles are up the game gets no touch. The real menu hides them. Entries the game does not offer yet are greyed. Logs `[BMENU]`.

### run 2 (2026-10-25)
- Fix (hardware): title-screen Lugia. The run-1 PICA W-buffer looked right in Azahar but not on a real New 3DS. W-buffer frames (title, brief transitions) now draw on the CPU 3D renderer, which matches the DS reference exactly; normal Z-buffer gameplay stays on the GPU. Title runs slower (~4-5 fps in emulator) in exchange for correct depth.
- Settings > 3D > W-buffer depth: OFF / CPU 3D (default) / PICA W (run 1 behaviour) / GPU Z (experimental)
- CPU renderer: near-plane fix for w<=0 vertices (removes streaks on the title water)
- Debug: `g3_shot=N` in config.txt dumps 8 raw 3D frames from frame N; `tools/emu_smoke.sh` accepts a CIA
- Credit: Platinum 3DS port (Giratina fixes, runs 23/24/27/31: TEXGEN 2/3, shadow-poly skip, vector-matrix wrap, linear w depth) - already present here, checked and kept

### run 1 (2026-10-25)
- Fix: crash on a field fade (first entry to map 67): DISPCNT read through an unmapped address (pspoke patch 0053)
- Fix: from-scratch build link error (sub_0208B278, patch 0052)
- Fix: title-screen Lugia depth (spikes floating, holes) with the PICA W-buffer; Settings > 3D > W-buffer depth (default on)
- Version string "run 1" in log header, settings app and HOME Menu metadata
- CIA icon from your ROM's DS icon at build time (as in the Platinum port); headless emulator smoke test `tools/emu_smoke.sh`

- GPU blend is on by default and its System-tab test toggle is removed (`gpu_blend=0` in config.txt turns it off)
- README rewritten (build, install, SD layout, settings tabs, known issues, credits) and RELEASE_NOTES.md added

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

## Faster battles and battle info (2026-10-25, pspoke-patches/0030)
- Settings > Battle > **Faster battles** (`battle_anim_speed`): Skip moves plays plain move animations through the game's own "battle animations off" path (damage, HP bar and text unchanged; Fly, Dig, Dive, Bounce, Shadow Force, Transform, Substitute and Minimize still animate); Skip + fast HP also drains the HP bar 3x faster. Log: `[BATTLE] move N animation skipped`.
- Settings > Battle > **Catch info (Y)** (`catch_overlay`): in a wild battle press Y at the command menu for species, nature, ability, IVs, status, Pokerus and catch odds per ball; a "SHINY POKEMON!" toast appears when the foe is shiny. Log: `[CATCH]` lines. Read-only, never touches the save.
- Settings > Battle > **Encounter counter** (`hunt_counter`): wild encounters per species in `sdmc:/3ds/soulsilver3ds/hunt.txt` (never the save), shown in the Y panel.
- The game hooks live in `scripts/ss3ds_hooks.d/` (sourced by `ss3ds_game.sh`, idempotent edits of the staged decomp).
- Fix: the GPU blend row index in the settings app pointed one past the last row (73 instead of 71), which tripped its own `static_assert`.
