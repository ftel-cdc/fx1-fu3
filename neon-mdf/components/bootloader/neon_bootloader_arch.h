/**
 * @file neon_bootloader_arch.h
 * @brief Public declarations for bootloader arch.
 * @details
 * * neon_bootloader_arch.h - interface CHUNG cho phan DAC THU KIEN TRUC CPU
 * ma neon_bootloader_main.c can (SysTick-tuong-duong de dem timeout SYNC,
 * nhay vao app, reset he thong) - state machine OTA/rollback/verify trong
 * neon_bootloader_main.c la 100% dung chung moi kien truc, CHI 4 ham nay la
 * khac nhau that giua ARM/RISC-V/... - implementation THAT nam trong
 * neon_bootloader_arch_<family>.c (chon dung file theo FAMILY luc build,
 * xem components/bootloader/CMakeLists.txt) - CUNG PATTERN voi
 * gpio/neon_gpio.h + flash/neon_flash.h (1 header, N file .c theo family).
 */

#ifndef __NEON_BOOTLOADER_ARCH_H__
#define __NEON_BOOTLOADER_ARCH_H__

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*==================================================================================================
*                                       FUNCTION PROTOTYPES
==================================================================================================*/
/**
 * @brief Bootloader arch systick start1ms.
 * @details
 * Bat dong ho dem 1ms (mien la khong dung interrupt - bootloader chi
 * poll qua neon_bootloader_arch_systick1ms_elapsed()).
 */
void neon_bootloader_arch_systick_start1ms(void);

/**
 * @brief Bootloader arch systick1ms elapsed.
 * @return The requested value or operation result.
 * @details
 * Tra ve 1 dung 1 lan MOI khi du 1ms troi qua ke tu lan goi truoc (hoac
 * ke tu SystickStart1ms() cho lan dau) - poll lien tuc trong vong lap
 * cho SYNC (xem main() trong neon_bootloader_main.c).
 */
int neon_bootloader_arch_systick1ms_elapsed(void);

/**
 * @brief Bootloader arch jump to app.
 * @param[in] vector_base Vector base.
 * @details
 * Nhay vao app tai vector_base (KHONG BAO GIO quay lai neu app hop le) -
 * tu lo lieu VTOR/mtvec/SP tuong ung kien truc, xem tung file
 * neon_bootloader_arch_<family>.c cho chi tiet that.
 */
void neon_bootloader_arch_jump_to_app(uint32_t vector_base);

/**
 * @brief Bootloader arch system reset.
 * @details
 * Reset he thong (KHONG BAO GIO quay lai) - dung sau khi ghi otadata
 * pending, xem run_update_mode_flashmap().
 */
void neon_bootloader_arch_system_reset(void);

#ifdef __cplusplus
}
#endif

#endif /* __NEON_BOOTLOADER_ARCH_H__ */
