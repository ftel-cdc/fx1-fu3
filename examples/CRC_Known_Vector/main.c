#include <stdio.h>
#include "mcu_device.h"
#include "crc_known_vector.h"

int main(void) {
    printf("\nFU2 CRC_Known_Vector\n");
    crc_known_vector_run();
    printf("[STOPPED] Reset the board to run again.\n");
    for (;;) { __WFI(); }
}
