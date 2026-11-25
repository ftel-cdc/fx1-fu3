/**
 * @file    cc.h
 * @brief   lwIP compiler/platform glue for the Neon network manager (GCC, RV32, newlib headers).
 * @details The FX1 image links no C library (fx1_libc.c supplies memset/memcpy/memmove/memcmp/strlen/
 *          strcmp only), so lwIP is configured to need nothing else: its own ctype macros, errno and
 *          random number from neon_net_port.c, diagnostics through the board console.
 */
#ifndef __NEON_NET_ARCH_CC_H__
#define __NEON_NET_ARCH_CC_H__

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/time.h>

/*==================================================================================================
*                                       DEFINES AND MACROS
==================================================================================================*/
/* newlib's <sys/time.h> already defines struct timeval. */
#define LWIP_TIMEVAL_PRIVATE 0
/* lwIP's own isdigit/isxdigit/...: newlib's need the _ctype_ table of libc. */
#define LWIP_NO_CTYPE_H 1
/* errno from <errno.h>; neon_net_port.c defines __errno() per FreeRTOS task. */
#define LWIP_ERRNO_STDINCLUDE 1

/* contrib/ports/freertos/sys_arch.c still uses the pre-V8 FreeRTOS name; the applications build
 * with configENABLE_BACKWARD_COMPATIBILITY 0. */
#ifndef portTICK_RATE_MS
#define portTICK_RATE_MS portTICK_PERIOD_MS
#endif

#define LWIP_RAND() neon_net_port_rand()
#define LWIP_PLATFORM_DIAG(message)                                                                                    \
    do {                                                                                                               \
        neon_net_port_printf message;                                                                                  \
    } while (0)
#define LWIP_PLATFORM_ASSERT(message) neon_net_port_assert((message), __FILE__, __LINE__)

/*==================================================================================================
*                                       FUNCTION PROTOTYPES
==================================================================================================*/
/**
 * @brief Pseudo-random 32-bit value for lwIP (DNS IDs, ports, TCP ISN, DHCP xid).
 * @return Next value of a xorshift generator seeded from the MAC address and mtime.
 * @warning Not cryptographic; replace once FX1 has a hardware entropy source.
 */
uint32_t neon_net_port_rand(void);
/**
 * @brief printf to the board console for lwIP diagnostics (subset, see neon_net_port_vsnprintf).
 * @param[in] format Format string.
 */
void neon_net_port_printf(const char *format, ...) __attribute__((format(printf, 1, 2)));
/**
 * @brief Bounded formatter, C99 vsnprintf semantics, for lwIP and mbedTLS (no C library on FX1).
 * @param[out] buffer Output, always terminated when size > 0.
 * @param[in] size Buffer size in bytes.
 * @param[in] format Flags '-' '0', width/precision (digits or '*'), length hh/h/l/ll/z,
 *            conversions c s d i u x X p %.
 * @param[in] args Arguments.
 * @return Length the full output would have, excluding the terminator; -1 on bad arguments.
 */
int neon_net_port_vsnprintf(char *buffer, size_t size, const char *format, va_list args);
/**
 * @brief Bounded formatter, C99 snprintf semantics (see neon_net_port_vsnprintf).
 * @param[out] buffer Output.
 * @param[in] size Buffer size in bytes.
 * @param[in] format Format string.
 * @return Length the full output would have, excluding the terminator.
 */
int neon_net_port_snprintf(char *buffer, size_t size, const char *format, ...) __attribute__((format(printf, 3, 4)));
/**
 * @brief Report a failed lwIP assertion and stop.
 * @param[in] message Assertion text.
 * @param[in] file Source file.
 * @param[in] line Source line.
 */
void neon_net_port_assert(const char *message, const char *file, int line) __attribute__((noreturn));

#endif /* __NEON_NET_ARCH_CC_H__ */
