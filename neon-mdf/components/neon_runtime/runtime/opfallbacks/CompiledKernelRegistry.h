#pragma once
// ============================================================
// CompiledKernelRegistry — kernel_id -> entry point of compiler-generated
// CPU code (neon/compiled_kernel_abi.h). Filled at program start by the
// global constructors of the linked model objects; read by
// CompiledKernelCPUFallback. Fixed size, no heap (usable on bare metal).
// ============================================================
#include <neon/compiled_kernel_abi.h>

#include <cstddef>
#include <cstdint>

class CompiledKernelRegistry {
public:
    static constexpr size_t kMaxKernels = 64;

    static CompiledKernelRegistry& get() {
        static CompiledKernelRegistry instance;
        return instance;
    }

    // false (and a message on stderr) when the table is full or the id is
    // already taken by a different function: two linked models must never
    // share an id, a silent overwrite would run the wrong code.
    bool add(uint32_t id, NeonCompiledKernelFn fn);

    NeonCompiledKernelFn lookup(uint32_t id) const;

private:
    CompiledKernelRegistry() = default;
    struct Entry { uint32_t id; NeonCompiledKernelFn fn; };
    Entry  entries_[kMaxKernels] = {};
    size_t count_ = 0;
};
