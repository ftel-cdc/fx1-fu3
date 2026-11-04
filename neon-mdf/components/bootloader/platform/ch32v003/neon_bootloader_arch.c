/**
 * @file platform/ch32v003/neon_bootloader_arch.c
 * @brief Implementation for bootloader arch.
 * @details
 * * platform/ch32v003/neon_bootloader_arch.c - implementation THAT cua
 * neon_bootloader_arch.h cho CH32V003 (WCH QingKe V2A/PFIC) - dia chi THAT
 * tu CH32V003RM.PDF V1.9:
 *   STK_CTLR/SR/CNTL/CMPLR (SysTick THAT cua core, KHAC ARM ve register
 *   layout hoan toan) tai 0xE000F000 (chuong 6.5.4) - dung STCLK=1 (HCLK
 *   lam time base) + STRE=1 (auto-reload) + so sanh CNTIF (STK_SR bit0).
 *   PFIC_SCTLR tai 0xE000ED10 (chuong 6.5.2.22), bit31=SYSRESET.
 *   PFIC dung mtvec[1:0]=3 ("address" mode vendor-specific) - vector[0]
 *   cua app (sinh boi gen_startup.py) la 1 LENH `j handle_reset`, KHONG
 *   PHAI [SP,ResetAddr] nhu ARM - app tu lo sp/gp cua CHINH NO, bootloader
 *   chi can set mtvec tro dung vao vector table cua app roi nhay thang
 *   toi dia chi do (thuc thi dung lenh `j handle_reset` tai do).
 */

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/

#include "neon_bootloader_arch.h"
#include "mcu_device.h"
#include "riscv_csr.h"

/*==================================================================================================
*                                  STRUCTURES AND OTHER TYPEDEFS
==================================================================================================*/
typedef void (*app_entry_t)(void);

/*==================================================================================================
*                                        DEFINES AND MACROS
==================================================================================================*/
#define STK_CTLR (*(volatile uint32_t *)0xE000F000UL)
#define STK_SR (*(volatile uint32_t *)0xE000F004UL)
#define STK_CNTL (*(volatile uint32_t *)0xE000F008UL)
#define STK_CMPLR (*(volatile uint32_t *)0xE000F010UL)

#define STK_CTLR_STE (1U << 0)
#define STK_CTLR_STCLK (1U << 2)
#define STK_CTLR_STRE (1U << 3)

#define PFIC_SCTLR (*(volatile uint32_t *)0xE000ED10UL)
#define PFIC_SCTLR_SYSRESET (1U << 31)

/*==================================================================================================
*                                         GLOBAL FUNCTIONS
==================================================================================================*/

/** @copydoc neon_bootloader_arch_systick_start1ms */
void neon_bootloader_arch_systick_start1ms(void) {
    STK_CTLR = 0U; /* tat truoc khi cau hinh */
    STK_CNTL = 0U;
    STK_SR = 0U;
    STK_CMPLR = (SystemCoreClock / 1000U) - 1U; /* STCLK=1 (HCLK) lam time base */
    STK_CTLR = STK_CTLR_STE | STK_CTLR_STCLK | STK_CTLR_STRE;
}

/** @copydoc neon_bootloader_arch_systick1ms_elapsed */
int neon_bootloader_arch_systick1ms_elapsed(void) {
    if (STK_SR & 1U) {
        STK_SR = 0U; /* ghi 0 de clear CNTIF (RM 6.5.4.2) */
        return 1;
    }
    return 0;
}

/** @copydoc neon_bootloader_arch_jump_to_app */
void neon_bootloader_arch_jump_to_app(uint32_t vector_base) {
    uint32_t entry = vector_base | 3U;
    RISCV_CSR_WRITE(mtvec, entry);
    ((app_entry_t)vector_base)();

    for (;;) {
        /* khong bao gio toi day neu app_reset dung */
    }
}

/** @copydoc neon_bootloader_arch_system_reset */
void neon_bootloader_arch_system_reset(void) {
    PFIC_SCTLR = PFIC_SCTLR_SYSRESET;
    for (;;) {
        /* khong bao gio toi day - reset da xay ra */
    }
}
