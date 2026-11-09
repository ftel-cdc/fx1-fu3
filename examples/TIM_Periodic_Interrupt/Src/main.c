#include <stdio.h>
#include "mcu_device.h"
#include "tim_periodic_interrupt.h"
#include "console/neon_console.h"

int main(void) {
    printf("\nFU2 TIM_Periodic_Interrupt\n");
    neon_uart_handle_t *uart = neon_console_get_uart();
    if (uart == NULL) {
        printf("[FAIL] UART0 initialization failed.\n");
        for (;;) { __WFI(); }
    }

    printf("Press r to run TIM_Periodic_Interrupt (no Enter).\n");
    printf("fpga:>> ");
    uint32_t skip_lf = 0U;
    for (;;) {
        uint32_t key = 0U;
        if (neon_uart_try_receive(uart, &key) != NEON_OK) { continue; }
        /* Treat CRLF as one Enter; CR and LF also work on their own. */
        if (key == '\n' && skip_lf != 0U) { skip_lf = 0U; continue; }
        skip_lf = key == '\r' ? 1U : 0U;
        if (key == '\r' || key == '\n') {
            printf("\nfpga:>> ");
            continue;
        }
        if (key == 'r') {
            printf("r\n");
            printf("[RUN] TIM_Periodic_Interrupt\n");
            tim_periodic_interrupt_run(uart);
            printf("fpga:>> ");
        }
    }
}
