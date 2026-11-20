#include "MaxPoolCPUFallback.h"
#include "CPUFallbackRegistry.h"
#include "../../core/NPU_ISA.h"
#include <cstddef>
#include <cstdint>

#include "FallbackIO.h"
#include "../support/NeonLog.h"

bool maxPoolCPUFallback(const RuntimeOp& op, const NPUModel& model) {
    if (!op.instr_ptr || op.instr_size < offsetof(NeonCPUFallbackMaxPoolInstr, layout)) {
        uart_puts("[ERROR] maxPoolCPUFallback: instruction too small\r\n");
        return false;
    }

    const auto* instr = static_cast<const NeonCPUFallbackMaxPoolInstr*>(op.instr_ptr);
    if (instr->fallback_id != NEON_CPU_FALLBACK_MAXPOOL) {
        uart_puts("[ERROR] maxPoolCPUFallback: incorrect fallback_id\r\n");
        return false;
    }

    const int8_t* input = neon_fb::srcPtr<const int8_t>(op, model, instr->src_offset);
    int8_t* output = neon_fb::dstPtr<int8_t>(op, model, instr->dst_offset);
    if (!input || !output) {
        uart_puts("[ERROR] maxPoolCPUFallback: unbound operand\r\n");
        return false;
    }

    const bool nchw = op.instr_size > offsetof(NeonCPUFallbackMaxPoolInstr, layout) &&
                      instr->layout == NEON_LAYOUT_NCHW;
    if (nchw) {
        const size_t in_plane = static_cast<size_t>(instr->input_h) * instr->input_w;
        const size_t out_plane = static_cast<size_t>(instr->output_h) * instr->output_w;
        for (uint32_t c = 0; c < instr->channels; ++c) {
            const int8_t* in_c = input + c * in_plane;
            int8_t* out_c = output + c * out_plane;
            for (uint32_t oh = 0; oh < instr->output_h; ++oh) {
                for (uint32_t ow = 0; ow < instr->output_w; ++ow) {
                    int8_t m = -128; // padding sentinel, matches Pool.cpp
                    for (uint32_t kh = 0; kh < instr->kernel_h; ++kh) {
                        const int32_t ih = static_cast<int32_t>(oh * instr->stride_h + kh) - static_cast<int32_t>(instr->pad_t);
                        if (ih < 0 || ih >= static_cast<int32_t>(instr->input_h)) continue;
                        for (uint32_t kw = 0; kw < instr->kernel_w; ++kw) {
                            const int32_t iw = static_cast<int32_t>(ow * instr->stride_w + kw) - static_cast<int32_t>(instr->pad_l);
                            if (iw < 0 || iw >= static_cast<int32_t>(instr->input_w)) continue;
                            const int8_t v = in_c[static_cast<size_t>(ih) * instr->input_w + iw];
                            if (v > m) m = v;
                        }
                    }
                    out_c[static_cast<size_t>(oh) * instr->output_w + ow] = m;
                }
            }
        }
        uart_puts("[Firmware] MaxPool2D CPU fallback executed successfully\r\n");
        return true;
    }

    for (uint32_t oh = 0; oh < instr->output_h; ++oh) {
        for (uint32_t ow = 0; ow < instr->output_w; ++ow) {
            int8_t* out_pixel = &output[(oh * instr->output_w + ow) * instr->channels];
            for (uint32_t c = 0; c < instr->channels; ++c) {
                out_pixel[c] = -128; // padding sentinel, matches Pool.cpp:48
            }
            for (uint32_t kh = 0; kh < instr->kernel_h; ++kh) {
                int32_t ih = static_cast<int32_t>(oh * instr->stride_h + kh) - static_cast<int32_t>(instr->pad_t);
                if (ih < 0 || ih >= static_cast<int32_t>(instr->input_h))
                    continue;
                for (uint32_t kw = 0; kw < instr->kernel_w; ++kw) {
                    int32_t iw = static_cast<int32_t>(ow * instr->stride_w + kw) - static_cast<int32_t>(instr->pad_l);
                    if (iw < 0 || iw >= static_cast<int32_t>(instr->input_w))
                        continue;
                    const int8_t* in_pixel = &input[(ih * instr->input_w + iw) * instr->channels];
                    for (uint32_t c = 0; c < instr->channels; ++c) {
                        if (in_pixel[c] > out_pixel[c]) {
                            out_pixel[c] = in_pixel[c];
                        }
                    }
                }
            }
        }
    }

    uart_puts("[Firmware] MaxPool2D CPU fallback executed successfully\r\n");
    return true;
}

NEON_REGISTER_CPU_FALLBACK(NEON_CPU_FALLBACK_MAXPOOL, maxPoolCPUFallback)
