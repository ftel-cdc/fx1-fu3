/**
 * @file system_FU2.h
 * @brief Public declarations for system fu2.
 * @details
 * * system_FU2.h - CMSIS system file, viết tay (không sinh tự động).
 * Khai báo chuẩn CMSIS: mọi CMSIS device header đều #include file này và
 * cần biến SystemCoreClock + 2 hàm bên dưới tồn tại.
 */

#ifndef SYSTEM_FU2_H
#define SYSTEM_FU2_H

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Tần số CPU hiện tại (Hz) - phải gọi SystemCoreClockUpdate() sau khi đổi clock. */
extern uint32_t SystemCoreClock;

/*==================================================================================================
*                                       FUNCTION PROTOTYPES
==================================================================================================*/
/**
 * @brief Initialize the device clocks and architecture state before entering main.
 * @details
 * Gọi từ Reset_Handler (xem startup_FU2.s) trước khi vào main().
 */
extern void SystemInit(void);

/**
 * @brief Update SystemCoreClock from the active device clock configuration.
 * @details
 * Đọc lại cấu hình clock hiện tại và cập nhật SystemCoreClock.
 */
extern void SystemCoreClockUpdate(void);

#ifdef __cplusplus
}
#endif

#endif /* SYSTEM_FU2_H */
