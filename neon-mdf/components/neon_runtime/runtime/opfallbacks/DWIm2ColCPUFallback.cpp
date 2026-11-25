#include "DWIm2ColCPUFallback.h"
#include "CPUFallbackRegistry.h"
#include "../../core/NPU_ISA.h"
#include <cstdint>
#include <cstring>

#include "FallbackIO.h"
#include "../support/NeonLog.h"

bool dwIm2colCPUFallback(const RuntimeOp& op, const NPUModel& model) {
    if (!op.instr_ptr || op.instr_size < sizeof(NeonCPUFallbackDWIm2ColInstr)) {
        uart_puts("[ERROR] dwIm2colCPUFallback: instruction too small\r\n");
        return false;
    }

    const auto* instr = static_cast<const NeonCPUFallbackDWIm2ColInstr*>(op.instr_ptr);
    if (instr->fallback_id != NEON_CPU_FALLBACK_DW_IM2COL) {
        uart_puts("[ERROR] dwIm2colCPUFallback: incorrect fallback_id\r\n");
        return false;
    }

    const int8_t* src = neon_fb::srcPtr<const int8_t>(op, model, instr->src_offset);
    int8_t* dst = neon_fb::dstPtr<int8_t>(op, model, instr->dst_offset);
    if (!src || !dst) {
        uart_puts("[ERROR] dwIm2colCPUFallback: unbound operand\r\n");
        return false;
    }

    int8_t* out_ptr = dst;
    for (uint32_t ho_idx = 0; ho_idx < instr->ho; ++ho_idx) {
        for (uint32_t wo_idx = 0; wo_idx < instr->wo; ++wo_idx) {
            for (uint32_t kh_idx = 0; kh_idx < instr->kh; ++kh_idx) {
                int32_t ih = static_cast<int32_t>(ho_idx * instr->stride_h + kh_idx * instr->dilation_h) - static_cast<int32_t>(instr->pad_t);
                for (uint32_t kw_idx = 0; kw_idx < instr->kw; ++kw_idx) {
                    int32_t iw = static_cast<int32_t>(wo_idx * instr->stride_w + kw_idx * instr->dilation_w) - static_cast<int32_t>(instr->pad_l);
                    bool in_bound = ih >= 0 && ih < static_cast<int32_t>(instr->ih) &&
                                    iw >= 0 && iw < static_cast<int32_t>(instr->iw);
                    if (in_bound) {
                        // Single-channel gather: real per-pixel stride is channel_stride
                        // (the source tensor's true channel count), not 1.
                        *out_ptr = src[(ih * instr->iw + iw) * instr->channel_stride];
                    } else {
                        *out_ptr = static_cast<int8_t>(instr->input_zero_point);
                    }
                    ++out_ptr;
                }
            }
        }
    }

    uart_puts("[Firmware] DW Im2Col CPU fallback executed successfully\r\n");
    return true;
}

NEON_REGISTER_CPU_FALLBACK(NEON_CPU_FALLBACK_DW_IM2COL, dwIm2colCPUFallback)
