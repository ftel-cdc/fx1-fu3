/**
 * @file system_WEACT_F412.c
 * @brief Implementation for system weact f412.
 * @details
 * * system_WEACT_F412.c - CMSIS system file, handwritten (not generated automatically). Default
 * STM32F4 running 16MHz internal HSI after reset (not yet configured PLL) - enough for current GPIO
 * blink example (see mcu-description/devices/ WEACT_F412.yaml - GPIOB range only, not yet
 * configured clock tree via RCC).
 *
 * IMPORTANT (OTHER than FU2/NUCLEO_L031K6 - this is the FIRST Cortex-M4F chip in the SDK): MUST enable
 * FPU full access (CP10/CP11 full access) via SCB->CPACR BEFORE any floating-point command runs -
 * build using flags
 * -mfloat-abi=hard (see cmake/arch/cortex-m.cmake) so the compiler CAN create FPU commands at any time
 * (even code that does not write floats by itself, e.g. some integer operations on M4 can still be
 * passed through the common branch code). Missing this step will UsageFault the first FPU command -
 * this is the standard step of CMSIS system_stm32f4xx.c for real, not manually.
 */

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/

#include "WEACT_F412.h"

/*==================================================================================================
*                                         LOCAL VARIABLES
==================================================================================================*/
extern uint32_t g_pfnVectors[];

uint32_t SystemCoreClock = 16000000UL;

/*==================================================================================================
*                                         GLOBAL FUNCTIONS
==================================================================================================*/

/** @copydoc SystemInit */
void SystemInit(void) {
#if (__FPU_PRESENT == 1) && (__FPU_USED == 1)
    /* Set CP10 and CP11 Full Access (using ST's real system_stm32f4xx.c) */
    SCB->CPACR |= ((3UL << 10 * 2) | (3UL << 11 * 2));
#endif

    /* from relocate SCB->VTOR to use only the vector table dia of this EXECUTABLE itself (g_pfnVectors is
   * the symbol linker set, the resolution dia differs depending on the linker script in use - see
   * gen_startup_vector.py). This applies to all three build layouts: whole flash (Blinky,
   * resolve to flash head, no-op by default HW), bootloader (resolve to bootloader head, no-op),
   * and applications under the bootloader resolve to NEON_MDF_APP_VECTOR_BASE; this covers
   * where this dynamic is needed, the bootloader jumping on the app does not automatically fix VTOR
   * support, and also the case where the flash app directly through SWD/debug without via
   * bootloader must be used together). must be set after enabling the FPU above (the same order as
   * the real system_stm32f4xx.c of ST), before enabling the interrupt can grind out. */
    SCB->VTOR = (uint32_t)g_pfnVectors;

    SystemCoreClockUpdate();
}

/** @copydoc SystemCoreClockUpdate */
void SystemCoreClockUpdate(void) {
    /* PLACEHOLDER: did not read RCC->CFGR to calculate true SystemCoreClock if later app manually
   * configures PLL/HSE - currently assumes default 16MHz HSI because for example GPIO blink does
   * not change clock tree. */
    SystemCoreClock = 16000000UL;
}
