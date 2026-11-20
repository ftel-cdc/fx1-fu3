#include "NeonRuntime.h"
#include "loader/ModelLoader.h"
#include "executor/Executor.h"
#include "../core/NPUComponent.h"
#include "NeonPortability.h"
#include <cstring>
#include <cstdio>
#include <atomic>
#include <map>

// ============================================================
// Target lookup — resolves through DeviceTargetRegistry (core/DeviceTarget.h).
//
// Each device's runtime .cpp (e.g. QEMUMPS3Runtime.cpp) calls
// NEON_REGISTER_TARGET(...) at static-init time, which constructs ONE
// process-wide singleton instance and registers it into
// DeviceTargetRegistry::get(). That instance is simultaneously a
// DeviceTarget, an NPUComponent and an NPUDriver (multiple inheritance —
// see NPUV4Runtime.h/QEMUMPS3Runtime.h), so a single lookup() plus
// dynamic_cast recovers every view NeonRuntime needs.
//
// This file deliberately never includes a device header (NPUV4Runtime.h,
// QEMUMPS3Runtime.h) — doing so would create the exact runtime/ → device/
// upward dependency NeonStudioArchitecture.md's layering forbids. The
// registry lives in core/, which runtime/ already depends on, so this
// resolution works purely through the DeviceTarget/NPUComponent/NPUDriver
// base-class views, with the concrete type only ever materializing inside
// each device's own .cpp.
//
// Returned pointers are the registry's singleton — callers must NEVER
// delete them (see NeonRuntime.h's getNPUDriver()/getNPUTarget() doc).
// A target linked into the binary but never dispatched to an NPU (e.g.
// linked for static analysis only) still registers, so a null result here
// means "not linked in", not "not opened yet".
// ============================================================

static NPUDriver* createNPUDriver(const char* target_name) {
    DeviceTarget* t = DeviceTargetRegistry::get().lookup(target_name);
    if (!t) {
        fprintf(stderr, "[WARN] NeonRuntime: createNPUDriver('%s') — "
                "no device npu_driver linked. "
                "Link the device runtime (e.g. NPUV4Runtime.o, QEMUMPS3Runtime.o).\n",
                target_name);
        return nullptr;
    }
    NPUDriver* drv = dynamic_cast<NPUDriver*>(t);
    if (!drv) {
        fprintf(stderr, "[WARN] NeonRuntime: target '%s' is registered but "
                "does not implement NPUDriver.\n", target_name);
    }
    return drv;
}

static const NPUComponent* createTarget(const char* target_name) {
    DeviceTarget* t = DeviceTargetRegistry::get().lookup(target_name);
    return t ? dynamic_cast<const NPUComponent*>(t) : nullptr;
}

static const DeviceTarget* createIdentity(const char* target_name) {
    return DeviceTargetRegistry::get().lookup(target_name);
}

// ============================================================
// Internal session state
// ============================================================

static std::atomic<uint32_t> g_next_session_id{1};
static NeonMutex              g_session_mutex;

// One inference asked for by submit() but not yet executed.
//
// Dual-lane execution needs BOTH models' instruction streams in hand at the
// same moment — NPUDriver::submitConcurrent() takes queue_a and queue_b
// together, because the point is that the array runs them side by side on
// its two row partitions. A submit() on one session therefore cannot
// execute immediately without giving up the split, so it parks here and the
// partner's submit() runs the pair. See submit() for the full protocol.
struct PendingRequest {
    bool            parked    = false;   // submitted, waiting for the peer
    const void*     input     = nullptr;
    void*           output    = nullptr;
    void          (*callback)(uint32_t, NPUDriverStatus) = nullptr;

    bool            completed = false;   // has run; status is meaningful
    NPUDriverStatus status    = NPUDriverStatus::OK;
};

// Simple map of session_id → (Executor*, NPUModel*)
struct InternalSession {
    Executor*     executor = nullptr;
    NPUModel*     model    = nullptr;
    NPUDriver* npu_driver   = nullptr;
    uint32_t      session_id = 0;
    PendingRequest request{};
};

static std::map<uint32_t, InternalSession> g_sessions;

// ============================================================
// neon::load()/run()/unload() state — keyed by NPUModel*, not session_id.
//
// load() is documented (NeonRuntime.h) as "equivalent to createSession() +
// neon::run() convenience wrapper": it must remember which Executor/NPUDriver
// a given NPUModel was bound to, so a later neon::run(model, ...) dispatches
// through the real device instead of silently no-op'ing against a nullptr
// npu_driver (Executor treats a null npu_driver_ as "static analysis only"
// and skips every device call — see Executor.cpp).
// ============================================================

struct LoadedModel {
    Executor*  executor   = nullptr;
    NPUDriver* npu_driver = nullptr;
};

static std::map<NPUModel*, LoadedModel> g_loaded_models;

// ============================================================
// neon::load / run / unload
// ============================================================

namespace neon {

NPUModel* load(const char* model_path, const char* target_name) {
    auto* npu_driver     = createNPUDriver(target_name);
    auto* npu_component  = createTarget(target_name);
    auto* device_target  = createIdentity(target_name);
    if (npu_driver) npu_driver->open();

    NPUModel* model = ModelLoader::load(model_path, device_target, npu_driver);
    if (!model) return nullptr;

    // Bind an Executor to this model now, while we still know which
    // npu_driver it loaded against — run() looks this back up by model
    // pointer instead of guessing.
    Executor* ex = new Executor(model, npu_component, npu_driver, nullptr, nullptr);
    {
        NeonLockGuard lock(g_session_mutex);
        g_loaded_models[model] = { ex, npu_driver };
    }
    return model;
}

NPUDriverStatus run(NPUModel* model, const void* input, void* output) {
    if (!model) return NPUDriverStatus::ERROR_INVALID_ADDR;

    Executor* ex = nullptr;
    {
        NeonLockGuard lock(g_session_mutex);
        auto it = g_loaded_models.find(model);
        if (it != g_loaded_models.end()) ex = it->second.executor;
    }

    if (ex) return ex->run(input, output);

    // Not loaded through neon::load() (e.g. a model built directly via
    // ModelLoader::load() for static analysis) — fall back to a
    // driver-less Executor, matching ModelLoader.h's "npu_driver may be
    // nullptr for static-analysis-only use" contract.
    Executor fallback(model, nullptr, nullptr);
    return fallback.run(input, output);
}

void unload(NPUModel* model) {
    {
        NeonLockGuard lock(g_session_mutex);
        auto it = g_loaded_models.find(model);
        if (it != g_loaded_models.end()) {
            // npu_driver is the DeviceTargetRegistry's process-wide
            // singleton (see createNPUDriver()) — close it, but never
            // delete it.
            if (it->second.npu_driver) it->second.npu_driver->close();
            delete it->second.executor;
            g_loaded_models.erase(it);
        }
    }
    ModelLoader::unload(model);
}

size_t   inputSize (const NPUModel* model) { return model ? model->input_size()  : 0; }
size_t   outputSize(const NPUModel* model) { return model ? model->output_size() : 0; }
uint32_t numOps    (const NPUModel* model) { return model ? model->num_ops()     : 0; }

// ============================================================
// Session API
// ============================================================

NPUSession* createSession(const char* model_path, const char* target_name) {
    auto* npu_driver        = createNPUDriver(target_name);
    auto* npu_component = createTarget(target_name);
    auto* device_target = createIdentity(target_name);
    if (npu_driver) npu_driver->open();

    NPUModel* model = ModelLoader::load(model_path, device_target, npu_driver);
    if (!model) {
        // npu_driver is the DeviceTargetRegistry's process-wide singleton
        // (see createNPUDriver()) — close it, but never delete it.
        if (npu_driver) npu_driver->close();
        return nullptr;
    }

    uint32_t sid = g_next_session_id.fetch_add(1);

    auto* session           = new NPUSession();
    session->session_id     = sid;
    session->model          = model;
    session->npu_driver         = npu_driver;
    session->device_target  = device_target;
    session->npu_component  = npu_component;
    session->lane           = 255;
    session->n_split        = 64;
    session->peer           = nullptr;

    {
        NeonLockGuard lock(g_session_mutex);
        Executor* ex = new Executor(model, npu_component,
                                     npu_driver, nullptr, nullptr);
        g_sessions[sid] = { ex, model, npu_driver, sid };
    }

    return session;
}

void destroySession(NPUSession* session) {
    if (!session) return;

    {
        NeonLockGuard lock(g_session_mutex);
        auto it = g_sessions.find(session->session_id);
        if (it != g_sessions.end()) {
            delete it->second.executor;
            g_sessions.erase(it);
        }
    }

    // npu_driver is the DeviceTargetRegistry's process-wide singleton
    // (see createNPUDriver()) — close it, but never delete it.
    if (session->npu_driver) {
        session->npu_driver->close();
    }
    ModelLoader::unload(session->model);
    delete session;
}

// Run one session's parked request on its own, giving the whole array to it.
// Caller must hold g_session_mutex.
static NPUDriverStatus runSolo(InternalSession& s, uint32_t session_id) {
    NPUDriverStatus st = s.executor->run(s.request.input, s.request.output);

    auto cb = s.request.callback;
    s.request.parked    = false;
    s.request.completed = true;
    s.request.status    = st;

    if (cb) cb(session_id, st);
    return st;
}

// Run a paired dual-lane submission. `a` must be the lane-0 session and `b`
// the lane-1 session (see loadDual(), which assigns those). Caller must hold
// g_session_mutex.
//
// Executor::runConcurrent() is called on lane 0's executor: its own model_ is
// the lane-A model, and lane B's model plus that model's own input/output are
// passed alongside. This is the whole reason a request has to park — both
// sides' buffers must be live in the same call.
static NPUDriverStatus runPaired(InternalSession& a, uint32_t a_id,
                                 InternalSession& b, uint32_t b_id,
                                 uint8_t n_split) {
    NPUDriverStatus st = a.executor->runConcurrent(
        a.request.input, a.request.output,
        b.model,
        b.request.input, b.request.output,
        n_split);

    auto cb_a = a.request.callback;
    auto cb_b = b.request.callback;

    for (InternalSession* s : { &a, &b }) {
        s->request.parked    = false;
        s->request.completed = true;
        s->request.status    = st;
    }

    // Both lanes finish together — submitConcurrent() returns once the array
    // has drained — so both callbacks fire with the same status.
    if (cb_a) cb_a(a_id, st);
    if (cb_b) cb_b(b_id, st);
    return st;
}

// ── submit ────────────────────────────────────────────────────────────────
//
// Rendezvous protocol for paired sessions:
//
//   submit(A)  → A has no partner request waiting → park A's buffers, return
//   submit(B)  → A is parked → run BOTH through runConcurrent(), complete
//                and fire both callbacks
//
// A session that is never partnered (solo, or single-model hardware) runs
// immediately, exactly as before. A parked request that never finds a
// partner is not lost: waitSession() flushes it solo, so a caller that
// submits on only one of a pair still makes progress instead of hanging.
//
// The old code passed nullptr for the peer's input/output here, which meant
// lane B computed from whatever happened to be in its scratch buffer and its
// result was never copied back out — the pairing has to carry real buffers,
// which is what parking provides.
NPUDriverStatus submit(NPUSession* session,
                     const void* input,
                     void*       output,
                     void (*callback)(uint32_t, NPUDriverStatus)) {
    if (!session || !session->model) return NPUDriverStatus::ERROR_INVALID_ADDR;

    NeonLockGuard lock(g_session_mutex);
    auto it = g_sessions.find(session->session_id);
    if (it == g_sessions.end()) return NPUDriverStatus::ERROR_INVALID_ADDR;

    InternalSession& self = it->second;
    self.request.input     = input;
    self.request.output    = output;
    self.request.callback  = callback;
    self.request.completed = false;

    const bool can_dual = session->peer
                       && session->npu_driver
                       && session->npu_component
                       && session->npu_component->maxConcurrentModels() >= 2;

    if (!can_dual) {
        return runSolo(self, session->session_id);
    }

    auto pit = g_sessions.find(session->peer->session_id);
    if (pit == g_sessions.end()) {
        return runSolo(self, session->session_id);
    }
    InternalSession& peer = pit->second;

    if (!peer.request.parked) {
        // Partner has nothing in flight — hold this one until it does.
        self.request.parked = true;
        return NPUDriverStatus::OK;
    }

    // Both sides are ready. Lane assignment comes from NPUSession::lane
    // (set by loadDual), NOT from arrival order, so the same model always
    // lands on the same rows regardless of which frame showed up first —
    // n_split was computed for that specific assignment.
    const bool self_is_lane_a = (session->lane == 0);
    InternalSession& a    = self_is_lane_a ? self : peer;
    InternalSession& b    = self_is_lane_a ? peer : self;
    const uint32_t   a_id = self_is_lane_a ? session->session_id
                                           : session->peer->session_id;
    const uint32_t   b_id = self_is_lane_a ? session->peer->session_id
                                           : session->session_id;
    const uint8_t    n_split = self_is_lane_a ? session->n_split
                                              : session->peer->n_split;

    NPUDriverStatus st = runPaired(a, a_id, b, b_id, n_split);

    // Report this caller's own view of the pair's outcome.
    return self.request.status == NPUDriverStatus::OK ? st : self.request.status;
}

NPUDriverStatus waitSession(NPUSession* session, uint32_t /*timeout_ms*/) {
    if (!session) return NPUDriverStatus::ERROR_INVALID_ADDR;

    NeonLockGuard lock(g_session_mutex);
    auto it = g_sessions.find(session->session_id);
    if (it == g_sessions.end()) return NPUDriverStatus::ERROR_INVALID_ADDR;

    InternalSession& self = it->second;

    // A request still parked here means its partner never submitted. Waiting
    // for a partner that may never come would hang, so flush it solo — the
    // caller asked for this inference and is entitled to it, just without the
    // spatial split.
    if (self.request.parked) {
        return runSolo(self, session->session_id);
    }

    // Otherwise execution already happened (in this session's own submit(),
    // or in the partner's when it closed the pair). submit() is blocking, so
    // there is nothing left to wait on — only the recorded status to report.
    return self.request.completed ? self.request.status : NPUDriverStatus::OK;
}

DualSession loadDual(const char* model_path_a,
                      const char* model_path_b,
                      const char* target_name) {
    NPUSession* sa = createSession(model_path_a, target_name);
    NPUSession* sb = createSession(model_path_b, target_name);

    if (sa && sb) {
        sa->peer = sb;
        sb->peer = sa;

        // Fixed lane assignment for the pair. submit()'s rendezvous reads
        // this rather than arrival order, so a given model always lands on
        // the same rows no matter which frame arrives first — the n_split
        // below is only correct for this specific assignment.
        sa->lane = 0;
        sb->lane = 1;

        // On FX1: compute optimal N_split from layer shapes
        // (brute-force 1..63 using StaticAnalyzer)
        if (sa->npu_component && sa->npu_component->maxConcurrentModels() >= 2) {
            auto shapes_a = ModelLoader::loadShapes(model_path_a);
            auto shapes_b = ModelLoader::loadShapes(model_path_b);
            if (!shapes_a.empty() && !shapes_b.empty()) {
                StaticAnalyzer analyzer(sa->device_target, sa->npu_component);
                auto split = analyzer.findOptimalSplits2(shapes_a, shapes_b);
                // findOptimalSplits2() returns the single lane-0/lane-1
                // boundary for 2 lanes. A 0 or out-of-range answer means the
                // search found nothing usable (e.g. one model's shapes did
                // not yield a latency estimate) — fall back to the even split
                // rather than programming a partition that starves a lane.
                const uint8_t n = split.n_splits.empty() ? 0 : split.n_splits[0];
                const uint8_t chosen = (n > 0 && n < 64) ? n : 32;
                sa->n_split = chosen;
                sb->n_split = chosen;
            } else {
                // No shape metadata in the .nsm — even split is the only
                // defensible guess.
                sa->n_split = 32;
                sb->n_split = 32;
            }
        }
    }

    return DualSession{ sa, sb };
}

void unloadDual(DualSession sessions) {
    if (sessions.a) sessions.a->peer = nullptr;
    if (sessions.b) sessions.b->peer = nullptr;
    destroySession(sessions.a);
    destroySession(sessions.b);
}

// ============================================================
// getNPUTarget / getNPUDriver
// ============================================================

const NPUComponent* getNPUTarget(const char* target_name) {
    return createTarget(target_name);  // may return nullptr if not linked
}

NPUDriver* getNPUDriver(const char* target_name) {
    // Each device npu_driver registers via NEON_REGISTER_TARGET macro,
    // resolved here through DeviceTargetRegistry — see createNPUDriver().
    return createNPUDriver(target_name);  // may return nullptr if not linked
}

} // namespace neon
