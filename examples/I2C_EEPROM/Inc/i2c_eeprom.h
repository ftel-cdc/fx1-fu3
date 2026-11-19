/**
 * @file i2c_eeprom.h
 * @brief Public declarations for the I2C EEPROM example.
 * @details
 * Scan I2C0, read 256 bytes once, or write and verify the first 1 KiB.
 */

#ifndef GT32U2086_EXAMPLE_I2C_EEPROM_H
#define GT32U2086_EXAMPLE_I2C_EEPROM_H

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
 * @brief Scan I2C0, read 256 bytes once, or write and verify the first 1 KiB.
 * @param[in,out] uart Initialized UART handle; ownership remains with the caller.
 * @details
 * Uses the caller-owned UART console; q or Ctrl+C returns to the application terminal.
 */
void i2c_eeprom_run(neon_uart_handle_t *uart);

#ifdef __cplusplus
}
#endif

#endif /* GT32U2086_EXAMPLE_I2C_EEPROM_H */
