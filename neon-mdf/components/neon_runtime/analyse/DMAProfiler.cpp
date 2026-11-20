#include "DMAProfiler.h"
#include "../core/NPUComponent.h"
#include <chrono>
#include <algorithm>
#include <climits>

// ============================================================
// Timing
// ============================================================

uint64_t DMAProfiler::nowNs() const {
#if defined(__riscv) || defined(__arm__)
    return 0;
#else
    return static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count());
#endif
}

// ============================================================
// Hooks
// ============================================================

void DMAProfiler::onTransferStart(int channel,
                                   size_t bytes,
                                   DMADirection direction,
                                   uint32_t op_index,
                                   int8_t lane_override) {
    if (!enabled_ || channel < 0 || channel >= 8) return;

    auto& p      = pending_[channel];
    p.start_ns   = nowNs();
    p.bytes      = bytes;
    p.direction  = direction;
    p.op_index   = op_index;
    p.lane       = (lane_override >= 0) ? lane_override : inferLane(channel);
    p.active     = true;
}

void DMAProfiler::onTransferEnd(int channel) {
    if (!enabled_ || channel < 0 || channel >= 8) return;

    auto& p = pending_[channel];
    if (!p.active) return;

    DMAEvent ev{};
    ev.start_ns   = p.start_ns;
    ev.end_ns     = nowNs();
    ev.bytes      = p.bytes;
    ev.channel    = channel;
    ev.direction  = p.direction;
    ev.op_index   = p.op_index;
    ev.lane       = p.lane;
    events_.push_back(ev);
    p.active = false;
}

// ============================================================
// Global aggregates
// ============================================================

size_t DMAProfiler::totalReadBytes() const {
    size_t total = 0;
    for (const auto& e : events_)
        if (e.direction == DMADirection::READ) total += e.bytes;
    return total;
}

size_t DMAProfiler::totalWriteBytes() const {
    size_t total = 0;
    for (const auto& e : events_)
        if (e.direction == DMADirection::WRITE) total += e.bytes;
    return total;
}

// Peak in any 1ms window across all events
float DMAProfiler::peakBandwidthGBps() const {
    auto tl = buildTimeline(1.0f);
    float peak = 0.f;
    for (const auto& b : tl)
        if (b.read_gbps + b.write_gbps > peak)
            peak = b.read_gbps + b.write_gbps;
    return peak;
}

float DMAProfiler::avgBandwidthGBps() const {
    if (events_.empty()) return 0.f;
    uint64_t t_min = UINT64_MAX, t_max = 0;
    size_t total = 0;
    for (const auto& e : events_) {
        if (e.start_ns < t_min) t_min = e.start_ns;
        if (e.end_ns   > t_max) t_max = e.end_ns;
        total += e.bytes;
    }
    double dur_s = (t_max > t_min) ? (double)(t_max - t_min) * 1e-9 : 0.0;
    return (dur_s > 0.0) ? (float)(total / dur_s / 1e9) : 0.f;
}

// ============================================================
// channelStats
// ============================================================

std::vector<DMAChannelStats> DMAProfiler::channelStats() const {
    // Collect by channel
    std::vector<DMAChannelStats> result;
    const int n_ch = npu_component_ ? npu_component_->getNPUSpecs().num_dma_channels : 8;
    // Build map channel → aggregate
    for (int ch = 0; ch < std::min(n_ch, 8); ++ch) {
        bool any = false;
        size_t tot_r = 0, tot_w = 0;
        uint64_t t_min = UINT64_MAX, t_max = 0;
        float peak_r = 0.f, peak_w = 0.f;
        for (const auto& e : events_) {
            if (e.channel != ch) continue;
            any = true;
            double dur_s = (e.end_ns > e.start_ns)
                          ? (double)(e.end_ns - e.start_ns) * 1e-9 : 1e-9;
            float gbps = (float)(e.bytes / dur_s / 1e9);
            if (e.direction == DMADirection::READ) {
                tot_r += e.bytes;
                if (gbps > peak_r) peak_r = gbps;
            } else {
                tot_w += e.bytes;
                if (gbps > peak_w) peak_w = gbps;
            }
            if (e.start_ns < t_min) t_min = e.start_ns;
            if (e.end_ns   > t_max) t_max = e.end_ns;
        }
        if (!any) continue;

        double busy_s = 0.0;
        for (const auto& e : events_) {
            if (e.channel != ch) continue;
            busy_s += (double)(e.end_ns - e.start_ns) * 1e-9;
        }
        double wall_s = (t_max > t_min) ? (double)(t_max - t_min) * 1e-9 : 0.0;
        float busy_ms = (float)(busy_s * 1e3);
        float idle_ms = (float)((wall_s - busy_s) * 1e3);

        DMAChannelStats cs{};
        cs.channel            = ch;
        cs.total_read_bytes   = tot_r;
        cs.total_write_bytes  = tot_w;
        cs.peak_read_gbps     = peak_r;
        cs.peak_write_gbps    = peak_w;
        cs.busy_time_ms       = busy_ms;
        cs.idle_time_ms       = std::max(0.f, idle_ms);
        cs.utilisation_pct    = (wall_s > 0.0) ? (float)(busy_s / wall_s * 100.0) : 0.f;
        result.push_back(cs);
    }
    return result;
}

// ============================================================
// laneStats
// ============================================================

std::vector<DMALaneStats> DMAProfiler::laneStats() const {
    // Identify unique lanes
    std::vector<int8_t> lanes;
    for (const auto& e : events_) {
        bool found = false;
        for (auto l : lanes) if (l == e.lane) { found = true; break; }
        if (!found) lanes.push_back(e.lane);
    }
    std::sort(lanes.begin(), lanes.end());

    std::vector<DMALaneStats> result;
    for (int8_t lane : lanes) {
        size_t tot_r = 0, tot_w = 0;
        double busy_s = 0.0;
        uint64_t t_min = UINT64_MAX, t_max = 0;
        float peak_r = 0.f, peak_w = 0.f;

        for (const auto& e : events_) {
            if (e.lane != lane) continue;
            double dur_s = (e.end_ns > e.start_ns)
                          ? (double)(e.end_ns - e.start_ns) * 1e-9 : 1e-9;
            float gbps = (float)(e.bytes / dur_s / 1e9);
            if (e.direction == DMADirection::READ) {
                tot_r += e.bytes;
                if (gbps > peak_r) peak_r = gbps;
            } else {
                tot_w += e.bytes;
                if (gbps > peak_w) peak_w = gbps;
            }
            busy_s += dur_s;
            if (e.start_ns < t_min) t_min = e.start_ns;
            if (e.end_ns   > t_max) t_max = e.end_ns;
        }

        double wall_s = (t_max > t_min) ? (double)(t_max - t_min) * 1e-9 : 0.0;
        float avg_r = (busy_s > 0.0) ? (float)(tot_r / busy_s / 1e9) : 0.f;

        DMALaneStats ls{};
        ls.lane               = lane;
        ls.total_read_bytes   = tot_r;
        ls.total_write_bytes  = tot_w;
        ls.peak_read_gbps     = peak_r;
        ls.peak_write_gbps    = peak_w;
        ls.avg_read_gbps      = avg_r;
        ls.busy_time_ms       = (float)(busy_s * 1e3);
        ls.idle_time_ms       = (float)((wall_s - busy_s) * 1e3);
        ls.utilisation_pct    = (wall_s > 0.0) ? (float)(busy_s / wall_s * 100.0) : 0.f;
        result.push_back(ls);
    }
    return result;
}

// ============================================================
// per-op bytes
// ============================================================

size_t DMAProfiler::bytesForOp(uint32_t op_index) const {
    size_t total = 0;
    for (const auto& e : events_)
        if (e.op_index == op_index) total += e.bytes;
    return total;
}

size_t DMAProfiler::bytesForOpLane(uint32_t op_index, int8_t lane) const {
    size_t total = 0;
    for (const auto& e : events_)
        if (e.op_index == op_index && e.lane == lane) total += e.bytes;
    return total;
}

// ============================================================
// buildTimelineFiltered (internal)
// ============================================================

std::vector<DMAProfiler::TimelineBucket> DMAProfiler::buildTimelineFiltered(
        float window_ms,
        int8_t lane_filter) const {
    if (events_.empty()) return {};

    uint64_t t_min = UINT64_MAX, t_max = 0;
    for (const auto& e : events_) {
        if (lane_filter >= 0 && e.lane != lane_filter) continue;
        if (e.start_ns < t_min) t_min = e.start_ns;
        if (e.end_ns   > t_max) t_max = e.end_ns;
    }
    if (t_min == UINT64_MAX || t_min == t_max) return {};

    uint64_t window_ns = (uint64_t)(window_ms * 1e6f);
    size_t num_buckets = (size_t)((t_max - t_min + window_ns - 1) / window_ns) + 1;

    std::vector<double> read_bytes(num_buckets, 0.0);
    std::vector<double> write_bytes(num_buckets, 0.0);

    for (const auto& e : events_) {
        if (lane_filter >= 0 && e.lane != lane_filter) continue;
        // Distribute bytes across covered buckets proportionally
        uint64_t dur = (e.end_ns > e.start_ns) ? (e.end_ns - e.start_ns) : 1;
        size_t b_start = (size_t)((e.start_ns - t_min) / window_ns);
        size_t b_end   = (size_t)((e.end_ns   - t_min) / window_ns);
        b_end = std::min(b_end, num_buckets - 1);

        for (size_t b = b_start; b <= b_end; ++b) {
            uint64_t ws = t_min + b * window_ns;
            uint64_t we = ws + window_ns;
            uint64_t overlap_start = std::max(ws, e.start_ns);
            uint64_t overlap_end   = std::min(we, e.end_ns);
            if (overlap_start >= overlap_end) continue;
            double frac = (double)(overlap_end - overlap_start) / (double)dur;
            if (e.direction == DMADirection::READ)  read_bytes[b]  += e.bytes * frac;
            else                                     write_bytes[b] += e.bytes * frac;
        }
    }

    double window_s = window_ms * 1e-3;
    std::vector<TimelineBucket> tl;
    for (size_t b = 0; b < num_buckets; ++b) {
        if (read_bytes[b] == 0.0 && write_bytes[b] == 0.0) continue;
        TimelineBucket bucket{};
        bucket.time_ms    = (float)b * window_ms;
        bucket.read_gbps  = (float)(read_bytes[b]  / window_s / 1e9);
        bucket.write_gbps = (float)(write_bytes[b] / window_s / 1e9);
        tl.push_back(bucket);
    }
    return tl;
}

std::vector<DMAProfiler::TimelineBucket> DMAProfiler::buildTimeline(
        float window_ms) const {
    return buildTimelineFiltered(window_ms, -1);
}

std::vector<DMAProfiler::TimelineBucket> DMAProfiler::buildLaneTimeline(
        int8_t lane, float window_ms) const {
    return buildTimelineFiltered(window_ms, lane);
}

std::vector<std::vector<DMAProfiler::TimelineBucket>>
DMAProfiler::buildAllLaneTimelines(float window_ms) const {
    // Find all unique lanes
    std::vector<int8_t> lanes;
    for (const auto& e : events_) {
        bool found = false;
        for (auto l : lanes) if (l == e.lane) { found = true; break; }
        if (!found) lanes.push_back(e.lane);
    }
    std::sort(lanes.begin(), lanes.end());

    std::vector<std::vector<TimelineBucket>> all;
    for (int8_t lane : lanes)
        all.push_back(buildLaneTimeline(lane, window_ms));
    return all;
}
