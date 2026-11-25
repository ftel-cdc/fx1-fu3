#include "LayerNormCPUFallback.h"
#include "CPUFallbackRegistry.h"
#include "../../core/NPU_ISA.h"
#include <cstdint>
#include <cmath>

#include "FallbackIO.h"
#include "../support/NeonLog.h"

namespace {
inline int64_t roundShift(int64_t x, int32_t s) {
    if (s <= 0) return (int64_t)((uint64_t)x << (uint32_t)(-s));
    return (x + (1LL << (s - 1))) >> s;
}
inline int32_t clamp8(int64_t v) {
    return v < -128 ? -128 : (v > 127 ? 127 : (int32_t)v);
}
} // namespace

bool layerNormCPUFallback(const RuntimeOp& op, const NPUModel& model) {
    if (!op.instr_ptr || op.instr_size < sizeof(NeonCPUFallbackLayerNormInstr)) {
        uart_puts("[ERROR] layerNormCPUFallback: instruction too small\r\n");
        return false;
    }

    const auto* I = static_cast<const NeonCPUFallbackLayerNormInstr*>(op.instr_ptr);
    if (I->fallback_id != NEON_CPU_FALLBACK_LAYERNORM) {
        uart_puts("[ERROR] layerNormCPUFallback: incorrect fallback_id\r\n");
        return false;
    }
    if (I->axis_size == 0 || I->axis_size > 513) {
        uart_puts("[ERROR] layerNormCPUFallback: axis_size out of range (1..513)\r\n");
        return false;
    }
    if (!model.weight_buffer) {
        uart_puts("[ERROR] layerNormCPUFallback: null weight_buffer\r\n");
        return false;
    }

    const int8_t* input = neon_fb::srcPtr<const int8_t>(op, model, I->src_offset);
    int8_t* output = neon_fb::dstPtr<int8_t>(op, model, I->dst_offset);
    if (!input || !output) {
        uart_puts("[ERROR] layerNormCPUFallback: unbound operand\r\n");
        return false;
    }

    const uint8_t* wb = static_cast<const uint8_t*>(model.weight_buffer);
    const uint16_t* lut   = reinterpret_cast<const uint16_t*>(wb + I->rsqrt_lut_offset);
    const int8_t*   gamma = reinterpret_cast<const int8_t*>(wb + I->gamma_offset);

    // bias[] is int32 and must be 4-byte aligned; the emitter calls
    // alignWeights(16) before appending it, so this only fires on a
    // hand-built or corrupted model.
    if ((reinterpret_cast<uintptr_t>(wb + I->bias_offset) & 0x3u) != 0) {
        uart_puts("[ERROR] layerNormCPUFallback: bias_offset not 4-byte aligned\r\n");
        return false;
    }
    const int32_t* bias = reinterpret_cast<const int32_t*>(wb + I->bias_offset);

    const uint32_t H = I->axis_size;

    for (uint32_t row = 0; row < I->outer_size; ++row) {
        const int8_t* src = input + (size_t)row * H;
        int8_t*       dst = output + (size_t)row * H;

        // (1)(2)(3) — exact integer mean/variance; Z_in cancels, H need not be 2^k
        int64_t SQ = 0;
        for (uint32_t j = 0; j < H; ++j) SQ += src[j];
        int64_t SD2 = 0;
        for (uint32_t j = 0; j < H; ++j) {
            int64_t d = (int64_t)H * src[j] - SQ;      // = H*(q - mean)
            SD2 += d * d;
        }

        // (4)(5)(6)
        const double Hf = (double)H;
        double var = I->s_in * I->s_in * (double)SD2 / (Hf * Hf * Hf);
        int32_t q_var = clamp8((int64_t)llround(var / I->s_var) + I->z_var);
        double  vdq   = I->s_var * (double)(q_var - I->z_var);
        int32_t q7    = clamp8((int64_t)llround((vdq + I->eps) / I->s_7) + I->z_7);
        int32_t u     = q7 - I->z_7;                    // 0..255

        if (u <= 0) {                                   // var+eps == 0 — degenerate row
            for (uint32_t j = 0; j < H; ++j) dst[j] = (int8_t)clamp8(I->output_zero_point);
            continue;
        }

        // (8)(9)(10) — PWL 1/sqrt, pwl_lut_mapping_en.md Part B
        int32_t E = 31 - (int32_t)__builtin_clz((uint32_t)u);   // bit_length(u)-1
        int32_t parity = E & 1, E_half = E >> 1;
        uint32_t raw = (E < 16) ? (((uint32_t)u << (16 - E)) & 0xFFFFu)
                                : (((uint32_t)u >> (E - 16)) & 0xFFFFu);
        uint32_t sub_idx = raw >> 8;
        uint32_t addr = ((uint32_t)parity << 8) | sub_idx;
        // Degenerate interpolation theorem: frac == 0 identically for all u in [1, 255].
        // LERP is inert: r = lut[addr] (Full-flow LayerNorm.md §5.2).
        const int32_t r = lut[addr];

        // (11) — three requant stages; E_half merged into the div shift
        for (uint32_t j = 0; j < H; ++j) {
            int64_t d = (int64_t)H * src[j] - SQ;
            int64_t a  = roundShift(((int64_t)d * (int64_t)r) * (int64_t)I->m0_div,
                                    I->total_shift_div + E_half);
            int32_t q_div = clamp8(a + I->z_div);
            int64_t a2 = roundShift((int64_t)(q_div - I->z_div) * (int64_t)gamma[j] * (int64_t)I->m0_mul,
                                    I->total_shift_mul);
            int32_t q_mul = clamp8(a2 + I->z_mul);
            // Stage 4: plain int32 add — beta was dequantized by the compiler (§4.2.5).
            dst[j] = (int8_t)clamp8((int64_t)(q_mul - I->z_mul)
                                    + (int64_t)bias[j]
                                    + (int64_t)I->output_zero_point);
        }
    }

    uart_puts("[Firmware] LayerNorm CPU fallback executed successfully\r\n");
    return true;
}

NEON_REGISTER_CPU_FALLBACK(NEON_CPU_FALLBACK_LAYERNORM, layerNormCPUFallback)
