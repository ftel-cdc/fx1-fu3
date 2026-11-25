#pragma once

// ============================================================
// NEON STUDIO — CPU FALLBACK REGISTRY
//
// Runtime-side half of the CPU-fallback escape hatch (see
// docs/hybrid_npu_cpu_fallback_design.md and core/NPUUnsupportedOpFallback.h
// for the full design). Maps a NeonCPUFallbackId (assigned by toolkit/ when
// it emits a NeonCPUFallbackInstr — see core/NPU_ISA.h) to an actual,
// statically-linked CPU function, looked up by Executor::dispatchOp() when
// it encounters NeonOpcode::CPU_FALLBACK.
//
// Lives in runtime/opfallbacks/, a sibling of runtime/driver/ (not nested
// inside it) — deliberately. runtime/driver/ is for hardware-abstraction
// interfaces attached via multiple inheritance to one concrete device
// Runtime class (NPUDriver, CPUCycleCounter — see CPUCycleCounter.h's own
// doc comment on that composition pattern). This registry, and every
// fallback function registered into it, has nothing to do with any single
// device target — it's a standalone lookup mechanism Executor consults
// when an op's opcode says "don't talk to hardware at all for this one."
// Deliberately not in core/ either (unlike DeviceTargetRegistry in
// core/DeviceTarget.h): only runtime/ (Executor) ever needs to look
// functions up here; toolkit/ only needs the ID type/catalog
// (core/NPUUnsupportedOpFallback.h) to assign IDs, never this registry.
//
// Registration mirrors NEON_REGISTER_TARGET's pattern (core/DeviceTarget.h)
// but keys by NeonCPUFallbackId instead of by device name, and registers a
// plain function pointer instead of a class instance:
//
//   bool myConv2DFallback(const RuntimeOp& op, const NPUModel& model) { ... }
//   NEON_REGISTER_CPU_FALLBACK(NEON_CPU_FALLBACK_CONV2D, myConv2DFallback)
//
// First real registration: runtime/opfallbacks/Conv2DCPUFallback.h/.cpp
// (NEON_CPU_FALLBACK_CONV2D) — see core/NPUUnsupportedOpFallback.h.
//
// Function signature takes the whole RuntimeOp + NPUModel rather than raw
// (input, output, sizes) — an earlier, narrower version of this signature
// (see docs/hybrid_npu_cpu_fallback_design.md's "Open questions" #2) turned
// out to be too narrow the moment a real fallback (Conv2D) needed weight
// data, per-channel quant scales, and shape params, none of which fit
// (in, out, size) alone. Each registered function re-interprets
// op.instr_ptr as whatever larger struct its own fallback_id implies (see
// NeonCPUFallbackConv2DInstr in core/NPU_ISA.h for the pattern) and pulls
// weight/bias bytes from model.weight_buffer directly — Executor itself
// stays generic, same as it already is for every NPU-dispatched opcode.
// ============================================================

#include "../../core/NPUUnsupportedOpFallback.h"
#include "../loader/NPUModel.h"

#include <stddef.h>

#ifdef __cplusplus
extern "C++" {

#include <vector>

using NeonCPUFallbackFn = bool (*)(const RuntimeOp& op, const NPUModel& model);

class CPUFallbackRegistry {
public:
    static CPUFallbackRegistry& get() {
        static CPUFallbackRegistry instance;
        return instance;
    }

    void registerFallback(NeonCPUFallbackId id, NeonCPUFallbackFn fn) {
        for (const auto& e : entries_)
            if (e.id == id) return;  // de-dupe, mirrors DeviceTargetRegistry::registerTarget()
        entries_.push_back({id, fn});
    }

    NeonCPUFallbackFn lookup(NeonCPUFallbackId id) const {
        for (const auto& e : entries_)
            if (e.id == id) return e.fn;
        return nullptr;
    }

private:
    CPUFallbackRegistry() = default;

    struct Entry {
        NeonCPUFallbackId id;
        NeonCPUFallbackFn fn;
    };
    std::vector<Entry> entries_;
};

// ============================================================
// REGISTRATION MACRO
// Call once per fallback function, anywhere it's linked in.
//
//   NEON_REGISTER_CPU_FALLBACK(NEON_CPU_FALLBACK_SOME_ID, someFallbackFn)
// ============================================================

#define NEON_REGISTER_CPU_FALLBACK(fallback_id, fn)                       \
    static struct fn##_CPUFallbackRegistrar {                             \
        fn##_CPUFallbackRegistrar() {                                     \
            CPUFallbackRegistry::get().registerFallback((fallback_id), (fn)); \
        }                                                                  \
    } _##fn##_cpu_fallback_registrar;

} // extern "C++"
#endif
