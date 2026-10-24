# Pokémon SoulSilver — New 3DS Edition
A native New Nintendo 3DS port of Pokémon SoulSilver, built from the
pret/pokeheartgold decompilation. Stereoscopic 3D, native resolution,
followers, all on real hardware.

## BYO-ROM rule
This repo contains no game content. You supply your own SoulSilver ROM;
the build extracts what it needs from your dump, on your machine.

## Roadmap
- Phase 1: boot, title, overworld in real stereo 3D
- Phase 2: battles, menus, Pokégear
- Phase 3: followers, day/night + weekly events
- Phase 4: Pokéathlon, Battle Frontier, extras
- Phase 5: polish and packaging

## Credits
Built on the pret/pokeheartgold decompilation.
## Credits

- **Pokémon Platinum — New 3DS Edition** — my own prior port, built first.
  SoulSilver builds on everything learned there: the renderer, the
  stereo depth system, the settings overlay, the crash logger, and
  the follower system.
- **[pokeplatinum-3ds](https://github.com/GabeSud0/pokeplatinum-3ds)**
  by GabeSud0 — an independent parallel port of Platinum to the 3DS,
  built from the same pret decompilation. Public reference for this
  project; any code borrowed from it is GPL-3.0 and credited in place.
- **[pret/pokeheartgold](https://github.com/pret/pokeheartgold)** and
  **[pret/pokeplatinum](https://github.com/pret/pokeplatinum)** — the
  decompilations these ports are built from.
- **[emerald-3ds](https://github.com/ZallaxDev/emerald-3ds)** — the
  native 3DS port that proved the concept and inspired both Platinum
  ports.
