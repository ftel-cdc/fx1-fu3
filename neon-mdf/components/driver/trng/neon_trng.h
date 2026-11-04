/**
 * @file neon_trng.h
 * @brief Public declarations for TRNG.
 * @details
 * * NEON_TRNG - sinh số ngẫu nhiên thật (True RNG). config/DeInit port THẬT từ
 * src_fu2/fu2_dti_trng.c (team cũ, BuuPQ). neon_trng_get_random_number KHÔNG
 * có trong nguồn team dạng hàm .c (chỉ có inline LL_TRNG_ReadData/IsDataValid
 * trong inc_fu2/fu2_dti_trng.h) - viết mới bằng cách ghép 2 hàm inline thật
 * đó theo đúng khuyến cáo trong header ("data is valid only once VLD is
 * asserted").
 *
 * Nguồn: inc_fu2/fu2_dti_trng.h ("DTI_TRNG User Guide Rev 1.0, Dolphin
 * Technology") - IP thứ 3 (Dolphin Technology), xem lưu ý license trong
 * memory dự án trước khi chia sẻ SDK ra ngoài.
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
#define NEON_TRNG_WARMUP_DEFAULT 0x00AU /* THẬT - LL_TRNG_TWARM_DEFAULT (10 chu kỳ) */

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
 * Blocking - poll VLD tới khi có số mới hoặc hết timeout_polls (số vòng poll).
 */
neon_status_t neon_trng_get_random_number(RNG_TypeDef *instance, uint32_t *p_value, uint32_t timeout_polls);

#ifdef __cplusplus
}
#endif

#endif /* __NEON_TRNG_H__ */
