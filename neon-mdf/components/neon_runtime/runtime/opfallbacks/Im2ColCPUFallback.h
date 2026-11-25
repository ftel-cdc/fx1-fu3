#pragma once

// ============================================================
// NEON STUDIO — IM2COL CPU FALLBACK
//
// CPU fallback for Im2Col operation on Rich instruction mode (Conv2D >= 3x3).
// Registered with fallback_id == NEON_CPU_FALLBACK_IM2COL.
// ============================================================

#include "../loader/NPUModel.h"

#ifdef __cplusplus
extern "C++" {

bool im2colCPUFallback(const RuntimeOp& op, const NPUModel& model);

} // extern "C++"
#endif
