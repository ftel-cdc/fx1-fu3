#pragma once

// ============================================================
// NEON STUDIO — LAYERNORM CPU FALLBACK
//
// Registered with fallback_id == NEON_CPU_FALLBACK_LAYERNORM.
// Implements per-row mean/variance reduction + PWL 1/sqrt LUT
// (pwl_lut_mapping_en.md Part B) + three requant stages.
// ============================================================

#include "../loader/NPUModel.h"

#ifdef __cplusplus
extern "C++" {

bool layerNormCPUFallback(const RuntimeOp& op, const NPUModel& model);

} // extern "C++"
#endif
