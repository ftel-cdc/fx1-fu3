# Neon Studio — Runtime

> **This directory is owned by the runtime team.**
> No dependency on `toolkit/` or MLIR — builds cleanly with a bare-metal ARM cross-compiler.
> If you need to compile `.tflite` → `.nsm`, see [`toolkit/`](../toolkit/TOOLKIT_DEV_README.md) instead.

---

## What is this?

`runtime/` is the inference engine — Neon Studio's model-loading and dispatch layer.

It loads a compiled `.nsm` binary, manages device memory, and drives the NPU hardware through a dispatch loop. The runtime never contains model-specific logic: all op semantics live in the compiled instruction stream.

```
model.nsm
   ↓  ModelLoader::load()     parse header, weights, instruction stream(s), batch profiles
   ↓  Executor::run()         dispatch loop: read opcode → write MMIO → trigger → wait
   ↓  NPUDriver            hardware abstraction: DMA, MMIO, perf counters
output bytes
```

---

## Directory Structure

```
runtime/
├── NeonRuntime.h              # Public API — single include for apps/
│
├── graph/
│   ├── NPUGraph.h             # Graph representation (tensors, fragments, memory plan)
│   ├── GraphLoader.h          # Loads NEONGRF or wraps NEONNSM as 1-fragment graph
│   ├── ArenaPlanner.h         # Greedy arena allocator (peak workspace computation)
│   └── GraphRunner.h          # Graph-level runner executing fragments sequentially
│
├── binder/
│   └── OperandBinder.h        # Binds graph tensor buffers to fragment inputs/outputs
│
├── support/
│   └── NeonLog.h              # Platform-independent logging abstraction (neon_log_printf)
│
├── loader/
│   ├── NPUModel.h             # In-memory model: ops, tensors, weights, batch profiles
│   └── ModelLoader.h          # .nsm file parser → NPUModel*
│
├── executor/
│   └── Executor.h             # Dispatch loop: run(), run_batch(), runConcurrent()
│
├── driver/
│   ├── NPUDriver.h            # Abstract HAL: DMA, MMIO, trigger, relocations, perf counters
│   └── CPUCycleCounter.h      # Raw CPU/core cycle counter — unrelated to NPU, separate interface
│
├── opfallbacks/
│   ├── FallbackIO.h           # Host/device agnostic pointer helpers for CPU fallbacks
│   └── *CPUFallback.cpp       # Portable CPU fallback kernel implementations
│
└── batch/
    ├── BatchAccumulator.h     # Thread-safe request queue with flush policy
    └── ProfileSelector.h      # Profile selection, input packing, output unpacking
```

Device-specific runtime implementations live in `device/<target>/runtime/`, not here.

---

## ISA Revision History

| Rev | `isa_version_minor` | Changes |
|-----|--------------------|-----------------------------------------|
| Rev1 | 0 | Single-model, single-queue `.nsm` format |
| Rev2 | 1 | `SET_NSPLIT` opcode, dual-queue binary layout, spatial-split lanes |
| Rev3 | 2 | `.nsm_batch_profiles` section, `BatchAccumulator`, `run_batch()` |

The current version is **Rev3** (`NEON_ISA_VERSION_MINOR = 2`).

Runtime và compiler đều include `core/NPU_ISA.h` — đây là source of truth duy nhất. Không hardcode opcode values hay struct layouts ngoài file này.

---

## Components

### `NPUModel` — in-memory model

Populated by `ModelLoader::load()`. Holds:

- `instruction_buffer` — raw AXI4MLIR-compiled instruction bytes (batch=1)
- `weight_buffer` — contiguous weight data, shared across all batch profiles
- `tensors[]`, `mem_plan[]` — scratch memory layout
- `quant_params[]` — per-tensor scale/zero_point
- `batch_profiles[]` — pre-compiled streams for batch sizes (Rev3)

`batch_profiles` được sort theo `batch_size` ascending. `getProfile(n)` trả về profile nhỏ nhất với `batch_size >= n`, saturate ở profile lớn nhất nếu `n` vượt quá.

### `ModelLoader` — file parser

Parses sections từ `.nsm` binary theo thứ tự:

1. Validate magic + ISA version
2. Parse `NeonFileHeader`
3. Parse dispatch table → `RuntimeOp[]`
4. Parse `.nsm_instructions` → `instruction_buffer`
5. Parse `.nsm_weights` → `weight_buffer` (shared, loaded once)
6. Parse `.nsm_quant` → `quant_params[]`
7. Parse `.nsm_memplan` → `mem_plan[]`
8. Allocate `scratch_buffer`
9. DMA upload weights → device SRAM (nếu `npu_driver != nullptr`)
10. Parse `.nsm_batch_profiles` → `batch_profiles[]` (Rev3, no-op nếu `num_batch_profiles == 0`)

Batch profiles dùng chung `weight_buffer` — không duplicate weight data.

### `Executor` — dispatch loop

Executor đọc instruction stream byte-by-byte, cast sang đúng packed struct, dispatch tới `NPUDriver`:

```
NeonOpcode::DMA_READ    → npu_driver->dmaTransfer(HOST_TO_DEVICE)
NeonOpcode::DMA_WRITE   → npu_driver->dmaTransfer(DEVICE_TO_HOST)
NeonOpcode::CONV2D      → npu_driver->writeReg(OPCODE)
                          npu_driver->writeReg(STRIDE) ...
                          npu_driver->trigger() → npu_driver->waitDone()
NeonOpcode::LAYERNORM   → tương tự
NeonOpcode::HALT        → kết thúc stream
```

DMA được emit bởi compiler (AXI4MLIR) dưới dạng `NeonDMAInstr` trong stream — Executor không tự orchestrate DMA. Compute op handlers chỉ làm MMIO config + trigger + wait.

**Quan trọng:** Executor không biết gì về semantics của model. Mọi scheduling DMA, double-buffering, và tensor layout đã được bake vào stream bởi compiler.

API:

```cpp
// Batch=1 — bất kỳ hardware nào
NPUDriverStatus run(const void* input, void* output);

// Batch=N — dùng pre-compiled BatchProfile (Rev3)
NPUDriverStatus run_batch(const BatchProfile& profile,
                       const void*         packed_input,   // batch_size × input_size bytes
                       void*               packed_output); // batch_size × output_size bytes

// Dual-lane spatial split (FX1 Rev2)
NPUDriverStatus runConcurrent(const void* input_a, void* output_a,
                           NPUModel*   model_b,
                           const void* input_b, void* output_b,
                           uint8_t     n_split);
```

### `NPUDriver` — hardware abstraction

Abstract base class. Implementations per target:

| Target | Implementation |
|--------|---------------|
| VP_FX1_V2.0 | `device/VP_FX1_V2.0/NPUV4Runtime.h` |
| QEMU MPS3 AN547 | `device/qemu_mps3_an547/runtime/QEMUMPS3Runtime.h` |
| HostStubNPUDriver | `bench/HostStubNPUDriver.h` (fallback only, unused by any registered target) |

Không có target nào chạy host-native nữa (`device/cpu_sim/` đã bị xoá). `NPUDriver::canRunOnHost()` đã bị xoá hẳn khỏi interface vì lý do tương tự — `pickBenchNPUDriver()` trong `neon-eval` luôn dùng npu_driver thật của target (không còn "host safety" nào cần bảo vệ, vì neon-eval chỉ build theo `Makefile.qemu`, cross-compile cho chính target đó).

Key virtuals: `dmaTransfer()`, `writeReg()`, `trigger()`, `waitDone()`, `readPerfTotalCycles()`, `readPerfDMAStalls()`, `setNSplit()`, `submitConcurrent()`, `relocateInstruction()`, `beginFragment()`, `endFragment()`.

Perf counter design: `readPerfTotalCycles()` returns raw counter value (hardware) hoặc software-accumulated cycles (`QEMUMPS3Runtime` — không có NPU device thật trong QEMU). 32-bit counter wrap được handle bởi `Executor::cycleDelta()`.

**Registration:** mỗi Runtime implementation phải gọi macro `NEON_REGISTER_TARGET(ClassName)` ở cuối `.cpp`. Macro này tạo một static initializer đăng ký device vào `DeviceTargetRegistry`. Linker phải được build với `--whole-archive`/`-force_load` để không drop các registrar objects (xem `Makefile.qemu`).

**MMIO code:** thật ra chỉ `NPUV4Runtime` còn dùng `reinterpret_cast<volatile uint32_t*>` cho MMIO thật — vi phạm strict aliasing rules, cần `-fno-strict-aliasing` (đã có trong `Makefile.qemu`) để tránh UB ở `-O2`. `QEMUMPS3Runtime` không còn chạm MMIO thật nữa — register file của nó là một mảng software.

---

## Graph Execution

To execute arbitrary directed acyclic graphs (DAGs) across multiple compiled fragments without baking physical addresses into binaries:

### Container: `NEONGRF`
- Magic: `NEONGRF\0`, format version 2 (`core/NPU_ISA.h`).
- Embeds individual `.nsm` fragment binaries byte-for-byte in a contiguous payload table, followed by embedded constant tensor byte payloads.
- Contains metadata for all graph tensors:
  - `NEON_GRAPH_TENSOR_INTERNAL` (0): Intermediate tensors placed in the runtime-allocated activation arena.
  - `NEON_GRAPH_TENSOR_INPUT` (1): Graph inputs bound to caller input buffers.
  - `NEON_GRAPH_TENSOR_OUTPUT` (2): Graph outputs bound to caller output buffers; may also be read by subsequent fragments.
  - `NEON_GRAPH_TENSOR_CONST` (3): Constant operands embedded directly in the container with 16-byte alignment (`data_offset`).
- Lifetimes (`producer`, `last_consumer`) and fragment connectivity (`input_tensors`, `output_tensor`).
- When a single-program `.nsm` (`NEONNSM\0`) is passed to `GraphLoader`, it is transparently wrapped as a 1-fragment graph.

### `GraphRunner` Execution Flow
1. **Load:** `GraphLoader::loadFromMemory` parses the header, tensor table, and fragment table without allocating model memory. CONST tensor pointers point directly into the container buffer (zero-copy).
2. **Plan Arena:** `ArenaPlanner::plan` determines optimal tensor offsets within a single contiguous arena using greedy interval-graph coloring (INTERNAL tensors only).
3. **Bind Operands:** `OperandBinder` determines the memory pointer for each fragment's input and output tensors based on graph topology (including zero-copy pointers to embedded CONST tensors).
4. **Fragment Loop:** For each fragment:
   - `driver->beginFragment(f, binding)`
   - Load fragment `.nsm` slice into `NPUModel` via `ModelLoader`.
   - Apply generic address relocations via `OperandBinder::applyRelocations`, and device-specific relocations via `driver->relocateInstruction`.
   - Execute fragment via `Executor::run`.
   - Unload fragment to release scratch memory.
   - `driver->endFragment(f)`

### `GraphRunConfig` Memory Contract
The caller provides:
- `arena`: Workspace for intermediate activations (must be at least `graph.arena_bytes` bytes, 64-byte aligned).
- `scratch`: Shared scratch memory across all fragments (must be at least `graph.max_scratch_bytes` bytes).
- Caller input and output buffer pointers.

### Memory Layout on Large Models (`MODEL_IN_BLOB=1`)
For large models exceeding the 16 MiB firmware window (such as YOLOv8m ~44 MB total blob):
- `linker_segment.ld` defines two regions in the top 64 MiB of DRAM outside SoC peripherals (`0x8C000000 - 0x90000000`):
  - `BLOB`: `0x8C000000`, 48 MiB. Contains the `.segment_blob` section holding the linked graph container (`model.nsm`) and test vectors (`test_input.bin`, `test_output_tflite.bin`).
  - `ARENA`: `0x8F000000`, 16 MiB. NOLOAD region `.tensor_arena` mapped to `__tensor_arena_start` .. `__tensor_arena_end` passed to `GraphRunConfig::arena`.

### `NPUDriver` Hooks
- `relocateInstruction(instr_bytes, binding, is_write)`: Device driver inspects instruction opcodes and patches device-specific memory addresses/offsets.
- `beginFragment(fragment_index, binding)`: Notifies driver that a fragment is starting.
- `endFragment(fragment_index)`: Notifies driver that a fragment has completed.

---

## Device Independence Rule

`runtime/` is strictly device-agnostic and free of board-specific headers or SoC memory addresses. This is enforced by:

```bash
grep -rnE 'CDC_|soc_memory_map|soc/|"uart\.h"|0x8[0-9A-Fa-f]{7}|NPU_INPUT_COUNT|VP_FX1|NPUV4' \
     runtime --include='*.cpp' --include='*.h' --include='CMakeLists.txt'
```

Any SoC-specific peripherals, physical addresses, or register layouts belong exclusively in `device/<target>/`.

---

## Batch Inference (Rev3)

### Vấn đề

LLM decode phase với batch=1 đạt ~1% NPU utilization vì arithmetic intensity thấp:

```
Weight matrix (FC): 4096×4096 FP16 = 32 MB loaded per step
Input vector:       4096×1    FP16 = 8 KB
Arithmetic intensity ≈ 1 FLOP/byte vs FX1 roof ~100 FLOP/byte
```

Với batch=32, weight load được amortized qua 32 sequences → ~32× throughput.

### Giải pháp: pre-compiled batch profiles

Compiler (AXI4MLIR) generate nhiều instruction streams, mỗi stream tối ưu cho một batch size cụ thể. Runtime chọn đúng profile tại inference time.

```
model.nsm
├── .nsm_weights          (shared — load 1 lần)
├── .nsm_batch_profiles   (index: NeonBatchProfileEntry[])
├── stream_batch1         (compiled for batch=1)
├── stream_batch4         (compiled for batch=4)
├── stream_batch8         (compiled for batch=8)
├── stream_batch16        (compiled for batch=16)
└── stream_batch32        (compiled for batch=32)
```

### Flow

```
Producers (app threads)
    │  push(InferenceRequest)
    ▼
BatchAccumulator              flush khi: count ≥ max_batch OR timeout
    │  pop_batch() → BatchFlushResult{requests[], n_actual}
    ▼
ProfileSelector::select()     smallest profile với batch_size ≥ n_actual
    │
ProfileSelector::pack_inputs()  concat N inputs + zero-pad → 1 buffer
    │
Executor::run_batch()          execute compiled stream 1 lần
    │                          weight loaded once, reused N times
ProfileSelector::unpack_outputs()  copy N results → callers + on_complete(ok)
```

### `BatchAccumulator` — flush policy

```cpp
BatchAccumulator::Config cfg;
cfg.max_batch_size  = 32;    // flush ngay khi đủ
cfg.min_batch_size  = 1;     // flush ngay khi có ≥ 1 item
cfg.max_wait_us     = 5000;  // timeout 5ms từ item cũ nhất
cfg.max_queue_depth = 256;   // backpressure khi vượt quá
```

Tuning:

| Use case | `min_batch` | `max_batch` | `max_wait_us` |
|----------|-------------|-------------|--------------|
| LLM high throughput | = max | = 32 | 10 000 |
| Vision low latency | 1 | 4 | 2 000 |
| Balanced | 1 | 8 | 5 000 |

`try_pop_batch()` — non-blocking variant cho baremetal polling loop (không có OS/RTOS).

### `ProfileSelector`

```cpp
// Chọn profile
const BatchProfile* p = ProfileSelector::select(model, n_actual);
// p->batch_size >= n_actual, saturate tại profile lớn nhất

// Pack
auto in_buf  = ProfileSelector::pack_inputs(*p, requests, input_size);
// [item_0][item_1]...[item_{n-1}][0x00 padding...]

// Unpack + notify
ProfileSelector::unpack_outputs(*p, out_buf.data(), requests, output_size, ok);
// gọi req.on_complete(ok) cho từng item
```

Nếu `n_actual > maxBatchSize()` → cần nhiều hơn 1 Executor call:

```cpp
uint32_t calls = ProfileSelector::calls_needed(model, n_actual);
// n=11, max_profile=8 → calls=2 (8 items + 3 items)
```

---

## Public API (`NeonRuntime.h`)

```cpp
// Bare-metal / Graph execution pattern:
neon::NPUGraph graph;
neon::GraphLoader::loadFromMemory(model_bytes, model_size, graph);
neon::GraphRunner runner(graph, *driver);
neon::GraphRunConfig config;
config.arena_buffer = arena_ptr;
config.arena_size = graph.arena_size;
config.dram_input = input_ptr;
config.dram_output = output_ptr;
runner.prepare(config);
runner.run();

// Single-model — blocking
NPUModel* model = neon::load("model.nsm", "VP_FX1_V2.0");
neon::run(model, input, output);
neon::unload(model);

// Session-based — async submit + callback
NPUSession* sa = neon::createSession("vit_b.nsm",   "VP_FX1_V2.0");
NPUSession* sb = neon::createSession("yolov8m.nsm", "VP_FX1_V2.0");
neon::submit(sa, input_a, output_a, on_done_a);
neon::submit(sb, input_b, output_b, on_done_b);
neon::waitSession(sa);
neon::waitSession(sb);

// Dual session
neon::DualSession ds = neon::loadDual("vit_b.nsm", "yolov8m.nsm", "VP_FX1_V2.0");
neon::submit(ds.a, ...); neon::submit(ds.b, ...);
neon::unloadDual(ds);

// Batch inference (Rev3)
BatchAccumulator acc(cfg);
std::thread runner([&]{ /* pop_batch() loop */ });
acc.push(req);      // từ bất kỳ thread nào
acc.shutdown();     // khi xong
runner.join();
```

Xem [`apps/examples/neon_runtime_example/main.cpp`](../apps/examples/neon_runtime_example/main.cpp) để xem đầy đủ cả 4 usage patterns ([A] single-model, [B] dual-session, [C] frame-driven, [D] batch inference).

---

## Thêm hardware target mới

1. Tạo `device/<name>/runtime/<Name>Runtime.h`
2. Kế thừa cả `<Name>NPUTarget` và `NPUDriver` (multiple inheritance — xem `NPUV4Runtime.h`)
3. Override tất cả pure virtuals của `NPUDriver`
4. Nếu code generator cho hardware target emit device-specific memory addresses/offsets, override `relocateInstruction()` và `beginFragment()` (xem `device/VP_FX1_V2.0/NPUV4Relocator.cpp` làm mẫu tham chiếu).
5. Đăng ký với `NEON_REGISTER_TARGET(<Name>Runtime)` ở cuối `.cpp` file
6. Override `readPerfTotalCycles()` nếu hardware có perf counter

```cpp
class MyNPURuntime : public MyNPUTarget, public NPUDriver {
    // maxConcurrentModels() không cần override — kế thừa thẳng từ
    // MyNPUTarget (NPUComponent), lấy từ getNPUSpecs().max_concurrent_models.
    // NPUDriver không có virtual này (single source of truth ở NPUComponent).
    uint64_t readPerfTotalCycles() const override { return readReg(MY_REG_CYCLES); }
    // ... implement dmaTransfer, writeReg, trigger, waitDone ...
};
NEON_REGISTER_TARGET(MyNPURuntime)
```

---

## Build

Runtime không depend vào MLIR, nhưng không có host build — chỉ cross-compile
cho target thật:

```bash
# qemu_mps3_an547 — cross-compile + boot dưới QEMU (Makefile.qemu ở root repo)
make -f Makefile.qemu BENCH=fc_sweep
# → build/qemu/neon_eval.elf
make -f Makefile.qemu run BENCH=fc_sweep

# Baremetal VP_FX1_V2.0:
cd device/VP_FX1_V2.0 && make
```

`-fno-strict-aliasing` là bắt buộc cho target nào còn dùng MMIO thật (VP_FX1_V2.0) — UB với strict aliasing tại `-O2`. `QEMUMPS3Runtime` không chạm MMIO thật nên không phụ thuộc flag này để đúng, nhưng Makefile.qemu vẫn bật cho nhất quán.

`--whole-archive` (hoặc `-force_load` trên macOS/ld64 — xem `Makefile.qemu`) là bắt buộc để linker giữ các `NEON_REGISTER_TARGET` static initializers.

`batch/BatchAccumulator.h` và `batch/ProfileSelector.h` là header-only — không cần thêm `.cpp`.
`NPUDriver.h` là abstract — không compile trực tiếp.

---

## Quan hệ với các module khác

```
core/NPU_ISA.h          ← source of truth: opcodes, packed structs, file format
     │
     ├── toolkit/       compile time: emit NeonXxxInstr bytes, write NeonFileHeader
     │
     └── runtime/       inference time: parse NeonFileHeader, dispatch NeonXxxInstr
          │
          ├── analyse/  profiling hooks: MeasuredProfiler, DMAProfiler (optional)
          │
          └── device/   hardware impl: NPUDriver per target
```

`runtime/` không import bất cứ thứ gì từ `toolkit/`. Hai module này chỉ liên hệ qua `.nsm` binary file và `core/NPU_ISA.h`.
