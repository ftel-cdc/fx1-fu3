/**
 * @file    neon_tls_port.h
 * @brief   Platform hooks of the Neon TLS component (memory, entropy).
 * @details A board with a hardware random number generator overrides neon_tls_port_entropy().
 */
#ifndef __NEON_TLS_PORT_H__
#define __NEON_TLS_PORT_H__

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*==================================================================================================
*                                       FUNCTION PROTOTYPES
==================================================================================================*/
/**
 * @brief mbedTLS calloc on the FreeRTOS heap (thread-safe, zeroed, overflow-checked).
 * @param[in] count Number of elements.
 * @param[in] size Element size in bytes.
 * @return Zeroed block, or NULL.
 */
void *neon_tls_port_calloc(size_t count, size_t size);
/**
 * @brief mbedTLS free on the FreeRTOS heap.
 * @param[in] pointer Block from neon_tls_port_calloc, or NULL.
 */
void neon_tls_port_free(void *pointer);
/**
 * @brief Fill output with entropy for the mbedTLS entropy pool (weak: override with a TRNG).
 * @param[out] output Destination.
 * @param[in] size_bytes Bytes requested.
 * @return 0 on success, nonzero when no entropy source exists.
 * @note Called from task context while a TLS handle is seeded or reseeded.
 */
int neon_tls_port_entropy(uint8_t *output, size_t size_bytes);

#ifdef __cplusplus
}
#endif
#endif /* __NEON_TLS_PORT_H__ */
