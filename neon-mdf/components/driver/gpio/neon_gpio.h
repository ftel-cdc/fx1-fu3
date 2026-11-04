/**
 * @file neon_gpio.h
 * @brief Public declarations for GPIO.
 * @details
 * * NEON GPIO - HAL GPIO dùng chung cho các target của Neon MDF.
 *
 * API chỉ dùng khái niệm logic như input/output, pull và tốc độ. Backend của
 * từng chip chịu trách nhiệm chuyển chúng sang register/mux thật. Với FU2,
 * neon_gpio_init() đồng thời cấu hình GPIO và PAD/SYSCSR; ứng dụng không cần
 * biết một chân GPIO tương ứng với giá trị mux 0, 1 hay 2.
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

/* FU2 và CH32V003 có 8 bit/port; các STM32 đang hỗ trợ có 16 bit/port. */
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
    /* FU2 phát interrupt khi input đã lọc chuyển lên mức cao (rising edge). */
    NEON_GPIO_INTERRUPT_ENABLE = 1,
} neon_gpio_interrupt_t;

typedef enum {
    NEON_GPIO_PIN_RESET = 0,
    NEON_GPIO_PIN_SET = 1,
} neon_gpio_pin_state_t;

typedef struct {
    uint32_t pin;                    /* Bitmask các pin cần cấu hình. */
    neon_gpio_mode_t mode;           /* Hướng input hoặc output. */
    neon_gpio_pull_t pull;           /* Không pull, pull-up hoặc pull-down. */
    neon_gpio_speed_t speed;         /* Mức drive tương đối của pad. */
    neon_gpio_schmitt_t schmitt;     /* Bộ schmitt của đường input. */
    neon_gpio_slew_rate_t slew_rate; /* Tốc độ chuyển mức output. */
    /* Khi interrupt=ENABLE, backend FU2 luôn bật đồng bộ vì phần cứng yêu cầu
   * CTL.SYNC_EN để lọc và phát hiện cạnh. */
    neon_gpio_input_sync_t input_sync;   /* Đồng bộ input vào clock GPIO. */
    neon_gpio_interrupt_t interrupt;     /* Cho phép ngắt của các pin đã chọn. */
    neon_gpio_pin_state_t initial_state; /* Mức output trước khi bật output. */
} neon_gpio_config_t;

/*==================================================================================================
*                                       FUNCTION PROTOTYPES
==================================================================================================*/
/**
 * @brief Fill the default configuration for GPIO.
 * @param[out] config Module configuration.
 * @details
 * Điền cấu hình an toàn tương đương mặc định LL_GPIO_StructInit(): toàn bộ
 * pin, input, no-pull, drive thấp, schmitt tắt, slew chậm, input trực tiếp và
 * ngắt tắt. initial_state mặc định là RESET. Nên gọi trước rồi chỉ sửa các
 * field cần dùng.
 */
void neon_gpio_get_default_config(neon_gpio_config_t *config);

/**
 * @brief enable the GPIO peripheral clock and release reset where required.
 * @param[in] instance Peripheral register instance.
 * @return NEON_OK on success, or a module error/status code.
 * @details
 * Clock không được bật ngầm trong config để giữ cùng flow trên mọi target.
 * Backend FU2 đồng thời nhả reset của GPIO khi bật clock.
 */
neon_status_t neon_gpio_clock_enable(GPIO_TypeDef *instance);

/**
 * @brief Configure GPIO direction, pin routing and supported pad properties.
 * @param[in] instance Peripheral register instance.
 * @param[in] init Initialization settings.
 * @return NEON_OK on success, or a module error/status code.
 * @details
 * Trên FU2, PA0..PA4 dùng chung route JTAG. config một trong các pin này sẽ
 * nhả JTAG trước khi chọn GPIO; nếu JTAG mux đã bị khóa ở trạng thái bật,
 * hàm trả NEON_LOCKED và không thay đổi cấu hình GPIO/PAD.
 *
 * Với die-pad index 0..11, config áp dụng cả mux và cấu hình điện của PAD.
 * VP mới chưa công bố đầy đủ encoding PAD_CTRL5..7 cho index 12..23, nên
 * các chân này vẫn được chọn mux GPIO nhưng giữ cấu hình điện reset.
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
 * API ngắt FU2 phát hiện rising edge sau bộ lọc/synchronizer. Backend chưa
 * hỗ trợ trên target khác trả NEON_NOT_SUPPORTED thay vì âm thầm bỏ qua.
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
 * Xóa cả raw status và nguồn software injection của các pin được chọn.
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
