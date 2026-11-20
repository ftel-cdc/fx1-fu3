#pragma once

// ============================================================
// NEON STUDIO — NEON RUNTIME (public API)
// Single header for end users and apps/examples/.
//
// ── Single-model usage (any hardware) ────────────────────────
//   auto* model = neon::load("model.nsm", "VP_FX1_V2.0");
//   neon::run(model, input, output);
//   neon::unload(model);
//
// ── Async submit — frame-driven, independent timing ───────────
//   auto* s_a = neon::createSession("vit_b.nsm",   "VP_FX1_V2.0");
//   auto* s_b = neon::createSession("yolov8m.nsm",  "VP_FX1_V2.0");
//   // Frame for model A arrives:
//   neon::submit(s_a, input_a, output_a, cb_a);
//   // Frame for model B arrives (any time, independently):
//   neon::submit(s_b, input_b, output_b, cb_b);
//   // Runtime routes to hardware spatial split (FX1) or software
//   // time-slice (single-model hardware) — caller doesn't care.
//
// For advanced use (profiling, multi-thread): use Executor directly.
// ============================================================

#include "loader/NPUModel.h"
#include "loader/ModelLoader.h"
#include "executor/Executor.h"
#include "driver/NPUDriver.h"
#include "../core/NPUComponent.h"
#include "../core/DeviceTarget.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C++" {

// ============================================================
// NPU SESSION
// Binds a loaded model to a npu_driver instance and tracks per-session
// state (SRAM region, LUT init, current n_split, lane assignment).
//
// Sessions are created via neon::createSession() / neon::loadDual().
// For single-model hardware: sessions are serialized internally.
// For FX1 (maxConcurrentModels=2): two sessions run in true parallel
// on Lane A / Lane B.
// ============================================================

struct NPUSession {
    uint32_t      session_id;
    NPUModel*     model;
    NPUDriver* npu_driver;

    // Identity + spec side of the target (may be nullptr if the registry
    // lookup in createSession() failed to resolve one — always check
    // before use). device_target->getName() is the single source of truth
    // for the device name; npu_component->maxConcurrentModels() etc. for
    // NPU specs — prefer these over npu_driver's own copies.
    const DeviceTarget*  device_target;
    const NPUComponent*  npu_component;

    // Lane assignment on hardware with spatial split.
    // 0 = Lane A (rows 0..n_split-1)
    // 1 = Lane B (rows n_split..63)
    // 255 = not assigned yet (single-model or pre-submit)
    uint8_t  lane;

    // Current N_split in effect for this session pair.
    // 64 = single-model mode (full SA).
    // Set by runtime when submit() is called with a peer session active.
    uint8_t  n_split;

    // Peer session — the other model running concurrently on the same device.
    // nullptr = solo session (or not yet paired).
    NPUSession* peer;
};

// ============================================================
// HIGH-LEVEL API — neon namespace
// ============================================================

namespace neon {

    // ── Single-model API (unchanged from v1) ─────────────────────────────

    // Load model, create internal session, open npu_driver.
    // Equivalent to createSession() + neon::run() convenience wrapper.
    NPUModel* load(const char* model_path,
                   const char* target_name);

    // Blocking inference. On FX1 with N_split=64: full SA, same as Rev1.
    NPUDriverStatus run(NPUModel*   model,
                     const void* input,
                     void*       output);

    void unload(NPUModel* model);

    size_t   inputSize (const NPUModel* model);
    size_t   outputSize(const NPUModel* model);
    uint32_t numOps    (const NPUModel* model);

    // ── Session-based API (concurrent / frame-driven) ────────────────────

    // Create a session for one model.
    // Does NOT open npu_driver or lock a lane yet — deferred until first submit().
    NPUSession* createSession(const char* model_path,
                              const char* target_name);

    // Destroy session and release all resources.
    void destroySession(NPUSession* session);

    // Async submit: push inference request for session.
    // callback(session_id, status) is called when this session's lane is done.
    // Pass callback=nullptr for fire-and-forget (use waitSession() instead).
    //
    // Runtime decision at submit time:
    //   - If peer session is currently running → enable spatial split,
    //     assign lanes, call npu_driver->setNSplit() + npu_driver->submitConcurrent()
    //   - If solo → run full SA (N_split=64), npu_driver->trigger() as usual
    //   - If hardware is single-model → always serialize regardless
    //
    // Thread-safe: sessions may call submit() from different threads.
    NPUDriverStatus submit(NPUSession* session,
                        const void* input,
                        void*       output,
                        void (*callback)(uint32_t session_id,
                                         NPUDriverStatus status) = nullptr);

    // Block until the given session's most recent submit() completes.
    // Returns immediately if no pending submit.
    NPUDriverStatus waitSession(NPUSession* session,
                             uint32_t    timeout_ms = 0);

    // ── Convenience: load two paired models ──────────────────────────────
    // Creates two sessions and links them as peers.
    // On FX1: runtime pre-computes N_split from layer shape metadata
    // embedded in each .nsm binary (brute-force 1..63, trivial cost).
    // On single-model hardware: sessions are serialized transparently.
    //
    //   auto [sa, sb] = neon::loadDual("vit_b.nsm", "yolov8m.nsm", "VP_FX1_V2.0");
    //   neon::submit(sa, input_a, output_a, cb_a);
    //   neon::submit(sb, input_b, output_b, cb_b);
    struct DualSession { NPUSession* a; NPUSession* b; };
    DualSession loadDual(const char* model_path_a,
                         const char* model_path_b,
                         const char* target_name);

    // Unload both sessions from a DualSession.
    void unloadDual(DualSession sessions);

    // ── Low-level access (for direct Executor use) ────────────────────────
    //
    // Return the registered NPUComponent / NPUDriver for target_name.
    // Returns nullptr if the target npu_driver has not been linked in.
    // Caller does NOT own the returned pointer — lifetime is static.
    //
    // Usage (example [D]):
    //   Executor exec(model, neon::getNPUTarget("VP_FX1_V2.0"),
    //                         neon::getNPUDriver("VP_FX1_V2.0"));
    const NPUComponent* getNPUTarget(const char* target_name);
    NPUDriver*         getNPUDriver(const char* target_name);

} // namespace neon

} // extern "C++"
#endif
