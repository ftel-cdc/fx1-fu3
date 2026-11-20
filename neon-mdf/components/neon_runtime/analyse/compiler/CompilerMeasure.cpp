#include "CompilerMeasure.h"
#include "../../core/NPUComponent.h"

#include <algorithm>
#include <cstdio>

void CompilerMeasure::setupConv2D(const Conv2DShape* shape) {
    conv_shape_ = shape;
}

// Same formula as QEMUMPS3Runtime::simulateInstr()'s CONV2D case
// (device/qemu_mps3_an547/runtime/QEMUMPS3Runtime.cpp) — kept identical so
// FLOPs counted here match FLOPs counted by the ISA-synthesize bench path.
int64_t CompilerMeasure::computeConvFlops(const Conv2DShape* s) const {
    return 2LL * s->batch * s->out_h * s->out_w * s->out_c
              * s->filter_h * s->filter_w * s->in_c;
}

OpBenchResult CompilerMeasure::run(const std::vector<uint64_t>& cpu_cycles,
                                   const std::vector<uint64_t>& npu_cycles) {
    OpBenchResult result{};
    result.op_name = op_name_;

    if (!conv_shape_ || cpu_cycles.empty() || cpu_cycles.size() != npu_cycles.size())
        return result;

    result.flops = computeConvFlops(conv_shape_);

    const auto* npu_component = dynamic_cast<const NPUComponent*>(device_target_);
    const float clock_mhz = npu_component ? npu_component->getNPUSpecs().clock_mhz : 0.f;

    const size_t n = cpu_cycles.size();
    uint64_t sum_cpu_cycles = 0, sum_npu_cycles = 0;
    float    sum_cpu_ms = 0.f, sum_npu_ms = 0.f;
    result.samples.reserve(n);

    for (size_t i = 0; i < n; ++i) {
        float cpu_ms = (clock_mhz > 0.f) ? (float)cpu_cycles[i] / clock_mhz / 1000.f : 0.f;
        float npu_ms = (clock_mhz > 0.f) ? (float)npu_cycles[i] / clock_mhz / 1000.f : 0.f;
        result.samples.push_back({cpu_cycles[i], npu_cycles[i], cpu_ms, npu_ms});
        sum_cpu_cycles += cpu_cycles[i];
        sum_npu_cycles += npu_cycles[i];
        sum_cpu_ms += cpu_ms;
        sum_npu_ms += npu_ms;
    }

    result.avg_cpu_cycles = sum_cpu_cycles / n;
    result.avg_cpu_ms     = sum_cpu_ms / (float)n;
    result.avg_npu_cycles = sum_npu_cycles / n;
    result.avg_npu_ms     = sum_npu_ms / (float)n;

    // actual_gops prefers the real NPU counter (npu_ms) when available —
    // only falls back to cpu_ms (CPU-execution time, not NPU throughput)
    // for targets whose software fallback doesn't dispatch through the
    // NPU at all (see file header).
    const float actual_ms = (result.avg_npu_ms > 0.f) ? result.avg_npu_ms : result.avg_cpu_ms;
    result.actual_gops = (actual_ms > 0.f)
        ? (float)((double)result.flops / (actual_ms / 1000.0) / 1e9)
        : 0.f;

    // ── Roofline ceiling — same methodology + CONV2D efficiency (0.35) as
    // QEMUMPS3Runtime::simulateInstr(), applied against the *target's*
    // declared NPUHWSpecs.
    if (npu_component) {
        const NPUHWSpecs specs = npu_component->getNPUSpecs();
        const double eff = 0.35;   // CONV2D efficiency, matches QEMUMPS3Runtime's table

        int64_t w_bytes  = (int64_t)conv_shape_->filter_h * conv_shape_->filter_w
                          * conv_shape_->in_c * conv_shape_->out_c;
        int64_t io_bytes = (int64_t)conv_shape_->in_h * conv_shape_->in_w * conv_shape_->in_c
                          + (int64_t)conv_shape_->out_h * conv_shape_->out_w * conv_shape_->out_c;
        int64_t mem_bytes = w_bytes + io_bytes;

        double compute_s = (specs.peak_tops > 0.f)
            ? (double)result.flops / (specs.peak_tops * eff * 1e12) : 0.0;
        double mem_s = (specs.dram_bandwidth_gbps > 0.f)
            ? (double)mem_bytes / (specs.dram_bandwidth_gbps * 1e9) : 0.0;
        double theoretical_s = std::max(compute_s, mem_s);

        result.theoretical_gops = (theoretical_s > 0.0)
            ? (float)((double)result.flops / theoretical_s / 1e9) : 0.f;
        result.efficiency_pct = (result.theoretical_gops > 0.f)
            ? result.actual_gops / result.theoretical_gops * 100.f : 0.f;
        result.bound = (mem_s > compute_s) ? BoundType::MEMORY : BoundType::COMPUTE;
    } else {
        result.theoretical_gops = 0.f;
        result.efficiency_pct   = 0.f;
        result.bound            = BoundType::UNKNOWN;
    }

    return result;
}

static void printSep(int width = 100) {
    for (int i = 0; i < width; ++i) putchar('-');
    putchar('\n');
}

// Same banner style as PerfReportPrinter::print() (analyse/PerfReport.cpp)
// for a consistent look across every neon-eval report — this is purely
// cosmetic. The body still only reports what CompilerMeasure actually
// aggregates (CPU/NPU cycles, roofline); it is NOT a ModelPerfReport and
// deliberately doesn't go through PerfReportPrinter, since the fields mean
// different things (see CompilerMeasure.h's header comment) and mixing
// them into the same table as the ISA-synthesize --bench reports would
// imply a comparability that doesn't exist.
void CompilerMeasure::printHeader() const {
    printf("\n");
    printSep();
    printf("  NEON STUDIO — Compiler Performance Report\n");
    printf("  Model:  %s\n", op_name_.c_str());
    printf("  Target: %s\n", device_target_ ? device_target_->getName() : "?");
    printSep();
}

// Each field prints "N/A" independently when its source had no signal —
// never fabricated. theoretical/bound are pure roofline math against the
// target's declared specs (no measurement needed), so they always print.
void CompilerMeasure::printResult(const OpBenchResult& result) const {
    printHeader();

    if (result.avg_cpu_cycles > 0)
        printf("  CPU cycles:   %llu  (%.3f ms)\n",
               (unsigned long long)result.avg_cpu_cycles, result.avg_cpu_ms);
    else
        printf("  CPU cycles:   N/A\n");

    if (result.avg_npu_cycles > 0)
        printf("  NPU cycles:   %llu  (%.3f ms)\n",
               (unsigned long long)result.avg_npu_cycles, result.avg_npu_ms);
    else
        printf("  NPU cycles:   N/A\n");

    printf("  Samples:      %d\n", (int)result.samples.size());
    printf("  FLOPs:        %lld\n", (long long)result.flops);
    printSep();
}

void CompilerMeasure::printComparison(const OpBenchResult& result) const {
    printHeader();

    const char* bound_str = (result.bound == BoundType::COMPUTE) ? "COMPUTE-bound"
                           : (result.bound == BoundType::MEMORY)  ? "MEMORY-bound"
                                                                    : "UNKNOWN";
    // theoretical_ms derived back from theoretical_gops (OpBenchResult stores
    // GOPS, not ms): theoretical_s = flops / (theoretical_gops * 1e9).
    float theoretical_ms = (result.theoretical_gops > 0.f)
        ? (float)((double)result.flops / (result.theoretical_gops * 1e9) * 1000.0)
        : 0.f;

    if (result.avg_cpu_cycles > 0)
        printf("  CPU cycles:   %llu  (%.3f ms)*\n",
               (unsigned long long)result.avg_cpu_cycles, result.avg_cpu_ms);
    else
        printf("  CPU cycles:   N/A\n");

    if (result.avg_npu_cycles > 0)
        printf("  NPU cycles:   %llu  (%.3f ms)\n",
               (unsigned long long)result.avg_npu_cycles, result.avg_npu_ms);
    else
        printf("  NPU cycles:   N/A\n");

    printf("  Theoretical:  %.3f ms\n", theoretical_ms);

    if (result.avg_cpu_cycles == 0 && result.avg_npu_cycles == 0)
        printf("  Efficiency:   N/A\n");
    else
        printf("  Efficiency:   %.0f%%\n", result.efficiency_pct);

    printf("  Bound:        %s\n", bound_str);

    if (result.avg_cpu_cycles > 0 && result.avg_npu_cycles == 0)
        printf("\n  * CPU-cycles-to-ms uses the target's declared clock_mhz —\n"
               "    a logical derivation, not a real elapsed-time measurement\n"
               "    (QEMU's CPU emulation isn't clock-rate-accurate).\n");

    printSep();
}
