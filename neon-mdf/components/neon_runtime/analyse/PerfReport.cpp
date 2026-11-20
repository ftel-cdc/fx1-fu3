#include "PerfReport.h"
#include <cstdio>
#include <cstring>
#include <algorithm>
#include <string>

#include <cstdarg>

extern "C" void uart_puts(const char *s);
extern "C" void uart_putc(char c);

static void uprint(const char* fmt, ...) {
    char buf[512];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    uart_puts(buf);
}

// ============================================================
// helpers
// ============================================================

static const char* opcodeName(NeonOpcode op) {
    switch (op) {
    case NeonOpcode::CONV2D:        return "Conv2D";
    case NeonOpcode::DEPTHWISE_CONV:return "DepthwiseConv";
    case NeonOpcode::RELU:          return "ReLU";
    case NeonOpcode::RELU6:         return "ReLU6";
    case NeonOpcode::SIGMOID:       return "Sigmoid";
    case NeonOpcode::TANH:          return "Tanh";
    case NeonOpcode::MAXPOOL:       return "MaxPool";
    case NeonOpcode::AVGPOOL:       return "AvgPool";
    case NeonOpcode::FC:            return "FC";
    case NeonOpcode::SOFTMAX:       return "Softmax";
    case NeonOpcode::LAYERNORM:     return "LayerNorm";
    case NeonOpcode::GEMM_FUSED:    return "GEMMFused";
    case NeonOpcode::FUSED_ATTN:    return "FusedAttn";
    case NeonOpcode::ELEM_WISE:     return "ElemWise";
    case NeonOpcode::BATCHNORM:     return "BatchNorm";
    case NeonOpcode::ADD:           return "Add";
    case NeonOpcode::MUL:           return "Mul";
    case NeonOpcode::DMA_READ:      return "DMA_RD";
    case NeonOpcode::DMA_WRITE:     return "DMA_WR";
    case NeonOpcode::SET_NSPLIT:    return "SET_NSPLIT";
    case NeonOpcode::CPU_FALLBACK:  return "CPUFallback";
    case NeonOpcode::NOP:           return "NOP";
    case NeonOpcode::HALT:          return "HALT";
    default:                        return "UNKNOWN";
    }
}

static const char* boundStr(BoundType b) {
    switch (b) {
    case BoundType::COMPUTE: return "Compute";
    case BoundType::MEMORY:  return "Memory ";
    default:                 return "Unknown";
    }
}

static void printSep(int width = 100) {
    static const char dash[] = "--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------";
    if (width > (int)sizeof(dash) - 1) width = (int)sizeof(dash) - 1;
    uprint("%.*s\n", width, dash);
}

// Width of the per-layer table: the original 100-column block plus the five
// appended metric columns (TOPS / CYCLES / SA% / OBP% / STALL%), which replaced
// the old ad-hoc "MAC=..  stall=.." suffixes so every row lines up.
static const int kLayerTableWidth = 146;

static std::string fmtInt64(int64_t value) {
    char buf[32];
    char* end = buf + sizeof(buf);
    char* out = end;

    uint64_t magnitude;
    if (value < 0)
        magnitude = static_cast<uint64_t>(-(value + 1)) + 1;
    else
        magnitude = static_cast<uint64_t>(value);

    do {
        *--out = static_cast<char>('0' + magnitude % 10);
        magnitude /= 10;
    } while (magnitude != 0);

    if (value < 0) *--out = '-';
    return std::string(out, end);
}

static std::string fmtBytes(int64_t bytes) {
    char buf[32];
    if (bytes >= 1024 * 1024)
        snprintf(buf, sizeof(buf), "%.1f MB", bytes / (1024.f * 1024.f));
    else if (bytes >= 1024)
        snprintf(buf, sizeof(buf), "%.1f KB", bytes / 1024.f);
    else
        return fmtInt64(bytes) + " B";
    return buf;
}

// ============================================================
// printLayerTable
// ============================================================

void PerfReportPrinter::printLayerTable(const ModelPerfReport& report) {
    uprint("\n  %-4s  %-16s  %-14s  %-9s  %-10s  %-10s  %-8s  %-8s  %-8s"
           "  %-7s  %-11s  %-6s  %-6s  %-7s\n",
           "IDX", "OP", "TYPE", "FLOPS", "EST ms", "ACT ms", "DRAM R", "DRAM W", "BOUND",
           "TOPS", "CYCLES", "SA%", "OBP%", "STALL%");
    printSep(kLayerTableWidth);

    for (const auto& L : report.layers) {
        std::string name = L.op_name.empty() ? "op_" + std::to_string(L.op_index) : L.op_name;
        if (name.size() > 16) name = name.substr(0, 14) + "..";

        // FLOPS human-readable
        char flops_buf[16];
        std::string flops_str;
        if (L.flops >= (int64_t)1e12)
            snprintf(flops_buf, sizeof(flops_buf), "%.1f T", L.flops / 1e12);
        else if (L.flops >= (int64_t)1e9)
            snprintf(flops_buf, sizeof(flops_buf), "%.1f G", L.flops / 1e9);
        else if (L.flops >= (int64_t)1e6)
            snprintf(flops_buf, sizeof(flops_buf), "%.1f M", L.flops / 1e6);
        else
            flops_str = fmtInt64(L.flops);

        if (flops_str.empty()) flops_str = flops_buf;

        std::string op_type = L.runs_on_npu ? opcodeName(L.opcode) : (std::string(opcodeName(L.opcode)) + " [cpu]");

        uprint("  %-4lu  %-16s  %-14s  %-9s  %-10.3f  %-10.3f  %-8s  %-8s  %-8s",
               static_cast<unsigned long>(L.op_index),
               name.c_str(),
               op_type.c_str(),
               flops_str.c_str(),
               L.estimated_ms,
               L.actual_ms,
               fmtBytes(L.dram_read_bytes).c_str(),
               fmtBytes(L.dram_write_bytes).c_str(),
               boundStr(L.bound));

        // Runtime metrics. A dash means "not measured" rather than "zero":
        // ops the driver rejected (e.g. FX1's depth > 992 SRAM staging limit)
        // never execute, so they have no cycle data at all - printing 0.0
        // there would read as a real measurement of zero.
        auto printPct = [](float value, bool measured, int width) {
            if (!measured) uprint("  %-*s", width, "-");
            else           uprint("  %-*.1f", width, value);
        };
        const bool measured = (L.actual_cycles > 0);

        if (measured && L.achieved_tops > 0.f) uprint("  %-7.3f", L.achieved_tops);
        else                                   uprint("  %-7s", "-");
        // Total hardware cycles for this op — the denominator every % column
        // below is taken against.
        if (measured) uprint("  %-11s", fmtInt64((int64_t)L.actual_cycles).c_str());
        else          uprint("  %-11s", "-");
        printPct(L.sa_cycles_pct,      measured, 6);
        printPct(L.obp_cycles_pct,     measured, 6);
        printPct(L.stall_ratio_pct,    measured, 7);
        uprint("\n");
    }
    printSep(kLayerTableWidth);
}

// ============================================================
// printMemorySummary
// ============================================================

void PerfReportPrinter::printMemorySummary(const ModelPerfReport& report) {
    const MemorySummary& m = report.memory;
    uprint("\n  Memory Summary:\n");
    uprint("    Weights:         %s\n",  fmtBytes((int64_t)m.weight_bytes).c_str());
    uprint("    Activations:     %s\n",  fmtBytes((int64_t)m.activation_bytes).c_str());
    uprint("    Peak SRAM:       %s / %s  (%.1f%%)\n",
           fmtBytes((int64_t)m.peak_sram_usage_bytes).c_str(),
           fmtBytes((int64_t)m.sram_capacity_bytes).c_str(),
           m.sram_utilization_pct);
    uprint("    Total DRAM R:    %s\n",  fmtBytes((int64_t)m.total_dram_read_bytes).c_str());
    uprint("    Total DRAM W:    %s\n",  fmtBytes((int64_t)m.total_dram_write_bytes).c_str());
    uprint("    Peak DRAM BW:    %.2f GB/s  (%.1f%% util)\n",
           m.peak_dram_bw_gbps, m.peak_dram_util_pct);
    uprint("    Avg  DRAM BW:    %.2f GB/s\n", m.avg_dram_bw_gbps);
}

// ============================================================
// printWarnings
// ============================================================

void PerfReportPrinter::printWarnings(const ModelPerfReport& report,
                                       float bw_warn_threshold_pct) {
    std::vector<uint32_t> bw_warned;
    std::vector<uint32_t> stall_warned;

    // A DMA-bottleneck claim requires observed traffic. When the byte counts are
    // static shape estimates (bench runs reuse one tile buffer instead of moving
    // whole tensors), estimate ÷ measured-time routinely exceeds peak bandwidth,
    // so warning here would assert an impossible measurement. Say so once
    // instead. Stall-ratio warnings below are unaffected — those are pure
    // hardware counters.
    if (!report.dram_bytes_are_measured) {
        bool any_over = false;
        for (const auto& L : report.layers)
            if (L.actual_dram_util_pct > bw_warn_threshold_pct) any_over = true;
        if (any_over)
            uprint("  [NOTE] DRAM-bottleneck warnings suppressed: DRAM byte counts"
                   " are static estimates, not observed traffic, so utilization"
                   " vs measured time is not a valid measurement here.\n");
    }

    for (const auto& L : report.layers) {
        if (report.dram_bytes_are_measured &&
            L.actual_dram_util_pct > bw_warn_threshold_pct &&
            std::find(bw_warned.begin(), bw_warned.end(), L.op_index) ==
                bw_warned.end()) {
            uprint("  [WARN] Op %lu (%s): DRAM util %.1f%% > %.0f%% threshold — DMA bottleneck\n",
                   static_cast<unsigned long>(L.op_index),
                   opcodeName(L.opcode),
                   L.actual_dram_util_pct,
                   bw_warn_threshold_pct);
            bw_warned.push_back(L.op_index);
        }
        if (L.stall_ratio_pct > 50.f &&
            std::find(stall_warned.begin(), stall_warned.end(), L.op_index) ==
                stall_warned.end()) {
            uprint("  [WARN] Op %lu (%s): stall ratio %.1f%% — SA waiting for DMA (memory-bound)\n",
                   static_cast<unsigned long>(L.op_index),
                   opcodeName(L.opcode),
                   L.stall_ratio_pct);
            stall_warned.push_back(L.op_index);
        }
    }
}

// ============================================================
// print (full single-model report)
// ============================================================

void PerfReportPrinter::print(const ModelPerfReport& report) {
    uprint("\n");
    printSep(kLayerTableWidth);
    uprint("  NEON STUDIO — Performance Report\n");
    uprint("  Model:  %s\n", report.model_name.c_str());
    uprint("  Target: %s", report.target_device.c_str());
    if (report.lane != 255)
        uprint("  Lane %u  (rows %u / %u)", report.lane, report.lane_rows, report.total_rows);
    uprint("\n");
    // Hardware constant, so it belongs here rather than repeated per row;
    // the per-layer TOPS column is measured throughput against this ceiling.
    if (report.peak_tops > 0.f)
        uprint("  Peak:   %.4f TOPS (INT8, estimated)\n", report.peak_tops);
    if (report.batch_size > 1)
        uprint("  Batch:  %lu items\n",
               static_cast<unsigned long>(report.batch_size));
    printSep(kLayerTableWidth);

    if (report.has_static_data || report.has_runtime_data)
        printLayerTable(report);

    uprint("\n  Totals:\n");
    if (report.has_static_data)
        uprint("    Estimated:  %.3f ms  (%.1f items/sec)\n",
               report.total_estimated_ms, report.estimated_fps);
    if (report.has_runtime_data) {
        uprint("    Actual:     %.3f ms  (%.1f items/sec)\n",
               report.total_actual_ms, report.actual_fps);

        // Say where the number came from. A host wall-clock reading looks
        // identical to a hardware cycle measurement in this table, and on a
        // target whose cycle counter reads zero the fallback silently reports
        // how long the HOST spent dispatching — a number several orders of
        // magnitude away from the device's own, with nothing to distinguish
        // it. MeasuredProfiler has recorded the source all along; it was
        // simply never printed.
        size_t hw = 0, host = 0;
        for (const auto& l : report.layers) {
            if (l.timing_src == LayerPerfRecord::TimingSource::HW_CYCLE_COUNTER) ++hw;
            else if (l.timing_src == LayerPerfRecord::TimingSource::HOST_CLOCK)  ++host;
        }
        if (host > 0 && hw == 0)
            uprint("    Timing source:  HOST CLOCK — the device reported no cycle\n"
                   "                    counter, so this measures host dispatch time,\n"
                   "                    NOT device execution.\n");
        else if (hw > 0 && host == 0)
            uprint("    Timing source:  device cycle counter\n");
        else if (hw > 0 && host > 0)
            uprint("    Timing source:  MIXED — %zu op(s) device, %zu host. The\n"
                   "                    total is not one consistent measurement.\n", hw, host);
    }


    // Compare only the ops that actually ran against THEIR estimate; using the
    // all-ops estimate here silently mixes op sets and can invert the verdict
    // (see ModelPerfReport::estimated_ms_measured_subset).
    if (report.has_runtime_data && report.has_static_data
        && report.estimated_ms_measured_subset > 0.f) {
        float ratio = report.total_actual_ms / report.estimated_ms_measured_subset;
        uprint("    Roofline accuracy:  %.1f%% (actual/estimate over the %u of %u"
               " ops measured; est %.3f ms for those)\n",
               ratio * 100.f,
               report.measured_op_count, report.total_op_count,
               report.estimated_ms_measured_subset);
        if (report.measured_op_count < report.total_op_count)
            uprint("    Note: %u op(s) were not executed (see '-' rows above);"
                   " 'Estimated' above still covers all %u.\n",
                   report.total_op_count - report.measured_op_count,
                   report.total_op_count);
    }

    if (report.memory.sram_capacity_bytes > 0)
        printMemorySummary(report);

    printSep(kLayerTableWidth);
}

// ============================================================
// printConcurrentSummary
// ============================================================

void PerfReportPrinter::printConcurrentSummary(const ConcurrentPerfSummary& s) {
    uprint("\n  Concurrent Execution Summary:\n");
    uprint("    N_splits:       {");
    for (size_t i = 0; i < s.n_splits.size(); ++i)
        uprint("%s%u", i ? ", " : "", s.n_splits[i]);
    uprint("}\n");
    for (size_t i = 0; i < s.lane_latency_ms.size(); ++i)
        uprint("    Lane %-2lu:       %.3f ms\n",
               static_cast<unsigned long>(i), s.lane_latency_ms[i]);
    uprint("    Wall time:      %.3f ms\n", s.wall_time_ms);
    uprint("    Serial time:    %.3f ms\n", s.serial_time_ms);
    uprint("    Speedup:        %.2f×\n",  s.speedup_vs_serial);
    if (!s.hw_parallel_supported)
        uprint("    [WARN] Hardware serializes — no true concurrent execution\n");
}

// ============================================================
// printMultiLane
// ============================================================

void PerfReportPrinter::printMultiLane(const MultiLanePerfReport& report) {
    uprint("\n");
    printSep();
    uprint("  NEON STUDIO — Multi-Lane Performance Report  (%lu lanes)\n",
           static_cast<unsigned long>(report.numLanes()));
    printSep();

    // Side-by-side header
    uprint("  %-20s", "");
    for (size_t i = 0; i < report.lanes.size(); ++i) {
        const auto& l = report.lanes[i];
        uprint("  %-20s", ("Lane " + std::to_string(i) + " (" + l.model_name + ")").c_str());
    }
    uprint("\n");
    printSep();

    auto printRow = [&](const char* label, auto fn) {
        uprint("  %-20s", label);
        for (const auto& l : report.lanes) uprint("  %-20s", fn(l).c_str());
        uprint("\n");
    };

    printRow("estimated_ms", [](const ModelPerfReport& l) {
        char buf[32]; snprintf(buf, sizeof(buf), "%.3f", l.total_estimated_ms); return std::string(buf);
    });
    printRow("actual_ms", [](const ModelPerfReport& l) {
        char buf[32]; snprintf(buf, sizeof(buf), "%.3f", l.total_actual_ms); return std::string(buf);
    });
    printRow("actual_fps", [](const ModelPerfReport& l) {
        char buf[32]; snprintf(buf, sizeof(buf), "%.1f", l.actual_fps); return std::string(buf);
    });

    printSep();
    printConcurrentSummary(report.summary);
    printSep();
}

// ============================================================
// printMultiLaneLayerTables
// ============================================================

void PerfReportPrinter::printMultiLaneLayerTables(const MultiLanePerfReport& report) {
    for (size_t i = 0; i < report.lanes.size(); ++i) {
        uprint("\n  === Lane %lu: %s ===\n",
               static_cast<unsigned long>(i), report.lanes[i].model_name.c_str());
        printLayerTable(report.lanes[i]);
    }
}
