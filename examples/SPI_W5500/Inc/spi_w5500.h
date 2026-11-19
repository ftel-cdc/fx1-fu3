/**
 * @file spi_w5500.h
 * @brief Public declarations for the SPI W5500 example.
 * @details
 * Run the SPI0 W5500 register, SRAM and timing-test menu.
 */

#ifndef GT32U2086_EXAMPLE_SPI_W5500_H
#define GT32U2086_EXAMPLE_SPI_W5500_H

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
 * @brief Run the SPI0 W5500 register, SRAM and timing-test menu.
 * @param[in,out] uart Initialized UART handle; ownership remains with the caller.
 * @details
 * Uses the caller-owned UART console; q or Ctrl+C returns to the application terminal.
 */
void spi_w5500_run(neon_uart_handle_t *uart);

#ifdef __cplusplus
}
#endif

#endif /* GT32U2086_EXAMPLE_SPI_W5500_H */
