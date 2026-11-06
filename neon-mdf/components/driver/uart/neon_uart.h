/**
 * @file neon_uart.h
 * @brief Public declarations for UART.
 * @details
 * * NEON_UART - HAL UART commonly used for Neon MDF backends.
 *
 * public configuration uses logical values, without revealing FU2 LL bit code. The backend is
 * responsible for mapping to the register of each chip. Newly added fields have a default value of
 * 0 so that old code only assigned baud_rate remains 8-N-1, oversampling x16, no FIFO and no
 * flow-control.
 * @note Blocking polling/configuration operations are intended for task context.
 * Serialize configuration/teardown with other users of the same peripheral;
 * timeout_polls values are polling budgets, not milliseconds, where applicable.
 */

#ifndef __NEON_UART_H__
#define __NEON_UART_H__

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include "neon_types.h"
#include "uart/neon_uart_device.h"

#ifdef __cplusplus
extern "C" {
#endif

/*==================================================================================================
*                                  STRUCTURES AND OTHER TYPEDEFS
==================================================================================================*/
typedef enum {
    NEON_UART_WORD_LENGTH_8B = 0,
    NEON_UART_WORD_LENGTH_5B = 1,
    NEON_UART_WORD_LENGTH_6B = 2,
    NEON_UART_WORD_LENGTH_7B = 3,
    NEON_UART_WORD_LENGTH_9B = 4,
} neon_uart_word_length_t;

typedef enum {
    NEON_UART_STOP_BITS_1 = 0,
    NEON_UART_STOP_BITS_2 = 1,
} neon_uart_stop_bits_t;

typedef enum {
    NEON_UART_PARITY_NONE = 0,
    NEON_UART_PARITY_ODD = 1,
    NEON_UART_PARITY_EVEN = 2,
} neon_uart_parity_t;

typedef enum {
    NEON_UART_OVERSAMPLING_16 = 0,
    NEON_UART_OVERSAMPLING_8 = 1,
} neon_uart_over_sampling_t;

typedef enum {
    NEON_UART_FIFO_DISABLE = 0,
    NEON_UART_FIFO_ENABLE = 1,
} neon_uart_fifo_state_t;

typedef enum {
    NEON_UART_FIFO_THRESHOLD_1_4 = 0,
    NEON_UART_FIFO_THRESHOLD_1_2 = 1,
    NEON_UART_FIFO_THRESHOLD_3_4 = 2,
    NEON_UART_FIFO_THRESHOLD_FULL = 3,
} neon_uart_fifo_threshold_t;

typedef enum {
    NEON_UART_HWCONTROL_NONE = 0,
    NEON_UART_HWCONTROL_RTS = 1,
    NEON_UART_HWCONTROL_AUTO_FLOW = 2,
    NEON_UART_HWCONTROL_RTS_AUTO_FLOW = 3,
} neon_uart_hardware_flow_control_t;

typedef enum {
    /* FU2 manually configures pad/mux TX/RX; DeInit restores saved bits. */
    NEON_UART_PIN_CONFIG_AUTO = 0,
    /* Maintain that pinmux/PAD board or boot firmware configured. */
    NEON_UART_PIN_CONFIG_KEEP = 1,
} neon_uart_pin_config_t;

typedef enum {
    /* FU2 DTI: TX stands for LSR.THRF=0; RX stands for LSR.DR=1. */
    NEON_UART_STATUS_SOURCE_HARDWARE = 0,
    /* Legacy compatibility shim: The old firmware/VP used to use MSR like PL011 FR with RXFE at bit 4. Do
   * not use this mode for the current FU2 VP. */
    NEON_UART_STATUS_SOURCE_FVP_MSR = 1,
} neon_uart_status_source_t;

typedef enum {
    NEON_UART_STATE_RESET = 0,
    NEON_UART_STATE_READY = 1,
    NEON_UART_STATE_BUSY_TX = 2,
    NEON_UART_STATE_BUSY_RX = 3,
    NEON_UART_STATE_BUSY_CONFIG = 4,
    NEON_UART_STATE_INITIALIZED = 5,
} neon_uart_state_t;

typedef uint32_t neon_uart_interrupt_t;
#define NEON_UART_INTERRUPT_RX_READY ((1U << 0))
#define NEON_UART_INTERRUPT_TX_FULL ((1U << 1))
/* Legacy name: on FU2 this interrupt indicates TX full, not TX ready. */
#define NEON_UART_INTERRUPT_TX_READY (NEON_UART_INTERRUPT_TX_FULL)
#define NEON_UART_INTERRUPT_LINE_ERROR ((1U << 2))
#define NEON_UART_INTERRUPT_MODEM ((1U << 3))
/* Logical policy flags: AUTO clears IER.AUCLR, MANUAL sets it on FU2. */
#define NEON_UART_INTERRUPT_AUTO_CLEAR ((1U << 4))
#define NEON_UART_INTERRUPT_MANUAL_CLEAR ((1U << 5))
#define NEON_UART_INTERRUPT_ALL (0x0FU)

typedef enum {
    NEON_UART_INTERRUPT_ID_NONE = 0,
    NEON_UART_INTERRUPT_ID_TX_FULL = 1,
    /* Legacy name; FU2 INTID=1 is the TX-full interrupt. */
    NEON_UART_INTERRUPT_ID_TX_READY = NEON_UART_INTERRUPT_ID_TX_FULL,
    NEON_UART_INTERRUPT_ID_RX_READY = 2,
    NEON_UART_INTERRUPT_ID_LINE_ERROR = 3,
    NEON_UART_INTERRUPT_ID_RX_TIMEOUT = 4,
    NEON_UART_INTERRUPT_ID_DMA_TX_DONE = 5,
    NEON_UART_INTERRUPT_ID_DMA_RX_DONE = 6,
    /* Reserved logical ID for other backends; FU2 INTID=0 means NONE. */
    NEON_UART_INTERRUPT_ID_MODEM = 0x80,
    NEON_UART_INTERRUPT_ID_UNKNOWN = 0xFF,
} neon_uart_interrupt_id_t;

/*==================================================================================================
*                                        DEFINES AND MACROS
==================================================================================================*/
#define NEON_UART_ERROR_NONE 0x00000000U
#define NEON_UART_ERROR_OVERRUN 0x00000001U
#define NEON_UART_ERROR_PARITY 0x00000002U
#define NEON_UART_ERROR_FRAME 0x00000004U
#define NEON_UART_ERROR_BREAK 0x00000008U
#define NEON_UART_ERROR_RX_FIFO 0x00000010U
#define NEON_UART_ERROR_CLOCK 0x00000020U
/* Legacy flags. FU2 reports CLOCK and stores address/command in clock_status. */
#define NEON_UART_ERROR_ADDRESS 0x00000040U
#define NEON_UART_ERROR_COMMAND 0x00000080U

typedef struct {
    uint32_t baud_rate;
    /* 0: backend takes the current clock from CRM/CMSIS. The non-zero value is the UART input clock in Hz
   * that the board is of the same level. */
    uint32_t clock_frequency_hz;
    neon_uart_word_length_t word_length;
    neon_uart_stop_bits_t stop_bits;
    neon_uart_parity_t parity;
    neon_uart_over_sampling_t over_sampling;
    neon_uart_fifo_state_t fifo_state;
    /* FU2 rev1p3 does not define threshold bits: only the default is supported. */
    neon_uart_fifo_threshold_t fifo_threshold;
    neon_uart_hardware_flow_control_t hardware_flow_control;
    /* FU2 does not have RTS/CTS mapping in SYSCSR LL table: hardware flow-control must select KEEP and
   * configure the handshake pins on board. */
    neon_uart_pin_config_t pin_config;
    neon_uart_status_source_t status_source;
} neon_uart_config_t;

typedef struct neon_uart_handle_s {
    /* Driver-owned runtime fields; use the public APIs to inspect or update. */
    neon_uart_t instance;
    volatile neon_uart_state_t state;
    volatile uint32_t error_code;
    /* Last FU2 CLKS fault snapshot: bit 0 flag, [12:1] address, bit 13 R/W.
   * CLKS is read-to-clear; this snapshot remains until the next receive/init
   * or ClearError. address/command fields are metadata, not error flags. */
    volatile uint32_t clock_status;
    uint32_t private_context; /* Backend used; application does not fix this field. */
} neon_uart_handle_t;

/*==================================================================================================
*                                       FUNCTION PROTOTYPES
==================================================================================================*/
/**
 * @brief Fill the default configuration for UART.
 * @param[out] config Module configuration.
 * @details
 * Fill default configuration 115200, 8-N-1, x16, FIFO off, manual pin configuration.
 */
void neon_uart_get_default_config(neon_uart_config_t *config);

/**
 * @brief Initialize a caller-owned UART runtime handle without configuring hardware.
 * @param[out] huart Handle storage; it need not be preinitialized.
 * @param[in] instance Device instance, for example NEON_UART0.
 * @return NEON_OK, NEON_INVALID_PARAM, or NEON_BUSY if the instance is owned.
 * @details Call neon_uart_set_config() before transfers. The driver owns all
 * handle fields. Initialization and configuration are task-context operations.
 */
neon_status_t neon_uart_init(neon_uart_handle_t *huart, neon_uart_t instance);

/**
 * @brief Apply configuration and acquire exclusive ownership of the peripheral.
 * @param[in,out] huart Initialized handle.
 * @param[in] config Configuration copied by the driver; valid until return.
 * @return NEON_OK, a validation error, NEON_BUSY, or a hardware error.
 * @details On a ready handle this operation deinitializes and reconfigures the
 * peripheral. Serialize configuration/teardown with other users of this handle.
 * A successful configuration owns the instance until neon_uart_deinit().
 * FU2 AUTO configures pin routing; KEEP preserves board-provided routing.
 */
neon_status_t neon_uart_set_config(neon_uart_handle_t *huart, const neon_uart_config_t *config);

/**
 * @brief Read the active driver-owned configuration.
 * @param[in] huart Configured handle.
 * @param[out] config Destination for a configuration snapshot.
 * @return NEON_OK, NEON_INVALID_PARAM, NEON_ERROR if unconfigured, or NEON_BUSY.
 */
neon_status_t neon_uart_get_config(const neon_uart_handle_t *huart, neon_uart_config_t *config);

/**
 * @brief Read the peripheral bound to a handle.
 * @param[in] huart Initialized handle, or NULL.
 * @return Device instance, or NULL for a NULL handle.
 */
neon_uart_t neon_uart_get_instance(const neon_uart_handle_t *huart);

/**
 * @brief Deinitialize UART.
 * @param[in,out] huart UART handle and runtime state.
 * @return NEON_OK on success, or a module error/status code.
 */
neon_status_t neon_uart_deinit(neon_uart_handle_t *huart);
/**
 * @brief Set UART baud rate.
 * @param[in,out] huart UART handle and runtime state.
 * @param[in] baud_rate Requested UART baud rate in bits per second.
 * @return NEON_OK on success, or a module error/status code.
 */
neon_status_t neon_uart_set_baud_rate(neon_uart_handle_t *huart, uint32_t baud_rate);

/**
 * @brief Poll the UART transmitter until the requested bytes have been queued.
 * @param[in,out] huart UART handle and runtime state.
 * @param[in] data Transfer data; FU2 RAM buffers must own their complete aligned 32-bit words.
 * @param[in] size Number of transfer elements.
 * @param[in] timeout_polls Polling iteration budget; this value is not milliseconds.
 * @return NEON_OK on success, or a module error/status code.
 * @details
 * timeout_polls is the maximum total number of poll rings for both calls, not milliseconds.
 * FU2 TX returns after enqueueing all data through THR using LSR[5] THRF=0;
 * it does not wait for the last byte to finish on the wire (LSR[6] TEMT).
 * Drain TEMT before resetting/reconfiguring the UART or changing its pins.
 */
neon_status_t neon_uart_transmit(neon_uart_handle_t *huart, const uint8_t *data, uint16_t size, uint32_t timeout_polls);
/**
 * @brief Poll the UART receiver until the requested bytes have been read.
 * @param[in,out] huart UART handle and runtime state.
 * @param[out] data Transfer data; FU2 RAM buffers must own their complete aligned 32-bit words.
 * @param[in] size Number of transfer elements.
 * @param[in] timeout_polls Polling iteration budget; this value is not milliseconds.
 * @return NEON_OK on success, or a module error/status code.
 */
neon_status_t neon_uart_receive(neon_uart_handle_t *huart, uint8_t *data, uint16_t size, uint32_t timeout_polls);

/**
 * @brief Read one available RX element without waiting (FU2).
 * @param[in,out] huart Configured UART handle.
 * @param[out] data Complete aligned 32-bit word; updated only on NEON_OK.
 * @return NEON_OK, NEON_TIMEOUT if empty, NEON_ERROR on a hardware error,
 * NEON_BUSY during RX/configuration, or NEON_NOT_SUPPORTED on other backends.
 * @details Safe in the UART ISR, including while polling TX owns the handle.
 * Does not change the transfer state or acknowledge IIR. Use a single RX
 * consumer; do not mix ISR reads with polling RX. Each attempt snapshots line
 * and clock errors, including errors without RX data. An erroneous available
 * element is discarded before NEON_ERROR is returned. Error snapshots remain
 * available through GetError until the next attempt or ClearError.
 */
neon_status_t neon_uart_try_receive(neon_uart_handle_t *huart, uint32_t *data);

/**
 * @brief Wait for the last queued TX element to leave the wire (FU2).
 * @param[in,out] huart Configured UART handle.
 * @param[in] timeout_polls Polling budget, not milliseconds.
 * @return NEON_OK, NEON_TIMEOUT, a handle error, or NEON_NOT_SUPPORTED.
 * @details Task-context operation. Call before resetting/reconfiguring UART.
 */
neon_status_t neon_uart_wait_tx_complete(neon_uart_handle_t *huart, uint32_t timeout_polls);

/**
 * @brief Transmit UART words.
 * @param[in,out] huart UART handle and runtime state.
 * @param[in] data Transfer data; FU2 RAM buffers must own their complete aligned 32-bit words.
 * @param[in] size Number of transfer elements.
 * @param[in] timeout_polls Polling iteration budget; this value is not milliseconds.
 * @return NEON_OK on success, or a module error/status code.
 * @details
 * 9-bit API. Each element uses only 9 low bits.
 */
neon_status_t neon_uart_transmit_words(neon_uart_handle_t *huart, const uint16_t *data, uint16_t size,
                                       uint32_t timeout_polls);
/**
 * @brief Receive UART words.
 * @param[in,out] huart UART handle and runtime state.
 * @param[out] data Transfer data; FU2 RAM buffers must own their complete aligned 32-bit words.
 * @param[in] size Number of transfer elements.
 * @param[in] timeout_polls Polling iteration budget; this value is not milliseconds.
 * @return NEON_OK on success, or a module error/status code.
 */
neon_status_t neon_uart_receive_words(neon_uart_handle_t *huart, uint16_t *data, uint16_t size, uint32_t timeout_polls);

/**
 * @brief enable UART interrupt.
 * @param[in,out] huart UART handle and runtime state.
 * @param[in] interrupts interrupt-source bitmask.
 * @return NEON_OK on success, or a module error/status code.
 */
neon_status_t neon_uart_enable_interrupt(neon_uart_handle_t *huart, uint32_t interrupts);
/**
 * @brief Disable UART interrupt.
 * @param[in,out] huart UART handle and runtime state.
 * @param[in] interrupts interrupt-source bitmask.
 * @return NEON_OK on success, or a module error/status code.
 */
neon_status_t neon_uart_disable_interrupt(neon_uart_handle_t *huart, uint32_t interrupts);
/**
 * @brief Read UART interrupt id.
 * @param[in] huart UART handle and runtime state.
 * @param[out] interrupt_id Receives the decoded interrupt source.
 * @return NEON_OK on success, or a module error/status code.
 */
neon_status_t neon_uart_get_interrupt_id(const neon_uart_handle_t *huart, neon_uart_interrupt_id_t *interrupt_id);
/**
 * @brief Check UART interrupt pending.
 * @param[in] huart UART handle and runtime state.
 * @param[out] pending Receives whether an interrupt is pending.
 * @return NEON_OK on success, or a module error/status code.
 */
neon_status_t neon_uart_is_interrupt_pending(const neon_uart_handle_t *huart, uint8_t *pending);
/* The above APIs control only UART's IER/IIR; the application still enables NVIC with
 * NVIC_EnableIRQ(USARTx_IRQn) and self - grants matching ISR/callback.
 * FU2 ALL includes interrupt sources only; select AUTO_CLEAR or MANUAL_CLEAR
 * separately (never both). enable/Disable AUTO_CLEAR clears/sets IER[4];
 * enable/Disable MANUAL_CLEAR sets/clears it. Reading IIR acknowledges pending
 * sources in manual mode: call GetInterruptID once and check NONE, rather than
 * calling IsInterruptPending followed by GetInterruptID for the same event. */

/* FU2 FIFO clear requires FIFO mode; non-FIFO mode returns NOT_SUPPORTED. */
/**
 * @brief Clear UART rx fifo.
 * @param[in,out] huart UART handle and runtime state.
 * @return NEON_OK on success, or a module error/status code.
 */
neon_status_t neon_uart_clear_rx_fifo(neon_uart_handle_t *huart);
/**
 * @brief Clear UART tx fifo.
 * @param[in,out] huart UART handle and runtime state.
 * @return NEON_OK on success, or a module error/status code.
 */
neon_status_t neon_uart_clear_tx_fifo(neon_uart_handle_t *huart);

/**
 * @brief Read UART state.
 * @param[in] huart UART handle and runtime state.
 * @return Current UART handle state.
 */
neon_uart_state_t neon_uart_get_state(const neon_uart_handle_t *huart);
/**
 * @brief Read UART error.
 * @param[in] huart UART handle and runtime state.
 * @return Module error bitmask.
 */
uint32_t neon_uart_get_error(const neon_uart_handle_t *huart);

#ifdef FU2
typedef enum {
    NEON_UART_DMA_RX = 0,
    NEON_UART_DMA_TX = 1,
} neon_uart_dma_direction_t;

/* UART0 uses peripheral request slot 2 on DMA0. */
#define NEON_UART0_DMA_PERIPHERAL_ID 2U

/**
 * @brief Configure single-request UART DMA handshaking (FU2).
 * @param[in,out] huart Configured 8-bit UART in FIFO mode.
 * @param[in] direction RX or TX.
 * @param[in] count UART characters; zero disables that direction.
 * @return NEON_OK, NEON_BUSY, NEON_INVALID_PARAM, NEON_NOT_SUPPORTED or NEON_ERROR.
 * @details Configure/start the DMA controller separately. Use aligned uint32_t
 * buffer elements, one UART character in each low byte; the DMA moves 4*count
 * memory bytes with one 32-bit FIFO access per request. Only one UART DMA
 * direction may be enabled at a time. CPU access to that direction and UART
 * reconfiguration are blocked until disabled. Polling TX remains available
 * during RX DMA for READY/status messages. Finish or reset the DMA controller
 * before reusing the buffer. Disabling RX also snapshots UART receive errors.
 */
neon_status_t neon_uart_set_dma(neon_uart_handle_t *huart, neon_uart_dma_direction_t direction, uint16_t count);
#endif

/**
 * @brief Clear UART error.
 * @param[in,out] huart UART handle and runtime state.
 * @details
 * Clears software error_code/clock_status only. In manual-clear mode, hardware
 * line-error flags are acknowledged by reading IIR through GetInterruptID.
 */
void neon_uart_clear_error(neon_uart_handle_t *huart);

#ifdef __cplusplus
}
#endif

#endif /* __NEON_UART_H__ */
