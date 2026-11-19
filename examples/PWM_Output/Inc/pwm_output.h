/**
 * @file pwm_output.h
 * @brief Public declarations for PWM output.
 * @details
 * Declares module interfaces.
 */

#ifndef FU2_EXAMPLE_PWM_OUTPUT_H
#define FU2_EXAMPLE_PWM_OUTPUT_H

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include <stdint.h>
#include "uart/neon_uart.h"

#ifdef __cplusplus
extern "C" {
#endif

/*==================================================================================================
*                                       FUNCTION PROTOTYPES
==================================================================================================*/
/**
 * @brief Run the TIM0 channel-0 PWM test menu.
 * @param[in,out] uart Initialized UART handle; ownership remains with the caller.
 * @details
 * Configure PWM at the selected frequency. UART commands set the duty cycle to
 * 0%, 25%, 50%, 75% or 100%, start/stop PWM and print the hardware register values.
 */
void pwm_output_run(neon_uart_handle_t *uart);

#ifdef __cplusplus
}
#endif

#endif /* FU2_EXAMPLE_PWM_OUTPUT_H */
