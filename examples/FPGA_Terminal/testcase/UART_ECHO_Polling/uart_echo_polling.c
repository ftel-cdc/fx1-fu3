/**
 * @file uart_echo_polling.c
 * @brief Implementation for UART echo polling.
 * @details
 * UART_ECHO_Polling adapted as a returnable FPGA_Terminal testcase.
 */

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/

#include "uart_echo_polling.h"
#include <stddef.h>
#include <stdio.h>

#include "sdkconfig.h"

/*==================================================================================================
*                                         LOCAL VARIABLES
==================================================================================================*/
volatile uart_echo_stats_t g_uart_echo_stats;

/*==================================================================================================
*                                   PRIVATE FUNCTION PROTOTYPES
==================================================================================================*/

static void uart_echo_polling_record_status(neon_uart_handle_t *uart, neon_status_t status);
static void uart_echo_polling_print_stats(void);

/*==================================================================================================
*                                         GLOBAL FUNCTIONS
==================================================================================================*/

/** @copydoc uart_echo_polling_run */
void uart_echo_polling_run(neon_uart_handle_t *uart, uint32_t skip_initial_lf) {
    g_uart_echo_stats = (uart_echo_stats_t){0};
    neon_uart_config_t config;
    if (uart == NULL || neon_uart_get_instance(uart) != NEON_UART0 ||
        neon_uart_get_state(uart) != NEON_UART_STATE_READY || neon_uart_get_config(uart, &config) != NEON_OK ||
        config.word_length != NEON_UART_WORD_LENGTH_8B) {
        printf("[ERROR] UART echo requires a ready 8-bit UART0.\n");
        return;
    }

    uart_echo_polling_record_status(uart, NEON_OK);
    g_uart_echo_stats.initialized = 1U;
    printf("[INFO] UART echo ready. Type a character; Ctrl+C returns to terminal.\n");

    for (;;) {
        /* A one-byte buffer owns a complete aligned word on FU2. */
        uint32_t byte_word = 0U;
        neon_status_t status = neon_uart_receive(uart, (uint8_t *)&byte_word, 1U, CONFIG_UART_ECHO_RX_TIMEOUT_POLLS);
        if (status == NEON_TIMEOUT) {
            ++g_uart_echo_stats.rx_timeouts;
            continue;
        }
        if (status != NEON_OK) {
            ++g_uart_echo_stats.rx_errors;
            uart_echo_polling_record_status(uart, status);
            printf("[ERROR] RX status=%lu error=0x%08lX\n", (unsigned long)status,
                   (unsigned long)g_uart_echo_stats.last_error);
            neon_uart_clear_error(uart);
            skip_initial_lf = 0U;
            continue;
        }

        uint32_t value = byte_word & 0xFFU;
        if (skip_initial_lf != 0U) {
            skip_initial_lf = 0U;
            if (value == '\n') {
                continue;
            }
        }
        if (value == 3U) {
            break;
        }
        ++g_uart_echo_stats.rx_bytes;

        int display = (value >= 0x20U && value <= 0x7EU) ? (int)value : '.';
        printf("[USER -> FPGA] '%c' (0x%02X)\n", display, (unsigned int)value);
        printf("[FPGA -> USER] ");

        status = neon_uart_transmit(uart, (const uint8_t *)&byte_word, 1U, CONFIG_UART_ECHO_TX_TIMEOUT_POLLS);
        if (status == NEON_OK) {
            ++g_uart_echo_stats.tx_bytes;
            printf(" (0x%02X)\n", (unsigned int)value);
        } else {
            if (status == NEON_TIMEOUT) {
                ++g_uart_echo_stats.tx_timeouts;
            } else {
                ++g_uart_echo_stats.tx_errors;
            }
            uart_echo_polling_record_status(uart, status);
            printf("[ERROR] TX status=%lu\n", (unsigned long)status);
            /* Resume RX without retrying a byte which may already be queued. */
        }
    }

    printf("\n[INFO] UART echo stopped.\n");
    uart_echo_polling_print_stats();
}

/*==================================================================================================
*                                        PRIVATE FUNCTIONS
==================================================================================================*/

static void uart_echo_polling_record_status(neon_uart_handle_t *uart, neon_status_t status) {
    g_uart_echo_stats.last_status = (uint32_t)status;
    g_uart_echo_stats.last_error = neon_uart_get_error(uart);
}

static void uart_echo_polling_print_stats(void) {
    printf("[STATS] RX=%lu TX=%lu RX_TIMEOUT=%lu TX_TIMEOUT=%lu "
           "RX_ERROR=%lu TX_ERROR=%lu\n",
           (unsigned long)g_uart_echo_stats.rx_bytes, (unsigned long)g_uart_echo_stats.tx_bytes,
           (unsigned long)g_uart_echo_stats.rx_timeouts, (unsigned long)g_uart_echo_stats.tx_timeouts,
           (unsigned long)g_uart_echo_stats.rx_errors, (unsigned long)g_uart_echo_stats.tx_errors);
    printf("[STATS] Last status=%lu error=0x%08lX\n", (unsigned long)g_uart_echo_stats.last_status,
           (unsigned long)g_uart_echo_stats.last_error);
}
