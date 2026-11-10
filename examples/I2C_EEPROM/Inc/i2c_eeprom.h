/** @file i2c_eeprom.h
 * @brief I2C0 AT24C256 scan, read and write/verify menu.
 */
#ifndef I2C_EEPROM_H
#define I2C_EEPROM_H
#include "uart/neon_uart.h"
#ifdef __cplusplus
extern "C" {
#endif
void i2c_eeprom_run(neon_uart_handle_t *uart);
#ifdef __cplusplus
}
#endif
#endif
