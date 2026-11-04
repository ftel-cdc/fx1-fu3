/**
 * @file system_CH32V003F4P6.c
 * @brief Implementation for system ch32v003f4p6.
 * @details
 * * system_CH32V003F4P6.c - viet tay (khong sinh tu dong), mirror
 * system_FU2.c/system_WEACT_F412.c.
 *
 * SystemInit() TOI THIEU cho Phase 3 (bring-up dau tien, xem plan) - KHONG
 * cham thanh ghi RCC->CFGR0/PLL (giu nguyen clock mac dinh sau reset: HSI
 * 24MHz noi bo, THAT theo CH32V003RM.PDF chuong 3 "Mac dinh sau reset chay
 * HSI 24MHz noi bo, chua bat PLL") - cau hinh PLL len 48MHz (neu can) la
 * viec sau, khong chan bring-up dau tien.
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
    /* TODO (sau Phase 3): doc that RCC->CFGR0.SW/SWS de biet dang chay HSI
   * hay PLL, tinh dung SystemCoreClock - hien gia dinh mac dinh HSI 24MHz
   * (chua bat PLL trong SystemInit() nay). */
    SystemCoreClock = CH32V003_HSI_DEFAULT_CLOCK_HZ;
}
