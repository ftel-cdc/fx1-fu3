#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/* Call from a task after the application's allocator is ready. Safe to call
 * more than once; the registry ignores duplicate fallback IDs. */
void NeonMDFRegisterEmbeddedCpuFallbacks(void);

#ifdef __cplusplus
}
#endif
