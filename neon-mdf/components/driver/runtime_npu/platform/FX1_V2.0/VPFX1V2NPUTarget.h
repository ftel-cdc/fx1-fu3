#pragma once

// ============================================================
// VPFX1V2NPUTarget.h — identity + hardware specs of VP_FX1_V2.0
// (SAURIA NPU MP1 V1.1 V4.5 "Neo Core Lite 32"). Header-only, core/ only.
//
// The SINGLE source of these numbers, shared by
//   runtime : device/VP_FX1_V2.0/NPUV4Runtime.h        (: VPFX1V2NPUTarget, NPUDriver)
//   toolkit : device/VP_FX1_V2.0/toolkit/VPFX1V2Backend.cpp
//   analyse : device/VP_FX1_V2.0/analyse/EstimateVsMeasuredCLI.cpp
// Same shape as device/qemu_mps3_an547/QEMUMPS3NPUTarget.h. The compiler's hardware
// description (device/VP_FX1_V2.0/toolkit/hw_config/FX1_acc.json) is checked
// against these values by vp_fx1_v2::HWConfig::load().
// ============================================================

#include "core/DeviceTarget.h"
#include "core/NPUComponent.h"
#include "core/NPU_ISA.h"
#include <cstddef>
#include <cstdint>

namespace vp_fx1_v2 {

// V4.5 SRAM banking — three physically separate buffers with independent
// address windows. A single "total SRAM" figure is the wrong budget for any
// per-tensor fit check: activations can only ever live in A, weights in B,
// partial sums in C. See toolkit/hw_config/FX1_acc.json:memory_layout.
constexpr size_t kSramABytes       = 161792u;   // 158 KiB — IFmap  (5056 x 32 B)
constexpr size_t kSramBBytes       = 165888u;   // 162 KiB — weight (5184 x 32 B)
constexpr size_t kSramCBytes       = 196608u;   // 192 KiB — psum   (1536 x 128 B)
constexpr size_t kSramScratchBytes =  49152u;   //  48 KiB — RE/LUT, host-invisible
constexpr size_t kSramTotalBytes   = kSramABytes + kSramBBytes + kSramCBytes + kSramScratchBytes;
static_assert(kSramTotalBytes == 573440u, "V4.5 total SRAM must be 560 KiB");

inline NPUHWSpecs makeVPFX1V2Specs() {
    NPUHWSpecs specs{};

    // --- Architecture: 32x32 output-stationary systolic array ---
    specs.arch_type            = NPUArchType::SYSTOLIC_ARRAY;
    specs.dataflow             = NPUDataflow::OUTPUT_STATIONARY;
    specs.matrix_block.arch    = NPUArchType::SYSTOLIC_ARRAY;
    specs.matrix_block.sa.rows = 32;
    specs.matrix_block.sa.cols = 32;          // 1024 MACs/cycle

    // --- Compute ---
    // 1024 MACs x 2 ops x 800 MHz = 1.6384 TOPS
    specs.clock_mhz            = 800.0f;      // 1.25 ns period
    specs.peak_tops            = 1.6384f;
    specs.peak_tops_fp32       = 0.0f;        // V4.5 instance is int8/int8/int32 — no FP path
    specs.num_cores            = 1;
    specs.max_concurrent_models = 1;

    // --- On-chip SRAM: three physically separate buffers ---
    specs.sram_size_bytes      = 573440u;     // 560 KiB total
    specs.sram_bandwidth_gbps  = 32.0f;

    // --- Off-chip DRAM ---
    specs.dram_bandwidth_gbps  = 2.0f;

    // --- DMA ---
    specs.num_dma_channels     = 1;
    specs.dma_burst_bytes      = 64u;

    // --- Precisions ---
    specs.supports_float32     = false;
    specs.supports_int8        = true;
    specs.supports_int4        = false;
    specs.supports_int16       = false;

    return specs;
}

} // namespace vp_fx1_v2

class VPFX1V2Target : public DeviceTarget {
public:
    const char* getName() const override { return "VP_FX1_V2.0"; }
    const char* getDescription() const override {
        return "VP_FX1_V2.0 SAURIA NPU v4 Hardware Target";
    }
};

class VPFX1V2NPUTarget : public VPFX1V2Target, public NPUComponent {
public:
    NPUHWSpecs getNPUSpecs() const override { return vp_fx1_v2::makeVPFX1V2Specs(); }

    size_t getSRAMBudget(SramClass c) const override {
        switch (c) {
        case SramClass::ACTIVATION: return vp_fx1_v2::kSramABytes;   // 158 KiB
        case SramClass::WEIGHT:     return vp_fx1_v2::kSramBBytes;   // 162 KiB
        case SramClass::PSUM:       return vp_fx1_v2::kSramCBytes;   // 192 KiB
        }
        return vp_fx1_v2::kSramTotalBytes;
    }

    // Compile-time mirror of what the runtime can encode natively. Anything not
    // listed executes on the RISC-V core through runtime/opfallbacks/.
    bool supportsOp(NeonOpcode op) const override {
        switch (op) {
        case NeonOpcode::CONV2D:      // native / GEMM_FUSED
        case NeonOpcode::FC:          // native / GEMM_FUSED (M = 1)
        case NeonOpcode::ADD:         // ELEM_WISE mode 0
        case NeonOpcode::MAXPOOL:     // ELEM_WISE mode 1
        case NeonOpcode::MUL:         // ELEM_WISE mode 2  (new in V4.5)
        case NeonOpcode::SET_NSPLIT:
        case NeonOpcode::DMA_READ:
        case NeonOpcode::DMA_WRITE:
            return true;
        default:
            return false;             // SOFTMAX, LAYERNORM, DEPTHWISE_CONV, ...
        }
    }
};
