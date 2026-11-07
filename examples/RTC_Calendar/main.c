#include <stdio.h>
#include "mcu_device.h"
#include "rtc_calendar.h"

int main(void) {
    printf("\nFU2 RTC_Calendar\n");
    rtc_calendar_run();
    printf("[STOPPED] Reset the board to run again.\n");
    for (;;) { __WFI(); }
}
