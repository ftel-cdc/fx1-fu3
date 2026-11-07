#include <stdio.h>
#include "mcu_device.h"
#include "spi_dma.h"

int main(void) {
    printf("\nFU2 SPI_DMA\n");
    spi_dma_run();
    printf("[STOPPED] Reset the board to run again.\n");
    for (;;) { __WFI(); }
}
