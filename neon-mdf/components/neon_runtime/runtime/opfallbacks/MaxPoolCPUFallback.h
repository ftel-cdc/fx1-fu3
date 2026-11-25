#pragma once

// ============================================================
// NEON STUDIO — MAXPOOL2D CPU FALLBACK
//
// Registered with fallback_id == NEON_CPU_FALLBACK_MAXPOOL. Pure spatial
// max reduction, int8, no rescale (input/output share quantization).
// ============================================================

#include "../loader/NPUModel.h"

#ifdef __cplusplus
extern "C++" {

bool maxPoolCPUFallback(const RuntimeOp& op, const NPUModel& model);

} // extern "C++"
#endif
