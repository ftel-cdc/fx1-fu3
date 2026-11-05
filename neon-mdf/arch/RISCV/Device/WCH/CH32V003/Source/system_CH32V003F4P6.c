/**
 * @file system_CH32V003F4P6.c
 * @brief Implementation for system ch32v003f4p6.
 * @details
 * * system_CH32V003F4P6.c - handwritten (not automatic creation), mirror
 * system_FU2.c/system_WEACT_F412.c.
 *
 * SystemInit() to missing for Phase 3 (first bring-up, see plan) - no cham register RCC->CFGR0/PLL
 * (keep the default clock after reset: HSI 24MHz where set, true according to CH32V003RM.PDF
 * chapter 3 "default after reset run HSI 24MHz where set, not yet enabled PLL") - configuration PLL
 * to 48MHz can be added later if needed; it is outside the initial bring-up scope.
 */

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/

#include "CH32V003F4P6.h"

/*==================================================================================================
*                                        DEFINES AND MACROS
==================================================================================================*/
#define CH32V003_HSI_DEFAULT_CLOCK_HZ 24000000UL

/*==================================================================================================
*                                         LOCAL VARIABLES
==================================================================================================*/
uint32_t SystemCoreClock = CH32V003_HSI_DEFAULT_CLOCK_HZ;

/*==================================================================================================
*                                         GLOBAL FUNCTIONS
==================================================================================================*/

/** @copydoc SystemInit */
void SystemInit(void) {
    SystemCoreClockUpdate();
}

/** @copydoc SystemCoreClockUpdate */
void SystemCoreClockUpdate(void) {
    /* TODO (after Phase 3): read real RCC->CFGR0.SW/SWS to know running HSI please PLL, use
   * SystemCoreClock - currently assumes default HSI 24MHz (PLL not enabled in this SystemInit()). */
    SystemCoreClock = CH32V003_HSI_DEFAULT_CLOCK_HZ;
}
