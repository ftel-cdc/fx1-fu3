#pragma once

#include <stddef.h>

#ifdef __cplusplus
extern "C++" {

#include <string>
#include <vector>

// ============================================================
// NEON STUDIO — DEVICE TARGET BASE
//
// Generic base interface shared by every hardware block target this repo
// registers (today: NPU only — VP_FX1_V2.0, qemu_mps3_an547). This file
// is intentionally block-specific-capability-agnostic:
//   - No NPU-specific fields (TOPS, SRAM tile, dataflow...)
//   - No op/dtype enums from NPU_ISA.h
//   - Safe to include from any block's runtime without pulling
//     in NPU compiler headers
//
// Type-specific interfaces (extend this base):
//   NPU — arch type, dataflow, matrix block, TOPS, supportsOp...
//       → include "NPUComponent.h"
//   Other block kinds (ISP/VPU/DSP/...) — no component headers exist yet;
//       add one following the NPUComponent.h pattern if/when a real
//       non-NPU block needs registering. A `DeviceType` enum tag +
//       `is*()`/`lookupByType()` helpers on this base and the registry
//       were removed here (2026-07-14) as unneeded speculation — every
//       registered target today is an NPU, identifiable by name alone
//       (see DeviceTargetRegistry::lookup() below); reintroduce a type tag
//       only once a real non-NPU block target actually needs filtering by
//       kind, not preemptively.
//
// Each hardware block extends this base, then further in:
//   device/<name>/toolkit/  → compiler-side target
//   device/<name>/runtime/  → runtime-side target
// ============================================================

// ============================================================
// DEVICE TARGET BASE
//
// Minimal identity interface — implement this for every block
// type. Block-specific capabilities go in type-specific components
// (NPUComponent, and future ISP/VPU/DSP components).
// ============================================================

class DeviceTarget {
public:
    virtual ~DeviceTarget() = default;

    // --- Identity ---
    virtual const char* getName()        const = 0;  // e.g. "VP_FX1_V2.0"
    virtual const char* getDescription() const = 0;
};

// ============================================================
// DEVICE TARGET REGISTRY
//
// Global registry for all registered targets (all block types).
// Toolkit and runtime use this to look up targets by name.
// ============================================================

class DeviceTargetRegistry {
public:
    static DeviceTargetRegistry& get() {
        static DeviceTargetRegistry instance;
        return instance;
    }

    void registerTarget(DeviceTarget* device_target) {
        for (auto* t : targets_)
            if (std::string(t->getName()) == device_target->getName()) return;
        targets_.push_back(device_target);
    }

    DeviceTarget* lookup(const char* name) const {
        for (auto* t : targets_)
            if (std::string(t->getName()) == name) return t;
        return nullptr;
    }

    const std::vector<DeviceTarget*>& all() const { return targets_; }

private:
    DeviceTargetRegistry() = default;
    std::vector<DeviceTarget*> targets_;
};

// ============================================================
// REGISTRATION MACRO
// Call once per device in its .cpp to auto-register on startup.
//
//   NEON_REGISTER_TARGET(QEMUMPS3Runtime)
// ============================================================

#define NEON_REGISTER_TARGET(TargetClass)                              \
    static struct TargetClass##Registrar {                             \
        TargetClass##Registrar() {                                     \
            static TargetClass instance;                               \
            DeviceTargetRegistry::get().registerTarget(&instance);     \
        }                                                              \
    } _##TargetClass##_registrar;

} // extern "C++"
#endif // __cplusplus
