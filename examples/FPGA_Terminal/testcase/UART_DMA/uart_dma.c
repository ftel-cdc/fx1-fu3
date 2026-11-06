/** @file uart_dma.c
 * @brief UART0 TX/RX DMA using fixed patterns and a working main loop.
 */
#include "uart_dma.h"
#include "crm/neon_crm.h"
#include "dma/neon_dma.h"
#include "sdkconfig.h"
#include <stdio.h>

#ifndef CONFIG_NEON_MDF_DRIVER_DMA
#error "UART_DMA requires CONFIG_NEON_MDF_DRIVER_DMA=y"
#endif

void uart_dma_run(neon_uart_handle_t *uart) {
    static const uint32_t pattern[16] = {'0', '1', '2', '3', '4', '5', '6', '7',
                                         '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};
    /* FU2 uses word accesses: one UART character per uint32_t, not packed bytes. */
    static uint32_t tx[64], rx[16];
    neon_uart_config_t saved, config;
    if (uart == NULL || neon_uart_get_instance(uart) != NEON_UART0 ||
        neon_uart_get_config(uart, &saved) != NEON_OK) {
        printf("[FAIL] UART DMA needs the terminal UART0.\n");
        return;
    }
    if (neon_uart_wait_tx_complete(uart, CONFIG_UART_DMA_TIMEOUT_POLLS) != NEON_OK) { return; }
    /* DMA0 is exclusive to this testcase; do not reset another active user. */
    if (neon_dma_deinit(DMA0) != NEON_OK || neon_crm_peripheral_reset(NEON_CRM_PERIPHERAL_DMA0) != NEON_OK) {
        printf("[FAIL] DMA0 is busy or could not be reset.\n");
        return;
    }
    config = saved;
    config.fifo_state = NEON_UART_FIFO_ENABLE;
    if (neon_uart_set_config(uart, &config) != NEON_OK) { goto restore; }
    for (uint32_t i = 0U; i < 64U; ++i) { tx[i] = pattern[i % 16U]; }
    printf("[UART DMA] UART0, DMA0 channel 0, request slot 2.\n");
    printf(" t: TX 64 characters | r: RX 0123456789ABCDEF | q/Ctrl+C: exit\n");
    printf(" Press one key, no Enter. RX waits for all 16 characters or the polling timeout.\n");
    for (;;) {
        uint32_t key = 0U;
        if (neon_uart_try_receive(uart, &key) != NEON_OK) { continue; }
        if (key == 'q' || key == 'Q' || key == 3U) { break; }
        if (key != 't' && key != 'r') { continue; }
        uint32_t receiving = key == 'r';
        uint32_t count = receiving != 0U ? 16U : 64U;
        uint32_t fifo_address = (uint32_t)(uintptr_t)&uart->instance->RBR_DLL_THR;
        neon_uart_dma_direction_t direction = receiving != 0U ? NEON_UART_DMA_RX : NEON_UART_DMA_TX;
        for (uint32_t i = 0U; i < 16U; ++i) { rx[i] = 0U; }
        neon_dma_config_t dma = {
            .src_addr = receiving != 0U ? fifo_address : (uint32_t)(uintptr_t)tx,
            .dst_addr = receiving != 0U ? (uint32_t)(uintptr_t)rx : fifo_address,
            .transfer_size = count * sizeof(uint32_t),
            .channel = 0U,
            .read_config = receiving != 0U ? NEON_DMA_CONFIG_FIXED_WORD : NEON_DMA_CONFIG_M2M_WORD,
            .write_config = receiving != 0U ? NEON_DMA_CONFIG_M2M_WORD : NEON_DMA_CONFIG_FIXED_WORD,
            .control = NEON_DMA_CONTROL_M2M,
            .peripheral_config = receiving != 0U ? NEON_UART0_DMA_PERIPHERAL_ID : NEON_UART0_DMA_PERIPHERAL_ID << 16,
        };
        if (receiving == 0U) { printf("[FPGA -> PC] DMA TX 64: <"); }
        if (neon_uart_wait_tx_complete(uart, CONFIG_UART_DMA_TIMEOUT_POLLS) != NEON_OK) { break; }
        if (receiving != 0U && neon_uart_clear_rx_fifo(uart) != NEON_OK) { break; }
        neon_status_t status = neon_uart_set_dma(uart, direction, (uint16_t)count);
        if (status == NEON_OK) { status = neon_dma_start_channel(DMA0, &dma); }
        uint32_t work = 0U;
        if (status == NEON_OK) {
            /* Arm RX before printing READY; CPU does not read RBR during DMA. */
            if (receiving != 0U) { printf("[READY] PC -> FPGA: send 0123456789ABCDEF, no Enter.\n"); }
            do {
                ++work; /* Main can do other work while DMA moves the characters. */
                status = neon_dma_poll_channel(DMA0, 0U);
                if (status == NEON_OK) {
                    uint32_t control = receiving != 0U ? uart->instance->DMA_RX : uart->instance->DMA_TX;
                    if ((control & UART_DMA_RX_TRAN_DONE_Msk) == 0U) { status = NEON_BUSY; }
                }
            } while (status == NEON_BUSY && work < CONFIG_UART_DMA_TIMEOUT_POLLS);
            if (status == NEON_BUSY) { status = NEON_TIMEOUT; }
        }
        neon_status_t stopped = neon_uart_set_dma(uart, direction, 0U);
        /* Disabling a DMA channel only parks it. Reset DMA0 to abort any partial transfer. */
        neon_status_t reset = neon_crm_peripheral_reset(NEON_CRM_PERIPHERAL_DMA0);
        if (reset != NEON_OK) { printf("[FAIL] DMA0 reset failed.\n"); break; }
        if (neon_uart_wait_tx_complete(uart, CONFIG_UART_DMA_TIMEOUT_POLLS) != NEON_OK) { break; }
        if (receiving == 0U) { printf(">\n"); }
        if (status != NEON_OK || stopped != NEON_OK) {
            printf("[FAIL] DMA status=%lu UART status=%lu; main iterations=%lu.\n",
                   (unsigned long)status, (unsigned long)stopped, (unsigned long)work);
            continue;
        }
        if (receiving != 0U) {
            uint32_t matches = 1U;
            printf("[PC -> FPGA] RX16: ");
            for (uint32_t i = 0U; i < 16U; ++i) {
                printf("%c", (int)(rx[i] & 0xFFU));
                if (rx[i] != pattern[i]) { matches = 0U; }
            }
            printf(" %s\n", matches != 0U ? "[PASS]" : "[FAIL] expected 0123456789ABCDEF");
        } else {
            printf("[TX] DMA and UART completed; compare the 64 characters above on PC.\n");
        }
        printf("[MAIN] iterations while DMA active: %lu\n", (unsigned long)work);
    }
restore:
    neon_uart_wait_tx_complete(uart, CONFIG_UART_DMA_TIMEOUT_POLLS);
    if (neon_uart_set_config(uart, &saved) != NEON_OK) { printf("[FAIL] UART0 restore failed.\n"); }
}
