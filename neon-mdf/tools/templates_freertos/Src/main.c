/**
 * @file main.c
 * @brief Implementation for templates freertos.
 * @details
 * * Minimal FreeRTOS app starting point: one task, no GPIO/LED demo, only printing the app
 * name repeatedly via printf() - proof-of-life to confirm pipeline mcu-description -> CMSIS/RISC-V
 * header/linker/startup
 * -> FreeRTOS-Kernel -> Firmware can run end-to-end (real scheduler, not superloop like regular
 * templates; see tools/templates/Src/main.c when FreeRTOS is unnecessary). Edit, remove or add
 * code freely; this is a starting point rather than mandatory application code.
 *
 * WEACT_F412 does not retarget printf() to UART; printf() commands on this target use the stub
 * newlib/nosys. CH32V003F4P6 (RISC-V) do not work printf() (toolchain bare-metal without real libc,
 * see arch/RISCV/README.md) - #include <stdio.h> must be checked (not just function calls)
 * otherwise it will fail at the preprocess step.
 *
 * Required architecture-specific glue: removing it prevents the scheduler from running, even though
 * build. See examples/FreeRTOS_Blinky/Src/main.c for the complete implementation and references):
 *   - WEACT_F412 (port ARM_CM4F "Direct Routing"): can SVC_Handler/
 *     PendSV_Handler/SysTick_Handler interleaved jump to vPortSVCHandler/
 *     xPortPendSVHandler/xPortSysTickHandler of the FreeRTOS - vector table generates non-magnetic
 *     generic use of this function name.
 *   - CH32V003F4P6 (port GCC_RISC_V_GENERIC, "no CLINT"): manually configure STK (QingKe's real SysTick)
 *     to create interrupt tick + change mtvec to Direct mode - the default RISC-V port assumes a
 *     real CLINT (register mtime) that this chip does not have.
 *   - FU2 (port ARM_CM0): no extra - weak-override generic function name (PendSV_Handler/SysTick_Handler)
 *     dynamically.
 *
 * CH32V003F4P6 has a bootloader with header/CRC32 validation and UART updates, but no
 * OTA (FreeRTOS-Kernel I lack has spent most 16KB flash/2KB real RAM of this chip, not child for 2
 * image copies. See examples/FreeRTOS_Blinky/Src/main.c for measurements and guidance when
 * recalculate Inc/FreeRTOSConfig.h requirements when the app grows beyond this template).
 */

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/

#include "FreeRTOS.h"
#include "task.h"
#include "mcu_device.h"

#if defined(FU2) || defined(WEACT_F412)
#include <stdio.h>
#endif

#if defined(CH32V003F4P6)
#include "riscv_csr.h"
#include "sdk_irq.h"
#endif

#if defined(WEACT_F412)
/* See above: ARM_CM4F uses "Direct Routing", so explicitly route the generated handler names to
 * FreeRTOS. GCC __attribute__((alias(...))) requires the aliased function body in the same
 * translation unit, while vPortSVCHandler/xPortPendSVHandler/xPortSysTickHandler are in port.c in
 * the separate freertos_kernel_port library. Attempting aliases produced "aliased to undefined
 * symbol". vPortSVCHandler/xPortPendSVHandler are naked functions that manage EXC_RETURN and the stack
 * for task switching; see port.c. Use a direct `b` branch rather than a
 * normal C call with `bl` and return, preserving the exception context as if NVIC called the
 * handler directly. xPortSysTickHandler is not naked, so a normal C call is sufficient. */
void vPortSVCHandler(void);
void xPortPendSVHandler(void);
void xPortSysTickHandler(void);

/**
 * @brief Handle the SVC Handler interrupt.
 */
__attribute__((naked)) void SVC_Handler(void) {
    __asm volatile("b vPortSVCHandler");
}

/**
 * @brief Handle the PendSV Handler interrupt.
 */
__attribute__((naked)) void PendSV_Handler(void) {
    __asm volatile("b xPortPendSVHandler");
}

/**
 * @brief Handle the SysTick Handler interrupt.
 */
void SysTick_Handler(void) {
    xPortSysTickHandler();
}
#endif

#if defined(CH32V003F4P6)

/*==================================================================================================
*                                        DEFINES AND MACROS
==================================================================================================*/
/* STK is QingKe V2A SysTick; its addresses/bits differ from CLINT mtime. Register addresses and bit names come
 * from QingKeV2 Processor Manual V1.0 chapter 4 and match
 * platform/ch32v003/neon_bootloader_arch.c (not at macro use since it is an internal bootloader
 * file. Declare them locally using the same documented values). */
#define STK_CTLR (*(volatile uint32_t *)0xE000F000UL)
#define STK_SR (*(volatile uint32_t *)0xE000F004UL)
#define STK_CNTR (*(volatile uint32_t *)0xE000F008UL)
#define STK_CMPR (*(volatile uint32_t *)0xE000F010UL)

#define STK_CTLR_STE (1U << 0)   /* System counter enable */
#define STK_CTLR_STIE (1U << 1)  /* Counter interrupt enable */
#define STK_CTLR_STCLK (1U << 2) /* 1 selects HCLK as the time base without division by 8 */
#define STK_CTLR_STRE (1U << 3)  /* Auto-reload: restart at 0 when CMPR is reached */

/**
 * @brief V port setup timer interrupt.
 * @details
 * vPortSetupTimerInterrupt() overrides the weak RISC-V port.c function because there is no CLINT.
 * xPortStartScheduler() calls it once before xPortStartFirstTask(), so changing mtvec here is safe:
 * interrupts are still disabled.
 */
void vPortSetupTimerInterrupt(void) {
    extern void freertos_risc_v_trap_handler(void);

    /* mtvec Direct mode (bits[1:0]=00). freertos_risc_v_trap_handler is already 8-byte aligned (`.align 8`
   * in portASM.S), so its low two bits are zero and no
   * OR operations are needed. From this point, every exception/interrupt, including ecall
   * from portYIELD()) enter this handler instead of the address-mode vector table in
   * startup_CH32V003F4P6.s. */
    RISCV_CSR_WRITE(mtvec, (uint32_t)freertos_risc_v_trap_handler);

    STK_CTLR = 0U; /* Disable before configuration, following the bootloader pattern */
    STK_CNTR = 0U;
    STK_SR = 0U;
    STK_CMPR = (SystemCoreClock / configTICK_RATE_HZ) - 1U;
    SDK_IRQ_Enable(
        SysTick_IRQn); /* PFIC IENR - "STIE" enables interrupt only in STK, PFIC must still be enabled separately (SysTick is "Programmable", not "Fixed") */
    STK_CTLR = STK_CTLR_STE | STK_CTLR_STCLK | STK_CTLR_STRE | STK_CTLR_STIE;
}

/**
 * @brief Freertos risc v application interrupt handler.
 * @details
 * freertos_risc_v_application_interrupt_handler() - another "weak" function of portASM.S, called for
 * each asynchronous interrupt when portasmHAS_MTIME=0 (see Inc/FreeRTOSConfig.h) - port.c does not
 * identify the tick timer automatically, so the application must handle it here:
 * read mcause (IRQ 12 = SysTick), clear pending status, increment the tick and switch tasks, as
 * freertos_risc_v_mtimer_interrupt_handler does on CLINT-based chips.
 */
void freertos_risc_v_application_interrupt_handler(void) {
    uint32_t mcause = RISCV_CSR_READ(mcause);
    uint32_t code = mcause & 0x7FFFFFFFU; /* bit31 (interrupt) is already known to be 1 on entry */

    if (code == (uint32_t)SysTick_IRQn) {
        STK_SR = 0U; /* Clear CNTIF: writing 0 clears it; writing 1 has no effect (reference manual) */
        if (xTaskIncrementTick() != pdFALSE) {
            vTaskSwitchContext();
        }
    }
    /* Other IRQs (not used in this template) - ignore, do not loop infinitely like default handler so as
   * not to "hang" falsely if interrupted from another peripheral. */
}
#endif /*==================================================================================================
*                                   PRIVATE FUNCTION PROTOTYPES
==================================================================================================*/

static void templates_freertos_v_main_task(void *pvParameters);

/*==================================================================================================
*                                         GLOBAL FUNCTIONS
==================================================================================================*/

/**
 * @brief Initialize the application and run its main loop.
 * @return The application normally runs indefinitely.
 */
int main(void) {
    xTaskCreate(templates_freertos_v_main_task, "main", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, NULL);
    vTaskStartScheduler();

    /* Reached only if creating the idle/timer task exhausts the heap; not expected during normal
   * operation. */
    for (;;) {
    }
}

/**
 * @brief V application stack overflow hook.
 * @param[in] xTask X task.
 * @param[in,out] pcTaskName Pc task name.
 * @details
 * configCHECK_FOR_STACK_OVERFLOW == 2 (see Inc/FreeRTOSConfig.h) forces the app to be of this level.
 */
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName) {
    (void)xTask;
    (void)pcTaskName;
    taskDISABLE_INTERRUPTS();
    for (;;) {
    }
}

/*==================================================================================================
*                                        PRIVATE FUNCTIONS
==================================================================================================*/

/* CH32V003F4P6 */

static void templates_freertos_v_main_task(void *pvParameters) {
    (void)pvParameters;
    for (;;) {
#if defined(FU2) || defined(WEACT_F412)
        printf("__APP_NAME__ is running\n");
#endif
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
