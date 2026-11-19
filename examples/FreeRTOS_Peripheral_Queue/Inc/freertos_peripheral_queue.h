/**
 * @file freertos_peripheral_queue.h
 * @brief UART/periodic events queued to a FreeRTOS task controlling GPIOB7.
 * @details
 * The terminal calls the demo within one task; the scheduler starts only once.
 */

#ifndef GT32U2086_EXAMPLE_FREERTOS_PERIPHERAL_QUEUE_H
#define GT32U2086_EXAMPLE_FREERTOS_PERIPHERAL_QUEUE_H

#include "uart/neon_uart.h"

#ifdef __cplusplus
extern "C" {
#endif

/*==================================================================================================
*                                       FUNCTION PROTOTYPES
==================================================================================================*/
/**
 * @brief Initialize GPIOB7/UART RX, create the event queue and periodic task once.
 * @param[in,out] uart Initialized UART0 handle borrowed from the default printf console.
 * @details
 * Call once before the scheduler starts, after the startup printf completes.
 * The handle must remain valid for the lifetime of the application. This function
 * waits for pending TX output before enabling the RX FIFO, preserving baud/format.
 * @return NEON_OK on success or NEON_ERROR on startup failure.
 */
neon_status_t free_rtos_peripheral_queue_init(neon_uart_handle_t *uart);

/**
 * @brief Process queued peripheral events until Ctrl+C, then return to the terminal.
 * @param[in,out] uart The same UART0 handle passed to init().
 * @details
 * Call from the terminal task after r, with the scheduler running. The terminal
 * must stop polling RX until this function returns. RX IRQ is enabled only for
 * the run, and is disabled before returning. Queue/tasks are reused across runs.
 */
void free_rtos_peripheral_queue_run(neon_uart_handle_t *uart);

#ifdef __cplusplus
}
#endif

#endif /* GT32U2086_EXAMPLE_FREERTOS_PERIPHERAL_QUEUE_H */
