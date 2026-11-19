/**
 * @file main.c
 * @brief UART terminal entry point for QSPI_XIP_Read.
 * @details
 * Initialize the UART console and run QSPI_XIP_Read when r is pressed.
 * Enter starts a new line and prints the fpga:>> prompt.
 */

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/

#include <stdio.h>
#include "mcu_device.h"
#include "qspi_xip_read.h"
#include "console/neon_console.h"

/*==================================================================================================
*                                         GLOBAL FUNCTIONS
==================================================================================================*/

/**
 * @brief Initialize the UART console and dispatch the example.
 * @return This function does not return.
 * @details
 * Press r to run the example; Enter starts a new terminal prompt line.
 */
int main(void) {
    printf("\nFU2 QSPI_XIP_Read\n");
    neon_uart_handle_t *uart = neon_console_get_uart();
    if (uart == NULL) {
        printf("[FAIL] UART0 initialization failed.\n");
        for (;;) { __WFI(); }
    }

    printf("Press r to run QSPI_XIP_Read (no Enter).\n");
    printf("fpga:>> ");
    uint32_t skip_lf = 0U;
    for (;;) {
        uint32_t key = 0U;
        if (neon_uart_try_receive(uart, &key) != NEON_OK) { continue; }
        /* Treat CRLF as one Enter; CR and LF also work on their own. */
        if (key == '\n' && skip_lf != 0U) { skip_lf = 0U; continue; }
        skip_lf = key == '\r' ? 1U : 0U;
        if (key == '\r' || key == '\n') {
            printf("\nfpga:>> ");
            continue;
        }
        if (key == 'r') {
            printf("r\n");
            printf("[RUN] QSPI_XIP_Read\n");
            qspi_xip_read_run(uart);
            printf("fpga:>> ");
        }
    }
}
