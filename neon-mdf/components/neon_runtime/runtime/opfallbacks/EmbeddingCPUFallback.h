#pragma once

// ============================================================
// NEON STUDIO — EMBEDDING CPU FALLBACK
//
// Registered with fallback_id == NEON_CPU_FALLBACK_EMBEDDING.
// Gathers rows of a const int8 table + optional folded const bias + requant.
// ============================================================

#include "../loader/NPUModel.h"

#ifdef __cplusplus
extern "C++" {

bool embeddingCPUFallback(const RuntimeOp& op, const NPUModel& model);

} // extern "C++"
#endif
