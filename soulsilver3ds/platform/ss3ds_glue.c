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
int sub_02037D78(void) { return ssdata_unk_02037C94__021D4150 != 0; }

/* Wide view (and the other overworld-only extras) key on P3DS_InField. Platinum sets it from pspoke's "enter app"
   memprobe hook (last app = gFieldMapTemplate); SoulSilver's app has no such hook, so watch the overlay managers
   instead. ov01_02206378 is the field map's template (field_system.c FieldSystem_LoadFieldOverlayInternal); every
   other manager (battle, bag, title, ...) replaces it while it runs and clears the flag. Weak: if a pspoke bump
   renames it, the link still works and wide view simply stays off in the overworld. */
extern int P3DS_InField;
extern const char ov01_02206378[] __attribute__((weak));
extern void *__real_OverlayManager_New(const void *template, void *parentWork, int heapID);
void *__wrap_OverlayManager_New(const void *template, void *parentWork, int heapID)
{
    const int field = template && template == (const void *)ov01_02206378;
    if (field != P3DS_InField) { extern void PSPNativeMemLog(const char *, ...); PSPNativeMemLog("[3DS] field %d", field); }
    P3DS_InField = field;
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
