#ifndef FPGA_TERMINAL_UART_RX_INTERRUPT_H
#define FPGA_TERMINAL_UART_RX_INTERRUPT_H

#include "uart/neon_uart.h"

#ifdef __cplusplus
extern "C" {
#endif

/* UART0 has been initialized by terminal; Ctrl+C finishes the example. */
void uart_rx_interrupt_run(neon_uart_handle_t *uart, uint32_t skip_initial_lf);

#ifdef __cplusplus
}
#endif

#endif /* FPGA_TERMINAL_UART_RX_INTERRUPT_H */
