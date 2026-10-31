/* ss3ds comm stack, stage 1 link fix (2026-10-28, run 53 link: 104 undefined symbols). Patch 0127 translates the real comm drivers
   (unk_02037C94 ... ), which also reach the Nintendo Wi-Fi Connection (DWC) glue: ARM-mode functions in lib/NitroDWC/asm/overlay_00_arm.s
   and overlay_45_arm.s (the translator only handles Thumb) and Thumb pointer-table targets in overlay_45_thumb.s / unk_0208F814.s
   that nothing translates. Local wireless never needs them (online WFC is a separate project, docs/WIRELESS.md Option B), so each is a
   weak stand-in that logs its first calls as "[COMM] DWC stub <name>" and returns 0 (= "not available / failed"). If one of these
   lines shows up in log.txt, that is the next function to port. Plain C, no game headers. */
extern void PSPNativeMemLog(const char *fmt, ...);
#define DWC_STUB(name) __attribute__((weak)) unsigned int name(void) { static int n; if (n++ < 4) PSPNativeMemLog("[COMM] DWC stub " #name " called (#%d) -> 0", n); return 0; }

/* overlay 0 and overlay 45 Wi-Fi glue (ARM asm in lib/NitroDWC, plus overlay 45 Thumb pointer-table targets) */
DWC_STUB(ov00_021EC11C)
DWC_STUB(ov00_021EC210)
DWC_STUB(ov00_021EC294)
DWC_STUB(ov00_021EC3F0)
DWC_STUB(ov00_021EC4A4)
DWC_STUB(ov00_021EC5B4)
DWC_STUB(ov00_021EC60C)
DWC_STUB(ov00_021EC724)
DWC_STUB(ov00_021EC8D8)
DWC_STUB(ov00_021EC9D4)
DWC_STUB(ov00_021ECB40)
DWC_STUB(ov00_021ED730)
DWC_STUB(ov00_021ED9B4)
DWC_STUB(ov00_021EDD6C)
DWC_STUB(ov00_021EDE74)
DWC_STUB(ov00_021EDFBC)
DWC_STUB(ov00_021EE054)
DWC_STUB(ov00_021EE24C)
DWC_STUB(ov00_021EE278)
DWC_STUB(ov00_021EE384)
DWC_STUB(ov00_021EE448)
DWC_STUB(ov00_021EE530)
DWC_STUB(ov00_021EFDCC)
DWC_STUB(ov00_021EFF04)
DWC_STUB(ov00_021F1284)
DWC_STUB(ov00_021F9580)
DWC_STUB(ov00_021F9750)
DWC_STUB(ov00_021F9850)
DWC_STUB(ov00_021F989C)
DWC_STUB(ov00_021F98BC)
DWC_STUB(ov00_021F98DC)
DWC_STUB(ov00_021F9988)
DWC_STUB(ov00_021E5CBC)
DWC_STUB(ov00_021E6A70)
DWC_STUB(ov00_021E6CCC)
DWC_STUB(ov00_021E6D60)
DWC_STUB(ov00_021E6D68)
DWC_STUB(ov00_021E6EBC)
DWC_STUB(ov00_021E7078)
DWC_STUB(ov00_021E7080)
DWC_STUB(ov00_021E709C)
DWC_STUB(ov00_021E70B8)
DWC_STUB(ov00_021E70F4)
DWC_STUB(ov00_021E714C)
DWC_STUB(ov00_021E7220)
DWC_STUB(ov00_021E7328)
DWC_STUB(ov00_021EC0FC)
DWC_STUB(ov00_021EC2A8)
DWC_STUB(ov00_021EC2EC)
DWC_STUB(ov00_021EC454)
DWC_STUB(ov00_021EC9E0)
DWC_STUB(ov00_021ECB94)
DWC_STUB(ov00_021ECD04)
DWC_STUB(ov00_021ECDC8)
DWC_STUB(ov00_021EDB1C)
DWC_STUB(ov00_021EE490)
DWC_STUB(ov00_021EE4FC)
DWC_STUB(ov00_021FA0B4)
DWC_STUB(ov00_021FA0D8)
DWC_STUB(ov00_021FA118)
DWC_STUB(ov00_021FA21C)
DWC_STUB(ov00_021FFC18)
DWC_STUB(ov45_0222CF00)
DWC_STUB(ov45_0222CF24)
DWC_STUB(ov45_0222CF40)
DWC_STUB(ov45_0222CF68)
DWC_STUB(ov45_0222CFF4)
DWC_STUB(ov45_0222D01C)
DWC_STUB(ov45_0222D028)
DWC_STUB(ov45_0222D054)
DWC_STUB(ov45_0222D078)
DWC_STUB(ov45_0222D0BC)
DWC_STUB(ov45_0222D0C8)
DWC_STUB(ov45_0222D0D8)
DWC_STUB(ov45_0222D0FC)
DWC_STUB(ov45_0222D130)
DWC_STUB(ov45_0222D13C)
DWC_STUB(ov45_0222D14C)
DWC_STUB(ov45_0222D164)
DWC_STUB(ov45_0222D19C)
DWC_STUB(ov45_0222D1B0)
DWC_STUB(ov45_0222D1DC)
DWC_STUB(ov45_0222D20C)
DWC_STUB(ov45_0222D23C)
DWC_STUB(ov45_0222D250)
DWC_STUB(ov45_0222D27C)
DWC_STUB(ov45_0222D2AC)
DWC_STUB(ov45_0222D2E4)
DWC_STUB(ov45_0222D2F8)
DWC_STUB(ov45_0222D324)
DWC_STUB(ov45_0222D354)
DWC_STUB(ov45_0222D380)
DWC_STUB(ov45_0222D38C)
DWC_STUB(ov45_0222D3B0)
DWC_STUB(ov45_0222D3C4)
DWC_STUB(ov45_0222D3D8)
DWC_STUB(ov45_0222D410)
DWC_STUB(ov45_0222D41C)
DWC_STUB(ov45_0222D428)
DWC_STUB(ov45_0222D448)
DWC_STUB(ov45_0222D44C)
DWC_STUB(ov45_0222D484)
DWC_STUB(ov45_0222D4C0)
DWC_STUB(ov45_0222D4CC)
DWC_STUB(ov45_0222D4DC)
DWC_STUB(ov45_02230B64)
DWC_STUB(ov45_02230B8C)
DWC_STUB(ov45_02230BFC)
DWC_STUB(ov45_02230C40)
DWC_STUB(ov45_02230CD8)
DWC_STUB(ov45_02230D20)
DWC_STUB(ov45_02230D5C)
DWC_STUB(ov45_022310C0)
DWC_STUB(ov45_02231514)
DWC_STUB(ov45_02231EC0)
DWC_STUB(ov45_022320C4)
DWC_STUB(ov45_02232170)
DWC_STUB(ov45_022321E0)
DWC_STUB(ov45_02232304)
DWC_STUB(ov45_02232580)
DWC_STUB(ov45_02232BB0)
DWC_STUB(ov45_02232CA4)

/* unk_0208F814.s pointer-table targets (Thumb, not in the translator closure) */
DWC_STUB(sub_0208F828)
DWC_STUB(sub_0208F8BC)
DWC_STUB(sub_0208F8E4)
DWC_STUB(sub_0208F918)
DWC_STUB(sub_0208F960)
DWC_STUB(sub_0208FA50)
DWC_STUB(sub_0208FA54)
DWC_STUB(sub_0208FA88)
DWC_STUB(sub_0208FA8C)
DWC_STUB(sub_0208FA90)
DWC_STUB(sub_0208FAA8)
DWC_STUB(sub_0208FAEC)
DWC_STUB(sub_0208FB4C)
DWC_STUB(sub_0208FB5C)
DWC_STUB(sub_0208FB60)

/* Run 55 (two consoles, Union Room, Local wireless ON): both crashed in the translated sub_0203A700 (unk_0203A3B0.s), the wireless-icon
   per-frame redraw: it reads DISPCNT/POWCNT through raw DS addresses and writes DS OBJ VRAM through a bank pointer that was NULL here
   (str r1,[r6], far=0). The icon is cosmetic, so patch 0132 lists it in native-provided.json and this strong no-op replaces it. */
unsigned int sub_0203A700(void *task) {
    static int n;
    (void)task;
    if (n++ < 2) PSPNativeMemLog("[COMM] wireless icon redraw (sub_0203A700) skipped (#%d)", n);
    return 0;
}

/* Run 56 (two consoles talked in the Union Room): the crash at the end of the conversation was the wireless-icon teardown sub_0203A444
   (inlined into the translated sub_0203A914): after destroying the icon task it hides the sprite by writing DS OAM directly at
   0x07000000 / 0x07000400, which is unmapped here. Patch 0133 lists it in native-provided.json; this keeps the task destroy and the free
   (the icon object: +0x18 = its SysTask) and drops the raw OAM writes (the redraw above never draws it anyway). */
extern void SysTask_Destroy(void *task);
extern void Heap_Free(void *p);
unsigned int sub_0203A444(void *icon) {
    static int n;
    if (n++ < 4) PSPNativeMemLog("[COMM] wireless icon teardown (sub_0203A444) %p (#%d)", icon, n);
    if (icon) { SysTask_Destroy(*(void **)((unsigned char *)icon + 0x18)); Heap_Free(icon); }
    return 0;
}
