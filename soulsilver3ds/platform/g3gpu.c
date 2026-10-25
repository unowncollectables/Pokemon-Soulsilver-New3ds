/* platinum3ds: PICA200 (citro3d) rasterizer for g3soft. g3soft still does vertex transform/clipping and DS
 * texture decoding; triangles are queued here per eye, drawn on the GPU into a 256x256 VRAM target, then read
 * back and converted into the compositor's 3D layer format (6-bit RGB, 5-bit alpha). */
#include <3ds.h>
#include <citro3d.h>
#include <string.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "g3_shbin.h"
extern int P3DS_GpuInit(void);
extern void PSPNativeMemLog(const char *, ...);
typedef struct { float x, y, z, w, s, t, r, g, b, a; } V;
typedef struct { C3D_Tex *tex; int wrapS, wrapT, blend, dwrite, dfunc, late, fog; unsigned first, count; } B;
/* 2026-10-25: 3*8192 -> 3*16384 (+1 MiB linear per eye) so the title's split W-buffer triangles (g3soft.cpp SplitW) fit. */
#define MAXV (3 * 16384)
#define MAXB 2048
static unsigned vn[2];
/* free GPU vertex slots this frame (fullest eye); g3soft.cpp SplitW keeps its extra triangles inside this */
unsigned G3G_VertsFree(void) { unsigned u = vn[0] > vn[1] ? vn[0] : vn[1]; return u < MAXV ? MAXV - u : 0; }
static V *vb[2]; static B bt[2][MAXB]; static unsigned bn[2];
static int ok = -1; static DVLB_s *dvlb; static shaderProgram_s prog;
static C3D_Tex rt[2]; static C3D_RenderTarget *tgt[2]; static u32 *rb[2];
static C3D_Tex *pendFree[512]; static unsigned pendN; static C3D_Tex *oldFree[512]; static unsigned oldN;
/* gpu3d_async=1: the GPU draws frame N while the CPU composites; frame N's readback happens at End(N+1)
   (3D layer shows one frame late). Frees are held one extra frame. */
int G3G_Async = 0; static int pendRB, pendEyes;
static u32 clearRGBA; static float clearD; static int eyes;
/* ss3ds g3_wdepth (Settings > 3D > W-buffer depth, default ON): g3soft gives every raw-mode vertex a depth linear in w
   (DS W-buffer; the title/Lugia projection has z == w). The PICA interpolates z/w affinely in screen space, which is wrong
   for a value linear in w: big body triangles got bad depth and Lugia's spikes/wings poked through or floated. With the
   PICA's own W-buffer mode (C3D_DepthMap(false)) the per-vertex depth -d is interpolated perspective-correctly. 0 = old. */
/* run 3: settings/config moved to g3_wmode (0 OFF, 1 PICA W = default, 2 GPU Z) + Debug-tab g3_wcpu (CPU 3D for W frames,
   slow: ~210 ms/frame; did not fix Lugia on hardware in run 2). Old g3_wdepth key is ignored, so run-2 configs get the fast
   path. G3G_WDepth keeps the internal codes: 0 off, 1 CPU 3D, 2 PICA W, 3 GPU Z, 4 GPU split. */
/* 2026-10-25 (Lugia thread): g3_wmode=3 GPU SPLIT (optional; was default for one build: Lugia looked no better on
   hardware and fps was terrible, so the default is back to 1 PICA W): plain Z-buffer, depth linear in w, and g3soft.cpp
   SplitW cuts every triangle whose w varies by more than 3% so the GPU's affine depth is exact enough (see SplitW). */
int G3G_WDepth = -1, G3G_WMode = -1, G3G_WCpu = -1;
static const int kWModeToCode[4] = {0, 2, 3, 4};
void G3G_WDepthRecalc(void) { G3G_WDepth = G3G_WCpu ? 1 : kWModeToCode[G3G_WMode]; }
int G3G_WDepthOn(void) { if (G3G_WDepth < 0) { extern int P3DS_CfgInt(const char *, int); G3G_WMode = P3DS_CfgInt("g3_wmode", 1); if (G3G_WMode < 0 || G3G_WMode > 3) G3G_WMode = 1; G3G_WCpu = P3DS_CfgInt("g3_wcpu", 0) ? 1 : 0; G3G_WDepthRecalc(); PSPNativeMemLog("[GPU3D] g3_wmode=%d (0 off, 1 PICA W, 2 GPU Z, 3 GPU split) g3_wcpu=%d -> depth code %d", G3G_WMode, G3G_WCpu, G3G_WDepth); } return G3G_WDepth; }
int G3G_TexFlip = 1, G3G_FlipY = 0, G3G_ZInv = 0;  /* run 18: original mapping is correct (run 17 breakage was the texture V flip) */
#define RB_FLAGS (GX_TRANSFER_FLIP_VERT(1) | GX_TRANSFER_OUT_TILED(0) | GX_TRANSFER_RAW_COPY(0) | \
    GX_TRANSFER_IN_FORMAT(GX_TRANSFER_FMT_RGBA8) | GX_TRANSFER_OUT_FORMAT(GX_TRANSFER_FMT_RGBA8) | GX_TRANSFER_SCALING(GX_TRANSFER_SCALE_NO))
/* run 37: rb_sync=1 (default). The old readback ran C3D_SyncDisplayTransfer OUTSIDE a frame, which waits with
   gspWaitForPPF(). That GSP event is sticky: the GPU-scale present (gpuscale=1) queues 3-4 display transfers per frame
   whose PPF nobody consumes, so the wait returned at once and the CPU converted rb[] WHILE the DMA was still writing it
   (and with stale cache lines: rb[] was never invalidated). Rows past the DMA's progress came from the previous frame:
   a fixed split row, both photos, only visible while the 3D image moves, only with gpuscale=1 (mode 0 has no stray PPF).
   Now the transfer is queued INSIDE the frame after the draws, completion is the citro3d queue itself (gxCmdQueueWait
   via C3D_FrameBegin), and the CPU cache is invalidated before reading. rb_sync=0 = old path. */
static int rbSync = -1;
static int rbSyncOn(void) {
    if (rbSync < 0) { rbSync = 1; FILE *f = fopen("sdmc:/3ds/soulsilver3ds/config.txt", "r"); char l[64]; int x; if (f) { while (fgets(l, sizeof l, f)) if (sscanf(l, "rb_sync=%d", &x) == 1) rbSync = x != 0; fclose(f); } PSPNativeMemLog("[GPU3D] rb_sync=%d (3D readback: queued in-frame + cache invalidate)", rbSync); }
    return rbSync;
}
/* run 44 wideview v2 (experimental, default OFF): the 3D layer is rendered ONCE into a 320x256 framebuffer (viewport 320); every vertex gets x' = 0.8x (clip space) / (sx+32)/160-1 (screen space): same vertical scale, H-FOV x1.25
   (Hor+). Columns 32..287 are pixel-identical to the normal 256 frame and go to the compositor as before; columns
   0..31 and 288..319 become G3G_Strip[eye] (RGBA8 0xRRGGBBAA, 64x192: left 32 | right 32) for gpuscale. The game's
   matrices/state are untouched (only the port's final viewport mapping changes). The 256x256 targets are swapped for
   320x256 ones while wide is on (VRAM is tight: gpuscale holds 3 MiB), and swapped back when it is off. */
int G3G_Wide = 0, G3G_StripValid = 0;
u32 G3G_Strip[2][64 * 192];
static int curW = 256, wideFrame, wideFail, hiFrame;
static unsigned rtStride(void) { return (unsigned)curW; }
static int curHi;
#define RB_FLAGS_2X (GX_TRANSFER_FLIP_VERT(1) | GX_TRANSFER_OUT_TILED(0) | GX_TRANSFER_RAW_COPY(0) | \
    GX_TRANSFER_IN_FORMAT(GX_TRANSFER_FMT_RGBA8) | GX_TRANSFER_OUT_FORMAT(GX_TRANSFER_FMT_RGBA8) | GX_TRANSFER_SCALING(GX_TRANSFER_SCALE_XY))
static void rbQueue(int e) {
    if (curHi == 1) C3D_SyncDisplayTransfer((u32 *)tgt[e]->frameBuf.colorBuf, GX_BUFFER_DIM(2 * curW, 384), rb[e], GX_BUFFER_DIM(curW, 192), RB_FLAGS_2X);
    else C3D_SyncDisplayTransfer((u32 *)tgt[e]->frameBuf.colorBuf, GX_BUFFER_DIM(curW, 256), rb[e], GX_BUFFER_DIM(curW, 256), RB_FLAGS); } /* inFrame: queued only */
extern unsigned char PSPNative_GfxRegisters[];
int P3DS_MapOutdoor = 1; int P3DS_CurMap = -1; /* run 76: current map header (field), for Distortion World effects */ /* run 73: set by the follower frame (MapHeader_IsOutdoors) */
static void stripConvert(int e) {
    const u32 *s = rb[e];
    u16 mb = *(u16 *)(PSPNative_GfxRegisters + 0x6C); unsigned mm = (mb >> 14) & 3, f = mb & 31; if (f > 16) f = 16;
    { extern int P3DS_WideFadeFix(void); extern int P3DS_WideFxCfg(void); if (P3DS_WideFadeFix() || P3DS_WideFxCfg()) f = 0; }  /* run 54: wide_fx fits the strips to the centre */  /* run 52: fade applied when drawing (gpuscale) */
    u32 *d = G3G_Strip[e];
    static int edgeFill = -1; if (edgeFill < 0) { extern int P3DS_CfgInt(const char *, int); edgeFill = P3DS_CfgInt("wide_edgefill", 1) != 0; PSPNativeMemLog("[WIDE] wide_edgefill=%d", edgeFill); }
    for (unsigned y = 0; y < 192; y++) { const u32 *row = s + y * 320;
        for (unsigned x = 0; x < 64; x++) { u32 c = row[x < 32 ? x : x + 256];
            /* run 69 wide_edgefill: past the edge of the loaded map (Iron Island dock, sea routes...) the DS draws
               nothing there, so the strip pixel is empty (alpha 0). Continue the scene with the nearest real pixel
               of the same row a whole number of 32-px (2 map tiles) periods further in - same texture/tile phase,
               not mirrored or blurred. Only empty pixels pay; rows with nothing real stay as before. */
            if ((c & 255) < 8 && edgeFill && P3DS_MapOutdoor) { int sx = x < 32 ? (int)x : (int)x + 256, dir = x < 32 ? 32 : -32;
                for (int k = sx + dir; k >= 0 && k < 320; k += dir) { u32 t = row[k]; if ((t & 255) < 8) continue;
                    /* run 73: only a pixel that repeats one period further in is real tiled background (sea, ground);
                       a sprite / NPC / unique object there is skipped. Indoors (houses, Centers, gyms): no fill. */
                    int k2 = k + dir; if (k2 < 0 || k2 >= 320) break; u32 t2 = row[k2]; (void)t2;
                    { int k3 = k + dir * 32; if (k3 < 0 || k3 >= 320) break; u32 u = row[k3];
                      int dr = (int)(t >> 24) - (int)(u >> 24), dg = (int)((t >> 16) & 255) - (int)((u >> 16) & 255), db = (int)((t >> 8) & 255) - (int)((u >> 8) & 255);
                      if (dr * dr + dg * dg + db * db > 48 * 48) continue; }
                    c = t; break; } }
            unsigned r = c >> 24, g = (c >> 16) & 255, b = (c >> 8) & 255;
            if ((c & 255) < 8) r = g = b = 0;   /* 3D clear / alpha 0: black like the pillarbox */
            if (mm == 1 && f) { r += (255 - r) * f / 16; g += (255 - g) * f / 16; b += (255 - b) * f / 16; }
            else if (mm == 2 && f) { r -= r * f / 16; g -= g * f / 16; b -= b * f / 16; }
            d[y * 64 + x] = (r << 24) | (g << 16) | (b << 8) | 0xFF; } }
}
/* run 55 fog3d=1 (default): DS 3D fog (DISP3DCNT bit 7) on the GPU path. Each opaque polygon writes its POLYGON_ATTR fog
   bit into the stencil buffer (clear plane: CLEAR_COLOR bit 15); after the draw the D24S8 buffer is copied out with
   the colour readback (same queue) and fog is applied to the readback exactly like the DS / melonDS: 24-bit depth,
   FOG_OFFSET<<9, (dz>>2)<<FOG_SHIFT -> 32-entry density table (0x360) with linear interpolation, colour and alpha
   (or alpha only, DISP3DCNT bit 6) blended toward FOG_COLOR. Covers the wide-view strips too (whole 320 target).
   Skipped in W-buffer frames (depth units differ). 0 = no fog (old). */
int G3G_CurFog, G3G_FogNoW = 1; unsigned G3G_ConvUs; static u32 *drb[2]; static int fogFrame, fogCfg = -1; static unsigned fogFrames;
static int fogCfgOn(void) { if (fogCfg < 0) { fogCfg = 1; FILE *f = fopen("sdmc:/3ds/soulsilver3ds/config.txt", "r"); char l[64]; int x; if (f) { while (fgets(l, sizeof l, f)) if (sscanf(l, "fog3d=%d", &x) == 1) fogCfg = x < 0 ? 0 : x > 2 ? 2 : x; fclose(f); } PSPNativeMemLog("[GPU3D] fog3d=%d", fogCfg); } return fogCfg; }
static void applyFog(int e) {
    const unsigned char *R = PSPNative_GfxRegisters; const u16 dc = *(const u16 *)(R + 0x60); const u32 fc = *(const u32 *)(R + 0x358);
    const unsigned shift = (dc >> 8) & 15, alphaOnly = (dc >> 6) & 1; const u32 off = (u32)(*(const u16 *)(R + 0x35C) & 0x7FFF) << 9;
    unsigned tab[34]; for (int i = 0; i < 32; i++) { unsigned d = R[0x360 + i] & 127; tab[i + 1] = d == 127 ? 128 : d; } tab[0] = tab[1]; tab[33] = tab[32];
    const int fr = (int)((fc & 31) * 255 / 31), fg = (int)(((fc >> 5) & 31) * 255 / 31), fb = (int)(((fc >> 10) & 31) * 255 / 31), fa = (int)(((fc >> 16) & 31) * 255 / 31);
    const unsigned st = rtStride(); u32 *c = rb[e]; const u32 *d = drb[e];
    GSPGPU_InvalidateDataCache(drb[e], st * 256 * 4);
    for (unsigned i = 0; i < st * 192; i++) { const u32 dv = d[i]; if (!(dv >> 24)) continue;
        u32 z = dv & 0xFFFFFF; unsigned id, fr17;
        if (z < off) { id = 0; fr17 = 0; } else { z -= off; z = (z >> 2) << shift; id = z >> 17; if (id >= 32) { id = 32; fr17 = 0; } else fr17 = z & 0x1FFFF; }
        const int f = (int)((tab[id] * (0x20000 - fr17) + tab[id + 1] * fr17) >> 17); if (!f) continue;
        const u32 p = c[i]; int r = p >> 24, g = (p >> 16) & 255, b = (p >> 8) & 255, a = p & 255;
        if (!alphaOnly) { r = (fr * f + r * (128 - f)) >> 7; g = (fg * f + g * (128 - f)) >> 7; b = (fb * f + b * (128 - f)) >> 7; }
        a = (fa * f + a * (128 - f)) >> 7;
        c[i] = ((u32)r << 24) | ((u32)g << 16) | ((u32)b << 8) | (u32)a; }
}
/* run 56: fog3d=1 = PICA fog unit (free on the GPU): DS fog density (same melonDS math as applyFog) sampled into the
   128-entry fog LUT indexed by the stored depth (= DS 24-bit z); per batch on/off from the polygon fog bit; clear plane
   fogged on the CPU (one colour). fog3d=2 = run-55 exact CPU pass (stencil + depth readback, ~6-10 ms). */
static unsigned fogDens(u32 z) { const unsigned char *R = PSPNative_GfxRegisters; const u16 dc = *(const u16 *)(R + 0x60);
    const unsigned shift = (dc >> 8) & 15; const u32 off = (u32)(*(const u16 *)(R + 0x35C) & 0x7FFF) << 9;
    unsigned tab[34]; for (int i = 0; i < 32; i++) { unsigned d = R[0x360 + i] & 127; tab[i + 1] = d == 127 ? 128 : d; } tab[0] = tab[1]; tab[33] = tab[32];
    unsigned id, fr17; if (z < off) { id = 0; fr17 = 0; } else { z -= off; z = (z >> 2) << shift; id = z >> 17; if (id >= 32) { id = 32; fr17 = 0; } else fr17 = z & 0x1FFFF; }
    return (tab[id] * (0x20000 - fr17) + tab[id + 1] * fr17) >> 17; }
static C3D_FogLut fogLut; static u32 fogKey[10]; static int fogLutOk;
static void fogLutSetup(void) { const unsigned char *R = PSPNative_GfxRegisters; u32 k[10];
    k[0] = *(const u16 *)(R + 0x60) & 0x0F40; k[1] = *(const u16 *)(R + 0x35C); memcpy(&k[2], R + 0x360, 32);
    if (fogLutOk && !memcmp(k, fogKey, sizeof k)) { C3D_FogLutBind(&fogLut); return; }
    float d[256]; for (int i = 0; i <= 128; i++) { u32 z = i >= 128 ? 0xFFFFFF : (u32)i << 17; float v = 1.0f - fogDens(z) / 128.0f; if (i < 128) d[i] = v; if (i) d[128 + i - 1] = v - d[i - 1]; }
    { static unsigned nl; if (nl++ < 4) { char b[400]; int o = 0; for (int i = 96; i < 128; i += 2) o += snprintf(b + o, sizeof b - o, " %d", (int)(128 - d[i] * 128)); o += snprintf(b + o, sizeof b - o, " | tab"); for (int i = 0; i < 32; i += 4) o += snprintf(b + o, sizeof b - o, " %d", R[0x360 + i] & 127);
        PSPNativeMemLog("[GPU3D] fog3d LUT (density/128 at depth 0.75..1 step 1/64):%s", b); } }
    FogLut_FromArray(&fogLut, d); memcpy(fogKey, k, sizeof k); fogLutOk = 1; C3D_FogLutBind(&fogLut); }
/* run 57 fast_conv=1: the RGBA8 readback word is R<<24|G<<16|B<<8|A; one byte-reverse gives R|G<<8|B<<16|A<<24, then
   >>2 per byte (6-bit RGB) and alpha >>3 (5-bit): bit-identical to the old per-channel shifts, ~half the ALU work. */
static int fastCfg = -1;
static int fastConv(void) { if (fastCfg < 0) { fastCfg = 1; FILE *f = fopen("sdmc:/3ds/soulsilver3ds/config.txt", "r"); char l[64]; int x; if (f) { while (fgets(l, sizeof l, f)) if (sscanf(l, "fast_conv=%d", &x) == 1) fastCfg = x != 0; fclose(f); } PSPNativeMemLog("[GPU3D] fast_conv=%d", fastCfg); } return fastCfg; }
void G3G_SetFastConv(int v) { fastCfg = v != 0; } int G3G_FastConv(void) { return fastConv(); }
static void rbConvert(int e, u32 *out) {
    if (fogFrame == 2 && drb[e]) applyFog(e);
    if (!out) return; const u32 *s = rb[e]; const unsigned st = rtStride(), ox = st == 320 ? 32 : 0;
    GSPGPU_InvalidateDataCache(rb[e], st * 256 * 4);
    for (unsigned y = 0; y < 192; y++) { const u32 *row = s + y * st + ox; u32 *o = out + y * 256;
        if (fastConv()) for (unsigned x = 0; x < 256; x++) { const u32 t = (__builtin_bswap32(row[x]) >> 2) & 0x3F3F3F3Fu; o[x] = (t & 0x003F3F3Fu) | ((t >> 1) & 0x1F000000u); }
        else for (unsigned x = 0; x < 256; x++) { u32 c = row[x]; unsigned a = (c & 255) >> 3;
            o[x] = ((c >> 26) & 63) | (((c >> 18) & 63) << 8) | (((c >> 10) & 63) << 16) | (a << 24); } }
    if (st == 320) stripConvert(e);
}
/* run 46 res3d=2 (config, default 0): 3D layer rendered at 2x (512x384, or 640x384 with wideview) into plain
   framebuffers; the readback display transfer does a 2x2 box downscale in hardware (GX_TRANSFER_SCALE_XY), so the
   compositor still gets 256x192 (320x192) and the CPU cost is zero. Result: supersampled (4x SSAA) 3D: smooth edges,
   no texture shimmer, cleaner thin lines. VRAM: 1.5 MiB per eye (1.9 MiB wide) with 24-bit depth; if that does not
   fit (stereo needs both eyes), 16-bit depth is tried, then 1x (logged, res3dFail). */
static int res3dCfg = -1, res3dFail;
static int sharedTgt;
/* run 49: res3d depth. Run 48 ended up with a 16-bit depth buffer (log: "depth 16"): the far (top) part of the field
   z-fought into stripes and long diagonal seams showed through. The 24-bit depth did not fit (after the 1x targets are
   freed, VRAM has one 1.31 MiB hole + one 0.74 MiB hole; 640x384 needs 2 x 0.94 MiB). Now: color in VRAM, 24-bit depth
   in linear FCRAM (GPU-addressable; cleared by a full-screen quad because the memory-fill engine is VRAM-only).
   Order: VRAM depth24 -> linear depth24 (res3d_lindepth=1, default) -> VRAM depth16 (correct 16-bit clear) -> 1x. */
static void *linDepth; static int depthLin, depth16, linDepthCfg = -1; static V *qv;
static void freeTargets(void) { if (tgt[1] == tgt[0]) tgt[1] = NULL; sharedTgt = 0; if (linDepth) linearFree(linDepth); linDepth = NULL; depthLin = depth16 = 0; for (int e = 0; e < 2; e++) { if (tgt[e]) C3D_RenderTargetDelete(tgt[e]); tgt[e] = NULL; if (rt[e].data) C3D_TexDelete(&rt[e]); memset(&rt[e], 0, sizeof rt[e]); if (rb[e]) linearFree(rb[e]); rb[e] = NULL; } }
/* (re)create the eye targets: w = 256 | 320 compositor width, hi = 2x. GPU idle: called at frame begin, sync path. */
static int setTargets(int w, int hi) {
    if (w == curW && hi == curHi) return 1;
    C3D_FrameBegin(0); C3D_FrameEnd(GX_CMDLIST_FLUSH);   /* nothing may still reference the old targets */
    freeTargets();
    unsigned long vf = (unsigned long)vramSpaceFree(); int okW = 0; const char *depth = "24";
    if (!hi && w == 256) {   /* the original 256x256 texture targets */
        okW = 1; for (int e = 0; e < 2 && okW; e++) { rb[e] = linearAlloc(256 * 256 * 4); if (!rb[e] || !C3D_TexInitVRAM(&rt[e], 256, 256, GPU_RGBA8) || !(tgt[e] = C3D_RenderTargetCreateFromTex(&rt[e], GPU_TEXFACE_2D, 0, GPU_RB_DEPTH24_STENCIL8))) okW = 0; }
    } else {
        const int tw = hi ? 2 * w : w, th = hi ? 384 : 256;
        if (linDepthCfg < 0) { linDepthCfg = 1; FILE *f = fopen("sdmc:/3ds/soulsilver3ds/config.txt", "r"); if (f) { char l[64]; int x; while (fgets(l, sizeof l, f)) if (sscanf(l, "res3d_lindepth=%d", &x) == 1) linDepthCfg = x != 0; fclose(f); } PSPNativeMemLog("[3DRES] res3d_lindepth=%d", linDepthCfg); }
        if (!qv) qv = linearAlloc(6 * sizeof(V));
        for (int pass = 0; pass < (hi ? 3 : 1) && !okW; pass++) {
            if (pass == 1 && (!linDepthCfg || !qv)) continue;
            const int lin = pass == 1;
            const GPU_DEPTHBUF df = pass == 2 ? GPU_RB_DEPTH16 : GPU_RB_DEPTH24_STENCIL8; depth = pass == 2 ? "16" : lin ? "24 in linear RAM" : "24";
            if (pass) freeTargets();
            /* run 48: res3d shares ONE 2x target between the eyes (draw L, HW-downscale copy L, clear, draw R, copy R:
               all in GPU queue order). Run 47 allocated two and never fitted: 2 x 1.2-2.0 MiB vs 2.07 MiB free. */
            okW = 1; for (int e = 0; e < 2 && okW; e++) { rb[e] = linearAlloc(w * 256 * 4); if (!rb[e]) { okW = 0; break; }
                if (hi && e == 1) { tgt[1] = tgt[0]; sharedTgt = 1; continue; }
                if (lin) {
                    tgt[e] = C3D_RenderTargetCreate(tw, th, GPU_RB_RGBA8, -1);
                    if (tgt[e]) { linDepth = linearMemAlign(tw * th * 4, 0x80);
                        if (!linDepth) okW = 0; else { memset(linDepth, 0xFF, tw * th * 4); GSPGPU_FlushDataCache(linDepth, tw * th * 4);
                            C3D_FrameBufDepth(&tgt[e]->frameBuf, linDepth, GPU_RB_DEPTH24_STENCIL8); depthLin = 1; } }
                    else okW = 0;
                } else tgt[e] = C3D_RenderTargetCreate(tw, th, GPU_RB_RGBA8, df);
                if (!tgt[e]) okW = 0; }
            if (okW && pass == 2) depth16 = 1;
        }
    }
    if (!okW) {
        PSPNativeMemLog("[3DRES] %dx%s targets FAILED (VRAM free before %lu, linear free %lu)", hi ? 2 * w : w, hi ? "384" : "256", vf, (unsigned long)linearSpaceFree());
        freeTargets(); curW = 0; curHi = -1;
        if (hi) { extern void P3DS_Toast(const char *); res3dFail = 1; P3DS_Toast("SHARP 3D: NO VRAM"); return setTargets(w, 0) ? 0 : 0; }
        if (w != 256) { wideFail = 1; return setTargets(256, 0) ? 0 : 0; }
        PSPNativeMemLog("[3DRES] restoring 256 targets FAILED: 3D off"); ok = 0; return 0;
    }
    curW = w; curHi = hi;
    PSPNativeMemLog("[3DRES] eye targets now %dx%d (%s, depth %s%s) VRAM free before %lu, after %lu", hi ? 2 * w : w, hi ? 384 : 256, hi ? "res3d=2, HW 2x2 downscale" : "1x", depth, sharedTgt ? ", one target shared by both eyes" : "", vf, (unsigned long)vramSpaceFree());
    return 1;
}
static void readback(int e, u32 *out) {
    C3D_SyncDisplayTransfer(rt[e].data, GX_BUFFER_DIM(256, 256), rb[e], GX_BUFFER_DIM(256, 256), RB_FLAGS);
    if (!out) return; const u32 *s = rb[e];
    for (unsigned i = 0; i < 256 * 192; i++) { u32 c = s[i]; unsigned a = (c & 255) >> 3;
        out[i] = ((c >> 26) & 63) | (((c >> 18) & 63) << 8) | (((c >> 10) & 63) << 16) | (a << 24); }
}
int G3G_Init(void) {
    if (ok >= 0) return ok;
    ok = 0; if (!P3DS_GpuInit()) return 0;
    dvlb = DVLB_ParseFile((u32 *)g3_shbin, g3_shbin_size); shaderProgramInit(&prog); shaderProgramSetVsh(&prog, &dvlb->DVLE[0]);
    for (int e = 0; e < 2; e++) {
        vb[e] = linearAlloc(MAXV * sizeof(V)); rb[e] = linearAlloc(256 * 256 * 4);
        if (!vb[e] || !rb[e] || !C3D_TexInitVRAM(&rt[e], 256, 256, GPU_RGBA8)) { PSPNativeMemLog("[GPU3D] init failed (alloc)"); return 0; }
        tgt[e] = C3D_RenderTargetCreateFromTex(&rt[e], GPU_TEXFACE_2D, 0, GPU_RB_DEPTH24_STENCIL8);
        if (!tgt[e]) { PSPNativeMemLog("[GPU3D] init failed (target)"); return 0; }
    }
    ok = 1; PSPNativeMemLog("[GPU3D] PICA200 3D path ready"); return 1;
}
/* RGBA8 (u32 R | G<<8 | B<<16 | A<<24, DS row 0 first) -> tiled PICA texture. */
unsigned long long P3DS_SpkTexTicks; unsigned P3DS_SpkTexN; /* run 43 [SPIKE] diag */
static void *TexCreate0(const u32 *px, unsigned w, unsigned h);
void *G3G_TexCreate(const u32 *px, unsigned w, unsigned h) { u64 t0 = svcGetSystemTick(); void *r = TexCreate0(px, w, h); P3DS_SpkTexTicks += svcGetSystemTick() - t0; P3DS_SpkTexN++; return r; }
static void texFill(C3D_Tex *t, const u32 *px, unsigned w, unsigned h);
int G3G_RbSyncOn(void) { return rbSyncOn(); }
/* run 54: refresh an existing GPU texture in place (tex_postvblank): no queued draw has executed yet (sync path). */
void G3G_TexUpdate(void *tp, const u32 *px, unsigned w, unsigned h) { C3D_Tex *t = tp; if (!t || !px || t->width != w || t->height != h) return; texFill(t, px, w, h); }
static void *TexCreate0(const u32 *px, unsigned w, unsigned h) {
    if (w < 8 || h < 8 || w > 1024 || h > 1024) return NULL;
    C3D_Tex *t = malloc(sizeof *t); if (!t) return NULL;
    if (!C3D_TexInit(t, w, h, GPU_RGBA8)) { static unsigned nf; if (nf++ < 20) PSPNativeMemLog("[GPU3D] tex alloc FAIL %ux%u linear free %lu (fail #%u)", w, h, (unsigned long)linearSpaceFree(), nf); free(t); return NULL; }
    texFill(t, px, w, h); C3D_TexSetFilter(t, GPU_NEAREST, GPU_NEAREST);
    return t;
}
static unsigned texGen;  /* bumped on every texture upload/refresh: part of the g3_reuse key */
static void texFill(C3D_Tex *t, const u32 *px, unsigned w, unsigned h) {
    u32 *d = t->data; texGen++;
    for (unsigned y = 0; y < h; y++) {
        unsigned my = G3G_TexFlip ? h - 1 - y : y;  /* sprites render upright with this (run 16) */
        for (unsigned x = 0; x < w; x++) {
            unsigned m = (x & 1) | ((my & 1) << 1) | ((x & 2) << 1) | ((my & 2) << 2) | ((x & 4) << 2) | ((my & 4) << 3);
            u32 c = px[y * w + x];
            d[((my >> 3) * (w >> 3) + (x >> 3)) * 64 + m] = __builtin_bswap32(c);
        }
    }
    C3D_TexFlush(t);
}
void G3G_TexFree(void *t) { if (!t) return; if (pendN < 512) pendFree[pendN++] = t; else { C3D_TexDelete(t); free(t); } }
void G3G_Begin(u32 nativeClear, unsigned clearDepth16, int stereo) {
    unsigned r = nativeClear & 63, g = (nativeClear >> 8) & 63, b = (nativeClear >> 16) & 63, a = (nativeClear >> 24) & 31;
    clearRGBA = ((r << 2 | r >> 4) << 24) | ((g << 2 | g >> 4) << 16) | ((b << 2 | b >> 4) << 8) | (a * 255 / 31);
    clearD = clearDepth16 / 65535.0f; eyes = stereo ? 2 : 1; vn[0] = vn[1] = bn[0] = bn[1] = 0;
    { const int want = G3G_Wide && !wideFail && !G3G_Async && rbSyncOn() ? 320 : 256;
      if (res3dCfg < 0) { res3dCfg = 0; PSPNativeMemLog("[3DRES] res3d disabled (run 52: broken on hardware; config res3d ignored)"); }
      const int wantHi = res3dCfg == 2 && !res3dFail && !G3G_Async && rbSyncOn() ? 1 : 0;
      if ((want != curW || wantHi != curHi) && ok == 1) setTargets(want, wantHi);
      wideFrame = curW == 320; hiFrame = curHi == 1; }
}
static int wrapMode(int rep, int flip) { return !rep ? GPU_CLAMP_TO_EDGE : flip ? GPU_MIRRORED_REPEAT : GPU_REPEAT; }
static B *batchFor(int eye, void *tex, int repS, int repT, int flipS, int flipT, int blend, int dwrite, int dtest, int late) {
    if (ok != 1 || eye < 0 || eye > 1 || vn[eye] + 3 > MAXV) return NULL;
    int ws = wrapMode(repS, flipS), wt = wrapMode(repT, flipT), df = dtest ? GPU_EQUAL : GPU_LEQUAL;
    B *last = bn[eye] ? &bt[eye][bn[eye] - 1] : NULL;
    if (!last || last->tex != tex || last->wrapS != ws || last->wrapT != wt || last->blend != blend || last->dwrite != dwrite || last->dfunc != df || last->late != late || last->fog != G3G_CurFog) {
        if (bn[eye] >= MAXB) return NULL;
        last = &bt[eye][bn[eye]++]; last->tex = tex; last->wrapS = ws; last->wrapT = wt; last->blend = blend; last->dwrite = dwrite; last->dfunc = df; last->late = late; last->fog = G3G_CurFog; last->first = vn[eye]; last->count = 0;
    }
    return last;
}
/* Raw-mode triangle in DS clip space (x,y,z,w per vertex), normalised uv: GPU does the divide, so texturing is
   perspective-correct and clipping is done in hardware. shiftPx: per-eye stereo shift in DS pixels per vertex. */
void G3G_TriClip(int eye, const float *c, const float *su, const float *sv, const u32 *col, void *tex,
                 int repS, int repT, int flipS, int flipT, int blend, int dwrite, int dtest, const float *shiftPx) {
    B *last = batchFor(eye, tex, repS, repT, flipS, flipT, blend, dwrite, dtest, blend); if (!last) return;
    for (int k = 0; k < 3; k++) {
        V *o = &vb[eye][vn[eye]++]; float x = c[4 * k], y = c[4 * k + 1], z = c[4 * k + 2], w = c[4 * k + 3];
        o->x = x + (shiftPx ? shiftPx[k] / 128.0f * w : 0); if (wideFrame) o->x *= 0.8f;
        o->y = hiFrame ? (G3G_FlipY ? y : -y) : G3G_FlipY ? (0.25f * w + 0.75f * y) : (-0.25f * w - 0.75f * y);  /* res3d: full-height 384 target */
        o->z = G3G_ZInv ? 0.5f * z - 0.5f * w : -0.5f * z - 0.5f * w; o->w = w;
        if (G3G_WDepth == 2 && w > 1e-6f) o->z /= w;   /* g3_wdepth: W-buffer mode wants the undivided depth (-d) */
        o->s = su[k]; o->t = G3G_TexFlip ? sv[k] : 1.0f - sv[k];
        u32 cc = col[k]; o->r = (cc & 255) / 255.0f; o->g = ((cc >> 8) & 255) / 255.0f; o->b = ((cc >> 16) & 255) / 255.0f; o->a = (cc >> 24) / 255.0f;
    }
    last->count += 3;
}
/* v: 3 x {sx, sy, z(0..65535), u(texels), v(texels), color RGBA8 as u32 R|G<<8|B<<16|A<<24} */
void G3G_Tri(int eye, const float *sx, const float *sy, const float *sz, const float *su, const float *sv, const u32 *col,
             void *tex, unsigned tw, unsigned th, int repS, int repT, int flipS, int flipT, int blend, int dwrite, int dtest) {
    B *last = batchFor(eye, tex, repS, repT, flipS, flipT, blend, dwrite, dtest, 0); if (!last) return;
    float itw = tw ? 1.0f / tw : 0, ith = th ? 1.0f / th : 0;
    for (int k = 0; k < 3; k++) {
        V *o = &vb[eye][vn[eye]++]; float z = sz[k] / 65535.0f; if (z < 0) z = 0; if (z > 1) z = 1;
        o->x = wideFrame ? (sx[k] + 32.0f) / 160.0f - 1.0f : sx[k] / 128.0f - 1.0f; o->y = (G3G_FlipY ? 192.0f - sy[k] : sy[k]) / (hiFrame ? 96.0f : 128.0f) - 1.0f; o->z = G3G_ZInv ? z - 1.0f : -z; o->w = 1.0f;
        o->s = su[k] * itw; o->t = G3G_TexFlip ? sv[k] * ith : 1.0f - sv[k] * ith;  /* run 17: billboards were upside down with 1-v */
        u32 c = col[k]; o->r = (c & 255) / 255.0f; o->g = ((c >> 8) & 255) / 255.0f; o->b = ((c >> 16) & 255) / 255.0f; o->a = (c >> 24) / 255.0f;
    }
    last->count += 3;
}
static unsigned lastUs;
unsigned G3G_LastUs(void) { return lastUs; }
int G3G_Hi(void) { return curHi == 1; }
/* run 47: L+R+A toggles res3d 0 <-> 2 in-game (takes effect next frame; a failed 2x allocation is retried on each toggle) */
int G3G_Res3dToggle(void) { res3dCfg = 0; return 0; }  /* run 52: sharp 3D disabled; L+R+A is a swallowed no-op */
/* Draw queued triangles, read back into out[eye] (compositor format). */
/* run 49: clear color + depth of the current target with a full-screen quad (depth in linear RAM: no memory fill) */
static void clearQuad(void) {
    const float d = clearD < 0.9999990f ? clearD : 0.9999990f, z = -d;
    const float r = (clearRGBA >> 24) / 255.0f, g = ((clearRGBA >> 16) & 255) / 255.0f, b = ((clearRGBA >> 8) & 255) / 255.0f, a = (clearRGBA & 255) / 255.0f;
    static const float xy[6][2] = { {-1, -1}, {1, -1}, {1, 1}, {-1, -1}, {1, 1}, {-1, 1} };
    for (int i = 0; i < 6; i++) { V *o = &qv[i]; o->x = xy[i][0]; o->y = xy[i][1]; o->z = z; o->w = 1.0f; o->s = o->t = 0; o->r = r; o->g = g; o->b = b; o->a = a; }
    GSPGPU_FlushDataCache(qv, 6 * sizeof(V));
    C3D_BufInfo *bi = C3D_GetBufInfo(); BufInfo_Init(bi); BufInfo_Add(bi, qv, sizeof(V), 3, 0x210);
    C3D_TexEnv *env = C3D_GetTexEnv(0); C3D_TexEnvInit(env); C3D_TexEnvSrc(env, C3D_Both, GPU_PRIMARY_COLOR, 0, 0); C3D_TexEnvFunc(env, C3D_Both, GPU_REPLACE);
    C3D_AlphaTest(false, GPU_ALWAYS, 0); C3D_AlphaBlend(GPU_BLEND_ADD, GPU_BLEND_ADD, GPU_ONE, GPU_ZERO, GPU_ONE, GPU_ZERO);
    C3D_DepthTest(true, GPU_ALWAYS, GPU_WRITE_ALL);
    C3D_DrawArrays(GPU_TRIANGLES, 0, 6);
    C3D_AlphaTest(true, GPU_GREATER, 3);
}
/* g3_reuse (menu: 3D FRAME REUSE, default ON): menus, title and dialogs draw only a few triangles but still pay the
   GPU draw + readback + convert (18-28 ms). The GPU is deterministic, so identical inputs give an identical image:
   a frame whose complete input (vertex data, batches, texture pointers + a texture upload counter, wrap modes, clear
   colour/depth, fog registers, eye count and layout) is byte-equal to the previous eligible frame reuses that
   frame's converted output (memcpy). A frame with no triangles at all is filled with the converted clear colour.
   Only small frames (<= RU_MAXV vertices, <= RU_MAXB batches) are compared; wide/2x/async/CPU-fog frames never.
   g3_reuse_verify=1 (config.txt): also renders the frame and logs any pixel that differs from the reused output. */
#define RU_MAXV 1200
#define RU_MAXB 64
typedef struct { u32 clr, fc, tg; float depth; int eyes, haveL, haveR, fog; u16 dc, f350, f35c, pad; unsigned vn[2], bn[2]; unsigned char tab[32]; } RK;
static RK ruKey; static V *ruV[2]; static B *ruB[2]; static u32 *ruOut[2]; static int ruValid, ruVerify = -1;
static unsigned ruHit, ruEmpty, ruDraw, ruBig, ruBad; static unsigned long long ruSaveUs, ruHitUs, ruDrawUs; static unsigned ruFrames;
extern int P3DS_G3ReuseCfg(void); extern int P3DS_CfgInt(const char *, int);
static u32 convWord(u32 c) { const u32 t = (__builtin_bswap32(c) >> 2) & 0x3F3F3F3Fu; return (t & 0x003F3F3Fu) | ((t >> 1) & 0x1F000000u); }
static void ruFreeDeferred(void) {
    pendRB = 0; for (unsigned i = 0; i < oldN; i++) { C3D_TexDelete(oldFree[i]); free(oldFree[i]); } oldN = 0;
    for (unsigned i = 0; i < pendN; i++) { C3D_TexDelete(pendFree[i]); free(pendFree[i]); } pendN = 0;
}
static void ruLog(void) {
    if (++ruFrames % 300 != 0) return;
    PSPNativeMemLog("[G3REUSE] 300f: reused %u, empty %u, drawn small %u (avg %u us), drawn big %u | reuse avg %u us, mismatches %u (verify %d) | saved ~%u ms",
        ruHit, ruEmpty, ruDraw, ruDraw ? (unsigned)(ruDrawUs / ruDraw) : 0, ruBig, ruHit ? (unsigned)(ruHitUs / ruHit) : 0, ruBad, ruVerify,
        (unsigned)(ruSaveUs / 1000));
    ruHit = ruEmpty = ruDraw = ruBig = 0; ruDrawUs = ruHitUs = ruSaveUs = 0;
}
extern void P3DS_Crumb(const char *tag, void *caller);
/* ss3ds fps60_probe=1 (log only, default 0): would a 60 fps field be able to pair this frame's triangles with the
   previous frame's? Pokeplatinum-3ds makes its in-between scene by pairing polygons across two game frames in draw
   order; this counts how many triangles at the same index still have the same texture coords (same kind of polygon) and
   how far their screen position moved (DS pixels: x/w*128, y/w*96 of the clip-space vertex). One line per ~5 s. */
static void fps60Probe(void) {
    static int on = -1; static V *prev; static unsigned prevN, frames, tot, kind, near16, near4, still, shortRun; static double sumMove;
    extern int P3DS_CfgInt(const char *, int); extern int P3DS_InField;
    if (on < 0) { on = P3DS_CfgInt("fps60_probe", 0) != 0; if (on) prev = malloc(MAXV * sizeof(V)); if (on && !prev) on = 0; }
    if (!on || !P3DS_InField || eyes < 1) { prevN = 0; return; }
    const unsigned n = vn[0] / 3;
    if (prevN && n) { const unsigned m = n < prevN ? n : prevN;
        for (unsigned i = 0; i < m; i++) { const V *a = &vb[0][i * 3], *b = &prev[i * 3]; int same = 1; float mv = 0;
            for (int k = 0; k < 3; k++) { if (fabsf(a[k].s - b[k].s) > 0.02f || fabsf(a[k].t - b[k].t) > 0.02f) same = 0;
                if (a[k].w > 1e-3f && b[k].w > 1e-3f) { float dx = (a[k].x / a[k].w - b[k].x / b[k].w) * 128.0f, dy = (a[k].y / a[k].w - b[k].y / b[k].w) * 96.0f; mv += sqrtf(dx * dx + dy * dy); } else mv += 999.0f; }
            tot++; if (!same) continue; kind++; mv /= 3.0f; sumMove += mv < 999.0f ? mv : 0; if (mv < 16.0f) near16++; if (mv < 4.0f) near4++; if (mv < 0.25f) still++; }
        if (n != prevN) shortRun++;
        tot += (n > prevN ? n : prevN) - m; }
    if (n) { memcpy(prev, vb[0], vn[0] * sizeof(V)); prevN = n; } else prevN = 0;
    if (++frames >= 150 && tot) {
        PSPNativeMemLog("[FPS] probe %u frames: %u tris/frame, same kind at same index %u%%, of those moved <16px %u%% <4px %u%% still %u%%, mean move %.2f px, triangle count changed in %u frames",
            frames, tot / frames, kind * 100 / tot, kind ? near16 * 100 / kind : 0, kind ? near4 * 100 / kind : 0, kind ? still * 100 / kind : 0, kind ? sumMove / kind : 0.0, shortRun);
        frames = tot = kind = near16 = near4 = still = shortRun = 0; sumMove = 0; }
}
void G3G_End(u32 *outL, u32 *outR) {
    if (ok != 1) return;
    fps60Probe();
    u64 t0 = svcGetSystemTick();
    for (int e = 0; e < eyes; e++) GSPGPU_FlushDataCache(vb[e], vn[e] * sizeof(V));
    const int rs = rbSyncOn();
    fogFrame = 0;
    if (fogCfgOn() && (rs || fogCfg == 1) && (!G3G_Async || fogCfg == 1) && !depthLin && (fogCfg == 1 || (!curHi && !depth16)) && G3G_FogNoW && (*(const u16 *)(PSPNative_GfxRegisters + 0x60) & 0x80)) {
        if (fogCfg == 2 && !drb[0]) { for (int e = 0; e < 2; e++) drb[e] = linearAlloc(320 * 256 * 4); if (!drb[0] || !drb[1]) { PSPNativeMemLog("[GPU3D] fog3d: no linear memory, fog off"); fogCfg = 0; } }
        fogFrame = fogCfg; if (fogFrame == 1) { fogLutSetup(); const u32 fc = *(const u32 *)(PSPNative_GfxRegisters + 0x358);
            C3D_FogColor(((fc & 31) * 255 / 31) | (((fc >> 5) & 31) * 255 / 31) << 8 | (((fc >> 10) & 31) * 255 / 31) << 16); }
        if (fogFrame && ++fogFrames % 600 == 1) PSPNativeMemLog("[GPU3D] fog3d: fog frame (DISP3DCNT %04x, FOG_COLOR %08lx, offset %04x)", *(const u16 *)(PSPNative_GfxRegisters + 0x60), (unsigned long)*(const u32 *)(PSPNative_GfxRegisters + 0x358), *(const u16 *)(PSPNative_GfxRegisters + 0x35C)); }
    int ruCand = 0, ruCheck = 0;
    if (ruVerify < 0) { ruVerify = P3DS_CfgInt("g3_reuse_verify", 0) != 0; PSPNativeMemLog("[G3REUSE] g3_reuse_verify=%d", ruVerify); }
    if (rs && !G3G_Async && curW == 256 && curHi == 0 && fogFrame != 2 && P3DS_G3ReuseCfg() && (outL || outR)) {
        const unsigned tot = vn[0] + vn[1];
        if (tot == 0 && fogFrame == 0 && !ruVerify) {
            const u32 w = convWord(clearRGBA);
            for (int e = 0; e < eyes; e++) { u32 *o = e ? outR : outL; if (o) for (unsigned i = 0; i < 256 * 192; i++) o[i] = w; }
            G3G_StripValid = 0; ruFreeDeferred(); ruEmpty++; ruSaveUs += ruDraw ? (unsigned)(ruDrawUs / ruDraw) : 18000; lastUs = (unsigned)((svcGetSystemTick() - t0) / 268); ruLog(); return;
        }
        if (tot <= RU_MAXV && bn[0] <= RU_MAXB && bn[1] <= RU_MAXB) {
            RK k; memset(&k, 0, sizeof k);
            k.clr = clearRGBA; k.depth = clearD; k.eyes = eyes; k.haveL = outL != NULL; k.haveR = outR != NULL; k.fog = fogFrame; k.tg = texGen;
            k.dc = *(const u16 *)(PSPNative_GfxRegisters + 0x60); k.f350 = *(const u16 *)(PSPNative_GfxRegisters + 0x350);
            k.f35c = *(const u16 *)(PSPNative_GfxRegisters + 0x35C); k.fc = *(const u32 *)(PSPNative_GfxRegisters + 0x358);
            memcpy(k.tab, PSPNative_GfxRegisters + 0x360, 32); k.vn[0] = vn[0]; k.vn[1] = vn[1]; k.bn[0] = bn[0]; k.bn[1] = bn[1];
            ruCand = 1;
            if (ruValid && !memcmp(&k, &ruKey, sizeof k)) {
                int same = 1; for (int e = 0; e < eyes && same; e++) same = !memcmp(vb[e], ruV[e], vn[e] * sizeof(V)) && !memcmp(bt[e], ruB[e], bn[e] * sizeof(B));
                if (same) {
                    if (!ruVerify) {
                        u64 tc = svcGetSystemTick();
                        for (int e = 0; e < eyes; e++) { u32 *o = e ? outR : outL; if (o) memcpy(o, ruOut[e], 256 * 192 * 4); }
                        G3G_StripValid = 0; ruFreeDeferred(); ruHit++; ruSaveUs += ruDraw ? (unsigned)(ruDrawUs / ruDraw) : 18000;
                        ruHitUs += (unsigned)((svcGetSystemTick() - tc) / 268);
                        { static unsigned n; if (n++ < 3) PSPNativeMemLog("[G3REUSE] frame reused (tris %u, batches %u, tex gen %u)", tot / 3, bn[0], texGen); }
                        lastUs = (unsigned)((svcGetSystemTick() - t0) / 268); ruLog(); return;
                    }
                    ruCheck = 1;
                }
            }
            ruKey = k;
        }
        else ruBig++;
    }
    P3DS_Crumb("g3e-begin", 0); /* run 65: locate the boot g3 stall */
    C3D_FrameBegin(0);
    P3DS_Crumb("g3e-draw", 0);
    if (G3G_Async) { /* previous frame's GPU work is finished (FrameBegin waited): read it back, then free its textures */
        if (pendRB) for (int e = 0; e < pendEyes; e++) { if (rs) rbConvert(e, e ? outR : outL); else readback(e, e ? outR : outL); }
        for (unsigned i = 0; i < oldN; i++) { C3D_TexDelete(oldFree[i]); free(oldFree[i]); }
        oldN = 0;
    }
    C3D_BindProgram(&prog);
    C3D_AttrInfo *ai = C3D_GetAttrInfo(); AttrInfo_Init(ai);
    AttrInfo_AddLoader(ai, 0, GPU_FLOAT, 4); AttrInfo_AddLoader(ai, 1, GPU_FLOAT, 2); AttrInfo_AddLoader(ai, 2, GPU_FLOAT, 4);
    C3D_CullFace(GPU_CULL_NONE); C3D_DepthMap(G3G_WDepthOn() != 2, -1.0f, 0.0f);
    C3D_AlphaTest(true, GPU_GREATER, 3);   /* DS: texel alpha 0 / final 5-bit alpha 0 are not drawn */
    for (int k = 1; k < 6; k++) C3D_TexEnvInit(C3D_GetTexEnv(k));
    u32 fogClr = clearRGBA;
    if (fogFrame == 1 && (*(const u16 *)(PSPNative_GfxRegisters + 0x350) & 0x8000)) { const u32 fc = *(const u32 *)(PSPNative_GfxRegisters + 0x358);
        const int f = (int)fogDens((u32)(clearD * 0xFFFFFF)); const int fc3[3] = { (int)((fc & 31) * 255 / 31), (int)(((fc >> 5) & 31) * 255 / 31), (int)(((fc >> 10) & 31) * 255 / 31) };
        fogClr = clearRGBA & 255; for (int c = 0; c < 3; c++) { int v = (clearRGBA >> (24 - 8 * c)) & 255; v = (fc3[c] * f + v * (128 - f)) >> 7; fogClr |= (u32)v << (24 - 8 * c); } }
    for (int e = 0; e < eyes; e++) {
        if (!depthLin) C3D_RenderTargetClear(tgt[e], C3D_CLEAR_ALL, fogFrame == 1 ? fogClr : clearRGBA, depth16 ? (u32)(clearD * 0xFFFF) : ((u32)(clearD * 0xFFFFFF) | (fogFrame == 2 && (*(const u16 *)(PSPNative_GfxRegisters + 0x350) & 0x8000) ? 0x01000000u : 0)));
        C3D_FrameDrawOn(tgt[e]);   /* viewport = whole target (256/320 x 256, or 2x x 384) */
        if (depthLin) clearQuad();
        C3D_BufInfo *bi = C3D_GetBufInfo(); BufInfo_Init(bi); BufInfo_Add(bi, vb[e], sizeof(V), 3, 0x210);
        for (int pass = 0; pass < 2; pass++) for (unsigned i = 0; i < bn[e]; i++) {
            B *b = &bt[e][i]; if (!b->count || b->late != pass) continue;
            C3D_TexEnv *env = C3D_GetTexEnv(0); C3D_TexEnvInit(env);
            if (b->tex) { C3D_TexSetWrap(b->tex, b->wrapS, b->wrapT); C3D_TexBind(0, b->tex);
                C3D_TexEnvSrc(env, C3D_Both, GPU_PRIMARY_COLOR, GPU_TEXTURE0, 0); C3D_TexEnvFunc(env, C3D_Both, GPU_MODULATE); }
            else { C3D_TexEnvSrc(env, C3D_Both, GPU_PRIMARY_COLOR, 0, 0); C3D_TexEnvFunc(env, C3D_Both, GPU_REPLACE); }
            if (b->blend) C3D_AlphaBlend(GPU_BLEND_ADD, GPU_BLEND_MAX, GPU_SRC_ALPHA, GPU_ONE_MINUS_SRC_ALPHA, GPU_ONE, GPU_ONE);
            else C3D_AlphaBlend(GPU_BLEND_ADD, GPU_BLEND_ADD, GPU_ONE, GPU_ZERO, GPU_ONE, GPU_ZERO);
            if (fogFrame == 1) C3D_FogGasMode(b->fog ? GPU_FOG : GPU_NO_FOG, GPU_PLAIN_DENSITY, false);
            if (fogFrame == 2) { C3D_StencilTest(true, GPU_ALWAYS, b->fog ? 1 : 0, 0xFF, b->blend ? 0x00 : 0xFF); C3D_StencilOp(GPU_STENCIL_KEEP, GPU_STENCIL_KEEP, GPU_STENCIL_REPLACE); }
            C3D_DepthTest(true, b->dfunc, b->dwrite ? GPU_WRITE_ALL : GPU_WRITE_COLOR);
            C3D_DrawArrays(GPU_TRIANGLES, b->first, b->count);
        }
        if (fogFrame == 1) C3D_FogGasMode(GPU_NO_FOG, GPU_PLAIN_DENSITY, false);
        if (fogFrame == 2) { C3D_StencilTest(false, GPU_ALWAYS, 0, 0xFF, 0xFF);
            C3D_SyncDisplayTransfer((u32 *)tgt[e]->frameBuf.depthBuf, GX_BUFFER_DIM(curW, 256), drb[e], GX_BUFFER_DIM(curW, 256), RB_FLAGS); }
        if (rs && sharedTgt) rbQueue(e);   /* shared 2x target: copy this eye out before the next eye clears it */
    }
    if (rs && !sharedTgt) for (int e = 0; e < eyes; e++) rbQueue(e);  /* GPU runs these after the draws, same queue */
    P3DS_Crumb("g3e-end", 0);
    C3D_FrameEnd(0);
    P3DS_Crumb("g3e-wait", 0);
    if (rs && !G3G_Async) { C3D_FrameBegin(0); C3D_FrameEnd(GX_CMDLIST_FLUSH); }
    P3DS_Crumb("g3e-done", 0); /* exact wait: queue (draw + readback DMA) done */
    if (G3G_Async) { pendRB = 1; pendEyes = eyes; memcpy(oldFree, pendFree, pendN * sizeof *pendFree); oldN = pendN; pendN = 0; }
    else {
        pendRB = 0; for (unsigned i = 0; i < oldN; i++) { C3D_TexDelete(oldFree[i]); free(oldFree[i]); } oldN = 0;
        { u64 tc = svcGetSystemTick(); for (int e = 0; e < eyes; e++) { if (rs) rbConvert(e, e ? outR : outL); else readback(e, e ? outR : outL); } G3G_ConvUs = (unsigned)((svcGetSystemTick() - tc) / 268); }
        G3G_StripValid = wideFrame ? eyes : 0;
        for (unsigned i = 0; i < pendN; i++) { C3D_TexDelete(pendFree[i]); free(pendFree[i]); }
        pendN = 0;
    }
    if (ruCand) {
        if (!ruV[0]) for (int e = 0; e < 2; e++) { ruV[e] = malloc(RU_MAXV * sizeof(V)); ruB[e] = malloc(RU_MAXB * sizeof(B)); ruOut[e] = malloc(256 * 192 * 4); }
        if (ruV[0] && ruV[1] && ruB[0] && ruB[1] && ruOut[0] && ruOut[1]) {
            if (ruCheck) { for (int e = 0; e < eyes; e++) { const u32 *o = e ? outR : outL; if (o && memcmp(o, ruOut[e], 256 * 192 * 4)) { ruBad++;
                static unsigned nb; if (nb++ < 10) PSPNativeMemLog("[G3REUSE] VERIFY MISMATCH eye %d (tris %u): reused frame differs from a fresh render", e, vn[e] / 3); } } }
            for (int e = 0; e < eyes; e++) { memcpy(ruV[e], vb[e], vn[e] * sizeof(V)); memcpy(ruB[e], bt[e], bn[e] * sizeof(B));
                const u32 *o = e ? outR : outL; if (o) memcpy(ruOut[e], o, 256 * 192 * 4); }
            ruValid = 1; ruDraw++; ruDrawUs += (unsigned)((svcGetSystemTick() - t0) / 268);
        } else ruValid = 0;
    }
    lastUs = (unsigned)((svcGetSystemTick() - t0) / 268);
    { static unsigned f; if (++f % 120 == 0)
 { unsigned late = 0, tx = 0; for (unsigned i = 0; i < bn[0]; i++) { if (bt[0][i].late) late += bt[0][i].count / 3; if (bt[0][i].tex) tx += bt[0][i].count / 3; }
        PSPNativeMemLog("[GPU3D] frame: eyes %d | tris %u (textured %u, translucent %u) | batches %u | gpu %u us", eyes, vn[0] / 3, tx, late, bn[0], lastUs); } }
}
