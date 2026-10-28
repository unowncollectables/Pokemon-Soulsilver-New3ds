/* ss3ds: definitions the Platinum platform layer expects from Platinum's own game/native patches, and the
   game-facing pieces of pspoke's SoulSilver PSP main.c (dropped from the link, see full/Makefile). */
#include <3ds.h>

/* VRAM write tracking. Platinum stamps 4 KB VRAM pages from redirected game writes (pspoke vram_dirty.c +
   opttex_redirect.py); SoulSilver's objects are not redirected, so every page counts as written once per
   frame (called from main3ds.c's OS_WaitIrq wrap on each VBlank wait). g3soft's texture cache then always falls
   back to its byte compare: correct, a little slower. */
unsigned PSPNativeVramGen = 1, PSPNativeVramPageGen[0xA4], PSPNativeVramDirtyCalls;
void SS3DS_VramFrame(void)
{
    unsigned g = ++PSPNativeVramGen;
    for (unsigned i = 0; i < sizeof PSPNativeVramPageGen / sizeof PSPNativeVramPageGen[0]; i++) PSPNativeVramPageGen[i] = g;
}

/* ROM read timing for the [SPIKE] log (Platinum: romfs.c patch). */
unsigned long long P3DS_SpkRomTicks;
unsigned P3DS_SpkRomBlocks;

/* Platinum plays a Poke Ball sound on lid close/open from its game patch; no SoulSilver hook yet. */
void P3DS_PlayLidSE(int open) { (void)open; }

/* Platinum links pspoke's stackprobe.o; SoulSilver's app does not build it. */
void PSPNativeStackProbeInit(void) {}

/* From pspoke's SoulSilver main.c. */
char gPSPNativeSelfPath[256];
/* The original routine: does the communication manager exist. */
extern void *ssdata_unk_02037C94__021D4150;
__attribute__((weak)) int sub_02037D78(void) { return ssdata_unk_02037C94__021D4150 != 0; }

/* Wide view (and the other overworld-only extras) key on P3DS_InField. Platinum sets it from pspoke's "enter app"
   memprobe hook (last app = gFieldMapTemplate); SoulSilver's app has no such hook, so watch the overlay managers
   instead. ov01_02206378 is the field map's template (field_system.c FieldSystem_LoadFieldOverlayInternal); every
   other manager (battle, bag, title, ...) replaces it while it runs and clears the flag. Weak: if a pspoke bump
   renames it, the link still works and wide view simply stays off in the overworld. */
extern int P3DS_InField;
extern const char ov01_02206378[] __attribute__((weak));
extern const char gOverlayTemplate_Battle[] __attribute__((weak));
int P3DS_InBattle;
/* wide_menu_screens: party menu, bag and Pokemon summary. Weak: a renamed symbol just leaves the toggle without effect;
   the first template changes are logged so the log shows which pointers were seen. */
int P3DS_InWideMenu;
extern const char gOverlayTemplate_PartyMenu[] __attribute__((weak));
extern const char gOverlayManagerTemplate_Bag[] __attribute__((weak));
extern const char gOverlayTemplate_PokemonSummary[] __attribute__((weak));
/* run 18: wide_pokedex / wide_pc. HGSS launches both from static templates in launch_application.c
   (sOverlayTemplate_Pokedex = {Pokedex_Init,..,OVY_18}, sOverlayTemplate_PCBox = {PCBox_Init,..,OVY_14}), so they are
   recognised by their Init function. P3DS_InWideKind: 1 party/bag/summary, 2 Pokedex, 3 PC boxes, 0 other. */
int P3DS_InWideKind;
extern int Pokedex_Init() __attribute__((weak));
extern int PCBox_Init() __attribute__((weak));
extern void *__real_OverlayManager_New(const void *template, void *parentWork, int heapID);
void *__wrap_OverlayManager_New(const void *template, void *parentWork, int heapID)
{
    const int field = template && template == (const void *)ov01_02206378;
    if (field != P3DS_InField) { extern void PSPNativeMemLog(const char *, ...); PSPNativeMemLog("[3DS] field %d", field); }
    P3DS_InField = field;
    /* wide_battle: the battle overlay manager (gOverlayTemplate_Battle, overlay 12) marks the battle; the field
       manager returning clears it. Sub-screens opened from the battle (bag, party) keep the flag. */
    if (template) {
        extern int P3DS_InBattle;
        const int battle = template == (const void *)gOverlayTemplate_Battle ||
                           (!gOverlayTemplate_Battle && ((const int *)template)[3] == 12);
        static unsigned seen;
        if (!field && seen < 24) { seen++; extern void PSPNativeMemLog(const char *, ...); PSPNativeMemLog("[3DS] overlay manager %p ovy %d battle %d", template, ((const int *)template)[3], battle); }
        {
            const char *nm = (gOverlayTemplate_PartyMenu && template == (const void *)gOverlayTemplate_PartyMenu) ? "party" :
                             (gOverlayManagerTemplate_Bag && template == (const void *)gOverlayManagerTemplate_Bag) ? "bag" :
                             (gOverlayTemplate_PokemonSummary && template == (const void *)gOverlayTemplate_PokemonSummary) ? "summary" :
                             (Pokedex_Init && ((void *const *)template)[0] == (void *)Pokedex_Init) ? "pokedex" :
                             (PCBox_Init && ((void *const *)template)[0] == (void *)PCBox_Init) ? "pcbox" : 0;
            static const char *lastNm; static unsigned logged;
            if (nm != lastNm && logged < 64) {
                extern void PSPNativeMemLog(const char *, ...);
                logged++;
                PSPNativeMemLog("[WIDE] menu screen %s -> %s (template %p, battle %d, field %d; symbols party %p bag %p summary %p)",
                                lastNm ? lastNm : "none", nm ? nm : "none", template, battle, field,
                                (const void *)gOverlayTemplate_PartyMenu, (const void *)gOverlayManagerTemplate_Bag,
                                (const void *)gOverlayTemplate_PokemonSummary);
            }
            lastNm = nm;
            P3DS_InWideKind = !nm ? 0 : nm[0] == 'p' && nm[1] == 'o' ? 2 : nm[0] == 'p' && nm[1] == 'c' ? 3 : 1;
            P3DS_InWideMenu = P3DS_InWideKind == 1;
        }
        /* [BATTLE] start / end: one line each per battle (counted in the clean-exit [USE] summary) */
        static unsigned battleN; static unsigned long long battleT0;
        if (battle) { if (!P3DS_InBattle) { extern void PSPNativeMemLog(const char *, ...); battleT0 = svcGetSystemTick(); PSPNativeMemLog("[BATTLE] start #%u", ++battleN); } P3DS_InBattle = 1; }
        else if (field) { if (P3DS_InBattle) { extern void PSPNativeMemLog(const char *, ...); PSPNativeMemLog("[BATTLE] end #%u after %u ms", battleN, (unsigned)((svcGetSystemTick() - battleT0) / CPU_TICKS_PER_USEC / 1000)); } P3DS_InBattle = 0; }
    }
    return __real_OverlayManager_New(template, parentWork, heapID);
}

/* Counted by Platinum's snd_bank.c patch (notes whose wave is missing) and logged by sas_out.c's [AUDIO-SE] line;
   SoulSilver's snd_bank is unpatched, so it stays 0. */
unsigned P3DS_NoteNoWave;

/* Battle start crash: ov01_021EFF90 (the encounter transition's brightness SysTask, overlay_01_021EFB38.s) passes the
   raw DS addresses 0x0400006C / 0x0400106C as the register pointer to libntr's GXx_{Set,Get}MasterBrightness_.
   pspoke's asm translator only rewrites loads and stores (ssio_*), not pointer arguments, so the C function stored
   straight to 0x0400106C, which is unmapped on 3DS (data abort). Map those addresses to the port's register
   variables here; any other DS I/O address is logged and ignored instead of faulting. */
extern volatile u16 s_reg_GX_MASTER_BRIGHT, s_reg_GXS_DB_MASTER_BRIGHT;
static volatile u16 *SS3DS_BrightReg(volatile u16 *reg, const char *fn, void *ra)
{
    static unsigned logged;
    const u32 a = (u32)reg;
    if ((a >> 24) != 0x04u) return reg;
    volatile u16 *mapped = a == 0x0400006Cu ? &s_reg_GX_MASTER_BRIGHT : a == 0x0400106Cu ? &s_reg_GXS_DB_MASTER_BRIGHT : 0;
    if (logged < 8) {
        extern void PSPNativeMemLog(const char *, ...);
        logged++;
        PSPNativeMemLog("[3DS] %s: raw DS register %08lx from %p -> %s", fn, (unsigned long)a, ra,
                        mapped ? "mapped" : "UNMAPPED, ignored");
    }
    return mapped;
}
extern void __real_GXx_SetMasterBrightness_(volatile u16 *reg, int brightness);
void __wrap_GXx_SetMasterBrightness_(volatile u16 *reg, int brightness)
{
    if ((reg = SS3DS_BrightReg(reg, "SetMasterBrightness", __builtin_return_address(0)))) __real_GXx_SetMasterBrightness_(reg, brightness);
}
extern int __real_GXx_GetMasterBrightness_(volatile u16 *reg);
int __wrap_GXx_GetMasterBrightness_(volatile u16 *reg)
{
    return (reg = SS3DS_BrightReg(reg, "GetMasterBrightness", __builtin_return_address(0))) ? __real_GXx_GetMasterBrightness_(reg) : 0;
}

/* Doubled battle Pokemon in 3D (Platinum 3DS run 55, same game code in SoulSilver): during a send-out, the encounter
   start and the ball effect the game makes 2D OBJ copies of every visible battle Pokemon, only so the ball-seal
   particles can layer with them. On the DS they sit pixel-exactly over the 3D sprite quads; on the 3DS the 3D sprite
   gets stereo depth while the 2D copy stays at screen depth, so the Pokemon already out shows twice until the ball
   opens. ov07_0221FDFC is the "copies for all battlers" call (BattleMonOBJData_NewAll in pokeplatinum; it tail-calls
   ov07_0221FB90 with battler 0xFF); its result holds the four ManagedSprite pointers at +0x24. The translated asm
   inlines it into every caller, so a linker --wrap never saw it (run: nm/map had no such symbol). Instead
   pspoke-patches/0017 retargets those calls to this function, which does what FDFC did and then hides the copies;
   the 3D sprites stay. battle_monobj_hide=0 = old behaviour. */
extern unsigned int ov07_0221FB90(unsigned int battleSystem, unsigned int heapID, unsigned int battler, unsigned int unused);
extern void ManagedSprite_SetDrawFlag(void *managedSprite, int draw);
extern int P3DS_CfgInt(const char *key, int def);
unsigned int SS3DS_BattleMonOBJ_NewAll(unsigned int battleSystem, unsigned int heapID)
{
    extern void PSPNativeMemLog(const char *, ...);
    static int hide = -1;
    static unsigned calls;
    unsigned int d = ov07_0221FB90(battleSystem, heapID, 0xFF, 0);
    if (hide < 0) {
        hide = P3DS_CfgInt("battle_monobj_hide", 1) != 0;
        PSPNativeMemLog("[BATTLE] battle_monobj_hide=%d (hide the 2D copies of battle Pokemon in send-outs)", hide);
    }
    int hidden = 0;
    if (hide && d) {
        void **sprites = (void **)((char *)(uintptr_t)d + 0x24);
        for (int i = 0; i < 4; i++)
            if (sprites[i]) { ManagedSprite_SetDrawFlag(sprites[i], 0); hidden++; }
    }
    if (calls++ < 16) PSPNativeMemLog("[BATTLE] mon OBJ copies made (call %u): result %08x, %d hidden", calls, d, hidden);
    return d;
}

/* Zone prefetch (romcache3ds.c) learns which ROM blocks each map needs and reads the neighbours' blocks ahead of time
   on an idle core, but only if told when the map changes. Platinum hooks its map change; in SoulSilver the field map
   loader (fieldmap.c: AreaDataManager_Alloc(MapHeader_GetAreaDataBank(location->mapId))) is the one place every
   map load goes through, warps and walking across a map border alike. Without this hook the [PREFETCH] lines never
   appeared in log.txt and every map entry read its data from the SD card cold (100-400 ms frame spikes, audio holds). */
extern void P3DS_ZoneEnter(int zone);
extern unsigned char __real_MapHeader_GetAreaDataBank(unsigned mapId);
unsigned char __wrap_MapHeader_GetAreaDataBank(unsigned mapId)
{
    static unsigned last = 0xFFFFFFFFu;
    if (mapId != last) { last = mapId; P3DS_ZoneEnter((int)mapId); }
    return __real_MapHeader_GetAreaDataBank(mapId);
}

/* Play time (the TIME on the Continue screen and the trainer card) stayed 0:00. The game adds seconds from DS timer 3
   (src/timer3.c: sub_02025488 reads the count, sub_02025504 turns ticks into seconds, sub_020183B0 feeds
   AddIGTSeconds every frame). That version counts on the timer's IRQ and register, which the port does not emulate, so
   the count never moved. pspoke's PSP build swaps in timer3_psp.c (the PSP system clock); this is the same idea on the
   3DS system tick. DS timer 3 runs at the 33513982 Hz bus clock / 64, and sub_02025504 divides by that rate. */
extern void PSPNativeMemLog(const char *, ...);
static u64 ss3dsT3Start;
u64 __wrap_sub_02025488(void)
{
    static u64 nextLog;
    const u64 now = svcGetSystemTick();
    if (!ss3dsT3Start) ss3dsT3Start = now;
    const u64 e = now - ss3dsT3Start, sec = e / SYSCLOCK_ARM11, rem = e % SYSCLOCK_ARM11;
    const u64 ticks = (sec * 33513982u + rem * 33513982u / SYSCLOCK_ARM11) / 64u;
    if (sec >= nextLog) {
        PSPNativeMemLog("[IGT] timer3 clock %llu s since start, %llu DS timer ticks (play time counts from this)", sec, ticks);
        nextLog = sec + 60;
    }
    return ticks;
}
u64 __wrap_sub_020254FC(void) { return __wrap_sub_02025488(); }

/* ZL run toggle (zl_run, Settings > Controls, default off). In SoulSilver the "is B held" test that decides running
   (sub_0205DE88(avatar, keys): keys & PAD_BUTTON_B) lives in translated asm, called from the walk start and the
   movement-state pick in unk_0205CB48. pspoke-patches/0050 retargets both calls here. With the toggle on (ZL flips
   P3DS_RunToggle in psp_ctru.c), holding B walks instead of runs. The running shoes check stays in the game, and
   nothing is written to the save. */
extern volatile int P3DS_RunToggle;
extern int P3DS_CtlZl;
extern unsigned int sub_0205DE88(unsigned int avatar, unsigned int keys); /* the game's own "run button held" answer (the call this hook replaced) */
volatile int P3DS_RunShown = -1; /* what the player does with no button held (the game's own answer, inverted by Auto run); the RUN tile text reads it */
unsigned int SS3DS_RunHeld(unsigned int avatar, unsigned int keys)
{
    extern void PSPNativeMemLog(const char *, ...);
    static int lastTog = -1, lastHeld = -1, calls;
    unsigned int held = sub_0205DE88(avatar, keys) ? 1u : 0u; /* follow the game's own decision; Auto run only inverts it */
    unsigned int out = P3DS_RunToggle ? !held : held; /* run 10: the JOHTO layout's RUN button sets it too, so no zl_run gate here (ZL itself still needs zl_run=1) */
    P3DS_RunShown = (P3DS_RunToggle ? !sub_0205DE88(avatar, 0) : sub_0205DE88(avatar, 0)) ? 1 : 0;
    calls++;
    /* [INPUT] first 12 calls, then every change of toggle or B: setting, raw keys and the run answer (run 52: "run stuck on while it says off") */
    if (calls <= 12 || P3DS_RunToggle != lastTog || (int)held != lastHeld) {
        lastTog = P3DS_RunToggle;
        lastHeld = (int)held;
        PSPNativeMemLog("[INPUT] run test #%d: auto_run toggle %d, keys 0x%x (B %u) -> run %u", calls, P3DS_RunToggle, keys, held, out);
    }
    return out;
}

/* IV / EV on the summary screen (summary_ivs, Settings > Controls, default off; Y shows / hides the box). The summary
   gathers a Pokemon's data in sub_0208981C, which ends with ReleaseMonLock(mon, lock) while the mon is still
   decrypted. pspoke-patches/0051 retargets that call here: copy the 12 values for the port's read-only overlay
   (render3ds.cpp drawSumIV), then release as the game would. PokemonSummary_Main's per-frame sub_0208B278 is
   retargeted to SS3DS_SummaryTick, which tells the overlay the summary is open. Nothing is written to the Pokemon. */
extern unsigned int GetMonData(unsigned int mon, int param, void *unused);
extern unsigned int ReleaseMonLock(unsigned int mon, unsigned int locked);
extern int P3DS_SummaryIvsCfg(void);
extern volatile int P3DS_SumIV[6], P3DS_SumEV[6], P3DS_SumAlive;
#include "ribbons.h"
unsigned int SS3DS_SummaryRelease(unsigned int mon, unsigned int locked)
{
    extern void PSPNativeMemLog(const char *, ...);
    static const int ivId[6] = { 70, 71, 72, 74, 75, 73 };   /* HP Atk Def SpA SpD Spe (MON_DATA_*_IV) */
    static const int evId[6] = { 13, 14, 15, 17, 18, 16 };   /* MON_DATA_*_EV */
    if (P3DS_SummaryIvsCfg()) {
        for (int i = 0; i < 6; i++) {
            P3DS_SumIV[i] = (int)GetMonData(mon, ivId[i], 0);
            P3DS_SumEV[i] = (int)GetMonData(mon, evId[i], 0);
        }
        PSPNativeMemLog("[SUMMARY] IV %d/%d/%d/%d/%d/%d EV %d/%d/%d/%d/%d/%d", P3DS_SumIV[0], P3DS_SumIV[1], P3DS_SumIV[2],
                        P3DS_SumIV[3], P3DS_SumIV[4], P3DS_SumIV[5], P3DS_SumEV[0], P3DS_SumEV[1], P3DS_SumEV[2],
                        P3DS_SumEV[3], P3DS_SumEV[4], P3DS_SumEV[5]);
    }
    if (P3DS_RibbonOn()) P3DS_RibbonCapture(mon, (unsigned (*)(unsigned, int, void *))GetMonData); /* ribbon_check: read-only, ribbons.c */
    return ReleaseMonLock(mon, locked);
}

extern unsigned int sub_0208B278(unsigned int data);
unsigned int SS3DS_SummaryTick(unsigned int data)
{
    P3DS_SumAlive = 4;
    return sub_0208B278(data);
}

/* run 3: sub_02038D90 (comm "safe to cancel save / stop comm" check, unk_02037C94.s) was only an [SS-MISSING] trap, so a
   soft reset (L+R+START+SELECT -> DoSoftReset) or a communication-error screen aborted (hardware run 2: abort() from the
   trap at 0x12ed10, map 33). Hand port. With no comm session (_021D4150 == NULL, always the case offline) the DS returns
   TRUE after sub_02037988 sets the flag at +0x6B9 of the _021D4140+8 object. The comm-active branch (link/wireless, not
   supported in this port) only tears down the comm task; here it logs and returns TRUE so the save is cancelled safely. */
extern unsigned char ssdata_unk_02035900__021D4140[];
extern void *ssdata_unk_02037C94__021D4150;
extern void PSPNativeMemLog(const char *, ...);
__attribute__((weak)) int sub_02038D90(void) {
    unsigned char *p = *(unsigned char **)(ssdata_unk_02035900__021D4140 + 8);
    if (p) p[0x6B9] = 1;
    if (ssdata_unk_02037C94__021D4150 != 0) PSPNativeMemLog("[SS3DS] sub_02038D90 with a comm session (unsupported) -> TRUE");
    return 1;
}
/* run 3: sub_02039AA4 ("comm is idle, OK to reset", unk_02037C94.s) was also only a trap; DoSoftReset polls it every
   frame before OS_ResetSystem. Offline (_021D4150 == NULL, sub_02037D78() false) the DS returns TRUE. */
__attribute__((weak)) int sub_02039AA4(void) { return 1; }
