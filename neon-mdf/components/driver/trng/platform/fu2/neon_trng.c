/**
 * @file neon_trng.c
 * @brief Implementation for TRNG.
 * @details
 * Implements module behavior and target-specific integration.
 */

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include "trng/neon_trng.h"

/*==================================================================================================
*                                        DEFINES AND MACROS
==================================================================================================*/
/* bit location/true mask (inc_fu2/fu2_dti_trng.h). */
#define TRNG_REC_EN (1U << 0)
#define TRNG_REC_MODE_NORMAL 0U /* bit1=0 -> normal operation (not LFSR test mode) */

#define TRNG_TREG_TWARM_POS 0U
#define TRNG_TREG_TWARM_MASK (0x3FFU << TRNG_TREG_TWARM_POS)

#define TRNG_VLD_RNUM (1U << 0)

/*==================================================================================================
*                                         GLOBAL FUNCTIONS
==================================================================================================*/

/** @copydoc neon_trng_init */
neon_status_t neon_trng_init(RNG_TypeDef *instance) {
    if (instance == 0) {
        return NEON_ERROR;
    }

    /* Disable before reconfiguring - match fu2_dti_trng.c::LL_TRNG_Init */
    instance->REC &= ~TRNG_REC_EN;
    instance->TREG = (instance->TREG & ~TRNG_TREG_TWARM_MASK) |
                     ((NEON_TRNG_WARMUP_DEFAULT << TRNG_TREG_TWARM_POS) & TRNG_TREG_TWARM_MASK);
    instance->REC = TRNG_REC_EN | TRNG_REC_MODE_NORMAL;

    return NEON_OK;
}

/** @copydoc neon_trng_deinit */
neon_status_t neon_trng_deinit(RNG_TypeDef *instance) {
    if (instance == 0) {
        return NEON_ERROR;
    }
    instance->REC = 0;
    instance->TREG = NEON_TRNG_WARMUP_DEFAULT;
    return NEON_OK;
}

/** @copydoc neon_trng_get_random_number * @details
 * * NOT in src_fu2/fu2_dti_trng.c (team only writes config/DeInit/ SetLFSRTestLengths) - newly written
 * pairing LL_TRNG_ReadData/IsDataValid real (2 inline function in inc_fu2/fu2_dti_trng.h) as
 * recommended "data is valid only once VLD is asserted" in original comment LL_TRNG_Init.
 */
neon_status_t neon_trng_get_random_number(RNG_TypeDef *instance, uint32_t *p_value, uint32_t timeout_polls) {
    uint32_t remaining;

    if (instance == 0 || p_value == 0) {
        return NEON_ERROR;
    }

    remaining = timeout_polls;
    while (!(instance->VLD & TRNG_VLD_RNUM)) {
        if (remaining-- == 0) {
            return NEON_TIMEOUT;
        }
    }

    *p_value = instance->DAT;
    return NEON_OK;
}
