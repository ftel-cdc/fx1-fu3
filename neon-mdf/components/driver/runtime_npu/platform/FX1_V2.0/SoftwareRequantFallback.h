#pragma once

// ============================================================
// SOFTWARE REQUANT FALLBACK
// Fallback path — chỉ dùng khi HW path disabled hoặc NEON_REQUANT_VERIFY
// ============================================================

#include <stddef.h>
#include <stdint.h>

class SoftwareRequantFallback {
public:
  // Multiply by quantized multiplier and scale INT32 accumulators to INT8
  // output.
  static void requantizeINT32ToINT8(const int32_t *src_int32, int8_t *dst_int8,
                                    size_t count, int32_t multiplier, int shift,
                                    int32_t zero_point, int8_t act_min = -128,
                                    int8_t act_max = 127);

  // Per-channel in-place or buffer-to-buffer tile requantization
  static void requantizeTileINT32ToINT8(
      const int32_t *src_int32, int8_t *dst_int8, size_t count,
      const int32_t *bias, const int32_t *multiplier, const int32_t *shift,
      int32_t output_zero_point, int8_t act_min = -128, int8_t act_max = 127,
      const uint8_t *lut_data = nullptr);

  // Quantized ADD and SUB, fused requant ("Full-flow Add (Optimized).md" §2.4,
  // §11). SUB is the same kernel: the compiler emits m_b < 0.
  // `params` is the NEON_ADD_FUSED_PARAMS_WORDS-int32 block written by
  // AddFusedParamSerializer (layout in core/NPU_ISA.h):
  //   [0] m_a  [1] m_b  [2] bias_lo  [3] bias_hi  [4] n  [5] z_c  [6] magic  [7] 0
  //   q_c = clamp(((q_a * m_a + q_b * m_b + bias) >> n) + z_c)
  // The arithmetic must match TopToLinalg/OpLowering/Add.cpp operation for
  // operation.
  //
  // Element-wise variant: input_b is a full tensor of the same element count as
  // input_a. This is the shape real models use — every Add in YOLOv8 is a C2f
  // residual joining two same-shaped activations.
  static void executeADD_tensor(const int8_t *input_a, const int8_t *input_b,
                                int8_t *output, size_t count,
                                const int32_t *params);

  // Scalar-B variant: B is a compile-time constant already folded into bias
  // (m_b == 0), so only input_a is read.
  static void executeADD_broadcast(const int8_t *input_a, int8_t *output,
                                   size_t count, const int32_t *params);

  // `params` is the 5-int32 block written by MulParamSerializer, in order:
  //   [0] m0_c   [1] shift_c   [2] z_a   [3] z_b   [4] z_c
  // out = clamp(round(M * (q_a - z_a) * (q_b - z_b)) + z_c), M encoded as
  // (m0_c, shift_c) via the Q31 multiplyByQuantizedMultiplier() convention.
  static void executeMUL_broadcast(const int8_t *input_a, int8_t input_b_scalar,
                                   int8_t *output, size_t count,
                                   const int32_t *params);

  static void executeMUL_tensor(const int8_t *input_a, const int8_t *input_b,
                                int8_t *output, size_t count,
                                const int32_t *params);

};
