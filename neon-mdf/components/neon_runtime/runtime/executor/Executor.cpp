#include "Executor.h"
#include "../opfallbacks/CPUFallbackRegistry.h"
#include "../support/NeonLog.h"
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>

// ============================================================
// Constructor
// ============================================================

Executor::Executor(NPUModel *model, const NPUComponent *npu_component,
                   NPUDriver *npu_driver, MeasuredProfiler *profiler,
                   DMAProfiler *dma_profiler)
    : model_(model), npu_component_(npu_component), npu_driver_(npu_driver),
      profiler_(profiler), dma_profiler_(dma_profiler) {}

// ============================================================
// Tensor / weight ptr helpers
// ============================================================

void *Executor::tensorPtr(uint32_t tensor_idx) const {
  if (tensor_idx >= model_->tensors.size())
    return nullptr;
  return model_->tensors[tensor_idx].data;
}

void *Executor::weightPtr(uint32_t weight_idx) const {
  if (weight_idx == UINT32_MAX || weight_idx >= model_->weights.size())
    return nullptr;
  return model_->weights[weight_idx].data;
}

// ============================================================
// I/O copy helpers
// ============================================================

void Executor::copyInput(const void *input) {
  void *dst = tensorPtr(model_->input_tensor_idx);
  if (!dst || !input)
    return;
  size_t sz = model_->tensors[model_->input_tensor_idx].size_bytes;
  std::memcpy(dst, input, sz);
}

void Executor::copyOutput(void *output) {
  void *src = tensorPtr(model_->output_tensor_idx);
  if (!src || !output)
    return;
  size_t sz = model_->tensors[model_->output_tensor_idx].size_bytes;
  std::memcpy(output, src, sz);
}

// ============================================================
// DMA upload/download for one op
// ============================================================

NPUDriverStatus Executor::uploadOpInputs(const RuntimeOp &op) {
  if (!npu_driver_)
    return NPUDriverStatus::OK;
  // For AXI4MLIR dispatch model: DMA_READ ops are explicit in the stream.
  // Compute ops (Conv2D, FC, etc.) don't DMA here — the instruction stream
  // already contains NeonDMAInstr for the weight/ifmap transfers.
  // This function handles the generic case for non-DMA opcodes
  // that need their input tensor in SRAM before the instruction executes.
  //
  // For simplicity: write input tensor to device via CH2 (IFmap Lane A).
  void *src = tensorPtr(op.input_tensor_idx);
  size_t sz = (op.input_tensor_idx < model_->tensors.size())
                  ? model_->tensors[op.input_tensor_idx].size_bytes
                  : 0;
  if (!src || sz == 0)
    return NPUDriverStatus::OK;

  if (dma_profiler_ && dma_profiler_->isEnabled())
    dma_profiler_->onTransferStart(2, sz, DMADirection::READ, op.index);

  NPUDriverStatus st =
      npu_driver_->dmaTransfer(2, src, nullptr, sz, DMADir::HOST_TO_DEVICE);
  npu_driver_->dmaWait(2);

  if (dma_profiler_ && dma_profiler_->isEnabled())
    dma_profiler_->onTransferEnd(2);

  return st;
}

NPUDriverStatus Executor::downloadOpOutput(const RuntimeOp &op) {
  if (!npu_driver_)
    return NPUDriverStatus::OK;
  void *dst = tensorPtr(op.output_tensor_idx);
  size_t sz = (op.output_tensor_idx < model_->tensors.size())
                  ? model_->tensors[op.output_tensor_idx].size_bytes
                  : 0;
  if (!dst || sz == 0)
    return NPUDriverStatus::OK;

  if (dma_profiler_ && dma_profiler_->isEnabled())
    dma_profiler_->onTransferStart(2, sz, DMADirection::WRITE, op.index);

  NPUDriverStatus st =
      npu_driver_->dmaTransfer(2, nullptr, dst, sz, DMADir::DEVICE_TO_HOST);
  npu_driver_->dmaWait(2);

  if (dma_profiler_ && dma_profiler_->isEnabled())
    dma_profiler_->onTransferEnd(2);

  return st;
}

// ============================================================
// Profiler hooks
// ============================================================

static PerfCounterSnapshot readPerfSnapshot(NPUDriver* d) {
  if (!d) return {};
  PerfCounterSnapshot s;
  s.total      = d->readPerfTotalCycles();
  s.stall      = d->readPerfDMAStalls();
  s.sa         = d->readPerfSAActiveCycles();
  s.obp        = d->readPerfOBPActiveCycles();
  s.mac_engine = d->readPerfMACEngineCycles();
  s.dma_engine = d->readPerfDMAEngineCycles();
  s.act_engine = d->readPerfActivationEngineCycles();
  s.pool_engine= d->readPerfPoolingEngineCycles();
  s.red_engine = d->readPerfReductionEngineCycles();
  s.dma_read   = d->readPerfDMAReadCycles();
  s.dma_write  = d->readPerfDMAWriteCycles();
  s.rich_m     = d->readPerfRichM();
  s.rich_k     = d->readPerfRichK();
  s.rich_n     = d->readPerfRichN();
  return s;
}

void Executor::notifyOpStart(const RuntimeOp &op, uint32_t effective_op_index) {
  if (!profiler_ || !profiler_->isEnabled())
    return;
  uint32_t idx = (effective_op_index != UINT32_MAX) ? effective_op_index : op.index;
  profiler_->onOpStart(idx, op.opcode, op.name, 255, readPerfSnapshot(npu_driver_));
}

void Executor::notifyOpEnd(const RuntimeOp &op, size_t read_bytes,
                           size_t write_bytes) {
  if (!profiler_ || !profiler_->isEnabled())
    return;
  (void)op;
  profiler_->onOpEnd(read_bytes, write_bytes, readPerfSnapshot(npu_driver_));
}

// ============================================================
// dispatchOp — single op execution
// ============================================================

NPUDriverStatus Executor::startOpDMA(const RuntimeOp &op, int *out_channel,
                                     const void *instr_override,
                                     uint32_t effective_op_index) {
  if (out_channel)
    *out_channel = -1;
  if (!npu_driver_)
    return NPUDriverStatus::OK;

  const void *effective_ptr = instr_override ? instr_override : op.instr_ptr;
  if (!effective_ptr || op.instr_size == 0)
    return NPUDriverStatus::OK;

  NPUDriverStatus st =
      npu_driver_->writeInstruction(effective_ptr, op.instr_size);
  if (st != NPUDriverStatus::OK)
    return st;

  // Matches pre-refactor dispatchOp(): trigger()/wait only fire when the
  // instruction is large enough to actually be a NeonDMAInstr — a
  // malformed/too-small DMA op gets its bytes written but is never
  // triggered (out_channel stays -1, so the caller's wait is a no-op too).
  if (op.instr_size < sizeof(NeonDMAInstr))
    return NPUDriverStatus::OK;

  const NeonDMAInstr *di = static_cast<const NeonDMAInstr *>(effective_ptr);
  int ch = di->channel;
  if (out_channel)
    *out_channel = ch;

  if (dma_profiler_ && dma_profiler_->isEnabled()) {
    DMADirection dir = (op.opcode == NeonOpcode::DMA_READ)
                           ? DMADirection::READ
                           : DMADirection::WRITE;
    uint32_t idx = (effective_op_index != UINT32_MAX) ? effective_op_index : op.index;
    dma_profiler_->onTransferStart(ch, di->size_bytes, dir, idx);
  }
  return npu_driver_->trigger();
}

NPUDriverStatus Executor::waitOpDMA(int channel) {
  if (!npu_driver_ || channel < 0)
    return NPUDriverStatus::OK;
  NPUDriverStatus st = npu_driver_->dmaWait(channel);
  if (dma_profiler_ && dma_profiler_->isEnabled())
    dma_profiler_->onTransferEnd(channel);
  return st;
}

NPUDriverStatus Executor::dispatchCPUFallback(const RuntimeOp &op) {
  if (!op.instr_ptr || op.instr_size < sizeof(NeonCPUFallbackInstr))
    return NPUDriverStatus::ERROR_UNSUPPORTED_OP;

  const auto *instr = static_cast<const NeonCPUFallbackInstr *>(op.instr_ptr);
  NeonCPUFallbackFn fn = CPUFallbackRegistry::get().lookup(instr->fallback_id);
  if (!fn) {
    fprintf(stderr,
            "[ERROR] Executor: no CPU fallback registered for fallback_id=%u "
            "(op index=%u) — this build is missing a required linked "
            "implementation, see docs/hybrid_npu_cpu_fallback_design.md\n",
            static_cast<uint32_t>(instr->fallback_id), static_cast<uint32_t>(op.index));
    return NPUDriverStatus::ERROR_UNSUPPORTED_OP;
  }

  // NOTE: no notifyOpStart()/notifyOpEnd() here — those read
  // npu_driver_->readPerfTotalCycles()/readPerfDMAStalls(), which has no
  // relevance to CPU-side compute bypassing the NPU entirely. CPU-fallback
  // ops are currently invisible in MeasuredProfiler reports — a known
  // gap, not solved here (see docs/hybrid_npu_cpu_fallback_design.md).
  //
  // fn resolves whatever it needs itself (input/output tensor pointers
  // via model.tensors[op.*_tensor_idx], weight/bias via
  // model.weight_buffer, and any extra shape/quant params by
  // re-interpreting op.instr_ptr as its own fallback_id's real struct —
  // see NeonCPUFallbackConv2DInstr in core/NPU_ISA.h for the pattern).
  bool ok = fn(op, *model_);
  return ok ? NPUDriverStatus::OK : NPUDriverStatus::ERROR_INVALID_ADDR;
}


NPUDriverStatus Executor::dispatchOp(const RuntimeOp &op, const void *instr_override,
                                     uint32_t effective_op_index) {
  // CPU fallback bypasses npu_driver_ entirely — checked first so this
  // still works even when npu_driver_ is null (e.g. a static-analysis-only
  // Executor — see ModelLoader::load()'s own npu_driver=nullptr mode).
  if (op.opcode == NeonOpcode::CPU_FALLBACK) {
    if (instr_override) {
      RuntimeOp op_copy = op;
      op_copy.instr_ptr = instr_override;
      return dispatchCPUFallback(op_copy);
    }
    return dispatchCPUFallback(op);
  }

  if (!npu_driver_)
    return NPUDriverStatus::OK;

  if (op.opcode == NeonOpcode::DMA_READ || op.opcode == NeonOpcode::DMA_WRITE) {
    int ch = -1;
    NPUDriverStatus st = startOpDMA(op, &ch, instr_override, effective_op_index);
    if (st != NPUDriverStatus::OK) {
      uart_put_u32("[Executor] startOpDMA failed with st=",
                   static_cast<uint32_t>(st), "\r\n");
      return st;
    }
    st = waitOpDMA(ch);
    if (st != NPUDriverStatus::OK) {
      uart_put_u32("[Executor] waitOpDMA failed with st=",
                   static_cast<uint32_t>(st), "\r\n");
    }
    return st;
  }

  const void *effective_instr_ptr = instr_override ? instr_override : op.instr_ptr;
  if (!effective_instr_ptr || op.instr_size == 0)
    return NPUDriverStatus::OK;

  NPUDriverStatus st = npu_driver_->writeInstructionEx(
      effective_instr_ptr, op.instr_size, op.input_tensor_idx, op.output_tensor_idx);
  if (st != NPUDriverStatus::OK) {
    uart_put_u32("[Executor] writeInstructionEx failed with st=",
                 static_cast<uint32_t>(st), "\r\n");
    return st;
  }

  notifyOpStart(op, effective_op_index);

  st = npu_driver_->trigger();
  if (st != NPUDriverStatus::OK) {
    notifyOpEnd(op, 0, 0);
    uart_put_u32("[Executor] trigger failed with st=",
                 static_cast<uint32_t>(st), "\r\n");
    return st;
  }
  st = npu_driver_->waitDone();
  if (st != NPUDriverStatus::OK) {
    notifyOpEnd(op, 0, 0);
    uart_put_u32("[Executor] waitDone failed with st=",
                 static_cast<uint32_t>(st), "\r\n");
    return st;
  }

  uint32_t idx = (effective_op_index != UINT32_MAX) ? effective_op_index : op.index;
  size_t read_bytes = dma_profiler_ ? dma_profiler_->bytesForOp(idx) : 0;
  size_t write_bytes = 0;
  notifyOpEnd(op, read_bytes, write_bytes);

  return st;
}

// ============================================================
// run — single-model blocking inference with loop support
// ============================================================

struct LoopFrame {
  const NeonLoopBeginInstr *hdr;
  const NeonLoopPatch *patches;
  uint32_t iv[NEON_LOOP_MAX_LEVELS];
  uint32_t body_first_op;
};


NPUDriverStatus Executor::run(const void *input, void *output) {
  if (!model_)
    return NPUDriverStatus::ERROR_INVALID_ADDR;

  copyInput(input);

  // NOTE: kMaxLoopNesting = 4 is the ISA limit. Current runtime only supports
  // flat (non-nested) loops. Nested support requires applying patches from ALL
  // enclosing frames, not just the innermost. See F6.
  constexpr size_t kMaxLoopNesting = 4;
  LoopFrame loop_stack[kMaxLoopNesting];
  size_t loop_depth = 0;

  alignas(16) static uint8_t scratch_instr[256];

  const auto &ops = model_->ops;
  size_t num_ops = ops.size();
  size_t op_idx = 0;

  while (op_idx < num_ops) {
    const auto &op = ops[op_idx];

    if (op.opcode == NeonOpcode::LOOP_BEGIN) {
      if (!op.instr_ptr || op.instr_size < sizeof(NeonLoopBeginInstr)) {
        return NPUDriverStatus::ERROR_UNSUPPORTED_OP;
      }
      if (loop_depth >= kMaxLoopNesting || loop_depth > 0) {
        uart_puts("[Executor] ERROR: Nested LOOP_BEGIN not supported yet\r\n");
        return NPUDriverStatus::ERROR_UNSUPPORTED_OP;
      }
      const auto *begin_hdr = static_cast<const NeonLoopBeginInstr *>(op.instr_ptr);
      const auto *patches = reinterpret_cast<const NeonLoopPatch *>(
          static_cast<const uint8_t *>(op.instr_ptr) + sizeof(NeonLoopBeginInstr));

      // F5: Check for zero trip count — skip entire loop body to matching LOOP_END
      bool has_zero_trip = false;
      for (size_t l = 0; l < begin_hdr->num_levels && l < NEON_LOOP_MAX_LEVELS; ++l) {
        if (begin_hdr->trip_count[l] == 0) {
          has_zero_trip = true;
          break;
        }
      }
      if (has_zero_trip) {
        size_t skip = op_idx + 1;
        while (skip < num_ops && ops[skip].opcode != NeonOpcode::LOOP_END) {
          ++skip;
        }
        op_idx = (skip < num_ops) ? skip + 1 : skip;
        continue;
      }

      LoopFrame &frame = loop_stack[loop_depth++];
      frame.hdr = begin_hdr;
      frame.patches = patches;
      frame.body_first_op = static_cast<uint32_t>(op_idx + 1);
      for (size_t l = 0; l < NEON_LOOP_MAX_LEVELS; ++l) {
        frame.iv[l] = 0;
      }
      op_idx++;
      continue;
    }

    if (op.opcode == NeonOpcode::LOOP_END) {
      if (loop_depth == 0) {
        uart_puts("[Executor] ERROR: LOOP_END encountered with empty loop stack!\r\n");
        return NPUDriverStatus::ERROR_UNSUPPORTED_OP;
      }
      LoopFrame &frame = loop_stack[loop_depth - 1];
      int lvl = static_cast<int>(frame.hdr->num_levels) - 1;
      while (lvl >= 0) {
        frame.iv[lvl]++;
        if (frame.iv[lvl] < frame.hdr->trip_count[lvl]) {
          break;
        }
        frame.iv[lvl] = 0;
        lvl--;
      }

      if (lvl >= 0) {
        // Loop continues -> jump back to first body op
        op_idx = frame.body_first_op;
      } else {
        // Loop iteration complete -> pop frame
        loop_depth--;
        op_idx++;
      }
      continue;
    }

    // Regular op or body op inside loop
    if (loop_depth > 0) {
      const LoopFrame &frame = loop_stack[loop_depth - 1];
      uint8_t b_idx = static_cast<uint8_t>(op_idx - frame.body_first_op);

      // F7: Null check before reading or copying op.instr_ptr
      if (!op.instr_ptr || op.instr_size == 0) {
        op_idx++;
        continue;
      }

      if (op.instr_size > sizeof(scratch_instr)) {
        return NPUDriverStatus::ERROR_UNSUPPORTED_OP;
      }
      std::memcpy(scratch_instr, op.instr_ptr, op.instr_size);

      // Apply patches for this body op
      for (uint8_t pi = 0; pi < frame.hdr->num_patches; ++pi) {
        const auto &p = frame.patches[pi];
        if (p.body_op != b_idx)
          continue;

        if (p.kind == NEON_LOOP_PATCH_AFFINE) {
          int64_t delta = 0;
          for (uint8_t l = 0; l < frame.hdr->num_levels; ++l) {
            delta += static_cast<int64_t>(frame.iv[l]) * p.step[l];
          }
          if (p.field_width == 8) {
            uint64_t val = 0;
            std::memcpy(&val, scratch_instr + p.field_offset, 8);
            val = static_cast<uint64_t>(static_cast<int64_t>(val) + delta);
            std::memcpy(scratch_instr + p.field_offset, &val, 8);
          } else if (p.field_width == 4) {
            uint32_t val = 0;
            std::memcpy(&val, scratch_instr + p.field_offset, 4);
            val = static_cast<uint32_t>(static_cast<int32_t>(val) + delta);
            std::memcpy(scratch_instr + p.field_offset, &val, 4);
          }
        } else if (p.kind == NEON_LOOP_PATCH_TABLE) {
          uint32_t tbl_idx = frame.iv[p.table_level];
          if (model_->weight_buffer) {
            const uint8_t *tbl_ptr =
                reinterpret_cast<const uint8_t *>(model_->weight_buffer) +
                p.table_offset + tbl_idx * p.field_width;
            std::memcpy(scratch_instr + p.field_offset, tbl_ptr, p.field_width);
          }
        }
      }

      // F9: Loop-aware synthetic op index for profiler tracking
      uint32_t loop_iteration = 0;
      if (frame.hdr->num_levels >= 1)
        loop_iteration = frame.iv[0];
      if (frame.hdr->num_levels >= 2)
        loop_iteration = loop_iteration * frame.hdr->trip_count[1] + frame.iv[1];
      uint32_t effective_idx = (loop_iteration * 10000) + op.index;

      NPUDriverStatus st = dispatchOp(op, scratch_instr, effective_idx);
      if (st != NPUDriverStatus::OK)
        return st;
    } else {
      NPUDriverStatus st = dispatchOp(op);
      if (st != NPUDriverStatus::OK)
        return st;
    }

    op_idx++;
  }

  copyOutput(output);
  return NPUDriverStatus::OK;
}

// ============================================================
// runDoubleBuffering — host-managed double buffering
// Assumes model_->ops = [DMA_READ(0), COMPUTE(0), DMA_READ(1), COMPUTE(1), ...]
// ============================================================

NPUDriverStatus Executor::runDoubleBuffering(const void *input, void *output) {
  if (!model_ || !npu_driver_)
    return NPUDriverStatus::ERROR_INVALID_ADDR;

  copyInput(input);

  const auto &ops = model_->ops;
  size_t n_pairs = ops.size() / 2;
  if (n_pairs == 0) {
    copyOutput(output);
    return NPUDriverStatus::OK;
  }

  NPUDriverStatus st;
  int prefetch_ch = -1;

  // Prologue: issue + wait tile 0's DMA_READ before anything can compute.
  st = startOpDMA(ops[0], &prefetch_ch);
  if (st != NPUDriverStatus::OK)
    return st;
  st = waitOpDMA(prefetch_ch);
  if (st != NPUDriverStatus::OK)
    return st;

  for (size_t i = 0; i < n_pairs; ++i) {
    const RuntimeOp &dma_op =
        ops[2 * i]; // this tile's own DMA_READ (already landed)
    const RuntimeOp &compute_op = ops[2 * i + 1];
    if (!compute_op.instr_ptr || compute_op.instr_size == 0)
      return NPUDriverStatus::OK;

    // Start compute i.
    st = npu_driver_->writeInstructionEx(
        compute_op.instr_ptr, compute_op.instr_size,
        compute_op.input_tensor_idx, compute_op.output_tensor_idx);
    if (st != NPUDriverStatus::OK)
      return st;
    notifyOpStart(compute_op);
    st = npu_driver_->trigger();
    if (st != NPUDriverStatus::OK)
      return st;

    // Concurrently prefetch tile i+1's input while compute i runs.
    bool has_next = (i + 1 < n_pairs);
    if (has_next) {
      st = startOpDMA(ops[2 * (i + 1)], &prefetch_ch);
      if (st != NPUDriverStatus::OK)
        return st;
    }

    // Wait for compute i to finish.
    st = npu_driver_->waitDone();
    // bytesForOp() must be keyed by the DMA op's own index (2*i), not
    // compute_op's index (2*i+1) — onTransferStart() in startOpDMA() always
    // tags the event with the DMA RuntimeOp's index, never the compute
    // op's, so looking this up by compute_op.index would silently always
    // return 0 (real bug, caught in review — never actually recorded there).
    size_t read_bytes =
        dma_profiler_ ? dma_profiler_->bytesForOp(dma_op.index) : 0;
    notifyOpEnd(compute_op, read_bytes, 0);
    if (st != NPUDriverStatus::OK)
      return st;

    // Wait for tile i+1's prefetch to land (usually already hidden behind
    // compute i).
    if (has_next) {
      st = waitOpDMA(prefetch_ch);
      if (st != NPUDriverStatus::OK)
        return st;
    }
  }

  copyOutput(output);
  return NPUDriverStatus::OK;
}

// ============================================================
// buildQueueBuffer — flatten model ops into one instruction buffer
// ============================================================

void *Executor::buildQueueBuffer(NPUModel *model, size_t *out_size) const {
  // Sum all instruction sizes
  size_t total = 0;
  for (const auto &op : model->ops)
    total += op.instr_size;
  if (total == 0) {
    *out_size = 0;
    return nullptr;
  }

  uint8_t *buf = static_cast<uint8_t *>(std::malloc(total));
  if (!buf) {
    *out_size = 0;
    return nullptr;
  }

  size_t off = 0;
  for (const auto &op : model->ops) {
    if (op.instr_ptr && op.instr_size > 0) {
      std::memcpy(buf + off, op.instr_ptr, op.instr_size);
      off += op.instr_size;
    }
  }
  *out_size = total;
  return buf;
}

// ============================================================
// runConcurrent — dual-model spatial split
// ============================================================

NPUDriverStatus Executor::runConcurrent(const void *input_a, void *output_a,
                                        NPUModel *model_b, const void *input_b,
                                        void *output_b, uint8_t n_split) {
  if (!model_ || !model_b || !npu_driver_)
    return NPUDriverStatus::ERROR_INVALID_ADDR;

  // NPUHWSpecs::max_concurrent_models (via NPUComponent) is the single
  // source of truth — NPUDriver has no copy of this to fall back to.
  // No npu_component_ means we don't know the hardware's concurrency
  // support, so assume the safe default (serialize).
  uint8_t max_concurrent =
      npu_component_ ? npu_component_->maxConcurrentModels() : 1;
  if (max_concurrent < 2) {
    // Hardware fallback: serialize
    NPUDriverStatus st = run(input_a, output_a);
    if (st != NPUDriverStatus::OK)
      return st;
    // Swap to model_b for second run
    NPUModel *saved = model_;
    model_ = model_b;
    st = run(input_b, output_b);
    model_ = saved;
    return st;
  }

  // Set N_split
  npu_driver_->setNSplit(n_split);

  // Copy inputs to scratch buffers
  copyInput(input_a);
  {
    // Temporarily swap model to copy input_b
    NPUModel *saved = model_;
    model_ = model_b;
    copyInput(input_b);
    model_ = saved;
  }

  // Build queue buffers for both models
  size_t sz_a = 0, sz_b = 0;
  void *qa = buildQueueBuffer(model_, &sz_a);
  void *qb = buildQueueBuffer(model_b, &sz_b);

  NPUDriverStatus st = npu_driver_->submitConcurrent(qa, sz_a, qb, sz_b);

  if (qa)
    std::free(qa);
  if (qb)
    std::free(qb);

  if (st != NPUDriverStatus::OK)
    return st;

  // Copy outputs back
  copyOutput(output_a);
  {
    NPUModel *saved = model_;
    model_ = model_b;
    copyOutput(output_b);
    model_ = saved;
  }

  return NPUDriverStatus::OK;
}

// ============================================================
// run_batch — batch profile execution (Rev3)
// ============================================================

NPUDriverStatus Executor::run_batch(const BatchProfile &profile,
                                    const void *packed_input,
                                    void *packed_output) {
  if (!npu_driver_)
    return NPUDriverStatus::ERROR_INVALID_ADDR;
  if (profile.instruction_buffer.empty())
    return NPUDriverStatus::ERROR_INVALID_ADDR;

  // Allocate batch scratch buffer
  void *scratch = std::malloc(profile.scratch_buffer_size);
  if (!scratch)
    return NPUDriverStatus::ERROR_INVALID_ADDR;

  // Copy packed input to scratch[input_tensor offset]
  // Use mem_plan to find input tensor offset
  size_t in_off = 0;
  for (const auto &e : profile.mem_plan) {
    if (e.tensor_idx == profile.input_tensor_idx) {
      in_off = (size_t)e.offset;
      break;
    }
  }
  size_t in_sz = profile.single_input_size * profile.batch_size;
  if (in_sz > 0 && packed_input)
    std::memcpy(static_cast<uint8_t *>(scratch) + in_off, packed_input, in_sz);

  // Write the batch instruction stream and trigger
  NPUDriverStatus st = npu_driver_->writeInstruction(
      profile.instruction_buffer.data(), profile.instruction_buffer.size());

  if (st == NPUDriverStatus::OK) {
    // Profile wrapper (single timing record for entire batch)
    PerfCounterSnapshot snap0 = readPerfSnapshot(npu_driver_);
    if (profiler_ && profiler_->isEnabled()) {
      RuntimeOp fake{};
      fake.index = UINT32_MAX;
      fake.opcode = NeonOpcode::CONV2D;
      fake.name = "batch_profile_" + std::to_string(profile.batch_size);
      profiler_->onOpStart(fake.index, fake.opcode, fake.name, 255, snap0);
    }

    st = npu_driver_->trigger();
    if (st == NPUDriverStatus::OK)
      st = npu_driver_->waitDone();

    PerfCounterSnapshot snap1 = readPerfSnapshot(npu_driver_);
    if (profiler_ && profiler_->isEnabled())
      profiler_->onOpEnd(0, 0, snap1);
  }

  // Copy packed output from scratch
  size_t out_off = 0;
  for (const auto &e : profile.mem_plan) {
    if (e.tensor_idx == profile.output_tensor_idx) {
      out_off = (size_t)e.offset;
      break;
    }
  }
  size_t out_sz = profile.single_output_size * profile.batch_size;
  if (out_sz > 0 && packed_output)
    std::memcpy(packed_output, static_cast<uint8_t *>(scratch) + out_off,
                out_sz);

  std::free(scratch);
  return st;
}
