/* Kiểm chứng nền tảng FreeRTOS; chưa sử dụng driver ISP hoặc dữ liệu RAW. */
#include "neon_freertos_fx1.h"
#include "neon_freertos_target.h"
#include "task.h"

static volatile uint32_t data_marker = 0x13579bdf;
static volatile uint32_t bss_marker;
static uint32_t checks;
static TaskHandle_t monitor_handle, irq_task_handle;
static volatile uint32_t irq_count, irq_wakes;
static volatile uint32_t worker_passes[2], worker_live[2], overlap_seen[2];
volatile uint32_t fx1_smoke_ticks;

extern uint8_t __bss_start[], __bss_end[], __stack_base[], __stack_end[];
extern void freertos_risc_v_trap_handler(void);
extern uint32_t fx1_smoke_fpu_probe(uint32_t pattern, uint32_t fcsr,
                                   volatile uint32_t *ticks, uint32_t wait_ticks);

void vApplicationTickHook(void)
{
    ++fx1_smoke_ticks;
}

static void check(int ok, const char *name)
{
    ++checks;
    fx1_puts(ok ? "[FX1_VP] PASS " : "[FX1_VP] FAIL ");
    fx1_putdec(checks);
    fx1_puts(" ");
    fx1_puts(name);
    fx1_putc('\n');
    if (!ok) fx1_fail(0xf400u + checks);
}

static uint32_t csr_mstatus(void)
{
    uint32_t value;
    __asm__ volatile("csrr %0, mstatus" : "=r"(value));
    return value;
}

static BaseType_t test_irq(void *user)
{
    BaseType_t woken = pdFALSE;
    /* IRQ test VP là level-sensitive: hạ line trước PLIC complete. */
    fx1_sim_irq_set(0);
    ++irq_count;
    vTaskNotifyGiveFromISR((TaskHandle_t)user, &woken);
    return woken;
}

static void irq_task(void *arg)
{
    (void)arg;
    for (;;) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        ++irq_wakes;
        xTaskNotifyGive(monitor_handle);
    }
}

static void fpu_task(void *arg)
{
    uint32_t id = (uint32_t)(uintptr_t)arg;
    uint32_t pattern = id == 0 ? 0x3f000000u : 0x41000000u;
    uint32_t fcsr = id == 0 ? 0x21u : 0x42u;
    for (uint32_t round = 0; round < 8; ++round) {
        worker_live[id] = 1;
        if (worker_live[id ^ 1u]) overlap_seen[id] = 1;
        /* Không yield hoặc gọi OS trong probe: phải bị tick preempt thật. */
        uint32_t error = fx1_smoke_fpu_probe(pattern + round * 64u, fcsr,
                                             &fx1_smoke_ticks, 4);
        worker_live[id] = 0;
        if (error) {
            fx1_log_hex("FPU worker", id);
            fx1_log_hex("FPU bad register (1..32), fcsr=33", error);
            fx1_fail(0xf500u + error);
        }
        ++worker_passes[id];
    }
    vTaskSuspend(NULL);
    fx1_fail(0xf501);
}

static void monitor_task(void *arg)
{
    (void)arg;
    check(worker_passes[0] == 8 && worker_passes[1] == 8,
          "f0..f31 and fcsr preserved in both tasks");
    check(overlap_seen[0] && overlap_seen[1], "equal-priority tasks preempt without yielding");
    check(fx1_smoke_ticks >= 32, "CLINT tick hook executed");
    check((csr_mstatus() & FX1_MSTATUS_MIE) != 0, "MIE enabled in task context");

    uint32_t outer = SDK_IRQ_EnterCritical();
    uint32_t inner = SDK_IRQ_EnterCritical();
    int masked = (csr_mstatus() & FX1_MSTATUS_MIE) == 0;
    SDK_IRQ_ExitCritical(inner);
    int still_masked = (csr_mstatus() & FX1_MSTATUS_MIE) == 0;
    SDK_IRQ_ExitCritical(outer);
    check(masked && still_masked && (csr_mstatus() & FX1_MSTATUS_MIE),
          "nested SDK critical section restores MIE");

    TickType_t before_tick = xTaskGetTickCount();
    uint64_t before_time = fx1_mtime();
    vTaskDelay(pdMS_TO_TICKS(10));
    uint64_t elapsed = fx1_mtime() - before_time;
    TickType_t elapsed_ticks = xTaskGetTickCount() - before_tick;
    fx1_log_hex("mtime delta for delay(10 ms)", (uint32_t)elapsed);
    check(elapsed_ticks >= 10 && elapsed >= FX1_MTIME_HZ * 9u / 1000u &&
          elapsed < FX1_MTIME_HZ * 20u / 1000u, "tick 20 MHz / 1 kHz and vTaskDelay");

    const uint32_t source = FX1_SIM_CTRL_IRQ_FIRST_SOURCE;
    check(NEON_FX1_IRQ_Register(0, 1, test_irq, irq_task_handle) == pdFAIL &&
          NEON_FX1_IRQ_Register(source, 0, test_irq, irq_task_handle) == pdFAIL,
          "invalid PLIC registration rejected");
    check(NEON_FX1_IRQ_Register(source, 2, test_irq, irq_task_handle) == pdPASS,
          "PLIC handler registered");
    check(fx1_read32(FX1_PLIC_BASE + FX1_PLIC_PRIORITY_OFF + 4u * source) == 2,
          "PLIC priority readback");
    SDK_IRQ_Disable((int32_t)source);
    fx1_sim_irq_set(1);
    vTaskDelay(2);
    check(irq_count == 0 && (fx1_plic_pending() & (1u << source)),
          "masked PLIC source stays pending");
    SDK_IRQ_Enable((int32_t)source);
    check(ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(100)) != 0 &&
          irq_count == 1 && irq_wakes == 1,
          "PLIC ISR wakes higher-priority task");
    check(fx1_sim_irq_get() == 0 &&
          fx1_plic_claim(FX1_PLIC_CONTEXT_HART_M(0)) == 0,
          "device ACK before PLIC complete");
    fx1_sim_irq_set(1);
    check(ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(100)) != 0 &&
          irq_count == 2 && irq_wakes == 2,
          "PLIC complete allows second interrupt");
    SDK_IRQ_Disable((int32_t)source);
    check(fx1_secondary_release == 0, "hart 1 remains parked; single-core FreeRTOS");
    check(uxTaskGetStackHighWaterMark(NULL) > 64, "monitor task stack margin");
    fx1_puts("[FX1_VP] SUMMARY checks=");
    fx1_putdec(checks);
    fx1_puts(" failed=0\n");
    fx1_pass();
}

int main(void)
{
    fx1_log("Neon-MDF FX1_VP FreeRTOS platform smoke");
    fx1_puts("[FX1_VP] kernel=");
    fx1_puts(tskKERNEL_VERSION_NUMBER);
    fx1_putc('\n');
    check(fx1_hartid() == 0, "main runs on hart 0");
    check(data_marker == 0x13579bdf && bss_marker == 0, "startup data/bss");
    uint32_t sp;
    __asm__ volatile("mv %0, sp" : "=r"(sp));
    check((sp & 15u) == 0 && sp > FX1_FW_STACK_BASE &&
          sp <= FX1_FW_STACK_BASE + FX1_FW_STACK_SIZE, "startup stack alignment and range");
    check((uintptr_t)__bss_start >= FX1_FW_IMAGE_BASE &&
          (uintptr_t)__bss_end <= FX1_FW_IMAGE_BASE + FX1_FW_IMAGE_SIZE &&
          (uintptr_t)__stack_base == FX1_FW_STACK_BASE &&
          (uintptr_t)__stack_end == FX1_FW_STACK_BASE + FX1_FW_STACK_SIZE * FX1_NUM_HARTS,
          "linker image and per-hart stack layout");
    check((csr_mstatus() & FX1_MSTATUS_MIE) == 0, "interrupts masked before scheduler");
    check(NEON_FREERTOS_TARGET_HAS_FPU == 1, "target capability declares FPU");
    /* Ghi lại FS trước/sau một lệnh FP để đối chiếu cơ chế dirty của CPU VP. */
    uint32_t fs_before, fs_after;
    __asm__ volatile("csrr %0, mstatus\nfmv.w.x ft0, zero\ncsrr %1, mstatus"
                     : "=&r"(fs_before), "=r"(fs_after) :: "ft0");
    fx1_log_hex("mstatus before FP", fs_before);
    fx1_log_hex("mstatus after FP", fs_after);
    NEON_FX1_FreeRTOS_Init();
    uint32_t mtvec;
    __asm__ volatile("csrr %0, mtvec" : "=r"(mtvec));
    check(mtvec == (uint32_t)(uintptr_t)&freertos_risc_v_trap_handler,
          "mtvec points to FreeRTOS trap entry");
    fx1_sim_irq_set(0);
    check(xTaskCreate(monitor_task, "monitor", 1024, NULL, 1, &monitor_handle) == pdPASS,
          "create monitor task");
    check(xTaskCreate(irq_task, "irq_waiter", 512, NULL, 3, &irq_task_handle) == pdPASS,
          "create IRQ waiter task");
    check(xTaskCreate(fpu_task, "fpu_A", 512, (void *)(uintptr_t)0, 2, NULL) == pdPASS &&
          xTaskCreate(fpu_task, "fpu_B", 512, (void *)(uintptr_t)1, 2, NULL) == pdPASS,
          "create two FPU tasks");
    vTaskStartScheduler();
    fx1_fail(0xf600);
}
