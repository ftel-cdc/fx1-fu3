#pragma once

#include "NPUV4OBPConfig.h"

// Forward declaration of functions to write MMIO
class NPUV4MMIOWriter {
public:
    // Ghi thanh ghi compatibility bank (Requant + Bias + Activation Clamp)
    static void writeCompatibilityBank(uintptr_t npu_base, const NeonJobContext& ctx);

    // Ghi OBP LUT (cho SiLU, GELU nếu act_mode != RELU)
    static void writeOBPLUT(uintptr_t npu_base, uint8_t act_mode,
                            const uint8_t* lut_table, size_t lut_size);
};
