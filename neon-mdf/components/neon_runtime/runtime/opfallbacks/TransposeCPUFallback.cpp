#include "TransposeCPUFallback.h"
#include "CPUFallbackRegistry.h"
#include "../../core/NPU_ISA.h"
#include <cstdint>
#include <cstring>

#include "FallbackIO.h"
#include "../support/NeonLog.h"

bool transposeCPUFallback(const RuntimeOp& op, const NPUModel& model) {
    if (!op.instr_ptr || op.instr_size < sizeof(NeonCPUFallbackTransposeInstr)) {
        uart_puts("[ERROR] transposeCPUFallback: instruction too small\r\n");
        return false;
    }

    const auto* instr = static_cast<const NeonCPUFallbackTransposeInstr*>(op.instr_ptr);
    if (instr->fallback_id != NEON_CPU_FALLBACK_TRANSPOSE) {
        uart_puts("[ERROR] transposeCPUFallback: incorrect fallback_id\r\n");
        return false;
    }

    uint32_t rank = instr->rank;
    if (rank == 0 || rank > 4) {
        uart_puts("[ERROR] transposeCPUFallback: rank must be between 1 and 4\r\n");
        return false;
    }

    uint32_t in_dims[4] = {1, 1, 1, 1};
    uint32_t perm[4] = {0, 1, 2, 3};
    for (uint32_t i = 0; i < rank; ++i) {
        in_dims[i] = instr->in_dims[i];
        perm[i] = instr->perm[i];
    }

    uint32_t total_elements = 1;
    for (uint32_t i = 0; i < rank; ++i) {
        total_elements *= in_dims[i];
    }

    uint32_t out_dims[4] = {1, 1, 1, 1};
    for (uint32_t i = 0; i < rank; ++i) {
        out_dims[i] = in_dims[perm[i]];
    }

    uint32_t in_strides[4] = {1, 1, 1, 1};
    uint32_t out_strides[4] = {1, 1, 1, 1};
    in_strides[rank - 1] = 1;
    out_strides[rank - 1] = 1;
    for (int i = static_cast<int>(rank) - 2; i >= 0; --i) {
        in_strides[i] = in_strides[i + 1] * in_dims[i + 1];
        out_strides[i] = out_strides[i + 1] * out_dims[i + 1];
    }

    const int8_t* src = neon_fb::srcPtr<const int8_t>(op, model, instr->src_offset);
    int8_t* dst = neon_fb::dstPtr<int8_t>(op, model, instr->dst_offset);
    if (!src || !dst) {
        uart_puts("[ERROR] transposeCPUFallback: unbound operand\r\n");
        return false;
    }

    uint32_t inv_perm[4] = {0, 1, 2, 3};
    for (uint32_t i = 0; i < rank; ++i) {
        inv_perm[perm[i]] = i;
    }

    // Pad to 4D for uniform branchless nested loop
    uint32_t d[4] = {1, 1, 1, 1};
    uint32_t step[4] = {0, 0, 0, 0};
    for (uint32_t i = 0; i < rank; ++i) {
        d[i] = in_dims[i];
        step[i] = out_strides[inv_perm[i]];
    }

    const int8_t* in_ptr = src;
    for (uint32_t i0 = 0; i0 < d[0]; ++i0) {
        uint32_t off0 = i0 * step[0];
        for (uint32_t i1 = 0; i1 < d[1]; ++i1) {
            uint32_t off1 = off0 + i1 * step[1];
            for (uint32_t i2 = 0; i2 < d[2]; ++i2) {
                uint32_t off2 = off1 + i2 * step[2];
                for (uint32_t i3 = 0; i3 < d[3]; ++i3) {
                    dst[off2 + i3 * step[3]] = *in_ptr++;
                }
            }
        }
    }

    uart_puts("[Firmware] Transpose CPU fallback executed successfully\r\n");
    return true;

}

NEON_REGISTER_CPU_FALLBACK(NEON_CPU_FALLBACK_TRANSPOSE, transposeCPUFallback)
