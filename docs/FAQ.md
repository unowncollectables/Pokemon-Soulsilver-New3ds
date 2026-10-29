# FAQ

## Is this a full playable game?

Mostly. The game boots and plays on a New 3DS (overworld, battles, menus, saving, Pokégear, local wireless trading), but it is a homebrew port in active development, not a finished retail build, and some features are still untested on hardware. See the README Status table.

## Do I need a ROM?

Yes, for local builds you need your own legal copy of Pokémon SoulSilver to dump or extract on your own machine. The repo does not include ROM data.

## Is it legal to distribute this project?

The repository is designed to stay within the BYO-ROM rule and avoid bundling copyrighted assets. You still need to comply with local laws and Nintendo's rights when building, testing, and sharing work.

## Can I contribute?

Yes. Contributions are welcome, especially documentation, build fixes, testing, compatibility notes, or feature work. Keep changes focused and do not add copyrighted content.

## Why is the repo mostly documentation right now?

The repo holds the 3DS platform layer, patches and tooling; the game code is built from pspoke and your own ROM at build time (`scripts/bootstrap.sh`), and no game data is stored here.

## What hardware does this target?

This project targets the New Nintendo 3DS family and compatible 3DS hardware.

## What is the goal of the project?

The goal is to bring Pokémon SoulSilver to original New 3DS hardware with native rendering, stereo 3D, and a more maintainable codebase built from decompilation work.

## Can my save go back to a real DS cart or an R4?

Yes. `sdmc:/3ds/soulsilver3ds/soulsilver.sav` is the raw 512 KiB image written by the game's own save code (two slots at
0x00000 and 0x40000, each with footers and CRC16), the same layout a cart or R4 `.sav` has. Copy it out to export; use
`import.sav` (see README) to bring one in. Check any `.sav` with `python3 tools/ss_savecheck.py file.sav` (read only).
Always back up before swapping saves.

## Can I trade with another player?

Yes, between two 3DS consoles running this port: turn on Settings > Play > Local wireless on both and meet in the Union Room. A full trade was confirmed on hardware (run 59). Trading with a real DS is not possible locally; online trading is a possible future project. See [`WIRELESS.md`](WIRELESS.md).
