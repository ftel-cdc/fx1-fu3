/*===- neon/c_runtime_api.h - Neon runtime Open ABI ----------------------===*
 * Stable C ABI over GraphLoader/ArenaPlanner/GraphRunner. Depends only on
 * <stdint.h>, <stddef.h> and <dlpack/dlpack.h>. No C++, no exceptions, no TLS:
 * the last error is a static buffer (single-threaded use on FreeRTOS).
 * int-returning functions: 0 = success, -1 = error (NeonRuntimeGetLastError).
 *===----------------------------------------------------------------------===*/
#ifndef NEON_C_RUNTIME_API_H_
#define NEON_C_RUNTIME_API_H_

#include <stddef.h>
#include <stdint.h>
#include <dlpack/dlpack.h>

#ifdef __cplusplus
extern "C" {
#endif

#define NEON_RUNTIME_ABI_VERSION 1

typedef struct NeonPlatform NeonPlatform;  /* opaque; built by device glue */
typedef struct NeonGraph *NeonGraphHandle;

typedef struct {
  uint32_t struct_size;
  void *arena;   size_t arena_bytes;    /* >= NeonGraphInfo.arena_bytes   */
  void *scratch; size_t scratch_bytes;  /* >= NeonGraphInfo.scratch_bytes */
  uintptr_t (*heap_mark)(void);         /* optional (bump allocators)     */
  void (*heap_release)(uintptr_t);      /* optional                       */
} NeonGraphMemory;

typedef struct {
  uint32_t struct_size;
  int32_t num_inputs, num_outputs, num_fragments, num_tensors;
  int32_t single_program;               /* 1 = wrapped single NEONNSM program */
  size_t arena_bytes, scratch_bytes;
} NeonGraphInfo;

/* phase: 0 = fragment begin, 1 = fragment end (status/out valid on end). */
typedef void (*NeonFragmentCallback)(void *user, uint32_t frag_idx, int32_t phase,
                                     int32_t status, const uint8_t *out, size_t out_bytes);

int32_t NeonRuntimeAbiVersion(void);
int NeonGraphLoad(const uint8_t *buf, size_t size, const NeonPlatform *platform,
                  uint32_t single_program_inputs, NeonGraphHandle *out);
int NeonGraphGetInfo(NeonGraphHandle h, NeonGraphInfo *info);
int NeonGraphGetIOBytes(NeonGraphHandle h, int32_t is_output, int32_t index, size_t *bytes);
int NeonGraphSetFragmentCallback(NeonGraphHandle h, NeonFragmentCallback cb, void *user);
int NeonGraphPrepare(NeonGraphHandle h, const NeonGraphMemory *mem);
/* DLTensor rules: device kDLCPU, lanes 1, bits % 8 == 0, strides NULL or compact,
 * prod(shape) * bits/8 == the graph I/O size; data + byte_offset is used zero-copy. */
int NeonGraphRun(NeonGraphHandle h, const DLTensor *const *inputs, int32_t num_inputs,
                 DLTensor *const *outputs, int32_t num_outputs);
void NeonGraphFree(NeonGraphHandle h);
const char *NeonRuntimeGetLastError(void);
int32_t NeonRuntimeGetLastDriverStatus(void);  /* last NPUDriverStatus as int */

#ifdef __cplusplus
}
#endif
#endif /* NEON_C_RUNTIME_API_H_ */
