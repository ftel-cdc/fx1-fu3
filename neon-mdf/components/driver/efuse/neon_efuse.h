/**
 * @file neon_efuse.h
 * @brief Public declarations for eFuse.
 * @details
 * Legacy only: OTP_TypeDef belongs to the old FU2 map. Do not include this
 * header in current applications; use flash/neon_flash.h for NVR records.
 */

/*
 * NEON_EFUSE - đọc/ghi OTP (one-time-programmable) theo hàng 8-bit.
 * Port THẬT từ src_fu2/fu2_efuse.c (team cũ, BuuPQ) - mảng eFuse là vùng nhớ
 * memory-mapped riêng (FU2_EFUSE_ARRAY_BASE = OTP_BASE + 0x1000, 128 hàng ×
 * 4 byte stride, xem inc_fu2/fu2_efuse.h), KHÔNG đọc/ghi qua thanh ghi
 * `instance` như CRC/ADC - `instance` chỉ dùng để set timing trước khi
 * đọc/ghi (timing áp dụng chung cho toàn khối OTP, không theo từng hàng).
 *
 * CẢNH BÁO GHI: mỗi hàng OTP chỉ ghi được 1 LẦN DUY NHẤT (one-time
 * programmable, đúng như tên) - ProgramRow gọi nhầm hàng đã ghi rồi sẽ
 * KHÔNG lỗi báo (phần cứng OTP thường chỉ OR thêm bit, không revert được)
 * nhưng dữ liệu cũ có thể bị hỏng vĩnh viễn - gọi cẩn thận.
 */
#ifndef __NEON_EFUSE_H__
#define __NEON_EFUSE_H__

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include "neon_types.h"
#include "mcu_device.h"

#ifdef __cplusplus
extern "C" {
#endif

/*==================================================================================================
*                                  STRUCTURES AND OTHER TYPEDEFS
==================================================================================================*/
typedef struct {
    uint32_t tcs_setup;
    uint32_t tcs_hold;
    uint32_t tprchg_setup;
    uint32_t tprchg_hold;
    uint32_t tsense_setup;
    uint32_t tsense_hold;
    uint32_t tcap_data;
} neon_efuse_read_timing_t;

typedef struct {
    uint32_t tprogwh;
    uint32_t tpsetup2;
    uint32_t tphold2;
    uint32_t taddr2prog;
    uint32_t tpah;
} neon_efuse_program_timing_t;

/*==================================================================================================
*                                        DEFINES AND MACROS
==================================================================================================*/
#define NEON_EFUSE_ROW_COUNT 128U /* 0..127, xem FU2_EFUSE_ROW_COUNT */

/*==================================================================================================
*                                       FUNCTION PROTOTYPES
==================================================================================================*/
/**
 * @brief Set eFuse read timing.
 * @param[in] instance Peripheral register instance.
 * @param[in] timing timing.
 * @return NEON_OK on success, or a module error/status code.
 */
neon_status_t neon_efuse_set_read_timing(OTP_TypeDef *instance, const neon_efuse_read_timing_t *timing);
/**
 * @brief Set eFuse program timing.
 * @param[in] instance Peripheral register instance.
 * @param[in] timing timing.
 * @return NEON_OK on success, or a module error/status code.
 */
neon_status_t neon_efuse_set_program_timing(OTP_TypeDef *instance, const neon_efuse_program_timing_t *timing);

/**
 * @brief Read eFuse row.
 * @param[in] row row.
 * @param[in,out] p_data P data.
 * @return NEON_OK on success, or a module error/status code.
 * @details
 * row: 0..NEON_EFUSE_ROW_COUNT-1. `instance` không dùng trong 2 hàm dưới
 * (mảng OTP là 1 vùng nhớ cố định duy nhất) nhưng vẫn nhận để giữ khuôn API
 * chung - xem cảnh báo ghi ở trên trước khi gọi ProgramRow.
 */
neon_status_t neon_efuse_read_row(uint32_t row, uint8_t *p_data);
/**
 * @brief Program eFuse row.
 * @param[in] row row.
 * @param[in] data data.
 * @return NEON_OK on success, or a module error/status code.
 */
neon_status_t neon_efuse_program_row(uint32_t row, uint8_t data);

#ifdef __cplusplus
}
#endif

#endif /* __NEON_EFUSE_H__ */
