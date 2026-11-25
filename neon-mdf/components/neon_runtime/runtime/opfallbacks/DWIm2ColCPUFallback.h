#pragma once

// ============================================================
// NEON STUDIO — DEPTHWISE-CONV2D IM2COL CPU FALLBACK
//
// Gathers one input channel's spatial patches into a [M][K] block, K = kh*kw.
// Registered with fallback_id == NEON_CPU_FALLBACK_DW_IM2COL. Invoked once per
// channel, either unrolled at compile time or looped via the Runtime Loop
// Contract (NeonLoopBeginInstr) — see DWRichGemmEmitter.cpp.
// ============================================================

#include "../loader/NPUModel.h"

#ifdef __cplusplus
extern "C++" {

bool dwIm2colCPUFallback(const RuntimeOp& op, const NPUModel& model);

} // extern "C++"
#endif
