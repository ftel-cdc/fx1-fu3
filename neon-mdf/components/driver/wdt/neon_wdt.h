/**
 * @file neon_wdt.h
 * @brief Public declarations for watchdog.
 * @details
 * FU2 watchdog: timeout_polls range, reset/interrupt response and refresh.
 * @note Blocking polling/configuration operations are intended for task context.
 * Serialize configuration/teardown with other users of the same peripheral;
 * timeout_polls values are polling budgets, not milliseconds, where applicable.
 */

#ifndef __NEON_WDT_H__
#define __NEON_WDT_H__

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
typedef enum {
    NEON_WDT_RESPONSE_RESET = 0,
    NEON_WDT_RESPONSE_INTERRUPT = 1,
} neon_wdt_response_t;

typedef struct {
    uint32_t timeout_range;      /* Raw TORR.TOP; depends on the watchdog clock. */
    uint32_t reset_pulse_length; /* CR.RPL, 0..255. */
    neon_wdt_response_t response;
} neon_wdt_config_t;

/*==================================================================================================
*                                       FUNCTION PROTOTYPES
==================================================================================================*/
/**
 * @brief Fill the default configuration for watchdog.
 * @param[out] config Module configuration.
 */
void neon_wdt_get_default_config(neon_wdt_config_t *config);
/**
 * @brief Initialize watchdog.
 * @param[in] instance Peripheral register instance.
 * @param[in] config Module configuration.
 * @return NEON_OK on success, or a module error/status code.
 * @details
 * config enables the CRM clock, resets the block and starts the watchdog.
 * In interrupt mode the application enables WDT_IRQn and supplies its ISR.
 */
neon_status_t neon_wdt_init(WDT_TypeDef *instance, const neon_wdt_config_t *config);
/**
 * @brief Deinitialize watchdog.
 * @param[in] instance Peripheral register instance.
 * @return NEON_OK on success, or a module error/status code.
 */
neon_status_t neon_wdt_deinit(WDT_TypeDef *instance);
/**
 * @brief Wdt refresh.
 * @param[in] instance Peripheral register instance.
 * @return NEON_OK on success, or a module error/status code.
 */
neon_status_t neon_wdt_refresh(WDT_TypeDef *instance);
/**
 * @brief Read watchdog counter.
 * @param[in] instance Peripheral register instance.
 * @param[in,out] counter Counter.
 * @return NEON_OK on success, or a module error/status code.
 */
neon_status_t neon_wdt_get_counter(WDT_TypeDef *instance, uint32_t *counter);
/**
 * @brief Check watchdog interrupt pending.
 * @param[in] instance Peripheral register instance.
 * @param[out] pending Receives whether an interrupt is pending.
 * @return NEON_OK on success, or a module error/status code.
 */
neon_status_t neon_wdt_is_interrupt_pending(WDT_TypeDef *instance, uint8_t *pending);
/**
 * @brief Clear watchdog interrupt.
 * @param[in] instance Peripheral register instance.
 * @return NEON_OK on success, or a module error/status code.
 * @details
 * Reads EOI once to acknowledge the watchdog interrupt.
 */
neon_status_t neon_wdt_clear_interrupt(WDT_TypeDef *instance);

#ifdef __cplusplus
}
#endif
#endif /* __NEON_WDT_H__ */
