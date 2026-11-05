/**
 * @file neon_gpio.c
 * @brief Implementation for GPIO.
 * @details
 * * platform/weact_f412/neon_gpio.c - REAL implementation for STM32F412 GPIO (MODER/BSRR/IDR/ODR -
 * same layout register as STM32F0/L0, see mcu-description/devices/WEACT_F412.yaml). Same API as
 * platform/fu2/neon_gpio.c/neon_gpio_nucleo_l031k6.c - app code (e.g. Blinky) called identical, no
 * matter what chip it's running on. See ARCHITECTURE.md.
 *
 * IMPORTANT: neon_gpio_clock_enable() uses RCC->AHB1ENR (bit GPIOBEN=1) - NOT THE previous two boards:
 * neither RCC->AHBENR (STM32F0) nor RCC->IOPENR (STM32L0). Confirm via real source
 * STMicroelectronics/cmsis_device_f4 (RCC_AHB1ENR_GPIOBEN), NOT deduced from the 2 lines STM32 did
 * earlier.
 */

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/

#include "gpio/neon_gpio.h"

/*==================================================================================================
*                                         GLOBAL FUNCTIONS
==================================================================================================*/

/** @copydoc neon_gpio_get_default_config */
void neon_gpio_get_default_config(neon_gpio_config_t *config) {
    if (config == 0) {
        return;
    }

    config->pin = NEON_GPIO_PIN_ALL;
    config->mode = NEON_GPIO_MODE_INPUT;
    config->pull = NEON_GPIO_PULL_NONE;
    config->speed = NEON_GPIO_SPEED_LOW;
    config->schmitt = NEON_GPIO_SCHMITT_DISABLE;
    config->slew_rate = NEON_GPIO_SLEW_SLOW;
    config->input_sync = NEON_GPIO_INPUT_SYNC_DIRECT;
    config->interrupt = NEON_GPIO_INTERRUPT_DISABLE;
    config->initial_state = NEON_GPIO_PIN_RESET;
}

/** @copydoc neon_gpio_deinit */
neon_status_t neon_gpio_deinit(GPIO_TypeDef *instance, uint32_t pin_mask) {
    (void)instance;
    (void)pin_mask;
    return NEON_NOT_SUPPORTED;
}

/** @copydoc neon_gpio_set_interrupt_enabled */
neon_status_t neon_gpio_set_interrupt_enabled(GPIO_TypeDef *instance, uint32_t pin_mask, uint8_t enable) {
    (void)instance;
    (void)pin_mask;
    (void)enable;
    return NEON_NOT_SUPPORTED;
}

/** @copydoc neon_gpio_get_raw_interrupt_status */
neon_status_t neon_gpio_get_raw_interrupt_status(GPIO_TypeDef *instance, uint32_t *status) {
    (void)instance;
    (void)status;
    return NEON_NOT_SUPPORTED;
}

/** @copydoc neon_gpio_get_interrupt_status */
neon_status_t neon_gpio_get_interrupt_status(GPIO_TypeDef *instance, uint32_t *status) {
    (void)instance;
    (void)status;
    return NEON_NOT_SUPPORTED;
}

/** @copydoc neon_gpio_clear_interrupt */
neon_status_t neon_gpio_clear_interrupt(GPIO_TypeDef *instance, uint32_t pin_mask) {
    (void)instance;
    (void)pin_mask;
    return NEON_NOT_SUPPORTED;
}

/** @copydoc neon_gpio_set_interrupt_injection */
neon_status_t neon_gpio_set_interrupt_injection(GPIO_TypeDef *instance, uint32_t pin_mask, uint8_t enable) {
    (void)instance;
    (void)pin_mask;
    (void)enable;
    return NEON_NOT_SUPPORTED;
}

/** @copydoc neon_gpio_clock_enable */
neon_status_t neon_gpio_clock_enable(GPIO_TypeDef *instance) {
    /* Only GPIOB is declared in WEACT_F412.yaml (enough for LED PB2) - no GPIOA yet, so it compares with
   * GPIOB. */
    if (instance == GPIOB) {
        RCC->AHB1ENR |= (1U << 1); /* bit 1 = GPIOBEN (GPIO port B clock enable) */
        return NEON_OK;
    }

    return NEON_INVALID_PARAM;
}

/** @copydoc neon_gpio_init */
neon_status_t neon_gpio_init(GPIO_TypeDef *instance, const neon_gpio_config_t *init) {
    if (instance == 0 || init == 0 || init->pin == 0U) {
        return NEON_INVALID_PARAM;
    }

    if ((init->mode > NEON_GPIO_MODE_OUTPUT) || (init->pull != NEON_GPIO_PULL_NONE) ||
        (init->speed != NEON_GPIO_SPEED_LOW) || (init->schmitt != NEON_GPIO_SCHMITT_DISABLE) ||
        (init->slew_rate != NEON_GPIO_SLEW_SLOW) || (init->input_sync != NEON_GPIO_INPUT_SYNC_DIRECT) ||
        (init->interrupt != NEON_GPIO_INTERRUPT_DISABLE) || (init->initial_state > NEON_GPIO_PIN_SET)) {
        return NEON_NOT_SUPPORTED;
    }

    /* BSRR flashes the output level before MODER turns on the output to avoid glitch. */
    if (init->mode == NEON_GPIO_MODE_OUTPUT) {
        instance->BSRR = (init->initial_state == NEON_GPIO_PIN_SET) ? init->pin : (init->pin << 16U);
    }

    /* MODER: 2 bit/pin (00=input, 01=output) - has to handle each battery separately because init->battery
   * can be bitmask multiple batteries at the same time. */
    for (uint32_t pin = 0; pin < 16U; pin++) {
        if ((init->pin & (1U << pin)) == 0U) {
            continue;
        }
        uint32_t moder = instance->MODER;
        moder &= ~(3U << (pin * 2U));
        if (init->mode == NEON_GPIO_MODE_OUTPUT) {
            moder |= (1U << (pin * 2U)); /* 01 = general purpose output */
        }
        /* NEON_GPIO_MODE_INPUT -> 00, no need to set anything more after clear */
        instance->MODER = moder;
    }
    return NEON_OK;
}

/** @copydoc neon_gpio_write_pin */
void neon_gpio_write_pin(GPIO_TypeDef *instance, uint32_t pin_mask, uint8_t state) {
    if (state) {
        instance->BSRR = pin_mask; /* bit[15:0] of BSRR = SET */
    } else {
        instance->BSRR = (pin_mask << 16); /* bit[31:16] of BSRR = RESET */
    }
}

/** @copydoc neon_gpio_read_pin */
uint8_t neon_gpio_read_pin(GPIO_TypeDef *instance, uint32_t pin_mask) {
    return (instance->IDR & pin_mask) ? 1U : 0U;
}

/** @copydoc neon_gpio_toggle_pin */
void neon_gpio_toggle_pin(GPIO_TypeDef *instance, uint32_t pin_mask) {
    if (instance->ODR & pin_mask) {
        instance->BSRR = (pin_mask << 16); /* high entry -> RESET */
    } else {
        instance->BSRR = pin_mask; /* item thap -> SET */
    }
}
