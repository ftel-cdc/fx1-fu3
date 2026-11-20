#include "Im2ColCPUFallback.h"
#include "CPUFallbackRegistry.h"
#include "../../core/NPU_ISA.h"
#include <cstddef>
#include <cstdint>
#include <cstring>

#include "FallbackIO.h"
#include "../support/NeonLog.h"

bool im2colCPUFallback(const RuntimeOp& op, const NPUModel& model) {
    if (!op.instr_ptr || op.instr_size < offsetof(NeonCPUFallbackIm2ColInstr, layout)) {
        uart_puts("[ERROR] im2colCPUFallback: instruction too small\r\n");
        return false;
    }

    const auto* instr = static_cast<const NeonCPUFallbackIm2ColInstr*>(op.instr_ptr);
    if (instr->fallback_id != NEON_CPU_FALLBACK_IM2COL) {
        uart_puts("[ERROR] im2colCPUFallback: incorrect fallback_id\r\n");
        return false;
    }

    const int8_t* src = neon_fb::srcPtr<const int8_t>(op, model, instr->src_offset);
    int8_t* dst = neon_fb::dstPtr<int8_t>(op, model, instr->dst_offset);
    if (!src || !dst) {
        uart_puts("[ERROR] im2colCPUFallback: unbound operand\r\n");
        return false;
    }

    const bool nchw = op.instr_size > offsetof(NeonCPUFallbackIm2ColInstr, layout) &&
                      instr->layout == NEON_LAYOUT_NCHW;
    const int8_t izp = static_cast<int8_t>(instr->input_zero_point);

    int8_t* out_ptr = dst;
    if (nchw) {
        // src is [C][H][W]; each im2col row is K = ic*kh*kw bytes in (ic, kh, kw) order,
        // matching OIHW weights (Im2ColConvToGemmPass, NCHW branch).
        const uint32_t plane = instr->ih * instr->iw;
        for (uint32_t ho_idx = 0; ho_idx < instr->ho; ++ho_idx) {
            for (uint32_t wo_idx = 0; wo_idx < instr->wo; ++wo_idx) {
                for (uint32_t c = 0; c < instr->ic; ++c) {
                    const int8_t* src_c = src + static_cast<size_t>(c) * plane;
                    for (uint32_t kh_idx = 0; kh_idx < instr->kh; ++kh_idx) {
                        const int32_t ih = static_cast<int32_t>(ho_idx * instr->stride_h + kh_idx * instr->dilation_h) - static_cast<int32_t>(instr->pad_t);
                        const bool row_ok = ih >= 0 && ih < static_cast<int32_t>(instr->ih);
                        for (uint32_t kw_idx = 0; kw_idx < instr->kw; ++kw_idx) {
                            const int32_t iw = static_cast<int32_t>(wo_idx * instr->stride_w + kw_idx * instr->dilation_w) - static_cast<int32_t>(instr->pad_l);
                            *out_ptr++ = (row_ok && iw >= 0 && iw < static_cast<int32_t>(instr->iw))
                                             ? src_c[static_cast<size_t>(ih) * instr->iw + iw]
                                             : izp;
                        }
                    }
                }
            }
        }
    } else {
        for (uint32_t ho_idx = 0; ho_idx < instr->ho; ++ho_idx) {
            for (uint32_t wo_idx = 0; wo_idx < instr->wo; ++wo_idx) {
                for (uint32_t kh_idx = 0; kh_idx < instr->kh; ++kh_idx) {
                    int32_t ih = static_cast<int32_t>(ho_idx * instr->stride_h + kh_idx * instr->dilation_h) - static_cast<int32_t>(instr->pad_t);
                    for (uint32_t kw_idx = 0; kw_idx < instr->kw; ++kw_idx) {
                        int32_t iw = static_cast<int32_t>(wo_idx * instr->stride_w + kw_idx * instr->dilation_w) - static_cast<int32_t>(instr->pad_l);
                        bool in_bound = ih >= 0 && ih < static_cast<int32_t>(instr->ih) &&
                                        iw >= 0 && iw < static_cast<int32_t>(instr->iw);
                        if (in_bound) {
                            const int8_t* src_ptr = src + (ih * instr->iw + iw) * instr->ic;
                            std::memcpy(out_ptr, src_ptr, instr->ic);
                        } else {
                            std::memset(out_ptr, static_cast<int8_t>(instr->input_zero_point), instr->ic);
                        }
                        out_ptr += instr->ic;
                    }
                }
            }
        }
    }

    uart_puts("[Firmware] Im2Col CPU fallback executed successfully\r\n");
    return true;
}

NEON_REGISTER_CPU_FALLBACK(NEON_CPU_FALLBACK_IM2COL, im2colCPUFallback)
