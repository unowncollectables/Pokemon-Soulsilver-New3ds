# Handoff: where the project stands

Last updated 2026-10-25 (main at the merge of PR #34). Update this file whenever state changes.

## Current state
- Builds cleanly with devkitARM 16.1 from a US SoulSilver dump; `.3dsx` (about 5.7 MB) and `.cia` (about 3.5 MB) are produced.
- Runs on New 3DS: boot, title, overworld, battles, 3D depth, wide view, GPU 3D, GPU 2D compositor (alpha blend on by default), settings app (L+R+Y), save backups, ROM cache and zone prefetch.
- Recently merged: wide battles (#34, Display > Wide battles, default off), touch-screen menu button (#32), 3D frame reuse for unchanged/empty small frames (#33), audio limiter, valid-only rotating save backups with `import.sav`.

## Merged but not confirmed on a device
Wide battles, menu button, 3D frame reuse (check `[3DS] battle 1`, `[BTN]`, `[G3REUSE]` in log.txt), plus the gameplay toggles listed in README (Play and Battle tabs, ZL run, IV/EV on summary). Treat these as untested until a log or the owner says otherwise.

## Known issues
See README "Known issues": Poke Ball opening double image with 3D slider up, Wide FX slow, audio clipping, menu fps below 60 in places, cart/flashcart save round trip unverified, first-visit map hitch.

## Next steps (suggested order)
1. Confirm the three recent features on hardware; fix whatever the log shows.
2. Hardware-test the untested gameplay toggles on a copy of a save.
3. Verify real cart / flashcart save compatibility with `tools/ss_savecheck.py`.
4. SoulSilver-specific systems from `docs/ROADMAP.md`: Pokegear bottom screen, Pokewalker (likely off), Pokeathlon, Battle Frontier, day/night and weekly events.
5. A SoulSilver patcher / installer for other people (see below).
6. Port more from Platinum per `docs/PLATINUM_SYNC.md` (wide layouts, shiny hunt tools).

## Patcher and install story
- The Platinum port has a GUI patch builder (`patch-builder` in the Platinum repo). It is **Platinum-only**. SoulSilver users currently build from source (README "Build") and copy files to the SD card by hand.
- Legal constraint: a patcher must take the user's own ROM and never ship game data. Any SoulSilver installer has to build or patch locally from their dump.
- Open work: a SoulSilver-capable builder UI with a ROM SHA1 check against `upstream.lock`, an SD card installer for both ports, and plain-language instructions for each.

## Working with several AI assistants
Claude, Grok and others all work from this repo. Rules: read `AGENTS.md`, branch per task, small PRs, never touch ROMs or saves, record what you did and what is untested in this file, and prefer the log markers above as evidence. Local layout the owner uses: repo and `roms/` under `~/3ds/` in WSL (ROM at `~/3ds/roms/SoulSilver.nds`), SD card as drive F: (`F:\3ds\soulsilver3ds\`, CIAs in `F:\cias\`).
