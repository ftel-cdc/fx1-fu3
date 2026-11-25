#pragma once

#include <stdint.h>

// ============================================================
// NEON STUDIO — NPU UNSUPPORTED-OP FALLBACK CATALOG
// (formerly core/CPUFallbackCatalog.h — renamed, contents unchanged)
//
// Shared contract (single source of truth, same role as NeonOpcode in
// core/NPU_ISA.h) between toolkit/ (assigns a NeonCPUFallbackId when it
// emits a NeonCPUFallbackInstr) and runtime/ (resolves that same ID to an
// actual, statically-linked CPU function at dispatch time). Neither side
// may hardcode a raw integer where a NeonCPUFallbackId belongs — same
// discipline as core/NPU_ISA.h's NeonOpcode.
//
// See docs/hybrid_npu_cpu_fallback_design.md for the full design this
// file is one piece of: a CPU-fallback escape hatch for ops with no NPU
// realization at all, even after decomposition. Not needed for this
// repo's own custom targets (VP_FX1_V2.0/qemu_mps3_an547 — both have broad
// native op coverage by design); exists ahead of vendor-NPU target work,
// where native op coverage isn't something this team controls.
//
// First real entry added below: NEON_CPU_FALLBACK_CONV2D, once a real
// consumer existed (runtime/opfallbacks/Conv2DCPUFallback.h/.cpp,
// wrapping the existing analyse/compiler/CompilerOpConv2D.h conv2D_Func()
// reference implementation). Still no toolkit emitter — this entry is
// dispatchable (Executor can run it if registered and reached) but not yet
// reachable from an actual compiled model. Don't add further speculative
// entries (e.g. a guessed LAYERNORM fallback) beyond what a real target
// concretely needs — see the decision to defer
// GEMM_FUSED/FUSED_ATTN/LAYERNORM/ELEM_WISE opcodes in core/NPU_ISA.h
// until a real emitter/consumer exists; same discipline applies here.
//
// See NeonCPUFallbackInstr's own comment in core/NPU_ISA.h for why the ISA
// version macros still aren't bumped (no toolkit emitter yet).
// ============================================================

#ifdef __cplusplus
extern "C" {
#endif

typedef uint32_t NeonCPUFallbackId;

// 0 is reserved and must never be assigned to a real fallback kind —
// mirrors NeonOpcode's own convention of never using 0x00 for a real
// opcode (see core/NPU_ISA.h: CONV2D starts at 0x01). A stray/uninitialized
// NeonCPUFallbackInstr.fallback_id of 0 should fail lookup loudly rather
// than silently resolving to some arbitrary first entry.
#define NEON_CPU_FALLBACK_RESERVED ((NeonCPUFallbackId)0)

// Conv2D-shaped fallback — see core/NPU_ISA.h's NeonCPUFallbackConv2DInstr
// and runtime/opfallbacks/Conv2DCPUFallback.h/.cpp.
#define NEON_CPU_FALLBACK_CONV2D ((NeonCPUFallbackId)1)

// Transpose fallback (rank <= 4, int8/permute) — see core/NPU_ISA.h's
// NeonCPUFallbackTransposeInstr and runtime/opfallbacks/TransposeCPUFallback.h/.cpp.
#define NEON_CPU_FALLBACK_TRANSPOSE ((NeonCPUFallbackId)2)

// Im2Col fallback (Rich mode Conv2D >= 3x3) — see core/NPU_ISA.h's
// NeonCPUFallbackIm2ColInstr and runtime/opfallbacks/Im2ColCPUFallback.h/.cpp.
#define NEON_CPU_FALLBACK_IM2COL ((NeonCPUFallbackId)3)

// LUT fallback (Rich mode post-GEMM activation+requant) — see core/NPU_ISA.h's
// NeonCPUFallbackLutInstr and runtime/opfallbacks/LutCPUFallback.h/.cpp.
#define NEON_CPU_FALLBACK_LUT ((NeonCPUFallbackId)4)

// Depthwise Conv2D fallback — see core/NPU_ISA.h's NeonCPUFallbackDWConvInstr
// and runtime/opfallbacks/DWConvCPUFallback.h/.cpp.
#define NEON_CPU_FALLBACK_DWCONV ((NeonCPUFallbackId)5)

// Slice fallback — see core/NPU_ISA.h's NeonCPUFallbackSliceInstr
// and runtime/opfallbacks/SliceCPUFallback.h/.cpp.
#define NEON_CPU_FALLBACK_SLICE ((NeonCPUFallbackId)6)

// Concatenation fallback — see core/NPU_ISA.h's NeonCPUFallbackConcatInstr
// and runtime/opfallbacks/ConcatCPUFallback.h/.cpp.
#define NEON_CPU_FALLBACK_CONCAT ((NeonCPUFallbackId)7)

// Resize Nearest Neighbor fallback — see core/NPU_ISA.h's NeonCPUFallbackResizeInstr
// and runtime/opfallbacks/ResizeCPUFallback.h/.cpp.
#define NEON_CPU_FALLBACK_RESIZE_NN ((NeonCPUFallbackId)8)

// Depthwise-Conv2D im2col fallback — gathers ONE channel's spatial patches out of
// a multi-channel NHWC tensor (regular Im2Col's `ic` field can't do this: it is used
// as both gather-count and per-pixel stride, see NeonCPUFallbackIm2ColInstr). See
// core/NPU_ISA.h's NeonCPUFallbackDWIm2ColInstr and
// runtime/opfallbacks/DWIm2ColCPUFallback.h/.cpp.
#define NEON_CPU_FALLBACK_DW_IM2COL ((NeonCPUFallbackId)9)

// MaxPool2D fallback (2D, overlapping, padded — neither ELEM_WISE's 1D
// non-overlapping hardware mode nor the compat bank can express this). See
// core/NPU_ISA.h's NeonCPUFallbackMaxPoolInstr and
// runtime/opfallbacks/MaxPoolCPUFallback.h/.cpp.
#define NEON_CPU_FALLBACK_MAXPOOL ((NeonCPUFallbackId)10)

// Softmax fallback — reduction (max, sum) + LUT-based exp/reciprocal +
// requantize. Neither ELEM_WISE's MAX_POOL mode (1D, non-overlapping, no
// LUT) nor the compat bank has anything resembling this; FUSED_ATTN/
// LAYERNORM (rich opcodes 0x13/0x14) implement a similar reduction shape in
// the emulator but are dead ISA plumbing never emitted by this toolkit. See
// core/NPU_ISA.h's NeonCPUFallbackSoftmaxInstr and
// runtime/opfallbacks/SoftmaxCPUFallback.h/.cpp. Algorithm ported verbatim
// from Conversion/TopToLinalg/OpLowering/Softmax.cpp's quantized branch —
// see that pass's own header comment ("Hardware-Aware Reciprocal LUT + CLZ
// Algorithm") for the origin of this design.
#define NEON_CPU_FALLBACK_SOFTMAX ((NeonCPUFallbackId)11)

// Constant-materialization fallback — copies a compile-time-folded constant
// from the weight buffer into the output buffer. Needed because ops that
// fold entirely to a constant (e.g. BROADCAST_TO over a constant coordinate
// vector, which the TFLite frontend folds via np.broadcast_to) leave a
// module with NO compute ops at all; without this the emitter would write an
// empty .nsm and silently report success. See core/NPU_ISA.h's
// NeonCPUFallbackConstCopyInstr and runtime/opfallbacks/ConstCopyCPUFallback.h/.cpp.
#define NEON_CPU_FALLBACK_CONST_COPY ((NeonCPUFallbackId)12)

// --- CPU_FALLBACK, Pad variant ---
// Zero/const-fills the full destination, then copies the source into an
// offset subview of it. Distinct from SLICE (NEON_CPU_FALLBACK_SLICE),
// which extracts FROM an offset source into a dense destination — PAD is
// the structural inverse (dense source INTO an offset destination) and
// SLICE's instruction/runtime cannot express it (see AccelToNSMPass.cpp
// section 2.3 and SliceCPUFallback.cpp).
#define NEON_CPU_FALLBACK_PAD ((NeonCPUFallbackId)13)

// LayerNorm fallback — per-row mean/variance reduction + PWL 1/sqrt LUT
// (pwl_lut_mapping_en.md Part B) + three requant stages. The rich LAYERNORM
// opcode (0x14) cannot be used: its emulator is not quantization-aware (no
// scales, no zero points, hardcoded eps). See core/NPU_ISA.h's
// NeonCPUFallbackLayerNormInstr and runtime/opfallbacks/LayerNormCPUFallback.h/.cpp.
#define NEON_CPU_FALLBACK_LAYERNORM ((NeonCPUFallbackId)14)

// Embedding fallback — Gather rows of a const int8 table + optional folded const bias + requant
#define NEON_CPU_FALLBACK_EMBEDDING ((NeonCPUFallbackId)15)

// Compiled kernel — calls a compiler-generated CPU entry point (e.g. the
// OpenC906 backend's RISC-V object) registered at program start under
// NeonCPUFallbackKernelInstr::kernel_id (runtime/opfallbacks/CompiledKernelRegistry.h).
#define NEON_CPU_FALLBACK_COMPILED_KERNEL ((NeonCPUFallbackId)16)

// StridedSlice fallback — rank <= 4 strided gather (pure data movement, no
// requant). begin is pre-normalized by the compiler (masks applied, negative
// indices wrapped, clamped); stride may be negative, never 0. Distinct from
// SLICE (6), which has no stride. See core/NPU_ISA.h's
// NeonCPUFallbackStridedSliceInstr and
// runtime/opfallbacks/StridedSliceCPUFallback.h/.cpp.
#define NEON_CPU_FALLBACK_STRIDED_SLICE ((NeonCPUFallbackId)17)

// Add further real entries here, each with a stable numeric value (never
// renumber an existing one once a compiled .nsm file could reference it —
// same append-only discipline as NeonOpcode), only once a real target and
// a real op require it.

#ifdef __cplusplus
} // extern "C"
#endif
