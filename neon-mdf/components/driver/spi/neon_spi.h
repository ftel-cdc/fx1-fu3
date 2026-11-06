/**
 * @file neon_spi.h
 * @brief Public declarations for SPI.
 * @details
 * * NEON SPI - HAL for FU2's DTI SPI controller.
 *
 * Register map follows DTI SPI rev1.1 chapter 9 (including the DDR slots).
 * These transfer APIs implement legacy SPI, not extended DDR commands.
 * The public API uses the logical value, does not reveal FU2's bit register. Backend maps
 * configuration to CTRL/CMDCONF/FIFO/TREGx and manages clock/reset via CRM. The controller supports
 * 8/16/24/32 bit master; slave only supports 8 bits.
 * @note Blocking polling/configuration operations are intended for task context.
 * Serialize configuration/teardown with other users of the same peripheral;
 * timeout_polls values are polling budgets, not milliseconds, where applicable.
 */

#ifndef __NEON_SPI_H__
#define __NEON_SPI_H__

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
#define NEON_SPI_FIFO_DEPTH 8U

#ifdef FU2
/* Default pause between bytes, measured in complete SCK periods. */
#ifndef NEON_SPI_FU2_B2B_SCK_CYCLES
#define NEON_SPI_FU2_B2B_SCK_CYCLES 10U
#endif
#endif

/*==================================================================================================
*                                  STRUCTURES AND OTHER TYPEDEFS
==================================================================================================*/
typedef enum {
    NEON_SPI_MODE_SLAVE = 0,
    NEON_SPI_MODE_MASTER = 1,
} neon_spi_mode_t;

typedef enum {
    NEON_SPI_PIN_CONFIG_AUTO = 0,
    NEON_SPI_PIN_CONFIG_KEEP = 1,
} neon_spi_pin_config_t;

typedef enum {
    NEON_SPI_POLARITY_LOW = 0,
    NEON_SPI_POLARITY_HIGH = 1,
} neon_spi_clock_polarity_t;

typedef enum {
    NEON_SPI_PHASE_1EDGE = 0,
    NEON_SPI_PHASE_2EDGE = 1,
} neon_spi_clock_phase_t;

typedef enum {
    NEON_SPI_FIRSTBIT_MSB = 0,
    NEON_SPI_FIRSTBIT_LSB = 1,
} neon_spi_first_bit_t;

typedef enum {
    NEON_SPI_DATA_WIDTH_8BIT = 8,
    NEON_SPI_DATA_WIDTH_16BIT = 16,
    NEON_SPI_DATA_WIDTH_24BIT = 24,
    NEON_SPI_DATA_WIDTH_32BIT = 32,
} neon_spi_data_width_t;

typedef enum {
    /* No SS hardware control; used when the board controls the chip-select itself. */
    NEON_SPI_SLAVE_SELECT_NONE = 0,
    NEON_SPI_SLAVE_SELECT_0 = 1,
    NEON_SPI_SLAVE_SELECT_1 = 2,
    NEON_SPI_SLAVE_SELECT_2 = 3,
    NEON_SPI_SLAVE_SELECT_3 = 4,
} neon_spi_slave_select_t;

typedef enum {
    /* Release SS after each data stub. */
    NEON_SPI_SS_MODE_NORMAL = 0,
    /* Dependent SS Behavior Force/LongAccess/timeout_polls. */
    NEON_SPI_SS_MODE_MANUAL = 1,
} neon_spi_slave_select_mode_t;

typedef enum {
    /* In manual mode, keep SS at active after each transaction. */
    NEON_SPI_SS_FORCE_HOLD = 0,
    /* Allow hardware to release SS when timing conditions are satisfied. */
    NEON_SPI_SS_FORCE_RELEASE = 1,
} neon_spi_slave_select_force_t;

typedef enum {
    NEON_SPI_SS_LONG_DISABLE = 0,
    /* Keep SS active until TX FIFO is empty. */
    NEON_SPI_SS_LONG_ENABLE = 1,
} neon_spi_slave_select_long_t;

typedef enum {
    NEON_SPI_INTERRUPT_DISABLE = 0,
    /* FU2 has only one general permissible bit for the entire FIFO interrupt signal. */
    NEON_SPI_INTERRUPT_ENABLE = 1,
} neon_spi_interrupt_state_t;

typedef enum {
    NEON_SPI_DMA_RX = 0,
    NEON_SPI_DMA_TX = 1,
} neon_spi_dma_direction_t;

typedef enum {
    NEON_SPI_DMA_SINGLE = 0,
    NEON_SPI_DMA_BURST = 1,
} neon_spi_dma_transfer_mode_t;

typedef enum {
    NEON_SPI_DMA_BURST_2 = 2,
    NEON_SPI_DMA_BURST_4 = 4,
    NEON_SPI_DMA_BURST_8 = 8,
    /* Field hardware has code 16 but FU2 integrates FIFO depth 8 so it is not usable. */
    NEON_SPI_DMA_BURST_16 = 16,
} neon_spi_dma_burst_length_t;

typedef enum {
    NEON_SPI_DMA_DISABLE = 0,
    NEON_SPI_DMA_ENABLE = 1,
} neon_spi_dma_state_t;

typedef enum {
    NEON_SPI_FAILED_COMMAND_WRITE = 0,
    NEON_SPI_FAILED_COMMAND_READ = 1,
} neon_spi_failed_command_t;

typedef uint32_t neon_spi_flag_t;
#define NEON_SPI_FLAG_TX_FIFO_FULL ((1U << 0))
#define NEON_SPI_FLAG_TX_FIFO_EMPTY ((1U << 1))
#define NEON_SPI_FLAG_RX_FIFO_FULL ((1U << 2))
#define NEON_SPI_FLAG_RX_FIFO_EMPTY ((1U << 3))
#define NEON_SPI_FLAG_TX_FIFO_PROG_FULL ((1U << 4))
#define NEON_SPI_FLAG_TX_FIFO_PROG_EMPTY ((1U << 5))
#define NEON_SPI_FLAG_RX_FIFO_PROG_FULL ((1U << 6))
#define NEON_SPI_FLAG_RX_FIFO_PROG_EMPTY ((1U << 7))
#define NEON_SPI_FLAG_ALL (0xFFU)

typedef struct {
    neon_field8_t tx_full_level;
    neon_field8_t tx_empty_level;
    neon_field8_t rx_full_level;
    neon_field8_t rx_empty_level;
} neon_spi_fifo_config_t;

#ifdef FU2
typedef enum {
    NEON_SPI_B2B_CORE_CYCLES = 0,
    NEON_SPI_B2B_SCK_CYCLES = 1,
} neon_spi_b2b_unit_t;
#endif

typedef struct {
    /* SS setup/hold and timeout always use core-clock cycles. */
    neon_field16_t slave_select_to_clock;
    neon_field16_t clock_to_slave_select;
    /* Pause between bytes, including bytes inside a 16/24/32-bit word. */
    neon_field16_t between_frames;
    /* 0 disables the hardware SS timeout. */
    neon_field16_t slave_select_timeout;
#ifdef FU2
    /* SCK units follow divider changes; CORE units program T_B2B directly.
     * Rebuild all objects after this structure change. */
    neon_spi_b2b_unit_t between_frames_unit;
#endif
} neon_spi_timing_t;

typedef struct {
    neon_spi_mode_t mode;
    neon_spi_clock_polarity_t clock_polarity;
    neon_spi_clock_phase_t clock_phase;
    neon_spi_first_bit_t first_bit;
    neon_spi_data_width_t data_width;
    /* fSCLK = fCORE / (2 * (clock_divider + 1)), domain 0..65535. */
    uint32_t clock_divider;
    neon_spi_slave_select_t slave_select;
    neon_spi_slave_select_mode_t slave_select_mode;
    neon_spi_slave_select_force_t slave_select_force;
    neon_spi_slave_select_long_t slave_select_long;
    neon_spi_interrupt_state_t interrupt_state;
    neon_spi_fifo_config_t fifo;
    neon_spi_timing_t timing;
    /* AUTO configures SCK/MISO/MOSI and SS0 when selected (always in slave).
     * SS1..3 have no package route: use KEEP with board-managed routing.
     * DeInit restores the bits saved by AUTO. Rebuild after struct changes. */
    neon_spi_pin_config_t pin_config;
#ifdef FU2
    /* Optional wait after RX-ready, before each received FIFO word is read.
     * NULL skips the wait. Return NEON_OK to continue; do not call SPI APIs
     * or read the RX register from this callback. Rebuild all FU2 objects. */
    neon_status_t (*before_rx_read)(void);
#endif
} neon_spi_config_t;

typedef struct {
    neon_spi_dma_state_t state;
    neon_spi_dma_transfer_mode_t transfer_mode;
    neon_spi_dma_burst_length_t burst_length;
    /* DTI DMA datapath 32 bit wide: must be nonzero, <=65535 and divisible by 4. */
    uint32_t total_bytes;
} neon_spi_dma_config_t;

typedef struct {
    neon_field8_t clock_failure;
    neon_field8_t failure_address;
    neon_spi_failed_command_t failed_command;
} neon_spi_clock_status_t;

/*==================================================================================================
*                                       FUNCTION PROTOTYPES
==================================================================================================*/
/**
 * @brief Fill the default configuration for SPI.
 * @param[out] config Module configuration.
 * @details
 * Default LL: master, mode 0, MSB, 8 bits, DIV=0, SS not selected, threshold FIFO full=0xFF, the
 * threshold empty=0, interrupt and SS timing off.
 * FU2 B2B defaults to NEON_SPI_FU2_B2B_SCK_CYCLES complete SCK periods.
 */
void neon_spi_get_default_config(neon_spi_config_t *config);

/**
 * @brief Initialize SPI.
 * @param[in] instance Peripheral register instance.
 * @param[in] init Initialization settings.
 * @return NEON_OK on success, or a module error/status code.
 * @details
 * config automatically turns on clock and pulse reset peripheral via CRM. AUTO configuration pad/mux
 * SCK/MISO/MOSI and SS0 when used; KEEP uses routing of the board. DeInit turns off DMA/interrupt,
 * resets SPI, restores AUTO pins and turns off clock via CRM.
 */
neon_status_t neon_spi_init(SPI_TypeDef *instance, const neon_spi_config_t *init);
/**
 * @brief Deinitialize SPI.
 * @param[in] instance Peripheral register instance.
 * @return NEON_OK on success, or a module error/status code.
 */
neon_status_t neon_spi_deinit(SPI_TypeDef *instance);

/**
 * @brief Set SPI clock divider.
 * @param[in] instance Peripheral register instance.
 * @param[in] divider Divider.
 * @return NEON_OK on success, or a module error/status code.
 * @details FU2 recalculates B2B when its unit is SCK. Returns
 * NEON_INVALID_PARAM without changing registers if T_B2B would exceed 65535.
 */
neon_status_t neon_spi_set_clock_divider(SPI_TypeDef *instance, uint32_t divider);
/**
 * @brief Read SPI clock frequency.
 * @param[in] instance Peripheral register instance.
 * @param[in,out] frequency Frequency.
 * @return NEON_OK on success, or a module error/status code.
 */
neon_status_t neon_spi_get_clock_frequency(SPI_TypeDef *instance, uint32_t *frequency);
/**
 * @brief Change legacy SPI frame width without resetting the controller.
 * @param[in] instance Peripheral register instance.
 * @param[in] width Master: 8/16/24/32 bits; slave: 8 bits only.
 * @return NEON_OK, NEON_BUSY, NEON_NOT_SUPPORTED, or a parameter/handle error.
 * @details Requires idle TX and inactive DMA; preserves RD, slave selection
 * and manual SS hold. This allows a header and payload of different widths
 * to share one CS assertion. Call after the previous polling transfer returns.
 */
neon_status_t neon_spi_set_data_width(SPI_TypeDef *instance, neon_spi_data_width_t width);
/**
 * @brief Set SPI slave select.
 * @param[in] instance Peripheral register instance.
 * @param[in] slave Slave.
 * @return NEON_OK on success, or a module error/status code.
 */
neon_status_t neon_spi_set_slave_select(SPI_TypeDef *instance, neon_spi_slave_select_t slave);
/**
 * @brief Set SPI receive capture.
 * @param[in] instance Peripheral register instance.
 * @param[in] enable Nonzero to enable, zero to disable.
 * @return NEON_OK on success, or a module error/status code.
 * @details
 * CMDCONF.RD control for DMA/raw stream: enable=1 save MISO to RX FIFO, enable=0 ignore. The polling
 * API sets the field itself by transfer type.
 */
neon_status_t neon_spi_set_receive_capture(SPI_TypeDef *instance, uint8_t enable);
/**
 * @brief Set SPI slave select control.
 * @param[in] instance Peripheral register instance.
 * @param[in] mode mode.
 * @param[in] force Force.
 * @param[in] long_access Long access.
 * @return NEON_OK on success, or a module error/status code.
 * @details SS_FORCE alone may change while DMA is active. Changing SS_MODE
 * or SS_LONG requires idle TX and inactive DMA, per rev1.1 section 4.4.
 */
neon_status_t neon_spi_set_slave_select_control(SPI_TypeDef *instance, neon_spi_slave_select_mode_t mode,
                                                neon_spi_slave_select_force_t force,
                                                neon_spi_slave_select_long_t long_access);
/**
 * @brief Set SPI fifo config.
 * @param[in] instance Peripheral register instance.
 * @param[in] config Module configuration.
 * @return NEON_OK on success, or a module error/status code.
 */
neon_status_t neon_spi_set_fifo_config(SPI_TypeDef *instance, const neon_spi_fifo_config_t *config);
/**
 * @brief Set SPI timing.
 * @param[in] instance Peripheral register instance.
 * @param[in] timing timing.
 * @return NEON_OK on success, or a module error/status code.
 * @details FU2 converts SCK units to core-clock cycles for TREG1.T_B2B.
 * Zero disables the byte pause. The encoded value must fit 16 bits.
 */
neon_status_t neon_spi_set_timing(SPI_TypeDef *instance, const neon_spi_timing_t *timing);

/**
 * @brief Transmit SPI.
 * @param[in] instance Peripheral register instance.
 * @param[in] data Transfer data; FU2 RAM buffers must own their complete aligned 32-bit words.
 * @param[in] size Number of transfer elements.
 * @param[in] timeout_polls Polling iteration budget; this value is not milliseconds.
 * @return NEON_OK on success, or a module error/status code.
 * @details
 * timeout_polls is the maximum total number of poll rings for both calls, not milliseconds.
 * Master calls include a conservative final-frame/SS timing guard because
 * rev1.1 exposes no shifter-BUSY flag. CPU and SPI core use the FU2 system
 * clock. The guard consumes the same polling budget and leaves manual HOLD
 * asserted when requested. Slave TX only observes FIFO consumption.
 * A polling timeout leaves the controller faulted: call Init/DeInit to reset
 * partial frames and stale FIFO data before another transfer/configuration.
 * Polling returns NEON_BUSY when DMA global is enabled; calls DisableDMA before using it. The byte
 * buffer must own all the 32-bit words containing it (two-ended padding), according to the FU2
 * runtime buffer contract. Drivers use memcpy for RAM buffers.
 */
neon_status_t neon_spi_transmit(SPI_TypeDef *instance, const uint8_t *data, uint16_t size, uint32_t timeout_polls);
/**
 * @brief Receive SPI.
 * @param[in] instance Peripheral register instance.
 * @param[out] data Transfer data; FU2 RAM buffers must own their complete aligned 32-bit words.
 * @param[in] size Number of transfer elements.
 * @param[in] timeout_polls Polling iteration budget; this value is not milliseconds.
 * @return NEON_OK on success, or a module error/status code.
 */
neon_status_t neon_spi_receive(SPI_TypeDef *instance, uint8_t *data, uint16_t size, uint32_t timeout_polls);
/**
 * @brief Transmit SPI receive.
 * @param[in] instance Peripheral register instance.
 * @param[in] tx_data Tx data.
 * @param[in,out] rx_data Rx data.
 * @param[in] size Number of transfer elements.
 * @param[in] timeout_polls Polling iteration budget; this value is not milliseconds.
 * @return NEON_OK on success, or a module error/status code.
 */
neon_status_t neon_spi_transmit_receive(SPI_TypeDef *instance, const uint8_t *tx_data, uint8_t *rx_data, uint16_t size,
                                        uint32_t timeout_polls);

/**
 * @brief Transmit SPI words.
 * @param[in] instance Peripheral register instance.
 * @param[in] data data for this operation.
 * @param[in] count Number of elements to process.
 * @param[in] timeout_polls Polling iteration budget; this value is not milliseconds.
 * @return NEON_OK on success, or a module error/status code.
 * @details
 * The Word API uses uint32_t for both the 8/16/24/32 bit stub; driver masks the data_width external
 * bits. ReceiveWords in master write dummy_data to create SCLK.
 * The byte-API completion guard and timeout recovery contract also apply.
 */
neon_status_t neon_spi_transmit_words(SPI_TypeDef *instance, const uint32_t *data, uint16_t count,
                                      uint32_t timeout_polls);
/**
 * @brief Receive SPI words.
 * @param[in] instance Peripheral register instance.
 * @param[out] data data for this operation.
 * @param[in] count Number of elements to process.
 * @param[in] dummy_data Dummy data.
 * @param[in] timeout_polls Polling iteration budget; this value is not milliseconds.
 * @return NEON_OK on success, or a module error/status code.
 */
neon_status_t neon_spi_receive_words(SPI_TypeDef *instance, uint32_t *data, uint16_t count, uint32_t dummy_data,
                                     uint32_t timeout_polls);
/**
 * @brief Transmit SPI receive words.
 * @param[in] instance Peripheral register instance.
 * @param[in] tx_data Tx data.
 * @param[in,out] rx_data Rx data.
 * @param[in] count Number of elements to process.
 * @param[in] timeout_polls Polling iteration budget; this value is not milliseconds.
 * @return NEON_OK on success, or a module error/status code.
 */
neon_status_t neon_spi_transmit_receive_words(SPI_TypeDef *instance, const uint32_t *tx_data, uint32_t *rx_data,
                                              uint16_t count, uint32_t timeout_polls);

/**
 * @brief enable SPI interrupt.
 * @param[in] instance Peripheral register instance.
 * @return NEON_OK on success, or a module error/status code.
 * @details
 * INT_EN gate only outputs IRQ pin signal; STT flags are readable when interrupt is turned off. Flags
 * are FIFO level state and self-erase when condition no longer exists, no W1C.
 */
neon_status_t neon_spi_enable_interrupt(SPI_TypeDef *instance);
/**
 * @brief Disable SPI interrupt.
 * @param[in] instance Peripheral register instance.
 * @return NEON_OK on success, or a module error/status code.
 */
neon_status_t neon_spi_disable_interrupt(SPI_TypeDef *instance);
/**
 * @brief Check SPI interrupt enabled.
 * @param[in] instance Peripheral register instance.
 * @param[in,out] enabled Enabled.
 * @return NEON_OK on success, or a module error/status code.
 */
neon_status_t neon_spi_is_interrupt_enabled(SPI_TypeDef *instance, uint8_t *enabled);
/**
 * @brief Read SPI active flags.
 * @param[in] instance Peripheral register instance.
 * @param[out] flags Status-flag bitmask.
 * @return NEON_OK on success, or a module error/status code.
 */
neon_status_t neon_spi_get_active_flags(SPI_TypeDef *instance, uint32_t *flags);
/**
 * @brief Check the selected status flags of SPI.
 * @param[in] instance Peripheral register instance.
 * @param[in] flags Status-flag bitmask.
 * @param[out] active Receives whether the selected flag is active.
 * @return NEON_OK on success, or a module error/status code.
 */
neon_status_t neon_spi_is_active_flag(SPI_TypeDef *instance, uint32_t flags, uint8_t *active);

/**
 * @brief Read SPI clock status.
 * @param[in] instance Peripheral register instance.
 * @param[out] status Destination for the status snapshot.
 * @return NEON_OK on success, or a module error/status code.
 * @details
 * CLKS is read correctly once so that three fields belong to the same snapshot.
 */
neon_status_t neon_spi_get_clock_status(SPI_TypeDef *instance, neon_spi_clock_status_t *status);

/**
 * @brief Configure SPI DMA.
 * @param[in] instance Peripheral register instance.
 * @param[in] direction direction.
 * @param[in] config Module configuration.
 * @return NEON_OK on success, or a module error/status code.
 * @details
 * ConfigureDMA to configure SPI's request channel and enable DMA global when state is ENABLE;
 * application must still configure DMAC and address TX/RX. For BURST, driver sets threshold FIFO by
 * UG. Master RX still needs TX to generate clock; before DMA full-duplex/RX must call
 * SetReceiveCapture(instance, 1).
 * This HAL configures one-shot DMA. Every enable programs the disabled
 * channel, pulses its CLEAR bit, then arms it, clearing stale completion and
 * counters without disturbing the other direction. Circular/DDR registers
 * are described in FU2.h but are not exposed by these legacy transfer APIs.
 * state=DISABLE stops the channel while running, ignores other fields and keeps channel
 * configuration/FIFO; disables DMA global when both channels are disabled. configuration FIFO,
 * timing, clock, data width, SS_MODE/SS_LONG and receive capture return BUSY when DMA is on with at
 * least one channel enabled. SS_FORCE remains switchable.
 */
neon_status_t neon_spi_configure_dma(SPI_TypeDef *instance, neon_spi_dma_direction_t direction,
                                     const neon_spi_dma_config_t *config);
/**
 * @brief enable SPI DMA.
 * @param[in] instance Peripheral register instance.
 * @return NEON_OK on success, or a module error/status code.
 */
neon_status_t neon_spi_enable_dma(SPI_TypeDef *instance);
/**
 * @brief Disable SPI DMA.
 * @param[in] instance Peripheral register instance.
 * @return NEON_OK on success, or a module error/status code.
 */
neon_status_t neon_spi_disable_dma(SPI_TypeDef *instance);
/**
 * @brief Check SPI DMA enabled.
 * @param[in] instance Peripheral register instance.
 * @param[in,out] enabled Enabled.
 * @return NEON_OK on success, or a module error/status code.
 */
neon_status_t neon_spi_is_dma_enabled(SPI_TypeDef *instance, uint8_t *enabled);
/**
 * @brief Check SPI DMA complete.
 * @param[in] instance Peripheral register instance.
 * @param[in] direction direction.
 * @param[in,out] complete Complete.
 * @return NEON_OK on success, or a module error/status code.
 * @details
 * The done flag by 1 is complete according to the DTI register table and UVM sequence.
 */
neon_status_t neon_spi_is_dma_complete(SPI_TypeDef *instance, neon_spi_dma_direction_t direction, uint8_t *complete);

#ifdef __cplusplus
}
#endif

#endif /* __NEON_SPI_H__ */
