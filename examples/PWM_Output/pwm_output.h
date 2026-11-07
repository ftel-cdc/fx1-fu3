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
 * @brief Execute testcase 5P1 - PWM_Output (TIM0_CH0).
 * @details Testcase initialize PWM at predefined frequency, same level interactive menu via UART to
 *          see change Duty Cycle (0%, 25%, 50%, 75%, 100%), check Stop cancer section and verify
 *          parameters via Hardware Readback.
 * @param uart Pointer uses the UART handle structure for exchanging console data.
 */
void pwm_output_run(neon_uart_handle_t *uart);

#ifdef __cplusplus
}
#endif

#endif /* FU2_EXAMPLE_PWM_OUTPUT_H */