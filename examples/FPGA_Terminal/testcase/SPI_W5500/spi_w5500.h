#ifndef FPGA_TERMINAL_SPI_W5500_H
#define FPGA_TERMINAL_SPI_W5500_H

#include "uart/neon_uart.h"

#ifdef __cplusplus
extern "C" {
#endif

/* SPI0 master <-> W5500; single-key menu, Ctrl+C returns to terminal. */
void spi_w5500_run(neon_uart_handle_t *uart);

#ifdef __cplusplus
}
#endif

#endif /* FPGA_TERMINAL_SPI_W5500_H */
