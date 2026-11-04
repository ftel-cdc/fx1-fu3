/**
 * @file system_STM32F103CB.h
 * @brief Public declarations for system stm32f103cb.
 * @details
 * * system_STM32F103CB.h - CMSIS system file, written by hand.
 * The generated STM32F103CB.h device header includes this file.
 */

#ifndef SYSTEM_STM32F103CB_H
#define SYSTEM_STM32F103CB_H

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Current CPU clock frequency in Hz. */
extern uint32_t SystemCoreClock;

/*==================================================================================================
*                                       FUNCTION PROTOTYPES
==================================================================================================*/
/**
 * @brief Initialize the device clocks and architecture state before entering main.
 * @details
 * Called by Reset_Handler before main().
 */
void SystemInit(void);

/**
 * @brief Update SystemCoreClock from the active device clock configuration.
 * @details
 * Recalculate SystemCoreClock after a clock-tree change.
 */
void SystemCoreClockUpdate(void);

#ifdef __cplusplus
}
#endif

#endif /* SYSTEM_STM32F103CB_H */
