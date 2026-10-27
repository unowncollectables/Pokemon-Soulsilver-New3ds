# Step 2 design: compose straight into what present samples (no readback) - 2026-10-27

Today per eye: GPU compose -> rtTex -> display transfer to linear `rb` -> blocking sync -> CPU bswap into `out` -> CPU overlays draw on `out` -> present uploads `out` (transfer to tiled tex) -> prescale -> quad. Cost: 2 syncs + 2 full-frame CPU passes per eye.

Plan (all behind Speed tweaks level 6, safe-boot guard, fallback = today's path per frame):
1. gpucomp: when the frame needs no CPU pixel work (no toast/save box/hunt/fps text/tint/debris/splash this frame, no CPU master-brightness fade, no wide strips needing eye pixels), skip the readback+sync and keep the compose target `rtTex` (per-eye ring of 2) as the present source.
2. gpuscale: `upload()` for such an eye is replaced by prescale sampling the compose target directly (compose output is already R<<24 order, so the TEV swizzle is not needed).
3. Overlay frames fall back to today's path. A later step can move the common overlays (toast, save box) to GPU quads.
4. Ordering: compose FrameEnd and present FrameBegin already serialise on citro3d's queue, so no new sync; the sync wait just disappears. Do NOT use GX_CMDLIST_FLUSH (hung on hardware).
Expected gain 3-5 ms/frame. Needs the next log's [PERF4] sync/end and [PERF5] upload columns to confirm it is worth the risk. Verify in Azahar (identical screenshots) before hardware. Not started.
