/**
 * @file gt32u2086_runtime.c
 * @brief Implementation for gt32u2086 runtime.
 * @details
 * Standard memory/string API implementations for GT32U2086's word-only SRAM bus.
 * Apps, drivers and RTOS include <string.h>; only this file uses neon_utils_memory.h.
 * Buffer allocations must include the aligned words containing their bytes.
 * The stock newlib implementations contain narrow bus transactions.
 */

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/

#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "neon_utils_memory.h"

/*==================================================================================================
*                                         GLOBAL FUNCTIONS
==================================================================================================*/

/** @copydoc memset */
void *memset(void *destination, int value, size_t count) {
    uint8_t *bytes = destination;
    for (size_t index = 0; index < count; ++index) {
        neon_utils_memory_write8(bytes + index, (uint32_t)value);
    }
    return destination;
}

/** @copydoc memcpy */
void *memcpy(void *destination, const void *source, size_t count) {
    uint8_t *out = destination;
    const uint8_t *in = source;
    for (size_t index = 0; index < count; ++index) {
        neon_utils_memory_write8(out + index, neon_utils_memory_read8(in + index));
    }
    return destination;
}

/** @copydoc memmove */
void *memmove(void *destination, const void *source, size_t count) {
    uint8_t *out = destination;
    const uint8_t *in = source;
    if ((uintptr_t)out <= (uintptr_t)in) {
        return memcpy(destination, source, count);
    }
    while (count != 0U) {
        --count;
        neon_utils_memory_write8(out + count, neon_utils_memory_read8(in + count));
    }
    return destination;
}

/**
 * @brief Compare two memory regions byte by byte.
 * @param[in] left Left.
 * @param[in] right Right.
 * @param[in] count Number of elements to process.
 * @return The requested value or operation result.
 */
int memcmp(const void *left, const void *right, size_t count) {
    const uint8_t *a = left, *b = right;
    for (size_t index = 0; index < count; ++index) {
        int difference = (int)neon_utils_memory_read8(a + index) - (int)neon_utils_memory_read8(b + index);
        if (difference != 0) {
            return difference;
        }
    }
    return 0;
}

/**
 * @brief Find the first matching byte in a memory region.
 * @param[in] memory Memory.
 * @param[in] character Character.
 * @param[in] count Number of elements to process.
 */
void *memchr(const void *memory, int character, size_t count) {
    const uint8_t *bytes = memory;
    uint32_t wanted = (uint32_t)character & 0xffU;
    for (size_t index = 0; index < count; ++index) {
        if (neon_utils_memory_read8(bytes + index) == wanted) {
            return (void *)(bytes + index);
        }
    }
    return NULL;
}

/** @copydoc strlen */
size_t strlen(const char *string) {
    size_t length = 0;
    while (neon_utils_memory_read8(string + length) != 0U) {
        ++length;
    }
    return length;
}

/**
 * @brief Compare two null-terminated strings.
 * @param[in] left Left.
 * @param[in] right Right.
 * @return The requested value or operation result.
 */
int strcmp(const char *left, const char *right) {
    for (;;) {
        uint32_t a = neon_utils_memory_read8(left++);
        uint32_t b = neon_utils_memory_read8(right++);
        if (a != b || a == 0U) {
            return (int)a - (int)b;
        }
    }
}

/**
 * @brief Compare at most the requested number of string characters.
 * @param[in] left Left.
 * @param[in] right Right.
 * @param[in] count Number of elements to process.
 * @return The requested value or operation result.
 */
int strncmp(const char *left, const char *right, size_t count) {
    for (size_t index = 0; index < count; ++index) {
        uint32_t a = neon_utils_memory_read8(left + index);
        uint32_t b = neon_utils_memory_read8(right + index);
        if (a != b || a == 0U) {
            return (int)a - (int)b;
        }
    }
    return 0;
}

/**
 * @brief Copy a null-terminated string to the destination.
 * @param[in,out] destination Destination buffer with sufficient space for the result.
 * @param[in] source Source buffer.
 * @return The requested value or operation result.
 */
char *strcpy(char *destination, const char *source) {
    char *out = destination;
    uint32_t value;
    do {
        value = neon_utils_memory_read8(source++);
        neon_utils_memory_write8(out++, value);
    } while (value != 0U);
    return destination;
}

/**
 * @brief Copy or pad the requested number of characters into the destination.
 * @param[in,out] destination Destination buffer with sufficient space for the result.
 * @param[in] source Source buffer.
 * @param[in] count Number of elements to process.
 * @return The requested value or operation result.
 */
char *strncpy(char *destination, const char *source, size_t count) {
    size_t index = 0;
    for (; index < count; ++index) {
        uint32_t value = neon_utils_memory_read8(source + index);
        if (value == 0U) {
            break;
        }
        neon_utils_memory_write8(destination + index, value);
    }
    /* The C API pads to count bytes; a truncated copy has no added terminator. */
    for (; index < count; ++index) {
        neon_utils_memory_write8(destination + index, 0U);
    }
    return destination;
}

/**
 * @brief Append a null-terminated string to the destination.
 * @param[in,out] destination Destination buffer with sufficient space for the result.
 * @param[in] source Source buffer.
 * @return The requested value or operation result.
 */
char *strcat(char *destination, const char *source) {
    strcpy(destination + strlen(destination), source);
    return destination;
}

/**
 * @brief Append at most the requested number of characters to a string.
 * @param[in,out] destination Destination buffer with sufficient space for the result.
 * @param[in] source Source buffer.
 * @param[in] count Number of elements to process.
 * @return The requested value or operation result.
 */
char *strncat(char *destination, const char *source, size_t count) {
    char *out = destination + strlen(destination);
    size_t index = 0;
    for (; index < count; ++index) {
        uint32_t value = neon_utils_memory_read8(source + index);
        if (value == 0U) {
            break;
        }
        neon_utils_memory_write8(out + index, value);
    }
    neon_utils_memory_write8(out + index, 0U);
    return destination;
}

/**
 * @brief Find the first matching character in a string.
 * @param[in] string String.
 * @param[in] character Character.
 * @return The requested value or operation result.
 */
char *strchr(const char *string, int character) {
    uint32_t wanted = (uint32_t)character & 0xffU;
    for (;;) {
        uint32_t value = neon_utils_memory_read8(string);
        if (value == wanted) {
            return (char *)string;
        }
        if (value == 0U) {
            return NULL;
        }
        ++string;
    }
}

/**
 * @brief Find the last matching character in a string.
 * @param[in] string String.
 * @param[in] character Character.
 * @return The requested value or operation result.
 */
char *strrchr(const char *string, int character) {
    uint32_t wanted = (uint32_t)character & 0xffU;
    char *last = NULL;
    uint32_t value;
    do {
        value = neon_utils_memory_read8(string);
        if (value == wanted) {
            last = (char *)string;
        }
        ++string;
    } while (value != 0U);
    return last;
}

/**
 * @brief Find the first occurrence of a substring.
 * @param[in] string String.
 * @param[in] substring Substring.
 * @return The requested value or operation result.
 */
char *strstr(const char *string, const char *substring) {
    if (neon_utils_memory_read8(substring) == 0U) {
        return (char *)string;
    }
    while (neon_utils_memory_read8(string) != 0U) {
        const char *candidate = string;
        const char *pattern = substring;
        for (;;) {
            uint32_t wanted = neon_utils_memory_read8(pattern);
            if (wanted == 0U) {
                return (char *)string;
            }
            if (neon_utils_memory_read8(candidate) != wanted) {
                break;
            }
            ++candidate;
            ++pattern;
        }
        ++string;
    }
    return NULL;
}

/**
 * @brief Count the leading characters belonging to the accepted character set.
 * @param[in] string String.
 * @param[in] accept Accept.
 * @return The requested value or operation result.
 */
size_t strspn(const char *string, const char *accept) {
    size_t count = 0;
    for (;;) {
        uint32_t value = neon_utils_memory_read8(string + count);
        if (value == 0U || strchr(accept, (int)value) == NULL) {
            return count;
        }
        ++count;
    }
}

/**
 * @brief Count the leading characters outside the rejected character set.
 * @param[in] string String.
 * @param[in] reject Reject.
 * @return The requested value or operation result.
 */
size_t strcspn(const char *string, const char *reject) {
    size_t count = 0;
    for (;;) {
        uint32_t value = neon_utils_memory_read8(string + count);
        if (value == 0U || strchr(reject, (int)value) != NULL) {
            return count;
        }
        ++count;
    }
}

/**
 * @brief Find the first string character belonging to the accepted set.
 * @param[in] string String.
 * @param[in] accept Accept.
 * @return The requested value or operation result.
 */
char *strpbrk(const char *string, const char *accept) {
    uint32_t value;
    while ((value = neon_utils_memory_read8(string)) != 0U) {
        if (strchr(accept, (int)value) != NULL) {
            return (char *)string;
        }
        ++string;
    }
    return NULL;
}

/**
 * @brief   clzsi2.
 * @param[in] value Value.
 * @return The requested value or operation result.
 * @details
 * The Cortex-M0 libgcc implementation reads a byte lookup table.
 */
int __clzsi2(uint32_t value) {
    int count = 0;
    if (value == 0U) {
        return 32;
    }
    while ((value & 0x80000000UL) == 0U) {
        value <<= 1U;
        ++count;
    }
    return count;
}
