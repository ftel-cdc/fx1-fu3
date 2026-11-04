/**
 * @file neon_bootloader_uart.h
 * @brief Public declarations for bootloader UART.
 * @details
 * * neon_bootloader_uart.h - UART toi thieu dung rieng cho bootloader (KHONG
 * phai components/driver/uart/neon_uart.h - cai do hien la stub PLACEHOLDER
 * chi cho FU2, khong dung duoc tren WEACT_F412 that - xem
 * platform/weact_f412/neon_bootloader_uart.c cho implementation THAT).
 *
 * Tach rieng thanh target CMake `neon_mdf_bootloader_uart` (khong gop chung
 * voi neon_mdf_bootloader chua ham main()) de bat ky executable nao (khong
 * chi bootloader) cung link duoc an toan - vd app muon printf() qua cung
 * UART nay (xem Projects/Examples/Bootloader_WeAct/Src/app_syscalls.c).
 */

#ifndef __NEON_BOOTLOADER_UART_H__
#define __NEON_BOOTLOADER_UART_H__

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
 * @brief Initialize bootloader UART.
 */
void neon_bootloader_uart_init(void);
/**
 * @brief Bootloader UART send byte.
 * @param[in] b B.
 */
void neon_bootloader_uart_send_byte(uint8_t b);
/**
 * @brief Bootloader UART recv byte.
 * @return The requested value or operation result.
 */
uint8_t neon_bootloader_uart_recv_byte(void); /* block toi khi co byte */
/* Nhu tren nhung co timeout (so vong poll, khong phai ms) - tra ve 1 neu
 * nhan duoc (ghi vao *out), 0 neu het timeout ma chua co gi.
 *
 * KHONG DUNG CHO TIMEOUT THAT NUA (xem neon_bootloader_main.c - da doi sang
 * SysTick that vi dem vong lap KHONG dang tin cay: do that nhieu lan tren
 * phan cung cho ket qua khac nhau dang ke tuy co dang gan debugger SWD hay
 * khong, gay that bai lien tuc luc tu dong hoa). Giu lai ham nay chi vi
 * tinh tuong thich nguoc/don gian - dung neon_bootloader_uart_try_recv_byte()
 * cho code moi. */
/**
 * @brief Bootloader UART recv byte timeout.
 * @param[out] out Destination for the validated result.
 * @param[in] timeout_polls Polling iteration budget per byte; this value is not milliseconds.
 * @return The requested value or operation result.
 */
int neon_bootloader_uart_recv_byte_timeout(uint8_t *out, uint32_t timeout_polls);
/**
 * @brief Bootloader UART try recv byte.
 * @param[out] out Destination for the validated result.
 * @return The requested value or operation result.
 * @details
 * KHONG chan (non-blocking) - kiem tra 1 LAN duy nhat xem co byte san sang
 * chua, tra ve 1 (ghi vao *out) neu co, 0 neu chua co gi NGAY LUC GOI.
 * Dung ket hop voi 1 bo dem thoi gian THAT (vd SysTick) o noi goi de tu xay
 * dung timeout dang tin cay, thay vi dua vao so vong lap.
 */
int neon_bootloader_uart_try_recv_byte(uint8_t *out);

#ifdef __cplusplus
}
#endif

#endif /* __NEON_BOOTLOADER_UART_H__ */
