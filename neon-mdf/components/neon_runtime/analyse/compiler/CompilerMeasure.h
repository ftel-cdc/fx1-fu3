#pragma once

// ============================================================
// NEON STUDIO — COMPILER MEASURE
// Aggregates already-measured per-call cycle counts for a compiler-
// generated op kernel into avg/min/max/std_dev + a roofline comparison.
//
// Device-agnostic by design, same reasoning as MeasuredProfiler.h: this
// class never touches NPUDriver or any device-specific timing mechanism —
// analyse/ may only depend on core/ (see CLAUDE.md's dependency layers;
// runtime/ already depends on analyse/, so the reverse would be circular).
// It has zero knowledge of QEMU, TLM, FPGA, or any other target.
//
// Two independent, separately-reported cycle sources — a real device may
// only provide one, the other, both, or neither:
//   cpu_cycles : caller measured via NPUDriver::readCPUCycles() — a CPU/
//                core cycle counter (e.g. SysTick on qemu_mps3_an547).
//                Meaningful as a *relative* measurement; converting to ms
//                via the target's declared clock_mhz is a logical
//                derivation ("if this ran at the declared clock"), not a
//                claim that it matches real elapsed wall-clock time
//                (QEMU's CPU emulation isn't clock-rate-accurate).
//   npu_cycles : caller measured via NPUDriver::readPerfTotalCycles() — the
//                NPU's own hardware performance counter. Real and
//                meaningful wherever available (e.g. VP_FX1_V2.0's
//                NPU perf counters); reads 0 on targets whose
//                software fallback doesn't dispatch through the NPU at
//                all (qemu_mps3_an547 — conv2D_Func() bypasses NPUDriver).
// Either can be all-zero, meaning that source had no signal — printed as
// N/A, never fabricated.
//
// Usage:
//   CompilerMeasure bench("conv2d", target);
//   bench.setupConv2D(&shape);
//   auto result = bench.run(cpu_cycles, npu_cycles);   // caller-measured
//   bench.printComparison(result);
// ============================================================

#include "CompilerOpConv2D.h"
#include "../PerfReport.h"
#include "../../core/DeviceTarget.h"

#ifdef __cplusplus
extern "C++" {

#include <string>
#include <vector>

// ============================================================
// TIMING SAMPLE
// ============================================================

struct BenchSample {
    uint64_t cpu_cycles;
    uint64_t npu_cycles;
    float    cpu_ms;
    float    npu_ms;
};

// ============================================================
// OP BENCHMARK RESULT
// ============================================================

struct OpBenchResult {
    std::string op_name;

    // CPU cycle counter (e.g. SysTick) — see file header.
    uint64_t avg_cpu_cycles;
    float    avg_cpu_ms;

    // NPU hardware performance counter — see file header.
    uint64_t avg_npu_cycles;
    float    avg_npu_ms;

    // Throughput / roofline — actual_gops prefers npu_ms (real HW
    // measurement) when available, falling back to cpu_ms otherwise.
    float actual_gops;
    float theoretical_gops;     // từ roofline (target's declared NPUHWSpecs)
    float efficiency_pct;       // actual / theoretical * 100

    // Tiling info
    int64_t flops;              // tổng FLOPs của op
    BoundType bound;            // COMPUTE hay MEMORY bound

    // Samples raw
    std::vector<BenchSample> samples;
};

// ============================================================
// COMPILER MEASURE
// ============================================================

class CompilerMeasure {
public:
    CompilerMeasure(const std::string&   op_name,
                    const DeviceTarget* device_target)
        : op_name_(op_name), device_target_(device_target) {}

    // --- Describe per op type ---
    // Only needs the shape (for FLOPs/roofline) — buffers/quant params are
    // the caller's concern, since the caller is the one invoking conv2D_Func().
    void setupConv2D(const Conv2DShape* shape);

    // TODO: setupDepthwiseConv(), setupFC(), setupRelu()... khi có contract tương ứng

    // --- Aggregate ---
    // cpu_cycles/npu_cycles: caller-measured per-call raw cycle counts,
    // same length, index i = same call. All-zero in either vector is
    // valid — means that source had no signal for these calls.
    OpBenchResult run(const std::vector<uint64_t>& cpu_cycles,
                      const std::vector<uint64_t>& npu_cycles);

    // --- Report ---

    void printResult(const OpBenchResult& result) const;

    // So sánh actual vs roofline estimate
    void printComparison(const OpBenchResult& result) const;

private:
    std::string          op_name_;
    const DeviceTarget*   device_target_;
    const Conv2DShape*    conv_shape_ = nullptr;

    // Compute theoretical FLOPs từ Conv2DShape
    int64_t computeConvFlops(const Conv2DShape* s) const;

    // Shared banner — same style as PerfReportPrinter::print(), purely
    // cosmetic (see printComparison()'s comment for why this doesn't
    // reuse PerfReportPrinter itself).
    void printHeader() const;
};

} // extern "C++"
#endif
