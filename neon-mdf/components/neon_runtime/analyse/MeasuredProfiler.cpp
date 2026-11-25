#include "MeasuredProfiler.h"
#include "StaticAnalyzer.h"
#include "../core/NPUComponent.h"
#include <algorithm>
#include <numeric>
#include <cstring>

// ============================================================
// onOpStart
// ============================================================

void MeasuredProfiler::onOpStart(uint32_t                    op_index,
                                  NeonOpcode                  opcode,
                                  const std::string&          op_name,
                                  uint8_t                     lane,
                                  const PerfCounterSnapshot&  before) {
    if (!enabled_) return;

    pending_.op_index  = op_index;
    pending_.opcode    = opcode;
    pending_.op_name   = op_name;
    pending_.lane      = lane;
    pending_.cycles    = before.total;       // save c0
    pending_.stall_cycles     = 0;
    pending_before_           = before;
    pending_.dma_read_bytes  = 0;
    pending_.dma_write_bytes = 0;

    if (before.total > 0) {
        pending_.timing_src = LayerPerfRecord::TimingSource::HW_CYCLE_COUNTER;
        pending_.start_ns   = 0;
    } else {
        pending_.timing_src = LayerPerfRecord::TimingSource::HOST_CLOCK;
        pending_.start_ns   = nowNs();
    }
    pending_.end_ns = 0;
    op_in_flight_   = true;
}

// ============================================================
// onOpEnd
// ============================================================

void MeasuredProfiler::onOpEnd(size_t                       dma_read_bytes,
                                size_t                       dma_write_bytes,
                                const PerfCounterSnapshot&   after) {
    if (!enabled_ || !op_in_flight_) return;
    op_in_flight_ = false;

    OpTimingRecord rec = pending_;
    rec.dma_read_bytes  = dma_read_bytes;
    rec.dma_write_bytes = dma_write_bytes;

    // Counters are absolute and monotonic (NPUV4Runtime accumulates rather than
    // mirroring a register that npu_tlm.cpp:1271 resets per job), so a plain
    // subtraction is valid. Guard against a reset anyway: a decrease means the
    // counter restarted, in which case the post-value IS the delta.
    auto delta = [](uint64_t after_val, uint64_t before_val) -> uint64_t {
        return (after_val >= before_val) ? (after_val - before_val) : after_val;
    };
    rec.stall_cycles      = delta(after.stall,       pending_before_.stall);
    rec.sa_cycles         = delta(after.sa,          pending_before_.sa);
    rec.obp_cycles        = delta(after.obp,         pending_before_.obp);
    rec.mac_engine_cycles = delta(after.mac_engine,  pending_before_.mac_engine);
    rec.dma_engine_cycles = delta(after.dma_engine,  pending_before_.dma_engine);
    rec.act_engine_cycles = delta(after.act_engine,  pending_before_.act_engine);
    rec.pool_engine_cycles= delta(after.pool_engine, pending_before_.pool_engine);
    rec.red_engine_cycles = delta(after.red_engine,  pending_before_.red_engine);
    rec.dma_read_cycles   = delta(after.dma_read,    pending_before_.dma_read);
    rec.dma_write_cycles  = delta(after.dma_write,   pending_before_.dma_write);
    rec.hw_m              = delta(after.rich_m,      pending_before_.rich_m);
    rec.hw_k              = delta(after.rich_k,      pending_before_.rich_k);
    rec.hw_n              = delta(after.rich_n,      pending_before_.rich_n);

    if (rec.timing_src == LayerPerfRecord::TimingSource::HW_CYCLE_COUNTER) {
        // Compute wrap-safe delta
        uint64_t c0 = rec.cycles;
        uint64_t c1 = after.total;
        rec.cycles = (c1 >= c0) ? (c1 - c0) : (0xFFFFFFFFULL - c0 + c1 + 1ULL);
        // latency_ms will be computed in buildReport (need clock_mhz from target)
        rec.latency_ms = 0.f;
        rec.end_ns     = 0;
    } else {
        rec.end_ns    = nowNs();
        rec.cycles    = 0;
        rec.latency_ms= (float)(rec.end_ns - rec.start_ns) * 1e-6f;
    }
    records_.push_back(rec);
}

// ============================================================
// totalLatencyMs / totalCycles / hasCycleData
// ============================================================

float MeasuredProfiler::totalLatencyMs() const {
    float total = 0.f;
    for (const auto& r : records_)
        total += r.latency_ms;
    return total;
}

uint64_t MeasuredProfiler::totalCycles() const {
    uint64_t total = 0;
    for (const auto& r : records_)
        total += r.cycles;
    return total;
}

bool MeasuredProfiler::hasCycleData() const {
    for (const auto& r : records_)
        if (r.timing_src == LayerPerfRecord::TimingSource::HW_CYCLE_COUNTER
            && r.cycles > 0)
            return true;
    return false;
}

// ============================================================
// buildReport
// ============================================================

ModelPerfReport MeasuredProfiler::buildReport(
        const DeviceTarget*     device_target,
        const NPUComponent*        npu_component,
        const std::string&          model_name,
        uint8_t                     lane,
        uint8_t                     lane_rows,
        const std::vector<OpShape>* shapes,
        const ModelPerfReport*      static_r,
        uint32_t                    batch_size) const {

    const NPUHWSpecs& specs = npu_component->getNPUSpecs();
    float clock_mhz  = specs.clock_mhz;
    uint8_t sa_cols  = (uint8_t)specs.matrix_block.sa.cols;
    uint8_t sa_rows  = (uint8_t)specs.matrix_block.sa.rows;
    uint8_t eff_rows = (lane_rows > 0) ? lane_rows : sa_rows;
    float   peak_flops_per_cycle = (float)eff_rows * (float)sa_cols * 2.f;
    float   dram_bw  = npu_component->getDRAMBandwidth();

    ModelPerfReport report{};
    report.model_name    = model_name;
    report.target_device = device_target->getName();
    report.peak_tops     = specs.peak_tops;
    report.dtype         = NeonDtype::INT8;
    report.lane          = lane;
    report.lane_rows     = eff_rows;
    report.total_rows    = sa_rows;
    report.batch_size    = batch_size;
    report.has_runtime_data = !records_.empty();
    report.has_static_data  = (static_r != nullptr);

    if (records_.empty() && static_r) {
        report.layers = static_r->layers;
        report.total_estimated_ms = static_r->total_estimated_ms;
        return report;
    }

    float total_actual_ms = 0.f;

    // Map op_index → shape (if available)
    std::vector<const OpShape*> shape_map;
    if (shapes) {
        for (const auto& op : records_) {
            const OpShape* found = nullptr;
            for (const auto& s : *shapes)
                if (s.op_index == op.op_index) { found = &s; break; }
            shape_map.push_back(found);
        }
    }

    for (size_t i = 0; i < records_.size(); ++i) {
        const OpTimingRecord& ot = records_[i];

        LayerPerfRecord lpr{};
        lpr.op_name    = ot.op_name;
        lpr.opcode     = ot.opcode;
        lpr.op_index   = ot.op_index;
        lpr.timing_src = ot.timing_src;
        lpr.actual_cycles  = ot.cycles;
        lpr.stall_cycles   = ot.stall_cycles;

        // Latency
        float lat_ms = ot.latency_ms;
        if (ot.timing_src == LayerPerfRecord::TimingSource::HW_CYCLE_COUNTER
            && ot.cycles > 0 && clock_mhz > 0.f) {
            lat_ms = (float)ot.cycles / clock_mhz / 1000.f;
        }
        lpr.actual_ms = lat_ms;
        total_actual_ms += lat_ms;

        // MAC efficiency (requires shapes + cycle counter)
        if (shapes && i < shape_map.size() && shape_map[i]
            && ot.timing_src == LayerPerfRecord::TimingSource::HW_CYCLE_COUNTER
            && ot.cycles > 0 && peak_flops_per_cycle > 0.f) {
            // Compute theoretical FLOPs from shape
            const OpShape& s = *shape_map[i];
            int64_t flops = 0;
            switch (s.opcode) {
            case NeonOpcode::CONV2D: {
                uint32_t oh = (s.input_h + 2*s.pad_h - s.kernel_h) / s.stride_h + 1;
                uint32_t ow = (s.input_w + 2*s.pad_w - s.kernel_w) / s.stride_w + 1;
                flops = 2LL * s.input_n * oh * ow * s.channels_out * s.kernel_h * s.kernel_w * s.input_c;
                break;
            }
            case NeonOpcode::FC:
                flops = 2LL * s.input_n * s.input_size * s.output_size;
                break;
            default:
                // Note: LAYERNORM/FUSED_ATTN/ELEM_WISE FLOPs not yet implemented;
                // see IMPLEMENTATION_PLAN_VP_MEASURED_INTO_DEVICE_ESTIMATION.md §Known limitation.
                break;
            }
            if (flops > 0) {
                float actual_fpc = (float)flops / (float)ot.cycles;
                lpr.actual_flops_per_cycle = actual_fpc;
                lpr.peak_flops_per_cycle   = peak_flops_per_cycle;
                lpr.mac_efficiency_pct     = actual_fpc / peak_flops_per_cycle * 100.f;

                // Achieved throughput: flops / seconds, expressed in TOPS.
                if (lat_ms > 0.f)
                    lpr.achieved_tops = (float)flops / (lat_ms * 1e-3f) / 1e12f;
            }
        }

        // Stall ratio, plus the SA/OBP activity split. All three are
        // fractions of the same denominator (ot.cycles = total_cycles delta);
        // SA and OBP overlap in time, so they do not sum to 100%.
        if (ot.cycles > 0) {
            lpr.stall_ratio_pct  = (float)ot.stall_cycles / (float)ot.cycles * 100.f;
            lpr.sa_cycles_pct    = (float)ot.sa_cycles    / (float)ot.cycles * 100.f;
            lpr.obp_cycles_pct   = (float)ot.obp_cycles   / (float)ot.cycles * 100.f;
            // Added — see IMPLEMENTATION_PLAN_VP_MEASURED_INTO_DEVICE_ESTIMATION.md Phase A6.
            lpr.mac_engine_cycles_pct  = (float)ot.mac_engine_cycles  / (float)ot.cycles * 100.f;
            lpr.dma_engine_cycles_pct  = (float)ot.dma_engine_cycles  / (float)ot.cycles * 100.f;
            lpr.act_engine_cycles_pct  = (float)ot.act_engine_cycles  / (float)ot.cycles * 100.f;
            lpr.pool_engine_cycles_pct = (float)ot.pool_engine_cycles / (float)ot.cycles * 100.f;
            lpr.red_engine_cycles_pct  = (float)ot.red_engine_cycles  / (float)ot.cycles * 100.f;
        }
        lpr.hw_reported_m = ot.hw_m;
        lpr.hw_reported_k = ot.hw_k;
        lpr.hw_reported_n = ot.hw_n;

        // DRAM BW (actual)
        if (lat_ms > 0.f && dram_bw > 0.f) {
            float dur_s = lat_ms * 1e-3f;
            lpr.actual_dram_read_gbps  = (float)ot.dma_read_bytes  / dur_s / 1e9f;
            lpr.actual_dram_write_gbps = (float)ot.dma_write_bytes / dur_s / 1e9f;
            lpr.actual_dram_util_pct   = (lpr.actual_dram_read_gbps + lpr.actual_dram_write_gbps)
                                         / dram_bw * 100.f;
        }

        // Copy static fields from static_r if available
        if (static_r && ot.op_index < static_r->layers.size()) {
            const auto& sl = static_r->layers[ot.op_index];
            lpr.flops                = sl.flops;
            lpr.dram_read_bytes      = sl.dram_read_bytes;
            lpr.dram_write_bytes     = sl.dram_write_bytes;
            lpr.sram_read_bytes      = sl.sram_read_bytes;
            lpr.sram_write_bytes     = sl.sram_write_bytes;
            lpr.arithmetic_intensity = sl.arithmetic_intensity;
            lpr.estimated_ms         = sl.estimated_ms;
            lpr.bound                = sl.bound;
            lpr.runs_on_npu          = sl.runs_on_npu;
            lpr.weight_reuse_factor  = sl.weight_reuse_factor;
            lpr.weight_fits_sram     = sl.weight_fits_sram;
            lpr.is_sequential_access = sl.is_sequential_access;
        } else if (npu_component) {
            lpr.runs_on_npu          = npu_component->supportsOp(ot.opcode);
        }

        // Roofline coverage: only ops that actually produced cycle data can be
        // compared against their estimate (see ModelPerfReport's comment).
        report.total_op_count++;
        if (ot.cycles > 0 && lpr.runs_on_npu) {
            report.measured_op_count++;
            report.estimated_ms_measured_subset += lpr.estimated_ms;
        }

        report.layers.push_back(lpr);
    }

    report.total_actual_ms    = total_actual_ms;
    report.total_estimated_ms = static_r ? static_r->total_estimated_ms : 0.f;

    // fps: items per second (batch-aware)
    report.actual_fps    = (total_actual_ms > 0.f)
                           ? (float)batch_size * 1000.f / total_actual_ms : 0.f;
    report.estimated_fps = (report.total_estimated_ms > 0.f)
                           ? (float)batch_size * 1000.f / report.total_estimated_ms : 0.f;

    // Memory summary from static_r
    if (static_r) {
        report.memory = static_r->memory;
    } else {
        size_t tot_r = 0, tot_w = 0;
        for (const auto& r : records_) {
            tot_r += r.dma_read_bytes;
            tot_w += r.dma_write_bytes;
        }
        report.memory.total_dram_read_bytes  = tot_r;
        report.memory.total_dram_write_bytes = tot_w;
    }

    return report;
}

// ============================================================
// buildMultiLaneReport (static)
// ============================================================

MultiLanePerfReport MeasuredProfiler::buildMultiLaneReport(
        const std::vector<const MeasuredProfiler*>&           profilers,
        const DeviceTarget*                              device_target,
        const NPUComponent*                                 npu_component,
        const std::vector<std::string>&                      model_names,
        const std::vector<const std::vector<OpShape>*>&      all_shapes,
        const std::vector<uint8_t>&                          n_splits) {

    size_t N = profilers.size();
    uint8_t total_rows = (uint8_t)npu_component->getNPUSpecs().matrix_block.sa.rows;

    // Compute lane_rows from n_splits
    std::vector<uint8_t> rows;
    uint8_t prev = 0;
    for (uint8_t sp : n_splits) {
        rows.push_back(sp - prev);
        prev = sp;
    }
    rows.push_back(total_rows - prev);

    MultiLanePerfReport ml{};
    ml.summary.n_splits = n_splits;
    ml.summary.hw_parallel_supported = (int)N <= (int)npu_component->maxConcurrentModels();

    float wall_time = 0.f, serial_time = 0.f;

    for (size_t i = 0; i < N; ++i) {
        uint8_t lr = (i < rows.size()) ? rows[i] : total_rows;
        std::string name = (i < model_names.size()) ? model_names[i]
                           : ("model_" + std::to_string(i));
        const std::vector<OpShape>* sh = (i < all_shapes.size()) ? all_shapes[i] : nullptr;

        ModelPerfReport lane_rep = profilers[i]->buildReport(
            device_target, npu_component, name, (uint8_t)i, lr, sh, nullptr, 1);

        float lat = lane_rep.total_actual_ms;
        ml.summary.lane_latency_ms.push_back(lat);
        ml.summary.lane_util_pct.push_back(0.f);   // TODO: from mac_efficiency
        serial_time += lat;
        if (lat > wall_time) wall_time = lat;
        ml.lanes.push_back(std::move(lane_rep));
    }

    ml.summary.wall_time_ms     = wall_time;
    ml.summary.serial_time_ms   = serial_time;
    ml.summary.speedup_vs_serial= (wall_time > 0.f) ? serial_time / wall_time : 1.f;
    return ml;
}
