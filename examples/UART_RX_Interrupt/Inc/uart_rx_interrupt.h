#ifndef FU2_EXAMPLE_UART_RX_INTERRUPT_H
#define FU2_EXAMPLE_UART_RX_INTERRUPT_H

#include "uart/neon_uart.h"

#ifdef __cplusplus
extern "C" {
#endif

/* UART0 has been initialized by main; Ctrl+C finishes the example. */
void uart_rx_interrupt_run(neon_uart_handle_t *uart);

#ifdef __cplusplus
}
#endif

#endif /* FU2_EXAMPLE_UART_RX_INTERRUPT_H */
