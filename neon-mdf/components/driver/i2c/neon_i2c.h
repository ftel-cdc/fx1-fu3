/**
 * @file neon_i2c.h
 * @brief Public declarations for I2C.
 * @details
 * * NEON I2C - HAL for FU2's DTI I2C controller.
 *
 * The public API uses unshifted addresses and logical values. The backend manages clock/reset, pinmux,
 * FIFO and FU2 hardware specific clear pulses.
 * @note Blocking polling/configuration operations are intended for task context.
 * Serialize configuration/teardown with other users of the same peripheral;
 * timeout_polls values are polling budgets, not milliseconds, where applicable.
 */

#ifndef __NEON_I2C_H__
#define __NEON_I2C_H__

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include "neon_types.h"
#include "i2c/neon_i2c_device.h"

#ifdef __cplusplus
extern "C" {
#endif

/*==================================================================================================
*                                        DEFINES AND MACROS
==================================================================================================*/
#define NEON_I2C_FIFO_DEPTH 32U
#define NEON_I2C_MAX_TRANSFER_SIZE 256U

/*==================================================================================================
*                                  STRUCTURES AND OTHER TYPEDEFS
==================================================================================================*/
typedef enum {
    NEON_I2C_MODE_SLAVE = 0,
    NEON_I2C_MODE_MASTER = 1,
} neon_i2c_mode_t;

typedef enum {
    NEON_I2C_SPEED_STANDARD_FAST = 0,
    NEON_I2C_SPEED_FAST_PLUS = 1,
    NEON_I2C_SPEED_HIGH = 2,
    NEON_I2C_SPEED_ULTRA_FAST = 3,
} neon_i2c_speed_mode_t;

typedef enum {
    NEON_I2C_ADDRESSING_7BIT = 0,
    NEON_I2C_ADDRESSING_10BIT = 1,
} neon_i2c_addressing_mode_t;

typedef enum {
    NEON_I2C_CLOCK_STRETCH_DISABLE = 0,
    NEON_I2C_CLOCK_STRETCH_ENABLE = 1,
} neon_i2c_clock_stretch_t;

typedef enum {
    /* FU2: SDA=package 18, SCL=19, input feedback/open-drain enabled. DeInit restores saved pad/mux; bus
     * needs external pull-up. */
    NEON_I2C_PIN_CONFIG_AUTO = 0,
    /* Preserve pin routing configured by the board/boot firmware. */
    NEON_I2C_PIN_CONFIG_KEEP = 1,
} neon_i2c_pin_config_t;

typedef enum {
    NEON_I2C_STATE_RESET = 0,
    NEON_I2C_STATE_READY = 1,
    NEON_I2C_STATE_BUSY_TX = 2,
    NEON_I2C_STATE_BUSY_RX = 3,
    NEON_I2C_STATE_BUSY_CONFIG = 4,
    NEON_I2C_STATE_INITIALIZED = 5,
    /* STOP/bus recovery failed. Call set_config again after fixing the bus. */
    NEON_I2C_STATE_FAULT = 6,
} neon_i2c_state_t;

typedef enum {
    NEON_I2C_DIRECTION_WRITE = 0,
    NEON_I2C_DIRECTION_READ = 1,
} neon_i2c_direction_t;

typedef enum {
    NEON_I2C_DMA_TX = 0,
    NEON_I2C_DMA_RX = 1,
} neon_i2c_dma_direction_t;

typedef enum {
    NEON_I2C_DMA_SINGLE = 0,
    NEON_I2C_DMA_BURST = 1,
} neon_i2c_dma_mode_t;

typedef enum {
    NEON_I2C_DMA_BURST_2 = 0,
    NEON_I2C_DMA_BURST_4 = 1,
    NEON_I2C_DMA_BURST_8 = 2,
    NEON_I2C_DMA_BURST_16 = 3,
} neon_i2c_dma_burst_t;

typedef struct {
    neon_i2c_dma_mode_t mode;
    neon_i2c_dma_burst_t burst_length;
    neon_field16_t total_bytes; /* 1..255; zero encoding is not used. */
} neon_i2c_dma_config_t;

/* Logical flags; the backend combines ISR with address/data NACK from IITR. */
typedef uint32_t neon_i2c_flag_t;
#define NEON_I2C_FLAG_BUS_BUSY ((1U << 0))
#define NEON_I2C_FLAG_TRANSFER_DONE ((1U << 1))
#define NEON_I2C_FLAG_ARBITRATION_LOST ((1U << 2))
#define NEON_I2C_FLAG_ADDRESS_NACK ((1U << 3))
#define NEON_I2C_FLAG_DATA_NACK ((1U << 4))
#define NEON_I2C_FLAG_TX_FIFO_FULL ((1U << 5))
#define NEON_I2C_FLAG_TX_FIFO_EMPTY ((1U << 6))
#define NEON_I2C_FLAG_TX_FIFO_PROG_FULL ((1U << 7))
#define NEON_I2C_FLAG_TX_FIFO_PROG_EMPTY ((1U << 8))
#define NEON_I2C_FLAG_RX_FIFO_FULL ((1U << 9))
#define NEON_I2C_FLAG_RX_FIFO_EMPTY ((1U << 10))
#define NEON_I2C_FLAG_RX_FIFO_PROG_FULL ((1U << 11))
#define NEON_I2C_FLAG_RX_FIFO_PROG_EMPTY ((1U << 12))
#define NEON_I2C_FLAG_SCL_STUCK_LOW ((1U << 13))
#define NEON_I2C_FLAG_SDA_STUCK_LOW ((1U << 14))
#define NEON_I2C_FLAG_ALERT_DETECTED ((1U << 15))
#define NEON_I2C_FLAG_ALL (0x0000FFFFU)

/* interrupt bit matches IIER/IICR/IITR, not flag position in ISR. */
typedef uint32_t neon_i2c_interrupt_t;
#define NEON_I2C_INTERRUPT_TRANSFER_DONE ((1U << 0))
#define NEON_I2C_INTERRUPT_ADDRESS_NACK ((1U << 1))
#define NEON_I2C_INTERRUPT_DATA_NACK ((1U << 2))
#define NEON_I2C_INTERRUPT_TX_FIFO_FULL ((1U << 3))
#define NEON_I2C_INTERRUPT_TX_FIFO_EMPTY ((1U << 4))
#define NEON_I2C_INTERRUPT_TX_FIFO_PROG_FULL ((1U << 5))
#define NEON_I2C_INTERRUPT_TX_FIFO_PROG_EMPTY ((1U << 6))
#define NEON_I2C_INTERRUPT_RX_FIFO_FULL ((1U << 7))
#define NEON_I2C_INTERRUPT_RX_FIFO_EMPTY ((1U << 8))
#define NEON_I2C_INTERRUPT_RX_FIFO_PROG_FULL ((1U << 9))
#define NEON_I2C_INTERRUPT_RX_FIFO_PROG_EMPTY ((1U << 10))
#define NEON_I2C_INTERRUPT_SCL_STUCK_LOW ((1U << 11))
#define NEON_I2C_INTERRUPT_SDA_STUCK_LOW ((1U << 12))
#define NEON_I2C_INTERRUPT_PEC_ERROR ((1U << 13))
#define NEON_I2C_INTERRUPT_ADDRESS_MATCH ((1U << 14))
#define NEON_I2C_INTERRUPT_ARBITRATION_LOST ((1U << 15))
#define NEON_I2C_INTERRUPT_ALERT_DETECTED ((1U << 16))
#define NEON_I2C_INTERRUPT_STOP_DETECTED ((1U << 17))
#define NEON_I2C_INTERRUPT_DMA_TX_DONE ((1U << 18))
#define NEON_I2C_INTERRUPT_DMA_RX_DONE ((1U << 19))
#define NEON_I2C_INTERRUPT_ALL (0x000FFFFFU)

#define NEON_I2C_ERROR_NONE 0x00000000U
#define NEON_I2C_ERROR_ADDRESS_NACK 0x00000001U
#define NEON_I2C_ERROR_DATA_NACK 0x00000002U
#define NEON_I2C_ERROR_ARBITRATION_LOST 0x00000004U
#define NEON_I2C_ERROR_SCL_STUCK_LOW 0x00000008U
#define NEON_I2C_ERROR_SDA_STUCK_LOW 0x00000010U
#define NEON_I2C_ERROR_PEC 0x00000020U
#define NEON_I2C_ERROR_CLOCK 0x00000040U
#define NEON_I2C_ERROR_TIMEOUT 0x00000080U
#define NEON_I2C_ERROR_RECOVERY_REQUIRED 0x00000100U
#define NEON_I2C_ERROR_SHORT_TRANSFER 0x00000200U
#define NEON_I2C_ERROR_RX_OVERFLOW 0x00000400U

typedef struct {
    /* Raw i2c_clk counts. scl_high_count excludes input synchronization/filter latency. */
    neon_field16_t scl_low_count;
    neon_field16_t scl_high_count;
    neon_field16_t scl_low_count_high_speed;
    neon_field16_t scl_high_count_high_speed;
    neon_field16_t spike_count;
    neon_field16_t start_setup_count;
    neon_field16_t start_hold_count;
    neon_field16_t stop_setup_count;
    neon_field16_t sda_hold_count;
    neon_field16_t bus_free_count;
    neon_field16_t scl_stuck_low_count;
    neon_field16_t sda_stuck_low_count;
} neon_i2c_timing_t;

typedef struct {
    /* FU2 integrates FIFO 32 deep but the field threshold is only 4 bit wide. */
    neon_field8_t tx_full_level;
    neon_field8_t tx_empty_level;
    neon_field8_t rx_full_level;
    neon_field8_t rx_empty_level;
} neon_i2c_fifo_config_t;

typedef struct {
    neon_field8_t enable;
    neon_field8_t pec_enable;
    neon_field8_t spike_filter_enable;
    neon_field8_t quick_command_enable;
    neon_field8_t alert_enable;
} neon_i2c_smbus_config_t;

typedef struct {
    neon_i2c_mode_t mode;
    neon_i2c_speed_mode_t speed_mode;
    neon_i2c_addressing_mode_t addressing_mode;
    neon_i2c_clock_stretch_t clock_stretch;
    /* address FU2 when in slave mode; master mode ignores this field. */
    neon_field16_t own_address;
    neon_i2c_pin_config_t pin_config;
    neon_i2c_timing_t timing;
    neon_i2c_fifo_config_t fifo;
} neon_i2c_config_t;

typedef struct {
    neon_field8_t clock_failure;
    neon_field16_t failure_address;
    neon_i2c_direction_t failed_direction;
} neon_i2c_clock_status_t;

typedef struct {
    neon_field8_t matched_address;
    neon_i2c_direction_t direction;
    neon_field8_t pec;
} neon_i2c_slave_status_t;

typedef struct neon_i2c_handle_s {
    /* Driver-owned runtime fields; use the public APIs to inspect or update. */
    neon_i2c_t instance;
    volatile neon_i2c_state_t state;
    volatile uint32_t error_code;
    uint32_t private_context; /* Backend used; application does not fix this field. */
} neon_i2c_handle_t;

/*==================================================================================================
*                                       FUNCTION PROTOTYPES
==================================================================================================*/
/**
 * @brief Fill the default configuration for i2 c.
 * @param[out] config Module configuration.
 * @details
 * FU2: master, 7-bit addressing, AUTO open-drain pins, target SCL 100 kHz.
 * Timing uses the current system clock, which also clocks I2C on FU2.
 * Recalculate timing after changing that clock.
 */
void neon_i2c_get_default_config(neon_i2c_config_t *config);

/**
 * @brief Calculate conservative Standard/Fast timing for FU2.
 * @param i2c_clock_hz Actual I2C input clock, 1..60000000 Hz.
 * @param scl_hz Requested SCL, 1..400000 Hz. Actual SCL can be slower.
 * @param[out] timing Register counts, including compensation for the two-stage
 * input synchronizer and spike_count=1. Output is unchanged on invalid input.
 */
neon_status_t neon_i2c_calculate_timing(uint32_t i2c_clock_hz, uint32_t scl_hz, neon_i2c_timing_t *timing);

/**
 * @brief Initialize a caller-owned I2C runtime handle without configuring hardware.
 * @param[out] hi2c Handle storage; it need not be preinitialized.
 * @param[in] instance Device instance, for example NEON_I2C0.
 * @return NEON_OK, NEON_INVALID_PARAM, or NEON_BUSY if the instance is owned.
 * @details Call neon_i2c_set_config() before transfers. The driver owns all
 * handle fields. Initialization and configuration are task-context operations.
 */
neon_status_t neon_i2c_init(neon_i2c_handle_t *hi2c, neon_i2c_t instance);

/**
 * @brief Apply configuration and acquire exclusive ownership of the peripheral.
 * @param[in,out] hi2c Initialized handle.
 * @param[in] config Configuration copied by the driver; valid until return.
 * @return NEON_OK, a validation error, NEON_BUSY, or a hardware error.
 * @details On a ready handle this operation deinitializes and reconfigures the
 * peripheral. Serialize configuration/teardown with other users of this handle.
 * A successful configuration owns the instance until neon_i2c_deinit().
 * FU2 AUTO configures pin routing; KEEP preserves board-provided routing.
 * Ultra Fast returns NEON_NOT_SUPPORTED. A FAULT handle can be reconfigured
 * to reset the controller after fixing the bus and quiescing any external DMAC.
 */
neon_status_t neon_i2c_set_config(neon_i2c_handle_t *hi2c, const neon_i2c_config_t *config);

/**
 * @brief Read the active driver-owned configuration.
 * @param[in] hi2c Configured handle.
 * @param[out] config Destination for a configuration snapshot.
 * @return NEON_OK, NEON_INVALID_PARAM, NEON_ERROR if unconfigured, or NEON_BUSY.
 */
neon_status_t neon_i2c_get_config(const neon_i2c_handle_t *hi2c, neon_i2c_config_t *config);

/**
 * @brief Read the peripheral bound to a handle.
 * @param[in] hi2c Initialized handle, or NULL.
 * @return Device instance, or NULL for a NULL handle.
 */
neon_i2c_t neon_i2c_get_instance(const neon_i2c_handle_t *hi2c);

/**
 * @brief Deinitialize i2 c.
 * @param[in,out] hi2c I2C handle and runtime state.
 * @return NEON_OK on success, or a module error/status code.
 */
neon_status_t neon_i2c_deinit(neon_i2c_handle_t *hi2c);

/**
 * @brief I2 c master transmit.
 * @param[in,out] hi2c I2C handle and runtime state.
 * @param[in] dev_address Unshifted 7-bit or 10-bit I2C device address.
 * @param[in] data Transfer data; FU2 RAM buffers must own their complete aligned 32-bit words.
 * @param[in] size Number of transfer elements.
 * @param[in] timeout_polls Polling iteration budget; this value is not milliseconds.
 * @return NEON_OK on success, or a module error/status code.
 * @details
 * dev_address is the unshifted address 7/10 bit. Valid size 1..256; value 256 encoded as 0 in 8-bit
 * IBCR according to DTI IP's 256-byte limit. Master polling temporarily uses
 * software STOP and masks I2C0 in NVIC while reading DONE/NACK from IITR;
 * the previous interrupt-enable and hardware-end settings are restored on normal completion.
 * timeout_polls is the polling budget for the transfer, not milliseconds. Error cleanup
 * can take up to 10000 additional polls while waiting for STOP and bus release. A failed
 * cleanup leaves NEON_I2C_STATE_FAULT and NEON_I2C_ERROR_RECOVERY_REQUIRED;
 * call set_config to recover. Polling APIs and ReadData/WriteData return NEON_BUSY
 * while DMA requests or handshakes remain active.
 * The byte buffer must own all 32-bit words containing it (two-end padding),
 * under the FU2 runtime buffer contract. Drivers use memcpy for RAM buffers.
 */
neon_status_t neon_i2c_master_transmit(neon_i2c_handle_t *hi2c, uint16_t dev_address, const uint8_t *data,
                                       uint16_t size, uint32_t timeout_polls);
/**
 * @brief I2 c master receive.
 * @param[in,out] hi2c I2C handle and runtime state.
 * @param[in] dev_address Unshifted 7-bit or 10-bit I2C device address.
 * @param[out] data Transfer data; FU2 RAM buffers must own their complete aligned 32-bit words.
 * @param[in] size Number of transfer elements.
 * @param[in] timeout_polls Polling iteration budget; this value is not milliseconds.
 * @return NEON_OK on success, or a module error/status code.
 */
neon_status_t neon_i2c_master_receive(neon_i2c_handle_t *hi2c, uint16_t dev_address, uint8_t *data, uint16_t size,
                                      uint32_t timeout_polls);

/**
 * @brief I2 c mem read.
 * @param[in,out] hi2c I2C handle and runtime state.
 * @param[in] dev_address Unshifted 7-bit or 10-bit I2C device address.
 * @param[in] register_address Device register sub-address.
 * @param[out] data Transfer data; FU2 RAM buffers must own their complete aligned 32-bit words.
 * @param[in] data_size Number of payload bytes.
 * @param[in] timeout_polls Polling iteration budget; this value is not milliseconds.
 * @return NEON_OK on success, or a module error/status code.
 * @details
 * Mem_Read uses 8 bit sub-address + repeated START by hardware. Mem_Write sends the sub-address as
 * first data byte so data_size max 255 bytes (total transaction does not exceed 256 bytes).
 */
neon_status_t neon_i2c_mem_read(neon_i2c_handle_t *hi2c, uint16_t dev_address, uint8_t register_address, uint8_t *data,
                                neon_field16_t data_size, uint32_t timeout_polls);
/**
 * @brief I2 c mem write.
 * @param[in,out] hi2c I2C handle and runtime state.
 * @param[in] dev_address Unshifted 7-bit or 10-bit I2C device address.
 * @param[in] register_address Device register sub-address.
 * @param[in] data Transfer data; FU2 RAM buffers must own their complete aligned 32-bit words.
 * @param[in] data_size Number of payload bytes.
 * @param[in] timeout_polls Polling iteration budget; this value is not milliseconds.
 * @return NEON_OK on success, or a module error/status code.
 */
neon_status_t neon_i2c_mem_write(neon_i2c_handle_t *hi2c, uint16_t dev_address, uint8_t register_address,
                                 const uint8_t *data, neon_field16_t data_size, uint32_t timeout_polls);

/**
 * @brief I2 c slave transmit.
 * @param[in,out] hi2c I2C handle and runtime state.
 * @param[in] data Transfer data; FU2 RAM buffers must own their complete aligned 32-bit words.
 * @param[in] size Number of transfer elements.
 * @param[in] timeout_polls Polling iteration budget; this value is not milliseconds.
 * @return NEON_OK on success, or a module error/status code.
 * @details
 * API slave blocking. Calls must be made before the other controller starts the transaction so that
 * FIFO is ready; API returns after receiving STOP. With data larger than FIFO, clock_stretch should
 * be enabled to avoid TX underflow/RX overflow.
 * Wait for this API to return before the next frame. Early STOP with unsent data returns NEON_ERROR and
 * NEON_I2C_ERROR_SHORT_TRANSFER. Hardware cannot report the exact ACKed TX count.
 */
neon_status_t neon_i2c_slave_transmit(neon_i2c_handle_t *hi2c, const uint8_t *data, uint16_t size,
                                      uint32_t timeout_polls);
/**
 * @brief I2 c slave receive.
 * @param[in,out] hi2c I2C handle and runtime state.
 * @param[out] data Transfer data; FU2 RAM buffers must own their complete aligned 32-bit words.
 * @param[in] size Number of transfer elements.
 * @param[in] timeout_polls Polling iteration budget; this value is not milliseconds.
 * @return NEON_OK on success, or a module error/status code.
 */
neon_status_t neon_i2c_slave_receive(neon_i2c_handle_t *hi2c, uint8_t *data, uint16_t size, uint32_t timeout_polls);

/**
 * @brief Receive one slave transaction and report bytes copied before STOP/timeout.
 * @details Requires a coordinated master: call before it starts, and wait for return
 * before the next frame. STOP before size sets NEON_I2C_ERROR_SHORT_TRANSFER; excess bytes
 * are drained without overrunning data and set NEON_I2C_ERROR_RX_OVERFLOW. Both return
 * NEON_ERROR. received_count is at most size. Clock stretching should be enabled
 * for frames larger than the FIFO. A timeout on an active bus leaves STATE_FAULT.
 */
neon_status_t neon_i2c_slave_receive_ex(neon_i2c_handle_t *hi2c, uint8_t *data, uint16_t size,
                                      uint32_t *received_count, uint32_t timeout_polls);

/**
 * @brief Write i2 c data.
 * @param[in,out] hi2c I2C handle and runtime state.
 * @param[in] data Transfer data; FU2 RAM buffers must own their complete aligned 32-bit words.
 * @return NEON_OK on success, or a module error/status code.
 * @details
 * Primitive does not block interrupt slaves. Returns NEON_BUSY when FIFO TX is full or FIFO RX is
 * empty. Calls only when handle is READY.
 */
neon_status_t neon_i2c_write_data(neon_i2c_handle_t *hi2c, uint8_t data);
/**
 * @brief Read i2 c data.
 * @param[in,out] hi2c I2C handle and runtime state.
 * @param[out] data Transfer data; FU2 RAM buffers must own their complete aligned 32-bit words.
 * @return NEON_OK on success, or a module error/status code.
 */
neon_status_t neon_i2c_read_data(neon_i2c_handle_t *hi2c, uint8_t *data);
/**
 * @brief Read i2 c slave status.
 * @param[in] hi2c I2C handle and runtime state.
 * @param[out] status Destination for the status snapshot.
 * @return NEON_OK on success, or a module error/status code.
 */
neon_status_t neon_i2c_get_slave_status(const neon_i2c_handle_t *hi2c, neon_i2c_slave_status_t *status);

/**
 * @brief enable i2 c interrupts.
 * @param[in,out] hi2c I2C handle and runtime state.
 * @param[in] interrupts interrupt-source bitmask.
 * @return NEON_OK on success, or a module error/status code.
 * @details
 * In master mode, enable TRANSFER_DONE as FU2 keeps SCL low at the end of byte-count and does not
 * generate STOP itself; ISR must decide to repeated START or call GenerateStop().
 */
neon_status_t neon_i2c_enable_interrupts(neon_i2c_handle_t *hi2c, uint32_t interrupts);
/**
 * @brief Disable i2 c interrupts.
 * @param[in,out] hi2c I2C handle and runtime state.
 * @param[in] interrupts interrupt-source bitmask.
 * @return NEON_OK on success, or a module error/status code.
 */
neon_status_t neon_i2c_disable_interrupts(neon_i2c_handle_t *hi2c, uint32_t interrupts);
/**
 * @brief Read i2 c interrupt status.
 * @param[in] hi2c I2C handle and runtime state.
 * @param[out] interrupts interrupt-source bitmask.
 * @return NEON_OK on success, or a module error/status code.
 */
neon_status_t neon_i2c_get_interrupt_status(const neon_i2c_handle_t *hi2c, uint32_t *interrupts);
/**
 * @brief Clear i2 c interrupts.
 * @param[in,out] hi2c I2C handle and runtime state.
 * @param[in] interrupts interrupt-source bitmask.
 * @return NEON_OK on success, or a module error/status code.
 * @details
 * FU2's IICR requires pulse 1 -> 0; the API performs enough of both writes.
 * ClearInterrupts/ClearSMBusFlags/GenerateStop returns NEON_BUSY when the other API is holding the
 * handle; ISR must not use raw MMIO to erase polling flags.
 */
neon_status_t neon_i2c_clear_interrupts(neon_i2c_handle_t *hi2c, uint32_t interrupts);

/**
 * @brief Read i2 c flags.
 * @param[in] hi2c I2C handle and runtime state.
 * @param[out] flags Status-flag bitmask.
 * @return NEON_OK on success, or a module error/status code.
 */
neon_status_t neon_i2c_get_flags(const neon_i2c_handle_t *hi2c, uint32_t *flags);
/**
 * @brief Check the selected status flags of i2 c.
 * @param[in] hi2c I2C handle and runtime state.
 * @param[in] flags Status-flag bitmask.
 * @param[out] active Receives whether the selected flag is active.
 * @return NEON_OK on success, or a module error/status code.
 */
neon_status_t neon_i2c_is_active_flag(const neon_i2c_handle_t *hi2c, uint32_t flags, uint8_t *active);

/**
 * @brief Configure i2 c DMA.
 * @param[in,out] hi2c I2C handle and runtime state.
 * @param[in] direction direction.
 * @param[in] enable Nonzero to enable, zero to disable.
 * @return NEON_OK on success, or a module error/status code.
 * @details
 * Call set_dma_config first. Enabling also enables the matching DMA DONE event in IIER
 * for polling/interrupt use. The application must configure the DMAC channel, addresses
 * and byte count separately. Disabling requests does not cancel outstanding DMAC accesses.
 */
neon_status_t neon_i2c_configure_dma(neon_i2c_handle_t *hi2c, neon_i2c_dma_direction_t direction, uint8_t enable);

/**
 * @brief Program a disabled I2C DMA request channel while the bus is idle.
 * @details Sets transfer length and SINGLE/BURST. Configure FIFO thresholds with set_config
 * to match the burst length and FIFO capacity before enabling DMA.
 * Configure the system DMAC separately, then call configure_dma(..., 1).
 * This API does not start an I2C transaction. Disabling requests does not abort
 * outstanding DMAC accesses; wait for DMAC quiescence before resetting the IP.
 */
neon_status_t neon_i2c_set_dma_config(neon_i2c_handle_t *hi2c, neon_i2c_dma_direction_t direction,
                                    const neon_i2c_dma_config_t *config);

/** Peripheral DMA DONE and no outstanding peripheral handshake; not I2C STOP. */
neon_status_t neon_i2c_is_dma_complete(const neon_i2c_handle_t *hi2c, neon_i2c_dma_direction_t direction,
                                     uint8_t *complete);
/**
 * @brief Request i2 c bus clear.
 * @param[in,out] hi2c I2C handle and runtime state.
 * @return NEON_OK on success, or a module error/status code.
 */
neon_status_t neon_i2c_request_bus_clear(neon_i2c_handle_t *hi2c);
/**
 * @brief I2 c generate stop.
 * @param[in,out] hi2c I2C handle and runtime state.
 * @return NEON_OK on success, or a module error/status code.
 */
neon_status_t neon_i2c_generate_stop(neon_i2c_handle_t *hi2c);

/**
 * @brief Configure i2 c sm bus.
 * @param[in,out] hi2c I2C handle and runtime state.
 * @param[in] config Module configuration.
 * @return NEON_OK on success, or a module error/status code.
 */
neon_status_t neon_i2c_configure_smbus(neon_i2c_handle_t *hi2c, const neon_i2c_smbus_config_t *config);
/**
 * @brief Clear i2 c sm bus flags.
 * @param[in,out] hi2c I2C handle and runtime state.
 * @param[in] clear_alert Clear alert.
 * @param[in] clear_pec Clear pec.
 * @return NEON_OK on success, or a module error/status code.
 */
neon_status_t neon_i2c_clear_smbus_flags(neon_i2c_handle_t *hi2c, uint8_t clear_alert, uint8_t clear_pec);
/**
 * @brief Set i2 c device id.
 * @param[in,out] hi2c I2C handle and runtime state.
 * @param[in] device_id Device id.
 * @return NEON_OK on success, or a module error/status code.
 */
neon_status_t neon_i2c_set_device_id(neon_i2c_handle_t *hi2c, uint32_t device_id);
/**
 * @brief enable i2 c device id read.
 * @param[in,out] hi2c I2C handle and runtime state.
 * @param[in] enable Nonzero to enable, zero to disable.
 * @return NEON_OK on success, or a module error/status code.
 */
neon_status_t neon_i2c_enable_device_id_read(neon_i2c_handle_t *hi2c, uint8_t enable);
/**
 * @brief enable i2 c general call reset.
 * @param[in,out] hi2c I2C handle and runtime state.
 * @param[in] enable Nonzero to enable, zero to disable.
 * @return NEON_OK on success, or a module error/status code.
 */
neon_status_t neon_i2c_enable_general_call_reset(neon_i2c_handle_t *hi2c, uint8_t enable);

/**
 * @brief Read i2 c clock status.
 * @param[in,out] hi2c I2C handle and runtime state.
 * @param[out] status Destination for the status snapshot.
 * @return NEON_OK on success, or a module error/status code.
 * @details
 * ICLFT is read-to-clear, so each call takes exactly one snapshot.
 */
neon_status_t neon_i2c_get_clock_status(neon_i2c_handle_t *hi2c, neon_i2c_clock_status_t *status);

/**
 * @brief Read i2 c state.
 * @param[in] hi2c I2C handle and runtime state.
 * @return Current I2C handle state.
 */
neon_i2c_state_t neon_i2c_get_state(const neon_i2c_handle_t *hi2c);
/**
 * @brief Read i2 c error.
 * @param[in] hi2c I2C handle and runtime state.
 * @return Module error bitmask.
 */
uint32_t neon_i2c_get_error(const neon_i2c_handle_t *hi2c);
/**
 * @brief Clear i2 c error.
 * @param[in,out] hi2c I2C handle and runtime state.
 * @details
 * Only erase errors when handle READY; call in transfer does not work.
 */
void neon_i2c_clear_error(neon_i2c_handle_t *hi2c);

#ifdef __cplusplus
}
#endif

#endif /* __NEON_I2C_H__ */
