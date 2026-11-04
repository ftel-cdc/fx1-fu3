/**
 * @file neon_efuse.h
 * @brief Public declarations for eFuse.
 * @details
 * Legacy only: OTP_TypeDef belongs to the old FU2 map. Do not include this
 * header in current applications; use flash/neon_flash.h for NVR records.
 */

/*
 * NEON_EFUSE - read/write OTP (one-time-programmable) in 8-bit rows. Port TRUE from
 * src_fu2/fu2_efuse.c (former team, BuuPQ) - eFuse array is a separate memory-mapped area
 * (FU2_EFUSE_ARRAY_BASE = OTP_BASE + 0x1000, 128 rows × 4 byte stride, see inc_fu2/fu2_efuse.h), DO
 * NOT read/write via register `instance` like CRC/ADC - `instance` is only used to set timing
 * before read/write (timing applies to the entire OTP block, not in rows).
 *
 * Write WARNING: OTP rows can only be written 1 TIME (one-time programmable, as the name implies) -
 * ProgramRow calls a row that has already been written incorrectly and will NOT report errors
 * (hardware OTP usually only OR add bits, cannot revert) but old data can be permanently corrupted
 * - call carefully.
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
#define NEON_EFUSE_ROW_COUNT 128U /* 0..127, see FU2_EFUSE_ROW_COUNT */

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
 * row: 0..NEON_EFUSE_ROW_COUNT-1. `instance` is not used in the underlying two functions (the OTP
 * array is a single fixed memory area) but is accepted to keep the common API framework - see write
 * warning above before calling ProgramRow.
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
