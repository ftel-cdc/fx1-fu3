#include <stdio.h>
#include "mcu_device.h"
#include "adc_sampling.h"

int main(void) {
    printf("\nFU2 ADC_Sampling\n");
    adc_sampling_run();
    printf("[STOPPED] Reset the board to run again.\n");
    for (;;) { __WFI(); }
}
