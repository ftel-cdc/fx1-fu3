/**
 * @file neon_qspi.h
 * @brief Public declarations for QSPI.
 * @details
 * * NEON_QSPI - Indirect mode (command + data via FIFO control software). config/DeInit/SendCommand
 * port is REAL from src_fu2/fu2_dti_qspi.c (former team, BuuPQ).
 * WriteData/ReadData/IsBusy/WaitReady is a direct wrap registering the real TX/RX/STT.BUSY (inline
 * LL_QSPI_IsBusy in header) - NO private full/empty FIFO flags are validated in
 * inc_fu2/fu2_dti_qspi.h (only STT.BUSY), so NO other polling flags are made - caller manually uses
 * IsBusy()/WaitReady() between read/write.
 *
 * ONLY control via APB window (DTI_QSPI = QSPI_APB_BASE) - AHB-Lite window (QSPI_CTRL, XiP
 * memory-mapped read) NO separate struct register in header team (only direct read memory when
 * CFG.FMODE=MEM_MAPPED), see
 * FU2.yaml peripheral QSPI_CTRL.
 *
 * Source: inc_fu2/fu2_dti_qspi.h ("Dolphin Technology DTI QSPI").
 * @note Blocking polling/configuration operations are intended for task context.
 * Serialize configuration/teardown with other users of the same peripheral;
 * timeout_polls values are polling budgets, not milliseconds, where applicable.
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
/* Clock divider - SCLK = sysclk / (2^(div_index+1)), div_index = below value */
#define NEON_QSPI_CLK_DIV_2 0x0U
#define NEON_QSPI_CLK_DIV_4 0x1U
#define NEON_QSPI_CLK_DIV_8 0x2U
#define NEON_QSPI_CLK_DIV_256 0x7U

/*==================================================================================================
*                                  STRUCTURES AND OTHER TYPEDEFS
==================================================================================================*/
typedef enum {
    NEON_QSPI_PIN_CONFIG_AUTO = 0,
    NEON_QSPI_PIN_CONFIG_KEEP = 1,
} neon_qspi_pin_config_t;

typedef struct {
    uint32_t clock_divider; /* NEON_QSPI_CLK_DIV_* */
    /* AUTO routes SCK/SSN/D0..D3; DeInit restores the saved pad/mux bits.
     * KEEP uses the board's existing routing. Zero initialization selects AUTO. */
    neon_qspi_pin_config_t pin_config;
} neon_qspi_config_t;

/* Line count for instruction/address/data - shared constant because of the same real encoding
 * (0=NONE,1=1LINE,2=2LINES,3=4LINES) on all 3 CCR fields. */
#define NEON_QSPI_LINES_NONE 0x0U
#define NEON_QSPI_LINES_1 0x1U
#define NEON_QSPI_LINES_2 0x2U
#define NEON_QSPI_LINES_4 0x3U

typedef struct {
    neon_field8_t instruction;    /* 8-bit opcode send to flash (e.g. 0x9F= Read JEDEC ID) */
    uint32_t instruction_lines;   /* NEON_QSPI_LINES_* */
    uint32_t address_lines;       /* NEON_QSPI_LINES_NONE if the command does not need address */
    uint32_t address;             /* only used when address_lines != NONE, always 24-bit (fixed ADDRSIZE) */
    uint32_t dummy_cycles;        /* 0-31 */
    uint32_t data_lines;          /* NEON_QSPI_LINES_NONE if the command has no data phase */
    uint32_t data_length;         /* number of bytes of data, only used when data_lines != NONE */
    neon_field8_t receive_enable; /* 1 = read data from slave (e.g. Read), 0 = write (e.g. Page Program) */
} neon_qspi_command_t;

/*==================================================================================================
*                                       FUNCTION PROTOTYPES
==================================================================================================*/
/** @brief Default QSPI config: divide by 2, automatic pad/mux setup. */
void neon_qspi_get_default_config(neon_qspi_config_t *config);

/**
 * @brief Initialize QSPI.
 * @param[in] instance Peripheral register instance.
 * @param[in] init Initialization settings.
 * @return NEON_OK on success, or a module error/status code.
 */
neon_status_t neon_qspi_init(QSPI_TypeDef *instance, const neon_qspi_config_t *init);
/**
 * @brief Deinitialize QSPI.
 * @param[in] instance Peripheral register instance.
 * @return NEON_OK on success, or a module error/status code.
 */
neon_status_t neon_qspi_deinit(QSPI_TypeDef *instance);

/**
 * @brief Qspi send command.
 * @param[in] instance Peripheral register instance.
 * @param[in] cmd Cmd.
 * @return NEON_OK on success, or a module error/status code.
 * @details
 * config set mode 0/SDR; config and SendCommand return BUSY while transferring. write AR/ABR/DLR then
 * CCR - in Indirect mode, write CCR REALLY STARTS the transfer immediately (match original comment
 * "writing CCR is the trigger"). Call WriteData/ReadData then to pump/read data via FIFO (standard
 * communication SPI/QSPI flash: send command first, data later / at the same time depending on the
 * real flash chip).
 */
neon_status_t neon_qspi_send_command(QSPI_TypeDef *instance, const neon_qspi_command_t *cmd);

/**
 * @brief Write QSPI data.
 * @param[in] instance Peripheral register instance.
 * @param[in] data data for this operation.
 * @return NEON_OK on success, or a module error/status code.
 */
neon_status_t neon_qspi_write_data(QSPI_TypeDef *instance, uint32_t data);
/**
 * @brief Read QSPI data.
 * @param[in] instance Peripheral register instance.
 * @return The requested value or operation result.
 */
uint32_t neon_qspi_read_data(QSPI_TypeDef *instance);
/**
 * @brief Check QSPI busy.
 * @param[in] instance Peripheral register instance.
 * @return The requested value or operation result.
 */
uint32_t neon_qspi_is_busy(QSPI_TypeDef *instance);
/**
 * @brief Qspi wait ready.
 * @param[in] instance Peripheral register instance.
 * @param[in] timeout_polls Polling iteration budget; this value is not milliseconds.
 * @return NEON_OK on success, or a module error/status code.
 * @details
 * Blocking - poll BUSY until =0 or timeout_polls (number of poll loops) is over.
 */
neon_status_t neon_qspi_wait_ready(QSPI_TypeDef *instance, uint32_t timeout_polls);

#ifdef __cplusplus
}
#endif

#endif /* __NEON_QSPI_H__ */
