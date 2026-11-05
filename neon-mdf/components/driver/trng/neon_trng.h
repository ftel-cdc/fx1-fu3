/**
 * @file neon_trng.h
 * @brief Public declarations for TRNG.
 * @details
 * * NEON_TRNG - generate real random numbers (True RNG). config/DeInit ports REAL from
 * src_fu2/fu2_dti_trng.c (old team, BuuPQ). neon_trng_get_random_number is NOT in .c team source
 * (only inline LL_TRNG_ReadData/IsDataValid in inc_fu2/fu2_dti_trng.h) - write new by combining 2
 * real inline functions as recommended in header ("data is valid only once VLD is
 * asserted").
 *
 * Source: inc_fu2/fu2_dti_trng.h ("DTI_TRNG User Guide Rev 1.0, Dolphin Technology") - 3rd IP (Dolphin
 * Technology), see note license in project memory before sharing SDK out.
 * @note Blocking polling/configuration operations are intended for task context.
 * Serialize configuration/teardown with other users of the same peripheral;
 * timeout_polls values are polling budgets, not milliseconds, where applicable.
 */

#ifndef __NEON_TRNG_H__
#define __NEON_TRNG_H__

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
#define NEON_TRNG_WARMUP_DEFAULT 0x00AU /* REAL - LL_TRNG_TWARM_DEFAULT (10 period) */

/*==================================================================================================
*                                       FUNCTION PROTOTYPES
==================================================================================================*/
/**
 * @brief Initialize TRNG.
 * @param[in] instance Peripheral register instance.
 * @return NEON_OK on success, or a module error/status code.
 */
neon_status_t neon_trng_init(RNG_TypeDef *instance);
/**
 * @brief Deinitialize TRNG.
 * @param[in] instance Peripheral register instance.
 * @return NEON_OK on success, or a module error/status code.
 */
neon_status_t neon_trng_deinit(RNG_TypeDef *instance);

/**
 * @brief Read TRNG random number.
 * @param[in] instance Peripheral register instance.
 * @param[in,out] p_value P value.
 * @param[in] timeout_polls Polling iteration budget; this value is not milliseconds.
 * @return NEON_OK on success, or a module error/status code.
 * @details
 * Blocking - poll VLD until there is a new number or timeout_polls (poll ring number).
 */
neon_status_t neon_trng_get_random_number(RNG_TypeDef *instance, uint32_t *p_value, uint32_t timeout_polls);

#ifdef __cplusplus
}
#endif

#endif /* __NEON_TRNG_H__ */
