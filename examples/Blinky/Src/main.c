/**
 * @file main.c
 * @brief GT32U2086 application example.
 */

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/

#include "gpio/neon_gpio.h"
#include "mcu_device.h"
#include "sdkconfig.h"



/* The old GPIO0 instance no longer exists. GT32U2086.yaml declares GPIOA-D from inc_gt32u2086/gt32u2086.h, with eight
 * pins per port. GPIOA pin 0 is provisional pending hardware/pin_map.md verification, not a
 * confirmed LED pin. */
#define LED_INSTANCE GPIOA
#define LED_PIN (1U << 0)

/*==================================================================================================
*                                   PRIVATE FUNCTION PROTOTYPES
==================================================================================================*/

static void blinky_delay_busy(volatile uint32_t count);

/*==================================================================================================
*                                         GLOBAL FUNCTIONS
==================================================================================================*/

/**
 * @brief Initialize the application and run its main loop.
 * @return The application normally runs indefinitely.
 */
int main(void) {
    neon_gpio_config_t led_init = {
        .pin = LED_PIN,
        .mode = NEON_GPIO_MODE_OUTPUT,
    };
    neon_gpio_clock_enable(LED_INSTANCE);
    neon_gpio_init(LED_INSTANCE, &led_init);


    for (;;) {
        neon_gpio_toggle_pin(LED_INSTANCE, LED_PIN);
        blinky_delay_busy(CONFIG_BLINKY_DELAY_LOOPS);

    }
}

/*==================================================================================================
*                                        PRIVATE FUNCTIONS
==================================================================================================*/

/**
 * @brief Delay using an uncalibrated busy loop.
 * @param[in] count Number of loop iterations.
 */
static void blinky_delay_busy(volatile uint32_t count) {
    while (count--) {
        __NOP();
    }
}
