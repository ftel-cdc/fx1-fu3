/**
 * @file pwm_output.h
 * @brief Public declarations for PWM output.
 * @details
 * Declares module interfaces.
 */

#ifndef FPGA_TERMINAL_PWM_OUTPUT_H
#define FPGA_TERMINAL_PWM_OUTPUT_H

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
 * @brief  Thuc thi testcase 5P1 - PWM_Output (TIM0_CH0).
 * @details Testcase khoi tao PWM o tan so dinh truoc, cung cap menu tuong tac
 *          qua UART de thay doi Duty Cycle (0%, 25%, 50%, 75%, 100%),
 *          kiem tra phan ung Stop va kiem chứng thong so qua Hardware Readback.
 * @param  uart Pointer toi cấu trúc handle UART dùng đẻ trao đổi dữ liệu terminal.
 */
void pwm_output_run(neon_uart_handle_t *uart);

#ifdef __cplusplus
}
#endif

#endif /* FPGA_TERMINAL_PWM_OUTPUT_H */