#include <stdio.h>
#include "mcu_device.h"
#include "freertos_peripheral_queue.h"
#include "console/neon_console.h"

int main(void) {
    printf("\nFU2 FreeRTOS_Peripheral_Queue\n");
    neon_uart_handle_t *uart = neon_console_get_uart();
    if (uart == NULL) {
        printf("[FAIL] UART0 initialization failed.\n");
        for (;;) { __WFI(); }
    }

    printf("Press r to run FreeRTOS_Peripheral_Queue (no Enter).\n");
    printf("fpga:>> ");
    for (;;) {
        uint32_t key = 0U;
        if (neon_uart_try_receive(uart, &key) != NEON_OK) { continue; }
        if (key == 'r') {
            printf("r\n");
            printf("[RUN] FreeRTOS_Peripheral_Queue\n");
            free_rtos_peripheral_queue_run();
            printf("fpga:>> ");
        }
    }
}
