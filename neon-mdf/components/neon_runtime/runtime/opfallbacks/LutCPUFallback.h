#pragma once

// ============================================================
// NEON STUDIO — LUT CPU FALLBACK
//
// CPU fallback for post-GEMM activation + requantization LUT on Rich mode.
// Registered with fallback_id == NEON_CPU_FALLBACK_LUT.
// ============================================================

#include "../loader/NPUModel.h"

#ifdef __cplusplus
extern "C++" {

bool lutCPUFallback(const RuntimeOp& op, const NPUModel& model);

} // extern "C++"
#endif
