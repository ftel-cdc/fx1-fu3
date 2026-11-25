#pragma once

#include <stdint.h>
#include <stddef.h>
#include <math.h>   // frexp — quantizeMultiplier()

// ============================================================
// NEON STUDIO — NPU ISA DEFINITION
// Source of truth for compiler (toolkit) and runtime.
// Both sides must include this file. Never hardcode opcodes
// or struct layouts outside of this header.
// ============================================================

#define NEON_ISA_VERSION_MAJOR 1
// ISA Minor Version: bumped to 20 (HAS_ELEMWISE: ELEM_WISE ADD / MAX_POOL on the V4.7 HasNpuTop vector unit)
#define NEON_ISA_VERSION_MINOR 20

// Physical activation layout carried by layout-sensitive instructions.
// 0 is the pre-1.17 default so older .nsm files keep decoding as NHWC.
#define NEON_LAYOUT_NHWC 0u
#define NEON_LAYOUT_NCHW 1u

// ============================================================
// DATA TYPES & OPCODES — C++ enum class
// Defined BEFORE extern "C" so struct fields can use these types
// while the structs themselves remain C-linkage compatible.
// Underlying type = uint8_t → same binary representation.
// ============================================================

#ifdef __cplusplus

enum class NeonDtype : uint8_t {
    FLOAT32 = 0x00,
    INT8    = 0x01,
    INT4    = 0x02,
    INT16   = 0x03,
    UINT8   = 0x04,
};

enum class NeonOpcode : uint8_t {
    // Convolution
    CONV2D         = 0x01,
    DEPTHWISE_CONV = 0x02,
    NATIVE_CONFIG  = 0x03,
    // Whole convolution layer on the V4.7 HasNpuTop controller (extended GEMM_FUSED):
    // the hardware walks the tiles and splits K = kh*kw*C_in into IN_C/TILE_CIN passes
    // that accumulate in SRAM-C. Emitted in native dispatch for Conv2D and FC layers
    // whose weight block does not fit one SRAM-B buffer. Struct: NeonHasConv2DInstr.
    HAS_CONV2D     = 0x04,
    // Whole-tensor ELEM_WISE on the V4.7 HasNpuTop vector unit (GvuTop): residual ADD (also SUB, an
    // ADD with a negative B multiplier) and MAX_POOL. Struct: NeonHasElemwiseInstr.
    HAS_ELEMWISE   = 0x07,

    // Hardware Output Buffer Processor (OBP) / RCE Control
    OBP_CONFIG     = 0x06,

    // Activation
    RELU           = 0x10,
    RELU6          = 0x11,
    SIGMOID        = 0x16,
    TANH           = 0x17,

    // SAURIA V4.2 Rich Instructions (Rev5)
    GEMM_FUSED     = 0x12,
    FUSED_ATTN     = 0x13,
    LAYERNORM      = 0x14,
    ELEM_WISE      = 0x15,

    // Pooling
    MAXPOOL        = 0x20,
    AVGPOOL        = 0x21,

    // Linear
    FC             = 0x30,   // Fully Connected

    // Normalization
    SOFTMAX        = 0x40,
    BATCHNORM      = 0x41,

    // Element-wise / Reduction
    ADD            = 0x50,
    MUL            = 0x51,
    SUB            = 0x52,
    SUM            = 0x53,

    // Memory
    DMA_READ       = 0x60,   // DRAM → SRAM
    DMA_WRITE      = 0x61,   // SRAM → DRAM

    // Dual-model spatial split control (Rev2 ISA)
    // SET_NSPLIT is a barrier instruction: decoder waits for both lanes
    // to drain before updating N_split_reg. Emitted once at session init,
    // then only when partition changes between layer pairs.
    SET_NSPLIT     = 0x05,

    // Escape hatch — CPU fallback for ops with no NPU realization at all
    CPU_FALLBACK   = 0x70,

    // Runtime Loop Contract (Rev6 — ISA 1.11)
    LOOP_BEGIN     = 0x80,
    LOOP_END       = 0x81,

    // Control
    NOP            = 0xFE,
    HALT           = 0xFF,
};

#else

typedef uint8_t NeonDtype;
typedef uint8_t NeonOpcode;

#endif // __cplusplus

// ============================================================
// INSTRUCTION STRUCTS
// All structs are packed — no padding, no alignment surprise.
// Compiler writes these bytes; runtime reads them as-is.
// opcode field: NeonOpcode (enum class : uint8_t, sizeof = 1).
// dtype  field: NeonDtype  (enum class : uint8_t, sizeof = 1).
// ============================================================

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================
// PER-CHANNEL REQUANTISATION  (Rev5)
//
// The quantisation scheme is the standard TFLite/ONNX one:
//   input   per-tensor   (one scale + one zero point)
//   weights per-channel  (one scale per output channel, zero point 0)
//   output  per-tensor   (one scale + one zero point)
//
// The arithmetic an INT8 conv/FC has to perform is:
//
//   acc[c] = SUM (q_in - in_zp) * q_w[c]                    (INT32)
//   q_out  = round( acc[c] * M[c] ) + out_zp
//   M[c]   = in_scale * w_scale[c] / out_scale              (per-channel)
//
// NOTHING in that expression reaches the device as a separate field, and
// that is deliberate. Two compile-time foldings collapse it:
//
//   1. Zero point folds into bias.
//        SUM (q_in - in_zp)*q_w = SUM q_in*q_w  -  in_zp * SUM q_w
//      The second term is a per-channel constant, so the toolkit emits
//      bias_adj[c] = bias[c] - in_zp * SUM_k w[c][k] and the device never
//      needs in_zp. (device/qemu_mps3_an547/analyse/compiler/
//      QEMUCompilerOpConv2D.c does exactly this on the CPU path.)
//
//   2. Three scales fold into one per-channel multiplier.
//      in_scale, w_scale[c] and out_scale are all known at compile time,
//      so only their product M[c] is emitted — as a fixed-point
//      {multiplier, shift} pair, which is what requantisation hardware
//      consumes.
//
// So the instruction carries ONE new pointer (requant_offset) plus the
// output zero point, and the device reads a NeonRequantEntry[channels_out]
// array from the weight buffer.
//
// WHY AN OFFSET AND NOT A TRAILING ARRAY: NeonCPUFallbackConv2DInstr does
// use a trailing float weight_scale[channels_out], which makes it
// variable-length. That is affordable there because CPU_FALLBACK never
// goes near the device queues. A variable-length CONV2D would break any
// driver that walks a concatenated instruction stream by opcode (it stops
// at an opcode whose size it cannot compute). Keeping CONV2D fixed-size
// preserves that walk.
// ============================================================

typedef struct __attribute__((packed)) {
    // Fixed-point form of M[c] = in_scale * w_scale[c] / out_scale.
    //
    //   value * M  ==  round( (value * multiplier) >> (31 + shift) )
    //
    // multiplier is a Q31 fixed-point number in [2^30, 2^31), i.e. the
    // mantissa of M normalised to [0.5, 1.0); shift is the remaining
    // power of two and may be negative (left shift). This is the same
    // representation TFLite calls quantized_multiplier / shift.
    //
    // NOTE: whether the FX1 OBP scale/shift banks (0x23000 / 0x24000)
    // actually expect this representation is UNCONFIRMED — see the
    // question list in docs/vp_open_questions.csv. The conversion lives in
    // exactly one place (quantizeMultiplier(), below) so a correction is a
    // one-function change on both the toolkit and the runtime side.
    int32_t multiplier;
    int32_t shift;
} NeonRequantEntry;

// --- Conv2D ---
typedef struct __attribute__((packed)) {
    NeonOpcode opcode;          // = NeonOpcode::CONV2D
    NeonDtype  dtype;
    uint16_t   stride_h;
    uint16_t   stride_w;
    uint16_t   pad_h;
    uint16_t   pad_w;
    uint16_t   dilation_h;
    uint16_t   dilation_w;
    uint32_t   input_h;
    uint32_t   input_w;
    uint32_t   channels_in;
    uint32_t   channels_out;
    uint32_t   kernel_h;
    uint32_t   kernel_w;
    uint64_t   input_offset;    // offset trong scratch buffer
    uint64_t   weight_offset;   // offset trong weight section
    uint64_t   bias_offset;     // 0 nếu không có bias
    uint64_t   output_offset;

    // --- Fused epilogue (Rev4) ---
    // Bias-add → requant → activation → optional residual-add, all folded
    // into this same instruction (no separate RELU/ADD op needed). Default
    // act_mode=0, has_skip=0 reproduces exactly pre-Rev4 behavior.
    uint8_t    act_mode;        // 0=None/bypass, 1=ReLU, 2=SiLU, 3=GELU
    uint8_t    has_skip;        // 0/1 — fused residual add
    uint64_t   skip_offset;     // valid chỉ khi has_skip=1

    // --- Per-channel requantisation (Rev5) ---
    // See the NeonRequantEntry block above for the full derivation.
    //
    // requant_offset: offset into NPUModel::weight_buffer of a
    //   NeonRequantEntry[channels_out] array. 0 = ABSENT, meaning the
    //   device falls back to whatever scalar scale it was configured with.
    //   Zero is a safe sentinel here for the same reason bias_offset uses
    //   it: offset 0 of the weight buffer holds weights, never a requant
    //   table, so no valid table can live there.
    //
    //   bias_offset must then point at bias_adj[] (bias with the input
    //   zero-point correction already folded in), NOT the raw bias.
    //
    // A Rev4 file has both fields zero and behaves exactly as before, so
    // old .nsm files keep working — this is an additive revision.
    uint64_t   requant_offset;
    int32_t    output_zero_point;
} NeonConv2DInstr;

#define NEON_CONV2D_INSTR_MIN_SIZE ((size_t)offsetof(NeonConv2DInstr, requant_offset))

// --- Native HW Config (Profile V1 SAURIA) ---
typedef struct __attribute__((packed)) {
    NeonOpcode opcode;          // = NeonOpcode::NATIVE_CONFIG
    uint8_t    profile;         // 0 = PROFILE_V1_SAURIA
    uint16_t   stride_h;
    uint16_t   stride_w;
    uint16_t   pad_h;
    uint16_t   pad_w;
    uint16_t   dilation_h;
    uint16_t   dilation_w;
    uint32_t   input_h;
    uint32_t   input_w;
    uint32_t   channels_in;
    uint32_t   channels_out;
    uint32_t   kernel_h;
    uint32_t   kernel_w;
    uint32_t   output_h;
    uint32_t   output_w;
    uint64_t   input_offset;
    uint64_t   weight_offset;
    uint64_t   bias_offset;
    uint64_t   output_offset;
    uint8_t    act_mode;        // 0=None, 1=ReLU, 2=SiLU
    int32_t    output_zero_point;
    uint8_t    act_layout;      // NEON_LAYOUT_NHWC | NEON_LAYOUT_NCHW (activation + output tensors)
} NeonNativeConfigInstr;

// --- Whole-layer convolution / FC on HasNpuTop (ISA 1.18) ---
// One instruction per layer. HasNpuTop reads activations as [C][H][W] int8.
//   io_layout NEON_HAS_IO_CHW     : NCHW conv; tensors are already [C][H][W].
//   io_layout NEON_HAS_IO_ROWMAJOR: FC as a 1x1 conv over M positions (in_c=K, in_h=1,
//                                   in_w=M, out_c=N, out_h=1, out_w=M); the fragment
//                                   tensors are [M][K] / [M][N], so the runtime transposes.
// The runtime stages every operand UNPADDED into its HasNpuTop DRAM window and passes
// pad_t/b/l/r + input_zero_point as the PAD_T/L/B/R + PAD_VALUE extension fields: the
// hardware inserts the border while it gathers the kernel windows (SW_INTEGRATION_GUIDE.md
// §3.3, §4.7; the biases already hold -zp_in*sum(W)). It programs the legacy + extension
// rich registers (§4.1) and copies the output back. All *_offset fields are offsets into
// NPUModel::weight_buffer.
#define NEON_HAS_IO_CHW      0u
#define NEON_HAS_IO_ROWMAJOR 1u
// Row-stacked stride-1 KhxKw conv (PLAN_has_tiling_search_v2.md §1.4): the instruction keeps the
// original layer; the runtime stages X'[(c,ky)][y*Wp+x] = Xpad[c][y+ky][x] and runs a 1 x kw conv
// over C' = in_c*kh channels and one row of out_h*Wp + kw - 1 positions. tile_cin counts C'.
#define NEON_HAS_IO_ROWSTACK 2u
typedef struct __attribute__((packed)) {
    NeonOpcode opcode;          // = NeonOpcode::HAS_CONV2D
    uint8_t    act_type;        // 0 = none, 2 = LUT at lut_offset (ReLU / SiLU baked by the compiler)
    uint8_t    flags;           // HasNpuTop FLAGS (bit0 PAD_TAIL); 0 today
    uint8_t    io_layout;       // NEON_HAS_IO_CHW | NEON_HAS_IO_ROWMAJOR | NEON_HAS_IO_ROWSTACK
    uint32_t   in_c, in_h, in_w;           // unpadded input
    uint32_t   out_c, out_h, out_w;
    uint16_t   kernel_h, kernel_w;
    uint16_t   stride;                     // one value for both axes (HasNpuTop limit)
    uint16_t   pad_t, pad_b, pad_l, pad_r; // HasNpuTop PAD_T/B/L/R (hardware padding, PAD_VALUE = input_zero_point)
    uint32_t   tile_cout, tile_h, tile_w;  // offline tile plan (TILING_GUIDE.md §3)
    uint32_t   tile_cin;                   // input channels per core pass; in_c % tile_cin == 0
    uint32_t   y_used;                     // array rows per context; 0 = hardware default
    int32_t    input_zero_point;           // border value inserted by the hardware (PAD_VALUE)
    int32_t    output_zero_point;          // ZP_OUT
    uint64_t   weight_offset;              // SAURIA order, Cin parts (SW_INTEGRATION_GUIDE.md §6.1)
    uint64_t   weight_bytes;               // = out_c * in_c * kernel_h * kernel_w
    uint64_t   bias_offset;                // int32[out_c] (input zero point folded)
    uint64_t   scale_offset;               // int32[out_c] requant multipliers
    uint64_t   shift_offset;               // uint32[out_c] requant right shifts
    uint64_t   lut_offset;                 // int8[256] indexed by q + 128; valid when act_type != 0
} NeonHasConv2DInstr;                      // 118 B

#ifdef __cplusplus
static_assert(sizeof(NeonHasConv2DInstr) == 118, "NeonHasConv2DInstr layout is ABI");
#endif

// --- Whole-tensor ELEM_WISE on HasNpuTop (ISA 1.20) ---
// One instruction per op. The HasNpuTop data flow controller runs it on GvuTop (SW_INTEGRATION_GUIDE.md
// §4.2 / §4.3). The runtime stages the operands into its HasNpuTop DRAM window and copies the result
// back, as for HAS_CONV2D. Tensors are dense int8.
//   ADD     : out[i] = clamp8(sat16(rshift(sat32(rshift((A[i]-zp_a)*sa, sha) + rshift((B[i]-zp_b)*sb, shb))
//                                          * so, sho)) + zp_o), rshift = half-up round-shift.
//             A = the op's input tensor; B = fragment operand 1 (b_src NEON_HAS_ELEM_B_OPERAND) or A
//             itself (NEON_HAS_ELEM_B_IS_A, requant-only, sb = 0). A, B and out have len elements (no
//             broadcast). SUB is an ADD with sb < 0.
//   MAX_POOL: [C][H][W] -> [C][out_h][out_w], square pool_k window, one stride, the same padding on all
//             sides (value -128), out = (in + 2 * pool_p - pool_k) / pool_s + 1.
#define NEON_HAS_ELEM_ADD       0u   // = HasNpuTop ELEM_WISE mode 0
#define NEON_HAS_ELEM_MAXPOOL   1u   // = HasNpuTop ELEM_WISE mode 1
#define NEON_HAS_ELEM_B_OPERAND 0u
#define NEON_HAS_ELEM_B_IS_A    1u
typedef struct __attribute__((packed)) {
    NeonOpcode opcode;          // = NeonOpcode::HAS_ELEMWISE
    uint8_t    mode;            // NEON_HAS_ELEM_ADD | NEON_HAS_ELEM_MAXPOOL
    uint8_t    b_src;           // ADD: NEON_HAS_ELEM_B_OPERAND | NEON_HAS_ELEM_B_IS_A
    uint8_t    _rsv0;
    uint32_t   len;                        // ADD: elements of A, B and the output
    int32_t    zp_a, zp_b, zp_o;           // ADD: ZP_A / ZP_B / ZP_O (int8 range)
    int32_t    sa, sb, so;                 // ADD: SA / SB / SO, signed 32-bit (SCALE_FMT I32)
    uint8_t    sha, shb, sho;              // ADD: SHA / SHB / SHO, 0..63
    uint8_t    _rsv1;
    uint32_t   in_c, in_h, in_w;           // MAX_POOL: input [C][H][W]
    uint32_t   out_h, out_w;               // MAX_POOL: checked against the formula above
    uint16_t   pool_k, pool_s, pool_p;     // MAX_POOL: POOL_K, STRIDE, POOL_P
    uint16_t   pool_mode;                  // MAX_POOL: POOL_MODE (0 direct window)
} NeonHasElemwiseInstr;                    // 64 B

#ifdef __cplusplus
static_assert(sizeof(NeonHasElemwiseInstr) == 64, "NeonHasElemwiseInstr layout is ABI");
#endif

// --- Depthwise Conv2D ---

typedef struct __attribute__((packed)) {
    NeonOpcode opcode;          // = NeonOpcode::DEPTHWISE_CONV
    NeonDtype  dtype;
    uint16_t   stride_h;
    uint16_t   stride_w;
    uint16_t   pad_h;
    uint16_t   pad_w;
    uint32_t   input_h;
    uint32_t   input_w;
    uint32_t   channels;
    uint32_t   kernel_h;
    uint32_t   kernel_w;
    uint32_t   depth_multiplier;
    uint64_t   input_offset;
    uint64_t   weight_offset;
    uint64_t   bias_offset;
    uint64_t   output_offset;
} NeonDepthwiseConvInstr;

// --- Activation (Relu, Relu6, Sigmoid, Tanh dùng chung struct) ---
typedef struct __attribute__((packed)) {
    NeonOpcode opcode;          // = RELU | RELU6 | SIGMOID | TANH
    NeonDtype  dtype;
    uint32_t   size;            // tổng số elements
    uint64_t   input_offset;
    uint64_t   output_offset;
} NeonActivationInstr;

// --- MaxPool / AvgPool ---
typedef struct __attribute__((packed)) {
    NeonOpcode opcode;          // = MAXPOOL | AVGPOOL
    NeonDtype  dtype;
    uint16_t   stride_h;
    uint16_t   stride_w;
    uint16_t   kernel_h;
    uint16_t   kernel_w;
    uint16_t   pad_h;
    uint16_t   pad_w;
    uint32_t   input_h;
    uint32_t   input_w;
    uint32_t   channels;
    uint64_t   input_offset;
    uint64_t   output_offset;
} NeonPoolInstr;

// --- Fully Connected ---
// V4.5 (32x32) native lowering: a GEMM [M,K] x [K,N] is emitted as
//   NATIVE_CONFIG (1x1 conv over a 1 x M x K image, channels_out = N)
// + OBP_CONFIG (lane_count = 32)
// + ceil(M/32) * ceil(N/32) FC instructions, one per (row tile, OC tile):
//     input_size    = K                                  (full reduction, never split)
//     output_size   = min(32, N - t*32)
//     weight_offset = kernel_base + t  * K_padded * 32
//     bias_offset   = bias_base   + t  * 32 * sizeof(int32_t)
//     input_offset  = mt * 32 * K                        // row tile, in bytes
//     output_offset = mt * 32 * N + t * 32               // element index of the tile corner
// The runtime recovers the OC tile from (bias_offset - NATIVE_CONFIG.bias_offset)
// and the row tile from (input_offset / input_size).
typedef struct __attribute__((packed)) {
    NeonOpcode opcode;          // = NeonOpcode::FC
    NeonDtype  dtype;
    uint32_t   input_size;      // K
    uint32_t   output_size;     // N
    uint64_t   input_offset;
    uint64_t   weight_offset;
    uint64_t   bias_offset;
    uint64_t   output_offset;

    // --- Fused epilogue (Rev4) — see NeonConv2DInstr's comment ---
    uint8_t    act_mode;        // 0=None/bypass, 1=ReLU, 2=SiLU, 3=GELU
    uint8_t    has_skip;        // 0/1 — fused residual add
    uint64_t   skip_offset;     // valid chỉ khi has_skip=1

    // --- Per-channel requantisation (Rev5) — see NeonConv2DInstr ---
    // For FC the "channel" is the output feature, so the array length is
    // output_size rather than channels_out.
    uint64_t   requant_offset;  // NeonRequantEntry[output_size]; 0 = absent
    int32_t    output_zero_point;
} NeonFCInstr;

// Rev4 FC size — see NEON_CONV2D_INSTR_MIN_SIZE for the rationale.
#define NEON_FC_INSTR_MIN_SIZE ((size_t)offsetof(NeonFCInstr, requant_offset))

// --- Softmax ---
typedef struct __attribute__((packed)) {
    NeonOpcode opcode;          // = NeonOpcode::SOFTMAX
    NeonDtype  dtype;
    uint32_t   size;
    float      beta;            // thường = 1.0
    uint64_t   input_offset;
    uint64_t   output_offset;
} NeonSoftmaxInstr;

// --- BatchNorm ---
typedef struct __attribute__((packed)) {
    NeonOpcode opcode;          // = NeonOpcode::BATCHNORM
    NeonDtype  dtype;
    uint32_t   channels;
    uint32_t   spatial_size;    // H * W
    float      epsilon;
    uint64_t   input_offset;
    uint64_t   scale_offset;    // gamma
    uint64_t   bias_offset;     // beta
    uint64_t   mean_offset;
    uint64_t   var_offset;
    uint64_t   output_offset;
} NeonBatchNormInstr;

// --- Element-wise Add / Mul ---
// Rev4 (ISA 1.4) appended param_offset/b_is_scalar for the first real consumer
// of this struct — the quantized INT8 ADD path. Fields are appended, never
// reordered, so pre-Rev4 readers (e.g. device/qemu_mps3_an547) keep
// reading the original six fields at their original offsets; a zeroed tail
// reproduces exactly the pre-Rev4 meaning.
typedef struct __attribute__((packed)) {
    NeonOpcode opcode;          // = NeonOpcode::ADD | MUL
    NeonDtype  dtype;
    uint32_t   size;            // element count of the output tensor
    uint64_t   input_a_offset;  // byte offset of input A from the input base
    uint64_t   input_b_offset;  // byte offset of input B from the same base;
                                // meaningful only when b_is_scalar == 0
    uint64_t   output_offset;
    // --- Rev4 ---
    uint64_t   param_offset;    // byte offset, inside the weight buffer, of the
                                // op's requant block:
                                //   ADD: NEON_ADD_FUSED_PARAMS_WORDS x int32
                                //        (AddFusedParamSerializer, see below)
                                //   SUB: the same fused block, m_b < 0
                                //   MUL: 6 x int32 (MulParamSerializer)
    uint8_t    b_is_scalar;     // 1 = input B is a single broadcast scalar
                                // 0 = input B is a full tensor of `size` elems
} NeonElemwiseInstr;

// Fused-requant ADD block at NeonElemwiseInstr::param_offset
// ("Full-flow Add (Optimized).md" §2.4), written by AddFusedParamSerializer:
//   [0] m_a  [1] m_b  [2] bias_lo  [3] bias_hi  [4] n  [5] z_c  [6] magic  [7] reserved (0)
//   q_c = clamp(((q_a * m_a + q_b * m_b + bias) >> n) + z_c),
//   bias = int64(bias_hi << 32 | bias_lo)
// A scalar constant B is folded into bias by the compiler (m_b == 0).
// SUB uses the same block with the B term negated (m_b < 0, §11).
// The runtime rejects a block whose word 6 is not the magic: the pre-fused
// 10-int32 layout has z_a there.
#define NEON_ADD_FUSED_PARAMS_WORDS 8
#define NEON_ADD_FUSED_PARAMS_MAGIC 0x32444441u   /* "ADD2" little-endian */

// --- DMA Transfer ---
typedef struct __attribute__((packed)) {
    NeonOpcode opcode;          // = NeonOpcode::DMA_READ | DMA_WRITE
    uint8_t    channel;         // DMA channel index (0..num_dma_channels-1)
    uint32_t   size_bytes;
    uint64_t   src_offset;
    uint64_t   dst_offset;
    uint32_t   tile_row;        // Im2Col: global spatial start index (ho*ow+wo) for channel-0 DMA_READ
                                // = 0 for channel 1 (weights) and channel 2 (write-back)
} NeonDMAInstr;

// --- NOP / HALT ---
typedef struct __attribute__((packed)) {
    NeonOpcode opcode;          // = NeonOpcode::NOP | HALT
} NeonControlInstr;

// --- SET_NSPLIT (Rev2 — dual-lane spatial split) ---
// Barrier instruction: decoder stalls both Queue_A and Queue_B until
// both SA lanes drain current tile, then updates N_split_reg.
// n_split = 0..64:
//   0  → Lane A idle, full SA to Lane B
//   64 → Lane B idle, full SA to Lane A (single-model mode)
//   N  → Lane A gets rows 0..N-1, Lane B gets rows N..63
// Compiler emits this once at session init; only re-emits when
// the optimal N changes between layer pairs.
typedef struct __attribute__((packed)) {
    NeonOpcode opcode;          // = NeonOpcode::SET_NSPLIT (0x05)
    uint8_t    n_split;         // 0..64
} NeonSetNSplitInstr;

// --- CPU_FALLBACK (staged — see NeonOpcode::CPU_FALLBACK's own comment) ---
// Deliberately minimal: `fallback_id` is an opaque lookup key into
// core/NPUUnsupportedOpFallback.h, resolved to an actual function pointer
// by runtime/opfallbacks/CPUFallbackRegistry.h — this struct itself stays
// plain data either way, same discipline as every other NeonXInstr.
//
// Unlike NeonConv2DInstr/NeonFCInstr/etc., this struct carries NO
// input_offset/output_offset fields — those would be SRAM-space addresses
// for the NPU/DMA hardware to use, meaningless here since CPU_FALLBACK
// bypasses NPUDriver entirely and runs directly against host memory.
// Executor::dispatchOp() resolves tensors the same way every op already
// does: via RuntimeOp.input_tensor_idx/output_tensor_idx (parsed from
// NeonOpEntry, universal across every opcode) + Executor::tensorPtr() —
// no new addressing scheme needed.
//
// Whoever implements the real emitter/consumer for this must also add a
// real NeonCPUFallbackInstr entry to the "Rev" table below and bump
// NEON_ISA_VERSION_MINOR — not done here, since nothing emits this from
// toolkit/ yet (Executor now has a real dispatch case — see
// runtime/executor/Executor.cpp's dispatchCPUFallback() — but that alone
// isn't a live end-to-end capability without a real emitter). Adding live
// opcodes without a real consumer is exactly what this project otherwise
// avoids (see core/NPU_ISA.h's own "never hardcode" discipline) — this
// one is staged only because its exact shape was already worked out in
// docs/hybrid_npu_cpu_fallback_design.md ahead of the vendor-NPU
// integration work that will actually emit it.
typedef struct __attribute__((packed)) {
    NeonOpcode opcode;          // = NeonOpcode::CPU_FALLBACK
    uint32_t   fallback_id;     // core/NPUUnsupportedOpFallback.h — NOT a real function pointer
} NeonCPUFallbackInstr;

// --- CPU_FALLBACK, Conv2D-shaped variant (first real fallback kind) ---
// Extends NeonCPUFallbackInstr's common header (opcode, fallback_id) with
// everything runtime/opfallbacks/Conv2DCPUFallback.cpp needs to call
// analyse/compiler/CompilerOpConv2D.h's conv2D_Func() directly — this is
// the "common header + per-kind variant" pattern: Executor reads just the
// header (any NeonCPUFallbackInstr*) to get fallback_id and look up the
// registered function; the function itself re-interprets op.instr_ptr as
// whichever larger struct its own fallback_id implies. instr_size (already
// generic, from NeonOpEntry.instr_offset deltas) is what makes a
// per-kind-variable-size instruction safe here, same as every other op.
//
// weight_offset/bias_offset are offsets into NPUModel::weight_buffer — the
// SAME shared weight buffer real NPU-dispatched CONV2D/FC instructions
// already use via their own weight_offset/bias_offset fields, not a new
// buffer. bias_offset may be 0 (no bias), same convention as
// NeonConv2DInstr.
//
// No dilation field — analyse/compiler/CompilerOpConv2D.h's Conv2DShape
// (what conv2D_Func() actually consumes) has no dilation concept; adding
// one here would silently be ignored, so it's left out rather than implied.
//
// input_scale/output_scale are single scalars, not per-channel — matches
// conv2D_Func()'s own real usage (qp->input_q.scales[0]/
// qp->output_q.scales[0], see device/qemu_mps3_an547/analyse/compiler/
// QEMUCompilerOpConv2D.c). No weight_zero_point field either — that same
// file's conv2D_Func() sums raw INT8 weights with no zero-point subtraction
// (s_wsum[c] += w[k]), i.e. it assumes symmetric (zero_point=0) per-channel
// weight quantization; this struct reflects that real behavior rather than
// implying a capability that isn't actually there.
//
// Per-channel weight scale (qp->weight_q.scales[c] in conv2D_Func()'s own
// terms) is NOT an embedded fixed field — its length depends on
// channels_out, not fixed at compile time. It's appended immediately after
// this struct in the same instruction blob: channels_out float32 values,
// read via (const float*)((const uint8_t*)instr + sizeof(*instr)). The
// instruction's total instr_size must equal
// sizeof(NeonCPUFallbackConv2DInstr) + channels_out * sizeof(float) —
// whoever emits this (toolkit, not built yet) must get this right; the
// consuming fallback function checks it defensively rather than trusting it.
typedef struct __attribute__((packed)) {
    NeonOpcode opcode;          // = NeonOpcode::CPU_FALLBACK
    uint32_t   fallback_id;     // = NEON_CPU_FALLBACK_CONV2D (core/NPUUnsupportedOpFallback.h)

    uint16_t   stride_h;
    uint16_t   stride_w;
    uint16_t   pad_h;
    uint16_t   pad_w;
    uint32_t   input_h;
    uint32_t   input_w;
    uint32_t   channels_in;
    uint32_t   channels_out;
    uint32_t   kernel_h;
    uint32_t   kernel_w;

    uint64_t   weight_offset;   // into NPUModel::weight_buffer
    uint64_t   bias_offset;     // into NPUModel::weight_buffer; 0 = no bias

    float      input_scale;
    int32_t    input_zero_point;
    float      output_scale;
    int32_t    output_zero_point;

    // followed by: float weight_scale[channels_out]  (per-output-channel)
} NeonCPUFallbackConv2DInstr;

// --- CPU_FALLBACK, Transpose variant ---
typedef struct __attribute__((packed)) {
    NeonOpcode opcode;          // = NeonOpcode::CPU_FALLBACK (0x70)
    uint32_t   fallback_id;     // = NEON_CPU_FALLBACK_TRANSPOSE (2)
    uint32_t   rank;            // 1..4
    uint32_t   in_dims[4];      // input tensor dimensions
    uint32_t   perm[4];         // permutation array
    uint32_t   elem_size;       // element size in bytes (1 for int8)
    uint64_t   src_offset;      // source address / offset; 0 = CDC_RAW_IN0_BASE
    uint64_t   dst_offset;      // dest address / offset; 0 = CDC_ISP_OUT0_BASE
} NeonCPUFallbackTransposeInstr;

// --- CPU_FALLBACK, Im2Col variant (Rich mode Conv2D >= 3x3) ---
typedef struct __attribute__((packed)) {
    NeonOpcode opcode;          // = NeonOpcode::CPU_FALLBACK (0x70)
    uint32_t   fallback_id;     // = NEON_CPU_FALLBACK_IM2COL (3)
    uint64_t   src_offset;      // offset into dram_input (raw activation, see layout)
    uint64_t   dst_offset;      // absolute address in scratch — [rows][K] written here
    uint32_t   ih, iw, ic;      // input activation spatial dims (NHWC, N=1)
    uint32_t   kh, kw;          // kernel dimensions
    uint32_t   stride_h, stride_w;
    uint32_t   dilation_h, dilation_w;
    uint32_t   pad_t, pad_l;    // top/left padding (from Im2ColConvToGemmPass)
    uint32_t   ho, wo;          // output spatial dims; rows = ho * wo
    int32_t    input_zero_point; // fill value for out-of-bounds padding
    uint8_t    layout;          // NEON_LAYOUT_NHWC: src [H][W][C], K=(kh,kw,ic)
                                // NEON_LAYOUT_NCHW: src [C][H][W], K=(ic,kh,kw)
    uint8_t    _rsv[3];
} NeonCPUFallbackIm2ColInstr;

// --- CPU_FALLBACK, Depthwise-Conv2D Im2Col variant ---
// Gathers ONE input channel's KH*KW spatial patches into a [M][K] row-major block
// (K = kh*kw), writing it into a channel-major [C][M][K] scratch buffer at dst_offset.
// Unlike NeonCPUFallbackIm2ColInstr, `channel_stride` (the real per-pixel channel count
// of the source tensor) is independent of the single channel being gathered — src_offset
// already points at that channel's first byte within each pixel.
typedef struct __attribute__((packed)) {
    NeonOpcode opcode;          // = NeonOpcode::CPU_FALLBACK (0x70)
    uint32_t   fallback_id;     // = NEON_CPU_FALLBACK_DW_IM2COL (9)
    uint64_t   src_offset;      // absolute address of THIS channel's first byte in the
                                 // NHWC input tensor (base + channel_index)
    uint64_t   dst_offset;      // absolute address of THIS channel's [M][K] scratch block
    uint32_t   ih, iw;          // input spatial dims
    uint32_t   channel_stride;  // real per-pixel channel count (C) of the source tensor
    uint32_t   kh, kw;
    uint32_t   stride_h, stride_w;
    uint32_t   dilation_h, dilation_w;
    uint32_t   pad_t, pad_l;
    uint32_t   ho, wo;          // output spatial dims; rows = ho * wo
    int32_t    input_zero_point;
} NeonCPUFallbackDWIm2ColInstr;

// --- CPU_FALLBACK, MaxPool2D variant ---
// Input and output share the SAME quantization (scale, zero_point) for
// MaxPool — it is monotonic, TFLite never rescales across it — so there is
// no weight/bias/multiplier/shift here at all, just a per-channel spatial
// max reduction. Padding sentinel is int8 -128 (matches the padding value
// TopToLinalg's MaxPoolLoweringToLinalg::Lowering itself uses,
// Pool.cpp:48), never the tensor's zero_point.
typedef struct __attribute__((packed)) {
    NeonOpcode opcode;          // = NeonOpcode::CPU_FALLBACK (0x70)
    uint32_t   fallback_id;     // = NEON_CPU_FALLBACK_MAXPOOL (10)
    uint16_t   stride_h;
    uint16_t   stride_w;
    uint16_t   pad_t;           // top padding
    uint16_t   pad_l;           // left padding
    uint32_t   input_h;
    uint32_t   input_w;
    uint32_t   channels;
    uint32_t   kernel_h;
    uint32_t   kernel_w;
    uint32_t   output_h;        // passed explicitly, not recomputed from a
    uint32_t   output_w;        // symmetric-padding formula — correct even
                                 // for asymmetric SAME padding
    uint64_t   src_offset;      // source address / offset; 0 = CDC_RAW_IN0_BASE
    uint64_t   dst_offset;      // dest address / offset; 0 = CDC_ISP_OUT0_BASE
    uint8_t    layout;          // NEON_LAYOUT_NHWC: input/output [H][W][C]; NEON_LAYOUT_NCHW: [C][H][W]
    uint8_t    _rsv[3];
} NeonCPUFallbackMaxPoolInstr;

// --- CPU_FALLBACK, Softmax variant ---
// axis_size must be <= 256 (bounds the runtime's per-row exp scratch array
// and matches the LUT table sizes) — the emitter must reject anything
// larger rather than let this silently misbehave.
typedef struct __attribute__((packed)) {
    NeonOpcode opcode;          // = NeonOpcode::CPU_FALLBACK (0x70)
    uint32_t   fallback_id;     // = NEON_CPU_FALLBACK_SOFTMAX (11)
    uint32_t   outer_size;      // product of dims before `axis`
    uint32_t   axis_size;       // size of the reduction axis (<= 256)
    uint32_t   inner_size;      // product of dims after `axis` (1 if axis is the last dim)
    int32_t    output_zero_point;
    int32_t    m0_out;          // Q31 multiplier for the final rescale
    int32_t    total_shift_out; // = 31 - shift_out, precomputed at compile time
    uint64_t   exp_lut_offset;  // -> 256 x uint8 (UQ8, 0..255) in the weight buffer
    uint64_t   recip_lut_offset;// -> 256 x uint16 (Q15) in the weight buffer
    uint64_t   src_offset;      // source address / offset; 0 = CDC_RAW_IN0_BASE
    uint64_t   dst_offset;      // dest address / offset; 0 = CDC_ISP_OUT0_BASE
} NeonCPUFallbackSoftmaxInstr;

// --- CPU_FALLBACK, LayerNorm variant ---
// Row-wise: mean/var over `axis_size`, quantize var through the ONNX Q/DQ
// chain, PWL 1/sqrt via a 512-entry Q15 LUT, then gamma-mul and bias-add.
// Two requant shifts (div, mul) are total shifts (31 - frexp_exp), applied as
// round_shift(x * m0, total_shift); the div stage additionally adds the
// runtime E_half. The bias stage has no shift — it is a plain int32 add.
typedef struct __attribute__((packed)) {
    NeonOpcode opcode;            // = NeonOpcode::CPU_FALLBACK (0x70)
    uint32_t   fallback_id;       // = NEON_CPU_FALLBACK_LAYERNORM (14)
    uint32_t   outer_size;        // rows = product of dims before `axis`
    uint32_t   axis_size;         // H, the normalized dim (<= 1024)

    // Stage 1 — variance quantization chain (ONNX Q/DQ, reproduced exactly)
    double     s_in;              // input scale
    double     s_var;             // ReduceMean_1 output scale
    int32_t    z_var;
    double     s_7;               // Add (var+eps) output scale
    int32_t    z_7;
    double     eps;               // dequantized epsilon constant (1e-5 here)

    // Stage 2 — normalize (Div).  total_shift_div is the COMPILE-TIME part;
    // the runtime adds E_half from the PWL exponent split.
    int32_t    m0_div;
    int32_t    total_shift_div;
    int32_t    z_div;

    // Stage 3 — gamma multiply
    int32_t    m0_mul;
    int32_t    total_shift_mul;
    int32_t    z_mul;

    // Stage 4 — bias add.  beta is NOT shipped as int8 codes: the compiler
    // has already applied R_bias = s_b/s_out and emitted a ready-to-add
    // int32 table at bias_offset.  m0_bias / total_shift_bias / s_b do not
    // exist on this side of the ABI.  See "Full-flow LayerNorm.md" §4.2.5.
    int32_t    output_zero_point;

    uint64_t   rsqrt_lut_offset;  // -> 512 x uint16 (Q15) in the weight buffer
    uint64_t   gamma_offset;      // -> axis_size x int8
    uint64_t   bias_offset;       // -> axis_size x int32, 16-byte aligned.
                                  //    bias[j] = roundShift(beta[j]*m0_bias, total_shift_bias)
    uint64_t   src_offset;        // 0 = CDC_RAW_IN0_BASE
    uint64_t   dst_offset;        // 0 = CDC_ISP_OUT0_BASE
} NeonCPUFallbackLayerNormInstr;

// --- CPU_FALLBACK, constant-materialization variant ---
// A pure weight_buffer -> output memcpy. No quantization math: the constant
// was already folded in the frontend at the op's own output scale/zero-point,
// so the bytes are copied verbatim.
typedef struct __attribute__((packed)) {
    NeonOpcode opcode;          // = NeonOpcode::CPU_FALLBACK (0x70)
    uint32_t   fallback_id;     // = NEON_CPU_FALLBACK_CONST_COPY (12)
    uint64_t   const_offset;    // offset into NPUModel::weight_buffer
    uint64_t   dst_offset;      // dest address / offset; 0 = CDC_ISP_OUT0_BASE
    uint32_t   size_bytes;      // number of bytes to copy
} NeonCPUFallbackConstCopyInstr;


// --- CPU_FALLBACK, LUT variant (Rich mode post-GEMM activation+requant) ---
typedef struct __attribute__((packed)) {
    NeonOpcode opcode;          // = NeonOpcode::CPU_FALLBACK (0x70)
    uint32_t   fallback_id;     // = NEON_CPU_FALLBACK_LUT (4)
    uint64_t   src_offset;      // source address / offset (e.g. CDC_ISP_OUT0_BASE)
    uint64_t   dst_offset;      // dest address / offset (in-place if == src_offset)
    uint64_t   table_offset;    // offset into weight_buffer (256 bytes baked LUT)
    uint32_t   len;             // total element count = M * OC
} NeonCPUFallbackLutInstr;

// --- CPU_FALLBACK, Depthwise Conv2D variant ---
typedef struct __attribute__((packed)) {
    NeonOpcode opcode;          // = NeonOpcode::CPU_FALLBACK (0x70)
    uint32_t   fallback_id;     // = NEON_CPU_FALLBACK_DWCONV (5)
    uint16_t   stride_h;
    uint16_t   stride_w;
    uint16_t   pad_h;
    uint16_t   pad_w;
    uint16_t   dilation_h;
    uint16_t   dilation_w;
    uint32_t   input_h;
    uint32_t   input_w;
    uint32_t   channels;        // in_c == out_c for depthwise
    uint32_t   kernel_h;
    uint32_t   kernel_w;

    uint64_t   src_offset;      // source address / offset (e.g. CDC_RAW_IN0_BASE)
    uint64_t   dst_offset;      // dest address / offset (e.g. CDC_ISP_OUT0_BASE)
    uint64_t   weight_offset;   // into NPUModel::weight_buffer
    uint64_t   bias_offset;     // into NPUModel::weight_buffer; 0 = no bias

    float      input_scale;
    int32_t    input_zero_point;
    float      output_scale;
    int32_t    output_zero_point;

    // followed by: float weight_scale[channels]  (per-channel)
} NeonCPUFallbackDWConvInstr;

// --- CPU_FALLBACK, Slice variant ---
typedef struct __attribute__((packed)) {
    NeonOpcode opcode;          // = NeonOpcode::CPU_FALLBACK (0x70)
    uint32_t   fallback_id;     // = NEON_CPU_FALLBACK_SLICE (6)
    uint64_t   src_offset;      // source address / offset; 0 = CDC_RAW_IN0_BASE
    uint64_t   dst_offset;      // dest address / offset; 0 = CDC_ISP_OUT0_BASE
    uint32_t   in_n, in_h, in_w, in_c;
    uint32_t   begin_n, begin_h, begin_w, begin_c;
    uint32_t   size_n, size_h, size_w, size_c;
} NeonCPUFallbackSliceInstr;

// --- CPU_FALLBACK, StridedSlice variant ---
// out[n,h,w,c] = in[begin[0]+n*stride[0], begin[1]+h*stride[1],
//                   begin[2]+w*stride[2], begin[3]+c*stride[3]]
// Both tensors dense row-major. Rank < 4 is left-padded with in/out dim 1,
// begin 0, stride 1. Every index touched lies in [0, in_dims[d]) (the
// compiler guarantees it; the runtime re-checks).
typedef struct __attribute__((packed)) {
    NeonOpcode opcode;          // = NeonOpcode::CPU_FALLBACK (0x70)
    uint32_t   fallback_id;     // = NEON_CPU_FALLBACK_STRIDED_SLICE (17)
    uint64_t   src_offset;      // 0 = this op's input tensor (OperandBinder)
    uint64_t   dst_offset;      // 0 = this op's output tensor (OperandBinder)
    uint32_t   rank;            // original rank 1..4 (informational)
    uint32_t   elem_size;       // bytes per element (1 for int8)
    uint32_t   in_dims[4];
    uint32_t   out_dims[4];
    int32_t    begin[4];        // normalized start index per axis
    int32_t    stride[4];       // non-zero, may be negative
} NeonCPUFallbackStridedSliceInstr;
#ifdef __cplusplus
static_assert(sizeof(NeonCPUFallbackStridedSliceInstr) == 93,
              "NeonCPUFallbackStridedSliceInstr layout is ABI");
#endif

// --- CPU_FALLBACK, Pad variant ---
// Zero/const-fills the FULL destination tensor with pad_value, then copies
// the source tensor into the destination at the given per-dimension offset.
// NHWC-style up to 4 dims; unused leading dims left at n=1/offset=0.
typedef struct __attribute__((packed)) {
    NeonOpcode opcode;          // = NeonOpcode::CPU_FALLBACK (0x70)
    uint32_t   fallback_id;     // = NEON_CPU_FALLBACK_PAD (13)
    uint64_t   src_offset;      // source address / offset; 0 = CDC_RAW_IN0_BASE
    uint64_t   dst_offset;      // dest address / offset; 0 = CDC_ISP_OUT0_BASE
    int8_t     pad_value;       // fill value for the padding border (input zero point)
    uint8_t    _pad[3];
    uint32_t   out_n, out_h, out_w, out_c;   // full destination dims
    uint32_t   in_n,  in_h,  in_w,  in_c;    // source dims (== out dims minus padding)
    uint32_t   off_n, off_h, off_w, off_c;   // where the source lands inside the dest
} NeonCPUFallbackPadInstr;

// --- CPU_FALLBACK, Concat variant ---
typedef struct __attribute__((packed)) {
    uint64_t src_offset;
    uint32_t channels;
    uint32_t _pad;
} NeonCPUFallbackConcatInput;

typedef struct __attribute__((packed)) {
    NeonOpcode opcode;          // = NeonOpcode::CPU_FALLBACK (0x70)
    uint32_t   fallback_id;     // = NEON_CPU_FALLBACK_CONCAT (7)
    uint64_t   dst_offset;      // dest address / offset; 0 = CDC_ISP_OUT0_BASE
    uint32_t   out_n, out_h, out_w, out_c;
    uint32_t   axis;            // concatenation axis (e.g. 3 for channel axis in NHWC)
    uint32_t   num_inputs;
    // followed in memory by: NeonCPUFallbackConcatInput inputs[num_inputs]
} NeonCPUFallbackConcatInstr;

// --- CPU_FALLBACK, Resize Nearest Neighbor variant ---
typedef struct __attribute__((packed)) {
    NeonOpcode opcode;          // = NeonOpcode::CPU_FALLBACK (0x70)
    uint32_t   fallback_id;     // = NEON_CPU_FALLBACK_RESIZE_NN (8)
    uint64_t   src_offset;      // source address / offset; 0 = CDC_RAW_IN0_BASE
    uint64_t   dst_offset;      // dest address / offset; 0 = CDC_ISP_OUT0_BASE
    uint32_t   in_h, in_w, in_c;
    uint32_t   out_h, out_w;
    uint8_t    align_corners;
    uint8_t    half_pixel_centers;
    uint8_t    layout;          // NEON_LAYOUT_NHWC | NEON_LAYOUT_NCHW
    uint8_t    _pad;
} NeonCPUFallbackResizeInstr;

// --- CPU_FALLBACK, Embedding (Gather rows of a const int8 table + optional folded const bias + requant) ---
typedef struct __attribute__((packed)) {
    NeonOpcode opcode;          // = NeonOpcode::CPU_FALLBACK (0x70)
    uint32_t   fallback_id;     // = NEON_CPU_FALLBACK_EMBEDDING (15)
    uint64_t   src_offset;      // indices address; 0 = CDC_RAW_IN0_BASE (patched by firmware)
    uint64_t   dst_offset;      // dest address;    0 = CDC_ISP_OUT0_BASE (patched by firmware)
    uint64_t   table_offset;    // weight_buffer offset: int8 [vocab_size][embed_dim]
    uint64_t   bias_offset;     // weight_buffer offset: float32 [num_indices][embed_dim] (valid iff has_bias)
    uint32_t   num_indices;     // B*T (77)
    uint32_t   vocab_size;      // 49408
    uint32_t   embed_dim;       // 512
    uint8_t    index_bytes;     // 4 (int32) or 8 (int64) little-endian signed indices
    uint8_t    has_bias;        // 1 = add bias[l][c] before requant
    uint8_t    _pad[2];
    float      table_scale;
    int32_t    table_zero_point;   // int8-rebased
    float      output_scale;
    int32_t    output_zero_point;  // int8-rebased
} NeonCPUFallbackEmbeddingInstr;

// --- CPU_FALLBACK, compiled kernel (ISA 1.19) ---
// Runs a whole fragment compiled to native CPU code by a code-generating
// backend (device/OpenC906/toolkit). The code lives in an object file linked
// into the runner; its entry point registers itself at program start as
//     int64_t neon_k_<kernel_id>(void *const *inputs, void *const *outputs)
// (runtime/include/neon/compiled_kernel_abi.h). This instruction only names it.
//
// Tensors: NeonOpEntry.input_tensor_idx / output_tensor_idx are inputs[0] /
// outputs[0]; num_inputs - 1 further input indices and then num_outputs - 1
// further output indices follow this struct as uint32_t, so
//     instr_size == sizeof(NeonCPUFallbackKernelInstr) + 4 * (num_inputs + num_outputs - 2).
// kernels_abi: version of the CPU micro-kernel library the code calls
// (C906_KERNELS_ABI of device/OpenC906/kernels/c906_kernels.h); 0 = the code
// calls no library kernel. The runtime checks it against the linked library.
typedef struct __attribute__((packed)) {
    NeonOpcode opcode;          // = NeonOpcode::CPU_FALLBACK (0x70)
    uint32_t   fallback_id;     // = NEON_CPU_FALLBACK_COMPILED_KERNEL (16)
    uint32_t   kernel_id;       // FNV-1a 32 of "<model>/<entry>" (registration key)
    uint16_t   num_inputs;      // >= 1
    uint16_t   num_outputs;     // >= 1
    uint32_t   kernels_abi;     // micro-kernel library version, see above
} NeonCPUFallbackKernelInstr;   // 17 B (+ trailing uint32_t tensor indices)



// --- Reduction Sum (GEMM bank 64x64) ---
typedef struct __attribute__((packed)) {
    NeonOpcode opcode;          // = NeonOpcode::SUM (0x53)
    NeonDtype  dtype;           // INT8
    uint32_t   k;           // H*W (reduction dimension)
    uint32_t   n;           // C (preserved channels dimension)
    uint64_t   param_offset;// offset into weight_buffer: {bias, m0, shift, z_out}
    int32_t    output_zero_point;
} NeonSumInstr;

// ============================================================
// SAURIA V4.2 NATIVE RICH INSTRUCTION (Rev5)
// Compact alias for native rich instruction interface
// (0x10210300..0x10210460). Covers GEMM_FUSED (0x12), FUSED_ATTN (0x13),
// LAYERNORM (0x14), ELEM_WISE (0x15).
// ============================================================

typedef struct __attribute__((packed)) {
    NeonOpcode opcode;        // 0x12 GEMM_FUSED | 0x13 FUSED_ATTN
                              // 0x14 LAYERNORM  | 0x15 ELEM_WISE
    uint8_t  act_type;        // 0 None, 1 ReLU, 2 SiLU, 3 GELU
    uint8_t  elem_mode;       // 0 Add, 1 MaxPool, 2 Mul, 3 Sub, 4 Div
                              // (v4.4_model_25Aug instruction_decoder.h:61)
    uint8_t  has_skip;
    uint8_t  lane;            // 0 = queue A, 1 = queue B
    uint8_t  in_addr_space;   // 0 = relative to dram_input (default), 1 = absolute DRAM address
    uint64_t in_addr, w_addr, out_addr, bias_addr, skip_addr;
    uint64_t q_gamma_a_addr, k_b_addr, v_beta_addr;
    uint32_t m, k, n, kh, kw, stride, pad;
    uint32_t seq_len, num_heads, dim, head_dim, len;
    uint32_t in_scale, w_scale, out_scale;   // IEEE-754 binary32 bit pattern
    uint32_t scale_a, scale_b, scale_out;
    uint32_t attn_scale, eps_shift;
    uint32_t a_len;           // ELEM_WISE: valid element count of source A
    uint32_t b_len;           // ELEM_WISE: valid element count of source B.
                              // The 25Aug model broadcasts each source with
                              // period a_len/b_len; leaving these zero makes the
                              // bridge fall back to seq_len (and, worse, to the
                              // packed `dim` field) — see this plan's TASK 3.
} NeonRichInstr;

// ============================================================
// OBP HARDWARE CONFIGURATION (Lane A)
// Configures bias, requantization scale/shift, and LUT table for the
// Output Buffer Processor. V4.5 (32x32) compact aliases:
//   cfg   0x10200820 | LUT 0x10220000..0x10221FFF (32 x 256 B)
//   bias  0x10224000..0x1022407F  (32 x int32)
//   scale 0x10225000..0x1022507F  | shift 0x10226000..0x1022607F
// ============================================================

typedef struct __attribute__((packed)) {
    NeonOpcode opcode;        // = NeonOpcode::OBP_CONFIG (0x06)
    uint8_t    lane_count;    // = 32 on V4.5 (was 64 on V4.4)
    uint8_t    cfg;           // bit0 bias, bit1 requant, bit2 lut, bit3 residual
    uint8_t    reserved;
    uint64_t   bias_offset;   // offset in weight buffer -> lane_count x int32
    uint64_t   scale_offset;  // -> lane_count x uint32
    uint64_t   shift_offset;  // -> lane_count x uint32
    uint64_t   lut_offset;    // -> lane_count x 256 byte, 0 if unused
} NeonOBPConfigInstr;

// ============================================================
// RUNTIME LOOP CONTRACT (Rev6 — ISA 1.11)
// Compresses unrolled tile loops into affine step descriptors.
// ============================================================

#define NEON_LOOP_MAX_LEVELS 4

// kind
#define NEON_LOOP_PATCH_AFFINE 0   // value = base + Σ iv[i]*step[i]
#define NEON_LOOP_PATCH_TABLE  1   // value = table[ iv[table_level] ]

typedef struct __attribute__((packed)) {
    uint8_t  body_op;        // which body op (0 .. body_op_count-1)
    uint8_t  kind;           // NEON_LOOP_PATCH_AFFINE or NEON_LOOP_PATCH_TABLE
    uint8_t  field_width;    // 4 or 8 bytes
    uint8_t  table_level;    // loop level indexing the table (kind==TABLE only)
    uint16_t field_offset;   // byte offset within that op's instruction
    uint16_t _pad;
    int32_t  step[NEON_LOOP_MAX_LEVELS];
    uint64_t table_offset;   // into weight_buffer, kind==TABLE only
} NeonLoopPatch;                                   // 32 B

typedef struct __attribute__((packed)) {
    NeonOpcode opcode;       // = LOOP_BEGIN (0x80)
    uint8_t    num_levels;   // 1..NEON_LOOP_MAX_LEVELS, outermost first
    uint8_t    body_op_count;
    uint8_t    num_patches;
    uint32_t   trip_count[NEON_LOOP_MAX_LEVELS];
    // followed by NeonLoopPatch[num_patches]
} NeonLoopBeginInstr;                              // 20 B + 32*num_patches

typedef struct __attribute__((packed)) {
    NeonOpcode opcode;       // = LOOP_END (0x81)
    uint8_t    _pad[3];
} NeonLoopEndInstr;                                // 4 B

// ============================================================
// QUANTIZATION PARAMS
// One entry per tensor — stored in .nsm_quant section.
// ============================================================

typedef struct __attribute__((packed)) {
    float   scale;
    int32_t zero_point;
    uint8_t bits;               // 4 hoặc 8
} NeonQuantParams;

// ============================================================
// .NPU BINARY FILE FORMAT
//
// Single-model layout (dual_queue_mode = 0):
//   [NeonFileHeader]
//   [NeonOpEntry * num_ops]         ← dispatch table
//   [instruction bytes]             ← .queue_a (single queue)
//   [weight bytes]                  ← .nsm_weights section
//   [NeonQuantParams * N]           ← .nsm_quant section
//   [NeonMemAlloc * N]              ← .nsm_memplan section
//
// Dual-model layout (dual_queue_mode = 1, Rev2 FX1 only):
//   [NeonFileHeader]
//   [NeonOpEntry * num_ops_a]       ← dispatch table Lane A
//   [NeonOpEntry * num_ops_b]       ← dispatch table Lane B
//   [queue_a instruction bytes]     ← SET_NSPLIT + GEMM_FUSED_A ...
//   [queue_b instruction bytes]     ← SET_NSPLIT + GEMM_FUSED_B ...
//   [weight_a bytes]                ← Lane A weights (Bank 0)
//   [weight_b bytes]                ← Lane B weights (Bank 1)
//   [NeonQuantParams * N]           ← shared quant params
//   [NeonMemAlloc * N]              ← shared memory plan
//   [layer_shapes bytes]            ← OpShape[] for N_split compute at load
// ============================================================

#define NEON_MAGIC "NEONNSM\0"

typedef struct __attribute__((packed)) {
    char       magic[8];            // = NEON_MAGIC
    uint16_t   isa_version_major;   // = NEON_ISA_VERSION_MAJOR
    uint16_t   isa_version_minor;   // = NEON_ISA_VERSION_MINOR
    char       target_device[32];   // e.g. "VP_FX1_V2.0"
    NeonDtype  default_dtype;

    // Dual-queue flag (Rev2)
    // 0 = single queue  → queue_b_* fields are 0, used on any hardware
    // 1 = dual queue    → hardware must have max_concurrent_models >= 2
    uint8_t    dual_queue_mode;
    uint8_t    n_split_default;     // compiler pre-computed optimal N_split
                                    // (0 when dual_queue_mode=0)
    uint8_t    _pad[2];

    // Op counts
    uint32_t   num_ops;             // Lane A ops (or all ops in single mode)
    uint32_t   num_ops_b;           // Lane B ops (0 in single mode)
    uint32_t   num_tensors;
    uint32_t   num_quant_params;

    // Section offsets từ đầu file
    uint64_t   dispatch_table_offset;   // Lane A dispatch table
    uint64_t   dispatch_table_b_offset; // Lane B dispatch table (0 in single mode)

    // Queue A = single queue in single mode
    uint64_t   queue_a_offset;
    uint64_t   queue_a_size;
    // Queue B = 0 in single mode
    uint64_t   queue_b_offset;
    uint64_t   queue_b_size;

    // Weights — one section per lane in dual mode
    uint64_t   weights_a_offset;
    uint64_t   weights_a_size;
    uint64_t   weights_b_offset;    // = weights_a_offset in single mode
    uint64_t   weights_b_size;      // = 0 in single mode

    uint64_t   quant_offset;
    uint64_t   memplan_offset;

    // Layer shapes for runtime N_split computation
    // (runtime can brute-force 1..63 to find optimal N_split per layer pair)
    uint64_t   layer_shapes_offset;
    uint32_t   num_layer_shapes;

    // I/O tensor indices
    uint32_t   input_tensor_idx;
    uint32_t   output_tensor_idx;

    // Memory requirement (for batch=1 / base profile)
    uint64_t   scratch_buffer_size;

    // Rev3: batch profiles
    // num_batch_profiles = 0 → model compiled without batch profiles (batch=1 only)
    // Profiles are sorted by batch_size ascending in the file.
    uint64_t   batch_profiles_offset;  // offset to NeonBatchProfileEntry[]
    uint32_t   num_batch_profiles;
    uint32_t   _pad3;                  // reserved, must be 0
} NeonFileHeader;

// Một entry trong dispatch table
typedef struct __attribute__((packed)) {
    NeonOpcode opcode;
    uint64_t   instr_offset;        // offset trong instructions section
    uint32_t   input_tensor_idx;
    uint32_t   output_tensor_idx;
} NeonOpEntry;

// ============================================================
// BATCH PROFILE ENTRY  (Rev3)
// On-disk index for one pre-compiled batch profile.
// Lives in .nsm_batch_profiles section.
// Weight section is shared across all profiles — weights are
// loaded once at model-load time regardless of batch size.
//
// File layout with batch profiles:
//   [NeonFileHeader]
//   [NeonOpEntry * num_ops]                      ← batch=1 backward-compat
//   [NeonBatchProfileEntry * num_batch_profiles] ← profile index
//   [per-profile dispatch tables]
//   [per-profile instruction streams]            ← one stream per batch size
//   [shared weight bytes]
//   [shared NeonQuantParams]
//   [per-profile NeonMemAlloc entries]
//   [layer_shapes for N_split]
// ============================================================

typedef struct __attribute__((packed)) {
    uint32_t   batch_size;              // 1, 2, 4, 8, 16, 32 …

    // Per-profile instruction stream
    uint64_t   instr_offset;            // offset in file
    uint64_t   instr_size;

    // Per-profile dispatch table (ops may differ in tensor shape)
    uint64_t   dispatch_table_offset;
    uint32_t   num_ops;

    // Per-profile memory plan (scratch offsets scale with batch)
    uint64_t   memplan_offset;
    uint32_t   num_memplan_entries;

    // I/O tensor indices (same across profiles; shape.dims[0] = batch_size)
    uint32_t   input_tensor_idx;
    uint32_t   output_tensor_idx;

    // Scratch memory required for this batch size
    uint64_t   scratch_buffer_size;

    uint32_t   _pad;                    // reserved, must be 0
} NeonBatchProfileEntry;

// ============================================================
// LAYER SHAPE — on-disk format for layer_shapes_offset section
//
// Compiler writes NeonLayerShape[] at layer_shapes_offset.
// Runtime (ModelLoader::loadShapes) reads them and converts to
// OpShape (analyse/StaticAnalyzer.h) for static analysis.
// This is the ONLY stable on-disk format — OpShape is runtime-only.
//
// All fields mirror OpShape exactly so conversion is a field copy.
// ============================================================

typedef struct __attribute__((packed)) {
    uint32_t   op_index;
    uint8_t    opcode;           // NeonOpcode value (uint8_t on-disk)
    uint8_t    dtype;            // NeonDtype value  (uint8_t on-disk)
    uint8_t    _pad[2];

    // Input tensor
    uint32_t   input_n;
    uint32_t   input_h;
    uint32_t   input_w;
    uint32_t   input_c;

    // Kernel / weight shape (conv only; 0 for other ops)
    uint32_t   kernel_h;
    uint32_t   kernel_w;
    uint32_t   channels_out;
    uint32_t   depth_multiplier;

    // Stride / padding
    uint32_t   stride_h;
    uint32_t   stride_w;
    uint32_t   pad_h;
    uint32_t   pad_w;

    // FC
    uint32_t   input_size;
    uint32_t   output_size;

    // Elementwise / activation
    uint32_t   element_count;
} NeonLayerShape;

// Memory plan — npu_driver dùng để assign tensor vào scratch buffer
typedef struct __attribute__((packed)) {
    uint32_t   tensor_idx;
    uint64_t   offset;              // offset trong scratch buffer
    uint64_t   size_bytes;
    uint32_t   first_use_op;        // lifetime: [first_use_op, last_use_op]
    uint32_t   last_use_op;
} NeonMemAlloc;

// ============================================================
// GRAPH CONTAINER (.nsm whose first 8 bytes are NEON_GRAPH_MAGIC)
//
// A model that the compiler emits as N independently compiled fragments
// (fx1_tools.pipeline.fragment_compiler) is shipped as ONE file:
//
//   [NeonGraphHeader]
//   [NeonGraphTensor   * num_tensors]
//   [NeonGraphFragment * num_fragments]
//   [uint32_t inputs[num_inputs]] [uint32_t outputs[num_outputs]]   <- graph tensor ids
//   [pad to NEON_GRAPH_ALIGN]
//   [fragment .nsm images, each NEON_GRAPH_ALIGN-aligned, embedded byte-for-byte]
//   [const tensor data, each NEON_GRAPH_ALIGN-aligned]                  <- format v2
//
// The file carries NO addresses: tensor sizes, lifetimes and constant bytes only; the runtime
// plans its own arena at load time (runtime/graph/ArenaPlanner) inside memory
// the platform hands it. A plain single-program .nsm (NEON_MAGIC) is still
// valid input everywhere — the runtime wraps it as a 1-fragment graph.
//
// Operand k of a fragment is the fragment's func argument k, which is also
// fragment-local tensor index k (AccelToNSMPass numbers func args first).
// ============================================================

#define NEON_GRAPH_MAGIC           "NEONGRF\0"
#define NEON_GRAPH_FORMAT_VERSION  3u          // v3: NEON_GRAPH_TENSOR_ALIAS; v2: NeonGraphTensor.data_offset + CONST
#define NEON_GRAPH_FORMAT_VERSION_MIN 2u       // oldest container the runtime still loads
#define NEON_GRAPH_MAX_FRAG_INPUTS 8u           // yolov8m max measured = 6
#define NEON_GRAPH_NO_TENSOR       0xFFFFFFFFu  // operand the graph does not bind (legacy: unresolved constant)
#define NEON_GRAPH_ALIGN           16u

#define NEON_GRAPH_TENSOR_INTERNAL 0u   // produced and consumed inside the graph -> runtime arena
#define NEON_GRAPH_TENSOR_INPUT    1u   // bound to a caller input buffer
#define NEON_GRAPH_TENSOR_OUTPUT   2u   // bound to a caller output buffer; may also be read by later fragments
#define NEON_GRAPH_TENSOR_CONST    3u   // read-only bytes embedded in the container (data_offset)
#define NEON_GRAPH_TENSOR_ALIAS    4u   // v3: bytes [data_offset, data_offset + size_bytes) of tensor alias_of
                                        // (never itself an ALIAS): a slice / concat piece linked as a view

typedef struct __attribute__((packed)) {
    char     magic[8];              // = NEON_GRAPH_MAGIC
    uint32_t format_version;        // = NEON_GRAPH_FORMAT_VERSION
    uint16_t isa_version_major;     // ISA the embedded fragments were compiled against
    uint16_t isa_version_minor;
    char     target_device[32];
    uint32_t num_tensors;
    uint32_t num_fragments;         // execution order == array order
    uint32_t num_inputs;
    uint32_t num_outputs;
    uint64_t tensors_offset;        // from start of file
    uint64_t fragments_offset;
    uint64_t io_offset;             // inputs[] then outputs[] (uint32_t graph tensor ids)
    uint64_t file_size;             // total bytes; loader rejects a shorter buffer
} NeonGraphHeader;                  // 96 B

typedef struct __attribute__((packed)) {
    uint64_t size_bytes;
    uint32_t producer;              // fragment index; NEON_GRAPH_NO_TENSOR for INPUT and CONST.
                                    // INTERNAL parent of views: first fragment that writes any of its bytes.
                                    // ALIAS: first fragment that writes its bytes (NO_TENSOR when the
                                    // parent is INPUT or CONST)
    uint32_t last_consumer;         // highest fragment index that reads it (for a parent: or any of its
                                    // views); num_fragments for graph outputs
    uint8_t  kind;                  // NEON_GRAPH_TENSOR_*
    uint8_t  _pad[3];
    uint32_t alias_of;              // ALIAS: parent tensor id; other kinds: 0 (v2: "_reserved")
    uint64_t data_offset;           // CONST: from start of file, NEON_GRAPH_ALIGN-aligned;
                                    // ALIAS: byte offset inside alias_of; else 0
} NeonGraphTensor;                  // 32 B

typedef struct __attribute__((packed)) {
    uint64_t nsm_offset;            // from start of file, NEON_GRAPH_ALIGN-aligned
    uint64_t nsm_size;
    uint32_t output_tensor;         // graph tensor id
    uint32_t num_inputs;            // operands, in func-argument order
    uint32_t input_tensors[NEON_GRAPH_MAX_FRAG_INPUTS]; // graph tensor id or NEON_GRAPH_NO_TENSOR
    uint32_t source_index;          // manifest "index" (diagnostics only)
    uint32_t _reserved;             // must be 0
} NeonGraphFragment;                // 64 B

#ifdef __cplusplus
static_assert(sizeof(NeonGraphHeader)   == 96, "NeonGraphHeader layout is ABI");
static_assert(sizeof(NeonGraphTensor)   == 32, "NeonGraphTensor layout is ABI");
static_assert(sizeof(NeonGraphFragment) == 64, "NeonGraphFragment layout is ABI");
#endif

// ============================================================
// HELPER MACROS
// ============================================================

#ifdef __cplusplus
// Type-safe size lookup for C++ (enum class requires cast-free comparison)
inline int neonDtypeSize(NeonDtype dtype) {
    switch (dtype) {
    case NeonDtype::FLOAT32: return 4;
    case NeonDtype::INT16:   return 2;
    case NeonDtype::INT8:    return 1;
    case NeonDtype::UINT8:   return 1;
    case NeonDtype::INT4:    return 1; // 2 values per byte — handle separately
    default:                 return 0;
    }
}
#endif

#define NEON_CHECK_MAGIC(hdr) \
    (*(const uint64_t*)(hdr)->magic == *(const uint64_t*)NEON_MAGIC)

#ifdef __cplusplus
// ============================================================
// quantizeMultiplier — float scale -> NeonRequantEntry  (Rev5)
//
// Lives here, in the shared ISA header, on purpose: the toolkit produces
// these entries and the runtime consumes them, so a mismatch between two
// separate copies would be a silent numeric bug. One definition, both
// sides.
//
// Decomposes M into a Q31 mantissa and a power of two:
//
//   M = multiplier * 2^-31 * 2^shift,   multiplier in [2^30, 2^31)
//
// which is the representation TFLite calls quantized_multiplier / shift.
//
// Edge cases handled rather than assumed away:
//   M <= 0        cannot be represented; returns {0,0} so the caller
//                 produces zeros instead of undefined behaviour. A
//                 non-positive scale means the quantisation parameters
//                 themselves are wrong, which is worth failing loudly on
//                 in the toolkit rather than papering over here.
//   M == 1.0      frexp gives 0.5 with exponent 1, so multiplier lands on
//                 2^30 and shift on 1 — exact, no rounding.
//   rounding to   frexp can return exactly 0.5 whose scaled value rounds
//   2^31          up to 2^31, one past the Q31 range; halve and bump the
//                 shift so the invariant holds.
//
// UNCONFIRMED: whether the FX1 OBP scale/shift banks expect this exact
// representation (see docs/vp_open_questions.csv). This is the single
// place to change if they expect e.g. float32 or a different Q format.
// ============================================================
inline NeonRequantEntry quantizeMultiplier(double m) {
    NeonRequantEntry e{0, 0};
    if (!(m > 0.0)) return e;          // also catches NaN

    int    exp = 0;
    double frac = frexp(m, &exp);      // m = frac * 2^exp, frac in [0.5, 1)

    int64_t q = (int64_t)(frac * (double)(1ll << 31) + 0.5);
    if (q == (1ll << 31)) {            // rounded up out of range
        q >>= 1;
        ++exp;
    }

    e.multiplier = (int32_t)q;
    e.shift      = exp;
    return e;
}
#endif // __cplusplus

#ifdef __cplusplus
} // extern "C"
#endif
