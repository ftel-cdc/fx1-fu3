#include "LutCPUFallback.h"
#include "CPUFallbackRegistry.h"
#include "../../core/NPU_ISA.h"
#include <cstdint>

#include "FallbackIO.h"
#include "../support/NeonLog.h"

bool lutCPUFallback(const RuntimeOp& op, const NPUModel& model) {
    if (!op.instr_ptr || op.instr_size < sizeof(NeonCPUFallbackLutInstr)) {
        uart_puts("[ERROR] lutCPUFallback: instruction too small\r\n");
        return false;
    }

    const auto* instr = static_cast<const NeonCPUFallbackLutInstr*>(op.instr_ptr);
    if (instr->fallback_id != NEON_CPU_FALLBACK_LUT) {
        uart_puts("[ERROR] lutCPUFallback: incorrect fallback_id\r\n");
        return false;
    }

    if (!model.weight_buffer || instr->table_offset + 256 > model.weight_buffer_size) {
        uart_puts("[ERROR] lutCPUFallback: invalid weight_buffer or table_offset\r\n");
        return false;
    }

    const uint8_t* lut = static_cast<const uint8_t*>(model.weight_buffer) + instr->table_offset;
    const int8_t* src = neon_fb::srcPtr<const int8_t>(op, model, instr->src_offset);
    int8_t* dst = neon_fb::dstPtr<int8_t>(op, model, instr->dst_offset);
    if (!src || !dst) {
        uart_puts("[ERROR] lutCPUFallback: unbound operand\r\n");
        return false;
    }

    for (uint32_t i = 0; i < instr->len; ++i) {
        uint8_t val_u8 = static_cast<uint8_t>(src[i]);
        uint8_t lut_idx = val_u8 ^ 0x80; // (int8 + 128) & 0xFF
        dst[i] = static_cast<int8_t>(lut[lut_idx]);
    }

    uart_puts("[Firmware] LUT CPU fallback executed successfully\r\n");
    return true;
}

NEON_REGISTER_CPU_FALLBACK(NEON_CPU_FALLBACK_LUT, lutCPUFallback)
