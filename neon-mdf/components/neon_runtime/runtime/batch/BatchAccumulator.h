#pragma once

// ============================================================
// NEON STUDIO — BATCH ACCUMULATOR  (Rev3)
//
// Collects single-item inference requests from multiple threads
// and flushes them as a batch when a flush condition is met.
//
// Design:
//   - Producers (caller threads) call push() — non-blocking, returns
//     false under backpressure (queue full).
//   - One consumer (runner thread) calls pop_batch() in a loop.
//     pop_batch() blocks until flush conditions are met, then
//     returns a BatchFlushResult ready for ProfileSelector + Executor.
//
// Flush conditions (evaluated in order):
//   1. queue.size() >= max_batch_size  → flush immediately
//   2. queue.size() >= min_batch_size AND oldest item age >= max_wait_us
//      → flush on timeout (prevents starvation of early arrivals)
//
// After receiving a BatchFlushResult:
//   1. ProfileSelector::select() picks the best BatchProfile
//   2. ProfileSelector::pack_inputs() builds the batched input buffer
//   3. Executor::run_batch() executes the profile stream once
//   4. ProfileSelector::unpack_outputs() distributes results to callers
//   5. Call on_complete(ok) on each InferenceRequest
//
// Thread safety: all public methods are thread-safe.
//
// Baremetal note: this class uses std::mutex / std::condition_variable
// and requires an OS or RTOS. For baremetal polling variants, use a
// single-threaded loop with BatchAccumulator::try_pop_batch() instead.
// ============================================================

#include "../NeonPortability.h"
#include <cstdint>
#include <cstddef>
#include <cstring>
#include <deque>
#include <vector>
#include <functional>
#include <chrono>
#include <atomic>
#if NEON_HAS_CONDVAR
#include <condition_variable>
#endif

#ifdef __cplusplus
extern "C++" {

// ============================================================
// INFERENCE REQUEST
// One pending inference job from a caller.
// Caller owns input_data and output_data — must remain valid
// until on_complete() is called.
// ============================================================

struct InferenceRequest {
    const void*   input_data         = nullptr;  // caller's input buffer
    void*         output_data        = nullptr;  // caller's output buffer
    size_t        input_size_bytes   = 0;
    size_t        output_size_bytes  = 0;

    uint32_t      priority           = 0;        // higher = more urgent
                                                 // (BatchAccumulator does NOT sort
                                                 //  by priority — use a priority queue
                                                 //  wrapper if needed)

    // Set by BatchAccumulator::push() — used for max_wait_us timeout.
    uint64_t      enqueue_time_us    = 0;

    // Called after Executor completes (or fails) for this item.
    // ok = true on success, false on hardware error.
    // Called from runner thread — must be non-blocking.
    std::function<void(bool ok)> on_complete;
};

// ============================================================
// BATCH FLUSH RESULT
// What pop_batch() returns to the runner thread.
// ============================================================

struct BatchFlushResult {
    std::vector<InferenceRequest> requests;   // n_actual items
    uint32_t                      n_actual;   // == requests.size()

    bool empty() const { return n_actual == 0; }
};

// ============================================================
// BATCH ACCUMULATOR
// ============================================================

class BatchAccumulator {
public:
    struct Config {
        // Flush immediately when queue reaches this size.
        // Should match the largest available BatchProfile.batch_size.
        uint32_t max_batch_size   = 32;

        // Do not flush below this count unless max_wait_us elapses.
        // Set to 1 to flush as soon as any item arrives (lowest latency).
        // Set to max_batch_size for maximum throughput (highest latency).
        uint32_t min_batch_size   = 1;

        // Maximum time (µs) an item may wait before forcing a flush,
        // even if min_batch_size has not been reached.
        // 0 = no timeout (wait indefinitely for min_batch_size).
        uint32_t max_wait_us      = 5000;   // 5 ms default

        // Hard cap on internal queue depth.
        // push() returns false (backpressure) when queue reaches this.
        uint32_t max_queue_depth  = 256;
    };

    BatchAccumulator()             : config_(Config{}) {}
    explicit BatchAccumulator(Config cfg) : config_(cfg) {}

    ~BatchAccumulator() { shutdown(); }

    // ── Producer API (call from any thread) ───────────────────────────────

    // Enqueue one inference request.
    // Returns true on success, false if queue is at max_queue_depth
    // (caller should retry or apply backpressure upstream).
    bool push(InferenceRequest req) {
        {
            NeonLockGuard lk(mutex_);
            if (shutdown_) return false;
            if (queue_.size() >= config_.max_queue_depth) return false;

            req.enqueue_time_us = now_us();
            queue_.push_back(std::move(req));
            ++total_pushed_;
        }
#if NEON_HAS_CONDVAR
        cv_.notify_one();
#endif
        return true;
    }

    // ── Consumer API (call from runner thread only) ────────────────────────

#if NEON_HAS_CONDVAR
    // Block until flush condition is met, then return a batch.
    // Returns an empty result only after shutdown() and queue is drained.
    // Requires an OS/RTOS (__GTHREADS) — see try_pop_batch() for baremetal.
    BatchFlushResult pop_batch() {
        using clk = std::chrono::steady_clock;
        NeonUniqueLock lk(mutex_);

        // Phase 1: wait until we have at least min_batch_size items, or shutdown.
        cv_.wait(lk, [this] {
            return shutdown_ || queue_.size() >= config_.min_batch_size;
        });

        if (queue_.empty()) return {};   // shutdown + empty

        // Phase 2: if we're below max_batch and max_wait_us is set,
        // wait a bit more for the batch to fill up — measured from the
        // oldest item's enqueue time to avoid unbounded starvation.
        if (config_.max_wait_us > 0
            && queue_.size() < config_.max_batch_size)
        {
            uint64_t oldest = queue_.front().enqueue_time_us;
            uint64_t elapsed_us = now_us() - oldest;

            if (elapsed_us < config_.max_wait_us) {
                auto remaining = std::chrono::microseconds(
                    config_.max_wait_us - elapsed_us);
                cv_.wait_for(lk, remaining, [this] {
                    return shutdown_
                        || queue_.size() >= config_.max_batch_size;
                });
            }
        }

        // Phase 3: collect up to max_batch_size items.
        BatchFlushResult result;
        uint32_t take = static_cast<uint32_t>(
            std::min(queue_.size(),
                     static_cast<size_t>(config_.max_batch_size)));
        result.requests.reserve(take);
        for (uint32_t i = 0; i < take; ++i) {
            result.requests.push_back(std::move(queue_.front()));
            queue_.pop_front();
        }
        result.n_actual = take;
        total_flushed_ += take;
        ++total_batches_;
        return result;
    }
#endif // NEON_HAS_CONDVAR

    // Non-blocking variant for baremetal polling loops.
    // Returns an empty result if flush condition is not yet met.
    BatchFlushResult try_pop_batch() {
        NeonLockGuard lk(mutex_);

        bool should_flush =
            queue_.size() >= config_.max_batch_size
            || (queue_.size() >= config_.min_batch_size
                && config_.max_wait_us > 0
                && (now_us() - queue_.front().enqueue_time_us)
                   >= config_.max_wait_us);

        if (!should_flush) return {};

        BatchFlushResult result;
        uint32_t take = static_cast<uint32_t>(
            std::min(queue_.size(),
                     static_cast<size_t>(config_.max_batch_size)));
        result.requests.reserve(take);
        for (uint32_t i = 0; i < take; ++i) {
            result.requests.push_back(std::move(queue_.front()));
            queue_.pop_front();
        }
        result.n_actual = take;
        total_flushed_ += take;
        ++total_batches_;
        return result;
    }

    // Signal runner thread to exit. Pending items are still drained.
    void shutdown() {
        { NeonLockGuard lk(mutex_); shutdown_ = true; }
#if NEON_HAS_CONDVAR
        cv_.notify_all();
#endif
    }

    // ── Stats (lock-free reads — approximate) ─────────────────────────────

    size_t   pending_count()  const { NeonLockGuard lk(mutex_); return queue_.size(); }
    uint64_t total_pushed()   const { return total_pushed_.load(std::memory_order_relaxed); }
    uint64_t total_flushed()  const { return total_flushed_.load(std::memory_order_relaxed); }
    uint64_t total_batches()  const { return total_batches_.load(std::memory_order_relaxed); }

    // Average batch size over all flushes (0 if no batches yet).
    float avg_batch_size() const {
        uint64_t b = total_batches_.load(std::memory_order_relaxed);
        uint64_t f = total_flushed_.load(std::memory_order_relaxed);
        return b == 0 ? 0.0f : static_cast<float>(f) / static_cast<float>(b);
    }

    const Config& config() const { return config_; }

private:
    static uint64_t now_us() {
        using namespace std::chrono;
        return static_cast<uint64_t>(
            duration_cast<microseconds>(
                steady_clock::now().time_since_epoch()).count());
    }

    Config                        config_;
    std::deque<InferenceRequest>  queue_;
    mutable NeonMutex             mutex_;
#if NEON_HAS_CONDVAR
    std::condition_variable       cv_;
#endif
    bool                          shutdown_      = false;

    std::atomic<uint64_t>         total_pushed_  {0};
    std::atomic<uint64_t>         total_flushed_ {0};
    std::atomic<uint64_t>         total_batches_ {0};
};

} // extern "C++"
#endif
