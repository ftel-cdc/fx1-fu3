/**
 * @file neon_crm.h
 * @brief Public declarations for CRM.
 * @details
 * * NEON_CRM - HAL quản lý clock và reset.
 *
 * API public chỉ dùng tên nguồn clock và tên ngoại vi ở mức Neon. Việc một
 * ngoại vi nằm trong CLK_GATE_0, CLK_GATE_1 hay SW_RST_N_x là chi tiết của
 * backend từng dòng chip và không được lộ ra ngoài ứng dụng.
 * @note Blocking polling/configuration operations are intended for task context.
 * Serialize configuration/teardown with other users of the same peripheral;
 * timeout_polls values are polling budgets, not milliseconds, where applicable.
 */

#ifndef __NEON_CRM_H__
#define __NEON_CRM_H__

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
#define NEON_CRM_DEFAULT_PLL_TIMEOUT 1000000UL

/*==================================================================================================
*                                  STRUCTURES AND OTHER TYPEDEFS
==================================================================================================*/
typedef enum {
    NEON_CRM_PLL_DISABLE = 0,
    NEON_CRM_PLL_ENABLE = 1,
} neon_crm_pll_state_t;

typedef enum {
    NEON_CRM_CLOCK_SOURCE_HSI = 0,
    NEON_CRM_CLOCK_SOURCE_PLL = 1,
} neon_crm_clock_source_t;

typedef struct {
    neon_crm_pll_state_t pll_state;
    neon_field8_t pll_output_divider; /* Raw OD field, output divisor is OD + 1. */
    neon_field8_t pllm_divider;       /* Direct PLL M divider field. */
    neon_field8_t plln_divider;       /* Direct PLL N divider field. */
    neon_field8_t pll_bypass;
    neon_field16_t pll_lock_count; /* Raw PLL_CTRL.COUNT field. */
    neon_crm_clock_source_t clock_source;
    uint32_t clock_divider; /* 0=bypass; n>0 divides the clock by 2*n. */
    uint32_t pll_timeout_polls;
} neon_crm_config_t;

/* Tên ngoại vi độc lập với cách chia nhóm clock/reset của phần cứng.
 * Không public MRV, ROM và SRAM. FLASH được public để flash HAL quản lý
 * clock; không được disable/reset FLASH khi CPU đang execute từ main array. */
typedef enum {
    NEON_CRM_PERIPHERAL_AES = 0,
    NEON_CRM_PERIPHERAL_QSPI = 1,
    NEON_CRM_PERIPHERAL_FLASH = 2,
    NEON_CRM_PERIPHERAL_CRC = 3,
    NEON_CRM_PERIPHERAL_TRNG = 4,
    NEON_CRM_PERIPHERAL_WDT = 5,
    NEON_CRM_PERIPHERAL_ADC = 6,
    NEON_CRM_PERIPHERAL_TIMER0 = 7,
    NEON_CRM_PERIPHERAL_TIMER1 = 8,
    NEON_CRM_PERIPHERAL_TIMER2 = 9,
    NEON_CRM_PERIPHERAL_TIMER3 = 10,
    NEON_CRM_PERIPHERAL_TIMER4 = 11,
    NEON_CRM_PERIPHERAL_TIMER5 = 12,
    NEON_CRM_PERIPHERAL_TIMER6 = 13,
    NEON_CRM_PERIPHERAL_TIMER7 = 14,
    NEON_CRM_PERIPHERAL_PWM0 = 15,
    NEON_CRM_PERIPHERAL_PWM1 = 16,
    NEON_CRM_PERIPHERAL_SPI0 = 17,
    NEON_CRM_PERIPHERAL_SPI1 = 18,
    NEON_CRM_PERIPHERAL_UART0 = 19,
    NEON_CRM_PERIPHERAL_UART1 = 20,
    NEON_CRM_PERIPHERAL_UART2 = 21,
    NEON_CRM_PERIPHERAL_UART3 = 22,
    NEON_CRM_PERIPHERAL_UART4 = 23,
    NEON_CRM_PERIPHERAL_I2C0 = 24,
    NEON_CRM_PERIPHERAL_GPIOA = 25,
    NEON_CRM_PERIPHERAL_GPIOB = 26,
    NEON_CRM_PERIPHERAL_GPIOC = 27,
    NEON_CRM_PERIPHERAL_GPIOD = 28,
    NEON_CRM_PERIPHERAL_GPIOE = 29,
    NEON_CRM_PERIPHERAL_GPIOF = 30,
    NEON_CRM_PERIPHERAL_RTC = 31,
    NEON_CRM_PERIPHERAL_DMA0 = 32,
    NEON_CRM_PERIPHERAL_DMA1 = 33,
    NEON_CRM_PERIPHERAL_COUNT = 34,
} neon_crm_peripheral_t;

typedef uint32_t neon_crm_reset_source_t;
#define NEON_CRM_RESET_SOURCE_NONE (0U)
#define NEON_CRM_RESET_SOURCE_GLOBAL ((1U << 0))
#define NEON_CRM_RESET_SOURCE_SYSTEM ((1U << 1))
#define NEON_CRM_RESET_SOURCE_WATCHDOG ((1U << 2))
#define NEON_CRM_RESET_SOURCE_CPU ((1U << 3))

/*==================================================================================================
*                                       FUNCTION PROTOTYPES
==================================================================================================*/
/* Quy ước status của CRM:
 * - NEON_INVALID_PARAM: con trỏ, enum hoặc tổ hợp cấu hình đầu vào không hợp lệ.
 * - NEON_NOT_SUPPORTED: ngoại vi hợp lệ nhưng FU2 không có chức năng yêu cầu.
 * - NEON_TIMEOUT: PLL không khóa trong giới hạn pll_timeout_polls.
 * - NEON_OK: thao tác hoàn tất; ngoại vi không có clock gate riêng được xem
 *   là luôn bật và các API clock vẫn trả NEON_OK. */

/* Điền cấu hình mặc định: HSI, PLL tắt, multiplier/divider bằng 0. */
/**
 * @brief Fill the default configuration for CRM.
 * @param[out] config Module configuration.
 */
void neon_crm_get_default_config(neon_crm_config_t *config);

/**
 * @brief Initialize CRM.
 * @param[in] config Module configuration.
 * @return NEON_OK on success, or a module error/status code.
 * @details
 * Cấu hình nguồn clock hệ thống và PLL.
 */
neon_status_t neon_crm_init(const neon_crm_config_t *config);
/**
 * @brief Deinitialize CRM.
 * @return NEON_OK on success, or a module error/status code.
 */
neon_status_t neon_crm_deinit(void);
/**
 * @brief Read CRM config.
 * @param[out] config Module configuration.
 * @return NEON_OK on success, or a module error/status code.
 */
neon_status_t neon_crm_get_config(neon_crm_config_t *config);

/**
 * @brief Read CRM system clock frequency.
 * @return The requested value or operation result.
 * @details
 * Đọc thông tin clock và trạng thái reset của hệ thống.
 */
uint32_t neon_crm_get_system_clock_frequency(void);
/**
 * @brief Read CRM version.
 * @return The requested value or operation result.
 */
uint32_t neon_crm_get_version(void);
/**
 * @brief Read CRM reset source.
 * @return The requested value or operation result.
 */
uint32_t neon_crm_get_reset_source(void);
/**
 * @brief Check CRM pll ready.
 * @return The requested value or operation result.
 */
uint8_t neon_crm_is_pll_ready(void);

/**
 * @brief Crm peripheral clock enable.
 * @param[in] peripheral Peripheral.
 * @return NEON_OK on success, or a module error/status code.
 * @details
 * Điều khiển clock ngoại vi. Ngoại vi không có clock gate riêng được xem là
 * luôn bật để ứng dụng có thể dùng chung một flow trên nhiều dòng chip.
 */
neon_status_t neon_crm_peripheral_clock_enable(neon_crm_peripheral_t peripheral);
/**
 * @brief Crm peripheral clock disable.
 * @param[in] peripheral Peripheral.
 * @return NEON_OK on success, or a module error/status code.
 */
neon_status_t neon_crm_peripheral_clock_disable(neon_crm_peripheral_t peripheral);
/**
 * @brief Check CRM peripheral clock enabled.
 * @param[in] peripheral Peripheral.
 * @param[in,out] enabled Enabled.
 * @return NEON_OK on success, or a module error/status code.
 */
neon_status_t neon_crm_is_peripheral_clock_enabled(neon_crm_peripheral_t peripheral, uint8_t *enabled);

/**
 * @brief Crm peripheral force reset.
 * @param[in] peripheral Peripheral.
 * @return NEON_OK on success, or a module error/status code.
 * @details
 * Reset của FU2 tích cực mức thấp: force reset ghi 0, release reset ghi 1.
 */
neon_status_t neon_crm_peripheral_force_reset(neon_crm_peripheral_t peripheral);
/**
 * @brief Crm peripheral release reset.
 * @param[in] peripheral Peripheral.
 * @return NEON_OK on success, or a module error/status code.
 */
neon_status_t neon_crm_peripheral_release_reset(neon_crm_peripheral_t peripheral);
/**
 * @brief Reset CRM peripheral.
 * @param[in] peripheral Peripheral.
 * @return NEON_OK on success, or a module error/status code.
 */
neon_status_t neon_crm_peripheral_reset(neon_crm_peripheral_t peripheral);
/**
 * @brief Check CRM peripheral reset asserted.
 * @param[in] peripheral Peripheral.
 * @param[in,out] asserted Asserted.
 * @return NEON_OK on success, or a module error/status code.
 */
neon_status_t neon_crm_is_peripheral_reset_asserted(neon_crm_peripheral_t peripheral, uint8_t *asserted);

#ifdef __cplusplus
}
#endif

#endif /* __NEON_CRM_H__ */
