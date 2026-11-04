/**
 * @file system_STM32F103CB.c
 * @brief Implementation for system stm32f103cb.
 * @details
 * * system_STM32F103CB.c - CMSIS system file, written by hand.
 *
 * Initial bring-up intentionally keeps the reset clock configuration: the
 * internal HSI oscillator is the system clock at 8 MHz. PLL/HSE setup belongs
 * to a later clock-tree implementation and must also update this file's
 * SystemCoreClock calculation.
 */

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/

#include "STM32F103CB.h"

/*==================================================================================================
*                                        DEFINES AND MACROS
==================================================================================================*/
#define STM32F103CB_HSI_CLOCK_HZ 8000000UL

/*==================================================================================================
*                                         LOCAL VARIABLES
==================================================================================================*/
extern uint32_t g_pfnVectors[];

uint32_t SystemCoreClock = STM32F103CB_HSI_CLOCK_HZ;

/*==================================================================================================
*                                         GLOBAL FUNCTIONS
==================================================================================================*/

/** @copydoc SystemInit */
void SystemInit(void) {
    /* Use the vector table linked into this executable. This remains correct
   * if a later linker layout relocates the application in Flash. */
    SCB->VTOR = (uint32_t)g_pfnVectors;
    SystemCoreClockUpdate();
}

/** @copydoc SystemCoreClockUpdate */
void SystemCoreClockUpdate(void) {
    /* Bring-up does not modify RCC yet, so SYSCLK remains HSI at 8 MHz. */
    SystemCoreClock = STM32F103CB_HSI_CLOCK_HZ;
}
