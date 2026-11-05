/**
 * @file system_WEACT_F412.h
 * @brief Public declarations for system weact f412.
 * @details
 * * system_WEACT_F412.h - CMSIS system file, handwritten (not auto generated). WEACT_F412 is real
 * hardware test rig (STM32F412RET6, WeAct Studio STM32F4 Core Board), NOT FU2 - see warning in
 * mcu-description/devices/WEACT_F412.yaml.
 */

#ifndef SYSTEM_WEACT_F412_H
#define SYSTEM_WEACT_F412_H

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

extern uint32_t SystemCoreClock;

/*==================================================================================================
*                                       FUNCTION PROTOTYPES
==================================================================================================*/
/**
 * @brief Initialize the device clocks and architecture state before entering main.
 */
extern void SystemInit(void);
/**
 * @brief Update SystemCoreClock from the active device clock configuration.
 */
extern void SystemCoreClockUpdate(void);

#ifdef __cplusplus
}
#endif

#endif /* SYSTEM_WEACT_F412_H */
