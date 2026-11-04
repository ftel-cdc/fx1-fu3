/**
 * @file neon_syscsr.h
 * @brief Public declarations for SYSCSR.
 * @details
 * * NEON_SYSCSR - HAL cho khối System control and Status của FU2.
 *
 * SYSCSR là một khối duy nhất trong SoC nên API không nhận con trỏ instance.
 * Pinmux/PAD không được public ở đây: đó là chi tiết backend được GPIO/UART/
 * SPI/I2C/PWM/QSPI dùng qua neon_syscsr_internal.h.
 * @note Blocking polling/configuration operations are intended for task context.
 * Serialize configuration/teardown with other users of the same peripheral;
 * timeout_polls values are polling budgets, not milliseconds, where applicable.
 */

#ifndef __NEON_SYSCSR_H__
#define __NEON_SYSCSR_H__

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
typedef struct {
    neon_field16_t major;
    neon_field16_t minor;
    uint32_t raw_value;
} neon_syscsr_version_t;

typedef struct {
    neon_field8_t boot_pin;
    neon_field8_t mode0_pin;
    neon_field8_t mode1_pin;
    neon_field8_t test_mode_pin;
    neon_field8_t test_mode_select_pin;
    neon_field8_t mode; /* Giá trị gộp: bit0=MODE_0, bit1=MODE_1. */
    uint32_t raw_value;
} neon_syscsr_boot_config_t;

typedef enum {
    NEON_SYSCSR_EDGE_LEFT = 0,
    NEON_SYSCSR_EDGE_BOTTOM = 1,
    NEON_SYSCSR_EDGE_RIGHT = 2,
    NEON_SYSCSR_EDGE_TOP = 3,
} neon_syscsr_edge_t;

typedef struct {
    neon_field8_t debug_watchdog_freeze;
    neon_field8_t wic_request;
    neon_field8_t wic_acknowledge;
    neon_field8_t wakeup;
    neon_field8_t wic_sense;
    neon_field8_t sleep_hold_requested;
    neon_field8_t sleep_hold_acknowledge;
    neon_field8_t sleeping;
    neon_field8_t sleep_deep;
    neon_field8_t tx_event;
    neon_field8_t system_reset_request;
    neon_field8_t lockup;
    uint32_t raw_value;
} neon_syscsr_cpu_status_t;

/*==================================================================================================
*                                       FUNCTION PROTOTYPES
==================================================================================================*/
/**
 * @brief Read SYSCSR version.
 * @param[in,out] version Version.
 * @return NEON_OK on success, or a module error/status code.
 * @details
 * Đọc thông tin nhận dạng và trạng thái hệ thống.
 */
neon_status_t neon_syscsr_get_version(neon_syscsr_version_t *version);
/**
 * @brief Read SYSCSR boot config.
 * @param[out] config Module configuration.
 * @return NEON_OK on success, or a module error/status code.
 */
neon_status_t neon_syscsr_get_boot_config(neon_syscsr_boot_config_t *config);
/**
 * @brief Read SYSCSR cpu status.
 * @param[out] status Destination for the status snapshot.
 * @return NEON_OK on success, or a module error/status code.
 */
neon_status_t neon_syscsr_get_cpu_status(neon_syscsr_cpu_status_t *status);

/**
 * @brief Set SYSCSR sys tick calibration.
 * @param[in] ten_ms_ticks Ten ms ticks.
 * @return NEON_OK on success, or a module error/status code.
 * @details
 * FU2 v8 stores the 10-ms SysTick calibration value in CPU_SYSTICK.TENMS.
 */
neon_status_t neon_syscsr_set_sys_tick_calibration(uint32_t ten_ms_ticks);
/**
 * @brief Read SYSCSR sys tick calibration.
 * @return The requested value or operation result.
 */
uint32_t neon_syscsr_get_sys_tick_calibration(void);

/**
 * @brief Set SYSCSR debug watchdog freeze.
 * @param[in] enable Nonzero to enable, zero to disable.
 * @return NEON_OK on success, or a module error/status code.
 * @details
 * Điều khiển các field RW của CPU_CTRL. enable/hold chỉ nhận 0 hoặc 1.
 */
neon_status_t neon_syscsr_set_debug_watchdog_freeze(uint8_t enable);
/**
 * @brief Set SYSCSR wic request.
 * @param[in] enable Nonzero to enable, zero to disable.
 * @return NEON_OK on success, or a module error/status code.
 */
neon_status_t neon_syscsr_set_wic_request(uint8_t enable);
/**
 * @brief Set SYSCSR sleep hold.
 * @param[in] hold Hold.
 * @return NEON_OK on success, or a module error/status code.
 */
neon_status_t neon_syscsr_set_sleep_hold(uint8_t hold);

/**
 * @brief Set SYSCSR oscillator pad control.
 * @param[in] control control.
 * @return NEON_OK on success, or a module error/status code.
 * @details
 * Các thanh ghi v11 có field/mask đã được FU2 v8 công bố.
 */
neon_status_t neon_syscsr_set_oscillator_pad_control(uint8_t control);
/**
 * @brief Read SYSCSR oscillator pad control.
 * @return The requested value or operation result.
 */
uint8_t neon_syscsr_get_oscillator_pad_control(void);
/**
 * @brief Set SYSCSR rom control.
 * @param[in] enable Nonzero to enable, zero to disable.
 * @return NEON_OK on success, or a module error/status code.
 */
neon_status_t neon_syscsr_set_rom_control(uint8_t enable);
/**
 * @brief Read SYSCSR rom control.
 * @return The requested value or operation result.
 */
uint8_t neon_syscsr_get_rom_control(void);

/**
 * @brief Set SYSCSR extended pad control.
 * @param[in] edge Edge.
 * @param[in] control_index control index.
 * @param[in] value Value.
 * @return NEON_OK on success, or a module error/status code.
 * @details
 * FU2 v8 chỉ công bố PAD_CTRL5..7 là RW[23:0] và IO_CFG_EXT là
 * RW[15:0], không có tên field. API raw tránh gán sai ý nghĩa bit.
 */
neon_status_t neon_syscsr_set_extended_pad_control(neon_syscsr_edge_t edge, uint8_t control_index, uint32_t value);
/**
 * @brief Read SYSCSR extended pad control.
 * @param[in] edge Edge.
 * @param[in] control_index control index.
 * @param[out] value Value.
 * @return NEON_OK on success, or a module error/status code.
 */
neon_status_t neon_syscsr_get_extended_pad_control(neon_syscsr_edge_t edge, uint8_t control_index, uint32_t *value);
/**
 * @brief Set SYSCSR extended io config.
 * @param[in] edge Edge.
 * @param[in] value Value.
 * @return NEON_OK on success, or a module error/status code.
 */
neon_status_t neon_syscsr_set_extended_io_config(neon_syscsr_edge_t edge, uint16_t value);
/**
 * @brief Read SYSCSR extended io config.
 * @param[in] edge Edge.
 * @param[out] value Value.
 * @return NEON_OK on success, or a module error/status code.
 */
neon_status_t neon_syscsr_get_extended_io_config(neon_syscsr_edge_t edge, uint16_t *value);

/**
 * @brief Set SYSCSR jtag mux.
 * @param[in] enable Nonzero to enable, zero to disable.
 * @return NEON_OK on success, or a module error/status code.
 * @details
 * JTAGMUX_SEL[8] và JTAGMUX_BLOCK[12] đều là bit RW thường trong
 * model FU2 v8. BlockJTAGMux là helper tương thích ghi BLOCK=1.
 */
neon_status_t neon_syscsr_set_jtag_mux(uint8_t enable);
/**
 * @brief Check SYSCSR jtag mux enabled.
 * @return The requested value or operation result.
 */
uint8_t neon_syscsr_is_jtag_mux_enabled(void);
/**
 * @brief Check SYSCSR jtag mux blocked.
 * @return The requested value or operation result.
 */
uint8_t neon_syscsr_is_jtag_mux_blocked(void);
/**
 * @brief Set SYSCSR jtag mux blocked.
 * @param[in] blocked Blocked.
 * @return NEON_OK on success, or a module error/status code.
 */
neon_status_t neon_syscsr_set_jtag_mux_blocked(uint8_t blocked);
/**
 * @brief Syscsr block jtag mux.
 * @return NEON_OK on success, or a module error/status code.
 */
neon_status_t neon_syscsr_block_jtag_mux(void);

/**
 * @brief Reset the subsystem and reinitialize Flash from SRAM.
 * @return NEON_OK on success, or a module error/status code.
 * @details
 * Subsystem reset không reset CPU nhưng có reset FLASHC. Hàm được đặt
 * trong SRAM, che interrupt, phát request, INIT lại FLASHC rồi mới return.
 */
neon_status_t neon_syscsr_request_subsystem_reset(void);
/**
 * @brief Request SYSCSR software reset.
 * @return NEON_OK on success, or a module error/status code.
 */
neon_status_t neon_syscsr_request_software_reset(void);

/**
 * @brief Request a global system reset; this function does not return.
 * @details
 * Global reset phải reset cả CPU nên không trả về.
 */
__NO_RETURN void neon_syscsr_request_global_reset(void);

#ifdef __cplusplus
}
#endif

#endif /* __NEON_SYSCSR_H__ */
