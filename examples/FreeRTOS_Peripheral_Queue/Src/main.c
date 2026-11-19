/**
 * @file main.c
 * @brief UART terminal entry point for FreeRTOS_Peripheral_Queue.
 * @details
 * Reuse the default printf UART and run the terminal loop in a FreeRTOS task.
 * Press r to call the example; Ctrl+C returns from the example to this terminal.
 */

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/

#include <stdio.h>
#include "mcu_device.h"
#include "freertos_peripheral_queue.h"
#include "console/neon_console.h"
#include "FreeRTOS.h"
#include "task.h"

static void peripheral_queue_terminal(void *context);
static void peripheral_queue_wait_for_run(neon_uart_handle_t *uart, BaseType_t scheduler_running);

/*==================================================================================================
*                                         GLOBAL FUNCTIONS
==================================================================================================*/

/**
 * @brief Initialize the UART console and dispatch the example.
 * @return This function does not return.
 * @details
 * The terminal loop runs inside a task because the Cortex-M0 scheduler cannot
 * return to main after a run. UART initialization follows the other FU2 examples.
 */
int main(void) {
    printf("\nFU2 FreeRTOS_Peripheral_Queue\n");
    neon_uart_handle_t *uart = neon_console_get_uart();
    if (uart == NULL) {
        printf("[FAIL] UART0 initialization failed.\n");
        for (;;) { __WFI(); }
    }

    /* UART0 is shared with the loader. Wait for the serial monitor to open. */
    printf("Press r to run FreeRTOS_Peripheral_Queue (no Enter).\n");
    printf("fpga:>> ");
    peripheral_queue_wait_for_run(uart, pdFALSE);

    printf("[BOOT] Initializing GPIOB7, UART FIFO, queue and periodic task.\n");
    if (free_rtos_peripheral_queue_init(uart) == NEON_OK) {
        printf("[BOOT] Peripheral initialization OK; creating terminal task.\n");
        if (xTaskCreate(peripheral_queue_terminal, "terminal", 512U, uart,
                        tskIDLE_PRIORITY + 1U, NULL) == pdPASS) {
            printf("[BOOT] Starting FreeRTOS scheduler.\n");
            vTaskStartScheduler();
        }
        printf("[FAIL] FreeRTOS terminal did not start.\n");
    }

    /* No scheduler or UART polling loop to return to after startup failure. */
    for (;;) {
        __WFI();
    }
}

/**
 * @brief Run the same r/Enter terminal flow as the other FU2 examples.
 * @details
 * Poll only at the prompt. During run(), RX belongs to the UART ISR. The demo
 * returns on Ctrl+C, so this loop prints the next prompt without restarting RTOS.
 */
static void peripheral_queue_terminal(void *context) {
    neon_uart_handle_t *uart = (neon_uart_handle_t *)context;
    printf("[BOOT] Terminal task running.\n");
    for (;;) {
        /* The first r was received in main before starting the scheduler. */
        free_rtos_peripheral_queue_run(uart);
        printf("fpga:>> ");
        peripheral_queue_wait_for_run(uart, pdTRUE);
    }
}

/** @brief Use the same terminal parser before and after RTOS startup. */
static void peripheral_queue_wait_for_run(neon_uart_handle_t *uart, BaseType_t scheduler_running) {
    uint32_t skip_lf = 0U;
    for (;;) {
        uint32_t key = 0U;
        if (neon_uart_try_receive(uart, &key) != NEON_OK) {
            if (scheduler_running != pdFALSE) {
                vTaskDelay(pdMS_TO_TICKS(1));
            }
            continue;
        }
        /* Treat CRLF as one Enter; CR and LF also work on their own. */
        if (key == '\n' && skip_lf != 0U) { skip_lf = 0U; continue; }
        skip_lf = key == '\r' ? 1U : 0U;
        if (key == '\r' || key == '\n' || key == 3U) {
            printf("\nfpga:>> ");
            continue;
        }
        if (key == 'r') {
            printf("r\n");
            printf("[RUN] FreeRTOS_Peripheral_Queue\n");
            return;
        }
    }
}
