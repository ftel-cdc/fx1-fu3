/**
 * @file system_CH32V003F4P6.h
 * @brief Public declarations for system ch32v003f4p6.
 * @details
 * * system_CH32V003F4P6.h - handwritten (not auto generated), mirror system_FU2.h/system_WEACT_F412.h
 * on Cortex-M (the same SystemCoreClock/SystemInit/SystemCoreClockUpdate API; startup_CH32V003F4P6.s
 * (auto generated) call `jal SystemInit` before vao main(), see gen_startup.py).
 */

#ifndef SYSTEM_CH32V003F4P6_H
#define SYSTEM_CH32V003F4P6_H

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** current CPU frequency (Hz) - must call SystemCoreClockUpdate() after changing clock. */
extern uint32_t SystemCoreClock;

/*==================================================================================================
*                                       FUNCTION PROTOTYPES
==================================================================================================*/
/**
 * @brief Initialize the device clocks and architecture state before entering main.
 * @details
 * Call from handle_reset (see startup_CH32V003F4P6.s) before entering main().
 */
extern void SystemInit(void);

/**
 * @brief Update SystemCoreClock from the active device clock configuration.
 * @details
 * read back the current clock configuration and update SystemCoreClock.
 */
extern void SystemCoreClockUpdate(void);

#ifdef __cplusplus
}
#endif

#endif /* SYSTEM_CH32V003F4P6_H */
