/**
 * @file neon_gpio.h
 * @brief Public declarations for GPIO.
 * @details
 * * NEON GPIO - HAL GPIO is used for Neon MDF targets.
 *
 * The API uses only logical concepts such as input/output, pull and speed. The backend of each chip is
 * responsible for porting them to the real register/mux. With FU2, neon_gpio_init() simultaneously
 * configure GPIO and PAD/SYSCSR; the application does not need to know a GPIO pin corresponding to
 * mux value 0, 1 or 2.
 * @note Blocking polling/configuration operations are intended for task context.
 * Serialize configuration/teardown with other users of the same peripheral;
 * timeout_polls values are polling budgets, not milliseconds, where applicable.
 */

#ifndef __NEON_GPIO_H__
#define __NEON_GPIO_H__

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include "neon_types.h"
#include "mcu_device.h"

#ifdef __cplusplus
extern "C" {
#endif

/*==================================================================================================
*                                        DEFINES AND MACROS
==================================================================================================*/
#define NEON_GPIO_PIN_0 (1UL << 0U)
#define NEON_GPIO_PIN_1 (1UL << 1U)
#define NEON_GPIO_PIN_2 (1UL << 2U)
#define NEON_GPIO_PIN_3 (1UL << 3U)
#define NEON_GPIO_PIN_4 (1UL << 4U)
#define NEON_GPIO_PIN_5 (1UL << 5U)
#define NEON_GPIO_PIN_6 (1UL << 6U)
#define NEON_GPIO_PIN_7 (1UL << 7U)
#define NEON_GPIO_PIN_8 (1UL << 8U)
#define NEON_GPIO_PIN_9 (1UL << 9U)
#define NEON_GPIO_PIN_10 (1UL << 10U)
#define NEON_GPIO_PIN_11 (1UL << 11U)
#define NEON_GPIO_PIN_12 (1UL << 12U)
#define NEON_GPIO_PIN_13 (1UL << 13U)
#define NEON_GPIO_PIN_14 (1UL << 14U)
#define NEON_GPIO_PIN_15 (1UL << 15U)

/* FU2 and CH32V003 have 8 bit/port; the currently supported STM32 has 16 bit/port. */
#if defined(FU2) || defined(CH32V003F4P6)
#define NEON_GPIO_PIN_ALL (0xFFUL)
#else
#define NEON_GPIO_PIN_ALL (0xFFFFUL)
#endif

/*==================================================================================================
*                                  STRUCTURES AND OTHER TYPEDEFS
==================================================================================================*/
typedef enum {
    NEON_GPIO_MODE_INPUT = 0,
    NEON_GPIO_MODE_OUTPUT = 1,
} neon_gpio_mode_t;

typedef enum {
    NEON_GPIO_PULL_NONE = 0,
    NEON_GPIO_PULL_UP = 1,
    NEON_GPIO_PULL_DOWN = 2,
} neon_gpio_pull_t;

typedef enum {
    NEON_GPIO_SPEED_LOW = 0,
    NEON_GPIO_SPEED_MEDIUM = 1,
    NEON_GPIO_SPEED_HIGH = 2,
    NEON_GPIO_SPEED_VERY_HIGH = 3,
} neon_gpio_speed_t;

typedef enum {
    NEON_GPIO_SCHMITT_DISABLE = 0,
    NEON_GPIO_SCHMITT_ENABLE = 1,
} neon_gpio_schmitt_t;

typedef enum {
    NEON_GPIO_SLEW_SLOW = 0,
    NEON_GPIO_SLEW_FAST = 1,
} neon_gpio_slew_rate_t;

typedef enum {
    NEON_GPIO_INPUT_SYNC_DIRECT = 0,
    NEON_GPIO_INPUT_SYNC_ENABLE = 1,
} neon_gpio_input_sync_t;

typedef enum {
    NEON_GPIO_INTERRUPT_DISABLE = 0,
    /* FU2 plays interrupt when the filtered input moves to a rising edge. */
    NEON_GPIO_INTERRUPT_ENABLE = 1,
} neon_gpio_interrupt_t;

typedef enum {
    NEON_GPIO_PIN_RESET = 0,
    NEON_GPIO_PIN_SET = 1,
} neon_gpio_pin_state_t;

typedef struct {
    uint32_t pin;                    /* Bitmask the pins to configure. */
    neon_gpio_mode_t mode;           /* Input or output direction. */
    neon_gpio_pull_t pull;           /* No pull, pull-up or pull-down. */
    neon_gpio_speed_t speed;         /* The relative drive level of the pad. */
    neon_gpio_schmitt_t schmitt;     /* Input line schmitt. */
    neon_gpio_slew_rate_t slew_rate; /* speed output level switch. */
    /* when interrupt=ENABLE, FU2 backend is always on synchronously because hardware requires CTL.SYNC_EN
   * for edge filtering and detection. */
    neon_gpio_input_sync_t input_sync;   /* Synchronize input to GPIO clock. */
    neon_gpio_interrupt_t interrupt;     /* Allow interrupt of selected batteries. */
    neon_gpio_pin_state_t initial_state; /* Output level before enabling output. */
} neon_gpio_config_t;

/*==================================================================================================
*                                       FUNCTION PROTOTYPES
==================================================================================================*/
/**
 * @brief Fill the default configuration for GPIO.
 * @param[out] config Module configuration.
 * @details
 * Fill in the default secure configuration LL_GPIO_StructInit(): full battery, input, no-pull, low
 * drive, schmitt off, slow slew, direct input and interrupt off. The default initial_state is
 * RESET. Call first and only edit fields to be used.
 */
void neon_gpio_get_default_config(neon_gpio_config_t *config);

/**
 * @brief enable the GPIO peripheral clock and release reset where required.
 * @param[in] instance Peripheral register instance.
 * @return NEON_OK on success, or a module error/status code.
 * @details
 * Clock is not enabled implicitly in config to keep the same flow on all targets. Backend FU2
 * simultaneously releases GPIO reset when clock is enabled.
 */
neon_status_t neon_gpio_clock_enable(GPIO_TypeDef *instance);

/**
 * @brief Configure GPIO direction, pin routing and supported pad properties.
 * @param[in] instance Peripheral register instance.
 * @param[in] init Initialization settings.
 * @return NEON_OK on success, or a module error/status code.
 * @details
 * On FU2, PA0..PA4 share the JTAG route. config one of these pins releases JTAG before selecting GPIO;
 * if JTAG mux is locked in on state, function returns NEON_LOCKED and no configuration changes are
 * seen GPIO/PAD.
 *
 * With die-pad index 0..11, the config applies both mux and power configuration of the PAD. The new VP
 * has not yet fully published encoding PAD_CTRL5..7 for index 12..23, so these pins are still
 * selected mux GPIO but keep power configuration reset.
 */
neon_status_t neon_gpio_init(GPIO_TypeDef *instance, const neon_gpio_config_t *init);
/**
 * @brief Deinitialize GPIO.
 * @param[in] instance Peripheral register instance.
 * @param[in] pin_mask Bitmask of the selected GPIO pins.
 * @return NEON_OK on success, or a module error/status code.
 */
neon_status_t neon_gpio_deinit(GPIO_TypeDef *instance, uint32_t pin_mask);

/**
 * @brief Write GPIO pin.
 * @param[in] instance Peripheral register instance.
 * @param[in] pin_mask Bitmask of the selected GPIO pins.
 * @param[in] state Requested logical state.
 */
void neon_gpio_write_pin(GPIO_TypeDef *instance, uint32_t pin_mask, uint8_t state);
/**
 * @brief Read GPIO pin.
 * @param[in] instance Peripheral register instance.
 * @param[in] pin_mask Bitmask of the selected GPIO pins.
 * @return The requested value or operation result.
 */
uint8_t neon_gpio_read_pin(GPIO_TypeDef *instance, uint32_t pin_mask);
/**
 * @brief Toggle GPIO pin.
 * @param[in] instance Peripheral register instance.
 * @param[in] pin_mask Bitmask of the selected GPIO pins.
 */
void neon_gpio_toggle_pin(GPIO_TypeDef *instance, uint32_t pin_mask);

/**
 * @brief Set GPIO interrupt enabled.
 * @param[in] instance Peripheral register instance.
 * @param[in] pin_mask Bitmask of the selected GPIO pins.
 * @param[in] enable Nonzero to enable, zero to disable.
 * @return NEON_OK on success, or a module error/status code.
 * @details
 * API interrupt FU2 detects rising edge after filter/synchronizer. Backend not yet supported on other
 * target return NEON_NOT_SUPPORTED see because silently ignored.
 */
neon_status_t neon_gpio_set_interrupt_enabled(GPIO_TypeDef *instance, uint32_t pin_mask, uint8_t enable);
/**
 * @brief Read GPIO raw interrupt status.
 * @param[in] instance Peripheral register instance.
 * @param[out] status Destination for the status snapshot.
 * @return NEON_OK on success, or a module error/status code.
 */
neon_status_t neon_gpio_get_raw_interrupt_status(GPIO_TypeDef *instance, uint32_t *status);
/**
 * @brief Read GPIO interrupt status.
 * @param[in] instance Peripheral register instance.
 * @param[out] status Destination for the status snapshot.
 * @return NEON_OK on success, or a module error/status code.
 */
neon_status_t neon_gpio_get_interrupt_status(GPIO_TypeDef *instance, uint32_t *status);
/**
 * @brief Clear GPIO interrupt.
 * @param[in] instance Peripheral register instance.
 * @param[in] pin_mask Bitmask of the selected GPIO pins.
 * @return NEON_OK on success, or a module error/status code.
 * @details
 * erase both raw status and software injection source of selected batteries.
 */
neon_status_t neon_gpio_clear_interrupt(GPIO_TypeDef *instance, uint32_t pin_mask);
/**
 * @brief Set GPIO interrupt injection.
 * @param[in] instance Peripheral register instance.
 * @param[in] pin_mask Bitmask of the selected GPIO pins.
 * @param[in] enable Nonzero to enable, zero to disable.
 * @return NEON_OK on success, or a module error/status code.
 */
neon_status_t neon_gpio_set_interrupt_injection(GPIO_TypeDef *instance, uint32_t pin_mask, uint8_t enable);

#ifdef __cplusplus
}
#endif

#endif /* __NEON_GPIO_H__ */
