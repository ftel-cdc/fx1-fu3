/**
 * @file    neon_net_port.c
 * @brief   lwIP platform services on FX1: random numbers, errno, diagnostics, assertions.
 * @details The FX1 image links no C library. errno follows the running FreeRTOS task when the
 *          application sets configUSE_POSIX_ERRNO (the kernel swaps FreeRTOS_errno at each context
 *          switch); otherwise one shared value is used.
 */

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include "arch/cc.h"
#include "neon_net_port_internal.h"
#include "FreeRTOS.h"
#include "task.h"
#include "fx1_fw.h"
#include <stdarg.h>
#include <stddef.h>

/*==================================================================================================
*                                       DEFINES AND MACROS
==================================================================================================*/
#define PORT_ASSERT_FAIL_CODE 0xF400U

/*==================================================================================================
*                                        LOCAL VARIABLES
==================================================================================================*/
static uint32_t port_rand_state;
#if (configUSE_POSIX_ERRNO == 1)
/* Defined by tasks.c, swapped at every context switch; no kernel header declares it. */
extern int FreeRTOS_errno;
#else
static int port_errno;
#endif

/*==================================================================================================
*                                   PRIVATE FUNCTION PROTOTYPES
==================================================================================================*/
static void port_put_unsigned(unsigned long long value, uint32_t base, uint32_t upper, uint32_t width);

/*==================================================================================================
*                                         GLOBAL FUNCTIONS
==================================================================================================*/
/** @copydoc neon_net_port_seed */
void neon_net_port_seed(const uint8_t *bytes, uint32_t size_bytes) {
    uint32_t seed = port_rand_state ^ (uint32_t)fx1_mtime() ^ 0x9E3779B9UL;
    for (uint32_t i = 0U; i < size_bytes; ++i) {
        seed = (seed ^ bytes[i]) * 16777619UL;
    }
    port_rand_state = seed != 0U ? seed : 0x6D2B79F5UL;
}

/** @copydoc neon_net_port_rand */
uint32_t neon_net_port_rand(void) {
    taskENTER_CRITICAL();
    uint32_t x = port_rand_state;
    if (x == 0U) {
        x = (uint32_t)fx1_mtime() | 1U;
    }
    /* xorshift32 (Marsaglia 2003) */
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    port_rand_state = x;
    taskEXIT_CRITICAL();
    return x;
}

/** @copydoc neon_net_port_printf */
void neon_net_port_printf(const char *format, ...) {
    va_list args;
    va_start(args, format);
    for (const char *p = format; *p != '\0'; ++p) {
        if (*p != '%') {
            fx1_putc(*p);
            continue;
        }
        ++p;
        uint32_t width = 0U;
        while (*p >= '0' && *p <= '9') {
            width = width * 10U + (uint32_t)(*p - '0');
            ++p;
        }
        uint32_t longs = 0U;
        while (*p == 'l' || *p == 'h' || *p == 'z') {
            longs += (*p == 'l') ? 1U : 0U;
            ++p;
        }
        switch (*p) {
            case 'c':
                fx1_putc((char)va_arg(args, int));
                break;
            case 's': {
                const char *text = va_arg(args, const char *);
                fx1_puts(text != NULL ? text : "(null)");
                break;
            }
            case 'd':
            case 'i': {
                long long value = longs >= 2U ? va_arg(args, long long) : (long long)va_arg(args, long);
                if (value < 0) {
                    fx1_putc('-');
                    value = -value;
                }
                port_put_unsigned((unsigned long long)value, 10U, 0U, width);
                break;
            }
            case 'u':
            case 'x':
            case 'X': {
                unsigned long long value =
                    longs >= 2U ? va_arg(args, unsigned long long) : (unsigned long long)va_arg(args, unsigned long);
                port_put_unsigned(value, *p == 'u' ? 10U : 16U, *p == 'X' ? 1U : 0U, width);
                break;
            }
            case 'p':
                fx1_puts("0x");
                port_put_unsigned((uintptr_t)va_arg(args, void *), 16U, 0U, 8U);
                break;
            case '%':
                fx1_putc('%');
                break;
            case '\0':
                va_end(args);
                return;
            default:
                fx1_putc('%');
                fx1_putc(*p);
                break;
        }
    }
    va_end(args);
}

/** @copydoc neon_net_port_assert */
void neon_net_port_assert(const char *message, const char *file, int line) {
    fx1_puts("[NEON_NET] lwIP assert: ");
    fx1_puts(message != NULL ? message : "?");
    fx1_puts(" at ");
    fx1_puts(file != NULL ? file : "?");
    fx1_putc(':');
    fx1_putdec((uint32_t)line);
    fx1_putc('\n');
    fx1_fail(PORT_ASSERT_FAIL_CODE);
}

/**
 * @brief newlib <errno.h> contract: errno expands to (*__errno()).
 * @return Address of the running task's errno (configUSE_POSIX_ERRNO) or of a shared one.
 */
int *__errno(void) {
#if (configUSE_POSIX_ERRNO == 1)
    return &FreeRTOS_errno;
#else
    return &port_errno;
#endif
}

/**
 * @brief C library atoi() for lwIP's netif_find() (the FX1 image links no C library).
 * @param[in] text Optional sign followed by decimal digits; parsing stops at the first non-digit.
 * @return Parsed value; overflow is not detected (lwIP parses one-digit interface numbers).
 */
int atoi(const char *text) {
    int sign = 1;
    int value = 0;
    while (*text == ' ' || (*text >= '\t' && *text <= '\r')) {
        ++text;
    }
    if (*text == '-' || *text == '+') {
        sign = *text == '-' ? -1 : 1;
        ++text;
    }
    while (*text >= '0' && *text <= '9') {
        value = value * 10 + (*text - '0');
        ++text;
    }
    return sign * value;
}

/*==================================================================================================
*                                        PRIVATE FUNCTIONS
==================================================================================================*/
static void port_put_unsigned(unsigned long long value, uint32_t base, uint32_t upper, uint32_t width) {
    char digits[24];
    uint32_t count = 0U;
    const char *table = upper != 0U ? "0123456789ABCDEF" : "0123456789abcdef";
    do {
        digits[count++] = table[value % base];
        value /= base;
    } while (value != 0ULL && count < sizeof(digits));
    while (width > count) {
        fx1_putc('0');
        --width;
    }
    while (count > 0U) {
        fx1_putc(digits[--count]);
    }
}
