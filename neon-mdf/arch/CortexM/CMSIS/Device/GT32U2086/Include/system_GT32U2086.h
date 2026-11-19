/**
 * @file system_GT32U2086.h
 * @brief Public declarations for system gt32u2086.
 * @details
 * * system_GT32U2086.h - CMSIS system file, handwritten (not generated automatically). CMSIS standard
 * declaration: every CMSIS device header #includes this file and needs the SystemCoreClock + 2
 * variable below to exist.
 */

#ifndef SYSTEM_GT32U2086_H
#define SYSTEM_GT32U2086_H

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
 * Call from Reset_Handler (see startup_GT32U2086.s) before entering main().
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

#endif /* SYSTEM_GT32U2086_H */
