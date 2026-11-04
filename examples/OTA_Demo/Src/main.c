/**
 * @file main.c
 * @brief Implementation for OTA demo.
 * @details
 * * OTA_Demo - vi du OTA THAT tren phan cung: 1 source main.c duoc BUILD
 * NHIEU LAN (macro NEON_OTA_APP_VERSION + NEON_OTA_SKIP_CONFIRM, xem
 * CMakeLists.txt) thanh nhieu executable khac nhau:
 *   ota_demo         - factory (slot "app"), version 1, nap CUNG bootloader
 *                       (flash_jtag/flash_uart_stm32) - KHONG BAO GIO bi
 *                       OTA dung toi.
 *   ota_demo_ota     - ota (slot "app_ota"), version 2, TU XAC NHAN tot
 *                       sau vai giay chay on dinh (goi neon_ota_confirm_valid()).
 *   ota_demo_ota_bad - ota (slot "app_ota"), version 3, CO CHU DICH KHONG
 *                       BAO GIO xac nhan - dung rieng de test co che tu
 *                       dong ROLLBACK cua bootloader (xem flash_ota target +
 *                       plan file, muc "Test that tren phan cung").
 *
 * WEACT_F412 dung PB3 (khong co LED onboard); STM32F103CB Blue Pill dung LED
 * onboard PC13 active-low. STM32F103CB in log qua USART1 PA9/PA10,
 * 115200 8N1; WEACT_F412 khong con syscall printf-to-UART.
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
/* Blue Pill LED onboard tai PC13, active-low. */
#define LED_INSTANCE GPIOC
#define LED_PIN (1U << 13)
#else
#error "OTA_Demo: chi ho tro WEACT_F412 va STM32F103CB"
#endif

/* So vong lap busy-wait truoc khi tu xac nhan tot - ~vai giay, du de quan
 * sat qua UART/SWD la app moi da chay on dinh (khong crash/treo ngay). */
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
    /* Tat LED active-low truoc khi chuyen PC13 sang output. */
    neon_gpio_write_pin(LED_INSTANCE, LED_PIN, 1U);
#endif
    neon_gpio_init(LED_INSTANCE, &led_init);
    neon_bootloader_uart_init();

    uint32_t loop_count = 0;
    for (;;) {
        neon_gpio_toggle_pin(LED_INSTANCE, LED_PIN);
        printf("OTA_Demo v%d dang chay (loop=%lu)\n", NEON_OTA_APP_VERSION, (unsigned long)loop_count);
        ota_demo_delay_busy(1000000U);
        loop_count++;

        if (loop_count == CONFIRM_AFTER_LOOPS) {
#if !defined(NEON_OTA_SKIP_CONFIRM)
            neon_ota_confirm_valid();
            printf("OTA_Demo v%d: DA CONFIRM VALID\n", NEON_OTA_APP_VERSION);
#else
            printf("OTA_Demo v%d: (test rollback) CO CHU DICH khong confirm\n", NEON_OTA_APP_VERSION);
#endif
        }
    }
}

/*==================================================================================================
*                                        PRIVATE FUNCTIONS
==================================================================================================*/

static void ota_demo_delay_busy(volatile uint32_t count) {
    while (count--) {
    }
}
