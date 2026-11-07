#include <stdio.h>
#include "mcu_device.h"
#include "spi_w5500.h"
#include "console/neon_console.h"

int main(void) {
    printf("\nFU2 SPI_W5500\n");
    neon_uart_handle_t *uart = neon_console_get_uart();
    if (uart == NULL) {
        printf("[FAIL] UART0 initialization failed.\n");
        for (;;) { __WFI(); }
    }

    printf("Press r to run SPI_W5500 (no Enter).\n");
    printf("fpga:>> ");
    for (;;) {
        uint32_t key = 0U;
        if (neon_uart_try_receive(uart, &key) != NEON_OK) { continue; }
        if (key == 'r') {
            printf("r\n");
            printf("[RUN] SPI_W5500\n");
            spi_w5500_run(uart);
            printf("fpga:>> ");
        }
    }
}
