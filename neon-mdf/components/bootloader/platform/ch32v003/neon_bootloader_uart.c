/**
 * @file platform/ch32v003/neon_bootloader_uart.c
 * @brief Implementation for bootloader UART.
 * @details
 * * platform/ch32v003/neon_bootloader_uart.c - implementation THAT cho USART1 CH32V003
 * (PD5=TX, PD6=RX, mapping mac dinh KHONG remap, 115200 8N1) - mirror
 * platform/weact_f412/neon_bootloader_uart.c, dung struct RCC/GPIOD/USART1 sinh tu
 * mcu-description (xem mcu-description/devices/CH32V003F4P6.yaml, muc
 * `bootloader.uart` + peripherals RCC/GPIOD/USART1).
 *
 * Thanh ghi/gia tri xac nhan qua CH32V003RM.PDF V1.9 (KHONG doan):
 *   - RCC->APB2PCENR: bit5=IOPDEN, bit14=USART1EN (chuong 3.4.7) - CUNG 1
 *     thanh ghi voi GPIO clock enable (khac STM32F4 tach AHB1ENR/APB2ENR
 *     rieng cho GPIO/USART).
 *   - USART1_TX=PD5, USART1_RX=PD6 la MAPPING MAC DINH (AFIO_PCFR1.USART1_RM1=00,
 *     chuong "pin function" - KHONG can dung AFIO/remap gi ca).
 *   - USART_STATR: TXE=bit7, TC=bit6, RXNE=bit5 (chuong 12.10.1) - CUNG VI
 *     TRI BIT voi USART_SR cua STM32F4 (IP loi thua ke chung, khong phai
 *     trung hop ngau nhien).
 *   - USART_CTLR1: UE=bit13, TE=bit3, RE=bit2 (chuong 12.10.4).
 *   - GPIOD->CFGLR: pin TX (PD5, alternate-function push-pull output) dung
 *     nibble CNF=10/MODE=01 = 0x9; pin RX (PD6, floating input) dung
 *     nibble CNF=01/MODE=00 = 0x4 (giong nibble floating-input trong
 *     platform/ch32v003/neon_gpio.c).
 *   - BRR = round(HCLK/baud) = round(24000000/115200) = 208 = 0xD0
 *     (mantissa=13 << 4 | fraction=0) - @ HSI 24MHz mac dinh sau reset
 *     (SystemInit() CHUA cau hinh PLL, xem system_CH32V003F4P6.c).
 */

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/

#include "neon_bootloader_uart.h"
#include "mcu_device.h"

/*==================================================================================================
*                                        DEFINES AND MACROS
==================================================================================================*/
#define RCC_APB2PCENR_IOPDEN (1U << 5)
#define RCC_APB2PCENR_USART1EN (1U << 14)

#define USART_STATR_RXNE (1U << 5)
#define USART_STATR_TC (1U << 6)
#define USART_STATR_TXE (1U << 7)
#define USART_CTLR1_RE (1U << 2)
#define USART_CTLR1_TE (1U << 3)
#define USART_CTLR1_UE (1U << 13)

/*==================================================================================================
*                                         GLOBAL FUNCTIONS
==================================================================================================*/

/** @copydoc neon_bootloader_uart_init */
void neon_bootloader_uart_init(void) {
    RCC->APB2PCENR |= RCC_APB2PCENR_IOPDEN | RCC_APB2PCENR_USART1EN;

    /* PD5 (TX) = alternate-function push-pull output (CNF=10,MODE=01=0x9),
   * PD6 (RX) = floating input (CNF=01,MODE=00=0x4) - xem comment dau file. */
    uint32_t cfglr = GPIOD->CFGLR;
    cfglr &= ~((0xFU << (5U * 4U)) | (0xFU << (6U * 4U)));
    cfglr |= (0x9U << (5U * 4U)) | (0x4U << (6U * 4U));
    GPIOD->CFGLR = cfglr;

    USART1->BRR = 0xD0; /* 115200 baud @ HSI 24MHz, xem comment dau file */
    USART1->CTLR1 = USART_CTLR1_UE | USART_CTLR1_TE | USART_CTLR1_RE;
}

/** @copydoc neon_bootloader_uart_send_byte */
void neon_bootloader_uart_send_byte(uint8_t b) {
    while (!(USART1->STATR & USART_STATR_TXE)) {
        /* poll */
    }
    USART1->DATAR = b;
    while (!(USART1->STATR & USART_STATR_TC)) {
        /* poll - doi truyen xong hoan toan */
    }
}

/** @copydoc neon_bootloader_uart_recv_byte */
uint8_t neon_bootloader_uart_recv_byte(void) {
    while (!(USART1->STATR & USART_STATR_RXNE)) {
        /* poll */
    }
    return (uint8_t)USART1->DATAR;
}

/** @copydoc neon_bootloader_uart_recv_byte_timeout */
int neon_bootloader_uart_recv_byte_timeout(uint8_t *out, uint32_t timeout_polls) {
    while (!(USART1->STATR & USART_STATR_RXNE)) {
        if (timeout_polls-- == 0) {
            return 0;
        }
    }
    *out = (uint8_t)USART1->DATAR;
    return 1;
}

/** @copydoc neon_bootloader_uart_try_recv_byte */
int neon_bootloader_uart_try_recv_byte(uint8_t *out) {
    if (!(USART1->STATR & USART_STATR_RXNE)) {
        return 0;
    }
    *out = (uint8_t)USART1->DATAR;
    return 1;
}
