/**
 * @file gpio_input_output.c
 * @brief Implementation for GPIO input output.
 * @details
 * GPIO_Input_Output adapted as a returnable testcase: PB5 input, PB7 output.
 */

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/

#include "gpio_input_output.h"
#include <stdint.h>
#include <stdio.h>

#include "gpio/neon_gpio.h"

/*==================================================================================================
*                                         GLOBAL FUNCTIONS
==================================================================================================*/

/** @copydoc gpio_input_output_run */
void gpio_input_output_run(neon_uart_handle_t *uart) {
    neon_status_t status = neon_gpio_clock_enable(GPIOB);
    if (status != NEON_OK) {
        printf("[ERROR] GPIO clock status=%lu\n", (unsigned long)status);
        return;
    }

    /* Start PB7 low before enabling the output. */
    neon_gpio_config_t config;
    neon_gpio_get_default_config(&config);
    config.pin = NEON_GPIO_PIN_7;
    config.mode = NEON_GPIO_MODE_OUTPUT;
    config.initial_state = NEON_GPIO_PIN_RESET;
    status = neon_gpio_init(GPIOB, &config);
    if (status == NEON_OK) {
        neon_gpio_get_default_config(&config);
        config.pin = NEON_GPIO_PIN_5;
        config.mode = NEON_GPIO_MODE_INPUT;
        config.pull = NEON_GPIO_PULL_UP;
        config.input_sync = NEON_GPIO_INPUT_SYNC_ENABLE;
        status = neon_gpio_init(GPIOB, &config);
    }

    if (status != NEON_OK) {
        printf("[ERROR] GPIO init status=%lu\n", (unsigned long)status);
    } else {
        printf("[INFO] PB5 button -> PB7 LED. Ctrl+C stops the example.\n");
        uint32_t last_state = 2U; /* Log the initial input state as well. */
        for (;;) {
            uint32_t input = neon_gpio_read_pin(GPIOB, NEON_GPIO_PIN_5);
            if (input != last_state) {
                last_state = input;
                if (input == 0U) {
                    neon_gpio_write_pin(GPIOB, NEON_GPIO_PIN_7, NEON_GPIO_PIN_SET);
                    printf("[GPIO] PB5=0 PRESSED -> PB7=1 (LED ON)\n");
                } else {
                    neon_gpio_write_pin(GPIOB, NEON_GPIO_PIN_7, NEON_GPIO_PIN_RESET);
                    printf("[GPIO] PB5=1 RELEASED -> PB7=0 (LED OFF)\n");
                }
            }

            /* Non-blocking UART polling keeps the GPIO loop responsive. Own a whole
       * aligned word even though Receive writes only one logical byte. */
            uint32_t receive_word = 0U;
            status = neon_uart_receive(uart, (uint8_t *)&receive_word, 1U, 0U);
            if (status == NEON_TIMEOUT) {
                continue;
            }
            if (status != NEON_OK) {
                printf("[ERROR] GPIO testcase UART RX status=%lu error=0x%08lX\n", (unsigned long)status,
                       (unsigned long)neon_uart_get_error(uart));
                neon_uart_clear_error(uart);
                break;
            }
            if ((receive_word & 0xFFU) == 3U) {
                break;
            }
        }
    }

    neon_gpio_write_pin(GPIOB, NEON_GPIO_PIN_7, NEON_GPIO_PIN_RESET);
    status = neon_gpio_deinit(GPIOB, NEON_GPIO_PIN_5 | NEON_GPIO_PIN_7);
    if (status != NEON_OK) {
        printf("[ERROR] GPIO deinit status=%lu\n", (unsigned long)status);
    }
    printf("[INFO] GPIO input/output stopped.\n");
}
