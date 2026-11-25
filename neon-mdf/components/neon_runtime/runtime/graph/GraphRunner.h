#pragma once
#include "NPUGraph.h"
#include "../loader/NPUModel.h"
#include "../driver/NPUDriver.h"
#include "../../core/DeviceTarget.h"
#include "../../core/NPUComponent.h"
#include "../../analyse/MeasuredProfiler.h"
#include "../../analyse/DMAProfiler.h"

#ifdef __cplusplus
extern "C++" {

// Optional per-fragment callbacks (reporting, tracing). The NPUModel is the
// live fragment program: valid only for the duration of the callback.
class GraphObserver {
public:
    virtual ~GraphObserver() = default;
    virtual void onFragmentBegin(uint32_t /*idx*/, const GraphFragment& /*f*/, const NPUModel& /*m*/) {}
    virtual void onFragmentEnd(uint32_t /*idx*/, const GraphFragment& /*f*/, const NPUModel& /*m*/,
                               NPUDriverStatus /*st*/, const uint8_t* /*output*/, size_t /*output_bytes*/) {}
};

// Memory is ALWAYS supplied by the platform — the runtime never picks an
// address. Sizes are checked in prepare().
struct GraphRunConfig {
    uint8_t* arena         = nullptr;   // >= NPUGraph::arena_bytes      (INTERNAL tensors)
    size_t   arena_bytes   = 0;
    uint8_t* scratch       = nullptr;   // >= NPUGraph::max_scratch_bytes (shared; fragments run one at a time)
    size_t   scratch_bytes = 0;
    MeasuredProfiler* profiler     = nullptr;
    DMAProfiler*      dma_profiler = nullptr;
    GraphObserver*    observer     = nullptr;
    // Bump-allocator platforms: called around each fragment's load/unload so
    // the fragment's heap is reclaimed. Leave null on platforms with a real free().
    uintptr_t (*heap_mark)()             = nullptr;
    void      (*heap_release)(uintptr_t) = nullptr;
};

class GraphRunner {
public:
    GraphRunner(const NPUGraph* graph, const DeviceTarget* target,
                const NPUComponent* component, NPUDriver* driver)
        : g_(graph), target_(target), component_(component), driver_(driver) {}

    NPUDriverStatus prepare(const GraphRunConfig& cfg);

    // inputs[i] / outputs[i] follow NPUGraph::inputs / outputs order and
    // must be device-reachable memory of at least inputSize(i)/outputSize(i).
    // Inputs are not copied (zero-copy binding).
    NPUDriverStatus run(const void* const* inputs, uint32_t num_inputs,
                        void* const* outputs, uint32_t num_outputs);

private:
    uint8_t* tensorAddress(uint32_t tensor_id) const;
    NPUDriverStatus runFragment(uint32_t idx);

    const NPUGraph*     g_;
    const DeviceTarget* target_;
    const NPUComponent* component_;
    NPUDriver*          driver_;
    GraphRunConfig      cfg_{};
    bool                prepared_ = false;
    const void* const*  inputs_   = nullptr;
    void* const*        outputs_  = nullptr;
};

} // extern "C++"
#endif
