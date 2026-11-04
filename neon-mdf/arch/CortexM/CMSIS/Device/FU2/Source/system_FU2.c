/**
 * @file system_FU2.c
 * @brief Implementation for system fu2.
 * @details
 * * system_FU2.c - CMSIS system clock support for the FU2 v8 CRM.
 *
 * The internal oscillator is 20 MHz. CLK_0 selects oscillator/PLL, CLK_1
 * selects oscillator/CLK_0, and CLK.DIV applies the final divider: zero is
 * bypass and a non-zero value n divides by 2*n. The v8 PLL output is
 * Fin*M/N/(OD+1), or Fin/N when bypassed.
 */

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/

#include "FU2.h"

/*==================================================================================================
*                                        DEFINES AND MACROS
==================================================================================================*/
#define CRM_HSI_HZ 20000000UL

/* FPGA builds can use a fixed externally supplied clock without touching CRM. */
#ifdef FU2_FPGA_FIXED_CLOCK_HZ

/*==================================================================================================
*                                         LOCAL VARIABLES
==================================================================================================*/
uint32_t SystemCoreClock = FU2_FPGA_FIXED_CLOCK_HZ;
#else
uint32_t SystemCoreClock = CRM_HSI_HZ;
#endif

/*==================================================================================================
*                                         GLOBAL FUNCTIONS
==================================================================================================*/

/** @copydoc SystemInit */
void SystemInit(void) {
    SystemCoreClockUpdate();
}

/** @copydoc SystemCoreClockUpdate */
void SystemCoreClockUpdate(void) {
#ifdef FU2_FPGA_FIXED_CLOCK_HZ
    /* The FPGA supplies the system/peripheral clock directly. Do not read or
   * configure the silicon PLL, clock muxes, or divider in this build. */
    SystemCoreClock = FU2_FPGA_FIXED_CLOCK_HZ;
#else
    uint32_t clock = CRM_HSI_HZ;
    uint32_t divider = (CRM->CLK & CRM_CLK_DIV_Msk) >> CRM_CLK_DIV_Pos;

    if ((CRM->CLK_1 & CRM_CLK_1_SEL_Msk) != 0U && (CRM->CLK_0 & CRM_CLK_0_SEL_Msk) != 0U &&
        (CRM->PLL_CTRL & CRM_PLL_CTRL_POWERDOWN_Msk) == 0U && (CRM->PLL_CTRL & CRM_PLL_CTRL_OE_Msk) != 0U) {
        uint32_t m_div = (CRM->PLL & CRM_PLL_M_DIV_Msk) >> CRM_PLL_M_DIV_Pos;
        uint32_t n_div = (CRM->PLL & CRM_PLL_N_DIV_Msk) >> CRM_PLL_N_DIV_Pos;
        uint32_t od = (CRM->PLL & CRM_PLL_OD_Msk) >> CRM_PLL_OD_Pos;

        if (n_div != 0U && (((CRM->PLL_CTRL & CRM_PLL_CTRL_BYPASS_Msk) != 0U) || m_div != 0U)) {
            uint64_t pll_clock = CRM_HSI_HZ;

            if ((CRM->PLL_CTRL & CRM_PLL_CTRL_BYPASS_Msk) != 0U) {
                pll_clock /= n_div;
            } else {
                pll_clock = (pll_clock * m_div) / ((uint64_t)n_div * (od + 1U));
            }
            clock = (pll_clock > UINT32_MAX) ? UINT32_MAX : (uint32_t)pll_clock;
        }
    }

    SystemCoreClock = (divider == 0U) ? clock : clock / (2U * divider);
#endif
}
