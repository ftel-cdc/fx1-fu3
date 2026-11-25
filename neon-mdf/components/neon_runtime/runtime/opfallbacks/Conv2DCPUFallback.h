#pragma once

// ============================================================
// NEON STUDIO — CONV2D CPU FALLBACK
//
// First real registration for the CPU-fallback escape hatch (see
// docs/hybrid_npu_cpu_fallback_design.md, core/NPUUnsupportedOpFallback.h,
// runtime/opfallbacks/CPUFallbackRegistry.h). Adapts a NeonOpcode::CPU_FALLBACK
// dispatch (fallback_id == NEON_CPU_FALLBACK_CONV2D, instruction shaped as
// NeonCPUFallbackConv2DInstr — see core/NPU_ISA.h) into a call to the
// already-existing analyse/compiler/CompilerOpConv2D.h contract's
// conv2D_Func().
//
// Does NOT implement conv2d math itself — whichever conv2D_Func()
// implementation is linked into the final binary is what actually runs
// (device/qemu_mps3_an547/analyse/compiler/QEMUCompilerOpConv2D.c's
// software reference, or a real MLIR-generated .o on a target that has
// one — see CompilerOpConv2D.h's own doc comment). This file only adapts
// the CPU_FALLBACK dispatch protocol to that existing, unrelated contract.
// ============================================================

#include "../loader/NPUModel.h"

#ifdef __cplusplus
extern "C++" {

bool conv2DCPUFallback(const RuntimeOp& op, const NPUModel& model);

} // extern "C++"
#endif
