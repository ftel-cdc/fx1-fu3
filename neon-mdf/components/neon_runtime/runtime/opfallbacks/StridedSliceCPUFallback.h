#pragma once

// ============================================================
// NEON STUDIO — STRIDED SLICE CPU FALLBACK
//
// Registered with fallback_id == NEON_CPU_FALLBACK_STRIDED_SLICE. Rank <= 4
// strided gather over a dense row-major tensor; no requantization (TFLite
// STRIDED_SLICE keeps scale/zero point). Contiguous innermost rows
// (stride[3] == 1) are moved with one memcpy each.
// ============================================================

#include "../loader/NPUModel.h"

#ifdef __cplusplus
extern "C++" {

bool stridedSliceCPUFallback(const RuntimeOp& op, const NPUModel& model);

} // extern "C++"
#endif

