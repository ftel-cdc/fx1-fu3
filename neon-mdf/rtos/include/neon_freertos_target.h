#ifndef NEON_FREERTOS_TARGET_H
#define NEON_FREERTOS_TARGET_H

/*
 * Capability FreeRTOS cua target hien tai.
 *
 * Cac gia tri Cortex-M duoc lay tu device header da sinh tu dong.
 * Nguon du lieu goc la:
 *
 *   mcu-description/devices/<target>.yaml
 *
 * Application khong nen tu kiem tra ten tung MCU de lay nhung thong tin
 * nhu so bit uu tien NVIC hoac su hien dien cua FPU.
 */

#if defined(NEON_MDF_FREERTOS_ARCH_CORTEX_M)

#include "mcu_device.h"

#ifndef __CORTEX_M
#error "Target Cortex-M phai dinh nghia __CORTEX_M"
#endif

#ifndef __NVIC_PRIO_BITS
#error "Target Cortex-M phai dinh nghia __NVIC_PRIO_BITS"
#endif

/*
 * So bit uu tien NVIC ma MCU thuc su trien khai.
 *
 * STM32F103C8T6 co 4 bit, tuong ung 16 muc uu tien tu 0 den 15.
 */
#define NEON_FREERTOS_NVIC_PRIO_BITS \
    (__NVIC_PRIO_BITS)

/*
 * Gia tri priority thap nhat theo cach danh so cua CMSIS.
 *
 * Priority co so cang nho thi muc uu tien phan cung cang cao.
 */
#define NEON_FREERTOS_LIBRARY_LOWEST_INTERRUPT_PRIORITY \
    ((1UL << NEON_FREERTOS_NVIC_PRIO_BITS) - 1UL)

/*
 * Chuyen priority dang CMSIS sang gia tri da dich bit de ghi vao
 * thanh ghi uu tien NVIC.
 *
 * Vi du STM32F103C8T6:
 *
 *   5 << (8 - 4) = 0x50
 */
#define NEON_FREERTOS_ENCODE_INTERRUPT_PRIORITY(priority) \
    ((priority) << (8UL - NEON_FREERTOS_NVIC_PRIO_BITS))

/*
 * Mot so core nhu Cortex-M4 co macro __FPU_PRESENT de cho biet
 * target co FPU hay khong.
 *
 * Cac core khong co kha nang tich hop FPU, vi du Cortex-M3,
 * co the khong dinh nghia macro nay. Khi do gia tri dung la 0.
 */
#if defined(__FPU_PRESENT)
#define NEON_FREERTOS_TARGET_HAS_FPU (__FPU_PRESENT)
#else
#define NEON_FREERTOS_TARGET_HAS_FPU 0U
#endif

/*
 * Port Cortex-M3 va Cortex-M4 co the dung co che chon task toi uu
 * dua tren lenh CLZ.
 *
 * Cortex-M0 dang dung port ARM_CM0 nen khong bat co che nay.
 */
#if (__CORTEX_M >= 3U)
#define NEON_FREERTOS_PORT_SUPPORTS_OPTIMISED_TASK_SELECTION 1
#else
#define NEON_FREERTOS_PORT_SUPPORTS_OPTIMISED_TASK_SELECTION 0
#endif

#elif defined(NEON_MDF_FREERTOS_ARCH_RISCV)

/*
 * Capability do port target cap: FX1_VP co FPU, CH32V003F4P6 khong co.
 * Mac dinh 0 de giu hanh vi cua cac target RISC-V cu.
 * Port RISC-V khong bat toi uu chon task nhu Cortex-M3/M4.
 */
#ifndef NEON_MDF_FREERTOS_HAS_FPU
#define NEON_MDF_FREERTOS_HAS_FPU 0U
#endif
#define NEON_FREERTOS_TARGET_HAS_FPU NEON_MDF_FREERTOS_HAS_FPU
#define NEON_FREERTOS_PORT_SUPPORTS_OPTIMISED_TASK_SELECTION 0

#else

#error "Kien truc FreeRTOS cua target chua duoc Neon MDF ho tro"

#endif

#endif /* NEON_FREERTOS_TARGET_H */