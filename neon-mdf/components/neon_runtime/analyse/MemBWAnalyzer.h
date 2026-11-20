#pragma once

// ============================================================
// NEON STUDIO — MEMORY BANDWIDTH ANALYSER
// Analyses memory access patterns and bandwidth utilisation.
//
// Two modes:
//   Static  — estimates from op shapes (no hardware)
//   Runtime — reads actual DMA counters from hardware
//
// ── Single-model (full SRAM budget) ──────────────────────────
//   MemBWAnalyzer bw(npu_component);
//   auto records = bw.estimateAll(ops);
//   auto summary = bw.summarise(records, weight_bytes, scratch_bytes);
//
// ── Multi-lane (per-lane SRAM budget) ────────────────────────
//   // SRAM is partitioned proportionally to lane row allocation
//   // Lane i budget ≈ total_sram * lane_rows[i] / total_rows
//   auto fit = bw.checkMultiLaneFit({records_a, records_b}, n_splits);
//   if (!fit.fits) { /* reduce model size or adjust n_splits */ }
// ============================================================

#include "PerfReport.h"
#include "StaticAnalyzer.h"
#include "../core/NPUComponent.h"

#ifdef __cplusplus
extern "C++" {

// ============================================================
// BANDWIDTH RECORD — per-op BW breakdown
// ============================================================

struct BWRecord {
    uint32_t    op_index;
    std::string op_name;

    // Bytes breakdown
    int64_t weight_bytes;           // weight section bytes for this op
    int64_t input_bytes;            // input activation bytes
    int64_t output_bytes;           // output activation bytes
    int64_t total_dram_bytes;       // actual bytes hitting DRAM

    // Bandwidth
    float   dram_read_gbps;
    float   dram_write_gbps;
    float   dram_total_gbps;
    float   dram_util_pct;          // % of peak DRAM BW

    // Reuse
    int64_t weight_reuse_factor;
    bool    weight_fits_sram;       // weight stays on SRAM across tiles?
                                    // checked against sram_budget_bytes, not total

    // Access pattern
    bool    is_sequential;
    bool    is_dma_bound;           // latency dominated by DMA, not compute

    // Per DMA channel utilisation — 8 channels max (future-proof)
    float   channel_util[8];        // index = channel id, value = % busy

    // Which lane this record belongs to (255 = single-model / unassigned)
    uint8_t lane;
};

// ============================================================
// MEMORY BANDWIDTH ANALYSER
// ============================================================

class MemBWAnalyzer {
public:
    explicit MemBWAnalyzer(const NPUComponent* npu_component)
        : npu_component_(npu_component) {}

    // ── Static mode — single-model (full SRAM budget) ─────────

    // Estimate BW from op shapes using full SRAM as budget.
    BWRecord estimateOp(const OpShape& op) const;
    std::vector<BWRecord> estimateAll(const std::vector<OpShape>& ops) const;

    // Build MemorySummary from static estimates.
    // sram_capacity_bytes: total device SRAM (single-model = full 1.6 MB).
    MemorySummary summarise(const std::vector<BWRecord>& records,
                            size_t weight_bytes_total,
                            size_t scratch_bytes_total,
                            size_t sram_capacity_bytes = 0) const;
                            // 0 → use npu_component->getSRAMSize()

    // ── Static mode — per-lane SRAM budget ────────────────────

    // Estimate BW for one op, checking weight_fits_sram against a specific
    // SRAM budget rather than the full device SRAM.
    // Use this when the model runs on a lane with partial SA and SRAM.
    //
    // sram_budget_bytes: bytes available to this lane
    //   ≈ total_sram * lane_rows / total_rows  (proportional to row allocation)
    BWRecord estimateOp(const OpShape& op,
                        size_t sram_budget_bytes) const;

    std::vector<BWRecord> estimateAll(const std::vector<OpShape>& ops,
                                      size_t sram_budget_bytes) const;

    // ── Bottleneck detection ───────────────────────────────────

    // Returns op indices where DRAM util > threshold
    std::vector<uint32_t> findBottlenecks(
        const std::vector<BWRecord>& records,
        float threshold_pct = 80.f) const;

    // ── Recommendation ────────────────────────────────────────

    // Suggest larger tile size if op is memory-bound and weight fits SRAM
    struct TileRecommendation {
        uint32_t    op_index;
        std::string reason;
        float       expected_speedup;
    };
    std::vector<TileRecommendation> recommend(
        const std::vector<BWRecord>& records) const;

    // ── Multi-lane SRAM fit check ──────────────────────────────

    // Per-lane SRAM fit result
    struct LaneFitInfo {
        uint8_t  lane;
        size_t   required_bytes;       // peak SRAM needed by this model
        size_t   budget_bytes;         // SRAM available to this lane
        float    utilisation_pct;      // required / budget * 100
        bool     fits;                 // required <= budget
    };

    // Combined fit result for all lanes
    struct MultiLaneFitResult {
        std::vector<LaneFitInfo> lanes;
        bool   fits;                   // true if ALL lanes fit
        // If !fits: which lane is the bottleneck
        int8_t tightest_lane;          // lane with highest utilisation_pct
        float  tightest_util_pct;
        // Suggested n_splits adjustment (empty if fits or N/A)
        // e.g. "increase n_split to give Lane 0 more rows"
        std::string suggestion;
    };

    // Check whether N models fit within their respective lane SRAM budgets.
    //
    // all_records[i] : BWRecord list for model i (from estimateAll())
    // n_splits       : N-1 boundary points (same convention as StaticAnalyzer)
    //
    // SRAM budget per lane i = sram_budget(WEIGHT) * lane_rows[i] / total_rows
    // lane_rows[i] derived from n_splits.
    MultiLaneFitResult checkMultiLaneFit(
        const std::vector<std::vector<BWRecord>>& all_records,
        const std::vector<uint8_t>&               n_splits) const;

private:
    const NPUComponent* npu_component_;

    // Compute actual DRAM bytes accounting for SRAM reuse
    int64_t effectiveDRAMBytes(int64_t weight_bytes,
                               int64_t input_bytes,
                               uint32_t reuse_factor,
                               bool weight_fits_sram) const;

    float peakDRAMBW() const { return npu_component_->getDRAMBandwidth(); }
    float peakSRAMBW() const { return npu_component_->getSRAMBandwidth(); }
    size_t totalSRAM() const { return npu_component_->getNPUSpecs().sram_size_bytes; }

    // Compute per-lane SRAM budgets from n_splits
    // Returns budget_bytes[i] for each lane
    std::vector<size_t> laneSRAMBudgets(
        const std::vector<uint8_t>& n_splits) const;

    // Internal estimateOp with explicit sram budget
    BWRecord estimateOpInternal(const OpShape& op,
                                size_t sram_budget_bytes,
                                uint8_t lane = 255) const;
};

// ============================================================
// TIMELINE — bandwidth over time (for visualisation)
// ============================================================

struct BWTimelineEntry {
    float    time_ms;               // start of window
    float    dram_read_gbps;
    float    dram_write_gbps;
    int      active_dma_channel;    // which channel was most active
    int8_t   lane;                  // -1 = combined, 0..N = per-lane
};

// Build BW timeline from a sequence of BWRecords + per-op latency
std::vector<BWTimelineEntry> buildBWTimeline(
    const std::vector<BWRecord>& records,
    const std::vector<float>& op_latencies_ms);

} // extern "C++"
#endif
