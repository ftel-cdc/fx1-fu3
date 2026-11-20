#pragma once
// ============================================================
// CompiledKernelCPUFallback — NEON_CPU_FALLBACK_COMPILED_KERNEL (id 16).
// Runs one fragment compiled to native CPU code (NeonCPUFallbackKernelInstr,
// core/NPU_ISA.h): resolves the I/O tensor pointers, checks the RVV kernel
// library fingerprint, looks the entry point up in CompiledKernelRegistry and
// calls it. Self-registers via NEON_REGISTER_CPU_FALLBACK (link with
// --whole-archive, like every other fallback of neon_opfallbacks).
// ============================================================
#include "../loader/NPUModel.h"

bool compiledKernelFallback(const RuntimeOp& op, const NPUModel& model);
