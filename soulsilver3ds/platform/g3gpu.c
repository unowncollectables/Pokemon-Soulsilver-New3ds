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
extern void P3DS_FrameEnd(void);  /* gpuscale.c: C3D_FrameEnd, lean flush with fps_tweaks >= 1 */
typedef struct { float x, y, z, w, s, t, r, g, b, a; } V;
typedef struct { C3D_Tex *tex; int wrapS, wrapT, blend, dwrite, dfunc, late, fog; unsigned first, count; } B;
/* 2026-10-25: 3*8192 -> 3*16384 (+1 MiB linear per eye) for the removed GPU SPLIT mode; kept at 3*16384 until a device test shows 3*8192 is enough again. */
#define MAXV (3 * 16384)
#define MAXB 2048
static unsigned vn[2];
static V *vb[2]; static B bt[2][MAXB]; static unsigned bn[2];
static int ok = -1; static DVLB_s *dvlb; static shaderProgram_s prog;
static C3D_Tex rt[2]; static C3D_RenderTarget *tgt[2]; static u32 *rb[2];
static C3D_Tex *pendFree[512]; static unsigned pendN; static C3D_Tex *oldFree[512]; static unsigned oldN;
/* gpu3d_async=1: the GPU draws frame N while the CPU composites; frame N's readback happens at End(N+1)
   (3D layer shows one frame late). Frees are held one extra frame. */
int G3G_Async = 0; static int pendRB, pendEyes;
/* 2026-10-26 gpu_direct3d (Debug tab, default OFF): the 3D layer stays on the GPU. The old path per 3D frame is draw -> wait ->
   readback DMA -> CPU 6-bit convert (NativeFrame) -> CPU re-swizzle into the compositor's texture -> upload. With direct on, G3G_End
   queues the draw and returns without waiting; gpucomp.c samples the 256x256 eye target rt[e] itself as the 3D-layer texture (same
   tiled RGBA8 layout, same row order as its own conv3dRaw output) and NativeFrame is left stale. Any consumer that needs the pixels on
   the CPU (CPU composite, stereo fallback, dumps) calls G3G_DirectResolve(), which does the readback + convert then.
   Colour is 8 bit here instead of the DS's 6 bit expanded to 8. Only plain 256x256 1x frames, rb_sync, gpu_compose=2, no CPU fog.
   dirValid: rt[e] holds the frame that dirKey[e] (the NativeFrame / right-eye pointer) stands for. dirPend: NativeFrame is stale.
   dirOut: GPU work may still be in flight (cleared when gpucomp's own C3D_FrameBegin has waited, or by a sync). */
/* 2026-10-26 wide direct (same toggle; config-only gpu_direct3d_wide=0 turns just this part off): with Wide view on, the eye
   targets are 320x256 plain framebuffers (not textures, 320 is not a power of two), so the compositor cannot sample them.
   G3G_End now also queues, after the draws and in the same GPU queue: (1) a GX texture copy of the centre columns 32..287
   (tile columns 4..35 of every 8-row tile row: 8192 bytes taken, 2048 skipped) into dirTex[e], a 256x256 RGBA8 texture
   in linear RAM with exactly the layout of the 256 target, which gpucomp samples as before; (2) the usual readback DMA into
   rb[e]. The CPU then converts only the two 32-px side strips (G3G_DirectStrips, called before present; the GPU work is
   already done by then because gpucomp's FrameBegin waited) instead of the whole 256x192 centre. dirTex is allocated on
   the first wide direct frame and freed with the wide targets (freeTargets). dirWide: dirTex holds this frame's centre;
   dirStrip: G3G_Strip is not converted yet; dirRb: rb[] readback already queued for this frame. */
static int dirCfg = -1, dirValid, dirPend, dirEyes, dirOut, ruRt;  /* ruRt: rt[] holds the frame saved in ruKey/ruV (direct reuse) */ static u32 *dirKey[2];
static int dirWideCfg = -1, dirWide, dirStrip, dirRb, dirTexOk, dirTexFail; static C3D_Tex dirTex[2];
static unsigned dirDirect, dirReuse, dirResolved, dirUsed, dirPlain, dirWhy[6], dirWideN, dirStripN, dirResWhy[8]; static unsigned long long dirUs, dirResUs, dirPlainUs, dirStripUs;
extern int P3DS_GpuComposeCfg(void);
int G3G_DirectCfg(void) { if (dirCfg < 0) { extern int P3DS_CfgInt(const char *, int); dirCfg = P3DS_CfgInt("gpu_direct3d", 0) != 0; PSPNativeMemLog("[GPU3D] gpu_direct3d=%d (1 = 3D layer stays on the GPU: no readback/convert/upload when the GPU compositor takes the frame)", dirCfg); } return dirCfg; }
void G3G_SetDirect(int v) { dirCfg = v != 0; PSPNativeMemLog("[GPU3D] gpu_direct3d=%d from settings", dirCfg); }
static void dirSync(void) { if (dirOut) { dirOut = 0; C3D_FrameBegin(0); C3D_FrameEnd(GX_CMDLIST_FLUSH); } }
void G3G_DirectSynced(void) { dirOut = 0; }
int G3G_DirectPending(void) { return dirPend; }
void G3G_DirectInvalidate(void) { dirValid = dirPend = 0; }
C3D_Tex *G3G_DirectTex(const u32 *f) {
    if (!dirValid || !f) return NULL;
    for (int e = 0; e < dirEyes; e++) if (dirKey[e] == f) { dirUsed++; return dirWide ? &dirTex[e] : &rt[e]; }
    return NULL;
}
static u32 clearRGBA; static float clearD; static int eyes;
/* ss3ds g3_wdepth (Settings > 3D > W-buffer depth, default ON): g3soft gives every raw-mode vertex a depth linear in w
   (DS W-buffer; the title/Lugia projection has z == w). The PICA interpolates z/w affinely in screen space, which is wrong
   for a value linear in w: big body triangles got bad depth and Lugia's spikes/wings poked through or floated. With the
   PICA's own W-buffer mode (C3D_DepthMap(false)) the per-vertex depth -d is interpolated perspective-correctly. 0 = old. */
/* g3_wmode in config.txt (no menu row any more): 0 off, 1 PICA W = default, 2 GPU Z. G3G_WDepth keeps the internal codes:
   0 off, 2 PICA W, 3 GPU Z (the removed 1 = CPU 3D for W frames and 4 = GPU split did not fix the title Lugia; the real
   fix was the clip-matrix patch 0095). Retired keys g3_wcpu and g3_wpersp are ignored. */
int G3G_WDepth = -1, G3G_WMode = -1;
static const int kWModeToCode[3] = {0, 2, 3};
int G3G_WDepthOn(void) { if (G3G_WDepth < 0) { extern int P3DS_CfgInt(const char *, int); G3G_WMode = P3DS_CfgInt("g3_wmode", 1); if (G3G_WMode < 0 || G3G_WMode > 2) G3G_WMode = 1; G3G_WDepth = kWModeToCode[G3G_WMode]; PSPNativeMemLog("[GPU3D] g3_wmode=%d (0 off, 1 PICA W, 2 GPU Z) -> depth code %d", G3G_WMode, G3G_WDepth); } return G3G_WDepth; }
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
    /* fps_tweaks 5 (CPU trims): no fade and nothing to fill = one test and one OR per pixel, bit-identical to the loop below.
       Pixels with alpha < 8 (and the edge fill that may replace them) go through the same code as before. */
    extern int P3DS_FpsTweaks(void);
    const int quick = P3DS_FpsTweaks() >= 5 && !(f && (mm == 1 || mm == 2));
    for (unsigned y = 0; y < 192; y++) { const u32 *row = s + y * 320;
        for (unsigned x = 0; x < 64; x++) { u32 c = row[x < 32 ? x : x + 256];
            if (quick && (c & 255) >= 8) { d[y * 64 + x] = c | 0xFFu; continue; }
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
static void freeTargets(void) { dirSync(); dirValid = dirPend = ruRt = dirWide = dirStrip = dirRb = 0; if (dirTexOk) { for (int e = 0; e < 2; e++) C3D_TexDelete(&dirTex[e]); memset(dirTex, 0, sizeof dirTex); dirTexOk = 0; } if (tgt[1] == tgt[0]) tgt[1] = NULL; sharedTgt = 0; if (linDepth) linearFree(linDepth); linDepth = NULL; depthLin = depth16 = 0; for (int e = 0; e < 2; e++) { if (tgt[e]) C3D_RenderTargetDelete(tgt[e]); tgt[e] = NULL; if (rt[e].data) C3D_TexDelete(&rt[e]); memset(&rt[e], 0, sizeof rt[e]); if (rb[e]) linearFree(rb[e]); rb[e] = NULL; } }
/* (re)create the eye targets: w = 256 | 320 compositor width, hi = 2x. GPU idle: called at frame begin, sync path. */
static int setTargets(int w, int hi) {
    if (w == curW && hi == curHi) return 1;
    C3D_FrameBegin(0); C3D_FrameEnd(GX_CMDLIST_FLUSH);   /* nothing may still reference the old targets */
    freeTargets();
    unsigned long vf = (unsigned long)vramSpaceFree(); int okW = 0; const char *depth = "24";
    if (!hi && w == 256) {   /* the original 256x256 texture targets */
        okW = 1; for (int e = 0; e < 2 && okW; e++) { rb[e] = linearAlloc(256 * 256 * 4); if (!rb[e] || !C3D_TexInitVRAM(&rt[e], 256, 256, GPU_RGBA8) || !(tgt[e] = C3D_RenderTargetCreateFromTex(&rt[e], GPU_TEXFACE_2D, 0, GPU_RB_DEPTH24_STENCIL8))) okW = 0; else { C3D_TexSetFilter(&rt[e], GPU_NEAREST, GPU_NEAREST); C3D_TexSetWrap(&rt[e], GPU_CLAMP_TO_EDGE, GPU_CLAMP_TO_EDGE); } }
    } else {
        const int tw = hi ? 2 * w : w, th = hi ? 384 : 256;
        if (linDepthCfg < 0) { linDepthCfg = 0; FILE *f = fopen("sdmc:/3ds/soulsilver3ds/config.txt", "r"); if (f) { char l[64]; int x; while (fgets(l, sizeof l, f)) if (sscanf(l, "res3d_lindepth=%d", &x) == 1) linDepthCfg = x != 0; fclose(f); } PSPNativeMemLog("[3DRES] res3d_lindepth=%d", linDepthCfg); }
        if (!qv) qv = linearAlloc(6 * sizeof(V));
        /* 2026-10-26: pass 3 = colour AND 24-bit depth in linear RAM (run 7 hardware log: 512x384 FAILED with 1.8 MB VRAM
           free in pieces, so not even the colour buffer found one 768 KB hole). Tried before the 16-bit depth pass. */
        /* run 49 hardware log (2026-10-28): with colour+depth in linear RAM (pass 3, PR #169) the 3D GPU draw+readback took
           50 ms instead of 0.5 ms (14 fps) and the picture was garbage (top strips, black bands), on the title Lugia too. The
           PICA does not render usably into FCRAM (citro3d refuses it for a reason). Pass 3 is off; linear depth (pass 1) is
           config-only now (res3d_lindepth=1, default 0) for the same reason. 2x targets are VRAM-only: they need VRAM freed. */
        static const int passOrder[3] = { 0, 1, 2 };
        for (int pi = 0; pi < (hi ? 3 : 1) && !okW; pi++) { const int pass = passOrder[pi];
            if ((pass == 1 || pass == 3) && (!linDepthCfg || !qv)) continue;
            const int lin = pass == 1 || pass == 3, linCol = pass == 3;
            const GPU_DEPTHBUF df = pass == 2 ? GPU_RB_DEPTH16 : GPU_RB_DEPTH24_STENCIL8; depth = pass == 2 ? "16" : linCol ? "24, colour+depth in linear RAM" : lin ? "24 in linear RAM" : "24";
            if (pass) freeTargets();
            /* run 48: res3d shares ONE 2x target between the eyes (draw L, HW-downscale copy L, clear, draw R, copy R:
               all in GPU queue order). Run 47 allocated two and never fitted: 2 x 1.2-2.0 MiB vs 2.07 MiB free. */
            okW = 1; for (int e = 0; e < 2 && okW; e++) { rb[e] = linearAlloc(w * 256 * 4); if (!rb[e]) { okW = 0; break; }
                if (hi && e == 1) { tgt[1] = tgt[0]; sharedTgt = 1; continue; }
                if (lin) {
                    if (linCol) { /* colour buffer = a linear 2^n texture's storage; frame buffer size overridden to tw x th.
                           2026-10-28 (Azahar run of run 48): citro3d's C3D_RenderTargetCreateFromTex returns NULL for a texture that is
                           not in VRAM, so this pass never worked (title "512x384 targets FAILED" on hardware run 7 too). The target is
                           now made from a tiny VRAM texture and then pointed at the linear colour buffer (the PICA can render to FCRAM). */
                        static C3D_Tex dumTex; static int dumOk = -1;
                        if (dumOk < 0) dumOk = C3D_TexInitVRAM(&dumTex, 8, 8, GPU_RGBA8) ? 1 : 0;
                        if (dumOk && C3D_TexInit(&rt[e], tw <= 512 ? 512 : 1024, 512, GPU_RGBA8) && (tgt[e] = C3D_RenderTargetCreateFromTex(&dumTex, GPU_TEXFACE_2D, 0, -1))) {
                            C3D_FrameBufColor(&tgt[e]->frameBuf, rt[e].data, GPU_RB_RGBA8); C3D_FrameBufAttrib(&tgt[e]->frameBuf, tw, th, false); }
                        else PSPNativeMemLog("[3DRES] linear colour target: dummy VRAM tex %d, linear tex %s", dumOk, rt[e].data ? "ok" : "FAILED");
                    } else
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
        C3D_TexSetFilter(&rt[e], GPU_NEAREST, GPU_NEAREST); C3D_TexSetWrap(&rt[e], GPU_CLAMP_TO_EDGE, GPU_CLAMP_TO_EDGE);
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
void G3G_TexUpdate(void *tp, const u32 *px, unsigned w, unsigned h) { C3D_Tex *t = tp; if (!t || !px || t->width != w || t->height != h) return; dirSync(); texFill(t, px, w, h); }
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
    dirSync();   /* gpu_direct3d: the previous frame's draw may still be running; the vertex buffers and textures are about to change */
    unsigned r = nativeClear & 63, g = (nativeClear >> 8) & 63, b = (nativeClear >> 16) & 63, a = (nativeClear >> 24) & 31;
    clearRGBA = ((r << 2 | r >> 4) << 24) | ((g << 2 | g >> 4) << 16) | ((b << 2 | b >> 4) << 8) | (a * 255 / 31);
    clearD = clearDepth16 / 65535.0f; eyes = stereo ? 2 : 1; vn[0] = vn[1] = bn[0] = bn[1] = 0;
    { const int want = G3G_Wide && !wideFail && !G3G_Async && rbSyncOn() ? 320 : 256;
      if (res3dCfg < 0) { res3dCfg = 0; PSPNativeMemLog("[3DRES] res3d disabled (run 52: broken on hardware; config res3d ignored)"); }
      /* 2026-10-26 g3_smooth (default 1, Settings > 3D > Smooth title 3D): the title Lugia (DS W-buffer frames; on the BOTTOM screen via the POWCNT
         swap, so never stereo: P3DS_StereoShiftNow is 0 there) uses the res3d 2x target + hardware 2x2 box downscale, i.e. 4x supersampled edges. At 1x Nathan found the title
         model blocky next to footage of the real game. Turns on after 30 W-buffer frames in a row (no target churn
         on short W bursts), off at the first other frame. Other scenes stay at 1x (res3d stays disabled there). */
      static unsigned smRun; static int smLast = -1; extern u8 s_SIM_useWBuffer; extern int P3DS_CfgInt(const char *, int);
      smRun = s_SIM_useWBuffer && !stereo ? smRun + 1 : 0;
      /* 2026-10-28: only with SHARP 3D on. Its 2x target never fitted before (all passes FAILED, title at 1x = the known-good look);
         with SHARP 3D off the title stays on that 1x path whatever the VRAM room work does. */
      const int sm = smRun >= 30 && P3DS_CfgInt("g3_smooth", 1) != 0 && P3DS_CfgInt("g3_sharp", 0) != 0;
      if (sm != smLast) { smLast = sm; PSPNativeMemLog("[LUGIA] smooth title 3D (2x supersampled) %s | 3D on the %s screen (POWCNT %04x)", sm ? "ON" : "off", (*(const u16 *)(PSPNative_GfxRegisters + 0x304) & 0x8000) ? "TOP" : "BOTTOM", *(const u16 *)(PSPNative_GfxRegisters + 0x304)); }
      /* 2026-10-28 g3_sharp (default 0, Settings > 3D > Sharp 3D, remove once tested): the same 2x target + box downscale for
         every scene (overworld, battles), mono or stereo (one target shared by both eyes, run 48). The run 52 "garbage" was the
         16-bit depth fallback (Platinum run 49 notes); depth is now 24-bit in VRAM or linear RAM and only 16 as a last resort.
         Not on fog frames (DISP3DCNT bit 7: the GPU fog needs the 1x target, so those stay 1x and keep their fog), and only
         after 30 eligible frames in a row so a fog flicker does not churn targets. GPU 3D direct and frame reuse are 1x-only. */
      static unsigned shRun, shN, shOn, shFog; static int shLast = -1;
      const int shCfg = P3DS_CfgInt("g3_sharp", 0) != 0, shFogNow = (*(const u16 *)(PSPNative_GfxRegisters + 0x60) & 0x80) != 0;
      shRun = shCfg && !shFogNow ? shRun + 1 : 0;
      const int sh = shRun >= 30;
      if (shCfg) { shN++; shOn += curHi == 1; shFog += shFogNow; if (shN >= 600) { PSPNativeMemLog("[3DRES] sharp 3D: %u of %u frames at 2x, %u fog frames kept 1x, depth %s, %s", shOn, shN, shFog, depth16 ? "16 (stripes possible!)" : depthLin ? "24 linear" : "24 VRAM", res3dFail ? "FAILED (no memory, 1x)" : "ok"); shN = shOn = shFog = 0; } }
      if (sh != shLast) { shLast = sh; PSPNativeMemLog("[3DRES] sharp 3D (g3_sharp=%d) %s%s", shCfg, sh ? "ON" : "off", shCfg && shFogNow ? " (fog frame)" : ""); }
      const int wantHi = (res3dCfg == 2 || sm || sh) && !res3dFail && !G3G_Async && rbSyncOn() ? 1 : 0;
      if ((want != curW || wantHi != curHi) && ok == 1) {
        /* 2026-10-28: the 2x target only works in VRAM (run 49), and VRAM is full of the scale targets: free them first,
           make the 2x target, then get the scale targets back at 16 bits (gpuscale.c P3DS_UpAlloc). Back at 1x: RGBA8 again. */
        extern void P3DS_UpFree(void); extern int P3DS_UpAlloc(int);
        /* run 50 (Azahar): going 512x384 -> 640x384 with the 16-bit scale targets still in place FAILED (VRAM in pieces), the 1x
           fallback then took the holes and the RGBA8 scale targets no longer fitted: no frame was presented. So whenever a 2x
           target is involved (old or new), the scale targets are freed first and allocated last, and P3DS_UpAlloc falls back to
           the other format rather than leaving none. */
        const int dance = wantHi || curHi == 1;
        if (dance) P3DS_UpFree();
        setTargets(want, wantHi);
        if (dance && !P3DS_UpAlloc(curHi == 1)) {
          PSPNativeMemLog("[3DRES] scale targets did not fit next to the %s target, back to 1x", curHi == 1 ? "2x" : "1x");
          if (curHi == 1) { extern void P3DS_Toast(const char *); res3dFail = 1; P3DS_Toast("SHARP 3D: NO VRAM"); }
          P3DS_UpFree(); setTargets(want, 0); P3DS_UpAlloc(0); }
        /* run 50 hardware log: a 320x256 (wide) target that failed once during the 2x dance set wideFail for the rest of the
           session ("my wide screen is not loading anymore"). After every 2x change the VRAM layout is new: try wide again. */
        if (dance && wideFail) { wideFail = 0; PSPNativeMemLog("[3DRES] wide view retried after the VRAM change"); } }
      if (sh && curHi == 1 && depth16 && !sm) { res3dFail = 1; PSPNativeMemLog("[3DRES] sharp 3D: only 16-bit depth fitted (the run 48 stripes), back to 1x"); { extern void P3DS_Toast(const char *); P3DS_Toast("SHARP 3D: NO MEMORY"); } setTargets(want, 0); { extern int P3DS_UpAlloc(int); P3DS_UpAlloc(0); } }
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
typedef struct { u32 clr, fc, tg; float depth; int eyes, haveL, haveR, fog, dir; u16 dc, f350, f35c, pad; unsigned vn[2], bn[2]; unsigned char tab[32]; } RK;
static RK ruKey; static V *ruV[2]; static B *ruB[2]; static u32 *ruOut[2]; static int ruValid, ruVerify = -1;
static unsigned ruHit, ruEmpty, ruDraw, ruBig, ruBad; static unsigned long long ruSaveUs, ruHitUs, ruDrawUs; static unsigned ruFrames;
extern int P3DS_G3ReuseCfg(void); extern int P3DS_CfgInt(const char *, int);
static u32 convWord(u32 c) { const u32 t = (__builtin_bswap32(c) >> 2) & 0x3F3F3F3Fu; return (t & 0x003F3F3Fu) | ((t >> 1) & 0x1F000000u); }
static void ruFreeDeferred(void) {
    dirSync(); pendRB = 0; for (unsigned i = 0; i < oldN; i++) { C3D_TexDelete(oldFree[i]); free(oldFree[i]); } oldN = 0;
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
    static int cfg = -1, pfail; int on; static V *prev; static unsigned prevN, frames, tot, kind, near16, near4, still, shortRun; static double sumMove;
    extern int P3DS_CfgInt(const char *, int); extern int P3DS_InField;
    extern int P3DS_FpsTweaks(void);
    if (cfg < 0) cfg = P3DS_CfgInt("fps60_probe", 0) != 0;
    on = (cfg || P3DS_FpsTweaks() >= 5) && !pfail;   /* follows the live Speed tweaks level, not just the boot one */
    if (on && !prev) { prev = malloc(MAXV * sizeof(V)); if (!prev) { pfail = 1; on = 0; } }
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
/* gpu_direct3d: the CPU needs the 3D layer pixels after all (CPU composite, stereo fallback, dump): queue the readback of the
   still-valid eye targets, wait, and convert into the pointers the frame was begun with. */
/* why (logged in the stats line): 0 other, 1 flat stereo shift, 2 CPU right eye, 3 CPU composite, 4 GPU compose rejected,
   5 dump/diagnostic, 6 wide haze fit, 7 g3_shot */
void G3G_DirectResolveWhy(int why) {
    if (!dirPend || ok != 1) return;
    u64 t0 = svcGetSystemTick(); dirPend = 0; dirResolved++; dirResWhy[why & 7]++;
    if (!dirRb) {   /* wide direct frames already queued the readback in G3G_End */
        C3D_FrameBegin(0);
        for (int e = 0; e < dirEyes; e++) if (dirKey[e]) rbQueue(e);
        P3DS_FrameEnd(); dirOut = 1; }
    dirSync();
    for (int e = 0; e < dirEyes; e++) if (dirKey[e]) rbConvert(e, dirKey[e]);   /* 320 wide: also converts the strips */
    if (dirStrip) { for (int e = 0; e < dirEyes; e++) if (!dirKey[e]) { GSPGPU_InvalidateDataCache(rb[e], 320 * 256 * 4); stripConvert(e); } dirStrip = 0; }
    dirResUs += (unsigned long long)(svcGetSystemTick() - t0) / 268;
    { static unsigned n; if (n < 8 && !(dirResWhy[why & 7] > 1)) { n++; PSPNativeMemLog("[GPU3D] direct: 3D layer pulled back to the CPU (reason %d: %s)%s", why,
        (const char *[]){ "other", "flat stereo shift", "CPU right eye", "CPU composite", "GPU compose rejected", "dump/diagnostic", "wide haze fit", "g3_shot" }[why & 7], dirWide ? " | wide frame" : ""); } }
}
void G3G_DirectResolve(void) { G3G_DirectResolveWhy(0); }
/* wide direct: convert the side strips from the readback G3G_End queued (before gpuscale draws them) */
void G3G_DirectStrips(void) {
    if (!dirStrip || ok != 1) return;
    u64 t0 = svcGetSystemTick(); dirStrip = 0;
    dirSync();   /* normally a no-op: gpucomp's FrameBegin already waited for the draw, copy and readback */
    for (int e = 0; e < dirEyes; e++) { GSPGPU_InvalidateDataCache(rb[e], 320 * 256 * 4); stripConvert(e); }
    dirStripN++; dirStripUs += (unsigned long long)(svcGetSystemTick() - t0) / 268;
}
static void dirLog(void) {
    static unsigned n; if (++n % 600 != 0) return;
    PSPNativeMemLog("[GPU3D] direct 600 3D frames: %u direct (%u of them reused without a draw, %u wide, avg End %u us), %u normal (avg End %u us), compositor sampled the target %u times, %u resolved to the CPU (avg %u us) | not direct because: off-path %u, fog-cpu %u, async/hi %u, compose!=2 %u, verify %u, wide_fx/wide-off/no-mem %u",
        dirDirect, dirReuse, dirWideN, dirDirect ? (unsigned)(dirUs / dirDirect) : 0, dirPlain, dirPlain ? (unsigned)(dirPlainUs / dirPlain) : 0, dirUsed,
        dirResolved, dirResolved ? (unsigned)(dirResUs / dirResolved) : 0, dirWhy[0], dirWhy[1], dirWhy[2], dirWhy[3], dirWhy[4], dirWhy[5]);
    PSPNativeMemLog("[GPU3D] direct resolves by reason: other %u, flat-stereo %u, cpu-right-eye %u, cpu-composite %u, compose-rejected %u, dump/diag %u, wide-haze %u, g3_shot %u | wide strips converted %u times (avg %u us)",
        dirResWhy[0], dirResWhy[1], dirResWhy[2], dirResWhy[3], dirResWhy[4], dirResWhy[5], dirResWhy[6], dirResWhy[7], dirStripN, dirStripN ? (unsigned)(dirStripUs / dirStripN) : 0);
    dirDirect = dirReuse = dirPlain = dirUsed = dirResolved = dirWideN = dirStripN = 0; dirUs = dirResUs = dirPlainUs = dirStripUs = 0; memset(dirWhy, 0, sizeof dirWhy); memset(dirResWhy, 0, sizeof dirResWhy);
}
/* wide direct: config gate + the two 256x256 centre textures (linear RAM: VRAM is kept for the eye targets) */
static int dirWideOk(void) {
    if (dirWideCfg < 0) { dirWideCfg = P3DS_CfgInt("gpu_direct3d_wide", 1) != 0; PSPNativeMemLog("[GPU3D] gpu_direct3d_wide=%d (1 = direct mode also with Wide view: centre copied on the GPU, strips from the readback)", dirWideCfg); }
    if (!dirWideCfg || dirTexFail) return 0;
    { extern int P3DS_WideFxCfg(void); if (P3DS_WideFxCfg()) return 0; }   /* Wide FX (slow, default OFF) reads the centre on the CPU every frame */
    if (dirTexOk) return 1;
    for (int e = 0; e < 2; e++) if (!C3D_TexInit(&dirTex[e], 256, 256, GPU_RGBA8)) {
        for (int k = 0; k < e; k++) C3D_TexDelete(&dirTex[k]); memset(dirTex, 0, sizeof dirTex); dirTexFail = 1;
        PSPNativeMemLog("[GPU3D] direct: wide centre textures FAILED (linear free %lu): wide frames stay on the normal path", (unsigned long)linearSpaceFree()); return 0; }
    for (int e = 0; e < 2; e++) { C3D_TexSetFilter(&dirTex[e], GPU_NEAREST, GPU_NEAREST); C3D_TexSetWrap(&dirTex[e], GPU_CLAMP_TO_EDGE, GPU_CLAMP_TO_EDGE); }
    dirTexOk = 1; PSPNativeMemLog("[GPU3D] direct: wide centre textures ready (2 x 256 KiB linear, linear free %lu)", (unsigned long)linearSpaceFree());
    return 1;
}
/* 2026-10-27 fog_hole (default 1, 3D tab "Cave light fix"): on the DS a pixel's fog flag is the AND of the fog bits of its
   opaque polygon and of every translucent polygon drawn over it (melonDS: translucent pixel with polygon fog off clears
   the flag). HGSS Flash weather (11/12, Dark Cave) fogs the whole cave black (FOG_COLOR 0, table all 127) and draws a
   translucent no-fog light sphere at the player (ov01_02203F98) to clear the flag there: the small lit circle. The PICA
   fog unit (fog3d=1) fogs each fragment as it is drawn, so the sphere could not un-fog the floor and the cave was fully
   black on hardware. Fog frames that hold a translucent no-fog batch now take the exact stencil path (fog3d=2: costs
   ~6-10 ms CPU in those frames only), where translucent no-fog batches write stencil 0 and translucent fog batches keep it. */
static int fogHoleV = -1; static unsigned fogHoleN;
int P3DS_FogHoleCfg(void) { if (fogHoleV < 0) { extern int P3DS_CfgInt(const char *, int); fogHoleV = P3DS_CfgInt("fog_hole", 1) != 0; PSPNativeMemLog("[GPU3D] fog_hole=%d (1 = translucent no-fog polygons clear the fog like the DS: Dark Cave / Flash light circle)", fogHoleV); } return fogHoleV; }
void P3DS_SetFogHole(int v) { fogHoleV = v != 0; PSPNativeMemLog("[GPU3D] fog_hole=%d from settings", fogHoleV); }
static int fogHoleFrame(void) { for (int e = 0; e < eyes; e++) for (unsigned i = 0; i < bn[e]; i++) if (bt[e][i].count && bt[e][i].blend && !bt[e][i].fog) return 1; return 0; }
void G3G_End(u32 *outL, u32 *outR) {
    if (ok != 1) return;
    fps60Probe();
    u64 t0 = svcGetSystemTick();
    for (int e = 0; e < eyes; e++) GSPGPU_FlushDataCache(vb[e], vn[e] * sizeof(V));
    const int rs = rbSyncOn();
    fogFrame = 0;
    if (fogCfgOn() && (rs || fogCfg == 1) && (!G3G_Async || fogCfg == 1) && !depthLin && (fogCfg == 1 || (!curHi && !depth16)) && G3G_FogNoW && (*(const u16 *)(PSPNative_GfxRegisters + 0x60) & 0x80)) {
        int fm = fogCfg;
        if (fm == 1 && rs && !G3G_Async && !curHi && !depth16 && P3DS_FogHoleCfg() && fogHoleFrame()) { fm = 2;
            if (fogHoleN++ % 600 == 0) PSPNativeMemLog("[GPU3D] fog_hole: fog frame with a translucent no-fog batch (Dark Cave / Flash light) -> exact stencil fog (%u frames)", fogHoleN); }
        if (fm == 2 && !drb[0]) { for (int e = 0; e < 2; e++) drb[e] = linearAlloc(320 * 256 * 4);
            if (!drb[0] || !drb[1]) { for (int e = 0; e < 2; e++) { if (drb[e]) linearFree(drb[e]); drb[e] = NULL; }
                if (fogCfg == 2) { PSPNativeMemLog("[GPU3D] fog3d: no linear memory, fog off"); fogCfg = 0; fm = 0; }
                else { PSPNativeMemLog("[GPU3D] fog_hole: no linear memory (640 KiB), cave light fix off"); fogHoleV = 0; fm = 1; } } }
        fogFrame = fm; if (fogFrame == 1) { fogLutSetup(); const u32 fc = *(const u32 *)(PSPNative_GfxRegisters + 0x358);
            C3D_FogColor(((fc & 31) * 255 / 31) | (((fc >> 5) & 31) * 255 / 31) << 8 | (((fc >> 10) & 31) * 255 / 31) << 16); }
        if (fogFrame && ++fogFrames % 600 == 1) PSPNativeMemLog("[GPU3D] fog3d: fog frame (DISP3DCNT %04x, FOG_COLOR %08lx, offset %04x)", *(const u16 *)(PSPNative_GfxRegisters + 0x60), (unsigned long)*(const u32 *)(PSPNative_GfxRegisters + 0x358), *(const u16 *)(PSPNative_GfxRegisters + 0x35C)); }
    int ruCand = 0, ruCheck = 0;
    if (ruVerify < 0) { ruVerify = P3DS_CfgInt("g3_reuse_verify", 0) != 0; PSPNativeMemLog("[G3REUSE] g3_reuse_verify=%d", ruVerify); }
    int direct = 0;
    if (G3G_DirectCfg()) {
        if (!rs || !outL) dirWhy[0]++;
        else if (fogFrame == 2) dirWhy[1]++;
        else if (G3G_Async || curHi != 0 || depthLin || (curW != 256 && !(curW == 320 && wideFrame))) dirWhy[2]++;
        else if (P3DS_GpuComposeCfg() != 2) dirWhy[3]++;
        else if (ruVerify) dirWhy[4]++;
        else if (wideFrame && !dirWideOk()) dirWhy[5]++;
        else direct = 1;
    }
    if (!direct) { if (dirValid) dirSync(); dirValid = dirPend = 0; }
    dirWide = dirStrip = dirRb = 0;
    if (rs && !G3G_Async && curW == 256 && curHi == 0 && fogFrame != 2 && P3DS_G3ReuseCfg() && (outL || outR)) {
        const unsigned tot = vn[0] + vn[1];
        if (tot == 0 && fogFrame == 0 && !ruVerify && !direct) {
            const u32 w = convWord(clearRGBA);
            for (int e = 0; e < eyes; e++) { u32 *o = e ? outR : outL; if (o) for (unsigned i = 0; i < 256 * 192; i++) o[i] = w; }
            G3G_StripValid = 0; ruFreeDeferred(); ruEmpty++; ruSaveUs += ruDraw ? (unsigned)(ruDrawUs / ruDraw) : 18000; lastUs = (unsigned)((svcGetSystemTick() - t0) / 268); ruLog(); return;
        }
        if (tot <= RU_MAXV && bn[0] <= RU_MAXB && bn[1] <= RU_MAXB) {
            RK k; memset(&k, 0, sizeof k);
            k.clr = clearRGBA; k.depth = clearD; k.eyes = eyes; k.dir = direct; k.haveL = outL != NULL; k.haveR = outR != NULL; k.fog = fogFrame; k.tg = texGen;
            k.dc = *(const u16 *)(PSPNative_GfxRegisters + 0x60); k.f350 = *(const u16 *)(PSPNative_GfxRegisters + 0x350);
            k.f35c = *(const u16 *)(PSPNative_GfxRegisters + 0x35C); k.fc = *(const u32 *)(PSPNative_GfxRegisters + 0x358);
            memcpy(k.tab, PSPNative_GfxRegisters + 0x360, 32); k.vn[0] = vn[0]; k.vn[1] = vn[1]; k.bn[0] = bn[0]; k.bn[1] = bn[1];
            ruCand = 1;
            if (ruValid && !memcmp(&k, &ruKey, sizeof k)) {
                int same = 1; for (int e = 0; e < eyes && same; e++) same = !memcmp(vb[e], ruV[e], vn[e] * sizeof(V)) && !memcmp(bt[e], ruB[e], bn[e] * sizeof(B));
                if (same) {
                    if (!ruVerify && (!direct || ruRt)) {
                        u64 tc = svcGetSystemTick();
                        if (direct) { /* rt[e] still holds exactly this frame (nothing else draws into it) */
                            dirValid = 1; dirPend = 1; dirEyes = eyes; dirKey[0] = outL; dirKey[1] = outR; dirReuse++; dirDirect++; }
                        else
                        for (int e = 0; e < eyes; e++) { u32 *o = e ? outR : outL; if (o) memcpy(o, ruOut[e], 256 * 192 * 4); }
                        G3G_StripValid = 0; ruFreeDeferred(); ruHit++; ruSaveUs += ruDraw ? (unsigned)(ruDrawUs / ruDraw) : 18000;
                        ruHitUs += (unsigned)((svcGetSystemTick() - tc) / 268);
                        { static unsigned n; if (n++ < 3) PSPNativeMemLog("[G3REUSE] frame reused (tris %u, batches %u, tex gen %u)", tot / 3, bn[0], texGen); }
                        lastUs = (unsigned)((svcGetSystemTick() - t0) / 268); ruLog(); return;
                    }
                    ruCheck = ruVerify;
                }
            }
            ruKey = k;
        }
        else ruBig++;
    }
    P3DS_Crumb("g3e-begin", 0); /* run 65: locate the boot g3 stall */
    ruRt = 0;
    C3D_FrameBegin(0);
    P3DS_Crumb("g3e-draw", 0);
    if (!G3G_Async && oldN) { for (unsigned i = 0; i < oldN; i++) { C3D_TexDelete(oldFree[i]); free(oldFree[i]); } oldN = 0; }   /* gpu_direct3d: textures held back from the previous frame; the FrameBegin above waited for it */
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
            if (fogFrame == 2) { C3D_StencilTest(true, GPU_ALWAYS, b->fog ? 1 : 0, 0xFF, b->blend && b->fog ? 0x00 : 0xFF); C3D_StencilOp(GPU_STENCIL_KEEP, GPU_STENCIL_KEEP, GPU_STENCIL_REPLACE); }
            C3D_DepthTest(true, b->dfunc, b->dwrite ? GPU_WRITE_ALL : GPU_WRITE_COLOR);
            C3D_DrawArrays(GPU_TRIANGLES, b->first, b->count);
        }
        if (fogFrame == 1) C3D_FogGasMode(GPU_NO_FOG, GPU_PLAIN_DENSITY, false);
        if (fogFrame == 2) { C3D_StencilTest(false, GPU_ALWAYS, 0, 0xFF, 0xFF);
            C3D_SyncDisplayTransfer((u32 *)tgt[e]->frameBuf.depthBuf, GX_BUFFER_DIM(curW, 256), drb[e], GX_BUFFER_DIM(curW, 256), RB_FLAGS); }
        if (rs && sharedTgt) rbQueue(e);   /* shared 2x target: copy this eye out before the next eye clears it */
    }
    if (rs && !sharedTgt && !direct) for (int e = 0; e < eyes; e++) rbQueue(e);  /* GPU runs these after the draws, same queue */
    if (direct && wideFrame) for (int e = 0; e < eyes; e++) {   /* wide direct: centre 256 columns -> dirTex[e] (texture copy, tiled to tiled), then the strip readback */
        C3D_SyncTextureCopy((u32 *)((u8 *)tgt[e]->frameBuf.colorBuf + 4 * 256), GX_BUFFER_DIM(8192 >> 4, 2048 >> 4), (u32 *)dirTex[e].data, GX_BUFFER_DIM(8192 >> 4, 0), 8192 * 32, GX_TRANSFER_RAW_COPY(1));
        rbQueue(e); }
    P3DS_Crumb("g3e-end", 0);
    P3DS_FrameEnd();   /* fps_tweaks >= 1: no whole-linear-heap flush (vb[] and textures are flushed by their writers) */
    P3DS_Crumb("g3e-wait", 0);
    if (rs && !G3G_Async && !direct) { C3D_FrameBegin(0); C3D_FrameEnd(GX_CMDLIST_FLUSH); }
    P3DS_Crumb("g3e-done", 0); /* exact wait: queue (draw + readback DMA) done */
    if (G3G_Async) { pendRB = 1; pendEyes = eyes; memcpy(oldFree, pendFree, pendN * sizeof *pendFree); oldN = pendN; pendN = 0; }
    else if (direct) {   /* no wait, no readback: the compositor samples rt[e]. Textures this frame used stay alive until the next frame's FrameBegin has waited */
        pendRB = 0; for (unsigned i = 0; i < pendN && oldN < 512; i++) oldFree[oldN++] = pendFree[i]; pendN = 0;
        dirValid = dirPend = 1; dirEyes = eyes; dirKey[0] = outL; dirKey[1] = outR; dirOut = 1; dirDirect++;
        dirWide = dirStrip = dirRb = wideFrame; if (wideFrame) dirWideN++;
        G3G_StripValid = wideFrame ? eyes : 0;   /* wide: G3G_DirectStrips() fills G3G_Strip before present */
        { static int said; if (wideFrame && !said) { said = 1; PSPNativeMemLog("[GPU3D] direct: first wide frame (320x256 targets, %d eye%s): centre copied on the GPU into the compositor texture, only the side strips converted on the CPU", eyes, eyes > 1 ? "s" : ""); } }
    }
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
                const u32 *o = e ? outR : outL; if (o && !direct) memcpy(ruOut[e], o, 256 * 192 * 4); }
            ruRt = direct; ruValid = 1; ruDraw++; ruDrawUs += (unsigned)((svcGetSystemTick() - t0) / 268);
        } else ruValid = 0;
    }
    lastUs = (unsigned)((svcGetSystemTick() - t0) / 268);
    if (G3G_DirectCfg()) { if (direct) dirUs += lastUs; else { dirPlain++; dirPlainUs += lastUs; } dirLog(); }
    { static unsigned f; if (++f % 120 == 0)
 { unsigned late = 0, tx = 0; for (unsigned i = 0; i < bn[0]; i++) { if (bt[0][i].late) late += bt[0][i].count / 3; if (bt[0][i].tex) tx += bt[0][i].count / 3; }
        PSPNativeMemLog("[GPU3D] frame: eyes %d | tris %u (textured %u, translucent %u) | batches %u | gpu %u us", eyes, vn[0] / 3, tx, late, bn[0], lastUs); } }
}
