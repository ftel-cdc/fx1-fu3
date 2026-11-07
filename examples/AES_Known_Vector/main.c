#include <stdio.h>
#include "mcu_device.h"
#include "aes_known_vector.h"

int main(void) {
    printf("\nFU2 AES_Known_Vector\n");
    aes_known_vector_run();
    printf("[STOPPED] Reset the board to run again.\n");
    for (;;) { __WFI(); }
}
