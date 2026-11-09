#ifndef FU2_EXAMPLE_SPI_W5500_H
#define FU2_EXAMPLE_SPI_W5500_H

#include "uart/neon_uart.h"

#ifdef __cplusplus
extern "C" {
#endif

/* SPI0 master <-> W5500; single-key menu, Ctrl+C stops the example. */
void spi_w5500_run(neon_uart_handle_t *uart);

#ifdef __cplusplus
}
#endif

#endif /* FU2_EXAMPLE_SPI_W5500_H */
