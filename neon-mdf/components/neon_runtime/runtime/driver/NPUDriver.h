#pragma once

// ============================================================
// NEON STUDIO — DEVICE DRIVER (abstract)
// Hardware abstraction layer between Executor and physical device.
//
// Executor calls this interface — never touches MMIO directly.
// Each device provides its own implementation:
//   device/VP_FX1_V2.0/          → NPUV4Runtime
//   device/qemu_mps3_an547/runtime/ → QEMUMPS3Runtime
//
// Responsibilities:
//   - DMA transfers (DRAM ↔ SRAM)
//   - Write instructions to device command buffer
//   - Trigger execution
//   - Wait for completion (interrupt or poll)
//   - MMIO register access
// ============================================================

#include "../../core/NPU_ISA.h"
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C++" {

// DMA transfer direction
enum class DMADir { HOST_TO_DEVICE, DEVICE_TO_HOST };

// Result of a npu_driver operation
enum class NPUDriverStatus {
    OK,
    ERROR_DMA_TIMEOUT,
    ERROR_DEVICE_BUSY,
    ERROR_INVALID_ADDR,
    ERROR_UNSUPPORTED_OP,
};

struct NPUModel;   // forward declaration only — drivers that need the model
                   // include runtime/loader/NPUModel.h themselves

// Where one fragment's operands live for the current run. Built by
// runtime/graph/GraphRunner from the arena plan and the caller's I/O
// buffers. Drivers read it; they never allocate or free anything in it.
struct FragmentBinding {
    uint8_t* inputs[NEON_GRAPH_MAX_FRAG_INPUTS]; // operand k; nullptr = constant / unbound
    uint32_t num_inputs;
    uint8_t* output;
    size_t   output_bytes;
    uint8_t* scratch;                            // NPUModel::scratch_buffer of this fragment
    size_t   scratch_bytes;
    uint8_t* weights;                            // NPUModel::weight_buffer of this fragment
    size_t   weight_bytes;
};

// One instruction offered to NPUDriver::relocateInstruction(). `bytes` points
// into the fragment's PRIVATE writable copy (NPUModel::instruction_buffer, a
// fresh copy per ModelLoader::loadFromMemory), so every patch applies exactly
// once. input_ptr / output_ptr are the bound addresses of the dispatch entry's
// tensors (OperandBinder::tensorPtr).
struct OpRelocation {
    NeonOpcode opcode;
    uint8_t*   bytes;
    size_t     size;
    uint32_t   input_tensor_idx;
    uint32_t   output_tensor_idx;
    uint8_t*   input_ptr;
    uint8_t*   output_ptr;
};

class NPUDriver {
public:
    virtual ~NPUDriver() = default;

    // --- Lifecycle ---

    // Open device, map MMIO, init DMA engine
    virtual NPUDriverStatus open()  = 0;

    // Release all device resources
    virtual NPUDriverStatus close() = 0;

    // Reset device to idle state
    virtual NPUDriverStatus reset() = 0;

    // --- DMA ---

    // Transfer data between host memory and device SRAM.
    // channel: DMA channel index (0 to num_dma_channels-1)
    // src/dst: host virtual addresses or device SRAM offsets
    virtual NPUDriverStatus dmaTransfer(int      channel,
                                     void*    src,
                                     void*    dst,
                                     size_t   size_bytes,
                                     DMADir   direction) = 0;

    // Block until DMA channel finishes
    virtual NPUDriverStatus dmaWait(int channel) = 0;

    // --- Model memory binding ---

    // Tell the device where this model's host-side buffers live.
    //
    // Instructions address tensors as OFFSETS into the model's scratch and
    // weight buffers (see core/NPU_ISA.h), but a device that masters the bus
    // itself needs full physical addresses. This is the one piece of
    // information needed to close that gap, and only the caller that loaded
    // the model has it — so ModelLoader/NeonRuntime hand it over here, once,
    // after load.
    //
    // Deliberately plain pointers and sizes rather than an NPUModel*: taking
    // NPUModel would make every device driver depend on runtime/loader, and
    // the device does not need to know a model is what these buffers belong
    // to. Two memory regions is the whole contract.
    //
    // Default is a no-op, which is correct for any device that never
    // dereferences a tensor address on its own — a device whose instructions
    // already carry absolute addresses, or a software timing model that
    // computes latency without touching operand data (QEMUMPS3Runtime).
    //
    // A device that DOES need this and does not get it cannot silently
    // guess: a driver that resolves an unbound offset as an absolute
    // address turns offset 0 into address 0, and must refuse the op with
    // ERROR_INVALID_ADDR rather than program a wild pointer.
    virtual void bindModelMemory(void*  /*scratch_base*/, size_t /*scratch_size*/,
                                 void*  /*weight_base*/,  size_t /*weight_size*/) {}

    // Supply the model's per-tensor quantisation scales.
    //
    // Separate from bindModelMemory() because it answers a different
    // question — that one says WHERE the data is, this one says how to
    // interpret its values — and because a device can legitimately need one
    // and not the other.
    //
    // These are the model-level input and output scales, taken from the
    // .nsm quant section. Per-CHANNEL weight scales do NOT come through
    // here: they are folded into the per-channel requantisation table the
    // instruction points at (see NeonRequantEntry in core/NPU_ISA.h), which
    // is the only form that can express one factor per output channel.
    //
    // Default no-op: a device that derives everything from the instruction
    // stream, or one whose timing model never looks at operand values
    // (QEMUMPS3Runtime), needs nothing here.
    virtual void bindQuantScales(float /*input_scale*/, float /*output_scale*/) {}

    // --- Graph execution (runtime/graph/GraphRunner) ---

    // Device-specific address fix-up for ONE instruction, after the generic
    // ISA rules (runtime/binder/OperandBinder) have run. A device whose code
    // generator emits only tensor-relative fields needs nothing here.
    // Returning anything but OK aborts the fragment.
    virtual NPUDriverStatus relocateInstruction(const OpRelocation& /*op*/,
                                                const FragmentBinding& /*b*/) {
        return NPUDriverStatus::OK;
    }

    // Called once per fragment after binding and before the first dispatch,
    // so per-model driver state (current model, cached geometry, operand
    // bases) can be reset. `model` stays valid until endFragment().
    virtual NPUDriverStatus beginFragment(const NPUModel* /*model*/,
                                          const FragmentBinding& /*b*/) {
        return NPUDriverStatus::OK;
    }

    // Drivers that can stage weights at the first instruction that consumes
    // WGT memory may defer ModelLoader's eager upload. Other drivers retain
    // the legacy eager behavior.
    virtual bool deferWeightUploadToFirstUse() const { return false; }

    // Called after the fragment's last op (also on failure).
    virtual void endFragment() {}

    // --- Instruction dispatch ---

    // Write one encoded instruction into device command buffer.
    // instr_bytes: pointer to NeonConv2DInstr / NeonReluInstr / etc.
    // The device reads this from its command buffer and executes it.
    virtual NPUDriverStatus writeInstruction(const void* instr_bytes,
                                          size_t      size) = 0;

    // Extended version that receives tensor indices (used by some runtimes to lookup quant params)
    virtual NPUDriverStatus writeInstructionEx(const void* instr_bytes,
                                            size_t      size,
                                            uint32_t    input_idx,
                                            uint32_t    output_idx) {
        (void)input_idx;
        (void)output_idx;
        return writeInstruction(instr_bytes, size);
    }

    // --- Execution control ---

    // Start execution of whatever is in the command buffer
    virtual NPUDriverStatus trigger() = 0;

    // Block until device signals done (interrupt or poll status register)
    // timeout_ms: 0 = wait forever
    virtual NPUDriverStatus waitDone(uint32_t timeout_ms = 0) = 0;

    // --- MMIO ---

    virtual void     writeReg(uint32_t offset, uint32_t value) = 0;
    virtual uint32_t readReg (uint32_t offset)                 = 0;

    // --- Query ---

    // true if device is currently executing
    virtual bool isBusy() = 0;

    // Device name — should match DeviceTarget::getName()
    virtual const char* getDeviceName() const = 0;

    // --- Performance counters ---
    //
    // Read hardware cycle counter from MMIO.
    // Used by Executor to measure per-op latency in cycles:
    //   uint64_t c0 = readPerfTotalCycles();
    //   trigger(); waitDone();
    //   uint64_t delta = readPerfTotalCycles() - c0;
    //
    // Returns 0 if the hardware does not provide a cycle counter.
    // Executor falls back to host wall-clock (std::chrono) when 0 is returned.
    //
    // TLM / FPGA / silicon: override to read FX1_REG_PERF_CYCLES or equivalent.
    // Pure SW simulation: leave as default (returns 0 → host clock fallback).
    virtual uint64_t readPerfTotalCycles() const { return 0; }

    // Read pipeline stall counter — cycles where SA was stalled waiting for DMA.
    // stall / (stall + active) = stall ratio; high value → DMA-bound.
    // Returns 0 if not available.
    virtual uint64_t readPerfDMAStalls() const { return 0; }

    // Read the systolic-array MAC-active cycle counter — cycles where the
    // array was actively performing multiply-accumulates. NOT necessarily
    // (readPerfTotalCycles() - readPerfDMAStalls()): total may include
    // cycles that are neither active nor DMA-stalled (e.g. pipeline
    // fill/drain latency), so this is its own independent hardware-reported
    // counter, not derived from the other two. Intended for later
    // utilization = active / total calculations. Returns 0 if not available.
    // This is a different, complementary metric from the existing
    // mac_efficiency_pct in PerfReport.h (that one is FLOPS-per-cycle
    // throughput vs peak — captures partial-array utilization even during
    // "active" cycles; this one is a coarser busy/idle time split).
    virtual uint64_t readPerfMACActiveCycles() const { return 0; }

    // ── V4.5 extended counters (SAURIA MP1 V1.1) ──────────────────────────
    // Default 0 == "this device does not expose it". analyse/ must treat 0 as
    // absent, never as a measured zero — see MeasuredProfiler::onOpEnd.
    //
    // Offsets: NPU_V4.5_INTEGRATION.md, "Raw V4.5 Performance Counters".
    virtual uint64_t readPerfSAActiveCycles()     const { return 0; }  // 0x1224
    virtual uint64_t readPerfOBPActiveCycles()    const { return 0; }  // 0x1228
    virtual uint64_t readPerfStallCycles()        const { return 0; }  // 0x1204
    virtual uint64_t readPerfExecutionCycles()    const { return 0; }  // 0x1200
    virtual uint64_t readPerfProcessingCycles()   const { return 0; }  // 0x1280
    virtual uint64_t readPerfTransferCycles()     const { return 0; }  // 0x1284
    virtual uint64_t readPerfDDRReadBytes()       const { return 0; }  // 0x129C
    virtual uint64_t readPerfDDRWriteBytes()      const { return 0; }  // 0x12A0

    // ── V4.5 engine-activity + rich-op cross-check counters ──────────────
    // Same "0 == device does not expose it" contract as the block above.
    virtual uint64_t readPerfMACEngineCycles()        const { return 0; }  // 0x1288
    virtual uint64_t readPerfDMAEngineCycles()         const { return 0; }  // 0x128C
    virtual uint64_t readPerfActivationEngineCycles()  const { return 0; }  // 0x1290
    virtual uint64_t readPerfPoolingEngineCycles()     const { return 0; }  // 0x1294
    virtual uint64_t readPerfReductionEngineCycles()   const { return 0; }  // 0x1298
    virtual uint64_t readPerfDMAReadCycles()           const { return 0; }  // 0x12A4
    virtual uint64_t readPerfDMAWriteCycles()          const { return 0; }  // 0x12A8

    // Accumulated rich-instruction GEMM dimensions (GEMM_FUSED/FUSED_ATTN/
    // LAYERNORM/ELEM_WISE). NOT a performance metric — a hardware-truth
    // cross-check that the executed M/K/N match the compiler-declared
    // OpShape. 0 if the driver does not expose it, or if the op wasn't a
    // rich instruction.
    virtual uint64_t readPerfRichM() const { return 0; }
    virtual uint64_t readPerfRichK() const { return 0; }
    virtual uint64_t readPerfRichN() const { return 0; }

    // Note: a raw CPU/core cycle counter (for timing code that bypasses
    // this driver entirely, e.g. a linked compiler kernel called
    // directly) is intentionally NOT here — it has nothing to do with
    // the NPU. See runtime/driver/CPUCycleCounter.h.

    // --- Concurrent execution (Rev2+) ---

    // How many models this npu_driver can run truly in parallel.
    //
    // Not declared here — NPUHWSpecs::max_concurrent_models (via
    // NPUComponent::maxConcurrentModels()) is the single source of
    // truth. Every current NPUDriver implementation is also an
    // NPUComponent (multiple inheritance — see NPUV4Runtime,
    // QEMUMPS3Runtime), so callers that need this should hold (or
    // dynamic_cast to) an NPUComponent*, not add a second virtual here.

    // Set SA lane partition boundary.
    // n_split: 0..64 — rows 0..n-1 → Lane A, rows n..63 → Lane B.
    // 64 = single-model mode (Lane B idle, full SA to Lane A).
    //  0 = reverse single-model (Lane A idle, full SA to Lane B).
    // Default no-op: hardware that doesn't support spatial split ignores this.
    // Called by Executor before dispatching the first op of a dual session.
    virtual NPUDriverStatus setNSplit(uint8_t /*n_split*/) {
        return NPUDriverStatus::ERROR_UNSUPPORTED_OP;
    }

    // Submit instruction streams for both lanes simultaneously.
    // queue_a / queue_b: encoded instruction bytes for each lane.
    // Hardware-parallel on devices with max_concurrent_models >= 2.
    //
    // Default fallback (single-model hardware): serialize — run queue_a
    // to completion, then queue_b. Caller sees the same API either way.
    virtual NPUDriverStatus submitConcurrent(const void* queue_a, size_t size_a,
                                          const void* queue_b, size_t size_b) {
        NPUDriverStatus st = writeInstruction(queue_a, size_a);
        if (st != NPUDriverStatus::OK) return st;
        st = trigger();
        if (st != NPUDriverStatus::OK) return st;
        st = waitDone();
        if (st != NPUDriverStatus::OK) return st;
        st = writeInstruction(queue_b, size_b);
        if (st != NPUDriverStatus::OK) return st;
        st = trigger();
        if (st != NPUDriverStatus::OK) return st;
        return waitDone();
    }

    // Per-lane done wait (only meaningful for max_concurrent_models >= 2).
    // Default: delegates to waitDone() — single-model hardware has one state.
    virtual NPUDriverStatus waitDoneLane(uint8_t /*lane*/, uint32_t timeout_ms = 0) {
        return waitDone(timeout_ms);
    }
};

} // extern "C++"
#endif
