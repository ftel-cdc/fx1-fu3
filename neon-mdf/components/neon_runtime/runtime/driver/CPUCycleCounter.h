#pragma once

// ============================================================
// NEON STUDIO — CPU CYCLE COUNTER
//
// Minimal interface for reading a raw CPU/core cycle counter — for timing
// code that bypasses NPUDriver entirely (e.g. a linked compiler kernel
// called directly, not dispatched through writeInstruction()/trigger()/
// waitDone() — see analyse/compiler/CompilerOpConv2D.h's conv2D_Func()
// and apps/tools/neon-eval/main.cpp's runCompilerBench()).
//
// Deliberately NOT part of NPUDriver: a CPU cycle count has nothing to do
// with the NPU itself — it's a property of the CPU core the driver's host
// software happens to run on. Same composition pattern as DeviceTarget/
// NPUComponent/NPUDriver — an orthogonal concern, attached via multiple
// inheritance on whichever concrete Runtime class can actually provide it
// (e.g. QEMUMPS3Runtime : public QEMUMPS3NPUTarget, public NPUDriver,
// public CPUCycleCounter). Callers that have an NPUDriver* but need this
// too should dynamic_cast<CPUCycleCounter*> it, same as the
// DeviceTarget*/NPUComponent* dual-pointer pattern used elsewhere.
//
// Returns 0 if this target provides no CPU cycle counter.
// ============================================================

#include <stdint.h>

#ifdef __cplusplus
extern "C++" {

class CPUCycleCounter {
public:
    virtual ~CPUCycleCounter() = default;
    virtual uint64_t readCPUCycles() const { return 0; }
};

} // extern "C++"
#endif
