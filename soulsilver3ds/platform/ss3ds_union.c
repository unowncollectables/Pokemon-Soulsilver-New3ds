/* ss3ds Union Room port, chunk A (2026-10-28, docs/WIRELESS.md "Union Room port plan"). Run 42: entering the Union Room aborted on
   [SS-MISSING] sub_02059DB0. The Union Room manager (asm/unk_02058AEC.s, unk_0205A44C.s) and the comm drivers under it
   (unk_02032844..0203A3B0) are assembly-only in the decomp. This file hand-ports the entry chain from the asm and backs the comm
   drivers with "no session" stubs that log every call ("[UNION] ..."), so the room can load and the next missing piece shows in
   log.txt instead of an abort. Real trading replaces the stubs one by one with wm_uds.c-backed code.
   Used only with union_guard OFF (hook 0117): with it ON the script command never reaches sub_02059DB0.
   Plain C with local declarations (like ss3ds_glue.c); no game headers. The Missing() trap lines for the names defined here are
   removed from missing.c by pspoke patch 0118. */
#include <string.h>

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;

extern void PSPNativeMemLog(const char *fmt, ...);
extern int Heap_CreateAtEnd(int parentId, int heapId, u32 size);
extern void *Heap_Alloc(int heapId, u32 size);
extern void MI_CpuFill8(void *dst, u8 data, u32 size);
extern void *SysTask_CreateOnMainQueue(void (*func)(void *task, void *data), void *data, u32 priority);
extern void SysTask_Destroy(void *task);
extern void *FieldSystem_GetSaveData(void *fieldSystem);
extern void *Save_PlayerData_GetProfile(void *saveData);
extern void sub_02091574(void *fieldSystem);
extern void MailMsg_Init_Default(void *msg);

/* A stub the game can call before the real code exists: logs the first calls, returns 0 ("nothing / no session"). */
#define UNION_STUB(name) int name(void) { static int n; if (n++ < 8) PSPNativeMemLog("[UNION] stub " #name " called (#%d) -> 0", n); return 0; }

/* Union Room manager layout used by the ported code (asm/unk_02058AEC.s): 0 fieldSystem, 4 saveData, 8 profile, 0xC SysTask,
   0x10 state callback, 0x14 state timer, 0x110 + 4*i per-player comm value, 0x150 random state; total 0x190 bytes. */
typedef struct UnionMgr { void *fieldSystem; void *saveData; void *profile; void *task; void (*state)(struct UnionMgr *); int timer; } UnionMgr;
#define MGR_SIZE 0x190
#define MGR_HEAP 0x1f
#define MGR_AT(m, off) (*(u32 *)((u8 *)(m) + (off)))

/* ---- comm driver stand-ins (asm/unk_02037C94.s, unk_02034B0C.s, unk_02035900.s): no comm session exists ---- */
void sub_02037F18(void) { static int n; if (n++ < 8) PSPNativeMemLog("[UNION] comm init (sub_02037F18) stub: no comm session (#%d)", n); }
int sub_02038C1C(int mode) { static int n; if (n++ < 8) PSPNativeMemLog("[UNION] sub_02038C1C(%d) stub (#%d)", mode, n); return 0; }
int sub_02035650(void) { return 0; }                 /* comm "connected" check: never */
u32 sub_02035754(int netId) { (void)netId; return 0; } /* per-player comm value */
void sub_02038094(void) { PSPNativeMemLog("[UNION] sub_02038094 stub"); }
int sub_02037474(void) { return 1; }                 /* comm active: keeps sub_0205A0A0 idle instead of tearing down */
void sub_0203778C(void *rng) { /* asm: 0x18-byte random state seeded from the RTC; the seed value is not part of any save, a fixed one is fine */
    u32 *p = (u32 *)rng; p[0] = 0x12345678; p[1] = 0; p[2] = 0x6C078965; p[3] = 0x5D588B65; p[4] = 0x00269EC3; p[5] = 0; }
void sub_0205A1D4(UnionMgr *m) { (void)m; PSPNativeMemLog("[UNION] sub_0205A1D4 (comm teardown) stub"); }
void sub_02059EBC(UnionMgr *m) { (void)m; static int n; if (n++ < 4) PSPNativeMemLog("[UNION] sub_02059EBC (state after the comm check) stub"); }

/* ---- Union Room manager (asm/unk_02058AEC.s), ported ---- */
void sub_0205A034(UnionMgr *m, void (*state)(UnionMgr *), int timer) { m->state = state; m->timer = timer; }

void sub_0205A03C(void *task, void *data) { /* SysTask: refresh the per-player comm values, then run the state callback */
    UnionMgr *m = (UnionMgr *)data;
    int i;
    if (!m) { SysTask_Destroy(task); return; }
    for (i = 0; i < 16; i++) MGR_AT(m, 0x110 + 4 * i) = sub_02035754(i);
    if (m->state) m->state(m);
}

void sub_0205ABBC(UnionMgr *m) { /* asm unk_0205A44C.s: clear the sub-state fields */
    MGR_AT(m, 0x20) = 0; MGR_AT(m, 0x24) = 0; MGR_AT(m, 0x2c) = 0; MGR_AT(m, 0x30) = 0; MGR_AT(m, 0x40) = 0;
    *(u16 *)((u8 *)m + 0x174) = 0; MGR_AT(m, 0x180) = 0; MGR_AT(m, 0x44) = 0; }

UnionMgr *sub_02059E1C(void *fieldSystem) {
    UnionMgr *m;
    if (MGR_AT(fieldSystem, 0x80)) return 0;
    void *save = FieldSystem_GetSaveData(fieldSystem);
    sub_02037F18();
    m = (UnionMgr *)Heap_Alloc(MGR_HEAP, MGR_SIZE);
    MI_CpuFill8(m, 0, MGR_SIZE);
    MGR_AT(m, 0x10) = 0; MGR_AT(m, 0x14) = 0x28;
    m->task = SysTask_CreateOnMainQueue(sub_0205A03C, m, 0xa);
    m->fieldSystem = fieldSystem; m->saveData = save;
    m->profile = Save_PlayerData_GetProfile(save);
    sub_0205ABBC(m);
    sub_0203778C((u8 *)m + 0x150);
    PSPNativeMemLog("[UNION] manager created (sub_02059E1C), comm is a stub");
    return m;
}

void sub_02059E88(UnionMgr *m) { /* first state: waits for a comm session; with none it just keeps waiting */
    u8 msg[8];
    if (sub_02035650()) {
        extern int sub_0205AB88(); extern int sub_0205AA6C();
        MailMsg_Init_Default(msg); sub_0205AB88(msg); sub_0205AA6C(m, msg);
        sub_0205A034(m, sub_02059EBC, 0x28);
    }
}

UnionMgr *sub_02059DB0(void *fieldSystem) {
    UnionMgr *m;
    if (MGR_AT(fieldSystem, 0x80)) return 0;
    Heap_CreateAtEnd(3, MGR_HEAP, 0x2a << 6);
    m = sub_02059E1C(fieldSystem);
    if (!m) m = (UnionMgr *)MGR_AT(fieldSystem, 0x80);
    sub_02091574(fieldSystem);
    sub_02038C1C(2);
    sub_0205A034(m, sub_02059E88, 0x28);
    PSPNativeMemLog("[UNION] sub_02059DB0 done: Union Room manager %p", (void *)m);
    return m;
}

static void sub_0205A0A0(UnionMgr *m) { if (!sub_02037474()) sub_0205A1D4(m); }
void sub_0205A07C(UnionMgr *m) {
    if (m->timer) { m->timer--; return; }
    sub_02038094();
    sub_0205A034(m, sub_0205A0A0, 0);
}
void sub_02059E04(void *fieldSystem) {
    UnionMgr *m = (UnionMgr *)MGR_AT(fieldSystem, 0x80);
    if (m) sub_0205A034(m, sub_0205A07C, 5);
}

/* ---- everything else in the Union Room files that compiled game code calls: "nothing / no session" until it is ported ---- */
UNION_STUB(sub_02059AE4)
UNION_STUB(sub_02059B18)
UNION_STUB(sub_02059B34)
UNION_STUB(sub_02059D44)
UNION_STUB(sub_0205A1F0)
UNION_STUB(sub_0205A1F4)
UNION_STUB(sub_0205A200)
UNION_STUB(sub_0205A284)
UNION_STUB(sub_0205A358)
UNION_STUB(sub_0205A35C)
UNION_STUB(sub_0205A39C)
UNION_STUB(sub_0205A3B0)
UNION_STUB(sub_0205A408)
UNION_STUB(sub_0205A40C)
UNION_STUB(sub_0205A410)
UNION_STUB(sub_0205A430)
UNION_STUB(sub_0205A44C)
UNION_STUB(sub_0205A46C)
UNION_STUB(sub_0205A478)
UNION_STUB(sub_0205A47C)
UNION_STUB(sub_0205A498)
UNION_STUB(sub_0205A4B4)
UNION_STUB(sub_0205A4C4)
UNION_STUB(sub_0205A4D8)
UNION_STUB(sub_0205A508)
UNION_STUB(sub_0205A6AC)
UNION_STUB(sub_0205A750)
UNION_STUB(sub_0205A894)
UNION_STUB(sub_0205A904)
UNION_STUB(sub_0205A9A0)
UNION_STUB(sub_0205AA6C)
UNION_STUB(sub_0205AA9C)
UNION_STUB(sub_0205AB88)
UNION_STUB(sub_0205ABB0)
UNION_STUB(sub_0205ABD8)
UNION_STUB(sub_0205AC4C)
UNION_STUB(sub_0205AC70)
