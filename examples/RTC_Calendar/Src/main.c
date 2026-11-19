/**
 * @file main.c
 * @brief UART terminal entry point for RTC_Calendar.
 * @details
 * Initialize the UART console and run RTC_Calendar when r is pressed.
 * Enter starts a new line and prints the fpga:>> prompt.
 */

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/

#include <stdio.h>
#include "mcu_device.h"
#include "rtc_calendar.h"
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
    printf("\nFU2 RTC_Calendar\n");
    neon_uart_handle_t *uart = neon_console_get_uart();
    if (uart == NULL) {
        printf("[FAIL] UART0 initialization failed.\n");
        for (;;) { __WFI(); }
    }

    printf("Press r to run RTC_Calendar (no Enter).\n");
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
            printf("[RUN] RTC_Calendar\n");
            rtc_calendar_run();
            printf("fpga:>> ");
        }
    }
}
