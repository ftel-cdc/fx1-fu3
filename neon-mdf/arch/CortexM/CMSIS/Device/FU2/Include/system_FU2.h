/**
 * @file system_FU2.h
 * @brief Public declarations for system fu2.
 * @details
 * * system_FU2.h - CMSIS system file, handwritten (not generated automatically). CMSIS standard
 * declaration: every CMSIS device header #includes this file and needs the SystemCoreClock + 2
 * variable below to exist.
 */

#ifndef SYSTEM_FU2_H
#define SYSTEM_FU2_H

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
 * Call from Reset_Handler (see startup_FU2.s) before entering main().
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

#endif /* SYSTEM_FU2_H */
