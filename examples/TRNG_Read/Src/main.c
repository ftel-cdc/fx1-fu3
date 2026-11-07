#include <stdio.h>
#include "mcu_device.h"
#include "trng_read.h"
#include "console/neon_console.h"

int main(void) {
    printf("\nFU2 TRNG_Read\n");
    neon_uart_handle_t *uart = neon_console_get_uart();
    if (uart == NULL) {
        printf("[FAIL] UART0 initialization failed.\n");
        for (;;) { __WFI(); }
    }

    printf("Press r to run TRNG_Read (no Enter).\n");
    printf("fpga:>> ");
    for (;;) {
        uint32_t key = 0U;
        if (neon_uart_try_receive(uart, &key) != NEON_OK) { continue; }
        if (key == 'r') {
            printf("r\n");
            printf("[RUN] TRNG_Read\n");
            trng_read_run();
            printf("fpga:>> ");
        }
    }
}
