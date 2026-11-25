#include "SoftwareRequantFallback.h"
#include "FX1V42Regs.h"
#include <cstring>
extern "C" {
#include "uart.h"
}

static inline int32_t multiplyByQuantizedMultiplier(int32_t x, int32_t multiplier, int shift) {
    int64_t total_mult = static_cast<int64_t>(x) * static_cast<int64_t>(multiplier);
    // The multiplier is a Q31 value. We must scale down by 2^31 (equivalent to TFLite's DoublingHighMul)
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

namespace {

// Fused-requant ADD / SUB block ("Full-flow Add (Optimized).md" §2.4, §11), written by
// AddFusedParamSerializer:
//   [0] m_a  [1] m_b  [2] bias_lo  [3] bias_hi  [4] n  [5] z_c  [6] magic  [7] reserved
struct AddFusedParams {
    int32_t m_a;
    int32_t m_b;
    int64_t bias;
    int32_t n;
    int32_t z_c;
};

inline AddFusedParams loadAddFusedParams(const int32_t* p) {
    AddFusedParams r;
    r.m_a = p[0];
    r.m_b = p[1];
    r.bias = static_cast<int64_t>(
        (static_cast<uint64_t>(static_cast<uint32_t>(p[3])) << 32) |
        static_cast<uint64_t>(static_cast<uint32_t>(p[2])));
    r.n = p[4];
    r.z_c = p[5];
    return r;
}

// q_c = clamp((acc >> n) + z_c): the single rounding of the whole op (round half up,
// the 2^(n-1) is already inside bias).
inline int8_t addFusedFinish(int64_t acc, int32_t n, int32_t z_c) {
    int64_t y = (acc >> n) + z_c;
    if (y < -128) y = -128;
    if (y > 127) y = 127;
    return static_cast<int8_t>(y);
}

constexpr size_t kAddLogEvery = 50000;

inline void addLogProgress(size_t i, size_t &next_log) {
    if (i == next_log) {
        uart_puts("[SoftwareRequantFallback] Processed ");
        uart_put_u32("", static_cast<uint32_t>(i), " elements...\r\n");
        next_log += kAddLogEvery;
    }
}

} // namespace

void SoftwareRequantFallback::requantizeINT32ToINT8(const int32_t* src_int32,
                                                 int8_t*        dst_int8,
                                                 size_t         count,
                                                 int32_t        multiplier,
                                                 int            shift,
                                                 int32_t        zero_point,
                                                 int8_t         act_min,
                                                 int8_t         act_max) {
    if (!src_int32 || !dst_int8) return;
    for (size_t i = 0; i < count; ++i) {
        // Debug print the first channel's OBP params to trace the issue
        if (i == 0) {
            uart_put_u32("[SoftwareRequantFallback] Channel 0 | Mult: ", static_cast<uint32_t>(multiplier), "");
            uart_put_u32(" | Shift: ", static_cast<uint32_t>(shift), "\r\n");
        }

        int32_t acc = src_int32[i];
        acc = multiplyByQuantizedMultiplier(acc, multiplier, shift);
        acc += zero_point;
        if (acc < act_min) acc = static_cast<int32_t>(act_min);
        if (acc > act_max) acc = static_cast<int32_t>(act_max);
        dst_int8[i] = static_cast<int8_t>(acc);
    }
}

void SoftwareRequantFallback::requantizeTileINT32ToINT8(const int32_t* src_int32,
                                                     int8_t*        dst_int8,
                                                     size_t         count,
                                                     const int32_t* bias,
                                                     const int32_t* multiplier,
                                                     const int32_t* shift,
                                                     int32_t        output_zero_point,
                                                     int8_t         act_min,
                                                     int8_t         act_max,
                                                     const uint8_t* lut_data) {
    if (!src_int32 || !dst_int8) return;
    for (size_t i = 0; i < count; ++i) {
        size_t c = i % FX1_SA_TILE;
        int32_t acc = src_int32[i];

        int32_t mult = multiplier ? multiplier[c] : 1;
        int sh = shift ? static_cast<int>(shift[c]) : 0;
        
        if (bias) {
            acc += bias[c];
        }
        if (multiplier) {
            const int rs = (sh < 0) ? -sh : sh;
            if (rs == 30) {
                int64_t prod = static_cast<int64_t>(acc) * static_cast<int64_t>(mult);
                acc = static_cast<int32_t>((prod + (1LL << 29)) >> 30);
            } else {
                acc = multiplyByQuantizedMultiplier(acc, mult, sh);
            }
        }

        acc += output_zero_point;
        if (lut_data) {
            if (acc < -128) acc = -128;
            if (acc > 127) acc = 127;
            acc = static_cast<int8_t>(lut_data[c * 256 + static_cast<uint8_t>(acc + 128)]);
        } else {
            if (acc < act_min) acc = static_cast<int32_t>(act_min);
            if (acc > act_max) acc = static_cast<int32_t>(act_max);
        }
        dst_int8[i] = static_cast<int8_t>(acc);
    }
}

void SoftwareRequantFallback::executeADD_broadcast(const int8_t* input_a,
                                                    int8_t* output,
                                                    size_t count,
                                                    const int32_t* params) {
    if (!input_a || !output || !params) return;
    const AddFusedParams p = loadAddFusedParams(params);
    size_t next_log = kAddLogEvery;

    if (p.n == 0 && p.m_a == 1) {
        // S_A == S_C and an integer constant B: q_c = clamp(q_a + bias + z_c).
        const int32_t k0 = static_cast<int32_t>(p.bias) + p.z_c;
        for (size_t i = 0; i < count; ++i) {
            addLogProgress(i, next_log);
            int32_t y = static_cast<int32_t>(input_a[i]) + k0;
            if (y < -128) y = -128;
            if (y > 127) y = 127;
            output[i] = static_cast<int8_t>(y);
        }
        return;
    }

    for (size_t i = 0; i < count; ++i) {
        addLogProgress(i, next_log);
        const int64_t acc = static_cast<int64_t>(input_a[i]) * p.m_a + p.bias;
        output[i] = addFusedFinish(acc, p.n, p.z_c);
    }
}

void SoftwareRequantFallback::executeADD_tensor(const int8_t* input_a,
                                                 const int8_t* input_b,
                                                 int8_t* output,
                                                 size_t count,
                                                 const int32_t* params) {
    if (!input_a || !input_b || !output || !params) return;
    const AddFusedParams p = loadAddFusedParams(params);
    size_t next_log = kAddLogEvery;

    if (p.n == 0 && p.m_a == 1 && (p.m_b == 1 || p.m_b == -1)) {
        // S_A == S_B == S_C: q_c = clamp(q_a +/- q_b + bias + z_c), no multiply, no rounding
        // (m_b = +1 for ADD, -1 for SUB).
        const int32_t k0 = static_cast<int32_t>(p.bias) + p.z_c;
        const int32_t sb = p.m_b;
        for (size_t i = 0; i < count; ++i) {
            addLogProgress(i, next_log);
            int32_t y = static_cast<int32_t>(input_a[i]) + sb * static_cast<int32_t>(input_b[i]) + k0;
            if (y < -128) y = -128;
            if (y > 127) y = 127;
            output[i] = static_cast<int8_t>(y);
        }
        return;
    }

    if (p.m_b == p.m_a || p.m_b == -p.m_a) {
        // S_A == S_B: one multiply on q_a +/- q_b (ADD: +, SUB: -).
        const int32_t sb = (p.m_b == p.m_a) ? 1 : -1;
        for (size_t i = 0; i < count; ++i) {
            addLogProgress(i, next_log);
            const int32_t s = static_cast<int32_t>(input_a[i]) + sb * static_cast<int32_t>(input_b[i]);
            const int64_t acc = static_cast<int64_t>(s) * p.m_a + p.bias;
            output[i] = addFusedFinish(acc, p.n, p.z_c);
        }
        return;
    }

    for (size_t i = 0; i < count; ++i) {
        addLogProgress(i, next_log);
        const int64_t acc = static_cast<int64_t>(input_a[i]) * p.m_a +
                            static_cast<int64_t>(input_b[i]) * p.m_b + p.bias;
        output[i] = addFusedFinish(acc, p.n, p.z_c);
    }
}

void SoftwareRequantFallback::executeMUL_broadcast(const int8_t* input_a,
                                                    int8_t input_b_scalar,
                                                    int8_t* output,
                                                    size_t count,
                                                    const int32_t* params) {
    if (!input_a || !output || !params) return;

    const int32_t m0_c = params[0];
    const int32_t shift_c = params[1];
    const int32_t z_a = params[2];
    const int32_t z_b = params[3];
    const int32_t z_c = params[4];

    const int32_t b_val = static_cast<int32_t>(input_b_scalar) - z_b;

    for (size_t i = 0; i < count; ++i) {
        const int32_t a_val = static_cast<int32_t>(input_a[i]) - z_a;
        const int32_t raw = a_val * b_val;
        int32_t acc = multiplyByQuantizedMultiplier(raw, m0_c, shift_c);
        acc += z_c;

        if (acc < -128) acc = -128;
        if (acc > 127) acc = 127;
        output[i] = static_cast<int8_t>(acc);
    }
}

void SoftwareRequantFallback::executeMUL_tensor(const int8_t* input_a,
                                                 const int8_t* input_b,
                                                 int8_t* output,
                                                 size_t count,
                                                 const int32_t* params) {
    if (!input_a || !input_b || !output || !params) return;

    const int32_t m0_c = params[0];
    const int32_t shift_c = params[1];
    const int32_t z_a = params[2];
    const int32_t z_b = params[3];
    const int32_t z_c = params[4];

    for (size_t i = 0; i < count; ++i) {
        const int32_t a_val = static_cast<int32_t>(input_a[i]) - z_a;
        const int32_t b_val = static_cast<int32_t>(input_b[i]) - z_b;
        const int32_t raw = a_val * b_val;
        int32_t acc = multiplyByQuantizedMultiplier(raw, m0_c, shift_c);
        acc += z_c;

        if (acc < -128) acc = -128;
        if (acc > 127) acc = 127;
        output[i] = static_cast<int8_t>(acc);
    }
}
