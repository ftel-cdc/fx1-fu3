#include "SliceCPUFallback.h"
#include "core/NPUUnsupportedOpFallback.h"
#include "FallbackIO.h"
#include "../support/NeonLog.h"
#include <cstring>
#include <cstdint>

bool sliceCPUFallback(const RuntimeOp& op, const NPUModel& model) {
    if (!op.instr_ptr) {
        uart_puts("[ERROR] sliceCPUFallback: null instr_ptr\r\n");
        return false;
    }

    const auto* instr = reinterpret_cast<const NeonCPUFallbackSliceInstr*>(op.instr_ptr);
    if (instr->opcode != NeonOpcode::CPU_FALLBACK ||
        instr->fallback_id != NEON_CPU_FALLBACK_SLICE) {
        uart_puts("[ERROR] sliceCPUFallback: invalid opcode or fallback_id\r\n");
        return false;
    }

    const int8_t* src = neon_fb::srcPtr<const int8_t>(op, model, instr->src_offset);
    int8_t* dst = neon_fb::dstPtr<int8_t>(op, model, instr->dst_offset);
    if (!src || !dst) {
        uart_puts("[ERROR] sliceCPUFallback: unbound operand\r\n");
        return false;
    }

    if (instr->begin_n + instr->size_n > instr->in_n ||
        instr->begin_h + instr->size_h > instr->in_h ||
        instr->begin_w + instr->size_w > instr->in_w ||
        instr->begin_c + instr->size_c > instr->in_c) {
        uart_puts("[ERROR] sliceCPUFallback: begin + size exceeds input dimensions\r\n");
        return false;
    }

    for (uint32_t n = 0; n < instr->size_n; ++n) {
        uint32_t in_n_idx = instr->begin_n + n;
        for (uint32_t h = 0; h < instr->size_h; ++h) {
            uint32_t in_h_idx = instr->begin_h + h;
            for (uint32_t w = 0; w < instr->size_w; ++w) {
                uint32_t in_w_idx = instr->begin_w + w;
                const int8_t* src_row = src + (((in_n_idx * instr->in_h + in_h_idx) * instr->in_w + in_w_idx) * instr->in_c + instr->begin_c);
                int8_t* dst_row = dst + (((n * instr->size_h + h) * instr->size_w + w) * instr->size_c);
                std::memcpy(dst_row, src_row, instr->size_c);
            }
        }
    }

    uart_puts("[Firmware] SLICE CPU fallback executed successfully\r\n");
    return true;
}
