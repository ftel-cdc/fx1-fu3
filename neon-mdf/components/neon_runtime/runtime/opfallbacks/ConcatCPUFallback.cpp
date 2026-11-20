#include "ConcatCPUFallback.h"
#include "core/NPUUnsupportedOpFallback.h"
#include "FallbackIO.h"
#include "../support/NeonLog.h"
#include <cstring>
#include <cstdint>

bool concatCPUFallback(const RuntimeOp& op, const NPUModel& model) {
    if (!op.instr_ptr) {
        uart_puts("[ERROR] concatCPUFallback: null instr_ptr\r\n");
        return false;
    }

    const auto* instr = reinterpret_cast<const NeonCPUFallbackConcatInstr*>(op.instr_ptr);
    if (instr->opcode != NeonOpcode::CPU_FALLBACK ||
        instr->fallback_id != NEON_CPU_FALLBACK_CONCAT) {
        uart_puts("[ERROR] concatCPUFallback: invalid opcode or fallback_id\r\n");
        return false;
    }

    const auto* inputs = reinterpret_cast<const NeonCPUFallbackConcatInput*>(
        reinterpret_cast<const uint8_t*>(instr) + sizeof(NeonCPUFallbackConcatInstr));

    int8_t* dst = neon_fb::dstPtr<int8_t>(op, model, instr->dst_offset);
    if (!dst) {
        uart_puts("[ERROR] concatCPUFallback: unbound operand\r\n");
        return false;
    }

    uint32_t dims[4] = {instr->out_n, instr->out_h, instr->out_w, instr->out_c};
    uint32_t axis = instr->axis;
    if (axis > 3) {
        uart_puts("[ERROR] concatCPUFallback: invalid axis > 3\r\n");
        return false;
    }

    uint32_t out_axis_size = dims[axis];
    uint32_t total_channels = 0;
    for (uint32_t k = 0; k < instr->num_inputs; ++k) {
        total_channels += inputs[k].channels;
    }

    if (total_channels != out_axis_size) {
        uart_puts("[ERROR] concatCPUFallback: sum(inputs[k].channels) != out_axis_size\r\n");
        return false;
    }

    uint32_t outer_size = 1;
    for (uint32_t d = 0; d < axis; ++d) {
        outer_size *= dims[d];
    }
    uint32_t inner_size = 1;
    for (uint32_t d = axis + 1; d < 4; ++d) {
        inner_size *= dims[d];
    }

    uint32_t axis_offset = 0;
    for (uint32_t k = 0; k < instr->num_inputs; ++k) {
        const int8_t* src_k = reinterpret_cast<const int8_t*>(static_cast<uintptr_t>(inputs[k].src_offset));
        if (!src_k) {
            uart_puts("[ERROR] concatCPUFallback: unbound input\r\n");
            return false;
        }
        uint32_t in_axis_k = inputs[k].channels;
        uint32_t copy_bytes = in_axis_k * inner_size;

        for (uint32_t o = 0; o < outer_size; ++o) {
            int8_t* dst_ptr = dst + (o * out_axis_size + axis_offset) * inner_size;
            const int8_t* src_ptr = src_k + o * copy_bytes;
            std::memcpy(dst_ptr, src_ptr, copy_bytes);
        }
        axis_offset += in_axis_k;
    }

    uart_puts("[Firmware] CONCAT CPU fallback executed successfully\r\n");
    return true;
}
