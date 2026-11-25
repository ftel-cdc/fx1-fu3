#include "SoftmaxCPUFallback.h"
#include "CPUFallbackRegistry.h"
#include "../../core/NPU_ISA.h"
#include <cstdint>

#include "FallbackIO.h"
#include "../support/NeonLog.h"

namespace {
constexpr uint32_t kMaxAxisSize = 256;

// (x + (1 << (s-1))) >> s for s > 0; x << (-s) for s <= 0 — matches
// Softmax.cpp's own round_shift construction exactly (both the inner
// prob_fixed shift, always s>0, and the final rescale, either sign).
int64_t roundShift(int64_t x, int32_t s) {
    if (s <= 0) {
        return x << (-s);
    }
    int64_t half = 1LL << (s - 1);
    return (x + half) >> s;
}
}  // namespace

bool softmaxCPUFallback(const RuntimeOp& op, const NPUModel& model) {
    if (!op.instr_ptr || op.instr_size < sizeof(NeonCPUFallbackSoftmaxInstr)) {
        uart_puts("[ERROR] softmaxCPUFallback: instruction too small\r\n");
        return false;
    }

    const auto* instr = static_cast<const NeonCPUFallbackSoftmaxInstr*>(op.instr_ptr);
    if (instr->fallback_id != NEON_CPU_FALLBACK_SOFTMAX) {
        uart_puts("[ERROR] softmaxCPUFallback: incorrect fallback_id\r\n");
        return false;
    }
    if (instr->axis_size == 0 || instr->axis_size > kMaxAxisSize) {
        uart_puts("[ERROR] softmaxCPUFallback: axis_size out of range\r\n");
        return false;
    }
    if (!model.weight_buffer) {
        uart_puts("[ERROR] softmaxCPUFallback: null weight_buffer\r\n");
        return false;
    }

    const int8_t* input = neon_fb::srcPtr<const int8_t>(op, model, instr->src_offset);
    int8_t* output = neon_fb::dstPtr<int8_t>(op, model, instr->dst_offset);
    if (!input || !output) {
        uart_puts("[ERROR] softmaxCPUFallback: unbound operand\r\n");
        return false;
    }

    const uint8_t* exp_lut = static_cast<const uint8_t*>(model.weight_buffer) + instr->exp_lut_offset;
    const uint16_t* recip_lut = reinterpret_cast<const uint16_t*>(
        static_cast<const uint8_t*>(model.weight_buffer) + instr->recip_lut_offset);

    int32_t e_scratch[kMaxAxisSize];

    for (uint32_t outer = 0; outer < instr->outer_size; ++outer) {
        for (uint32_t inner = 0; inner < instr->inner_size; ++inner) {
            const int8_t* row_in = input + (static_cast<uint64_t>(outer) * instr->axis_size * instr->inner_size + inner);
            int8_t* row_out = output + (static_cast<uint64_t>(outer) * instr->axis_size * instr->inner_size + inner);

            // Pass 1: max
            int32_t m = -128;
            for (uint32_t a = 0; a < instr->axis_size; ++a) {
                int32_t v = row_in[static_cast<uint64_t>(a) * instr->inner_size];
                if (v > m) m = v;
            }

            // Pass 2: exp lookup + sum
            int32_t sum = 0;
            for (uint32_t a = 0; a < instr->axis_size; ++a) {
                int32_t x = row_in[static_cast<uint64_t>(a) * instr->inner_size];
                int32_t k = m - x; // always in [0,255]: m,x in [-128,127]
                int32_t ev = exp_lut[k];
                e_scratch[a] = ev;
                sum += ev;
            }

            // Pass 3 — PWL reciprocal, pwl_lut_mapping_en.md §2.1-§2.4 (Part A).
            //
            // sum is never 0: the row's own max element contributes
            // exp_lut[0] = 255, so sum >= 255 and e_exp >= 7. With
            // axis_size <= 256 and exp_lut <= 255, sum <= 65280 and
            // e_exp <= 15, so the E >= W branch is unreachable here; it is
            // kept because the spec's stage 2 is defined for both.
            const uint32_t usum = static_cast<uint32_t>(sum);
            const int32_t  e_exp = 31 - __builtin_clz(usum);    // §2.1: E = cfg_e_bias - clz

            constexpr int kK = 8;                 // table index width  -> 256 entries
            constexpr int kF = 8;                 // interpolation fraction width
            constexpr int kW = kK + kF;           // §2.2: W = 16
            constexpr uint32_t kWMask = (1u << kW) - 1u;

            const uint32_t raw = (e_exp < kW)
                ? ((usum << (kW - e_exp)) & kWMask)
                : ((usum >> (e_exp - kW)) & kWMask);
            const uint32_t idx  = raw >> kF;      // 0..255
            const uint32_t frac = raw & ((1u << kF) - 1u);

            // §2.3 — two adjacent reads plus a blend. At idx = 255 there is
            // no idx+1 to blend toward, so SATURATE to the stored value.
            // Clamping idx while leaving frac measured against a different
            // segment's origin is a real bug (0.195 % vs 0.0084 %) already
            // documented in hwsim/fx1_vlu.c:155-172.
            int32_t recip;
            if (idx >= 255u) {
                recip = recip_lut[255];
            } else {
                const int32_t t0 = recip_lut[idx];
                const int32_t t1 = recip_lut[idx + 1u];
                recip = t0 + static_cast<int32_t>(
                    (static_cast<int64_t>(t1 - t0) * static_cast<int64_t>(frac)) >> kF);
            }
            const int32_t total_shift = e_exp;    // §2.4: 1/x reapplies E in full

            // Pass 4: finalize per element
            for (uint32_t a = 0; a < instr->axis_size; ++a) {
                int64_t prod = static_cast<int64_t>(e_scratch[a]) * static_cast<int64_t>(recip);
                int64_t prob_fixed = roundShift(prod, total_shift);

                int64_t prod_out = prob_fixed * static_cast<int64_t>(instr->m0_out);
                int64_t shifted_out = roundShift(prod_out, instr->total_shift_out);

                int32_t out32 = static_cast<int32_t>(shifted_out) + instr->output_zero_point;
                if (out32 < -128) out32 = -128;
                if (out32 > 127) out32 = 127;
                row_out[static_cast<uint64_t>(a) * instr->inner_size] = static_cast<int8_t>(out32);
            }
        }
    }

    uart_puts("[Firmware] Softmax CPU fallback executed successfully\r\n");
    return true;
}

NEON_REGISTER_CPU_FALLBACK(NEON_CPU_FALLBACK_SOFTMAX, softmaxCPUFallback)
