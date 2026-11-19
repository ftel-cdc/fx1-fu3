/**
 * @file uart_rx_interrupt.h
 * @brief Public declarations for the UART RX interrupt example.
 * @details
 * Receive UART0 data through an interrupt-driven ring buffer and echo it.
 */

#ifndef FU2_EXAMPLE_UART_RX_INTERRUPT_H
#define FU2_EXAMPLE_UART_RX_INTERRUPT_H

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
 * @brief Receive UART0 data through an interrupt-driven ring buffer and echo it.
 * @param[in,out] uart Initialized UART handle; ownership remains with the caller.
 * @details
 * The initialized UART0 handle remains caller-owned; Ctrl+C ends the example.
 */
void uart_rx_interrupt_run(neon_uart_handle_t *uart);

#ifdef __cplusplus
}
#endif

#endif /* FU2_EXAMPLE_UART_RX_INTERRUPT_H */
