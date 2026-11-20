#pragma once

// ============================================================
// NPUV4Runtime (HAL Boundary for Bare-metal VP_FX1_V2.0)
//
// Wraps the SAURIA NPU v4 FreeRTOS driver (npu_v4.h).
// trigger() invokes npu_v4_gemm() directly without performing SW math.
// ============================================================

#include "core/NPU_ISA.h"
#include "core/DeviceTarget.h"
#include "core/NPUComponent.h"
#include "VPFX1V2NPUTarget.h"
#include "runtime/driver/NPUDriver.h"
#include "runtime/loader/NPUModel.h"
#include "hal/NPUV4OBPConfig.h"
#include "FX1V42Regs.h"
#include <cstddef>
#include <cstdint>

// Constants now live in VPFX1V2NPUTarget.h; old spelling kept for NPUV4Runtime.cpp.
namespace fx1_v45 = vp_fx1_v2;

class NPUV4Runtime : public VPFX1V2NPUTarget, public NPUDriver {
public:
    NPUV4Runtime() = default;
    virtual ~NPUV4Runtime() = default;

    void setModel(const NPUModel* model);

    // --- NPUDriver interface ---
    NPUDriverStatus open() override;
    void resetPerModelState();
    // Base for the software ADD/MUL/SUB second operand; set per fragment by beginFragment() from FragmentBinding::inputs[1]. nullptr keeps CDC_RAW_IN0_BASE.
    void setOperandBBase(uint8_t *base) { operand_b_base_ = base; }
    NPUDriverStatus close() override;
    NPUDriverStatus reset() override;

    NPUDriverStatus dmaTransfer(int channel, void* src, void* dst,
                                size_t size_bytes, DMADir direction) override;
    NPUDriverStatus dmaWait(int channel) override;

    NPUDriverStatus writeInstruction(const void* instr_bytes, size_t size) override;
    NPUDriverStatus writeInstructionEx(const void* instr_bytes, size_t size, uint32_t input_idx, uint32_t output_idx) override;
    NPUDriverStatus trigger() override;
    NPUDriverStatus waitDone(uint32_t timeout_ms = 0) override;
    NPUDriverStatus setNSplit(uint8_t n_split) override;

    // --- Graph execution hooks (runtime/driver/NPUDriver.h). See NPUV4Relocator.cpp. ---
    NPUDriverStatus relocateInstruction(const OpRelocation& op, const FragmentBinding& b) override;
    NPUDriverStatus beginFragment(const NPUModel* model, const FragmentBinding& b) override;
    bool deferWeightUploadToFirstUse() const override { return true; }

    void writeReg(uint32_t offset, uint32_t value) override;
    uint32_t readReg(uint32_t offset) override;

    bool isBusy() override;
    const char* getDeviceName() const override { return "VP_FX1_V2.0"; }

    uint64_t readPerfTotalCycles()      const override;
    uint64_t readPerfSAActiveCycles()   const override;
    uint64_t readPerfOBPActiveCycles()  const override;
    uint64_t readPerfStallCycles()      const override;
    uint64_t readPerfExecutionCycles()  const override;
    uint64_t readPerfProcessingCycles() const override;
    uint64_t readPerfTransferCycles()   const override;
    uint64_t readPerfDDRReadBytes()     const override;
    uint64_t readPerfDDRWriteBytes()    const override;
    uint64_t readPerfDMAStalls()        const override { return readPerfStallCycles(); }
    // Promoted to the NPUDriver vtable — see runtime/driver/NPUDriver.h.
    uint64_t readPerfMACEngineCycles()        const override;
    uint64_t readPerfDMAEngineCycles()        const override;
    uint64_t readPerfRichM()                  const override;
    uint64_t readPerfRichK()                  const override;
    uint64_t readPerfRichN()                  const override;
    uint64_t readPerfActivationEngineCycles() const override;
    uint64_t readPerfPoolingEngineCycles()    const override;
    uint64_t readPerfReductionEngineCycles()  const override;
    uint64_t readPerfDMAReadCycles()          const override;
    uint64_t readPerfDMAWriteCycles()         const override;
    NPUDriverStatus triggerRich();
    NPUDriverStatus triggerCpuFallback();
    NPUDriverStatus triggerNative();
    NPUDriverStatus triggerHasConv();   // HAS_CONV2D: whole layer on HasNpuTop (K x C_in split)
    NPUDriverStatus triggerHasConvRowStack();   // HAS_CONV2D with io_layout NEON_HAS_IO_ROWSTACK
    NPUDriverStatus triggerHasElemwise();      // HAS_ELEMWISE: ELEM_WISE ADD / MAX_POOL on HasNpuTop (GvuTop)

    // The 22 raw counters of NPU_V4.7_INTEGRATION.md "Raw V4.7 Performance Counters", same order
    // as NPU_PERF_* in bsp_ext/npu_v4_native_driver.h. snapPerf() returns the driver's per-job
    // accumulated totals since boot (monotonic, except RICH_M/K/N = shape of the last rich job);
    // main.cpp's FirmwareObserver takes the difference of two snapshots per fragment.
    enum PerfIdx : uint32_t {
      kPerfExec, kPerfStall, kPerfMacOps, kPerfActivePe, kPerfTotalPe, kPerfTotal,
      kPerfRichM, kPerfRichK, kPerfRichN, kPerfSA, kPerfOBP,
      kPerfProc, kPerfXfer, kPerfMacEng, kPerfDmaEng, kPerfActEng, kPerfPoolEng, kPerfRedEng,
      kPerfDdrRd, kPerfDdrWr, kPerfDmaRdCyc, kPerfDmaWrCyc, kPerfCount
    };
    struct PerfAccum {
      uint64_t v[kPerfCount] = {};
      uint32_t native_jobs = 0;   // triggerNative() calls
      uint32_t rich_jobs   = 0;   // triggerRich() calls
      uint32_t hw_jobs     = 0;   // NPU starts: native runs + rich pushes, staging included
      uint32_t stage_jobs  = 0;   // of which SRAM staging passes
    };
    void snapPerf(PerfAccum& out) const;

private:
    uint32_t native_jobs_ = 0;
    uint32_t rich_jobs_   = 0;

    void initRceLUTs();
    NPUDriverStatus ensureWeightsStaged(const char* consumer);
    uint32_t current_k_ = 32;
    uint64_t perf_cycles_ = 0;
    bool is_initialized_ = false;
    
    NeonOpcode      last_op_       = static_cast<NeonOpcode>(0);
    NeonJobContext  pending_ctx_   = {};
    NeonRichInstr   pending_rich_instr_ = {};
    NeonHasConv2DInstr pending_has_conv_ = {};      // last HAS_CONV2D (writeInstructionEx)
    NeonHasElemwiseInstr pending_has_elem_ = {};    // last HAS_ELEMWISE; operands in has_in_ptr_ / has_out_ptr_
    const int8_t*   has_in_ptr_  = nullptr;         // its bound input tensor  [C][H][W]
    int8_t*         has_out_ptr_ = nullptr;         // its bound output tensor [C][OH][OW]
    const NPUModel* current_model_ = nullptr;
    uint64_t bias_offset_raw_ = 0;
    uint64_t weight_offset_raw_ = 0;
    uint64_t output_offset_ = 0;
    int32_t output_zero_point_ = 0;
    int32_t configured_output_zero_point_ = 0;
    bool has_native_config_ = false;
    uint32_t full_channels_out_ = 48;
    // Per-layer OBP bias-section base (absolute offset into the weight
    // buffer), captured from NATIVE_CONFIG's un-adjusted bias_offset. Used
    // to recover which OC-tile a given CONV2D instruction's bias_offset
    // belongs to, and to locate the multiplier/shift/LUT sections that
    // follow it (see Conv2DOBPSerializer::serialize()).
    uint64_t obp_bias_base_ = 0;

    // Native dispatch parameters
    uint32_t native_kh_ = 1;
    uint32_t native_kw_ = 1;
    uint32_t native_stride_ = 1;
    uint32_t native_dil_ = 1;
    uint32_t native_cin_ = FX1_SA_X;
    uint32_t native_k_exact_ = 0;
    uint32_t native_oc_total_ = 0;
    uint32_t native_oh_ = 0;
    uint32_t native_ow_ = 0;
    uint32_t native_pad_h_ = 0;
    uint32_t native_pad_w_ = 0;
    bool native_nchw_ = false;   // NATIVE_CONFIG.act_layout == NEON_LAYOUT_NCHW: activation/output are [C][H][W]
    uint32_t native_tile_row_ = 0;
    uint32_t native_oc_tile_start_ = 0;
    uint32_t native_act_bytes_ = 0;
    const int8_t *native_img_ptr_ = nullptr;
    bool obp_lut_from_compiler_ = false;
    uint32_t act_phys_ = 0;
    uint32_t wgt_phys_ = 0;
    uint32_t scratch_phys_ = 0;
    uint8_t  obp_lut_[FX1_OBP_LANES * FX1_OBP_LUT_BYTES_PER_LANE] = {};
    uint32_t obp_cfg_ = 0;

    // Per-OC bias with the output zero point folded in (see NPUV4Runtime.cpp,
    // buildAdjustedBias). The OBP clamps the requantized accumulator to int8
    // BEFORE the LUT index is formed, so a zp of -128 cannot be added by the LUT
    // -- it has to be inside the value that reaches that clamp.
    int32_t obp_bias_adj_[FX1_OBP_LANES];
    uint8_t native_act_mode_ = 0;   // act_mode of the op currently configured

    const int32_t *buildAdjustedBias(uint32_t lanes);

    // FC native dispatch (GEMM modelled as 1x1 conv over a 1 x M x K image).
    bool      native_is_fc_       = false;
    uint32_t  native_out_stride_  = FX1_SA_TILE;  // dst row stride for read_sramc
    int8_t   *native_out_ptr_     = nullptr;      // nullptr => CDC_NPU_WORK0_BASE + output_offset
    bool      weights_staged_     = false;        // valid only for the current fragment/source
    const void* staged_weights_source_ = nullptr;
    size_t    staged_weights_size_ = 0;

    // Im2Col state tracking for software VPU emulation
    uint32_t im2col_ih_ = 0;
    uint32_t im2col_iw_ = 0;
    uint32_t im2col_ic_ = 0;
    uint32_t im2col_kh_ = 0;
    uint32_t im2col_kw_ = 0;
    uint32_t im2col_stride_h_ = 0;
    uint32_t im2col_stride_w_ = 0;
    uint32_t im2col_pad_h_ = 0;
    uint32_t im2col_pad_w_ = 0;
    uint32_t im2col_oh_ = 0;
    uint32_t im2col_ow_ = 0;
    uint32_t im2col_tile_counter_ = 0;
    bool im2col_enabled_ = false;
    int32_t im2col_izp_ = 0;

    // Element-wise ADD (CPU fallback) state, taken from NeonElemwiseInstr.
    uint32_t add_num_elements_ = 0;
    uint64_t add_input_b_offset_ = 0;
    bool add_b_is_scalar_ = false;

    // Element-wise MUL (CPU fallback) state, taken from NeonElemwiseInstr.
    uint32_t mul_num_elements_ = 0;
    uint64_t mul_input_b_offset_ = 0;
    bool mul_b_is_scalar_ = false;

    // Element-wise SUB (CPU fallback) state, taken from NeonElemwiseInstr.
    uint32_t sub_num_elements_ = 0;
    uint64_t sub_input_b_offset_ = 0;
    bool sub_b_is_scalar_ = false;

    // SUM (GEMM bank + CPU requant) state, taken from NeonSumInstr.
    uint32_t sum_k_ = 0;
    uint32_t sum_n_ = 0;
    uint64_t sum_param_offset_ = 0;
    int32_t  sum_output_zero_point_ = 0;
    uint32_t sum_input_idx_ = 0;
    uint32_t sum_output_idx_ = 0;
    uint8_t *operand_b_base_ = nullptr;
};
