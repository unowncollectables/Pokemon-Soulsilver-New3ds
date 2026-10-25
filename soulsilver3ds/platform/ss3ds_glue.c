/* ss3ds: definitions the Platinum platform layer expects from Platinum's own game/native patches, and the
   game-facing pieces of pspoke's SoulSilver PSP main.c (dropped from the link, see full/Makefile). */
#include <3ds.h>

/* VRAM write tracking. Platinum stamps 4 KB VRAM pages from redirected game writes (pspoke vram_dirty.c +
   opttex_redirect.py); SoulSilver's objects are not redirected, so every page counts as written once per
   frame. g3soft's texture cache then always falls back to its byte compare: correct, a little slower. */
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
