#include "neon_npu.h"
#include "RuntimeInputProvider.h"
#include "runtime/capi/NeonPlatform.h"
#include <neon/c_runtime_api.h>
#include <dlpack/dlpack.h>

extern "C" {
#include "FreeRTOS.h"
#include "task.h"
#include "uart.h"
#include "plic.h"
}

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#ifndef NEON_MODEL_BYTES
#error "Build must supply NEON_MODEL_BYTES; the .nsm itself is loaded separately into IFLASH"
#endif

namespace {
constexpr uintptr_t kModelFlashBase = 0x04000000u;
constexpr uintptr_t kModelBlobBase = 0x8C000000u;
constexpr size_t kModelBlobBytes = 48u * 1024u * 1024u;
constexpr uintptr_t kInputBase = 0x81000000u;
constexpr uintptr_t kOutputBase = 0x82000000u;
constexpr uintptr_t kScratchBase = 0x88000000u;
constexpr size_t kScratchBytes = 48u * 1024u * 1024u;
constexpr uintptr_t kArenaBase = 0x8F000000u;
constexpr size_t kArenaBytes = 16u * 1024u * 1024u;
constexpr size_t kInputBytes = 1u * 3u * 32u * 32u;
constexpr size_t kOutputBytes = 2u;
constexpr uint32_t kGaussianSeed = 0x4E454F4Eu;

alignas(16) uint8_t g_runtime_heap[12u * 1024u * 1024u];
size_t g_runtime_heap_used;

void *runtimeAlloc(size_t bytes)
{
    bytes = (bytes + 15u) & ~size_t(15u);
    if (bytes == 0 || bytes > sizeof(g_runtime_heap) - g_runtime_heap_used) {
        uart_put_u32("[NEON_E2E] ERROR heap allocation bytes=", (uint32_t)bytes, "\r\n");
        return nullptr;
    }
    void *p = g_runtime_heap + g_runtime_heap_used;
    g_runtime_heap_used += bytes;
    return p;
}

uintptr_t heapMark() { return g_runtime_heap_used; }
void heapRelease(uintptr_t mark)
{
    if (mark <= g_runtime_heap_used) g_runtime_heap_used = (size_t)mark;
}

void stopFirmware()
{
    uart_puts("[NEON_E2E] TEST COMPLETE\r\n");
    __asm__ volatile("unimp");
    vTaskDelete(nullptr);
}

void fail(const char *where)
{
    uart_puts("[NEON_E2E] FAIL ");
    uart_puts(where);
    uart_puts("\r\n");
    uart_puts(NeonRuntimeGetLastError());
    uart_puts("\r\n");
    stopFirmware();
}

uint32_t fnv1a(const uint8_t *p, size_t n)
{
    uint32_t h = 2166136261u;
    for (size_t i = 0; i < n; ++i) h = (h ^ p[i]) * 16777619u;
    return h;
}

DLTensor makeTensor(void *data, int32_t ndim, int64_t *shape)
{
    DLTensor t{};
    t.data = data;
    t.device.device_type = kDLCPU;
    t.device.device_id = 0;
    t.ndim = ndim;
    t.dtype.code = kDLInt;
    t.dtype.bits = 8;
    t.dtype.lanes = 1;
    t.shape = shape;
    t.strides = nullptr;
    t.byte_offset = 0;
    return t;
}

void fragmentProgress(void *, uint32_t frag_idx, int32_t phase, int32_t status,
                      const uint8_t *, size_t out_bytes)
{
    if (phase == 0) {
        uart_put_u32("[NEON_E2E] FRAGMENT_BEGIN index=", frag_idx, "\r\n");
        return;
    }
    uart_put_u32("[NEON_E2E] FRAGMENT_END index=", frag_idx, "");
    uart_put_u32(" status=", static_cast<uint32_t>(status), "");
    uart_put_u32(" bytes=", static_cast<uint32_t>(out_bytes), "\r\n");
}

void inferenceTask(void *)
{
    uart_puts("[NEON_E2E] VP_FX1_V2.0 graph inference start\r\n");
    uart_put_u32("[NEON_E2E] model flash=0x04000000 bytes=", NEON_MODEL_BYTES, "\r\n");

    const uint8_t *flash_model = reinterpret_cast<const uint8_t *>(kModelFlashBase);
    static const uint8_t kGraphMagic[8] = {'N','E','O','N','G','R','F',0};
    if (NEON_MODEL_BYTES > kModelBlobBytes) {
        fail("model exceeds BLOB DDR capacity");
        return;
    }
    if (memcmp(flash_model, kGraphMagic, sizeof(kGraphMagic)) != 0) {
        fail("model missing from IFLASH or not NEONGRF");
        return;
    }
    auto *model_blob = reinterpret_cast<uint8_t *>(kModelBlobBase);
    memcpy(model_blob, flash_model, NEON_MODEL_BYTES);
    const uint8_t *model = model_blob;
    uart_put_u32("[NEON_E2E] model copied to BLOB=0x8C000000 bytes=",
                 NEON_MODEL_BYTES, "\r\n");

    auto *input = reinterpret_cast<uint8_t *>(kInputBase);
    auto *output = reinterpret_cast<uint8_t *>(kOutputBase);
    memset(output, 0xA5, kOutputBytes);
    uart_puts("[NEON_E2E] input buffer RAW_IN0=0x81000000\r\n");

    static neon_npu_t npu;
    if (npu.open() != NPUDriverStatus::OK) {
        fail("NPUV4Runtime::open");
        return;
    }
    static NeonPlatform platform{&npu, &npu, &npu};
    NeonGraphHandle graph = nullptr;
    if (NeonGraphLoad(model, NEON_MODEL_BYTES, &platform, 1, &graph) != 0) {
        fail("NeonGraphLoad");
        return;
    }

    NeonGraphInfo info{};
    info.struct_size = sizeof(info);
    if (NeonGraphGetInfo(graph, &info) != 0) {
        fail("NeonGraphGetInfo");
        return;
    }
    uart_put_u32("[NEON_E2E] fragments=", (uint32_t)info.num_fragments, "");
    uart_put_u32(" tensors=", (uint32_t)info.num_tensors, "\r\n");
    if (info.num_inputs != 1 || info.num_outputs != 1 ||
        info.num_fragments != 10 || info.arena_bytes > kArenaBytes ||
        info.scratch_bytes > kScratchBytes) {
        fail("graph I/O/fragment/arena contract mismatch");
        return;
    }

    int64_t out_shape[2] = {1, (int64_t)kOutputBytes};
    DLTensor in{};
    if (RuntimeNpuAppPrepareInput(input, kInputBytes, &in) != 0) {
        fail("RuntimeNpuAppPrepareInput");
        return;
    }
    uart_puts("[NEON_E2E] INPUT_READY addr=0x81000000 layout=NCHW shape=[1,3,32,32] dtype=int8 bytes=3072 hash=");
    uart_put_u32("", fnv1a(input, kInputBytes), "\r\n");
    DLTensor out = makeTensor(output, 2, out_shape);
    const DLTensor *inputs[] = {&in};
    DLTensor *outputs[] = {&out};

    NeonGraphMemory mem{};
    mem.struct_size = sizeof(mem);
    mem.arena = reinterpret_cast<void *>(kArenaBase);
    mem.arena_bytes = kArenaBytes;
    mem.scratch = reinterpret_cast<void *>(kScratchBase);
    mem.scratch_bytes = kScratchBytes;
    mem.heap_mark = heapMark;
    mem.heap_release = heapRelease;

    if (NeonGraphSetFragmentCallback(graph, fragmentProgress, nullptr) != 0) {
        fail("NeonGraphSetFragmentCallback");
        return;
    }
    if (NeonGraphPrepare(graph, &mem) != 0) {
        fail("NeonGraphPrepare");
        return;
    }
    uart_puts("[NEON_E2E] prepared; running 10 fragments\r\n");
    if (NeonGraphRun(graph, inputs, 1, outputs, 1) != 0) {
        fail("NeonGraphRun");
        return;
    }

    if (output[0] == 0xA5 && output[1] == 0xA5) {
        fail("output buffer still contains sentinel");
        return;
    }
    uart_puts("[NEON_E2E] OUTPUT_OK addr=0x82000000 bytes=2 values=");
    uart_put_i32("", (int8_t)output[0], " ");
    uart_put_i32("", (int8_t)output[1], "");
    uart_put_u32(" hash=", fnv1a(output, kOutputBytes), "\r\n");

    NeonGraphFree(graph);
    npu.close();
    uart_puts("[NEON_E2E] PASS (graph execution completed; no golden comparison for synthetic input)\r\n");
    stopFirmware();
}
} // namespace

void *operator new(size_t n) { return runtimeAlloc(n); }
void *operator new[](size_t n) { return runtimeAlloc(n); }
void operator delete(void *) noexcept {}
void operator delete[](void *) noexcept {}
void operator delete(void *, size_t) noexcept {}
void operator delete[](void *, size_t) noexcept {}

extern "C" void *malloc(size_t n) { return runtimeAlloc(n); }
extern "C" void free(void *) {}
extern "C" void *realloc(void *p, size_t n)
{
    if (!p) return runtimeAlloc(n);
    void *next = runtimeAlloc(n);
    if (next) memcpy(next, p, n);
    return next;
}
extern "C" int atexit(void (*)(void)) { return 0; }
extern "C" void *_sbrk(ptrdiff_t) { return reinterpret_cast<void *>(-1); }
extern "C" int _close(int) { return -1; }
extern "C" int _fstat(int, void *) { return 0; }
extern "C" int _isatty(int) { return 1; }
extern "C" int _lseek(int, int, int) { return 0; }
extern "C" int _read(int, char *, int) { return 0; }
extern "C" int _write(int, const char *p, int n)
{
    if (p) for (int i = 0; i < n; ++i) uart_putc(p[i]);
    return n;
}

extern "C" int main(void)
{
    plic_init();
    uart_init();
    uart_puts("[NEON_E2E] MDF runtime firmware booted\r\n");
    RuntimeNpuAppSetGaussianSeed(kGaussianSeed);
    uart_puts("[NEON_E2E] default input=seeded Gaussian int8 seed=0x4E454F4E\r\n");
    if (xTaskCreate(inferenceTask, "neon_graph", 8192, nullptr, 2, nullptr) != pdPASS) {
        uart_puts("[NEON_E2E] FAIL xTaskCreate\r\n");
        for (;;) {}
    }
    vTaskStartScheduler();
    uart_puts("[NEON_E2E] FAIL scheduler returned\r\n");
    for (;;) {}
}

extern "C" void vApplicationIdleHook(void) { __asm__ volatile("wfi"); }
extern "C" void vApplicationStackOverflowHook(TaskHandle_t, char *name)
{
    portDISABLE_INTERRUPTS();
    uart_puts2("[NEON_E2E] FAIL stack overflow: ", name);
    for (;;) {}
}
extern "C" void vApplicationMallocFailedHook(void)
{
    portDISABLE_INTERRUPTS();
    uart_puts("[NEON_E2E] FAIL FreeRTOS allocation\r\n");
    for (;;) {}
}
extern "C" void vAssertCalled(const char *, int)
{
    portDISABLE_INTERRUPTS();
    uart_puts("[NEON_E2E] FAIL assert\r\n");
    for (;;) {}
}
