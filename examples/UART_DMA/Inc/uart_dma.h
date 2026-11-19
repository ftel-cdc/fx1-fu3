/**
 * @file uart_dma.h
 * @brief Public declarations for the UART DMA example.
 * @details
 * Run UART0 TX/RX DMA pattern tests.
 */

#ifndef GT32U2086_EXAMPLE_UART_DMA_H
#define GT32U2086_EXAMPLE_UART_DMA_H

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/

#include "uart/neon_uart.h"

#ifdef __cplusplus
extern "C" {
#endif

/*==================================================================================================
*                                       FUNCTION PROTOTYPES
==================================================================================================*/

/**
 * @brief Run UART0 TX/RX DMA pattern tests.
 * @param[in,out] uart Initialized UART handle; ownership remains with the caller.
 * @details
 * Uses the caller-owned UART0 handle and restores its configuration after completed tests.
 */
void uart_dma_run(neon_uart_handle_t *uart);

#ifdef __cplusplus
}
#endif

#endif /* GT32U2086_EXAMPLE_UART_DMA_H */
