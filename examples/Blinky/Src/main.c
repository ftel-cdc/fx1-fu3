/**
 * @file main.c
 * @brief Implementation for blinky.
 * @details
 * * Minimal example: continuously toggle one GPIO pin to demonstrate the pipeline
 * mcu-description -> CMSIS/RISC-V header/linker/startup -> Driver -> firmware
 * works end to end. Delay uses a busy loop; SysTick/NEON_Delay support was deferred to the FreeRTOS
 * example. CONFIG_BLINKY_DELAY_LOOPS comes from this app's Kconfig beside CMakeLists.txt. App
 * options share one menuconfig menu with SDK options; see neon-mdf/Kconfig.
 *
 * The same main.c supports FU2, WEACT_F412 and CH32V003F4P6 (added 2026-08-10, the SDK's first RISC-V
 * target). Only these settings differ:
 * LED_INSTANCE/LED_PIN (macro target do
 * target_compile_definitions(..., PUBLIC ${TARGET_NAME}) in cmake/mcu_variants.cmake). NEON_GPIO_*
 * calls are shared; implementations are in
 * components/driver/gpio/neon_gpio_{fu2,weact_f412,ch32v003}.c, selected by FAMILY. See
 * components/driver/CMakeLists.txt and
 * ARCHITECTURE.md).
 *
 * Call neon_gpio_clock_enable() before config(). On clock-gated STM32/CH32V003 devices, GPIO requires
 * its peripheral clock; see ARCHITECTURE.md.
 *
 * WEACT_F412 has no UART printf() syscall retarget; newlib/nosys stubs produce no UART output.
 * CH32V003F4P6 does not use printf(): its bare-metal RISC-V toolchain has no libc, so printf()
 * would not link (cmake/toolchain/riscv-none-elf.cmake). As in FreeRTOS_Blinky, the blink delay
 * identifies the version.
 *
 * OTA (2026-08-10, WEACT_F412 and CH32V003F4P6): build this source twice, as factory
 * (NEON_OTA_APP_VERSION=1) and OTA (NEON_OTA_APP_VERSION=2); see CMakeLists.txt and
 * examples/OTA_Demo/. flashmap.csv/flashmap.<DEVICE>.csv define device-specific partition layouts,
 * because flash sizes differ greatly: 512KB on WEACT_F412 versus 16KB on CH32V003F4P6).
 * After about ten toggles, call neon_ota_confirm_valid(); see components/ota/neon_ota.h. FU2 has no
 * SDK bootloader/OTA and retains a single build.
 */

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/

#include "gpio/neon_gpio.h"
#include "mcu_device.h"
#include "sdkconfig.h"

#if defined(WEACT_F412)
/* <stdio.h>/printf() require a target with libc, such as FU2/WEACT_F412 using arm-none-eabi-gcc and
 * newlib nano.specs. The riscv64-elf-gcc toolchain (cmake/toolchain/riscv-none-elf.cmake) is bare
 * metal without libc. Guard #include <stdio.h> to avoid a "No such file" preprocessing error. */
#include <stdio.h>
#include "neon_bootloader_uart.h"
#include "neon_ota.h"
#elif defined(CH32V003F4P6)
#include "neon_ota.h"
#endif

#ifndef NEON_OTA_APP_VERSION

/*==================================================================================================
*                                        DEFINES AND MACROS
==================================================================================================*/
#define NEON_OTA_APP_VERSION 1
#endif

#if defined(FU2)
/* The old GPIO0 instance no longer exists. FU2.yaml declares GPIOA-D from inc_fu2/fu2.h, with eight
 * pins per port. GPIOA pin 0 is provisional pending hardware/pin_map.md verification, not a
 * confirmed LED pin. */
#define LED_INSTANCE GPIOA
#define LED_PIN (1U << 0)
#elif defined(WEACT_F412)
#define LED_INSTANCE GPIOB
#define LED_PIN (1U << 2) /* PB2 = LED onboard WeAct STM32F4 Core Board */
#elif defined(CH32V003F4P6)
#define LED_INSTANCE GPIOD
#define LED_PIN                                                                                                        \
    (1U << 1) /* PD1 is shared by SWIO debug and the development-board LED on CH32V003F4P6; checked against Zephyr board documentation */
#else
#error "Blinky: LED_INSTANCE/LED_PIN are not defined for this target"
#endif

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

#if defined(WEACT_F412)
    neon_bootloader_uart_init();
#endif
#if defined(WEACT_F412) || defined(CH32V003F4P6)
    uint32_t loop_count = 0;
#endif

    for (;;) {
        neon_gpio_toggle_pin(LED_INSTANCE, LED_PIN);
        blinky_delay_busy(CONFIG_BLINKY_DELAY_LOOPS);
#if defined(WEACT_F412)
        printf("LED toggled (v%d)\n", NEON_OTA_APP_VERSION);
#endif

#if defined(WEACT_F412) || defined(CH32V003F4P6)
        loop_count++;
        if (loop_count == 10U) {
            neon_ota_confirm_valid();
#if defined(WEACT_F412)
            printf("Blinky v%d: CONFIRMED VALID\n", NEON_OTA_APP_VERSION);
#endif
        }
#endif
    }
}

/*==================================================================================================
*                                        PRIVATE FUNCTIONS
==================================================================================================*/

static void blinky_delay_busy(volatile uint32_t count) {
    while (count--) {
        __NOP();
    }
}
