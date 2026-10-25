# Release notes

Current build (main). Not an official release; see README for install and known issues.

## What is in it
- Native New 3DS port of SoulSilver: boot, title, overworld and battles on the pspoke base, with 3D depth, wide view, GPU 3D and a GPU 2D compositor.
- Settings app on L+R+Y with Display, 3D, Controls, Battle, System and Play tabs (hidden Debug tab on SELECT).
- GPU blend is now on by default (tested clean on device: 299-300 of 300 frames composed on the GPU, zero pixel mismatches). The test toggle is gone from the menu; `gpu_blend=0` in `config.txt` turns it off.
- Fast text (2x, 4x, instant), hold B to skip text, auto-advance text.
- Faster battles, catch info panel (Y), encounter counter.
- ZL run toggle, IV/EV on the summary screen, reusable TMs, field moves without HMs, Repel prompt, no wild battles.
- Zone prefetch and ROM cache for fewer load hitches; three rolling save backups at every launch.

## Needs testing on device
Faster battles, Catch info, Encounter counter, ZL run, IV/EV on summary, Reusable TMs, Field moves w/o HM, Repel prompt and No wild battles were merged untested. Use a copy of your save.

## Known issues
Double image when a Poké Ball opens with the 3D slider up; Wide FX costs about 29 ms per frame (keep off); audio can clip; retail cart / flashcart save round trip not yet verified. Back up your save before using any build.
