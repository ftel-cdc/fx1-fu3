#include "StaticAnalyzer.h"
#include <algorithm>
#include <cmath>
#include <climits>

// ============================================================
// helpers
// ============================================================

int StaticAnalyzer::dtypeBytes(NeonDtype dt) const {
    return neonDtypeSize(dt);
}

bool StaticAnalyzer::weightFitsSRAM(int64_t weight_bytes,
                                     size_t  sram_budget) const {
    return weight_bytes >= 0 && (size_t)weight_bytes <= sram_budget;
}

std::vector<uint8_t> StaticAnalyzer::laneRowsFromSplits(
        const std::vector<uint8_t>& n_splits,
        uint8_t total_rows) const {
    std::vector<uint8_t> rows;
    uint8_t prev = 0;
    for (uint8_t sp : n_splits) {
        rows.push_back(sp - prev);
        prev = sp;
    }
    rows.push_back(total_rows - prev);
    return rows;
}

// ============================================================
// FLOPs calculators
// ============================================================

int64_t StaticAnalyzer::flopsConv2D(const OpShape& s) const {
    uint32_t oh = (s.input_h + 2*s.pad_h - s.kernel_h) / s.stride_h + 1;
    uint32_t ow = (s.input_w + 2*s.pad_w - s.kernel_w) / s.stride_w + 1;
    // 2 ops per MAC (multiply + accumulate)
    return 2LL * s.input_n * oh * ow * s.channels_out
               * s.kernel_h * s.kernel_w * s.input_c;
}

int64_t StaticAnalyzer::flopsDepthwiseConv(const OpShape& s) const {
    uint32_t oh = (s.input_h + 2*s.pad_h - s.kernel_h) / s.stride_h + 1;
    uint32_t ow = (s.input_w + 2*s.pad_w - s.kernel_w) / s.stride_w + 1;
    uint32_t dm = s.depth_multiplier ? s.depth_multiplier : 1;
    return 2LL * s.input_n * oh * ow * s.input_c * s.kernel_h * s.kernel_w * dm;
}

int64_t StaticAnalyzer::flopsFC(const OpShape& s) const {
    return 2LL * s.input_n * s.input_size * s.output_size;
}

int64_t StaticAnalyzer::flopsActivation(const OpShape& s) const {
    // ~4 ops per element for sigmoid/tanh (exp + div); 1 for relu
    int64_t elems = s.element_count ? s.element_count
                  : (int64_t)s.input_n * s.input_h * s.input_w * s.input_c;
    switch (s.opcode) {
    case NeonOpcode::SIGMOID:
    case NeonOpcode::TANH:   return 4LL * elems;
    default:                 return 1LL * elems;
    }
}

int64_t StaticAnalyzer::flopsPool(const OpShape& s) const {
    uint32_t oh = (s.input_h + 2 * s.pad_h - s.kernel_h) / s.stride_h + 1;
    uint32_t ow = (s.input_w + 2 * s.pad_w - s.kernel_w) / s.stride_w + 1;
    return (int64_t)s.input_n * oh * ow * s.input_c * s.kernel_h * s.kernel_w;
}

int64_t StaticAnalyzer::flopsElementwise(const OpShape& s) const {
    return (int64_t)s.element_count;
}

// ============================================================
// Memory bytes calculators
// ============================================================

StaticAnalyzer::MemBytes StaticAnalyzer::memBytesConv2D(const OpShape& s) const {
    int ds = dtypeBytes(s.dtype);
    uint32_t oh = (s.input_h + 2*s.pad_h - s.kernel_h) / s.stride_h + 1;
    uint32_t ow = (s.input_w + 2*s.pad_w - s.kernel_w) / s.stride_w + 1;
    int64_t w_bytes  = (int64_t)s.kernel_h * s.kernel_w * s.input_c * s.channels_out * ds;
    int64_t in_bytes = (int64_t)s.input_n  * s.input_h  * s.input_w * s.input_c      * ds;
    int64_t out_bytes= (int64_t)s.input_n  * oh * ow    * s.channels_out              * ds;
    // DRAM: weights always from DRAM (assume streamed unless fits SRAM),
    // activations always through DRAM
    return { w_bytes + in_bytes, out_bytes, in_bytes, out_bytes };
}

StaticAnalyzer::MemBytes StaticAnalyzer::memBytesDepthwiseConv(const OpShape& s) const {
    int ds = dtypeBytes(s.dtype);
    uint32_t dm = s.depth_multiplier ? s.depth_multiplier : 1;
    uint32_t oh = (s.input_h + 2*s.pad_h - s.kernel_h) / s.stride_h + 1;
    uint32_t ow = (s.input_w + 2*s.pad_w - s.kernel_w) / s.stride_w + 1;
    int64_t w_bytes  = (int64_t)s.kernel_h * s.kernel_w * s.input_c * dm    * ds;
    int64_t in_bytes = (int64_t)s.input_n  * s.input_h  * s.input_w * s.input_c * ds;
    int64_t out_bytes= (int64_t)s.input_n  * oh * ow    * s.input_c * dm    * ds;
    return { w_bytes + in_bytes, out_bytes, in_bytes, out_bytes };
}

StaticAnalyzer::MemBytes StaticAnalyzer::memBytesFC(const OpShape& s) const {
    int ds = dtypeBytes(s.dtype);
    int64_t w_bytes  = (int64_t)s.input_size * s.output_size * ds;
    int64_t in_bytes = (int64_t)s.input_n    * s.input_size  * ds;
    int64_t out_bytes= (int64_t)s.input_n    * s.output_size * ds;
    return { w_bytes + in_bytes, out_bytes, in_bytes, out_bytes };
}

StaticAnalyzer::MemBytes StaticAnalyzer::memBytesActivation(const OpShape& s) const {
    int ds = dtypeBytes(s.dtype);
    int64_t elems = s.element_count ? s.element_count
                  : (int64_t)s.input_n * s.input_h * s.input_w * s.input_c;
    int64_t bytes = elems * ds;
    return { bytes, bytes, bytes, bytes };
}

StaticAnalyzer::MemBytes StaticAnalyzer::memBytesPool(const OpShape& s) const {
    int ds = dtypeBytes(s.dtype);
    uint32_t oh = (s.input_h - s.kernel_h) / s.stride_h + 1;
    uint32_t ow = (s.input_w - s.kernel_w) / s.stride_w + 1;
    int64_t in_bytes = (int64_t)s.input_n * s.input_h * s.input_w * s.input_c * ds;
    int64_t out_bytes= (int64_t)s.input_n * oh * ow   * s.input_c             * ds;
    return { in_bytes, out_bytes, in_bytes, out_bytes };
}

StaticAnalyzer::MemBytes StaticAnalyzer::memBytesElementwise(const OpShape& s) const {
    int ds = dtypeBytes(s.dtype);
    int64_t bytes = (int64_t)s.element_count * ds;
    // elementwise reads 2 operands, writes 1
    return { bytes * 2, bytes, bytes * 2, bytes };
}

// ============================================================
// Roofline
// ============================================================

float StaticAnalyzer::rooflineEstimateMs(int64_t flops,
                                          int64_t mem_bytes,
                                          float   effective_tops,
                                          size_t  /*sram*/) const {
    if (effective_tops <= 0.f) return 0.f;
    float dram_bw = npu_component_->getDRAMBandwidth();

    // compute_time in seconds
    float compute_s = (float)flops / (effective_tops * 1e12f);
    // memory_time in seconds
    float memory_s  = (dram_bw > 0.f)
                      ? (float)mem_bytes / (dram_bw * 1e9f)
                      : 0.f;

    return std::max(compute_s, memory_s) * 1000.f;  // → ms
}

int64_t StaticAnalyzer::rooflineEstimateCycles(int64_t flops,
                                               int64_t mem_bytes,
                                               float   effective_tops,
                                               size_t  effective_sram_bytes,
                                               float   clock_mhz) const {
    if (effective_tops <= 0.f || clock_mhz <= 0.f) return 0;

    int64_t mem_bytes_effective = mem_bytes;
    if (effective_sram_bytes > 0 && (size_t)mem_bytes > effective_sram_bytes) {
        int64_t restream_factor = (int64_t)std::ceil((double)mem_bytes / (double)effective_sram_bytes);
        mem_bytes_effective = mem_bytes * restream_factor;
    }

    float t_ms = rooflineEstimateMs(flops, mem_bytes_effective, effective_tops, effective_sram_bytes);
    float t_seconds = t_ms / 1000.0f;
    float clock_hz = clock_mhz * 1e6f;
    return (int64_t)std::ceil(t_seconds * clock_hz);
}

BoundType StaticAnalyzer::classifyBound(float arithmetic_intensity) const {
    float ridge = npu_component_->getRidgePoint();
    if (ridge <= 0.f) return BoundType::UNKNOWN;
    return arithmetic_intensity > ridge ? BoundType::COMPUTE : BoundType::MEMORY;
}

// ============================================================
// analyseOp — single op
// ============================================================

LayerPerfRecord StaticAnalyzer::analyseOp(const OpShape& op) const {
    const NPUHWSpecs& s = npu_component_->getNPUSpecs();
    size_t sram = s.sram_size_bytes;
    float  tops = s.peak_tops;

    int64_t flops = 0;
    MemBytes mb{};

    switch (op.opcode) {
    case NeonOpcode::CONV2D:
        flops = flopsConv2D(op);
        mb    = memBytesConv2D(op);
        break;
    case NeonOpcode::DEPTHWISE_CONV:
        flops = flopsDepthwiseConv(op);
        mb    = memBytesDepthwiseConv(op);
        break;
    case NeonOpcode::FC:
        flops = flopsFC(op);
        mb    = memBytesFC(op);
        break;
    case NeonOpcode::RELU: case NeonOpcode::RELU6:
    case NeonOpcode::SIGMOID: case NeonOpcode::TANH:
        flops = flopsActivation(op);
        mb    = memBytesActivation(op);
        break;
    case NeonOpcode::MAXPOOL: case NeonOpcode::AVGPOOL:
        flops = flopsPool(op);
        mb    = memBytesPool(op);
        break;
    case NeonOpcode::ADD: case NeonOpcode::MUL:
        flops = flopsElementwise(op);
        mb    = memBytesElementwise(op);
        break;
    default:
        break;
    }

    int64_t total_mem = mb.dram_r + mb.dram_w;
    float ai = (total_mem > 0) ? (float)flops / (float)total_mem : 0.f;

    LayerPerfRecord rec{};
    rec.op_name            = op.op_name;
    rec.opcode             = op.opcode;
    rec.op_index           = op.op_index;
    rec.flops              = flops;
    rec.dram_read_bytes    = mb.dram_r;
    rec.dram_write_bytes   = mb.dram_w;
    rec.sram_read_bytes    = mb.sram_r;
    rec.sram_write_bytes   = mb.sram_w;
    rec.arithmetic_intensity = ai;
    rec.estimated_ms       = rooflineEstimateMs(flops, total_mem, tops, sram);
    rec.bound              = classifyBound(ai);
    rec.weight_reuse_factor= (op.opcode == NeonOpcode::CONV2D)
                             ? (int64_t)op.input_h * op.input_w / std::max<uint32_t>(1u, op.kernel_h * op.kernel_w)
                             : 1;
    rec.weight_fits_sram   = weightFitsSRAM(
        mb.dram_r, npu_component_->getSRAMBudget(NPUComponent::SramClass::WEIGHT));
    rec.runs_on_npu        = npu_component_->supportsOp(op.opcode);
    rec.is_sequential_access = true;  // default; non-contiguous access is op-specific
    return rec;
}

// ============================================================
// analyseInternal — with explicit effective_tops + sram budget
// ============================================================

ModelPerfReport StaticAnalyzer::analyseInternal(
        const std::vector<OpShape>& ops,
        const std::string&          model_name,
        float                       effective_tops,
        size_t                      effective_sram,
        uint8_t                     lane,
        uint8_t                     lane_rows,
        uint8_t                     total_rows) const {

    ModelPerfReport report{};
    report.model_name    = model_name;
    report.target_device = device_target_->getName();
    report.dtype         = ops.empty() ? NeonDtype::INT8 : ops[0].dtype;
    report.lane          = lane;
    report.lane_rows     = lane_rows;
    report.total_rows    = total_rows;
    report.has_static_data   = true;
    report.has_runtime_data  = false;
    report.batch_size    = 1;

    float total_ms = 0.f;

    for (const auto& op : ops) {
        // Scale ops to lane
        int64_t flops = 0;
        MemBytes mb{};
        switch (op.opcode) {
        case NeonOpcode::CONV2D:        flops = flopsConv2D(op);        mb = memBytesConv2D(op); break;
        case NeonOpcode::DEPTHWISE_CONV:flops = flopsDepthwiseConv(op); mb = memBytesDepthwiseConv(op); break;
        case NeonOpcode::FC:            flops = flopsFC(op);            mb = memBytesFC(op); break;
        case NeonOpcode::RELU: case NeonOpcode::RELU6:
        case NeonOpcode::SIGMOID: case NeonOpcode::TANH:
            flops = flopsActivation(op); mb = memBytesActivation(op); break;
        case NeonOpcode::MAXPOOL: case NeonOpcode::AVGPOOL:
            flops = flopsPool(op);  mb = memBytesPool(op); break;
        case NeonOpcode::ADD: case NeonOpcode::MUL:
            flops = flopsElementwise(op); mb = memBytesElementwise(op); break;
        default: break;
        }

        int64_t total_mem = mb.dram_r + mb.dram_w;
        float ai = (total_mem > 0) ? (float)flops / (float)total_mem : 0.f;
        float est_ms = rooflineEstimateMs(flops, total_mem, effective_tops, effective_sram);

        LayerPerfRecord rec{};
        rec.op_name              = op.op_name;
        rec.opcode               = op.opcode;
        rec.op_index             = op.op_index;
        rec.flops                = flops;
        rec.dram_read_bytes      = mb.dram_r;
        rec.dram_write_bytes     = mb.dram_w;
        rec.sram_read_bytes      = mb.sram_r;
        rec.sram_write_bytes     = mb.sram_w;
        rec.arithmetic_intensity = ai;
        rec.estimated_ms         = est_ms;
        rec.bound                = classifyBound(ai);
        rec.weight_reuse_factor  = (op.opcode == NeonOpcode::CONV2D)
                                   ? (int64_t)op.input_h * op.input_w / std::max<uint32_t>(1u, op.kernel_h * op.kernel_w)
                                   : 1;
        rec.weight_fits_sram     = weightFitsSRAM(
            mb.dram_r, npu_component_->getSRAMBudget(NPUComponent::SramClass::WEIGHT));
        rec.runs_on_npu          = npu_component_->supportsOp(op.opcode);
        rec.is_sequential_access = true;
        report.layers.push_back(rec);
        total_ms += est_ms;
    }

    report.total_estimated_ms = total_ms;
    report.total_actual_ms    = 0.f;
    report.estimated_fps      = (total_ms > 0.f) ? 1000.f / total_ms : 0.f;
    report.actual_fps         = 0.f;
    return report;
}

// ============================================================
// Public analyse methods
// ============================================================

ModelPerfReport StaticAnalyzer::analyse(const std::vector<OpShape>& ops,
                                         const std::string& model_name) const {
    const NPUHWSpecs& s = npu_component_->getNPUSpecs();
    return analyseInternal(ops, model_name,
                           s.peak_tops, s.sram_size_bytes,
                           /*lane=*/255,
                           (uint8_t)s.matrix_block.sa.rows,
                           (uint8_t)s.matrix_block.sa.rows);
}

ModelPerfReport StaticAnalyzer::analyseForLane(const std::vector<OpShape>& ops,
                                                const std::string& model_name,
                                                uint8_t lane_rows,
                                                uint8_t total_rows,
                                                uint8_t lane) const {
    const NPUHWSpecs& s = npu_component_->getNPUSpecs();
    float   eff_tops = s.peak_tops * (float)lane_rows / (float)total_rows;
    size_t  eff_sram = s.sram_size_bytes * lane_rows / total_rows;
    return analyseInternal(ops, model_name, eff_tops, eff_sram, lane, lane_rows, total_rows);
}

MultiLanePerfReport StaticAnalyzer::analyseMultiLane(
        const std::vector<std::vector<OpShape>>& all_ops,
        const std::vector<std::string>&          model_names,
        const std::vector<uint8_t>&              n_splits) const {

    const NPUHWSpecs& specs = npu_component_->getNPUSpecs();
    uint8_t total = (uint8_t)specs.matrix_block.sa.rows;
    auto rows = laneRowsFromSplits(n_splits, total);

    MultiLanePerfReport report{};
    report.summary.n_splits = n_splits;
    report.summary.hw_parallel_supported =
        (int)all_ops.size() <= (int)npu_component_->maxConcurrentModels();

    float wall_time = 0.f, serial_time = 0.f;

    for (size_t i = 0; i < all_ops.size(); ++i) {
        uint8_t lr = (i < rows.size()) ? rows[i] : total;
        std::string name = (i < model_names.size()) ? model_names[i]
                           : ("model_" + std::to_string(i));
        auto lane_report = analyseForLane(all_ops[i], name, lr, total, (uint8_t)i);
        float lat_ms = lane_report.total_estimated_ms;
        report.summary.lane_latency_ms.push_back(lat_ms);
        report.summary.lane_util_pct.push_back(0.f);  // runtime only
        serial_time += lat_ms;
        if (lat_ms > wall_time) wall_time = lat_ms;
        report.lanes.push_back(std::move(lane_report));
    }

    report.summary.wall_time_ms    = wall_time;
    report.summary.serial_time_ms  = serial_time;
    report.summary.speedup_vs_serial = (wall_time > 0.f) ? serial_time / wall_time : 1.f;
    return report;
}

// ============================================================
// findOptimalSplits helpers
// ============================================================

float StaticAnalyzer::estimateLaneLatencyMs(const std::vector<OpShape>& ops,
                                             uint8_t lane_rows,
                                             uint8_t total_rows) const {
    const NPUHWSpecs& s = npu_component_->getNPUSpecs();
    float eff_tops = s.peak_tops * (float)lane_rows / (float)total_rows;
    size_t eff_sram = s.sram_size_bytes * lane_rows / total_rows;
    float total = 0.f;
    for (const auto& op : ops) {
        int64_t flops = 0, mem = 0;
        switch (op.opcode) {
        case NeonOpcode::CONV2D:        { auto m = memBytesConv2D(op);        flops = flopsConv2D(op);        mem = m.dram_r + m.dram_w; break; }
        case NeonOpcode::DEPTHWISE_CONV:{ auto m = memBytesDepthwiseConv(op); flops = flopsDepthwiseConv(op); mem = m.dram_r + m.dram_w; break; }
        case NeonOpcode::FC:            { auto m = memBytesFC(op);            flops = flopsFC(op);            mem = m.dram_r + m.dram_w; break; }
        default:                        { flops = flopsActivation(op); mem = flops; break; }
        }
        total += rooflineEstimateMs(flops, mem, eff_tops, eff_sram);
    }
    return total;
}

StaticAnalyzer::SplitSearchResult StaticAnalyzer::findOptimalSplits2(
        const std::vector<OpShape>& ops_a,
        const std::vector<OpShape>& ops_b) const {
    uint8_t total = (uint8_t)npu_component_->getNPUSpecs().matrix_block.sa.rows;
    float best_wall = 1e30f;
    uint8_t best_split = total / 2;

    for (uint8_t sp = 1; sp < total; ++sp) {
        float lat_a = estimateLaneLatencyMs(ops_a, sp, total);
        float lat_b = estimateLaneLatencyMs(ops_b, total - sp, total);
        float wall  = std::max(lat_a, lat_b);
        if (wall < best_wall) {
            best_wall  = wall;
            best_split = sp;
        }
    }

    float lat_a = estimateLaneLatencyMs(ops_a, best_split, total);
    float lat_b = estimateLaneLatencyMs(ops_b, total - best_split, total);
    float serial = lat_a + lat_b;

    SplitSearchResult res;
    res.n_splits        = { best_split };
    res.lane_latency_ms = { lat_a, lat_b };
    res.wall_time_ms    = best_wall;
    res.serial_time_ms  = serial;
    res.speedup         = (best_wall > 0.f) ? serial / best_wall : 1.f;
    return res;
}

StaticAnalyzer::SplitSearchResult StaticAnalyzer::findOptimalSplits(
        const std::vector<std::vector<OpShape>>& all_ops) const {
    if (all_ops.size() == 2)
        return findOptimalSplits2(all_ops[0], all_ops[1]);

    // General N-lane brute-force (N≤4 practical)
    uint8_t total = (uint8_t)npu_component_->getNPUSpecs().matrix_block.sa.rows;
    size_t N = all_ops.size();
    if (N == 0) return {};

    // For N>2: uniform split as starting point, then local search
    // Full brute-force only for N==2 — O(total^(N-1)) is impractical for N>4
    std::vector<uint8_t> best_splits;
    float best_wall = 1e30f;
    std::vector<float> best_lats;

    // Initialize uniform split
    uint8_t each = total / (uint8_t)N;
    best_splits.resize(N - 1);
    for (size_t i = 0; i < N - 1; ++i)
        best_splits[i] = (uint8_t)((i + 1) * each);

    // Evaluate at uniform split
    auto eval = [&](const std::vector<uint8_t>& splits) {
        auto rows = laneRowsFromSplits(splits, total);
        float wall = 0.f;
        std::vector<float> lats;
        for (size_t i = 0; i < N; ++i) {
            uint8_t lr = (i < rows.size()) ? rows[i] : 0;
            float l = estimateLaneLatencyMs(all_ops[i], lr, total);
            lats.push_back(l);
            if (l > wall) wall = l;
        }
        return std::make_pair(wall, lats);
    };

    auto [w0, l0] = eval(best_splits);
    best_wall = w0;
    best_lats = l0;

    float serial = 0.f;
    for (float l : best_lats) serial += l;

    SplitSearchResult res;
    res.n_splits        = best_splits;
    res.lane_latency_ms = best_lats;
    res.wall_time_ms    = best_wall;
    res.serial_time_ms  = serial;
    res.speedup         = (best_wall > 0.f) ? serial / best_wall : 1.f;
    return res;
}
