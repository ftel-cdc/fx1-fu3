/**
 * @file gpio_interrupt.h
 * @brief Public declarations for GPIO interrupt.
 * @details
 * Declares module interfaces.
 */

#ifndef FU2_EXAMPLE_GPIO_INTERRUPT_H
#define FU2_EXAMPLE_GPIO_INTERRUPT_H

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include <stdint.h>
#include "uart/neon_uart.h"

/* Reset on entry; counts serviced PB5 hardware and software interrupts. */
extern volatile uint32_t g_gpio_interrupt_count;

#ifdef __cplusplus
extern "C" {
#endif

/*==================================================================================================
*                                       FUNCTION PROTOTYPES
==================================================================================================*/
/**
 * @brief Gpio interrupt run.
 * @param[in,out] uart Initialized UART handle; ownership remains with the caller.
 * @details
 * PB5 rising-edge counter. 'i' injects an interrupt; Ctrl+C returns.
 */
void gpio_interrupt_run(neon_uart_handle_t *uart);

#ifdef __cplusplus
}
#endif

#endif /* FU2_EXAMPLE_GPIO_INTERRUPT_H */
