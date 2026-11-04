/**
 * @file neon_gpio.c
 * @brief Implementation for GPIO.
 * @details
 * * NEON GPIO backend for FU2.
 *
 * Register GPIO, mapping physical pin and reset behavior taken according to FU2 LL. Other LL, public
 * API does not receive raw mux value: config always requires the GPIO function and backend SYSCSR
 * manually selects the proper mux of each physical pin.
 */

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/

#include "gpio/neon_gpio.h"
#include <string.h>

#include "crm/neon_crm.h"
#include "syscsr/platform/fu2/neon_syscsr_internal.h"

#include <stddef.h>

/*==================================================================================================
*                                        DEFINES AND MACROS
==================================================================================================*/
#define GPIO_PIN_MASK GPIO_OE_EN_Msk
#define GPIO_GPIOA_JTAG_PIN_MASK                                                                                       \
    (NEON_GPIO_PIN_0 | NEON_GPIO_PIN_1 | NEON_GPIO_PIN_2 | NEON_GPIO_PIN_3 | NEON_GPIO_PIN_4)

/*==================================================================================================
*                                   PRIVATE FUNCTION PROTOTYPES
==================================================================================================*/

static uint8_t gpio_is_valid_instance(GPIO_TypeDef const *instance);
static uint8_t gpio_is_valid_pin_mask(uint32_t pin_mask);
static uint8_t gpio_is_valid_config(const neon_gpio_config_t *config);
static neon_status_t gpio_claim_jtag_pins(GPIO_TypeDef const *instance, uint32_t pin_mask);
static uint32_t gpio_enter_critical(void);
static void gpio_exit_critical(uint32_t primask);
static neon_status_t gpio_get_crm_peripheral(GPIO_TypeDef const *instance, neon_crm_peripheral_t *peripheral);
static void gpio_make_pad_config(const neon_gpio_config_t *init, fu2_syscsr_pad_config_t *pad_config);

/*==================================================================================================
*                                         GLOBAL FUNCTIONS
==================================================================================================*/

/** @copydoc neon_gpio_get_default_config */
void neon_gpio_get_default_config(neon_gpio_config_t *config) {
    if (config == NULL) {
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

/** @copydoc neon_gpio_clock_enable */
neon_status_t neon_gpio_clock_enable(GPIO_TypeDef *instance) {
    neon_crm_peripheral_t peripheral;
    neon_status_t status;

    status = gpio_get_crm_peripheral(instance, &peripheral);
    if (status != NEON_OK) {
        return status;
    }

    status = neon_crm_peripheral_clock_enable(peripheral);
    if (status != NEON_OK) {
        return status;
    }

    /* ClockEnable must be sufficient for peripheral to work again both after each force reset software.
   * GPIOE/F does not have its own bit reset in CRM v8. */
    status = neon_crm_peripheral_release_reset(peripheral);
    return (status == NEON_NOT_SUPPORTED) ? NEON_OK : status;
}

/** @copydoc neon_gpio_init */
neon_status_t neon_gpio_init(GPIO_TypeDef *instance, const neon_gpio_config_t *init) {
    fu2_syscsr_pad_config_t pad_config;
    uint32_t pin_mask;
    neon_status_t status;
    uint32_t primask;

    if ((gpio_is_valid_instance(instance) == 0U) || (gpio_is_valid_config(init) == 0U)) {
        return NEON_INVALID_PARAM;
    }

    /* Confirm all pins selected are bonded pins and have GPIO route before writing any register. GPIOE has
   * E0/E1/E2/E4 according to IO_PAD_MAP; E3 has no bonded pins. GPIOF currently does not have a
   * bonded pin in the pin-mux panel FU2 v8 so the corresponding mask will be rejected. */
    memcpy(&pin_mask, &init->pin, sizeof(pin_mask));
    status = fu2_syscsr_validate_gpio_pins(instance, pin_mask);
    if (status != NEON_OK) {
        return status;
    }

    status = gpio_claim_jtag_pins(instance, pin_mask);
    if (status != NEON_OK) {
        return status;
    }

    gpio_make_pad_config(init, &pad_config);

    /* Disable output and interrupt before seeing PAD/mux. flash OUT while OE is off so that when output is
   * on, straight output pins initial_state see because of the old OUT level. */
    primask = gpio_enter_critical();
    instance->OE &= ~(pin_mask & GPIO_OE_EN_Msk);
    instance->INT_EN &= ~(pin_mask & GPIO_INT_EN_INTR_A_Msk);
    if (init->mode == NEON_GPIO_MODE_OUTPUT) {
        if (init->initial_state == NEON_GPIO_PIN_SET) {
            instance->OUT |= pin_mask & GPIO_OUT_OUT_Msk;
        } else {
            instance->OUT &= ~(pin_mask & GPIO_OUT_OUT_Msk);
        }
    }
    gpio_exit_critical(primask);

    status = fu2_syscsr_configure_gpio_pads(instance, pin_mask, &pad_config);
    if (status != NEON_OK) {
        return status;
    }

    status = fu2_syscsr_set_gpio_pins_function(instance, pin_mask);
    if (status != NEON_OK) {
        return status;
    }

    primask = gpio_enter_critical();

    if ((init->input_sync == NEON_GPIO_INPUT_SYNC_ENABLE) || (init->interrupt == NEON_GPIO_INTERRUPT_ENABLE)) {
        instance->CTL |= pin_mask & GPIO_CTL_SYNC_EN_Msk;
    } else {
        instance->CTL &= ~(pin_mask & GPIO_CTL_SYNC_EN_Msk);
    }

    if (init->interrupt == NEON_GPIO_INTERRUPT_ENABLE) {
        /* INT_STA = (RAW_INT | INT_INJ) & INT_EN. erase both old and new sources enable to not accept an IRQ
     * that existed before configuration. */
        instance->INT_INJ &= ~(pin_mask & GPIO_INT_INJ_INTR_A_Msk);
        instance->RAW_INT = pin_mask & GPIO_RAW_INT_INTR_A_Msk;
        instance->INT_EN |= pin_mask & GPIO_INT_EN_INTR_A_Msk;
    }

    /* OE is written last so that output only works after PAD/mux is complete. */
    if (init->mode == NEON_GPIO_MODE_OUTPUT) {
        instance->OE |= pin_mask & GPIO_OE_EN_Msk;
    }

    gpio_exit_critical(primask);
    return NEON_OK;
}

/** @copydoc neon_gpio_deinit */
neon_status_t neon_gpio_deinit(GPIO_TypeDef *instance, uint32_t pin_mask) {
    neon_status_t status;
    uint32_t primask;

    if ((gpio_is_valid_instance(instance) == 0U) || (gpio_is_valid_pin_mask(pin_mask) == 0U)) {
        return NEON_INVALID_PARAM;
    }

    status = fu2_syscsr_validate_gpio_pins(instance, pin_mask);
    if (status != NEON_OK) {
        return status;
    }

    /* Return the GPIO register to LL state for the selected batteries alone. */
    primask = gpio_enter_critical();
    instance->OE &= ~(pin_mask & GPIO_OE_EN_Msk);
    instance->OUT &= ~(pin_mask & GPIO_OUT_OUT_Msk);
    instance->CTL &= ~(pin_mask & GPIO_CTL_SYNC_EN_Msk);
    instance->INT_EN &= ~(pin_mask & GPIO_INT_EN_INTR_A_Msk);
    instance->INT_INJ &= ~(pin_mask & GPIO_INT_INJ_INTR_A_Msk);
    gpio_exit_critical(primask);

    /* RAW_INT is W1C, absolutely not read-modify-write. */
    instance->RAW_INT = pin_mask & GPIO_RAW_INT_INTR_A_Msk;

    return fu2_syscsr_reset_gpio_pins(instance, pin_mask);
}

/** @copydoc neon_gpio_write_pin */
void neon_gpio_write_pin(GPIO_TypeDef *instance, uint32_t pin_mask, uint8_t state) {
    uint32_t selected_pins;
    uint32_t primask;

    if (gpio_is_valid_instance(instance) == 0U) {
        return;
    }

    selected_pins = pin_mask & GPIO_OUT_OUT_Msk;
    if (selected_pins == 0UL) {
        return;
    }

    primask = gpio_enter_critical();
    if (state != 0U) {
        instance->OUT |= selected_pins;
    } else {
        instance->OUT &= ~selected_pins;
    }
    gpio_exit_critical(primask);
}

/** @copydoc neon_gpio_read_pin */
uint8_t neon_gpio_read_pin(GPIO_TypeDef *instance, uint32_t pin_mask) {
    if (gpio_is_valid_instance(instance) == 0U) {
        return 0U;
    }

    return ((instance->IN & pin_mask & GPIO_IN_IN_Msk) != 0UL) ? 1U : 0U;
}

/** @copydoc neon_gpio_toggle_pin */
void neon_gpio_toggle_pin(GPIO_TypeDef *instance, uint32_t pin_mask) {
    uint32_t selected_pins;
    uint32_t primask;

    if (gpio_is_valid_instance(instance) == 0U) {
        return;
    }

    selected_pins = pin_mask & GPIO_OUT_OUT_Msk;
    if (selected_pins == 0UL) {
        return;
    }

    primask = gpio_enter_critical();
    instance->OUT = (instance->OUT ^ selected_pins) & GPIO_OUT_OUT_Msk;
    gpio_exit_critical(primask);
}

/** @copydoc neon_gpio_set_interrupt_enabled */
neon_status_t neon_gpio_set_interrupt_enabled(GPIO_TypeDef *instance, uint32_t pin_mask, uint8_t enable) {
    uint32_t primask;

    if ((gpio_is_valid_instance(instance) == 0U) || (gpio_is_valid_pin_mask(pin_mask) == 0U) || (enable > 1U)) {
        return NEON_INVALID_PARAM;
    }

    primask = gpio_enter_critical();
    if (enable != 0U) {
        /* FU2 only detects edges when SYNC_EN is on. A new session must erase both raw pending and software
     * injection before opening the mask. */
        instance->CTL |= pin_mask & GPIO_CTL_SYNC_EN_Msk;
        instance->INT_INJ &= ~(pin_mask & GPIO_INT_INJ_INTR_A_Msk);
        instance->RAW_INT = pin_mask & GPIO_RAW_INT_INTR_A_Msk;
        instance->INT_EN |= pin_mask & GPIO_INT_EN_INTR_A_Msk;
    } else {
        instance->INT_EN &= ~(pin_mask & GPIO_INT_EN_INTR_A_Msk);
    }
    gpio_exit_critical(primask);
    return NEON_OK;
}

/** @copydoc neon_gpio_get_raw_interrupt_status */
neon_status_t neon_gpio_get_raw_interrupt_status(GPIO_TypeDef *instance, uint32_t *status) {
    if ((gpio_is_valid_instance(instance) == 0U) || (status == NULL)) {
        return NEON_INVALID_PARAM;
    }

    *status = instance->RAW_INT & GPIO_RAW_INT_INTR_A_Msk;
    return NEON_OK;
}

/** @copydoc neon_gpio_get_interrupt_status */
neon_status_t neon_gpio_get_interrupt_status(GPIO_TypeDef *instance, uint32_t *status) {
    if ((gpio_is_valid_instance(instance) == 0U) || (status == NULL)) {
        return NEON_INVALID_PARAM;
    }

    *status = instance->INT_STA & GPIO_INT_STA_INTR_A_Msk;
    return NEON_OK;
}

/** @copydoc neon_gpio_clear_interrupt */
neon_status_t neon_gpio_clear_interrupt(GPIO_TypeDef *instance, uint32_t pin_mask) {
    uint32_t primask;

    if ((gpio_is_valid_instance(instance) == 0U) || (gpio_is_valid_pin_mask(pin_mask) == 0U)) {
        return NEON_INVALID_PARAM;
    }

    primask = gpio_enter_critical();
    instance->INT_INJ &= ~(pin_mask & GPIO_INT_INJ_INTR_A_Msk);
    instance->RAW_INT = pin_mask & GPIO_RAW_INT_INTR_A_Msk;
    gpio_exit_critical(primask);
    return NEON_OK;
}

/** @copydoc neon_gpio_set_interrupt_injection */
neon_status_t neon_gpio_set_interrupt_injection(GPIO_TypeDef *instance, uint32_t pin_mask, uint8_t enable) {
    uint32_t primask;

    if ((gpio_is_valid_instance(instance) == 0U) || (gpio_is_valid_pin_mask(pin_mask) == 0U) || (enable > 1U)) {
        return NEON_INVALID_PARAM;
    }

    primask = gpio_enter_critical();
    if (enable != 0U) {
        instance->INT_INJ |= pin_mask & GPIO_INT_INJ_INTR_A_Msk;
    } else {
        instance->INT_INJ &= ~(pin_mask & GPIO_INT_INJ_INTR_A_Msk);
    }
    gpio_exit_critical(primask);
    return NEON_OK;
}

/*==================================================================================================
*                                        PRIVATE FUNCTIONS
==================================================================================================*/

static uint8_t gpio_is_valid_instance(GPIO_TypeDef const *instance) {
    return ((instance == GPIOA) || (instance == GPIOB) || (instance == GPIOC) || (instance == GPIOD) ||
            (instance == GPIOE) || (instance == GPIOF))
               ? 1U
               : 0U;
}

static uint8_t gpio_is_valid_pin_mask(uint32_t pin_mask) {
    return ((pin_mask != 0UL) && ((pin_mask & ~GPIO_PIN_MASK) == 0UL)) ? 1U : 0U;
}

static uint8_t gpio_is_valid_config(const neon_gpio_config_t *config) {
    if (config == NULL) {
        return 0U;
    }

    return ((gpio_is_valid_pin_mask(config->pin) != 0U) && (config->mode <= NEON_GPIO_MODE_OUTPUT) &&
            (config->pull <= NEON_GPIO_PULL_DOWN) && (config->speed <= NEON_GPIO_SPEED_VERY_HIGH) &&
            (config->schmitt <= NEON_GPIO_SCHMITT_ENABLE) && (config->slew_rate <= NEON_GPIO_SLEW_FAST) &&
            (config->input_sync <= NEON_GPIO_INPUT_SYNC_ENABLE) && (config->interrupt <= NEON_GPIO_INTERRUPT_ENABLE) &&
            (config->initial_state <= NEON_GPIO_PIN_SET))
               ? 1U
               : 0U;
}

static neon_status_t gpio_claim_jtag_pins(GPIO_TypeDef const *instance, uint32_t pin_mask) {
    if ((instance != GPIOA) || ((pin_mask & GPIO_GPIOA_JTAG_PIN_MASK) == 0UL)) {
        return NEON_OK;
    }

    /* PA0..PA4 share physical pins with JTAG. Requesting them to do GPIO means releasing the JTAG route.
   * If the route is locked while on, SYSCSR returns NEON_LOCKED and config stops before seeing
   * GPIO/PAD. change */
    return neon_syscsr_set_jtag_mux(0U);
}

static uint32_t gpio_enter_critical(void) {
    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    return primask;
}

static void gpio_exit_critical(uint32_t primask) {
    __set_PRIMASK(primask);
}

static neon_status_t gpio_get_crm_peripheral(GPIO_TypeDef const *instance, neon_crm_peripheral_t *peripheral) {
    if (peripheral == NULL) {
        return NEON_INVALID_PARAM;
    }

    if (instance == GPIOA) {
        *peripheral = NEON_CRM_PERIPHERAL_GPIOA;
    } else if (instance == GPIOB) {
        *peripheral = NEON_CRM_PERIPHERAL_GPIOB;
    } else if (instance == GPIOC) {
        *peripheral = NEON_CRM_PERIPHERAL_GPIOC;
    } else if (instance == GPIOD) {
        *peripheral = NEON_CRM_PERIPHERAL_GPIOD;
    } else if (instance == GPIOE) {
        *peripheral = NEON_CRM_PERIPHERAL_GPIOE;
    } else if (instance == GPIOF) {
        *peripheral = NEON_CRM_PERIPHERAL_GPIOF;
    } else {
        return NEON_INVALID_PARAM;
    }

    return NEON_OK;
}

static void gpio_make_pad_config(const neon_gpio_config_t *init, fu2_syscsr_pad_config_t *pad_config) {
    /* The Neon and SYSCSR enums were explicitly converted so that the two interfaces did not implicitly
   * depend on each other's numerical values. */
    pad_config->input_enable = 1U;

    switch (init->pull) {
        case NEON_GPIO_PULL_UP:
            pad_config->pull = SYSCSR_PULL_UP;
            break;
        case NEON_GPIO_PULL_DOWN:
            pad_config->pull = SYSCSR_PULL_DOWN;
            break;
        default:
            pad_config->pull = SYSCSR_PULL_NONE;
            break;
    }

    switch (init->speed) {
        case NEON_GPIO_SPEED_MEDIUM:
            pad_config->drive = SYSCSR_DRIVE_4MA;
            break;
        case NEON_GPIO_SPEED_HIGH:
            pad_config->drive = SYSCSR_DRIVE_8MA;
            break;
        case NEON_GPIO_SPEED_VERY_HIGH:
            pad_config->drive = SYSCSR_DRIVE_12MA;
            break;
        default:
            pad_config->drive = SYSCSR_DRIVE_2MA;
            break;
    }

    pad_config->schmitt_enable = (init->schmitt == NEON_GPIO_SCHMITT_ENABLE) ? 1U : 0U;
    pad_config->slew_rate = (init->slew_rate == NEON_GPIO_SLEW_FAST) ? SYSCSR_SLEW_FAST : SYSCSR_SLEW_SLOW;
}
