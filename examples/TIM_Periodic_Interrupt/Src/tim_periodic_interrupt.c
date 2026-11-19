/**
 * @file tim_periodic_interrupt.c
 * @brief Implementation for timer periodic interrupt.
 * @details
 * GT32U2086 periodic timer interrupt; PB7 is the scope/logic-analyzer output.
 */

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/

#include "tim_periodic_interrupt.h"
#include <stdio.h>

#include "sdkconfig.h"
#include "crm/neon_crm.h"
#include "gpio/neon_gpio.h"
#include "tim/neon_tim.h"

#ifndef CONFIG_NEON_MDF_DRIVER_TIM
#error "TIM_Periodic_Interrupt requires CONFIG_NEON_MDF_DRIVER_TIM=y"
#endif

/*==================================================================================================
*                                        DEFINES AND MACROS
==================================================================================================*/
/* Two interrupts per second: LED on for 500 ms, off for 500 ms. */
#define TIM_PERIODIC_TICK_HZ 2U

/*==================================================================================================
*                                         LOCAL VARIABLES
==================================================================================================*/
volatile uint32_t g_tim_periodic_ticks;

/*==================================================================================================
*                                         GLOBAL FUNCTIONS
==================================================================================================*/

/**
 * @brief Handle the TIM1 IRQHandler interrupt.
 * @details
 * GT32U2086 routes TIM1 to IRQ13; the startup vector uses this handler name.
 */
void TIM1_IRQHandler(void) {
    uint32_t flags = 0U;
    neon_tim_get_active_flags(TIM1, NEON_TIM_CHANNEL_0, &flags);
    if ((flags & NEON_TIM_FLAG_UPDATE) == 0U) {
        return;
    }

    neon_gpio_toggle_pin(GPIOB, NEON_GPIO_PIN_7);
    ++g_tim_periodic_ticks;
    /* Wait until UPDATE reads inactive before leaving the ISR. */
    while ((TIM1->STT_CH0 & NEON_TIM_FLAG_UPDATE) != 0U) {
    }
    __DSB();
}

/** @copydoc tim_periodic_interrupt_run */
void tim_periodic_interrupt_run(neon_uart_handle_t *uart) {
    /* GT32U2086 FPGA feeds TIM1 from the system clock. */
    if (SystemCoreClock < 1000U || (SystemCoreClock % 1000U) != 0U) {
        printf("[ERROR] Timer clock must support a 1 kHz counter.\n");
        return;
    }

    uint32_t saved_primask = __get_PRIMASK();
    neon_status_t status = neon_gpio_clock_enable(GPIOB);
    if (status != NEON_OK) {
        printf("[ERROR] GPIO clock status=%lu\n", (unsigned long)status);
        return;
    }

    neon_gpio_config_t gpio_config;
    neon_gpio_get_default_config(&gpio_config);
    gpio_config.pin = NEON_GPIO_PIN_7;
    gpio_config.mode = NEON_GPIO_MODE_OUTPUT;
    gpio_config.speed = NEON_GPIO_SPEED_HIGH;
    gpio_config.slew_rate = NEON_GPIO_SLEW_FAST;
    status = neon_gpio_init(GPIOB, &gpio_config);
    if (status != NEON_OK) {
        printf("[ERROR] Timer GPIO init status=%lu\n", (unsigned long)status);
        return;
    }

    NVIC_DisableIRQ(TIM1_IRQn);
    NVIC_ClearPendingIRQ(TIM1_IRQn);
    /* Reserve TIM1 for this testcase. Reset affects both CH0 and CH1 and
   * clears the read-only counter so a previous run cannot leave its phase. */
    neon_crm_peripheral_clock_enable(NEON_CRM_PERIPHERAL_TIMER1);
    neon_crm_peripheral_reset(NEON_CRM_PERIPHERAL_TIMER1);
    neon_tim_deinit(TIM1, NEON_TIM_CHANNEL_1);
    g_tim_periodic_ticks = 0U;

    neon_tim_config_t tim_config;
    neon_tim_get_default_config(&tim_config);
    /* A 1 kHz counter with 500 steps per UPDATE gives a 500 ms period. */
    tim_config.prescaler = SystemCoreClock / 1000U - 1U;
    tim_config.auto_reload = 1000U / TIM_PERIODIC_TICK_HZ - 1U;
    tim_config.counter_mode = NEON_TIM_COUNTERMODE_DOWN;
    tim_config.auto_reload_mode = NEON_TIM_AUTORELOAD_ENABLE;
    tim_config.interrupts = NEON_TIM_INTERRUPT_UPDATE;
    /* Keep mode=DISABLE during setup; Start enables normal counting below. */
    status = neon_tim_init(TIM1, NEON_TIM_CHANNEL_0, &tim_config);
    if (status != NEON_OK) {
        printf("[ERROR] Timer init status=%lu\n", (unsigned long)status);
        goto cleanup;
    }

    printf("[INFO] TIM1 CH0 update interrupt on IRQ13, clock=%lu Hz.\n", (unsigned long)SystemCoreClock);
    printf("[INFO] PSC=%lu, CHL=%lu; PB7 toggles every 500 ms (LED 1 Hz).\n", (unsigned long)tim_config.prescaler,
           (unsigned long)tim_config.auto_reload);
    printf("[INFO] Ctrl+C stops the timer and example.\n");
    status = neon_tim_start(TIM1, NEON_TIM_CHANNEL_0);
    if (status != NEON_OK) {
        printf("[ERROR] Timer start status=%lu\n", (unsigned long)status);
        goto cleanup;
    }
    /* The reference FPGA loads CNT from CHL only while the channel is running. */
    neon_tim_set_auto_reload(TIM1, NEON_TIM_CHANNEL_0, tim_config.auto_reload);
    NVIC_ClearPendingIRQ(TIM1_IRQn);
    NVIC_EnableIRQ(TIM1_IRQn);
    /* BootROM v5 enters the app with PRIMASK=1. Unmask only after setup. */
    __enable_irq();

    uint32_t last_count = 0U;
    for (;;) {
        uint32_t count = g_tim_periodic_ticks;
        if ((count - last_count) >= TIM_PERIODIC_TICK_HZ) {
            /* Log once per second; printf stays outside the ISR. */
            printf("[TIM IRQ] ticks=%lu (+%lu)\n", (unsigned long)count, (unsigned long)(count - last_count));
            last_count = count;
        }

        uint32_t receive_word = 0U;
        status = neon_uart_receive(uart, (uint8_t *)&receive_word, 1U, 0U);
        if (status == NEON_TIMEOUT) {
            continue;
        }
        if (status != NEON_OK) {
            printf("[ERROR] Timer UART RX status=%lu error=0x%08lX\n", (unsigned long)status,
                   (unsigned long)neon_uart_get_error(uart));
            neon_uart_clear_error(uart);
            break;
        }
        if ((receive_word & 0xFFU) == 3U) {
            break;
        }
    }

cleanup:
    /* Stop the source before releasing PB7 and restoring the caller's mask. */
    NVIC_DisableIRQ(TIM1_IRQn);
    neon_tim_stop(TIM1, NEON_TIM_CHANNEL_0);
    neon_tim_disable_interrupt(TIM1, NEON_TIM_CHANNEL_0, NEON_TIM_INTERRUPT_ALL);
    NVIC_ClearPendingIRQ(TIM1_IRQn);
    neon_tim_deinit(TIM1, NEON_TIM_CHANNEL_0);
    neon_gpio_write_pin(GPIOB, NEON_GPIO_PIN_7, NEON_GPIO_PIN_RESET);
    neon_gpio_deinit(GPIOB, NEON_GPIO_PIN_7);
    __set_PRIMASK(saved_primask);
    printf("[INFO] Timer stopped. Total ticks=%lu\n", (unsigned long)g_tim_periodic_ticks);
}
