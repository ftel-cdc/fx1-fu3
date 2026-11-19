/**
 * @file neon_crc.c
 * @brief Implementation for CRC.
 * @details
 * Implements module behavior and target-specific integration.
 */

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include "crc/neon_crc.h"
#include <string.h>

/*==================================================================================================
*                                        DEFINES AND MACROS
==================================================================================================*/
/*
 * REAL location/mask bits (inc_gt32u2086/gt32u2086_crc.h) - generator mcu-description only creates field struct +
 * offset, does not generate its own bit macro, so re-create local here (same way
 * platform/gt32u2086/neon_gpio.c/system_GT32U2086.c did for CRM).
 */
#define CRC_CTRL_DATA_LENGTH_POS 8U
#define CRC_CTRL_DATA_LENGTH_MASK (0xFFU << CRC_CTRL_DATA_LENGTH_POS)
#define CRC_CTRL_EN (1U << 0)

#define CRC_GEN_POLY_VALUE_MASK 0xFFU

#define CRC_STATUS_CRC_DONE (1U << 4)
#define CRC_STATUS_FIFO_FULL (1U << 0)

#define CRC_RESULT_VALUE_MASK 0xFFU

/*==================================================================================================
*                                         GLOBAL FUNCTIONS
==================================================================================================*/

/** @copydoc neon_crc_calculate */
neon_status_t neon_crc_calculate(CRC_TypeDef *instance, uint32_t polynomial, const uint32_t *p_data, uint32_t length,
                                 uint8_t *p_result, uint32_t timeout_polls) {
    uint32_t i;

    if (instance != CRC || p_data == 0 || p_result == 0 || length == 0 || length > NEON_CRC_DATA_LENGTH_MAX) {
        return NEON_INVALID_PARAM;
    }

    /* Disable before reconfiguring - match gt32u2086_crc.c::LL_CRC_Calc */
    instance->CTRL = 0;
    instance->GEN_POLY = polynomial & CRC_GEN_POLY_VALUE_MASK;
    instance->CTRL = ((length << CRC_CTRL_DATA_LENGTH_POS) & CRC_CTRL_DATA_LENGTH_MASK) | CRC_CTRL_EN;

    for (i = 0; i < length; i++) {
        while (instance->STATUS & CRC_STATUS_FIFO_FULL) {
            if (timeout_polls-- == 0U) {
                instance->CTRL = 0U;
                return NEON_TIMEOUT;
            }
        }
        instance->DATAIN = p_data[i];
    }

    while (!(instance->STATUS & CRC_STATUS_CRC_DONE)) {
        if (timeout_polls-- == 0U) {
            instance->CTRL = 0U;
            return NEON_TIMEOUT;
        }
    }

    const uint32_t result_word = (uint8_t)(instance->RESULT & CRC_RESULT_VALUE_MASK);
    memcpy(p_result, &result_word, 1U);

    instance->INT_RAW = 1U; /* W1C - erase flag done */
    instance->CTRL = 0;     /* disable engine after finishing, match original */

    return NEON_OK;
}
