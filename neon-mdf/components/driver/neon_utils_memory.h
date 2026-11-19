/**
 * @file neon_utils_memory.h
 * @brief Internal backend interface for memory.
 * @details
 * Internal backend of gt32u2086_runtime.c; drivers, apps and RTOS use <string.h> APIs.
 * Normal-memory buffer access using aligned 32-bit bus transactions only.
 * Buffers must own the complete aligned words containing their bytes (pad
 * both ends). Do not use on MMIO or concurrently modified DMA storage.
 * @internal
 */

#ifndef __NEON_UTILS_MEMORY_H__
#define __NEON_UTILS_MEMORY_H__

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include <stdint.h>
#if defined(__arm__)
#include "mcu_device.h"
#endif

/*==================================================================================================
*                                  STRUCTURES AND OTHER TYPEDEFS
==================================================================================================*/
typedef uint32_t neon_utils_memory_word_t __attribute__((__may_alias__));

static inline uint32_t neon_utils_memory_read8(const void *address) {
    uintptr_t lane = (uintptr_t)address;
    const volatile neon_utils_memory_word_t *word = (const volatile neon_utils_memory_word_t *)(lane & ~(uintptr_t)3U);
    return (*word >> ((lane & 3U) * 8U)) & 0xffU;
}

static inline void neon_utils_memory_write8(void *address, uint32_t value) {
    uintptr_t lane = (uintptr_t)address;
    uint32_t shift = (uint32_t)(lane & 3U) * 8U;
    volatile neon_utils_memory_word_t *word = (volatile neon_utils_memory_word_t *)(lane & ~(uintptr_t)3U);
#if defined(__arm__)
    uint32_t primask = __get_PRIMASK();
    __disable_irq();
#endif
    *word = (*word & ~(0xffU << shift)) | ((value & 0xffU) << shift);
#if defined(__arm__)
    __set_PRIMASK(primask);
#endif
}

static inline uint32_t neon_utils_memory_read16(const void *address) {
    const uint8_t *bytes = (const uint8_t *)address;
    return neon_utils_memory_read8(bytes) | (neon_utils_memory_read8(bytes + 1) << 8U);
}

static inline void neon_utils_memory_write16(void *address, uint32_t value) {
    uint8_t *bytes = (uint8_t *)address;
    neon_utils_memory_write8(bytes, value);
    neon_utils_memory_write8(bytes + 1, value >> 8U);
}

static inline uint32_t neon_utils_memory_read32(const void *address) {
    uintptr_t lane = (uintptr_t)address;
    const volatile neon_utils_memory_word_t *word = (const volatile neon_utils_memory_word_t *)(lane & ~(uintptr_t)3U);
    uint32_t low = word[0];
    uint32_t shift = (uint32_t)(lane & 3U) * 8U;
    return (shift == 0U) ? low : ((low >> shift) | (word[1] << (32U - shift)));
}
#endif /* __NEON_UTILS_MEMORY_H__ */
