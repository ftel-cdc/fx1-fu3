#pragma once

// ============================================================
// NEON STUDIO — DMA PROFILER
// Tracks DMA transfer events from hardware counters.
// Works alongside MeasuredProfiler — provides byte counts
// that MeasuredProfiler stores in OpTimingRecord.
//
// ── Single-model ──────────────────────────────────────────────
//   DMAProfiler dma;
//   dma.enable();
//   // npu_driver calls dma.onTransferStart / onTransferEnd around each DMA
//   auto bw = dma.peakBandwidthGBps();
//
// ── Multi-lane ────────────────────────────────────────────────
//   // Lane is inferred from DMA channel (fixed assignment, Rev2):
//   //   CH0 / CH2  → lane 0 (Lane A)
//   //   CH1 / CH3  → lane 1 (Lane B)
//   // For >2 lanes: CH(2i) / CH(2i+1) → lane i (convention)
//   auto stats = dma.laneStats();        // per-lane aggregate
//   auto tl    = dma.buildLaneTimeline(0); // BW over time for lane 0
// ============================================================

#include "../core/NPU_ISA.h"
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C++" {

#include <vector>

// ============================================================
// DMA EVENT
// ============================================================

enum class DMADirection { READ, WRITE };

struct DMAEvent {
    uint64_t     start_ns;
    uint64_t     end_ns;
    size_t       bytes;
    int          channel;           // DMA channel index (0-based)
    DMADirection direction;
    uint32_t     op_index;          // which op triggered this transfer

    // Lane inferred from channel at record time.
    // Convention (FX1 Rev2 fixed assignment):
    //   channel % 2 == 0 → lane 0 (Lane A: CH0=weight, CH2=ifmap)
    //   channel % 2 == 1 → lane 1 (Lane B: CH1=weight, CH3=ifmap)
    // For future hardware: lanes may have more than 2 channels each.
    // -1 = unknown / single-model mode
    int8_t       lane;
};

// ============================================================
// PER-CHANNEL STATS (existing, unchanged)
// ============================================================

struct DMAChannelStats {
    int     channel;
    size_t  total_read_bytes;
    size_t  total_write_bytes;
    float   peak_read_gbps;
    float   peak_write_gbps;
    float   busy_time_ms;
    float   idle_time_ms;
    float   utilisation_pct;        // busy / (busy + idle)
};

// ============================================================
// PER-LANE STATS (new — multi-lane aggregate)
// ============================================================

struct DMALaneStats {
    int8_t  lane;                   // 0-based lane index
    size_t  total_read_bytes;       // sum across all channels of this lane
    size_t  total_write_bytes;
    float   peak_read_gbps;         // peak across 1ms windows
    float   peak_write_gbps;
    float   avg_read_gbps;
    float   busy_time_ms;
    float   idle_time_ms;
    float   utilisation_pct;        // busy / (busy + idle)
};

// ============================================================
// DMA PROFILER
// ============================================================

class NPUComponent;

class DMAProfiler {
public:
    explicit DMAProfiler(const NPUComponent* npu_component = nullptr)
        : npu_component_(npu_component) {}

    void setNPUComponent(const NPUComponent* comp) { npu_component_ = comp; }

    void enable()  { enabled_ = true;  events_.clear(); }
    void disable() { enabled_ = false; }
    bool isEnabled() const { return enabled_; }

    // ── Hooks — called by Executor / NPUDriver ─────────────────────

    // Call when DMA transfer starts.
    // lane is inferred from channel automatically (channel % 2).
    // Pass lane_override >= 0 to set explicitly (for non-standard hardware).
    void onTransferStart(int channel,
                         size_t bytes,
                         DMADirection direction,
                         uint32_t op_index,
                         int8_t lane_override = -1);

    // Call when DMA transfer completes
    void onTransferEnd(int channel);

    // ── Aggregated metrics — global ───────────────────────────

    // Total bytes transferred since last enable()
    size_t totalReadBytes()  const;
    size_t totalWriteBytes() const;

    // Peak bandwidth observed in any 1ms window (all channels combined)
    float  peakBandwidthGBps()  const;
    float  avgBandwidthGBps()   const;

    // ── Per-channel breakdown ─────────────────────────────────

    std::vector<DMAChannelStats> channelStats() const;

    // ── Per-lane breakdown (multi-lane) ──────────────────────

    // Aggregate transfer stats grouped by lane.
    // Returns one DMALaneStats per unique lane seen in events.
    std::vector<DMALaneStats> laneStats() const;

    // ── Per-op ───────────────────────────────────────────────

    // Bytes transferred for a specific op (all channels / all lanes)
    size_t bytesForOp(uint32_t op_index) const;

    // Bytes transferred for a specific op on a specific lane
    size_t bytesForOpLane(uint32_t op_index, int8_t lane) const;

    // ── Raw access ────────────────────────────────────────────

    const std::vector<DMAEvent>& events() const { return events_; }
    void  clear() { events_.clear(); }

    // ── Timelines ────────────────────────────────────────────

    // Global BW timeline (all channels combined, same as before)
    struct TimelineBucket {
        float time_ms;
        float read_gbps;
        float write_gbps;
    };
    std::vector<TimelineBucket> buildTimeline(float window_ms = 1.0f) const;

    // Per-lane BW timeline — shows traffic of one lane over time.
    // Useful for visualising overlap between lane 0 and lane 1.
    //   auto tl_a = dma.buildLaneTimeline(0, 0.5f);
    //   auto tl_b = dma.buildLaneTimeline(1, 0.5f);
    std::vector<TimelineBucket> buildLaneTimeline(int8_t lane,
                                                  float window_ms = 1.0f) const;

    // Convenience: build timelines for all lanes in one call.
    // Returns timelines[i] for lane i (sorted by lane index).
    std::vector<std::vector<TimelineBucket>> buildAllLaneTimelines(
        float window_ms = 1.0f) const;

private:
    const NPUComponent*     npu_component_ = nullptr;
    bool                    enabled_ = false;
    std::vector<DMAEvent>   events_;

    // Pending events per channel (in-flight transfers)
    struct PendingTransfer {
        uint64_t     start_ns;
        size_t       bytes;
        DMADirection direction;
        uint32_t     op_index;
        int8_t       lane;
        bool         active = false;
    };
    // Support up to 8 DMA channels (future-proof beyond 4)
    PendingTransfer pending_[8];

    // Infer lane from channel index using standard convention
    static int8_t inferLane(int channel) {
        return static_cast<int8_t>(channel % 2);
    }

    uint64_t nowNs() const;

    // Internal: build timeline filtered by predicate
    std::vector<TimelineBucket> buildTimelineFiltered(
        float window_ms,
        int8_t lane_filter  // -1 = all lanes
    ) const;
};

} // extern "C++"
#endif
