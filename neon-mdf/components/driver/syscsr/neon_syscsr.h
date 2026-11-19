/**
 * @file neon_syscsr.h
 * @brief Public declarations for SYSCSR.
 * @details
 * * NEON_SYSCSR - HAL for GT32U2086's System control and Status block.
 *
 * SYSCSR is a single block in SoC so the API doesn't accept pointer instances. Pinmux/PAD isn't public
 * here: it's only the backend that is used by GPIO/UART/ SPI/I2C/PWM/QSPI via
 * neon_syscsr_internal.h.
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
    neon_field8_t mode; /* Gross value: bit0=MODE_0, bit1=MODE_1. */
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
 * read identification information and system status.
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
 * @param[in] ten_ms_ticks name ms ticks.
 * @return NEON_OK on success, or a module error/status code.
 * @details
 * GT32U2086 v8 stores the 10-ms SysTick calibration value in CPU_SYSTICK.TENMS.
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
 * Control the RW fields of CPU_CTRL. enable/hold only receive 0 or 1.
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
 * The v11 register with field/mask has been published by GT32U2086 v8.
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
 * GT32U2086 v8 only publishes PAD_CTRL5..7 as RW[23:0] and IO_CFG_EXT as RW[15:0], without field names. API
 * raw avoids mislabeling bit meanings.
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
 * JTAGMUX_SEL[8] and JTAGMUX_BLOCK[12] are both common RW bits in the GT32U2086 v8 model. BlockJTAGMux is a
 * write BLOCK=1 compatible helper.
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
 * Subsystem reset does not reset the CPU but FLASHC resets. Functions are set in SRAM, cover
 * interrupt, play request, INIT FLASHC and then return.
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
 * Global reset has to reset the CPU too, so it wo n't return.
 */
__NO_RETURN void neon_syscsr_request_global_reset(void);

#ifdef __cplusplus
}
#endif

#endif /* __NEON_SYSCSR_H__ */
