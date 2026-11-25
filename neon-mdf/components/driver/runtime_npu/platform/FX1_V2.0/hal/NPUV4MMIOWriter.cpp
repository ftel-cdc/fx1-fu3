#include "NPUV4MMIOWriter.h"
#include "soc/regs/soc_regs_npu_v4.h"
#include "../FX1V42Regs.h"

void NPUV4MMIOWriter::writeCompatibilityBank(uintptr_t npu_base, const NeonJobContext& ctx) {
    if (!ctx.has_requantization) return;

    // V4.5 CDC software GEMM bank: fixed 32x32 output, K <= 992
    // (npu_tlm.cpp validate_job / kSoftwareMaxK).
    *((volatile uint32_t*)(npu_base + CDC_NPU_WIDTH))  = FX1_SA_X;
    *((volatile uint32_t*)(npu_base + CDC_NPU_HEIGHT)) = FX1_SA_Y;

    // Write Bias parameters
    if (ctx.has_bias && ctx.bias_ptr != nullptr) {
        *((volatile uint32_t*)(npu_base + CDC_NPU_BIAS_ADDR))       = (uint32_t)(uintptr_t)ctx.bias_ptr;
        *((volatile uint32_t*)(npu_base + CDC_NPU_BIAS_SIZE_BYTES)) = ctx.bias_size_bytes;
    } else {
        *((volatile uint32_t*)(npu_base + CDC_NPU_BIAS_ADDR))       = 0;
        *((volatile uint32_t*)(npu_base + CDC_NPU_BIAS_SIZE_BYTES)) = 0;
    }

    // Write Multiplier and Shift
    if (ctx.multiplier_ptr != nullptr) {
        *((volatile uint32_t*)(npu_base + CDC_NPU_MULTIPLIER_ADDR))       = (uint32_t)(uintptr_t)ctx.multiplier_ptr;
        *((volatile uint32_t*)(npu_base + CDC_NPU_MULTIPLIER_SIZE_BYTES)) = ctx.multiplier_size_bytes;
    } else {
        *((volatile uint32_t*)(npu_base + CDC_NPU_MULTIPLIER_ADDR))       = 0;
        *((volatile uint32_t*)(npu_base + CDC_NPU_MULTIPLIER_SIZE_BYTES)) = 0;
    }

    if (ctx.shift_ptr != nullptr) {
        *((volatile uint32_t*)(npu_base + CDC_NPU_SHIFT_ADDR))       = (uint32_t)(uintptr_t)ctx.shift_ptr;
        *((volatile uint32_t*)(npu_base + CDC_NPU_SHIFT_SIZE_BYTES)) = ctx.shift_size_bytes;
    } else {
        *((volatile uint32_t*)(npu_base + CDC_NPU_SHIFT_ADDR))       = 0;
        *((volatile uint32_t*)(npu_base + CDC_NPU_SHIFT_SIZE_BYTES)) = 0;
    }

    // Write Input/Output Zero Points and Activation Clamps
    // The zero points are usually passed as encoded INT8 but stored as INT32 in hardware registers
    *((volatile uint32_t*)(npu_base + CDC_NPU_INPUT_OFFSET))   = (uint32_t)ctx.input_zero_point;
    *((volatile uint32_t*)(npu_base + CDC_NPU_WEIGHT_OFFSET))  = (uint32_t)ctx.weight_zero_point;
    *((volatile uint32_t*)(npu_base + CDC_NPU_OUTPUT_OFFSET))  = (uint32_t)ctx.output_zero_point;
    
    *((volatile uint32_t*)(npu_base + CDC_NPU_ACTIVATION_MIN)) = (uint32_t)(int32_t)ctx.act_min;
    *((volatile uint32_t*)(npu_base + CDC_NPU_ACTIVATION_MAX)) = (uint32_t)(int32_t)ctx.act_max;
}

void NPUV4MMIOWriter::writeOBPLUT(uintptr_t npu_base, uint8_t act_mode,
                                  const uint8_t* lut_table, size_t lut_size) {
    // Currently, this function can be empty as OBP LUT logic for SiLU/GELU 
    // requires more clarification based on Open Question 3.
    // However, keeping the interface ready.
    (void)npu_base;
    (void)act_mode;
    (void)lut_table;
    (void)lut_size;
}
