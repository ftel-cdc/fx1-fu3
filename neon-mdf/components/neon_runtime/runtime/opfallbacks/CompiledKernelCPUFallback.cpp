#include "CompiledKernelCPUFallback.h"
#include "CPUFallbackRegistry.h"
#include "CompiledKernelRegistry.h"
#include "../../core/NPUUnsupportedOpFallback.h"

#include <cstdio>
#include <cstring>

namespace {

constexpr uint32_t kMaxIO = 16;   // per fragment; NeonGraph limits fragments to 8 inputs anyway

// Data pointer of model tensor `idx` (bound by OperandBinder before execution).
void* tensorData(const NPUModel& model, uint32_t idx) {
    return idx < model.tensors.size() ? model.tensors[idx].data : nullptr;
}

}  // namespace

bool compiledKernelFallback(const RuntimeOp& op, const NPUModel& model) {
    if (!op.instr_ptr || op.instr_size < sizeof(NeonCPUFallbackKernelInstr)) {
        fprintf(stderr, "[ERROR] compiledKernelFallback: instruction too small (op %u)\n", op.index);
        return false;
    }
    NeonCPUFallbackKernelInstr k;
    std::memcpy(&k, op.instr_ptr, sizeof(k));
    const size_t extra = (size_t)k.num_inputs + k.num_outputs - 2;
    if (k.num_inputs == 0 || k.num_outputs == 0 || k.num_inputs > kMaxIO || k.num_outputs > kMaxIO ||
        op.instr_size != sizeof(k) + extra * sizeof(uint32_t)) {
        fprintf(stderr, "[ERROR] compiledKernelFallback: malformed instruction (op %u, size %zu)\n",
                op.index, op.instr_size);
        return false;
    }

    // Micro-kernel library version: 0 = the object calls no library kernel.
    if (k.kernels_abi != 0) {
        const uint32_t linked = neon_c906_kernels_abi ? neon_c906_kernels_abi() : 0;
        if (linked != k.kernels_abi) {
            if (linked == 0)
                fprintf(stderr, "[ERROR] compiledKernelFallback: model compiled for C906 kernel ABI %u "
                                "but the runner links no kernel library (libneon_c906_kernels.a)\n",
                        k.kernels_abi);
            else
                fprintf(stderr, "[ERROR] compiledKernelFallback: model compiled for C906 kernel ABI %u "
                                "but the runner links ABI %u (rebuild the model or the runner)\n",
                        k.kernels_abi, linked);
            return false;
        }
    }

    NeonCompiledKernelFn fn = CompiledKernelRegistry::get().lookup(k.kernel_id);
    if (!fn) {
        fprintf(stderr, "[ERROR] compiledKernelFallback: kernel 0x%08x not registered: "
                        "link the model's .o into the runner\n", k.kernel_id);
        return false;
    }

    // inputs[0]/outputs[0] from the dispatch entry, the rest from the trailing indices.
    void* in[kMaxIO] = {};
    void* out[kMaxIO] = {};
    uint32_t idx[2 * kMaxIO];
    std::memcpy(idx, static_cast<const uint8_t*>(op.instr_ptr) + sizeof(k), extra * sizeof(uint32_t));
    in[0] = tensorData(model, op.input_tensor_idx);
    for (uint32_t i = 1; i < k.num_inputs; ++i) in[i] = tensorData(model, idx[i - 1]);
    out[0] = tensorData(model, op.output_tensor_idx);
    for (uint32_t i = 1; i < k.num_outputs; ++i) out[i] = tensorData(model, idx[k.num_inputs - 1 + i - 1]);
    for (uint32_t i = 0; i < k.num_inputs; ++i)
        if (!in[i]) { fprintf(stderr, "[ERROR] compiledKernelFallback: input %u unbound\n", i); return false; }
    for (uint32_t i = 0; i < k.num_outputs; ++i)
        if (!out[i]) { fprintf(stderr, "[ERROR] compiledKernelFallback: output %u unbound\n", i); return false; }

    const int64_t rc = fn(in, out);
    if (rc != 0) fprintf(stderr, "[ERROR] compiledKernelFallback: kernel 0x%08x returned %lld\n",
                         k.kernel_id, (long long)rc);
    return rc == 0;
}

NEON_REGISTER_CPU_FALLBACK(NEON_CPU_FALLBACK_COMPILED_KERNEL, compiledKernelFallback)
