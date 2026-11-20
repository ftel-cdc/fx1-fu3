#pragma once
#include "NPUGraph.h"

#ifdef __cplusplus
extern "C++" {

// Lifetime-aware placement of INTERNAL tensors (best-fit with a coalescing
// free list). A tensor is live over [producer, last_consumer]; its block is
// reusable starting at fragment last_consumer + 1. A fragment's output can
// therefore never alias any of its own inputs. Writes GraphTensor::arena_offset
// and NPUGraph::arena_bytes; returns arena_bytes.
// Views (NEON_GRAPH_TENSOR_ALIAS) get no space of their own: they live inside their parent, whose producer /
// last_consumer the linker sets to cover them. A parent may have no producing fragment (an elided concat): it is
// placed at its producer field, the first fragment that writes one of its views.
class ArenaPlanner {
public:
    static size_t plan(NPUGraph& graph, size_t alignment = NEON_GRAPH_ARENA_ALIGN);
};

} // extern "C++"
#endif
