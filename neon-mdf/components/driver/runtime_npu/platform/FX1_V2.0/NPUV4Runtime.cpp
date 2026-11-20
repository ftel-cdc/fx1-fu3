#include "NPUV4Runtime.h"
#include "SoftwareRequantFallback.h"
#include "hal/NPUV4MMIOWriter.h"
#include "FX1V42Regs.h"
#include "runtime/opfallbacks/TransposeCPUFallback.h"
#include "runtime/opfallbacks/Im2ColCPUFallback.h"
#include "runtime/opfallbacks/DWIm2ColCPUFallback.h"
#include "runtime/opfallbacks/LutCPUFallback.h"
#include "runtime/opfallbacks/DWConvCPUFallback.h"
#include "runtime/opfallbacks/SliceCPUFallback.h"
#include "runtime/opfallbacks/StridedSliceCPUFallback.h"
#include "runtime/opfallbacks/PadCPUFallback.h"
#include "runtime/opfallbacks/ConcatCPUFallback.h"
#include "runtime/opfallbacks/ResizeCPUFallback.h"
#include "runtime/opfallbacks/MaxPoolCPUFallback.h"
#include "runtime/opfallbacks/SoftmaxCPUFallback.h"
#include "runtime/opfallbacks/LayerNormCPUFallback.h"
#include "runtime/opfallbacks/ConstCopyCPUFallback.h"
#include "runtime/opfallbacks/EmbeddingCPUFallback.h"
#include "runtime/opfallbacks/CPUFallbackRegistry.h"
#include <cstddef>
#include <cstring>
#include <cmath>
#include <cassert>

extern "C" {
#include "FreeRTOS.h"
#include "bsp_ext/npu_v4_native_driver.h"
#include "soc/soc_memory_map.h"
#include "task.h"
#include "uart.h"
}

// Must match the compiler's switch (toolkit/mlir/LegacyNativeConv.h). 0 = dense CONV2D
// only ever arrives as HAS_CONV2D; the native CONV2D handler remains for native
// depthwise, which issues per-channel jobs (channels_in == channels_out == 1).
#ifndef NEON_LEGACY_NATIVE_CONV2D
#define NEON_LEGACY_NATIVE_CONV2D 0
#endif


// OBP-A channel indexing for native jobs (Makefile NPU_OBP_MODE). OBP_CONFIG.cfg is
// 8 bits wide, so the compiler cannot carry bit 8; it is applied here.
#if defined(NPU_OBP_LANE_MODE)
static constexpr uint32_t kObpChannelModeBits = 0u;
static constexpr const char *kObpChannelModeName = "LANE";
#else
static constexpr uint32_t kObpChannelModeBits = FX1_OBP_CFG_VEC_CHANNEL_MODE;
static constexpr const char *kObpChannelModeName = "VECTOR";
#endif


// A zero in any of these fields makes analyse/ emit a full table of zeros with
// no error. Catch it at build/runtime instead of in a report someone trusts.
static void validateV45Specs() {
    NPUV4Runtime rt;
    const NPUHWSpecs s = rt.getNPUSpecs();
    (void)s;
    assert(s.peak_tops            > 0.f && "peak_tops == 0 -> estimated_ms always 0");
    assert(s.clock_mhz            > 0.f && "clock_mhz == 0 -> actual_ms never computed");
    assert(s.dram_bandwidth_gbps  > 0.f && "dram_bw == 0 -> every layer classified UNKNOWN");
    assert(s.sram_size_bytes      > 0u  && "sram == 0 -> weight_fits_sram always false");
    assert(s.matrix_block.sa.rows == 32 && s.matrix_block.sa.cols == 32);
    assert(s.sram_size_bytes == fx1_v45::kSramTotalBytes);
}

NPUDriverStatus NPUV4Runtime::open() {
  validateV45Specs();
  CPUFallbackRegistry::get().registerFallback(NEON_CPU_FALLBACK_TRANSPOSE, transposeCPUFallback);
  CPUFallbackRegistry::get().registerFallback(NEON_CPU_FALLBACK_IM2COL, im2colCPUFallback);
  CPUFallbackRegistry::get().registerFallback(NEON_CPU_FALLBACK_DW_IM2COL, dwIm2colCPUFallback);
  CPUFallbackRegistry::get().registerFallback(NEON_CPU_FALLBACK_LUT, lutCPUFallback);
  CPUFallbackRegistry::get().registerFallback(NEON_CPU_FALLBACK_DWCONV, dwConvCPUFallback);
  CPUFallbackRegistry::get().registerFallback(NEON_CPU_FALLBACK_SLICE, sliceCPUFallback);
  CPUFallbackRegistry::get().registerFallback(NEON_CPU_FALLBACK_STRIDED_SLICE, stridedSliceCPUFallback);
  CPUFallbackRegistry::get().registerFallback(NEON_CPU_FALLBACK_PAD, padCPUFallback);
  CPUFallbackRegistry::get().registerFallback(NEON_CPU_FALLBACK_CONCAT, concatCPUFallback);
  CPUFallbackRegistry::get().registerFallback(NEON_CPU_FALLBACK_RESIZE_NN, resizeCPUFallback);
  CPUFallbackRegistry::get().registerFallback(NEON_CPU_FALLBACK_MAXPOOL, maxPoolCPUFallback);
  CPUFallbackRegistry::get().registerFallback(NEON_CPU_FALLBACK_SOFTMAX, softmaxCPUFallback);
  CPUFallbackRegistry::get().registerFallback(NEON_CPU_FALLBACK_LAYERNORM, layerNormCPUFallback);
  CPUFallbackRegistry::get().registerFallback(NEON_CPU_FALLBACK_CONST_COPY, constCopyCPUFallback);
  CPUFallbackRegistry::get().registerFallback(NEON_CPU_FALLBACK_EMBEDDING, embeddingCPUFallback);
  initRceLUTs();
  is_initialized_ = true;
  has_native_config_ = false;
  configured_output_zero_point_ = 0;
  obp_bias_base_ = 0;
  im2col_enabled_ = false;
  im2col_ih_ = 0;
  im2col_iw_ = 0;
  im2col_ic_ = 0;
  im2col_kh_ = 0;
  im2col_kw_ = 0;
  im2col_stride_h_ = 1;
  im2col_stride_w_ = 1;
  im2col_pad_h_ = 0;
  im2col_pad_w_ = 0;
  im2col_oh_ = 1;
  im2col_ow_ = 1;
  uart_puts("[NPUV4Runtime] OBP-A channel mode: ");
  uart_puts(kObpChannelModeName);
  uart_puts("\r\n");
  uart_puts("[NPUV4Runtime] open succeeded\r\n");
  return NPUDriverStatus::OK;
}

void NPUV4Runtime::initRceLUTs() {
  // --- EXP LUT (256 bytes) ---
  // Quantized exp(x) table for Softmax pass2 (same as SoftmaxEmitter::expLut)
  // exp_lut[j] = round(255 * exp(-(j/31.875 - 8)))  for j in [0,255]
  uint8_t exp_lut[256];
  for (int j = 0; j < 256; ++j) {
    double x = (double)j / 31.875 - 8.0;
    double v = 255.0 * std::exp(-x);
    exp_lut[j] = (uint8_t)(v > 255.0 ? 255 : (v < 0.0 ? 0 : (uint8_t)v));
  }
  npu_rce_load_exp_a(exp_lut);

  // --- RECIP LUT (512 bytes) ---
  // recip_lut[j] = round(2^15 / (1 + j/256))  for j in [0,511]
  // Matches SoftmaxEmitter and hwsim/fx1_sm_recip_lut
  uint8_t recip_lut[512];
  for (int j = 0; j < 512; ++j) {
    double D = 1.0 + (double)j / 256.0;
    int64_t v = (int64_t)round(32768.0 / D);
    if (v > 32767) v = 32767;
    recip_lut[j] = (uint8_t)(v & 0xFF);  // low byte for byte-addressed MMIO
  }
  npu_rce_load_recip_a(recip_lut);

  // --- RSQRT LUT (1024 x uint16_t = 2048 bytes) ---
  // rsqrt_lut[a] = round(32768 / sqrt((1 + (a&0xFF)/256) * 2^(a>>8)))
  // Matches LayerNormEmitter and pwl_lut_mapping_en.md Step B3
  uint16_t rsqrt_lut[1024];
  for (int a = 0; a < 1024; ++a) {
    double Dpp = (1.0 + (double)(a & 0xFF) / 256.0) * (double)(1 << (a >> 8));
    int64_t v = (int64_t)round(32768.0 / std::sqrt(Dpp));
    rsqrt_lut[a] = (uint16_t)(v > 32767 ? 32767 : v);
  }
  npu_rce_load_rsqrt_a(rsqrt_lut);
}

void NPUV4Runtime::resetPerModelState() {
  last_op_ = static_cast<NeonOpcode>(0);
  current_model_ = nullptr;
  current_k_ = FX1_SA_TILE;
  bias_offset_raw_ = 0;
  weight_offset_raw_ = 0;
  output_offset_ = 0;
  output_zero_point_ = 0;
  full_channels_out_ = FX1_SA_TILE;

  native_is_fc_ = false;
  native_nchw_ = false;
  native_out_stride_ = FX1_SA_TILE;
  native_out_ptr_ = nullptr;
  weights_staged_ = false;
  staged_weights_source_ = nullptr;
  staged_weights_size_ = 0;

  has_native_config_ = false;
  configured_output_zero_point_ = 0;
  obp_bias_base_ = 0;

  im2col_enabled_ = false;
  im2col_ih_ = 0;  im2col_iw_ = 0;  im2col_ic_ = 0;
  im2col_kh_ = 0;  im2col_kw_ = 0;
  im2col_stride_h_ = 1;  im2col_stride_w_ = 1;
  im2col_pad_h_ = 0;  im2col_pad_w_ = 0;
  im2col_oh_ = 0;  im2col_ow_ = 0;
  im2col_tile_counter_ = 0;
  im2col_izp_ = 0;

  add_num_elements_ = 0;  add_input_b_offset_ = 0;  add_b_is_scalar_ = false;
  mul_num_elements_ = 0;  mul_input_b_offset_ = 0;  mul_b_is_scalar_ = false;
  sub_num_elements_ = 0;  sub_input_b_offset_ = 0;  sub_b_is_scalar_ = false;
  operand_b_base_ = nullptr;

  sum_k_ = 0;  sum_n_ = 0;  sum_param_offset_ = 0;
  sum_output_zero_point_ = 0;  sum_input_idx_ = 0;  sum_output_idx_ = 0;

  pending_ctx_ = {};
  pending_rich_instr_ = {};
}

NPUDriverStatus NPUV4Runtime::ensureWeightsStaged(const char* consumer) {
  if (!current_model_ || !current_model_->weight_buffer ||
      current_model_->weight_buffer_size == 0) {
    uart_puts("[NPUV4Runtime] ERROR: WGT0 consumer has no weight buffer\r\n");
    return NPUDriverStatus::ERROR_INVALID_ADDR;
  }

  const void *source = current_model_->weight_buffer;
  const size_t size = current_model_->weight_buffer_size;
  if (weights_staged_ && staged_weights_source_ == source &&
      staged_weights_size_ == size) {
#if defined(NEON_WGT_STAGING_TRACE)
    uart_puts("[NPUV4Runtime] WGT0 cache-hit consumer=");
    uart_puts(consumer);
    uart_put_u32(" bytes=", static_cast<uint32_t>(size), "\r\n");
#else
    (void)consumer;
#endif
    return NPUDriverStatus::OK;
  }

  // The fragment's BLOB slice is canonical. Stage it only when a WGT0
  // consumer needs it; HAS_CONV2D stages its own slice into the HAS window.
  std::memcpy(reinterpret_cast<void *>(static_cast<uintptr_t>(CDC_NPU_WGT0_BASE)),
              source, size);
  weights_staged_ = true;
  staged_weights_source_ = source;
  staged_weights_size_ = size;
  current_model_->weights_on_device = true;
  if (!current_model_->weights.empty())
    current_model_->weights[0].on_device = true;
#if defined(NEON_WGT_STAGING_TRACE)
  uart_puts("[NPUV4Runtime] WGT0 lazy-stage consumer=");
  uart_puts(consumer);
  uart_put_u32(" bytes=", static_cast<uint32_t>(size), "");
  uart_puts(" source_addr=");
  uart_put_u32("", static_cast<uint32_t>(reinterpret_cast<uintptr_t>(source)), "\r\n");
#endif
  return NPUDriverStatus::OK;
}

void NPUV4Runtime::setModel(const NPUModel *model) {
  current_model_ = model;
  if (!model)
    return;
  for (const auto &op : model->ops) {
    if (op.opcode == NeonOpcode::NATIVE_CONFIG && op.instr_ptr &&
        op.instr_size >= offsetof(NeonNativeConfigInstr, act_layout)) {
      const auto *instr =
          reinterpret_cast<const NeonNativeConfigInstr *>(op.instr_ptr);
      native_nchw_ = (op.instr_size > offsetof(NeonNativeConfigInstr, act_layout)) &&
                     instr->act_layout == NEON_LAYOUT_NCHW;
      im2col_ih_ = instr->input_h;
      im2col_iw_ = instr->input_w;
      im2col_ic_ = instr->channels_in;
      im2col_kh_ = instr->kernel_h;
      im2col_kw_ = instr->kernel_w;
      im2col_stride_h_ = instr->stride_h;
      im2col_stride_w_ = instr->stride_w;
      im2col_pad_h_ = instr->pad_h;
      im2col_pad_w_ = instr->pad_w;
      im2col_oh_ = instr->output_h;
      im2col_ow_ = instr->output_w;
      full_channels_out_ = instr->channels_out;
      output_offset_ = instr->output_offset;
      im2col_enabled_ = true;
      has_native_config_ = true;
      obp_bias_base_ = instr->bias_offset;
      configured_output_zero_point_ = instr->output_zero_point;

      native_kh_ = instr->kernel_h;
      native_kw_ = instr->kernel_w;
      native_stride_ = (instr->stride_w > 0) ? instr->stride_w : 1;
      native_dil_ = (instr->dilation_w > 0) ? instr->dilation_w : 1;
      native_cin_ = instr->channels_in;
      native_pad_h_ = instr->pad_h;
      native_pad_w_ = instr->pad_w;
      native_k_exact_ = instr->kernel_h * instr->kernel_w * instr->channels_in;
      native_oc_total_ = instr->channels_out;
      native_oh_ = instr->output_h;
      native_ow_ = instr->output_w;

      uart_puts("[NPUV4Runtime] Pre-configured geometry from NATIVE_CONFIG\r\n");
      break;
    }
  }
}

NPUDriverStatus NPUV4Runtime::close() {
  is_initialized_ = false;
  return NPUDriverStatus::OK;
}

NPUDriverStatus NPUV4Runtime::reset() {
  perf_cycles_ = 0;
  return NPUDriverStatus::OK;
}

NPUDriverStatus NPUV4Runtime::dmaTransfer(int channel, void *src, void *dst,
                                          size_t size_bytes, DMADir direction) {
  (void)channel;
  if (size_bytes == 0)
    return NPUDriverStatus::OK;

  if (direction == DMADir::HOST_TO_DEVICE && src) {
    // Copy to NPU input activation buffer if channel 2, or weights if channel 0
    if (channel == 0) {
      std::memcpy((void *)(uintptr_t)CDC_NPU_WGT0_BASE, src, size_bytes);
    } else {
      std::memcpy((void *)(uintptr_t)CDC_VPU_OUT0_BASE, src, size_bytes);
    }
  } else if (direction == DMADir::DEVICE_TO_HOST && dst) {
    std::memcpy(dst, (const void *)(uintptr_t)CDC_NPU_WORK0_BASE, size_bytes);
  }
  return NPUDriverStatus::OK;
}

NPUDriverStatus NPUV4Runtime::dmaWait(int channel) {
  (void)channel;
  return NPUDriverStatus::OK;
}

NPUDriverStatus NPUV4Runtime::setNSplit(uint8_t n_split) {
  uart_put_u32("[NPUV4Runtime] setNSplit: n_split=",
               static_cast<uint32_t>(n_split), "\r\n");
  // CDC_NPU_NATIVE_CON_NSPLIT is offset 0x0214 from CDC_NPU0_BASE
  *reinterpret_cast<volatile uint32_t *>(CDC_NPU0_BASE + 0x0214) = n_split;
  return NPUDriverStatus::OK;
}

NPUDriverStatus NPUV4Runtime::writeInstruction(const void *instr_bytes,
                                               size_t size) {
  return writeInstructionEx(instr_bytes, size, UINT32_MAX, UINT32_MAX);
}

NPUDriverStatus NPUV4Runtime::writeInstructionEx(const void *instr_bytes,
                                                 size_t size,
                                                 uint32_t input_idx,
                                                 uint32_t output_idx) {
  if (!instr_bytes || size < sizeof(NeonOpcode)) {
    return NPUDriverStatus::ERROR_INVALID_ADDR;
  }

  // Reset context
  pending_ctx_ = {};
  pending_ctx_.weight_zero_point = 0; // INT8 symmetric weights (convention)
  pending_ctx_.output_offset = output_offset_;

  if (current_model_ && !current_model_->requant_table.empty()) {
    size_t in_idx =
        (input_idx < current_model_->requant_table.size()) ? input_idx : 0;
    pending_ctx_.input_zero_point =
        current_model_->requant_table[in_idx].zero_point;
    pending_ctx_.output_zero_point =
        (current_model_->output_tensor_idx <
         current_model_->requant_table.size())
            ? current_model_->requant_table[current_model_->output_tensor_idx]
                  .zero_point
            : 0;
  } else {
    pending_ctx_.input_zero_point = 0;
    pending_ctx_.output_zero_point = 0;
  }
  if (has_native_config_ && configured_output_zero_point_ != 0)
    pending_ctx_.output_zero_point = configured_output_zero_point_;

  NeonOpcode op = *static_cast<const NeonOpcode *>(instr_bytes);
  last_op_ = op;

  // Whole-layer conv on HasNpuTop: keep the instruction and the fragment's bound
  // tensors (OperandBinder::bindTensors set tensors[].data, as the FC path relies on).
  if (op == NeonOpcode::HAS_CONV2D) {
    if (size < sizeof(NeonHasConv2DInstr)) return NPUDriverStatus::ERROR_INVALID_ADDR;
    pending_has_conv_ = *static_cast<const NeonHasConv2DInstr *>(instr_bytes);
    has_in_ptr_  = (current_model_ && input_idx < current_model_->tensors.size())
        ? static_cast<const int8_t *>(current_model_->tensors[input_idx].data) : nullptr;
    has_out_ptr_ = (current_model_ && output_idx < current_model_->tensors.size())
        ? static_cast<int8_t *>(current_model_->tensors[output_idx].data) : nullptr;
    return NPUDriverStatus::OK;
  }
  if (op == NeonOpcode::HAS_ELEMWISE) {
    if (size < sizeof(NeonHasElemwiseInstr)) return NPUDriverStatus::ERROR_INVALID_ADDR;
    pending_has_elem_ = *static_cast<const NeonHasElemwiseInstr *>(instr_bytes);
    has_in_ptr_  = (current_model_ && input_idx < current_model_->tensors.size())
        ? static_cast<const int8_t *>(current_model_->tensors[input_idx].data) : nullptr;
    has_out_ptr_ = (current_model_ && output_idx < current_model_->tensors.size())
        ? static_cast<int8_t *>(current_model_->tensors[output_idx].data) : nullptr;
    return NPUDriverStatus::OK;
  }
  if (op == NeonOpcode::SET_NSPLIT && size >= sizeof(NeonSetNSplitInstr)) {
    const auto *instr = static_cast<const NeonSetNSplitInstr *>(instr_bytes);
    return setNSplit(instr->n_split);
  }
  if (op == NeonOpcode::OBP_CONFIG && size >= sizeof(NeonOBPConfigInstr)) {
    const auto *instr = static_cast<const NeonOBPConfigInstr *>(instr_bytes);
    if (instr->bias_offset != 0 || instr->scale_offset != 0 ||
        instr->shift_offset != 0 || instr->lut_offset != 0) {
      const NPUDriverStatus stage_status = ensureWeightsStaged("OBP_CONFIG");
      if (stage_status != NPUDriverStatus::OK) return stage_status;
    }
    npu_native_set_profile(FX1_PROFILE_V1_SAURIA);
    obp_lut_from_compiler_ = (instr->lut_offset != 0);

    uint8_t *base = reinterpret_cast<uint8_t *>(CDC_NPU_WGT0_BASE);
    if (instr->bias_offset != 0) {
      const int32_t *bias_v =
          reinterpret_cast<const int32_t *>(base + instr->bias_offset);
      npu_obp_load_bias_a(bias_v, instr->lane_count);
    }
    if (instr->scale_offset != 0) {
      const uint32_t *scale_v =
          reinterpret_cast<const uint32_t *>(base + instr->scale_offset);
      npu_obp_load_scale_a(scale_v, instr->lane_count);
    }
    if (instr->shift_offset != 0) {
      const uint32_t *shift_v =
          reinterpret_cast<const uint32_t *>(base + instr->shift_offset);
      npu_obp_load_shift_a(shift_v, instr->lane_count);
    }
    if (instr->lut_offset != 0) {
      const uint8_t *lut_v =
          reinterpret_cast<const uint8_t *>(base + instr->lut_offset);
      npu_obp_load_lut_a(lut_v, instr->lane_count);
    }
    (instr->cfg);
    obp_lut_from_compiler_ = (instr->lut_offset != 0);
    obp_cfg_ = instr->cfg;
    return NPUDriverStatus::OK;
  }
  if (op == NeonOpcode::CPU_FALLBACK && size >= sizeof(NeonCPUFallbackInstr)) {
    const auto *fb = static_cast<const NeonCPUFallbackInstr *>(instr_bytes);
    if (fb->fallback_id == NEON_CPU_FALLBACK_DW_IM2COL && size >= sizeof(NeonCPUFallbackDWIm2ColInstr)) {
      const auto *dw = static_cast<const NeonCPUFallbackDWIm2ColInstr *>(instr_bytes);
      im2col_izp_ = dw->input_zero_point;
    } else if (fb->fallback_id == NEON_CPU_FALLBACK_IM2COL && size >= offsetof(NeonCPUFallbackIm2ColInstr, layout)) {
      const auto *im = static_cast<const NeonCPUFallbackIm2ColInstr *>(instr_bytes);
      im2col_izp_ = im->input_zero_point;
    }
  }
  // Native Rich instructions carry every operand address in the instruction
  // itself, so writeInstruction only has to remember the latest one; trigger()
  // programs the MMIO window from it. Dtype-agnostic: the same path serves the
  // INT8 V4.4 engine that previously served the FP32 emulation.
  if ((op == NeonOpcode::GEMM_FUSED || op == NeonOpcode::ELEM_WISE ||
       op == NeonOpcode::LAYERNORM || op == NeonOpcode::FUSED_ATTN) &&
      size >= sizeof(NeonRichInstr)) {
    const auto *instr = static_cast<const NeonRichInstr *>(instr_bytes);
    pending_rich_instr_ = *instr;
    return NPUDriverStatus::OK;
  }
  if (op == NeonOpcode::CONV2D && size >= sizeof(NeonConv2DInstr)) {
    static int conv2d_count = 0;
    // if (conv2d_count++ < 5) {
    //   // uart_put_i32("[NPUV4Runtime] CONV2D output_zero_point=",
    //   // pending_ctx_.output_zero_point, "\r\n");
    // }
    const auto *instr = static_cast<const NeonConv2DInstr *>(instr_bytes);
    const NPUDriverStatus stage_status = ensureWeightsStaged("CONV2D");
    if (stage_status != NPUDriverStatus::OK) return stage_status;
#if !NEON_LEGACY_NATIVE_CONV2D
    if (instr->channels_in != 1 || instr->channels_out != 1) {
      uart_puts("[NPUV4Runtime] ERROR: dense CONV2D on the legacy V4.7 native path is "
                "disabled (NEON_LEGACY_NATIVE_CONV2D=0) - recompile the model so every "
                "CONV2D is HAS_CONV2D\r\n");
      return NPUDriverStatus::ERROR_UNSUPPORTED_OP;
    }
#endif
    // A CONV2D normally spans the whole layer (channels_out == NATIVE_CONFIG.channels_out).
    // The native depthwise path issues one CONV2D per channel (channels_out = 1); its
    // bias/multiplier/shift tables are still laid out for the whole layer, so table bases
    // use layer_oc and the job covers [oc_start, oc_start + channels_out).
    const uint32_t layer_oc =
        (has_native_config_ && full_channels_out_ > instr->channels_out)
            ? full_channels_out_ : static_cast<uint32_t>(instr->channels_out);
    current_k_ = static_cast<uint32_t>(instr->kernel_h) * instr->kernel_w *
                 instr->channels_in;
    // Align K to the systolic tile to match the padded weight/activation layout.
    current_k_ = (current_k_ + FX1_SA_TILE - 1u) / FX1_SA_TILE * FX1_SA_TILE;
#if defined(NPU_DISPATCH_NATIVE)
    // One OC-tile of weights is [K_padded][32] and must fit ONE SRAM B bank (81 KiB).
    if (current_k_ == 0 || (current_k_ * FX1_SA_TILE) > FX1_SRAMB_BANK_BYTES) {
      uart_put_i32("[NPUV4Runtime] ERROR: invalid K-dim=", current_k_, "\r\n");
      return NPUDriverStatus::ERROR_INVALID_ADDR;
    }
#else
    // Rich path: instruction_decoder.h clamps weight prefetch to 81 KiB (k*n).
    if (current_k_ == 0 || current_k_ > (FX1_SRAMB_BANK_BYTES / FX1_SA_TILE)) {
      uart_put_i32("[NPUV4Runtime] ERROR: invalid K-dim=", current_k_, "\r\n");
      return NPUDriverStatus::ERROR_INVALID_ADDR;
    }
#endif
    // For Im2Col GEMM the activation buffer is [M=64 rows x K_padded cols], so
    // the row stride is K_padded (NOT input_w * channels_in, the raw image
    // stride). current_k_ is already the tile-aligned K computed above.
    pending_ctx_.activation_stride = current_k_;
    bias_offset_raw_ = instr->bias_offset;
    weight_offset_raw_ = instr->weight_offset;

    if (current_model_ && current_model_->weight_buffer &&
        instr->bias_offset != 0) {
      pending_ctx_.has_bias = true;
      pending_ctx_.has_requantization = true;

      // The DMA channel 1 copies the entire weight buffer (including OBP
      // params) to CDC_NPU_WGT0_BASE. Therefore, the HW should read
      // bias/mult/shift/lut from CDC_NPU_WGT0_BASE + offset, NOT from host
      // DRAM.
      uint8_t *base = reinterpret_cast<uint8_t *>(CDC_NPU_WGT0_BASE);

      // Bias array pointer. instr->bias_offset already points at the exact
      // per-OC-tile bias entries (see AccelToNSMPass::walkBlock,
      // conv.bias_offset), no correction needed.
      pending_ctx_.bias_ptr =
          reinterpret_cast<const int32_t *>(base + instr->bias_offset);
      pending_ctx_.bias_size_bytes = instr->channels_out * sizeof(int32_t);

      // Which OC-tile (0, 32, ...) this instruction's bias_offset falls in,
      // relative to this layer's OBP block base (captured from the layer's
      // NATIVE_CONFIG, whose bias_offset is the un-adjusted, tile-0 base).
      uint32_t out_channel_idx =
          obp_bias_base_ && instr->bias_offset >= obp_bias_base_
              ? static_cast<uint32_t>((instr->bias_offset - obp_bias_base_) /
                                      sizeof(int32_t))
              : 0;

      // Conv2DOBPSerializer writes bias, multiplier and shift back to back,
      // each OC_padded entries of int32. OC_padded must be derived the same way
      // on both sides — see the OC_padded computation in
      // Conv2DOBPSerializer::serialize(). obp_bias_base_ IS the bias section's
      // start, so each following section sits one OC_padded-sized array later.
      const uint32_t oc_padded =
          (layer_oc + FX1_SA_TILE - 1u) / FX1_SA_TILE * FX1_SA_TILE;
      const int32_t *multiplier_base = reinterpret_cast<const int32_t *>(
          base + obp_bias_base_ + oc_padded * sizeof(int32_t));
      pending_ctx_.multiplier_ptr = multiplier_base + out_channel_idx;
      pending_ctx_.multiplier_size_bytes =
          instr->channels_out * sizeof(int32_t);

      const int32_t *shift_base = multiplier_base + oc_padded;
      pending_ctx_.shift_ptr = shift_base + out_channel_idx;
      pending_ctx_.shift_size_bytes = instr->channels_out * sizeof(int32_t);
    }
    if (instr->output_zero_point != 0)
      pending_ctx_.output_zero_point = instr->output_zero_point;
    native_act_mode_ = static_cast<uint8_t>(instr->act_mode);
    if (instr->act_mode == 1) { // ReLU
      // TFLite's CalculateActivationRangeQuantized: the quantized representation
      // of real 0.0 is the output zero point, NOT literal 0. Every conv output in
      // a QAT int8 MobileNet has zp = -128, so hardcoding 0 clamped away the whole
      // negative half of a correct output tensor.
      pending_ctx_.act_min = pending_ctx_.output_zero_point;
      pending_ctx_.act_max = 127;
    } else if (instr->act_mode == 2 && pending_ctx_.shift_ptr) { // SiLU
      pending_ctx_.act_min = -128;
      pending_ctx_.act_max = 127;
      pending_ctx_.has_lut = true;
      // LUT follows the bias, multiplier and shift arrays (OC_padded int32
      // entries each) and is shared across all OC-tiles, so it takes no
      // out_channel_idx offset.
      const uint32_t oc_padded =
          (layer_oc + FX1_SA_TILE - 1u) / FX1_SA_TILE * FX1_SA_TILE;
      pending_ctx_.lut_data =
          reinterpret_cast<const uint8_t *>(CDC_NPU_WGT0_BASE + obp_bias_base_ +
                                            3u * oc_padded * sizeof(int32_t));
      pending_ctx_.lut_size = 256; // 256 entries
    } else {                       // None
      pending_ctx_.act_min = -128;
      pending_ctx_.act_max = 127;
    }

    if (instr->has_skip) {
      uart_puts("[NPUV4Runtime] WARNING: Residual skip-add requested but not "
                "fully implemented.\r\n");
    }

    act_phys_ = static_cast<uint32_t>(CDC_VPU_OUT0_BASE);
    wgt_phys_ = static_cast<uint32_t>(CDC_NPU_WGT0_BASE + weight_offset_raw_);
    scratch_phys_ = static_cast<uint32_t>(CDC_NPU_WORK0_BASE + 0x40000UL);
    native_kh_ = instr->kernel_h;
    native_kw_ = instr->kernel_w;
    native_stride_ = (instr->stride_w > 0) ? instr->stride_w : 1;
    native_dil_ = (instr->dilation_w > 0) ? instr->dilation_w : 1;
    native_cin_ = instr->channels_in;

    uint32_t out_h = (im2col_oh_ > 0) ? im2col_oh_ : ((instr->input_h + 2 * instr->pad_h - instr->kernel_h) / (instr->stride_h > 0 ? instr->stride_h : 1) + 1);
    uint32_t out_w = (im2col_ow_ > 0) ? im2col_ow_ : ((instr->input_w + 2 * instr->pad_w - instr->kernel_w) / (instr->stride_w > 0 ? instr->stride_w : 1) + 1);
    native_k_exact_   = instr->kernel_h * instr->kernel_w * instr->channels_in;
    native_oh_        = out_h;
    native_ow_        = out_w;
    native_pad_h_     = instr->pad_h;
    native_pad_w_     = instr->pad_w;
    native_oc_tile_start_ = (obp_bias_base_ && instr->bias_offset >= obp_bias_base_)
        ? (uint32_t)((instr->bias_offset - obp_bias_base_) / sizeof(int32_t)) : 0u;
    native_oc_total_ = (instr->channels_out < layer_oc)
                           ? native_oc_tile_start_ + instr->channels_out   // per-channel job
                           : instr->channels_out;                          // whole-layer job

    if (!obp_lut_from_compiler_) {
      obp_cfg_ = 0;
      if (pending_ctx_.has_bias) obp_cfg_ |= FX1_OBP_CFG_BIAS_EN;
      if (pending_ctx_.has_requantization) obp_cfg_ |= FX1_OBP_CFG_REQUANT_EN;
      if (instr->act_mode != 0 || pending_ctx_.output_zero_point != 0) {
        obp_cfg_ |= FX1_OBP_CFG_LUT_EN;
      }
    }
    // The OBP hardware computes idx = clamp(requant(acc), -128, 127) + 128 and
    // emits (int8)LUT[idx] (verified by disassembling Obp::pipeline_process:
    // the int8 clamp at 0x509d93 happens BEFORE the index add at 0x509df0).
    // Because that clamp precedes the table, the table cannot shift a value by
    // the output zero point -- a zp of -128 would need the pre-zp value to span
    // [0,255], which the clamp has already destroyed. The zero point is therefore
    // folded into the bias (buildAdjustedBias) and this table is now nothing but
    // the activation clamp, expressed in the FINAL output domain.
    for (uint32_t l = 0; l < FX1_OBP_LANES; ++l) {
      for (uint32_t i = 0; i < FX1_OBP_LUT_BYTES_PER_LANE; ++i) {
        int32_t y = static_cast<int32_t>(i) - 128;   // undo the hardware's index bias
        if ((instr->act_mode == 2 || instr->act_mode == 3) && pending_ctx_.lut_data) {
          // compiler-baked tables are indexed by the final quantized conv output q
          // (zero point folded into the bias).
          y = static_cast<int8_t>(pending_ctx_.lut_data[i]);
        } else {
          if (y < pending_ctx_.act_min) y = pending_ctx_.act_min;
          if (y > pending_ctx_.act_max) y = pending_ctx_.act_max;
        }
        if (y < -128) y = -128;
        if (y >  127) y =  127;
        obp_lut_[l * FX1_OBP_LUT_BYTES_PER_LANE + i] = static_cast<uint8_t>(static_cast<int8_t>(y));
      }
    }
    native_is_fc_      = false;
    native_out_stride_ = FX1_SA_TILE;    // tile-local scratch; DMA_WRITE scatters to NHWC or NCHW (native_nchw_)
    native_out_ptr_    = nullptr;
  } else if (op == NeonOpcode::FC && size >= sizeof(NeonFCInstr)) {
    const auto *instr = static_cast<const NeonFCInstr *>(instr_bytes);
    native_nchw_ = false;

    current_k_ = (instr->input_size + FX1_SA_TILE - 1u) / FX1_SA_TILE * FX1_SA_TILE;
    if (current_k_ == 0 || (current_k_ * FX1_SA_TILE) > FX1_SRAMB_BANK_BYTES) {
      uart_put_i32("[NPUV4Runtime] ERROR: FC invalid K-dim=", current_k_, "\r\n");
      return NPUDriverStatus::ERROR_INVALID_ADDR;
    }
    pending_ctx_.activation_stride = current_k_;

    // The compiler emits NATIVE_CONFIG + OBP_CONFIG before the FC tiles, so
    // obp_bias_base_, native_oh_/ow_, native_oc_total_ and native_k_exact_ are
    // already set. Refuse to run otherwise rather than silently producing zeros.
    if (!has_native_config_ || native_ow_ == 0 || native_oc_total_ == 0) {
      uart_puts("[NPUV4Runtime] ERROR: FC without a preceding NATIVE_CONFIG\r\n");
      return NPUDriverStatus::ERROR_INVALID_ADDR;
    }

    const NPUDriverStatus stage_status = ensureWeightsStaged("FC");
    if (stage_status != NPUDriverStatus::OK) return stage_status;

    bias_offset_raw_   = instr->bias_offset;
    weight_offset_raw_ = instr->weight_offset;

    uint8_t *base = reinterpret_cast<uint8_t *>(CDC_NPU_WGT0_BASE);
    const uint32_t oc_padded =
        (native_oc_total_ + FX1_SA_TILE - 1u) / FX1_SA_TILE * FX1_SA_TILE;
    const uint32_t oc_start =
        (obp_bias_base_ && instr->bias_offset >= obp_bias_base_)
            ? static_cast<uint32_t>((instr->bias_offset - obp_bias_base_) / sizeof(int32_t))
            : 0u;

    pending_ctx_.has_bias = (instr->bias_offset != 0);
    pending_ctx_.has_requantization = true;
    pending_ctx_.bias_ptr = reinterpret_cast<const int32_t *>(base + instr->bias_offset);
    pending_ctx_.bias_size_bytes = instr->output_size * sizeof(int32_t);

    const int32_t *multiplier_base = reinterpret_cast<const int32_t *>(
        base + obp_bias_base_ + oc_padded * sizeof(int32_t));
    pending_ctx_.multiplier_ptr = multiplier_base + oc_start;
    pending_ctx_.multiplier_size_bytes = instr->output_size * sizeof(int32_t);
    pending_ctx_.shift_ptr = multiplier_base + oc_padded + oc_start;
    pending_ctx_.shift_size_bytes = instr->output_size * sizeof(int32_t);

    pending_ctx_.act_mode_raw = instr->act_mode;
    // Same correction as the CONV2D path (:415-421): the quantized representation
    // of real 0.0 is the output zero point, not literal 0. FC op 12 in
    // mobilenet_nano has zp = +37, so the old floor of 0 was wrong here too.
    pending_ctx_.act_min = (instr->act_mode == 1) ? pending_ctx_.output_zero_point : -128;
    pending_ctx_.act_max = 127;
    pending_ctx_.input_zero_point = 0;            // folded into bias by pass 1
    pending_ctx_.output_zero_point = instr->output_zero_point;

    // Native dispatch geometry: GEMM as 1x1 conv over 1 x M x K.
    native_is_fc_         = true;
    native_kh_ = native_kw_ = 1;
    native_stride_ = native_dil_ = 1;
    native_pad_h_ = native_pad_w_ = 0;
    native_cin_           = instr->input_size;
    native_k_exact_       = instr->input_size;
    native_oc_tile_start_ = oc_start;
    // Row tile: FCEmitter packs it as input_offset = m_start * K.
    native_tile_row_      = (instr->input_size > 0)
                          ? static_cast<uint32_t>(instr->input_offset / instr->input_size)
                          : 0u;
    im2col_enabled_       = true;                 // reuse the planar gather
    act_phys_             = static_cast<uint32_t>(CDC_VPU_OUT0_BASE);
    wgt_phys_             = static_cast<uint32_t>(CDC_NPU_WGT0_BASE + weight_offset_raw_);
    scratch_phys_         = static_cast<uint32_t>(CDC_NPU_WORK0_BASE + 0x40000UL);

    // Source rows: the fragment input tensor, row-major [M][K].
    native_img_ptr_ = reinterpret_cast<const int8_t *>(CDC_RAW_IN0_BASE);
    if (current_model_ && input_idx < current_model_->tensors.size() &&
        current_model_->tensors[input_idx].data) {
      native_img_ptr_ =
          static_cast<const int8_t *>(current_model_->tensors[input_idx].data);
    }
    // Destination: the fragment output tensor, row-major [M][N]. FCEmitter packs
    // output_offset = m_start * N + oc_start, i.e. the element index of this
    // tile's top-left corner.
    int8_t *out_base = reinterpret_cast<int8_t *>(CDC_ISP_OUT0_BASE);
    if (current_model_ && output_idx < current_model_->tensors.size() &&
        current_model_->tensors[output_idx].data) {
      out_base = static_cast<int8_t *>(current_model_->tensors[output_idx].data);
    }
    native_out_ptr_    = out_base + instr->output_offset;
    native_out_stride_ = native_oc_total_;      // = N, the full output row pitch

    native_act_mode_ = static_cast<uint8_t>(instr->act_mode);
    obp_cfg_ = 0;
    if (pending_ctx_.has_bias) obp_cfg_ |= FX1_OBP_CFG_BIAS_EN;
    if (pending_ctx_.has_requantization) obp_cfg_ |= FX1_OBP_CFG_REQUANT_EN;
    if (pending_ctx_.output_zero_point != 0 || instr->act_mode != 0)
      obp_cfg_ |= FX1_OBP_CFG_LUT_EN;

    for (uint32_t l = 0; l < FX1_OBP_LANES; ++l) {
      for (uint32_t i = 0; i < FX1_OBP_LUT_BYTES_PER_LANE; ++i) {
        int32_t y = static_cast<int32_t>(i) - 128;
        if (y < pending_ctx_.act_min) y = pending_ctx_.act_min;
        if (y > pending_ctx_.act_max) y = pending_ctx_.act_max;
        if (y < -128) y = -128;
        if (y >  127) y =  127;
        obp_lut_[l * FX1_OBP_LUT_BYTES_PER_LANE + i] =
            static_cast<uint8_t>(static_cast<int8_t>(y));
      }
    }
  } else if (op == NeonOpcode::ADD && size >= sizeof(NeonElemwiseInstr)) {
    const auto *instr = static_cast<const NeonElemwiseInstr *>(instr_bytes);

    // The fused-requant block (core/NPU_ISA.h, NEON_ADD_FUSED_PARAMS_*) lives in
    // the weight buffer at param_offset; its magic is checked at execution time.
    uint8_t *base = (current_model_ && current_model_->weight_buffer)
                        ? static_cast<uint8_t *>(current_model_->weight_buffer)
                        : reinterpret_cast<uint8_t *>(CDC_NPU_WGT0_BASE);
    pending_ctx_.add_params_ptr =
        reinterpret_cast<const int32_t *>(base + instr->param_offset);
    pending_ctx_.has_requantization = true;
    pending_ctx_.has_bias = false;
    pending_ctx_.output_offset = instr->output_offset;
    output_offset_ = instr->output_offset;
    add_num_elements_ = instr->size;
    add_input_b_offset_ = instr->input_b_offset;
    add_b_is_scalar_ = (instr->b_is_scalar != 0u);
  } else if (op == NeonOpcode::MUL && size >= sizeof(NeonElemwiseInstr)) {
    const auto *instr = static_cast<const NeonElemwiseInstr *>(instr_bytes);

    uint8_t *base = (current_model_ && current_model_->weight_buffer)
                        ? static_cast<uint8_t *>(current_model_->weight_buffer)
                        : reinterpret_cast<uint8_t *>(CDC_NPU_WGT0_BASE);
    pending_ctx_.mul_params_ptr =
        reinterpret_cast<const int32_t *>(base + instr->param_offset);
    pending_ctx_.has_requantization = true;
    pending_ctx_.has_bias = false;
    pending_ctx_.output_offset = instr->output_offset;
    output_offset_ = instr->output_offset;
    mul_num_elements_ = instr->size;
    mul_input_b_offset_ = instr->input_b_offset;
    mul_b_is_scalar_ = (instr->b_is_scalar != 0u);
  } else if (op == NeonOpcode::SUB && size >= sizeof(NeonElemwiseInstr)) {
    const auto *instr = static_cast<const NeonElemwiseInstr *>(instr_bytes);

    uint8_t *base = (current_model_ && current_model_->weight_buffer)
                        ? static_cast<uint8_t *>(current_model_->weight_buffer)
                        : reinterpret_cast<uint8_t *>(CDC_NPU_WGT0_BASE);
    pending_ctx_.sub_params_ptr =
        reinterpret_cast<const int32_t *>(base + instr->param_offset);
    pending_ctx_.has_requantization = true;
    pending_ctx_.has_bias = false;
    pending_ctx_.output_offset = instr->output_offset;
    output_offset_ = instr->output_offset;
    sub_num_elements_ = instr->size;
    sub_input_b_offset_ = instr->input_b_offset;
    sub_b_is_scalar_ = (instr->b_is_scalar != 0u);
  } else if (op == NeonOpcode::SUM && size >= sizeof(NeonSumInstr)) {
    const auto *instr = static_cast<const NeonSumInstr *>(instr_bytes);
    sum_k_ = instr->k;
    sum_n_ = instr->n;
    sum_param_offset_ = instr->param_offset;
    sum_output_zero_point_ = instr->output_zero_point;
    sum_input_idx_ = input_idx;
    sum_output_idx_ = output_idx;
  } else if (op == NeonOpcode::DMA_READ && size >= sizeof(NeonDMAInstr)) {
    const auto *instr = static_cast<const NeonDMAInstr *>(instr_bytes);
    if (instr->channel == 0) {
      
#if defined(NPU_DISPATCH_NATIVE)
      if (im2col_enabled_) {
        native_img_ptr_ = reinterpret_cast<const int8_t *>(instr->src_offset);
        native_tile_row_ = instr->tile_row;          // consumed by triggerNative()
      } else {
        void *src = reinterpret_cast<void *>(instr->src_offset);
        std::memcpy((void *)(uintptr_t)(CDC_VPU_OUT0_BASE + instr->dst_offset),
                    src, instr->size_bytes);
      }
#else
      if (im2col_enabled_) {
        int8_t *dst_ptr =
            reinterpret_cast<int8_t *>(CDC_VPU_OUT0_BASE + instr->dst_offset);
        const int8_t *raw_img =
            reinterpret_cast<const int8_t *>(instr->src_offset);

        uint32_t K = im2col_kh_ * im2col_kw_ * im2col_ic_;
        // Row width of the im2col matrix, 32-aligned to match the compiler's
        // padding (Im2ColConvToGemmPass). This is 32 only when KH*KW*CI <= 32
        // (a first-layer conv); anything wider needs the full K per row, and
        // the whole K is fed to the SA in a single GEMM job.
        const uint32_t K_padded = (K + FX1_SA_TILE - 1u) / FX1_SA_TILE * FX1_SA_TILE;
        constexpr uint32_t num_rows = FX1_SA_TILE; // SA tile spatial height (32)

        // tile_row encodes global spatial start index in Im2Col matrix (ho*ow +
        // wo). Each spatial position maps to one row in the Im2Col output.
        uint32_t start_spatial = instr->tile_row;

        uint8_t izp = static_cast<uint8_t>(
            im2col_izp_ != 0 ? im2col_izp_ : pending_ctx_.input_zero_point);

        uint32_t total_spatial = im2col_oh_ * im2col_ow_;
        for (uint32_t r_idx = 0; r_idx < num_rows; ++r_idx) {
          uint32_t spatial_idx = start_spatial + r_idx;

          if (spatial_idx >= total_spatial) {
            for (uint32_t d = 0; d < K_padded; ++d) {
              dst_ptr[r_idx * K_padded + d] = static_cast<int8_t>(izp);
            }
            continue;
          }

          int ho =
              (im2col_ow_ > 0) ? static_cast<int>(spatial_idx / im2col_ow_) : 0;
          int wo =
              (im2col_ow_ > 0) ? static_cast<int>(spatial_idx % im2col_ow_) : 0;

          for (uint32_t d = 0; d < K_padded; ++d) {
            if (d < K) {
              int ic = 0, kh = 0, kw = 0;
              if (native_nchw_) {
                uint32_t khkw = im2col_kh_ * im2col_kw_;
                ic = (khkw > 0) ? static_cast<int>(d / khkw) : 0;
                int r = (khkw > 0) ? static_cast<int>(d % khkw) : 0;
                kh = (im2col_kw_ > 0) ? static_cast<int>(r / im2col_kw_) : 0;
                kw = (im2col_kw_ > 0) ? static_cast<int>(r % im2col_kw_) : 0;
              } else {
                kh = static_cast<int>(d / (im2col_kw_ * im2col_ic_));
                int kw_ic = static_cast<int>(d % (im2col_kw_ * im2col_ic_));
                kw = static_cast<int>(kw_ic / im2col_ic_);
                ic = static_cast<int>(kw_ic % im2col_ic_);
              }

              int ih = ho * static_cast<int>(im2col_stride_h_) + kh -
                       static_cast<int>(im2col_pad_h_);
              int iw = wo * static_cast<int>(im2col_stride_w_) + kw -
                       static_cast<int>(im2col_pad_w_);

              if (ih >= 0 && ih < static_cast<int>(im2col_ih_) && iw >= 0 &&
                  iw < static_cast<int>(im2col_iw_)) {
                size_t img_offset = native_nchw_
                    ? ((static_cast<size_t>(ic) * im2col_ih_ + ih) * im2col_iw_ + iw)
                    : ((static_cast<size_t>(ih) * im2col_iw_ + iw) * im2col_ic_ + ic);
                dst_ptr[r_idx * K_padded + d] = raw_img[img_offset];
              } else {
                dst_ptr[r_idx * K_padded + d] = static_cast<int8_t>(izp);
              }
            } else {
              dst_ptr[r_idx * K_padded + d] = static_cast<int8_t>(izp);
            }
          }
        }
      } else {
        void *src = reinterpret_cast<void *>(instr->src_offset);
        std::memcpy((void *)(uintptr_t)(CDC_VPU_OUT0_BASE + instr->dst_offset),
                    src, instr->size_bytes);
      }
#endif
    } else if (instr->channel == 1) {
      const bool is_full_weight_buffer =
          current_model_ && current_model_->weight_buffer &&
          reinterpret_cast<uintptr_t>(current_model_->weight_buffer) ==
              static_cast<uintptr_t>(instr->src_offset) &&
          current_model_->weight_buffer_size == instr->size_bytes;
      if (is_full_weight_buffer) {
        const NPUDriverStatus stage_status = ensureWeightsStaged("DMA_READ.ch1");
        if (stage_status != NPUDriverStatus::OK) return stage_status;
      } else {
        void *src = reinterpret_cast<void *>(instr->src_offset);
        std::memcpy((void *)(uintptr_t)CDC_NPU_WGT0_BASE, src,
                    instr->size_bytes);
        // A partial or unrelated transfer changes WGT0 contents, so the
        // current fragment's full-buffer residency can no longer be trusted.
        weights_staged_ = false;
        staged_weights_source_ = nullptr;
        staged_weights_size_ = 0;
        if (current_model_) {
          current_model_->weights_on_device = false;
          if (!current_model_->weights.empty())
            current_model_->weights[0].on_device = false;
        }
      }
    }
  } else if (op == NeonOpcode::DMA_WRITE && size >= sizeof(NeonDMAInstr)) {
    const auto *instr = static_cast<const NeonDMAInstr *>(instr_bytes);
    if (instr->channel == 2) {
      const uint8_t *src = reinterpret_cast<const uint8_t *>(
          CDC_NPU_WORK0_BASE + output_offset_);

      // In segment mode, main.cpp patches dst_offset by adding the fragment's output
      // tensor address (dram_output). In single-fragment mode, dram_output == CDC_ISP_OUT0_BASE.
      // We resolve out_base from current_model_'s output tensor, and strip out_base
      // rather than assuming CDC_ISP_OUT0_BASE.
      uint8_t *out_base = (current_model_ && current_model_->output_tensor_idx < current_model_->tensors.size() && current_model_->tensors[current_model_->output_tensor_idx].data)
          ? static_cast<uint8_t *>(current_model_->tensors[current_model_->output_tensor_idx].data)
          : reinterpret_cast<uint8_t *>(CDC_ISP_OUT0_BASE);

      uint32_t dst_offset_32 = static_cast<uint32_t>(instr->dst_offset);
      uint32_t out_base_u32 = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(out_base));
      uint32_t mlir_offset_int32 = (dst_offset_32 >= out_base_u32)
          ? (dst_offset_32 - out_base_u32)
          : ((dst_offset_32 >= CDC_ISP_OUT0_BASE) ? (dst_offset_32 - CDC_ISP_OUT0_BASE) : dst_offset_32);
      uint32_t int8_offset = mlir_offset_int32 / 4u;
      uint8_t *dst = out_base + int8_offset;

      // Fix #2b: compute ch_start and ch_count from the int8 element index.
      // element_idx = (spatial_start * channels_out + ch_start)
      // ch_start is the first channel of this OC tile (0, 32, 64, ...) and
      // ch_count is how many of the tile's 32 columns are real channels.
      // channels_out comes from the layer's NATIVE_CONFIG rather than being
      // fixed at 48, which only matched the first-layer model.
      const uint32_t channels_out = full_channels_out_;
      if (channels_out == 0) {
        uart_puts("[NPUV4Runtime] ERROR: DMA_WRITE before a valid "
                  "NATIVE_CONFIG (channels_out=0)\r\n");
        return NPUDriverStatus::ERROR_INVALID_ADDR;
      }
      uint32_t element_idx = static_cast<uint32_t>(int8_offset);
      uint32_t ch_start = element_idx % channels_out; // 0, 32, ...
      uint32_t ch_count = (instr->size_bytes == 1)
                              ? 1u
                              : ((FX1_SA_TILE < channels_out - ch_start)
                                     ? FX1_SA_TILE
                                     : (channels_out - ch_start));

      uint32_t spatial_start = element_idx / channels_out;
      uint32_t total_spatial = (im2col_oh_ > 0 && im2col_ow_ > 0)
                                   ? (im2col_oh_ * im2col_ow_)
                                   : 0u;
      uint32_t rows_to_copy = (total_spatial > 0 && spatial_start < total_spatial)
                                  ? std::min<uint32_t>(FX1_SA_TILE, total_spatial - spatial_start)
                                  : FX1_SA_TILE;

      if (spatial_start == 0) {
        uart_puts("[NPUV4Runtime] DEBUG DMA_WRITE tile 0:\r\n");
        uart_put_u32("  int8_offset=", int8_offset, "\r\n");
        uart_put_u32("  ch_start=", ch_start, "");
        uart_put_u32("  ch_count=", ch_count, "\r\n");
        uart_put_u32("  spatial_start=", spatial_start, "");
        uart_put_u32("  rows_to_copy=", rows_to_copy, "\r\n");
        uart_puts("  src[0..7]: ");
        for (int k = 0; k < 8; ++k) uart_put_i32("", static_cast<int32_t>(src[k]), " ");
        uart_puts("\r\n");
      }

      if (native_nchw_) {
        // Output tensor is [OC][H*W]; element_idx still encodes the logical
        // (spatial_start, ch_start) the compiler emitted.
        const uint32_t hw = im2col_oh_ * im2col_ow_;
        for (uint32_t ch = 0; ch < ch_count; ++ch) {
          uint8_t *plane = out_base + (size_t)(ch_start + ch) * hw + spatial_start;
          for (uint32_t r = 0; r < rows_to_copy; ++r)
            plane[r] = src[r * FX1_SA_TILE + ch];
        }
      } else {
        for (uint32_t r = 0; r < rows_to_copy; ++r) {
          // src layout: [32 spatial rows × 32 channels] int8 (GEMM tile, post-requant)
          // dst layout: NHWC [..., channels_out] int8
          std::memcpy(dst + r * channels_out, src + r * FX1_SA_TILE, ch_count);
        }
      }
    }
  } else if (op == NeonOpcode::NATIVE_CONFIG &&
             size >= offsetof(NeonNativeConfigInstr, act_layout)) {
    const auto *instr = static_cast<const NeonNativeConfigInstr *>(instr_bytes);
    native_nchw_ = (size > offsetof(NeonNativeConfigInstr, act_layout)) &&
                   instr->act_layout == NEON_LAYOUT_NCHW;
    if (instr->output_h == 0 || instr->output_w == 0) {
      uart_puts(
          "[NPUV4Runtime] ERROR: NATIVE_CONFIG has invalid output shape\r\n");
      return NPUDriverStatus::ERROR_INVALID_ADDR;
    }
    // NOTE: this used to program the NATIVE V1_SAURIA register interface
    // (CDC_NPU0_BASE + kRegLayerIn*/Out*/kRegCfgProfile) directly. That is a
    // separate MMIO path from the CDC Compatibility GEMM Bank that
    // npu_v4_gemm_fused() actually uses to run the computation (see
    // NPU_V4.2_INTEGRATION.md: "The compatibility bank is a CDC-VP
    // extension. It is not part of the native V4.2 register map."). Writing
    // kRegCfgProfile there selects a native execution profile on the shared
    // core that this driver no longer uses. Only the software-side state
    // below (im2col_*, output_offset_, output_zero_point_) is needed.

    if (instr->input_h == 0 || instr->input_w == 0 || instr->channels_in == 0 ||
        instr->kernel_h == 0 || instr->kernel_w == 0 || instr->stride_h == 0 ||
        instr->stride_w == 0) {
      uart_puts("[NPUV4Runtime] ERROR: NATIVE_CONFIG has invalid geometry\r\n");
      return NPUDriverStatus::ERROR_INVALID_ADDR;
    }
    im2col_ih_ = instr->input_h;
    im2col_iw_ = instr->input_w;
    im2col_ic_ = instr->channels_in;
    im2col_kh_ = instr->kernel_h;
    im2col_kw_ = instr->kernel_w;
    im2col_stride_h_ = instr->stride_h;
    im2col_stride_w_ = instr->stride_w;
    im2col_pad_h_ = instr->pad_h;
    im2col_pad_w_ = instr->pad_w;
    im2col_oh_ = instr->output_h;
    im2col_ow_ = instr->output_w;
    im2col_tile_counter_ = 0;
    im2col_enabled_ = true;
    im2col_izp_ = static_cast<int32_t>(instr->input_offset);

    native_kh_ = instr->kernel_h;
    native_kw_ = instr->kernel_w;
    native_stride_ = (instr->stride_w > 0) ? instr->stride_w : 1;
    native_dil_ = (instr->dilation_w > 0) ? instr->dilation_w : 1;
    native_cin_ = instr->channels_in;
    native_pad_h_ = instr->pad_h;
    native_pad_w_ = instr->pad_w;
    native_k_exact_ = instr->kernel_h * instr->kernel_w * instr->channels_in;
    native_oc_total_ = instr->channels_out;
    native_oh_ = instr->output_h;
    native_ow_ = instr->output_w;

    pending_ctx_.output_offset = instr->output_offset;
    configured_output_zero_point_ = instr->output_zero_point;
    has_native_config_ = true;
    // NATIVE_CONFIG's bias_offset is the layer's un-adjusted (OC-tile 0)
    // bias-section base — anchor for recovering out_channel_idx and the
    // multiplier/shift/LUT section offsets on each subsequent CONV2D.
    obp_bias_base_ = instr->bias_offset;
    if (configured_output_zero_point_ != 0)
      pending_ctx_.output_zero_point = configured_output_zero_point_;
    output_offset_ = instr->output_offset;
    output_zero_point_ = configured_output_zero_point_;
    full_channels_out_ =
        instr->channels_out; // Store the actual full channels out

    native_is_fc_      = false;      // an FC instruction sets this on arrival
    native_out_ptr_    = nullptr;
    native_out_stride_ = FX1_SA_TILE;

  } else {
    current_k_ = FX1_SA_TILE;
  }

  // If we don't have per-channel arrays from weight buffer (e.g. op without
  // weights), fallback to scalar requantization from output tensor (if needed).
  if (!pending_ctx_.has_requantization) {
    // Not supported in this simplified structure, but zero points are still
    // written.
    pending_ctx_.has_requantization = true; // Needs at least zero points
  }

  return NPUDriverStatus::OK;
}

NPUDriverStatus NPUV4Runtime::triggerRich() {
  ++rich_jobs_;
  npu_rich_program(&pending_rich_instr_);
  npu_rich_push(static_cast<uint8_t>(last_op_), pending_rich_instr_.lane);
  int ok = npu_rich_wait_done(400000);
  if (ok <= 0) {
    if (ok < 0) {
      uart_put_i32("[NPUV4Runtime] ERROR: npu_rich_wait_done error=", -ok, "\r\n");
    } else {
      uart_puts("[NPUV4Runtime] ERROR: npu_rich_wait_done timeout!\r\n");
    }
    return NPUDriverStatus::ERROR_DEVICE_BUSY;
  }
  return NPUDriverStatus::OK;
}

// Builds a per-OC bias with the output zero point AND the rounding term (2^(S-1))
// folded in, so the value that reaches the OBP's pre-LUT int8 clamp is already
// the final, round-to-nearest quantized output q.
//
//   hardware:  esi = ((acc + bias') * mult) >> S
//   want:      esi = round(requant(acc)) + zp = floor(((acc + bias) * mult + zp * 2^S + 2^(S-1)) / 2^S)
//   =>         delta_c * mult_c = zp * 2^S + 2^(S-1)
//   =>         delta_c = round((zp * 2^S + 2^(S-1)) / mult_c)
//
// Returns the array to hand to npu_native_program_obp(), or the untouched
// pointer when folding does not apply.
const int32_t *NPUV4Runtime::buildAdjustedBias(uint32_t lanes) {
  const int32_t zp = pending_ctx_.output_zero_point;          // conv-output zp (NATIVE_CONFIG / CONV2D)
  if (!pending_ctx_.bias_ptr || !pending_ctx_.multiplier_ptr ||
      !pending_ctx_.shift_ptr || !pending_ctx_.has_requantization) {
    return pending_ctx_.bias_ptr;
  }
  // All activation modes: the OBP truncates ((x*M) >> S) and clamps to int8 BEFORE
  // the LUT. Folding zp*2^S + 2^(S-1) into the bias makes the clamped value the final,
  // round-to-nearest quantized conv output q, so every LUT (ReLU clamp table or the
  // compiler-baked SiLU table) is indexed by q itself.
  if (lanes > FX1_OBP_LANES) lanes = FX1_OBP_LANES;

  for (uint32_t l = 0; l < lanes; ++l) {
    const int32_t mult = pending_ctx_.multiplier_ptr[l];
    const int32_t sh   = pending_ctx_.shift_ptr[l];
    const uint32_t S   = (sh < 0) ? (uint32_t)(-sh) : (uint32_t)sh;   // driver convention
    int64_t delta = 0;
    if (mult != 0 && S >= 1u && S < 62u) {
      const int64_t num = ((int64_t)zp << S) + ((int64_t)1 << (S - 1));   // zp + 0.5 LSB
      const int64_t den = (int64_t)mult;
      delta = ((num >= 0) == (den > 0)) ? (num + den / 2) / den
                                        : (num - den / 2) / den;
    }
    const int64_t adj = (int64_t)pending_ctx_.bias_ptr[l] + delta;
    if (adj > INT32_MAX || adj < INT32_MIN) {
      uart_put_i32("[NPUV4Runtime] ERROR: bias fold overflows int32 at lane ", (int32_t)l, "\r\n");
      return pending_ctx_.bias_ptr;
    }
    obp_bias_adj_[l] = (int32_t)adj;
  }
  return obp_bias_adj_;
}

static_assert(static_cast<uint32_t>(NPUV4Runtime::kPerfCount)    == static_cast<uint32_t>(NPU_PERF_COUNT) &&
              static_cast<uint32_t>(NPUV4Runtime::kPerfTotal)    == static_cast<uint32_t>(NPU_PERF_TOTAL) &&
              static_cast<uint32_t>(NPUV4Runtime::kPerfRichM)    == static_cast<uint32_t>(NPU_PERF_RICH_M) &&
              static_cast<uint32_t>(NPUV4Runtime::kPerfDmaWrCyc) == static_cast<uint32_t>(NPU_PERF_DMA_WR_CYC),
              "PerfIdx must match NPU_PERF_* in bsp_ext/npu_v4_native_driver.h");

void NPUV4Runtime::snapPerf(PerfAccum& out) const {
  npu_perf_acc_snapshot(out.v, &out.hw_jobs, &out.stage_jobs);
  out.native_jobs = native_jobs_;
  out.rich_jobs   = rich_jobs_;
}

NPUDriverStatus NPUV4Runtime::triggerNative() {
  npu_native_soft_reset();
  ++native_jobs_;

  const uint32_t spatial_total = native_oh_ * native_ow_;
  const uint32_t spatial_start = native_tile_row_;                  // from DMA_READ (or FC tile)
  const uint32_t spatial_left  = (spatial_start < spatial_total)
                               ? (spatial_total - spatial_start) : 0u;
  const uint32_t oc_start      = native_oc_tile_start_;             // from bias_offset
  const uint32_t oc_left       = (oc_start < native_oc_total_)
                               ? (native_oc_total_ - oc_start) : 0u;

  if (spatial_left == 0u || oc_left == 0u) {
    uart_puts("[NPUV4Runtime] ERROR: native job has an empty tile "
              "(missing NATIVE_CONFIG or out-of-range tile index)\r\n");
    return NPUDriverStatus::ERROR_INVALID_ADDR;
  }

  // One native job covers at most FX1_SA_TILE spatial rows and FX1_SA_TILE OCs.
  const uint32_t w_til_total = (spatial_left < FX1_SA_TILE) ? spatial_left : FX1_SA_TILE;
  const uint32_t k_til       = (oc_left      < FX1_SA_X)    ? oc_left      : FX1_SA_X;
  // OpParamSerializer emits each OC tile as [K_padded][T=32] with the tail rows
  // ZERO-FILLED, where K_padded = ceil(K/32)*32. Staging only K_exact rows made
  // the staged row count (= the operand's k) a non-multiple of 32 -- e.g. 27 for
  // this model's 3x3 conv and 16 for its 1x1 convs -- so neither SRAM A's nor
  // SRAM B's rows landed on 32-byte word boundaries. Staging the padded row
  // count costs nothing (the zeros are already in DRAM) and the core still walks
  // only K_exact rows (npu_native_program_core's k_dim below is unchanged), so
  // the extra zero rows are loaded but never read.
  const uint32_t k_padded    = ((native_k_exact_ + FX1_SA_X - 1u) / FX1_SA_X) * FX1_SA_X;
  const uint32_t wgt_bytes   = k_padded * FX1_SA_X;
  if (wgt_bytes > FX1_SRAMB_BANK_BYTES) {
    uart_puts("[NPUV4Runtime] ERROR: wgt_bytes exceeds the 81 KiB SRAM B bank\r\n");
    return NPUDriverStatus::ERROR_UNSUPPORTED_OP;
  }

  const uint32_t Bw_eff = 1u + (native_kw_ - 1u) * native_dil_;
  const uint32_t Bh_eff = 1u + (native_kh_ - 1u) * native_dil_;
  const uint32_t A_h_sub = Bh_eff;
  const uint8_t izp = static_cast<uint8_t>(
      im2col_izp_ != 0 ? im2col_izp_ : pending_ctx_.input_zero_point);

  for (uint32_t w_offset = 0; w_offset < w_til_total; w_offset += FX1_NATIVE_Y_USED) {
    uint32_t cur_w = (w_til_total - w_offset < FX1_NATIVE_Y_USED)
                   ? (w_til_total - w_offset) : FX1_NATIVE_Y_USED;
    // The staged patch is one horizontal strip of a single output row (oh0, ow0 .. ow0+cur_w-1).
    // It is only valid when the strip does not wrap to the next row; that holds iff OW % Y_USED == 0.
    if (!native_is_fc_ && native_ow_ != 0 &&
        ((spatial_start + w_offset) % native_ow_) + cur_w > native_ow_) {
      uart_put_u32("[NPUV4Runtime] ERROR: native strip crosses an output row, OW=", native_ow_, "\r\n");
      return NPUDriverStatus::ERROR_UNSUPPORTED_OP;
    }
    uint32_t A_w_sub = (cur_w - 1u) * native_stride_ + Bw_eff;
    // The staging op declares A as m x k with the SAME k as B (see
    // npu_native_stage_operands_sized), so act_bytes must be a whole number of
    // k_padded-byte rows or the staging falls back to the old gcd shape.
    const uint32_t act_raw_bytes = native_cin_ * A_h_sub * A_w_sub;
    uint32_t sub_act_bytes = ((act_raw_bytes + k_padded - 1u) / k_padded) * k_padded;
    if (sub_act_bytes > FX1_SRAMA_BANK_BYTES) {
      uart_puts("[NPUV4Runtime] ERROR: sub_act_bytes exceeds the 79 KiB SRAM A bank\r\n");
      return NPUDriverStatus::ERROR_UNSUPPORTED_OP;
    }

    int8_t *dst_act = reinterpret_cast<int8_t *>(CDC_VPU_OUT0_BASE);
    if (sub_act_bytes > act_raw_bytes) {
      for (uint32_t p = act_raw_bytes; p < sub_act_bytes; ++p)
        dst_act[p] = static_cast<int8_t>(izp);
    }
    if (native_img_ptr_ && im2col_enabled_) {
      uint32_t sub_spatial_start = spatial_start + w_offset;
      const int oh0 = (int)(sub_spatial_start / native_ow_);
      const int ow0 = (int)(sub_spatial_start % native_ow_);
      const int ih0 = oh0 * (int)native_stride_ - (int)native_pad_h_;
      const int iw0 = ow0 * (int)native_stride_ - (int)native_pad_w_;

      if (native_nchw_) {
        // Source is already C*H*W (NPU_V4.6_INTEGRATION.md, implicit im2col): every
        // (c, y) row of the staged patch is one contiguous run of an input plane.
        const size_t plane = (size_t)im2col_ih_ * im2col_iw_;
        const int x_lo = (iw0 < 0) ? -iw0 : 0;
        int x_hi = (int)im2col_iw_ - iw0;
        if (x_hi > (int)A_w_sub) x_hi = (int)A_w_sub;
        for (uint32_t c = 0; c < native_cin_; ++c) {
          const int8_t *src_c = native_img_ptr_ + (size_t)c * plane;
          for (uint32_t y = 0; y < A_h_sub; ++y) {
            int8_t *row = dst_act + ((size_t)c * A_h_sub + y) * A_w_sub;
            const int ih = ih0 + (int)y;
            if (ih < 0 || ih >= (int)im2col_ih_ || x_hi <= x_lo) {
              std::memset(row, (int)(int8_t)izp, A_w_sub);
              continue;
            }
            if (x_lo > 0) std::memset(row, (int)(int8_t)izp, (size_t)x_lo);
            std::memcpy(row + x_lo, src_c + (size_t)ih * im2col_iw_ + (iw0 + x_lo), (size_t)(x_hi - x_lo));
            if (x_hi < (int)A_w_sub) std::memset(row + x_hi, (int)(int8_t)izp, (size_t)(A_w_sub - x_hi));
          }
        }
      } else {
        for (uint32_t c = 0; c < native_cin_; ++c) {
          for (uint32_t y = 0; y < A_h_sub; ++y) {
            for (uint32_t x = 0; x < A_w_sub; ++x) {
              const int ih = ih0 + (int)y, iw = iw0 + (int)x;
              const bool inb = ih >= 0 && ih < (int)im2col_ih_ &&
                               iw >= 0 && iw < (int)im2col_iw_;
              dst_act[(c * A_h_sub + y) * A_w_sub + x] = inb
                  ? native_img_ptr_[((size_t)ih * im2col_iw_ + iw) * native_cin_ + c]
                  : (int8_t)izp;
            }
          }
        }
      }
    }

    // Phase 1: load SRAM A/B through the rich DMA
    if (npu_native_stage_operands_sized(act_phys_, sub_act_bytes,
                                        wgt_phys_, wgt_bytes,
                                        scratch_phys_) != 1) {
      uart_puts("[NPUV4Runtime] ERROR: npu_native_stage_operands_sized failed\r\n");
      return NPUDriverStatus::ERROR_DEVICE_BUSY;
    }

    // Phase 2: native run for this sub-chunk
    npu_layer_desc_t d = {};
    d.B_w = native_kw_;  d.B_h = native_kh_;
    d.d   = native_dil_; d.s   = native_stride_;
    d.c_til = native_cin_;
    d.k_til = k_til;
    d.w_til = cur_w;
    d.h_til = 1;
    d.X_used = d.k_til;
    d.Y_used = (d.w_til < FX1_NATIVE_Y_USED) ? d.w_til : FX1_NATIVE_Y_USED;
    d.preload_en = 0;

    npu_core_fields_t f;
    npu_compute_core_fields(&d, &f);

    npu_native_program_core(&d, &f, native_k_exact_);
    int32_t obp_bias_padded[FX1_OBP_LANES] = {0};
    int32_t obp_mult_padded[FX1_OBP_LANES] = {0};
    int32_t obp_shift_padded[FX1_OBP_LANES] = {0};

    const int32_t *dbg_adj_bias = buildAdjustedBias(d.k_til);
    for (uint32_t l = 0; l < d.k_til && l < FX1_OBP_LANES; ++l) {
      if (dbg_adj_bias) obp_bias_padded[l] = dbg_adj_bias[l];
      if (pending_ctx_.multiplier_ptr) obp_mult_padded[l] = pending_ctx_.multiplier_ptr[l];
      if (pending_ctx_.shift_ptr) obp_shift_padded[l] = pending_ctx_.shift_ptr[l];
    }
    npu_native_program_obp(obp_bias_padded,
                           obp_mult_padded,
                           obp_shift_padded,
                           obp_lut_from_compiler_ ? nullptr : obp_lut_,
                           FX1_OBP_LANES,
                           obp_cfg_ | kObpChannelModeBits);
    npu_native_start();

    int rc = npu_native_wait_done(400000);
    if (rc <= 0) {
      uart_put_i32("[NPUV4Runtime] native job failed rc=", rc, "\r\n");
      return NPUDriverStatus::ERROR_DEVICE_BUSY;
    }

    int8_t *out_base;
    uint32_t out_stride;
    if (native_is_fc_) {
      // Straight into the fragment output tensor: [M][N], this tile's columns.
      out_base   = native_out_ptr_ + (size_t)w_offset * native_out_stride_;
      out_stride = native_out_stride_;
    } else {
      out_base   = reinterpret_cast<int8_t *>(
                       CDC_NPU_WORK0_BASE + pending_ctx_.output_offset) +
                   (size_t)w_offset * FX1_SA_TILE;
      out_stride = FX1_SA_TILE;
    }
    
    npu_native_read_sramc(out_base, d.k_til, cur_w, out_stride);

  }

  return NPUDriverStatus::OK;
}

NPUDriverStatus NPUV4Runtime::triggerCpuFallback() {
  if (last_op_ == NeonOpcode::SUM) {
    uart_put_u32("[NPUV4Runtime] SUM (CPU fallback): K=", sum_k_, "");
    uart_put_u32(" N=", sum_n_, "\r\n");

    const int8_t *in_data = reinterpret_cast<const int8_t *>(CDC_RAW_IN0_BASE);
    if (current_model_ && sum_input_idx_ < current_model_->tensors.size() && current_model_->tensors[sum_input_idx_].data) {
      in_data = static_cast<const int8_t *>(current_model_->tensors[sum_input_idx_].data);
    }
    int8_t *out_data = reinterpret_cast<int8_t *>(CDC_ISP_OUT0_BASE);
    if (current_model_ && sum_output_idx_ < current_model_->tensors.size() && current_model_->tensors[sum_output_idx_].data) {
      out_data = static_cast<int8_t *>(current_model_->tensors[sum_output_idx_].data);
    }

    if (!current_model_ || !current_model_->weight_buffer) {
      uart_puts("[NPUV4Runtime] ERROR: SUM missing weight_buffer\r\n");
      return NPUDriverStatus::ERROR_INVALID_ADDR;
    }
    const int32_t *params = reinterpret_cast<const int32_t *>(
        static_cast<const uint8_t *>(current_model_->weight_buffer) + sum_param_offset_);
    int32_t bias = params[0];
    int64_t m0 = params[1];
    int32_t shift = params[2];
    int32_t z_out = params[3];

    if (shift != -30) {
      uart_puts("[NPUV4Runtime] ERROR: SUM requant_shift != -30 not supported\r\n");
      return NPUDriverStatus::ERROR_INVALID_ADDR;
    }

    int8_t *dst = out_data;
    for (uint32_t n = 0; n < sum_n_; ++n) {
      int32_t raw_sum = 0;
      for (uint32_t k = 0; k < sum_k_; ++k) {
        raw_sum += in_data[k * sum_n_ + n];
      }
      int64_t acc = static_cast<int64_t>(raw_sum) + bias;
      int64_t scaled = (acc * m0 + (1LL << 29)) >> 30;
      int64_t out_val = scaled + z_out;
      if (out_val < -128) out_val = -128;
      if (out_val > 127) out_val = 127;
      dst[n] = static_cast<int8_t>(out_val);
    }

    if (current_model_->output_tensor_idx < current_model_->tensors.size() &&
        current_model_->tensors[current_model_->output_tensor_idx].data) {
      std::memcpy(current_model_->tensors[current_model_->output_tensor_idx].data,
                  dst, sum_n_);
    }

    perf_cycles_ += sum_k_ * sum_n_;
    uart_puts("[NPUV4Runtime] SUM execution completed successfully\r\n");
    return NPUDriverStatus::OK;
  }

  if (last_op_ == NeonOpcode::ADD) {
    size_t num_elements = add_num_elements_;
    if (num_elements == 0 && current_model_ &&
        current_model_->output_tensor_idx < current_model_->tensors.size()) {
      num_elements =
          current_model_->tensors[current_model_->output_tensor_idx].size_bytes;
    }
    if (num_elements == 0) {
      uart_puts("[NPUV4Runtime] ADD: output tensor size=0, skipping\r\n");
      return NPUDriverStatus::OK;
    }

    const int8_t *input_a = (current_model_ && current_model_->input_tensor_idx < current_model_->tensors.size() && current_model_->tensors[current_model_->input_tensor_idx].data)
        ? static_cast<const int8_t *>(current_model_->tensors[current_model_->input_tensor_idx].data)
        : reinterpret_cast<const int8_t *>(CDC_RAW_IN0_BASE);
    const int32_t *params_ptr = pending_ctx_.add_params_ptr;
    int8_t *output_ptr = (current_model_ && current_model_->output_tensor_idx < current_model_->tensors.size() && current_model_->tensors[current_model_->output_tensor_idx].data)
        ? static_cast<int8_t *>(current_model_->tensors[current_model_->output_tensor_idx].data)
        : reinterpret_cast<int8_t *>(CDC_ISP_OUT0_BASE);

    // A stream compiled before the fused layout carries the 10-int32 block,
    // whose word 6 is z_a, never the magic.
    if (!params_ptr ||
        static_cast<uint32_t>(params_ptr[6]) != NEON_ADD_FUSED_PARAMS_MAGIC) {
      uart_puts("[NPUV4Runtime] ADD: param block is not the fused-requant layout "
                "(recompile the .nsm)\r\n");
      return NPUDriverStatus::ERROR_UNSUPPORTED_OP;
    }

    uart_put_u32("[NPUV4Runtime] ADD CPU fallback, num_elements=",
                 static_cast<uint32_t>(num_elements), "\r\n");
    const uintptr_t weight_base = current_model_ && current_model_->weight_buffer
        ? reinterpret_cast<uintptr_t>(current_model_->weight_buffer) : 0;
    const uintptr_t params_addr =
        reinterpret_cast<uintptr_t>(pending_ctx_.add_params_ptr);
    uart_put_u32("  -> params_offset=",
                 static_cast<uint32_t>(weight_base && params_addr >= weight_base
                     ? params_addr - weight_base : 0),
                 "\r\n");
    uart_put_u32("  -> m_a=", static_cast<uint32_t>(params_ptr[0]), "");
    uart_put_u32(" m_b=", static_cast<uint32_t>(params_ptr[1]), "");
    uart_put_u32(" n=", static_cast<uint32_t>(params_ptr[4]), "\r\n");

    if (add_b_is_scalar_) {
      // Scalar B is a compile-time constant that AddLoweringToLinalg folded into bias.
      uart_puts("  -> mode=scalar (B folded into bias)\r\n");
      SoftwareRequantFallback::executeADD_broadcast(input_a, output_ptr,
                                                    num_elements, params_ptr);
    } else {
      // Both operands are full tensors laid out back to back from the input
      // base, which is how the test harness stages a two-input model.
      const int8_t *input_b = operand_b_base_
          ? reinterpret_cast<const int8_t *>(operand_b_base_)
          : reinterpret_cast<const int8_t *>(static_cast<uint64_t>(CDC_RAW_IN0_BASE) + add_input_b_offset_);
      uart_put_u32("  -> mode=tensor, b_offset=",
                   static_cast<uint32_t>(add_input_b_offset_), "\r\n");
      SoftwareRequantFallback::executeADD_tensor(input_a, input_b, output_ptr,
                                                 num_elements, params_ptr);
    }

    uart_puts("[NPUV4Runtime] ADD CPU fallback done\r\n");
    return NPUDriverStatus::OK;
  }

  if (last_op_ == NeonOpcode::MUL) {
    // Mirrors the ADD block above exactly — same DRAM layout convention
    // (no DMA_READ/DMA_WRITE for this op; operate directly on DRAM buffers),
    // same broadcast-vs-tensor branch, same b_is_scalar/params layout idea,
    // just calling executeMUL_* instead of executeADD_*.
    size_t num_elements = mul_num_elements_;
    if (num_elements == 0 && current_model_ &&
        current_model_->output_tensor_idx < current_model_->tensors.size()) {
      num_elements =
          current_model_->tensors[current_model_->output_tensor_idx].size_bytes;
    }
    if (num_elements == 0) {
      uart_puts("[NPUV4Runtime] MUL: output tensor size=0, skipping\r\n");
      return NPUDriverStatus::OK;
    }

    const int8_t *input_a = (current_model_ && current_model_->input_tensor_idx < current_model_->tensors.size() && current_model_->tensors[current_model_->input_tensor_idx].data)
        ? static_cast<const int8_t *>(current_model_->tensors[current_model_->input_tensor_idx].data)
        : reinterpret_cast<const int8_t *>(CDC_RAW_IN0_BASE);
    const int32_t *params_ptr = pending_ctx_.mul_params_ptr;
    int8_t *output_ptr = (current_model_ && current_model_->output_tensor_idx < current_model_->tensors.size() && current_model_->tensors[current_model_->output_tensor_idx].data)
        ? static_cast<int8_t *>(current_model_->tensors[current_model_->output_tensor_idx].data)
        : reinterpret_cast<int8_t *>(CDC_ISP_OUT0_BASE);

    uart_put_u32("[NPUV4Runtime] MUL CPU fallback, num_elements=",
                 static_cast<uint32_t>(num_elements), "\r\n");

    if (mul_b_is_scalar_) {
      // params[5] holds the quantized scalar value of operand B written by
      // MulParamSerializer.
      int8_t input_b_scalar =
          static_cast<int8_t>(params_ptr ? params_ptr[5] : 0);
      SoftwareRequantFallback::executeMUL_broadcast(
          input_a, input_b_scalar, output_ptr, num_elements, params_ptr);
    } else {
      const int8_t *input_b = operand_b_base_
          ? reinterpret_cast<const int8_t *>(operand_b_base_)
          : reinterpret_cast<const int8_t *>(static_cast<uint64_t>(CDC_RAW_IN0_BASE) + mul_input_b_offset_);
      SoftwareRequantFallback::executeMUL_tensor(input_a, input_b, output_ptr,
                                                 num_elements, params_ptr);
    }

    uart_puts("[NPUV4Runtime] MUL CPU fallback done\r\n");
    return NPUDriverStatus::OK;
  }

  if (last_op_ == NeonOpcode::SUB) {
    size_t num_elements = sub_num_elements_;
    if (num_elements == 0 && current_model_) {
      if (current_model_->output_tensor_idx < current_model_->tensors.size()) {
        num_elements = current_model_->tensors[current_model_->output_tensor_idx].size_bytes;
      }
    }
    if (num_elements == 0) {
      uart_puts("[NPUV4Runtime] SUB: output tensor size=0, skipping\r\n");
      return NPUDriverStatus::OK;
    }

    const int8_t *input_a = (current_model_ && current_model_->input_tensor_idx < current_model_->tensors.size() && current_model_->tensors[current_model_->input_tensor_idx].data)
        ? static_cast<const int8_t *>(current_model_->tensors[current_model_->input_tensor_idx].data)
        : reinterpret_cast<const int8_t *>(CDC_RAW_IN0_BASE);
    const int32_t *params_ptr = pending_ctx_.sub_params_ptr;
    int8_t *output_ptr = (current_model_ && current_model_->output_tensor_idx < current_model_->tensors.size() && current_model_->tensors[current_model_->output_tensor_idx].data)
        ? static_cast<int8_t *>(current_model_->tensors[current_model_->output_tensor_idx].data)
        : reinterpret_cast<int8_t *>(CDC_ISP_OUT0_BASE);

    // SUB shares ADD's fused block (m_b < 0). A stream compiled before the switch
    // carries the 10-int32 block, whose word 6 is z_a, never the magic.
    if (!params_ptr ||
        static_cast<uint32_t>(params_ptr[6]) != NEON_ADD_FUSED_PARAMS_MAGIC) {
      uart_puts("[NPUV4Runtime] SUB: param block is not the fused-requant layout "
                "(recompile the .nsm)\r\n");
      return NPUDriverStatus::ERROR_UNSUPPORTED_OP;
    }

    uart_put_u32("[NPUV4Runtime] SUB CPU fallback, num_elements=",
                 static_cast<uint32_t>(num_elements), "\r\n");
    uart_put_u32("  -> m_a=", static_cast<uint32_t>(params_ptr[0]), "");
    uart_put_u32(" m_b=", static_cast<uint32_t>(params_ptr[1]), "");
    uart_put_u32(" n=", static_cast<uint32_t>(params_ptr[4]), "\r\n");

    if (sub_b_is_scalar_) {
      // Scalar B is a compile-time constant that SubLoweringToLinalg folded into bias.
      SoftwareRequantFallback::executeADD_broadcast(input_a, output_ptr,
                                                    num_elements, params_ptr);
    } else {
      const int8_t *input_b = operand_b_base_
          ? reinterpret_cast<const int8_t *>(operand_b_base_)
          : reinterpret_cast<const int8_t *>(static_cast<uint64_t>(CDC_RAW_IN0_BASE) + sub_input_b_offset_);
      SoftwareRequantFallback::executeADD_tensor(input_a, input_b, output_ptr,
                                                 num_elements, params_ptr);
    }

    uart_puts("[NPUV4Runtime] SUB CPU fallback done\r\n");
    return NPUDriverStatus::OK;
  }

  return NPUDriverStatus::OK;
}

// HAS_CONV2D: one convolution layer on the V4.7 HasNpuTop controller. The controller
// walks the tiles; with tile_cin < in_c each tile runs in_c / tile_cin core passes that
// accumulate in SRAM-C (bias preload on the first, requant + LUT on the last).
//
// The VP copies the whole DRAM window in and out around the instruction, so every
// operand is staged into the private window FX1_HAS_WIN_BASE, 64-byte aligned:
//   [input][weights][bias][scale][shift][lut][output]
// NEON_HAS_IO_CHW (NCHW conv): the input is padded here with zp_in (PAD = 0 to the
// hardware), because HasNpuTop zero-fills its padding but the folded bias
// (fold_input_zp) expects the input zero point there.
// NEON_HAS_IO_ROWMAJOR (FC as 1x1 conv over M = in_w positions): the [M][K] input is
// transposed to [K][M] on the way in, and the [N][M] result to [M][N] on the way out.
// Row-stacked HAS_CONV2D (io_layout NEON_HAS_IO_ROWSTACK, PLAN_has_tiling_search_v2.md §1.4).
// The instruction carries the original stride-1 Kh x Kw layer (unpadded input, pads, output).
// The runtime stages
//   X'[(c, ky)][y * Wp + x] = Xpad[c][y + ky][x],  C' = in_c * kh, one row of OH * Wp + kw - 1,
// with zp_in on the border and in the kw - 1 tail positions, and runs HasNpuTop as a 1 x kw
// convolution over that row (no hardware padding). Each output channel then holds OH * Wp
// positions; the kw - 1 last columns of every output row are discarded on the copy back.
// The weights were written by the compiler for the stacked geometry (OIHW [O][C][kh][kw] is
// byte-identical to [O][C*kh][1][kw]); TILE_CIN counts stacked channels.
NPUDriverStatus NPUV4Runtime::triggerHasConvRowStack() {
  const NeonHasConv2DInstr &c = pending_has_conv_;
  const uint32_t C = c.in_c, H = c.in_h, W = c.in_w, kh = c.kernel_h, kw = c.kernel_w;
  const uint32_t pt = c.pad_t, pb = c.pad_b, pl = c.pad_l, pr = c.pad_r;
  const uint32_t Hp = H + pt + pb, Wp = W + pl + pr, OH = c.out_h, OW = c.out_w;
  if (c.stride != 1 || kh < 2 || kw < 1 || OH + kh - 1 != Hp || OW + kw - 1 != Wp || c.tile_h != 1) {
    uart_puts("[NPUV4Runtime] ERROR: HAS_CONV2D row stack needs a stride-1 KxK conv with the full output\r\n");
    return NPUDriverStatus::ERROR_INVALID_ADDR;
  }
  const uint32_t Cs = C * kh;                 // stacked input channels
  const uint32_t len = OH * Wp + kw - 1;      // the stacked row
  const uint32_t outW = OH * Wp;              // computed positions per output channel
  const uint32_t oc4 = c.out_c * 4u;
  const uint32_t w_bytes = static_cast<uint32_t>(c.weight_bytes);
  auto a64 = [](uint32_t v) { return (v + 63u) & ~63u; };
  uint32_t off = 0;
  const uint32_t in_off  = off; off = a64(off + Cs * len);
  const uint32_t w_off   = off; off = a64(off + w_bytes);
  const uint32_t b_off   = off; off = a64(off + oc4);
  const uint32_t sc_off  = off; off = a64(off + oc4);
  const uint32_t sh_off  = off; off = a64(off + oc4);
  const uint32_t lut_off = off; off = a64(off + 256u);
  const uint32_t out_off = off; off = a64(off + c.out_c * outW);
  if (off > FX1_HAS_WIN_SIZE) {
    uart_put_u32("[NPUV4Runtime] ERROR: HAS_CONV2D row stack needs window bytes=", off, "\r\n");
    return NPUDriverStatus::ERROR_UNSUPPORTED_OP;
  }
  uint8_t *win = reinterpret_cast<uint8_t *>(FX1_HAS_WIN_BASE);

  // 1. Stage X' (border and tail = input zero point, which the folded bias expects).
  const int izp = static_cast<int>(static_cast<int8_t>(c.input_zero_point));
  int8_t *xs = reinterpret_cast<int8_t *>(win + in_off);
  for (uint32_t ch = 0; ch < C; ++ch) {
    const int8_t *src = has_in_ptr_ + static_cast<size_t>(ch) * H * W;
    for (uint32_t ky = 0; ky < kh; ++ky) {
      int8_t *d = xs + static_cast<size_t>(ch * kh + ky) * len;
      for (uint32_t y = 0; y < OH; ++y, d += Wp) {
        const int32_t r = static_cast<int32_t>(y + ky) - static_cast<int32_t>(pt);   // input row
        if (r < 0 || r >= static_cast<int32_t>(H)) { std::memset(d, izp, Wp); continue; }
        if (pl) std::memset(d, izp, pl);
        std::memcpy(d + pl, src + static_cast<size_t>(r) * W, W);
        if (pr) std::memset(d + pl + W, izp, pr);
      }
      if (kw > 1) std::memset(d, izp, kw - 1);   // tail: feeds discarded outputs only
    }
  }
  // 2. Weights and per-channel tables, verbatim.
  const uint8_t *wb = static_cast<const uint8_t *>(current_model_->weight_buffer);
  std::memcpy(win + w_off,  wb + c.weight_offset, w_bytes);
  std::memcpy(win + b_off,  wb + c.bias_offset,   oc4);
  std::memcpy(win + sc_off, wb + c.scale_offset,  oc4);
  std::memcpy(win + sh_off, wb + c.shift_offset,  oc4);
  if (c.act_type != 0) std::memcpy(win + lut_off, wb + c.lut_offset, 256);
  __asm__ volatile("fence" ::: "memory");

  // 3. Program a 1 x kw convolution over one row, no hardware padding.
  const uint32_t base = static_cast<uint32_t>(FX1_HAS_WIN_BASE);
  npu_has_gemm_t g = {};
  g.win_base = base;
  g.win_size = (off + 4095u) & ~4095u;
  g.in_addr = base + in_off;   g.w_addr = base + w_off;     g.out_addr = base + out_off;
  g.bias_addr = base + b_off;  g.scale_addr = base + sc_off; g.shift_addr = base + sh_off;
  g.lut_addr = base + lut_off;
  g.in_c = Cs;       g.in_h = 1u;  g.in_w = len;
  g.out_c = c.out_c; g.out_h = 1u; g.out_w = outW;
  g.kh = 1u; g.kw = kw; g.stride = 1u; g.pad = 0u;
  g.pad_t = g.pad_l = g.pad_b = g.pad_r = 0u; g.pad_value = 0;
  g.act_type = c.act_type;
  g.zp_out = c.output_zero_point;
  g.flags = c.flags;
  g.tile_cout = c.tile_cout; g.tile_h = c.tile_h; g.tile_w = c.tile_w;
  g.tile_cin = c.tile_cin;   g.y_used = c.y_used;
  uart_put_u32("[NPUV4Runtime] HAS_CONV2D rowstack cin=", Cs, "");
  uart_put_u32(" len=", len, "");
  uart_put_u32(" tile_cin=", c.tile_cin, "");
  uart_put_u32(" cout=", c.out_c, "");
  uart_put_u32(" tile_w=", c.tile_w, "\r\n");
  const int rc = npu_has_gemm_conv(&g);
  if (rc <= 0) {
    uart_put_i32("[NPUV4Runtime] ERROR: HAS_CONV2D rc=", rc, "\r\n");
    return NPUDriverStatus::ERROR_DEVICE_BUSY;
  }

  // 4. [out_c][OH * Wp] -> [out_c][OH][OW]: drop the kw - 1 last columns of every row.
  const int8_t *o = reinterpret_cast<const int8_t *>(win + out_off);
  for (uint32_t oc = 0; oc < c.out_c; ++oc)
    for (uint32_t y = 0; y < OH; ++y)
      std::memcpy(has_out_ptr_ + (static_cast<size_t>(oc) * OH + y) * OW,
                  o + static_cast<size_t>(oc) * outW + static_cast<size_t>(y) * Wp, OW);
  return NPUDriverStatus::OK;
}

NPUDriverStatus NPUV4Runtime::triggerHasConv() {
  const NeonHasConv2DInstr &c = pending_has_conv_;
  if (!has_in_ptr_ || !has_out_ptr_ || !current_model_ || !current_model_->weight_buffer) {
    uart_puts("[NPUV4Runtime] ERROR: HAS_CONV2D without bound tensors or weights\r\n");
    return NPUDriverStatus::ERROR_INVALID_ADDR;
  }
  if (c.io_layout == NEON_HAS_IO_ROWSTACK)
    return triggerHasConvRowStack();
  if (c.io_layout != NEON_HAS_IO_CHW && c.io_layout != NEON_HAS_IO_ROWMAJOR) {
    uart_put_u32("[NPUV4Runtime] ERROR: HAS_CONV2D unknown io_layout=", c.io_layout, "\r\n");
    return NPUDriverStatus::ERROR_UNSUPPORTED_OP;
  }
  const uint32_t in_c = c.in_c, in_h = c.in_h, in_w = c.in_w;
  const uint32_t pt = c.pad_t, pb = c.pad_b, pl = c.pad_l, pr = c.pad_r;
#ifdef NEON_HAS_SW_PAD
  // Legacy: the runtime writes a padded [C][H+pt+pb][W+pl+pr] copy and HasNpuTop runs an
  // unpadded convolution on it (Makefile HAS_SW_PAD=1, kept for A/B comparison only).
  const uint32_t hp = in_h + pt + pb, wp = in_w + pl + pr;
#else
  // Default: the window holds the UNPADDED [C][H][W] input; HasNpuTop inserts the padding
  // value itself while it gathers the kernel windows (PAD_T/L/B/R + PAD_VALUE,
  // SW_INTEGRATION_GUIDE.md §4.7; in the IFMAP feeder when the VP runs with HAS_FEEDER_PAD=1).
  const uint32_t hp = in_h, wp = in_w;
  // The rule HasNpuTop checks at the push (BAD_GEOM otherwise), reported here with the layer's numbers.
  const uint32_t s = c.stride ? c.stride : 1u;
  if ((c.out_h - 1u) * s + c.kernel_h > in_h + pt + pb || (c.out_w - 1u) * s + c.kernel_w > in_w + pl + pr) {
    uart_puts("[NPUV4Runtime] ERROR: HAS_CONV2D output does not fit the padded input\r\n");
    return NPUDriverStatus::ERROR_INVALID_ADDR;
  }
#endif
  const uint32_t out_bytes = c.out_c * c.out_h * c.out_w;
  const uint32_t oc4 = c.out_c * 4u;
  const uint32_t w_bytes = static_cast<uint32_t>(c.weight_bytes);
  auto a64 = [](uint32_t v) { return (v + 63u) & ~63u; };
  uint32_t off = 0;
  const uint32_t in_off  = off; off = a64(off + in_c * hp * wp);
  const uint32_t w_off   = off; off = a64(off + w_bytes);
  const uint32_t b_off   = off; off = a64(off + oc4);
  const uint32_t sc_off  = off; off = a64(off + oc4);
  const uint32_t sh_off  = off; off = a64(off + oc4);
  const uint32_t lut_off = off; off = a64(off + 256u);
  const uint32_t out_off = off; off = a64(off + out_bytes);
  if (off > FX1_HAS_WIN_SIZE) {
    uart_put_u32("[NPUV4Runtime] ERROR: HAS_CONV2D needs window bytes=", off, "\r\n");
    return NPUDriverStatus::ERROR_UNSUPPORTED_OP;
  }
  uint8_t *win = reinterpret_cast<uint8_t *>(FX1_HAS_WIN_BASE);

  const bool rowmajor = (c.io_layout == NEON_HAS_IO_ROWMAJOR);
  if (rowmajor && (in_h != 1 || c.out_h != 1 || (pt | pb | pl | pr) != 0u)) {
    uart_puts("[NPUV4Runtime] ERROR: HAS_CONV2D FC layout needs in_h = out_h = 1 and no padding\r\n");
    return NPUDriverStatus::ERROR_INVALID_ADDR;
  }

  // 1. Input -> HasNpuTop's [C][H][W] in the window.
  if (rowmajor) {
    // FC: [M][K] -> [K][M] (M = in_w positions, K = in_c channels).
    int8_t *dst = reinterpret_cast<int8_t *>(win + in_off);
    for (uint32_t m = 0; m < in_w; ++m)
      for (uint32_t k = 0; k < in_c; ++k)
        dst[static_cast<size_t>(k) * in_w + m] = has_in_ptr_[static_cast<size_t>(m) * in_c + k];
  } else {
#ifdef NEON_HAS_SW_PAD
    // Conv: [C][H][W] -> [C][hp][wp] with the input zero point on the border.
    const int izp = static_cast<int>(static_cast<int8_t>(c.input_zero_point));
    const bool padded = (pt | pb | pl | pr) != 0u;
    for (uint32_t ch = 0; ch < in_c; ++ch) {
      int8_t *dst = reinterpret_cast<int8_t *>(win + in_off) + static_cast<size_t>(ch) * hp * wp;
      const int8_t *src = has_in_ptr_ + static_cast<size_t>(ch) * in_h * in_w;
      if (padded) std::memset(dst, izp, static_cast<size_t>(hp) * wp);
      for (uint32_t y = 0; y < in_h; ++y)
        std::memcpy(dst + static_cast<size_t>(y + pt) * wp + pl, src + static_cast<size_t>(y) * in_w, in_w);
    }
#else
    // Conv: the tensor is already [C][H][W] and contiguous: one block copy, no border.
    std::memcpy(win + in_off, has_in_ptr_, static_cast<size_t>(in_c) * in_h * in_w);
#endif
  }
  // 2. Weights (already in SAURIA order with Cin parts) and per-channel tables, verbatim.
  const uint8_t *wb = static_cast<const uint8_t *>(current_model_->weight_buffer);
  std::memcpy(win + w_off,  wb + c.weight_offset, w_bytes);
  std::memcpy(win + b_off,  wb + c.bias_offset,   oc4);
  std::memcpy(win + sc_off, wb + c.scale_offset,  oc4);
  std::memcpy(win + sh_off, wb + c.shift_offset,  oc4);
  if (c.act_type != 0) std::memcpy(win + lut_off, wb + c.lut_offset, 256);
  __asm__ volatile("fence" ::: "memory");

  // 3. Program, push, wait (bsp_ext/npu_v4_native_driver.c).
  const uint32_t base = static_cast<uint32_t>(FX1_HAS_WIN_BASE);
  npu_has_gemm_t g = {};
  g.win_base = base;
  g.win_size = (off + 4095u) & ~4095u;
  g.in_addr = base + in_off;   g.w_addr = base + w_off;     g.out_addr = base + out_off;
  g.bias_addr = base + b_off;  g.scale_addr = base + sc_off; g.shift_addr = base + sh_off;
  g.lut_addr = base + lut_off;
  g.in_c = in_c;  g.in_h = hp;  g.in_w = wp;
  g.out_c = c.out_c; g.out_h = c.out_h; g.out_w = c.out_w;
  g.kh = c.kernel_h; g.kw = c.kernel_w; g.stride = c.stride; g.pad = 0u;
#ifndef NEON_HAS_SW_PAD
  g.pad_t = pt; g.pad_l = pl; g.pad_b = pb; g.pad_r = pr;   // hardware padding
  g.pad_value = static_cast<int8_t>(c.input_zero_point);
#endif
  g.act_type = c.act_type;
  g.zp_out = c.output_zero_point;
  g.flags = c.flags;
  g.tile_cout = c.tile_cout; g.tile_h = c.tile_h; g.tile_w = c.tile_w;
  g.tile_cin = c.tile_cin;   g.y_used = c.y_used;
  uart_put_u32("[NPUV4Runtime] HAS_CONV2D cin=", in_c, "");
  uart_put_u32(" tile_cin=", c.tile_cin, "");
  uart_put_u32(" cout=", c.out_c, "");
  uart_put_u32(" tile=", c.tile_h, "");
  uart_put_u32("x", c.tile_w, "");
#ifdef NEON_HAS_SW_PAD
  uart_puts(" pad=sw\r\n");
#else
  uart_put_u32(" pad=hw t", pt, "");
  uart_put_u32(" l", pl, "");
  uart_put_u32(" b", pb, "");
  uart_put_u32(" r", pr, "");
  uart_put_i32(" value=", static_cast<int>(static_cast<int8_t>(c.input_zero_point)), "\r\n");
#endif
#ifdef NEON_HAS_DEBUG
  uart_put_u32("  in_off=", in_off, " w_off=");
  uart_put_u32("", w_off, " b_off=");
  uart_put_u32("", b_off, " out_off=");
  uart_put_u32("", out_off, "\r\n");
  uart_put_i32("  bias[0]=", *reinterpret_cast<const int32_t *>(win + b_off), " scale[0]=");
  uart_put_i32("", *reinterpret_cast<const int32_t *>(win + sc_off), " shift[0]=");
  uart_put_i32("", *reinterpret_cast<const int32_t *>(win + sh_off), "\r\n");
  uart_put_i32("  has_in[0]=", static_cast<int>(has_in_ptr_[0]), " has_in[1]=");
  uart_put_i32("", static_cast<int>(has_in_ptr_[1]), " in_padded[10]=");
  uart_put_i32("", static_cast<int>(reinterpret_cast<const int8_t *>(win + in_off)[10]), "\r\n");
  uart_put_i32("  w[0]=", static_cast<int>(reinterpret_cast<const int8_t *>(win + w_off)[0]), " w[1]=");
  uart_put_i32("", static_cast<int>(reinterpret_cast<const int8_t *>(win + w_off)[1]), "\r\n");
  uart_put_i32("  act_type=", static_cast<int>(c.act_type), " lut[0]=");
  uart_put_i32("", static_cast<int>(reinterpret_cast<const int8_t *>(win + lut_off)[0]), " lut[128]=");
  uart_put_i32("", static_cast<int>(reinterpret_cast<const int8_t *>(win + lut_off)[128]), "\r\n");
#endif
  const int rc = npu_has_gemm_conv(&g);
  if (rc <= 0) {
    uart_put_i32("[NPUV4Runtime] ERROR: HAS_CONV2D rc=", rc, "\r\n");
    return NPUDriverStatus::ERROR_DEVICE_BUSY;
  }
#ifdef NEON_HAS_DEBUG
  uint32_t n_non_zp = 0;
  for (uint32_t i = 0; i < out_bytes; ++i) {
    if (reinterpret_cast<const int8_t *>(win + out_off)[i] != static_cast<int8_t>(c.output_zero_point))
      n_non_zp++;
  }
  uart_put_i32("[NPUV4Runtime] HAS_CONV2D returned rc=", rc, " non_zp_count=");
  uart_put_u32("", n_non_zp, "\r\n");
  uart_put_i32("  out[0]=", static_cast<int>(reinterpret_cast<const int8_t *>(win + out_off)[0]), " out[1]=");
  uart_put_i32("", static_cast<int>(reinterpret_cast<const int8_t *>(win + out_off)[1]), "\r\n");
#endif

  // 4. Output back into the fragment's output tensor.
  if (rowmajor) {
    // FC: HasNpuTop wrote [N][M] (N = out_c, M = out_w); the tensor is [M][N].
    const int8_t *src = reinterpret_cast<const int8_t *>(win + out_off);
    const uint32_t n_out = c.out_c, m_out = c.out_w;
    for (uint32_t n = 0; n < n_out; ++n)
      for (uint32_t m = 0; m < m_out; ++m)
        has_out_ptr_[static_cast<size_t>(m) * n_out + n] = src[static_cast<size_t>(n) * m_out + m];
  } else {
    // Conv: [C][OH][OW], the same layout as the tensor.
    std::memcpy(has_out_ptr_, win + out_off, out_bytes);
  }
  return NPUDriverStatus::OK;
}

// HAS_ELEMWISE: ELEM_WISE ADD / MAX_POOL on the V4.7 HasNpuTop vector unit. HasNpuTop's data flow
// controller runs it on GvuTop through the GVU register file in the VP's default configuration
// (HAS_GVU_TOP=3 HAS_GVU_ELEM_CLK=1 HAS_GVU_REGS=1 HAS_GVU_AGU_ND=1). Same window contract as
// HAS_CONV2D: operands staged into FX1_HAS_WIN_BASE (64-byte aligned), result copied back.
//   ADD     : [A][B][out]   (no B region when b_src == NEON_HAS_ELEM_B_IS_A: B_ADDR = A_ADDR)
//   MAX_POOL: [in][out]
NPUDriverStatus NPUV4Runtime::triggerHasElemwise() {
  const NeonHasElemwiseInstr &e = pending_has_elem_;
  if (!has_in_ptr_ || !has_out_ptr_) {
    uart_puts("[NPUV4Runtime] ERROR: HAS_ELEMWISE without bound tensors\r\n");
    return NPUDriverStatus::ERROR_INVALID_ADDR;
  }
  auto a64 = [](uint32_t v) { return (v + 63u) & ~63u; };
  uint8_t *win = reinterpret_cast<uint8_t *>(FX1_HAS_WIN_BASE);
  const uint32_t base = static_cast<uint32_t>(FX1_HAS_WIN_BASE);
  int rc = 0;

  if (e.mode == NEON_HAS_ELEM_ADD) {
    const uint32_t n = e.len;
    const bool b_is_a = (e.b_src == NEON_HAS_ELEM_B_IS_A);
    // B: fragment operand 1 (beginFragment); single-program harness: A and B back to back from RAW_IN0.
    const int8_t *b = b_is_a ? has_in_ptr_
        : operand_b_base_ ? reinterpret_cast<const int8_t *>(operand_b_base_)
        : reinterpret_cast<const int8_t *>(static_cast<uintptr_t>(CDC_RAW_IN0_BASE) + n);
    uint32_t off = 0;
    const uint32_t a_off = off; off = a64(off + n);
    const uint32_t b_off = b_is_a ? a_off : off;
    if (!b_is_a) off = a64(off + n);
    const uint32_t o_off = off; off = a64(off + n);
    if (n == 0 || off > FX1_HAS_WIN_SIZE) {
      uart_put_u32("[NPUV4Runtime] ERROR: HAS_ELEMWISE ADD needs window bytes=", off, "\r\n");
      return NPUDriverStatus::ERROR_UNSUPPORTED_OP;
    }
    std::memcpy(win + a_off, has_in_ptr_, n);
    if (!b_is_a) std::memcpy(win + b_off, b, n);
    __asm__ volatile("fence" ::: "memory");

    npu_has_add_t a = {};
    a.win_base = base;
    a.win_size = (off + 4095u) & ~4095u;
    a.a_addr = base + a_off; a.b_addr = base + b_off; a.out_addr = base + o_off;
    a.len = n;
    a.zp_a = e.zp_a; a.zp_b = e.zp_b; a.zp_o = e.zp_o;
    a.sa = static_cast<uint32_t>(e.sa); a.sb = static_cast<uint32_t>(e.sb); a.so = static_cast<uint32_t>(e.so);
    a.sha = e.sha; a.shb = e.shb; a.sho = e.sho;
    uart_put_u32("[NPUV4Runtime] HAS_ELEM ADD len=", n, "");
    uart_put_i32(" SA=", e.sa, "");
    uart_put_i32(" SB=", e.sb, "");
    uart_put_u32(" SHO=", e.sho, b_is_a ? " (B=A)\r\n" : "\r\n");
    rc = npu_has_elem_add(&a);
    if (rc > 0) std::memcpy(has_out_ptr_, win + o_off, n);
  } else if (e.mode == NEON_HAS_ELEM_MAXPOOL) {
    const uint32_t s = e.pool_s ? e.pool_s : 1u;
    const uint32_t hp = e.in_h + 2u * e.pool_p, wp = e.in_w + 2u * e.pool_p;
    if (hp < e.pool_k || wp < e.pool_k ||
        e.out_h != (hp - e.pool_k) / s + 1u || e.out_w != (wp - e.pool_k) / s + 1u) {
      uart_puts("[NPUV4Runtime] ERROR: HAS_ELEMWISE MAX_POOL output size does not match k/s/p\r\n");
      return NPUDriverStatus::ERROR_INVALID_ADDR;
    }
    const uint32_t in_bytes = e.in_c * e.in_h * e.in_w, out_bytes = e.in_c * e.out_h * e.out_w;
    uint32_t off = 0;
    const uint32_t i_off = off; off = a64(off + in_bytes);
    const uint32_t o_off = off; off = a64(off + out_bytes);
    if (off > FX1_HAS_WIN_SIZE) {
      uart_put_u32("[NPUV4Runtime] ERROR: HAS_ELEMWISE MAX_POOL needs window bytes=", off, "\r\n");
      return NPUDriverStatus::ERROR_UNSUPPORTED_OP;
    }
    std::memcpy(win + i_off, has_in_ptr_, in_bytes);
    __asm__ volatile("fence" ::: "memory");

    npu_has_pool_t p = {};
    p.win_base = base;
    p.win_size = (off + 4095u) & ~4095u;
    p.in_addr = base + i_off; p.out_addr = base + o_off;
    p.c = e.in_c; p.h = e.in_h; p.w = e.in_w;
    p.k = e.pool_k; p.stride = s; p.pad = e.pool_p; p.mode = e.pool_mode;
    uart_put_u32("[NPUV4Runtime] HAS_ELEM MAX_POOL c=", e.in_c, "");
    uart_put_u32(" h=", e.in_h, "");
    uart_put_u32(" w=", e.in_w, "");
    uart_put_u32(" k=", e.pool_k, "");
    uart_put_u32(" s=", s, "");
    uart_put_u32(" p=", e.pool_p, "\r\n");
    rc = npu_has_maxpool(&p);
    if (rc > 0) std::memcpy(has_out_ptr_, win + o_off, out_bytes);
  } else {
    uart_put_u32("[NPUV4Runtime] ERROR: HAS_ELEMWISE unknown mode=", e.mode, "\r\n");
    return NPUDriverStatus::ERROR_UNSUPPORTED_OP;
  }
  if (rc <= 0) {
    uart_put_i32("[NPUV4Runtime] ERROR: HAS_ELEMWISE rc=", rc, "\r\n");   // < 0: -(HasNpuTop error), 0: timeout
    return NPUDriverStatus::ERROR_DEVICE_BUSY;
  }
  return NPUDriverStatus::OK;
}

NPUDriverStatus NPUV4Runtime::trigger() {
  if (last_op_ == NeonOpcode::HAS_CONV2D)
    return triggerHasConv();
  if (last_op_ == NeonOpcode::HAS_ELEMWISE)
    return triggerHasElemwise();
  if (last_op_ == NeonOpcode::GEMM_FUSED || last_op_ == NeonOpcode::ELEM_WISE ||
      last_op_ == NeonOpcode::LAYERNORM  || last_op_ == NeonOpcode::FUSED_ATTN)
    return triggerRich();

  if (last_op_ == NeonOpcode::ADD || last_op_ == NeonOpcode::MUL ||
      last_op_ == NeonOpcode::SUB || last_op_ == NeonOpcode::SUM)
    return triggerCpuFallback();

  if (last_op_ == NeonOpcode::CONV2D || last_op_ == NeonOpcode::FC)
    return triggerNative();

  return NPUDriverStatus::OK;
}

NPUDriverStatus NPUV4Runtime::waitDone(uint32_t timeout_ms) {
  (void)timeout_ms;
  return NPUDriverStatus::OK;
}

void NPUV4Runtime::writeReg(uint32_t offset, uint32_t value) {
  (void)offset;
  (void)value;
}

uint32_t NPUV4Runtime::readReg(uint32_t offset) {
  (void)offset;
  return 0;
}

bool NPUV4Runtime::isBusy() { return false; }

// Accumulated over every NPU job since boot (bsp_ext/npu_v4_native_driver.c), not the
// last job's registers, so before/after differences (MeasuredProfiler) are per op.
uint64_t NPUV4Runtime::readPerfTotalCycles() const {
  return npu_perf_acc_get(NPU_PERF_TOTAL) + perf_cycles_;   // + CPU SUM estimate
}

uint64_t NPUV4Runtime::readPerfSAActiveCycles() const         { return npu_perf_acc_get(NPU_PERF_SA); }
uint64_t NPUV4Runtime::readPerfOBPActiveCycles() const        { return npu_perf_acc_get(NPU_PERF_OBP); }
uint64_t NPUV4Runtime::readPerfDDRReadBytes() const           { return npu_perf_acc_get(NPU_PERF_DDR_RD); }
uint64_t NPUV4Runtime::readPerfDDRWriteBytes() const          { return npu_perf_acc_get(NPU_PERF_DDR_WR); }
uint64_t NPUV4Runtime::readPerfMACEngineCycles() const        { return npu_perf_acc_get(NPU_PERF_MAC_ENG); }
uint64_t NPUV4Runtime::readPerfDMAEngineCycles() const        { return npu_perf_acc_get(NPU_PERF_DMA_ENG); }
uint64_t NPUV4Runtime::readPerfExecutionCycles() const        { return npu_perf_acc_get(NPU_PERF_EXEC); }
uint64_t NPUV4Runtime::readPerfStallCycles() const            { return npu_perf_acc_get(NPU_PERF_STALL); }
uint64_t NPUV4Runtime::readPerfRichM() const                  { return npu_perf_acc_get(NPU_PERF_RICH_M); }
uint64_t NPUV4Runtime::readPerfRichK() const                  { return npu_perf_acc_get(NPU_PERF_RICH_K); }
uint64_t NPUV4Runtime::readPerfRichN() const                  { return npu_perf_acc_get(NPU_PERF_RICH_N); }
uint64_t NPUV4Runtime::readPerfProcessingCycles() const       { return npu_perf_acc_get(NPU_PERF_PROC); }
uint64_t NPUV4Runtime::readPerfTransferCycles() const         { return npu_perf_acc_get(NPU_PERF_XFER); }
uint64_t NPUV4Runtime::readPerfActivationEngineCycles() const { return npu_perf_acc_get(NPU_PERF_ACT_ENG); }
uint64_t NPUV4Runtime::readPerfPoolingEngineCycles() const    { return npu_perf_acc_get(NPU_PERF_POOL_ENG); }
uint64_t NPUV4Runtime::readPerfReductionEngineCycles() const  { return npu_perf_acc_get(NPU_PERF_RED_ENG); }
uint64_t NPUV4Runtime::readPerfDMAReadCycles() const          { return npu_perf_acc_get(NPU_PERF_DMA_RD_CYC); }
uint64_t NPUV4Runtime::readPerfDMAWriteCycles() const         { return npu_perf_acc_get(NPU_PERF_DMA_WR_CYC); }
