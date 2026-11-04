/**
 * @file neon_tim.c
 * @brief Implementation for timer.
 * @details
 * * Backend TIM cho FU2.
 *
 * Register, POS và MASK lấy từ FU2.h được sinh bởi FU2.yaml. Logic cấu hình
 * nền tảng bám theo fu2_dti_tim.[ch], còn kiểm tra tham số, quản lý clock và
 * API status được tổ chức theo lớp HAL Neon.
 */

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/

#include "tim/neon_tim.h"
#include <string.h>

#include "crm/neon_crm.h"

#include <stddef.h>

/*==================================================================================================
*                                   PRIVATE FUNCTION PROTOTYPES
==================================================================================================*/

static uint8_t tim_is_valid_instance(const TIM_TypeDef *instance);
static uint8_t tim_is_valid_channel(uint8_t channel);
static uint8_t tim_is_valid_compare_channel(uint8_t channel);
static uint8_t tim_is_valid_interrupt_mask(uint32_t interrupts, uint8_t allow_empty);
static uint8_t tim_is_valid_flag_mask(uint32_t flags);
static neon_status_t tim_validate_access(const TIM_TypeDef *instance, uint8_t channel);
static neon_status_t tim_validate_config(const neon_tim_config_t *config);
static uint32_t tim_enter_critical(void);
static void tim_exit_critical(uint32_t primask);
static volatile uint32_t *tim_prescaler_reg(TIM_TypeDef *instance, uint8_t channel);
static volatile uint32_t *tim_auto_reload_reg(TIM_TypeDef *instance, uint8_t channel);
static const volatile uint32_t *tim_counter_reg(const TIM_TypeDef *instance, uint8_t channel);
static volatile uint32_t *tim_control_reg(TIM_TypeDef *instance, uint8_t channel);
static volatile uint32_t *tim_interrupt_reg(TIM_TypeDef *instance, uint8_t channel);
static const volatile uint32_t *tim_status_reg(TIM_TypeDef *instance, uint8_t channel);
static volatile uint32_t *tim_compare_reg(TIM_TypeDef *instance, uint8_t channel, uint8_t compare_channel);
static uint32_t tim_compare_enable_mask(uint8_t compare_channel);
static neon_crm_peripheral_t tim_crm_peripheral(const TIM_TypeDef *instance);
static uint32_t tim_interrupts_to_hw(uint32_t interrupts);
static uint32_t tim_flags_from_hw(uint32_t value);
static neon_status_t tim_set_control_bit(TIM_TypeDef *instance, uint8_t channel, uint32_t mask, uint8_t enabled);
static neon_status_t tim_get_control_bit(TIM_TypeDef *instance, uint8_t channel, uint32_t mask, uint8_t *enabled);

/*==================================================================================================
*                                         GLOBAL FUNCTIONS
==================================================================================================*/

/** @copydoc neon_tim_get_default_config */
void neon_tim_get_default_config(neon_tim_config_t *config) {
    if (config == NULL) {
        return;
    }

    config->prescaler = 0U;
    config->auto_reload = 0U;
    config->mode = NEON_TIM_MODE_DISABLE;
    config->counter_mode = NEON_TIM_COUNTERMODE_UP;
    config->auto_reload_mode = NEON_TIM_AUTORELOAD_DISABLE;
    config->one_pulse_mode = NEON_TIM_ONE_PULSE_REPETITIVE;
    config->center_aligned_mode = NEON_TIM_CENTER_ALIGNED_DISABLE;
    config->interrupts = 0U;
}

/** @copydoc neon_tim_init */
neon_status_t neon_tim_init(TIM_TypeDef *instance, uint8_t channel, const neon_tim_config_t *init) {
    neon_status_t status;
    neon_crm_peripheral_t peripheral;
    volatile uint32_t *control;
    uint32_t control_value = 0U;
    uint32_t primask;
    uint8_t compare_channel;

    status = tim_validate_access(instance, channel);
    if (status != NEON_OK) {
        return status;
    }
    status = tim_validate_config(init);
    if (status != NEON_OK) {
        return status;
    }

    peripheral = tim_crm_peripheral(instance);
    status = neon_crm_peripheral_clock_enable(peripheral);
    if (status == NEON_OK) {
        /* Không pulse reset: reset này dùng chung cho cả CH0 và CH1. */
        status = neon_crm_peripheral_release_reset(peripheral);
    }
    if (status != NEON_OK) {
        return status;
    }

    if (init->mode == NEON_TIM_MODE_NORMAL) {
        control_value |= (1U << TIM_CTRL_CH0_TIMER_MODE_Pos) & TIM_CTRL_CH0_TIMER_MODE_Msk;
    }
    if (init->counter_mode == NEON_TIM_COUNTERMODE_DOWN) {
        control_value |= TIM_CTRL_CH0_DIRECTION_Msk;
    }
    if (init->auto_reload_mode == NEON_TIM_AUTORELOAD_ENABLE) {
        control_value |= TIM_CTRL_CH0_AUTO_RELOAD_Msk;
    }
    if (init->one_pulse_mode == NEON_TIM_ONE_PULSE_SINGLE) {
        control_value |= TIM_CTRL_CH0_ONE_PULSE_Msk;
    }
    if (init->center_aligned_mode == NEON_TIM_CENTER_ALIGNED_ENABLE) {
        control_value |= TIM_CTRL_CH0_CENTER_ALIGN_Msk;
    }

    control = tim_control_reg(instance, channel);
    primask = tim_enter_critical();

    /* Tắt counter và nguồn ngắt trước khi thay cấu hình. mode được ghi lại ở
   * cuối nên config với NORMAL chỉ chạy sau khi toàn bộ register đã sẵn sàng. */
    *control &= ~TIM_CTRL_CH0_TIMER_MODE_Msk;
    *tim_interrupt_reg(instance, channel) = 0U;
    *tim_prescaler_reg(instance, channel) = init->prescaler;
    *tim_auto_reload_reg(instance, channel) = init->auto_reload;
    for (compare_channel = NEON_TIM_COMPARE_1; compare_channel <= NEON_TIM_COMPARE_4; compare_channel++) {
        *tim_compare_reg(instance, channel, compare_channel) = 0xFFFFFFFFU;
    }

    *control = control_value;
    *tim_interrupt_reg(instance, channel) = tim_interrupts_to_hw(init->interrupts);

    tim_exit_critical(primask);
    return NEON_OK;
}

/** @copydoc neon_tim_deinit */
neon_status_t neon_tim_deinit(TIM_TypeDef *instance, uint8_t channel) {
    uint32_t primask;
    uint8_t compare_channel;

    if (tim_validate_access(instance, channel) != NEON_OK) {
        return NEON_INVALID_PARAM;
    }

    primask = tim_enter_critical();
    *tim_interrupt_reg(instance, channel) = 0U;
    *tim_control_reg(instance, channel) = 0U;
    *tim_prescaler_reg(instance, channel) = 0U;
    *tim_auto_reload_reg(instance, channel) = 0U;
    for (compare_channel = NEON_TIM_COMPARE_1; compare_channel <= NEON_TIM_COMPARE_4; compare_channel++) {
        *tim_compare_reg(instance, channel, compare_channel) = 0xFFFFFFFFU;
    }
    tim_exit_critical(primask);
    return NEON_OK;
}

/** @copydoc neon_tim_start */
neon_status_t neon_tim_start(TIM_TypeDef *instance, uint8_t channel) {
    return neon_tim_set_mode(instance, channel, NEON_TIM_MODE_NORMAL);
}

/** @copydoc neon_tim_stop */
neon_status_t neon_tim_stop(TIM_TypeDef *instance, uint8_t channel) {
    return neon_tim_set_mode(instance, channel, NEON_TIM_MODE_DISABLE);
}

/** @copydoc neon_tim_set_mode */
neon_status_t neon_tim_set_mode(TIM_TypeDef *instance, uint8_t channel, neon_tim_mode_t mode) {
    volatile uint32_t *reg;
    uint32_t primask;

    if ((tim_validate_access(instance, channel) != NEON_OK) ||
        ((mode != NEON_TIM_MODE_DISABLE) && (mode != NEON_TIM_MODE_NORMAL) && (mode != NEON_TIM_MODE_PWM0) &&
         (mode != NEON_TIM_MODE_PWM1))) {
        return NEON_INVALID_PARAM;
    }
    if ((mode == NEON_TIM_MODE_PWM0) || (mode == NEON_TIM_MODE_PWM1)) {
        return NEON_NOT_SUPPORTED;
    }

    reg = tim_control_reg(instance, channel);
    primask = tim_enter_critical();
    *reg = (*reg & ~TIM_CTRL_CH0_TIMER_MODE_Msk) |
           (((uint32_t)mode << TIM_CTRL_CH0_TIMER_MODE_Pos) & TIM_CTRL_CH0_TIMER_MODE_Msk);
    tim_exit_critical(primask);
    return NEON_OK;
}

/** @copydoc neon_tim_get_mode */
neon_status_t neon_tim_get_mode(TIM_TypeDef *instance, uint8_t channel, neon_tim_mode_t *mode) {
    uint32_t value;

    if ((tim_validate_access(instance, channel) != NEON_OK) || (mode == NULL)) {
        return NEON_INVALID_PARAM;
    }
    value = *tim_control_reg(instance, channel);
    *mode = (neon_tim_mode_t)((value & TIM_CTRL_CH0_TIMER_MODE_Msk) >> TIM_CTRL_CH0_TIMER_MODE_Pos);
    return NEON_OK;
}

/** @copydoc neon_tim_is_running */
neon_status_t neon_tim_is_running(TIM_TypeDef *instance, uint8_t channel, uint8_t *running) {
    if ((tim_validate_access(instance, channel) != NEON_OK) || (running == NULL)) {
        return NEON_INVALID_PARAM;
    }

    const uint32_t result_word = ((*tim_control_reg(instance, channel) & TIM_CTRL_CH0_TIMER_MODE_Msk) != 0U) ? 1U : 0U;
    memcpy(running, &result_word, 1U);
    return NEON_OK;
}

/** @copydoc neon_tim_set_prescaler */
neon_status_t neon_tim_set_prescaler(TIM_TypeDef *instance, uint8_t channel, uint32_t value) {
    if (tim_validate_access(instance, channel) != NEON_OK) {
        return NEON_INVALID_PARAM;
    }
    *tim_prescaler_reg(instance, channel) = value;
    return NEON_OK;
}

/** @copydoc neon_tim_get_prescaler */
neon_status_t neon_tim_get_prescaler(TIM_TypeDef *instance, uint8_t channel, uint32_t *value) {
    if ((tim_validate_access(instance, channel) != NEON_OK) || (value == NULL)) {
        return NEON_INVALID_PARAM;
    }
    *value = *tim_prescaler_reg(instance, channel);
    return NEON_OK;
}

/** @copydoc neon_tim_set_auto_reload */
neon_status_t neon_tim_set_auto_reload(TIM_TypeDef *instance, uint8_t channel, uint32_t value) {
    if (tim_validate_access(instance, channel) != NEON_OK) {
        return NEON_INVALID_PARAM;
    }
    *tim_auto_reload_reg(instance, channel) = value;
    return NEON_OK;
}

/** @copydoc neon_tim_get_auto_reload */
neon_status_t neon_tim_get_auto_reload(TIM_TypeDef *instance, uint8_t channel, uint32_t *value) {
    if ((tim_validate_access(instance, channel) != NEON_OK) || (value == NULL)) {
        return NEON_INVALID_PARAM;
    }
    *value = *tim_auto_reload_reg(instance, channel);
    return NEON_OK;
}

/** @copydoc neon_tim_get_counter */
neon_status_t neon_tim_get_counter(TIM_TypeDef *instance, uint8_t channel, uint32_t *value) {
    if ((tim_validate_access(instance, channel) != NEON_OK) || (value == NULL)) {
        return NEON_INVALID_PARAM;
    }
    *value = *tim_counter_reg(instance, channel);
    return NEON_OK;
}

/** @copydoc neon_tim_set_counter_mode */
neon_status_t neon_tim_set_counter_mode(TIM_TypeDef *instance, uint8_t channel, neon_tim_counter_mode_t mode) {
    if ((mode != NEON_TIM_COUNTERMODE_UP) && (mode != NEON_TIM_COUNTERMODE_DOWN)) {
        return NEON_INVALID_PARAM;
    }
    return tim_set_control_bit(instance, channel, TIM_CTRL_CH0_DIRECTION_Msk,
                               (mode == NEON_TIM_COUNTERMODE_DOWN) ? 1U : 0U);
}

/** @copydoc neon_tim_get_counter_mode */
neon_status_t neon_tim_get_counter_mode(TIM_TypeDef *instance, uint8_t channel, neon_tim_counter_mode_t *mode) {
    neon_field8_t enabled = 0U;
    neon_status_t status;

    if (mode == NULL) {
        return NEON_INVALID_PARAM;
    }
    status = tim_get_control_bit(instance, channel, TIM_CTRL_CH0_DIRECTION_Msk, (uint8_t *)&enabled);
    if (status == NEON_OK) {
        *mode = (enabled != 0U) ? NEON_TIM_COUNTERMODE_DOWN : NEON_TIM_COUNTERMODE_UP;
    }
    return status;
}

/** @copydoc neon_tim_enable_update_event */
neon_status_t neon_tim_enable_update_event(TIM_TypeDef *instance, uint8_t channel) {
    /* UPDATE_DIS có logic đảo: bit 0 mới là enable. */
    return tim_set_control_bit(instance, channel, TIM_CTRL_CH0_UPDATE_DIS_Msk, 0U);
}

/** @copydoc neon_tim_disable_update_event */
neon_status_t neon_tim_disable_update_event(TIM_TypeDef *instance, uint8_t channel) {
    return tim_set_control_bit(instance, channel, TIM_CTRL_CH0_UPDATE_DIS_Msk, 1U);
}

/** @copydoc neon_tim_is_update_event_enabled */
neon_status_t neon_tim_is_update_event_enabled(TIM_TypeDef *instance, uint8_t channel, uint8_t *enabled) {
    neon_status_t status;
    neon_field8_t update_disabled = 0U;

    if (enabled == NULL) {
        return NEON_INVALID_PARAM;
    }
    status = tim_get_control_bit(instance, channel, TIM_CTRL_CH0_UPDATE_DIS_Msk, (uint8_t *)&update_disabled);
    if (status == NEON_OK) {
        const uint32_t result_word = (update_disabled == 0U) ? 1U : 0U;
        memcpy(enabled, &result_word, 1U);
    }
    return status;
}

/** @copydoc neon_tim_enable_auto_reload */
neon_status_t neon_tim_enable_auto_reload(TIM_TypeDef *instance, uint8_t channel) {
    return tim_set_control_bit(instance, channel, TIM_CTRL_CH0_AUTO_RELOAD_Msk, 1U);
}

/** @copydoc neon_tim_disable_auto_reload */
neon_status_t neon_tim_disable_auto_reload(TIM_TypeDef *instance, uint8_t channel) {
    return tim_set_control_bit(instance, channel, TIM_CTRL_CH0_AUTO_RELOAD_Msk, 0U);
}

/** @copydoc neon_tim_is_auto_reload_enabled */
neon_status_t neon_tim_is_auto_reload_enabled(TIM_TypeDef *instance, uint8_t channel, uint8_t *enabled) {
    return tim_get_control_bit(instance, channel, TIM_CTRL_CH0_AUTO_RELOAD_Msk, enabled);
}

/** @copydoc neon_tim_enable_one_pulse_mode */
neon_status_t neon_tim_enable_one_pulse_mode(TIM_TypeDef *instance, uint8_t channel) {
    if (tim_validate_access(instance, channel) != NEON_OK) {
        return NEON_INVALID_PARAM;
    }
    return NEON_NOT_SUPPORTED;
}

/** @copydoc neon_tim_disable_one_pulse_mode */
neon_status_t neon_tim_disable_one_pulse_mode(TIM_TypeDef *instance, uint8_t channel) {
    return tim_set_control_bit(instance, channel, TIM_CTRL_CH0_ONE_PULSE_Msk, 0U);
}

/** @copydoc neon_tim_is_one_pulse_mode_enabled */
neon_status_t neon_tim_is_one_pulse_mode_enabled(TIM_TypeDef *instance, uint8_t channel, uint8_t *enabled) {
    return tim_get_control_bit(instance, channel, TIM_CTRL_CH0_ONE_PULSE_Msk, enabled);
}

/** @copydoc neon_tim_enable_center_aligned_mode */
neon_status_t neon_tim_enable_center_aligned_mode(TIM_TypeDef *instance, uint8_t channel) {
    if (tim_validate_access(instance, channel) != NEON_OK) {
        return NEON_INVALID_PARAM;
    }
    return NEON_NOT_SUPPORTED;
}

/** @copydoc neon_tim_disable_center_aligned_mode */
neon_status_t neon_tim_disable_center_aligned_mode(TIM_TypeDef *instance, uint8_t channel) {
    return tim_set_control_bit(instance, channel, TIM_CTRL_CH0_CENTER_ALIGN_Msk, 0U);
}

/** @copydoc neon_tim_is_center_aligned_mode_enabled */
neon_status_t neon_tim_is_center_aligned_mode_enabled(TIM_TypeDef *instance, uint8_t channel, uint8_t *enabled) {
    return tim_get_control_bit(instance, channel, TIM_CTRL_CH0_CENTER_ALIGN_Msk, enabled);
}

/** @copydoc neon_tim_set_compare_value */
neon_status_t neon_tim_set_compare_value(TIM_TypeDef *instance, uint8_t channel, uint8_t compare_channel,
                                         uint32_t value) {
    if ((tim_validate_access(instance, channel) != NEON_OK) || (tim_is_valid_compare_channel(compare_channel) == 0U)) {
        return NEON_INVALID_PARAM;
    }
    *tim_compare_reg(instance, channel, compare_channel) = value;
    return NEON_OK;
}

/** @copydoc neon_tim_get_compare_value */
neon_status_t neon_tim_get_compare_value(TIM_TypeDef *instance, uint8_t channel, uint8_t compare_channel,
                                         uint32_t *value) {
    if ((tim_validate_access(instance, channel) != NEON_OK) || (tim_is_valid_compare_channel(compare_channel) == 0U) ||
        (value == NULL)) {
        return NEON_INVALID_PARAM;
    }
    *value = *tim_compare_reg(instance, channel, compare_channel);
    return NEON_OK;
}

/** @copydoc neon_tim_enable_compare_channel */
neon_status_t neon_tim_enable_compare_channel(TIM_TypeDef *instance, uint8_t channel, uint8_t compare_channel) {
    if (tim_is_valid_compare_channel(compare_channel) == 0U) {
        return NEON_INVALID_PARAM;
    }
    return tim_set_control_bit(instance, channel, tim_compare_enable_mask(compare_channel), 1U);
}

/** @copydoc neon_tim_disable_compare_channel */
neon_status_t neon_tim_disable_compare_channel(TIM_TypeDef *instance, uint8_t channel, uint8_t compare_channel) {
    if (tim_is_valid_compare_channel(compare_channel) == 0U) {
        return NEON_INVALID_PARAM;
    }
    return tim_set_control_bit(instance, channel, tim_compare_enable_mask(compare_channel), 0U);
}

/** @copydoc neon_tim_is_compare_channel_enabled */
neon_status_t neon_tim_is_compare_channel_enabled(TIM_TypeDef *instance, uint8_t channel, uint8_t compare_channel,
                                                  uint8_t *enabled) {
    if (tim_is_valid_compare_channel(compare_channel) == 0U) {
        return NEON_INVALID_PARAM;
    }
    return tim_get_control_bit(instance, channel, tim_compare_enable_mask(compare_channel), enabled);
}

/** @copydoc neon_tim_enable_interrupt */
neon_status_t neon_tim_enable_interrupt(TIM_TypeDef *instance, uint8_t channel, uint32_t interrupts) {
    volatile uint32_t *reg;
    uint32_t primask;

    if ((tim_validate_access(instance, channel) != NEON_OK) || (tim_is_valid_interrupt_mask(interrupts, 0U) == 0U)) {
        return NEON_INVALID_PARAM;
    }

    reg = tim_interrupt_reg(instance, channel);
    primask = tim_enter_critical();
    *reg |= tim_interrupts_to_hw(interrupts);
    tim_exit_critical(primask);
    return NEON_OK;
}

/** @copydoc neon_tim_disable_interrupt */
neon_status_t neon_tim_disable_interrupt(TIM_TypeDef *instance, uint8_t channel, uint32_t interrupts) {
    volatile uint32_t *reg;
    uint32_t primask;

    if ((tim_validate_access(instance, channel) != NEON_OK) || (tim_is_valid_interrupt_mask(interrupts, 0U) == 0U)) {
        return NEON_INVALID_PARAM;
    }

    reg = tim_interrupt_reg(instance, channel);
    primask = tim_enter_critical();
    *reg &= ~tim_interrupts_to_hw(interrupts);
    tim_exit_critical(primask);
    return NEON_OK;
}

/** @copydoc neon_tim_is_interrupt_enabled */
neon_status_t neon_tim_is_interrupt_enabled(TIM_TypeDef *instance, uint8_t channel, uint32_t interrupts,
                                            uint8_t *enabled) {
    uint32_t hardware_mask;

    if ((tim_validate_access(instance, channel) != NEON_OK) || (tim_is_valid_interrupt_mask(interrupts, 0U) == 0U) ||
        (enabled == NULL)) {
        return NEON_INVALID_PARAM;
    }

    hardware_mask = tim_interrupts_to_hw(interrupts);
    const uint32_t result_word = ((*tim_interrupt_reg(instance, channel) & hardware_mask) == hardware_mask) ? 1U : 0U;
    memcpy(enabled, &result_word, 1U);
    return NEON_OK;
}

/** @copydoc neon_tim_get_active_flags */
neon_status_t neon_tim_get_active_flags(TIM_TypeDef *instance, uint8_t channel, uint32_t *flags) {
    if ((tim_validate_access(instance, channel) != NEON_OK) || (flags == NULL)) {
        return NEON_INVALID_PARAM;
    }

    /* STT reports live event levels on FPGA; reading does not clear them. */
    *flags = tim_flags_from_hw(*tim_status_reg(instance, channel));
    return NEON_OK;
}

/** @copydoc neon_tim_is_active_flag */
neon_status_t neon_tim_is_active_flag(TIM_TypeDef *instance, uint8_t channel, uint32_t flags, uint8_t *active) {
    uint32_t active_flags;
    neon_status_t status;

    if ((tim_is_valid_flag_mask(flags) == 0U) || (active == NULL)) {
        return NEON_INVALID_PARAM;
    }

    status = neon_tim_get_active_flags(instance, channel, &active_flags);
    if (status == NEON_OK) {
        const uint32_t result_word = ((active_flags & flags) == flags) ? 1U : 0U;
        memcpy(active, &result_word, 1U);
    }
    return status;
}

/** @copydoc neon_tim_clear_flag */
neon_status_t neon_tim_clear_flag(TIM_TypeDef *instance, uint8_t channel, uint32_t flags) {
    if ((tim_validate_access(instance, channel) != NEON_OK) || (tim_is_valid_flag_mask(flags) == 0U)) {
        return NEON_INVALID_PARAM;
    }

    /* FPGA STT is read-only and not read-to-clear. Live flags drop when the
   * counter leaves the event condition; there is no software clear command. */
    return NEON_NOT_SUPPORTED;
}

/** @copydoc neon_tim_get_clock_status */
neon_status_t neon_tim_get_clock_status(TIM_TypeDef *instance, neon_tim_clock_status_t *status) {
    uint32_t value;

    if ((tim_is_valid_instance(instance) == 0U) || (status == NULL)) {
        return NEON_INVALID_PARAM;
    }

    value = instance->CLKS;
    status->clock_failure = ((value & TIM_CLKS_CLK_FAILURE_Msk) != 0U) ? 1U : 0U;
    status->failed_command =
        ((value & TIM_CLKS_CMD_FAILURE_Msk) != 0U) ? NEON_TIM_FAILED_COMMAND_READ : NEON_TIM_FAILED_COMMAND_WRITE;
    status->failure_address = (uint16_t)((value & TIM_CLKS_ADDR_FAILURE_Msk) >> TIM_CLKS_ADDR_FAILURE_Pos);
    return NEON_OK;
}

/*==================================================================================================
*                                        PRIVATE FUNCTIONS
==================================================================================================*/

static uint8_t tim_is_valid_instance(const TIM_TypeDef *instance) {
    return ((instance == TIM0) || (instance == TIM1) || (instance == TIM2) || (instance == TIM3) ||
            (instance == TIM4) || (instance == TIM5) || (instance == TIM6) || (instance == TIM7))
               ? 1U
               : 0U;
}

static uint8_t tim_is_valid_channel(uint8_t channel) {
    return ((channel == NEON_TIM_CHANNEL_0) || (channel == NEON_TIM_CHANNEL_1)) ? 1U : 0U;
}

static uint8_t tim_is_valid_compare_channel(uint8_t channel) {
    return ((channel >= NEON_TIM_COMPARE_1) && (channel <= NEON_TIM_COMPARE_4)) ? 1U : 0U;
}

static uint8_t tim_is_valid_interrupt_mask(uint32_t interrupts, uint8_t allow_empty) {
    if ((!allow_empty && (interrupts == 0U)) || ((interrupts & ~(uint32_t)NEON_TIM_INTERRUPT_ALL) != 0U)) {
        return 0U;
    }
    return 1U;
}

static uint8_t tim_is_valid_flag_mask(uint32_t flags) {
    return ((flags != 0U) && ((flags & ~(uint32_t)NEON_TIM_FLAG_ALL) == 0U)) ? 1U : 0U;
}

static neon_status_t tim_validate_access(const TIM_TypeDef *instance, uint8_t channel) {
    if ((tim_is_valid_instance(instance) == 0U) || (tim_is_valid_channel(channel) == 0U)) {
        return NEON_INVALID_PARAM;
    }
    return NEON_OK;
}

static neon_status_t tim_validate_config(const neon_tim_config_t *config) {
    if (config == NULL) {
        return NEON_INVALID_PARAM;
    }

    if ((config->mode == NEON_TIM_MODE_PWM0) || (config->mode == NEON_TIM_MODE_PWM1)) {
        return NEON_NOT_SUPPORTED;
    }

    if (((config->mode != NEON_TIM_MODE_DISABLE) && (config->mode != NEON_TIM_MODE_NORMAL)) ||
        ((config->counter_mode != NEON_TIM_COUNTERMODE_UP) && (config->counter_mode != NEON_TIM_COUNTERMODE_DOWN)) ||
        ((config->auto_reload_mode != NEON_TIM_AUTORELOAD_DISABLE) &&
         (config->auto_reload_mode != NEON_TIM_AUTORELOAD_ENABLE)) ||
        ((config->one_pulse_mode != NEON_TIM_ONE_PULSE_REPETITIVE) &&
         (config->one_pulse_mode != NEON_TIM_ONE_PULSE_SINGLE)) ||
        ((config->center_aligned_mode != NEON_TIM_CENTER_ALIGNED_DISABLE) &&
         (config->center_aligned_mode != NEON_TIM_CENTER_ALIGNED_ENABLE)) ||
        (tim_is_valid_interrupt_mask(config->interrupts, 1U) == 0U)) {
        return NEON_INVALID_PARAM;
    }

    /* FU2 v8 chỉ áp dụng ONE_PULSE và CENTER_ALIGN cho PWM0/PWM1. Driver hiện
   * chưa hỗ trợ PWM nên không được báo thành công cho cấu hình vô hiệu. */
    if ((config->one_pulse_mode == NEON_TIM_ONE_PULSE_SINGLE) ||
        (config->center_aligned_mode == NEON_TIM_CENTER_ALIGNED_ENABLE)) {
        return NEON_NOT_SUPPORTED;
    }

    return NEON_OK;
}

static uint32_t tim_enter_critical(void) {
    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    return primask;
}

static void tim_exit_critical(uint32_t primask) {
    __set_PRIMASK(primask);
}

static volatile uint32_t *tim_prescaler_reg(TIM_TypeDef *instance, uint8_t channel) {
    return (channel == NEON_TIM_CHANNEL_1) ? &instance->PRSC_CH1 : &instance->PRSC_CH0;
}

static volatile uint32_t *tim_auto_reload_reg(TIM_TypeDef *instance, uint8_t channel) {
    return (channel == NEON_TIM_CHANNEL_1) ? &instance->CHL_CH1 : &instance->CHL_CH0;
}

static const volatile uint32_t *tim_counter_reg(const TIM_TypeDef *instance, uint8_t channel) {
    return (channel == NEON_TIM_CHANNEL_1) ? &instance->CNT_CH1 : &instance->CNT_CH0;
}

static volatile uint32_t *tim_control_reg(TIM_TypeDef *instance, uint8_t channel) {
    return (channel == NEON_TIM_CHANNEL_1) ? &instance->CTRL_CH1 : &instance->CTRL_CH0;
}

static volatile uint32_t *tim_interrupt_reg(TIM_TypeDef *instance, uint8_t channel) {
    return (channel == NEON_TIM_CHANNEL_1) ? &instance->INTR_EN_CH1 : &instance->INTR_EN_CH0;
}

static const volatile uint32_t *tim_status_reg(TIM_TypeDef *instance, uint8_t channel) {
    return (channel == NEON_TIM_CHANNEL_1) ? &instance->STT_CH1 : &instance->STT_CH0;
}

static volatile uint32_t *tim_compare_reg(TIM_TypeDef *instance, uint8_t channel, uint8_t compare_channel) {
    if (channel == NEON_TIM_CHANNEL_1) {
        switch (compare_channel) {
            case NEON_TIM_COMPARE_2:
                return &instance->CCR2_CH1;
            case NEON_TIM_COMPARE_3:
                return &instance->CCR3_CH1;
            case NEON_TIM_COMPARE_4:
                return &instance->CCR4_CH1;
            default:
                return &instance->CCR1_CH1;
        }
    }

    switch (compare_channel) {
        case NEON_TIM_COMPARE_2:
            return &instance->CCR2_CH0;
        case NEON_TIM_COMPARE_3:
            return &instance->CCR3_CH0;
        case NEON_TIM_COMPARE_4:
            return &instance->CCR4_CH0;
        default:
            return &instance->CCR1_CH0;
    }
}

static uint32_t tim_compare_enable_mask(uint8_t compare_channel) {
    switch (compare_channel) {
        case NEON_TIM_COMPARE_2:
            return TIM_CTRL_CH0_CC2_EN_Msk;
        case NEON_TIM_COMPARE_3:
            return TIM_CTRL_CH0_CC3_EN_Msk;
        case NEON_TIM_COMPARE_4:
            return TIM_CTRL_CH0_CC4_EN_Msk;
        default:
            return TIM_CTRL_CH0_CC1_EN_Msk;
    }
}

static neon_crm_peripheral_t tim_crm_peripheral(const TIM_TypeDef *instance) {
    if (instance == TIM1) {
        return NEON_CRM_PERIPHERAL_TIMER1;
    }
    if (instance == TIM2) {
        return NEON_CRM_PERIPHERAL_TIMER2;
    }
    if (instance == TIM3) {
        return NEON_CRM_PERIPHERAL_TIMER3;
    }
    if (instance == TIM4) {
        return NEON_CRM_PERIPHERAL_TIMER4;
    }
    if (instance == TIM5) {
        return NEON_CRM_PERIPHERAL_TIMER5;
    }
    if (instance == TIM6) {
        return NEON_CRM_PERIPHERAL_TIMER6;
    }
    if (instance == TIM7) {
        return NEON_CRM_PERIPHERAL_TIMER7;
    }
    return NEON_CRM_PERIPHERAL_TIMER0;
}

static uint32_t tim_interrupts_to_hw(uint32_t interrupts) {
    uint32_t result = 0U;

    if ((interrupts & NEON_TIM_INTERRUPT_UPDATE) != 0U) {
        result |= TIM_INTR_EN_CH0_UP_INTR_EN_Msk;
    }
    if ((interrupts & NEON_TIM_INTERRUPT_CC1) != 0U) {
        result |= TIM_INTR_EN_CH0_CC1_INTR_EN_Msk;
    }
    if ((interrupts & NEON_TIM_INTERRUPT_CC2) != 0U) {
        result |= TIM_INTR_EN_CH0_CC2_INTR_EN_Msk;
    }
    if ((interrupts & NEON_TIM_INTERRUPT_CC3) != 0U) {
        result |= TIM_INTR_EN_CH0_CC3_INTR_EN_Msk;
    }
    if ((interrupts & NEON_TIM_INTERRUPT_CC4) != 0U) {
        result |= TIM_INTR_EN_CH0_CC4_INTR_EN_Msk;
    }
    if ((interrupts & NEON_TIM_INTERRUPT_BREAK) != 0U) {
        result |= TIM_INTR_EN_CH0_BREAK_INTR_EN_Msk;
    }
    return result;
}

static uint32_t tim_flags_from_hw(uint32_t value) {
    uint32_t result = 0U;

    if ((value & TIM_STT_CH0_UP_FLAG_Msk) != 0U) {
        result |= NEON_TIM_FLAG_UPDATE;
    }
    if ((value & TIM_STT_CH0_CC1_FLAG_Msk) != 0U) {
        result |= NEON_TIM_FLAG_CC1;
    }
    if ((value & TIM_STT_CH0_CC2_FLAG_Msk) != 0U) {
        result |= NEON_TIM_FLAG_CC2;
    }
    if ((value & TIM_STT_CH0_CC3_FLAG_Msk) != 0U) {
        result |= NEON_TIM_FLAG_CC3;
    }
    if ((value & TIM_STT_CH0_CC4_FLAG_Msk) != 0U) {
        result |= NEON_TIM_FLAG_CC4;
    }
    if ((value & TIM_STT_CH0_BREAK_FLAG_Msk) != 0U) {
        result |= NEON_TIM_FLAG_BREAK;
    }
    return result;
}

static neon_status_t tim_set_control_bit(TIM_TypeDef *instance, uint8_t channel, uint32_t mask, uint8_t enabled) {
    volatile uint32_t *reg;
    uint32_t primask;

    if (tim_validate_access(instance, channel) != NEON_OK) {
        return NEON_INVALID_PARAM;
    }

    reg = tim_control_reg(instance, channel);
    primask = tim_enter_critical();
    if (enabled != 0U) {
        *reg |= mask;
    } else {
        *reg &= ~mask;
    }
    tim_exit_critical(primask);
    return NEON_OK;
}

static neon_status_t tim_get_control_bit(TIM_TypeDef *instance, uint8_t channel, uint32_t mask, uint8_t *enabled) {
    if ((tim_validate_access(instance, channel) != NEON_OK) || (enabled == NULL)) {
        return NEON_INVALID_PARAM;
    }

    const uint32_t result_word = ((*tim_control_reg(instance, channel) & mask) != 0U) ? 1U : 0U;
    memcpy(enabled, &result_word, 1U);
    return NEON_OK;
}
