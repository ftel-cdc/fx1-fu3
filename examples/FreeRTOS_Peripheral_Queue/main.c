#include <stdio.h>
#include "mcu_device.h"
#include "freertos_peripheral_queue.h"

int main(void) {
    printf("\nFU2 FreeRTOS_Peripheral_Queue\n");
    free_rtos_peripheral_queue_run();
    printf("[STOPPED] Reset the board to run again.\n");
    for (;;) { __WFI(); }
}
