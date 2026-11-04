/**
 * @file neon_gpio.c
 * @brief Implementation for GPIO.
 * @details
 * * platform/stm32f103/neon_gpio.c - GPIO driver cho STM32F103.
 *
 * STM32F1 dung CRL/CRH (4 bit cho moi pin), khac GPIO MODER cua STM32F4:
 *   0x2 = CNF=00, MODE=10: general-purpose output push-pull, toi da 2 MHz
 *   0x4 = CNF=01, MODE=00: floating input
 *
 * Tham chieu: RM0008 muc 7.3.7 (RCC_APB2ENR), muc 9.2.1/9.2.2 va
 * Table 20 (cau hinh GPIO), muc 9.2.3-9.2.5 (IDR/ODR/BSRR).
 */

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/

#include "gpio/neon_gpio.h"

/*==================================================================================================
*                                        DEFINES AND MACROS
==================================================================================================*/
#define GPIO_STM32F103_GPIO_PIN_MASK 0xFFFFU
#define GPIO_STM32F103_GPIO_OUTPUT_PP_2MHZ 0x2U
#define GPIO_STM32F103_GPIO_INPUT_FLOATING 0x4U

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
    uint32_t clock_mask = 0U;

    /* RCC_APB2ENR: IOPAEN=bit 2, IOPBEN=bit 3, IOPCEN=bit 4, IOPDEN=bit 5. */
    if (instance == GPIOA) {
        clock_mask = (1U << 2);
    } else if (instance == GPIOB) {
        clock_mask = (1U << 3);
    } else if (instance == GPIOC) {
        clock_mask = (1U << 4);
    } else if (instance == GPIOD) {
        clock_mask = (1U << 5);
    }

    if (clock_mask == 0U) {
        return NEON_INVALID_PARAM;
    }

    RCC->APB2ENR |= clock_mask;
    return NEON_OK;
}

/** @copydoc neon_gpio_init */
neon_status_t neon_gpio_init(GPIO_TypeDef *instance, const neon_gpio_config_t *init) {
    uint32_t configuration;
    uint32_t crl;
    uint32_t crh;
    uint32_t selected_pins;

    if (instance == 0 || init == 0 || init->pin == 0U) {
        return NEON_INVALID_PARAM;
    }

    /* Các thuộc tính mở rộng chưa được port cho backend này. Không được âm
   * thầm báo OK nếu phần cứng chưa nhận cấu hình mà ứng dụng yêu cầu. */
    if ((init->pull != NEON_GPIO_PULL_NONE) || (init->speed != NEON_GPIO_SPEED_LOW) ||
        (init->schmitt != NEON_GPIO_SCHMITT_DISABLE) || (init->slew_rate != NEON_GPIO_SLEW_SLOW) ||
        (init->input_sync != NEON_GPIO_INPUT_SYNC_DIRECT) || (init->interrupt != NEON_GPIO_INTERRUPT_DISABLE) ||
        (init->initial_state > NEON_GPIO_PIN_SET)) {
        return NEON_NOT_SUPPORTED;
    }

    if (init->mode == NEON_GPIO_MODE_OUTPUT) {
        configuration = GPIO_STM32F103_GPIO_OUTPUT_PP_2MHZ;
    } else if (init->mode == NEON_GPIO_MODE_INPUT) {
        configuration = GPIO_STM32F103_GPIO_INPUT_FLOATING;
    } else {
        return NEON_INVALID_PARAM;
    }

    selected_pins = init->pin & GPIO_STM32F103_GPIO_PIN_MASK;

    /* BSRR nạp mức output trước khi đổi CRL/CRH sang output để tránh glitch. */
    if (init->mode == NEON_GPIO_MODE_OUTPUT) {
        instance->BSRR = (init->initial_state == NEON_GPIO_PIN_SET) ? selected_pins : (selected_pins << 16U);
    }

    crl = instance->CRL;
    crh = instance->CRH;

    for (uint32_t pin = 0U; pin < 16U; pin++) {
        uint32_t shift;

        if ((selected_pins & (1U << pin)) == 0U) {
            continue;
        }

        if (pin < 8U) {
            shift = pin * 4U;
            crl = (crl & ~(0xFU << shift)) | (configuration << shift);
        } else {
            shift = (pin - 8U) * 4U;
            crh = (crh & ~(0xFU << shift)) | (configuration << shift);
        }
    }

    if ((selected_pins & 0x00FFU) != 0U) {
        instance->CRL = crl;
    }
    if ((selected_pins & 0xFF00U) != 0U) {
        instance->CRH = crh;
    }

    return NEON_OK;
}

/** @copydoc neon_gpio_write_pin */
void neon_gpio_write_pin(GPIO_TypeDef *instance, uint32_t pin_mask, uint8_t state) {
    pin_mask &= GPIO_STM32F103_GPIO_PIN_MASK;

    if (state != 0U) {
        instance->BSRR = pin_mask; /* BSRR[15:0]: set pin. */
    } else {
        instance->BSRR = pin_mask << 16U; /* BSRR[31:16]: reset pin. */
    }
}

/** @copydoc neon_gpio_read_pin */
uint8_t neon_gpio_read_pin(GPIO_TypeDef *instance, uint32_t pin_mask) {
    pin_mask &= GPIO_STM32F103_GPIO_PIN_MASK;
    return ((instance->IDR & pin_mask) != 0U) ? 1U : 0U;
}

/** @copydoc neon_gpio_toggle_pin */
void neon_gpio_toggle_pin(GPIO_TypeDef *instance, uint32_t pin_mask) {
    uint32_t output_state;
    uint32_t pins_to_set;
    uint32_t pins_to_reset;

    pin_mask &= GPIO_STM32F103_GPIO_PIN_MASK;
    output_state = instance->ODR;
    pins_to_reset = output_state & pin_mask;
    pins_to_set = (~output_state) & pin_mask;

    /* Mot lan ghi BSRR: pin dang 0 duoc set, pin dang 1 duoc reset. */
    instance->BSRR = pins_to_set | (pins_to_reset << 16U);
}
