/* Breadcrumb ring (cheap): tag + caller address; repeats of the same entry only bump a counter. */
#ifndef P3DS_CRUMBS_H
#define P3DS_CRUMBS_H
#ifdef __cplusplus
extern "C" {
#endif
void P3DS_Crumb(const char *tag, void *caller);
void P3DS_CrumbDump(void);
/* Boot diagnostics (main3ds.c): synchronous lines in sdmc:/3ds/soulsilver3ds/boot.txt, and a per-thread fault handler
   that writes thread name/id, registers, FPU state and a stack sample there. Call P3DS_ThreadGuard first in every thread. */
void P3DS_Bc(const char *s);
void P3DS_Bcf(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
void P3DS_ThreadGuard(const char *name);
#ifdef __cplusplus
}
#endif
#define CRUMB(t) P3DS_Crumb(t, __builtin_return_address(0))
#endif
