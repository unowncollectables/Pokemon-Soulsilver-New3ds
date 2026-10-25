/* ss3ds: the PSP kernel calls SoulSilver's pspoke app uses on top of what psp_ctru.c (from Platinum) provides. */
#include <3ds.h>
#include <pspkernel.h>
#include <psploadexec.h>
_Static_assert(sizeof(RecursiveLock) <= sizeof(SceLwMutexWorkarea), "SceLwMutexWorkarea too small");
int sceKernelCreateLwMutex(SceLwMutexWorkarea *work, const char *name, SceUInt attr, int count, void *opt)
{ (void)name; (void)attr; (void)count; (void)opt; RecursiveLock_Init((RecursiveLock *)work); return 0; }
int sceKernelLockLwMutex(SceLwMutexWorkarea *work, int count, SceUInt *timeout)
{ (void)timeout; for (int i = 0; i < count; i++) RecursiveLock_Lock((RecursiveLock *)work); return 0; }
int sceKernelUnlockLwMutex(SceLwMutexWorkarea *work, int count)
{ for (int i = 0; i < count; i++) RecursiveLock_Unlock((RecursiveLock *)work); return 0; }
/* The PSP relaunches its EBOOT to soft-reset. Not wired up on the 3DS yet: returning an error makes
   OS_ResetSystem log "native restart failed" and abort, which the platform layer reports on screen. */
/* run 3: the soft reset (L+R+START+SELECT) now reaches this (hardware run 2 aborted earlier, in the sub_02038D90 trap);
   hand it to P3DS_SoftReset (main3ds.c, run 53): clean exit + relaunch of this title / 3dsx. */
int sceKernelLoadExec(const char *file, struct SceKernelLoadExecParam *param) { extern void P3DS_SoftReset(unsigned); (void)file; (void)param; P3DS_SoftReset(0); return -1; }  /* fallback; OS_ResetSystem calls P3DS_SoftReset itself (0061) */
