/**
 * @file qspi_xip_read.h
 * @brief Public entry point for QSPI_XIP_Read.
 * @details
 * Indirect Quad programming and verified QSPI memory-mapped reads.
 */

#ifndef FU2_EXAMPLE_QSPI_XIP_READ_H
#define FU2_EXAMPLE_QSPI_XIP_READ_H

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
 * @brief Run the QSPI_XIP_Read single-key menu.
 * @param[in,out] uart Initialized UART handle; ownership remains with the caller.
 * @details
 * Write tests erase the configured external NOR sector without restoring it.
 * Use q or Ctrl+C to return to the application terminal.
 */
void qspi_xip_read_run(neon_uart_handle_t *uart);

#ifdef __cplusplus
}
#endif

#endif /* FU2_EXAMPLE_QSPI_XIP_READ_H */
