#include <stdio.h>
#include "mcu_device.h"
#include "i2c_slave.h"

int main(void) {
    printf("\nFU2 I2C_Slave\n");
    i2c_slave_run();
    printf("[STOPPED] Reset the board to run again.\n");
    for (;;) { __WFI(); }
}
