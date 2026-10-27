/* run 35: non-blocking log. Callers (any thread, incl. audio/game) only format + memcpy into a RAM ring under a
   short LightLock; a lowest-priority thread appends to sdmc:/3ds/soulsilver3ds/log.txt every 100 ms through one
   persistent FILE. Whole lines are copied atomically, so lines never interleave. Full ring = line dropped (counted). */
#include <3ds.h>
#include "crumbs.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#define RING (512 * 1024)
static char ring[RING]; static u32 head, tail; static LightLock lk; static int init; static Thread th; static unsigned dropped;
static FILE *lf; static char out[64 * 1024];
/* run 45 log_level (config, default 1): 1 = boot info, errors/failures, [SPIKE], [WIDE]/[SAVE]/[MEM]/[OVERLAY] events,
   [PERF] every 10th (~10 s), [AUDIO-CLK] every 4th (~11 s); the per-frame/per-second detail lines are NOT written but
   kept in a 128 KiB in-RAM "detail" ring that is appended to log.txt on every synchronous flush (crash handler,
   hold, exit), so a crash still comes with the full recent diagnostics. 2 = everything (old behaviour). Any line
   containing fail/error/abort/crash/fatal/exception/assert is always written. */
static int logLevel = -1;
void P3DS_SetLogLevel(int v) { logLevel = v; }
int P3DS_LogLevel(void) { return logLevel < 0 ? 1 : logLevel; }
#define DRING (128 * 1024)
static char dring[DRING]; static u32 dhead;
static void ensureInit(void) { if (!init) { LightLock_Init(&lk); init = 1; }
    if (logLevel < 0) { logLevel = 1; FILE *f = fopen("sdmc:/3ds/soulsilver3ds/config.txt", "r"); if (f) { char l[64]; int x; while (fgets(l, sizeof l, f)) if (sscanf(l, "log_level=%d", &x) == 1) logLevel = x; fclose(f); } } }
static int hasCI(const char *s, const char *w) { size_t n = strlen(w); for (; *s; s++) { size_t k = 0; while (k < n && s[k] && ((s[k] | 32) == w[k])) k++; if (k == n) return 1; } return 0; }
static int isDetail(const char *line) {
    if (logLevel >= 2) return 0;
    static const char *const keep[] = { "fail", "error", "abort", "crash", "fatal", "exception", "assert", "overflow" };
    for (unsigned i = 0; i < sizeof keep / sizeof *keep; i++) if (hasCI(line, keep[i])) return 0;
    if (!strncmp(line, "[PERF]", 6)) { static unsigned n; return (n++ % 10) != 0; }
    if (!strncmp(line, "[PERF3]", 7)) { static unsigned n; return (n++ % 10) != 0; }
    if (!strncmp(line, "[PERF4]", 7)) { static unsigned n; return (n++ % 20) >= 2; }  /* two lines per second: keep 1 pair in 10 */
    if (!strncmp(line, "[AUDIO-CLK]", 11)) { static unsigned n; return (n++ % 4) != 0; }
    static const char *const det[] = { "[AUDIO-KEY]", "[AUDIO-OFF]", "[AUDIO-PIT]", "[AUDIO-SE]", "[AUDIO-SAS]", "[G3DIAG]",
        "[GPU3D] f=", "[GPU3D] frame:", "[GPU3D] cpu-clipped", "[G3PROF]", "[STEREO]", "[3DLAYER]", "[VDUMP]", "[2D] game state",
        "[2D] bottom_skip", "[3DS] present_skipbot:", "[3DS] tearfix=2: waited", "[TEX]", "[ROMCACHE] block hits", "[ROMFS-CACHE]", "[PREFETCH] persist" };
    for (unsigned i = 0; i < sizeof det / sizeof *det; i++) if (!strncmp(line, det[i], strlen(det[i]))) return 1;
    return 0;
}
volatile int P3DS_LogWriting; /* run 51: zone_prefetch pauses while the log is written to SD */
/* fsync batching: fflush alone leaves log.txt in the OS layer, so a HOME close or power loss could lose the last lines.
   The writer fsyncs at most every 2 s, right away after a [SAVE] line, and on every synchronous flush (crash/hold/exit). */
static volatile int logSyncNow; static int logPending; static u64 logSyncTick; static unsigned logSyncs, logSyncFails;
void P3DS_LogRequestSync(void) { logSyncNow = 1; } /* HOME/sleep: the writer thread fsyncs within 100 ms (no FILE use from the caller's thread) */
static void logSync(int force) {
    if (!lf) return; u64 now = svcGetSystemTick();
    if (!force && !logSyncNow && now - logSyncTick < 2ULL * SYSCLOCK_ARM11) return;
    logSyncNow = 0; logSyncTick = now; logPending = 0;
    if (fsync(fileno(lf))) { if (!logSyncFails++) fprintf(lf, "[LOG] fsync of log.txt FAILED (further failures counted silently)\n"); }
    else if (!logSyncs++) fprintf(lf, "[LOG] log.txt fsync active (every 2 s, after [SAVE] lines, on crash/exit)\n");
}
static void flushOnce(int lock) {
    P3DS_LogWriting = 1; int wrote = 0;
    for (;;) {
        u32 n = 0;
        if (lock) LightLock_Lock(&lk);
        u32 avail = head - tail; n = avail < sizeof out ? avail : sizeof out;
        for (u32 k = 0; k < n; k++) out[k] = ring[(tail + k) % RING];
        tail += n; unsigned d = dropped; dropped = 0;
        if (lock) LightLock_Unlock(&lk);
        if (!lf) lf = fopen("sdmc:/3ds/soulsilver3ds/log.txt", "w");
        if (!lf) return;
        if (d) fprintf(lf, "[LOG] %u lines dropped (ring full)\n", d);
        if (!n) break;
        fwrite(out, 1, n, lf); wrote = 1;
        { /* run 74 log_max_kb (default 1024, 0 = no limit): past the limit log.txt becomes log_old.txt and a new one starts */
          static long wr; static int mx = -1; extern int P3DS_CfgInt(const char *, int); if (mx < 0) mx = P3DS_CfgInt("log_max_kb", 1024);
          wr += n; if (mx > 0 && wr > (long)mx * 1024) { fclose(lf); remove("sdmc:/3ds/soulsilver3ds/log_old.txt"); rename("sdmc:/3ds/soulsilver3ds/log.txt", "sdmc:/3ds/soulsilver3ds/log_old.txt");
            lf = fopen("sdmc:/3ds/soulsilver3ds/log.txt", "w"); wr = 0; if (!lf) return; fprintf(lf, "[LOG] log_max_kb=%d reached: older lines in log_old.txt\n", mx); } }
    }
    if (lf) { fflush(lf); if (wrote) logPending = 1; if (logPending || logSyncNow) logSync(!lock); }
    P3DS_LogWriting = 0;
}
static void writer(void *p) { (void)p; P3DS_ThreadGuard("logwriter"); for (;;) { svcSleepThread(100000000LL); flushOnce(1); } }
static void useCount(const char *line);
static char tailBuf[8][112]; static unsigned tailN; /* last 8 written (non-detail) lines, shown on the crash screen */
void P3DS_LogLine(const char *line) {
    ensureInit(); u32 len = (u32)strlen(line);
    useCount(line);
    const int detail = isDetail(line);
    LightLock_Lock(&lk);
    if (!detail) { char *t = tailBuf[tailN++ & 7]; strncpy(t, line, sizeof tailBuf[0] - 1); t[sizeof tailBuf[0] - 1] = 0; }
    if (detail) { for (u32 k = 0; k < len; k++) dring[(dhead + k) % DRING] = line[k]; dring[(dhead + len) % DRING] = '\n'; dhead += len + 1; LightLock_Unlock(&lk); return; }
    if (!strncmp(line, "[SAVE]", 6)) logSyncNow = 1;
    if (head - tail + len + 1 > RING) dropped++;
    else { for (u32 k = 0; k < len; k++) ring[(head + k) % RING] = line[k]; ring[(head + len) % RING] = '\n'; head += len + 1; }
    int start = !th; if (start) th = (Thread)1;
    LightLock_Unlock(&lk);
    if (start) { th = threadCreate(writer, NULL, 0x4000, 0x3F, -2, true); if (!th) th = (Thread)1; }
}
/* Synchronous flush for crash/exit/hold paths (lock attempted; written even if it can't be taken). */
void P3DS_LogFlushSync(void) { ensureInit(); int got = !LightLock_TryLock(&lk); if (got) LightLock_Unlock(&lk); flushOnce(got);
    if (lf && dhead) { /* log_level 1: append the recent suppressed detail lines (oldest first, from a line start) */
        u32 n = dhead < DRING ? dhead : DRING, start = dhead - n; u32 k = 0;
        if (dhead > DRING) while (k < n && dring[(start + k) % DRING] != '\n') k++;
        fprintf(lf, "[LOG] ---- last %u bytes of suppressed detail lines (log_level=%d) ----\n", n - k, logLevel);
        for (; k < n; k++) fputc(dring[(start + k) % DRING], lf);
        fprintf(lf, "[LOG] ---- end of detail ----\n"); fflush(lf); dhead = 0; }
    if (lf) { fflush(lf); logSync(1); } }

/* ---- [USE] feature-usage counters, [CFG] header, crash-screen tail --------------------------------------------------
   Every feature that should leave evidence in log.txt writes one tagged line per EVENT (not per frame). Those lines are
   also counted here by prefix (useTab), so the clean-exit "[USE]" summary shows which features were exercised this run
   and which were never touched. Counting is by line prefix: keep the prefixes in step with the log lines (docs/HANDOFF.md). */
extern int P3DS_CfgInt(const char *, int);
extern void PSPNativeMemLog(const char *, ...);
typedef struct { const char *name, *prefix; volatile unsigned n; } UseEnt;
static UseEnt useTab[] = {
    { "battles", "[BATTLE] start", 0 }, { "battle_anim_skips", "[BATTLE] move ", 0 },
    { "catch_info_panel", "[CATCH] panel opened", 0 }, { "shiny_alerts", "[CATCH] shiny alert", 0 },
    { "summary_iv_ev_pages", "[SUMMARY] IV", 0 }, { "summary_iv_ev_box_toggles", "[UI] summary IV/EV box", 0 },
    { "repel_checks", "[REPEL] repel check", 0 }, { "reusable_tm_uses", "[ITEM] reusable TM:", 0 },
    { "field_moves_without_hm", "[HM] move ", 0 }, { "no_wild_battle_skips", NULL, 0 }, /* counted by P3DS_Use (log is sampled) */
    { "hunt_counter_increments", "[HUNT] species", 0 },
    { "wide_battle_entries", "[WIDE] battle entered", 0 }, { "wide_menu_screens", "[WIDE] menu screen", 0 },
    { "bottom_menu_taps", "[BMENU] tap ", 0 }, { "bottom_menu_actions", "[BMENU] request handled", 0 },
    { "minimap_mode_changes", "[MINIMAP] mode ", 0 }, { "minimap_new_cells", "[MINIMAP] new cell", 0 }, { "pgmap_wild_lists", "[PGMAP] card page wild", 0 },
    { "menu_button_taps", "[BTN] opened by menu button tap", 0 },
    { "quick_actions", "[QUICK] ", 0 }, { "settings_opened", "[APP] settings open", 0 },
    { "settings_changes", "[APP] saved ", 0 }, { "soft_resets", "[3DS] soft reset", 0 },
    { "saves_synced", "[SAVE] sync", 0 },
    { "audio_stall_fades", "[AUDIO] stall fade: no game", 0 }, { "audio_dsp_stalls", "[AUDIO] dsp stall", 0 },
    { "wireless_sessions", "[WIRELESS] WM on 3DS local wireless", 0 }, { "wireless_links", "[WIRELESS] joined parent", 0 },
    { "wireless_children", "[WIRELESS] child aid", 0 },
};
#define NUSE ((int)(sizeof useTab / sizeof useTab[0]))
void P3DS_Use(const char *name) { for (int i = 0; i < NUSE; i++) if (!strcmp(useTab[i].name, name)) { __sync_fetch_and_add(&useTab[i].n, 1); return; } }
static void useCount(const char *line) {
    if (line[0] != '[') return;
    for (int i = 0; i < NUSE; i++) if (useTab[i].prefix && !strncmp(line, useTab[i].prefix, strlen(useTab[i].prefix))) { __sync_fetch_and_add(&useTab[i].n, 1); return; }
}
/* Clean exit (and crash): which features were used, and which never were. */
void P3DS_UseSummary(const char *why) {
    static char used[900], unused[700]; used[0] = unused[0] = 0; size_t u = 0, z = 0; int nu = 0;
    for (int i = 0; i < NUSE; i++) {
        if (useTab[i].n) { if (u < sizeof used - 40) u += (size_t)snprintf(used + u, sizeof used - u, " %s=%u", useTab[i].name, useTab[i].n); nu++; }
        else if (z < sizeof unused - 40) z += (size_t)snprintf(unused + z, sizeof unused - z, " %s", useTab[i].name);
    }
    static char b[1100];
    snprintf(b, sizeof b, "[USE] feature usage (%s): %d of %d features used:%s", why, nu, NUSE, nu ? used : " none"); P3DS_LogLine(b);
    snprintf(b, sizeof b, "[USE] not used this run:%s", z ? unused : " (all used)"); P3DS_LogLine(b);
}
unsigned P3DS_LogTailCount(void) { return tailN < 8 ? tailN : 8; }
const char *P3DS_LogTailLine(unsigned idx) { const unsigned n = P3DS_LogTailCount(); return idx < n ? tailBuf[(tailN - n + idx) & 7] : ""; }

/* Header: every key found in config.txt with its default and a DEAD / UNLISTED flag, then the effective value of every
   setting this build reads with P3DS_CfgInt. Written once at boot (main3ds.c) so a log always says what it ran with. */
typedef struct { const char *key; int def; const char *dead; } CfgEnt; /* def == INT_MIN: no single default / read elsewhere */
#define NODEF (-2147483647 - 1)
static const CfgEnt cfgTab[] = {
    { "battle_monobj_hide", 1, 0 }, { "fps_tweaks", 0, 0 }, { "bottom_layout", 1, 0 }, { "bottom_hd", 1, 0 }, { "wide_pokedex", 0, 0 }, { "wide_pc", 0, 0 }, { "bottom_map", 1, 0 }, { "bottom_menu", 0, 0 }, { "bottom_vramgen", 1, 0 }, { "cam_zoom", 0, 0 }, { "catch_overlay", 0, 0 },
    { "cheat_no_encounters", 0, 0 }, { "cheat_time", 0, 0 }, { "cpad_dpad", 1, 0 }, { "credits_splash", 1, 0 }, { "cstick_mode", 2, 0 },
    { "display_capture", 1, 0 }, { "fast_text", 0, 0 }, { "follower", 0, 0 }, { "follower_slot", 0, 0 }, { "follower_talk", 0, 0 },
    { "fps60_probe", 0, 0 }, { "fps_counter", 0, 0 }, { "g3_reuse_verify", 0, 0 }, { "g3_shot", 0, 0 }, { "g3_wmode", 1, 0 },
    { "gpu_blend", 1, 0 }, { "gpu_compose", 2, 0 }, { "gpu_compose_stereo", 1, 0 }, { "gpu_compose_stereo_nf", 1, 0 }, { "gpu_direct3d", 0, 0 }, { "gpu_direct3d_wide", 1, 0 }, { "heap_watch", 0, 0 },
    { "hunt_counter", 0, 0 }, { "lid_sound", 1, 0 }, { "lid_test", 0, 0 }, { "log_max_kb", 1024, 0 }, { "menu_audio", 1, 0 }, { "menu_button", 1, 0 },
    { "menu_combo", 1, 0 }, { "repel_prompt", 0, 0 }, { "reset_skip_intro", 0, 0 }, { "reusable_tms", 0, 0 }, { "save_backup", 1, 0 },
    { "select_reset", 0, 0 }, { "settings_app", 1, 0 }, { "settings_touch", 1, 0 }, { "soft_reset", 1, 0 }, { "speedcheck", 1, 0 }, { "splash_ms", 2000, 0 },
    { "stereo_2d", 2, 0 }, { "stereo_blobdiag", 1, 0 }, { "stereo_edgefix", 1, 0 }, { "stereo_fake", 0, 0 }, { "stereo_strength", 0, 0 },
    { "summary_ivs", 0, 0 }, { "tex_trace", 0, 0 }, { "tod_tint", 0, 0 }, { "ui_frame", 0, 0 }, { "ui_gamefont", 1, 0 }, { "vram_frame_gen", 1, 0 },
    { "vram_trace", 0, 0 }, { "wide_battle", 0, 0 }, { "wide_edgefill", 1, 0 }, { "wide_fill", 0, 0 }, { "wide_menu_screens", 0, 0 },
    { "wide_menus", 0, 0 }, { "wide_weather", 1, 0 }, { "zl_run", 0, 0 }, { "zr_item", 1, 0 }, { "log_level", 1, 0 }, { "pg_kanto", 0, 0 },
    /* read with their own parsers: listed so they are not flagged, no single default shown */
    { "async2d", NODEF, 0 }, { "audio_gain", NODEF, 0 }, { "audio_interp", NODEF, 0 }, { "audio_lead", NODEF, 0 }, { "audio_limiter", NODEF, 0 },
    { "audio_rate", NODEF, 0 }, { "audio_stallfade", 1, 0 }, { "audio_volramp", NODEF, 0 }, { "bottom_refresh", NODEF, 0 }, { "bottom_skip", NODEF, 0 }, { "comp_excl", NODEF, 0 },
    { "core1_limit", NODEF, 0 }, { "cull3d", NODEF, 0 }, { "dump_frame", NODEF, 0 }, { "dump_eyes", NODEF, 0 }, { "early_b", NODEF, 0 }, { "fast_conv", NODEF, 0 },
    { "fog3d", NODEF, 0 }, { "g3_boxtest_force", NODEF, 0 }, { "g3_wbuf", NODEF, 0 }, { "g3_wmax", NODEF, 0 }, { "gpu3d", NODEF, 0 }, { "gpu3d_async", NODEF, 0 },
    { "gpu3d_cpuclip", NODEF, 0 }, { "gpu3d_flipy", NODEF, 0 }, { "gpu3d_texflip", NODEF, 0 }, { "gpu3d_zinv", NODEF, 0 }, { "gpu_swizzle", NODEF, 0 },
    { "gpuscale", NODEF, 0 }, { "gpuscale_zoom", NODEF, 0 }, { "hotkey_swallow", NODEF, 0 }, { "land_fastload", NODEF, 0 }, { "mode0_border", NODEF, 0 },
    { "present_skipbot", NODEF, 0 }, { "rb_sync", NODEF, 0 }, { "res3d_lindepth", NODEF, 0 }, { "rom_cache_mb", NODEF, 0 }, { "rom_ra_kb", NODEF, 0 },
    { "save_anim", NODEF, 0 }, { "save_minms", NODEF, 0 }, { "save_seq", NODEF, 0 }, { "scalesrc_dbl", NODEF, 0 }, { "spike_ms", NODEF, 0 },
    { "split_top", NODEF, 0 }, { "steal_b", NODEF, 0 }, { "stereo2d_par", NODEF, 0 }, { "stereo_c1", NODEF, 0 }, { "stereo_clamp", NODEF, 0 },
    { "stereo_depth", NODEF, 0 }, { "tearfix", NODEF, 0 }, { "tex_pagecheck", NODEF, 0 }, { "tex_postvblank", NODEF, 0 }, { "touch_latch", NODEF, 0 },
    { "verbose", NODEF, 0 }, { "verbose_boot", NODEF, 0 }, { "vsync", NODEF, 0 }, { "wide_cull", NODEF, 0 }, { "wide_fadefix", NODEF, 0 },
    { "wide_fx", NODEF, 0 }, { "wide_haze", NODEF, 0 }, { "wideview", NODEF, 0 }, { "zone_prefetch", NODEF, 0 },
    /* generic settings rows read through the menu table (render3ds.cpp genGet) */
    { "hm_free", NODEF, 0 }, { "battle_anim_speed", NODEF, 0 }, { "text_hold_b", NODEF, 0 }, { "text_auto", NODEF, 0 }, { "g3_reuse", NODEF, 0 },
    { "cheat_money", NODEF, 0 }, { "cheat_catch", NODEF, 0 }, { "cheat_shiny", NODEF, 0 }, { "cheat_eggs", NODEF, 0 }, { "cheat_exp", NODEF, 0 }, { "cheat_candy", NODEF, 0 },
    { "follower_emotes", NODEF, 0 }, { "follower_sidefix", NODEF, 0 }, { "follower_debug", NODEF, 0 }, { "dw_depth", NODEF, "Platinum Distortion World only" },
    /* dead / ignored keys: still accepted in config.txt, nothing in this build reads them */
    { "g3_wcpu", NODEF, "DEAD: CPU 3D W-frames removed (ignored)" }, { "g3_wpersp", NODEF, "DEAD: CPU W raster is always perspective-correct (ignored)" },
    { "g3_wdepth", NODEF, "DEAD: replaced by g3_wmode (ignored since run 3)" }, { "scale", NODEF, "DEAD: old key, use gpuscale (ignored)" },
    { "cstick_poketch", NODEF, "DEAD: use cstick_mode (ignored)" },
    { "wild_vis", NODEF, "DEAD: Platinum only, no reader in SoulSilver" }, { "wild_vis_max", NODEF, "DEAD: Platinum only" },
    { "wild_vis_replace", NODEF, "DEAD: Platinum only" }, { "wild_vis_test", NODEF, "DEAD: Platinum only" }, { "wild_vis_species", NODEF, "DEAD: Platinum only" },
    { "dw_debris", NODEF, "DEAD: Platinum Distortion World only" }, { "dw_unease", NODEF, "DEAD: Platinum Distortion World only" },
    { "dw_cry", NODEF, "DEAD: Platinum Distortion World only" }, { "dw_roll", NODEF, "DEAD: Platinum Distortion World only" },
    { "item_ping", NODEF, "DEAD: Platinum Poketch only" }, { "touch_menu", NODEF, "DEAD: Platinum game hook missing" }, { "bottom_strip", NODEF, "DEAD: Platinum only" },
    { "cheat_pp", NODEF, "DEAD: Platinum game hook missing" }, { "cheat_speed", NODEF, "DEAD: Platinum game hook missing" },
    { "cheat_walk_walls", NODEF, "DEAD: Platinum game hook missing" },
    { "follower_area_talk", NODEF, "DEAD: no reader" }, { "follower_cry", NODEF, "DEAD: no reader" }, { "follower_items", NODEF, "DEAD: no reader" },
    { "follower_ballpop", NODEF, "DEAD: no reader" }, { "follower_friend_talk", NODEF, "DEAD: no reader" }, { "follower_idle", NODEF, "DEAD: no reader" },
    { "follower_gap", NODEF, "DEAD: no reader" }, { "follower_behind", NODEF, "DEAD: no reader" },
};
#define NCFG ((int)(sizeof cfgTab / sizeof cfgTab[0]))
void P3DS_LogConfigHeader(const char *version) {
    PSPNativeMemLog("[CFG] ==== SoulSilver 3DS %s | built %s %s ====", version, __DATE__, __TIME__);
    FILE *f = fopen("sdmc:/3ds/soulsilver3ds/config.txt", "r"); int nkeys = 0, ndead = 0, nunl = 0;
    if (!f) PSPNativeMemLog("[CFG] config.txt: none (all defaults)");
    else {
        char l[128];
        while (fgets(l, sizeof l, f)) {
            char *eq = strchr(l, '='); if (!eq || l[0] == '#' || l[0] == '\n') continue;
            *eq = 0; char *v = eq + 1; for (char *q = v; *q; q++) if (*q == '\n' || *q == '\r') *q = 0;
            const CfgEnt *e = NULL; for (int i = 0; i < NCFG; i++) if (!strcmp(cfgTab[i].key, l)) { e = &cfgTab[i]; break; }
            nkeys++;
            if (!e) { nunl++; PSPNativeMemLog("[CFG] %s=%s  UNLISTED: not a key this build is known to read (ignored or new)", l, v); }
            else if (e->dead) { ndead++; PSPNativeMemLog("[CFG] %s=%s  %s", l, v, e->dead); }
            else if (e->def != NODEF && atoi(v) != e->def) PSPNativeMemLog("[CFG] %s=%s  (default %d) NON-DEFAULT", l, v, e->def);
            else PSPNativeMemLog("[CFG] %s=%s", l, v);
        }
        fclose(f); PSPNativeMemLog("[CFG] config.txt: %d keys, %d dead/ignored, %d unlisted", nkeys, ndead, nunl);
    }
    char b[300]; int n = 0, cnt = 0;
    for (int i = 0; i < NCFG; i++) {
        if (cfgTab[i].def == NODEF || cfgTab[i].dead) continue;
        n += snprintf(b + n, sizeof b - n, " %s=%d", cfgTab[i].key, P3DS_CfgInt(cfgTab[i].key, cfgTab[i].def));
        if (++cnt % 8 == 0 || n > 240) { PSPNativeMemLog("[CFG] effective:%s", b); n = 0; }
    }
    if (n) PSPNativeMemLog("[CFG] effective:%s", b);
}
