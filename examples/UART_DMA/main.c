#include <stdio.h>
#include "mcu_device.h"
#include "uart_dma.h"
#include "console/neon_console.h"

int main(void) {
    printf("\nFU2 UART_DMA\n");
    neon_uart_handle_t *uart = neon_console_get_uart();
    if (uart != NULL) {
        uart_dma_run(uart);
    } else {
        printf("[FAIL] UART0 initialization failed.\n");
    }
    printf("[STOPPED] Reset the board to run again.\n");
    for (;;) { __WFI(); }
}
