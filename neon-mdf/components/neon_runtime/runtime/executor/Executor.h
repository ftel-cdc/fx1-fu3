#pragma once

// ============================================================
// NEON STUDIO — EXECUTOR
// Core execution loop: iterates ops, dispatches each to NPUDriver.
//
// Single-model path (any hardware):
//   For each op:
//     1. Assign tensor ptrs from scratch_buffer (via mem_plan)
//     2. DMA input + weight → device SRAM
//     3. writeInstruction() → device command buffer
//     4. [cycles_before = npu_driver->readPerfTotalCycles()]
//        [stalls_before = npu_driver->readPerfDMAStalls()]
//     5. profiler->onOpStart(op_index, opcode, name, lane, cycles_before)
//     6. trigger() → device starts executing
//     7. waitDone() → block until done
//     8. [cycles_after = npu_driver->readPerfTotalCycles()]
//        [stalls_after = npu_driver->readPerfDMAStalls()]
//     9. profiler->onOpEnd(dma_r, dma_w, cycles_after, stalls_after - stalls_before)
//    10. DMA output ← device SRAM → scratch_buffer
//
// Cycle counter delta (TLM / FPGA / silicon):
//   delta = cycles_after - cycles_before  (wrap-safe: uint64_t subtraction)
//   If npu_driver->readPerfTotalCycles() == 0 → profiler falls back to host wall-clock.
//   On TLM: cycles advance in SystemC simulation time — correct latency ground-truth.
//   On FPGA / silicon: hardware free-running counter at device clock frequency.
//
// Concurrent dual-model path (hardware with maxConcurrentModels >= 2):
//   setNSplit(n_split) once at session start, then:
//   submitConcurrent(queue_a, size_a, queue_b, size_b) — hardware runs both
//   lanes in true parallel. Falls back to serial on single-model hardware.
//
// Pipelined path (host-managed double buffering, see runDoubleBuffering()):
//   For alternating [DMA_READ(i), COMPUTE(i)] pairs, prefetches tile i+1's
//   DMA_READ while tile i's compute is still running, instead of dispatching
//   each pair fully before starting the next. No NPUDriver interface change
//   needed — dmaWait()/waitDone() are already independently pollable.
//
// Executor does NOT know about MMIO or DMA details —
// those are fully delegated to NPUDriver.
// ============================================================

#include "../loader/NPUModel.h"
#include "../driver/NPUDriver.h"
#include "../../core/NPUComponent.h"

// Profiler hooks — optional, may be nullptr
#include "../../analyse/MeasuredProfiler.h"
#include "../../analyse/DMAProfiler.h"

#ifdef __cplusplus
extern "C++" {

class Executor {
public:
    Executor(NPUModel*              model,
             const NPUComponent*   npu_component,
             NPUDriver*          npu_driver,
             MeasuredProfiler*       profiler     = nullptr,
             DMAProfiler*           dma_profiler = nullptr);

    ~Executor() = default;

    // ── Single-model run ─────────────────────────────────────────────────
    // Full inference pass: copy input → run all ops → copy output.
    // Works on any hardware (concurrent or not).
    // On FX1 with N_split=64 this uses the full SA → same performance as Rev1.
    NPUDriverStatus run(const void* input, void* output);

    // ── Concurrent dual-model run ─────────────────────────────────────────
    // Submit two models together. Routing depends on hardware capability
    // (npu_component_->maxConcurrentModels() — NPUComponent is the single
    // source of truth, not NPUDriver):
    //
    //   npu_component_->maxConcurrentModels() >= 2:
    //     → hardware spatial split (true parallel, Queue_A + Queue_B)
    //     → npu_driver->setNSplit(n_split) then npu_driver->submitConcurrent(...)
    //
    //   npu_component_->maxConcurrentModels() == 1 (fallback):
    //     → npu_driver->submitConcurrent() default serialize
    //     → model_a runs to completion, then model_b runs
    //     → same API, lower latency on single-model hardware
    //
    // model_b may target a different SRAM region than the primary model
    // (typical for dual sessions); both share the same NPUDriver here.
    NPUDriverStatus runConcurrent(const void*   input_a, void*   output_a,
                               NPUModel*     model_b,
                               const void*   input_b, void*   output_b,
                               uint8_t       n_split);

    // ── Batch run (Rev3) ─────────────────────────────────────────────────
    // Execute using a pre-compiled BatchProfile.
    //
    // packed_input:  profile.batch_size × model.input_size() bytes
    //   → built by ProfileSelector::pack_inputs()
    //   → real items at slots [0, n_actual), zeros at [n_actual, batch_size)
    //
    // packed_output: profile.batch_size × model.output_size() bytes
    //   → allocated by caller
    //   → pass to ProfileSelector::unpack_outputs() after return
    //   → dummy slots [n_actual, batch_size) are populated but ignored
    //
    // Uses profile.instruction_buffer (the batch-compiled stream).
    // Weights come from model_->weight_buffer (shared across all profiles).
    // Allocates profile.scratch_buffer_size bytes of scratch internally.
    NPUDriverStatus run_batch(const BatchProfile& profile,
                           const void*         packed_input,
                           void*               packed_output);

    // ── Per-op dispatch (exposed for testing) ────────────────────────────
    NPUDriverStatus dispatchOp(const RuntimeOp& op, const void* instr_override = nullptr,
                               uint32_t effective_op_index = UINT32_MAX);

    // ── Pipelined run (host-managed double buffering) ─────────────────────
    // Assumes model_->ops is laid out as strictly alternating
    // [DMA_READ(0), COMPUTE(0), DMA_READ(1), COMPUTE(1), ...] pairs — a
    // convention the op-list producer (bench code / future toolkit) must
    // follow; not enforced by core/NPU_ISA.h, since this is purely a
    // dispatch-order decision (no new opcode, no new struct field).
    //
    // Prefetches tile i+1's input (DMA_READ) while tile i is still
    // computing, instead of run()'s fully-serial dispatch. Reuses the same
    // NPUDriver calls run()/dispatchOp() already use — no interface change.
    NPUDriverStatus runDoubleBuffering(const void* input, void* output);

private:
    NPUModel*             model_;
    const NPUComponent*  npu_component_;
    NPUDriver*         npu_driver_;
    MeasuredProfiler*      profiler_;
    DMAProfiler*          dma_profiler_;

    // ── Internal single-model steps ──────────────────────────────────────

    void copyInput (const void* input);
    void copyOutput(void*       output);

    NPUDriverStatus uploadOpInputs  (const RuntimeOp& op);
    NPUDriverStatus downloadOpOutput(const RuntimeOp& op);

    void* tensorPtr(uint32_t tensor_idx) const;
    void* weightPtr(uint32_t weight_idx) const;

    void notifyOpStart(const RuntimeOp& op, uint32_t effective_op_index = UINT32_MAX);
    void notifyOpEnd  (const RuntimeOp& op,
                       size_t read_bytes, size_t write_bytes);

    // ── Shared DMA issue/wait halves ──────────────────────────────────────
    // Split out of dispatchOp()'s DMA branch so run()/dispatchOp() and
    // runDoubleBuffering() can interleave issue and wait independently instead
    // of always pairing them back-to-back.
    // startOpDMA: writeInstruction + trigger + profiler onTransferStart.
    //   out_channel receives the NeonDMAInstr's channel (-1 if op has no
    //   usable instruction), to be passed to waitOpDMA() later.
    NPUDriverStatus startOpDMA(const RuntimeOp& op, int* out_channel,
                               const void* instr_override = nullptr,
                               uint32_t effective_op_index = UINT32_MAX);
    // waitOpDMA: dmaWait(channel) + profiler onTransferEnd. No-op if channel < 0.
    NPUDriverStatus waitOpDMA(int channel);

    // ── CPU fallback dispatch (see runtime/opfallbacks/CPUFallbackRegistry.h) ──
    // Bypasses npu_driver_ entirely — looks up op's fallback_id in
    // CPUFallbackRegistry and calls the linked function directly against
    // tensorPtr()-resolved host memory. No profiler hooks (see .cpp) —
    // known gap, not solved here.
    NPUDriverStatus dispatchCPUFallback(const RuntimeOp& op);

    // ── Internal concurrent helpers ───────────────────────────────────────

    // Build the flat instruction stream for one model into a contiguous buffer.
    // Used by runConcurrent() to pass queue_a / queue_b to npu_driver.
    // Returned buffer is caller-owned (allocated from scratch space).
    void* buildQueueBuffer(NPUModel* model, size_t* out_size) const;

    // ── Cycle counter helpers ─────────────────────────────────────────────

    // Wrap-safe delta between two counter snapshots.
    // Handles 32-bit hardware counter wrap (common on TLM/FPGA).
    static uint64_t cycleDelta(uint64_t before, uint64_t after) {
        if (after >= before) return after - before;
        // 32-bit wrap (npu_driver returns uint32_t cast to uint64_t)
        return (0xFFFFFFFFULL - before) + after + 1ULL;
    }
};

} // extern "C++"
#endif
