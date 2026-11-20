#pragma once
#include "NPUGraph.h"
#include "../../core/DeviceTarget.h"

#ifdef __cplusplus
extern "C++" {

#include <string>

class GraphLoader {
public:
    // Accepts either a graph container (NEON_GRAPH_MAGIC) or a single-program
    // .nsm (NEON_MAGIC). Zero-copy: fragments point into `buf`, which must
    // outlive the graph. `single_program_inputs` is only used for NEON_MAGIC
    // input (number of operands bound to fragment-local tensors 0..n-1; the
    // single-op verifier passes input count). Runs ArenaPlanner before
    // returning. nullptr on any validation failure (reason in *err and on UART).
    static NPUGraph* loadFromMemory(const uint8_t*      buf,
                                    size_t              size,
                                    const DeviceTarget* target,
                                    uint32_t            single_program_inputs = 1,
                                    std::string*        err = nullptr);

    static void unload(NPUGraph* graph) { delete graph; }

    static bool isGraphContainer(const uint8_t* buf, size_t size);
};

} // extern "C++"
#endif
