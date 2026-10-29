# [SS-MISSING] audit (2026-10-29, static, cloud only, no ROM, no build)

Method: applied patches 0001-0141 to pspoke 7477648, took every `Missing()` line in `missing.c` (635), mapped each name to its asm file in pokeheartgold(-slop), and found who launches it (script commands, `launch_application.c`, start menu, field subscreen table `ov01_02206C60`). The translator closure in `port.py` was reproduced offline: with the current seeds none of the 635 stubs is inside it, which confirms the model. A candidate overlay was then checked by (a) translating every function with `translate2.py`, (b) confirming the closure adds only functions of that overlay, and (c) confirming every new external is a C function in the decomp. Not checked (needs the ROM): data-table extraction, compile, link, behaviour.

Why they crash: a stub exists for each symbol a translated caller or data table references. Overlays reached only through app templates or pointer tables are never rooted, so the first time the feature opens the game aborts with `[SS-MISSING] name`. Fix pattern (0134, 0141, 0142): add the overlay's `.s` to `seed-files.json` and delete its `Missing()` lines.

## Fixed in patch 0142 (untested on device, same mechanism as 0141)
| Overlay | Feature | Stubs removed |
|---|---|---|
| 28 | Itemfinder (Dowsing Machine) touch screen, field subscreen row 4 | 3 |
| 64 | Hall of Fame showcase (Elite Four / Mt. Silver ending, HOF room machine) | 3 |
| 103 | PC Mailbox (any PC, also the player's room PC) | 3 |
| 56 | Mail screen (read or write mail) | 3 |
| 34 | Start menu action 12 subscreen (field subscreen row 8) | 2 |

## Still missing, ranked by how likely you are to hit it
1. Continue-screen rows (overlay 74 Mystery Gift / Pal Park migrate / Ranger, 112 Pokewalker, 75 Wii message): 61 + 228 + 57 stubs. Visible on every boot once unlocked. Not safe to auto-seed: 74 pulls in ARM-mode crypto functions the translator cannot handle, 112 and 75 need SDK CARD/RTC/DWC symbols.
2. Wireless error screen (overlay 0 `ov00_021E6A70` and friends, 14 stubs): any comm error or partner disconnect in Union Room or a trade ends in `[SS-MISSING]` instead of the error message. Statically clean but it also drags in DWC and NNS heap SDK calls, so wants a build to check the link.
3. Union Room extras: overlay 5 link-battle party select (19 stubs, statically clean), overlay 86 (Union Room app via ScrCmd_631, 3 stubs, clean), comm club helpers `sub_020573AC`, `sub_02058124`, `sub_020698E8`, `sub_0205D240` in unk_02056D7C / unk_02058034 / unk_0206979C / unk_0205CB48. Union thread should own these.
4. Global Terminal / GTS (overlay 70, ScrCmd_165 at MAP_GOLDENROD_GLOBAL_TERMINAL_1F, 3 stubs, 518 functions): GTS thread owns it; needs DWC/RTC/WCM SDK pieces.
5. Statically clean next seeds, lower reach: overlay 87 Scratch-Off cards, 109 Photo Album (PC menu when photos exist), 67 Pal Park, 57 Poke Ball seal app (ScrCmd_156), 41 Accessory Portrait (nine asm files), 108 Safari Zone area customizer and decoration (needs NNS_Gfd link sizes checked), 39 and 72 Wi-Fi apps (need DWC).
6. Unreachable in HGSS (Battle Frontier leftovers): overlays 80 to 83, 85, 91 to 93, all `FrtCmd_*`, `Frontier*`, `BattleArcade*`, `sub_02095xxx`, `sub_02096xxx`. Leave them.
7. `ov01_021EACBC` (ScrCmd_610, no script uses it) and `ov13_0222B430` look dead.

## Quiet build failures found (patch 0143)
`ss3ds_game.sh` re-synced only seeds.json, seed-files.json, port.py and native-provided.json into a staged tree. Also staged once and changed by patches: `call-retargets.json` (0136 blend routing among others), `arg-overrides.json`, `translate2.py` (0053), `native_gpu.cpp`, `GPU2D_Soft.cpp`, `nitromain-perf/input.c`. A tree staged before those patches never got them, with no error. 0143 adds them to the compare-and-copy loops (one recompile on the first build with a real difference). Also fixed: the reference `scripts/ss3ds_game.sh` had drifted from the live copy.
Still unguarded (staged once, patched, not in any re-sync): `build_data.py`, `native-probe/backing/generate*.py`, `overlays/gen-link.py`, `native-audio-app` memprobe/stackprobe/os_sync, `nitromain-perf/Makefile`. They are generator or probe inputs inside one-shot phases; if one of those patches ever seems to have no effect, delete the staged tree rather than debug.
Known and accepted: pspoke's own `patches/soulsilver/*.patch` loop only checks the last patch's exit code (0091 added a check for the DISPCNT case only), and the DWC weak stubs return 0 silently after four log lines (`[COMM] DWC stub`).
