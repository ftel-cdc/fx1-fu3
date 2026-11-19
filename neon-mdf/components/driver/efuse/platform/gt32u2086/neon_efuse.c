/**
 * @file neon_efuse.c
 * @brief Implementation for eFuse.
 * @details
 * OTP backend for the old GT32U2086 map; kept in efuse/ but disabled on current GT32U2086.
 * Current GT32U2086 security records use the Flash NVR API.
 */

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/

#include "efuse/neon_efuse.h"
#include <string.h>

/*==================================================================================================
*                                        DEFINES AND MACROS
==================================================================================================*/
/* REAL position bits (inc_gt32u2086/gt32u2086_efuse.h) for 2 register timing. */
#define EFUSE_RD_TIMING0_TCS_SETUP_POS 24U
#define EFUSE_RD_TIMING0_TCS_HOLD_POS 20U
#define EFUSE_RD_TIMING0_TPRCHG_SETUP_POS 16U
#define EFUSE_RD_TIMING0_TPRCHG_HOLD_POS 12U
#define EFUSE_RD_TIMING0_TSENSE_SETUP_POS 8U
#define EFUSE_RD_TIMING0_TSENSE_HOLD_POS 4U
#define EFUSE_RD_TIMING0_TCAP_DATA_POS 0U
#define EFUSE_TIMING4_MASK 0xFU /* every 4 bit wide TIMING4 field */

#define EFUSE_PGM_TIMING0_TPROGWH_POS 16U
#define EFUSE_PGM_TIMING0_TPROGWH_MASK 0x3FFU
#define EFUSE_PGM_TIMING0_TPSETUP2_POS 12U
#define EFUSE_PGM_TIMING0_TPHOLD2_POS 8U
#define EFUSE_PGM_TIMING0_TADDR2PROG_POS 4U
#define EFUSE_PGM_TIMING0_TPAH_POS 0U

/* True eFuse array - TRUE according to inc_gt32u2086/gt32u2086_efuse.h, memory-mapped separately, NOT via register
 * OTP_TypeDef (see neon_efuse.h). Base = OTP_BASE + 0x1000, stride 4 bytes/row. */
#define EFUSE_NEON_EFUSE_ARRAY_BASE ((uintptr_t)OTP_BASE + 0x1000U)
#define EFUSE_NEON_EFUSE_ROW_STRIDE 4U

/*==================================================================================================
*                                         GLOBAL FUNCTIONS
==================================================================================================*/

/** @copydoc neon_efuse_set_read_timing */
neon_status_t neon_efuse_set_read_timing(OTP_TypeDef *instance, const neon_efuse_read_timing_t *timing) {
    uint32_t reg;

    if (instance == 0 || timing == 0 || timing->tcs_setup > EFUSE_TIMING4_MASK ||
        timing->tcs_hold > EFUSE_TIMING4_MASK || timing->tprchg_setup > EFUSE_TIMING4_MASK ||
        timing->tprchg_hold > EFUSE_TIMING4_MASK || timing->tsense_setup > EFUSE_TIMING4_MASK ||
        timing->tsense_hold > EFUSE_TIMING4_MASK || timing->tcap_data > EFUSE_TIMING4_MASK) {
        return NEON_ERROR;
    }

    reg = (timing->tcs_setup << EFUSE_RD_TIMING0_TCS_SETUP_POS) | (timing->tcs_hold << EFUSE_RD_TIMING0_TCS_HOLD_POS) |
          (timing->tprchg_setup << EFUSE_RD_TIMING0_TPRCHG_SETUP_POS) |
          (timing->tprchg_hold << EFUSE_RD_TIMING0_TPRCHG_HOLD_POS) |
          (timing->tsense_setup << EFUSE_RD_TIMING0_TSENSE_SETUP_POS) |
          (timing->tsense_hold << EFUSE_RD_TIMING0_TSENSE_HOLD_POS) |
          (timing->tcap_data << EFUSE_RD_TIMING0_TCAP_DATA_POS);

    instance->RD_TIMING0 = reg;
    return NEON_OK;
}

/** @copydoc neon_efuse_set_program_timing */
neon_status_t neon_efuse_set_program_timing(OTP_TypeDef *instance, const neon_efuse_program_timing_t *timing) {
    uint32_t reg;

    if (instance == 0 || timing == 0 || timing->tprogwh > EFUSE_PGM_TIMING0_TPROGWH_MASK ||
        timing->tpsetup2 > EFUSE_TIMING4_MASK || timing->tphold2 > EFUSE_TIMING4_MASK ||
        timing->taddr2prog > EFUSE_TIMING4_MASK || timing->tpah > EFUSE_TIMING4_MASK) {
        return NEON_ERROR;
    }

    reg = (timing->tprogwh << EFUSE_PGM_TIMING0_TPROGWH_POS) | (timing->tpsetup2 << EFUSE_PGM_TIMING0_TPSETUP2_POS) |
          (timing->tphold2 << EFUSE_PGM_TIMING0_TPHOLD2_POS) |
          (timing->taddr2prog << EFUSE_PGM_TIMING0_TADDR2PROG_POS) | (timing->tpah << EFUSE_PGM_TIMING0_TPAH_POS);

    instance->PGM_TIMING0 = reg;
    return NEON_OK;
}

/** @copydoc neon_efuse_read_row */
neon_status_t neon_efuse_read_row(uint32_t row, uint8_t *p_data) {
    volatile uint32_t *row_addr;

    if (p_data == 0 || row >= NEON_EFUSE_ROW_COUNT) {
        return NEON_ERROR;
    }

    row_addr = (volatile uint32_t *)(EFUSE_NEON_EFUSE_ARRAY_BASE + (row * EFUSE_NEON_EFUSE_ROW_STRIDE));
    const uint32_t result_word = (uint8_t)(*row_addr & 0xFFU);
    memcpy(p_data, &result_word, 1U);
    return NEON_OK;
}

/** @copydoc neon_efuse_program_row */
neon_status_t neon_efuse_program_row(uint32_t row, uint8_t data) {
    volatile uint32_t *row_addr;

    if (row >= NEON_EFUSE_ROW_COUNT) {
        return NEON_ERROR;
    }

    row_addr = (volatile uint32_t *)(EFUSE_NEON_EFUSE_ARRAY_BASE + (row * EFUSE_NEON_EFUSE_ROW_STRIDE));
    *row_addr = (uint32_t)data & 0xFFU;
    return NEON_OK;
}
