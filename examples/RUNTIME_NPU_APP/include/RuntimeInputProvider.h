#pragma once

#include <stddef.h>
#include <stdint.h>
#include <dlpack/dlpack.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Generate the app's deterministic synthetic int8 NCHW input directly in the
 * fixed RAW_IN0 region, then describe it as a DLTensor for NeonGraphRun(). */
void RuntimeNpuAppSetGaussianSeed(uint32_t seed);
int RuntimeNpuAppPrepareInput(void *raw_in0, size_t capacity,
                              DLTensor *tensor);

#ifdef __cplusplus
}
#endif
