/**
 * @file neon_tim.h
 * @brief Public declarations for timer.
 * @details
 * * NEON TIM - HAL timer dùng cho FU2.
 *
 * Mỗi TIM0..TIM7 chứa hai kênh đếm độc lập CH0/CH1. Mỗi kênh có bốn
 * thanh ghi compare, một thanh ghi điều khiển, ngắt và trạng thái riêng.
 * Driver chỉ công khai chức năng đã được FU2 LL xác nhận; hai mode PWM thô
 * vẫn giữ tên để tương thích source nhưng neon_tim_init() trả
 * NEON_NOT_SUPPORTED vì LL chưa mô tả route output, polarity và quy tắc duty.
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
    /* Chỉ là mã mode thô của phần cứng; config hiện trả NEON_NOT_SUPPORTED. */
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
    /* Tick timer = clock TIMx / (prescaler + 1), theo DTI Timer User Guide. */
    uint32_t prescaler;
    uint32_t auto_reload;                 /* Giá trị CHL_CHx 32-bit. */
    neon_tim_mode_t mode;                 /* Disable hoặc normal. */
    neon_tim_counter_mode_t counter_mode; /* Đếm lên hoặc xuống. */
    /* Bit AUTO_RELOAD điều khiển reload CNT từ CHL; đây không phải shadow
   * register/preload như timer STM32. */
    neon_tim_auto_reload_t auto_reload_mode;
    neon_tim_one_pulse_mode_t one_pulse_mode;
    neon_tim_center_aligned_mode_t center_aligned_mode;
    uint32_t interrupts; /* Tổ hợp NEON_TIM_INTERRUPT_*. */
} neon_tim_config_t;

typedef struct {
    neon_field8_t clock_failure;
    /* Chỉ có nghĩa khi clock_failure=1. */
    neon_tim_failed_command_t failed_command;
    neon_field16_t failure_address; /* CLKS.ADDR_FAILURE[9:1], đã dịch về bit 0. */
} neon_tim_clock_status_t;

/*==================================================================================================
*                                       FUNCTION PROTOTYPES
==================================================================================================*/
/**
 * @brief Fill the default configuration for timer.
 * @param[out] config Module configuration.
 * @details
 * Mặc định theo reset của FU2: prescaler=0, reload=0, timer dừng,
 * đếm lên, auto-reload/one-pulse/center-aligned/ngắt đều tắt.
 */
void neon_tim_get_default_config(neon_tim_config_t *config);

/**
 * @brief Initialize timer.
 * @param[in] instance Peripheral register instance.
 * @param[in] channel channel.
 * @param[in] init Initialization settings.
 * @return NEON_OK on success, or a module error/status code.
 * @details
 * config tự bật clock và nhả reset của instance nhưng không pulse reset toàn
 * peripheral, vì một lần reset TIMx sẽ làm mất cấu hình của cả CH0 và CH1.
 */
neon_status_t neon_tim_init(TIM_TypeDef *instance, uint8_t channel, const neon_tim_config_t *init);

/**
 * @brief Deinitialize timer.
 * @param[in] instance Peripheral register instance.
 * @param[in] channel channel.
 * @return NEON_OK on success, or a module error/status code.
 * @details
 * DeInit chỉ đưa kênh được chọn về reset value, không tắt clock và không ảnh
 * hưởng kênh còn lại trong cùng instance.
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
 * Các API dưới chỉ điều khiển INTR_EN_CHx. Ứng dụng vẫn phải bật đúng IRQ
 * trong NVIC; FU2 v8 dùng IRQ riêng cho TIM0/TIM1 và ba IRQ dùng chung cho
 * các cặp TIM2/3, TIM4/5, TIM6/7.
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
 * STT_CH0/1 trên FPGA là trạng thái sự kiện hiện tại, không chốt cờ.
 * GetActiveFlags/IsActiveFlag chỉ đọc, không xóa cờ hoặc acknowledge IRQ.
 * UPDATE tự hạ khi counter rời điểm reload ở tick kế tiếp. Chọn prescaler
 * đủ lớn để ISR kịp đọc cờ; chờ cờ hạ trước khi thoát nếu IRQ là level.
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
 * Giữ API để tương thích source; FU2 trả NEON_NOT_SUPPORTED vì STT không
 * có lệnh clear bằng phần mềm. Không ghi STT: FPGA có thể báo lỗi bus.
 */
neon_status_t neon_tim_clear_flag(TIM_TypeDef *instance, uint8_t channel, uint32_t flags);

/**
 * @brief Read timer clock status.
 * @param[in] instance Peripheral register instance.
 * @param[out] status Destination for the status snapshot.
 * @return NEON_OK on success, or a module error/status code.
 * @details
 * CLKS dùng chung cho hai kênh. Hàm chỉ đọc register đúng một lần để các cờ
 * vẫn nhất quán nếu phần cứng có side effect khi đọc.
 */
neon_status_t neon_tim_get_clock_status(TIM_TypeDef *instance, neon_tim_clock_status_t *status);

#ifdef __cplusplus
}
#endif

#endif /* __NEON_TIM_H__ */
