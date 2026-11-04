/**
 * @file neon_tim.h
 * @brief Public declarations for timer.
 * @details
 * * NEON find - HAL timer for FU2.
 *
 * Each TIM0..TIM7 contains two independent counting channels CH0/CH1. Each channel has four compare
 * registers, a control register, interrupt and its own state. The driver only publicly functions
 * that have been validated by FU2 LL; the two raw PWM modes retain names for source compatibility
 * but neon_tim_init() returns NEON_NOT_SUPPORTED because LL has not yet described route output,
 * polarity and duty rules.
 * @note Blocking polling/configuration operations are intended for task context.
 * Serialize configuration/teardown with other users of the same peripheral;
 * timeout_polls values are polling budgets, not milliseconds, where applicable.
 */

#ifndef __NEON_TIM_H__
#define __NEON_TIM_H__

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
    NEON_TIM_CHANNEL_0 = 0,
    NEON_TIM_CHANNEL_1 = 1,
} neon_tim_channel_t;

typedef enum {
    NEON_TIM_COMPARE_1 = 1,
    NEON_TIM_COMPARE_2 = 2,
    NEON_TIM_COMPARE_3 = 3,
    NEON_TIM_COMPARE_4 = 4,
} neon_tim_compare_channel_t;

typedef enum {
    NEON_TIM_MODE_DISABLE = 0,
    NEON_TIM_MODE_NORMAL = 1,
    /* Only the raw hardware mode code; config now returns NEON_NOT_SUPPORTED. */
    NEON_TIM_MODE_PWM0 = 2,
    NEON_TIM_MODE_PWM1 = 3,
} neon_tim_mode_t;

typedef enum {
    NEON_TIM_COUNTERMODE_UP = 0,
    NEON_TIM_COUNTERMODE_DOWN = 1,
} neon_tim_counter_mode_t;

typedef enum {
    NEON_TIM_AUTORELOAD_DISABLE = 0,
    NEON_TIM_AUTORELOAD_ENABLE = 1,
} neon_tim_auto_reload_t;

typedef enum {
    NEON_TIM_ONE_PULSE_REPETITIVE = 0,
    NEON_TIM_ONE_PULSE_SINGLE = 1,
} neon_tim_one_pulse_mode_t;

typedef enum {
    NEON_TIM_CENTER_ALIGNED_DISABLE = 0,
    NEON_TIM_CENTER_ALIGNED_ENABLE = 1,
} neon_tim_center_aligned_mode_t;

typedef uint32_t neon_tim_interrupt_t;
#define NEON_TIM_INTERRUPT_UPDATE ((1U << 0))
#define NEON_TIM_INTERRUPT_CC1 ((1U << 1))
#define NEON_TIM_INTERRUPT_CC2 ((1U << 2))
#define NEON_TIM_INTERRUPT_CC3 ((1U << 3))
#define NEON_TIM_INTERRUPT_CC4 ((1U << 4))
#define NEON_TIM_INTERRUPT_BREAK ((1U << 5))
#define NEON_TIM_INTERRUPT_ALL (0x3FU)

typedef uint32_t neon_tim_flag_t;
#define NEON_TIM_FLAG_UPDATE ((1U << 0))
#define NEON_TIM_FLAG_CC1 ((1U << 1))
#define NEON_TIM_FLAG_CC2 ((1U << 2))
#define NEON_TIM_FLAG_CC3 ((1U << 3))
#define NEON_TIM_FLAG_CC4 ((1U << 4))
#define NEON_TIM_FLAG_BREAK ((1U << 5))
#define NEON_TIM_FLAG_ALL (0x3FU)

typedef enum {
    NEON_TIM_FAILED_COMMAND_WRITE = 0,
    NEON_TIM_FAILED_COMMAND_READ = 1,
} neon_tim_failed_command_t;

typedef struct {
    /* Tick timer = clock TIMx / (prescaler + 1), according to DTI Timer User Guide. */
    uint32_t prescaler;
    uint32_t auto_reload;                 /* 32-bit CHL_CHx value. */
    neon_tim_mode_t mode;                 /* Disable or normal. */
    neon_tim_counter_mode_t counter_mode; /* Count up or down. */
    /* AUTO_RELOAD bit controls CNT reload from CHL; this is not the register/preload shadow like the STM32
   * timer. */
    neon_tim_auto_reload_t auto_reload_mode;
    neon_tim_one_pulse_mode_t one_pulse_mode;
    neon_tim_center_aligned_mode_t center_aligned_mode;
    uint32_t interrupts; /* Complex NEON_TIM_INTERRUPT_*. */
} neon_tim_config_t;

typedef struct {
    neon_field8_t clock_failure;
    /* It only makes sense when clock_failure=1. */
    neon_tim_failed_command_t failed_command;
    neon_field16_t failure_address; /* CLKS.ADDR_FAILURE[9:1], translated to bit 0. */
} neon_tim_clock_status_t;

/*==================================================================================================
*                                       FUNCTION PROTOTYPES
==================================================================================================*/
/**
 * @brief Fill the default configuration for timer.
 * @param[out] config Module configuration.
 * @details
 * Default according to FU2's reset: prescaler=0, reload=0, timer stop, count up,
 * auto-reload/one-pulse/center-aligned/interrupt are all off.
 */
void neon_tim_get_default_config(neon_tim_config_t *config);

/**
 * @brief Initialize timer.
 * @param[in] instance Peripheral register instance.
 * @param[in] channel channel.
 * @param[in] init Initialization settings.
 * @return NEON_OK on success, or a module error/status code.
 * @details
 * config turns on the clock itself and releases instance resets but does not pulse full peripheral
 * resets, as a single TIMx reset will cause the configuration of both CH0 and CH1 to be lost.
 */
neon_status_t neon_tim_init(TIM_TypeDef *instance, uint8_t channel, const neon_tim_config_t *init);

/**
 * @brief Deinitialize timer.
 * @param[in] instance Peripheral register instance.
 * @param[in] channel channel.
 * @return NEON_OK on success, or a module error/status code.
 * @details
 * DeInit only returns the selected channel to reset value, does not turn off the clock and does not
 * affect the other channel in the same instance.
 */
neon_status_t neon_tim_deinit(TIM_TypeDef *instance, uint8_t channel);

/**
 * @brief Start timer.
 * @param[in] instance Peripheral register instance.
 * @param[in] channel channel.
 * @return NEON_OK on success, or a module error/status code.
 */
neon_status_t neon_tim_start(TIM_TypeDef *instance, uint8_t channel);
/**
 * @brief Stop timer.
 * @param[in] instance Peripheral register instance.
 * @param[in] channel channel.
 * @return NEON_OK on success, or a module error/status code.
 */
neon_status_t neon_tim_stop(TIM_TypeDef *instance, uint8_t channel);
/**
 * @brief Set timer mode.
 * @param[in] instance Peripheral register instance.
 * @param[in] channel channel.
 * @param[in] mode mode.
 * @return NEON_OK on success, or a module error/status code.
 */
neon_status_t neon_tim_set_mode(TIM_TypeDef *instance, uint8_t channel, neon_tim_mode_t mode);
/**
 * @brief Read timer mode.
 * @param[in] instance Peripheral register instance.
 * @param[in] channel channel.
 * @param[in,out] mode mode.
 * @return NEON_OK on success, or a module error/status code.
 */
neon_status_t neon_tim_get_mode(TIM_TypeDef *instance, uint8_t channel, neon_tim_mode_t *mode);
/**
 * @brief Check timer running.
 * @param[in] instance Peripheral register instance.
 * @param[in] channel channel.
 * @param[in,out] running Running.
 * @return NEON_OK on success, or a module error/status code.
 */
neon_status_t neon_tim_is_running(TIM_TypeDef *instance, uint8_t channel, uint8_t *running);

/**
 * @brief Set timer prescaler.
 * @param[in] instance Peripheral register instance.
 * @param[in] channel channel.
 * @param[in] value Value.
 * @return NEON_OK on success, or a module error/status code.
 */
neon_status_t neon_tim_set_prescaler(TIM_TypeDef *instance, uint8_t channel, uint32_t value);
/**
 * @brief Read timer prescaler.
 * @param[in] instance Peripheral register instance.
 * @param[in] channel channel.
 * @param[out] value Value.
 * @return NEON_OK on success, or a module error/status code.
 */
neon_status_t neon_tim_get_prescaler(TIM_TypeDef *instance, uint8_t channel, uint32_t *value);
/**
 * @brief Set timer auto reload.
 * @param[in] instance Peripheral register instance.
 * @param[in] channel channel.
 * @param[in] value Value.
 * @return NEON_OK on success, or a module error/status code.
 */
neon_status_t neon_tim_set_auto_reload(TIM_TypeDef *instance, uint8_t channel, uint32_t value);
/**
 * @brief Read timer auto reload.
 * @param[in] instance Peripheral register instance.
 * @param[in] channel channel.
 * @param[out] value Value.
 * @return NEON_OK on success, or a module error/status code.
 */
neon_status_t neon_tim_get_auto_reload(TIM_TypeDef *instance, uint8_t channel, uint32_t *value);
/**
 * @brief Read timer counter.
 * @param[in] instance Peripheral register instance.
 * @param[in] channel channel.
 * @param[out] value Value.
 * @return NEON_OK on success, or a module error/status code.
 */
neon_status_t neon_tim_get_counter(TIM_TypeDef *instance, uint8_t channel, uint32_t *value);

/**
 * @brief Set timer counter mode.
 * @param[in] instance Peripheral register instance.
 * @param[in] channel channel.
 * @param[in] mode mode.
 * @return NEON_OK on success, or a module error/status code.
 */
neon_status_t neon_tim_set_counter_mode(TIM_TypeDef *instance, uint8_t channel, neon_tim_counter_mode_t mode);
/**
 * @brief Read timer counter mode.
 * @param[in] instance Peripheral register instance.
 * @param[in] channel channel.
 * @param[in,out] mode mode.
 * @return NEON_OK on success, or a module error/status code.
 */
neon_status_t neon_tim_get_counter_mode(TIM_TypeDef *instance, uint8_t channel, neon_tim_counter_mode_t *mode);

/**
 * @brief enable timer update event.
 * @param[in] instance Peripheral register instance.
 * @param[in] channel channel.
 * @return NEON_OK on success, or a module error/status code.
 */
neon_status_t neon_tim_enable_update_event(TIM_TypeDef *instance, uint8_t channel);
/**
 * @brief Disable timer update event.
 * @param[in] instance Peripheral register instance.
 * @param[in] channel channel.
 * @return NEON_OK on success, or a module error/status code.
 */
neon_status_t neon_tim_disable_update_event(TIM_TypeDef *instance, uint8_t channel);
/**
 * @brief Check timer update event enabled.
 * @param[in] instance Peripheral register instance.
 * @param[in] channel channel.
 * @param[in,out] enabled Enabled.
 * @return NEON_OK on success, or a module error/status code.
 */
neon_status_t neon_tim_is_update_event_enabled(TIM_TypeDef *instance, uint8_t channel, uint8_t *enabled);

/**
 * @brief enable timer auto reload.
 * @param[in] instance Peripheral register instance.
 * @param[in] channel channel.
 * @return NEON_OK on success, or a module error/status code.
 */
neon_status_t neon_tim_enable_auto_reload(TIM_TypeDef *instance, uint8_t channel);
/**
 * @brief Disable timer auto reload.
 * @param[in] instance Peripheral register instance.
 * @param[in] channel channel.
 * @return NEON_OK on success, or a module error/status code.
 */
neon_status_t neon_tim_disable_auto_reload(TIM_TypeDef *instance, uint8_t channel);
/**
 * @brief Check timer auto reload enabled.
 * @param[in] instance Peripheral register instance.
 * @param[in] channel channel.
 * @param[in,out] enabled Enabled.
 * @return NEON_OK on success, or a module error/status code.
 */
neon_status_t neon_tim_is_auto_reload_enabled(TIM_TypeDef *instance, uint8_t channel, uint8_t *enabled);

/**
 * @brief enable timer one pulse mode.
 * @param[in] instance Peripheral register instance.
 * @param[in] channel channel.
 * @return NEON_OK on success, or a module error/status code.
 */
neon_status_t neon_tim_enable_one_pulse_mode(TIM_TypeDef *instance, uint8_t channel);
/**
 * @brief Disable timer one pulse mode.
 * @param[in] instance Peripheral register instance.
 * @param[in] channel channel.
 * @return NEON_OK on success, or a module error/status code.
 */
neon_status_t neon_tim_disable_one_pulse_mode(TIM_TypeDef *instance, uint8_t channel);
/**
 * @brief Check timer one pulse mode enabled.
 * @param[in] instance Peripheral register instance.
 * @param[in] channel channel.
 * @param[in,out] enabled Enabled.
 * @return NEON_OK on success, or a module error/status code.
 */
neon_status_t neon_tim_is_one_pulse_mode_enabled(TIM_TypeDef *instance, uint8_t channel, uint8_t *enabled);

/**
 * @brief enable timer center aligned mode.
 * @param[in] instance Peripheral register instance.
 * @param[in] channel channel.
 * @return NEON_OK on success, or a module error/status code.
 */
neon_status_t neon_tim_enable_center_aligned_mode(TIM_TypeDef *instance, uint8_t channel);
/**
 * @brief Disable timer center aligned mode.
 * @param[in] instance Peripheral register instance.
 * @param[in] channel channel.
 * @return NEON_OK on success, or a module error/status code.
 */
neon_status_t neon_tim_disable_center_aligned_mode(TIM_TypeDef *instance, uint8_t channel);
/**
 * @brief Check timer center aligned mode enabled.
 * @param[in] instance Peripheral register instance.
 * @param[in] channel channel.
 * @param[in,out] enabled Enabled.
 * @return NEON_OK on success, or a module error/status code.
 */
neon_status_t neon_tim_is_center_aligned_mode_enabled(TIM_TypeDef *instance, uint8_t channel, uint8_t *enabled);

/**
 * @brief Set timer compare value.
 * @param[in] instance Peripheral register instance.
 * @param[in] channel channel.
 * @param[in] compare_channel Compare channel.
 * @param[in] value Value.
 * @return NEON_OK on success, or a module error/status code.
 */
neon_status_t neon_tim_set_compare_value(TIM_TypeDef *instance, uint8_t channel, uint8_t compare_channel,
                                         uint32_t value);
/**
 * @brief Read timer compare value.
 * @param[in] instance Peripheral register instance.
 * @param[in] channel channel.
 * @param[in] compare_channel Compare channel.
 * @param[out] value Value.
 * @return NEON_OK on success, or a module error/status code.
 */
neon_status_t neon_tim_get_compare_value(TIM_TypeDef *instance, uint8_t channel, uint8_t compare_channel,
                                         uint32_t *value);
/**
 * @brief enable timer compare channel.
 * @param[in] instance Peripheral register instance.
 * @param[in] channel channel.
 * @param[in] compare_channel Compare channel.
 * @return NEON_OK on success, or a module error/status code.
 */
neon_status_t neon_tim_enable_compare_channel(TIM_TypeDef *instance, uint8_t channel, uint8_t compare_channel);
/**
 * @brief Disable timer compare channel.
 * @param[in] instance Peripheral register instance.
 * @param[in] channel channel.
 * @param[in] compare_channel Compare channel.
 * @return NEON_OK on success, or a module error/status code.
 */
neon_status_t neon_tim_disable_compare_channel(TIM_TypeDef *instance, uint8_t channel, uint8_t compare_channel);
/**
 * @brief Check timer compare channel enabled.
 * @param[in] instance Peripheral register instance.
 * @param[in] channel channel.
 * @param[in] compare_channel Compare channel.
 * @param[in,out] enabled Enabled.
 * @return NEON_OK on success, or a module error/status code.
 */
neon_status_t neon_tim_is_compare_channel_enabled(TIM_TypeDef *instance, uint8_t channel, uint8_t compare_channel,
                                                  uint8_t *enabled);

/**
 * @brief enable timer interrupt.
 * @param[in] instance Peripheral register instance.
 * @param[in] channel channel.
 * @param[in] interrupts interrupt-source bitmask.
 * @return NEON_OK on success, or a module error/status code.
 * @details
 * The below APIs only control INTR_EN_CHx. The application must still enable the correct IRQ in NVIC;
 * FU2 v8 uses a separate IRQ for TIM0/TIM1 and three common IRQs for pairs TIM2/3, TIM4/5, TIM6/7.
 */
neon_status_t neon_tim_enable_interrupt(TIM_TypeDef *instance, uint8_t channel, uint32_t interrupts);
/**
 * @brief Disable timer interrupt.
 * @param[in] instance Peripheral register instance.
 * @param[in] channel channel.
 * @param[in] interrupts interrupt-source bitmask.
 * @return NEON_OK on success, or a module error/status code.
 */
neon_status_t neon_tim_disable_interrupt(TIM_TypeDef *instance, uint8_t channel, uint32_t interrupts);
/**
 * @brief Check timer interrupt enabled.
 * @param[in] instance Peripheral register instance.
 * @param[in] channel channel.
 * @param[in] interrupts interrupt-source bitmask.
 * @param[in,out] enabled Enabled.
 * @return NEON_OK on success, or a module error/status code.
 */
neon_status_t neon_tim_is_interrupt_enabled(TIM_TypeDef *instance, uint8_t channel, uint32_t interrupts,
                                            uint8_t *enabled);

/**
 * @brief Read timer active flags.
 * @param[in] instance Peripheral register instance.
 * @param[in] channel channel.
 * @param[out] flags Status-flag bitmask.
 * @return NEON_OK on success, or a module error/status code.
 * @details
 * STT_CH0/1 on FPGA is the current event state, no flag latching. GetActiveFlags/IsActiveFlag reads
 * only, does not erase flag or acknowledge IRQ. UPDATE automatically drops when counter leaves
 * reload point at next tick. Select prescaler large enough for ISR to read flag; wait for flag to
 * drop before exiting if IRQ is level.
 */
neon_status_t neon_tim_get_active_flags(TIM_TypeDef *instance, uint8_t channel, uint32_t *flags);
/**
 * @brief Check the selected status flags of timer.
 * @param[in] instance Peripheral register instance.
 * @param[in] channel channel.
 * @param[in] flags Status-flag bitmask.
 * @param[out] active Receives whether the selected flag is active.
 * @return NEON_OK on success, or a module error/status code.
 */
neon_status_t neon_tim_is_active_flag(TIM_TypeDef *instance, uint8_t channel, uint32_t flags, uint8_t *active);
/**
 * @brief Clear timer flag.
 * @param[in] instance Peripheral register instance.
 * @param[in] channel channel.
 * @param[in] flags Status-flag bitmask.
 * @return NEON_OK on success, or a module error/status code.
 * @details
 * Keep API for source compatibility; FU2 returns NEON_NOT_SUPPORTED because STT does not have a clear
 * command by software. Do not write STT: FPGA can report bus errors.
 */
neon_status_t neon_tim_clear_flag(TIM_TypeDef *instance, uint8_t channel, uint32_t flags);

/**
 * @brief Read timer clock status.
 * @param[in] instance Peripheral register instance.
 * @param[out] status Destination for the status snapshot.
 * @return NEON_OK on success, or a module error/status code.
 * @details
 * CLKS is shared for two channels. The function only read registers correctly once so that the flags
 * remain consistent if hardware has side effects when reading.
 */
neon_status_t neon_tim_get_clock_status(TIM_TypeDef *instance, neon_tim_clock_status_t *status);

#ifdef __cplusplus
}
#endif

#endif /* __NEON_TIM_H__ */
