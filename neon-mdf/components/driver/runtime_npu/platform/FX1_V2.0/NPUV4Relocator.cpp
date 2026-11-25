// ============================================================
// NPUV4Runtime — instruction relocation + fragment lifecycle
//
// The VP_FX1_V2.0 code generator (toolkit/mlir/lib/Target/FX1/VP_FX1_V2.0)
// emits address conventions only this device understands:
//   1. values below CDC_RAM0_BASE are offsets relative to the fragment's
//      input / output / weight buffer (rich and DMA instructions);
//   2. ABSOLUTE SoC buffer addresses baked in by the DW-conv, transpose and
//      LUT emitters (CDC_RAW_IN0_BASE, CDC_ISP_OUT0_BASE, CDC_NPU_WORK0_BASE);
//   3. DMA channel roles: ch0 activations, ch1 weights, ch2 write-back.
// These rules used to live in main.cpp run_one_model(); they are moved here
// unchanged so segment-mode results stay reproducible. Zero src/dst fields
// of CPU_FALLBACK ops were already bound by runtime/binder/OperandBinder.
// ============================================================

#include "NPUV4Runtime.h"
#include "core/NPUUnsupportedOpFallback.h"

extern "C" {
#include "soc/soc_memory_map.h"
}

namespace {

constexpr uint64_t kHostAddrFloor = CDC_RAM0_BASE;   // below = compiler-relative offset

template <typename T>
T* as(const OpRelocation& r) {
    return r.size >= sizeof(T) ? reinterpret_cast<T*>(r.bytes) : nullptr;
}

uint64_t addr(const void* p) { return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(p)); }

void relocateFallback(const OpRelocation& r) {
    const auto* fb = as<NeonCPUFallbackInstr>(r);
    if (!fb) return;
    switch (fb->fallback_id) {
    case NEON_CPU_FALLBACK_DW_IM2COL:
        if (auto* i = as<NeonCPUFallbackDWIm2ColInstr>(r)) {
            // DWConvEmitter bakes CDC_RAW_IN0_BASE + channel; rebase onto the
            // real producer, preserving the per-channel offset. dst (planar
            // scratch in CDC_NPU_WORK0) is left alone: the paired DMA_READ
            // reads the same baked address.
            if (i->src_offset >= CDC_RAW_IN0_BASE &&
                i->src_offset < (uint64_t)CDC_RAW_IN0_BASE + CDC_RAW_IN0_SIZE)
                i->src_offset = addr(r.input_ptr) + (i->src_offset - CDC_RAW_IN0_BASE);
        }
        break;
    case NEON_CPU_FALLBACK_TRANSPOSE:
        if (auto* i = as<NeonCPUFallbackTransposeInstr>(r)) {
            // Parity with the segment-mode rule (origin of +0x06000000 undocumented).
            if (i->src_offset == CDC_NPU_WORK0_BASE) i->src_offset = addr(r.output_ptr) + 0x06000000ULL;
            if (i->dst_offset == CDC_ISP_OUT0_BASE)  i->dst_offset = addr(r.output_ptr);
        }
        break;
    case NEON_CPU_FALLBACK_LUT:
        if (auto* i = as<NeonCPUFallbackLutInstr>(r)) {
            if (i->src_offset == CDC_ISP_OUT0_BASE)
                i->src_offset = addr(r.input_tensor_idx != r.output_tensor_idx ? r.input_ptr : r.output_ptr);
            if (i->dst_offset == CDC_ISP_OUT0_BASE) i->dst_offset = addr(r.output_ptr);
        }
        break;
    default:
        break;
    }
}

} // namespace

NPUDriverStatus NPUV4Runtime::relocateInstruction(const OpRelocation& r, const FragmentBinding& b) {
    uint8_t* dram_input  = (b.num_inputs > 0 && b.inputs[0]) ? b.inputs[0] : r.input_ptr;
    uint8_t* dram_output = b.output;

    switch (r.opcode) {
    case NeonOpcode::CPU_FALLBACK:
        relocateFallback(r);
        break;

    case NeonOpcode::GEMM_FUSED:
    case NeonOpcode::ELEM_WISE:
    case NeonOpcode::LAYERNORM:
    case NeonOpcode::FUSED_ATTN: {
        auto* rich = as<NeonRichInstr>(r);
        if (!rich) break;
        const uint64_t w = addr(b.weights);
        if (rich->in_addr_space == 0 && rich->in_addr < kHostAddrFloor) rich->in_addr += addr(dram_input);
        if (rich->out_addr < kHostAddrFloor) rich->out_addr += addr(r.output_ptr);
        if (r.opcode == NeonOpcode::LAYERNORM) {
            if (b.weights) { rich->q_gamma_a_addr += w; rich->v_beta_addr += w; }
        } else {
            rich->q_gamma_a_addr += addr(r.input_ptr);
            rich->k_b_addr       += addr(r.input_ptr);
        }
        if (b.weights) rich->w_addr += w;
        if (rich->bias_addr != 0 && b.weights) rich->bias_addr += w;
        if (rich->has_skip && rich->skip_addr != 0) {
            uint8_t* skip = (b.num_inputs > 1 && b.inputs[1]) ? b.inputs[1] : dram_output;
            rich->skip_addr += addr(skip);
        }
        break;
    }

    case NeonOpcode::DMA_READ: {
        auto* d = as<NeonDMAInstr>(r);
        if (!d) break;
        if (d->channel == 0) {
            if (d->src_offset < kHostAddrFloor) d->src_offset += addr(dram_input);
            d->dst_offset = 0;
        } else if (d->channel == 1) {
            d->src_offset = addr(b.weights);
            d->dst_offset = 0;
            d->size_bytes = static_cast<uint32_t>(b.weight_bytes);
        }
        break;
    }

    case NeonOpcode::DMA_WRITE: {
        auto* d = as<NeonDMAInstr>(r);
        if (d && d->channel == 2 && d->dst_offset < kHostAddrFloor) d->dst_offset += addr(dram_output);
        break;
    }

    default:
        break;
    }
    return NPUDriverStatus::OK;
}

NPUDriverStatus NPUV4Runtime::beginFragment(const NPUModel* model, const FragmentBinding& b) {
    resetPerModelState();
    setModel(model);
    setOperandBBase((b.num_inputs > 1 && b.inputs[1]) ? b.inputs[1] : nullptr);
    return NPUDriverStatus::OK;
}
