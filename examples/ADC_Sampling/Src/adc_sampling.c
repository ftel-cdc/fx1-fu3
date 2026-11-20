/**
 * @file adc_sampling.c
 * @brief ADC batch sampling and range checks.
 * @details
 * Samples remain in aligned word storage throughout CPU processing.
 */

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include "adc_sampling.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "adc/neon_adc.h"
#include "crm/neon_crm.h"
/* Set to the ADC input channel connected by the FPGA design. */
#ifndef ADC_SAMPLING_CHANNEL
#define ADC_SAMPLING_CHANNEL 0U
#endif

#define ADC_SAMPLING_SAMPLE_SLOTS 16U
#define ADC_SAMPLING_TIMEOUT_POLLS 1000000U

/*==================================================================================================
*                                         GLOBAL FUNCTIONS
==================================================================================================*/

static bool adc_read_batch(uint32_t length, uint32_t *samples) {
    neon_adc_config_t config;
    neon_status_t status;
    uint32_t min_value = 0xFFFFU;
    uint32_t max_value = 0U;
    uint32_t sum = 0U;

    neon_adc_get_default_config(&config);
    config.channel = ADC_SAMPLING_CHANNEL;
    config.sample_count = ADC_SAMPLING_SAMPLE_SLOTS;

    status = neon_adc_init(ADC, &config);
    if (status != NEON_OK) {
        printf("[ADC][FAIL] init failed: status=%d\n", (int)status);
        return false;
    }

    status = neon_adc_start_conversion(ADC);
    if (status != NEON_OK) {
        printf("[ADC][FAIL] start conversion failed: status=%d\n", (int)status);
        (void)neon_adc_deinit(ADC);
        return false;
    }

    status = neon_adc_read_samples(ADC, (uint16_t *)samples, length, ADC_SAMPLING_TIMEOUT_POLLS);
    if (status != NEON_OK) {
        printf("[ADC][FAIL] read batch of %lu sample(s): status=%d\n", (unsigned long)length, (int)status);
        (void)neon_adc_deinit(ADC);
        return false;
    }

    printf("[ADC] channel=%u, batch=%lu: ", (unsigned int)config.channel, (unsigned long)length);
    for (uint32_t i = 0U; i < length; ++i) {
        const uint32_t value = (samples[i / 2U] >> ((i % 2U) * 16U)) & 0xFFFFU;

        printf("%u%s", (unsigned int)value, (i + 1U == length) ? "" : ", ");
        if (value > 4095U) {
            printf("\n[ADC][FAIL] sample[%lu]=%u is outside 12-bit range.\n", (unsigned long)i, (unsigned int)value);
            (void)neon_adc_deinit(ADC);
            return false;
        }
        if (value < min_value) {
            min_value = value;
        }
        if (value > max_value) {
            max_value = value;
        }
        sum += value;
    }
    printf("\n[ADC] min=%u max=%u average=%lu\n", (unsigned int)min_value, (unsigned int)max_value,
           (unsigned long)(sum / length));

    status = neon_adc_deinit(ADC);
    if (status != NEON_OK) {
        printf("[ADC][FAIL] deinit failed: status=%d\n", (int)status);
        return false;
    }
    return true;
}

void adc_sampling_run(void) {
    /* The driver reads complete aligned 32-bit words for uint16_t samples. */
    uint32_t sample_storage[ADC_SAMPLING_SAMPLE_SLOTS / 2U] = {0U};
    static const uint32_t batch_lengths[] = {1U, 4U, 16U};
    const uint32_t batch_count = (uint32_t)(sizeof(batch_lengths) / sizeof(batch_lengths[0]));
    bool passed = true;
    neon_status_t status;

    printf("\n=== ADC Sampling Test ===\n");
    printf("Channel=%u, result range=0..4095, timeout=%lu polls\n", (unsigned int)ADC_SAMPLING_CHANNEL,
           (unsigned long)ADC_SAMPLING_TIMEOUT_POLLS);

    status = neon_crm_peripheral_clock_enable(NEON_CRM_PERIPHERAL_ADC);
    if (status != NEON_OK) {
        printf("[ADC][FAIL] CRM clock enable failed: status=%d\n", (int)status);
        return;
    }

    status = neon_crm_peripheral_reset(NEON_CRM_PERIPHERAL_ADC);
    if (status != NEON_OK) {
        printf("[ADC][FAIL] CRM reset failed: status=%d\n", (int)status);
        (void)neon_crm_peripheral_clock_disable(NEON_CRM_PERIPHERAL_ADC);
        return;
    }

    for (uint32_t i = 0U; i < batch_count; ++i) {
        if (!adc_read_batch(batch_lengths[i], sample_storage)) {
            passed = false;
            break;
        }
    }

    status = neon_crm_peripheral_clock_disable(NEON_CRM_PERIPHERAL_ADC);
    if (status != NEON_OK) {
        printf("[ADC][FAIL] CRM clock disable failed: status=%d\n", (int)status);
        passed = false;
    }

    printf("%s\n", passed ? "[ADC][PASS] all sample batches completed." : "[ADC][FAIL] sampling test failed.");
}
