/**
 * @file gpio_input_output.h
 * @brief Public declarations for GPIO input output.
 * @details
 * Declares module interfaces.
 */

#ifndef FPGA_TERMINAL_GPIO_INPUT_OUTPUT_H
#define FPGA_TERMINAL_GPIO_INPUT_OUTPUT_H

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
 * @brief Gpio input output run.
 * @param[in,out] uart Initialized UART handle; ownership remains with the caller.
 * @details
 * PB5 active-low button controls PB7. Borrow the terminal UART for Ctrl+C.
 */
void gpio_input_output_run(neon_uart_handle_t *uart);

#ifdef __cplusplus
}
#endif

#endif /* FPGA_TERMINAL_GPIO_INPUT_OUTPUT_H */
