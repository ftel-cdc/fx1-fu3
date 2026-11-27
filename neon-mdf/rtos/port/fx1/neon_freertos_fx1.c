/* CLINT tick do port FreeRTOS quản lý; file này nối MEIP -> PLIC -> ISR IP. */
#include "neon_freertos_fx1.h"
#include "task.h"

#if configENABLE_FPU != 0
#error "FX1 VP luu FPU bang chip extension; dat configENABLE_FPU=0 de khong luu hai lan"
#endif

extern void freertos_risc_v_trap_handler(void);

static struct {
    NEON_FX1_IRQ_Handler handler;
    void *user;
} irq_handlers[FX1_PLIC_NUM_SOURCES];

void NEON_FX1_FreeRTOS_Init(void)
{
    configASSERT(fx1_hartid() == 0);
    uint32_t mstatus;
    __asm__ volatile("csrr %0, mstatus" : "=r"(mstatus));
    configASSERT((mstatus & FX1_MSTATUS_MIE) == 0);
    for (uint32_t id = 1; id < FX1_PLIC_NUM_SOURCES; ++id)
        fx1_plic_enable(FX1_PLIC_CONTEXT_HART_M(0), id, 0);
    fx1_plic_set_threshold(FX1_PLIC_CONTEXT_HART_M(0), 0);
    fx1_set_msip(0, 0);
    __asm__ volatile("csrw mtvec, %0" :: "r"(&freertos_risc_v_trap_handler) : "memory");
    fx1_mb();
    fx1_irq_enable(FX1_MIE_MEIE);
}

BaseType_t NEON_FX1_IRQ_Register(uint32_t source, uint32_t priority,
                                NEON_FX1_IRQ_Handler handler, void *user)
{
    if (fx1_hartid() != 0 || source == 0 || source >= FX1_PLIC_NUM_SOURCES ||
        priority == 0 || priority > FX1_PLIC_PRIORITY_MAX || handler == NULL)
        return pdFAIL;
    uint32_t saved = SDK_IRQ_EnterCritical();
    if (irq_handlers[source].handler != NULL) {
        SDK_IRQ_ExitCritical(saved);
        return pdFAIL;
    }
    SDK_IRQ_Disable((int32_t)source);
    irq_handlers[source].handler = handler;
    irq_handlers[source].user = user;
    SDK_IRQ_SetPriority((int32_t)source, priority);
    SDK_IRQ_Enable((int32_t)source);
    SDK_IRQ_ExitCritical(saved);
    return pdPASS;
}

BaseType_t NEON_FX1_IRQ_Unregister(uint32_t source,
                                  NEON_FX1_IRQ_Handler handler, void *user)
{
    if (fx1_hartid() != 0 || source == 0 || source >= FX1_PLIC_NUM_SOURCES || !handler)
        return pdFAIL;
    uint32_t saved = SDK_IRQ_EnterCritical();
    if (irq_handlers[source].handler != handler || irq_handlers[source].user != user) {
        SDK_IRQ_ExitCritical(saved);
        return pdFAIL;
    }
    SDK_IRQ_Disable((int32_t)source);
    fx1_mb();
    irq_handlers[source].handler = NULL;
    irq_handlers[source].user = NULL;
    fx1_mb();
    SDK_IRQ_ExitCritical(saved);
    return pdPASS;
}

void freertos_risc_v_application_interrupt_handler(uint32_t mcause)
{
    if (mcause != (FX1_MCAUSE_INTERRUPT | FX1_IRQ_M_EXT)) fx1_fail(0xF101);
    uint32_t source = fx1_plic_claim(FX1_PLIC_CONTEXT_HART_M(0));
    BaseType_t woken = pdFALSE;
    if (source != 0) {
        if (source >= FX1_PLIC_NUM_SOURCES || irq_handlers[source].handler == NULL)
            fx1_fail(0xF102);
        woken = irq_handlers[source].handler(irq_handlers[source].user);
        fx1_mb();
        fx1_plic_complete(FX1_PLIC_CONTEXT_HART_M(0), source);
        fx1_mb();
    }
    portYIELD_FROM_ISR(woken);
}

void freertos_risc_v_application_exception_handler(uint32_t mcause)
{
    uint32_t mepc, mtval;
    __asm__ volatile("csrr %0, mepc" : "=r"(mepc));
    __asm__ volatile("csrr %0, mtval" : "=r"(mtval));
    fx1_log_hex("FX1 exception mcause", mcause);
    fx1_log_hex("mepc", mepc);
    fx1_log_hex("mtval", mtval);
    fx1_fail(0xF200u | (mcause & 0xffu));
}

void NEON_FX1_RTOS_Assert(const char *file, int line)
{
    fx1_log(file);
    fx1_log_hex("FreeRTOS assert line", (uint32_t)line);
    fx1_fail(0xF300);
}

void vApplicationStackOverflowHook(TaskHandle_t task, char *name)
{
    (void)task;
    fx1_log(name);
    fx1_fail(0xF301);
}

void vApplicationMallocFailedHook(void)
{
    fx1_fail(0xF302);
}

void vApplicationIdleHook(void)
{
    fx1_wfi();
}
