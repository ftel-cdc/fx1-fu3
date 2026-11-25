#pragma once

// ============================================================
// NEON STUDIO — RUNTIME PROFILER
// Measures actual op latency on hardware.
// Hooked into runtime/executor — called before/after each op.
//
// ── Timing source strategy ────────────────────────────────────
//
//   HW_CYCLE_COUNTER (preferred — TLM, FPGA, silicon):
//     Executor reads npu_driver->readPerfTotalCycles() before and after each op.
//     The delta is the ground-truth cycle count for that op.
//     latency_ms = cycles / clock_mhz / 1000
//     mac_efficiency = theoretical_flops / (cycles × peak_flops_per_cycle)
//
//   HOST_CLOCK (fallback — pure SW when readPerfTotalCycles() returns 0):
//     std::chrono::steady_clock measures host wall-clock time.
//     Meaningless on TLM (simulation ≠ wall time), rough on FPGA.
//     No mac_efficiency computable (cycles unknown).
//
// ── Single-model ──────────────────────────────────────────────
//   MeasuredProfiler profiler;
//   profiler.enable();
//   // ... Executor runs model, calls onOpStart/onOpEnd per op ...
//   auto report = profiler.buildReport(target, "model.nsm", shapes);
//   PerfReportPrinter::print(report);
//
// ── Multi-lane (one profiler per session) ─────────────────────
//   MeasuredProfiler pa, pb;
//   pa.enable(); pb.enable();
//   // ... Executor(sa) uses pa, Executor(sb) uses pb ...
//   auto ml = MeasuredProfiler::buildMultiLaneReport(
//       {&pa, &pb}, target, model_names, shapes_all, n_splits);
//   PerfReportPrinter::printMultiLane(ml);
// ============================================================

#include "PerfReport.h"
#include "StaticAnalyzer.h"
#include "../core/NPU_ISA.h"
#include "../core/NPUComponent.h"
#include "../core/DeviceTarget.h"

#ifdef __cplusplus
extern "C++" {

#include <chrono>

// One before/after read of every counter Executor knows how to fetch from
// npu_driver_. Fields not exposed by a given driver stay 0 (NPUDriver's
// documented "0 == absent" contract).
struct PerfCounterSnapshot {
    uint64_t total = 0, stall = 0, sa = 0, obp = 0;
    uint64_t mac_engine = 0, dma_engine = 0;
    uint64_t act_engine = 0, pool_engine = 0, red_engine = 0;
    uint64_t dma_read = 0, dma_write = 0;
    uint64_t rich_m = 0, rich_k = 0, rich_n = 0;
};

// ============================================================
// OP TIMING RECORD
// ============================================================

struct OpTimingRecord {
    uint32_t    op_index;
    NeonOpcode  opcode;
    std::string op_name;

    // Which SA lane this op ran on.
    // 255 = single-model mode (full SA, no split)
    // 0   = Lane A  (rows 0..n_split-1)
    // N   = Lane N  for future hardware
    uint8_t     lane;

    // ── Timing ───────────────────────────────────────────────

    // Source tells the consumer how to interpret the timing fields.
    LayerPerfRecord::TimingSource timing_src;

    // HW_CYCLE_COUNTER path:
    uint64_t    cycles;             // delta cycles for this op (0 if host clock)
    uint64_t    stall_cycles;       // pipeline stalls during this op (0 if N/A)
    uint64_t    sa_cycles;          // systolic array active during this op (0 if N/A)
    uint64_t    obp_cycles;         // OBP active during this op (0 if N/A)

    // ── V4.5 engine-activity counters (added: VP-measured → device_estimation plan) ──
    uint64_t    mac_engine_cycles;
    uint64_t    dma_engine_cycles;
    uint64_t    act_engine_cycles;
    uint64_t    pool_engine_cycles;
    uint64_t    red_engine_cycles;
    uint64_t    dma_read_cycles;
    uint64_t    dma_write_cycles;

    // Hardware-reported rich-op M/K/N — cross-check against OpShape, not a
    // performance metric. 0 if this op wasn't a rich instruction or the
    // driver doesn't expose it.
    uint64_t    hw_m, hw_k, hw_n;

    // HOST_CLOCK path (fallback):
    uint64_t    start_ns;           // host timestamp before dispatch
    uint64_t    end_ns;             // host timestamp after NPU done

    // Derived (set by buildReport):
    float       latency_ms;         // from cycles/clock or (end-start)/1e6

    // ── DMA ─────────────────────────────────────────────────
    size_t      dma_read_bytes;     // bytes DMA'd host→device for this op
    size_t      dma_write_bytes;    // bytes DMA'd device→host
};

// ============================================================
// RUNTIME PROFILER
// ============================================================

class MeasuredProfiler {
public:
    MeasuredProfiler() = default;

    void enable()  { enabled_ = true;  records_.clear(); }
    void disable() { enabled_ = false; }
    bool isEnabled() const { return enabled_; }

    // ── Hooks — called by runtime/executor ───────────────────

    // Call immediately before dispatching op to NPU.
    //   lane   : 255 = single-model, 0..N = lane index for concurrent
    //   before : snapshot of counters read from npu_driver just before trigger()
    void onOpStart(uint32_t                    op_index,
                   NeonOpcode                  opcode,
                   const std::string&          op_name,
                   uint8_t                     lane = 255,
                   const PerfCounterSnapshot&  before = {});

    // Call immediately after NPU signals done (after waitDone()).
    //   dma_read/write : bytes transferred for this op (from DMAProfiler)
    //   after          : snapshot of counters read from npu_driver after waitDone()
    void onOpEnd(size_t                       dma_read_bytes  = 0,
                 size_t                       dma_write_bytes = 0,
                 const PerfCounterSnapshot&   after = {});

    // ── Single-model report ───────────────────────────────────

    // Build ModelPerfReport from recorded timing.
    //
    // npu_component : used for clock_mhz (cycles → ms) and peak_flops_per_cycle
    // model_name : label for the report
    // lane       : 255 = single-model; 0..N = this profiler's lane
    // lane_rows  : SA rows for this lane (0 = full SA for single-model)
    // shapes     : if provided, computes theoretical_flops → mac_efficiency_pct
    //              if nullptr, mac_efficiency_pct = 0 (cycles still recorded)
    // static_r   : if provided, merge static estimates into the report
    // batch_size : items per batch execution (Rev3 batch profiles).
    //              1  = single inference (default, batch=1 stream)
    //              N  = batch inference; actual_fps = N * 1000 / total_actual_ms
    //              Caller sets this when using Executor::run_batch() so that
    //              reported throughput is items/sec, not batches/sec.
    ModelPerfReport buildReport(
        const DeviceTarget*           device_target,
        const NPUComponent*           npu_component,
        const std::string&            model_name,
        uint8_t                       lane       = 255,
        uint8_t                       lane_rows  = 0,
        const std::vector<OpShape>*   shapes     = nullptr,
        const ModelPerfReport*        static_r   = nullptr,
        uint32_t                      batch_size = 1
    ) const;

    // ── Multi-lane report ─────────────────────────────────────

    // Build MultiLanePerfReport from N profilers (one per lane).
    //
    // profilers[i]   : MeasuredProfiler for lane i
    // npu_component  : shared target (same hardware for all lanes)
    // model_names[i] : label for lane i
    // all_shapes[i]  : op shapes for lane i (for mac_efficiency; may be nullptr)
    // n_splits       : N-1 boundary points (row partition)
    //
    // wall_time_ms   = max(profiler[i].totalLatencyMs())
    // speedup        = sum(totalLatencyMs) / wall_time_ms
    static MultiLanePerfReport buildMultiLaneReport(
        const std::vector<const MeasuredProfiler*>&           profilers,
        const DeviceTarget*                                   device_target,
        const NPUComponent*                                   npu_component,
        const std::vector<std::string>&                       model_names,
        const std::vector<const std::vector<OpShape>*>&       all_shapes,
        const std::vector<uint8_t>&                           n_splits);

    // ── Raw accessors ─────────────────────────────────────────

    const std::vector<OpTimingRecord>& records() const { return records_; }
    size_t recordCount() const { return records_.size(); }
    void clear() { records_.clear(); }

    // Sum of latency_ms across all ops (set during buildReport or via onOpEnd)
    float totalLatencyMs() const;

    // Total cycle count across all ops (0 if using host clock)
    uint64_t totalCycles() const;

    // true if at least one op was timed via HW cycle counter
    bool hasCycleData() const;

private:
    bool                          enabled_      = false;
    std::vector<OpTimingRecord>   records_;

    // Pending record while op is in-flight
    OpTimingRecord      pending_{};
    PerfCounterSnapshot pending_before_{};
    bool                op_in_flight_ = false;

    uint64_t nowNs() const {
#if defined(__riscv) || defined(__arm__)
        return 0;
#else
        return std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count();
#endif
    }
};

} // extern "C++"
#endif
