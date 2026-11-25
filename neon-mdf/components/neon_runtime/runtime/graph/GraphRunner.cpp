#include "GraphRunner.h"
#include "../binder/OperandBinder.h"
#include "../executor/Executor.h"
#include "../loader/ModelLoader.h"
#include "../support/NeonLog.h"

NPUDriverStatus GraphRunner::prepare(const GraphRunConfig& cfg) {
    cfg_ = cfg;
    prepared_ = false;
    if (!g_ || !driver_) return NPUDriverStatus::ERROR_INVALID_ADDR;
    if (g_->arena_bytes > 0 && (!cfg.arena || cfg.arena_bytes < g_->arena_bytes)) {
        uart_put_u32("[GraphRunner] ERROR: tensor arena too small, need ", (uint32_t)g_->arena_bytes, "");
        uart_put_u32(" have ", (uint32_t)cfg.arena_bytes, "\r\n");
        return NPUDriverStatus::ERROR_INVALID_ADDR;
    }
    if (g_->max_scratch_bytes > 0 && (!cfg.scratch || cfg.scratch_bytes < g_->max_scratch_bytes)) {
        uart_put_u32("[GraphRunner] ERROR: scratch too small, need ", (uint32_t)g_->max_scratch_bytes, "");
        uart_put_u32(" have ", (uint32_t)cfg.scratch_bytes, "\r\n");
        return NPUDriverStatus::ERROR_INVALID_ADDR;
    }
    prepared_ = true;
    return NPUDriverStatus::OK;
}

uint8_t* GraphRunner::tensorAddress(uint32_t tid) const {
    if (tid == NEON_GRAPH_NO_TENSOR || tid >= g_->tensors.size()) return nullptr;
    const GraphTensor& t = g_->tensors[tid];
    if (t.kind == NEON_GRAPH_TENSOR_ALIAS) {          // view: parent + offset (the loader rejects nested views)
        uint8_t* p = tensorAddress(t.alias_of);
        return p ? p + t.alias_offset : nullptr;
    }
    if (t.kind == NEON_GRAPH_TENSOR_INPUT) {
        for (size_t k = 0; k < g_->inputs.size(); ++k)
            if (g_->inputs[k] == tid)
                return const_cast<uint8_t*>(static_cast<const uint8_t*>(inputs_[k]));
        return nullptr;
    }
    if (t.kind == NEON_GRAPH_TENSOR_OUTPUT) {
        for (size_t k = 0; k < g_->outputs.size(); ++k)
            if (g_->outputs[k] == tid) return static_cast<uint8_t*>(outputs_[k]);
        return nullptr;
    }
    if (t.kind == NEON_GRAPH_TENSOR_CONST)
        return const_cast<uint8_t*>(t.data);   // read-only operand; ops write only their output
    return cfg_.arena + t.arena_offset;
}

NPUDriverStatus GraphRunner::run(const void* const* inputs, uint32_t num_inputs,
                                 void* const* outputs, uint32_t num_outputs) {
    if (!prepared_) return NPUDriverStatus::ERROR_INVALID_ADDR;
    if (num_inputs != g_->inputs.size() || num_outputs != g_->outputs.size()) {
        uart_puts("[GraphRunner] ERROR: I/O count does not match the graph\r\n");
        return NPUDriverStatus::ERROR_INVALID_ADDR;
    }
    for (uint32_t i = 0; i < num_inputs; ++i)  if (!inputs[i])  return NPUDriverStatus::ERROR_INVALID_ADDR;
    for (uint32_t i = 0; i < num_outputs; ++i) if (!outputs[i]) return NPUDriverStatus::ERROR_INVALID_ADDR;
    inputs_  = inputs;
    outputs_ = outputs;
    for (uint32_t i = 0; i < g_->fragments.size(); ++i) {
        const NPUDriverStatus st = runFragment(i);
        if (st != NPUDriverStatus::OK) {
            uart_put_u32("[GraphRunner] ERROR: fragment ", i, "");
            uart_put_u32(" failed, status=", (uint32_t)st, "\r\n");
            return st;
        }
    }
    return NPUDriverStatus::OK;
}

NPUDriverStatus GraphRunner::runFragment(uint32_t idx) {
    const GraphFragment& f = g_->fragments[idx];

    FragmentBinding b{};
    b.num_inputs = f.num_inputs;
    for (uint32_t k = 0; k < f.num_inputs; ++k) b.inputs[k] = tensorAddress(f.input_tensors[k]);
    b.output        = tensorAddress(f.output_tensor);
    b.output_bytes  = (size_t)g_->tensors[f.output_tensor].size_bytes;
    b.scratch       = cfg_.scratch;
    b.scratch_bytes = cfg_.scratch_bytes;
    if (!b.output) return NPUDriverStatus::ERROR_INVALID_ADDR;

    const uintptr_t mark = cfg_.heap_mark ? cfg_.heap_mark() : 0;
    NPUModel* m = ModelLoader::loadFromMemory(f.nsm, f.nsm_size, target_, driver_,
                                              cfg_.scratch, cfg_.scratch_bytes,
                                              nullptr, 0, f.num_inputs,
                                              !driver_->deferWeightUploadToFirstUse());
    if (!m) {
        if (cfg_.heap_release) cfg_.heap_release(mark);
        return NPUDriverStatus::ERROR_INVALID_ADDR;
    }
    b.weights      = static_cast<uint8_t*>(m->weight_buffer);
    b.weight_bytes = m->weight_buffer_size;

    NPUDriverStatus st = OperandBinder::bind(*m, b, driver_);
    if (st == NPUDriverStatus::OK) st = driver_->beginFragment(m, b);
    if (cfg_.observer) cfg_.observer->onFragmentBegin(idx, f, *m);
    if (st == NPUDriverStatus::OK) {
        Executor ex(m, component_, driver_, cfg_.profiler, cfg_.dma_profiler);
        st = ex.run(nullptr, nullptr);   // I/O already bound in place
    }
    driver_->endFragment();
    if (cfg_.observer) cfg_.observer->onFragmentEnd(idx, f, *m, st, b.output, b.output_bytes);

    ModelLoader::unload(m);
    if (cfg_.heap_release) cfg_.heap_release(mark);
    return st;
}
