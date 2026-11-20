#pragma once

// ============================================================
// NEON STUDIO — threading portability shim
//
// Baremetal single-core targets (arm-none-eabi newlib without an RTOS)
// have no gthreads backend, so <mutex>/<condition_variable> aren't
// available at all. There is no concurrent access to shared state
// without an RTOS/ISR touching it on those targets, so the no-op
// stand-ins below are the correct behavior there, not a workaround.
//
// NEON_HAS_CONDVAR gates APIs that fundamentally require a second
// thread to make progress (e.g. BatchAccumulator::pop_batch()) —
// those simply aren't available without __GTHREADS.
// ============================================================

#if defined(__GTHREADS)
#include <mutex>
using NeonMutex      = std::mutex;
using NeonLockGuard   = std::lock_guard<std::mutex>;
using NeonUniqueLock  = std::unique_lock<std::mutex>;
#define NEON_HAS_CONDVAR 1
#else
struct NeonMutex {
    void lock()   {}
    void unlock() {}
};
struct NeonLockGuard {
    explicit NeonLockGuard(NeonMutex& m) : m_(m) { m_.lock(); }
    ~NeonLockGuard() { m_.unlock(); }
    NeonMutex& m_;
};
struct NeonUniqueLock {
    explicit NeonUniqueLock(NeonMutex& m) : m_(m) { m_.lock(); }
    ~NeonUniqueLock() { m_.unlock(); }
    NeonMutex& m_;
};
#define NEON_HAS_CONDVAR 0
#endif
