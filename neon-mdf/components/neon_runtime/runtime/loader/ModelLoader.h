#pragma once

// ============================================================
// NEON STUDIO — MODEL LOADER
// Parses a .nsm binary file and builds an NPUModel in memory.
//
// Steps:
//   1. Validate magic + ISA version
//   2. Parse NeonFileHeader
//   3. Parse dispatch table → RuntimeOp[]
//   4. Parse .nsm_instructions → instruction_buffer (batch=1)
//   5. Parse .nsm_weights → WeightBlob[] + weight_buffer (shared)
//   6. Parse .nsm_quant → quant_params[]
//   7. Parse .nsm_memplan → mem_plan[]
//   8. Build TensorDesc[] from memplan
//   9. Allocate scratch_buffer (sized for batch=1)
//  10. DMA upload weights → device SRAM (via NPUDriver)
//  11. Parse .nsm_batch_profiles → BatchProfile[] (Rev3, if present)
//      Each BatchProfile gets its own instruction_buffer + mem_plan.
//      All profiles share NPUModel::weight_buffer — no duplication.
//      Profiles are stored sorted by batch_size ascending.
// ============================================================

#include "NPUModel.h"
#include "../../core/DeviceTarget.h"
#include "../../analyse/StaticAnalyzer.h"  // OpShape — used in loadShapes()

#ifdef __cplusplus
extern "C++" {

#include <string>
#include <vector>

// Forward declare — full definition in runtime/
class NPUDriver;

class ModelLoader {
public:
    // --- Load / Unload ---

    // Parse .nsm file and build NPUModel.
    // npu_driver may be nullptr for static-analysis-only use (no DMA upload).
    // Parse .nsm file and build NPUModel.
    // npu_driver may be nullptr for static-analysis-only use (no DMA upload).
    static NPUModel* load(const std::string&   model_path,
                          const DeviceTarget* device_target,
                          NPUDriver*        npu_driver  = nullptr);

    // Load .nsm directly from memory buffer (no file I/O). Supports static scratch and weight buffers.
    // eager_upload_weights=false lets a capable driver stage only on first use.
    static NPUModel* loadFromMemory(const uint8_t*       buffer,
                                    size_t               size,
                                    const DeviceTarget*  device_target,
                                    NPUDriver*           npu_driver          = nullptr,
                                    void*                static_scratch      = nullptr,
                                    size_t               static_scratch_size = 0,
                                    void*                static_weight       = nullptr,
                                    size_t               static_weight_size  = 0,
                                    uint32_t             num_inputs          = 0,
                                    bool                 eager_upload_weights = true);

    // Release all memory allocated by load()
    static void unload(NPUModel* model);

    // --- Validation ---

    // Check magic, ISA version, target device name match
    static bool validate(const std::string& model_path,
                         const DeviceTarget* device_target,
                         std::string* error_msg = nullptr);

    static bool validateMemory(const uint8_t* buffer,
                               size_t size,
                               const DeviceTarget* device_target,
                               std::string* error_msg = nullptr);

    // --- Tensor I/O helpers ---

    // Load raw binary file as byte vector.
    // Input tensors on baremetal are raw INT8/UINT8 bytes (post-preprocess).
    // Caller casts to float* if model runs float32.
    static std::vector<uint8_t> loadBinary(const std::string& path);

    // Save raw bytes to file
    static bool saveBinary(const std::string&          path,
                           const std::vector<uint8_t>& data);

    // --- OpShape extraction (used by neon-eval) ---
    // Extracts OpShape list from .nsm_meta for static analysis
    // without fully loading the model
    static std::vector<struct OpShape> loadShapes(const std::string& model_path);

private:
    // Section parsers
    static bool parseHeader      (const uint8_t* buf, size_t size,
                                  NeonFileHeader& hdr);
    static bool parseDispatch    (const uint8_t* buf, const NeonFileHeader& hdr,
                                  NPUModel* model);
    static bool parseInstructions(const uint8_t* buf, const NeonFileHeader& hdr,
                                  NPUModel* model);
    static bool parseWeights     (const uint8_t* buf, const NeonFileHeader& hdr,
                                  NPUModel* model,
                                  void* static_weight = nullptr,
                                  size_t static_weight_size = 0,
                                  bool is_rom_buffer = false);
    static bool parseQuantParams (const uint8_t* buf, const NeonFileHeader& hdr,
                                  NPUModel* model);
    static bool buildRequantTable(NPUModel* model);
    static bool parseMemPlan     (const uint8_t* buf, const NeonFileHeader& hdr,
                                  NPUModel* model);

    // Rev3: batch profile parser
    // Reads NeonBatchProfileEntry[] and populates NPUModel::batch_profiles.
    // Called after parseWeights() — profiles share the already-loaded weight_buffer.
    // No-op if hdr.num_batch_profiles == 0 (backward compat with Rev1/Rev2 files).
    static bool parseBatchProfiles(const uint8_t* buf, size_t file_size,
                                   const NeonFileHeader& hdr,
                                   NPUModel* model);

    // Memory setup
    static bool allocScratchBuffer(NPUModel* model,
                                   void* static_scratch = nullptr,
                                   size_t static_scratch_size = 0);
    // Standalone .nsm execution stages I/O through its scratch allocation;
    // graph fragments bind I/O to arena tensors later through OperandBinder.
    static bool assignTensorPtrs  (NPUModel* model, bool keep_io_unbound);
    static bool uploadWeights     (NPUModel* model, NPUDriver* npu_driver);
};

} // extern "C++"
#endif
