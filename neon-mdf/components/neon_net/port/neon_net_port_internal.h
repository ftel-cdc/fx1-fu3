/**
 * @file    neon_net_port_internal.h
 * @brief   @internal Platform services of the network manager not seen by lwIP itself.
 */
#ifndef __NEON_NET_PORT_INTERNAL_H__
#define __NEON_NET_PORT_INTERNAL_H__

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*==================================================================================================
*                                       FUNCTION PROTOTYPES
==================================================================================================*/
/**
 * @brief Mix bytes (MAC address) and the current mtime into the lwIP random generator.
 * @param[in] bytes Seed material.
 * @param[in] size_bytes Number of bytes.
 */
void neon_net_port_seed(const uint8_t *bytes, uint32_t size_bytes);

#ifdef __cplusplus
}
#endif
#endif /* __NEON_NET_PORT_INTERNAL_H__ */
