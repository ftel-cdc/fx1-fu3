#include "MemBWAnalyzer.h"
#include "StaticAnalyzer.h"
#include <algorithm>
#include <cstring>

// ============================================================
// helpers
// ============================================================

static int dtypeSz(NeonDtype dt) { return neonDtypeSize(dt); }

static int64_t opWeightBytes(const OpShape& op) {
    int ds = dtypeSz(op.dtype);
    switch (op.opcode) {
    case NeonOpcode::CONV2D:
        return (int64_t)op.kernel_h * op.kernel_w * op.input_c * op.channels_out * ds;
    case NeonOpcode::DEPTHWISE_CONV: {
        uint32_t dm = op.depth_multiplier ? op.depth_multiplier : 1;
        return (int64_t)op.kernel_h * op.kernel_w * op.input_c * dm * ds;
    }
    case NeonOpcode::FC:
        return (int64_t)op.input_size * op.output_size * ds;
    default:
        return 0;
    }
}

static int64_t opInputBytes(const OpShape& op) {
    int ds = dtypeSz(op.dtype);
    int64_t elems = (int64_t)op.input_n * op.input_h * op.input_w * op.input_c;
    if (op.opcode == NeonOpcode::FC)
        elems = (int64_t)op.input_n * op.input_size;
    else if (op.element_count)
        elems = op.element_count;
    return elems * ds;
}

static int64_t opOutputBytes(const OpShape& op) {
    int ds = dtypeSz(op.dtype);
    switch (op.opcode) {
    case NeonOpcode::CONV2D: {
        uint32_t oh = (op.input_h + 2*op.pad_h - op.kernel_h) / op.stride_h + 1;
        uint32_t ow = (op.input_w + 2*op.pad_w - op.kernel_w) / op.stride_w + 1;
        return (int64_t)op.input_n * oh * ow * op.channels_out * ds;
    }
    case NeonOpcode::DEPTHWISE_CONV: {
        uint32_t dm = op.depth_multiplier ? op.depth_multiplier : 1;
        uint32_t oh = (op.input_h + 2*op.pad_h - op.kernel_h) / op.stride_h + 1;
        uint32_t ow = (op.input_w + 2*op.pad_w - op.kernel_w) / op.stride_w + 1;
        return (int64_t)op.input_n * oh * ow * op.input_c * dm * ds;
    }
    case NeonOpcode::MAXPOOL: case NeonOpcode::AVGPOOL: {
        uint32_t oh = (op.input_h - op.kernel_h) / op.stride_h + 1;
        uint32_t ow = (op.input_w - op.kernel_w) / op.stride_w + 1;
        return (int64_t)op.input_n * oh * ow * op.input_c * ds;
    }
    case NeonOpcode::FC:
        return (int64_t)op.input_n * op.output_size * ds;
    default: {
        int64_t elems = op.element_count ? op.element_count
                      : (int64_t)op.input_n * op.input_h * op.input_w * op.input_c;
        return elems * ds;
    }
    }
}

static uint32_t weightReuse(const OpShape& op) {
    if (op.opcode == NeonOpcode::CONV2D)
        return std::max<uint32_t>(1u, op.input_h * op.input_w / (op.kernel_h * op.kernel_w));
    return 1;
}

// ============================================================
// effectiveDRAMBytes — weight reuse reduces DRAM reads
// ============================================================

int64_t MemBWAnalyzer::effectiveDRAMBytes(int64_t weight_bytes,
                                           int64_t input_bytes,
                                           uint32_t reuse_factor,
                                           bool weight_fits_sram) const {
    // If weight fits in SRAM: loaded once, reused free.
    // If not: streamed once per tile (reuse_factor tiles → 1x if SRAM fits)
    int64_t effective_w = weight_fits_sram
                         ? weight_bytes            // still DMA once at load
                         : weight_bytes * reuse_factor;
    return effective_w + input_bytes;
}

// ============================================================
// estimateOpInternal
// ============================================================

BWRecord MemBWAnalyzer::estimateOpInternal(const OpShape& op,
                                            size_t sram_budget_bytes,
                                            uint8_t lane) const {
    int64_t w_bytes  = opWeightBytes(op);
    int64_t in_bytes = opInputBytes(op);
    int64_t out_bytes= opOutputBytes(op);
    uint32_t reuse   = weightReuse(op);
    bool fits_sram   = (sram_budget_bytes > 0)
                       ? (size_t)w_bytes <= sram_budget_bytes
                       : false;

    int64_t dram_r   = effectiveDRAMBytes(w_bytes, in_bytes, reuse, fits_sram);
    int64_t total    = dram_r + out_bytes;
    float   peak_bw  = peakDRAMBW();

    BWRecord rec{};
    rec.op_index          = op.op_index;
    rec.op_name           = op.op_name;
    rec.weight_bytes      = w_bytes;
    rec.input_bytes       = in_bytes;
    rec.output_bytes      = out_bytes;
    rec.total_dram_bytes  = total;
    rec.dram_read_gbps    = (peak_bw > 0.f) ? (float)dram_r  / (peak_bw * 1e9f) * 1e3f : 0.f;
    rec.dram_write_gbps   = (peak_bw > 0.f) ? (float)out_bytes/ (peak_bw * 1e9f) * 1e3f : 0.f;
    rec.dram_total_gbps   = rec.dram_read_gbps + rec.dram_write_gbps;
    rec.dram_util_pct     = (peak_bw > 0.f)
                            ? rec.dram_total_gbps / peak_bw * 100.f
                            : 0.f;
    rec.weight_reuse_factor  = reuse;
    rec.weight_fits_sram     = fits_sram;
    rec.is_sequential        = true;
    rec.is_dma_bound         = rec.dram_util_pct > 70.f;
    rec.lane                 = lane;
    std::memset(rec.channel_util, 0, sizeof(rec.channel_util));
    return rec;
}

// ============================================================
// Public estimateOp overloads
// ============================================================

BWRecord MemBWAnalyzer::estimateOp(const OpShape& op) const {
    return estimateOpInternal(op, npu_component_->getSRAMBudget(NPUComponent::SramClass::WEIGHT));
}

BWRecord MemBWAnalyzer::estimateOp(const OpShape& op,
                                    size_t sram_budget_bytes) const {
    return estimateOpInternal(op, sram_budget_bytes);
}

std::vector<BWRecord> MemBWAnalyzer::estimateAll(
        const std::vector<OpShape>& ops) const {
    std::vector<BWRecord> out;
    out.reserve(ops.size());
    for (const auto& op : ops)
        out.push_back(estimateOp(op));
    return out;
}

std::vector<BWRecord> MemBWAnalyzer::estimateAll(
        const std::vector<OpShape>& ops,
        size_t sram_budget_bytes) const {
    std::vector<BWRecord> out;
    out.reserve(ops.size());
    for (const auto& op : ops)
        out.push_back(estimateOp(op, sram_budget_bytes));
    return out;
}

// ============================================================
// summarise
// ============================================================

MemorySummary MemBWAnalyzer::summarise(const std::vector<BWRecord>& records,
                                        size_t weight_bytes_total,
                                        size_t scratch_bytes_total,
                                        size_t sram_capacity_bytes) const {
    if (sram_capacity_bytes == 0)
        sram_capacity_bytes = totalSRAM();

    size_t tot_r = 0, tot_w = 0;
    float peak_gbps = 0.f;

    for (const auto& r : records) {
        tot_r += (size_t)(r.weight_bytes + r.input_bytes);
        tot_w += (size_t)r.output_bytes;
        if (r.dram_total_gbps > peak_gbps)
            peak_gbps = r.dram_total_gbps;
    }

    size_t peak_sram = weight_bytes_total + scratch_bytes_total;
    float  bw        = peakDRAMBW();
    float  avg_gbps  = (bw > 0.f && !records.empty())
                       ? (float)(tot_r + tot_w) / (bw * 1e9f) * 1e3f / records.size()
                       : 0.f;

    MemorySummary ms{};
    ms.weight_bytes         = weight_bytes_total;
    ms.activation_bytes     = scratch_bytes_total;
    ms.peak_sram_usage_bytes= peak_sram;
    ms.sram_capacity_bytes  = sram_capacity_bytes;
    ms.sram_utilization_pct = (sram_capacity_bytes > 0)
                              ? (float)peak_sram / (float)sram_capacity_bytes * 100.f
                              : 0.f;
    ms.total_dram_read_bytes = tot_r;
    ms.total_dram_write_bytes= tot_w;
    ms.peak_dram_bw_gbps    = peak_gbps;
    ms.avg_dram_bw_gbps     = avg_gbps;
    ms.peak_dram_util_pct   = (bw > 0.f) ? peak_gbps / bw * 100.f : 0.f;
    return ms;
}

// ============================================================
// findBottlenecks
// ============================================================

std::vector<uint32_t> MemBWAnalyzer::findBottlenecks(
        const std::vector<BWRecord>& records,
        float threshold_pct) const {
    std::vector<uint32_t> out;
    for (const auto& r : records)
        if (r.dram_util_pct > threshold_pct)
            out.push_back(r.op_index);
    return out;
}

// ============================================================
// recommend
// ============================================================

std::vector<MemBWAnalyzer::TileRecommendation> MemBWAnalyzer::recommend(
        const std::vector<BWRecord>& records) const {
    std::vector<TileRecommendation> out;
    for (const auto& r : records) {
        if (r.is_dma_bound && r.weight_fits_sram) {
            TileRecommendation rec{};
            rec.op_index        = r.op_index;
            rec.reason          = "DMA-bound; weight fits SRAM — increase tile size to reduce DMA overhead";
            rec.expected_speedup= 1.2f + (r.dram_util_pct / 100.f) * 0.5f;
            out.push_back(rec);
        }
    }
    return out;
}

// ============================================================
// laneSRAMBudgets
// ============================================================

std::vector<size_t> MemBWAnalyzer::laneSRAMBudgets(
        const std::vector<uint8_t>& n_splits) const {
    uint8_t total = (uint8_t)npu_component_->getNPUSpecs().matrix_block.sa.rows;
    size_t  sram  = npu_component_->getSRAMBudget(NPUComponent::SramClass::WEIGHT);
    size_t N = n_splits.size() + 1;

    std::vector<uint8_t> rows;
    uint8_t prev = 0;
    for (uint8_t sp : n_splits) {
        rows.push_back(sp - prev);
        prev = sp;
    }
    rows.push_back(total - prev);

    std::vector<size_t> budgets;
    for (size_t i = 0; i < N; ++i) {
        uint8_t lr = (i < rows.size()) ? rows[i] : 0;
        budgets.push_back(sram * lr / total);
    }
    return budgets;
}

// ============================================================
// checkMultiLaneFit
// ============================================================

MemBWAnalyzer::MultiLaneFitResult MemBWAnalyzer::checkMultiLaneFit(
        const std::vector<std::vector<BWRecord>>& all_records,
        const std::vector<uint8_t>&               n_splits) const {
    auto budgets = laneSRAMBudgets(n_splits);
    size_t N = all_records.size();

    MultiLaneFitResult result{};
    result.fits = true;
    result.tightest_lane     = -1;
    result.tightest_util_pct = 0.f;

    for (size_t i = 0; i < N; ++i) {
        size_t budget = (i < budgets.size()) ? budgets[i] : totalSRAM();

        // Peak SRAM needed = sum of all weight bytes (worst case all in SRAM)
        size_t needed = 0;
        for (const auto& r : all_records[i])
            needed += (size_t)r.weight_bytes;

        float util = (budget > 0) ? (float)needed / (float)budget * 100.f : 100.f;
        bool  lane_fits = needed <= budget;

        LaneFitInfo info{};
        info.lane           = (uint8_t)i;
        info.required_bytes = needed;
        info.budget_bytes   = budget;
        info.utilisation_pct= util;
        info.fits           = lane_fits;
        result.lanes.push_back(info);

        if (!lane_fits) result.fits = false;
        if (util > result.tightest_util_pct) {
            result.tightest_util_pct = util;
            result.tightest_lane     = (int8_t)i;
        }
    }

    if (!result.fits && result.tightest_lane >= 0) {
        result.suggestion = "Lane " + std::to_string(result.tightest_lane)
                          + " is too tight (" + std::to_string((int)result.tightest_util_pct)
                          + "%). Increase n_split to give it more SA rows, "
                            "or reduce model weight footprint.";
    }
    return result;
}

// ============================================================
// buildBWTimeline (free function)
// ============================================================

std::vector<BWTimelineEntry> buildBWTimeline(
        const std::vector<BWRecord>& records,
        const std::vector<float>& op_latencies_ms) {
    std::vector<BWTimelineEntry> tl;
    float t = 0.f;
    for (size_t i = 0; i < records.size(); ++i) {
        float dt = (i < op_latencies_ms.size()) ? op_latencies_ms[i] : 0.f;
        if (dt <= 0.f) continue;
        BWTimelineEntry e{};
        e.time_ms           = t;
        e.dram_read_gbps    = (float)records[i].weight_bytes / (dt * 1e-3f) / 1e9f;
        e.dram_write_gbps   = (float)records[i].output_bytes / (dt * 1e-3f) / 1e9f;
        e.active_dma_channel= 0;
        e.lane              = records[i].lane;
        tl.push_back(e);
        t += dt;
    }
    return tl;
}
