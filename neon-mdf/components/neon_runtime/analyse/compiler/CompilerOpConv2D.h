#pragma once

// ============================================================
// NEON STUDIO — COMPILER OP: CONV2D
// Interface contract between compiler-generated .o and analyser.
//
// Implemented by:
//   - MLIR-generated .o (just_accel / linalg_memref) for VP_FX1_V2.0
//   - device/qemu_mps3_an547/analyse/compiler/QEMUCompilerOpConv2D.c
//     software fallback for testing (see file header there)
//
// Used by:
//   - analyse/compiler/CompilerMeasure.h  → eval tiling efficiency
//   - runtime/executor/Executor           → dispatch conv2d op
// ============================================================

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================
// QUANTIZATION INFO
// Per-channel scale and zero-point arrays for one tensor.
// ============================================================

typedef struct {
    float*  scales;
    int8_t* zero_points;
} QuantInfo;

// Quantization parameters for all Conv2D tensors.
typedef struct {
    QuantInfo input_q;
    QuantInfo weight_q;
    QuantInfo bias_q;
    QuantInfo output_q;
} Conv2DQuantParams;

// ============================================================
// CONV2D SHAPE
// Spatial and channel dimensions.
// MLIR-generated kernels receive this but may ignore it
// (shapes are baked in the IR at compile time).
// ============================================================

typedef struct {
    int32_t batch;
    int32_t in_h,     in_w,     in_c;
    int32_t filter_h, filter_w;
    int32_t out_h,    out_w,    out_c;
    int32_t stride_h, stride_w;
    int32_t pad_h,    pad_w;
} Conv2DShape;

// ============================================================
// CONV2D FUNCTION
// Linked from compiler-generated .o file.
// ============================================================

void conv2D_Func(
    const int8_t*            input,
    const int8_t*            weight,
    const int32_t*           bias,
    int8_t*                  output,
    const Conv2DQuantParams* quant_params,
    const Conv2DShape*       shape
);

#ifdef __cplusplus
}
#endif
