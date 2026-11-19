/**
 * @file neon_aes.c
 * @brief Implementation for AES.
 * @details
 * Implements module behavior and target-specific integration.
 */

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include "aes/neon_aes.h"

/*==================================================================================================
*                                        DEFINES AND MACROS
==================================================================================================*/
/* GT32U2086 AES IP: start bits [1:0], chain bit [2], mode [5:3]. */
#define AES_START_ENCRYPT 1UL
#define AES_START_DECRYPT 2UL
#define AES_STATUS_READY 1UL
#define AES_STATUS_DONE 2UL

/*==================================================================================================
*                                         LOCAL VARIABLES
==================================================================================================*/
/* direction is software state: the hardware start bits automatically clear. */
static uint32_t gt32u2086_aes_start_command = AES_START_ENCRYPT;

/*==================================================================================================
*                                         GLOBAL FUNCTIONS
==================================================================================================*/

/** @copydoc neon_aes_init */
neon_status_t neon_aes_init(AES_TypeDef *instance, const neon_aes_config_t *init) {
    if (instance != AES || init == 0 || init->mode > NEON_AES_MODE_CTR ||
        (init->direction != NEON_AES_DIRECTION_ENCRYPT && init->direction != NEON_AES_DIRECTION_DECRYPT)) {
        return NEON_INVALID_PARAM;
    }
    if ((instance->STATUS & AES_STATUS_READY) == 0U) {
        return NEON_BUSY;
    }
    /* Public mode constants are logical; native CTR mode is 4, not 2. */
    uint32_t mode = init->mode == NEON_AES_MODE_CTR ? 4U : init->mode;
    instance->CTRL = mode << 3U;
    gt32u2086_aes_start_command = init->direction == NEON_AES_DIRECTION_DECRYPT ? AES_START_DECRYPT : AES_START_ENCRYPT;
    return NEON_OK;
}

/** @copydoc neon_aes_deinit */
neon_status_t neon_aes_deinit(AES_TypeDef *instance) {
    if (instance != AES) {
        return NEON_INVALID_PARAM;
    }
    if ((instance->STATUS & AES_STATUS_READY) == 0U) {
        return NEON_BUSY;
    }
    instance->CTRL = 0U;
    instance->KEY0 = instance->KEY1 = instance->KEY2 = instance->KEY3 = 0U;
    instance->IV0 = instance->IV1 = instance->IV2 = instance->IV3 = 0U;
    instance->CTR0 = instance->CTR1 = instance->CTR2 = instance->CTR3 = 0U;
    gt32u2086_aes_start_command = AES_START_ENCRYPT;
    return NEON_OK;
}

/** @copydoc neon_aes_set_key128 */
neon_status_t neon_aes_set_key128(AES_TypeDef *instance, const uint32_t key[4]) {
    if (instance != AES || key == 0) {
        return NEON_INVALID_PARAM;
    }
    if ((instance->STATUS & AES_STATUS_READY) == 0U) {
        return NEON_BUSY;
    }
    instance->KEY0 = key[0];
    instance->KEY1 = key[1];
    instance->KEY2 = key[2];
    instance->KEY3 = key[3];
    return NEON_OK;
}

/** @copydoc neon_aes_set_iv */
neon_status_t neon_aes_set_iv(AES_TypeDef *instance, const uint32_t iv[4]) {
    if (instance != AES || iv == 0) {
        return NEON_INVALID_PARAM;
    }
    if ((instance->STATUS & AES_STATUS_READY) == 0U) {
        return NEON_BUSY;
    }
    instance->IV0 = iv[0];
    instance->IV1 = iv[1];
    instance->IV2 = iv[2];
    instance->IV3 = iv[3];
    /* CTR has its own initial-counter register bank on the native IP. */
    instance->CTR0 = iv[0];
    instance->CTR1 = iv[1];
    instance->CTR2 = iv[2];
    instance->CTR3 = iv[3];
    return NEON_OK;
}

/** @copydoc neon_aes_process_block */
neon_status_t neon_aes_process_block(AES_TypeDef *instance, const uint32_t in[4], uint32_t out[4],
                                     uint32_t timeout_polls) {
    if (instance != AES || in == 0 || out == 0) {
        return NEON_INVALID_PARAM;
    }
    uint32_t remaining = timeout_polls;
    while ((instance->STATUS & AES_STATUS_READY) == 0U) {
        if (remaining-- == 0U) {
            return NEON_TIMEOUT;
        }
    }
    instance->DIN0 = in[0];
    instance->DIN1 = in[1];
    instance->DIN2 = in[2];
    instance->DIN3 = in[3];
    instance->CTRL = (instance->CTRL & 0x3CUL) | gt32u2086_aes_start_command;
    remaining = timeout_polls;
    while ((instance->STATUS & (AES_STATUS_READY | AES_STATUS_DONE)) != (AES_STATUS_READY | AES_STATUS_DONE)) {
        if (remaining-- == 0U) {
            return NEON_TIMEOUT;
        }
    }
    out[0] = instance->DOUT0;
    out[1] = instance->DOUT1;
    out[2] = instance->DOUT2;
    out[3] = instance->DOUT3;
    return NEON_OK;
}
