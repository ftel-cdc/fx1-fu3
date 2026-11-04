/**
 * @file main.c
 * @brief Implementation for blinky.
 * @details
 * * Vi du toi thieu: toggle 1 pin GPIO lien tuc. Chi de chung minh pipeline
 * mcu-description -> CMSIS/RISC-V header/linker/startup -> Driver -> firmware
 * chay duoc end-to-end. Delay dung busy-loop (chua co SysTick/NEON_Delay
 * that - se lam khi co vi du FreeRTOS), so vong lap doc tu
 * CONFIG_BLINKY_DELAY_LOOPS (xem Kconfig cua RIENG app nay canh file
 * CMakeLists.txt - vi du menuconfig KET HOP: option cua app gop chung 1
 * menu voi option cua SDK, xem neon-mdf/Kconfig).
 *
 * 1 file main.c CHAY DUOC CA 3 TARGET (FU2, WEACT_F412, CH32V003F4P6 -
 * them 2026-08-10, target RISC-V dau tien cua SDK) - chi khac nhau o
 * LED_INSTANCE/LED_PIN (macro target do
 * target_compile_definitions(..., PUBLIC ${TARGET_NAME}) trong
 * cmake/mcu_variants.cmake dinh nghia san). Logic goi NEON_GPIO_* giong het
 * nhau - implementation THAT khac nhau nam trong
 * components/driver/gpio/neon_gpio_{fu2,weact_f412,ch32v003}.c (chon dung
 * file theo FAMILY luc build, xem CMakeLists.txt cua components/driver va
 * ARCHITECTURE.md).
 *
 * neon_gpio_clock_enable() PHAI goi truoc config() - tren chip that co
 * clock-gating (STM32/CH32V003), GPIO khong hoat dong neu chua bat clock -
 * xem ARCHITECTURE.md.
 *
 * WEACT_F412 khong con syscall retarget printf() sang UART; cac lenh log
 * cua example nay dung stub newlib/nosys, khong co output UART.
 * CH32V003F4P6 KHONG dung printf() o day (toolchain RISC-V bare-metal dang
 * dung KHONG co libc thuc su, xem cmake/toolchain/riscv-none-elf.cmake -
 * printf() se khong link duoc) - dung chu ky delay lam dau hieu phan biet
 * version thay the, giong FreeRTOS_Blinky.
 *
 * OTA (2026-08-10, WEACT_F412 va CH32V003F4P6): app nay gio duoc BUILD 2
 * LAN tu CUNG 1 source (xem CMakeLists.txt) - ban factory
 * (NEON_OTA_APP_VERSION=1) va ban ota (NEON_OTA_APP_VERSION=2) - xem
 * examples/OTA_Demo/ cho vi du OTA day du + flashmap.csv/flashmap.<DEVICE>.csv
 * canh file nay cho partition table (moi device co layout partition RIENG,
 * vi flash size chenh lech rat lon - 512KB WEACT_F412 vs 16KB CH32V003F4P6).
 * Sau ~10 lan toggle, tu goi neon_ota_confirm_valid() - xem
 * components/ota/neon_ota.h. FU2 CHUA co bootloader/OTA (giu nguyen build
 * don, khong OTA).
 */

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/

#include "gpio/neon_gpio.h"
#include "mcu_device.h"
#include "sdkconfig.h"

#if defined(WEACT_F412)
/* <stdio.h>/printf() CHI dung duoc tren target co libc that (FU2/WEACT_F412
 * qua arm-none-eabi-gcc + newlib nano.specs) - toolchain RISC-V dang dung
 * (riscv64-elf-gcc, xem cmake/toolchain/riscv-none-elf.cmake) la bare-metal
 * KHONG libc, #include <stdio.h> se loi "No such file" ngay o buoc
 * preprocess neu khong bao boc dieu kien. */
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
/* GPIO0 (bản cũ) không còn tồn tại - FU2.yaml giờ khai đúng 4 instance thật
 * GPIOA-D (theo inc_fu2/fu2.h), mỗi port 8 pin. Chưa có board thật/pin_map.md
 * nên chọn tạm GPIOA pin 0 (arbitrary, không phải LED thật nào đã xác nhận). */
#define LED_INSTANCE GPIOA
#define LED_PIN (1U << 0)
#elif defined(WEACT_F412)
#define LED_INSTANCE GPIOB
#define LED_PIN (1U << 2) /* PB2 = LED onboard WeAct STM32F4 Core Board */
#elif defined(CH32V003F4P6)
#define LED_INSTANCE GPIOD
#define LED_PIN                                                                                                        \
    (1U << 1) /* PD1 - trung SWIO debug + LED tren board dev CH32V003F4P6 (xac nhan qua Zephyr board doc, xem memory) */
#else
#error "Blinky: chua dinh nghia LED_INSTANCE/LED_PIN cho target nay"
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
            printf("Blinky v%d: DA CONFIRM VALID\n", NEON_OTA_APP_VERSION);
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
