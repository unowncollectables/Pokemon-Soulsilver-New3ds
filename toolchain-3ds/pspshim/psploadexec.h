/* ss3ds: PSP LoadExec (SoulSilver's OS_ResetSystem relaunches its own EBOOT); see psp_ss_ctru.c. */
#ifndef SS3DS_PSPLOADEXEC_H
#define SS3DS_PSPLOADEXEC_H
#include <pspkernel.h>
#ifdef __cplusplus
extern "C" {
#endif
struct SceKernelLoadExecParam { SceSize size; SceSize args; void *argp; const char *key; };
int sceKernelLoadExec(const char *file, struct SceKernelLoadExecParam *param);
#ifdef __cplusplus
}
#endif
#endif
