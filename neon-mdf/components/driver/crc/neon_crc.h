/**
 * @file neon_crc.h
 * @brief Public declarations for CRC.
 * @details
 * * NEON_CRC - CRC-8 engine, blocking feature. API model STM32 HAL. REAL port from
 * src_fu2/fu2_crc.c::LL_CRC_Calc (old team, BuuPQ) - self-contained, no need to call separate
 * config() first (original design matching: LL_CRC_Calc manually configure
 * polynomial/data_length/enable then feed FIFO and disable again).
 *
 * Source: inc_fu2/fu2_crc.h ("FU2 CRC Hardware Specification v2.0"). FU2_CRC is the only engine (no
 * more instances) - still receives the `instance` parameter to keep the generic API framework like
 * other peripherals.
 * @note Blocking polling/configuration operations are intended for task context.
 * Serialize configuration/teardown with other users of the same peripheral;
 * timeout_polls values are polling budgets, not milliseconds, where applicable.
 */

#ifndef __NEON_CRC_H__
#define __NEON_CRC_H__

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
/* Common 8-bit polynomials - see inc_fu2/fu2_crc.h LL_CRC_POLY_*. */
#define NEON_CRC_POLY_SMBUS 0x07U     /* CRC-8/SMBUS, also = CRC-8/ITU */
#define NEON_CRC_POLY_MAXIM 0x31U     /* CRC-8/MAXIM */
#define NEON_CRC_POLY_SAE_J1850 0x1DU /* CRC-8/SAE-J1850 */

#define NEON_CRC_DATA_LENGTH_MAX 255U /* real limit CTRL.data_length [15:8] */

/*==================================================================================================
*                                       FUNCTION PROTOTYPES
==================================================================================================*/
/**
 * @brief Calculate CRC.
 * @param[in] instance Peripheral register instance.
 * @param[in] polynomial polynomial.
 * @param[in] p_data P data.
 * @param[in] length length.
 * @param[in,out] p_result P result.
 * @param[in] timeout_polls Polling iteration budget; this value is not milliseconds.
 * @return NEON_OK on success, or a module error/status code.
 * @details
 * Calculate CRC-8 for 32-bit word array, return 8-bit results via *p_result. length:
 * 1..NEON_CRC_DATA_LENGTH_MAX. timeout_polls is the total number of poll pending FIFO rounds and
 * results, not milliseconds. When timeout_polls is over, engine shuts down.
 */
neon_status_t neon_crc_calculate(CRC_TypeDef *instance, uint32_t polynomial, const uint32_t *p_data, uint32_t length,
                                 uint8_t *p_result, uint32_t timeout_polls);

#ifdef __cplusplus
}
#endif

#endif /* __NEON_CRC_H__ */
