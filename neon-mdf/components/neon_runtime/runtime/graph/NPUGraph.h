#pragma once

// ============================================================
// NEON STUDIO — NPU GRAPH
// In-memory view of a graph container (or of a single-program .nsm wrapped as
// a 1-fragment graph). Fragment programs are NOT parsed here: GraphRunner
// loads each one on demand with ModelLoader::loadFromMemory so peak heap is
// one fragment, not the whole model.
// ============================================================

#include "../../core/NPU_ISA.h"

#ifdef __cplusplus
extern "C++" {

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

// Arena placement granularity (runtime choice, not on disk).
#define NEON_GRAPH_ARENA_ALIGN 64u

struct GraphTensor {
    uint64_t size_bytes    = 0;
    uint32_t producer      = NEON_GRAPH_NO_TENSOR;
    uint32_t last_consumer = 0;
    uint8_t  kind          = NEON_GRAPH_TENSOR_INTERNAL;
    uint64_t arena_offset  = 0;     // INTERNAL only; set by ArenaPlanner
    uint32_t alias_of      = NEON_GRAPH_NO_TENSOR;   // ALIAS only: parent tensor (never itself an ALIAS)
    uint64_t alias_offset  = 0;                      // ALIAS only: byte offset inside the parent
    const uint8_t* data    = nullptr;   // CONST only: points into the container buffer
};

struct GraphFragment {
    const uint8_t* nsm           = nullptr;   // points into the caller's (possibly ROM) buffer
    size_t         nsm_size      = 0;
    uint32_t       output_tensor = NEON_GRAPH_NO_TENSOR;
    uint32_t       num_inputs    = 0;
    uint32_t       input_tensors[NEON_GRAPH_MAX_FRAG_INPUTS];
    uint32_t       source_index  = 0;
    size_t         scratch_bytes = 0;         // NeonFileHeader::scratch_buffer_size

    GraphFragment() {
        for (uint32_t k = 0; k < NEON_GRAPH_MAX_FRAG_INPUTS; ++k)
            input_tensors[k] = NEON_GRAPH_NO_TENSOR;
    }
};

struct NPUGraph {
    std::string                target_device;
    bool                       single_program = false;  // wrapped NEONNSM
    std::vector<GraphTensor>   tensors;
    std::vector<GraphFragment> fragments;                // execution order
    std::vector<uint32_t>      inputs;                   // graph tensor ids
    std::vector<uint32_t>      outputs;
    size_t                     max_scratch_bytes = 0;    // largest fragment scratch
    size_t                     arena_bytes       = 0;    // ArenaPlanner high-water mark

    size_t inputSize(uint32_t i)  const { return i < inputs.size()  ? (size_t)tensors[inputs[i]].size_bytes  : 0; }
    size_t outputSize(uint32_t i) const { return i < outputs.size() ? (size_t)tensors[outputs[i]].size_bytes : 0; }
};

} // extern "C++"
#endif
