/**
 * @file platform/weact_f412/neon_bootloader_arch.c
 * @brief Implementation for bootloader arch.
 * @details
 * * platform/weact_f412/neon_bootloader_arch.c - implementation THAT cua
 * neon_bootloader_arch.h cho WEACT_F412 (ARMv7-M CO VTOR - xem
 * cpu.vtor_supported trong device YAML) - dung CMSIS SysTick/SCB chuan,
 * KHONG co gi rieng cua STM32F412 ca (SysTick/SCB la core peripheral cua
 * CHINH kien truc ARMv7-M, khong phai cua vendor) - dat ten theo FAMILY
 * (khop pattern platform/weact_f412/neon_gpio.c/platform/weact_f412/neon_flash.c) vi day la
 * family Cortex-M DUY NHAT co bootloader hien nay - LUU Y cho nguoi sau: 1
 * family Cortex-M KHAC co VTOR (M3/M4/M7 CO, M0/M0+ TUY CHON) van dung
 * duoc file nay NGUYEN VEN neu muon (chi can them entry vao
 * CMakeLists.txt), khong bat buoc viet lai.
 */

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/

#include "neon_bootloader_arch.h"
#include "mcu_device.h"

/*==================================================================================================
*                                  STRUCTURES AND OTHER TYPEDEFS
==================================================================================================*/
typedef void (*app_entry_t)(void);

/*==================================================================================================
*                                         GLOBAL FUNCTIONS
==================================================================================================*/

/** @copydoc neon_bootloader_arch_systick_start1ms * @details
 * SysTick CLKSOURCE=0 (mac dinh, HCLK/8) - khong can ngat, chi doc
 * COUNTFLAG (tu dong ve 0 sau moi lan doc, bao hieu 1 chu ky reload da het -
 * SysTick tu dong nap lai LOAD va dem tiep, dung dac tinh down-counter chuan
 * cua peripheral nay, khong can code tu reload tay).
 */
void neon_bootloader_arch_systick_start1ms(void) {
    SysTick->LOAD = (SystemCoreClock / 8U) / 1000U - 1U;
    SysTick->VAL = 0U;
    SysTick->CTRL = SysTick_CTRL_ENABLE_Msk;
}

/** @copydoc neon_bootloader_arch_systick1ms_elapsed */
int neon_bootloader_arch_systick1ms_elapsed(void) {
    return (SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk) != 0U;
}

/** @copydoc neon_bootloader_arch_jump_to_app */
void neon_bootloader_arch_jump_to_app(uint32_t vector_base) {
    uint32_t app_msp = *(volatile uint32_t *)(vector_base + 0U);
    uint32_t app_reset = *(volatile uint32_t *)(vector_base + 4U);

    /* SCB->VTOR that (Cortex-M co VTOR) - relocate de ngat/exception sau khi
   * nhay tro dung vao bang cua app. */
    SCB->VTOR = vector_base;

    __asm volatile("msr msp, %0" : : "r"(app_msp));
    ((app_entry_t)app_reset)();

    for (;;) {
        /* khong bao gio toi day neu app_reset dung */
    }
}

/** @copydoc neon_bootloader_arch_system_reset */
void neon_bootloader_arch_system_reset(void) {
    NVIC_SystemReset();
    for (;;) {
        /* khong bao gio toi day - NVIC_SystemReset() khong quay lai */
    }
}
