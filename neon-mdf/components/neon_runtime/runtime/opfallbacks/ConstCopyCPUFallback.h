#pragma once

// ============================================================
// NEON STUDIO — CONSTANT-MATERIALIZATION CPU FALLBACK
//
// Copies a compile-time-folded constant from the weight buffer to the output
// buffer. Registered with fallback_id == NEON_CPU_FALLBACK_CONST_COPY.
// ============================================================

#include "../loader/NPUModel.h"

#ifdef __cplusplus
extern "C++" {

bool constCopyCPUFallback(const RuntimeOp& op, const NPUModel& model);

} // extern "C++"
#endif
