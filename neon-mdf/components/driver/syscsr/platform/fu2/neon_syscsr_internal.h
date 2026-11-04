/**
 * @file neon_syscsr_internal.h
 * @brief Internal backend interface for SYSCSR fu2 internal.
 * @details
 * * Backend pin control nội bộ của SYSCSR FU2.
 *
 * Ứng dụng không include file này. GPIO/UART/SPI/I2C/PWM/QSPI dùng tên
 * function logic; chỉ backend này biết giá trị mux 0..3 và chân vật lý.
 * @internal
 */

#ifndef __NEON_SYSCSR_INTERNAL_H__
#define __NEON_SYSCSR_INTERNAL_H__

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include "syscsr/neon_syscsr.h"

#ifdef __cplusplus
extern "C" {
#endif

/*==================================================================================================
*                                  STRUCTURES AND OTHER TYPEDEFS
==================================================================================================*/
/* Package pin theo IO map mới của FU2 VP: package 1..80, die 1..96. */
typedef enum {
    SYSCSR_PIN_INVALID = 0,
    SYSCSR_PIN_1 = 1,
    SYSCSR_PIN_2 = 2,
    SYSCSR_PIN_3 = 3,
    SYSCSR_PIN_4 = 4,
    SYSCSR_PIN_5 = 5,
    SYSCSR_PIN_6 = 6,
    SYSCSR_PIN_7 = 7,
    SYSCSR_PIN_8 = 8,
    SYSCSR_PIN_9 = 9,
    SYSCSR_PIN_10 = 10,
    SYSCSR_PIN_11 = 11,
    SYSCSR_PIN_12 = 12,
    SYSCSR_PIN_13 = 13,
    SYSCSR_PIN_14 = 14,
    SYSCSR_PIN_15 = 15,
    SYSCSR_PIN_16 = 16,
    SYSCSR_PIN_17 = 17,
    SYSCSR_PIN_18 = 18,
    SYSCSR_PIN_19 = 19,
    SYSCSR_PIN_20 = 20,
    SYSCSR_PIN_21 = 21,
    SYSCSR_PIN_22 = 22,
    SYSCSR_PIN_23 = 23,
    SYSCSR_PIN_24 = 24,
    SYSCSR_PIN_25 = 25,
    SYSCSR_PIN_26 = 26,
    SYSCSR_PIN_27 = 27,
    SYSCSR_PIN_28 = 28,
    SYSCSR_PIN_29 = 29,
    SYSCSR_PIN_30 = 30,
    SYSCSR_PIN_31 = 31,
    SYSCSR_PIN_32 = 32,
    SYSCSR_PIN_33 = 33,
    SYSCSR_PIN_34 = 34,
    SYSCSR_PIN_35 = 35,
    SYSCSR_PIN_36 = 36,
    SYSCSR_PIN_37 = 37,
    SYSCSR_PIN_38 = 38,
    SYSCSR_PIN_39 = 39,
    SYSCSR_PIN_40 = 40,
    SYSCSR_PIN_41 = 41,
    SYSCSR_PIN_42 = 42,
    SYSCSR_PIN_43 = 43,
    SYSCSR_PIN_44 = 44,
    SYSCSR_PIN_45 = 45,
    SYSCSR_PIN_46 = 46,
    SYSCSR_PIN_47 = 47,
    SYSCSR_PIN_48 = 48,
    SYSCSR_PIN_49 = 49,
    SYSCSR_PIN_50 = 50,
    SYSCSR_PIN_51 = 51,
    SYSCSR_PIN_52 = 52,
    SYSCSR_PIN_53 = 53,
    SYSCSR_PIN_54 = 54,
    SYSCSR_PIN_55 = 55,
    SYSCSR_PIN_56 = 56,
    SYSCSR_PIN_57 = 57,
    SYSCSR_PIN_58 = 58,
    SYSCSR_PIN_59 = 59,
    SYSCSR_PIN_60 = 60,
    SYSCSR_PIN_61 = 61,
    SYSCSR_PIN_62 = 62,
    SYSCSR_PIN_63 = 63,
    SYSCSR_PIN_64 = 64,
    SYSCSR_PIN_65 = 65,
    SYSCSR_PIN_66 = 66,
    SYSCSR_PIN_67 = 67,
    SYSCSR_PIN_68 = 68,
    SYSCSR_PIN_69 = 69,
    SYSCSR_PIN_70 = 70,
    SYSCSR_PIN_71 = 71,
    SYSCSR_PIN_72 = 72,
    SYSCSR_PIN_73 = 73,
    SYSCSR_PIN_74 = 74,
    SYSCSR_PIN_75 = 75,
    SYSCSR_PIN_76 = 76,
    SYSCSR_PIN_77 = 77,
    SYSCSR_PIN_78 = 78,
    SYSCSR_PIN_79 = 79,
    SYSCSR_PIN_80 = 80,
} fu2_syscsr_physical_pin_t;

/*==================================================================================================
*                                        DEFINES AND MACROS
==================================================================================================*/
/* GPIO bank/bit → package pin theo vp/.../syscsr/iopad_map.h. */
#define SYSCSR_PIN_PA0 SYSCSR_PIN_6
#define SYSCSR_PIN_PA1 SYSCSR_PIN_11
#define SYSCSR_PIN_PA2 SYSCSR_PIN_12
#define SYSCSR_PIN_PA3 SYSCSR_PIN_13
#define SYSCSR_PIN_PA4 SYSCSR_PIN_14
#define SYSCSR_PIN_PA5 SYSCSR_PIN_18
#define SYSCSR_PIN_PA6 SYSCSR_PIN_19
#define SYSCSR_PIN_PA7 SYSCSR_PIN_20
#define SYSCSR_PIN_PB0 SYSCSR_PIN_21
#define SYSCSR_PIN_PB1 SYSCSR_PIN_22
#define SYSCSR_PIN_PB2 SYSCSR_PIN_23
#define SYSCSR_PIN_PB3 SYSCSR_PIN_24
#define SYSCSR_PIN_PB4 SYSCSR_PIN_25
#define SYSCSR_PIN_PB5 SYSCSR_PIN_26
#define SYSCSR_PIN_PB6 SYSCSR_PIN_48
#define SYSCSR_PIN_PB7 SYSCSR_PIN_49
#define SYSCSR_PIN_PC0 SYSCSR_PIN_50
#define SYSCSR_PIN_PC1 SYSCSR_PIN_51
#define SYSCSR_PIN_PC2 SYSCSR_PIN_58
#define SYSCSR_PIN_PC3 SYSCSR_PIN_59
#define SYSCSR_PIN_PC4 SYSCSR_PIN_60
#define SYSCSR_PIN_PC5 SYSCSR_PIN_61
#define SYSCSR_PIN_PC6 SYSCSR_PIN_62
#define SYSCSR_PIN_PC7 SYSCSR_PIN_63
#define SYSCSR_PIN_PD0 SYSCSR_PIN_64
#define SYSCSR_PIN_PD1 SYSCSR_PIN_68
#define SYSCSR_PIN_PD2 SYSCSR_PIN_69
#define SYSCSR_PIN_PD3 SYSCSR_PIN_70
#define SYSCSR_PIN_PD4 SYSCSR_PIN_71
#define SYSCSR_PIN_PD5 SYSCSR_PIN_72
#define SYSCSR_PIN_PD6 SYSCSR_PIN_73
#define SYSCSR_PIN_PD7 SYSCSR_PIN_74
#define SYSCSR_PIN_PE0 SYSCSR_PIN_75
#define SYSCSR_PIN_PE1 SYSCSR_PIN_76
#define SYSCSR_PIN_PE2 SYSCSR_PIN_80
/* IO_PAD_MAP: die PIN_31 / package PIN_27 is GPIOE_4; E3 is not bonded. */
#define SYSCSR_PIN_PE4 SYSCSR_PIN_27

typedef enum {
    SYSCSR_PIN_FUNCTION_NONE = 0,
    SYSCSR_PIN_FUNCTION_GPIO = 1,
    SYSCSR_PIN_FUNCTION_JTAG_TDI = 2,
    SYSCSR_PIN_FUNCTION_JTAG_TSM = 3, /* Giữ đúng tên TSM trong FU2 LL. */
    SYSCSR_PIN_FUNCTION_JTAG_TDO = 4,
    SYSCSR_PIN_FUNCTION_JTAG_TRSTN = 5,
    SYSCSR_PIN_FUNCTION_JTAG_TCLK = 6,
    SYSCSR_PIN_FUNCTION_UART0_TX = 7,
    SYSCSR_PIN_FUNCTION_UART0_RX = 8,
    SYSCSR_PIN_FUNCTION_UART1_TX = 9,
    SYSCSR_PIN_FUNCTION_UART1_RX = 10,
    SYSCSR_PIN_FUNCTION_UART2_TX = 11,
    SYSCSR_PIN_FUNCTION_UART2_RX = 12,
    SYSCSR_PIN_FUNCTION_UART3_TX = 13,
    SYSCSR_PIN_FUNCTION_UART3_RX = 14,
    SYSCSR_PIN_FUNCTION_UART4_TX = 15,
    SYSCSR_PIN_FUNCTION_UART4_RX = 16,
    SYSCSR_PIN_FUNCTION_UART0_RTS = 17,
    SYSCSR_PIN_FUNCTION_UART0_CTS = 18,
    SYSCSR_PIN_FUNCTION_UART1_RTS = 19,
    SYSCSR_PIN_FUNCTION_UART1_CTS = 20,
    SYSCSR_PIN_FUNCTION_UART2_RTS = 21,
    SYSCSR_PIN_FUNCTION_UART2_CTS = 22,
    SYSCSR_PIN_FUNCTION_UART3_RTS = 23,
    SYSCSR_PIN_FUNCTION_UART3_CTS = 24,
    SYSCSR_PIN_FUNCTION_PWM0 = 25,
    SYSCSR_PIN_FUNCTION_PWM1 = 26,
    SYSCSR_PIN_FUNCTION_PWM_T0 = 27,
    SYSCSR_PIN_FUNCTION_PWM_T1 = 28,
    SYSCSR_PIN_FUNCTION_PWM_T4 = 29,
    SYSCSR_PIN_FUNCTION_PWM_T5 = 30,
    SYSCSR_PIN_FUNCTION_PWM_T6 = 31,
    SYSCSR_PIN_FUNCTION_PWM_T7 = 32,
    SYSCSR_PIN_FUNCTION_I2C0_SDA = 33,
    SYSCSR_PIN_FUNCTION_I2C0_SCL = 34,
    SYSCSR_PIN_FUNCTION_QSPI_D0 = 35,
    SYSCSR_PIN_FUNCTION_QSPI_D1 = 36,
    SYSCSR_PIN_FUNCTION_QSPI_D2 = 37,
    SYSCSR_PIN_FUNCTION_QSPI_D3 = 38,
    SYSCSR_PIN_FUNCTION_QSPI_SCK = 39,
    SYSCSR_PIN_FUNCTION_QSPI_SSN = 40,
    SYSCSR_PIN_FUNCTION_SPI0_SCK = 41,
    SYSCSR_PIN_FUNCTION_SPI0_MISO = 42,
    SYSCSR_PIN_FUNCTION_SPI0_MOSI = 43,
    SYSCSR_PIN_FUNCTION_SPI0_SSN = 44,
    SYSCSR_PIN_FUNCTION_SPI1_SCK = 45,
    SYSCSR_PIN_FUNCTION_SPI1_MISO = 46,
    SYSCSR_PIN_FUNCTION_SPI1_MOSI = 47,
    SYSCSR_PIN_FUNCTION_SPI1_SSN = 48,
    SYSCSR_PIN_FUNCTION_CLOCK_MONITOR = 49,
    SYSCSR_PIN_FUNCTION_TIMESTAMP = 50,
    SYSCSR_PIN_FUNCTION_TAMPER = 51,
    SYSCSR_PIN_FUNCTION_PLL_CLOCK_OUT = 52,
    SYSCSR_PIN_FUNCTION_OSC32_CLOCK_FAULT = 53,
    SYSCSR_PIN_FUNCTION_COUNT = 54,
} fu2_syscsr_pin_function_t;

typedef enum {
    SYSCSR_PULL_NONE = 0,
    SYSCSR_PULL_UP = 1,
    SYSCSR_PULL_DOWN = 2,
} fu2_syscsr_pull_t;

typedef enum {
    SYSCSR_DRIVE_2MA = 0,
    SYSCSR_DRIVE_4MA = 1,
    SYSCSR_DRIVE_8MA = 2,
    SYSCSR_DRIVE_12MA = 3,
} fu2_syscsr_drive_t;

typedef enum {
    SYSCSR_COARSE_DRIVE_LOW = 0,
    SYSCSR_COARSE_DRIVE_HIGH = 1,
} fu2_syscsr_coarse_drive_t;

typedef enum {
    SYSCSR_SLEW_SLOW = 0,
    SYSCSR_SLEW_FAST = 1,
} fu2_syscsr_slew_t;

typedef enum {
    SYSCSR_REGISTER_GROUP_LEFT = 0,
    SYSCSR_REGISTER_GROUP_BOTTOM = 1,
    SYSCSR_REGISTER_GROUP_RIGHT = 2,
    SYSCSR_REGISTER_GROUP_TOP = 3,
} fu2_syscsr_register_group_t;

/* Cấu hình PAD mà LL_GPIO_Init() thực sự sử dụng. DS/coarse drive không nằm
 * đây vì LL_GPIO_Init() không thay đổi DS; dùng API SetPadCoarseDrive riêng. */
typedef struct {
    neon_field8_t input_enable;
    fu2_syscsr_pull_t pull;
    fu2_syscsr_drive_t drive;
    neon_field8_t schmitt_enable;
    fu2_syscsr_slew_t slew_rate;
} fu2_syscsr_pad_config_t;

typedef struct {
    fu2_syscsr_physical_pin_t physical_pin;
    fu2_syscsr_register_group_t register_group;
    neon_field8_t pad_index;
} fu2_syscsr_physical_pad_map_t;

typedef struct {
    fu2_syscsr_physical_pin_t physical_pin;
    fu2_syscsr_pin_function_t mux_function[4];
} fu2_syscsr_pin_function_map_t;

typedef struct {
    GPIO_TypeDef *gpio_port;
    neon_field8_t gpio_pin_index;
    fu2_syscsr_physical_pin_t physical_pin;
} fu2_syscsr_gpio_map_t;

/* Peripheral routes save only their own pad/mux bits. Reinitializing the same
 * route keeps the original snapshot; releasing it restores that snapshot. */
typedef struct {
    fu2_syscsr_physical_pin_t pin;
    fu2_syscsr_pin_function_t function;
} fu2_syscsr_pin_route_t;

#define FU2_SYSCSR_MAX_ROUTE_PINS 6U
typedef struct {
    fu2_syscsr_physical_pin_t pin;
    uint32_t input_enable;
    uint32_t drive;
    uint32_t open_source;
    uint32_t open_drain;
    uint32_t mux;
} fu2_syscsr_saved_pin_t;

typedef struct {
    uint32_t count;
    fu2_syscsr_saved_pin_t pins[FU2_SYSCSR_MAX_ROUTE_PINS];
} fu2_syscsr_pin_group_t;

/* SYSCSR v11: input buffer enabled, high drive, open-source disabled;
 * open_drain=1 for I2C, 0 for UART/SPI/QSPI/PWM. Pull/slew settings are kept.
 * Validate all routes before writing. Pass a zero-initialized group. */
neon_status_t fu2_syscsr_configure_pins(fu2_syscsr_pin_group_t *group,
                                       const fu2_syscsr_pin_route_t *routes, uint32_t count, uint32_t open_drain);
void fu2_syscsr_release_pins(fu2_syscsr_pin_group_t *group);

/*==================================================================================================
*                                       FUNCTION PROTOTYPES
==================================================================================================*/
/**
 * @brief Read SYSCSR GPIO init default pad config.
 * @param[out] config Module configuration.
 * @details
 * Cấu hình khởi tạo GPIO mặc định của LL: input path bật, no-pull, 2mA,
 * schmitt tắt và slew chậm. Đây không phải reset state của phần cứng.
 */
void fu2_syscsr_get_gpio_init_default_pad_config(fu2_syscsr_pad_config_t *config);

/**
 * @brief Read SYSCSR physical pad map.
 * @param[in] physical_pin Physical pin.
 * @param[in,out] map Map.
 * @return NEON_OK on success, or a module error/status code.
 * @details
 * API tra cứu ba lớp mapping độc lập.
 */
neon_status_t fu2_syscsr_get_physical_pad_map(fu2_syscsr_physical_pin_t physical_pin,
                                              fu2_syscsr_physical_pad_map_t *map);
/**
 * @brief Read SYSCSR pin function map.
 * @param[in] physical_pin Physical pin.
 * @param[in,out] map Map.
 * @return NEON_OK on success, or a module error/status code.
 */
neon_status_t fu2_syscsr_get_pin_function_map(fu2_syscsr_physical_pin_t physical_pin,
                                              fu2_syscsr_pin_function_map_t *map);
/**
 * @brief Read SYSCSR GPIO physical pin.
 * @param[in,out] gpio Gpio.
 * @param[in] pin_index pin index.
 * @param[in,out] physical_pin Physical pin.
 * @return NEON_OK on success, or a module error/status code.
 */
neon_status_t fu2_syscsr_get_gpio_physical_pin(GPIO_TypeDef *gpio, uint32_t pin_index,
                                               fu2_syscsr_physical_pin_t *physical_pin);

/**
 * @brief Set SYSCSR pin function.
 * @param[in] physical_pin Physical pin.
 * @param[in] function Function.
 * @return NEON_OK on success, or a module error/status code.
 * @details
 * Mux và PAD được tách riêng để driver ngoại vi không vô tình ghi đè cấu
 * hình điện khi chỉ muốn đổi chức năng pin.
 */
neon_status_t fu2_syscsr_set_pin_function(fu2_syscsr_physical_pin_t physical_pin, fu2_syscsr_pin_function_t function);
/**
 * @brief Read SYSCSR pin function.
 * @param[in] physical_pin Physical pin.
 * @param[in,out] function Function.
 * @return NEON_OK on success, or a module error/status code.
 */
neon_status_t fu2_syscsr_get_pin_function(fu2_syscsr_physical_pin_t physical_pin, fu2_syscsr_pin_function_t *function);
/**
 * @brief Configure SYSCSR pad.
 * @param[in] physical_pin Physical pin.
 * @param[in] config Module configuration.
 * @return NEON_OK on success, or a module error/status code.
 */
neon_status_t fu2_syscsr_configure_pad(fu2_syscsr_physical_pin_t physical_pin, const fu2_syscsr_pad_config_t *config);
/**
 * @brief Read SYSCSR pad config.
 * @param[in] physical_pin Physical pin.
 * @param[out] config Module configuration.
 * @return NEON_OK on success, or a module error/status code.
 */
neon_status_t fu2_syscsr_get_pad_config(fu2_syscsr_physical_pin_t physical_pin, fu2_syscsr_pad_config_t *config);

/**
 * @brief Set SYSCSR pad coarse drive.
 * @param[in] physical_pin Physical pin.
 * @param[in] drive drive.
 * @return NEON_OK on success, or a module error/status code.
 * @details
 * DS được điều khiển riêng vì LL_GPIO_Init() không thay đổi field này.
 */
neon_status_t fu2_syscsr_set_pad_coarse_drive(fu2_syscsr_physical_pin_t physical_pin, fu2_syscsr_coarse_drive_t drive);
/**
 * @brief Read SYSCSR pad coarse drive.
 * @param[in] physical_pin Physical pin.
 * @param[in,out] drive drive.
 * @return NEON_OK on success, or a module error/status code.
 */
neon_status_t fu2_syscsr_get_pad_coarse_drive(fu2_syscsr_physical_pin_t physical_pin, fu2_syscsr_coarse_drive_t *drive);

/**
 * @brief Reset SYSCSR pin.
 * @param[in] physical_pin Physical pin.
 * @return NEON_OK on success, or a module error/status code.
 * @details
 * Khôi phục chính xác mux/PAD của một pin theo reset state FU2 v8.
 * Driver sở hữu pin phải tắt output/OE trước khi gọi để tránh glitch khi mux
 * quay về chức năng mặc định MUX_0 (có thể là JTAG/QSPI/UART, không phải GPIO).
 */
neon_status_t fu2_syscsr_reset_pin(fu2_syscsr_physical_pin_t physical_pin);

/**
 * @brief Set SYSCSR GPIO pins function.
 * @param[in,out] gpio Gpio.
 * @param[in] pin_mask Bitmask of the selected GPIO pins.
 * @return NEON_OK on success, or a module error/status code.
 * @details
 * Helper cho GPIO backend; hai bước vẫn tách riêng để GPIO chủ động thứ tự
 * chống glitch giữa OUT, PAD, mux và OE.
 */
neon_status_t fu2_syscsr_set_gpio_pins_function(GPIO_TypeDef *gpio, uint32_t pin_mask);
/**
 * @brief Configure SYSCSR GPIO pads.
 * @param[in,out] gpio Gpio.
 * @param[in] pin_mask Bitmask of the selected GPIO pins.
 * @param[in] config Module configuration.
 * @return NEON_OK on success, or a module error/status code.
 */
neon_status_t fu2_syscsr_configure_gpio_pads(GPIO_TypeDef *gpio, uint32_t pin_mask,
                                             const fu2_syscsr_pad_config_t *config);

/**
 * @brief Syscsr validate GPIO pins.
 * @param[in,out] gpio Gpio.
 * @param[in] pin_mask Bitmask of the selected GPIO pins.
 * @return NEON_OK on success, or a module error/status code.
 * @details
 * Kiểm tra trước toàn bộ mapping GPIO -> physical pad -> mux mà không ghi
 * register. GPIO dùng hàm này để bảo đảm lỗi mapping không để cấu hình dở.
 */
neon_status_t fu2_syscsr_validate_gpio_pins(GPIO_TypeDef *gpio, uint32_t pin_mask);

/**
 * @brief Reset SYSCSR GPIO pins.
 * @param[in,out] gpio Gpio.
 * @param[in] pin_mask Bitmask of the selected GPIO pins.
 * @return NEON_OK on success, or a module error/status code.
 * @details
 * Khôi phục mux và PAD reset cho toàn bộ pin GPIO đã chọn. GPIO phải tắt OE
 * trước khi gọi để chức năng mặc định như JTAG/QSPI không gây glitch.
 */
neon_status_t fu2_syscsr_reset_gpio_pins(GPIO_TypeDef *gpio, uint32_t pin_mask);

#ifdef __cplusplus
}
#endif

#endif /* __NEON_SYSCSR_INTERNAL_H__ */
