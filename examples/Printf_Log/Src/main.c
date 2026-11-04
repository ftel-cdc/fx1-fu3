/**
 * @file main.c
 * @brief Implementation for printf log.
 * @details
 * Implements module behavior and target-specific integration.
 */

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include <stdio.h>

/*==================================================================================================
*                                   PRIVATE FUNCTION PROTOTYPES
==================================================================================================*/

static void printf_log_halt(void);

/*==================================================================================================
*                                         GLOBAL FUNCTIONS
==================================================================================================*/

/**
 * @brief Initialize the application and run its main loop.
 * @return The application normally runs indefinitely.
 */
int main(void) {
    if (printf("[INFO] NEON MDF printf ready\n") < 0 || printf("[INFO] printf via _write\n") < 0 ||
        printf("[DEBUG] signed=%d hex=0x%08lX text=%s\n", -42, 0x1234ABCDUL, "hello") < 0) {
        printf_log_halt();
    }

    for (unsigned long count = 0U;; ++count) {
        if (printf("[INFO] counter=%lu\n", count) < 0) {
            printf_log_halt();
        }
        /* Busy-loop demonstration only; this is not a calibrated timebase. */
        for (volatile unsigned long delay = 0U; delay < 1000000UL; ++delay) {
        }
    }
}

/*==================================================================================================
*                                        PRIVATE FUNCTIONS
==================================================================================================*/

static void printf_log_halt(void) {
    for (;;) {
    }
}
