#pragma once

#include <cstdint>
#include <cstddef>

struct NeonJobContext {
    // GEMM dimensions (filled by writeInstruction)
    uint32_t k_dim;
    uint32_t activation_stride;

    // Requantization (filled từ NPUModel::requant_table)
    const int32_t* bias_ptr;          // physical addr bias array
    uint32_t       bias_size_bytes;

    const int32_t* multiplier_ptr;    // physical addr multiplier array
    uint32_t       multiplier_size_bytes;

    const int32_t* shift_ptr;         // physical addr shift array
    uint32_t       shift_size_bytes;

    int32_t  output_zero_point;
    int32_t  input_zero_point;
    int8_t   act_min;
    int8_t   act_max;

    // ADD instruction parameters
    const int32_t* add_params_ptr;

    // MUL instruction parameters
    const int32_t* mul_params_ptr;

    // SUB instruction parameters
    const int32_t* sub_params_ptr;

    // Feature flags
    bool has_bias;
    bool has_requantization;

    // Fused epilogue
    int32_t  weight_zero_point;
    bool     has_lut;
    const uint8_t* lut_data;
    uint32_t lut_size;
    uint8_t  act_mode_raw;
    void*    output_dram_ptr;
    uint64_t output_offset;

    // Verification support (compile-time flag)
#ifdef NEON_REQUANT_VERIFY
    const int32_t* sw_ref_output;     // SW reference output để compare
    size_t         output_count;
#endif
};
