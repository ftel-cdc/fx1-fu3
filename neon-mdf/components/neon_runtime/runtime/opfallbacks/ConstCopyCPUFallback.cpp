#include "ConstCopyCPUFallback.h"
#include "CPUFallbackRegistry.h"
#include "../../core/NPU_ISA.h"
#include <cstdint>
#include <cstring>

#include "FallbackIO.h"
#include "../support/NeonLog.h"

bool constCopyCPUFallback(const RuntimeOp& op, const NPUModel& model) {
    if (!op.instr_ptr || op.instr_size < sizeof(NeonCPUFallbackConstCopyInstr)) {
        uart_puts("[ERROR] constCopyCPUFallback: instruction too small\r\n");
        return false;
    }

    const auto* instr = static_cast<const NeonCPUFallbackConstCopyInstr*>(op.instr_ptr);
    if (instr->fallback_id != NEON_CPU_FALLBACK_CONST_COPY) {
        uart_puts("[ERROR] constCopyCPUFallback: incorrect fallback_id\r\n");
        return false;
    }
    if (!model.weight_buffer) {
        uart_puts("[ERROR] constCopyCPUFallback: null weight_buffer\r\n");
        return false;
    }
    if (instr->size_bytes == 0) {
        uart_puts("[ERROR] constCopyCPUFallback: size_bytes == 0\r\n");
        return false;
    }

    const uint8_t* src = static_cast<const uint8_t*>(model.weight_buffer) + instr->const_offset;
    uint8_t* dst = neon_fb::dstPtr<uint8_t>(op, model, instr->dst_offset);
    if (!dst) {
        uart_puts("[ERROR] constCopyCPUFallback: unbound operand\r\n");
        return false;
    }

    std::memcpy(dst, src, instr->size_bytes);

    uart_put_u32("[Firmware] ConstCopy CPU fallback copied ", instr->size_bytes, " bytes\r\n");
    return true;
}

NEON_REGISTER_CPU_FALLBACK(NEON_CPU_FALLBACK_CONST_COPY, constCopyCPUFallback)
