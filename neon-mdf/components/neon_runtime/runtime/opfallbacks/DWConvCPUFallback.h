#pragma once

// ============================================================
// NEON STUDIO — DEPTHWISE CONV2D CPU FALLBACK
//
// CPU fallback for depthwise 2D convolution operations.
// Registered with fallback_id == NEON_CPU_FALLBACK_DWCONV.
// ============================================================

#include "../loader/NPUModel.h"

#ifdef __cplusplus
extern "C++" {

bool dwConvCPUFallback(const RuntimeOp& op, const NPUModel& model);

} // extern "C++"
#endif
