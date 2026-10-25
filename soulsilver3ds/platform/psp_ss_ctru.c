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
int sceKernelLoadExec(const char *file, struct SceKernelLoadExecParam *param) { (void)file; (void)param; return -1; }
