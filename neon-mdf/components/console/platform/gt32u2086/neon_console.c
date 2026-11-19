/**
 * @file platform/gt32u2086/neon_console.c
 * @brief Implementation for console.
 * @details
 * Small integer printf for GT32U2086's aligned-word SRAM bus.
 */

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/

#include "console/neon_console.h"
#include <limits.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

/*==================================================================================================
*                                        DEFINES AND MACROS
==================================================================================================*/
#define CONSOLE_FIELD_LIMIT 1024U
#define CONSOLE_FLAG_LEFT 1U
#define CONSOLE_FLAG_PLUS 2U
#define CONSOLE_FLAG_SPACE 4U
#define CONSOLE_FLAG_ALT 8U
#define CONSOLE_FLAG_ZERO 16U

/*==================================================================================================
*                                         LOCAL VARIABLES
==================================================================================================*/
static neon_uart_handle_t *console_uart;
static neon_uart_handle_t default_uart;
static uint32_t default_uart_dirty;
static uint32_t console_timeout;
static volatile uint32_t console_busy;
static volatile uint32_t write_busy;
static neon_status_t console_status = NEON_ERROR;
static uint32_t previous_char;

/*==================================================================================================
*                                   PRIVATE FUNCTION PROTOTYPES
==================================================================================================*/

static uint32_t console_enter_critical(void);
static void console_exit_critical(uint32_t saved);
static int console_acquire(void);
static int console_ensure_uart(void);
static int console_transmit_byte(uint32_t value);
static int console_emit(uint32_t value, int *count);
static int console_repeat(uint32_t value, uint32_t length, int *count);
static uint32_t console_read_char(const char *text);
static int console_read_field(const char **cursor, uint32_t *value);
static int console_print_text(const char *text, uint32_t length, uint32_t width, uint32_t flags, int *count);
static uint32_t console_divide_small(uint64_t *number, uint32_t base);
static int console_print_number(uint64_t number, uint32_t negative, uint32_t base, uint32_t upper, uint32_t flags,
                                uint32_t width, int precision, uint32_t pointer, int *count);
static int console_format(const char *cursor, va_list args);

/*==================================================================================================
*                                         GLOBAL FUNCTIONS
==================================================================================================*/

/** @copydoc neon_console_attach_uart */
neon_status_t neon_console_attach_uart(neon_uart_handle_t *uart, uint32_t timeout_polls) {
    if (uart == NULL || timeout_polls == 0U) {
        return NEON_INVALID_PARAM;
    }
    neon_uart_config_t config;
    neon_status_t config_status = neon_uart_get_config(uart, &config);
    if (config_status != NEON_OK) {
        return config_status;
    }
    if (config.word_length != NEON_UART_WORD_LENGTH_8B) {
        return NEON_NOT_SUPPORTED;
    }
    if (!console_acquire()) {
        return NEON_BUSY;
    }
    if (neon_uart_get_state(uart) != NEON_UART_STATE_READY) {
        console_status = NEON_BUSY;
    } else {
        console_uart = uart;
        console_timeout = timeout_polls;
        if (uart != &default_uart && neon_uart_get_instance(uart) == NEON_UART0) {
            default_uart_dirty = 1U;
        }
        previous_char = 0U;
    }
    console_busy = 0U;
    return console_status;
}

/** @copydoc neon_console_detach_uart */
neon_status_t neon_console_detach_uart(void) {
    if (!console_acquire()) {
        return NEON_BUSY;
    }
    console_uart = NULL;
    previous_char = 0U;
    console_busy = 0U;
    return NEON_OK;
}

/** @copydoc neon_console_get_status */
neon_status_t neon_console_get_status(void) {
    return console_status;
}

/** @copydoc neon_console_get_uart */
neon_uart_handle_t *neon_console_get_uart(void) {
    return console_uart;
}

/** @copydoc _write * @details
 * Newlib-style output hook, also used by our word-only printf formatter.
 * No newlib stdio or errno storage is linked by this bare-metal backend.
 */
int _write(int file, char *data, int length) {
    if ((file != 1 && file != 2) || length < 0 || (data == NULL && length != 0)) {
        console_status = NEON_INVALID_PARAM;
        return -1;
    }
    if (length == 0) {
        return 0;
    }
    uint32_t saved = console_enter_critical();
    if (write_busy != 0U) {
        console_status = NEON_BUSY;
        console_exit_critical(saved);
        return -1;
    }
    write_busy = 1U;
    console_exit_critical(saved);
    int written = 0;
    console_status = NEON_OK;
    if (console_ensure_uart()) {
        for (; written < length; ++written) {
            uint32_t value = 0U;
            memcpy(&value, data + written, 1U);
            if (value == '\n' && previous_char != '\r') {
                if (!console_transmit_byte('\r')) {
                    break;
                }
                previous_char = '\r';
            }
            if (!console_transmit_byte(value)) {
                break;
            }
            previous_char = value;
        }
    }
    write_busy = 0U;
    return written != 0 || console_status == NEON_OK ? written : -1;
}

/**
 * @brief Write a formatted GT32U2086 console message from a variable-argument list.
 * @param[in] text Text.
 * @param[in] args Args.
 * @return The requested value or operation result.
 */
int vprintf(const char *text, va_list args) {
    if (!console_acquire()) {
        return -1;
    }
    int result = -1;
    if (text == NULL) {
        console_status = NEON_INVALID_PARAM;
    } else {
        va_list copy;
        va_copy(copy, args);
        result = console_format(text, copy);
        va_end(copy);
    }
    console_busy = 0U;
    return result;
}

/**
 * @brief Format and write supported conversions through the GT32U2086 console.
 * @param[in] text Text.
 * @return The requested value or operation result.
 */
int printf(const char *text, ...) {
    va_list args;
    va_start(args, text);
    int result = vprintf(text, args);
    va_end(args);
    return result;
}

/**
 * @brief Write one character through the GT32U2086 console.
 * @param[in] character Character.
 * @return The requested value or operation result.
 */
int putchar(int character) {
    if (!console_acquire()) {
        return EOF;
    }
    int count = 0;
    int result = EOF;
    if (console_emit((uint32_t)character & 0xFFU, &count)) {
        result = (unsigned char)character;
    }
    console_busy = 0U;
    return result;
}

/**
 * @brief Write a string and a newline through the GT32U2086 console.
 * @param[in] text Text.
 * @return The requested value or operation result.
 */
int puts(const char *text) {
    static const char pattern[8] __attribute__((aligned(4))) = "%s\n";
    return printf(pattern, text);
}

/*==================================================================================================
*                                        PRIVATE FUNCTIONS
==================================================================================================*/

static uint32_t console_enter_critical(void) {
    uint32_t saved = __get_PRIMASK();
    __disable_irq();
    return saved;
}

static void console_exit_critical(uint32_t saved) {
    __set_PRIMASK(saved);
}

static int console_acquire(void) {
    uint32_t saved = console_enter_critical();
    if (console_busy != 0U || write_busy != 0U) {
        console_status = NEON_BUSY;
        console_exit_critical(saved);
        return 0;
    }
    console_busy = 1U;
    console_exit_critical(saved);
    console_status = NEON_OK;
    return 1;
}

static int console_ensure_uart(void) {
    if (console_uart != NULL) {
        return 1;
    }
    if (default_uart_dirty != 0U && neon_uart_get_state(&default_uart) == NEON_UART_STATE_READY) {
        console_status = neon_uart_deinit(&default_uart);
        if (console_status != NEON_OK) {
            return 0;
        }
    }
    /* Startup/SystemInit has already established SystemCoreClock. Reuse an
   * untouched default handle, or restore defaults after a custom UART0 setup. */
    if (neon_uart_get_state(&default_uart) != NEON_UART_STATE_READY) {
        neon_uart_config_t config;
        neon_uart_get_default_config(&config);
        config.baud_rate = NEON_CONSOLE_DEFAULT_BAUD_RATE;
        config.clock_frequency_hz = SystemCoreClock;
        console_status = neon_uart_init(&default_uart, NEON_UART0);
        if (console_status == NEON_OK) {
            console_status = neon_uart_set_config(&default_uart, &config);
        }
        if (console_status != NEON_OK) {
            return 0;
        }
    }
    console_uart = &default_uart;
    default_uart_dirty = 0U;
    console_timeout = NEON_CONSOLE_DEFAULT_TIMEOUT_POLLS;
    return 1;
}

static int console_transmit_byte(uint32_t value) {
    /* Own a complete aligned word; never put a single char on the stack. */
    uint32_t word = value & 0xFFU;
    console_status = neon_uart_transmit(console_uart, (const uint8_t *)&word, 1U, console_timeout);
    return console_status == NEON_OK;
}

static int console_emit(uint32_t value, int *count) {
    if (*count == INT_MAX) {
        console_status = NEON_ERROR;
        return 0;
    }
    uint32_t word = value & 0xFFU;
    if (_write(1, (char *)&word, 1) != 1) {
        return 0;
    }
    ++*count;
    return 1;
}

static int console_repeat(uint32_t value, uint32_t length, int *count) {
    while (length-- != 0U) {
        if (!console_emit(value, count)) {
            return 0;
        }
    }
    return 1;
}

static uint32_t console_read_char(const char *text) {
    uint32_t value = 0U;
    memcpy(&value, text, 1U);
    return value;
}

static int console_read_field(const char **cursor, uint32_t *value) {
    *value = 0U;
    while (console_read_char(*cursor) >= '0' && console_read_char(*cursor) <= '9') {
        *value = *value * 10U + console_read_char((*cursor)++) - '0';
        if (*value > CONSOLE_FIELD_LIMIT) {
            console_status = NEON_INVALID_PARAM;
            return 0;
        }
    }
    return 1;
}

static int console_print_text(const char *text, uint32_t length, uint32_t width, uint32_t flags, int *count) {
    uint32_t padding = width > length ? width - length : 0U;
    if ((flags & CONSOLE_FLAG_LEFT) == 0U && !console_repeat(' ', padding, count)) {
        return 0;
    }
    for (uint32_t i = 0U; i < length; ++i) {
        if (!console_emit(console_read_char(text + i), count)) {
            return 0;
        }
    }
    return (flags & CONSOLE_FLAG_LEFT) == 0U || console_repeat(' ', padding, count);
}

/* Avoid libgcc's 64-bit divide: its CLZ lookup can read bytes from Flash.
 * A word-only restoring division suffices for the bases used by printf. */
static uint32_t console_divide_small(uint64_t *number, uint32_t base) {
    uint32_t high = (uint32_t)(*number >> 32U), low = (uint32_t)*number;
    uint32_t quotient_high = 0U, quotient_low = 0U, remainder = 0U;
    for (uint32_t bit = 0U; bit < 64U; ++bit) {
        remainder = (remainder << 1U) | (high >> 31U);
        high = (high << 1U) | (low >> 31U);
        low <<= 1U;
        quotient_high = (quotient_high << 1U) | (quotient_low >> 31U);
        quotient_low <<= 1U;
        if (remainder >= base) {
            remainder -= base;
            quotient_low |= 1U;
        }
    }
    *number = ((uint64_t)quotient_high << 32U) | quotient_low;
    return remainder;
}

static int console_print_number(uint64_t number, uint32_t negative, uint32_t base, uint32_t upper, uint32_t flags,
                                uint32_t width, int precision, uint32_t pointer, int *count) {
    uint32_t digits[64];
    uint32_t length = 0U;
    uint32_t nonzero = number != 0U;
    if (number != 0U || precision != 0) {
        do {
            uint32_t digit = console_divide_small(&number, base);
            digits[length++] = digit < 10U ? '0' + digit : (upper != 0U ? 'A' : 'a') + digit - 10U;
        } while (number != 0U);
    }
    uint32_t sign = negative != 0U                       ? '-'
                    : (flags & CONSOLE_FLAG_PLUS) != 0U  ? '+'
                    : (flags & CONSOLE_FLAG_SPACE) != 0U ? ' '
                                                         : 0U;
    uint32_t prefix = pointer != 0U || (base == 16U && nonzero != 0U && (flags & CONSOLE_FLAG_ALT) != 0U);
    uint32_t zeros = precision > (int)length ? (uint32_t)precision - length : 0U;
    if (base == 8U && (flags & CONSOLE_FLAG_ALT) != 0U && zeros == 0U && (length == 0U || digits[length - 1U] != '0')) {
        zeros = 1U;
    }
    uint32_t total = length + zeros + (sign != 0U) + 2U * prefix;
    uint32_t padding = width > total ? width - total : 0U;
    uint32_t zero_pad = (flags & CONSOLE_FLAG_ZERO) != 0U && (flags & CONSOLE_FLAG_LEFT) == 0U && precision < 0;
    if ((flags & CONSOLE_FLAG_LEFT) == 0U && zero_pad == 0U && !console_repeat(' ', padding, count)) {
        return 0;
    }
    if (sign != 0U && !console_emit(sign, count)) {
        return 0;
    }
    if (prefix != 0U && (!console_emit('0', count) || !console_emit(upper != 0U ? 'X' : 'x', count))) {
        return 0;
    }
    if (!console_repeat('0', zeros + (zero_pad != 0U ? padding : 0U), count)) {
        return 0;
    }
    while (length != 0U) {
        if (!console_emit(digits[--length], count)) {
            return 0;
        }
    }
    return (flags & CONSOLE_FLAG_LEFT) == 0U || console_repeat(' ', padding, count);
}

static int console_format(const char *cursor, va_list args) {
    int count = 0;
    while (console_read_char(cursor) != 0U) {
        uint32_t ch = console_read_char(cursor++);
        if (ch != '%') {
            if (!console_emit(ch, &count)) {
                return -1;
            }
            continue;
        }
        if (console_read_char(cursor) == '%') {
            ++cursor;
            if (!console_emit('%', &count)) {
                return -1;
            }
            continue;
        }
        uint32_t flags = 0U, width = 0U, field = 0U, length = 0U;
        int precision = -1;
        for (;;) {
            ch = console_read_char(cursor);
            uint32_t flag = ch == '-'   ? CONSOLE_FLAG_LEFT
                            : ch == '+' ? CONSOLE_FLAG_PLUS
                            : ch == ' ' ? CONSOLE_FLAG_SPACE
                            : ch == '#' ? CONSOLE_FLAG_ALT
                            : ch == '0' ? CONSOLE_FLAG_ZERO
                                        : 0U;
            if (flag == 0U) {
                break;
            }
            flags |= flag;
            ++cursor;
        }
        if (console_read_char(cursor) == '*') {
            int value = va_arg(args, int);
            ++cursor;
            if (value < 0) {
                flags |= CONSOLE_FLAG_LEFT;
            }
            width = value < 0 ? 0U - (uint32_t)value : (uint32_t)value;
        } else if (!console_read_field(&cursor, &width)) {
            return -1;
        }
        if (console_read_char(cursor) == '.') {
            ++cursor;
            if (console_read_char(cursor) == '*') {
                precision = va_arg(args, int);
                ++cursor;
                if (precision < 0) {
                    precision = -1;
                }
            } else {
                if (!console_read_field(&cursor, &field)) {
                    return -1;
                }
                precision = (int)field;
            }
        }
        if (width > CONSOLE_FIELD_LIMIT || precision > (int)CONSOLE_FIELD_LIMIT) {
            console_status = NEON_INVALID_PARAM;
            return -1;
        }
        ch = console_read_char(cursor);
        if (ch == 'h') {
            length = 1U;
            ++cursor;
            if (console_read_char(cursor) == 'h') {
                length = 2U;
                ++cursor;
            }
        } else if (ch == 'l') {
            length = 3U;
            ++cursor;
            if (console_read_char(cursor) == 'l') {
                length = 4U;
                ++cursor;
            }
        } else if (ch == 'z') {
            length = 5U;
            ++cursor;
        }
        ch = console_read_char(cursor);
        if (ch == 0U) {
            console_status = NEON_NOT_SUPPORTED;
            return -1;
        }
        ++cursor;
        if (ch == 's' && length == 0U) {
            static const char null_text[8] __attribute__((aligned(4))) = "(null)";
            const char *text = va_arg(args, const char *);
            if (text == NULL) {
                text = null_text;
            }
            uint32_t size = 0U;
            while ((precision < 0 || size < (uint32_t)precision) && console_read_char(text + size) != 0U) {
                if (size == INT_MAX) {
                    console_status = NEON_ERROR;
                    return -1;
                }
                ++size;
            }
            if (!console_print_text(text, size, width, flags, &count)) {
                return -1;
            }
        } else if (ch == 'c' && length == 0U) {
            uint32_t value = (uint32_t)va_arg(args, int) & 0xFFU;
            uint32_t padding = width > 1U ? width - 1U : 0U;
            if (((flags & CONSOLE_FLAG_LEFT) == 0U && !console_repeat(' ', padding, &count)) ||
                !console_emit(value, &count) ||
                ((flags & CONSOLE_FLAG_LEFT) != 0U && !console_repeat(' ', padding, &count))) {
                return -1;
            }
        } else if (ch == 'd' || ch == 'i' || ch == 'u' || ch == 'o' || ch == 'x' || ch == 'X' ||
                   (ch == 'p' && length == 0U)) {
            uint64_t number;
            uint32_t negative = 0U;
            uint32_t base = ch == 'o' ? 8U : (ch == 'x' || ch == 'X' || ch == 'p') ? 16U : 10U;
            if (ch == 'd' || ch == 'i') {
                int64_t value;
                if (length == 4U) {
                    value = va_arg(args, long long);
                } else if (length == 3U) {
                    value = va_arg(args, long);
                } else if (length == 5U) {
                    value = va_arg(args, ptrdiff_t);
                } else {
                    int promoted = va_arg(args, int);
                    value = length == 2U ? (int8_t)promoted : length == 1U ? (int16_t)promoted : promoted;
                }
                negative = value < 0;
                number = negative != 0U ? 0ULL - (uint64_t)value : (uint64_t)value;
            } else {
                flags &= ~(CONSOLE_FLAG_PLUS | CONSOLE_FLAG_SPACE);
                if (ch == 'p') {
                    number = (uintptr_t)va_arg(args, void *);
                } else if (length == 4U) {
                    number = va_arg(args, unsigned long long);
                } else if (length == 3U) {
                    number = va_arg(args, unsigned long);
                } else if (length == 5U) {
                    number = va_arg(args, size_t);
                } else {
                    number = va_arg(args, unsigned int);
                    if (length == 1U) {
                        number &= 0xFFFFU;
                    }
                    if (length == 2U) {
                        number &= 0xFFU;
                    }
                }
            }
            if (!console_print_number(number, negative, base, ch == 'X', flags, width, precision, ch == 'p', &count)) {
                return -1;
            }
        } else {
            console_status = NEON_NOT_SUPPORTED;
            return -1;
        }
    }
    return count;
}
