/**
 * @file system_CH32V003F4P6.h
 * @brief Public declarations for system ch32v003f4p6.
 * @details
 * * system_CH32V003F4P6.h - viet tay (khong sinh tu dong), mirror
 * system_FU2.h/system_WEACT_F412.h ben Cortex-M (cung API SystemCoreClock/
 * SystemInit/SystemCoreClockUpdate - startup_CH32V003F4P6.s (sinh tu dong)
 * goi `jal SystemInit` truoc khi vao main(), xem gen_startup.py).
 */

#ifndef SYSTEM_CH32V003F4P6_H
#define SYSTEM_CH32V003F4P6_H

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
 * Gọi từ handle_reset (xem startup_CH32V003F4P6.s) trước khi vào main().
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

#endif /* SYSTEM_CH32V003F4P6_H */
