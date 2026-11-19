/**
 * @file main.c
 * @brief Implementation for OTA demo.
 * @details
 * * OTA_Demo - full real OTA on hardware: 1 source main.c is BUILD several times (macro
 *                       NEON_OTA_APP_VERSION + NEON_OTA_SKIP_CONFIRM, see CMakeLists.txt) into
 *                       various executable: ota_demo - factory (slot "app"), version 1, flash with
 *                       bootloader (flash_jtag/flash_uart_stm32) - does not indicate the time to
 *                       use my real OTA. ota_demo_ota - ota (slot "app_ota"), version 2, from tot
 *                       confirmation after a few seconds of running on specified (call
 *                       neon_ota_confirm_valid()). ota_demo_ota_bad - ota (slot "app_ota"), version
 *                       3, has translation text that does not indicate the confirmation time - used
 *                       exclusively for testing with automatic ROLLBACK covering of the bootloader
 *                       (see flash_ota target + plan file, section "Real test on hardware").
 *
 * WEACT_F412 uses PB3 (without LED onboard); STM32F103CB Blue Pill uses LED onboard PC13 active-low.
 * STM32F103CB logs through USART1 PA9/PA10 at 115200 8N1; WEACT_F412 no longer has a syscall
 * printf-to-UART.
 */

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/

#include <stdio.h>
#include "gpio/neon_gpio.h"
#include "mcu_device.h"
#include "neon_bootloader_uart.h"
#include "neon_ota.h"

#ifndef NEON_OTA_APP_VERSION

/*==================================================================================================
*                                        DEFINES AND MACROS
==================================================================================================*/
#define NEON_OTA_APP_VERSION 0
#endif

#if defined(WEACT_F412)
#define LED_INSTANCE GPIOB
#define LED_PIN (1U << 3)
#elif defined(STM32F103CB)
/* Blue Pill onboard LED on PC13, active-low. */
#define LED_INSTANCE GPIOC
#define LED_PIN (1U << 13)
#else
#error "OTA_Demo: only support WEACT_F412 and STM32F103CB"
#endif

/* busy-wait loop number before tot confirmation word - ~several seconds, enough to quan sat via
 * UART/SWD is the new app has run on specified (not crash/treo right). */
#define CONFIRM_AFTER_LOOPS 10U

/*==================================================================================================
*                                   PRIVATE FUNCTION PROTOTYPES
==================================================================================================*/

static void ota_demo_delay_busy(volatile uint32_t count);

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
#if defined(STM32F103CB)
    /* turn off the active-low LED before switching PC13 to output. */
    neon_gpio_write_pin(LED_INSTANCE, LED_PIN, 1U);
#endif
    neon_gpio_init(LED_INSTANCE, &led_init);
    neon_bootloader_uart_init();

    uint32_t loop_count = 0;
    for (;;) {
        neon_gpio_toggle_pin(LED_INSTANCE, LED_PIN);
        printf("OTA_Demo v%d running (loop=%lu)\n", NEON_OTA_APP_VERSION, (unsigned long)loop_count);
        ota_demo_delay_busy(1000000U);
        loop_count++;

        if (loop_count == CONFIRM_AFTER_LOOPS) {
#if !defined(NEON_OTA_SKIP_CONFIRM)
            neon_ota_confirm_valid();
            printf("OTA_Demo v%d: CONFIRMED VALID\n", NEON_OTA_APP_VERSION);
#else
            printf("OTA_Demo v%d: (test rollback) has translation not confirm\n", NEON_OTA_APP_VERSION);
#endif
        }
    }
}

/*==================================================================================================
*                                        PRIVATE FUNCTIONS
==================================================================================================*/

/**
 * @brief Delay using an uncalibrated busy loop between OTA demo iterations.
 * @param[in] count Number of loop iterations.
 */
static void ota_demo_delay_busy(volatile uint32_t count) {
    while (count--) {
    }
}
