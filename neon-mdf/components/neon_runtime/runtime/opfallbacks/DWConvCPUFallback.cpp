#include "DWConvCPUFallback.h"
#include "CPUFallbackRegistry.h"
#include "../../core/NPUUnsupportedOpFallback.h"

#include <cmath>
#include <cstdio>
#include <cstring>

#include "FallbackIO.h"
#include "../support/NeonLog.h"

static inline int32_t multiplyByQuantizedMultiplier(int32_t x, int32_t multiplier, int shift) {
    int64_t total_mult = static_cast<int64_t>(x) * static_cast<int64_t>(multiplier);
    int64_t q31_mult = (total_mult + (1LL << 30)) >> 31;
    
    int left_shift = shift > 0 ? shift : 0;
    int right_shift = shift < 0 ? -shift : 0;
    if (left_shift > 0) {
        q31_mult <<= left_shift;
    }
    if (right_shift > 0) {
        int64_t remainder = q31_mult & ((1LL << right_shift) - 1);
        int64_t threshold = (1LL << (right_shift - 1));
        q31_mult >>= right_shift;
        if (remainder > threshold || (remainder == threshold && (q31_mult & 1))) {
            q31_mult += 1;
        }
    }
    return static_cast<int32_t>(q31_mult);
}

static inline void quantizeMultiplier(float double_multiplier, int32_t *quantized_multiplier, int *shift) {
    if (double_multiplier <= 0.0f) {
        *quantized_multiplier = 0;
        *shift = 0;
        return;
    }
    int exp = 0;
    float q = frexpf(double_multiplier, &exp);
    int64_t q_fixed = static_cast<int64_t>(std::round(q * 2147483648.0f));
    if (q_fixed >= (1LL << 31)) {
        q_fixed /= 2;
        exp++;
    }
    *quantized_multiplier = static_cast<int32_t>(q_fixed);
    *shift = exp;
}

bool dwConvCPUFallback(const RuntimeOp& op, const NPUModel& model) {
    if (!op.instr_ptr || op.instr_size < sizeof(NeonCPUFallbackDWConvInstr)) {
        uart_puts("[ERROR DWConv] instruction too small\r\n");
        return false;
    }

    const auto* instr = static_cast<const NeonCPUFallbackDWConvInstr*>(op.instr_ptr);
    if (instr->fallback_id != NEON_CPU_FALLBACK_DWCONV) {
        uart_puts("[ERROR DWConv] wrong fallback_id\r\n");
        return false;
    }

    const size_t expected_size = sizeof(NeonCPUFallbackDWConvInstr)
                               + (size_t)instr->channels * sizeof(float);
    if (op.instr_size < expected_size) {
        uart_puts("[ERROR DWConv] instr_size < expected_size\r\n");
        return false;
    }

    uart_puts("[Firmware] Executing DWConv CPU fallback...\r\n");
    uart_put_u32("  -> channels=", instr->channels, "");
    uart_put_u32(" ih=", instr->input_h, "");
    uart_put_u32(" iw=", instr->input_w, "");
    uart_put_u32(" kh=", instr->kernel_h, "");
    uart_put_u32(" kw=", instr->kernel_w, "\r\n");

    const int32_t out_h = (int32_t)((instr->input_h + 2u * instr->pad_h - instr->kernel_h) / instr->stride_h) + 1;
    const int32_t out_w = (int32_t)((instr->input_w + 2u * instr->pad_w - instr->kernel_w) / instr->stride_w) + 1;
    uart_put_u32("  -> out_h=", out_h, "");
    uart_put_u32(" out_w=", out_w, "\r\n");

    const int8_t* input = neon_fb::srcPtr<const int8_t>(op, model, instr->src_offset);
    int8_t* output = neon_fb::dstPtr<int8_t>(op, model, instr->dst_offset);

    if (!input || !output || !model.weight_buffer) {
        uart_puts("[ERROR DWConv] null input/output/weight_buffer\r\n");
        return false;
    }

    const int8_t* weight = reinterpret_cast<const int8_t*>(
        static_cast<const uint8_t*>(model.weight_buffer) + instr->weight_offset);

    const int32_t* bias = nullptr;
    if (instr->bias_offset != 0) {
        bias = reinterpret_cast<const int32_t*>(
            static_cast<const uint8_t*>(model.weight_buffer) + instr->bias_offset);
    }

    const uint8_t* raw_weight_scale = reinterpret_cast<const uint8_t*>(instr) + sizeof(NeonCPUFallbackDWConvInstr);

    uart_puts("  -> quantizing multipliers...\r\n");
    int32_t mult[256];
    int shift[256];
    uint32_t num_ch = instr->channels < 256 ? instr->channels : 256;
    for (uint32_t c = 0; c < num_ch; ++c) {
        float w_scale = 1.0f;
        std::memcpy(&w_scale, raw_weight_scale + c * sizeof(float), sizeof(float));
        float eff_scale = (instr->input_scale * w_scale) / instr->output_scale;
        quantizeMultiplier(eff_scale, &mult[c], &shift[c]);
    }

    uart_puts("  -> running loops...\r\n");

    uint16_t dilation_h = instr->dilation_h ? instr->dilation_h : 1;
    uint16_t dilation_w = instr->dilation_w ? instr->dilation_w : 1;

    for (int32_t oh = 0; oh < out_h; ++oh) {
        for (int32_t ow = 0; ow < out_w; ++ow) {
            for (uint32_t c = 0; c < instr->channels; ++c) {
                int32_t acc = bias ? bias[c] : 0;
                for (uint32_t kh = 0; kh < instr->kernel_h; ++kh) {
                    int32_t ih = (int32_t)(oh * instr->stride_h + kh * dilation_h) - (int32_t)instr->pad_h;
                    for (uint32_t kw = 0; kw < instr->kernel_w; ++kw) {
                        int32_t iw = (int32_t)(ow * instr->stride_w + kw * dilation_w) - (int32_t)instr->pad_w;
                        int32_t in_val;
                        if (ih >= 0 && ih < (int32_t)instr->input_h && iw >= 0 && iw < (int32_t)instr->input_w) {
                            in_val = (int32_t)input[(ih * instr->input_w + iw) * instr->channels + c];
                        } else {
                            in_val = instr->input_zero_point;
                        }
                        int32_t w_val = (int32_t)weight[(kh * instr->kernel_w + kw) * instr->channels + c];
                        // TFLite reference: acc += w * (x - input_zero_point). The
                        // subtraction was missing, so every in-bounds tap was biased by
                        // -zp*w and every padding tap contributed zp*w instead of 0.
                        acc += (in_val - instr->input_zero_point) * w_val;
                    }
                }

                int32_t res = multiplyByQuantizedMultiplier(acc, mult[c], shift[c]);
                res += instr->output_zero_point;
                if (res < -128) res = -128;
                if (res > 127) res = 127;
                output[(oh * out_w + ow) * instr->channels + c] = (int8_t)res;
            }
        }
    }

    uart_puts("[Firmware] DWConv CPU fallback executed successfully\r\n");
    return true;
}

NEON_REGISTER_CPU_FALLBACK(NEON_CPU_FALLBACK_DWCONV, dwConvCPUFallback)
