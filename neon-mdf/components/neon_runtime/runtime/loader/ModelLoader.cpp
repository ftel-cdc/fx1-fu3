#include "ModelLoader.h"
#include "../../analyse/StaticAnalyzer.h"
#include "../driver/NPUDriver.h"
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <sstream>

// ============================================================
// P2: ISA version compat policy
//   major mismatch  → hard fail
//   minor newer     → warn, can proceed (may have unknown fields)
//   target mismatch → warn (not hard fail — user may intend it)
// ============================================================

#include "../support/NeonLog.h"

static std::vector<uint8_t> readFileBytes(const std::string &path,
                                          std::string *err) {
  std::ifstream f(path, std::ios::binary | std::ios::ate);
  if (!f.is_open()) {
    if (err)
      *err = "Cannot open file: " + path;
    return {};
  }
  std::streamsize sz = f.tellg();
  f.seekg(0, std::ios::beg);
  std::vector<uint8_t> buf((size_t)sz);
  if (!f.read(reinterpret_cast<char *>(buf.data()), sz)) {
    if (err)
      *err = "Read error: " + path;
    return {};
  }
  return buf;
}

// ============================================================
// validate — P2 implementation
// ============================================================

bool ModelLoader::validateMemory(const uint8_t *buf, size_t size,
                                 const DeviceTarget *device_target,
                                 std::string *error_msg) {
  uart_puts("[ModelLoader::validateMemory] Entered validateMemory()\r\n");
  if (!buf || size < sizeof(NeonFileHeader)) {
    if (error_msg)
      *error_msg = "Buffer too small to contain valid header";
    return false;
  }

  const NeonFileHeader *hdr = reinterpret_cast<const NeonFileHeader *>(buf);

  // 1. Magic check
  if (std::memcmp(hdr->magic, NEON_MAGIC, 8) != 0) {
    if (error_msg)
      *error_msg = "Bad magic: not a .nsm file";
    return false;
  }

  // 2. ISA version: major mismatch = hard fail
  uint16_t file_major = hdr->isa_version_major;
  uint16_t file_minor = hdr->isa_version_minor;
  uint16_t rt_major = NEON_ISA_VERSION_MAJOR;
  uint16_t rt_minor = NEON_ISA_VERSION_MINOR;

  if (file_major != rt_major) {
    std::ostringstream oss;
    oss << "ISA major version mismatch: file=" << file_major
        << " runtime=" << rt_major
        << " — recompile model with matching toolchain";
    if (error_msg)
      *error_msg = oss.str();
    return false;
  }

  if (file_minor > rt_minor) {
    // Warn — newer minor may have unknown fields but is generally backward
    // compat
    fprintf(stderr,
            "[WARN] ModelLoader: file ISA minor %u > runtime minor %u "
            "(file has newer features — some fields may be ignored)\n",
            file_minor, rt_minor);
  }

  // 3. Target device: warn, not hard fail
  char target_in_file[33];
  std::memcpy(target_in_file, hdr->target_device, 32);
  target_in_file[32] = '\0';

  if (device_target &&
      std::strcmp(target_in_file, device_target->getName()) != 0) {
    printf("[WARN] ModelLoader: model compiled for '%s', running on '%s' "
           "(mismatched target — check compiler flags if results are "
           "unexpected)\n",
           target_in_file, device_target->getName());
  }

  return true;
}

bool ModelLoader::validate(const std::string &model_path,
                           const DeviceTarget *device_target,
                           std::string *error_msg) {
  uart_puts("[ModelLoader::validate] Entered validate()\r\n");
  std::string err;
  auto buf = readFileBytes(model_path, &err);
  if (buf.empty()) {
    if (error_msg)
      *error_msg = err;
    return false;
  }

  return validateMemory(buf.data(), buf.size(), device_target, error_msg);
}

// ============================================================
// parseHeader
// ============================================================

bool ModelLoader::parseHeader(const uint8_t *buf, size_t size,
                              NeonFileHeader &hdr) {
  if (size < sizeof(NeonFileHeader))
    return false;
  std::memcpy(&hdr, buf, sizeof(NeonFileHeader));
  if (std::memcmp(hdr.magic, NEON_MAGIC, 8) != 0)
    return false;
  return true;
}

// ============================================================
// parseDispatch
// ============================================================

bool ModelLoader::parseDispatch(const uint8_t *buf, const NeonFileHeader &hdr,
                                NPUModel *model) {
  if (hdr.num_ops == 0)
    return true;

  const NeonOpEntry *table =
      reinterpret_cast<const NeonOpEntry *>(buf + hdr.dispatch_table_offset);

  model->ops.reserve(hdr.num_ops);
  for (uint32_t i = 0; i < hdr.num_ops; ++i) {
    const NeonOpEntry &e = table[i];
    RuntimeOp op{};
    op.index = i;
    op.opcode = e.opcode;
    if (hdr.num_ops <= 100) {
      op.name = "op_" + std::to_string(i);
    }
    // instr_ptr will be set after parseInstructions
    op.instr_ptr = nullptr;
    op.instr_size = 0;
    op.input_tensor_idx = e.input_tensor_idx;
    op.output_tensor_idx = e.output_tensor_idx;
    op.weight_blob_idx = UINT32_MAX;
    op.estimated_read_bytes = 0;
    op.estimated_write_bytes = 0;
    model->ops.push_back(op);
  }
  return true;
}

// ============================================================
// parseInstructions
// ============================================================

bool ModelLoader::parseInstructions(const uint8_t *buf,
                                    const NeonFileHeader &hdr,
                                    NPUModel *model) {
  if (hdr.queue_a_size == 0)
    return true;

  model->instruction_buffer.resize((size_t)hdr.queue_a_size);
  std::memcpy(model->instruction_buffer.data(), buf + hdr.queue_a_offset,
              (size_t)hdr.queue_a_size);

  // Patch RuntimeOp::instr_ptr using dispatch table offsets
  const NeonOpEntry *table =
      reinterpret_cast<const NeonOpEntry *>(buf + hdr.dispatch_table_offset);
  for (uint32_t i = 0; i < (uint32_t)model->ops.size() && i < hdr.num_ops;
       ++i) {
    uint64_t off = table[i].instr_offset;
    if (off < hdr.queue_a_size) {
      model->ops[i].instr_ptr = model->instruction_buffer.data() + off;
      // Estimate size from next op offset (last op gets remaining)
      uint64_t next_off =
          (i + 1 < hdr.num_ops) ? table[i + 1].instr_offset : hdr.queue_a_size;
      model->ops[i].instr_size = (size_t)(next_off - off);
    }
  }
  return true;
}

// ============================================================
// parseWeights
// ============================================================

bool ModelLoader::parseWeights(const uint8_t *buf, const NeonFileHeader &hdr,
                               NPUModel *model, void *static_weight,
                               size_t static_weight_size, bool is_rom_buffer) {
  if (hdr.weights_a_size == 0)
    return true;

  model->weight_buffer_size = (size_t)hdr.weights_a_size;
  if (static_weight != nullptr) {
    if (static_weight_size < model->weight_buffer_size)
      return false;
    model->weight_buffer = static_weight;
    std::memcpy(model->weight_buffer, buf + hdr.weights_a_offset,
                model->weight_buffer_size);
    model->is_static_weight = true;
  } else if (is_rom_buffer) {
    model->weight_buffer = const_cast<void *>(
        reinterpret_cast<const void *>(buf + hdr.weights_a_offset));
    model->is_static_weight = true;
  } else {
    model->weight_buffer = std::malloc(model->weight_buffer_size);
    if (!model->weight_buffer)
      return false;
    std::memcpy(model->weight_buffer, buf + hdr.weights_a_offset,
                model->weight_buffer_size);
    model->is_static_weight = false;
  }
  model->weights_on_device = false;

  // Create one WeightBlob covering the entire buffer
  WeightBlob blob{};
  blob.data = model->weight_buffer;
  blob.size_bytes = model->weight_buffer_size;
  blob.dtype = hdr.default_dtype;
  blob.on_device = false;
  model->weights.push_back(blob);
  return true;
}

// ============================================================
// parseQuantParams
// ============================================================

bool ModelLoader::parseQuantParams(const uint8_t *buf,
                                   const NeonFileHeader &hdr, NPUModel *model) {
  if (hdr.num_quant_params == 0 || hdr.quant_offset == 0)
    return true;

  const NeonQuantParams *qp =
      reinterpret_cast<const NeonQuantParams *>(buf + hdr.quant_offset);
  model->quant_params.resize(hdr.num_quant_params);
  std::memcpy(model->quant_params.data(), qp,
              hdr.num_quant_params * sizeof(NeonQuantParams));
  return true;
}

// ============================================================
// buildRequantTable
// ============================================================

bool ModelLoader::buildRequantTable(NPUModel *model) {
  const size_t n = model->quant_params.size();
  model->requant_table.resize(n);
  for (size_t i = 0; i < n; ++i) {
    float scale = model->quant_params[i].scale;
    int32_t zero_pt = model->quant_params[i].zero_point;

    // Tính multiplier và shift theo chuẩn TFLite QuantizeMultiplier:
    // scale = multiplier * 2^shift, multiplier ∈ [0.5, 1.0)
    double q = scale;
    int shift = 0;
    if (q == 0.0) {
      model->requant_table[i] = {0, 0, zero_pt, -128, 127};
      continue;
    }
    while (q < 0.5) {
      q *= 2.0;
      ++shift;
    }
    while (q >= 1.0) {
      q *= 0.5;
      --shift;
    }
    int32_t mult = (int32_t)(q * (1LL << 31) + 0.5);

    model->requant_table[i] = {
        /* .multiplier = */ mult,
        /* .shift      = */ -shift, // hardware dùng right-shift dương
        /* .zero_point = */ zero_pt,
        /* .act_min    = */ -128,
        /* .act_max    = */ 127,
    };
  }
  return true;
}

// ============================================================
// parseMemPlan
// ============================================================

bool ModelLoader::parseMemPlan(const uint8_t *buf, const NeonFileHeader &hdr,
                               NPUModel *model) {
  if (hdr.memplan_offset == 0 || hdr.num_tensors == 0)
    return true;

  const NeonMemAlloc *ma =
      reinterpret_cast<const NeonMemAlloc *>(buf + hdr.memplan_offset);

  model->mem_plan.resize(hdr.num_tensors);
  for (uint32_t i = 0; i < hdr.num_tensors; ++i) {
    MemAllocEntry e{};
    e.tensor_idx = ma[i].tensor_idx;
    e.offset = ma[i].offset;
    e.size_bytes = ma[i].size_bytes;
    e.first_use_op = ma[i].first_use_op;
    e.last_use_op = ma[i].last_use_op;
    model->mem_plan[i] = e;
  }
  return true;
}

// ============================================================
// parseBatchProfiles (Rev3)
// ============================================================

bool ModelLoader::parseBatchProfiles(const uint8_t *buf, size_t file_size,
                                     const NeonFileHeader &hdr,
                                     NPUModel *model) {
  if (hdr.num_batch_profiles == 0 || hdr.batch_profiles_offset == 0)
    return true; // backward compat — no batch profiles

  const NeonBatchProfileEntry *entries =
      reinterpret_cast<const NeonBatchProfileEntry *>(
          buf + hdr.batch_profiles_offset);

  model->batch_profiles.reserve(hdr.num_batch_profiles);

  for (uint32_t i = 0; i < hdr.num_batch_profiles; ++i) {
    const NeonBatchProfileEntry &pe = entries[i];

    BatchProfile bp{};
    bp.batch_size = pe.batch_size;
    bp.scratch_buffer_size = (size_t)pe.scratch_buffer_size;
    bp.input_tensor_idx = pe.input_tensor_idx;
    bp.output_tensor_idx = pe.output_tensor_idx;

    // Copy instruction stream
    if (pe.instr_size > 0 && pe.instr_offset + pe.instr_size <= file_size) {
      bp.instruction_buffer.resize((size_t)pe.instr_size);
      std::memcpy(bp.instruction_buffer.data(), buf + pe.instr_offset,
                  (size_t)pe.instr_size);
    }

    // Parse per-profile memplan
    if (pe.num_memplan_entries > 0 && pe.memplan_offset > 0) {
      const NeonMemAlloc *ma =
          reinterpret_cast<const NeonMemAlloc *>(buf + pe.memplan_offset);
      for (uint32_t j = 0; j < pe.num_memplan_entries; ++j) {
        MemAllocEntry e{};
        e.tensor_idx = ma[j].tensor_idx;
        e.offset = ma[j].offset;
        e.size_bytes = ma[j].size_bytes;
        e.first_use_op = ma[j].first_use_op;
        e.last_use_op = ma[j].last_use_op;
        bp.mem_plan.push_back(e);
      }
    }

    // Parse per-profile dispatch table for RuntimeOps
    if (pe.num_ops > 0 && pe.dispatch_table_offset > 0) {
      const NeonOpEntry *tbl =
          reinterpret_cast<const NeonOpEntry *>(buf + pe.dispatch_table_offset);
      for (uint32_t j = 0; j < pe.num_ops; ++j) {
        RuntimeOp op{};
        op.index = j;
        op.opcode = tbl[j].opcode;
        op.name = "batch" + std::to_string(pe.batch_size) + "_op_" +
                  std::to_string(j);
        op.input_tensor_idx = tbl[j].input_tensor_idx;
        op.output_tensor_idx = tbl[j].output_tensor_idx;
        op.weight_blob_idx = UINT32_MAX;
        // instr_ptr: into per-profile instruction_buffer
        if (tbl[j].instr_offset < pe.instr_size) {
          op.instr_ptr = bp.instruction_buffer.data() + tbl[j].instr_offset;
          uint64_t next =
              (j + 1 < pe.num_ops) ? tbl[j + 1].instr_offset : pe.instr_size;
          op.instr_size = (size_t)(next - tbl[j].instr_offset);
        }
        bp.ops.push_back(op);
      }
    }

    // Derive single_input/output sizes from base model tensors
    if (pe.input_tensor_idx < model->tensors.size())
      bp.single_input_size = model->tensors[pe.input_tensor_idx].size_bytes;
    else
      bp.single_input_size = model->input_size();

    if (pe.output_tensor_idx < model->tensors.size())
      bp.single_output_size = model->tensors[pe.output_tensor_idx].size_bytes;
    else
      bp.single_output_size = model->output_size();

    model->batch_profiles.push_back(std::move(bp));
  }

  // Sort ascending by batch_size
  std::sort(model->batch_profiles.begin(), model->batch_profiles.end(),
            [](const BatchProfile &a, const BatchProfile &b) {
              return a.batch_size < b.batch_size;
            });
  return true;
}

// ============================================================
// allocScratchBuffer + assignTensorPtrs
// ============================================================

bool ModelLoader::allocScratchBuffer(NPUModel *model, void *static_scratch,
                                     size_t static_scratch_size) {
  if (model->scratch_buffer_size == 0)
    return true;
  if (static_scratch != nullptr) {
    if (static_scratch_size < model->scratch_buffer_size)
      return false;
    model->scratch_buffer = static_scratch;
    model->is_static_scratch = true;
    return true;
  }
  model->scratch_buffer = std::malloc(model->scratch_buffer_size);
  model->is_static_scratch = false;
  return model->scratch_buffer != nullptr;
}

bool ModelLoader::assignTensorPtrs(NPUModel *model, bool keep_io_unbound) {
  if (!model->scratch_buffer)
    return true;

  // Ensure tensors vector is sized
  uint32_t max_idx = 0;
  for (const auto &e : model->mem_plan)
    if (e.tensor_idx > max_idx)
      max_idx = e.tensor_idx;
  if (model->tensors.size() <= max_idx)
    model->tensors.resize(max_idx + 1);

  for (const auto &e : model->mem_plan) {
    if (e.tensor_idx >= model->tensors.size())
      continue;
    TensorDesc &td = model->tensors[e.tensor_idx];
    td.size_bytes = (size_t)e.size_bytes;

    // Graph I/O tensors are rebound to arena/caller buffers by
    // runtime/binder/OperandBinder. Standalone .nsm execution instead copies
    // caller input/output through these scratch-backed tensor descriptors.
    // Tile descriptors (>= 1000) live in device SRAM in either mode.
    const bool is_model_io = e.tensor_idx < model->num_inputs ||
                             e.tensor_idx == model->input_tensor_idx ||
                             e.tensor_idx == model->output_tensor_idx;
    if ((keep_io_unbound && is_model_io) || e.tensor_idx >= 1000) {
      td.data = nullptr;
      continue;
    }

    // External graph operands or I/O tensors that have offset == 0
    // and size exceeding scratch buffer are bound externally.
    if (e.offset == 0 && e.size_bytes > model->scratch_buffer_size) {
      td.data = nullptr;
      continue;
    }

    // A scratch plan entry that escapes the scratch buffer is not survivable:
    // the pointer would land in the heap arena, the .bss tail, and then the machine
    // stack (RAM is fully mapped, so nothing faults -- it just corrupts). Fail
    // the load instead.
    if (e.offset + e.size_bytes > model->scratch_buffer_size) {
      uart_puts("[ModelLoader] ERROR: mem_plan entry escapes scratch buffer\r\n");
      uart_put_u32("  -> tensor_idx : ", e.tensor_idx, "\r\n");
      uart_put_u32("  -> offset     : ", static_cast<uint32_t>(e.offset), "\r\n");
      uart_put_u32("  -> size_bytes : ", static_cast<uint32_t>(e.size_bytes), "\r\n");
      uart_put_u32("  -> scratch    : ",
                   static_cast<uint32_t>(model->scratch_buffer_size), "\r\n");
      return false;
    }
    td.data = static_cast<uint8_t *>(model->scratch_buffer) + e.offset;
  }
  return true;
}

bool ModelLoader::uploadWeights(NPUModel *model, NPUDriver *npu_driver) {
  if (!npu_driver || !model->weight_buffer || model->weight_buffer_size == 0)
    return true;
  // DMA CH0 → weights (single-model mode)
  NPUDriverStatus st = npu_driver->dmaTransfer(
      0, model->weight_buffer,
      nullptr, // device knows SRAM base for weights
      model->weight_buffer_size, DMADir::HOST_TO_DEVICE);
  if (st == NPUDriverStatus::OK) {
    model->weights_on_device = true;
    if (!model->weights.empty())
      model->weights[0].on_device = true;
  }
  return st == NPUDriverStatus::OK;
}

// ============================================================
// load
// ============================================================

NPUModel *ModelLoader::load(const std::string &model_path,
                            const DeviceTarget *device_target,
                            NPUDriver *npu_driver) {
  // Validate first (P2)
  std::string verr;
  if (!validate(model_path, device_target, &verr)) {
    fprintf(stderr, "[ERROR] ModelLoader::load: %s\n", verr.c_str());
    return nullptr;
  }

  std::string err;
  auto raw = readFileBytes(model_path, &err);
  if (raw.empty()) {
    fprintf(stderr, "[ERROR] ModelLoader::load: %s\n", err.c_str());
    return nullptr;
  }

  const uint8_t *buf = raw.data();
  size_t size = raw.size();

  NPUModel *model = new NPUModel();
  NeonFileHeader hdr{};

  if (!parseHeader(buf, size, hdr)) {
    fprintf(stderr, "[ERROR] ModelLoader: invalid header\n");
    delete model;
    return nullptr;
  }

  model->isa_version =
      ((uint32_t)hdr.isa_version_major << 16) | hdr.isa_version_minor;
  model->default_dtype = hdr.default_dtype;
  model->input_tensor_idx = hdr.input_tensor_idx;
  model->output_tensor_idx = hdr.output_tensor_idx;
  model->scratch_buffer_size = (size_t)hdr.scratch_buffer_size;
  {
    char name[33];
    std::memcpy(name, hdr.target_device, 32);
    name[32] = '\0';
    model->target_device = name;
  }
  model->model_name = model_path;

  if (!parseDispatch(buf, hdr, model) || !parseInstructions(buf, hdr, model) ||
      !parseWeights(buf, hdr, model) || !parseQuantParams(buf, hdr, model) ||
      !buildRequantTable(model) || !parseMemPlan(buf, hdr, model) ||
      !allocScratchBuffer(model) || !assignTensorPtrs(model, false)) {
    fprintf(stderr, "[ERROR] ModelLoader: parse failure\n");
    delete model;
    return nullptr;
  }

  // Rev3: batch profiles (after tensors are set up)
  if (!parseBatchProfiles(buf, size, hdr, model)) {
    fprintf(stderr, "[WARN] ModelLoader: batch profile parse failed — "
                    "continuing without\n");
  }

  if (npu_driver) {
    // Hand the device its two base addresses BEFORE any DMA or dispatch.
    // Instructions carry offsets into these buffers; a device that
    // masters the bus itself (FX1) cannot turn an offset into a physical
    // address without them, and would reject every op as
    // ERROR_INVALID_ADDR. Default is a no-op for devices that don't need
    // it — see NPUDriver::bindModelMemory().
    npu_driver->bindModelMemory(
        model->scratch_buffer, model->scratch_buffer_size, model->weight_buffer,
        model->weight_buffer_size);

    // Per-tensor quantisation scales. Instructions carry no scale
    // fields — NPUModel::quant_params is indexed by tensor_idx, which
    // writeInstruction() never receives — so this is the only route by
    // which a device learns them. Without it every scale stays at the
    // 1.0 default, which is exact for FLOAT32 models and wrong for
    // every quantised one.
    //
    // Absent or short quant data leaves the device at its default
    // rather than substituting a guess.
    if (!model->quant_params.empty()) {
      const auto &qp = model->quant_params;
      const float in_scale = (model->input_tensor_idx < qp.size())
                                 ? qp[model->input_tensor_idx].scale
                                 : 0.f;
      const float out_scale = (model->output_tensor_idx < qp.size())
                                  ? qp[model->output_tensor_idx].scale
                                  : 0.f;
      npu_driver->bindQuantScales(in_scale, out_scale);
    }

    // Upload weights to device
    uploadWeights(model, npu_driver);
  }

  return model;
}

// ============================================================
// loadFromMemory
// ============================================================

NPUModel *ModelLoader::loadFromMemory(
    const uint8_t *buffer, size_t size, const DeviceTarget *device_target,
    NPUDriver *npu_driver, void *static_scratch, size_t static_scratch_size,
    void *static_weight, size_t static_weight_size, uint32_t num_inputs,
    bool eager_upload_weights) {
  uart_puts("[ModelLoader::loadFromMemory] Entered loadFromMemory()\r\n");
  std::string verr;
  if (!validateMemory(buffer, size, device_target, &verr)) {
    uart_puts("[ERROR] ModelLoader::loadFromMemory validate failed\r\n");
    return nullptr;
  }

  NPUModel *model = new NPUModel();
  NeonFileHeader hdr{};

  if (!parseHeader(buffer, size, hdr)) {
    delete model;
    return nullptr;
  }

  model->isa_version =
      ((uint32_t)hdr.isa_version_major << 16) | hdr.isa_version_minor;
  model->default_dtype = hdr.default_dtype;
  model->input_tensor_idx = hdr.input_tensor_idx;
  model->output_tensor_idx = hdr.output_tensor_idx;
  if (num_inputs > 0) {
    model->num_inputs = num_inputs;
  }
  model->scratch_buffer_size = (size_t)hdr.scratch_buffer_size;
  {
    char name[33];
    std::memcpy(name, hdr.target_device, 32);
    name[32] = '\0';
    model->target_device = name;
  }
  model->model_name = "embedded_memory_model";

  if (!parseDispatch(buffer, hdr, model) ||
      !parseInstructions(buffer, hdr, model) ||
      !parseWeights(buffer, hdr, model, static_weight, static_weight_size,
                    true) ||
      !parseQuantParams(buffer, hdr, model) || !buildRequantTable(model) ||
      !parseMemPlan(buffer, hdr, model) ||
      !allocScratchBuffer(model, static_scratch, static_scratch_size) ||
      !assignTensorPtrs(model, true)) {
    delete model;
    return nullptr;
  }

  if (!parseBatchProfiles(buffer, size, hdr, model)) {
    uart_puts("[WARN] ModelLoader: batch profile parse failed\r\n");
  }

  if (npu_driver && eager_upload_weights)
    uploadWeights(model, npu_driver);

  return model;
}

// ============================================================
// unload
// ============================================================

void ModelLoader::unload(NPUModel *model) {
  if (!model)
    return;
  if (!model->is_static_scratch && model->scratch_buffer) {
    std::free(model->scratch_buffer);
    model->scratch_buffer = nullptr;
  }
  if (!model->is_static_weight && model->weight_buffer) {
    std::free(model->weight_buffer);
    model->weight_buffer = nullptr;
  }
  delete model;
}

// ============================================================
// loadBinary / saveBinary
// ============================================================

std::vector<uint8_t> ModelLoader::loadBinary(const std::string &path) {
  std::string err;
  return readFileBytes(path, &err);
}

bool ModelLoader::saveBinary(const std::string &path,
                             const std::vector<uint8_t> &data) {
  std::ofstream f(path, std::ios::binary);
  if (!f.is_open())
    return false;
  f.write(reinterpret_cast<const char *>(data.data()),
          (std::streamsize)data.size());
  return f.good();
}

// ============================================================
// loadShapes — read NeonLayerShape[] → OpShape[]
// ============================================================

std::vector<OpShape> ModelLoader::loadShapes(const std::string &model_path) {
  std::string err;
  auto raw = readFileBytes(model_path, &err);
  if (raw.empty())
    return {};

  const uint8_t *buf = raw.data();
  NeonFileHeader hdr{};
  if (!parseHeader(buf, raw.size(), hdr))
    return {};
  if (hdr.num_layer_shapes == 0 || hdr.layer_shapes_offset == 0)
    return {};

  const NeonLayerShape *shapes =
      reinterpret_cast<const NeonLayerShape *>(buf + hdr.layer_shapes_offset);

  std::vector<OpShape> result;
  result.reserve(hdr.num_layer_shapes);

  for (uint32_t i = 0; i < hdr.num_layer_shapes; ++i) {
    const NeonLayerShape &s = shapes[i];
    OpShape op{};
    op.op_index = s.op_index;
    op.opcode = static_cast<NeonOpcode>(s.opcode);
    op.dtype = static_cast<NeonDtype>(s.dtype);
    op.op_name = "op_" + std::to_string(s.op_index);
    op.input_n = s.input_n;
    op.input_h = s.input_h;
    op.input_w = s.input_w;
    op.input_c = s.input_c;
    op.kernel_h = s.kernel_h;
    op.kernel_w = s.kernel_w;
    op.channels_out = s.channels_out;
    op.depth_multiplier = s.depth_multiplier;
    op.stride_h = s.stride_h;
    op.stride_w = s.stride_w;
    op.pad_h = s.pad_h;
    op.pad_w = s.pad_w;
    op.input_size = s.input_size;
    op.output_size = s.output_size;
    op.element_count = s.element_count;
    result.push_back(op);
  }
  return result;
}
