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
*                                  STRUCTURES AND OTHER TYPEDEFS
==================================================================================================*/
/* Formatter output: the console, or a bounded buffer (which may be NULL with size 0). */
typedef struct {
    int console;
    char *buffer;
    size_t size;
    size_t length;
} port_sink_t;

typedef struct {
    uint32_t width;
    int precision;
    int left;
    int zero;
    int upper;
} port_spec_t;

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
static void port_emit(port_sink_t *sink, char c);
static void port_emit_padded(port_sink_t *sink, const char *text, uint32_t length, const port_spec_t *spec);
static void port_emit_number(port_sink_t *sink, unsigned long long value, int negative, uint32_t base,
                             const port_spec_t *spec);
static void port_format(port_sink_t *sink, const char *format, va_list args);

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
    port_sink_t sink = {.console = 1, .buffer = NULL, .size = 0U, .length = 0U};
    va_list args;
    va_start(args, format);
    port_format(&sink, format, args);
    va_end(args);
}

/** @copydoc neon_net_port_vsnprintf */
int neon_net_port_vsnprintf(char *buffer, size_t size, const char *format, va_list args) {
    port_sink_t sink = {.console = 0, .buffer = buffer, .size = size, .length = 0U};
    if (buffer == NULL && size != 0U) {
        return -1;
    }
    port_format(&sink, format, args);
    if (size != 0U) {
        buffer[sink.length < size ? sink.length : size - 1U] = '\0';
    }
    return (int)sink.length;
}

/** @copydoc neon_net_port_snprintf */
int neon_net_port_snprintf(char *buffer, size_t size, const char *format, ...) {
    va_list args;
    va_start(args, format);
    int length = neon_net_port_vsnprintf(buffer, size, format, args);
    va_end(args);
    return length;
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
static void port_emit(port_sink_t *sink, char c) {
    if (sink->console != 0) {
        fx1_putc(c);
    } else if (sink->length + 1U < sink->size) {
        sink->buffer[sink->length] = c;
    }
    ++sink->length;
}

static void port_emit_padded(port_sink_t *sink, const char *text, uint32_t length, const port_spec_t *spec) {
    uint32_t pad = spec->width > length ? spec->width - length : 0U;
    if (!spec->left) {
        for (; pad > 0U; --pad) {
            port_emit(sink, ' ');
        }
    }
    for (uint32_t i = 0U; i < length; ++i) {
        port_emit(sink, text[i]);
    }
    for (; pad > 0U; --pad) {
        port_emit(sink, ' ');
    }
}

static void port_emit_number(port_sink_t *sink, unsigned long long value, int negative, uint32_t base,
                             const port_spec_t *spec) {
    char digits[24];
    uint32_t count = 0U;
    const char *table = spec->upper ? "0123456789ABCDEF" : "0123456789abcdef";
    do {
        digits[count++] = table[value % base];
        value /= base;
    } while (value != 0ULL && count < sizeof(digits));
    uint32_t length = count + (negative ? 1U : 0U);
    uint32_t pad = spec->width > length ? spec->width - length : 0U;
    if (!spec->left && !spec->zero) {
        for (; pad > 0U; --pad) {
            port_emit(sink, ' ');
        }
    }
    if (negative) {
        port_emit(sink, '-');
    }
    if (!spec->left && spec->zero) {
        for (; pad > 0U; --pad) {
            port_emit(sink, '0');
        }
    }
    while (count > 0U) {
        port_emit(sink, digits[--count]);
    }
    for (; pad > 0U; --pad) {
        port_emit(sink, ' ');
    }
}

/* Subset of C99 printf used by lwIP diagnostics and mbedTLS: flags '-' '0', width and precision
 * (digits or '*'), length hh/h/l/ll/z, conversions c s d i u x X p %. */
static void port_format(port_sink_t *sink, const char *format, va_list args) {
    for (const char *p = format; *p != '\0'; ++p) {
        if (*p != '%') {
            port_emit(sink, *p);
            continue;
        }
        port_spec_t spec = {0};
        spec.precision = -1;
        for (++p; *p == '-' || *p == '0'; ++p) {
            spec.left |= (*p == '-');
            spec.zero |= (*p == '0');
        }
        if (*p == '*') {
            int width = va_arg(args, int);
            spec.left |= width < 0;
            spec.width = (uint32_t)(width < 0 ? -width : width);
            ++p;
        }
        for (; *p >= '0' && *p <= '9'; ++p) {
            spec.width = spec.width * 10U + (uint32_t)(*p - '0');
        }
        if (*p == '.') {
            spec.precision = 0;
            if (*++p == '*') {
                spec.precision = va_arg(args, int);
                ++p;
            }
            for (; *p >= '0' && *p <= '9'; ++p) {
                spec.precision = spec.precision * 10 + (*p - '0');
            }
        }
        /* Arguments are read with their promoted type: int, long, long long or size_t. */
        uint32_t longs = 0U;
        int size_type = 0;
        for (; *p == 'l' || *p == 'h' || *p == 'z'; ++p) {
            longs += (*p == 'l') ? 1U : 0U;
            size_type |= (*p == 'z');
        }
        switch (*p) {
            case 'c': {
                char c = (char)va_arg(args, int);
                port_emit_padded(sink, &c, 1U, &spec);
                break;
            }
            case 's': {
                const char *text = va_arg(args, const char *);
                if (text == NULL) {
                    text = "(null)";
                }
                uint32_t length = 0U;
                while (text[length] != '\0' && (spec.precision < 0 || length < (uint32_t)spec.precision)) {
                    ++length;
                }
                port_emit_padded(sink, text, length, &spec);
                break;
            }
            case 'd':
            case 'i': {
                long long value;
                if (longs >= 2U) {
                    value = va_arg(args, long long);
                } else if (longs == 1U) {
                    value = va_arg(args, long);
                } else if (size_type != 0) {
                    value = (long long)va_arg(args, size_t);
                } else {
                    value = va_arg(args, int);
                }
                unsigned long long magnitude = value < 0 ? 0ULL - (unsigned long long)value : (unsigned long long)value;
                port_emit_number(sink, magnitude, value < 0, 10U, &spec);
                break;
            }
            case 'u':
            case 'x':
            case 'X': {
                unsigned long long value;
                if (longs >= 2U) {
                    value = va_arg(args, unsigned long long);
                } else if (longs == 1U) {
                    value = va_arg(args, unsigned long);
                } else if (size_type != 0) {
                    value = va_arg(args, size_t);
                } else {
                    value = va_arg(args, unsigned int);
                }
                spec.upper = *p == 'X';
                port_emit_number(sink, value, 0, *p == 'u' ? 10U : 16U, &spec);
                break;
            }
            case 'p':
                port_emit(sink, '0');
                port_emit(sink, 'x');
                spec.zero = 1;
                spec.width = 8U;
                port_emit_number(sink, (uintptr_t)va_arg(args, void *), 0, 16U, &spec);
                break;
            case '%':
                port_emit(sink, '%');
                break;
            case '\0':
                return;
            default:
                port_emit(sink, '%');
                port_emit(sink, *p);
                break;
        }
    }
}
