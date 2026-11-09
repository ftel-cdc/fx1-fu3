/**
 * @file tim_periodic_interrupt.h
 * @brief Public declarations for timer periodic interrupt.
 * @details
 * Declares module interfaces.
 */

#ifndef FU2_EXAMPLE_TIM_PERIODIC_INTERRUPT_H
#define FU2_EXAMPLE_TIM_PERIODIC_INTERRUPT_H

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include <stdint.h>
#include "uart/neon_uart.h"

/* Reset on entry; counts enabled TIM1 CH0 UPDATE events observed in the ISR. */
extern volatile uint32_t g_tim_periodic_ticks;

#ifdef __cplusplus
extern "C" {
#endif

/*==================================================================================================
*                                       FUNCTION PROTOTYPES
==================================================================================================*/
/**
 * @brief Tim periodic interrupt run.
 * @param[in,out] uart Initialized UART handle; ownership remains with the caller.
 * @details
 * TIM1 CH0 toggles PB7 every 500 ms. Ctrl+C stops the example.
 */
void tim_periodic_interrupt_run(neon_uart_handle_t *uart);

#ifdef __cplusplus
}
#endif

#endif /* FU2_EXAMPLE_TIM_PERIODIC_INTERRUPT_H */
