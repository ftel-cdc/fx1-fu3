#pragma once

// ============================================================
// Pointer resolution for CPU-fallback src/dst fields.
//
// Contract (core/NPU_ISA.h + runtime/binder/OperandBinder): a NON-ZERO field
// is an absolute host address (already rebased by OperandBinder or by the
// device's NPUDriver::relocateInstruction); ZERO means "this op's own
// input/output tensor" from the dispatch entry. Returns nullptr if neither is
// available — the caller must fail the op. Never substitute a device address.
// ============================================================

#include "../loader/NPUModel.h"
#include <stdint.h>

namespace neon_fb {

inline void* tensorData(const NPUModel& m, uint32_t idx) {
    return idx < m.tensors.size() ? m.tensors[idx].data : nullptr;
}

template <typename T>
inline T* srcPtr(const RuntimeOp& op, const NPUModel& m, uint64_t field) {
    return field ? reinterpret_cast<T*>(static_cast<uintptr_t>(field))
                 : static_cast<T*>(tensorData(m, op.input_tensor_idx));
}

template <typename T>
inline T* dstPtr(const RuntimeOp& op, const NPUModel& m, uint64_t field) {
    return field ? reinterpret_cast<T*>(static_cast<uintptr_t>(field))
                 : static_cast<T*>(tensorData(m, op.output_tensor_idx));
}

} // namespace neon_fb
