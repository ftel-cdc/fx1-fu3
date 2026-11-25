#include "Conv2DCPUFallback.h"
#include "CPUFallbackRegistry.h"
#include "../../analyse/compiler/CompilerOpConv2D.h"

#include <cstdio>
#include <cstring>
#include <vector>

namespace {

// Must stay <= the static buffers' real capacity in
// device/qemu_mps3_an547/analyse/compiler/QEMUCompilerOpConv2D.c
// (CH_OUT_MAX / IM2COL_MAX are private to that .c file, not exposed via
// analyse/compiler/CompilerOpConv2D.h's contract, so these are duplicated
// constants, not shared ones — keep in sync by hand if that file changes).
// A real MLIR-generated conv2D_Func() may have entirely
// different limits; these bounds only protect against the specific
// software reference implementation's fixed-size scratch buffers.
constexpr uint32_t kQEMURefMaxChannelsOut = 512;
constexpr size_t   kQEMURefMaxIm2colElems = 28 * 28 * 9 * 64;

} // namespace

bool conv2DCPUFallback(const RuntimeOp& op, const NPUModel& model) {
    if (!op.instr_ptr || op.instr_size < sizeof(NeonCPUFallbackConv2DInstr)) {
        fprintf(stderr, "[ERROR] conv2DCPUFallback: instruction too small (op index=%u)\n", op.index);
        return false;
    }

    const auto* instr = static_cast<const NeonCPUFallbackConv2DInstr*>(op.instr_ptr);
    if (instr->fallback_id != NEON_CPU_FALLBACK_CONV2D) {
        fprintf(stderr, "[ERROR] conv2DCPUFallback: called for wrong fallback_id=%u (op index=%u)\n",
                instr->fallback_id, op.index);
        return false;
    }

    // Trailing per-output-channel weight scale array — see
    // NeonCPUFallbackConv2DInstr's own comment in core/NPU_ISA.h.
    const size_t expected_size = sizeof(NeonCPUFallbackConv2DInstr)
                               + (size_t)instr->channels_out * sizeof(float);
    if (op.instr_size != expected_size) {
        fprintf(stderr,
                "[ERROR] conv2DCPUFallback: instr_size=%zu != expected=%zu "
                "(channels_out=%u) — malformed instruction (op index=%u)\n",
                op.instr_size, expected_size, instr->channels_out, op.index);
        return false;
    }
    if (instr->channels_out > kQEMURefMaxChannelsOut) {
        fprintf(stderr,
                "[ERROR] conv2DCPUFallback: channels_out=%u exceeds this build's "
                "reference conv2D_Func() capacity (%u) — would overflow its "
                "internal scratch buffers (op index=%u)\n",
                instr->channels_out, kQEMURefMaxChannelsOut, op.index);
        return false;
    }

    // conv2D_Func()'s own contract has no dilation and assumes batch=1
    // (NPUModel::TensorDesc.dims[]/strides[] are never populated anywhere
    // in this repo today — see CLAUDE.md's Known Issues — so batch isn't
    // even readable from the tensor descriptor; batch=1 matches every
    // other real usage in this repo).
    const int32_t batch  = 1;
    const int32_t out_h  = (int32_t)((instr->input_h + 2u * instr->pad_h - instr->kernel_h)
                                      / instr->stride_h) + 1;
    const int32_t out_w  = (int32_t)((instr->input_w + 2u * instr->pad_w - instr->kernel_w)
                                      / instr->stride_w) + 1;

    const size_t im2col_elems = (size_t)batch * out_h * out_w
                              * (size_t)instr->kernel_h * instr->kernel_w * instr->channels_in;
    if (im2col_elems > kQEMURefMaxIm2colElems) {
        fprintf(stderr,
                "[ERROR] conv2DCPUFallback: im2col size=%zu exceeds this build's "
                "reference conv2D_Func() capacity (%zu) — would overflow its "
                "internal scratch buffer (op index=%u)\n",
                im2col_elems, kQEMURefMaxIm2colElems, op.index);
        return false;
    }

    if (op.input_tensor_idx  >= model.tensors.size()
     || op.output_tensor_idx >= model.tensors.size()) {
        fprintf(stderr, "[ERROR] conv2DCPUFallback: tensor index out of range (op index=%u)\n", op.index);
        return false;
    }
    const int8_t* input  = static_cast<const int8_t*>(model.tensors[op.input_tensor_idx].data);
    int8_t*       output = static_cast<int8_t*>(model.tensors[op.output_tensor_idx].data);
    if (!input || !output || !model.weight_buffer) {
        fprintf(stderr, "[ERROR] conv2DCPUFallback: null input/output/weight_buffer (op index=%u)\n", op.index);
        return false;
    }

    const int8_t* weight = reinterpret_cast<const int8_t*>(
        static_cast<const uint8_t*>(model.weight_buffer) + instr->weight_offset);

    // bias_offset==0 means "no bias" (same convention NeonConv2DInstr
    // already uses) — conv2D_Func() itself requires a non-null bias
    // pointer unconditionally, so synthesize a zero-filled one rather than
    // passing nullptr.
    std::vector<int32_t> zero_bias;
    const int32_t* bias;
    if (instr->bias_offset != 0) {
        bias = reinterpret_cast<const int32_t*>(
            static_cast<const uint8_t*>(model.weight_buffer) + instr->bias_offset);
    } else {
        zero_bias.assign(instr->channels_out, 0);
        bias = zero_bias.data();
    }

    // Trailing per-channel weight scale array, right after the fixed header.
    const float* weight_scale = reinterpret_cast<const float*>(
        reinterpret_cast<const uint8_t*>(instr) + sizeof(NeonCPUFallbackConv2DInstr));

    // int8_t locals for QuantInfo.zero_points (CompilerOpConv2D.h's own
    // type — not this instruction's int32_t fields).
    const int8_t input_zp  = (int8_t)instr->input_zero_point;
    const int8_t output_zp = (int8_t)instr->output_zero_point;
    float input_scale  = instr->input_scale;
    float output_scale = instr->output_scale;

    Conv2DQuantParams qp{};
    qp.input_q.scales      = &input_scale;
    qp.input_q.zero_points = const_cast<int8_t*>(&input_zp);
    // weight_q.zero_points intentionally left null: this reference
    // conv2D_Func() sums raw INT8 weights with no zero-point subtraction
    // (see device/qemu_mps3_an547/analyse/compiler/QEMUCompilerOpConv2D.c)
    // — i.e. it assumes symmetric (zero_point=0) per-channel weight
    // quantization and never reads this field.
    qp.weight_q.scales      = const_cast<float*>(weight_scale);
    qp.weight_q.zero_points = nullptr;
    // bias_q intentionally left null: also never read by this
    // implementation (bias is pre-scaled to int32 accumulator units).
    qp.bias_q.scales      = nullptr;
    qp.bias_q.zero_points  = nullptr;
    qp.output_q.scales      = &output_scale;
    qp.output_q.zero_points = const_cast<int8_t*>(&output_zp);

    Conv2DShape shape{};
    shape.batch     = batch;
    shape.in_h      = (int32_t)instr->input_h;
    shape.in_w      = (int32_t)instr->input_w;
    shape.in_c      = (int32_t)instr->channels_in;
    shape.filter_h  = (int32_t)instr->kernel_h;
    shape.filter_w  = (int32_t)instr->kernel_w;
    shape.out_h     = out_h;
    shape.out_w     = out_w;
    shape.out_c     = (int32_t)instr->channels_out;
    shape.stride_h  = instr->stride_h;
    shape.stride_w  = instr->stride_w;
    shape.pad_h     = instr->pad_h;
    shape.pad_w     = instr->pad_w;

    conv2D_Func(input, weight, bias, output, &qp, &shape);
    return true;
}

NEON_REGISTER_CPU_FALLBACK(NEON_CPU_FALLBACK_CONV2D, conv2DCPUFallback)
