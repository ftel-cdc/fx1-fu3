/**
 * @file gpio_interrupt.c
 * @brief Implementation for GPIO interrupt.
 * @details
 * FU2 GPIOB interrupt counter; UART output is handled outside the ISR.
 */

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/

#include "gpio_interrupt.h"
#include <stdio.h>

#include "gpio/neon_gpio.h"

/*==================================================================================================
*                                         LOCAL VARIABLES
==================================================================================================*/
volatile uint32_t g_gpio_interrupt_count;

/*==================================================================================================
*                                         GLOBAL FUNCTIONS
==================================================================================================*/

/**
 * @brief Handle the EXTI1 IRQHandler interrupt.
 * @details
 * Common Spec v8 routes GPIOB to IRQ20. The SDK startup names this vector
 * EXTI1_IRQHandler and the corresponding IRQn EXTI1_IRQn.
 */
void EXTI1_IRQHandler(void) {
    uint32_t pending = 0U;
    neon_gpio_get_interrupt_status(GPIOB, &pending);
    if ((pending & NEON_GPIO_PIN_5) != 0U) {
        /* Clear the W1C source and software injection before leaving the ISR. */
        neon_gpio_clear_interrupt(GPIOB, NEON_GPIO_PIN_5);
        ++g_gpio_interrupt_count;
    }
}

/** @copydoc gpio_interrupt_run */
void gpio_interrupt_run(neon_uart_handle_t *uart) {
    uint32_t saved_primask = __get_PRIMASK();
    neon_status_t status = neon_gpio_clock_enable(GPIOB);
    if (status != NEON_OK) {
        printf("[ERROR] GPIO clock status=%lu\n", (unsigned long)status);
        return;
    }

    NVIC_DisableIRQ(EXTI1_IRQn);
    NVIC_ClearPendingIRQ(EXTI1_IRQn);
    g_gpio_interrupt_count = 0U;

    neon_gpio_config_t config;
    neon_gpio_get_default_config(&config);
    config.pin = NEON_GPIO_PIN_5;
    config.mode = NEON_GPIO_MODE_INPUT;
    config.pull = NEON_GPIO_PULL_UP;
    config.input_sync = NEON_GPIO_INPUT_SYNC_ENABLE;
    config.interrupt = NEON_GPIO_INTERRUPT_ENABLE;
    status = neon_gpio_init(GPIOB, &config);
    if (status != NEON_OK) {
        printf("[ERROR] GPIO interrupt init status=%lu\n", (unsigned long)status);
    } else {
        printf("[INFO] PB5 rising-edge interrupt on IRQ20 (EXTI1).\n");
        printf("[INFO] Release the button to count; 'i' injects an IRQ; Ctrl+C returns.\n");
        /* Discard setup-time edges before starting this measurement. */
        neon_gpio_clear_interrupt(GPIOB, NEON_GPIO_PIN_5);
        NVIC_ClearPendingIRQ(EXTI1_IRQn);
        NVIC_EnableIRQ(EXTI1_IRQn);
        /* BootROM v5 enters the app with PRIMASK=1; NVIC enable alone is not enough. */
        __enable_irq();

        uint32_t last_count = 0U;
        for (;;) {
            /* Only the ISR increments the aligned word; main takes a snapshot. */
            uint32_t count = g_gpio_interrupt_count;
            if (count != last_count) {
                printf("[GPIO IRQ] PB5 events=%lu (+%lu)\n", (unsigned long)count, (unsigned long)(count - last_count));
                last_count = count;
            }

            uint32_t receive_word = 0U;
            status = neon_uart_receive(uart, (uint8_t *)&receive_word, 1U, 0U);
            if (status == NEON_TIMEOUT) {
                continue;
            }
            if (status != NEON_OK) {
                printf("[ERROR] GPIO interrupt UART RX status=%lu error=0x%08lX\n", (unsigned long)status,
                       (unsigned long)neon_uart_get_error(uart));
                neon_uart_clear_error(uart);
                break;
            }
            uint32_t value = receive_word & 0xFFU;
            if (value == 3U) {
                break;
            }
            if (value == 'i') {
                printf("[INFO] Injecting a GPIO interrupt event.\n");
                neon_gpio_set_interrupt_injection(GPIOB, NEON_GPIO_PIN_5, 1U);
            }
        }
    }

    /* Stop the interrupt source before restoring the pin and stopping. */
    NVIC_DisableIRQ(EXTI1_IRQn);
    neon_gpio_set_interrupt_enabled(GPIOB, NEON_GPIO_PIN_5, 0U);
    neon_gpio_clear_interrupt(GPIOB, NEON_GPIO_PIN_5);
    NVIC_ClearPendingIRQ(EXTI1_IRQn);
    __set_PRIMASK(saved_primask);
    status = neon_gpio_deinit(GPIOB, NEON_GPIO_PIN_5);
    if (status != NEON_OK) {
        printf("[ERROR] GPIO interrupt deinit status=%lu\n", (unsigned long)status);
    }
    printf("[INFO] GPIO interrupt stopped. Total=%lu\n", (unsigned long)g_gpio_interrupt_count);
}
