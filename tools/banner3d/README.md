# SoulSilver HOME Menu 3D banner

When SoulSilver 3DS is highlighted on the HOME Menu, the top screen shows a 3D banner laid out like the Platinum port's (and
Pokémon Y's): a silver/gold portal glow at the back, **the title screen's Lugia flapping in the middle (from your own ROM,
see below) with a gentle hover bob**, and the SoulSilver logo in front, with a ~2.9 s original jingle
(`soulsilver3ds/full/cia/make_art.py` synthesizes it).

**Built to be structurally the Platinum banner** (the one that runs on Nathan's HOME Menu): no sparkles, Lugia cut to
Platinum's 1000-triangle budget (`decimate.py`), 5 materials / 4 textures / 10 meshes, and `cgfx_fingerprint.py` checks every
banner against `reference/platinum_banner.json` (object counts and every setting, flag and format value of Platinum's tested
CGFX; no game data) and `to_cgfx.py` against Platinum-sized limits. Anything that does not match fails the 3D build and the CIA
gets the flat banner. Why the first Lugia banner froze HOME and what differed: `docs/HANDOFF.md` ("3D banner vs Platinum").

`make cia BANNER=3d` in `soulsilver3ds/full` builds it (needs the ROM); `BANNER=3d-art` = portal + logo only; the default is
`BANNER=3d` (no ROM or any failure falls back to the flat banner; `BANNER=flat` forces it). The logo (`art/logo.png`) is a made
Platinum-style logo, remade by `dev/make_logo.py` (small POKEMON, big slanted chrome SOULSILVER on a navy silver-rimmed body with feather wings, NEW 3DS tag). Needs Python 3.12+ and git (pycgfx is fetched once into `build/pycgfx`).

Checks and tools: `python3 tools/banner3d/test_banner3d.py` (no ROM needed); `make_banner.py ... --fingerprint-out F.json`
writes a banner's fingerprint; `python3 tools/banner3d/cgfx_fingerprint.py F.json` diffs it against Platinum's;
`--max-tris N` sets Lugia's triangle budget (default = Platinum's total of 1000, cards included; 0 = none);
`--allow-mismatch` and `--sparkles` are experiments that fail the check (never used by the Makefile).

## Lugia (from your ROM, build time only)

`make_banner.py --rom SoulSilver.nds` (the Makefile passes `$(SSROM)`, `roms/SoulSilver.nds`, when it exists) reads the
title screen's Lugia out of your own US SoulSilver ROM: NARC `a/0/4/6`, the NSBMD named `lugia01_10t` (member 20) and its
NSBCA (member 21, the next member). That file is the title scene's model, which also holds the sea and sky layers; only
the three Lugia materials are kept (`scene.LUGIA_KEEP`: body+arm, foot, eye). The animation is a 30 frame wing beat
repeated, so the banner plays one beat (1 s) on a loop. The model is reduced with an edge-collapse that never touches texture seams or the outline (about 2,040 -> 996 triangles). Lugia is posed (`scene.PITCH`/`YAW`/`LUGIA_WIDTH`/`LUGIA_POS`) to
fly to the left inside the band the HOME Menu draws and, like Pokémon Y's `y_autorotate`, sits under a bone that cancels
the HOME Menu's 10 s spin (`to_cgfx.py`), so a spin turns it in place.

The banner made from the ROM contains the game's model and textures, so it only ever exists in `build/` (gitignored,
`*.bnr`). **Nothing extracted is committed.** If `--rom` is given but is not a US SoulSilver (`IPGE`), has no `lugia01_10t`, or anything in the Lugia scene fails, the build stops with a `[BANNER] FAILED` line and the CIA gets the flat banner (`make cia BANNER=flat` forces it); without `--rom` it makes the art-only banner.

Files: `nitrofs.py` (ROM file table), `narc.py`, `nitro.py` (NSBMD/NSBCA reader, format knowledge from
[apicula](https://github.com/scurest/apicula)), `lugia_parts.py` (rigid per-joint parts + per-frame joint matrices; the
Platinum port's `giratina_parts.py` with a material filter), all adapted from the Platinum port (GabeSud0/pokeplatinum-3ds,
GPL-3.0). Stand-alone check: `python3 tools/banner3d/make_banner.py --rom R.nds --bannertool X --chime a.wav -o out.bnr
--keep DIR` also leaves the glTF for inspection (do not commit it).

Converter: [skyfloogle/pycgfx](https://github.com/skyfloogle/pycgfx) (pinned, fetched, not vendored: no licence), with the
HOME Menu fixes from [daaaaavia/CBC-for-YANBF](https://github.com/daaaaavia/CBC-for-YANBF) (MIT) in `to_cgfx.py`; scene
code, `linalg.py`, `pngio.py`, `shims/` adapted from the Platinum port's `tools/banner3d` (GabeSud0/pokeplatinum-3ds, GPL-3.0).

**2026-10-26: the first Lugia banner (CGFX 315,400 bytes; the "483,788" quoted earlier was the whole `.bnr`) FROZE the HOME Menu on the icon.** It had four animated sparkle cards, 14 meshes, 8 textures, 9 materials and 2,056 triangles, none of which Platinum's banner has. This one is rebuilt to Platinum's structure (see top). **Untested on a HOME Menu**: Lugia's pose and wing animation, the hover bob and the jingle have not been seen on a real 3DS (a software preview of the same math was used for framing). The HOME Menu remembers a title's banner: delete the title in FBI and reinstall to see a new one. If HOME freezes on the icon, do not hover over it; delete the title with FBI.
