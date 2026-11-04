/**
 * @file neon_uart.c
 * @brief Implementation for UART.
 * @details
 * * Backend UART FU2.
 *
 * Register behavior follows DTI USART CTRL User Guide rev1p3. Public APIs
 * and parameter validation use the Neon HAL conventions.
 */

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/

#include "uart/neon_uart.h"
#include <string.h>

#include "crm/neon_crm.h"
#include "syscsr/platform/fu2/neon_syscsr_internal.h"

/*==================================================================================================
*                                        DEFINES AND MACROS
==================================================================================================*/
#define UART_CONTEXT_PIN_AUTO (1U << 0)
#define UART_CONTEXT_JTAG_WAS_ENABLED (1U << 1)

/* FVP FU2 dùng tạm MSR[5] làm TXFF và MSR[4] làm RXFE. Trên silicon thật
 * hai bit này vẫn mang nghĩa DSR/CTS và tuyệt đối không được dùng để poll. */
#define UART_FVP_MSR_RXFE UART_MSR_CTS_Msk
#define UART_FVP_MSR_TXFF UART_MSR_DSR_Msk

/*==================================================================================================
*                                         LOCAL VARIABLES
==================================================================================================*/
static uint32_t fu2_uart_fcr_shadow[5] = {0U, 0U, 0U, 0U, 0U};
static neon_uart_handle_t *fu2_uart_owner[5];
static neon_uart_config_t uart_configs[5];
static fu2_syscsr_pin_group_t uart_pins[5];

/*==================================================================================================
*                                   PRIVATE FUNCTION PROTOTYPES
==================================================================================================*/

static neon_uart_config_t *uart_config(const neon_uart_handle_t *huart);
static neon_status_t uart_apply_config(neon_uart_handle_t *huart, const neon_uart_config_t *settings);

static uint32_t uart_enter_critical(void);
static void uart_exit_critical(uint32_t primask);
static uint8_t uart_is_valid_instance(const UART_TypeDef *instance);
static uint32_t uart_get_index(const UART_TypeDef *instance);
static neon_crm_peripheral_t uart_get_crm_peripheral(const UART_TypeDef *instance);
static uint8_t uart_is_valid_config(const neon_uart_config_t *config);
static uint8_t uart_map_contains_function(const fu2_syscsr_pin_function_map_t *map, fu2_syscsr_pin_function_t function);
static neon_status_t uart_validate_physical_route(fu2_syscsr_physical_pin_t tx_pin,
                                                  fu2_syscsr_pin_function_t tx_function,
                                                  fu2_syscsr_physical_pin_t rx_pin,
                                                  fu2_syscsr_pin_function_t rx_function);
static neon_status_t uart_validate_pin_route(const neon_uart_handle_t *huart, const neon_uart_config_t *settings);
static neon_status_t uart_configure_two_pins(neon_uart_handle_t *huart, fu2_syscsr_physical_pin_t tx_pin,
                                             fu2_syscsr_pin_function_t tx_function, fu2_syscsr_physical_pin_t rx_pin,
                                             fu2_syscsr_pin_function_t rx_function);
static neon_status_t uart_configure_pin_route(neon_uart_handle_t *huart);
static neon_status_t uart_reset_pin_route(neon_uart_handle_t *huart);
static uint32_t uart_word_length_bits(neon_uart_word_length_t word_length);
static uint32_t uart_parity_bits(neon_uart_parity_t parity);
static uint32_t uart_flow_control_bits(neon_uart_hardware_flow_control_t flow_control);
static uint32_t uart_fifo_bits(const neon_uart_config_t *config);
static neon_status_t uart_set_baud_rate_raw(UART_TypeDef *instance, uint32_t clock_frequency_hz, uint32_t baud_rate,
                                            neon_uart_over_sampling_t over_sampling);
static neon_status_t uart_acquire(neon_uart_handle_t *huart, neon_uart_state_t busy_state);
static void uart_release(neon_uart_handle_t *huart);
static void uart_release_owner(neon_uart_handle_t *huart);
static uint8_t uart_tx_ready(const neon_uart_handle_t *huart);
static uint8_t uart_rx_ready(const neon_uart_handle_t *huart);
static neon_status_t uart_wait_tx_ready(const neon_uart_handle_t *huart, uint32_t *timeout_polls);
static neon_status_t uart_wait_rx_ready(const neon_uart_handle_t *huart, uint32_t *timeout_polls);
static void uart_update_receive_errors(neon_uart_handle_t *huart, uint32_t line_status);
static neon_status_t uart_transmit_core(neon_uart_handle_t *huart, const uint16_t *words, const uint8_t *bytes,
                                        uint16_t size, uint32_t timeout_polls);
static neon_status_t uart_receive_core(neon_uart_handle_t *huart, uint16_t *words, uint8_t *bytes, uint16_t size,
                                       uint32_t timeout_polls);
static neon_status_t uart_clear_fifo(neon_uart_handle_t *huart, uint32_t clear_bit);

/*==================================================================================================
*                                         GLOBAL FUNCTIONS
==================================================================================================*/

/** @copydoc neon_uart_get_default_config */
void neon_uart_get_default_config(neon_uart_config_t *config) {
    if (config == 0) {
        return;
    }

    config->baud_rate = 115200U;
    config->clock_frequency_hz = 0U;
    config->word_length = NEON_UART_WORD_LENGTH_8B;
    config->stop_bits = NEON_UART_STOP_BITS_1;
    config->parity = NEON_UART_PARITY_NONE;
    config->over_sampling = NEON_UART_OVERSAMPLING_16;
    config->fifo_state = NEON_UART_FIFO_DISABLE;
    config->fifo_threshold = NEON_UART_FIFO_THRESHOLD_1_4;
    config->hardware_flow_control = NEON_UART_HWCONTROL_NONE;
    config->pin_config = NEON_UART_PIN_CONFIG_AUTO;
    config->status_source = NEON_UART_STATUS_SOURCE_HARDWARE;
}

/** @copydoc neon_uart_get_state */
neon_uart_state_t neon_uart_get_state(const neon_uart_handle_t *huart) {
    return (huart == 0) ? NEON_UART_STATE_RESET : huart->state;
}

/** @copydoc neon_uart_get_error */
uint32_t neon_uart_get_error(const neon_uart_handle_t *huart) {
    return (huart == 0) ? NEON_UART_ERROR_NONE : huart->error_code;
}

/** @copydoc neon_uart_clear_error */
void neon_uart_clear_error(neon_uart_handle_t *huart) {
    if (huart != 0) {
        huart->error_code = NEON_UART_ERROR_NONE;
        huart->clock_status = 0U;
    }
}

/** @copydoc neon_uart_init */
neon_status_t neon_uart_init(neon_uart_handle_t *huart, neon_uart_t instance) {
    if (huart == NULL || !(uart_is_valid_instance(instance) != 0U)) {
        return NEON_INVALID_PARAM;
    }
    uint32_t saved = uart_enter_critical();
    for (uint32_t index = 0U; index < 5U; ++index) {
        if (fu2_uart_owner[index] == huart) {
            uart_exit_critical(saved);
            return NEON_BUSY;
        }
    }
    if (fu2_uart_owner[uart_get_index(instance)] != NULL) {
        uart_exit_critical(saved);
        return NEON_BUSY;
    }
    memset(huart, 0, sizeof(*huart));
    huart->instance = instance;
    huart->state = NEON_UART_STATE_INITIALIZED;
    uart_exit_critical(saved);
    return NEON_OK;
}

/** @copydoc neon_uart_set_config */
neon_status_t neon_uart_set_config(neon_uart_handle_t *huart, const neon_uart_config_t *config) {
    if (huart == NULL || config == NULL || !(uart_is_valid_instance(huart->instance) != 0U)) {
        return NEON_INVALID_PARAM;
    }
    if (uart_is_valid_config(config) == 0U) {
        return NEON_INVALID_PARAM;
    }
    if (config->fifo_threshold != NEON_UART_FIFO_THRESHOLD_1_4 ||
        (config->word_length == NEON_UART_WORD_LENGTH_5B && config->stop_bits == NEON_UART_STOP_BITS_2)) {
        return NEON_NOT_SUPPORTED;
    }
    neon_status_t route_status = uart_validate_pin_route(huart, config);
    if (route_status != NEON_OK) {
        return route_status;
    }
    if (huart->state == NEON_UART_STATE_READY) {
        neon_status_t status = neon_uart_deinit(huart);
        if (status != NEON_OK) {
            return status;
        }
        huart->state = NEON_UART_STATE_INITIALIZED;
    }
    return uart_apply_config(huart, config);
}

/** @copydoc neon_uart_get_config */
neon_status_t neon_uart_get_config(const neon_uart_handle_t *huart, neon_uart_config_t *config) {
    if (huart == NULL || config == NULL || !(uart_is_valid_instance(huart->instance) != 0U)) {
        return NEON_INVALID_PARAM;
    }
    uint32_t saved = uart_enter_critical();
    neon_status_t status = NEON_ERROR;
    if (fu2_uart_owner[uart_get_index(huart->instance)] == huart) {
        status = NEON_BUSY;
        if (huart->state == NEON_UART_STATE_READY) {
            memcpy(config, uart_config(huart), sizeof(*config));
            status = NEON_OK;
        }
    }
    uart_exit_critical(saved);
    return status;
}

/** @copydoc neon_uart_get_instance */
neon_uart_t neon_uart_get_instance(const neon_uart_handle_t *huart) {
    return huart == NULL ? NULL : huart->instance;
}

/** @copydoc neon_uart_deinit */
neon_status_t neon_uart_deinit(neon_uart_handle_t *huart) {
    if (huart != NULL && huart->state == NEON_UART_STATE_INITIALIZED) {
        huart->state = NEON_UART_STATE_RESET;
        return NEON_OK;
    }

    neon_status_t status;
    neon_status_t route_status;
    uint32_t index;

    if ((huart == 0) || (uart_is_valid_instance((huart == 0) ? 0 : huart->instance) == 0U)) {
        return NEON_INVALID_PARAM;
    }
    if (huart->state == NEON_UART_STATE_RESET) {
        return NEON_OK;
    }
    status = uart_acquire(huart, NEON_UART_STATE_BUSY_CONFIG);
    if (status != NEON_OK) {
        return status;
    }

    huart->instance->LCR &= ~UART_LCR_DLAB_Msk;
    huart->instance->IER_DLM = 0U;

    /* enable FIFO (0), clear counters, then select non-FIFO mode (1). */
    huart->instance->IIR_FCR = 0U;
    huart->instance->IIR_FCR = UART_FCR_RXCLR_Msk | UART_FCR_TXCLR_Msk;
    huart->instance->IIR_FCR = UART_FCR_FIFOEN_Msk;
    huart->instance->LCR = 0U;
    huart->instance->MCR = 0U;
    huart->instance->SCR = 0U;
    huart->instance->MDR = 0U;
    huart->instance->TCR = 0U;
    huart->instance->DET = 0U;
    huart->instance->TAT = 0U;
    huart->instance->DLF = 0U;
    huart->instance->RAR = 0U;
    huart->instance->TAR = 0U;
    huart->instance->LCR_EXT = 0U;

    index = uart_get_index(huart->instance);
    fu2_uart_fcr_shadow[index] = 0U;
    status = neon_crm_peripheral_reset(uart_get_crm_peripheral(huart->instance));
    route_status = uart_reset_pin_route(huart);
    if (status == NEON_OK) {
        status = route_status;
    }

    huart->error_code = NEON_UART_ERROR_NONE;
    huart->clock_status = 0U;
    uart_release_owner(huart);
    return status;
}

/** @copydoc neon_uart_set_baud_rate */
neon_status_t neon_uart_set_baud_rate(neon_uart_handle_t *huart, uint32_t baud_rate) {
    neon_status_t status;
    uint32_t clock_frequency_hz;

    if ((huart == 0) || (uart_is_valid_instance((huart == 0) ? 0 : huart->instance) == 0U) || (baud_rate == 0U) ||
        (baud_rate > 0x00FFFFFFU)) {
        return NEON_INVALID_PARAM;
    }
    status = uart_acquire(huart, NEON_UART_STATE_BUSY_CONFIG);
    if (status != NEON_OK) {
        return status;
    }

    clock_frequency_hz = (*uart_config(huart)).clock_frequency_hz;
    if (clock_frequency_hz == 0U) {
        clock_frequency_hz = neon_crm_get_system_clock_frequency();
    }
    status =
        uart_set_baud_rate_raw(huart->instance, clock_frequency_hz, baud_rate, (*uart_config(huart)).over_sampling);
    if (status == NEON_OK) {
        (*uart_config(huart)).baud_rate = baud_rate;
    }
    uart_release(huart);
    return status;
}

/** @copydoc neon_uart_transmit */
neon_status_t neon_uart_transmit(neon_uart_handle_t *huart, const uint8_t *data, uint16_t size,
                                 uint32_t timeout_polls) {
    if ((huart == 0) || (uart_is_valid_instance((huart == 0) ? 0 : huart->instance) == 0U) ||
        ((data == 0) && (size != 0U))) {
        return NEON_INVALID_PARAM;
    }
    if ((*uart_config(huart)).word_length == NEON_UART_WORD_LENGTH_9B) {
        return NEON_NOT_SUPPORTED;
    }
    return uart_transmit_core(huart, 0, data, size, timeout_polls);
}

/** @copydoc neon_uart_receive */
neon_status_t neon_uart_receive(neon_uart_handle_t *huart, uint8_t *data, uint16_t size, uint32_t timeout_polls) {
    if ((huart == 0) || (uart_is_valid_instance((huart == 0) ? 0 : huart->instance) == 0U) ||
        ((data == 0) && (size != 0U))) {
        return NEON_INVALID_PARAM;
    }
    if ((*uart_config(huart)).word_length == NEON_UART_WORD_LENGTH_9B) {
        return NEON_NOT_SUPPORTED;
    }
    return uart_receive_core(huart, 0, data, size, timeout_polls);
}

/** @copydoc neon_uart_try_receive */
neon_status_t neon_uart_try_receive(neon_uart_handle_t *huart, uint32_t *data) {
    if (huart == NULL || data == NULL || ((uintptr_t)data & 3U) != 0U ||
        uart_is_valid_instance(huart->instance) == 0U) {
        return NEON_INVALID_PARAM;
    }

    /* Serialize only this register snapshot/pop. TX may remain BUSY_TX while
     * interrupted: RX must neither acquire nor release its transfer state. */
    uint32_t saved = uart_enter_critical();
    if (fu2_uart_owner[uart_get_index(huart->instance)] != huart) {
        uart_exit_critical(saved);
        return NEON_ERROR;
    }
    if ((huart->state != NEON_UART_STATE_READY && huart->state != NEON_UART_STATE_BUSY_TX) ||
        (huart->instance->LCR & UART_LCR_DLAB_Msk) != 0U) {
        uart_exit_critical(saved);
        return NEON_BUSY;
    }

    huart->error_code = NEON_UART_ERROR_NONE;
    huart->clock_status = 0U;
    uint32_t ready;
    if (uart_config(huart)->status_source == NEON_UART_STATUS_SOURCE_HARDWARE) {
        /* One LSR snapshot retains errors that a second status read may lose. */
        uint32_t line_status = huart->instance->LSR;
        ready = line_status & UART_LSR_DR_Msk;
        uart_update_receive_errors(huart, line_status);
    } else {
        ready = uart_rx_ready(huart);
    }

    neon_status_t status = NEON_TIMEOUT;
    if (huart->error_code != NEON_UART_ERROR_NONE) {
        if (ready != 0U) {
            (void)huart->instance->RBR_DLL_THR;
        }
        status = NEON_ERROR;
    } else if (ready != 0U) {
        *data = huart->instance->RBR_DLL_THR & UART_RBR_RDATA_Msk;
        status = NEON_OK;
    }
    uart_exit_critical(saved);
    return status;
}

/** @copydoc neon_uart_wait_tx_complete */
neon_status_t neon_uart_wait_tx_complete(neon_uart_handle_t *huart, uint32_t timeout_polls) {
    if (huart == NULL || uart_is_valid_instance(huart->instance) == 0U) {
        return NEON_INVALID_PARAM;
    }
    neon_status_t status = uart_acquire(huart, NEON_UART_STATE_BUSY_TX);
    if (status != NEON_OK) {
        return status;
    }
    while ((huart->instance->LSR & UART_LSR_TEMT_Msk) == 0U) {
        if (timeout_polls == 0U) {
            uart_release(huart);
            return NEON_TIMEOUT;
        }
        --timeout_polls;
    }
    uart_release(huart);
    return NEON_OK;
}

/** @copydoc neon_uart_transmit_words */
neon_status_t neon_uart_transmit_words(neon_uart_handle_t *huart, const uint16_t *data, uint16_t size,
                                       uint32_t timeout_polls) {
    if ((huart == 0) || (uart_is_valid_instance((huart == 0) ? 0 : huart->instance) == 0U) ||
        ((data == 0) && (size != 0U))) {
        return NEON_INVALID_PARAM;
    }
    if ((*uart_config(huart)).word_length != NEON_UART_WORD_LENGTH_9B) {
        return NEON_NOT_SUPPORTED;
    }
    return uart_transmit_core(huart, data, 0, size, timeout_polls);
}

/** @copydoc neon_uart_receive_words */
neon_status_t neon_uart_receive_words(neon_uart_handle_t *huart, uint16_t *data, uint16_t size,
                                      uint32_t timeout_polls) {
    if ((huart == 0) || (uart_is_valid_instance((huart == 0) ? 0 : huart->instance) == 0U) ||
        ((data == 0) && (size != 0U))) {
        return NEON_INVALID_PARAM;
    }
    if ((*uart_config(huart)).word_length != NEON_UART_WORD_LENGTH_9B) {
        return NEON_NOT_SUPPORTED;
    }
    return uart_receive_core(huart, data, 0, size, timeout_polls);
}

/** @copydoc neon_uart_enable_interrupt */
neon_status_t neon_uart_enable_interrupt(neon_uart_handle_t *huart, uint32_t interrupts) {
    uint32_t primask;
    uint32_t ier;
    const uint32_t policy = NEON_UART_INTERRUPT_AUTO_CLEAR | NEON_UART_INTERRUPT_MANUAL_CLEAR;

    if ((huart == 0) || (uart_is_valid_instance((huart == 0) ? 0 : huart->instance) == 0U) ||
        ((interrupts & ~((uint32_t)NEON_UART_INTERRUPT_ALL | policy)) != 0U) || ((interrupts & policy) == policy)) {
        return NEON_INVALID_PARAM;
    }
    if (fu2_uart_owner[uart_get_index(huart->instance)] != huart || huart->state == NEON_UART_STATE_RESET) {
        return NEON_ERROR;
    }

    primask = uart_enter_critical();
    huart->instance->LCR &= ~UART_LCR_DLAB_Msk;
    ier = huart->instance->IER_DLM | (interrupts & NEON_UART_INTERRUPT_ALL);
    if ((interrupts & NEON_UART_INTERRUPT_AUTO_CLEAR) != 0U) {
        ier &= ~UART_IER_AUCLR_Msk;
    }
    if ((interrupts & NEON_UART_INTERRUPT_MANUAL_CLEAR) != 0U) {
        ier |= UART_IER_AUCLR_Msk;
    }
    huart->instance->IER_DLM = ier;
    uart_exit_critical(primask);
    return NEON_OK;
}

/** @copydoc neon_uart_disable_interrupt */
neon_status_t neon_uart_disable_interrupt(neon_uart_handle_t *huart, uint32_t interrupts) {
    uint32_t primask;
    uint32_t ier;
    const uint32_t policy = NEON_UART_INTERRUPT_AUTO_CLEAR | NEON_UART_INTERRUPT_MANUAL_CLEAR;

    if ((huart == 0) || (uart_is_valid_instance((huart == 0) ? 0 : huart->instance) == 0U) ||
        ((interrupts & ~((uint32_t)NEON_UART_INTERRUPT_ALL | policy)) != 0U) || ((interrupts & policy) == policy)) {
        return NEON_INVALID_PARAM;
    }
    if (fu2_uart_owner[uart_get_index(huart->instance)] != huart || huart->state == NEON_UART_STATE_RESET) {
        return NEON_ERROR;
    }

    primask = uart_enter_critical();
    huart->instance->LCR &= ~UART_LCR_DLAB_Msk;
    ier = huart->instance->IER_DLM & ~(interrupts & NEON_UART_INTERRUPT_ALL);
    if ((interrupts & NEON_UART_INTERRUPT_AUTO_CLEAR) != 0U) {
        ier |= UART_IER_AUCLR_Msk;
    }
    if ((interrupts & NEON_UART_INTERRUPT_MANUAL_CLEAR) != 0U) {
        ier &= ~UART_IER_AUCLR_Msk;
    }
    huart->instance->IER_DLM = ier;
    uart_exit_critical(primask);
    return NEON_OK;
}

/** @copydoc neon_uart_get_interrupt_id */
neon_status_t neon_uart_get_interrupt_id(const neon_uart_handle_t *huart, neon_uart_interrupt_id_t *interrupt_id) {
    uint32_t raw_id;
    uint32_t iir;

    if ((huart == 0) || (interrupt_id == 0) || (uart_is_valid_instance((huart == 0) ? 0 : huart->instance) == 0U)) {
        return NEON_INVALID_PARAM;
    }
    if (fu2_uart_owner[uart_get_index(huart->instance)] != huart || huart->state == NEON_UART_STATE_RESET) {
        return NEON_ERROR;
    }

    /* One read also acknowledges manual-clear interrupts. */
    iir = huart->instance->IIR_FCR;
    if ((iir & UART_IIR_IPEND_Msk) == 0U) {
        *interrupt_id = NEON_UART_INTERRUPT_ID_NONE;
        return NEON_OK;
    }
    raw_id = (iir & UART_IIR_INTID_Msk) >> UART_IIR_INTID_Pos;
    switch (raw_id) {
        case NEON_UART_INTERRUPT_ID_NONE:
        case NEON_UART_INTERRUPT_ID_TX_FULL:
        case NEON_UART_INTERRUPT_ID_RX_READY:
        case NEON_UART_INTERRUPT_ID_LINE_ERROR:
        case NEON_UART_INTERRUPT_ID_RX_TIMEOUT:
        case NEON_UART_INTERRUPT_ID_DMA_TX_DONE:
        case NEON_UART_INTERRUPT_ID_DMA_RX_DONE:
            *interrupt_id = (neon_uart_interrupt_id_t)raw_id;
            break;
        default:
            *interrupt_id = NEON_UART_INTERRUPT_ID_UNKNOWN;
            break;
    }
    return NEON_OK;
}

/** @copydoc neon_uart_is_interrupt_pending */
neon_status_t neon_uart_is_interrupt_pending(const neon_uart_handle_t *huart, uint8_t *pending) {
    if ((huart == 0) || (pending == 0) || (uart_is_valid_instance((huart == 0) ? 0 : huart->instance) == 0U)) {
        return NEON_INVALID_PARAM;
    }
    if (fu2_uart_owner[uart_get_index(huart->instance)] != huart || huart->state == NEON_UART_STATE_RESET) {
        return NEON_ERROR;
    }
    const uint32_t result_word = ((huart->instance->IIR_FCR & UART_IIR_IPEND_Msk) != 0U) ? 1U : 0U;
    memcpy(pending, &result_word, 1U);
    return NEON_OK;
}

/** @copydoc neon_uart_clear_rx_fifo */
neon_status_t neon_uart_clear_rx_fifo(neon_uart_handle_t *huart) {
    return uart_clear_fifo(huart, UART_FCR_RXCLR_Msk);
}

/** @copydoc neon_uart_clear_tx_fifo */
neon_status_t neon_uart_clear_tx_fifo(neon_uart_handle_t *huart) {
    return uart_clear_fifo(huart, UART_FCR_TXCLR_Msk);
}

/*==================================================================================================
*                                        PRIVATE FUNCTIONS
==================================================================================================*/

static neon_uart_config_t *uart_config(const neon_uart_handle_t *huart) {
    return &uart_configs[uart_get_index(huart->instance)];
}

static neon_status_t uart_apply_config(neon_uart_handle_t *huart, const neon_uart_config_t *settings) {
    neon_status_t status;
    neon_crm_peripheral_t peripheral;
    uint32_t clock_frequency_hz;
    uint32_t lcr;
    uint32_t index;
    uint32_t primask;

    if ((huart == 0) || (uart_is_valid_instance((huart == 0) ? 0 : huart->instance) == 0U) ||
        (uart_is_valid_config((huart == 0) ? 0 : settings) == 0U)) {
        return NEON_INVALID_PARAM;
    }
    if (settings->fifo_threshold != NEON_UART_FIFO_THRESHOLD_1_4 ||
        (settings->word_length == NEON_UART_WORD_LENGTH_5B && settings->stop_bits == NEON_UART_STOP_BITS_2)) {
        /* rev1p3 has no FIFO threshold field; 5-bit STB=1 generates 1.5 stops. */
        return NEON_NOT_SUPPORTED;
    }

    status = uart_validate_pin_route(huart, settings);
    if (status != NEON_OK) {
        return status;
    }

    index = uart_get_index(huart->instance);
    primask = uart_enter_critical();
    if (huart->state != NEON_UART_STATE_INITIALIZED || fu2_uart_owner[index] != 0) {
        uart_exit_critical(primask);
        return NEON_BUSY;
    }
    fu2_uart_owner[index] = huart;
    memcpy(&uart_configs[index], settings, sizeof(*settings));
    huart->state = NEON_UART_STATE_BUSY_CONFIG;
    uart_exit_critical(primask);
    huart->error_code = NEON_UART_ERROR_NONE;
    huart->clock_status = 0U;
    huart->private_context = 0U;
    peripheral = uart_get_crm_peripheral(huart->instance);

    status = neon_crm_peripheral_clock_enable(peripheral);
    if (status == NEON_OK) {
        status = neon_crm_peripheral_reset(peripheral);
    }
    if (status != NEON_OK) {
        uart_release_owner(huart);
        return status;
    }

    huart->instance->MDR = (settings->over_sampling == NEON_UART_OVERSAMPLING_8) ? UART_MDR_OSM_Msk : 0U;

    clock_frequency_hz = settings->clock_frequency_hz;
    if (clock_frequency_hz == 0U) {
        clock_frequency_hz = neon_crm_get_system_clock_frequency();
    }
    status = uart_set_baud_rate_raw(huart->instance, clock_frequency_hz, settings->baud_rate, settings->over_sampling);
    if (status != NEON_OK) {
        uart_release_owner(huart);
        return status;
    }

    lcr = uart_word_length_bits(settings->word_length) | uart_parity_bits(settings->parity);
    if (settings->stop_bits == NEON_UART_STOP_BITS_2) {
        lcr |= UART_LCR_STB_Msk;
    }
    huart->instance->LCR = lcr;
    huart->instance->LCR_EXT = (settings->word_length == NEON_UART_WORD_LENGTH_9B)
                                   ? (UART_LCR_EXT_DLS_E_Msk | UART_LCR_EXT_TRANSMIT_MODE_Msk)
                                   : 0U;

    index = uart_get_index(huart->instance);
    fu2_uart_fcr_shadow[index] = uart_fifo_bits(settings);
    /* FIFOEN must be zero before programming other FCR bits (Table 12). */
    huart->instance->IIR_FCR = 0U;
    huart->instance->IIR_FCR = fu2_uart_fcr_shadow[index];
    huart->instance->MCR = uart_flow_control_bits(settings->hardware_flow_control);
    huart->instance->IER_DLM = 0U;

    status = uart_configure_pin_route(huart);
    if (status != NEON_OK) {
        (void)neon_crm_peripheral_reset(peripheral);
        (void)uart_reset_pin_route(huart);
        uart_release_owner(huart);
        return status;
    }

    uart_release(huart);
    return NEON_OK;
}

static uint32_t uart_enter_critical(void) {
    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    return primask;
}

static void uart_exit_critical(uint32_t primask) {
    __set_PRIMASK(primask);
}

static uint8_t uart_is_valid_instance(const UART_TypeDef *instance) {
    return ((instance == UART0) || (instance == UART1) || (instance == UART2) || (instance == UART3) ||
            (instance == UART4))
               ? 1U
               : 0U;
}

static uint32_t uart_get_index(const UART_TypeDef *instance) {
    if (instance == UART1) {
        return 1U;
    }
    if (instance == UART2) {
        return 2U;
    }
    if (instance == UART3) {
        return 3U;
    }
    if (instance == UART4) {
        return 4U;
    }
    return 0U;
}

static neon_crm_peripheral_t uart_get_crm_peripheral(const UART_TypeDef *instance) {
    if (instance == UART1) {
        return NEON_CRM_PERIPHERAL_UART1;
    }
    if (instance == UART2) {
        return NEON_CRM_PERIPHERAL_UART2;
    }
    if (instance == UART3) {
        return NEON_CRM_PERIPHERAL_UART3;
    }
    if (instance == UART4) {
        return NEON_CRM_PERIPHERAL_UART4;
    }
    return NEON_CRM_PERIPHERAL_UART0;
}

static uint8_t uart_is_valid_config(const neon_uart_config_t *config) {
    if ((config == 0) || (config->baud_rate == 0U) || (config->baud_rate > 0x00FFFFFFU)) {
        return 0U;
    }

    return ((config->word_length <= NEON_UART_WORD_LENGTH_9B) && (config->stop_bits <= NEON_UART_STOP_BITS_2) &&
            (config->parity <= NEON_UART_PARITY_EVEN) && (config->over_sampling <= NEON_UART_OVERSAMPLING_8) &&
            (config->fifo_state <= NEON_UART_FIFO_ENABLE) &&
            (config->fifo_threshold <= NEON_UART_FIFO_THRESHOLD_FULL) &&
            (config->hardware_flow_control <= NEON_UART_HWCONTROL_RTS_AUTO_FLOW) &&
            (config->pin_config <= NEON_UART_PIN_CONFIG_KEEP) &&
            (config->status_source <= NEON_UART_STATUS_SOURCE_FVP_MSR))
               ? 1U
               : 0U;
}

static uint8_t uart_map_contains_function(const fu2_syscsr_pin_function_map_t *map,
                                          fu2_syscsr_pin_function_t function) {
    uint32_t index;

    for (index = 0U; index < 4U; index++) {
        if (map->mux_function[index] == function) {
            return 1U;
        }
    }
    return 0U;
}

static neon_status_t uart_validate_physical_route(fu2_syscsr_physical_pin_t tx_pin,
                                                  fu2_syscsr_pin_function_t tx_function,
                                                  fu2_syscsr_physical_pin_t rx_pin,
                                                  fu2_syscsr_pin_function_t rx_function) {
    fu2_syscsr_physical_pad_map_t pad_map;
    fu2_syscsr_pin_function_map_t function_map;
    neon_status_t status;

    status = fu2_syscsr_get_physical_pad_map(tx_pin, &pad_map);
    if (status != NEON_OK) {
        return status;
    }
    status = fu2_syscsr_get_pin_function_map(tx_pin, &function_map);
    if ((status != NEON_OK) || (uart_map_contains_function(&function_map, tx_function) == 0U)) {
        return (status == NEON_OK) ? NEON_NOT_MAPPED : status;
    }

    status = fu2_syscsr_get_physical_pad_map(rx_pin, &pad_map);
    if (status != NEON_OK) {
        return status;
    }
    status = fu2_syscsr_get_pin_function_map(rx_pin, &function_map);
    if ((status != NEON_OK) || (uart_map_contains_function(&function_map, rx_function) == 0U)) {
        return (status == NEON_OK) ? NEON_NOT_MAPPED : status;
    }

    return NEON_OK;
}

static neon_status_t uart_validate_pin_route(const neon_uart_handle_t *huart, const neon_uart_config_t *settings) {
    if (settings->pin_config == NEON_UART_PIN_CONFIG_KEEP) {
        return NEON_OK;
    }

    /* Bảng pinmux LL hiện chỉ mô tả TX/RX, không có route RTS/CTS. Không âm
   * thầm bật flow-control khi AUTO vì đường handshake có thể chưa ra pin. */
    if (settings->hardware_flow_control != NEON_UART_HWCONTROL_NONE) {
        return NEON_NOT_MAPPED;
    }

    if (huart->instance == UART0) {
        return uart_validate_physical_route(SYSCSR_PIN_50, SYSCSR_PIN_FUNCTION_UART0_TX, SYSCSR_PIN_51,
                                            SYSCSR_PIN_FUNCTION_UART0_RX);
    }
    if (huart->instance == UART1) {
        return uart_validate_physical_route(SYSCSR_PIN_58, SYSCSR_PIN_FUNCTION_UART1_TX, SYSCSR_PIN_59,
                                            SYSCSR_PIN_FUNCTION_UART1_RX);
    }
    if (huart->instance == UART2) {
        return uart_validate_physical_route(SYSCSR_PIN_12, SYSCSR_PIN_FUNCTION_UART2_TX, SYSCSR_PIN_13,
                                            SYSCSR_PIN_FUNCTION_UART2_RX);
    }

    if (huart->instance == UART3) {
        return uart_validate_physical_route(SYSCSR_PIN_24, SYSCSR_PIN_FUNCTION_UART3_TX, SYSCSR_PIN_25,
                                            SYSCSR_PIN_FUNCTION_UART3_RX);
    }

    if (huart->instance == UART4) {
        return uart_validate_physical_route(SYSCSR_PIN_26, SYSCSR_PIN_FUNCTION_UART4_TX, SYSCSR_PIN_48,
                                            SYSCSR_PIN_FUNCTION_UART4_RX);
    }

    return NEON_NOT_MAPPED;
}

static neon_status_t uart_configure_two_pins(neon_uart_handle_t *huart, fu2_syscsr_physical_pin_t tx_pin,
                                             fu2_syscsr_pin_function_t tx_function, fu2_syscsr_physical_pin_t rx_pin,
                                             fu2_syscsr_pin_function_t rx_function) {
    const fu2_syscsr_pin_route_t routes[] = {{tx_pin, tx_function}, {rx_pin, rx_function}};
    return fu2_syscsr_configure_pins(&uart_pins[uart_get_index(huart->instance)], routes, 2U, 0U);
}

static neon_status_t uart_configure_pin_route(neon_uart_handle_t *huart) {
    neon_status_t status;

    if ((*uart_config(huart)).pin_config == NEON_UART_PIN_CONFIG_KEEP) {
        return NEON_OK;
    }

    huart->private_context |= UART_CONTEXT_PIN_AUTO;
    if (huart->instance == UART0) {
        return uart_configure_two_pins(huart, SYSCSR_PIN_50, SYSCSR_PIN_FUNCTION_UART0_TX, SYSCSR_PIN_51,
                                       SYSCSR_PIN_FUNCTION_UART0_RX);
    }
    if (huart->instance == UART1) {
        return uart_configure_two_pins(huart, SYSCSR_PIN_58, SYSCSR_PIN_FUNCTION_UART1_TX, SYSCSR_PIN_59,
                                       SYSCSR_PIN_FUNCTION_UART1_RX);
    }
    if (huart->instance == UART2) {
        if (neon_syscsr_is_jtag_mux_enabled() != 0U) {
            huart->private_context |= UART_CONTEXT_JTAG_WAS_ENABLED;
        }
        status = neon_syscsr_set_jtag_mux(0U);
        if (status != NEON_OK) {
            return status;
        }
        return uart_configure_two_pins(huart, SYSCSR_PIN_12, SYSCSR_PIN_FUNCTION_UART2_TX, SYSCSR_PIN_13,
                                       SYSCSR_PIN_FUNCTION_UART2_RX);
    }

    if (huart->instance == UART3) {
        return uart_configure_two_pins(huart, SYSCSR_PIN_24, SYSCSR_PIN_FUNCTION_UART3_TX, SYSCSR_PIN_25,
                                       SYSCSR_PIN_FUNCTION_UART3_RX);
    }

    if (huart->instance == UART4) {
        return uart_configure_two_pins(huart, SYSCSR_PIN_26, SYSCSR_PIN_FUNCTION_UART4_TX, SYSCSR_PIN_48,
                                       SYSCSR_PIN_FUNCTION_UART4_RX);
    }

    return NEON_NOT_MAPPED;
}

static neon_status_t uart_reset_pin_route(neon_uart_handle_t *huart) {
    if ((huart->private_context & UART_CONTEXT_PIN_AUTO) != 0U) {
        fu2_syscsr_release_pins(&uart_pins[uart_get_index(huart->instance)]);
        if ((huart->private_context & UART_CONTEXT_JTAG_WAS_ENABLED) != 0U) {
            return neon_syscsr_set_jtag_mux(1U);
        }
    }
    return NEON_OK;
}

static uint32_t uart_word_length_bits(neon_uart_word_length_t word_length) {
    switch (word_length) {
        case NEON_UART_WORD_LENGTH_5B:
            return 0U;
        case NEON_UART_WORD_LENGTH_6B:
            return (1U << UART_LCR_WLS_Pos);
        case NEON_UART_WORD_LENGTH_7B:
            return (2U << UART_LCR_WLS_Pos);
        case NEON_UART_WORD_LENGTH_8B:
        case NEON_UART_WORD_LENGTH_9B:
        default:
            return (3U << UART_LCR_WLS_Pos);
    }
}

static uint32_t uart_parity_bits(neon_uart_parity_t parity) {
    /* Follow rev1p3 Table 14; the EPS prose in Table 13 contradicts it. */
    if (parity == NEON_UART_PARITY_ODD) {
        return UART_LCR_PEN_Msk;
    }
    if (parity == NEON_UART_PARITY_EVEN) {
        return UART_LCR_PEN_Msk | UART_LCR_EPS_Msk;
    }
    return 0U;
}

static uint32_t uart_flow_control_bits(neon_uart_hardware_flow_control_t flow_control) {
    uint32_t bits = 0U;

    /* Table 16: RTS=0 forces the active level; RTS=1 gives hardware control. */
    if ((flow_control == NEON_UART_HWCONTROL_AUTO_FLOW) || (flow_control == NEON_UART_HWCONTROL_RTS_AUTO_FLOW)) {
        bits |= UART_MCR_AFE_Msk | UART_MCR_RTS_Msk;
    }
    return bits;
}

static uint32_t uart_fifo_bits(const neon_uart_config_t *config) {
    /* Despite its name, FIFOEN is active-low in rev1p3 Table 12. */
    return (config->fifo_state == NEON_UART_FIFO_ENABLE) ? 0U : UART_FCR_FIFOEN_Msk;
}

static neon_status_t uart_set_baud_rate_raw(UART_TypeDef *instance, uint32_t clock_frequency_hz, uint32_t baud_rate,
                                            neon_uart_over_sampling_t over_sampling) {
    uint32_t oversampling;
    uint32_t denominator;
    uint32_t divisor;
    uint32_t fraction;
    uint32_t remainder;
    uint32_t lcr;
    uint32_t primask;

    if ((clock_frequency_hz == 0U) || (baud_rate == 0U) || (baud_rate > 0x00FFFFFFU)) {
        return NEON_INVALID_PARAM;
    }

    oversampling = (over_sampling == NEON_UART_OVERSAMPLING_8) ? 8U : 16U;
    denominator = baud_rate * oversampling;
    if ((denominator == 0U) || (clock_frequency_hz < denominator)) {
        return NEON_INVALID_PARAM;
    }

    divisor = clock_frequency_hz / denominator;
    remainder = clock_frequency_hz % denominator;
    fraction = ((remainder * 16U) + (denominator / 2U)) / denominator;
    if (fraction >= 16U) {
        divisor++;
        fraction = 0U;
    }
    if ((divisor == 0U) || (divisor > 0xFFFFU)) {
        return NEON_INVALID_PARAM;
    }

    /* RBR/THR/DLL và IER/DLM dùng chung địa chỉ, nên toàn bộ cửa sổ DLAB phải
   * atomic đối với ISR UART. */
    primask = uart_enter_critical();
    lcr = instance->LCR;
    instance->LCR = lcr | UART_LCR_DLAB_Msk;
    instance->RBR_DLL_THR = divisor & UART_DLL_DLSB_Msk;
    instance->IER_DLM = (divisor >> 8U) & UART_DLM_DLSM_Msk;
    instance->DLF = (fraction << UART_DLF_DLF_Pos) & UART_DLF_DLF_Msk;
    instance->LCR = lcr & ~UART_LCR_DLAB_Msk;
    uart_exit_critical(primask);

    return NEON_OK;
}

static neon_status_t uart_acquire(neon_uart_handle_t *huart, neon_uart_state_t busy_state) {
    uint32_t primask;

    primask = uart_enter_critical();
    if (fu2_uart_owner[uart_get_index(huart->instance)] != huart || huart->state == NEON_UART_STATE_RESET) {
        uart_exit_critical(primask);
        return NEON_ERROR;
    }
    if (huart->state != NEON_UART_STATE_READY) {
        uart_exit_critical(primask);
        return NEON_BUSY;
    }
    huart->state = busy_state;
    uart_exit_critical(primask);
    return NEON_OK;
}

static void uart_release(neon_uart_handle_t *huart) {
    uint32_t primask = uart_enter_critical();
    huart->state = NEON_UART_STATE_READY;
    uart_exit_critical(primask);
}

static void uart_release_owner(neon_uart_handle_t *huart) {
    uint32_t primask = uart_enter_critical();
    fu2_uart_owner[uart_get_index(huart->instance)] = 0;
    huart->private_context = 0U;
    huart->state = NEON_UART_STATE_RESET;
    uart_exit_critical(primask);
}

static uint8_t uart_tx_ready(const neon_uart_handle_t *huart) {
    if ((*uart_config(huart)).status_source == NEON_UART_STATUS_SOURCE_FVP_MSR) {
        return ((huart->instance->MSR & UART_FVP_MSR_TXFF) == 0U) ? 1U : 0U;
    }
    /* Table 17: THRF=1 means full; write only when THRF=0. */
    return ((huart->instance->LSR & UART_LSR_THRF_Msk) == 0U) ? 1U : 0U;
}

static uint8_t uart_rx_ready(const neon_uart_handle_t *huart) {
    if ((*uart_config(huart)).status_source == NEON_UART_STATUS_SOURCE_FVP_MSR) {
        return ((huart->instance->MSR & UART_FVP_MSR_RXFE) == 0U) ? 1U : 0U;
    }
    return ((huart->instance->LSR & UART_LSR_DR_Msk) != 0U) ? 1U : 0U;
}

static neon_status_t uart_wait_tx_ready(const neon_uart_handle_t *huart, uint32_t *timeout_polls) {
    while (uart_tx_ready(huart) == 0U) {
        if (*timeout_polls == 0U) {
            return NEON_TIMEOUT;
        }
        (*timeout_polls)--;
    }
    return NEON_OK;
}

static neon_status_t uart_wait_rx_ready(const neon_uart_handle_t *huart, uint32_t *timeout_polls) {
    while (uart_rx_ready(huart) == 0U) {
        if (*timeout_polls == 0U) {
            return NEON_TIMEOUT;
        }
        (*timeout_polls)--;
    }
    return NEON_OK;
}

static void uart_update_receive_errors(neon_uart_handle_t *huart, uint32_t line_status) {
    uint32_t clock_status;

    if ((line_status & UART_LSR_OE_Msk) != 0U) {
        huart->error_code |= NEON_UART_ERROR_OVERRUN;
    }
    if ((line_status & UART_LSR_PE_Msk) != 0U) {
        huart->error_code |= NEON_UART_ERROR_PARITY;
    }
    if ((line_status & UART_LSR_FE_Msk) != 0U) {
        huart->error_code |= NEON_UART_ERROR_FRAME;
    }
    if ((line_status & UART_LSR_BI_Msk) != 0U) {
        huart->error_code |= NEON_UART_ERROR_BREAK;
    }
    if ((line_status & UART_LSR_RXFIFOERR_Msk) != 0U) {
        huart->error_code |= NEON_UART_ERROR_RX_FIFO;
    }

    /* Theo LL, đọc CLKS sẽ xóa trạng thái lỗi. */
    clock_status = huart->instance->CLKS;
    if ((clock_status & UART_CLKS_CLK_FAILURE_Msk) != 0U) {
        huart->error_code |= NEON_UART_ERROR_CLOCK;
        /* [12:1] is the failed address, [13] is read/write, not error flags. */
        huart->clock_status = clock_status;
    }
}

static neon_status_t uart_transmit_core(neon_uart_handle_t *huart, const uint16_t *words, const uint8_t *bytes,
                                        uint16_t size, uint32_t timeout_polls) {
    neon_status_t status;
    uint16_t index;
    uint32_t data;

    status = uart_acquire(huart, NEON_UART_STATE_BUSY_TX);
    if (status != NEON_OK) {
        return status;
    }

    huart->instance->LCR &= ~UART_LCR_DLAB_Msk;
    for (index = 0U; index < size; index++) {
        status = uart_wait_tx_ready(huart, &timeout_polls);
        if (status != NEON_OK) {
            uart_release(huart);
            return status;
        }
        data = 0U;
        if (words != 0) {
            memcpy(&data, &words[index], sizeof(words[index]));
        } else {
            memcpy(&data, &bytes[index], sizeof(bytes[index]));
        }
        huart->instance->RBR_DLL_THR = data & UART_THR_TDATA_Msk;
    }

    /* All data is queued; wire completion is a separate TEMT check. */
    uart_release(huart);
    return NEON_OK;
}

static neon_status_t uart_receive_core(neon_uart_handle_t *huart, uint16_t *words, uint8_t *bytes, uint16_t size,
                                       uint32_t timeout_polls) {
    const uint32_t line_error_mask =
        UART_LSR_OE_Msk | UART_LSR_PE_Msk | UART_LSR_FE_Msk | UART_LSR_BI_Msk | UART_LSR_RXFIFOERR_Msk;
    neon_status_t status;
    uint16_t index;
    uint32_t line_status;
    uint32_t data;

    status = uart_acquire(huart, NEON_UART_STATE_BUSY_RX);
    if (status != NEON_OK) {
        return status;
    }

    huart->error_code = NEON_UART_ERROR_NONE;
    huart->clock_status = 0U;
    huart->instance->LCR &= ~UART_LCR_DLAB_Msk;
    for (index = 0U; index < size; index++) {
        status = uart_wait_rx_ready(huart, &timeout_polls);
        if (status != NEON_OK) {
            uart_release(huart);
            return status;
        }

        if ((*uart_config(huart)).status_source == NEON_UART_STATUS_SOURCE_HARDWARE) {
            line_status = huart->instance->LSR;
            uart_update_receive_errors(huart, line_status);
            if (((line_status & line_error_mask) != 0U) || (huart->error_code != NEON_UART_ERROR_NONE)) {
                /* Đọc RBR để bỏ phần tử lỗi và nhả FIFO trước khi báo lỗi. */
                (void)huart->instance->RBR_DLL_THR;
                uart_release(huart);
                return NEON_ERROR;
            }
        }

        data = huart->instance->RBR_DLL_THR & UART_RBR_RDATA_Msk;
        if (words != 0) {
            const uint32_t result_word = data;
            memcpy(&words[index], &result_word, 2U);
        } else {
            const uint32_t result_word = data;
            memcpy(&bytes[index], &result_word, 1U);
        }
    }

    uart_release(huart);
    return NEON_OK;
}

static neon_status_t uart_clear_fifo(neon_uart_handle_t *huart, uint32_t clear_bit) {
    neon_status_t status;
    uint32_t index;
    uint32_t shadow;
    uint32_t primask;

    if ((huart == 0) || (uart_is_valid_instance((huart == 0) ? 0 : huart->instance) == 0U)) {
        return NEON_INVALID_PARAM;
    }
    status = uart_acquire(huart, NEON_UART_STATE_BUSY_CONFIG);
    if (status != NEON_OK) {
        return status;
    }

    index = uart_get_index(huart->instance);
    primask = uart_enter_critical();
    shadow = fu2_uart_fcr_shadow[index];
    if ((shadow & UART_FCR_FIFOEN_Msk) != 0U) {
        /* Toggling non-FIFO mode would clear both directions, not just one. */
        uart_exit_critical(primask);
        uart_release(huart);
        return NEON_NOT_SUPPORTED;
    }
    huart->instance->IIR_FCR = shadow | clear_bit;
    huart->instance->IIR_FCR = shadow;
    uart_exit_critical(primask);
    uart_release(huart);
    return NEON_OK;
}
