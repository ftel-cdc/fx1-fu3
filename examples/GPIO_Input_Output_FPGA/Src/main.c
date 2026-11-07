#include <stdio.h>
#include "mcu_device.h"
#include "gpio_input_output.h"
#include "console/neon_console.h"

int main(void) {
    printf("\nFU2 GPIO_Input_Output_FPGA\n");
    neon_uart_handle_t *uart = neon_console_get_uart();
    if (uart == NULL) {
        printf("[FAIL] UART0 initialization failed.\n");
        for (;;) { __WFI(); }
    }

    printf("Press r to run GPIO_Input_Output_FPGA (no Enter).\n");
    printf("fpga:>> ");
    for (;;) {
        uint32_t key = 0U;
        if (neon_uart_try_receive(uart, &key) != NEON_OK) { continue; }
        if (key == 'r') {
            printf("r\n");
            printf("[RUN] GPIO_Input_Output_FPGA\n");
            gpio_input_output_run(uart);
            printf("fpga:>> ");
        }
    }
}
