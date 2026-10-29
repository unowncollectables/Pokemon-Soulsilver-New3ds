
## Status 2026-10-29: S1 test build merged (untested, compile not checked in the cloud)
Shipped as Settings > 3D > Sharp 3D > CRISP (g3_sharp=4), mono, Wide view off only. Not the VRAM-texture design above: the 2x target is
copied out unscaled to linear RAM (g3gpu.c G3G_CrispHi), the mask is made on the CPU in gpuscale.c crispPrepare (composite pixel equals the 6-bit 3D
pixel within 6/255), mask goes into the copy's alpha, uploaded as a linear 512x512 texture and alpha-blended over the normal quad. Cost is
a few ms (logged: "[CRISP] ... avg prep us"). Dropped for the test: S0 dump (L+R+X+Y BMP dump already exists), stereo, wide, fog frames (stay 1x).
Check on device: 3D crisper than SHARP 3D ON? seams around sprites/text? frame time? Log lines [CRISP] and [3DRES].
