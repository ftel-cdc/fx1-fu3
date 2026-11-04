/**
 * @file main.c
 * @brief Implementation for freertos blinky.
 * @details
 * * FreeRTOS_Blinky uses two tasks to toggle two GPIO pins at different periods and demonstrate
 * preemptive scheduling on all three targets: FU2 (Cortex-M0, port
 * GCC_ARM_CM0), WEACT_F412 (Cortex-M4F, GCC_ARM_CM4F) and CH32V003F4P6
 * (RISC-V QingKe V2A, GCC_RISC_V_GENERIC; added 2026-08-10, the first
 * RISC-V target with FreeRTOS). See cmake/freertos.cmake for architecture-specific port selection.
 *
 * FU2 (ARM_CM0): the strong PendSV_Handler/SysTick_Handler definitions in FreeRTOS portasm.c/port.c
 * match the weak names in startup_FU2.s and override them at link time. No additional glue is
 * required.
 *
 * WEACT_F412 (ARM_CM4F) uses "Direct Routing": vPortSVCHandler/xPortPendSVHandler/xPortSysTickHandler
 * differ from the generated SVC_Handler/PendSV_Handler/SysTick_Handler names in
 * startup_WEACT_F412.s, so weak overriding does not happen automatically. Explicit routing is
 * required below. Without it, the vector-table configASSERT in xPortStartScheduler()
 * (pxVectorTable[SVC] == vPortSVCHandler) fails and hangs. Confirmed by reading the PC through SWD
 * during debugging.
 *
 * WEACT_F412: LED1_PIN (PB3) has no physical LED attached. The two tasks still demonstrate preemption
 * by calling neon_gpio_toggle_pin on different pins at different periods; only PB2 has a visible
 * onboard LED.
 *
 * CH32V003F4P6 (GCC_RISC_V_GENERIC): QingKe V2A has no standard RISC-V CLINT, which this port assumes
 * by default. It uses WCH PFIC (Programmable Fast Interrupt Controller), verified against the
 * official QingKeV2 Processor Manual V1.0 downloaded from ch405-labs.com:
 *   - mtvec[1:0]: bit0=0 selects "Direct mode". All exceptions and interrupts enter
 *     freertos_risc_v_trap_handler in portASM.S, which distinguishes causes through mcause. The
 *     shared startup_CH32V003F4P6.s instead selects "address mode" (mtvec[1:0]=3), with a vector
 *     per IRQ, for all apps including Blinky and the bootloader. This FreeRTOS app overrides mtvec
 *     in vPortSetupTimerInterrupt() below, once before xPortStartFirstTask() and before interrupts
 *     are enabled. It does not change the shared startup.s.
 *   - For interrupts (mcause[31]=1), mcause[30:0] contains the PFIC IRQ number, for example 12 = SysTick.
 *     Verified against manual Table 2-1, "V2 microprocessor
 *     exception codes"): "12 - Precise asynchronous - SysTick interrupts",
 *     "11 - Synchronous - Environment call in Machine mode" (matching exactly
 *     the existing `li t0, 11` in portASM.S for ecall/portYIELD(); no portASM.S changes are needed).
 *   - Tick timer: vPortSetupTimerInterrupt() overrides the weak port.c implementation to configure QingKe
 *     STK and enable its IRQ through PFIC IENR (SDK_IRQ_Enable(SysTick_IRQn)). STK addresses and
 *     bits differ from CLINT mtime; see manual chapter 4.
 *     freertos_risc_v_application_interrupt_handler(), another weak hook in portASM.S, reads
 *     mcause, recognizes SysTick and calls xTaskIncrementTick()/vTaskSwitchContext(). It cannot use
 *     the port.c portasmHAS_MTIME path: portasmHAS_MTIME=0 because there is no CLINT; see
 *     FreeRTOSConfig.h.
 *   - portYIELD() uses ecall for task switches outside interrupts, such as vTaskDelay. It is a standard
 *     synchronous RISC-V exception and works independently of PFIC/CLINT. It does not
 *     require additional code.
 *   - Hardware verification was pending when this was written because the RISC-V board had not arrived.
 *     The design above follows the official documentation; Phase 4/5 verification is recorded in
 *     .claude/plans/proud-sniffing-teacup.md.
 *
 * OTA (WEACT_F412 only): the same source is built twice (see
 * CMakeLists.txt): "freertos_blinky" (factory, NEON_OTA_APP_VERSION=1) and
 * "freertos_blinky_ota" (OTA slot, NEON_OTA_APP_VERSION=2). See examples/OTA_Demo/ for a fuller OTA
 * example. This app has no UART/printf, so the vLed1Task period identifies the version. It can be
 * measured by reading IDR through SWD, as previously used to verify preemption on WEACT_F412
 * hardware. After about 4s (20 vLed0Task iterations at 200ms), the app calls
 * neon_ota_confirm_valid().
 *
 * FU2 has no bootloader/OTA and retains a single executable. CH32V003F4P6 (added 2026-08-10) has a
 * bootloader that verifies the header/CRC32 and receives firmware through UART, but no OTA slot or
 * neon_ota_confirm_valid():
 * FreeRTOS-Kernel alone uses about 8.5KB flash/2KB RAM, measured from the
 * build, after reducing memory use: 1.25KB heap, 256B per task stack, timer service task disabled).
 * Two images (factory+OTA) plus the bootloader cannot fit in the physical 16KB flash/2KB RAM. See
 * CMakeLists.txt/flashmap.WEACT_F412.csv and .claude/plans/proud-sniffing-teacup.md.
 * On CH32V003F4P6, NEON_OTA_APP_VERSION=1 (defined in CMakeLists.txt) only determines period_ms for
 * vLed1Task; it does not mean
 * an "OTA version" as it does on WEACT_F412).
 */

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/

#include "FreeRTOS.h"
#include "task.h"

#include "gpio/neon_gpio.h"
#include "mcu_device.h"

#if defined(WEACT_F412)
/* CH32V003F4P6 does not use neon_ota.h because it has no OTA slot. See
 * CMakeLists.txt/flashmap.WEACT_F412.csv: FreeRTOS-Kernel alone needs about 8.5KB of flash, so two
 * images cannot fit in 16KB. The target still has a bootloader with header/CRC32 validation; it
 * simply has no OTA slot to confirm with neon_ota_confirm_valid(). */
#include "neon_ota.h"
#endif

#if defined(CH32V003F4P6)
#include "riscv_csr.h"
#include "sdk_irq.h"
#endif

#ifndef NEON_OTA_APP_VERSION

/*==================================================================================================
*                                        DEFINES AND MACROS
==================================================================================================*/
#define NEON_OTA_APP_VERSION 1
#endif

#if defined(FU2)
/* The former GPIO0 no longer exists. FU2.yaml declares the four actual GPIOA-D instances from
 * inc_fu2/fu2.h, with eight pins per port. GPIOA pins 0/1 are provisional choices pending hardware
 * and pin_map.md verification, not confirmed LED pins. */
#define LED_INSTANCE GPIOA
#define LED0_PIN (1U << 0)
#define LED1_PIN (1U << 1)
#elif defined(WEACT_F412)
#define LED_INSTANCE GPIOB
#define LED0_PIN (1U << 2) /* PB2 = LED that onboard */
#define LED1_PIN (1U << 3) /* PB3 has no LED; used to demonstrate preemption */
#elif defined(CH32V003F4P6)
#define LED_INSTANCE GPIOD
#define LED0_PIN (1U << 1) /* PD1 is the physical LED (see Blinky/Src/main.c) */
#define LED1_PIN                                                                                                       \
    (1U                                                                                                                \
     << 0) /* PD0 has no LED; used to demonstrate preemption. Physical board pin routing remains unverified. */
#else
#error "FreeRTOS_Blinky: LED_INSTANCE/LED0_PIN/LED1_PIN are not defined for this target"
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
/* QingKe V2A STK differs from CLINT mtime. These register addresses and bit names come from chapter 4
 * of QingKeV2 Processor Manual V1.0 and match platform/ch32v003/neon_bootloader_arch.c. They are
 * declared locally because that bootloader-internal file is not a shared API; both use the same
 * documented values. */
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
        SysTick_IRQn); /* PFIC IENR: STIE enables the interrupt only inside STK; PFIC must be enabled separately. Manual Table 3-1 lists SysTick as "Programmable", not "Fixed". */
    STK_CTLR = STK_CTLR_STE | STK_CTLR_STCLK | STK_CTLR_STRE | STK_CTLR_STIE;
}

/**
 * @brief Freertos risc v application interrupt handler.
 * @details
 * freertos_risc_v_application_interrupt_handler() is the weak portASM.S hook for every asynchronous
 * interrupt when portasmHAS_MTIME=0 (FreeRTOSConfig.h). The port.c timer-recognition path only
 * exists when portasmHAS_MTIME!=0, so identify SysTick here using mcause (manual Table 2-1: IRQ
 * 12), clear pending status, increment the tick and
 * switch tasks, exactly as freertos_risc_v_mtimer_interrupt_handler originally
 * does on CLINT-based chips.
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
    /* Ignore other IRQs, unused by this demo. Avoid the default infinite loop (`j .` in portASM.S), which
   * would hang on an accidentally enabled peripheral interrupt. */
}
#endif /*==================================================================================================
*                                   PRIVATE FUNCTION PROTOTYPES
==================================================================================================*/

static void freertos_blinky_v_led0_task(void *pvParameters);
static void freertos_blinky_v_led1_task(void *pvParameters);

/*==================================================================================================
*                                         GLOBAL FUNCTIONS
==================================================================================================*/

/**
 * @brief Initialize the application and run its main loop.
 * @return The application normally runs indefinitely.
 */
int main(void) {
    neon_gpio_config_t leds_init = {
        .pin = LED0_PIN | LED1_PIN,
        .mode = NEON_GPIO_MODE_OUTPUT,
    };
    neon_gpio_clock_enable(LED_INSTANCE);
    neon_gpio_init(LED_INSTANCE, &leds_init);

    xTaskCreate(freertos_blinky_v_led0_task, "led0", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, NULL);
    xTaskCreate(freertos_blinky_v_led1_task, "led1", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, NULL);

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
 * configCHECK_FOR_STACK_OVERFLOW == 2 (FreeRTOSConfig.h) requires the application to provide this
 * hook.
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

static void freertos_blinky_v_led0_task(void *pvParameters) {
    (void)pvParameters;
#if defined(WEACT_F412)
    uint32_t loop_count = 0;
#endif
    for (;;) {
        neon_gpio_toggle_pin(LED_INSTANCE, LED0_PIN);
        vTaskDelay(pdMS_TO_TICKS(200));

#if defined(WEACT_F412)
        loop_count++;
        if (loop_count == 20U) { /* About 4s of stable operation */
            neon_ota_confirm_valid();
        }
#endif
    }
}

static void freertos_blinky_v_led1_task(void *pvParameters) {
    (void)pvParameters;
    /* The LED1 period identifies the version because this app has no UART/printf: v1=500ms, v2=250ms.
   * Measure through SWD with repeated mdw reads of GPIOB->IDR; see the plan file. */
    const TickType_t period_ms = (NEON_OTA_APP_VERSION == 1) ? 500U : 250U;
    for (;;) {
        neon_gpio_toggle_pin(LED_INSTANCE, LED1_PIN);
        vTaskDelay(pdMS_TO_TICKS(period_ms));
    }
}
