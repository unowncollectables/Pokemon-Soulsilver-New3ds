# Platinum 3DS -> SoulSilver: what is carried over

Checked 2026-10-25 against `3ds-platinum` main (run 94), `pokeplatinum-3ds` main and `patch-builder`.

## Ported
- Boot speed check (`speedcheck.c`, run 81): `log.txt` says whether 804 MHz / L2 is really on and times each core. `speedcheck=0` skips it.
- Frame-spike diagnostics (runs 86 and 94): every `[SPIKE]` now has a second line with the map, whether zone prefetch had learned it, and the ROM files that missed. `[ZONE]` lines on every map change. `[PERF3]` names the zone. A run of slow frames logs its first frame and then every 30th, so one slow screen no longer uses up the 60-line cap. SoulSilver has no map-name table, so maps show as `map N`.
- Already in main before this sweep: audio timing, TexGen/shadow/light/scale fixes, ROM cache and prefetch, touch, wide view, settings app, HOME Menu banner, save backups, GF_ASSERT log, fault guards.

## Not ported (and why)
- Wide layouts, wide compositor (runs 83-93, 85, 87, 90, 92, 93): per-game UI layouts and game patches for Platinum's party, bag, PC and dex screens. Alpha-blend compositing is covered by its own thread.
- Shiny hunt suite, hunt counter, cheats, followers, Pal/Cheats/Hunt tabs, Poketch corners, repel prompt, HM-free, Snowpoint weather fix (runs 62-76, 88): need hooks in Platinum's game code.
- `res3d_test=2` sharp 3D: off by default in Platinum, broken on hardware before.
- Platinum's Start-menu/lower-screen work in `pokeplatinum-3ds` (menu controller, Town Map panel, circle wipes): native DS-API port, a different codebase from the pspoke base used here.
- `patch-builder`: one GUI builder for both ports (SoulSilver support added 2026-10-25).

## Ported later
- Shiny hunt suite (Platinum run 88 + 119), 2026-10-27: hunt.c (per-species counters, soft resets, time, odds, chime/flash), hunt page on the Y foe info screen, shiny_rate (hook 0113), battery_warn. Skipped: Radar/Masuda odds, party shiny count, rng_info, CSV export and reset buttons.
- Visible wild Pokemon (Platinum run 73 `wild_vis` + run 119 shiny sparkle), 2026-10-27: hook 0110, redone on SoulSilver's encounter_check.c (time-of-day, swarm, radio, surf tables, roamers) and fieldmap.c (spawn, behaviours, draw). Unlike Platinum they are not removed by menus, the bike or battles. Keys in README_settings.txt.
