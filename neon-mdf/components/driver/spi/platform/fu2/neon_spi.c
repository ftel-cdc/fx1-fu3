/**
 * @file neon_spi.c
 * @brief Implementation for SPI.
 * @details
 * * Backend SPI cho FU2.
 *
 * POS/MASK lấy từ FU2.h sinh bởi FU2.yaml. Logic register bám theo
 * DTI SPI User Guide rev1.1, chapters 4/7/9; lớp HAL bổ sung kiểm tra tham số,
 * quản lý CRM, chống truy cập đồng thời và các transfer polling an toàn.
 */

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/

#include "spi/neon_spi.h"
#include <string.h>

#include "crm/neon_crm.h"
#include "syscsr/platform/fu2/neon_syscsr_internal.h"

#include <stddef.h>

/*==================================================================================================
*                                  STRUCTURES AND OTHER TYPEDEFS
==================================================================================================*/
typedef enum {
    FU2_SPI_TRANSFER_TX = 0,
    FU2_SPI_TRANSFER_RX,
    FU2_SPI_TRANSFER_TX_RX,
} fu2_spi_transfer_t;

/*==================================================================================================
*                                         LOCAL VARIABLES
==================================================================================================*/
static volatile uint32_t fu2_spi_busy[2];
static volatile uint32_t fu2_spi_initialized[2];
static volatile uint32_t fu2_spi_faulted[2];
static fu2_syscsr_pin_group_t spi_pins[2];
static neon_spi_pin_config_t spi_pin_policy[2];
static neon_spi_timing_t spi_timing[2];

/*==================================================================================================
*                                   PRIVATE FUNCTION PROTOTYPES
==================================================================================================*/

static uint8_t spi_is_valid_instance(const SPI_TypeDef *instance);
static uint32_t spi_index(const SPI_TypeDef *instance);
static neon_status_t spi_configure_pins(SPI_TypeDef *instance, neon_spi_mode_t mode, neon_spi_slave_select_t select);
static neon_crm_peripheral_t spi_crm_peripheral(const SPI_TypeDef *instance);
static uint32_t spi_enter_critical(void);
static void spi_exit_critical(uint32_t primask);
static uint8_t spi_is_valid_data_width(neon_spi_data_width_t width);
static uint8_t spi_is_valid_slave_select(neon_spi_slave_select_t slave);
static uint8_t spi_is_valid_fifo_config(const neon_spi_fifo_config_t *config);
static uint8_t spi_is_valid_timing(const neon_spi_timing_t *timing);
static uint8_t spi_timing_fits_clock(const neon_spi_timing_t *timing, uint32_t divider);
static uint8_t spi_is_valid_config(const neon_spi_config_t *config);
static neon_status_t spi_acquire(SPI_TypeDef *instance);
static void spi_release(SPI_TypeDef *instance);
static uint32_t spi_data_width_encoding(neon_spi_data_width_t width);
static uint32_t spi_slave_select_encoding(neon_spi_slave_select_t slave);
static uint32_t spi_fifo_encoding(const volatile neon_spi_fifo_config_t *config);
static uint32_t spi_treg0_encoding(const volatile neon_spi_timing_t *timing);
static uint32_t spi_treg1_encoding(const volatile neon_spi_timing_t *timing, uint32_t divider);
static neon_status_t spi_require_tx_empty(const SPI_TypeDef *instance);
static neon_status_t spi_wait_tx_ready(const SPI_TypeDef *instance, uint32_t *timeout_polls);
static neon_status_t spi_wait_tx_empty(const SPI_TypeDef *instance, uint32_t *timeout_polls);
static neon_status_t spi_wait_rx_ready(const SPI_TypeDef *instance, uint32_t *timeout_polls);
static neon_status_t spi_wait_frame_complete(const SPI_TypeDef *instance, uint32_t received,
                                            uint32_t *timeout_polls);
static neon_status_t spi_end_transfer(SPI_TypeDef *instance, neon_status_t status);
static void spi_drain_rx_fifo(SPI_TypeDef *instance);
static uint32_t spi_current_data_mask(const SPI_TypeDef *instance);
static uint8_t spi_is_byte_mode(const SPI_TypeDef *instance);
static neon_status_t spi_transfer_core(SPI_TypeDef *instance, const uint32_t *tx_words, const uint8_t *tx_bytes,
                                       uint32_t *rx_words, uint8_t *rx_bytes, uint32_t count, uint32_t dummy_data,
                                       uint32_t timeout_polls, fu2_spi_transfer_t transfer, uint32_t byte_api);
static uint8_t spi_is_valid_dma_config(const neon_spi_dma_config_t *config);
static uint32_t spi_dma_burst_encoding(neon_spi_dma_burst_length_t burst);

/*==================================================================================================
*                                         GLOBAL FUNCTIONS
==================================================================================================*/

/** @copydoc neon_spi_get_default_config */
void neon_spi_get_default_config(neon_spi_config_t *config) {
    if (config == NULL) {
        return;
    }

    config->mode = NEON_SPI_MODE_MASTER;
    config->clock_polarity = NEON_SPI_POLARITY_LOW;
    config->clock_phase = NEON_SPI_PHASE_1EDGE;
    config->first_bit = NEON_SPI_FIRSTBIT_MSB;
    config->data_width = NEON_SPI_DATA_WIDTH_8BIT;
    config->clock_divider = 0U;
    config->slave_select = NEON_SPI_SLAVE_SELECT_NONE;
    config->slave_select_mode = NEON_SPI_SS_MODE_NORMAL;
    config->slave_select_force = NEON_SPI_SS_FORCE_HOLD;
    config->slave_select_long = NEON_SPI_SS_LONG_DISABLE;
    config->interrupt_state = NEON_SPI_INTERRUPT_DISABLE;
    config->fifo.tx_full_level = 0xFFU;
    config->fifo.tx_empty_level = 0U;
    config->fifo.rx_full_level = 0xFFU;
    config->fifo.rx_empty_level = 0U;
    config->timing.slave_select_to_clock = 0U;
    config->timing.clock_to_slave_select = 0U;
    config->timing.between_frames = NEON_SPI_FU2_B2B_SCK_CYCLES;
    config->timing.slave_select_timeout = 0U;
    config->timing.between_frames_unit = NEON_SPI_B2B_SCK_CYCLES;
    config->pin_config = NEON_SPI_PIN_CONFIG_AUTO;
}

/** @copydoc neon_spi_init */
neon_status_t neon_spi_init(SPI_TypeDef *instance, const neon_spi_config_t *init) {
    neon_status_t status;
    uint32_t index;
    uint32_t ctrl = 0U;
    uint32_t primask;

    if ((spi_is_valid_instance(instance) == 0U) || (spi_is_valid_config(init) == 0U)) {
        return NEON_INVALID_PARAM;
    }
    if (init->pin_config == NEON_SPI_PIN_CONFIG_AUTO && init->slave_select > NEON_SPI_SLAVE_SELECT_0) {
        return NEON_NOT_MAPPED;
    }

    index = spi_index(instance);
    primask = spi_enter_critical();
    if (fu2_spi_busy[index] != 0U) {
        spi_exit_critical(primask);
        return NEON_BUSY;
    }
    if ((fu2_spi_initialized[index] != 0U) && (fu2_spi_faulted[index] == 0U) &&
        (spi_require_tx_empty(instance) != NEON_OK)) {
        spi_exit_critical(primask);
        return NEON_BUSY;
    }
    fu2_spi_busy[index] = 1U;
    fu2_spi_initialized[index] = 0U;
    spi_exit_critical(primask);

    status = neon_crm_peripheral_clock_enable(spi_crm_peripheral(instance));
    if (status == NEON_OK) {
        status = neon_crm_peripheral_reset(spi_crm_peripheral(instance));
        if (status != NEON_OK) {
            (void)neon_crm_peripheral_clock_disable(spi_crm_peripheral(instance));
        }
    }
    if (status != NEON_OK) {
        fu2_syscsr_release_pins(&spi_pins[index]);
        primask = spi_enter_critical();
        fu2_spi_busy[index] = 0U;
        spi_exit_critical(primask);
        return status;
    }

    if (init->pin_config == NEON_SPI_PIN_CONFIG_AUTO) {
        status = spi_configure_pins(instance, init->mode, init->slave_select);
    } else {
        fu2_syscsr_release_pins(&spi_pins[index]);
    }
    if (status != NEON_OK) {
        (void)neon_crm_peripheral_clock_disable(spi_crm_peripheral(instance));
        fu2_syscsr_release_pins(&spi_pins[index]);
        primask = spi_enter_critical();
        fu2_spi_busy[index] = 0U;
        spi_exit_critical(primask);
        return status;
    }
    spi_pin_policy[index] = init->pin_config;

    if (init->mode == NEON_SPI_MODE_MASTER) {
        ctrl |= 1U << SPI_CTRL_SPI_MODE_Pos;
    }
    if (init->clock_polarity == NEON_SPI_POLARITY_HIGH) {
        ctrl |= SPI_CTRL_CPOL_Msk;
    }
    if (init->clock_phase == NEON_SPI_PHASE_2EDGE) {
        ctrl |= SPI_CTRL_CPHA_Msk;
    }
    if (init->first_bit == NEON_SPI_FIRSTBIT_LSB) {
        ctrl |= SPI_CTRL_LSB_Msk;
    }
    if (init->interrupt_state == NEON_SPI_INTERRUPT_ENABLE) {
        ctrl |= SPI_CTRL_INT_EN_Msk;
    }
    if (init->slave_select_mode == NEON_SPI_SS_MODE_MANUAL) {
        ctrl |= SPI_CTRL_SS_MODE_Msk;
    }
    if (init->slave_select_force == NEON_SPI_SS_FORCE_RELEASE) {
        ctrl |= SPI_CTRL_SS_FORCE_Msk;
    }
    if (init->slave_select_long == NEON_SPI_SS_LONG_ENABLE) {
        ctrl |= SPI_CTRL_SS_LONG_Msk;
    }

    instance->FIFO = spi_fifo_encoding(&init->fifo);
    instance->CTRL = ctrl;
    instance->CLK = (((const volatile neon_spi_config_t *)init)->clock_divider << SPI_CLK_DIV_Pos) & SPI_CLK_DIV_Msk;
    instance->CMDCONF =
        spi_data_width_encoding(init->data_width) | SPI_CMDCONF_RD_Msk | spi_slave_select_encoding(init->slave_select);
    instance->TREG0 = spi_treg0_encoding(&init->timing);
    instance->TREG1 = spi_treg1_encoding(&init->timing, init->clock_divider);
    spi_timing[index] = init->timing;
    instance->CTRL_RX = 0U;
    instance->CTRL_TX = 0U;

    primask = spi_enter_critical();
    fu2_spi_initialized[index] = 1U;
    fu2_spi_faulted[index] = 0U;
    fu2_spi_busy[index] = 0U;
    spi_exit_critical(primask);
    return NEON_OK;
}

/** @copydoc neon_spi_deinit */
neon_status_t neon_spi_deinit(SPI_TypeDef *instance) {
    neon_status_t status;
    neon_status_t clock_status;
    uint32_t index;
    uint32_t primask;

    if (spi_is_valid_instance(instance) == 0U) {
        return NEON_INVALID_PARAM;
    }
    index = spi_index(instance);
    primask = spi_enter_critical();
    if (fu2_spi_busy[index] != 0U) {
        spi_exit_critical(primask);
        return NEON_BUSY;
    }
    if (fu2_spi_initialized[index] == 0U) {
        spi_exit_critical(primask);
        return NEON_OK;
    }
    fu2_spi_busy[index] = 1U;
    spi_exit_critical(primask);

    instance->CTRL &= ~(SPI_CTRL_INT_EN_Msk | SPI_CTRL_DMA_ENABLE_Msk);
    instance->CTRL_RX = 0U;
    instance->CTRL_TX = 0U;
    status = neon_crm_peripheral_reset(spi_crm_peripheral(instance));
    fu2_syscsr_release_pins(&spi_pins[index]);
    clock_status = neon_crm_peripheral_clock_disable(spi_crm_peripheral(instance));
    if (status == NEON_OK) {
        status = clock_status;
    }

    primask = spi_enter_critical();
    fu2_spi_initialized[index] = 0U;
    fu2_spi_faulted[index] = 0U;
    fu2_spi_busy[index] = 0U;
    spi_exit_critical(primask);
    return status;
}

/** @copydoc neon_spi_set_clock_divider */
neon_status_t neon_spi_set_clock_divider(SPI_TypeDef *instance, uint32_t divider) {
    neon_status_t status;

    if (divider > (SPI_CLK_DIV_Msk >> SPI_CLK_DIV_Pos)) {
        return NEON_INVALID_PARAM;
    }
    status = spi_acquire(instance);
    if (status != NEON_OK) {
        return status;
    }
    status = spi_require_tx_empty(instance);
    if (status == NEON_OK) {
        const neon_spi_timing_t *timing = &spi_timing[spi_index(instance)];
        if (spi_timing_fits_clock(timing, divider) == 0U) {
            status = NEON_INVALID_PARAM;
        } else {
            instance->CLK = (instance->CLK & ~SPI_CLK_DIV_Msk) | ((divider << SPI_CLK_DIV_Pos) & SPI_CLK_DIV_Msk);
            instance->TREG1 = spi_treg1_encoding(timing, divider);
        }
    }
    spi_release(instance);
    return status;
}

/** @copydoc neon_spi_get_clock_frequency */
neon_status_t neon_spi_get_clock_frequency(SPI_TypeDef *instance, uint32_t *frequency) {
    uint32_t divider;

    if ((spi_is_valid_instance(instance) == 0U) || (frequency == NULL)) {
        return NEON_INVALID_PARAM;
    }
    if (fu2_spi_initialized[spi_index(instance)] == 0U) {
        return NEON_ERROR;
    }
    if ((instance->CTRL & SPI_CTRL_SPI_MODE_Msk) != (1U << SPI_CTRL_SPI_MODE_Pos)) {
        return NEON_NOT_SUPPORTED;
    }
    divider = (instance->CLK & SPI_CLK_DIV_Msk) >> SPI_CLK_DIV_Pos;
    *frequency = neon_crm_get_system_clock_frequency() / (2U * (divider + 1U));
    return NEON_OK;
}

/** @copydoc neon_spi_set_data_width */
neon_status_t neon_spi_set_data_width(SPI_TypeDef *instance, neon_spi_data_width_t width) {
    if (spi_is_valid_data_width(width) == 0U) {
        return NEON_INVALID_PARAM;
    }
    neon_status_t status = spi_acquire(instance);
    if (status != NEON_OK) {
        return status;
    }
    uint32_t mode = (instance->CTRL & SPI_CTRL_SPI_MODE_Msk) >> SPI_CTRL_SPI_MODE_Pos;
    if (mode > 1U || (mode == 0U && width != NEON_SPI_DATA_WIDTH_8BIT)) {
        spi_release(instance);
        return NEON_NOT_SUPPORTED;
    }
    status = spi_require_tx_empty(instance);
    if (status == NEON_OK) {
        instance->CMDCONF = (instance->CMDCONF & ~SPI_CMDCONF_LEN_Msk) | spi_data_width_encoding(width);
    }
    spi_release(instance);
    return status;
}

/** @copydoc neon_spi_set_slave_select */
neon_status_t neon_spi_set_slave_select(SPI_TypeDef *instance, neon_spi_slave_select_t slave) {
    neon_status_t status;

    if (spi_is_valid_slave_select(slave) == 0U) {
        return NEON_INVALID_PARAM;
    }
    status = spi_acquire(instance);
    if (status != NEON_OK) {
        return status;
    }
    status = spi_require_tx_empty(instance);
    if (status == NEON_OK && spi_pin_policy[spi_index(instance)] == NEON_SPI_PIN_CONFIG_AUTO) {
        if (slave > NEON_SPI_SLAVE_SELECT_0) {
            status = NEON_NOT_MAPPED;
        } else {
            neon_spi_mode_t mode = (instance->CTRL & SPI_CTRL_SPI_MODE_Msk) == 1U
                                       ? NEON_SPI_MODE_MASTER : NEON_SPI_MODE_SLAVE;
            status = spi_configure_pins(instance, mode, slave);
        }
    }
    if (status == NEON_OK) {
        instance->CMDCONF = (instance->CMDCONF & ~SPI_CMDCONF_SLAVE_EN_Msk) | spi_slave_select_encoding(slave);
    }
    spi_release(instance);
    return status;
}

/** @copydoc neon_spi_set_receive_capture */
neon_status_t neon_spi_set_receive_capture(SPI_TypeDef *instance, uint8_t enable) {
    neon_status_t status;

    if (enable > 1U) {
        return NEON_INVALID_PARAM;
    }
    status = spi_acquire(instance);
    if (status != NEON_OK) {
        return status;
    }
    status = spi_require_tx_empty(instance);
    if (status == NEON_OK) {
        if (enable != 0U) {
            instance->CMDCONF |= SPI_CMDCONF_RD_Msk;
        } else {
            instance->CMDCONF &= ~SPI_CMDCONF_RD_Msk;
        }
    }
    spi_release(instance);
    return status;
}

/** @copydoc neon_spi_set_slave_select_control */
neon_status_t neon_spi_set_slave_select_control(SPI_TypeDef *instance, neon_spi_slave_select_mode_t mode,
                                                neon_spi_slave_select_force_t force,
                                                neon_spi_slave_select_long_t long_access) {
    neon_status_t status;
    uint32_t value = 0U;
    uint32_t mask = SPI_CTRL_SS_MODE_Msk | SPI_CTRL_SS_FORCE_Msk | SPI_CTRL_SS_LONG_Msk;

    if (((mode != NEON_SPI_SS_MODE_NORMAL) && (mode != NEON_SPI_SS_MODE_MANUAL)) ||
        ((force != NEON_SPI_SS_FORCE_HOLD) && (force != NEON_SPI_SS_FORCE_RELEASE)) ||
        ((long_access != NEON_SPI_SS_LONG_DISABLE) && (long_access != NEON_SPI_SS_LONG_ENABLE))) {
        return NEON_INVALID_PARAM;
    }
    status = spi_acquire(instance);
    if (status != NEON_OK) {
        return status;
    }
    if (mode == NEON_SPI_SS_MODE_MANUAL) {
        value |= SPI_CTRL_SS_MODE_Msk;
    }
    if (force == NEON_SPI_SS_FORCE_RELEASE) {
        value |= SPI_CTRL_SS_FORCE_Msk;
    }
    if (long_access == NEON_SPI_SS_LONG_ENABLE) {
        value |= SPI_CTRL_SS_LONG_Msk;
    }
    uint32_t ctrl = instance->CTRL;
    /* Section 4.4 allows SS_FORCE to change while a transaction/DMA is active.
     * Changes to SS_MODE or SS_LONG still require the controller to be idle. */
    if (((ctrl ^ value) & (SPI_CTRL_SS_MODE_Msk | SPI_CTRL_SS_LONG_Msk)) != 0U) {
        status = spi_require_tx_empty(instance);
    }
    if (status == NEON_OK) {
        instance->CTRL = (ctrl & ~mask) | value;
    }
    spi_release(instance);
    return status;
}

/** @copydoc neon_spi_set_fifo_config */
neon_status_t neon_spi_set_fifo_config(SPI_TypeDef *instance, const neon_spi_fifo_config_t *config) {
    neon_status_t status;

    if (spi_is_valid_fifo_config(config) == 0U) {
        return NEON_INVALID_PARAM;
    }
    status = spi_acquire(instance);
    if (status != NEON_OK) {
        return status;
    }
    status = spi_require_tx_empty(instance);
    if (status == NEON_OK) {
        instance->FIFO = spi_fifo_encoding(config);
    }
    spi_release(instance);
    return status;
}

/** @copydoc neon_spi_set_timing */
neon_status_t neon_spi_set_timing(SPI_TypeDef *instance, const neon_spi_timing_t *timing) {
    neon_status_t status;

    if (spi_is_valid_timing(timing) == 0U) {
        return NEON_INVALID_PARAM;
    }
    status = spi_acquire(instance);
    if (status != NEON_OK) {
        return status;
    }
    status = spi_require_tx_empty(instance);
    if (status == NEON_OK) {
        uint32_t divider = (instance->CLK & SPI_CLK_DIV_Msk) >> SPI_CLK_DIV_Pos;
        if (spi_timing_fits_clock(timing, divider) == 0U) {
            status = NEON_INVALID_PARAM;
        } else {
            instance->TREG0 = spi_treg0_encoding(timing);
            instance->TREG1 = spi_treg1_encoding(timing, divider);
            spi_timing[spi_index(instance)] = *timing;
        }
    }
    spi_release(instance);
    return status;
}

/** @copydoc neon_spi_transmit */
neon_status_t neon_spi_transmit(SPI_TypeDef *instance, const uint8_t *data, uint16_t size, uint32_t timeout_polls) {
    return spi_transfer_core(instance, NULL, data, NULL, NULL, size, 0U, timeout_polls, FU2_SPI_TRANSFER_TX, 1U);
}

/** @copydoc neon_spi_receive */
neon_status_t neon_spi_receive(SPI_TypeDef *instance, uint8_t *data, uint16_t size, uint32_t timeout_polls) {
    return spi_transfer_core(instance, NULL, NULL, NULL, data, size, 0xFFU, timeout_polls, FU2_SPI_TRANSFER_RX, 1U);
}

/** @copydoc neon_spi_transmit_receive */
neon_status_t neon_spi_transmit_receive(SPI_TypeDef *instance, const uint8_t *tx_data, uint8_t *rx_data, uint16_t size,
                                        uint32_t timeout_polls) {
    return spi_transfer_core(instance, NULL, tx_data, NULL, rx_data, size, 0U, timeout_polls, FU2_SPI_TRANSFER_TX_RX,
                             1U);
}

/** @copydoc neon_spi_transmit_words */
neon_status_t neon_spi_transmit_words(SPI_TypeDef *instance, const uint32_t *data, uint16_t count,
                                      uint32_t timeout_polls) {
    return spi_transfer_core(instance, data, NULL, NULL, NULL, count, 0U, timeout_polls, FU2_SPI_TRANSFER_TX, 0U);
}

/** @copydoc neon_spi_receive_words */
neon_status_t neon_spi_receive_words(SPI_TypeDef *instance, uint32_t *data, uint16_t count, uint32_t dummy_data,
                                     uint32_t timeout_polls) {
    return spi_transfer_core(instance, NULL, NULL, data, NULL, count, dummy_data, timeout_polls, FU2_SPI_TRANSFER_RX,
                             0U);
}

/** @copydoc neon_spi_transmit_receive_words */
neon_status_t neon_spi_transmit_receive_words(SPI_TypeDef *instance, const uint32_t *tx_data, uint32_t *rx_data,
                                              uint16_t count, uint32_t timeout_polls) {
    return spi_transfer_core(instance, tx_data, NULL, rx_data, NULL, count, 0U, timeout_polls, FU2_SPI_TRANSFER_TX_RX,
                             0U);
}

/** @copydoc neon_spi_enable_interrupt */
neon_status_t neon_spi_enable_interrupt(SPI_TypeDef *instance) {
    neon_status_t status = spi_acquire(instance);

    if (status == NEON_OK) {
        instance->CTRL |= SPI_CTRL_INT_EN_Msk;
        spi_release(instance);
    }
    return status;
}

/** @copydoc neon_spi_disable_interrupt */
neon_status_t neon_spi_disable_interrupt(SPI_TypeDef *instance) {
    neon_status_t status = spi_acquire(instance);

    if (status == NEON_OK) {
        instance->CTRL &= ~SPI_CTRL_INT_EN_Msk;
        spi_release(instance);
    }
    return status;
}

/** @copydoc neon_spi_is_interrupt_enabled */
neon_status_t neon_spi_is_interrupt_enabled(SPI_TypeDef *instance, uint8_t *enabled) {
    if ((spi_is_valid_instance(instance) == 0U) || (enabled == NULL)) {
        return NEON_INVALID_PARAM;
    }
    if (fu2_spi_initialized[spi_index(instance)] == 0U) {
        return NEON_ERROR;
    }
    const uint32_t result_word = ((instance->CTRL & SPI_CTRL_INT_EN_Msk) != 0U) ? 1U : 0U;
    memcpy(enabled, &result_word, 1U);
    return NEON_OK;
}

/** @copydoc neon_spi_get_active_flags */
neon_status_t neon_spi_get_active_flags(SPI_TypeDef *instance, uint32_t *flags) {
    if ((spi_is_valid_instance(instance) == 0U) || (flags == NULL)) {
        return NEON_INVALID_PARAM;
    }
    if (fu2_spi_initialized[spi_index(instance)] == 0U) {
        return NEON_ERROR;
    }
    *flags = instance->STT & (uint32_t)NEON_SPI_FLAG_ALL;
    return NEON_OK;
}

/** @copydoc neon_spi_is_active_flag */
neon_status_t neon_spi_is_active_flag(SPI_TypeDef *instance, uint32_t flags, uint8_t *active) {
    uint32_t current;
    neon_status_t status;

    if ((flags == 0U) || ((flags & ~(uint32_t)NEON_SPI_FLAG_ALL) != 0U) || (active == NULL)) {
        return NEON_INVALID_PARAM;
    }
    status = neon_spi_get_active_flags(instance, &current);
    if (status == NEON_OK) {
        const uint32_t result_word = ((current & flags) == flags) ? 1U : 0U;
        memcpy(active, &result_word, 1U);
    }
    return status;
}

/** @copydoc neon_spi_get_clock_status */
neon_status_t neon_spi_get_clock_status(SPI_TypeDef *instance, neon_spi_clock_status_t *status) {
    uint32_t value;

    if ((spi_is_valid_instance(instance) == 0U) || (status == NULL)) {
        return NEON_INVALID_PARAM;
    }
    if (fu2_spi_initialized[spi_index(instance)] == 0U) {
        return NEON_ERROR;
    }
    value = instance->CLKS;
    status->clock_failure = ((value & SPI_CLKS_CLK_FAILURE_Msk) != 0U) ? 1U : 0U;
    status->failure_address = (uint8_t)((value & SPI_CLKS_ADDR_FAILURE_Msk) >> SPI_CLKS_ADDR_FAILURE_Pos);
    status->failed_command =
        ((value & SPI_CLKS_CMD_FAILURE_Msk) != 0U) ? NEON_SPI_FAILED_COMMAND_READ : NEON_SPI_FAILED_COMMAND_WRITE;
    return NEON_OK;
}

/** @copydoc neon_spi_configure_dma */
neon_status_t neon_spi_configure_dma(SPI_TypeDef *instance, neon_spi_dma_direction_t direction,
                                     const neon_spi_dma_config_t *config) {
    neon_status_t status;
    uint32_t value;
    uint32_t burst;
    uint32_t threshold;
    uint32_t channel_control;
    uint32_t done_mask;

    if (((direction != NEON_SPI_DMA_RX) && (direction != NEON_SPI_DMA_TX)) || (spi_is_valid_dma_config(config) == 0U)) {
        return NEON_INVALID_PARAM;
    }
    if ((config->state == NEON_SPI_DMA_ENABLE) && (config->transfer_mode == NEON_SPI_DMA_BURST) &&
        ((uint32_t)config->burst_length > NEON_SPI_FIFO_DEPTH)) {
        return NEON_NOT_SUPPORTED;
    }

    status = spi_acquire(instance);
    if (status != NEON_OK) {
        return status;
    }

    /* Disable is an abort request: it must work while FIFO/DMA are busy. */
    if (config->state == NEON_SPI_DMA_DISABLE) {
        if (direction == NEON_SPI_DMA_RX) {
            instance->CTRL_RX &= ~SPI_CTRL_RX_ENABLE_Msk;
        } else {
            instance->CTRL_TX &= ~SPI_CTRL_TX_ENABLE_Msk;
        }
        if (((instance->CTRL_RX & SPI_CTRL_RX_ENABLE_Msk) == 0U) &&
            ((instance->CTRL_TX & SPI_CTRL_TX_ENABLE_Msk) == 0U)) {
            instance->CTRL &= ~SPI_CTRL_DMA_ENABLE_Msk;
        }
        spi_release(instance);
        return NEON_OK;
    }
    /* Allow the other direction to be armed for full-duplex DMA. */
    if ((instance->STT & SPI_STT_INT_TXFF_EMPTY_Msk) == 0U) {
        spi_release(instance);
        return NEON_BUSY;
    }

    burst = spi_dma_burst_encoding(config->burst_length);
    if (direction == NEON_SPI_DMA_RX) {
        channel_control = instance->CTRL_RX;
        done_mask = SPI_STT_DMA_TRAN_DONE_RX_Msk;
        if (((channel_control & SPI_CTRL_RX_ENABLE_Msk) != 0U) && ((instance->STT_DMA & done_mask) == 0U)) {
            spi_release(instance);
            return NEON_BUSY;
        }
        instance->CTRL_RX = channel_control & ~SPI_CTRL_RX_ENABLE_Msk;
        value = (burst << SPI_CTRL_RX_MAX_BURST_LEN_Pos) & SPI_CTRL_RX_MAX_BURST_LEN_Msk;
        if (config->transfer_mode == NEON_SPI_DMA_BURST) {
            value |= SPI_CTRL_RX_BURST_MODE_Msk;
        }
        value |= (config->total_bytes << SPI_CTRL_RX_TOTAL_BYTES_Pos) & SPI_CTRL_RX_TOTAL_BYTES_Msk;
        if (config->state == NEON_SPI_DMA_ENABLE) {
            value |= SPI_CTRL_RX_ENABLE_Msk;
        }
        if (config->transfer_mode == NEON_SPI_DMA_BURST) {
            threshold = (uint32_t)config->burst_length - 1U;
            instance->FIFO = (instance->FIFO & ~SPI_FIFO_RXFF_PROG_FLVL_Msk) |
                             ((threshold << SPI_FIFO_RXFF_PROG_FLVL_Pos) & SPI_FIFO_RXFF_PROG_FLVL_Msk);
        }
        /* Rev1.1 section 7.1: configure, clear the previous one-shot state,
         * then arm. Keep this channel disabled during CLEAR, since the other
         * direction may already have enabled DMA globally. */
        instance->CTRL_RX = value & ~SPI_CTRL_RX_ENABLE_Msk;
        instance->CTRL_RX = (value & ~SPI_CTRL_RX_ENABLE_Msk) | SPI_CTRL_RX_CLEAR_Msk;
        instance->CTRL_RX = value;
    } else {
        channel_control = instance->CTRL_TX;
        done_mask = SPI_STT_DMA_TRAN_DONE_TX_Msk;
        if (((channel_control & SPI_CTRL_TX_ENABLE_Msk) != 0U) && ((instance->STT_DMA & done_mask) == 0U)) {
            spi_release(instance);
            return NEON_BUSY;
        }
        instance->CTRL_TX = channel_control & ~SPI_CTRL_TX_ENABLE_Msk;
        value = (burst << SPI_CTRL_TX_MAX_BURST_LEN_Pos) & SPI_CTRL_TX_MAX_BURST_LEN_Msk;
        if (config->transfer_mode == NEON_SPI_DMA_BURST) {
            value |= SPI_CTRL_TX_BURST_MODE_Msk;
        }
        value |= (config->total_bytes << SPI_CTRL_TX_TOTAL_BYTES_Pos) & SPI_CTRL_TX_TOTAL_BYTES_Msk;
        if (config->state == NEON_SPI_DMA_ENABLE) {
            value |= SPI_CTRL_TX_ENABLE_Msk;
        }
        if (config->transfer_mode == NEON_SPI_DMA_BURST) {
            threshold = NEON_SPI_FIFO_DEPTH - (uint32_t)config->burst_length + 1U;
            instance->FIFO = (instance->FIFO & ~SPI_FIFO_TXFF_PROG_ELVL_Msk) |
                             ((threshold << SPI_FIFO_TXFF_PROG_ELVL_Pos) & SPI_FIFO_TXFF_PROG_ELVL_Msk);
        }
        instance->CTRL_TX = value & ~SPI_CTRL_TX_ENABLE_Msk;
        instance->CTRL_TX = (value & ~SPI_CTRL_TX_ENABLE_Msk) | SPI_CTRL_TX_CLEAR_Msk;
        instance->CTRL_TX = value;
    }

    if (config->state == NEON_SPI_DMA_ENABLE) {
        instance->CTRL |= SPI_CTRL_DMA_ENABLE_Msk;
    }

    spi_release(instance);
    return NEON_OK;
}

/** @copydoc neon_spi_enable_dma */
neon_status_t neon_spi_enable_dma(SPI_TypeDef *instance) {
    neon_status_t status = spi_acquire(instance);

    if (status == NEON_OK) {
        instance->CTRL |= SPI_CTRL_DMA_ENABLE_Msk;
        spi_release(instance);
    }
    return status;
}

/** @copydoc neon_spi_disable_dma */
neon_status_t neon_spi_disable_dma(SPI_TypeDef *instance) {
    neon_status_t status = spi_acquire(instance);

    if (status == NEON_OK) {
        instance->CTRL &= ~SPI_CTRL_DMA_ENABLE_Msk;
        instance->CTRL_RX &= ~SPI_CTRL_RX_ENABLE_Msk;
        instance->CTRL_TX &= ~SPI_CTRL_TX_ENABLE_Msk;
        spi_release(instance);
    }
    return status;
}

/** @copydoc neon_spi_is_dma_enabled */
neon_status_t neon_spi_is_dma_enabled(SPI_TypeDef *instance, uint8_t *enabled) {
    if ((spi_is_valid_instance(instance) == 0U) || (enabled == NULL)) {
        return NEON_INVALID_PARAM;
    }
    if (fu2_spi_initialized[spi_index(instance)] == 0U) {
        return NEON_ERROR;
    }
    const uint32_t result_word = ((instance->CTRL & SPI_CTRL_DMA_ENABLE_Msk) != 0U) ? 1U : 0U;
    memcpy(enabled, &result_word, 1U);
    return NEON_OK;
}

/** @copydoc neon_spi_is_dma_complete */
neon_status_t neon_spi_is_dma_complete(SPI_TypeDef *instance, neon_spi_dma_direction_t direction, uint8_t *complete) {
    uint32_t mask;

    if ((spi_is_valid_instance(instance) == 0U) || ((direction != NEON_SPI_DMA_RX) && (direction != NEON_SPI_DMA_TX)) ||
        (complete == NULL)) {
        return NEON_INVALID_PARAM;
    }
    if (fu2_spi_initialized[spi_index(instance)] == 0U) {
        return NEON_ERROR;
    }
    mask = (direction == NEON_SPI_DMA_RX) ? SPI_STT_DMA_TRAN_DONE_RX_Msk : SPI_STT_DMA_TRAN_DONE_TX_Msk;
    const uint32_t result_word = ((instance->STT_DMA & mask) != 0U) ? 1U : 0U;
    memcpy(complete, &result_word, 1U);
    return NEON_OK;
}

/*==================================================================================================
*                                        PRIVATE FUNCTIONS
==================================================================================================*/

static neon_status_t spi_configure_pins(SPI_TypeDef *instance, neon_spi_mode_t mode, neon_spi_slave_select_t select) {
    static const fu2_syscsr_pin_route_t routes[2][4] = {
        {{SYSCSR_PIN_60, SYSCSR_PIN_FUNCTION_SPI0_SCK}, {SYSCSR_PIN_61, SYSCSR_PIN_FUNCTION_SPI0_MISO},
         {SYSCSR_PIN_62, SYSCSR_PIN_FUNCTION_SPI0_MOSI}, {SYSCSR_PIN_63, SYSCSR_PIN_FUNCTION_SPI0_SSN}},
        {{SYSCSR_PIN_20, SYSCSR_PIN_FUNCTION_SPI1_SCK}, {SYSCSR_PIN_21, SYSCSR_PIN_FUNCTION_SPI1_MISO},
         {SYSCSR_PIN_22, SYSCSR_PIN_FUNCTION_SPI1_MOSI}, {SYSCSR_PIN_23, SYSCSR_PIN_FUNCTION_SPI1_SSN}},
    };
    uint32_t index = spi_index(instance);
    uint32_t count = mode == NEON_SPI_MODE_SLAVE || select == NEON_SPI_SLAVE_SELECT_0 ? 4U : 3U;
    return fu2_syscsr_configure_pins(&spi_pins[index], routes[index], count, 0U);
}

static uint8_t spi_is_valid_instance(const SPI_TypeDef *instance) {
    return ((instance == SPI0) || (instance == SPI1)) ? 1U : 0U;
}

static uint32_t spi_index(const SPI_TypeDef *instance) {
    return (instance == SPI1) ? 1U : 0U;
}

static neon_crm_peripheral_t spi_crm_peripheral(const SPI_TypeDef *instance) {
    return (instance == SPI1) ? NEON_CRM_PERIPHERAL_SPI1 : NEON_CRM_PERIPHERAL_SPI0;
}

static uint32_t spi_enter_critical(void) {
    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    return primask;
}

static void spi_exit_critical(uint32_t primask) {
    __set_PRIMASK(primask);
}

static uint8_t spi_is_valid_data_width(neon_spi_data_width_t width) {
    return ((width == NEON_SPI_DATA_WIDTH_8BIT) || (width == NEON_SPI_DATA_WIDTH_16BIT) ||
            (width == NEON_SPI_DATA_WIDTH_24BIT) || (width == NEON_SPI_DATA_WIDTH_32BIT))
               ? 1U
               : 0U;
}

static uint8_t spi_is_valid_slave_select(neon_spi_slave_select_t slave) {
    return ((uint32_t)slave <= NEON_SPI_SLAVE_SELECT_3) ? 1U : 0U;
}

static uint8_t spi_is_valid_fifo_config(const neon_spi_fifo_config_t *config) {
    return ((config != NULL) && (config->tx_full_level <= 0xFFU) && (config->tx_empty_level <= 0xFFU) &&
            (config->rx_full_level <= 0xFFU) && (config->rx_empty_level <= 0xFFU))
               ? 1U
               : 0U;
}

static uint8_t spi_is_valid_timing(const neon_spi_timing_t *timing) {
    return ((timing != NULL) && (timing->slave_select_to_clock <= 0xFFFFU) &&
            (timing->clock_to_slave_select <= 0xFFFFU) && (timing->between_frames <= 0xFFFFU) &&
            (timing->slave_select_timeout <= 0xFFFFU) &&
            ((timing->between_frames_unit == NEON_SPI_B2B_CORE_CYCLES) ||
             (timing->between_frames_unit == NEON_SPI_B2B_SCK_CYCLES)))
               ? 1U
               : 0U;
}

static uint8_t spi_timing_fits_clock(const neon_spi_timing_t *timing, uint32_t divider) {
    return (timing->between_frames_unit == NEON_SPI_B2B_CORE_CYCLES ||
            timing->between_frames <= 0xFFFFU / (2U * (divider + 1U))) ? 1U : 0U;
}

static uint8_t spi_is_valid_config(const neon_spi_config_t *config) {
    if (config == NULL) {
        return 0U;
    }
    if (((config->mode != NEON_SPI_MODE_SLAVE) && (config->mode != NEON_SPI_MODE_MASTER)) ||
        ((config->clock_polarity != NEON_SPI_POLARITY_LOW) && (config->clock_polarity != NEON_SPI_POLARITY_HIGH)) ||
        ((config->clock_phase != NEON_SPI_PHASE_1EDGE) && (config->clock_phase != NEON_SPI_PHASE_2EDGE)) ||
        ((config->first_bit != NEON_SPI_FIRSTBIT_MSB) && (config->first_bit != NEON_SPI_FIRSTBIT_LSB)) ||
        (spi_is_valid_data_width(config->data_width) == 0U) ||
        ((uint32_t)config->pin_config > NEON_SPI_PIN_CONFIG_KEEP) ||
        /* Keep the full word load even when GCC tests only its upper bits. */
        (((const volatile neon_spi_config_t *)config)->clock_divider > (SPI_CLK_DIV_Msk >> SPI_CLK_DIV_Pos)) ||
        (spi_is_valid_slave_select(config->slave_select) == 0U) ||
        ((config->slave_select_mode != NEON_SPI_SS_MODE_NORMAL) &&
         (config->slave_select_mode != NEON_SPI_SS_MODE_MANUAL)) ||
        ((config->slave_select_force != NEON_SPI_SS_FORCE_HOLD) &&
         (config->slave_select_force != NEON_SPI_SS_FORCE_RELEASE)) ||
        ((config->slave_select_long != NEON_SPI_SS_LONG_DISABLE) &&
         (config->slave_select_long != NEON_SPI_SS_LONG_ENABLE)) ||
        ((config->interrupt_state != NEON_SPI_INTERRUPT_DISABLE) &&
         (config->interrupt_state != NEON_SPI_INTERRUPT_ENABLE)) ||
        (spi_is_valid_fifo_config(&config->fifo) == 0U) || (spi_is_valid_timing(&config->timing) == 0U)) {
        return 0U;
    }
    if (spi_timing_fits_clock(&config->timing, config->clock_divider) == 0U) {
        return 0U;
    }

    /* Shift register ở slave được cấu hình cố định 8 bit trong FU2. */
    if ((config->mode == NEON_SPI_MODE_SLAVE) && (config->data_width != NEON_SPI_DATA_WIDTH_8BIT)) {
        return 0U;
    }
    return 1U;
}

static neon_status_t spi_acquire(SPI_TypeDef *instance) {
    uint32_t index;
    uint32_t primask;

    if (spi_is_valid_instance(instance) == 0U) {
        return NEON_INVALID_PARAM;
    }
    index = spi_index(instance);
    primask = spi_enter_critical();
    if ((fu2_spi_initialized[index] == 0U) || (fu2_spi_faulted[index] != 0U)) {
        spi_exit_critical(primask);
        return NEON_ERROR;
    }
    if (fu2_spi_busy[index] != 0U) {
        spi_exit_critical(primask);
        return NEON_BUSY;
    }
    fu2_spi_busy[index] = 1U;
    spi_exit_critical(primask);
    return NEON_OK;
}

static void spi_release(SPI_TypeDef *instance) {
    uint32_t primask = spi_enter_critical();
    fu2_spi_busy[spi_index(instance)] = 0U;
    spi_exit_critical(primask);
}

static uint32_t spi_data_width_encoding(neon_spi_data_width_t width) {
    return (((uint32_t)width - 1U) << SPI_CMDCONF_LEN_Pos) & SPI_CMDCONF_LEN_Msk;
}

static uint32_t spi_slave_select_encoding(neon_spi_slave_select_t slave) {
    if (slave == NEON_SPI_SLAVE_SELECT_NONE) {
        return 0U;
    }
    return (1U << (SPI_CMDCONF_SLAVE_EN_Pos + (uint32_t)slave - 1U)) & SPI_CMDCONF_SLAVE_EN_Msk;
}

static uint32_t spi_fifo_encoding(const volatile neon_spi_fifo_config_t *config) {
    return (((uint32_t)config->tx_full_level << SPI_FIFO_TXFF_PROG_FLVL_Pos) & SPI_FIFO_TXFF_PROG_FLVL_Msk) |
           (((uint32_t)config->tx_empty_level << SPI_FIFO_TXFF_PROG_ELVL_Pos) & SPI_FIFO_TXFF_PROG_ELVL_Msk) |
           (((uint32_t)config->rx_full_level << SPI_FIFO_RXFF_PROG_FLVL_Pos) & SPI_FIFO_RXFF_PROG_FLVL_Msk) |
           (((uint32_t)config->rx_empty_level << SPI_FIFO_RXFF_PROG_ELVL_Pos) & SPI_FIFO_RXFF_PROG_ELVL_Msk);
}

static uint32_t spi_treg0_encoding(const volatile neon_spi_timing_t *timing) {
    return (((uint32_t)timing->slave_select_to_clock << SPI_TREG0_T_SS2CK_Pos) & SPI_TREG0_T_SS2CK_Msk) |
           (((uint32_t)timing->clock_to_slave_select << SPI_TREG0_T_CK2SS_Pos) & SPI_TREG0_T_CK2SS_Msk);
}

static uint32_t spi_treg1_encoding(const volatile neon_spi_timing_t *timing, uint32_t divider) {
    uint32_t gap = timing->between_frames;
    if (timing->between_frames_unit == NEON_SPI_B2B_SCK_CYCLES) {
        gap *= 2U * (divider + 1U);
    }
    return ((gap << SPI_TREG1_T_B2B_Pos) & SPI_TREG1_T_B2B_Msk) |
           (((uint32_t)timing->slave_select_timeout << SPI_TREG1_T_SS_TIMEOUT_Pos) & SPI_TREG1_T_SS_TIMEOUT_Msk);
}

static neon_status_t spi_require_tx_empty(const SPI_TypeDef *instance) {
    if (((instance->CTRL & SPI_CTRL_DMA_ENABLE_Msk) != 0U) && (((instance->CTRL_RX & SPI_CTRL_RX_ENABLE_Msk) != 0U) ||
                                                               ((instance->CTRL_TX & SPI_CTRL_TX_ENABLE_Msk) != 0U))) {
        return NEON_BUSY;
    }
    return ((instance->STT & SPI_STT_INT_TXFF_EMPTY_Msk) != 0U) ? NEON_OK : NEON_BUSY;
}

static neon_status_t spi_wait_tx_ready(const SPI_TypeDef *instance, uint32_t *timeout_polls) {
    while ((instance->STT & SPI_STT_INT_TXFF_FULL_Msk) != 0U) {
        if (*timeout_polls == 0U) {
            return NEON_TIMEOUT;
        }
        (*timeout_polls)--;
    }
    return NEON_OK;
}

static neon_status_t spi_wait_tx_empty(const SPI_TypeDef *instance, uint32_t *timeout_polls) {
    while ((instance->STT & SPI_STT_INT_TXFF_EMPTY_Msk) == 0U) {
        if (*timeout_polls == 0U) {
            return NEON_TIMEOUT;
        }
        (*timeout_polls)--;
    }
    return NEON_OK;
}

static neon_status_t spi_wait_rx_ready(const SPI_TypeDef *instance, uint32_t *timeout_polls) {
    while ((instance->STT & SPI_STT_INT_RXFF_EMPTY_Msk) != 0U) {
        if (*timeout_polls == 0U) {
            return NEON_TIMEOUT;
        }
        (*timeout_polls)--;
    }
    return NEON_OK;
}

static neon_status_t spi_wait_frame_complete(const SPI_TypeDef *instance, uint32_t received,
                                            uint32_t *timeout_polls) {
    /* Rev1.1 has no shifter-BUSY bit. FIFO empty is not wire completion.
     * RX can become ready at the last sampling edge. Wait a conservative
     * number of core-clock cycles without taking over SysTick or masking IRQs.
     * FU2 supplies SPI and CPU from the same system clock. Every loop below
     * includes a NOP and therefore takes at least one CPU/core clock cycle. */
    uint32_t period = 2U * (((instance->CLK & SPI_CLK_DIV_Msk) >> SPI_CLK_DIV_Pos) + 1U);
    uint32_t bits = ((instance->CMDCONF & SPI_CMDCONF_LEN_Msk) >> SPI_CMDCONF_LEN_Pos) + 1U;
    uint32_t setup = (instance->TREG0 & SPI_TREG0_T_SS2CK_Msk) >> SPI_TREG0_T_SS2CK_Pos;
    uint32_t hold = (instance->TREG0 & SPI_TREG0_T_CK2SS_Msk) >> SPI_TREG0_T_CK2SS_Pos;
    uint32_t gap = (instance->TREG1 & SPI_TREG1_T_B2B_Msk) >> SPI_TREG1_T_B2B_Pos;
    uint32_t cycles = period + gap;
    if (received == 0U) {
        /* The final TX word may have only just entered the shifter. Include
         * setup and inter-byte gaps as well as all remaining serial clocks. */
        cycles += setup + bits * period + ((bits + 7U) / 8U) * gap;
    }
    uint32_t ctrl = instance->CTRL;
    if ((ctrl & SPI_CTRL_SS_MODE_Msk) == 0U || (ctrl & SPI_CTRL_SS_FORCE_Msk) != 0U) {
        cycles += hold;
        if ((ctrl & SPI_CTRL_SS_MODE_Msk) != 0U) {
            cycles += (instance->TREG1 & SPI_TREG1_T_SS_TIMEOUT_Msk) >> SPI_TREG1_T_SS_TIMEOUT_Pos;
        }
    }
    while (cycles != 0U) {
        if (*timeout_polls == 0U) {
            return NEON_TIMEOUT;
        }
        --*timeout_polls;
        --cycles;
        __NOP();
    }
    return NEON_OK;
}

static neon_status_t spi_end_transfer(SPI_TypeDef *instance, neon_status_t status) {
    if (status == NEON_TIMEOUT) {
        /* A partial frame must not be mistaken for a new transaction. Init
         * or DeInit must reset the controller before another transfer. */
        fu2_spi_faulted[spi_index(instance)] = 1U;
    }
    spi_release(instance);
    return status;
}

static void spi_drain_rx_fifo(SPI_TypeDef *instance) {
    uint32_t count = NEON_SPI_FIFO_DEPTH;

    while (((instance->STT & SPI_STT_INT_RXFF_EMPTY_Msk) == 0U) && (count != 0U)) {
        (void)instance->RX;
        count--;
    }
}

static uint32_t spi_current_data_mask(const SPI_TypeDef *instance) {
    uint32_t length = ((instance->CMDCONF & SPI_CMDCONF_LEN_Msk) >> SPI_CMDCONF_LEN_Pos) + 1U;

    if (length >= 32U) {
        return 0xFFFFFFFFU;
    }
    return (1U << length) - 1U;
}

static uint8_t spi_is_byte_mode(const SPI_TypeDef *instance) {
    return (((instance->CMDCONF & SPI_CMDCONF_LEN_Msk) >> SPI_CMDCONF_LEN_Pos) == 7U) ? 1U : 0U;
}

static neon_status_t spi_transfer_core(SPI_TypeDef *instance, const uint32_t *tx_words, const uint8_t *tx_bytes,
                                       uint32_t *rx_words, uint8_t *rx_bytes, uint32_t count, uint32_t dummy_data,
                                       uint32_t timeout_polls, fu2_spi_transfer_t transfer, uint32_t byte_api) {
    neon_status_t status;
    uint32_t data_mask;
    uint32_t tx_value;
    uint32_t rx_value;
    uint16_t index;
    uint8_t master;

    if ((spi_is_valid_instance(instance) == 0U) ||
        ((transfer != FU2_SPI_TRANSFER_RX) && (count != 0U) && (tx_words == NULL) && (tx_bytes == NULL)) ||
        ((transfer != FU2_SPI_TRANSFER_TX) && (count != 0U) && (rx_words == NULL) && (rx_bytes == NULL))) {
        return NEON_INVALID_PARAM;
    }
    if (count == 0U) {
        return NEON_OK;
    }

    status = spi_acquire(instance);
    if (status != NEON_OK) {
        return status;
    }
    if ((byte_api != 0U) && (spi_is_byte_mode(instance) == 0U)) {
        spi_release(instance);
        return NEON_NOT_SUPPORTED;
    }
    if ((instance->CTRL & SPI_CTRL_DMA_ENABLE_Msk) != 0U) {
        spi_release(instance);
        return NEON_BUSY;
    }

    uint32_t mode = (instance->CTRL & SPI_CTRL_SPI_MODE_Msk) >> SPI_CTRL_SPI_MODE_Pos;
    if (mode > 1U) {
        spi_release(instance);
        return NEON_NOT_SUPPORTED; /* These polling APIs implement legacy SPI. */
    }
    master = (mode == 1U) ? 1U : 0U;
    if ((transfer == FU2_SPI_TRANSFER_TX_RX) && (master == 0U)) {
        /* Slave cần preload FIFO theo timing của master; API blocking từng word
     * không thể bảo đảm không underrun khi master chạy liên tục. */
        spi_release(instance);
        return NEON_NOT_SUPPORTED;
    }

    if ((master != 0U) || (transfer == FU2_SPI_TRANSFER_TX)) {
        status = spi_wait_tx_empty(instance, &timeout_polls);
        if (status != NEON_OK) {
            return spi_end_transfer(instance, status);
        }
    }

    if (master != 0U) {
        if (transfer == FU2_SPI_TRANSFER_TX) {
            instance->CMDCONF &= ~SPI_CMDCONF_RD_Msk;
        } else {
            instance->CMDCONF |= SPI_CMDCONF_RD_Msk;
            /* Loại dữ liệu sót lại từ một transfer trước bị timeout_polls. */
            spi_drain_rx_fifo(instance);
        }
    }

    data_mask = spi_current_data_mask(instance);
    for (index = 0U; index < count; index++) {
        if (transfer != FU2_SPI_TRANSFER_RX) {
            if (tx_words != NULL) {
                tx_value = tx_words[index];
            } else {
                tx_value = 0U;
                memcpy(&tx_value, &tx_bytes[index], sizeof(tx_bytes[index]));
            }
        } else {
            tx_value = dummy_data;
        }

        if ((master != 0U) || (transfer != FU2_SPI_TRANSFER_RX)) {
            status = spi_wait_tx_ready(instance, &timeout_polls);
            if (status != NEON_OK) {
                return spi_end_transfer(instance, status);
            }
            instance->TX = tx_value & data_mask;
        }

        if (transfer != FU2_SPI_TRANSFER_TX) {
            status = spi_wait_rx_ready(instance, &timeout_polls);
            if (status != NEON_OK) {
                return spi_end_transfer(instance, status);
            }
            rx_value = instance->RX & data_mask;
            if (rx_words != NULL) {
                rx_words[index] = rx_value;
            } else {
                const uint32_t result_word = rx_value;
                memcpy(&rx_bytes[index], &result_word, 1U);
            }
        }
    }

    if ((master != 0U) || (transfer == FU2_SPI_TRANSFER_TX)) {
        status = spi_wait_tx_empty(instance, &timeout_polls);
    }
    if (status == NEON_OK && master != 0U) {
        status = spi_wait_frame_complete(instance, transfer != FU2_SPI_TRANSFER_TX, &timeout_polls);
    }
    return spi_end_transfer(instance, status);
}

static uint8_t spi_is_valid_dma_config(const neon_spi_dma_config_t *config) {
    if ((config == NULL) || ((config->state != NEON_SPI_DMA_DISABLE) && (config->state != NEON_SPI_DMA_ENABLE))) {
        return 0U;
    }
    if (config->state == NEON_SPI_DMA_DISABLE) {
        return 1U; /* Abort does not require a new transfer configuration. */
    }
    if (((config->transfer_mode != NEON_SPI_DMA_SINGLE) && (config->transfer_mode != NEON_SPI_DMA_BURST)) ||
        ((config->burst_length != NEON_SPI_DMA_BURST_2) && (config->burst_length != NEON_SPI_DMA_BURST_4) &&
         (config->burst_length != NEON_SPI_DMA_BURST_8) && (config->burst_length != NEON_SPI_DMA_BURST_16)) ||
        (config->total_bytes == 0U) || (config->total_bytes > 0xFFFFU) || ((config->total_bytes & 0x3U) != 0U)) {
        return 0U;
    }
    return 1U;
}

static uint32_t spi_dma_burst_encoding(neon_spi_dma_burst_length_t burst) {
    if (burst == NEON_SPI_DMA_BURST_4) {
        return 1U;
    }
    if (burst == NEON_SPI_DMA_BURST_8) {
        return 2U;
    }
    if (burst == NEON_SPI_DMA_BURST_16) {
        return 3U;
    }
    return 0U;
}
