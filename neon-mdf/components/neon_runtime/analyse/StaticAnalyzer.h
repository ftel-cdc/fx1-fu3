#pragma once

// ============================================================
// NEON STUDIO — STATIC ANALYSER
// Estimates performance from compiled graph — no hardware needed.
// Uses roofline model: bottleneck = max(compute_time, memory_time).
//
// ── Single-model usage ────────────────────────────────────────
//   StaticAnalyzer analyser(target);
//   auto report = analyser.analyse(ops, "model.nsm");
//   PerfReportPrinter::print(report);
//
// ── Multi-lane usage (--lanes N) ──────────────────────────────
//   // Auto-compute optimal split points
//   auto splits = analyser.findOptimalSplits({ops_a, ops_b});
//
//   // Analyse all lanes with the chosen splits
//   auto ml_report = analyser.analyseMultiLane(
//       {ops_a, ops_b}, {"vit_b.nsm", "yolov8m.nsm"}, splits);
//
//   PerfReportPrinter::printMultiLane(ml_report);
// ============================================================

#include "PerfReport.h"
#include "../core/NPUComponent.h"
#include "../core/DeviceTarget.h"
#include "../core/NPU_ISA.h"

#ifdef __cplusplus
extern "C++" {

// ============================================================
// OP SHAPE — describes tensor dimensions for one op
// Populated by toolkit before calling analyser
// ============================================================

struct OpShape {
    uint32_t   op_index;
    NeonOpcode opcode;
    std::string op_name;

    // Input tensor
    uint32_t input_n, input_h, input_w, input_c;

    // Weight / kernel (conv only)
    uint32_t kernel_h, kernel_w;
    uint32_t channels_out;
    uint32_t depth_multiplier;      // depthwise only

    // Stride / pad
    uint32_t stride_h, stride_w;
    uint32_t pad_h, pad_w;

    // FC
    uint32_t input_size, output_size;

    // General
    uint32_t  element_count;        // for elementwise / activation ops
    NeonDtype dtype;

    // Fused epilogue (Rev4, CONV2D/FC only) — see NeonConv2DInstr's comment
    // in core/NPU_ISA.h. Default 0/0 reproduces pre-Rev4 behavior.
    uint8_t   act_mode = 0;         // 0=None, 1=ReLU, 2=SiLU, 3=GELU
    uint8_t   has_skip = 0;         // 0/1 — fused residual add
};

// ============================================================
// STATIC ANALYSER
// ============================================================

class StaticAnalyzer {
public:
    StaticAnalyzer() = default;
    StaticAnalyzer(const DeviceTarget* device_target, const NPUComponent* npu_component)
        : device_target_(device_target), npu_component_(npu_component) {}

    // ── Single-model (full SA, N_split=max) ──────────────────

    // Analyse a list of op shapes, return full report.
    // Uses full SA (lane_rows = total_rows = sa.rows).
    ModelPerfReport analyse(const std::vector<OpShape>& ops,
                            const std::string& model_name = "") const;

    // Analyse a single op — useful for per-op breakdown
    LayerPerfRecord analyseOp(const OpShape& op) const;

    // ── Lane-aware single model ───────────────────────────────

    // Analyse one model assigned to a specific SA lane partition.
    // lane_rows : SA rows allocated to this lane (e.g. 40 for Lane A with N_split=40)
    // total_rows: total SA rows of the device (e.g. 64 on FX1)
    // lane      : lane index (0-based), stored in the returned report
    //
    // effective peak_tops = peak_tops * lane_rows / total_rows
    // effective sram_budget = sram_size_bytes * lane_rows / total_rows  (approx)
    ModelPerfReport analyseForLane(const std::vector<OpShape>& ops,
                                   const std::string& model_name,
                                   uint8_t lane_rows,
                                   uint8_t total_rows,
                                   uint8_t lane) const;

    // ── Multi-lane ────────────────────────────────────────────

    // Analyse N models across N lanes in one call.
    //
    // all_ops[i]     : op shapes for model i
    // model_names[i] : name for model i (used in report)
    // n_splits       : N-1 boundary points (row indices), size = all_ops.size() - 1
    //                  Lane i gets rows: sum(n_splits[0..i-1]) .. n_splits[i] - 1
    //
    // Example for 2 lanes, n_splits={40}:
    //   Lane 0 → rows  0..39  (40 rows)
    //   Lane 1 → rows 40..63  (24 rows)
    //
    // Example for 4 lanes, n_splits={16,32,48}:
    //   Lane 0 → rows  0..15  (16 rows)
    //   Lane 1 → rows 16..31  (16 rows)
    //   Lane 2 → rows 32..47  (16 rows)
    //   Lane 3 → rows 48..63  (16 rows)
    MultiLanePerfReport analyseMultiLane(
        const std::vector<std::vector<OpShape>>& all_ops,
        const std::vector<std::string>&          model_names,
        const std::vector<uint8_t>&              n_splits) const;

    // ── Optimal split search ──────────────────────────────────

    // Result of findOptimalSplits()
    struct SplitSearchResult {
        std::vector<uint8_t> n_splits;         // N-1 boundary points
        std::vector<float>   lane_latency_ms;  // per-lane latency at optimal split
        float                wall_time_ms;     // max(lane_latency_ms)
        float                serial_time_ms;   // sum(lane_latency_ms)
        float                speedup;          // serial / wall
    };

    // Brute-force search for split points that minimise wall_time_ms.
    //
    // For N=2 lanes: scans all 1..total_rows-1 (63 candidates on FX1).
    // For N>2 lanes: scans all combinations — complexity O(total_rows^(N-1)).
    //   Practical only for N≤4 on typical SA sizes (64 rows → 63^3 = 250K iters).
    //
    // Objective: minimise max(latency_lane_i) across all lanes.
    //
    // all_ops.size() must equal num_lanes.
    SplitSearchResult findOptimalSplits(
        const std::vector<std::vector<OpShape>>& all_ops) const;

    // Quick 2-lane split search (most common case, no combinatorial explosion)
    SplitSearchResult findOptimalSplits2(
        const std::vector<OpShape>& ops_a,
        const std::vector<OpShape>& ops_b) const;

private:
    const DeviceTarget* device_target_;
    const NPUComponent* npu_component_;

    // ── FLOPs calculators ────────────────────────────────────
    int64_t flopsConv2D(const OpShape& op) const;
    int64_t flopsDepthwiseConv(const OpShape& op) const;
    int64_t flopsFC(const OpShape& op) const;
    int64_t flopsActivation(const OpShape& op) const;
    int64_t flopsPool(const OpShape& op) const;
    int64_t flopsElementwise(const OpShape& op) const;

    // ── Memory bytes calculators ─────────────────────────────
    // Returns {dram_read, dram_write, sram_read, sram_write}
    struct MemBytes { int64_t dram_r, dram_w, sram_r, sram_w; };
    MemBytes memBytesConv2D(const OpShape& op) const;
    MemBytes memBytesDepthwiseConv(const OpShape& op) const;
    MemBytes memBytesFC(const OpShape& op) const;
    MemBytes memBytesActivation(const OpShape& op) const;
    MemBytes memBytesPool(const OpShape& op) const;
    MemBytes memBytesElementwise(const OpShape& op) const;

    // ── Roofline ─────────────────────────────────────────────
    // effective_tops : scaled peak_tops for the lane (full or partial SA)
    // effective_sram : sram budget for the lane
    float rooflineEstimateMs(int64_t flops, int64_t mem_bytes,
                             float effective_tops,
                             size_t effective_sram_bytes) const;

    int64_t rooflineEstimateCycles(int64_t flops, int64_t mem_bytes,
                                   float effective_tops,
                                   size_t effective_sram_bytes,
                                   float clock_mhz = 500.0f) const;

    BoundType classifyBound(float arithmetic_intensity) const;

    // ── Helpers ──────────────────────────────────────────────
    int    dtypeBytes(NeonDtype dtype) const;
    bool   weightFitsSRAM(int64_t weight_bytes,
                          size_t sram_budget_bytes) const;

    // Compute row counts from n_splits vector
    // Returns vector of lane_rows[i] for each lane
    std::vector<uint8_t> laneRowsFromSplits(
        const std::vector<uint8_t>& n_splits,
        uint8_t total_rows) const;

    // Internal: analyse one model given explicit effective_tops + sram budget
    ModelPerfReport analyseInternal(const std::vector<OpShape>& ops,
                                    const std::string& model_name,
                                    float effective_tops,
                                    size_t effective_sram_bytes,
                                    uint8_t lane,
                                    uint8_t lane_rows,
                                    uint8_t total_rows) const;

    // Estimate latency for one model given lane_rows (used by findOptimalSplits)
    float estimateLaneLatencyMs(const std::vector<OpShape>& ops,
                                uint8_t lane_rows,
                                uint8_t total_rows) const;
};

} // extern "C++"
#endif
