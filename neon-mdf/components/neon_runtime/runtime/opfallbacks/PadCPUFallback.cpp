#include "PadCPUFallback.h"
#include "core/NPUUnsupportedOpFallback.h"
#include "FallbackIO.h"
#include "../support/NeonLog.h"
#include <cstring>
#include <cstdint>

bool padCPUFallback(const RuntimeOp& op, const NPUModel& model) {
    if (!op.instr_ptr) {
        uart_puts("[ERROR] padCPUFallback: null instr_ptr\r\n");
        return false;
    }

    const auto* instr = reinterpret_cast<const NeonCPUFallbackPadInstr*>(op.instr_ptr);
    if (instr->opcode != NeonOpcode::CPU_FALLBACK ||
        instr->fallback_id != NEON_CPU_FALLBACK_PAD) {
        uart_puts("[ERROR] padCPUFallback: invalid opcode or fallback_id\r\n");
        return false;
    }

    const int8_t* src = neon_fb::srcPtr<const int8_t>(op, model, instr->src_offset);
    int8_t* dst = neon_fb::dstPtr<int8_t>(op, model, instr->dst_offset);
    if (!src || !dst) {
        uart_puts("[ERROR] padCPUFallback: unbound operand\r\n");
        return false;
    }

    if (instr->off_n + instr->in_n > instr->out_n ||
        instr->off_h + instr->in_h > instr->out_h ||
        instr->off_w + instr->in_w > instr->out_w ||
        instr->off_c + instr->in_c > instr->out_c) {
        uart_puts("[ERROR] padCPUFallback: offset + in dims exceed out dims\r\n");
        return false;
    }

    // 1. Fill the ENTIRE destination with pad_value.
    const uint64_t out_total = (uint64_t)instr->out_n * instr->out_h * instr->out_w * instr->out_c;
    std::memset(dst, static_cast<int>(instr->pad_value), static_cast<size_t>(out_total));

    // 2. Copy source into the offset region, row-major NHWC, contiguous
    //    innermost (channel) run per (n,h,w) — matches the row-per-row
    //    copy shape SliceCPUFallback.cpp already uses.
    for (uint32_t n = 0; n < instr->in_n; ++n) {
        uint32_t out_n_idx = instr->off_n + n;
        for (uint32_t h = 0; h < instr->in_h; ++h) {
            uint32_t out_h_idx = instr->off_h + h;
            for (uint32_t w = 0; w < instr->in_w; ++w) {
                uint32_t out_w_idx = instr->off_w + w;
                const int8_t* src_row = src + (((uint64_t)n * instr->in_h + h) * instr->in_w + w) * instr->in_c;
                int8_t* dst_row = dst + (((uint64_t)out_n_idx * instr->out_h + out_h_idx) * instr->out_w + out_w_idx) * instr->out_c
                                       + instr->off_c;
                std::memcpy(dst_row, src_row, instr->in_c);
            }
        }
    }

    uart_puts("[Firmware] PAD CPU fallback executed successfully\r\n");
    return true;
}
