/**
 * @file main.c
 * @brief Implementation for GPIO input output.
 * @details
 * PB5: button to GND. PB1: active-high output.
 */

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/

#include <stdint.h>
#include <stdio.h>
#include "gpio/neon_gpio.h"

/*==================================================================================================
*                                   PRIVATE FUNCTION PROTOTYPES
==================================================================================================*/

static void gpio_input_output_check_status(neon_status_t status);

/*==================================================================================================
*                                         GLOBAL FUNCTIONS
==================================================================================================*/

/**
 * @brief Initialize the application and run its main loop.
 * @return The application normally runs indefinitely.
 */
int main(void) {
    gpio_input_output_check_status(neon_gpio_clock_enable(GPIOB));

    /* Start PB1 low before enabling the output. */
    neon_gpio_config_t config;
    neon_gpio_get_default_config(&config);
    config.pin = NEON_GPIO_PIN_1;
    config.mode = NEON_GPIO_MODE_OUTPUT;
    config.initial_state = NEON_GPIO_PIN_RESET;
    gpio_input_output_check_status(neon_gpio_init(GPIOB, &config));

    /* PB5 is high when released, low when pressed. */
    neon_gpio_get_default_config(&config);
    config.pin = NEON_GPIO_PIN_5;
    config.mode = NEON_GPIO_MODE_INPUT;
    config.pull = NEON_GPIO_PULL_UP;
    config.input_sync = NEON_GPIO_INPUT_SYNC_ENABLE;
    gpio_input_output_check_status(neon_gpio_init(GPIOB, &config));

    printf("[INFO] PB5 button -> PB1 LED\n");

    uint32_t last_state = 2U; /* Force the initial state to be logged. */
    for (;;) {
        uint32_t input = neon_gpio_read_pin(GPIOB, NEON_GPIO_PIN_5);
        if (input != last_state) {
            last_state = input;
            if (input == 0U) {
                neon_gpio_write_pin(GPIOB, NEON_GPIO_PIN_1, 1U);
                printf("[GPIO] PRESSED -> LED ON\n");
            } else {
                neon_gpio_write_pin(GPIOB, NEON_GPIO_PIN_1, 0U);
                printf("[GPIO] RELEASED -> LED OFF\n");
            }
        }
    }
}

/*==================================================================================================
*                                        PRIVATE FUNCTIONS
==================================================================================================*/

static void gpio_input_output_check_status(neon_status_t status) {
    if (status != NEON_OK) {
        printf("[ERROR] status=%lu\n", (unsigned long)status);
        for (;;) {
            __WFI();
        }
    }
}
