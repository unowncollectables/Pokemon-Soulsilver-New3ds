/* 2026-10-27 dc_flush_skip (Debug tab "Skip game cache flushes", default OFF), ported from the Platinum port (run 102, heapfast.c).
   The game calls DC_FlushRange before handing buffers to DS DMA / GPU hardware. pspoke's os_sync.c turns that into
   sceKernelDcacheWritebackInvalidateRange, which psp_ctru.c maps to GSPGPU_FlushDataCache: one GSP IPC per call (Platinum
   counted 70 in one field-load frame). Here the consumers of those buffers are CPU code (memcpy VRAM, software 2D/3D, mixer)
   and every buffer that reaches the 3DS GPU or DSP is flushed by our own platform code (g3gpu.c, gpucomp.c, gpuscale.c,
   psp_ctru.c DSP_FlushDataCache). pspoke already wraps DC_FlushRange itself (mi_hwaddr.c, address fixups), so this wraps the
   call one level down. The only other caller of that shim is the audio grain in sas_out.c (exactly SAS_GRAIN*2*2 = 1024 bytes),
   which is always passed through. OFF flushes exactly as before; with it OFF the log still says how many flushes the game made
   and how long they took, so one OFF run and one ON run compare directly. Heap free (Platinum's heap_fast) is NOT ported: it
   skipped table scans in pokeplatinum's WIN_CheckAndFree*, which do not exist in the SoulSilver build. */
extern int P3DS_CfgInt(const char *, int);
extern void PSPNativeMemLog(const char *, ...);
extern void __real_sceKernelDcacheWritebackInvalidateRange(const void *, unsigned);
extern unsigned long long svcGetSystemTick(void);

#define AUDIO_GRAIN_BYTES 1024u
#define LOG_EVERY 200u
static int cfg = -1;
static unsigned calls, bytes, flushedUs, since;
static unsigned long long ticks;

static int on(void) {
    if (cfg < 0) { cfg = P3DS_CfgInt("dc_flush_skip", 0) != 0; PSPNativeMemLog("[HEAP] dc_flush_skip=%d (1 = the game's own DC_FlushRange calls are ignored; audio and platform flushes are untouched)", cfg); }
    return cfg;
}
void P3DS_SetDcFlushSkip(int v) { on(); cfg = v != 0; PSPNativeMemLog("[HEAP] dc_flush_skip=%d from settings", cfg); }
int P3DS_DcFlushSkipCfg(void) { return on(); }

void __wrap_sceKernelDcacheWritebackInvalidateRange(const void *p, unsigned n) {
    if (n == AUDIO_GRAIN_BYTES || !on()) {
        const unsigned long long t0 = svcGetSystemTick();
        __real_sceKernelDcacheWritebackInvalidateRange(p, n);
        if (n != AUDIO_GRAIN_BYTES) { ticks += svcGetSystemTick() - t0; calls++; bytes += n; since++; }
    } else { calls++; bytes += n; since++; }
    if (since >= LOG_EVERY || calls == 1) {
        since = 0; flushedUs = (unsigned)(ticks / 268);
        PSPNativeMemLog(cfg ? "[HEAP] dc_flush_skip: %u game flushes skipped so far (%u KB)" : "[HEAP] dc_flush: %u game flushes so far (%u KB), %u us spent in them",
                        calls, bytes >> 10, flushedUs);
    }
}
