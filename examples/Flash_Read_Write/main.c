#include <stdio.h>
#include "mcu_device.h"
#include "flash_read_write.h"

int main(void) {
    printf("\nFU2 Flash_Read_Write\n");
    flash_read_write_run();
    printf("[STOPPED] Reset the board to run again.\n");
    for (;;) { __WFI(); }
}
