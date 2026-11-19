/**
 * @file neon_wdt.c
 * @brief Implementation for watchdog.
 * @details
 * Implements module behavior and target-specific integration.
 */

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include "wdt/neon_wdt.h"
#include "crm/neon_crm.h"
#include <string.h>

/*==================================================================================================
*                                        DEFINES AND MACROS
==================================================================================================*/
/* GT32U2086.yaml WDT register descriptions. */
#define WDT_ENABLE (1U << 0)
#define WDT_RESPONSE_POS 1U
#define WDT_RESET_PULSE_POS 2U
#define WDT_RESTART_KEY 0x76U

/*==================================================================================================
*                                         GLOBAL FUNCTIONS
==================================================================================================*/

/** @copydoc neon_wdt_get_default_config */
void neon_wdt_get_default_config(neon_wdt_config_t *config) {
    if (config == 0) {
        return;
    }
    config->timeout_range = 0xFFFFFFFFU;
    config->reset_pulse_length = 0U;
    config->response = NEON_WDT_RESPONSE_RESET;
}

/** @copydoc neon_wdt_init */
neon_status_t neon_wdt_init(WDT_TypeDef *instance, const neon_wdt_config_t *config) {
    neon_status_t status;
    if (instance != WDT || config == 0 || config->reset_pulse_length > 255U ||
        (uint32_t)config->response > NEON_WDT_RESPONSE_INTERRUPT) {
        return NEON_INVALID_PARAM;
    }
    status = neon_crm_peripheral_clock_enable(NEON_CRM_PERIPHERAL_WDT);
    if (status == NEON_OK) {
        status = neon_crm_peripheral_reset(NEON_CRM_PERIPHERAL_WDT);
    }
    if (status != NEON_OK) {
        return status;
    }

    instance->TORR = config->timeout_range;
    instance->CRR = WDT_RESTART_KEY;
    instance->CR = WDT_ENABLE | ((uint32_t)config->response << WDT_RESPONSE_POS) |
                   (config->reset_pulse_length << WDT_RESET_PULSE_POS);
    return NEON_OK;
}

/** @copydoc neon_wdt_deinit */
neon_status_t neon_wdt_deinit(WDT_TypeDef *instance) {
    neon_status_t status;
    if (instance != WDT) {
        return NEON_INVALID_PARAM;
    }
    status = neon_crm_peripheral_reset(NEON_CRM_PERIPHERAL_WDT);
    if (status != NEON_OK) {
        return status;
    }
    return neon_crm_peripheral_clock_disable(NEON_CRM_PERIPHERAL_WDT);
}

/** @copydoc neon_wdt_refresh */
neon_status_t neon_wdt_refresh(WDT_TypeDef *instance) {
    if (instance != WDT) {
        return NEON_INVALID_PARAM;
    }
    instance->CRR = WDT_RESTART_KEY;
    return NEON_OK;
}

/** @copydoc neon_wdt_get_counter */
neon_status_t neon_wdt_get_counter(WDT_TypeDef *instance, uint32_t *counter) {
    if (instance != WDT || counter == 0) {
        return NEON_INVALID_PARAM;
    }
    *counter = instance->CCVR;
    return NEON_OK;
}

/** @copydoc neon_wdt_is_interrupt_pending */
neon_status_t neon_wdt_is_interrupt_pending(WDT_TypeDef *instance, uint8_t *pending) {
    if (instance != WDT || pending == 0) {
        return NEON_INVALID_PARAM;
    }
    const uint32_t result = instance->STAT & 1U;
    memcpy(pending, &result, 1U);
    return NEON_OK;
}

/** @copydoc neon_wdt_clear_interrupt */
neon_status_t neon_wdt_clear_interrupt(WDT_TypeDef *instance) {
    if (instance != WDT) {
        return NEON_INVALID_PARAM;
    }
    (void)instance->EOI;
    return NEON_OK;
}
