/**
 * @file neon_adc.h
 * @brief Public declarations for ADC.
 * @details
 * GT32U2086 23-register ADC controller: 12-bit samples, up to 16 sample slots.
 * @note Blocking polling/configuration operations are intended for task context.
 * Serialize configuration/teardown with other users of the same peripheral;
 * timeout_polls values are polling budgets, not milliseconds, where applicable.
 */

#ifndef __NEON_ADC_H__
#define __NEON_ADC_H__

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
    uint32_t channel;           /* 0..15 */
    uint32_t sample_source;     /* CFG.SAMP_SEL: 0 or 1 */
    uint32_t sample_count;      /* 1..16 */
    uint32_t sample_clock_low;  /* TIMING.SAMPCLK_LOW, at least 12 cycles */
    uint32_t sample_clock_high; /* TIMING.SAMPCLK_HIGH, at least 4 cycles */
    uint32_t setup_wait;        /* TIMING.SETUP_WAIT, at least 32 cycles */
} neon_adc_config_t;

/*==================================================================================================
*                                       FUNCTION PROTOTYPES
==================================================================================================*/
/**
 * @brief Fill the default configuration for ADC.
 * @param[out] config Module configuration.
 */
void neon_adc_get_default_config(neon_adc_config_t *config);
/**
 * @brief Initialize ADC.
 * @param[in] instance Peripheral register instance.
 * @param[in] init Initialization settings.
 * @return NEON_OK on success, or a module error/status code.
 */
neon_status_t neon_adc_init(ADC_TypeDef *instance, const neon_adc_config_t *init);
/**
 * @brief Deinitialize ADC.
 * @param[in] instance Peripheral register instance.
 * @return NEON_OK on success, or a module error/status code.
 */
neon_status_t neon_adc_deinit(ADC_TypeDef *instance);
/**
 * @brief Start ADC conversion.
 * @param[in] instance Peripheral register instance.
 * @return NEON_OK on success, or a module error/status code.
 */
neon_status_t neon_adc_start_conversion(ADC_TypeDef *instance);
/**
 * @brief Read ADC samples.
 * @param[in] instance Peripheral register instance.
 * @param[out] data Transfer data; GT32U2086 RAM buffers must own their complete aligned 32-bit words.
 * @param[in] length Number of elements to process.
 * @param[in] timeout_polls Polling iteration budget; this value is not milliseconds.
 * @return NEON_OK on success, or a module error/status code.
 * @details
 * Wait for DONE/READY and read length sample slots (1..sample_count).
 * Destination must own the complete aligned words containing its uint16_t data.
 * StartConversion must be called again to capture a new batch.
 * timeout_polls is a poll budget, not milliseconds; zero only checks readiness.
 */
neon_status_t neon_adc_read_samples(ADC_TypeDef *instance, uint16_t *data, uint32_t length, uint32_t timeout_polls);
#ifdef __cplusplus
}
#endif
#endif /* __NEON_ADC_H__ */
