/**
 * @file main.c
 * @brief Implementation for templates.
 * @details
 * * Minimal starting point: no GPIO/LED demo, just periodically print the app name through printf() -
 * proof-of-life to confirm pipeline mcu-description -> CMSIS/RISC-V header/linker/startup ->
 * firmware runs end to end. Edit, remove or add code freely; this is only a starting point, not a
 * mandatory code retained.
 *
 * WEACT_F412 does not retarget printf() to UART; printf() commands on this target use the stub
 * newlib/nosys. CH32V003F4P6 (RISC-V) do not work printf() (toolchain bare-metal without real libc,
 * see arch/RISCV/README.md) - #include <stdio.h> must be checked (not just function calls)
 * otherwise it will fail at the preprocess step.
 */

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/

#include <stdint.h>

#if defined(FU2) || defined(WEACT_F412)
#include <stdio.h>
#endif

/*==================================================================================================
*                                   PRIVATE FUNCTION PROTOTYPES
==================================================================================================*/

static void templates_delay_busy(volatile uint32_t count);

/*==================================================================================================
*                                         GLOBAL FUNCTIONS
==================================================================================================*/

/**
 * @brief Initialize the application and run its main loop.
 * @return The application normally runs indefinitely.
 */
int main(void) {
    for (;;) {
#if defined(FU2) || defined(WEACT_F412)
        printf("__APP_NAME__ is running\n");
#endif
        templates_delay_busy(2000000U);
    }
}

/*==================================================================================================
*                                        PRIVATE FUNCTIONS
==================================================================================================*/

static void templates_delay_busy(volatile uint32_t count) {
    while (count--) {
    }
}
