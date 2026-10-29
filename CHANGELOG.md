# Changelog

All notable changes to this project will be documented here.

## Unreleased

### Union Room two-console fixes (patch 0133, not compiled)
- Run 56, two consoles, Local wireless ON: they saw each other and talked, but the child's join failed 4 times with `uds: connect target not in the scan list` (the game said the other player was busy). The Union Room host restarts its network every second or so, so it is missing from some scan snapshots. `uds_ctru.c` now keeps a 20 s last-seen beacon cache for connecting, retries `udsConnectNetwork` up to 20 times 150 ms apart and rescans between tries (logs `using its cached beacon`, `connect attempt N failed`).
- Then a crash at the end of the conversation: the wireless-icon teardown `sub_0203A444` (inlined into translated `sub_0203A914`) writes DS OAM at raw 0x07000000. Patch 0133 provides it natively (task destroy + free, no OAM writes), logged as `[COMM] wireless icon teardown`.

### Wireless icon redraw crash (patch 0132, not compiled)
- Run 55 with Local wireless ON, Union Room, on two consoles: both hosted 6-7 UDS networks, then crashed at the same spot (`native_sub_0203A700+0x1dc`, `str r1,[r6]` with r6 NULL): the translated wireless-icon redraw reads DISPCNT/POWCNT via raw DS addresses and writes DS OBJ VRAM through a NULL bank pointer. It only draws the icon, so it is now provided natively as a logged no-op (`[COMM] wireless icon redraw skipped`). No partner joined before the crash.

### PHONE tab: MAP chip, whole row calls (run 53 feedback)
- Run 53 log: four contact taps all logged "tap X -> map location", never a call. The binary had the call code, but the part of a row that jumped to the map was the whole place-name area (up to half the row), so taps landed there. Each row now has a small MAP chip at the right end for the map jump; everywhere else on the row calls.

### Comm stack duplicate-symbol fix (patch 0131, not compiled)
- Run 53 link (0 undefined after the DWC stand-ins) had 27 multiple definitions: 21 functions the decomp already compiles (`unk_0202C034`, `unk_0202C730` incl. `Save_FriendGroup_Get`, `unk_02030A98`, `unk_02034354`, `unk_020379A0`, `unk_02068FC8`) were translated a second time by the 0127 closure, and 6 `Missing()` traps (`ov00_021E5CA0`, `021E69A8`, `021E6A4C`, `021E7130`, `021E7314`, `sub_020588DC`) are now defined by the translation. Patch 0131 lists the 21 in `native-provided.json` (the translator calls the compiled ones) and drops the 6 Missing lines; `ss3ds_game.sh` re-syncs the staged `native-provided.json`.

### Comm stack link fix (not compiled)
- Nathan's link of main with #188/#198 had 105 undefined symbols: the translated comm core (`unk_02037C94`) also reaches the Nintendo Wi-Fi Connection glue (ARM-mode DWC functions in `overlay_00_arm.s`/`overlay_45_arm.s`, which the Thumb-only translator can't do, plus Thumb pointer-table targets in `overlay_45_thumb.s` and `unk_0208F814.s`). New `ss3ds_dwcstub.c`: 104 weak stand-ins that log `[COMM] DWC stub <name>` on their first calls and return 0 ("unavailable"). Local wireless doesn't use them; online WFC is a separate project. The 105th (`sub_0205DE88`) is fixed by the run-thread PR.

### Comm stack build fix (patch 0130, not compiled)
- Nathan's builder failed at ss-maploader: the translated comm/Union Room closure calls libc (`strcmp` from overlay 45, plus memcmp, strlen, strncmp, strcpy, strcat, abs), and the generated `extern uint32_t strcmp(uint32_t,...)` clashes with devkitARM's string.h. Patch 0130 routes those names through compiler builtins like memset/memcpy already are; `ss3ds_game.sh` now also re-syncs the staged `port.py`.

### Pokegear RADIO tab: tap a station to tune it (patch 0129, not compiled)
- Tapping a station row (with the Radio Card) opens the Pokegear on its radio app already tuned to that station; the game's own radio show code plays it. `Radio_Start` puts the dial cursor on the station's static-free hitbox before it reads the tuned station. No Radio Card = a toast and no call into the game. Toggle: Settings > Input > Pokegear tabs. Logs: `[RADIO] tune station N`, `[RADIO] Pokegear opened on the radio to tune station N`, `[RADIO] tuned station N (dial area A, cursor x,y)`; build log `ss3ds 0129 radiotune hook NOT applied` = taps only open the Pokegear.

### Pokegear PHONE tab: tap a contact to call (patch 0128, not compiled)
- Tapping a contact row now places a real call: the platform asks the game's Start menu for the Pokegear (like the POKEGEAR tile) and sets `P3DS_PhCallReq`; hook 0128 opens the Pokegear on the phone app and jumps to the app's own Call state (`PHONE_MAIN_STATE_CLOSE_SUBMENU_BEFORE_CALL`) with `callerID` set, so the game's call code, scripts, text and voice run unchanged. Tap the place name at the right of a row for the old jump to the map. Toggle: Settings > Input > Pokegear tabs. Logs: `[PHONE] call <name>`, `[PHONE] Pokegear opened on the phone to call contact N`, `[PHONE] calling contact N from the PHONE tab`; build log `ss3ds 0128 phonecall hook NOT applied` = taps only open the Pokegear.

### FAST 3D: crisp mono (2026-10-28, not compiled)
- Run 54: with the 3D slider at 0 and SHARP 3D on, the 2x box downscale made the game's blocky textures "a tad mushy". With SHARP 3D = FAST 3D and the slider at 0 (30 mono frames in a row) the 3D now draws at plain 1x, the crisp picture of SHARP 3D OFF; the first stereo frame goes back to 2x left eye + 1x right eye. SHARP 3D = ON still draws 2x everywhere. Log: `[3DRES] FAST 3D: mono (slider 0) draws at ...`.

### SHARP SCALING removed (2026-10-28, after run 54)
- Nathan's photos: nearest sampling of the 1.25X stretch makes text and button labels ragged (some pixel columns one pixel wider), so the toggle is gone again (reverts #207). A true sharp-bilinear needs a per-pixel fractional-position blend; the PICA has no fragment shader, and the existing 2x nearest prescale + LINEAR stretch already blends only the one column in five that straddles a DS pixel edge. Genuinely crisper 1.25X needs the 3D and 2D composed at final size (docs/FPS60_STEP2_DESIGN.md).

### Sharp 3D FAST 3D mode for the 3D slider (2026-10-28, not compiled, SHARP 3D still default OFF)
- Settings > 3D > SHARP 3D now has a third state, FAST 3D (`g3_sharp=2`). Run 53: stereo SHARP 3D cost ~6 ms GPU per eye at 2x, run back to back while the CPU waited (field 23-25 fps, battle intros 14-16). In FAST 3D the left eye is drawn at 2x and box-downscaled as before, the right eye is drawn at 1x in a 192-row viewport inside the same 2x target and copied out unscaled (`rbQueue`, `r1xCalibrate` in g3gpu.c). About 4-5 ms less GPU fill per stereo frame; the right eye has plain (not supersampled) edges.
- The first frame that draws something copies both 192-row halves of the target and keeps the one that differs from the clear colour, so no guess about where the PICA puts a small viewport is baked in. Logs: `[3DRES] FAST 3D: right eye 1x lands in half N`, and `[PERF] sharp 3D stereo, both eyes 2x | FAST 3D (right eye 1x): N frames, G3G_End avg X us ... CPU convert avg Y us` every 300 stereo frames (compare the two lines for the saving).

### Sharp 3D: stripes fixed at the source? (2026-10-28, after run 52)
- Run 52 photos: SHARP 3D showed horizontal stripes and a split, shifted picture whose lower half did not refresh. The 2x2 hardware downscale was given the half size as its output dimensions; the hardware (and citro3d, which passes the same dimension for input and output) wants the pre-scale size, so it only converted part of the picture and left old rows behind. Now passes 2*w x 384. Untested.

### Union Room / comm stack: translate the real asm instead of hand-porting (patch 0127, not compiled, UNVERIFIED translator run)
- Chunks A-D hand-ported ~30 functions of an ~220-function asm-only comm stack. Patch 0127 instead seeds the 71 remaining `Missing()` comm/Union Room entry points and the 61 hand-ported names into the maploader translator (`seeds.json`), so pspoke's Thumb-to-C translator emits the real code (closure over unk_02032844, 02033AE0, 02034B0C, 02035900, 02037C94, 0203A3B0, 02058AEC, 0205A44C) and the matching `Missing()` lines are dropped. The translated comm layer calls the DS `WM_*` API, which `wm_uds.c` already backs with 3DS UDS when Settings > Game > Local wireless is ON (OFF = nobody in range).
- `ss3ds_union.c` is removed (its stand-ins would duplicate the translated functions). `sub_02037D78`, `sub_02038D90`, `sub_02039AA4` in `ss3ds_glue.c` become weak so the translated versions win. `ss3ds_game.sh` re-syncs the staged `seeds.json` and redoes the game compile.
- The translator is fail-closed: an unsupported instruction or unknown call signature in this code stops the maploader step. That has not been run yet; see the PR.

### Union Room chunk D (2026-10-28, not compiled)
- Run 52 (guard OFF): the room loaded and walked fine, no crash. The log shows the comm teardown stub `sub_0205A1D4` running hundreds of times (every frame after leaving the room), so the manager was never freed. Ported the real teardown: destroy the manager task, free it and its heap, clear fieldSystem+0x80; logs `[UNION] manager ... torn down` once. Still no comm session (real wireless needs the comm core ported).

### Sharp 3D: stripe fix attempt (2026-10-28, after run 51)
- Run 51 photos: with SHARP 3D on, far ground and battle grass showed horizontal stripes. Best guess: depth-EQUAL overlay batches miss the ground's depth on alternate rows at 2x. At 2x those batches now draw LEQUAL with a tiny depth offset. Untested; only active with SHARP 3D on.
### Union Room chunk C (2026-10-28, not compiled)
- Run 51 (guard OFF): the room map loaded, then a null read in `sub_02033250` (called by `FieldInput_Process_UnionRoom`): the comm block pointer `_021D4124+4` was never set. The comm init stand-in now points it at a zeroed 0x2000 block (no link flags), logged as `[UNION] comm system block ...`. Test: guard OFF, enter the Union Room, send log.txt and log_crash.txt.

### Sharp 3D: wide view and black screen fixes (2026-10-28, after run 50)
- Run 50 hardware log: leaving 2x for a fog frame freed the 2x target but the RGBA8 scale targets no longer fitted (148 `scale targets now RGBA8: FAILED`, frames not presented), and one failed 320x256 target turned wide view off for the whole session. #178 frees the scale targets first and falls back to 16-bit instead of none; wide view is now retried after every 2x change.

### Sharp 3D: room in VRAM for the 2x target (2026-10-28, not compiled)
- The 2x 3D target only works in VRAM, and VRAM was full: the three 512x512 RGBA8 screen-scale targets (gpuscale.c `up[]`) take 3 MiB of 6. While a 2x target is wanted (SHARP 3D, or the title's Smooth 3D), they are freed, the 2x target is made first, and they come back as RGB565 (1.5 MiB; red/blue lose their lowest bit, the DS 2D is 5-bit anyway). Back at 1x they are RGBA8 again. If anything doesn't fit: 1x, RGBA8, toast "SHARP 3D: NO VRAM".
- The title's Smooth 3D only uses the 2x target while SHARP 3D is on; with it off the title stays on the 1x path it has always really used.
- Log: `[3DRES] scale targets now RGB565|RGBA8: ok|FAILED (VRAM free after N)` next to `[3DRES] eye targets now ...`.

### Sharp 3D: linear-RAM 2x target removed (2026-10-28, after run 49)
- Run 49 on hardware: with the 2x target in linear RAM (PR #169) the 3D took 50 ms a frame (14 fps) and showed garbage, and the title Lugia used the same target and broke. That pass is gone and linear depth is config-only (`res3d_lindepth=1`, default 0). The title's Smooth 3D and SHARP 3D now only use VRAM, and fall back to 1x when it doesn't fit (the Lugia title is back to how it was before #169).

### Sharp 3D in every scene (2026-10-28, not compiled, default OFF)
- New Settings > 3D > SHARP 3D (`g3_sharp`, live): the overworld and battles use the title's 2x 3D target with the hardware 2x2 box downscale (4x supersampled edges), mono or stereo. First step towards GabeSud0's native-resolution remaster display (pokeplatinum-3ds docs/REMASTER.md).
- Fog frames stay at 1x so they keep their fog; 30 eligible frames in a row before switching to 2x. If only 16-bit depth fits (the run 48 stripes) it goes back to 1x with a toast "SHARP 3D: NO MEMORY".
- Fix (cloud Azahar run of run 48): the 2x target never got made ("512x384 targets FAILED" with 18 MB linear RAM free). citro3d refuses a render target on a texture outside VRAM, so the "colour in linear RAM" fallback always failed; the title's Smooth 3D hit the same wall on hardware run 7. The target is now made from a tiny VRAM texture and pointed at the linear colour buffer.
- Log: `[3DRES] sharp 3D ON/off`, every 600 frames `[3DRES] sharp 3D: N of 600 frames at 2x, fog frames, depth ...`, plus the usual `[3DRES] eye targets now ...`. GPU 3D direct and frame reuse are 1x-only, so expect some fps cost; `[PERF]` shows it.

### Stale staged missing.c fix (2026-10-28, pspoke patch 0126)
- Run 46 linked with duplicate `sub_02037F94/FCC`: the live `ss3ds_game.sh` comes from the pspoke patches, so the earlier edit to the reference copy in `scripts/` never ran. Patch 0126 adds `nitromain-perf/missing.c` to the re-sync list in the live script (logged as `staged ... was stale, re-synced`); make rebuilds `missing.o` itself.
- Numbering note: two patches are numbered 0125 (Union Room chunk B and the Pokegear phone fixes). Both apply; do not rename them (PATCH RULE). New patches start at 0126.

### Union Room chunk B (2026-10-28, not compiled)
- Run 45 (union_guard OFF): the ported entry chain ran (`[UNION] manager created`) and the game data-aborted (far=0): `sub_02091574` (tail call of sub_0203410C, the comm command table) is compiled game C that dereferences the comm state `_021D4150`, which the comm-init stand-in does not create. It is skipped now (logged).
- Next crash it would have hit: `sub_0205AC88` (compiled) calls `sub_0205A1F0` and dereferenced its 0 result. Ported the small Union Room getters from the asm (`sub_0205A1F0/A1F4/A358/A478/A4B4/ABB0/AC4C`) and the poll functions the Union Room scripts use (`sub_0205A35C/A39C/A47C/A508`), which with no comm session answer 7 ("link lost") so the script ends. New comm driver stand-ins (weak where the staged core may already define them); pspoke patch 0125 drops the `sub_02037F94/FCC` traps.
- Still only reachable with `union_guard` OFF. Test: guard OFF, enter the Union Room, send log.txt (and log_crash.txt).
### X is a tile cursor, not a wave (run 49 feedback, not compiled yet)
- Nathan did not want the wave. With the Bottom menu tiles up, X opens a highlight (a second X closes it, like the game's menu toggle); D-pad right/down steps it over the usable tiles (Pokedex .. Options, MAP/JOHTO, Y ITEM, RUN), D-pad left/up steps back, **A taps the lit tile**, B or 6 s idle drops it. While the cursor is up A/B/D-pad do not reach the game. X no longer opens the hidden Start menu while tiles are up. Settings > Input > "X tile cursor" (`bottom_bump`, default ON until tested; remove once tested). Logs `[BMENU] tile cursor ...`. Pokemon-menu X/Y shortcuts (#151) are untouched.

### Pokegear tabs fixes from run 45 (2026-10-28, not compiled, untested)
- PHONE listed people you have not met: hook 0122 read `SavePokegear_IsNumberRegistered` as a yes/no, but it returns the slot (0xFF = not registered). New pspoke patch 0125 (hook `0125_pgphone2`, builds on 0122, never edits it) fixes the check, so only really registered numbers show.
- RADIO tab worked before you had the Radio card: it now says "No Radio Card yet" until the game's own gate (`Pokegear_GetRegisteredCardsArray & GEARCARD_RADIO`) is set.
- Incoming call: when the Pokegear rings, an amber "CALL: <name>  ANSWER" banner blinks over the bottom of the JOHTO panel (any tab) and the POKEGEAR button reads ANSWER. Tapping either is the POKEGEAR button: the game's Pokegear opens on the phone and answers (`FieldSystem_InitPokegearArgs_Phone`). Ring state comes from the same hook every field step. Logs `[PHONE] Pokegear ringing: caller N 'name'` / `ringing stopped`. If the hook is not applied (build log `ss3ds 0125 pgphone2 hook NOT applied`) the old behaviour remains. Respects the `pg_extras` toggle.

### Bottom menu: no old Start menu in the sub-screen gap (run 45 feedback, not compiled yet)
- Run 45 still showed the game's old Start menu for ~4 frames. The log shows the tiles never hide around a tap, except when the game drops the field flag to start the Pokemon/Bag/etc. overlay; that overlay rebuilds the touch screen a few frames (60-90 ms frames) later, and the old menu sat on the bottom screen in between. The tiles now stay up for 12 frames after a tile-launched sub-screen starts. Not proven on device.

### Sprite depth (2026-10-28, not compiled, untested)
- Settings > 3D > "Sprite depth" (`sprite_depth`, default OFF): in the overworld, small upright sprite quads (the following Pokemon, NPCs, the player; texture <= 64x64, <= 2 triangles) get ONE stereo parallax taken from their feet vertex instead of per-vertex parallax, so with the 3D slider up they stand on the ground at ground depth and no longer lean/shear between the eyes. GPU 3D path only (g3soft.cpp GpuRaw). The HGSS follower is a normal map object (follow_mon.c MapObject sprite), drawn by the same path as NPCs, so this applies to all of them. Logs `[SPRDEPTH]` (first use, then every 3000 quads: quads fixed/skipped, parallax spread removed, feet depth). Remove the toggle once tested.

### Pokemon-menu shortcuts and Auto run (2026-10-28, not compiled, untested)
- Hook 0123 (pspoke patch 0123): with Settings > Play > "Pokemon menu X/Y" ON (`pm_shortcuts`, default OFF), pressing X in the field's Pokemon menu opens the game's Move Reminder for the highlighted Pokemon (only when it has a forgotten level-up move), and Y opens the Move Tutor for the one tutor move it qualifies for (Draco Meteor for Dragon types; Blast Burn, Hydro Cannon or Frenzy Plant for fully evolved starters; not if it already knows it). No Heart Scale and no friendship check. Afterwards the Pokemon menu reopens on the same slot. Logs `[PMENU]`, counters `pm_relearn` / `pm_tutor`.
- Settings > Input > "Auto run" (`auto_run`, default OFF): the run toggle starts ON, so B walks and you run by default (the ZL toggle and the RUN tile still flip it). Logs `[RUN] auto_run`.
- Also fixed the Setup tab row count (declared 10, lists 9; the last row had no name).
- Already in before this change, so not duplicated: ZL run toggle, RUN tile, Y ITEM tile and ZR = Y, Bag/Pokemon tiles, reusable TMs, Repel prompt, IV/EV view.

- Move type hint (run 50): small 3x5-font badges in the gap under each move button (SUPER X2/X4 green, RESIST X1/2, X1/4 red, NO EFFECT grey), no longer over the move name.
- Move type hint reworked (Settings > Battle "Move type hint", default OFF): the tags are now drawn on the touch-screen move buttons (SUPER EFFECTIVE / NOT VERY EFF. / NO EFFECT, nothing for neutral) instead of a top-screen panel that just repeated the attack list. Tag position is a best guess for the HGSS button layout; the [HINT] log lines include a bottom-image colour profile to adjust it. Not compiled yet.

### Pokegear tabs on the JOHTO panel: MAP / PHONE / RADIO (2026-10-28, not compiled, untested)
- The JOHTO map panel's top bar now has three tabs. MAP is the existing Pokegear map plus pins on places where a phone contact lives (blue = number registered, amber = wants a rematch) and a "Phone:" line on the info card. PHONE lists the Pokegear's registered numbers (drag to scroll with a fling, drag the right edge to scrub, SORT chip A-Z / PLACE / REMATCH, tap a name = jump to that person's place on the map; "!" = wants a rematch, "G" = gift waiting). RADIO shows the stations you can tune where you stand (the game's own Radio_GetAvailableChannels logic) and what is on air this hour (the real rotations: Trainer Profiles / That Town These People, Search Party / Serial Radio Drama / Buena's Password, today's Pokemon Music track), from the 3DS clock.
- Real game data only, read-only, nothing written to the save. New pspoke patch 0122 (hook `0122_pgphone`, field_control.c, after 0083/0084): reads the save's phone book, the ROM's `tel/pmtel_book.dat` places and rematch/gift flags, and the radio state. If it does not compile the hook puts the pre-0122 file back and the panes say there is no data. Calling stays in the game's own Pokegear (POKEGEAR button).
- Toggle: Settings > Input > "Pokegear tabs" (`pg_extras`, default ON until tested; OFF = the panel is the plain map). Logs `[PHONE]` / `[RADIO]` (`[PHONE] phone book read: N entries, M placed`, `phone pane open: N contacts`, `tap <name> -> map location`, `radio pane open: area ...`). Code: `platform/ss3ds_pgextra.h`.
### Roamer + breeding tracker (2026-10-28, not compiled, untested)
- New toggle Settings > Play "Roamer + breeding tracker" (`tracker=1`, default OFF). On the JOHTO bottom map, each roaming Pokemon (Raikou, Entei, Latias, Latios: the game's four roamer slots; Suicune does not roam in HGSS) gets a coloured R/E/L/L mark on its Pokegear location. The progress overview (menu-dots button) gets a block at the top: roamers with level and place, then the Day-Care: parents, egg chance per 256 steps (70/50/20/0 %), steps until the next egg roll or "egg waiting", and Masuda status with the egg's shiny odds (5/8192 with a language mismatch, else 1/8192; the game's own PID rerolls, not `shiny_rate`).
- Data comes from new pspoke patch 0120 (read-only getters in field_control.c, every 16 steps and on a map change, only while the toggle is on) into `tracker.c`. Log tag `[TRACK]`, `[USE]` name tracker_publishes. Needs the JOHTO bottom layout to be visible.
### Automatic save backups and safe export (2026-10-28, not compiled)
- pspoke hook 0119 (`CARD_UnlockBackup`) calls `P3DS_SaveDone()`; main3ds.c makes a rotating backup 2 s after the last save transaction (`[SAVE] backup (after save): save count N crc ...`). Rotation is now `soulsilver.sav.bak1..8` (was 5), still only valid saves that differ from bak1.
- Settings > Setup > Export save: press A, close the menu. Writes a byte-for-byte copy of `soulsilver.sav` (the raw 512 KiB image a DS cart / R4 holds) to `sdmc:/3ds/soulsilver3ds/export/soulsilver_YYYYMMDD_HHMMSS.sav` and `export/soulsilver.sav`, refused unless a slot validates, read back and compared (`[SAVE] export: ... verified by read-back`). The live save is only read, never written. It exports the last saved game, not unsaved progress.
- Settings: "Save backups" toggle kept (it controls backups only, export always works); "3D frame reuse" moved to the 3D tab to make room. Untested on device.

### Union Room chunk A link fix (2026-10-28)
- Run 44 link: `sub_02035650` and `sub_02037474` were already defined by the staged core's overworld_native.c; ss3ds_union.c no longer defines them. ss3ds_game.sh now also re-syncs missing.c into a stale staged tree (patch 0118 deletes Missing() lines; a stale copy gave about 40 duplicate symbols).

### Settings app redo (2026-10-28, not compiled)
- L+R+Y app is now a scrolling list (7 rows visible, scroll bar) with a tooltip panel under it describing the selected setting; every setting has a one-line description (`kTabs` in render3ds.cpp, `suiList`/`suiTip` in ss3ds_settings_ui.h). Up/Down = row, Left/Right (or X/A) = value, L/R = tab, tap a value half to step it. Rows regrouped: Battle now holds the visible-wild rows, Play holds ribbon/date effects/text speed, Setup holds wireless and the Union Room guard. `mItems` indices unchanged.
- `tools/settings_preview.cpp`: `-` as the ROM path renders with a block font (layout check, no ROM needed); optional selected-row argument.

### Good defaults on (2026-10-28, not compiled)
- Keys absent from config.txt now default to ON: wild_vis, wild_vis_shiny, ribbon_check, celebrations, summary_ivs, hm_free, repel_prompt, select_reset, wide_battle, wide_boxes, wide_menu_screens, battle_anim_speed=1, fast_text=2 (plus shiny_suite, catch_overlay from the cleanup). bottom_menu, gpu_compose=2, fog_hole, audio limiter and stall fade were already on. Still off: wireless, cheats, wild_vis_replace, fps_counter, fps_tweaks, gpu_direct3d, dc_flush_skip. A key already in config.txt keeps its value, so an existing install only changes for keys it never touched. Table: `kCfgDefaults` in `psp_ctru.c`.

### Cleanup (2026-10-28, not compiled)
- Settings rows removed because the features are proven on device: Cave light fix (`fog_hole`), Shiny hunt suite (`shiny_suite`), Foe info (Y) (`catch_overlay`), Encounter counter (`hunt_counter`). The config keys still work (a key in config.txt always wins). `shiny_suite` and `catch_overlay` now default to ON when absent from config.txt (new `kCfgDefaults` table in `psp_ctru.c`, applies to game-side readers too).
- Removed the old text-list settings screen (`runMenu`, `settings_app=0`) and `g3null.cpp` (unbuilt). `settings_app` in config.txt is ignored.

### Union Room port, chunk A (2026-10-28, not compiled, link risk)
- `soulsilver3ds/platform/ss3ds_union.c` (new, in POBJS): hand-ports the Union Room entry chain from the asm (sub_02059DB0/E04/E1C/E88, SysTask sub_0205A03C, sub_0205A034/A07C/ABBC) and stubs the rest of the Union Room manager (37 functions called by game C, all logging `[UNION] stub <name>`) and the comm drivers it needs (sub_02037F18, sub_02038C1C, sub_02035650 ...) as "no session". pspoke patch 0118 drops the matching `Missing()` lines from missing.c (names listed in the patch).
- Reachable only with `union_guard` OFF (Settings > Controls "Union Room guard"). Test: guard OFF, walk into the Union Room from Pokemon Center 2F, read the `[UNION]` lines and the next `[SS-MISSING]` (if any). Builder: report any undefined/duplicate symbol at link. Next chunk: replace stubs with wm_uds.c-backed code.

### Union Room guard, hook 0117 (2026-10-28, not compiled)
- Run 42 log: entering the Union Room aborted on `[SS-MISSING] sub_02059DB0` (the Union Room manager, assembly-only; the whole comm stack under it is too, about 220 functions, see docs/WIRELESS.md "Union Room port plan"). Not a wireless/UDS fault (no `[WIRELESS]` lines).
- pspoke patch 0117 (hook 0117_union_guard): `ScrCmd_286` in scrcmd_c.c, the script command that warps into the Union Room, is skipped and logs `[UNION] ... skipped by union_guard` when `union_guard` is ON (default ON; Settings > Controls "Union Room guard", also `union_guard=` in config.txt). OFF runs the original call, which still aborts: test only. Remove the toggle once the comm stack is ported.
- Next for real trading: stub and port the Union Room manager (unk_02058AEC.s, unk_0205A44C.s) and back the comm drivers with wm_uds.c.

### Deleted the old Bottom menu code (2026-10-28, not compiled yet)
- The 4x2 tile layout, the INFO/WORLD/LOCAL panel cycling, `bmRect/bmHit/mmDrawTop/bmDrawTile`, `blCfg()/bmMapCfg()` and the `tools/minimap_preview.cpp` preview are gone; JOHTO is the only Bottom menu. The Settings rows BOTTOM MAP / BOTTOM LAYOUT are retired like the other dead rows (indices unchanged, in no tab); `bottom_map` / `bottom_layout` in an old config.txt are still accepted and logged as DEAD.

### Bottom menu: Save prompt visible, old Start menu never shown (run 42 feedback, hook 0116, not compiled yet)
- Run 42 log: after tapping SAVE the tiles stayed up the whole time, because run 40 keeps them over the game's touch-screen mode 1 (to hide the old Start menu) and the Save confirm with its YES/NO is also mode 1. Hook 0116 publishes the Start menu state (`P3DS_StartPrompt` in the Save states, `P3DS_StartIdle` while it waits for input); the tiles hide only during the Save prompt (`[BMENU] game Save prompt up/done`) and the game's own screen shows. No injected button presses.
- The tiles now cover the touch screen for as long as the game's Start menu task is alive (also when it was opened with X), so its old look is never drawn; a tile tap while it is already open selects directly (no second X). A Start menu left idle (Save cancelled) is closed through hook 0102 after 1.5 s (`[BMENU] idle Start menu nobody is using`).

### Keep the log of a crashed run (2026-10-27, not compiled)
- Run 39 Union Room crash: again nothing to decode, because the next launch rewrote log.txt. logring.c now renames a previous log.txt that does not end with `[3DS] exit: clean` (and its log_old.txt) to log_crash.txt / log_crash_old.txt before starting a new log. After a crash, do not delete them; read the last lines. Run 39's log also has no `[WIRELESS]` line at all, so check the build included PR #100.
- Ribbon checklist + special-date effects (Muse brief, Settings > Battle, both default OFF, not compiled yet): `ribbon_check` = press X on the Pokemon summary screen for a read-only ribbon checklist (Earned in this game n/14, Still available, From earlier games; ids from pret/pokeheartgold, read through the game's own GetMonData, never writes the Pokemon or the save, IV/EV box untouched). `celebrations` = birthday star sparkle at Sprout Tower (New Bark Town fallback) from `birthdays=MM-DD,MM-DD` in the player's own config.txt, plus public holiday effects (Jan 1 fireworks Goldenrod, Feb 14 hearts Cherrygrove, Mar 17 green sparkles Ilex Forest, Jul 1 red/white Olivine, Oct 31 wisps Lavender Town, Dec 25 snow Mahogany); `celeb_test=1..7` forces an effect for testing. Details: docs/RIBBON_CHECKLIST_AND_CELEBRATIONS.md.

- Bottom menu (run 39 feedback, hook 0102, not compiled yet): the game's old Start menu no longer flashes for ~4 frames after a tile tap or while it closes (tiles stay up through our own tile flow, touch-screen mode 1, plus a 30-frame grace), and returning from a tile no longer injects up to 8 audible B presses: the platform sets P3DS_StartClose and hook 0102 moves the idle Start menu task to its CLOSE state silently. Logs `[BMENU] flow: ... asking the game to close it` and `hook 0102: closing the Start menu task`.

- Shiny hunt suite ported from Platinum (Settings > Battle, all default OFF, hook 0113, not compiled yet): Shiny hunt suite (hunt.txt counters per species: encounters, streak, soft resets, shinies, time; shiny chime + flash; page 2 on the wild Y foe info screen with the odds and chance by now), Shiny odds (1/8192, 1/4096, 1/2048, 1/1024), Low battery warning.
- Visible wild Pokemon v2 (hook 0111 + platform, not compiled yet): followers.bin is built on the 3DS from soulsilver.nds the first time it is needed (no Platinum port or PC step; the Platinum-folder fallback is gone); up to 6 at once; soft ground shadow; a "!" over one that notices you; one you run away from stays (hidden during its battle), and one Repel blocked stays too. Log `[FOL] build`, `[WILD] ... it stays`. Untested on device.
- Visible wild Pokemon (Settings > Play, default OFF, hook 0110, not compiled yet): wild Pokemon from the map's real tables (time of day, swarms, radio, surfing, roamers) walk around in grass, caves and water; step onto one for its battle. They stay through menus, the bike and battles, hide their feet in tall grass, notice you (hop + cry), and are curious or shy. Optional shiny sparkle (1/8192) and "only visible battle". Log `[WILD]`. Untested on device.
- Speed tweaks (level 2 and up): the bottom overlay cache no longer force-repaints an unchanged picture every 60 frames (10.7 ms hitch every 2 s at idle in the run 34 log); the safety refresh is every 600 frames. Real changes still repaint at once. Check `[BMENU] overlay cache` repaints per 300 drop to ~0 at idle. Untested on device.
- JOHTO layout: the MAP button no longer cycles the old INFO / WORLD / LOCAL panels (Nathan: the Pokegear map is far better); it now closes any card or overview and glides the map back to the player. The Settings row BOTTOM MAP and the old 4x2 layout code are left dormant. Untested on device.

### Union Room crash: UDS worker guarded, wireless crash marker, synced [WIRELESS] log (2026-10-27, not compiled)
- Run 34: entering the Union Room hard-crashed the 3DS with `wireless=1`. No log tail and no boot.txt survived (log.txt/log_old.txt on the card were a later clean run), so the cause is not decoded. Prime suspect: the UDS worker thread had no fault guard and a 16 KB stack.
- uds_ctru.c: worker calls `P3DS_ThreadGuard("uds")` (a fault now lands in boot.txt), stack 64 KB, every `[WIRELESS]` line is flushed to the card at once, and `wireless.pending` is written before `udsInit` and removed on clean exit / `udsExit`. If it is still there at the next `udsInit` the previous run died with UDS active, so that run refuses UDS (radio with nobody in range, the Union Room still loads) and the next run tries again. wm_uds.c logs "WM init called by the game". Next step: with `wireless=1`, enter the Union Room and read the last `[WIRELESS]` line (or boot.txt).
- Bottom menu: a tile tap keeps our screen drawn while the game's Start menu opens (no zoom-in of the game's menu), and leaving Pokemon/Bag/Trainer card/Options presses B for you so the JOHTO map returns straight away. Logs `[BMENU] flow: ...`. Untested on device.
### HP box cache: cheaper hash (2026-10-27, not compiled)
- Run 34 log (level 6, mono battles): the 768 KiB hash cost 4.3 ms a frame (memory bound, not the ~1 ms estimated) and the cache reused 70% of frames: wide prep fell from 12.5 to 8.0 ms. Now hashes regs, OAM, palettes and OBJ VRAM only (256 KiB, ~1.4 ms) and recomposes at least every 8th frame.
- +OVERLAP (run 34, stereo overworld): left eye never waited for (600/600), 0 late conversions, no errors; left sync 0.83 ms -> 0, composite 8.7 -> 7.9 ms.

### Speed tweaks level 6 +OVERLAP: stereo compose overlap ported from Platinum frame_pacing (2026-10-27, not compiled)
- Platinum run 118 frame_pacing (FIXED worked on hardware; with the wide view on only its no-lag overlap is used). Ported only the overlap: the left eye's GPU compose + readback is queued and not waited for, the CPU walks the right eye meanwhile, and the left pixels are converted while the GPU draws the right eye. Atlas tile decode and 3D texture convert wait first if a draw is queued; second vertex buffer and a right-eye readback buffer (linear memory; failure logs and turns it off). Expected ~1 ms per stereo frame (run 33: left sync ~0.85 ms). Log: `[GPUC] overlap: last 600 left eyes ...`. Safe-boot guard (fps_tweaks.pending) covers it.

### Level 5: reuse the wide HP box compose when nothing changed (2026-10-27, not compiled)
- Run 33 log: with Wide HP boxes on, stereo battles spent 10-14 ms a frame in "wide prep" (WC_ComposeObjStrips for ~110 lines), VBlank idle ~0, fps 27-28; with the boxes off it was 0.3-2 ms. Mono battles stayed at 29-30.
- Level 5 hashes the compose inputs (2D regs, OAM, palettes, BG and OBJ VRAM; ~1 ms) and reuses last frame's result when identical. Log: `[WIDE] HP box cache (level 5): reused N, recomposed M, hash X us per frame`.
- Run 33 also: fps60 probe works now (field: 100% same-kind polygons, 100% moved <4 px when walking steady; short scenes 30-78%).

### dc_flush_skip toggle: ignore the game's cache flushes (2026-10-27, not compiled)
- Ported from the Platinum port (run 102). New `soulsilver3ds/platform/dcskip.c` wraps `sceKernelDcacheWritebackInvalidateRange` (the call under pspoke's `DC_FlushRange`; pspoke already wraps `DC_FlushRange` itself, so this goes one level down). Each game flush is one GSP IPC call; with the toggle ON they are skipped. The audio grain flush (1024 bytes, `sas_out.c`) always passes through, and the platform's own GPU/DSP flushes are untouched.
- Debug tab row "Skip game cache flushes" (`dc_flush_skip`, default OFF). Log: `[HEAP] dc_flush_skip=N` at first use, then `[HEAP] dc_flush: calls, KB, us spent` (OFF) or `[HEAP] dc_flush_skip: calls skipped, KB` (ON) every 200 calls, so one OFF run and one ON run compare directly.
- Not ported: Platinum's heap_fast. It skipped table scans in pokeplatinum's `WIN_CheckAndFree*`, which do not exist in the SoulSilver build (pspoke 7477648 has no such symbols), so there is nothing to skip.

### Bottom menu steps aside for the game's own touch screen: YES/NO buttons, starter Poke Balls (2026-10-27, not compiled)
- Bug (Nathan, run 30/31 on two 3DS): NPC and nurse YES/NO boxes and the starter Poke Balls in Elm's lab were invisible, though tapping or pressing still worked. Cause, reproduced in Azahar at the Cherrygrove nurse: HGSS puts those on the touch screen (the field's touch-screen mode 3, set by scripts), and the Bottom menu (JOHTO map) was drawn over the whole touch screen whenever the player was in the field. With `bottom_menu=0` the YES/NO buttons show.
- Fix: pspoke patch 0096 adds hook 0096 (`src/field_system.c` FieldSystem_Main publishes `ov01_021F6B00`, the field's touch-screen mode, every frame as `P3DS_SubMode`). The Bottom menu and the menu button are hidden while the mode is not 0 (the game's normal touch menu), so the game's own screen shows and gets the touches.
- Crash fix (run 32, entering a building or a battle, data abort pc 0x001a4198 in `ov01_021F6B00`): the getter reads `SysTask_GetData(fieldSystem->unk_D8)[0]` and the task is already gone during those transitions, so the data pointer is NULL. New pspoke patch 0101 (hook 0101) adds the NULL check; 0096 is left as applied and the YES/NO fix stays. Not compiled.
- Log: `[BMENU] game touch screen mode A -> B (...)` on every change, `game_touch_mode` in the `[BMENU] tiles shown/hidden` line, and `[BMENU] no touch-screen mode from the game ...` if the hook did not apply (builder: watch for "ss3ds 0096 touch-screen mode hook NOT applied").

### Run 31 log read: Bottom menu glide repaints halved at level 5, probe follows the live level (2026-10-27, not compiled)
- Run 31 (+TRIM, hardware): stereo overworld present 7.2 ms of which Bottom menu repaint 4.5 ms (10 repaints/s x 10.7 ms; up to 108 of 300 frames while the Pokegear map glides to a new location, one repaint per frame). Mono overworld has ~16.5 ms VBlank idle; stereo ~5 ms.
- Level 5: the glide repaints every second frame with double the step (same slide pace, half the repaints). fps60_probe followed the boot level only (logged 0); it now follows the live Speed tweaks level.
- +TRIM measured: wide DirectStrips ~1.1 ms -> ~0.6 ms (mono). Strips/overlays stage is only ~0.2 ms.

### Wide HP boxes: Platinum-style wild battle layout (2026-10-27, not compiled, untested)
- New toggle Settings > Wide > Wide HP boxes (`wide_boxes`, default OFF). pspoke patch 0100 shifts the battle HP box templates (player right, enemy left, 32 px); the renderer draws the sprites that reach the side strips with widecomp.c (from the Platinum port, `WC_ComposeObjStrips` draws only the lines those sprites cover). Needs a game rebuild for the hook. Logs: `[WIDE] wide_boxes`, `HP box offset`, `HP box overlay`.

### fps60_probe also runs at Speed tweaks level 5 (2026-10-27, not compiled)
- The polygon-pairing probe (`[FPS] probe`, `[FPS] field 10 s`) now also runs at +TRIM, so one run gives [PERF4], [PERF5] and the probe. Design for removing the compose readback: docs/FPS60_STEP2_DESIGN.md.

### Speed tweaks level 5 (+TRIM) and [PERF5] overlay/present split (2026-10-27, not compiled, not device-tested)
- Overworld is still ~28 ms of work in a 33 ms frame (game ~13, composite ~9, present ~6). No GPU-path change here (the last blind one hung the game); only CPU-side trims behind the Speed tweaks row: level 5 `+TRIM` converts the wide side strips with a one-OR-per-pixel fast path when there is no fade (bit-identical to the old loop) and skips the mid-composite OAM/palette hash check and the stereo blob scan (both diagnostics only).
- New `[PERF5]` line (one per second, 1 in 10 kept at log level 1): overlay stage = wide prep + overlay draws, wide DirectStrips, and GPU present = upload + prescale/quads/overlay icons + wide strips. It answers where the ~2.7 ms "strips+overlays" and the ~3 ms GPU present go, before anything bigger is attempted.
- See docs/FPS60_OVERWORLD.md for what can and cannot reach 60 fps.

### Speed tweaks level 4: 3D translucency blended on the GPU in battles (2026-10-27, not compiled, not device-tested)
- Run 29 log: battle frames were rejected by the GPU compositor almost always for the same setup, BLDCNT 3f40 (no first target, every layer and the backdrop second target, BG0 = 3D, BLDALPHA 0808): only the 3D engine's own per-pixel alpha blends with what is below. New Speed tweaks level **+3D BLEND** (`fps_tweaks=4`, includes BOTTOM and +FADES) lets the GPU compositor take those frames: the 3D target is premultiplied, so it is drawn with ONE / ONE_MINUS_SRC_ALPHA. Only used when the 3D came directly from the GPU target and the 3D clear colour is black (transparent 3D pixels carry the clear RGB); anything else still goes to the CPU path. `[PERF4]` shows "GPU 3D blend N" frames; `[GPUC] blend reject` still lists what remains. Check battle visuals (Pokemon edges, move effects, shadows) against the CPU path (`fps_tweaks` 3).
- Other rejects seen: menu/title fades with BG0 first target and backdrop not second (clause 4): left alone.

### Why alpha-blend frames (battles) stay on the CPU compositor: new log lines (2026-10-27, not compiled)
- Run 28 battle frames: composite 16 ms, of which ~7.4 ms is the CPU rendering engine A, because 140 of 300 frames are rejected by the GPU compositor's alpha-blend rules. The log cannot say which rule or which layers. `[GPUC] blend reject xN: clause C | BLDCNT ... BLDALPHA ... | layers on ...` now lists the top combinations per 300 frames (clause 1 = a second target is set while BG0 is the 3D layer, 2/3 = a 2D layer above a non-second-target / first-target layer, 4 = backdrop not a second target, 5 = the 3D layer is the first target). The fix (exact coverage-aware blending on the GPU) needs these numbers; nothing else changed, no setting.
- Merging the two eye composes into one GPU submit was looked at and left out: it would save about 1 ms of the 33 ms frame, needs a second vertex buffer and per-eye 3D textures in flight, and the last blind change to GPU frame handling hung the game on hardware.

### Speed tweaks hang fix and safe-boot guard (2026-10-27)
- Hardware: Speed tweaks > FLUSH hung the game at frame 4 ("game stalled", last step gc-draw: the GPU compose waited forever on the earlier queue) and, because the setting was saved, every later boot hung too. The lean GPU flush is gone from the menu; it exists only as `fps_leanflush=1` in config.txt (default 0, not recommended until the cause is found). Speed tweaks is now OFF / BOTTOM / +FADES.
- Safe boot: a non-zero Speed tweaks level writes `fps_tweaks.pending` on the SD card and deletes it after 600 presented frames. A boot that finds the file (hang, crash or power-off before that) forces `fps_tweaks=0`, rewrites config.txt and logs `[PERF4] SAFE BOOT`. Old config values of 1 count as 0.

### 60 fps work, stage 2a: find and cut the remaining frame cost (2026-10-27, not compiled, not device-tested)
- What the run 24 log says (GPU 3D DIRECT + Wide, overworld, 3D slider up, 33.3 ms/frame): the frame is pinned at exactly two VBlanks, and the "g3soft" share (13-16 ms) moves up and down to fill whatever composite and present leave, so it holds the game's own idle VBlank wait, not CPU 3D work (real 3D work is GpuRaw ~3.6 ms + End ~1 ms per [G3PROF]). The real costs: composite 11-14 ms = two GPU composes (left eye ~7.3 ms of which ~6.2 ms is GPU wait, right eye ~5 ms) for a 37-quad job, present ~5 ms, and the JOHTO bottom menu repainting all of itself (10.7 ms) every time the blink flips, twice a second (20 of ~25 repaints per 300 frames), which is what makes the 43 ms worst frames. The CPU pull-backs are fade frames (master brightness, rejected by the GPU compositor) and battle frames with 3D + blend.
- citro3d's `C3D_FrameEnd(0)` flushes the whole linear heap (~32 MiB) from the data cache on every call, and a stereo frame ends 4 times (3D, compose left, compose right, present).
- New Debug-tab row **Speed tweaks** (`fps_tweaks`, default OFF, cumulative so a bad step can be found): FLUSH = frames end with `GX_CMDLIST_FLUSH` only (every GPU input is already flushed by its writer); +BOTTOM = engine B is not composed while the Bottom menu covers the whole bottom screen, and the Bottom menu keeps one cached picture per blink state (no repaint on the blink); +FADES = master-brightness fades are composed on the GPU (applied exactly, melonDS 6-bit formula, in the readback pass) instead of pulling the 3D back to the CPU.
- New `[PERF4]` lines (always on, one pair per second at log detail 2): g3soft split into VBlank idle and real work; composite split into compose left/right (FrameBegin wait, FrameEnd, sync wait), CPU engine A and worker wait; present split into strips+overlays, Bottom menu (repaints), GPU present (FrameBegin wait, tearfix, FrameEnd); average FrameEnd cost; engine B hidden-skips and GPU fades.

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
