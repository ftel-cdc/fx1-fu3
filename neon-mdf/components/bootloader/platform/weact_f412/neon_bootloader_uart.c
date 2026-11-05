/**
 * @file platform/weact_f412/neon_bootloader_uart.c
 * @brief Implementation for bootloader UART.
 * @details
 * * platform/weact_f412/neon_bootloader_uart.c - implementation THAT cho USART1
 * STM32F412 (PA9=TX, PA10=RX, AF7, 115200 8N1) - tong quat hoa tu
 * Bootloader_WeAct/Src/bootloader_uart.c, dung struct RCC/GPIOA/USART1 sinh
 * tu mcu-description (xem mcu-description/devices/WEACT_F412.yaml, muc
 * `bootloader.uart` + peripherals GPIOA/RCC/USART1) thay vi struct rieng
 * hand-roll nhu truoc.
 *
 * pin/peripheral/baud o day la HARDCODE CO CHU DICH khop voi khai bao tai
 * lieu trong `bootloader.uart` cua YAML (generator hien KHONG tu sinh logic
 * khoi tao GPIO/USART tu khai bao do - chi dung no lam tai lieu tham chieu +
 * cac macro kich thuoc partition, xem gen_cmsis_header.py) - giong cach GPIO
 * driver van hand-write logic thanh ghi rieng cho tung family.
 *
 * Thanh ghi/gia tri xac nhan qua source that (KHONG doan), tu
 * STMicroelectronics/cmsis_device_f4 (Include/stm32f412zx.h):
 *   RCC_AHB1ENR_GPIOAEN=bit0, RCC_APB2ENR_USART1EN=bit4,
 *   USART_SR: RXNE=bit5, TC=bit6, TXE=bit7. USART_CR1: RE=bit2, TE=bit3, UE=bit13.
 * BRR=0x8B tinh tay cho 115200 baud @ HSI 16MHz mac dinh sau reset (chua
 * cau hinh PLL - xem system_WEACT_F412.c): BRR = round(fCK/baud) =
 * round(16000000/115200) = 139 = 0x8B (OVER8=0, mac dinh).
 * AF7 = alternate function cho USART1/2/3 tren GPIO - quy uoc chuan
 * STM32F4 (khong rieng gi F412), khong can xac nhan them.
 */

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/

#include "neon_bootloader_uart.h"
#include "mcu_device.h"

/*==================================================================================================
*                                        DEFINES AND MACROS
==================================================================================================*/
#define RCC_AHB1ENR_GPIOAEN (1U << 0)
#define RCC_APB2ENR_USART1EN (1U << 4)

#define USART_SR_RXNE (1U << 5)
#define USART_SR_TC (1U << 6)
#define USART_SR_TXE (1U << 7)
#define USART_CR1_RE (1U << 2)
#define USART_CR1_TE (1U << 3)
#define USART_CR1_UE (1U << 13)

/*==================================================================================================
*                                         GLOBAL FUNCTIONS
==================================================================================================*/

/** @copydoc neon_bootloader_uart_init */
void neon_bootloader_uart_init(void) {
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    RCC->APB2ENR |= RCC_APB2ENR_USART1EN;

    /* PA9/PA10 = alternate function mode (10b moi pin trong MODER) */
    GPIOA->MODER &= ~((3U << (9 * 2)) | (3U << (10 * 2)));
    GPIOA->MODER |= (2U << (9 * 2)) | (2U << (10 * 2));

    /* AF7 (USART1/2/3) cho ca PA9 va PA10 - AFRH quan ly pin 8..15,
   * vi tri bit trong AFRH = (pin - 8) * 4. */
    GPIOA->AFRH &= ~((0xFU << ((9 - 8) * 4)) | (0xFU << ((10 - 8) * 4)));
    GPIOA->AFRH |= (7U << ((9 - 8) * 4)) | (7U << ((10 - 8) * 4));

    USART1->BRR = 0x8B; /* 115200 baud @ HSI 16MHz, xem comment dau file */
    USART1->CR1 = USART_CR1_UE | USART_CR1_TE | USART_CR1_RE;
}

/** @copydoc neon_bootloader_uart_send_byte */
void neon_bootloader_uart_send_byte(uint8_t b) {
    while (!(USART1->SR & USART_SR_TXE)) {
        /* poll */
    }
    USART1->DR = b;
    while (!(USART1->SR & USART_SR_TC)) {
        /* poll - doi truyen xong hoan toan, khong chi la "san sang nhan byte tiep" */
    }
}

/** @copydoc neon_bootloader_uart_recv_byte */
uint8_t neon_bootloader_uart_recv_byte(void) {
    while (!(USART1->SR & USART_SR_RXNE)) {
        /* poll */
    }
    return (uint8_t)USART1->DR;
}

/** @copydoc neon_bootloader_uart_recv_byte_timeout */
int neon_bootloader_uart_recv_byte_timeout(uint8_t *out, uint32_t timeout_polls) {
    while (!(USART1->SR & USART_SR_RXNE)) {
        if (timeout_polls-- == 0) {
            return 0;
        }
    }
    *out = (uint8_t)USART1->DR;
    return 1;
}

/** @copydoc neon_bootloader_uart_try_recv_byte */
int neon_bootloader_uart_try_recv_byte(uint8_t *out) {
    if (!(USART1->SR & USART_SR_RXNE)) {
        return 0;
    }
    *out = (uint8_t)USART1->DR;
    return 1;
}
