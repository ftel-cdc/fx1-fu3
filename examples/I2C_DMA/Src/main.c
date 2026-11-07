#include <stdio.h>
#include "mcu_device.h"
#include "i2c_dma.h"
#include "console/neon_console.h"

int main(void) {
    printf("\nFU2 I2C_DMA\n");
    neon_uart_handle_t *uart = neon_console_get_uart();
    if (uart == NULL) {
        printf("[FAIL] UART0 initialization failed.\n");
        for (;;) { __WFI(); }
    }

    printf("Press r to run I2C_DMA (no Enter).\n");
    printf("fpga:>> ");
    for (;;) {
        uint32_t key = 0U;
        if (neon_uart_try_receive(uart, &key) != NEON_OK) { continue; }
        if (key == 'r') {
            printf("r\n");
            printf("[RUN] I2C_DMA\n");
            i2c_dma_run();
            printf("fpga:>> ");
        }
    }
}
