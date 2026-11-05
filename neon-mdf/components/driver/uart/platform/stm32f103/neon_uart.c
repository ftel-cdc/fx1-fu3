/**
 * @file neon_uart.c
 * @brief Implementation for UART.
 * @details
 * UART polling backend minimum for STM32F103 USART1.
 */

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/

#include "uart/neon_uart.h"
#include <string.h>
#include <stddef.h>

/*==================================================================================================
*                                        DEFINES AND MACROS
==================================================================================================*/
#define UART_RCC_APB2_USART1_RESET (1U << 14)
#define UART_RCC_APB2_IOPA_ENABLE (1U << 2)
#define UART_RCC_APB2_USART1_ENABLE (1U << 14)

#define UART_RCC_CFGR_PPRE2_POSITION 11U
#define UART_RCC_CFGR_PPRE2_MASK 0x7U

#define UART_GPIO_PA9_CONFIGURATION_POSITION 4U
#define UART_GPIO_PA10_CONFIGURATION_POSITION 8U
#define UART_GPIO_CONFIGURATION_MASK 0xFU
#define UART_GPIO_AF_OUTPUT_PP_2MHZ 0xAU
#define UART_GPIO_INPUT_FLOATING 0x4U

#define UART_USART_SR_RXNE (1U << 5)
#define UART_USART_SR_TC (1U << 6)
#define UART_USART_SR_TXE (1U << 7)

#define UART_USART_CR1_RE (1U << 2)
#define UART_USART_CR1_TE (1U << 3)
#define UART_USART_CR1_UE (1U << 13)

/*==================================================================================================
*                                         LOCAL VARIABLES
==================================================================================================*/
extern uint32_t SystemCoreClock;
static neon_uart_handle_t *uart_owner;
static neon_uart_config_t uart_active_config;

/*==================================================================================================
*                                   PRIVATE FUNCTION PROTOTYPES
==================================================================================================*/

static neon_status_t uart_apply_config(neon_uart_handle_t *huart, const neon_uart_config_t *settings);

static uint32_t uart_get_clock_frequency(void);
static uint8_t uart_is_config_supported(const neon_uart_config_t *config);
static neon_status_t uart_set_baud_rate_raw(neon_uart_handle_t *huart, uint32_t baud_rate);
static neon_status_t uart_wait_flag(UART_TypeDef *instance, uint32_t flag, uint32_t *timeout_polls);

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
    }
}

/** @copydoc neon_uart_init */
neon_status_t neon_uart_init(neon_uart_handle_t *huart, neon_uart_t instance) {
    if (huart == NULL || !(instance == USART1)) {
        return NEON_INVALID_PARAM;
    }
    uint32_t saved = __get_PRIMASK();
    __disable_irq();
    if (uart_owner != NULL) {
        __set_PRIMASK(saved);
        return NEON_BUSY;
    }
    memset(huart, 0, sizeof(*huart));
    huart->instance = instance;
    huart->state = NEON_UART_STATE_INITIALIZED;
    __set_PRIMASK(saved);
    return NEON_OK;
}

/** @copydoc neon_uart_set_config */
neon_status_t neon_uart_set_config(neon_uart_handle_t *huart, const neon_uart_config_t *config) {
    if (huart == NULL || config == NULL || !(huart->instance == USART1)) {
        return NEON_INVALID_PARAM;
    }
    if (config->baud_rate == 0U) {
        return NEON_INVALID_PARAM;
    }
    if (uart_is_config_supported(config) == 0U) {
        return NEON_NOT_SUPPORTED;
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
    if (huart == NULL || config == NULL || !(huart->instance == USART1)) {
        return NEON_INVALID_PARAM;
    }
    uint32_t saved = __get_PRIMASK();
    __disable_irq();
    neon_status_t status = NEON_ERROR;
    if (uart_owner == huart) {
        status = NEON_BUSY;
        if (huart->state == NEON_UART_STATE_READY) {
            memcpy(config, &(uart_active_config), sizeof(*config));
            status = NEON_OK;
        }
    }
    __set_PRIMASK(saved);
    return status;
}

/** @copydoc neon_uart_get_instance */
neon_uart_t neon_uart_get_instance(const neon_uart_handle_t *huart) {
    return huart == NULL ? NULL : huart->instance;
}

/** @copydoc neon_uart_deinit */
neon_status_t neon_uart_deinit(neon_uart_handle_t *huart) {
    if ((huart == 0) || (huart->instance != USART1)) {
        return NEON_INVALID_PARAM;
    }
    if (huart->state == NEON_UART_STATE_RESET || huart->state == NEON_UART_STATE_INITIALIZED) {
        huart->state = NEON_UART_STATE_RESET;
        return NEON_OK;
    }
    if ((huart->state == NEON_UART_STATE_BUSY_TX) || (huart->state == NEON_UART_STATE_BUSY_RX) ||
        (huart->state == NEON_UART_STATE_BUSY_CONFIG)) {
        return NEON_BUSY;
    }

    huart->instance->CR1 = 0U;
    RCC->APB2RSTR |= UART_RCC_APB2_USART1_RESET;
    RCC->APB2RSTR &= ~UART_RCC_APB2_USART1_RESET;
    huart->state = NEON_UART_STATE_RESET;
    uart_owner = NULL;
    huart->error_code = NEON_UART_ERROR_NONE;
    return NEON_OK;
}

/** @copydoc neon_uart_set_baud_rate */
neon_status_t neon_uart_set_baud_rate(neon_uart_handle_t *huart, uint32_t baud_rate) {
    neon_status_t status;

    if ((huart == 0) || (huart->instance != USART1)) {
        return NEON_INVALID_PARAM;
    }
    if (huart->state != NEON_UART_STATE_READY) {
        return (huart->state == NEON_UART_STATE_RESET) ? NEON_ERROR : NEON_BUSY;
    }
    huart->state = NEON_UART_STATE_BUSY_CONFIG;
    status = uart_set_baud_rate_raw(huart, baud_rate);
    if (status == NEON_OK) {
        uart_active_config.baud_rate = baud_rate;
    }
    huart->state = NEON_UART_STATE_READY;
    return status;
}

/** @copydoc neon_uart_transmit */
neon_status_t neon_uart_transmit(neon_uart_handle_t *huart, const uint8_t *data, uint16_t size,
                                 uint32_t timeout_polls) {
    uint16_t index;

    if ((huart == 0) || (huart->instance != USART1) || ((data == 0) && (size != 0U))) {
        return NEON_INVALID_PARAM;
    }
    if (huart->state != NEON_UART_STATE_READY) {
        return (huart->state == NEON_UART_STATE_RESET) ? NEON_ERROR : NEON_BUSY;
    }

    huart->state = NEON_UART_STATE_BUSY_TX;
    for (index = 0U; index < size; index++) {
        if (uart_wait_flag(huart->instance, UART_USART_SR_TXE, &timeout_polls) != NEON_OK) {
            huart->state = NEON_UART_STATE_READY;
            return NEON_TIMEOUT;
        }
        huart->instance->DR = data[index];
    }
    if ((size != 0U) && (uart_wait_flag(huart->instance, UART_USART_SR_TC, &timeout_polls) != NEON_OK)) {
        huart->state = NEON_UART_STATE_READY;
        return NEON_TIMEOUT;
    }
    huart->state = NEON_UART_STATE_READY;
    return NEON_OK;
}

/** @copydoc neon_uart_receive */
neon_status_t neon_uart_receive(neon_uart_handle_t *huart, uint8_t *data, uint16_t size, uint32_t timeout_polls) {
    uint16_t index;

    if ((huart == 0) || (huart->instance != USART1) || ((data == 0) && (size != 0U))) {
        return NEON_INVALID_PARAM;
    }
    if (huart->state != NEON_UART_STATE_READY) {
        return (huart->state == NEON_UART_STATE_RESET) ? NEON_ERROR : NEON_BUSY;
    }

    huart->state = NEON_UART_STATE_BUSY_RX;
    for (index = 0U; index < size; index++) {
        if (uart_wait_flag(huart->instance, UART_USART_SR_RXNE, &timeout_polls) != NEON_OK) {
            huart->state = NEON_UART_STATE_READY;
            return NEON_TIMEOUT;
        }
        data[index] = (uint8_t)huart->instance->DR;
    }
    huart->state = NEON_UART_STATE_READY;
    return NEON_OK;
}

/** @copydoc neon_uart_transmit_words * @details
 * The STM32F103 backend currently only holds the 8-N-1 polling function. The extended API still has a
 * clear symbol for the receiver application NEON_NOT_SUPPORTED to see because of the link error.
 */
neon_status_t neon_uart_transmit_words(neon_uart_handle_t *huart, const uint16_t *data, uint16_t size,
                                       uint32_t timeout_polls) {
    (void)huart;
    (void)data;
    (void)size;
    (void)timeout_polls;
    return NEON_NOT_SUPPORTED;
}

/** @copydoc neon_uart_receive_words */
neon_status_t neon_uart_receive_words(neon_uart_handle_t *huart, uint16_t *data, uint16_t size,
                                      uint32_t timeout_polls) {
    (void)huart;
    (void)data;
    (void)size;
    (void)timeout_polls;
    return NEON_NOT_SUPPORTED;
}

/** @copydoc neon_uart_enable_interrupt */
neon_status_t neon_uart_enable_interrupt(neon_uart_handle_t *huart, uint32_t interrupts) {
    (void)huart;
    (void)interrupts;
    return NEON_NOT_SUPPORTED;
}

/** @copydoc neon_uart_disable_interrupt */
neon_status_t neon_uart_disable_interrupt(neon_uart_handle_t *huart, uint32_t interrupts) {
    (void)huart;
    (void)interrupts;
    return NEON_NOT_SUPPORTED;
}

/** @copydoc neon_uart_get_interrupt_id */
neon_status_t neon_uart_get_interrupt_id(const neon_uart_handle_t *huart, neon_uart_interrupt_id_t *interrupt_id) {
    (void)huart;
    (void)interrupt_id;
    return NEON_NOT_SUPPORTED;
}

/** @copydoc neon_uart_is_interrupt_pending */
neon_status_t neon_uart_is_interrupt_pending(const neon_uart_handle_t *huart, uint8_t *pending) {
    (void)huart;
    (void)pending;
    return NEON_NOT_SUPPORTED;
}

/** @copydoc neon_uart_clear_rx_fifo */
neon_status_t neon_uart_clear_rx_fifo(neon_uart_handle_t *huart) {
    (void)huart;
    return NEON_NOT_SUPPORTED;
}

/** @copydoc neon_uart_clear_tx_fifo */
neon_status_t neon_uart_clear_tx_fifo(neon_uart_handle_t *huart) {
    (void)huart;
    return NEON_NOT_SUPPORTED;
}

/*==================================================================================================
*                                        PRIVATE FUNCTIONS
==================================================================================================*/

static neon_status_t uart_apply_config(neon_uart_handle_t *huart, const neon_uart_config_t *settings) {
    neon_status_t status;
    uint32_t gpio_crh;

    if ((huart == 0) || (huart->instance != USART1) || (settings->baud_rate == 0U)) {
        return NEON_INVALID_PARAM;
    }
    if (uart_is_config_supported(settings) == 0U) {
        return NEON_NOT_SUPPORTED;
    }
    if (huart->state != NEON_UART_STATE_INITIALIZED) {
        return NEON_BUSY;
    }

    uint32_t saved = __get_PRIMASK();
    __disable_irq();
    if (uart_owner != NULL) {
        __set_PRIMASK(saved);
        return NEON_BUSY;
    }
    uart_owner = huart;
    memcpy(&uart_active_config, settings, sizeof(*settings));
    huart->state = NEON_UART_STATE_BUSY_CONFIG;
    __set_PRIMASK(saved);
    huart->error_code = NEON_UART_ERROR_NONE;
    huart->private_context = 0U;
    RCC->APB2ENR |= UART_RCC_APB2_USART1_ENABLE;

    if (settings->pin_config == NEON_UART_PIN_CONFIG_AUTO) {
        RCC->APB2ENR |= UART_RCC_APB2_IOPA_ENABLE;
        gpio_crh = GPIOA->CRH;
        gpio_crh &= ~((UART_GPIO_CONFIGURATION_MASK << UART_GPIO_PA9_CONFIGURATION_POSITION) |
                      (UART_GPIO_CONFIGURATION_MASK << UART_GPIO_PA10_CONFIGURATION_POSITION));
        gpio_crh |= (UART_GPIO_AF_OUTPUT_PP_2MHZ << UART_GPIO_PA9_CONFIGURATION_POSITION) |
                    (UART_GPIO_INPUT_FLOATING << UART_GPIO_PA10_CONFIGURATION_POSITION);
        GPIOA->CRH = gpio_crh;
    }

    huart->instance->CR1 = 0U;
    huart->instance->CR2 = 0U;
    huart->instance->CR3 = 0U;
    status = uart_set_baud_rate_raw(huart, settings->baud_rate);
    if (status != NEON_OK) {
        huart->state = NEON_UART_STATE_RESET;
        uart_owner = NULL;
        return status;
    }
    huart->instance->CR1 = UART_USART_CR1_UE | UART_USART_CR1_TE | UART_USART_CR1_RE;
    huart->state = NEON_UART_STATE_READY;
    return NEON_OK;
}

static uint32_t uart_get_clock_frequency(void) {
    static const uint8_t apb_shift[8] = {0U, 0U, 0U, 0U, 1U, 2U, 3U, 4U};
    uint32_t ppre2 = (RCC->CFGR >> UART_RCC_CFGR_PPRE2_POSITION) & UART_RCC_CFGR_PPRE2_MASK;
    return SystemCoreClock >> apb_shift[ppre2];
}

static uint8_t uart_is_config_supported(const neon_uart_config_t *config) {
    return ((config->word_length == NEON_UART_WORD_LENGTH_8B) && (config->stop_bits == NEON_UART_STOP_BITS_1) &&
            (config->parity == NEON_UART_PARITY_NONE) && (config->over_sampling == NEON_UART_OVERSAMPLING_16) &&
            (config->fifo_state == NEON_UART_FIFO_DISABLE) &&
            (config->hardware_flow_control == NEON_UART_HWCONTROL_NONE) &&
            (config->pin_config <= NEON_UART_PIN_CONFIG_KEEP) &&
            (config->status_source == NEON_UART_STATUS_SOURCE_HARDWARE))
               ? 1U
               : 0U;
}

static neon_status_t uart_set_baud_rate_raw(neon_uart_handle_t *huart, uint32_t baud_rate) {
    uint32_t baud_divider;
    uint32_t peripheral_clock = uart_active_config.clock_frequency_hz;

    if (peripheral_clock == 0U) {
        peripheral_clock = uart_get_clock_frequency();
    }
    if (baud_rate == 0U) {
        return NEON_INVALID_PARAM;
    }

    baud_divider = peripheral_clock / baud_rate;
    if ((peripheral_clock % baud_rate) >= ((baud_rate / 2U) + (baud_rate % 2U))) {
        baud_divider++;
    }
    if ((baud_divider < 16U) || (baud_divider > 0xFFFFU)) {
        return NEON_INVALID_PARAM;
    }
    huart->instance->BRR = baud_divider;
    return NEON_OK;
}

static neon_status_t uart_wait_flag(UART_TypeDef *instance, uint32_t flag, uint32_t *timeout_polls) {
    while ((instance->SR & flag) == 0U) {
        if (*timeout_polls == 0U) {
            return NEON_TIMEOUT;
        }
        (*timeout_polls)--;
    }
    return NEON_OK;
}
