# SoulSilver HOME Menu 3D banner

When SoulSilver 3DS is highlighted on the HOME Menu, the top screen shows a 3D banner (layout after Pokémon Y's): a
silver/gold portal glow at the back, four drifting sparkles, and the SoulSilver logo in front, with a ~2.9 s original
jingle (`soulsilver3ds/full/cia/make_art.py` synthesizes it: rising silver bells over a soft pad, a gold chime last).

`make cia` in `soulsilver3ds/full` builds it. `make cia BANNER=flat` skips it. If it cannot be made (Python < 3.12, no
git or network for pycgfx, converter error) the CIA silently falls back to the flat banner (`banner.png`).

**Needs** Python 3.12+ and git (pycgfx is fetched once into `build/pycgfx`); nothing else, and **no ROM**: everything
is the repo's own art in [`art/`](art) (`dev/make_art.py` remakes it from `SoulSilver banner 1024.png`). Unlike the
Platinum port there is no ROM-extracted model (no Giratina equivalent yet: a Lugia/Ho-Oh model would come from the ROM at
build time, never committed).

Converter: [skyfloogle/pycgfx](https://github.com/skyfloogle/pycgfx) (pinned, fetched, not vendored: no licence), with the
HOME Menu fixes from [daaaaavia/CBC-for-YANBF](https://github.com/daaaaavia/CBC-for-YANBF) (MIT) in `to_cgfx.py`; scene
code, `linalg.py`, `pngio.py`, `shims/` adapted from the Platinum port's `tools/banner3d` (GabeSud0/pokeplatinum-3ds, GPL-3.0).

**Untested on hardware.** The scene generates and converts (CGFX ~115 KB, limit 512 KB), but HOME Menu appearance,
sparkle animation and the jingle have not been seen on a real 3DS. Platinum's lessons: a bad banner can freeze HOME;
use `BANNER=flat` if so. The HOME Menu remembers a title's banner: delete and reinstall the CIA to see a new one.
