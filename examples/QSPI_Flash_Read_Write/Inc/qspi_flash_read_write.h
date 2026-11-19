/**
 * @file qspi_flash_read_write.h
 * @brief Public entry point for QSPI_Flash_Read_Write.
 * @details
 * Indirect Single/Dual/Quad NOR erase, program and byte verification.
 */

#ifndef GT32U2086_EXAMPLE_QSPI_FLASH_READ_WRITE_H
#define GT32U2086_EXAMPLE_QSPI_FLASH_READ_WRITE_H

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
 * @brief Run the QSPI_Flash_Read_Write single-key menu.
 * @param[in,out] uart Initialized UART handle; ownership remains with the caller.
 * @details
 * Write tests erase the configured external NOR sector without restoring it.
 * Use x or Ctrl+C to return to the application terminal.
 */
void qspi_flash_read_write_run(neon_uart_handle_t *uart);

#ifdef __cplusplus
}
#endif

#endif /* GT32U2086_EXAMPLE_QSPI_FLASH_READ_WRITE_H */
