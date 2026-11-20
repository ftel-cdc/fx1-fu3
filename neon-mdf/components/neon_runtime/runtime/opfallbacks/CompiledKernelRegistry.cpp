#include "CompiledKernelRegistry.h"

#include <cstdio>

bool CompiledKernelRegistry::add(uint32_t id, NeonCompiledKernelFn fn) {
    for (size_t i = 0; i < count_; ++i) {
        if (entries_[i].id != id) continue;
        if (entries_[i].fn == fn) return true;   // same object registered twice: harmless
        fprintf(stderr, "[ERROR] CompiledKernelRegistry: kernel id 0x%08x registered twice "
                        "with different code (two models with the same name/entry?)\n", id);
        return false;
    }
    if (count_ == kMaxKernels) {
        fprintf(stderr, "[ERROR] CompiledKernelRegistry: more than %zu compiled kernels\n", kMaxKernels);
        return false;
    }
    entries_[count_++] = Entry{id, fn};
    return true;
}

NeonCompiledKernelFn CompiledKernelRegistry::lookup(uint32_t id) const {
    for (size_t i = 0; i < count_; ++i)
        if (entries_[i].id == id) return entries_[i].fn;
    return nullptr;
}

// C entry point called by the generated global constructors (C906EntryABI.cpp).
extern "C" void neon_register_compiled_kernel(uint64_t kernel_id, NeonCompiledKernelFn fn) {
    (void)CompiledKernelRegistry::get().add(static_cast<uint32_t>(kernel_id), fn);
}
