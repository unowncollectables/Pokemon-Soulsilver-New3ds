# Third-party code and credits

Per-file credits are in the file headers; this is the overview.

- pspoke (IbrahimIrfan/pspoke, GPL-3.0): the PSP port this 3DS port is built on (pinned in `upstream.lock`, our changes in `pspoke-patches/`).
- pret/pokeheartgold (decompilation) via pspoke.
- The Platinum 3DS port (`platform/` copied from it), and melonDS-derived 2D/3D cores (GPL-3.0) inside it.
- GabeSud0/pokeplatinum-3ds (GPL-3.0): `tools/home_menu_art.py` (HOME Menu icon from the ROM). The `bottom_layout=1` (JOHTO) screen in `platform/ss3ds_minimap.h` follows the arrangement of its lower menu (`3ds_port/nitro/src/port_lowermenu.c`: map panel left, eight buttons, item square); no art from it is used there, the icons are drawn in the file. Since run 18 `platform/ss3ds_blnative.h` (bottom_hd=1) uses its box drawing (outline/highlight/shade) and its 9-row capital glyphs (A-X; F J L U W Y - added).
- ZallaxDev/pokeemerald-3Ds-dualscreen: idea for the touch-screen Start menu (`bottom_menu`); no code used.
