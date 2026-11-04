/**
 * @file string.h
 * @brief Public declarations for string.
 * @details
 * * string.h provides the minimal shim required by FreeRTOS-Kernel core (list.c/tasks.c/
 * queue.c/timers.c/portable/MemMang/heap_4.c/stream_buffer.c/
 * portable/GCC/RISC-V/port.c) on RISC-V target - riscv64-elf-gcc toolchain (Homebrew) does not have
 * multilib matching ilp32e ABI (see cmake/toolchain/riscv-none-elf.cmake) so there is no real
 * sysroot/libc for this live kien -> no real <string.h>. FreeRTOS-Kernel (vendor,
 * rtos/FreeRTOS-Kernel/) uses memcpy()/memset()/strlen() without errors (heap_4.c for allocator,
 * tasks.c for TCB, queue.c for item copy...) - cannot "bop" with #if as code of SDK itself (doesn't
 * want to patch through multiple source vendors).
 *
 * include only this dan path for freertos_kernel/freertos_kernel_port when arch RISC-V (see
 * cmake/freertos.cmake) - not image-oriented FU2/WEACT_F412 (real newlib via arm-none-eabi-gcc)
 * please code other SDKs of CH32V003F4P6 (still use __builtin_memcpy() as before, see
 * components/ota/neon_ota.c).
 *
 * Real implementation: arch/RISCV/Core/Src/riscv_libc_shim.c (written don space, secure for each ISA -
 * uses no instructions beyond rv32ec, like
 * than riscv_soft_math.c).
 */

#ifndef NEON_MDF_RISCV_LIBC_SHIM_STRING_H
#define NEON_MDF_RISCV_LIBC_SHIM_STRING_H

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include <stddef.h> /* size_t - freestanding-safe, bundled with GCC (no sysroot required) */

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
