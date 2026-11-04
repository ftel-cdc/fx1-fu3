/**
 * @file system_WEACT_F412.c
 * @brief Implementation for system weact f412.
 * @details
 * * system_WEACT_F412.c - CMSIS system file, viết tay (không sinh tự động).
 * STM32F4 mặc định chạy HSI nội bộ 16MHz sau reset (chưa cấu hình PLL) -
 * đủ cho ví dụ GPIO blink hiện tại (xem mcu-description/devices/
 * WEACT_F412.yaml - phạm vi CHỈ GPIOB, chưa cấu hình clock tree qua RCC).
 *
 * QUAN TRỌNG (KHÁC FU2/NUCLEO_L031K6 - đây là chip Cortex-M4F ĐẦU TIÊN
 * trong SDK): PHẢI bật quyền truy cập FPU (CP10/CP11 full access) qua
 * SCB->CPACR TRƯỚC KHI bất kỳ lệnh floating-point nào chạy - build dùng cờ
 * -mfloat-abi=hard (xem cmake/arch/cortex-m.cmake) nên compiler CÓ THỂ sinh
 * lệnh FPU bất cứ lúc nào (kể cả code không tự viết float, vd một số phép
 * toán integer trên M4 vẫn có thể qua nhánh code chung). Thiếu bước này sẽ
 * UsageFault ngay lệnh FPU đầu tiên - đây là bước chuẩn của CMSIS
 * system_stm32f4xx.c thật, không phải tự sáng tác.
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
    /* Set CP10 va CP11 Full Access (theo dung system_stm32f4xx.c that cua ST) */
    SCB->CPACR |= ((3UL << 10 * 2) | (3UL << 11 * 2));
#endif

    /* Tu relocate SCB->VTOR ve dung dia chi vector table CUA CHINH EXECUTABLE
   * NAY (g_pfnVectors la symbol linker dat, dia chi resolve khac nhau tuy
   * linker script dang dung - xem gen_startup_vector.py) - dong nay dung y
   * het cho ca 3 truong hop build: whole-flash (Blinky, resolve ve dau
   * flash, no-op vi trung mac dinh HW), bootloader (resolve ve dau
   * bootloader, no-op), va app chay duoi bootloader (resolve ve
   * NEON_MDF_APP_VECTOR_BASE - day la truong hop CAN dong nay, vi bootloader
   * nhay vao app KHONG tu dong sua VTOR ho, va ca truong hop nap app truc
   * tiep qua SWD/debug KHONG qua bootloader cung phai tu dung duoc). PHAI
   * dat SAU khi bat FPU o tren (thu tu giong system_stm32f4xx.c that cua
   * ST), TRUOC bat ky ngat nao co the xay ra. */
    SCB->VTOR = (uint32_t)g_pfnVectors;

    SystemCoreClockUpdate();
}

/** @copydoc SystemCoreClockUpdate */
void SystemCoreClockUpdate(void) {
    /* PLACEHOLDER: chưa đọc RCC->CFGR để tính SystemCoreClock thật nếu app
   * sau này tự cấu hình PLL/HSE - hiện luôn giả định HSI 16MHz mặc định vì
   * ví dụ GPIO blink không đổi clock tree. */
    SystemCoreClock = 16000000UL;
}
