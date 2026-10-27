# Ribbon checklist, RTC audit, special-date effects (plan from Muse brief, 2026-10-27)

Status: PLAN + RTC AUDIT only. Nothing below is implemented yet. Shared with the 3ds-platinum port.

## 1. RTC / time audit (code read, not yet device-tested)
- `sceRtcGetCurrentClockLocalTime` (soulsilver3ds/platform/psp_ctru.c) calls `time(NULL)` + `gmtime` on every call. libctru's `time()` is the 3DS system clock (already local), so the game sees the 3DS clock continuously, not just at boot. Sleep/lid-close cannot skew it: nothing caches a tick counter.
- `sceRtcGetDayOfWeek` uses `mktime` on y/m/d: correct for weekday-gated events (Week Siblings, HGSS daily events).
- `cheat_time` (render3ds.cpp, tod tint) only overrides the *tint hour* when set 1-4 and is 0 by default; it never touches the game clock or save. No interference when off.
- Not checked (pspoke tree is not in this repo): whether the staged decomp's `GF_RTC`/`RTC_GetDateTime` paths all route through `sceRtcGetCurrentClockLocalTime`. Device test: set the 3DS clock to each weekday and check the right Week Sibling appears, and day/night + berries advance.

## 2. Ribbon checklist (v1 plan)
- Read-only panel, never writes the save, no ribbon grant/edit. Own access path (settings-app tab or summary-screen button), NOT the summary Y-cycle, so IV/EV (patch 0051) is untouched.
- Groups: Earned / Available in this game / From earlier games. Must read the full ribbon bitfields of the Pokemon struct.
- Ribbon bit positions must come from pret/pokeheartgold (`PokemonData` ribbon fields), verified against the summary screen with a known-ribbon Pokemon. Do not guess.
- Brief's HGSS list (verify against decomp): 7 Week Sibling dailies, Effort, Legend, 6 Battle Tower (World Ability = unobtainable). No contest ribbons obtainable.
- No Nintendo assets committed; text list is the fallback.

## 3. Special-date effects (new, port-side, visual only)
- Birthdays: shiny-style star sparkle, always at Sprout Tower (New Bark Town fallback). Dates ONLY in the player's local config.txt, never in repo/docs/code.
- Holidays (public dates may be in code): New Year Goldenrod fireworks; Valentine's Cherrygrove hearts; St Patrick's Ilex Forest green sparkles; Canada Day Olivine red/white sparkles; Halloween Lavender Town purple wisps; Christmas Mahogany snow. Birthday beats holiday. Toggle in settings menu and config.txt.
- Optional first-boot birthday prompt: drop if it cannot hook cleanly.
- Needs the companion implementation file from Muse (particle counts, speeds, colours) which was not attached.
