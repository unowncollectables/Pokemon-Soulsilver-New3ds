# Handoff: where the project stands

Last updated 2026-10-25 (Lugia note added after run 3). Update this file whenever state changes.

## Current state
- Builds cleanly with devkitARM 16.1 from a US SoulSilver dump; `.3dsx` (about 5.7 MB) and `.cia` (about 3.5 MB) are produced.
- Runs on New 3DS: boot, title, overworld, battles, 3D depth, wide view, GPU 3D, GPU 2D compositor (alpha blend on by default), settings app (L+R+Y), save backups, ROM cache and zone prefetch.
- Recently merged: wide battles (#34, Display > Wide battles, default off), touch-screen menu button (#32), 3D frame reuse for unchanged/empty small frames (#33), audio limiter, valid-only rotating save backups with `import.sav`.

## Merged but not confirmed on a device
Wide party/bag/summary (`wide_menu_screens`; check `[WIDE] menu screen` lines), wide battles, menu button, 3D frame reuse (check `[BATTLE] start`, `[BTN]`, `[G3REUSE]` in log.txt), plus the gameplay toggles listed in README (Play and Battle tabs, ZL run, IV/EV on summary). Treat these as untested until a log or the owner says otherwise.

HOME Menu banner Lugia (2026-10-25, untested on a real HOME Menu): `make cia` builds the 3D banner with the title-screen Lugia from `roms/SoulSilver.nds` (NARC a/0/4/6 members 20+21, never committed; falls back to the art-only banner, then flat). Reinstall the CIA (delete the old one first) to see it. See `tools/banner3d/README.md`.

ROM auto-find for `make cia` (2026-10-25): the first CIA build fell back to the art-only banner because the ROM lived outside the repo. `tools/find_rom.sh` now searches `SSROM`, `P3DS_SOULSILVER`, `build/rom_path` (written by `ss3ds_build.sh` with the ROM it was given), `roms/`, `~/3ds/roms/`, `~/roms/`, then the SD copy (`$P3DS_SD/3ds/soulsilver3ds/soulsilver.nds`). If none exists `make cia` prints a boxed `[ROM] WARNING: art-only banner ... ROM not found at ...` with every path checked. Look for `[ROM] using ...` in the CIA build output. Untested on PC (checked with `make -n` only).

Bottom menu (Settings > Controls, `bottom_menu`, default OFF, untested): Start-menu tiles on the touch screen in the overworld, modelled on the Emerald dual-screen 3DS port (ZallaxDev/pokeemerald-3Ds-dualscreen, touch UI replacing START). Tap = platform sets `P3DS_StartReq` + presses X; pspoke patch 0060 (`start_menu.c` hook, non-fatal if it does not apply) maps it to the real menu button. Check `[BMENU]` in log.txt. Not done: showing a map or party on the bottom screen (Pokegear map/party screens are assembly here).

Save/log fsync (merged in the combined build, untested): the save is fsynced after each in-game save (pspoke patch 0070) and on HOME/sleep; log.txt every 2 s and after `[SAVE]` lines. Check `[SAVE] sync (save done) ... fsync ok` after saving in game, then save, close from HOME and confirm the save loads. Use a copy of the save.
Shiny-hunt reset helpers (Settings > Battle, `select_reset` and `reset_skip_intro`, default OFF, untested, patch 0071): SELECT tap in a wild battle = soft reset (not on a shiny); after a soft reset NitroMain goes straight to the save check / Continue menu (pspoke patch 0070). Check `[BOOT]` lines in log.txt: `select_reset: ... -> soft reset`, `soft reset marker ... skip 1`, `game hook asked, answer 1`; `hook NOT applied` means the build lacks the hook. Build log: `ss3ds 0070 reset_skip_intro hook applied`.

## Log tags (log.txt) and what proves a feature was used
Header at boot: `[CFG] ==== SoulSilver 3DS <version> | built ... ====`, then one `[CFG] key=value` line per key in config.txt, flagged `(default N) NON-DEFAULT`, `DEAD: ...` (key nothing reads: g3_wdepth, wild_vis*, dw_*, item_ping, touch_menu, bottom_strip, cheat_pp/speed/walk_walls, unused follower_* sub-keys) or `UNLISTED` (not in the header's key table, so ignored or new), then `[CFG] effective: key=value ...` for every default-bearing setting. At clean exit and in a crash: `[USE] feature usage (...)` (counts per feature) and `[USE] not used this run: ...`. The counts come from line prefixes in `logring.c` (`useTab`); keep prefix and log line in step when you rename one.
One line per event (never per frame):
| Feature | Line | `[USE]` name |
|---|---|---|
| Battle start / end | `[BATTLE] start #N`, `[BATTLE] end #N after X ms` (ss3ds_glue.c, from the overlay manager) | battles |
| Move animation skipped (battle_anim_speed) | `[BATTLE] move N animation skipped` (patch 0030) | battle_anim_skips |
| Catch info panel / shiny alert | `[CATCH] panel opened (wild #N)`, `[CATCH] wild #N Lv.. IVs ..`, `[CATCH] shiny alert` | catch_info_panel, shiny_alerts |
| IV/EV on summary | `[SUMMARY] IV .. EV ..` (page gathered), `[UI] summary IV/EV box 0/1` (Y toggle) | summary_iv_ev_pages, summary_iv_ev_box_toggles |
| Repel prompt | `[REPEL] repel check: prompt asked / skipped (repel_prompt=0)` | repel_checks |
| Reusable TM | `[ITEM] reusable TM: item N kept after teaching move M` | reusable_tm_uses |
| Field move without HM | `[HM] move N used by party slot S without knowing it` | field_moves_without_hm |
| No wild battles | `[CHEAT] no wild encounters: skipped N random encounter checks` (first, then every 25th) | no_wild_battle_skips (every check) |
| Hunt counter | `[HUNT] species N met M times` | hunt_counter_increments |
| Wide battle / wide menus | `[WIDE] battle entered ...`, `[WIDE] menu screen A -> B` | wide_battle_entries, wide_menu_screens |
| Bottom menu | `[BMENU] tap X -> action N`, `[BMENU] request handled by the game after N frames` | bottom_menu_taps, bottom_menu_actions |
| Menu button | `[BTN] opened by menu button tap` | menu_button_taps |
| Quick actions | `[QUICK] ZL run toggle N`, `[QUICK] ZR -> Y (item shortcut)`, `[QUICK] L+R+X wide view toggle: wideview=N` | quick_actions |
| Settings | `[APP] settings open`, `[APP] saved key=VALUE`, `[MENU] config.txt: key=N (saved)` or `... WRITE FAILED` | settings_opened, settings_changes |
| Soft reset / SELECT reset | `[3DS] soft reset (param ..)`, `[BOOT] ...` | soft_resets |
| Saves | `[SAVE] sync (why): N bytes ... fsync ok` | saves_synced |
Crash: `[CRASH] type= pc= lr= sp= far= fsr=`, `[CRASH] SoulSilver 3DS <version> | up S s | field F battle B | linear/app free | g3_wmode | last msg`, `[USE]` summary, breadcrumbs, then the red crash screen on the bottom screen (SoulSilver 3DS, version, type, pc/lr/sp/far, uptime, field/battle, last 3 log lines). Settings changed in the menu are persisted by `P3DS_CfgWrite` (verified: every row writes its key; failures log `WRITE FAILED`).
Config note: `catch_overlay` (Settings > Battle > Catch info) and `hunt_counter` (Encounter counter) default OFF, so a log with no `[CATCH]`/`[HUNT]` lines may only mean they were off: check `[CFG]`.

## Lugia title screen (2026-10-25, Claude thread "Fix Lugia title-screen rendering on hardware")
Hardware run 2 log (F:\3ds\soulsilver3ds\log.txt, pre-run-3) confirmed the title ran on the CPU renderer (`W-buffer frame -> CPU 3D renderer`, `[G3DIAG] gpu3d 2`, 2250 tris, 7 fps). But that renderer (g3soft.cpp TriEye) interpolated depth, UV and colour affinely in screen space, so its "W-buffer" depth was the same wrong affine-in-screen depth the GPU path had. Run 2 therefore did not rule depth out. Fixed: W-buffer triangles on the CPU renderer are now perspective-correct (TriWPersp, `g3_wpersp=1` default, 0 = old). Untested on device and not built.
Update, same day: Settings > 3D > W-buffer depth now defaults to **GPU SPLIT** (`g3_wmode=3`, depth code 4): normal Z-buffer, depth linear in w, triangles split until w varies < 3% across each (g3soft.cpp SplitW, g3gpu.c MAXV doubled). This is the intended fix and needs no menu change. Check the title first with the default; log must show `[GPU3D] g3_wmode=3 ... depth code 4` and `[LUGIA] ... split verts N -> M`. If Lugia is still wrong, do the steps below.
Next test on hardware (one build from main): (1) Settings > Debug > CPU 3D W-frames ON, look at the title. Log must show `[LUGIA] ... CPU W-buffer raster ON (perspective-correct)` and `[LUGIA] f=... W-frame path: CPU perspective-correct`. If Lugia is right, the geometry is fine and the PICA "W" depth mode behaves differently on hardware than in Azahar; then (2) set 3D > W-buffer depth = GPU Z (fast, hyperbolic depth in w that any GPU interpolates exactly) and check again; if that is clean it should become the default. If (1) is still broken, compare `[WSUM] ... psum` between hardware and Azahar: same psum = model data identical (look at matrices/rasteriser), different psum = model data differs (uninitialised or corrupted load).

## Known issues
See README "Known issues": Poke Ball opening double image with 3D slider up, Wide FX slow, audio clipping, menu fps below 60 in places, cart/flashcart save round trip unverified, first-visit map hitch.

## Next steps (suggested order)
1. Confirm the three recent features on hardware; fix whatever the log shows.
2. Hardware-test the untested gameplay toggles on a copy of a save.
3. Verify real cart / flashcart save compatibility with `tools/ss_savecheck.py`.
4. SoulSilver-specific systems from `docs/ROADMAP.md`: Pokegear bottom screen, Pokewalker (likely off), Pokeathlon, Battle Frontier, day/night and weekly events.
5. Run the new patcher once on a real PC and fix what it shows (see below).
6. Port more from Platinum per `docs/PLATINUM_SYNC.md` (wide layouts, shiny hunt tools).

## Patcher and install story
- `patch-builder` ("Pokemon 3DS Builder") builds and installs **Platinum, SoulSilver or both** (merged 2026-10-25): pick the game(s), your own ROM(s) and the SD card, press Build and install. It checks the SoulSilver ROM, downloads this repo, runs `scripts/ss3ds_build.sh` and `make` (optional `.cia`), backs up the save and copies to `3ds/soulsilver3ds`. End-user steps are in its README.
- Legal constraint: it takes the user's own ROM and never ships game data.
- Untested: the real SoulSilver compile through it, the `.cia` steps, Windows/WSL and the `.exe` (the window was only driven with stub scripts). The first real run on Nathan's PC is the test.

## Working with several AI assistants
Claude, Grok and others all work from this repo. Rules: read `AGENTS.md`, branch per task, small PRs, never touch ROMs or saves, record what you did and what is untested in this file, and prefer the log markers above as evidence. Local layout the owner uses: repo and `roms/` under `~/3ds/` in WSL (ROM at `~/3ds/roms/SoulSilver.nds`), SD card as drive F: (`F:\3ds\soulsilver3ds\`, CIAs in `F:\cias\`).

### Grok as beta tester (Nathan's setup)
Nathan runs a Grok bot alongside Claude. Claude is the main developer; Grok is a beta tester that can run the build in an emulator (`tools/emu_smoke.sh`, Azahar headless), judge screenshots, and push fixes. How to work with it:
- Grok pushes fixes to `main`, Claude builds from `main`, and Nathan tests that one combined build (never separate Claude and Grok builds).
- Grok's commits can land directly on `main` (seen 2026-10-25: "SS3DS run 1", committer `platinum3ds <local@box>`), not through a PR. Review `git log origin/main` for commits that do not come from a `claude/...` branch before starting work, and rebase on them.
- Its fixes are verified in the emulator only, not on the device. Treat them as untested on hardware until Nathan confirms.
- It follows the same rules as everyone: never commit ROMs or saves, tests use a copy of the save, new experimental features get a settings-menu toggle (L+R+Y), and the version string in `soulsilver3ds/platform/ss3ds_version.h` is bumped on each delivered build.
- Run 1 (Grok) fixed: field-fade crash on map 67 (patch 0053), from-scratch link error caused by patch 0051 (patch 0052), title-screen Lugia depth (`g3_wdepth` toggle, Settings > 3D), ROM-icon CIA.

## Device log review, 2026-10-25 (main 26f15c8 "run 3 + PRs 43-46 + Lugia GPU split")
Read from the SD card (log.txt 1942 lines, config.txt, boot.txt, hunt.txt). Save files not touched.
- Boot: clean, no crash, ~2 min in New Bark Town (map 33), `[3DS] exit: clean`. Save written twice, every `[SAVE] sync` line says `fflush ok, fsync ok`, exit backup rotated (count 15 -> 17, bak1..5).
- **Lugia NOT tested.** The run started right after a soft reset (`[BOOT] soft reset marker, age 3 s`), `reset_skip_intro=1` skipped the title, so no W-buffer frame happened: no `[LUGIA]` path lines, no `[WSUM]`, no `split verts`. Also config.txt has `g3_wmode=2` (GPU Z), which overrides the new default 3 (GPU SPLIT). To test: set Settings > 3D > W-buffer depth = GPU SPLIT (or delete the `g3_wmode` line) and cold-start with the skip-intro setting off. `g3_wdepth=2` in config.txt is a dead key.
- Checks: `skip intro hook NOT applied` absent (`[BOOT] skip intro: game hook asked, answer 1`) OK; `[BMENU] menu has 10 buttons, action mask 0e3f` OK; `[WIDE] wide_menu_screens` line never printed (lazy, party/bag/summary not opened); fps60_probe=0 (off).
- Perf: overworld 29-30 fps (143 of 150 samples at 29-30), worst frames are the two saves (197 and 204 ms) and map loads (450 ms first visit). 3D GPU draw+readback 10-12.6 ms with 353 tris, g3soft ~22 ms, composite ~9 ms. 100% of frames GPU-composed, gpu_blend=1, no ineligible frames.
- Audio: `audio_limiter=1` active, but pre-limiter peak still 43-53k and up to 1073 of 1720 grains over the knee per 10 s; the log has no post-limiter peak so effectiveness cannot be judged from it. Underfeed holds only during loads (29-41), 0-11 in steady state.
- Exercised: field walk, ZL run toggle, fast_text=2, menu (L+R+Y/menu button), 2 saves. Not exercised: battle, catch overlay, IV summary, repel, hunt counter increments, wide battle/menus, no-wild-battle, Lugia.
