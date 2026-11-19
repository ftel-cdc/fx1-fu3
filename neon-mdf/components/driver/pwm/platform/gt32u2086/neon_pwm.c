/**
 * @file neon_pwm.c
 * @brief Implementation for PWM.
 * @details
 * Implements module behavior and target-specific integration.
 */

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include "pwm/neon_pwm.h"
#include <string.h>

#include "crm/neon_crm.h"
#include "syscsr/platform/gt32u2086/neon_syscsr_internal.h"

#include <stddef.h>

/*==================================================================================================
*                                         LOCAL VARIABLES
==================================================================================================*/
static neon_pwm_mode_t pwm_mode[3][2];
static uint32_t pwm_initialized[3][2];
static gt32u2086_syscsr_pin_group_t pwm_pins[3];
static uint32_t pwm_auto_channels[3];
static const gt32u2086_syscsr_pin_route_t pwm_routes[3] = {
    {SYSCSR_PIN_73, SYSCSR_PIN_FUNCTION_PWM0},
    {SYSCSR_PIN_74, SYSCSR_PIN_FUNCTION_PWM1},
    {SYSCSR_PIN_48, SYSCSR_PIN_FUNCTION_PWM_T0},
};

static neon_crm_peripheral_t pwm_crm_peripheral(const TIM_TypeDef *instance);

/*==================================================================================================
*                                   PRIVATE FUNCTION PROTOTYPES
==================================================================================================*/

static uint8_t pwm_instance_index(const TIM_TypeDef *instance);
static uint8_t pwm_is_valid_instance(const TIM_TypeDef *instance);
static neon_status_t pwm_validate_access(const TIM_TypeDef *instance, uint8_t channel);
static uint32_t pwm_is_initialized(const TIM_TypeDef *instance, uint8_t channel);
static uint32_t pwm_enter_critical(void);
static void pwm_exit_critical(uint32_t primask);
static volatile uint32_t *pwm_ctrl(TIM_TypeDef *instance, uint8_t channel);
static volatile uint32_t *pwm_prescaler(TIM_TypeDef *instance, uint8_t channel);
static volatile uint32_t *pwm_period(TIM_TypeDef *instance, uint8_t channel);
static volatile uint32_t *pwm_pulse(TIM_TypeDef *instance, uint8_t channel);
static volatile uint32_t *pwm_compare(TIM_TypeDef *instance, uint8_t channel, uint8_t compare);
static const volatile uint32_t *pwm_counter(const TIM_TypeDef *instance, uint8_t channel);
static volatile uint32_t *pwm_interrupt(TIM_TypeDef *instance, uint8_t channel);
static const volatile uint32_t *pwm_status(TIM_TypeDef *instance, uint8_t channel);
static uint32_t pwm_interrupt_mask(uint32_t interrupts);
static neon_crm_peripheral_t pwm_crm_peripheral(const TIM_TypeDef *instance);

/*==================================================================================================
*                                         GLOBAL FUNCTIONS
==================================================================================================*/

/** @copydoc neon_pwm_get_default_config */
void neon_pwm_get_default_config(neon_pwm_config_t *config) {
    if (config == NULL) {
        return;
    }
    config->prescaler = 0U;
    config->period = 0xFFFFU;
    config->pulse = 0U;
    config->mode = NEON_PWM_MODE_0;
    config->counter_mode = NEON_PWM_COUNTERMODE_UP;
    config->one_pulse = 0U;
    config->center_aligned = 0U;
    config->interrupts = 0U;
    config->pin_config = NEON_PWM_PIN_CONFIG_AUTO;
}

/** @copydoc neon_pwm_init */
neon_status_t neon_pwm_init(TIM_TypeDef *instance, uint8_t channel, const neon_pwm_config_t *config) {
    neon_crm_peripheral_t peripheral;
    neon_status_t status;
    volatile uint32_t *control;
    uint32_t control_value;
    uint32_t primask;
    uint8_t index;
    uint8_t compare;

    if ((pwm_validate_access(instance, channel) != NEON_OK) || (config == NULL) || (config->period == 0U) ||
        (config->pulse > config->period) || ((config->mode != NEON_PWM_MODE_0) && (config->mode != NEON_PWM_MODE_1)) ||
        ((config->counter_mode != NEON_PWM_COUNTERMODE_UP) && (config->counter_mode != NEON_PWM_COUNTERMODE_DOWN)) ||
        (config->one_pulse > 1U) || (config->center_aligned > 1U) ||
        ((config->interrupts & ~NEON_PWM_INTERRUPT_ALL) != 0U) ||
        (uint32_t)config->pin_config > NEON_PWM_PIN_CONFIG_KEEP) {
        return NEON_INVALID_PARAM;
    }

    if (config->pin_config == NEON_PWM_PIN_CONFIG_AUTO && channel != NEON_PWM_CHANNEL_0) {
        return NEON_NOT_MAPPED;
    }

    peripheral = pwm_crm_peripheral(instance);
    status = neon_crm_peripheral_clock_enable(peripheral);
    if (status == NEON_OK) {
        status = neon_crm_peripheral_release_reset(peripheral);
    }
    if (status != NEON_OK) {
        return status;
    }

    /* CHL is the period boundary and CCR1 drives the PWM output. Keep mode zero
   * until Start so no partially configured waveform reaches the pin. */
    control_value = TIM_CTRL_CH0_AUTO_RELOAD_Msk | TIM_CTRL_CH0_CC1_EN_Msk;
    if (config->counter_mode == NEON_PWM_COUNTERMODE_DOWN) {
        control_value |= TIM_CTRL_CH0_DIRECTION_Msk;
    }
    if (config->one_pulse != 0U) {
        control_value |= TIM_CTRL_CH0_ONE_PULSE_Msk;
    }
    if (config->center_aligned != 0U) {
        control_value |= TIM_CTRL_CH0_CENTER_ALIGN_Msk;
    }

    control = pwm_ctrl(instance, channel);
    primask = pwm_enter_critical();
    *control = 0U;
    index = pwm_instance_index(instance);
    if (config->pin_config == NEON_PWM_PIN_CONFIG_AUTO) {
        status = gt32u2086_syscsr_configure_pins(&pwm_pins[index], &pwm_routes[index], 1U, 0U);
        if (status != NEON_OK) {
            pwm_exit_critical(primask);
            return status;
        }
        pwm_auto_channels[index] |= 1U << channel;
    } else {
        pwm_auto_channels[index] &= ~(1U << channel);
        if (pwm_auto_channels[index] == 0U) {
            gt32u2086_syscsr_release_pins(&pwm_pins[index]);
        }
    }
    *pwm_interrupt(instance, channel) = 0U;
    *pwm_prescaler(instance, channel) = config->prescaler;
    *pwm_period(instance, channel) = config->period;
    *pwm_pulse(instance, channel) = config->pulse;
    for (compare = 2U; compare <= 4U; ++compare) {
        *pwm_compare(instance, channel, compare) = 0xFFFFFFFFU;
    }
    *control = control_value;
    *pwm_interrupt(instance, channel) = pwm_interrupt_mask(config->interrupts);
    index = pwm_instance_index(instance);
    pwm_mode[index][channel] = config->mode;
    pwm_initialized[index][channel] = 1U;
    pwm_exit_critical(primask);
    return NEON_OK;
}

/** @copydoc neon_pwm_deinit */
neon_status_t neon_pwm_deinit(TIM_TypeDef *instance, uint8_t channel) {
    neon_crm_peripheral_t peripheral;
    neon_status_t status;
    uint32_t primask;
    uint8_t index;
    uint8_t compare;

    if (pwm_validate_access(instance, channel) != NEON_OK) {
        return NEON_INVALID_PARAM;
    }
    peripheral = pwm_crm_peripheral(instance);
    status = neon_crm_peripheral_clock_enable(peripheral);
    if (status == NEON_OK) {
        status = neon_crm_peripheral_release_reset(peripheral);
    }
    if (status != NEON_OK) {
        return status;
    }

    primask = pwm_enter_critical();
    *pwm_interrupt(instance, channel) = 0U;
    *pwm_ctrl(instance, channel) = 0U;
    *pwm_prescaler(instance, channel) = 0U;
    *pwm_period(instance, channel) = 0U;
    for (compare = 1U; compare <= 4U; ++compare) {
        *pwm_compare(instance, channel, compare) = 0xFFFFFFFFU;
    }
    index = pwm_instance_index(instance);
    pwm_initialized[index][channel] = 0U;
    pwm_mode[index][channel] = NEON_PWM_MODE_0;
    pwm_auto_channels[index] &= ~(1U << channel);
    if (pwm_auto_channels[index] == 0U) {
        gt32u2086_syscsr_release_pins(&pwm_pins[index]);
    }
    pwm_exit_critical(primask);
    return NEON_OK;
}

/** @copydoc neon_pwm_start */
neon_status_t neon_pwm_start(TIM_TypeDef *instance, uint8_t channel) {
    volatile uint32_t *control;
    uint32_t primask;
    uint8_t index;

    if (pwm_validate_access(instance, channel) != NEON_OK) {
        return NEON_INVALID_PARAM;
    }
    index = pwm_instance_index(instance);
    if (pwm_is_initialized(instance, channel) == 0U) {
        return NEON_ERROR;
    }
    control = pwm_ctrl(instance, channel);
    primask = pwm_enter_critical();
    *control = (*control & ~TIM_CTRL_CH0_TIMER_MODE_Msk) |
               (((uint32_t)pwm_mode[index][channel] << TIM_CTRL_CH0_TIMER_MODE_Pos) & TIM_CTRL_CH0_TIMER_MODE_Msk);
    pwm_exit_critical(primask);
    return NEON_OK;
}

/** @copydoc neon_pwm_stop */
neon_status_t neon_pwm_stop(TIM_TypeDef *instance, uint8_t channel) {
    volatile uint32_t *control;
    uint32_t primask;

    if (pwm_validate_access(instance, channel) != NEON_OK) {
        return NEON_INVALID_PARAM;
    }
    if (pwm_is_initialized(instance, channel) == 0U) {
        return NEON_ERROR;
    }
    control = pwm_ctrl(instance, channel);
    primask = pwm_enter_critical();
    *control &= ~TIM_CTRL_CH0_TIMER_MODE_Msk;
    pwm_exit_critical(primask);
    return NEON_OK;
}

/** @copydoc neon_pwm_is_running */
neon_status_t neon_pwm_is_running(TIM_TypeDef *instance, uint8_t channel, uint8_t *running) {
    if ((pwm_validate_access(instance, channel) != NEON_OK) || (running == NULL)) {
        return NEON_INVALID_PARAM;
    }
    if (pwm_is_initialized(instance, channel) == 0U) {
        return NEON_ERROR;
    }
    {
        uint32_t mode = *pwm_ctrl(instance, channel) & TIM_CTRL_CH0_TIMER_MODE_Msk;
        const uint32_t result_word =
            ((mode == (uint32_t)NEON_PWM_MODE_0) || (mode == (uint32_t)NEON_PWM_MODE_1)) ? 1U : 0U;
        memcpy(running, &result_word, 1U);
    }
    return NEON_OK;
}

/** @copydoc neon_pwm_set_period */
neon_status_t neon_pwm_set_period(TIM_TypeDef *instance, uint8_t channel, uint32_t period) {
    uint32_t primask;

    if ((pwm_validate_access(instance, channel) != NEON_OK) || (period == 0U)) {
        return NEON_INVALID_PARAM;
    }
    if (pwm_is_initialized(instance, channel) == 0U) {
        return NEON_ERROR;
    }
    primask = pwm_enter_critical();
    if (*pwm_pulse(instance, channel) > period) {
        pwm_exit_critical(primask);
        return NEON_INVALID_PARAM;
    }
    *pwm_period(instance, channel) = period;
    pwm_exit_critical(primask);
    return NEON_OK;
}

/** @copydoc neon_pwm_get_period */
neon_status_t neon_pwm_get_period(TIM_TypeDef *instance, uint8_t channel, uint32_t *period) {
    if ((pwm_validate_access(instance, channel) != NEON_OK) || (period == NULL)) {
        return NEON_INVALID_PARAM;
    }
    if (pwm_is_initialized(instance, channel) == 0U) {
        return NEON_ERROR;
    }
    *period = *pwm_period(instance, channel);
    return NEON_OK;
}

/** @copydoc neon_pwm_set_pulse */
neon_status_t neon_pwm_set_pulse(TIM_TypeDef *instance, uint8_t channel, uint32_t pulse) {
    uint32_t primask;

    if (pwm_validate_access(instance, channel) != NEON_OK) {
        return NEON_INVALID_PARAM;
    }
    if (pwm_is_initialized(instance, channel) == 0U) {
        return NEON_ERROR;
    }
    primask = pwm_enter_critical();
    if (pulse > *pwm_period(instance, channel)) {
        pwm_exit_critical(primask);
        return NEON_INVALID_PARAM;
    }
    *pwm_pulse(instance, channel) = pulse;
    pwm_exit_critical(primask);
    return NEON_OK;
}

/** @copydoc neon_pwm_get_pulse */
neon_status_t neon_pwm_get_pulse(TIM_TypeDef *instance, uint8_t channel, uint32_t *pulse) {
    if ((pwm_validate_access(instance, channel) != NEON_OK) || (pulse == NULL)) {
        return NEON_INVALID_PARAM;
    }
    if (pwm_is_initialized(instance, channel) == 0U) {
        return NEON_ERROR;
    }
    *pulse = *pwm_pulse(instance, channel);
    return NEON_OK;
}

/** @copydoc neon_pwm_get_counter */
neon_status_t neon_pwm_get_counter(TIM_TypeDef *instance, uint8_t channel, uint32_t *counter) {
    if ((pwm_validate_access(instance, channel) != NEON_OK) || (counter == NULL)) {
        return NEON_INVALID_PARAM;
    }
    if (pwm_is_initialized(instance, channel) == 0U) {
        return NEON_ERROR;
    }
    *counter = *pwm_counter(instance, channel);
    return NEON_OK;
}

/** @copydoc neon_pwm_set_prescaler */
neon_status_t neon_pwm_set_prescaler(TIM_TypeDef *instance, uint8_t channel, uint32_t prescaler) {
    if (pwm_validate_access(instance, channel) != NEON_OK) {
        return NEON_INVALID_PARAM;
    }
    if (pwm_is_initialized(instance, channel) == 0U) {
        return NEON_ERROR;
    }
    *pwm_prescaler(instance, channel) = prescaler;
    return NEON_OK;
}

/** @copydoc neon_pwm_get_prescaler */
neon_status_t neon_pwm_get_prescaler(TIM_TypeDef *instance, uint8_t channel, uint32_t *prescaler) {
    if ((pwm_validate_access(instance, channel) != NEON_OK) || (prescaler == NULL)) {
        return NEON_INVALID_PARAM;
    }
    if (pwm_is_initialized(instance, channel) == 0U) {
        return NEON_ERROR;
    }
    *prescaler = *pwm_prescaler(instance, channel);
    return NEON_OK;
}

/** @copydoc neon_pwm_enable_interrupt */
neon_status_t neon_pwm_enable_interrupt(TIM_TypeDef *instance, uint8_t channel, uint32_t interrupts) {
    volatile uint32_t *reg;
    uint32_t primask;
    if ((pwm_validate_access(instance, channel) != NEON_OK) || (interrupts == 0U) ||
        ((interrupts & ~NEON_PWM_INTERRUPT_ALL) != 0U)) {
        return NEON_INVALID_PARAM;
    }
    if (pwm_is_initialized(instance, channel) == 0U) {
        return NEON_ERROR;
    }
    reg = pwm_interrupt(instance, channel);
    primask = pwm_enter_critical();
    *reg |= pwm_interrupt_mask(interrupts);
    pwm_exit_critical(primask);
    return NEON_OK;
}

/** @copydoc neon_pwm_disable_interrupt */
neon_status_t neon_pwm_disable_interrupt(TIM_TypeDef *instance, uint8_t channel, uint32_t interrupts) {
    volatile uint32_t *reg;
    uint32_t primask;
    if ((pwm_validate_access(instance, channel) != NEON_OK) || (interrupts == 0U) ||
        ((interrupts & ~NEON_PWM_INTERRUPT_ALL) != 0U)) {
        return NEON_INVALID_PARAM;
    }
    if (pwm_is_initialized(instance, channel) == 0U) {
        return NEON_ERROR;
    }
    reg = pwm_interrupt(instance, channel);
    primask = pwm_enter_critical();
    *reg &= ~pwm_interrupt_mask(interrupts);
    pwm_exit_critical(primask);
    return NEON_OK;
}

/** @copydoc neon_pwm_is_interrupt_enabled */
neon_status_t neon_pwm_is_interrupt_enabled(TIM_TypeDef *instance, uint8_t channel, uint32_t interrupts,
                                            uint8_t *enabled) {
    uint32_t mask;

    if ((pwm_validate_access(instance, channel) != NEON_OK) || (interrupts == 0U) ||
        ((interrupts & ~NEON_PWM_INTERRUPT_ALL) != 0U) || (enabled == NULL)) {
        return NEON_INVALID_PARAM;
    }
    if (pwm_is_initialized(instance, channel) == 0U) {
        return NEON_ERROR;
    }
    mask = pwm_interrupt_mask(interrupts);
    const uint32_t result_word = ((*pwm_interrupt(instance, channel) & mask) == mask) ? 1U : 0U;
    memcpy(enabled, &result_word, 1U);
    return NEON_OK;
}

/** @copydoc neon_pwm_get_active_flags */
neon_status_t neon_pwm_get_active_flags(TIM_TypeDef *instance, uint8_t channel, uint32_t *flags) {
    uint32_t value;
    if ((pwm_validate_access(instance, channel) != NEON_OK) || (flags == NULL)) {
        return NEON_INVALID_PARAM;
    }
    if (pwm_is_initialized(instance, channel) == 0U) {
        return NEON_ERROR;
    }
    value = *pwm_status(instance, channel);
    *flags = 0U;
    if ((value & TIM_STT_CH0_UP_FLAG_Msk) != 0U) {
        *flags |= NEON_PWM_FLAG_UPDATE;
    }
    if ((value & TIM_STT_CH0_CC1_FLAG_Msk) != 0U) {
        *flags |= NEON_PWM_FLAG_COMPARE;
    }
    return NEON_OK;
}

/** @copydoc neon_pwm_is_active_flag */
neon_status_t neon_pwm_is_active_flag(TIM_TypeDef *instance, uint8_t channel, uint32_t flags, uint8_t *active) {
    uint32_t active_flags;
    neon_status_t status;

    if ((flags == 0U) || ((flags & ~NEON_PWM_FLAG_ALL) != 0U) || (active == NULL)) {
        return NEON_INVALID_PARAM;
    }
    status = neon_pwm_get_active_flags(instance, channel, &active_flags);
    if (status == NEON_OK) {
        const uint32_t result_word = ((active_flags & flags) == flags) ? 1U : 0U;
        memcpy(active, &result_word, 1U);
    }
    return status;
}

/** @copydoc neon_pwm_clear_flags */
neon_status_t neon_pwm_clear_flags(TIM_TypeDef *instance, uint8_t channel, uint32_t flags) {
    if ((pwm_validate_access(instance, channel) != NEON_OK) || (flags == 0U) || ((flags & ~NEON_PWM_FLAG_ALL) != 0U)) {
        return NEON_INVALID_PARAM;
    }
    if (pwm_is_initialized(instance, channel) == 0U) {
        return NEON_ERROR;
    }
    /* Same read-only live STT as TIM: a read does not clear the event. */
    return NEON_NOT_SUPPORTED;
}

/*==================================================================================================
*                                        PRIVATE FUNCTIONS
==================================================================================================*/

static uint8_t pwm_instance_index(const TIM_TypeDef *instance) {
    return (instance == TIM0) ? 2U : ((instance == PWM1) ? 1U : 0U);
}

static uint8_t pwm_is_valid_instance(const TIM_TypeDef *instance) {
    return ((instance == PWM0) || (instance == PWM1) || (instance == TIM0)) ? 1U : 0U;
}

static neon_status_t pwm_validate_access(const TIM_TypeDef *instance, uint8_t channel) {
    return ((pwm_is_valid_instance(instance) != 0U) && (channel <= NEON_PWM_CHANNEL_1)) ? NEON_OK : NEON_INVALID_PARAM;
}

static uint32_t pwm_is_initialized(const TIM_TypeDef *instance, uint8_t channel) {
    neon_field8_t clock_enabled = 0U;
    neon_field8_t reset_asserted = 1U;
    neon_crm_peripheral_t peripheral;
    uint32_t control;
    const uint32_t configured_mask = TIM_CTRL_CH0_AUTO_RELOAD_Msk | TIM_CTRL_CH0_CC1_EN_Msk;

    if (pwm_initialized[pwm_instance_index(instance)][channel] == 0U) {
        return 0U;
    }

    peripheral = pwm_crm_peripheral(instance);
    if ((neon_crm_is_peripheral_clock_enabled(peripheral, (uint8_t *)&clock_enabled) != NEON_OK) ||
        (clock_enabled == 0U)) {
        return 0U;
    }
    if (neon_crm_is_peripheral_reset_asserted(peripheral, (uint8_t *)&reset_asserted) != NEON_OK) {
        return 0U;
    }
    if (reset_asserted != 0U) {
        pwm_initialized[pwm_instance_index(instance)][channel] = 0U;
        return 0U;
    }

    /* CRM can reset the PWM block without clearing this driver's SRAM state. */
    control = (channel == NEON_PWM_CHANNEL_1) ? instance->CTRL_CH1 : instance->CTRL_CH0;
    if ((control & configured_mask) != configured_mask) {
        pwm_initialized[pwm_instance_index(instance)][channel] = 0U;
        return 0U;
    }
    return 1U;
}

static uint32_t pwm_enter_critical(void) {
    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    return primask;
}

static void pwm_exit_critical(uint32_t primask) {
    __set_PRIMASK(primask);
}

static volatile uint32_t *pwm_ctrl(TIM_TypeDef *instance, uint8_t channel) {
    return (channel == NEON_PWM_CHANNEL_1) ? &instance->CTRL_CH1 : &instance->CTRL_CH0;
}

static volatile uint32_t *pwm_prescaler(TIM_TypeDef *instance, uint8_t channel) {
    return (channel == NEON_PWM_CHANNEL_1) ? &instance->PRSC_CH1 : &instance->PRSC_CH0;
}

static volatile uint32_t *pwm_period(TIM_TypeDef *instance, uint8_t channel) {
    return (channel == NEON_PWM_CHANNEL_1) ? &instance->CHL_CH1 : &instance->CHL_CH0;
}

static volatile uint32_t *pwm_pulse(TIM_TypeDef *instance, uint8_t channel) {
    return (channel == NEON_PWM_CHANNEL_1) ? &instance->CCR1_CH1 : &instance->CCR1_CH0;
}

static volatile uint32_t *pwm_compare(TIM_TypeDef *instance, uint8_t channel, uint8_t compare) {
    if (channel == NEON_PWM_CHANNEL_1) {
        if (compare == 2U) {
            return &instance->CCR2_CH1;
        }
        if (compare == 3U) {
            return &instance->CCR3_CH1;
        }
        if (compare == 4U) {
            return &instance->CCR4_CH1;
        }
        return &instance->CCR1_CH1;
    }
    if (compare == 2U) {
        return &instance->CCR2_CH0;
    }
    if (compare == 3U) {
        return &instance->CCR3_CH0;
    }
    if (compare == 4U) {
        return &instance->CCR4_CH0;
    }
    return &instance->CCR1_CH0;
}

static const volatile uint32_t *pwm_counter(const TIM_TypeDef *instance, uint8_t channel) {
    return (channel == NEON_PWM_CHANNEL_1) ? &instance->CNT_CH1 : &instance->CNT_CH0;
}

static volatile uint32_t *pwm_interrupt(TIM_TypeDef *instance, uint8_t channel) {
    return (channel == NEON_PWM_CHANNEL_1) ? &instance->INTR_EN_CH1 : &instance->INTR_EN_CH0;
}

static const volatile uint32_t *pwm_status(TIM_TypeDef *instance, uint8_t channel) {
    return (channel == NEON_PWM_CHANNEL_1) ? &instance->STT_CH1 : &instance->STT_CH0;
}

static uint32_t pwm_interrupt_mask(uint32_t interrupts) {
    uint32_t mask = 0U;
    if ((interrupts & NEON_PWM_INTERRUPT_UPDATE) != 0U) {
        mask |= TIM_INTR_EN_CH0_UP_INTR_EN_Msk;
    }
    if ((interrupts & NEON_PWM_INTERRUPT_COMPARE) != 0U) {
        mask |= TIM_INTR_EN_CH0_CC1_INTR_EN_Msk;
    }
    return mask;
}

static neon_crm_peripheral_t pwm_crm_peripheral(const TIM_TypeDef *instance) {
    if (instance == TIM0) {
        return NEON_CRM_PERIPHERAL_TIMER0;
    }
    return (instance == PWM1) ? NEON_CRM_PERIPHERAL_PWM1 : NEON_CRM_PERIPHERAL_PWM0;
}
