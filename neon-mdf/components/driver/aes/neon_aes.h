/**
 * @file neon_aes.h
 * @brief Public declarations for AES.
 * @details
 * GT32U2086 native AES-128 IP: key/IV/input/output banks, polling READY/DONE.
 * config stores direction for the single AES instance. Serialize access to the
 * engine, its configuration and key when sharing it between tasks.
 * @note Blocking polling/configuration operations are intended for task context.
 * Serialize configuration/teardown with other users of the same peripheral;
 * timeout_polls values are polling budgets, not milliseconds, where applicable.
 */

#ifndef __NEON_AES_H__
#define __NEON_AES_H__

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include "neon_types.h"
#include "mcu_device.h"

#ifdef __cplusplus
extern "C" {
#endif

/*==================================================================================================
*                                        DEFINES AND MACROS
==================================================================================================*/
#define NEON_AES_MODE_ECB 0x0U
#define NEON_AES_MODE_CBC 0x1U
#define NEON_AES_MODE_CTR 0x2U

#define NEON_AES_DIRECTION_ENCRYPT 0x0U
#define NEON_AES_DIRECTION_DECRYPT (1U << 3)

/*==================================================================================================
*                                  STRUCTURES AND OTHER TYPEDEFS
==================================================================================================*/
typedef struct {
    uint32_t mode;      /* NEON_AES_MODE_* */
    uint32_t direction; /* NEON_AES_DIRECTION_* */
} neon_aes_config_t;

/*==================================================================================================
*                                       FUNCTION PROTOTYPES
==================================================================================================*/
/**
 * @brief Initialize AES.
 * @param[in] instance Peripheral register instance.
 * @param[in] init Initialization settings.
 * @return NEON_OK on success, or a module error/status code.
 */
neon_status_t neon_aes_init(AES_TypeDef *instance, const neon_aes_config_t *init);
/**
 * @brief Deinitialize AES.
 * @param[in] instance Peripheral register instance.
 * @return NEON_OK on success, or a module error/status code.
 */
neon_status_t neon_aes_deinit(AES_TypeDef *instance);

/**
 * @brief Set AES key128.
 * @param[in] instance Peripheral register instance.
 * @param[in] key Key.
 * @return NEON_OK on success, or a module error/status code.
 * @details
 * Four words in the native register order.
 */
neon_status_t neon_aes_set_key128(AES_TypeDef *instance, const uint32_t key[4]);
/**
 * @brief Set AES iv.
 * @param[in] instance Peripheral register instance.
 * @param[in] iv Iv.
 * @return NEON_OK on success, or a module error/status code.
 */
neon_status_t neon_aes_set_iv(AES_TypeDef *instance, const uint32_t iv[4]);

/**
 * @brief Aes process block.
 * @param[in] instance Peripheral register instance.
 * @param[in] in In.
 * @param[out] out Destination for the validated result.
 * @param[in] timeout_polls Polling iteration budget; this value is not milliseconds.
 * @return NEON_OK on success, or a module error/status code.
 * @details
 * Process one independent 128-bit block; SetIV supplies IV or CTR counter.
 * timeout_polls is the poll budget for readiness/completion, not milliseconds.
 */
neon_status_t neon_aes_process_block(AES_TypeDef *instance, const uint32_t in[4], uint32_t out[4],
                                     uint32_t timeout_polls);

#ifdef __cplusplus
}
#endif

#endif /* __NEON_AES_H__ */
