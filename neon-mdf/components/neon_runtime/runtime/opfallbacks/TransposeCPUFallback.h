#pragma once

// ============================================================
// NEON STUDIO — TRANSPOSE CPU FALLBACK
//
// CPU fallback for permute/transpose operations (rank <= 4).
// Registered with fallback_id == NEON_CPU_FALLBACK_TRANSPOSE.
// ============================================================

#include "../loader/NPUModel.h"

#ifdef __cplusplus
extern "C++" {

bool transposeCPUFallback(const RuntimeOp& op, const NPUModel& model);

} // extern "C++"
#endif
