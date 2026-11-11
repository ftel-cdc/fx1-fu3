/**
 * @file neon_qspi.h
 * @brief QSPI SDR command, FIFO and memory-mapped read APIs.
 * @details
 * Supports Single/Dual/Quad phases with software polling. Flash-specific commands
 * such as write-enable, erase, status polling and Quad Enable belong to the caller.
 * Automatic status-polling, DDR and DMA transfers are not implemented by these APIs.
 * @note Serialize operations on the peripheral. Polling budgets are not milliseconds.
 * Code and constants must reside outside QSPI memory while configuring or aborting it.
 */

#ifndef __NEON_QSPI_H__
#define __NEON_QSPI_H__

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
/* SCK = QSPI input clock / 2^(divider + 1). */
#define NEON_QSPI_CLK_DIV_2 0U
#define NEON_QSPI_CLK_DIV_4 1U
#define NEON_QSPI_CLK_DIV_8 2U
#define NEON_QSPI_CLK_DIV_16 3U
#define NEON_QSPI_CLK_DIV_32 4U
#define NEON_QSPI_CLK_DIV_64 5U
#define NEON_QSPI_CLK_DIV_128 6U
#define NEON_QSPI_CLK_DIV_256 7U

#define NEON_QSPI_LINES_NONE 0U
#define NEON_QSPI_LINES_1 1U
#define NEON_QSPI_LINES_2 2U
#define NEON_QSPI_LINES_4 3U

/* Bounded start observation for command-only frames, which may not generate TC. */
#ifndef NEON_QSPI_COMMAND_START_TIMEOUT_POLLS
#define NEON_QSPI_COMMAND_START_TIMEOUT_POLLS 100000U
#endif

/*==================================================================================================
*                                  STRUCTURES AND OTHER TYPEDEFS
==================================================================================================*/
typedef enum {
    NEON_QSPI_PIN_CONFIG_AUTO = 0,
    NEON_QSPI_PIN_CONFIG_KEEP = 1,
} neon_qspi_pin_config_t;

typedef enum {
    NEON_QSPI_CLOCK_MODE_0 = 0,
    NEON_QSPI_CLOCK_MODE_3 = 3,
} neon_qspi_clock_mode_t;

typedef enum {
    NEON_QSPI_SLAVE_SELECT_0 = 0,
    NEON_QSPI_SLAVE_SELECT_1 = 1,
} neon_qspi_slave_select_t;

typedef struct {
    neon_field16_t slave_select_to_clock;
    neon_field16_t clock_to_slave_select;
    neon_field16_t between_commands;
    /* System-clock cycles after the mapped RX FIFO fills; zero disables timeout. */
    neon_field16_t memory_mapped_timeout;
} neon_qspi_timing_t;

typedef struct {
    uint32_t clock_divider;
    /* AUTO routes SCK/SS0/D0..D3 and DeInit restores their saved settings.
     * SS1 requires KEEP with board-managed routing. */
    neon_qspi_pin_config_t pin_config;
    neon_qspi_clock_mode_t clock_mode;
    neon_qspi_slave_select_t slave_select;
    neon_qspi_timing_t timing;
    /* Delay SDR input sampling by half an SCK period. */
    neon_field8_t sample_shift;
} neon_qspi_config_t;

typedef struct {
    neon_field8_t instruction;
    uint32_t instruction_lines;
    uint32_t address_lines;
    uint32_t address; /* Indirect address; mapped reads use the AHB access address. */
    uint32_t dummy_cycles; /* 0..31 complete SCK periods. */
    uint32_t data_lines;
    uint32_t data_length; /* Logical byte count; driver writes count - 1 to DLR. */
    neon_field8_t receive_enable;
    /* Sizes are byte counts. Zero keeps the legacy defaults: address=3, alternate=1. */
    uint32_t address_size;
    uint32_t alternate_byte_lines;
    uint32_t alternate_byte_size;
    uint32_t alternate_bytes;
    /* Only for memory-mapped reads; the flash must support the selected protocol. */
    neon_field8_t instruction_once;
} neon_qspi_command_t;

/*==================================================================================================
*                                       FUNCTION PROTOTYPES
==================================================================================================*/
/**
 * @brief Fill mode-0 SDR defaults, divider /2, SS0 and automatic pin routing.
 * @param[out] config Configuration to initialize.
 * @details
 * Rebuild all users after configuration/command structure changes.
 */
void neon_qspi_get_default_config(neon_qspi_config_t *config);

/**
 * @brief Configure an idle QSPI controller for indirect transfers.
 * @param[in] instance QSPI_APB.
 * @param[in] init Configuration initialized with defaults or zero initialization.
 * @return NEON_OK, NEON_BUSY, NEON_INVALID_PARAM or a clock/pin configuration error.
 */
neon_status_t neon_qspi_init(QSPI_TypeDef *instance, const neon_qspi_config_t *init);

/**
 * @brief Reset an idle controller and restore its automatic pin routing.
 * @param[in] instance QSPI_APB.
 * @return NEON_OK, NEON_INVALID_PARAM, a reset error, or NEON_BUSY until a pending
 * command completes or mapped mode is aborted.
 */
neon_status_t neon_qspi_deinit(QSPI_TypeDef *instance);

/**
 * @brief Start an indirect command, writing CCR after all other frame registers.
 * @param[in] instance QSPI_APB.
 * @param[in] cmd Frame configuration; data_length must be nonzero for a data phase.
 * @return NEON_OK when started, NEON_BUSY, NEON_INVALID_PARAM, NEON_NOT_SUPPORTED
 * for DMA mode, or NEON_TIMEOUT if a command-only frame never starts.
 * @details
 * Pump/drain the data FIFO, then call wait_complete. A command without data uses
 * at least divider /32 until completion. Before returning, this function observes
 * BUSY or TC within NEON_QSPI_COMMAND_START_TIMEOUT_POLLS polls.
 * The configured divider is restored by wait_complete or a successful abort.
 * NEON_OK here does not mean the flash has finished an internal erase/program.
 */
neon_status_t neon_qspi_send_command(QSPI_TypeDef *instance, const neon_qspi_command_t *cmd);

/**
 * @brief Queue one packed word for the pending write command.
 * @param[in] instance QSPI_APB.
 * @param[in] data Up to four serial bytes, low byte first; DLR limits the final word.
 * @return NEON_OK, NEON_BUSY for a full FIFO, or an argument/state error.
 */
neon_status_t neon_qspi_write_data(QSPI_TypeDef *instance, uint32_t data);

/**
 * @brief Retrieve one packed word from the pending read command.
 * @param[in] instance QSPI_APB.
 * @param[out] data Aligned output word, low received byte in bits 7:0.
 * @return NEON_OK, NEON_BUSY for an empty FIFO, or an argument/state error.
 */
neon_status_t neon_qspi_try_read_data(QSPI_TypeDef *instance, uint32_t *data);

/**
 * @brief Retrieve a word, returning zero if no word is available.
 * @param[in] instance QSPI_APB.
 * @return Received word, or zero on an empty FIFO/invalid state.
 * @details
 * Use try_read_data to distinguish valid zero data from an unavailable word.
 */
uint32_t neon_qspi_read_data(QSPI_TypeDef *instance);

/**
 * @brief Queue all remaining words of a pending write command, waiting for FIFO space.
 * @param[in] instance QSPI_APB.
 * @param[in] data Aligned packed buffer with ceil(remaining_bytes / 4) elements.
 * @param[in] timeout_polls Shared waiting budget; zero permits immediate progress only.
 * @return Transfer status. On timeout, abort before issuing another command.
 */
neon_status_t neon_qspi_transmit_words(QSPI_TypeDef *instance, const uint32_t *data, uint32_t timeout_polls);

/**
 * @brief Drain all remaining words of a pending read command, waiting for received data.
 * @param[in] instance QSPI_APB.
 * @param[out] data Aligned packed buffer with ceil(remaining_bytes / 4) elements.
 * @param[in] timeout_polls Shared waiting budget; zero permits immediate progress only.
 * @return Transfer status. Unused high bytes of the final word are zeroed.
 */
neon_status_t neon_qspi_receive_words(QSPI_TypeDef *instance, uint32_t *data, uint32_t timeout_polls);

/**
 * @brief Read the hardware BUSY bit.
 * @param[in] instance QSPI_APB.
 * @return One while hardware is busy, otherwise zero.
 * @details BUSY alone cannot establish command completion; use wait_complete.
 */
uint32_t neon_qspi_is_busy(QSPI_TypeDef *instance);

/**
 * @brief Finish a pending command after the software has serviced all data words.
 * @param[in] instance QSPI_APB.
 * @param[in] timeout_polls Polling budget; zero performs one completion check.
 * @return NEON_OK on completion, NEON_BUSY if data words remain, NEON_TIMEOUT,
 * NEON_INVALID_PARAM, or NEON_ERROR if no command is pending.
 * @details
 * Data commands require TC and BUSY=0. Command-only frames also accept an observed
 * BUSY high-to-low transition. Timeout leaves the command pending until abort.
 * Poll the flash status separately after erase/program; controller TC is not flash WIP.
 */
neon_status_t neon_qspi_wait_complete(QSPI_TypeDef *instance, uint32_t timeout_polls);

/**
 * @brief Finish a pending command, or wait for an otherwise idle controller.
 * @param[in] instance QSPI_APB.
 * @param[in] timeout_polls Polling budget; zero performs one readiness check.
 * @return NEON_OK, NEON_BUSY if data words remain, NEON_TIMEOUT or NEON_INVALID_PARAM.
 */
neon_status_t neon_qspi_wait_ready(QSPI_TypeDef *instance, uint32_t timeout_polls);

/**
 * @brief Configure memory-mapped SDR reads; the first AHB read starts the frame.
 * @param[in] instance QSPI_APB.
 * @param[in] cmd Read frame, including alternate bytes when needed.
 * @return NEON_OK, NEON_BUSY, NEON_INVALID_PARAM or NEON_NOT_SUPPORTED for DMA mode.
 * @details
 * Initialize indirect mode and configure the flash first, including QE when required.
 * The address and data_length fields are ignored; the AHB access supplies the address.
 * Read the QSPI_XIP window using aligned words. Abort before changing modes again.
 */
neon_status_t neon_qspi_enable_memory_mapped(QSPI_TypeDef *instance, const neon_qspi_command_t *cmd);

/**
 * @brief Abort the current operation, flush FIFOs and return to indirect mode.
 * @param[in] instance QSPI_APB.
 * @param[in] timeout_polls Polling budget; zero performs one completion check.
 * @return NEON_OK, NEON_TIMEOUT, NEON_BUSY if DMA is enabled, or NEON_INVALID_PARAM.
 * @details
 * Stop any external DMA channel and disable its QSPI request before calling.
 * This does not reset the flash chip.
 */
neon_status_t neon_qspi_abort(QSPI_TypeDef *instance, uint32_t timeout_polls);

#ifdef __cplusplus
}
#endif

#endif /* __NEON_QSPI_H__ */
