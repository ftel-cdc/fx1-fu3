#include <stdio.h>
#include "mcu_device.h"
#include "gpio_interrupt.h"
#include "console/neon_console.h"

int main(void) {
    printf("\nFU2 GPIO_Interrupt\n");
    neon_uart_handle_t *uart = neon_console_get_uart();
    if (uart != NULL) {
        gpio_interrupt_run(uart);
    } else {
        printf("[FAIL] UART0 initialization failed.\n");
    }
    printf("[STOPPED] Reset the board to run again.\n");
    for (;;) { __WFI(); }
}
