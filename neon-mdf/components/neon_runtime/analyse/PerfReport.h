#pragma once

// ============================================================
// NEON STUDIO — PERF REPORT
// Common data structures shared across all analysers.
// Both static and runtime profilers populate these structs;
// the report generator consumes them.
// ============================================================

#include "../core/NPU_ISA.h"
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C++" {

#include <string>
#include <vector>

// ============================================================
// BOUND TYPE — result of roofline classification
// ============================================================

enum class BoundType {
    COMPUTE,        // arithmetic intensity > ridge point
    MEMORY,         // arithmetic intensity <= ridge point
    UNKNOWN,
};

// ============================================================
// PER-LAYER REPORT
// ============================================================

struct LayerPerfRecord {
    std::string  op_name;
    NeonOpcode   opcode;
    uint32_t     op_index;          // position in execution order

    // --- Static estimates ---
    int64_t      flops;             // total floating/integer ops
    int64_t      dram_read_bytes;   // estimated bytes read from DRAM
    int64_t      dram_write_bytes;  // estimated bytes written to DRAM
    int64_t      sram_read_bytes;   // estimated bytes read from SRAM
    int64_t      sram_write_bytes;
    float        arithmetic_intensity; // flops / (dram_read + dram_write)
    float        estimated_ms;      // roofline estimate
    BoundType    bound;             // COMPUTE or MEMORY

    // false = executed on the host core via runtime/opfallbacks/, so the
    // roofline estimate below models hardware this op never touched.
    bool         runs_on_npu = true;

    // --- Runtime measurements ---

    // Timing source tells us how latency_ms was measured.
    // HOST_CLOCK     = std::chrono (fallback, unreliable on TLM)
    // HW_CYCLE_COUNTER = MMIO perf counter (ground-truth on TLM/FPGA/silicon)
    enum class TimingSource : uint8_t {
        NONE,              // not profiled
        HOST_CLOCK,        // std::chrono::steady_clock (host wall-clock)
        HW_CYCLE_COUNTER,  // npu_driver->readPerfTotalCycles() delta
    };

    TimingSource timing_src      = TimingSource::NONE;
    float        actual_ms       = 0.f;   // 0 if not profiled
    uint64_t     actual_cycles   = 0;     // raw cycle delta (0 if not measured)
    uint64_t     stall_cycles    = 0;     // pipeline stall cycles for this op

    // MAC efficiency — how effectively the SA was used.
    // actual_flops_per_cycle = theoretical_flops / actual_cycles
    // peak_flops_per_cycle   = lane_rows × sa.cols × 2
    // mac_efficiency_pct     = actual / peak × 100
    // Requires both shapes (for theoretical_flops) AND hw cycle counter.
    float        actual_flops_per_cycle = 0.f;
    float        peak_flops_per_cycle   = 0.f;
    float        mac_efficiency_pct     = 0.f;  // 0 if not computable

    // Stall ratio = stall_cycles / actual_cycles × 100
    // High value → DMA is the bottleneck (SA waiting for data)
    float        stall_ratio_pct        = 0.f;

    // Hardware time-split for this op, each as % of actual_cycles:
    //   sa_cycles_pct  = systolic array actively computing
    //   obp_cycles_pct = output boundary pipeline active (bias/requant/LUT stage)
    // These overlap in time (OBP drains while the SA still computes) and so do
    // NOT sum to 100% with each other or with stall_ratio_pct. 0 if the driver
    // exposes no such counter (see NPUDriver::readPerfSACycles()).
    //
    // No dma_cycles_pct: the FX1 model reports no DMA-cycle counter at all
    // (hardcoded 0 in the bridge) — stall_ratio_pct is the DMA-wait proxy.
    float        sa_cycles_pct          = 0.f;
    float        obp_cycles_pct         = 0.f;

    // V4.5 engine-activity split, same "% of actual_cycles, may overlap"
    // semantics as sa_cycles_pct/obp_cycles_pct above. 0 if the driver
    // exposes no such counter.
    float        mac_engine_cycles_pct  = 0.f;
    float        dma_engine_cycles_pct  = 0.f;
    float        act_engine_cycles_pct  = 0.f;
    float        pool_engine_cycles_pct = 0.f;
    float        red_engine_cycles_pct  = 0.f;

    // Hardware-reported rich-op M/K/N. Compare against the compiler's
    // declared OpShape (input_h/w/c, channels_out, etc.) as a correctness
    // cross-check, not a performance number. 0 if not a rich op.
    uint64_t     hw_reported_m = 0, hw_reported_k = 0, hw_reported_n = 0;

    // Achieved throughput = flops / actual_ms, in TOPS. Compare against
    // ModelPerfReport::peak_tops for the roofline fraction (this equals
    // mac_efficiency_pct/100 * peak_tops when both are computable).
    float        achieved_tops          = 0.f;

    float        actual_dram_read_gbps  = 0.f;
    float        actual_dram_write_gbps = 0.f;
    float        actual_dram_util_pct   = 0.f;  // % of peak DRAM BW

    // --- Reuse / pattern ---
    int64_t      weight_reuse_factor;   // how many times weight is reused
    bool         weight_fits_sram;
    bool         is_sequential_access;  // false = strided
};

// ============================================================
// MEMORY SUMMARY
// ============================================================

struct MemorySummary {
    size_t  weight_bytes;               // static, loaded once
    size_t  activation_bytes;           // dynamic scratch
    size_t  peak_sram_usage_bytes;
    size_t  sram_capacity_bytes;        // budget for this lane (may be < total)
    float   sram_utilization_pct;

    size_t  total_dram_read_bytes;      // summed across all ops
    size_t  total_dram_write_bytes;
    float   peak_dram_bw_gbps;          // peak observed/estimated
    float   avg_dram_bw_gbps;
    float   peak_dram_util_pct;
};

// ============================================================
// FULL MODEL REPORT (single model / single lane)
// ============================================================

struct ModelPerfReport {
    std::string  model_name;
    std::string  target_device;
    NeonDtype    dtype;

    // Device peak INT8 throughput from NPUHWSpecs::peak_tops — a hardware
    // constant (identical for every layer), so it is reported once in the
    // report header rather than per row. Per-layer achieved throughput is
    // LayerPerfRecord::achieved_tops.
    float        peak_tops = 0.f;

    // Roofline accuracy must compare like with like. total_estimated_ms covers
    // EVERY op (static analysis always runs), but total_actual_ms only covers
    // ops that actually executed — a driver may reject some (e.g. FX1 refuses
    // depth > 992), contributing 0 ms. Dividing the two directly mixes op sets
    // and can invert the conclusion entirely (observed: 6-of-13 ops measured
    // gave "37.9% accuracy", i.e. seemingly 3x faster than predicted, when the
    // like-for-like figure was 124% — hardware ~24% SLOWER than the estimate).
    // So keep the estimate restricted to the measured subset, plus the counts
    // needed to disclose coverage.
    float        estimated_ms_measured_subset = 0.f;
    uint32_t     measured_op_count = 0;
    uint32_t     total_op_count    = 0;

    // Whether dram_read_bytes/dram_write_bytes (and the *_gbps / util_pct
    // figures derived from them) come from observed DMA traffic or from static
    // shape estimates. On bench runs SyntheticExecutor supplies shape-derived
    // full-tensor sizes while the harness actually reuses one small tile
    // buffer, so estimate ÷ measured-time can exceed peak bandwidth (observed:
    // 117% DRAM util). printWarnings() must not assert a DMA bottleneck from
    // numbers that were never measured.
    bool         dram_bytes_are_measured = false;

    // Which lane this model ran on (255 = single-model, not split)
    uint8_t      lane;
    uint8_t      lane_rows;         // SA rows assigned to this lane
    uint8_t      total_rows;        // total SA rows (e.g. 64 on FX1)

    // Batch size used for this profile run (Rev3).
    // 1  = single-item inference (default, batch=1 stream)
    // N  = batch inference via run_batch() with a NeonBatchProfileEntry
    //
    // Impact on reported metrics:
    //   total_actual_ms  = wall time for ONE batch execution (N items)
    //   actual_fps       = batch_size * (1000.f / total_actual_ms)   — items/sec
    //   estimated_fps    = batch_size * (1000.f / total_estimated_ms) — items/sec
    //
    // When batch_size == 1 (default), formulas collapse to the original:
    //   actual_fps = 1000.f / total_actual_ms
    uint32_t     batch_size = 1;

    // Per-layer records
    std::vector<LayerPerfRecord> layers;

    // Aggregates
    float   total_estimated_ms;
    float   total_actual_ms;        // 0 if not profiled
    float   estimated_fps;          // items/sec = batch_size / total_estimated_ms * 1000
    float   actual_fps;             // items/sec = batch_size / total_actual_ms   * 1000

    // Memory
    MemorySummary memory;

    // Flags
    bool    has_runtime_data;       // true if runtime profiler ran
    bool    has_static_data;        // true if static analyser ran
};

// ============================================================
// CONCURRENT PERF SUMMARY
// Aggregated view across all lanes in a multi-lane run.
// ============================================================

struct ConcurrentPerfSummary {
    // N_split boundary points: size = num_lanes - 1
    // For 2 lanes: {40}         → rows 0..39 | 40..63
    // For 4 lanes: {16, 32, 48} → 4 equal splits
    std::vector<uint8_t> n_splits;

    // Per-lane latency and MAC utilisation
    // lane_latency_ms[i]  = estimated or actual latency for lane i
    // lane_util_pct[i]    = MAC utilisation for lane i (effective TOPS / peak_tops_lane)
    std::vector<float>   lane_latency_ms;
    std::vector<float>   lane_util_pct;

    // Overall
    float wall_time_ms;             // = max(lane_latency_ms)
    float serial_time_ms;           // = sum(lane_latency_ms) — baseline for speedup
    float speedup_vs_serial;        // serial_time_ms / wall_time_ms

    // Hardware validation
    // true if num_lanes <= target.max_concurrent_models (hardware can run in parallel)
    // false → will serialize (wall_time = serial_time, speedup = 1.0)
    bool  hw_parallel_supported;
};

// ============================================================
// MULTI-LANE PERF REPORT
// Combines N ModelPerfReports (one per lane) with concurrent summary.
// Replaces the earlier DualModelPerfReport concept.
// ============================================================

struct MultiLanePerfReport {
    // Per-lane report — lanes[i] corresponds to model i
    // lane_rows[i] = SA rows allocated to lane i
    // sum(lane_rows) == total_rows (no gaps)
    std::vector<ModelPerfReport> lanes;

    // Concurrent execution summary
    ConcurrentPerfSummary summary;

    // Convenience accessors
    size_t numLanes()    const { return lanes.size(); }
    bool   hasRuntime()  const {
        for (const auto& l : lanes) if (l.has_runtime_data) return true;
        return false;
    }
};

// ============================================================
// REPORT PRINTER
// ============================================================

class PerfReportPrinter {
public:
    // ── Single-model ──────────────────────────────────────────

    // Print full report to stdout
    static void print(const ModelPerfReport& report);

    // Print only layer table
    static void printLayerTable(const ModelPerfReport& report);

    // Print only memory summary
    static void printMemorySummary(const ModelPerfReport& report);

    // Print bottleneck warnings (BW > threshold)
    static void printWarnings(const ModelPerfReport& report,
                              float bw_warn_threshold_pct = 80.f);

    // ── Multi-lane ────────────────────────────────────────────

    // Print side-by-side lane comparison table
    //
    // Example output:
    //   ┌──────────────┬───────────────┬───────────────┐
    //   │              │  Lane 0 (A)   │  Lane 1 (B)   │
    //   │              │  vit_b.nsm    │  yolov8m.nsm  │
    //   │              │  rows 0..39   │  rows 40..63  │
    //   ├──────────────┼───────────────┼───────────────┤
    //   │ estimated_ms │     12.4      │      8.7      │
    //   │ actual_ms    │     13.1      │      9.2      │
    //   │ MAC util     │     78.3 %    │     61.4 %    │
    //   │ DRAM util    │     54.2 %    │     42.1 %    │
    //   ├──────────────┼───────────────┴───────────────┤
    //   │ wall_time    │         13.1 ms               │
    //   │ serial_time  │         22.3 ms               │
    //   │ speedup      │         1.70×                 │
    //   └──────────────┴───────────────────────────────┘
    static void printMultiLane(const MultiLanePerfReport& report);

    // Print concurrent summary only (no per-layer breakdown)
    static void printConcurrentSummary(const ConcurrentPerfSummary& summary);

    // Print per-lane layer tables sequentially
    static void printMultiLaneLayerTables(const MultiLanePerfReport& report);
};

} // extern "C++"
#endif
