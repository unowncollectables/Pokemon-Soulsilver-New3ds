# Changelog

All notable changes to this project will be documented here.

## Unreleased

### Cleanup after the Lugia fix (2026-10-26, not compiled, not device-tested)
Grok's clip-matrix fix (patch 0095) is what fixed the title Lugia, so the old depth experiments are gone:
- GPU SPLIT depth mode removed (g3soft.cpp SplitW, its 1.2 MB vertex buffer and its Settings option).
- Debug > CPU 3D W-frames (`g3_wcpu`) removed; `g3_wpersp` removed (the CPU renderer, only used when GPU 3D is off, always draws W-buffer frames perspective-correct).
- Settings > 3D > W-buffer depth row hidden. `g3_wmode` still works from config.txt (1 PICA W default, 2 GPU Z), so an existing config keeps its value.
- `[WSUM]` and `[LUGIA]` per-frame title diagnostics removed (Smooth title 3D keeps its one `[LUGIA] smooth title 3D` line).
- The extra boot.txt breadcrumb logger removed (one line per boot step and thread start). The crash handler still writes registers to boot.txt when a thread crashes; log.txt is unchanged.
- Old keys `g3_wcpu` / `g3_wpersp` in config.txt are harmless: the `[CFG]` header flags them DEAD.

### Run 24: title-screen Lugia drawn correctly (2026-10-26)
- The title Lugia's neck, body and belly no longer come out thin and broken: each vertex now uses its own joint's matrix (G3 lazy clip-matrix bug, pspoke patch 0095). Matches DeSmuME.

### 60 fps work, stage 1b: GPU 3D DIRECT with Wide view (2026-10-26, not compiled, not device-tested)
- Nathan's hardware log with GPU 3D DIRECT on: 432 of 600 3D frames went direct (G3G_End 3.4 ms vs 7.7 ms), but every Wide-view frame (344) fell back, and 306 frames still pulled the 3D back to the CPU (~6.5 ms), so the frame time stayed ~33 ms. 3D looked right.
- Wide view (the normal 400-px overworld, not the slow Wide FX) now also goes direct: after the draw the GPU copies the 256 centre columns of the 320x256 eye target into a 256x256 compositor texture (GX texture copy, tiled to tiled) and queues the usual readback; the CPU converts only the two 32-px side strips just before present, not the 256x192 centre. Wide FX on keeps the old path. Same Debug toggle; `gpu_direct3d_wide=0` in config.txt turns off only the wide part.
- Logging: the stats line now has a wide count and a separate "wide_fx/wide-off/no-mem" reason, and a second line `[GPU3D] direct resolves by reason: ...` counts every CPU pull-back by cause (flat stereo shift, CPU right eye, CPU composite, compose rejected, dump/diag, wide haze fit, g3_shot) plus strip conversion time. The first pull-back of each cause is logged once with its reason.

### Run 22: no old bottom menu flash on load (2026-10-26)
- After loading a save the touch screen stays blank (dark blue) for the moment before the game has handed over its icons and your position, then shows the map, ROM icons and Sprint Button. The old drawn fallback icons and the "LOCATING... WALK AROUND" panel no longer appear in the JOHTO layout.

### Run 21: clean wide Pokedex sides, map right after loading (2026-10-26)
- Wide Pokedex/PC: the side areas rebuild the screen's background (e.g. the Pokedex grid paper, lines every 40 px) instead of a mirrored, blurred copy, so no smeared red bars or ghost text. A band continues only where it really reaches the screen edge; panels just end at the edge.
- The bottom screen shows the map at your position, the ROM icons and the Sprint Button as soon as a save loads (pspoke patch 0094), not only after the first step.

### Run 20: the game's Sprint Button on RUN (2026-10-26)
- The JOHTO layout's RUN square shows HeartGold/SoulSilver's own Sprint Button (grey tab and shoes = off, orange = on), read from your ROM at runtime (a/0/1/4: palette 7, cells 68, chars 70; cells 4+12 off, 5+8 on), cropped to the square, with an orange outline. Before the game gives the Running Shoes the old icon stays. Toggle behaviour unchanged.

### Run 18: sharper JOHTO button column (2026-10-26)
- The JOHTO layout's eight buttons and the item/RUN squares are drawn at the 3DS's native 320x240 resolution (no stretch blur), styled after GabeSud0's pokeplatinum-3ds lower menu: outlined boxes with a highlight row and shading, an icon cell and divider, a blue tab, a small crisp font, and a blue pressed state. Since the follow-up it uses GabeSud0's frame itself (his colours, geometry, green pressed state, red tabs, his MAP icon on the JOHTO button) around our map, with the start-menu icons read from the SoulSilver ROM. Toggle: Settings > Display > "Bottom menu HD". The wide rows moved to a new "Wide" tab, which adds Wide Pokedex and Wide PC boxes.

### Run 17: ZL/ZR press detection (2026-10-26)
- ZL run toggle and ZR item shortcut use their own press detection (a cleared hidKeysDown could drop the press), and the first ZL/ZR press is logged for device diagnosis.

### Run 16: map stats no longer rebuilt after every battle (2026-10-26)
- The JOHTO map's progress tables survive the field overlay reload after a battle (pspoke patch 0092), removing a hitch on the first step after each battle.

### 60 fps work, stage 1: gpu_direct3d (2026-10-26, untested, not compiled, default OFF)
- New Debug-tab toggle `GPU 3D DIRECT` (`gpu_direct3d=1`). The 3D layer stays on the GPU: `G3G_End` queues the draw and does not wait, no readback DMA, no CPU convert, no CPU re-swizzle and upload; `gpucomp.c` samples the eye render target (`rt[e]`) as the 3D-layer texture. Anything that needs the pixels on the CPU (CPU composite, GPU-compose rejects, stereo fallback, dumps) triggers `G3G_DirectResolve()` which reads back on demand. Expected saving ~12 ms per 3D frame when the GPU compositor takes the frame. 3D colour is 8 bit instead of 6 bit expanded.
- Only for plain 256x256 frames with `gpu_compose=2`, rb_sync and no CPU fog; wide strips, 2x title Lugia, async and verify frames use the old path. Log: `[GPU3D] direct 600 3D frames: ...`.

### Frame-rate work and cleanup (2026-10-26, PRs #67-#69, untested, not built)
- JOHTO/tiles bottom overlay is repainted only when it changed and the unchanged bottom upload is skipped.
- With the 3D slider on, battle and menu frames are GPU-composed for both eyes, and the converted 3D-layer texture is reused per eye outside the overworld.
- Wild battle crash (raw DS sub-engine DISPCNT read at 0x04001000): patches 0054, 0090 (re-applies the rewrite to the staged decomp on every build, fails the build if a raw read remains) and 0091 (re-syncs a stale staged prepare.py). Built in 436bde7.
- docs/SPEEDUPS.md: how pokeplatinum-3ds reaches 60 fps and ranked speedups for SoulSilver. gpucomp.c now includes stdlib.h for calloc.

### Bottom menu toggle + settings overlap fix (2026-10-26, untested, not compiled)
- Fixed: the JOHTO icon column (GPU overlay) stayed on the touch screen on top of the L+R+Y settings screen; the overlay is now cleared when settings opens.
- Fixed: Settings > Controls > Bottom menu did nothing while Bottom layout was JOHTO (the layout alone showed the screen). Now Bottom menu ON/OFF is the master switch; OFF = stock touch screen with the small menu button.
- `bottom_menu` now defaults to 1 (ON), so the JOHTO screen stays as it was. Bottom menu ON/OFF is the first row of the Controls tab (L/R to reach the tab, A to flip).

### Audio cleanup (2026-10-26, untested, not compiled)
- No more drone when the game stops: if the game makes no sound update for ~0.5 s (long load, crash screen, menu with Menu music off) the held notes fade out and fade back in when it continues (`audio_stallfade`, Settings > Debug > Audio stall fade, default on).
- No more permanent audio freeze if the DSP stops taking buffers: the output wait is now bounded (1 s with a frozen DSP position, outside HOME/sleep), then the channel is cleared and sound restarts (`[AUDIO] dsp stall` line).
- Settings > System > Volume (55..145, 100 = the current default gain 7), live. Audio limiter toggle on the Debug tab, live.
- The `[AUDIO] 10s` line now shows the real output: peak in/out, grains still soft-clipped after the limiter, samples at the rail, limiter minimum, faded grains, DSP stalls and the longest buffer wait.

### JOHTO bottom map, run 16 (2026-10-26, untested, not compiled)
- JOHTO is now the default bottom layout (`bottom_layout` default 1). The Bottom menu itself stays OFF by default (`bottom_menu=0` = stock game).
- The map slides smoothly to your new location when you change area instead of jumping; dragging still pans.
- Info card: tap the card to flip to a wild Pokemon list for that place (land/surf/rock smash/fishing species from the ROM's encounter tables, caught ones in green, "CAUGHT n/m"). pspoke patch 0084 publishes the species set hook 0083 already builds; names are public names in `p3ds_speciesnames.h`.
- Kanto is hidden on the map until you have been to a Kanto place (Settings > Controls > Kanto on map, `pg_kanto` 0 GATED / 1 ALWAYS).

- Version string is now "run 15 + 3D banner default"; docs updated (3D banner default and tested, single CIA, no TEST CIA).

### 3D banner: made logo, now the default (2026-10-26)
- Logo v4: sharper (stronger sharpening, heavier outline, more contrast in the chrome).
- Logo v3: SoulSilver colours (silver, steel blue, navy, white); the gold burst is replaced by a navy body with a silver rim and Lugia-style feather wings, ocean swell lines and white glints. Same card and texture size.
- Logo v2: fancier (bevelled chrome lettering with gloss and drop shadow, gold inner line, glowing burst, sparkles, glossy POKEMON and NEW 3DS), still the same single card and texture size.
- HOME Menu banner logo redrawn Platinum-style (small POKEMON, big slanted chrome SOULSILVER on a gold-rimmed burst, NEW 3DS tag; tools/banner3d/dev/make_logo.py). Same card, texture size and CGFX structure as before, so the Platinum structure check is unchanged.
- `make cia` now builds the 3D banner by default (BANNER=3d); no ROM, a failed structure check or any error still falls back to the flat banner, and the [BANNER] RESULT line says which was built. BANNER=flat forces flat.

### Run 14: labelled map stats (2026-10-26)
- Info cards and the progress overview label every stat (Caught, Seen, Trainers, Items, Hidden) instead of using small icons.

### Run 13: map progress overview and per-route stats (2026-10-26)
- JOHTO map: menu-dots button opens a progress overview (visited, species caught/seen, trainers beaten, item balls and hidden items found, whole map and per visited place). Info cards show the same stats per place. Read-only from game flags in RAM (pspoke patch 0083); the save is never written. The Pokedex menu icon is no longer clipped.

### Run 12: menu icons without the green backdrop (2026-10-26)
- The JOHTO layout's Start-menu icons show just the symbol (backdrop removed by flood fill from the tile edge), recentred 1:1 in each button.

### Run 11: JOHTO bottom layout uses the game's own icons (2026-10-26)
- Start-menu icons (Pokedex, Pokemon, Bag boy/girl, Pokegear, Trainer card, Save, Options), the Pokegear player head (Lyra/Ethan) and the registered item's icon are read from the user's ROM at runtime (pspoke patch 0082, hook 0081) and drawn 1:1 with nearest filtering in a native 320x240 overlay after the bottom-screen stretch (`gpuscale.c`). The drawn icons remain the fallback (scale mode 0 or a failed load). Still drawn by the port: running-shoe and map-view icons, panel frames.

### 3D banner rebuilt the Platinum way (2026-10-26, untested on a HOME Menu; flat is still the default)
- Found: the "483,788 byte CGFX" that froze HOME was the whole `.bnr` (CGFX 315,400 + jingle); Platinum's working `.bnr` is 486,844. Size was not the cause. What differed from Platinum: four animated 16x16 sparkle cards, 14 meshes / 8 textures / 9 materials (6 translucent) against Platinum's 11 / 4 / 5 (2), and 2,056 triangles against Platinum's 1,000. Details in `docs/HANDOFF.md`.
- New 3D banner = Platinum's scene: portal, Lugia (ROM, build time), logo, a hover bob like Giratina's; no sparkles; Lugia reduced to Platinum's triangle budget (`tools/banner3d/decimate.py`, edge collapse that never touches seams or the outline). CGFX 259 KB, 1,000 triangles, 4 textures, 5 materials.
- `tools/banner3d/cgfx_fingerprint.py` + `reference/platinum_banner.json`: structural fingerprint of Platinum's tested banner (object counts and every setting/flag/format value, no game data). `make_banner.py` checks each banner against it (and against Platinum-sized limits) and fails into the flat banner on any unjustified mismatch. `--fingerprint-out` writes a banner's fingerprint; `--allow-mismatch` builds experiments anyway (never used by the Makefile). Tests: `python3 tools/banner3d/test_banner3d.py`.
- `BANNER=3d` is now this banner (needs the ROM), `3d-art` is portal + logo only, `3d-lugia` = `3d`.
### CIA banner: flat by default (2026-10-26)
- The 3D Lugia banner froze the HOME Menu on the icon. `make cia` now defaults to the flat PR #17 banner and chime; `BANNER=3d` / `BANNER=3d-lugia` are opt-in, validated (size/mesh/texture/vertex limits) and fall back to flat. Loud `[BANNER]` lines say what was built.

### Bottom menu redesign + mini map (2026-10-26, untested on device)
- The Bottom menu layout is new: a header bar with the location name and X/Z, a map panel, and two rows of four tiles (Pokedex, Pokemon, Bag, Pokegear / Trainer card, Save, Options, Map). Everything sits inside the 256x192 touch screen with 4 px margins; the tile hit boxes and the drawing share one geometry function. Labels fall back to short forms (DEX, PKMN, GEAR, CARD, OPTS) if they would not fit. Same colours as the settings app / Pokegear. `bottom_menu` still defaults OFF; OFF is the stock game.
- Mini map panel, Settings > Controls > **Bottom map** (`bottom_map`: 0 INFO, 1 WORLD default, 2 LOCAL); tapping the panel or the MAP tile cycles it. INFO = location name, X/Z, map id and type. WORLD = every map-matrix cell you have stood in, coloured town/route/cave/indoors, with a blinking player marker; kept in `minimap.bin` on the SD card (not the save). LOCAL = the path walked on this map, centred on you.
- Data comes from new pspoke patch 0080 (`field_control.c`, in FieldSystem_ProcessStep, read-only, non-fatal if it does not apply): map id, map-section name (ASCII, from the game's own map-name messages), player tile, matrix cell. No ROM art is read and nothing touches the save. Logs: `[MINIMAP]` (source, map, name, cell, new cells, mode changes) and `[BMENU] layout`. `tools/minimap_preview.cpp` renders the layout on a PC.
- Not done: the Pokegear/Town Map picture itself (tile-sheet NSCR decode, would need a ROM to verify); a tile-by-tile collision map (needs the assembly collision accessor).

### Lugia title fixes (2026-10-26, untested on device)
- W-buffer depth = GPU Z: Lugia's skinned parts now use the same depth scale as the rest of the model (they sorted wrongly: spikes over the wing, holes).
- Smooth title 3D falls back to colour + depth in linear RAM when VRAM is too fragmented (it failed on hardware in run 7).

### Smooth title 3D (2026-10-26, untested on device)
- Settings > 3D > **Smooth title 3D** (`g3_smooth`, default ON): the title-screen Lugia is drawn at 2x and scaled down by the GPU (4x supersampling), so its edges are smooth instead of blocky. Only the title (DS W-buffer frames) uses it; other scenes stay at 1x. Log: `[LUGIA] smooth title 3D ... ON`, `[3DRES] eye targets now 512x384`.

### Lugia title diagnostics (2026-10-25, log only)
- `[WSUM]` adds `ssum` (hash of the title's skinned/CPU-path vertices) and `trans`/`nodw`/`eq` vertex counts, so one cold-boot hardware log can be compared with Azahar to tell bad model data from bad GPU state.

### HOME Menu banner: Lugia from your ROM (2026-10-25, untested on device; not in any build yet)
- `make cia` now puts the title screen's Lugia (model, textures, wing beat) in the 3D banner when your ROM is at `roms/SoulSilver.nds`, built at build time like the Platinum port's Giratina and never committed. No ROM, a different ROM, or any extraction/scene/convert failure falls back to the art-only 3D banner, then to the flat one. See `tools/banner3d/README.md`.

### Logger sweep and SoulSilver wording (2026-10-25, untested; not in any build yet, no behaviour change)
- Crash/boot/error screens said "Platinum 3DS": now "SoulSilver 3DS". The crash screen also shows the version, type, pc/lr/sp/far, uptime, field/battle state and the last 3 log lines; `[CRASH]` log lines carry version, uptime, free memory and `g3_wmode`. `README_settings.txt`, the CIA rsf comment and the GF_ASSERT addr2line hint no longer name Platinum paths.
- log.txt header: `[CFG]` block (version, build time, every config.txt key with default / NON-DEFAULT / DEAD / UNLISTED flags, effective values). Dead keys flagged: `g3_wdepth`, `wild_vis*`, `dw_*`, `item_ping`, `touch_menu`, `bottom_strip`, `cheat_pp/speed/walk_walls`, unused `follower_*`.
- New one-line-per-event tags: `[BATTLE] start/end`, `[QUICK]` (ZL run, ZR item, L+R+X), `[REPEL] repel check`, `[WIDE] battle entered`; `[MENU] config.txt: key=N (saved)` / `WRITE FAILED`. `[USE]` feature-usage summary at clean exit and in a crash. See docs/HANDOFF.md "Log tags".
### Lugia title: default back to PICA W (2026-10-25)
- GPU SPLIT looked no better on hardware and was very slow, so `g3_wmode` defaults to 1 (PICA W) again. GPU SPLIT stays in the menu.
### Lugia title: GPU SPLIT W-buffer depth (2026-10-25, untested; not in any build yet)
- Settings > 3D > W-buffer depth gains **GPU SPLIT** (`g3_wmode=3`), now the default. W-buffer frames (the title) are drawn by the GPU with the ordinary Z-buffer every other 3D scene uses on hardware: depth linear in w, and every triangle whose w varies by more than 3% is cut into 4 (up to 4 levels), so the GPU's screen-space depth interpolation stays accurate. It does not depend on the PICA W-buffer mode, which looked right in Azahar but not on hardware (run 1). PICA W / GPU Z / OFF remain selectable; configs that saved `g3_wmode` keep their choice.
- GPU 3D vertex buffer 24K -> 48K vertices per eye (+2 MiB linear RAM) for the extra triangles; splitting never uses the last 6K slots of a frame, so it cannot make other triangles drop.
- Log: `[LUGIA] ... depth code 4 ... split verts N -> M, max level L, over budget K` every 60 frames; `over budget` > 0 means some triangles were left unsplit.
### Lugia title: CPU W-buffer renderer made perspective-correct (2026-10-25, untested; not in any build yet)
- Finding: the "CPU 3D" renderer used for W-buffer frames (run 2, now Debug > CPU 3D W-frames, `g3_wcpu`) interpolated depth, UV and colour affinely in screen space. A DS W-buffer needs depth = w per pixel (1/w is what is affine on screen), so run 2 repeated the same depth error as the GPU path and could not prove that depth is not the cause. "Both renderers look the same on hardware" therefore does not rule depth out.
- Fix: W-buffer triangles on the CPU renderer now interpolate 1/w and attr/w and divide per pixel (depth = w like the DS, perspective-correct textures and colour). Config `g3_wpersp=0` brings back the old affine raster for A/B. Only W-buffer frames (title) change; Z-buffer frames and the GPU path are untouched.
- Logging: `[WSUM]` adds `psum` (object-space position+UV only, comparable between hardware and emulator), w range, behind-camera and CPU-path vertex counts; `[LUGIA]` names the renderer used for W frames every 60 frames.
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
