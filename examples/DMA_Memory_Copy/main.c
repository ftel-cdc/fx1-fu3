#include <stdio.h>
#include "mcu_device.h"
#include "dma_memory_copy.h"

int main(void) {
    printf("\nFU2 DMA_Memory_Copy\n");
    dma_memory_copy_run();
    printf("[STOPPED] Reset the board to run again.\n");
    for (;;) { __WFI(); }
}
