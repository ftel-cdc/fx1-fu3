/**
 * @file neon_gpio.c
 * @brief Implementation for GPIO.
 * @details
 * * platform/ch32v003/neon_gpio.c - implementation THAT cho CH32V003 GPIO (CFGLR/INDR/
 * OUTDR/BSHR/BCR - kieu STM32F1, THAT tu CH32V003RM.PDF V1.9 chuong 7, xem
 * mcu-description/devices/CH32V003F4P6.yaml). Cung API voi
 * platform/weact_f412/neon_gpio.c/platform/fu2/neon_gpio.c - app code (vd Blinky) goi giong
 * het nhau.
 *
 * CFGLR: 4 bit/pin (CNFy[1:0] + MODEy[1:0], y=0-7, KHONG co CFGHR vi chip
 * nay toi da 8 chan/port) - gia tri nibble dung trong file nay:
 *   0x1 = MODE=01 (output, 10MHz), CNF=00 (general push-pull output)
 *   0x4 = MODE=00 (input), CNF=01 (floating input) - KHOP voi reset value
 *         0x44444444 that cua CFGLR (moi pin floating input mac dinh sau
 *         reset) - xac nhan cheo voi RM, khong doan.
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
    /* RCC->APB2PCENR bit2=IOPAEN, bit4=IOPCEN, bit5=IOPDEN - THAT, RM 3.4.7. */
    if (instance == GPIOA) {
        RCC->APB2PCENR |= (1U << 2);
    } else if (instance == GPIOC) {
        RCC->APB2PCENR |= (1U << 4);
    } else if (instance == GPIOD) {
        RCC->APB2PCENR |= (1U << 5);
    } else {
        return NEON_INVALID_PARAM;
    }

    return NEON_OK;
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

    /* Nạp OUTDR gián tiếp trước khi CFGLR bật output để tránh glitch. */
    if (init->mode == NEON_GPIO_MODE_OUTPUT) {
        if (init->initial_state == NEON_GPIO_PIN_SET) {
            instance->BSHR = init->pin;
        } else {
            instance->BCR = init->pin;
        }
    }

    for (uint32_t pin = 0; pin < 8U; pin++) {
        if ((init->pin & (1U << pin)) == 0U) {
            continue;
        }
        uint32_t cfglr = instance->CFGLR;
        cfglr &= ~(0xFU << (pin * 4U));
        if (init->mode == NEON_GPIO_MODE_OUTPUT) {
            cfglr |= (0x1U << (pin * 4U)); /* CNF=00 push-pull, MODE=01 output 10MHz */
        } else {
            cfglr |= (0x4U << (pin * 4U)); /* CNF=01 floating input, MODE=00 */
        }
        instance->CFGLR = cfglr;
    }
    return NEON_OK;
}

/** @copydoc neon_gpio_write_pin */
void neon_gpio_write_pin(GPIO_TypeDef *instance, uint32_t pin_mask, uint8_t state) {
    /* BSHR = set (THAT, RM 7.3.1.4), BCR = reset (THAT, RM 7.3.1.5) - 2 thanh
   * ghi rieng, khac BSRR gop chung 1 thanh ghi 32-bit cua STM32F4. */
    if (state) {
        instance->BSHR = pin_mask;
    } else {
        instance->BCR = pin_mask;
    }
}

/** @copydoc neon_gpio_read_pin */
uint8_t neon_gpio_read_pin(GPIO_TypeDef *instance, uint32_t pin_mask) {
    return (instance->INDR & pin_mask) ? 1U : 0U;
}

/** @copydoc neon_gpio_toggle_pin */
void neon_gpio_toggle_pin(GPIO_TypeDef *instance, uint32_t pin_mask) {
    if (instance->OUTDR & pin_mask) {
        instance->BCR = pin_mask;
    } else {
        instance->BSHR = pin_mask;
    }
}
