#pragma once

// ============================================================
// NEON STUDIO — SOFTMAX CPU FALLBACK
//
// Registered with fallback_id == NEON_CPU_FALLBACK_SOFTMAX. Implements
// pwl_lut_mapping_en.md Part A (§3): row max -> exp_lut -> integer sum ->
// CLZ exponent split -> 256-entry Q15 reciprocal table with 8-bit linear
// interpolation -> exponent reapplied as a shift -> INT8 requantization.
//
// Must stay bit-identical to hwsim/fx1_vlu.c's DIRECT + IDX_NORM path with
// cfg_num_segments = 8, cfg_seg_shift = 8, cfg_e_bias = 31, and to the
// tables in hwsim/fx1_softmax_coeffs.h.

#include "../loader/NPUModel.h"

#ifdef __cplusplus
extern "C++" {

bool softmaxCPUFallback(const RuntimeOp& op, const NPUModel& model);

} // extern "C++"
#endif
