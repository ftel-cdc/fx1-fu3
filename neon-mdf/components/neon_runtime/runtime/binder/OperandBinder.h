#pragma once
#include "../loader/NPUModel.h"
#include "../driver/NPUDriver.h"

#ifdef __cplusplus
extern "C++" {

// Binds one freshly loaded fragment to its run-time buffers:
//   1. NPUModel::tensors[k].data  <- operand k, input_tensor_idx <- operand 0,
//      output_tensor_idx <- output                     (fragment-local indices)
//   2. CPU_FALLBACK src/dst fields that are 0 (ISA: "the op's own tensor")
//      <- the bound tensor address; concat operand k <- operand k
//   3. NPUDriver::relocateInstruction() for every op (device conventions)
// Must run exactly once per ModelLoader::loadFromMemory() result: step 2/3
// patch the model's private instruction copy in place.
class OperandBinder {
public:
    static NPUDriverStatus bind(NPUModel& model, const FragmentBinding& b, NPUDriver* driver);

    // Address of fragment-local tensor `idx` under `b`. Resolution order is
    // the one validated in segment mode (old main.cpp getTensorPtr), including
    // its final default to the output buffer for unresolved indices (e.g. the
    // >=1000 SRAM tile descriptors) — keep it identical.
    static uint8_t* tensorPtr(const NPUModel& model, const FragmentBinding& b, uint32_t idx);

private:
    static void bindTensors(NPUModel& model, const FragmentBinding& b);
    static void fillFallbackOperands(NPUModel& model, const FragmentBinding& b);
};

} // extern "C++"
#endif
