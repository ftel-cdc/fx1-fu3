/**
 * @file uart_echo_polling.h
 * @brief Public declarations for UART echo polling.
 * @details
 * Declares module interfaces.
 */

#ifndef FU2_EXAMPLE_UART_ECHO_POLLING_H
#define FU2_EXAMPLE_UART_ECHO_POLLING_H

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include <stdint.h>
#include "uart/neon_uart.h"

/*==================================================================================================
*                                  STRUCTURES AND OTHER TYPEDEFS
==================================================================================================*/
typedef struct {
    uint32_t initialized;
    uint32_t rx_bytes;
    uint32_t tx_bytes;
    uint32_t rx_timeouts;
    uint32_t tx_timeouts;
    uint32_t rx_errors;
    uint32_t tx_errors;
    uint32_t last_status;
    uint32_t last_error;
} uart_echo_stats_t;

/* Reset on entry. Counts exclude diagnostics and Ctrl+C. */
extern volatile uart_echo_stats_t g_uart_echo_stats;

#ifdef __cplusplus
extern "C" {
#endif

/*==================================================================================================
*                                       FUNCTION PROTOTYPES
==================================================================================================*/
/**
 * @brief Uart echo polling run.
 * @param[in,out] uart Initialized UART handle; ownership remains with the caller.
 * @details
 * Borrow the initialized UART0 handle without reconfiguring it. Ctrl+C returns.
 */
void uart_echo_polling_run(neon_uart_handle_t *uart);

#ifdef __cplusplus
}
#endif

#endif /* FU2_EXAMPLE_UART_ECHO_POLLING_H */
