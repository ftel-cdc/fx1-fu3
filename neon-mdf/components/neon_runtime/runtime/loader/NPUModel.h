#pragma once

// ============================================================
// NEON STUDIO — NPU MODEL
// In-memory representation of a loaded .nsm binary.
// Populated by ModelLoader, consumed by Executor.
//
// Lifecycle:
//   NPUModel* model = ModelLoader::load("model.nsm", target);
//   Executor::run(model, input, output);
//   ModelLoader::unload(model);
// ============================================================

#include "../../core/NPU_ISA.h"
#include "../../core/DeviceTarget.h"

#ifdef __cplusplus
extern "C++" {

#include <cstdint>
#include <cstddef>
#include <vector>
#include <string>

// ============================================================
// TENSOR DESCRIPTOR
// Describes shape and memory location of one tensor.
// ============================================================

struct TensorDesc {
    uint32_t  dims[4];          // N, H, W, C
    uint32_t  strides[4];       // để support non-contiguous layout
    NeonDtype dtype;
    size_t    size_bytes;
    void*     data;             // trỏ vào scratch_buffer + offset
};

// ============================================================
// WEIGHT BLOB
// One blob per layer that has weights (conv, fc, bn...).
// Loaded once at startup, stays in memory for lifetime of model.
// ============================================================

struct WeightBlob {
    void*     data;             // raw weight bytes (quantized)
    size_t    size_bytes;
    NeonDtype dtype;
    mutable bool on_device;     // true khi nội dung blob này đang được staging trên device
};

// ============================================================
// RUNTIME OP — một op trong execution order
// Parsed từ dispatch table của .nsm file.
// ============================================================

struct RuntimeOp {
    uint32_t    index;          // vị trí trong execution order
    NeonOpcode  opcode;
    std::string name;           // debug label

    // Con trỏ tới instruction bytes trong instruction_buffer
    // Runtime đọc bytes này và gửi xuống device qua npu_driver
    const void* instr_ptr;
    size_t      instr_size;

    // Tensor indices (vào arrays bên dưới)
    uint32_t    input_tensor_idx;
    uint32_t    output_tensor_idx;
    uint32_t    weight_blob_idx;    // UINT32_MAX nếu không có weight

    // Bytes ước tính để DMAProfiler track
    size_t      estimated_read_bytes;
    size_t      estimated_write_bytes;
};

// ============================================================
// MEMORY ALLOCATION ENTRY
// Từ .nsm_memplan section — npu_driver dùng để assign
// tensor.data = scratch_buffer + offset.
// ============================================================

struct MemAllocEntry {
    uint32_t tensor_idx;
    uint64_t offset;            // offset trong scratch_buffer
    uint64_t size_bytes;
    uint32_t first_use_op;      // lifetime: [first_use_op, last_use_op]
    uint32_t last_use_op;
};

// ============================================================
// BATCH PROFILE  (Rev3)
// In-memory representation of one pre-compiled batch profile.
// Populated by ModelLoader from .nsm_batch_profiles section.
// Weights are NOT stored here — shared from NPUModel::weight_buffer.
// ============================================================

struct BatchProfile {
    uint32_t               batch_size;

    // Compiled instruction stream for this batch size.
    // AXI4MLIR-generated — DMA sizes and tensor offsets already
    // account for the batch dimension.
    std::vector<uint8_t>   instruction_buffer;

    // Scratch memory required to run this profile.
    // BatchAccumulator allocates this per-session from a pool.
    size_t                 scratch_buffer_size;

    // Size of one item (batch=1 equivalent) in bytes.
    // Used by ProfileSelector to pack/unpack batched I/O buffers.
    size_t                 single_input_size;
    size_t                 single_output_size;

    uint32_t               input_tensor_idx;
    uint32_t               output_tensor_idx;

    // Memory plan specific to this profile (offsets scale with batch_size).
    std::vector<MemAllocEntry> mem_plan;

    // RuntimeOp dispatch table for this profile.
    // Ops may have different tensor shapes than the batch=1 model.
    std::vector<RuntimeOp> ops;
};

// ============================================================
// REQUANT ENTRY
// ============================================================
// Runtime-only — không sửa NeonQuantParams trong NPU_ISA.h (on-disk format).
// Indexed by tensor_idx, song song với NPUModel::quant_params[].
struct RequantEntry {
    int32_t multiplier;    // fixed-point M từ quantize_multiplier(scale)
    int32_t shift;         // right-shift S — negative = left shift
    int32_t zero_point;    // output zero-point (copy từ NeonQuantParams)
    int8_t  act_min;       // activation clamp min (mặc định -128)
    int8_t  act_max;       // activation clamp max (mặc định  127)
};

// ============================================================
// NPU MODEL
// ============================================================

struct NPUModel {
    // --- Metadata ---
    std::string   model_name;
    std::string   target_device;    // phải match với device đang chạy
    NeonDtype     default_dtype;
    uint32_t      isa_version;

    // --- Graph ---
    std::vector<RuntimeOp>  ops;            // theo execution order
    uint32_t                input_tensor_idx = 0;
    uint32_t                output_tensor_idx = 1;
    uint32_t                num_inputs = 1;

    // --- Tensors ---
    std::vector<TensorDesc> tensors;

    // --- Weights ---
    std::vector<WeightBlob> weights;

    // Contiguous buffer chứa tất cả weight data
    void*   weight_buffer       = nullptr;
    size_t  weight_buffer_size  = 0;
    mutable bool weights_on_device = false; // true khi buffer weights đã được staging đầy đủ
    bool    is_static_weight    = false;

    // --- Memory plan ---
    std::vector<MemAllocEntry> mem_plan;

    // Scratch buffer — cấp phát 1 lần, tái dùng qua các tensor
    void*   scratch_buffer      = nullptr;
    size_t  scratch_buffer_size = 0;        // từ NeonFileHeader
    bool    is_static_scratch   = false;

    // --- Quant params ---
    // Indexed by tensor_idx
    std::vector<NeonQuantParams> quant_params;
    std::vector<RequantEntry>    requant_table;

    // --- Raw instruction bytes ---
    // Giữ nguyên bytes từ .nsm_instructions section
    // RuntimeOp.instr_ptr trỏ vào đây
    std::vector<uint8_t> instruction_buffer;

    // --- Batch profiles (Rev3) ---
    // Sorted by batch_size ascending. Empty if compiled without batch profiles.
    std::vector<BatchProfile> batch_profiles;

    // --- Convenience ---
    size_t input_size()  const {
        return tensors[input_tensor_idx].size_bytes;
    }
    size_t output_size() const {
        return tensors[output_tensor_idx].size_bytes;
    }
    uint32_t num_ops()   const { return (uint32_t)ops.size(); }

    // Find the smallest batch profile with batch_size >= n.
    // Returns the largest available profile if n exceeds all profiles.
    // Returns nullptr if no batch profiles are loaded.
    const BatchProfile* getProfile(uint32_t n) const {
        if (batch_profiles.empty()) return nullptr;
        for (const auto& p : batch_profiles)
            if (p.batch_size >= n) return &p;
        return &batch_profiles.back();  // saturate at largest
    }

    // Maximum batch size available for this model.
    uint32_t maxBatchSize() const {
        return batch_profiles.empty() ? 1u
                                      : batch_profiles.back().batch_size;
    }
};

} // extern "C++"
#endif
