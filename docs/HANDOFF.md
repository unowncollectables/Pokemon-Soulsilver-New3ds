# Handoff: where the project stands

Last updated 2026-10-25 (main at the merge of PR #34). Update this file whenever state changes.

## Current state
- Builds cleanly with devkitARM 16.1 from a US SoulSilver dump; `.3dsx` (about 5.7 MB) and `.cia` (about 3.5 MB) are produced.
- Runs on New 3DS: boot, title, overworld, battles, 3D depth, wide view, GPU 3D, GPU 2D compositor (alpha blend on by default), settings app (L+R+Y), save backups, ROM cache and zone prefetch.
- Recently merged: wide battles (#34, Display > Wide battles, default off), touch-screen menu button (#32, now opens a quick-toggle strip: TEXT/BATTLE/RUN/WILD/MORE, log tag `[QUICK]`, untested), 3D frame reuse for unchanged/empty small frames (#33), audio limiter, valid-only rotating save backups with `import.sav`.

## Merged but not confirmed on a device
Wide battles, menu button, 3D frame reuse (check `[3DS] battle 1`, `[BTN]`, `[G3REUSE]` in log.txt), plus the gameplay toggles listed in README (Play and Battle tabs, ZL run, IV/EV on summary). Treat these as untested until a log or the owner says otherwise.

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
