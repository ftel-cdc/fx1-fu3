/**
 * @file platform/stm32f103/neon_bootloader_uart.c
 * @brief Implementation for bootloader UART.
 * @details
 * STM32F103 USART1 bootloader transport: PA9 TX, PA10 RX, 115200 8N1.
 */

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/

#include "neon_bootloader_uart.h"
#include "mcu_device.h"

/*==================================================================================================
*                                        DEFINES AND MACROS
==================================================================================================*/
#define RCC_APB2ENR_IOPAEN (1U << 2)
#define RCC_APB2ENR_USART1EN (1U << 14)

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
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN | RCC_APB2ENR_USART1EN;

    /* STM32F1 CRH nibbles: PA9=AF push-pull 2 MHz, PA10=floating input. */
    GPIOA->CRH &= ~((0xFU << 4U) | (0xFU << 8U));
    GPIOA->CRH |= (0xAU << 4U) | (0x4U << 8U);

    /* Reset clock is HSI 8 MHz; oversampling x16 gives BRR=round(8M/115200). */
    USART1->BRR = 69U;
    USART1->CR2 = 0U;
    USART1->CR3 = 0U;
    USART1->CR1 = USART_CR1_UE | USART_CR1_TE | USART_CR1_RE;
}

/** @copydoc neon_bootloader_uart_send_byte */
void neon_bootloader_uart_send_byte(uint8_t b) {
    while ((USART1->SR & USART_SR_TXE) == 0U) {
    }
    USART1->DR = b;
    while ((USART1->SR & USART_SR_TC) == 0U) {
    }
}

/** @copydoc neon_bootloader_uart_recv_byte */
uint8_t neon_bootloader_uart_recv_byte(void) {
    while ((USART1->SR & USART_SR_RXNE) == 0U) {
    }
    return (uint8_t)USART1->DR;
}

/** @copydoc neon_bootloader_uart_recv_byte_timeout */
int neon_bootloader_uart_recv_byte_timeout(uint8_t *out, uint32_t timeout_polls) {
    while ((USART1->SR & USART_SR_RXNE) == 0U) {
        if (timeout_polls-- == 0U) {
            return 0;
        }
    }
    *out = (uint8_t)USART1->DR;
    return 1;
}

/** @copydoc neon_bootloader_uart_try_recv_byte */
int neon_bootloader_uart_try_recv_byte(uint8_t *out) {
    if ((USART1->SR & USART_SR_RXNE) == 0U) {
        return 0;
    }
    *out = (uint8_t)USART1->DR;
    return 1;
}
