#include <stdio.h>
#include "mcu_device.h"
#include "i2c_dma.h"

int main(void) {
    printf("\nFU2 I2C_DMA\n");
    i2c_dma_run();
    printf("[STOPPED] Reset the board to run again.\n");
    for (;;) { __WFI(); }
}
