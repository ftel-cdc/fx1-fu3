/**
 * @file string.h
 * @brief Public declarations for string.
 * @details
 * * string.h - shim TOI THIEU cho FreeRTOS-Kernel core (list.c/tasks.c/
 * queue.c/timers.c/portable/MemMang/heap_4.c/stream_buffer.c/
 * portable/GCC/RISC-V/port.c) tren target RISC-V - toolchain riscv64-elf-gcc
 * (Homebrew) KHONG co multilib khop ABI ilp32e (xem
 * cmake/toolchain/riscv-none-elf.cmake) nen KHONG co sysroot/libc that cho
 * kien truc nay -> KHONG co <string.h> that. FreeRTOS-Kernel (vendor,
 * rtos/FreeRTOS-Kernel/) dung memcpy()/memset()/strlen() KHONG DIEU KIEN o
 * vai noi cot loi (heap_4.c cho allocator, tasks.c cho TCB, queue.c cho
 * item copy...) - khong the "bop" bang #if nhu code cua CHINH SDK nay
 * (khong muon patch qua nhieu vao source vendor).
 *
 * CHI include duong dan nay cho freertos_kernel/freertos_kernel_port khi
 * arch RISC-V (xem cmake/freertos.cmake) - KHONG anh huong FU2/WEACT_F412
 * (co newlib that qua arm-none-eabi-gcc) hay code SDK khac cua CH32V003F4P6
 * (van dung __builtin_memcpy() nhu truoc, xem components/ota/neon_ota.c).
 *
 * Implementation THAT: arch/RISCV/Core/Src/riscv_libc_shim.c (tu viet don
 * gian, an toan cho MOI ISA - khong dung lenh nao ngoai rv32ec, giong tinh
 * than riscv_soft_math.c).
 */

#ifndef NEON_MDF_RISCV_LIBC_SHIM_STRING_H
#define NEON_MDF_RISCV_LIBC_SHIM_STRING_H

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include <stddef.h> /* size_t - freestanding-safe, bundled voi GCC (khong can sysroot) */

#ifdef __cplusplus
extern "C" {
#endif

/*==================================================================================================
*                                       FUNCTION PROTOTYPES
==================================================================================================*/
/**
 * @brief Copy bytes between nonoverlapping memory regions.
 * @param[in,out] dst Dst.
 * @param[in] src Src.
 * @param[in] n N.
 */
void *memcpy(void *dst, const void *src, size_t n);
/**
 * @brief Fill a memory region with a byte value.
 * @param[in,out] dst Dst.
 * @param[in] c C.
 * @param[in] n N.
 */
void *memset(void *dst, int c, size_t n);
/**
 * @brief Copy bytes between memory regions that may overlap.
 * @param[in,out] dst Dst.
 * @param[in] src Src.
 * @param[in] n N.
 */
void *memmove(void *dst, const void *src, size_t n);
/**
 * @brief Count characters before the terminating null byte.
 * @param[in] s S.
 * @return The requested value or operation result.
 */
size_t strlen(const char *s);

#ifdef __cplusplus
}
#endif

#endif /* NEON_MDF_RISCV_LIBC_SHIM_STRING_H */
