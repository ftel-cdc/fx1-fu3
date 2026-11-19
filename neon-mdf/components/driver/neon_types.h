/**
 * @file neon_types.h
 * @brief Common Neon MDF status and ABI-safe field types.
 * @details
 * Defines shared status codes and preserves the GT32U2086 aligned-word field ABI.
 */

#ifndef __NEON_TYPES_H__
#define __NEON_TYPES_H__

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include <stdint.h>

/* GT32U2086 logical narrow fields always occupy complete words in SRAM.
 * Rebuild all GT32U2086 objects when switching from the old narrow-field ABI. */
#if defined(GT32U2086)

/*==================================================================================================
*                                  STRUCTURES AND OTHER TYPEDEFS
==================================================================================================*/
typedef uint32_t neon_field8_t;
typedef uint32_t neon_field16_t;
#else
typedef uint8_t neon_field8_t;
typedef uint16_t neon_field16_t;
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    NEON_OK = 0x00,
    NEON_ERROR = 0x01,
    NEON_BUSY = 0x02,
    NEON_TIMEOUT = 0x03,
    NEON_INVALID_PARAM = 0x04,
    NEON_NOT_SUPPORTED = 0x05,
    NEON_LOCKED = 0x06,
    NEON_NOT_MAPPED = 0x07,
} neon_status_t;

#ifdef __cplusplus
}
#endif

#endif /* __NEON_TYPES_H__ */
