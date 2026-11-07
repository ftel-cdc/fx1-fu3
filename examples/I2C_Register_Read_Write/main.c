#include <stdio.h>
#include "mcu_device.h"
#include "i2c_register_read_write.h"

int main(void) {
    printf("\nFU2 I2C_Register_Read_Write\n");
    i2c_register_read_write_run();
    printf("[STOPPED] Reset the board to run again.\n");
    for (;;) { __WFI(); }
}
