/**
 * @file neon_crc.h
 * @brief Public declarations for CRC.
 * @details
 * * NEON_CRC - CRC-8 engine, tính chặn (blocking). API kiểu STM32 HAL.
 * Port THẬT từ src_fu2/fu2_crc.c::LL_CRC_Calc (team cũ, BuuPQ) - tự chứa,
 * không cần gọi config() riêng trước (khớp thiết kế gốc: LL_CRC_Calc tự cấu
 * hình polynomial/data_length/enable rồi feed FIFO rồi disable lại).
 *
 * Nguồn: inc_fu2/fu2_crc.h ("FU2 CRC Hardware Specification v2.0").
 * FU2_CRC là engine DUY NHẤT (không có nhiều instance) - vẫn nhận tham số
 * `instance` để giữ khuôn API chung như các peripheral khác.
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
/* polynomial 8-bit thường dùng - xem inc_fu2/fu2_crc.h LL_CRC_POLY_*. */
#define NEON_CRC_POLY_SMBUS 0x07U     /* CRC-8/SMBUS, cũng = CRC-8/ITU */
#define NEON_CRC_POLY_MAXIM 0x31U     /* CRC-8/MAXIM */
#define NEON_CRC_POLY_SAE_J1850 0x1DU /* CRC-8/SAE-J1850 */

#define NEON_CRC_DATA_LENGTH_MAX 255U /* giới hạn thật CTRL.data_length [15:8] */

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
 * Tính CRC-8 cho mảng word 32-bit, trả kết quả 8-bit qua *p_result.
 * length: 1..NEON_CRC_DATA_LENGTH_MAX. timeout_polls là tổng số vòng poll chờ
 * FIFO và kết quả, không phải mili-giây. Hết timeout_polls sẽ tắt engine.
 */
neon_status_t neon_crc_calculate(CRC_TypeDef *instance, uint32_t polynomial, const uint32_t *p_data, uint32_t length,
                                 uint8_t *p_result, uint32_t timeout_polls);

#ifdef __cplusplus
}
#endif

#endif /* __NEON_CRC_H__ */
