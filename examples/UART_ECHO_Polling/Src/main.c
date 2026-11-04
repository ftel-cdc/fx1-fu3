/**
 * @file main.c
 * @brief Implementation for UART echo polling.
 * @details
 * FU2 UART0 polling echo, with simple RX/TX diagnostic messages.
 */

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/

#include <stdint.h>
#include <stdio.h>
#include "uart/neon_uart.h"
#include "sdkconfig.h"

#ifndef CONFIG_NEON_MDF_DRIVER_UART
#error "UART_ECHO_Polling requires CONFIG_NEON_MDF_DRIVER_UART=y"
#endif

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

/*==================================================================================================
*                                         LOCAL VARIABLES
==================================================================================================*/
/* Counts refer to received/echoed data bytes, excluding printf output. */
volatile uart_echo_stats_t g_uart_echo_stats;
static neon_uart_handle_t echo_uart;

/*==================================================================================================
*                                   PRIVATE FUNCTION PROTOTYPES
==================================================================================================*/

static void uart_echo_polling_record_status(neon_status_t status);

/*==================================================================================================
*                                         GLOBAL FUNCTIONS
==================================================================================================*/

/**
 * @brief Initialize the application and run its main loop.
 * @return The application normally runs indefinitely.
 */
int main(void) {
    /* Initialize printf's default UART0 before applying the echo configuration. */
    printf("[INFO] Starting UART0 echo...\n");
    /* Drain the queued banner before config resets/reconfigures UART0. */
    uint32_t startup_timeout = CONFIG_UART_ECHO_TX_TIMEOUT_POLLS;
    while ((UART0->LSR & UART_LSR_TEMT_Msk) == 0U) {
        if (startup_timeout == 0U) {
            for (;;) {
                __WFI();
            }
        }
        --startup_timeout;
    }

    neon_uart_config_t config;
    neon_uart_get_default_config(&config);
    config.baud_rate = CONFIG_UART_ECHO_BAUD_RATE;
    config.clock_frequency_hz = SystemCoreClock;
    config.fifo_state = NEON_UART_FIFO_ENABLE;

    neon_status_t status = neon_uart_init(&echo_uart, NEON_UART0);
    if (status == NEON_OK) {
        status = neon_uart_set_config(&echo_uart, &config);
    }
    uart_echo_polling_record_status(status);
    if (status != NEON_OK) {
        for (;;) {
            __WFI();
        }
    }
    g_uart_echo_stats.initialized = 1U;
    printf("[INFO] UART echo ready. Type a character.\n");

    for (;;) {
        /* A one-byte buffer owns a complete aligned word on FU2. */
        uint32_t byte_word = 0U;
        status = neon_uart_receive(&echo_uart, (uint8_t *)&byte_word, 1U, CONFIG_UART_ECHO_RX_TIMEOUT_POLLS);
        if (status == NEON_TIMEOUT) {
            ++g_uart_echo_stats.rx_timeouts;
            continue;
        }
        if (status != NEON_OK) {
            ++g_uart_echo_stats.rx_errors;
            uart_echo_polling_record_status(status);
            printf("[ERROR] RX status=%lu error=0x%08lX\n", (unsigned long)status,
                   (unsigned long)g_uart_echo_stats.last_error);
            neon_uart_clear_error(&echo_uart);
            continue;
        }
        ++g_uart_echo_stats.rx_bytes;

        uint32_t value = byte_word & 0xffU;
        int display = (value >= 0x20U && value <= 0x7eU) ? (int)value : '.';
        printf("[USER -> FPGA] '%c' (0x%02X)\n", display, (unsigned int)value);
        printf("[FPGA -> USER] ");

        status = neon_uart_transmit(&echo_uart, (const uint8_t *)&byte_word, 1U, CONFIG_UART_ECHO_TX_TIMEOUT_POLLS);
        if (status == NEON_OK) {
            ++g_uart_echo_stats.tx_bytes;
            printf(" (0x%02X)\n", (unsigned int)value);
        } else {
            if (status == NEON_TIMEOUT) {
                ++g_uart_echo_stats.tx_timeouts;
            } else {
                ++g_uart_echo_stats.tx_errors;
            }
            uart_echo_polling_record_status(status);
            printf("[ERROR] TX status=%lu\n", (unsigned long)status);
            /* Resume RX; a failed echo is not retried automatically. */
        }
    }
}

/*==================================================================================================
*                                        PRIVATE FUNCTIONS
==================================================================================================*/

static void uart_echo_polling_record_status(neon_status_t status) {
    g_uart_echo_stats.last_status = (uint32_t)status;
    g_uart_echo_stats.last_error = neon_uart_get_error(&echo_uart);
}
