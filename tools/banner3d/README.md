# SoulSilver HOME Menu 3D banner

When SoulSilver 3DS is highlighted on the HOME Menu, the top screen shows a 3D banner (layout after Pokémon Y's): a
silver/gold portal glow at the back, four drifting sparkles, **the title screen's Lugia flapping in the middle (from your
own ROM, see below)**, and the SoulSilver logo in front, with a ~2.9 s original
jingle (`soulsilver3ds/full/cia/make_art.py` synthesizes it: rising silver bells over a soft pad, a gold chime last).

`make cia` in `soulsilver3ds/full` builds it. `make cia BANNER=flat` skips it. If it cannot be made (Python < 3.12, no
git or network for pycgfx, converter error) the CIA silently falls back to the flat banner (`banner.png`).

**Needs** Python 3.12+ and git (pycgfx is fetched once into `build/pycgfx`); nothing else.

## Lugia (from your ROM, build time only)

`make_banner.py --rom SoulSilver.nds` (the Makefile passes `$(SSROM)`, `roms/SoulSilver.nds`, when it exists) reads the
title screen's Lugia out of your own US SoulSilver ROM: NARC `a/0/4/6`, the NSBMD named `lugia01_10t` (member 20) and its
NSBCA (member 21, the next member). That file is the title scene's model, which also holds the sea and sky layers; only
the three Lugia materials are kept (`scene.LUGIA_KEEP`: body+arm, foot, eye). The animation is a 30 frame wing beat
repeated, so the banner plays one beat (1 s) on a loop. Lugia is posed (`scene.PITCH`/`YAW`/`LUGIA_WIDTH`/`LUGIA_POS`) to
fly to the left inside the band the HOME Menu draws and, like Pokémon Y's `y_autorotate`, sits under a bone that cancels
the HOME Menu's 10 s spin (`to_cgfx.py`), so a spin turns it in place.

The banner made from the ROM contains the game's model and textures, so it only ever exists in `build/` (gitignored,
`*.bnr`). **Nothing extracted is committed.** If `--rom` is missing, is not a US SoulSilver (`IPGE`), has no
`lugia01_10t`, or anything in the Lugia scene fails, the build prints why and makes the **art-only banner** (identical to
before this change); if that fails too the CIA gets the flat banner (`make cia BANNER=flat` forces it).

Files: `nitrofs.py` (ROM file table), `narc.py`, `nitro.py` (NSBMD/NSBCA reader, format knowledge from
[apicula](https://github.com/scurest/apicula)), `lugia_parts.py` (rigid per-joint parts + per-frame joint matrices; the
Platinum port's `giratina_parts.py` with a material filter), all adapted from the Platinum port (GabeSud0/pokeplatinum-3ds,
GPL-3.0). Stand-alone check: `python3 tools/banner3d/make_banner.py --rom R.nds --bannertool X --chime a.wav -o out.bnr
--keep DIR` also leaves the glTF for inspection (do not commit it).

Converter: [skyfloogle/pycgfx](https://github.com/skyfloogle/pycgfx) (pinned, fetched, not vendored: no licence), with the
HOME Menu fixes from [daaaaavia/CBC-for-YANBF](https://github.com/daaaaavia/CBC-for-YANBF) (MIT) in `to_cgfx.py`; scene
code, `linalg.py`, `pngio.py`, `shims/` adapted from the Platinum port's `tools/banner3d` (GabeSud0/pokeplatinum-3ds, GPL-3.0).

**2026-10-26: the Lugia banner (483,788 byte CGFX) FROZE the HOME Menu on the icon, so `make cia` defaults to the flat banner and this is opt-in (`BANNER=3d`, `BANNER=3d-lugia`); `to_cgfx.py` validates size/meshes/textures/verts and the build falls back to flat.**

**Untested on hardware.** The scene generates and converts (CGFX ~115 KB art-only, ~315 KB with Lugia, limit 512 KB;
Lugia's framing was checked with a software preview of the same glTF math, not the HOME Menu), but HOME Menu appearance,
Lugia's pose and wing animation, sparkle animation and the jingle have not been seen on a real 3DS. Platinum's lessons: a bad banner can freeze HOME;
use `BANNER=flat` if so. The HOME Menu remembers a title's banner: delete and reinstall the CIA to see a new one.
