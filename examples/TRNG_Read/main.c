#include <stdio.h>
#include "mcu_device.h"
#include "trng_read.h"

int main(void) {
    printf("\nFU2 TRNG_Read\n");
    trng_read_run();
    printf("[STOPPED] Reset the board to run again.\n");
    for (;;) { __WFI(); }
}
