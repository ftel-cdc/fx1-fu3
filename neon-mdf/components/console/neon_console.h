/**
 * @file neon_console.h
 * @brief Public declarations for console.
 * @details
 * Declares module interfaces.
 */

#ifndef __NEON_CONSOLE_H__
#define __NEON_CONSOLE_H__

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include "uart/neon_uart.h"

/*==================================================================================================
*                                        DEFINES AND MACROS
==================================================================================================*/
#define NEON_CONSOLE_DEFAULT_BAUD_RATE 19200U
#define NEON_CONSOLE_DEFAULT_TIMEOUT_POLLS 1000000U

#ifdef __cplusplus
extern "C" {
#endif

/*==================================================================================================
*                                       FUNCTION PROTOTYPES
==================================================================================================*/
/**
 * @brief Attach an initialized, caller-owned UART to the FU2 console.
 * @param[in,out] uart Initialized UART handle; ownership remains with the caller.
 * @param[in] timeout_polls Polling iteration budget per byte; this value is not milliseconds.
 * @return NEON_OK on success, or a module error/status code.
 * @details
 * FU2: CONFIG_NEON_MDF_DRIVER_UART enables printf/vprintf/puts/putchar and
 * _write together; include <stdio.h> to use them. No separate printf flag, heap,
 * buffering or newlib stdio. The first output automatically initializes UART0
 * at 19200 baud, 8N1, using SystemCoreClock. App code only needs <stdio.h>.
 * AttachUART is optional, for selecting another initialized UART/configuration.
 * The caller owns an initialized, 8-bit UART handle for the entire attachment.
 * timeout_polls is a finite per-byte poll budget, not milliseconds.
 * Concurrent/reentrant console calls fail with BUSY instead of waiting.
 * Do not use printf in an ISR or change/deinit the UART while attached.
 */
neon_status_t neon_console_attach_uart(neon_uart_handle_t *uart, uint32_t timeout_polls);
/**
 * @brief Detach the console UART and restore lazy default-UART initialization.
 * @return NEON_OK on success, or a module error/status code.
 */
neon_status_t neon_console_detach_uart(void);
/**
 * @brief Return the status recorded by the most recent console operation.
 * @return NEON_OK on success, or a module error/status code.
 */
neon_status_t neon_console_get_status(void);

/**
 * @brief Borrow the currently attached console UART without transferring ownership.
 * @return The requested value or operation result.
 * @details
 * Borrow the current console UART after printf or AttachUART succeeds.
 * Returns NULL before initialization or after DetachUART. Do not reconfigure
 * or deinit this handle while the console uses it.
 */
neon_uart_handle_t *neon_console_get_uart(void);

/**
 * @brief Write stdout or stderr bytes through the FU2 console UART.
 * @param[in] file Standard output or standard error descriptor.
 * @param[in,out] data data for this operation.
 * @param[in] length Number of elements to process.
 * @return Bytes consumed, a partial count on failure, or -1 before progress.
 * @details
 * stdout/stderr output hook. Initializes the default UART on first output;
 * DetachUART returns subsequent output to this default UART. Returns input
 * bytes consumed, a partial count on failure after progress, or -1. Failure
 * details are in GetStatus; this backend does not set newlib errno.
 * Direct _write calls must be serialized with other users of the console.
 */
int _write(int file, char *data, int length);

/* Supported: %% %c %s %d %i %u %o %x %X %p, flags -+ #0, width/precision
 * (literal or *), integer lengths hh/h/l/ll/z. Width/precision <= 1024.
 * Unsupported conversions (including float and %n) return -1, as do UART
 * failures; a prefix may already have been sent. GetStatus explains failure.
 * LF becomes CRLF unless already preceded by CR. Return counts logical
 * characters before this terminal newline translation. Successful FU2 output
 * is queued in UART; wait for TEMT before resetting or reconfiguring UART.
 * FU2 strings/buffers must own their complete aligned containing words.
 */

#ifdef __cplusplus
}
#endif
#endif /* __NEON_CONSOLE_H__ */
