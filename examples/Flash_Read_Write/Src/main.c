#include <stdio.h>
#include "mcu_device.h"
#include "flash_read_write.h"
#include "console/neon_console.h"

int main(void) {
    printf("\nFU2 Flash_Read_Write\n");
    neon_uart_handle_t *uart = neon_console_get_uart();
    if (uart == NULL) {
        printf("[FAIL] UART0 initialization failed.\n");
        for (;;) { __WFI(); }
    }

    printf("Press r to run Flash_Read_Write (no Enter).\n");
    printf("fpga:>> ");
    for (;;) {
        uint32_t key = 0U;
        if (neon_uart_try_receive(uart, &key) != NEON_OK) { continue; }
        if (key == 'r') {
            printf("r\n");
            printf("[RUN] Flash_Read_Write\n");
            flash_read_write_run();
            printf("fpga:>> ");
        }
    }
}
