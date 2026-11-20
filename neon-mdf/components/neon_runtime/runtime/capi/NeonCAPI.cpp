// ============================================================
// NEON STUDIO — Runtime Open ABI (neon/c_runtime_api.h)
// Thin C wrapper over GraphLoader / ArenaPlanner / GraphRunner. Runtime
// behaviour is unchanged: same loader, same planner, same runner, same driver.
// ============================================================
#include <neon/c_runtime_api.h>

#include "NeonCAPIExt.h"
#include "NeonPlatform.h"
#include "../graph/GraphLoader.h"
#include "../graph/GraphRunner.h"

#include <cstring>
#include <string>

namespace {

char g_err[256] = "";
int32_t g_status = 0;

int fail(const char* msg) {
  std::strncpy(g_err, msg ? msg : "error", sizeof(g_err) - 1);
  g_err[sizeof(g_err) - 1] = 0;
  return -1;
}

// Forwards fragment events to an optional C++ observer and an optional C callback.
class ChainObserver final : public GraphObserver {
 public:
  GraphObserver* cpp = nullptr;
  NeonFragmentCallback cb = nullptr;
  void* user = nullptr;
  bool active() const { return cpp || cb; }
  void onFragmentBegin(uint32_t idx, const GraphFragment& f, const NPUModel& m) override {
    if (cpp) cpp->onFragmentBegin(idx, f, m);
    if (cb) cb(user, idx, 0, 0, nullptr, 0);
  }
  void onFragmentEnd(uint32_t idx, const GraphFragment& f, const NPUModel& m,
                     NPUDriverStatus st, const uint8_t* out, size_t out_bytes) override {
    if (cpp) cpp->onFragmentEnd(idx, f, m, st, out, out_bytes);
    if (cb) cb(user, idx, 1, static_cast<int32_t>(st), out, out_bytes);
  }
};

// Validates a DLTensor against the expected byte size; returns the data pointer or nullptr.
uint8_t* tensorData(const DLTensor* t, size_t expect, const char* what) {
  if (!t || !t->data) { fail(what); return nullptr; }
  if (t->device.device_type != kDLCPU) { fail("DLTensor device must be kDLCPU"); return nullptr; }
  if (t->dtype.lanes != 1 || t->dtype.bits == 0 || (t->dtype.bits % 8) != 0) {
    fail("DLTensor dtype must have lanes == 1 and a whole number of bytes");
    return nullptr;
  }
  uint64_t elems = 1;
  for (int32_t d = 0; d < t->ndim; ++d) {
    if (t->shape[d] < 0) { fail("DLTensor has a negative dimension"); return nullptr; }
    elems *= static_cast<uint64_t>(t->shape[d]);
  }
  if (t->strides) {  // must be compact row-major
    int64_t expect_stride = 1;
    for (int32_t d = t->ndim - 1; d >= 0; --d) {
      if (t->shape[d] != 1 && t->strides[d] != expect_stride) {
        fail("DLTensor strides must be compact row-major");
        return nullptr;
      }
      expect_stride *= t->shape[d];
    }
  }
  const uint64_t bytes = elems * (t->dtype.bits / 8u);
  if (expect != 0 && bytes != expect) { fail("DLTensor byte size does not match the graph I/O size"); return nullptr; }
  return static_cast<uint8_t*>(t->data) + t->byte_offset;
}

}  // namespace

struct NeonGraph {
  NPUGraph*         graph = nullptr;
  NeonPlatform      platform{nullptr, nullptr, nullptr};
  bool              has_platform = false;
  GraphRunner*      runner = nullptr;
  ChainObserver     observer;
  MeasuredProfiler* profiler = nullptr;
  DMAProfiler*      dma_profiler = nullptr;
  bool              prepared = false;
};

extern "C" {

int32_t NeonRuntimeAbiVersion(void) { return NEON_RUNTIME_ABI_VERSION; }
const char* NeonRuntimeGetLastError(void) { return g_err; }
int32_t NeonRuntimeGetLastDriverStatus(void) { return g_status; }

int NeonGraphLoad(const uint8_t* buf, size_t size, const NeonPlatform* platform,
                  uint32_t single_program_inputs, NeonGraphHandle* out) {
  if (!buf || !out) return fail("NeonGraphLoad: NULL argument");
  *out = nullptr;
  std::string err;
  NPUGraph* g = GraphLoader::loadFromMemory(buf, size, platform ? platform->target : nullptr,
                                            single_program_inputs, &err);
  if (!g) return fail(err.empty() ? "GraphLoader::loadFromMemory failed" : err.c_str());
  NeonGraph* h = new NeonGraph();
  h->graph = g;
  if (platform) { h->platform = *platform; h->has_platform = true; }
  *out = h;
  return 0;
}

int NeonGraphGetInfo(NeonGraphHandle h, NeonGraphInfo* info) {
  if (!h || !info) return fail("NeonGraphGetInfo: NULL argument");
  if (info->struct_size < sizeof(NeonGraphInfo)) return fail("NeonGraphGetInfo: struct_size too small");
  const NPUGraph* g = h->graph;
  info->num_inputs     = static_cast<int32_t>(g->inputs.size());
  info->num_outputs    = static_cast<int32_t>(g->outputs.size());
  info->num_fragments  = static_cast<int32_t>(g->fragments.size());
  info->num_tensors    = static_cast<int32_t>(g->tensors.size());
  info->single_program = g->single_program ? 1 : 0;
  info->arena_bytes    = g->arena_bytes;
  info->scratch_bytes  = g->max_scratch_bytes;
  return 0;
}

int NeonGraphGetIOBytes(NeonGraphHandle h, int32_t is_output, int32_t index, size_t* bytes) {
  if (!h || !bytes || index < 0) return fail("NeonGraphGetIOBytes: bad argument");
  const NPUGraph* g = h->graph;
  const size_t n = is_output ? g->outputs.size() : g->inputs.size();
  if (static_cast<size_t>(index) >= n) return fail("NeonGraphGetIOBytes: index out of range");
  *bytes = is_output ? g->outputSize(static_cast<uint32_t>(index)) : g->inputSize(static_cast<uint32_t>(index));
  return 0;
}

int NeonGraphSetFragmentCallback(NeonGraphHandle h, NeonFragmentCallback cb, void* user) {
  if (!h) return fail("NeonGraphSetFragmentCallback: NULL handle");
  h->observer.cb = cb;
  h->observer.user = user;
  return 0;
}

int NeonGraphPrepare(NeonGraphHandle h, const NeonGraphMemory* mem) {
  if (!h || !mem) return fail("NeonGraphPrepare: NULL argument");
  if (mem->struct_size < sizeof(NeonGraphMemory)) return fail("NeonGraphPrepare: struct_size too small");
  if (!h->has_platform) return fail("NeonGraphPrepare: handle was loaded without a platform");
  GraphRunConfig cfg;
  cfg.arena         = static_cast<uint8_t*>(mem->arena);
  cfg.arena_bytes   = mem->arena_bytes;
  cfg.scratch       = static_cast<uint8_t*>(mem->scratch);
  cfg.scratch_bytes = mem->scratch_bytes;
  cfg.profiler      = h->profiler;
  cfg.dma_profiler  = h->dma_profiler;
  cfg.observer      = h->observer.active() ? &h->observer : nullptr;
  cfg.heap_mark     = mem->heap_mark;
  cfg.heap_release  = mem->heap_release;
  if (!h->runner)
    h->runner = new GraphRunner(h->graph, h->platform.target, h->platform.component, h->platform.driver);
  const NPUDriverStatus st = h->runner->prepare(cfg);
  g_status = static_cast<int32_t>(st);
  h->prepared = (st == NPUDriverStatus::OK);
  return h->prepared ? 0 : fail("GraphRunner::prepare failed (arena/scratch too small?)");
}

int NeonGraphRun(NeonGraphHandle h, const DLTensor* const* inputs, int32_t num_inputs,
                 DLTensor* const* outputs, int32_t num_outputs) {
  if (!h || !h->prepared) return fail("NeonGraphRun: graph not prepared");
  const NPUGraph* g = h->graph;
  if (num_inputs != static_cast<int32_t>(g->inputs.size()) ||
      num_outputs != static_cast<int32_t>(g->outputs.size()))
    return fail("NeonGraphRun: input/output count does not match the graph");
  if (num_inputs > static_cast<int32_t>(NEON_GRAPH_MAX_FRAG_INPUTS) ||
      num_outputs > static_cast<int32_t>(NEON_GRAPH_MAX_FRAG_INPUTS))
    return fail("NeonGraphRun: too many inputs/outputs");
  const void* in_ptrs[NEON_GRAPH_MAX_FRAG_INPUTS] = {};
  void* out_ptrs[NEON_GRAPH_MAX_FRAG_INPUTS] = {};
  for (int32_t i = 0; i < num_inputs; ++i)
    if (!(in_ptrs[i] = tensorData(inputs[i], g->inputSize(static_cast<uint32_t>(i)), "NeonGraphRun: NULL input")))
      return -1;
  for (int32_t i = 0; i < num_outputs; ++i)
    if (!(out_ptrs[i] = tensorData(outputs[i], g->outputSize(static_cast<uint32_t>(i)), "NeonGraphRun: NULL output")))
      return -1;
  const NPUDriverStatus st = h->runner->run(in_ptrs, static_cast<uint32_t>(num_inputs),
                                            out_ptrs, static_cast<uint32_t>(num_outputs));
  g_status = static_cast<int32_t>(st);
  return st == NPUDriverStatus::OK ? 0 : fail("GraphRunner::run failed");
}

void NeonGraphFree(NeonGraphHandle h) {
  if (!h) return;
  delete h->runner;
  GraphLoader::unload(h->graph);
  delete h;
}

}  // extern "C"

int NeonGraphSetObserver(NeonGraphHandle h, GraphObserver* observer) {
  if (!h) return fail("NeonGraphSetObserver: NULL handle");
  h->observer.cpp = observer;
  return 0;
}

int NeonGraphAttachProfilers(NeonGraphHandle h, MeasuredProfiler* profiler, DMAProfiler* dma_profiler) {
  if (!h) return fail("NeonGraphAttachProfilers: NULL handle");
  h->profiler = profiler;
  h->dma_profiler = dma_profiler;
  return 0;
}
