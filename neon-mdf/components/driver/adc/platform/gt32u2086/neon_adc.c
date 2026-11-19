/**
 * @file neon_adc.c
 * @brief Implementation for ADC.
 * @details
 * Implements module behavior and target-specific integration.
 */

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include "adc/neon_adc.h"
#include <string.h>

/*==================================================================================================
*                                         GLOBAL FUNCTIONS
==================================================================================================*/

/** @copydoc neon_adc_get_default_config */
void neon_adc_get_default_config(neon_adc_config_t *config) {
    if (config == 0) {
        return;
    }
    config->channel = 0U;
    config->sample_source = 1U;
    config->sample_count = 1U;
    config->sample_clock_low = 12U;
    config->sample_clock_high = 4U;
    config->setup_wait = 32U;
}

/** @copydoc neon_adc_init */
neon_status_t neon_adc_init(ADC_TypeDef *instance, const neon_adc_config_t *init) {
    if (instance != ADC || init == 0 || init->channel > 15U || init->sample_source > 1U || init->sample_count < 1U ||
        init->sample_count > 16U || init->sample_clock_low < 12U || init->sample_clock_low > 255U ||
        init->sample_clock_high < 4U || init->sample_clock_high > 255U || init->setup_wait < 32U ||
        init->setup_wait > 255U) {
        return NEON_INVALID_PARAM;
    }
    instance->CTRL = 0U;
    instance->CFG = (init->channel << ADC_CFG_CHANNEL_Pos) | (init->sample_source << ADC_CFG_SAMP_SEL_Pos);
    instance->TIMING = (init->sample_clock_low << ADC_TIMING_SAMPCLK_LOW_Pos) |
                       (init->sample_clock_high << ADC_TIMING_SAMPCLK_HIGH_Pos) |
                       (init->setup_wait << ADC_TIMING_SETUP_WAIT_Pos);
    instance->SAMPLE_CFG = init->sample_count;
    return NEON_OK;
}

/** @copydoc neon_adc_deinit */
neon_status_t neon_adc_deinit(ADC_TypeDef *instance) {
    if (instance != ADC) {
        return NEON_INVALID_PARAM;
    }
    instance->CTRL = 0U; /* Clearing ADC_EN clears completion/IRQ in the GT32U2086 model. */
    instance->CFG = ADC_CFG_SAMP_SEL_Msk;
    instance->TIMING = 0x20040C00UL;
    instance->SAMPLE_CFG = 1U;
    return NEON_OK;
}

/** @copydoc neon_adc_start_conversion */
neon_status_t neon_adc_start_conversion(ADC_TypeDef *instance) {
    if (instance != ADC) {
        return NEON_INVALID_PARAM;
    }
    if ((instance->STATUS & ADC_STATUS_BUSY_Msk) != 0U) {
        return NEON_BUSY;
    }
    instance->CTRL = 0U;
    instance->CTRL = ADC_CTRL_ADC_EN_Msk;
    return NEON_OK;
}

/** @copydoc neon_adc_read_samples */
neon_status_t neon_adc_read_samples(ADC_TypeDef *instance, uint16_t *data, uint32_t length, uint32_t timeout_polls) {
    if (instance != ADC || data == 0 || length == 0U || length > 16U ||
        length > (instance->SAMPLE_CFG & ADC_SAMPLE_CFG_SAMPLE_COUNT_Msk)) {
        return NEON_INVALID_PARAM;
    }
    while ((instance->STATUS & (ADC_STATUS_DONE_Msk | ADC_STATUS_READY_Msk)) !=
           (ADC_STATUS_DONE_Msk | ADC_STATUS_READY_Msk)) {
        if (timeout_polls-- == 0U) {
            instance->CTRL = 0U;
            return NEON_TIMEOUT;
        }
    }
    for (uint32_t index = 0U; index < length; ++index) {
        const volatile uint32_t *sample = (const volatile uint32_t *)((uintptr_t)instance + 0x1CU + index * 4U);
        const uint32_t result_word = *sample & 0xFFFU;
        memcpy(data + index, &result_word, 2U);
    }
    return NEON_OK;
}
