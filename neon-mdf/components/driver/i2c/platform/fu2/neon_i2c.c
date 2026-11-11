/**
 * @file neon_i2c.c
 * @brief Implementation for I2C.
 * @details
 * * Backend I2C for FU2.
 *
 * Blocking transfers, DMA request control, clock/reset and pin configuration for FU2.
 */

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/

#include "i2c/neon_i2c.h"
#include <stdio.h>
#include <string.h>

#include "crm/neon_crm.h"
#include "syscsr/platform/fu2/neon_syscsr_internal.h"

#include <stddef.h>

/*==================================================================================================
*                                        DEFINES AND MACROS
==================================================================================================*/
#define I2C_CONTEXT_PIN_AUTO (1U << 0)
#define I2C_CONTEXT_RESTORE_IRQ (1U << 1)
#define I2C_RECOVERY_POLLS 10000U
#define I2C_DMA_BUSY_MASK (I2C_ISR_DMA_BUSY_TX_Msk | I2C_ISR_DMA_BUSY_RX_Msk)
#define I2C_INTERRUPT_MASK ((uint32_t)NEON_I2C_INTERRUPT_ALL)
#define I2C_FLAG_MASK ((uint32_t)NEON_I2C_FLAG_ALL)

#define I2C_TRANSFER_ERROR_FLAGS                                                                                       \
    (I2C_ISR_ARB_LOST_Msk | I2C_ISR_SCL_STUCK_LOW_Msk | I2C_ISR_SDA_STUCK_LOW_Msk)

/*==================================================================================================
*                                         LOCAL VARIABLES
==================================================================================================*/
static neon_i2c_handle_t *fu2_i2c_owner;
static neon_i2c_config_t i2c_active_config;
static fu2_syscsr_pin_group_t i2c_pins;
/* Temporary RX diagnostics: preserve flags before IICR clears the error. */
static uint32_t i2c_error_isr;
static uint32_t i2c_error_iitr;

static neon_status_t i2c_reset_pin_route(void);

/*==================================================================================================
*                                   PRIVATE FUNCTION PROTOTYPES
==================================================================================================*/

static neon_status_t i2c_apply_config(neon_i2c_handle_t *hi2c, const neon_i2c_config_t *settings);

static uint32_t i2c_enter_critical(void);
static void i2c_exit_critical(uint32_t primask);
static uint8_t i2c_is_valid_handle(const neon_i2c_handle_t *hi2c);
static uint8_t i2c_is_valid_address(const neon_i2c_handle_t *hi2c, uint16_t address);
static uint8_t i2c_is_valid_config(const neon_i2c_config_t *config);
static neon_status_t i2c_validate_pin_route(void);
static neon_status_t i2c_configure_pin_route(void);
static neon_status_t i2c_reset_pin_route(void);
static void i2c_pulse_interrupt_clear(I2C_TypeDef *instance, uint32_t interrupts);
static uint32_t i2c_error_to_clear_mask(uint32_t status);
static uint32_t i2c_error_code(uint32_t status);
static uint32_t i2c_logical_flags(const I2C_TypeDef *instance);
static neon_status_t i2c_check_transfer_error(neon_i2c_handle_t *hi2c);
static void i2c_abort_master(neon_i2c_handle_t *hi2c, uint8_t transfer_started);
static void i2c_mark_fault(neon_i2c_handle_t *hi2c);
static void i2c_restore_polling_irq(neon_i2c_handle_t *hi2c, uint32_t saved_interrupts,
                                  uint32_t irq_enabled, uint32_t irq_pending);
static neon_status_t i2c_consume_timeout(neon_i2c_handle_t *hi2c, uint32_t *timeout_polls);
static neon_status_t i2c_wait_bus_idle(neon_i2c_handle_t *hi2c, uint32_t *timeout_polls, uint8_t check_error);
static neon_status_t i2c_wait_start_consumed(neon_i2c_handle_t *hi2c, uint32_t *timeout_polls);
static neon_status_t i2c_wait_tx_space(neon_i2c_handle_t *hi2c, uint32_t *timeout_polls);
static neon_status_t i2c_wait_rx_data(neon_i2c_handle_t *hi2c, uint32_t *timeout_polls,
                                     uint32_t *transfer_started);
static neon_status_t i2c_wait_transfer_done(neon_i2c_handle_t *hi2c, uint32_t *timeout_polls);
static neon_status_t i2c_stop_master(neon_i2c_handle_t *hi2c, uint32_t *timeout_polls,
                                    uint32_t *transfer_started);
static neon_status_t i2c_acquire(neon_i2c_handle_t *hi2c, neon_i2c_state_t state);
static void i2c_release(neon_i2c_handle_t *hi2c);
static void i2c_flush_fifos(I2C_TypeDef *instance);
static neon_status_t i2c_prepare_master(neon_i2c_handle_t *hi2c, uint32_t address, uint32_t size,
                                        neon_i2c_direction_t direction, uint32_t use_sub_address, uint32_t sub_address,
                                        uint32_t *timeout_polls, uint32_t *saved_interrupts);
static neon_status_t i2c_finish_polling_transfer(neon_i2c_handle_t *hi2c, uint32_t *timeout_polls,
                                                 uint32_t *transfer_started);
static neon_status_t i2c_master_write_core(neon_i2c_handle_t *hi2c, uint32_t address, uint32_t prefix_valid,
                                           uint32_t prefix, const uint8_t *data, uint32_t data_size,
                                           uint32_t timeout_polls);
static neon_status_t i2c_master_read_core(neon_i2c_handle_t *hi2c, uint32_t address, uint32_t use_sub_address,
                                          uint32_t sub_address, uint8_t *data, uint32_t size, uint32_t timeout_polls);
static void i2c_prepare_slave(neon_i2c_handle_t *hi2c, uint32_t *saved_interrupts);
static neon_status_t i2c_finish_slave(neon_i2c_handle_t *hi2c);

/*==================================================================================================
*                                         GLOBAL FUNCTIONS
==================================================================================================*/

/** @copydoc neon_i2c_get_default_config */
void neon_i2c_get_default_config(neon_i2c_config_t *config) {
    if (config == NULL) {
        return;
    }

    config->mode = NEON_I2C_MODE_MASTER;
    config->speed_mode = NEON_I2C_SPEED_STANDARD_FAST;
    config->addressing_mode = NEON_I2C_ADDRESSING_7BIT;
    config->clock_stretch = NEON_I2C_CLOCK_STRETCH_DISABLE;
    config->own_address = 0U;
    config->pin_config = NEON_I2C_PIN_CONFIG_AUTO;
    memset(&config->timing, 0, sizeof(config->timing));
    (void)neon_i2c_calculate_timing(neon_crm_get_system_clock_frequency(), 100000U, &config->timing);
    config->fifo.tx_full_level = 0U;
    config->fifo.tx_empty_level = 0U;
    config->fifo.rx_full_level = 0U;
    config->fifo.rx_empty_level = 0U;
}

/* The largest product is 60000 kHz * 9500 ns and fits in 32 bits. */
static uint32_t i2c_ns_to_cycles(uint32_t clock_khz, uint32_t ns) {
    return (clock_khz * ns + 999999U) / 1000000U;
}

/** @copydoc neon_i2c_calculate_timing */
neon_status_t neon_i2c_calculate_timing(uint32_t i2c_clock_hz, uint32_t scl_hz, neon_i2c_timing_t *timing) {
    if (timing == NULL || i2c_clock_hz == 0U || i2c_clock_hz > 60000000U ||
        scl_hz == 0U || scl_hz > 400000U) {
        return NEON_INVALID_PARAM;
    }
    uint32_t khz = (i2c_clock_hz + 999U) / 1000U;
    uint32_t standard = scl_hz <= 100000U;
    uint32_t period = (i2c_clock_hz + scl_hz - 1U) / scl_hz;
    uint32_t low = (period + 1U) / 2U;
    uint32_t min_low = i2c_ns_to_cycles(khz, standard ? 4700U : 1300U);
    /* spike=1, synchronizer=2: low >= 4*(spike+2+sync+1). */
    if (min_low < 24U) { min_low = 24U; }
    if (low < min_low) { low = min_low; }
    uint32_t high = period > low ? period - low : 0U;
    uint32_t min_high = i2c_ns_to_cycles(khz, standard ? 4000U : 600U);
    /* Observed high includes six filter/synchronizer cycles. */
    if (min_high < 7U) { min_high = 7U; }
    if (high < min_high) { high = min_high; }
    if (low > 0xFFFFU || high - 6U > 0xFFFFU) { return NEON_INVALID_PARAM; }

    neon_i2c_timing_t result = {0};
    result.scl_low_count = low;
    result.scl_high_count = high - 6U;
    result.spike_count = 1U;
    result.start_setup_count = i2c_ns_to_cycles(khz, standard ? 9500U : 1300U);
    result.start_hold_count = i2c_ns_to_cycles(khz, standard ? 8100U : 1300U);
    result.stop_setup_count = i2c_ns_to_cycles(khz, standard ? 8100U : 1300U);
    result.sda_hold_count = i2c_ns_to_cycles(khz, 300U);
    result.bus_free_count = i2c_ns_to_cycles(khz, standard ? 9500U : 2700U);
    result.scl_stuck_low_count = 0xFFFFU;
    result.sda_stuck_low_count = 0xFFFFU;
    memcpy(timing, &result, sizeof(result));
    return NEON_OK;
}

/** @copydoc neon_i2c_get_state */
neon_i2c_state_t neon_i2c_get_state(const neon_i2c_handle_t *hi2c) {
    return (hi2c == NULL) ? NEON_I2C_STATE_RESET : hi2c->state;
}

/** @copydoc neon_i2c_get_error */
uint32_t neon_i2c_get_error(const neon_i2c_handle_t *hi2c) {
    return (hi2c == NULL) ? NEON_I2C_ERROR_NONE : hi2c->error_code;
}

/** @copydoc neon_i2c_clear_error */
void neon_i2c_clear_error(neon_i2c_handle_t *hi2c) {
    uint32_t primask = i2c_enter_critical();
    if ((hi2c != NULL) && (hi2c->state == NEON_I2C_STATE_READY)) {
        hi2c->error_code = NEON_I2C_ERROR_NONE;
    }
    i2c_exit_critical(primask);
}

/** @copydoc neon_i2c_init */
neon_status_t neon_i2c_init(neon_i2c_handle_t *hi2c, neon_i2c_t instance) {
    if (hi2c == NULL || !(instance == I2C0)) {
        return NEON_INVALID_PARAM;
    }
    uint32_t saved = i2c_enter_critical();
    if (fu2_i2c_owner != NULL) {
        i2c_exit_critical(saved);
        return NEON_BUSY;
    }
    memset(hi2c, 0, sizeof(*hi2c));
    hi2c->instance = instance;
    hi2c->state = NEON_I2C_STATE_INITIALIZED;
    i2c_exit_critical(saved);
    return NEON_OK;
}

/** @copydoc neon_i2c_set_config */
neon_status_t neon_i2c_set_config(neon_i2c_handle_t *hi2c, const neon_i2c_config_t *config) {
    if (hi2c == NULL || config == NULL || !(hi2c->instance == I2C0)) {
        return NEON_INVALID_PARAM;
    }
    if (i2c_is_valid_config(config) == 0U) {
        return NEON_INVALID_PARAM;
    }
    /* Ultra Fast needs push-pull pins and different transfer semantics. */
    if (config->speed_mode == NEON_I2C_SPEED_ULTRA_FAST) {
        return NEON_NOT_SUPPORTED;
    }
    if (config->pin_config == NEON_I2C_PIN_CONFIG_AUTO) {
        neon_status_t route_status = i2c_validate_pin_route();
        if (route_status != NEON_OK) {
            return route_status;
        }
    }
    if (hi2c->state == NEON_I2C_STATE_READY || hi2c->state == NEON_I2C_STATE_FAULT) {
        neon_status_t status = neon_i2c_deinit(hi2c);
        if (status != NEON_OK) {
            return status;
        }
        hi2c->state = NEON_I2C_STATE_INITIALIZED;
    }
    return i2c_apply_config(hi2c, config);
}

/** @copydoc neon_i2c_get_config */
neon_status_t neon_i2c_get_config(const neon_i2c_handle_t *hi2c, neon_i2c_config_t *config) {
    if (hi2c == NULL || config == NULL || !(hi2c->instance == I2C0)) {
        return NEON_INVALID_PARAM;
    }
    uint32_t saved = i2c_enter_critical();
    neon_status_t status = NEON_ERROR;
    if (fu2_i2c_owner == hi2c) {
        status = NEON_BUSY;
        if (hi2c->state == NEON_I2C_STATE_READY) {
            memcpy(config, &(i2c_active_config), sizeof(*config));
            status = NEON_OK;
        }
    }
    i2c_exit_critical(saved);
    return status;
}

/** @copydoc neon_i2c_get_instance */
neon_i2c_t neon_i2c_get_instance(const neon_i2c_handle_t *hi2c) {
    return hi2c == NULL ? NULL : hi2c->instance;
}

/** @copydoc neon_i2c_deinit */
neon_status_t neon_i2c_deinit(neon_i2c_handle_t *hi2c) {
    if (hi2c != NULL && hi2c->state == NEON_I2C_STATE_INITIALIZED) {
        hi2c->state = NEON_I2C_STATE_RESET;
        return NEON_OK;
    }

    neon_status_t status = NEON_OK;
    neon_status_t next_status;
    uint32_t primask;

    if (i2c_is_valid_handle(hi2c) == 0U) {
        return NEON_INVALID_PARAM;
    }

    primask = i2c_enter_critical();
    if (fu2_i2c_owner != hi2c) {
        i2c_exit_critical(primask);
        return NEON_ERROR;
    }
    if (hi2c->state != NEON_I2C_STATE_READY && hi2c->state != NEON_I2C_STATE_FAULT) {
        i2c_exit_critical(primask);
        return NEON_BUSY;
    }
    /* Request disable alone does not acknowledge outstanding DMAC accesses. */
    if ((hi2c->instance->ISR & I2C_DMA_BUSY_MASK) != 0U ||
        ((hi2c->instance->IDTR | hi2c->instance->IDRR) & I2C_IDTR_ENABLE_Msk) != 0U ||
        (hi2c->state == NEON_I2C_STATE_READY &&
         (((hi2c->instance->ISR & I2C_ISR_BUS_BUSY_Msk) != 0U) ||
          ((hi2c->instance->ICTLR & (I2C_ICTLR_START_Msk | I2C_ICTLR_STOP_Msk)) != 0U)))) {
        i2c_exit_critical(primask);
        return NEON_BUSY;
    }
    hi2c->state = NEON_I2C_STATE_BUSY_CONFIG;
    i2c_exit_critical(primask);

    hi2c->instance->IER = 0U;
    next_status = neon_crm_peripheral_reset(NEON_CRM_PERIPHERAL_I2C0);
    if (next_status != NEON_OK) {
        i2c_mark_fault(hi2c);
        return next_status;
    }
    if (status == NEON_OK) {
        status = next_status;
    }
    if ((hi2c->private_context & I2C_CONTEXT_PIN_AUTO) != 0U) {
        next_status = i2c_reset_pin_route();
        if (status == NEON_OK) {
            status = next_status;
        }
    }
    next_status = neon_crm_peripheral_clock_disable(NEON_CRM_PERIPHERAL_I2C0);
    if (status == NEON_OK) {
        status = next_status;
    }

    if ((hi2c->private_context & I2C_CONTEXT_RESTORE_IRQ) != 0U) {
        NVIC_ClearPendingIRQ(I2C0_IRQn);
        NVIC_EnableIRQ(I2C0_IRQn);
    }
    primask = i2c_enter_critical();
    fu2_i2c_owner = NULL;
    hi2c->state = NEON_I2C_STATE_RESET;
    hi2c->error_code = NEON_I2C_ERROR_NONE;
    hi2c->private_context = 0U;
    i2c_exit_critical(primask);
    return status;
}

/** @copydoc neon_i2c_master_transmit */
neon_status_t neon_i2c_master_transmit(neon_i2c_handle_t *hi2c, uint16_t dev_address, const uint8_t *data,
                                       uint16_t size, uint32_t timeout_polls) {
    neon_status_t status;

    if ((i2c_is_valid_handle(hi2c) == 0U) || (data == NULL) || (size == 0U) || (size > NEON_I2C_MAX_TRANSFER_SIZE) ||
        (i2c_is_valid_address(hi2c, dev_address) == 0U)) {
        return NEON_INVALID_PARAM;
    }
    if (i2c_active_config.mode != NEON_I2C_MODE_MASTER) {
        return NEON_NOT_SUPPORTED;
    }

    status = i2c_acquire(hi2c, NEON_I2C_STATE_BUSY_TX);
    if (status != NEON_OK) {
        return status;
    }
    hi2c->error_code = NEON_I2C_ERROR_NONE;
    status = i2c_master_write_core(hi2c, dev_address, 0U, 0U, data, size, timeout_polls);
    i2c_release(hi2c);
    return status;
}

/** @copydoc neon_i2c_master_receive */
neon_status_t neon_i2c_master_receive(neon_i2c_handle_t *hi2c, uint16_t dev_address, uint8_t *data, uint16_t size,
                                      uint32_t timeout_polls) {
    neon_status_t status;

    if ((i2c_is_valid_handle(hi2c) == 0U) || (data == NULL) || (size == 0U) || (size > NEON_I2C_MAX_TRANSFER_SIZE) ||
        (i2c_is_valid_address(hi2c, dev_address) == 0U)) {
        return NEON_INVALID_PARAM;
    }
    if (i2c_active_config.mode != NEON_I2C_MODE_MASTER) {
        return NEON_NOT_SUPPORTED;
    }

    status = i2c_acquire(hi2c, NEON_I2C_STATE_BUSY_RX);
    if (status != NEON_OK) {
        return status;
    }
    hi2c->error_code = NEON_I2C_ERROR_NONE;
    status = i2c_master_read_core(hi2c, dev_address, 0U, 0U, data, size, timeout_polls);
    i2c_release(hi2c);
    return status;
}

/** @copydoc neon_i2c_mem_read */
neon_status_t neon_i2c_mem_read(neon_i2c_handle_t *hi2c, uint16_t dev_address, uint8_t register_address, uint8_t *data,
                                neon_field16_t data_size, uint32_t timeout_polls) {
    neon_status_t status;

    if ((i2c_is_valid_handle(hi2c) == 0U) || (data == NULL) || (data_size == 0U) ||
        (data_size > NEON_I2C_MAX_TRANSFER_SIZE) || (i2c_is_valid_address(hi2c, dev_address) == 0U)) {
        return NEON_INVALID_PARAM;
    }
    if (i2c_active_config.mode != NEON_I2C_MODE_MASTER) {
        return NEON_NOT_SUPPORTED;
    }

    status = i2c_acquire(hi2c, NEON_I2C_STATE_BUSY_RX);
    if (status != NEON_OK) {
        return status;
    }
    hi2c->error_code = NEON_I2C_ERROR_NONE;
    status = i2c_master_read_core(hi2c, dev_address, 1U, register_address, data, data_size, timeout_polls);
    i2c_release(hi2c);
    return status;
}

/** @copydoc neon_i2c_mem_write */
neon_status_t neon_i2c_mem_write(neon_i2c_handle_t *hi2c, uint16_t dev_address, uint8_t register_address,
                                 const uint8_t *data, neon_field16_t data_size, uint32_t timeout_polls) {
    neon_status_t status;

    if ((i2c_is_valid_handle(hi2c) == 0U) || ((data == NULL) && (data_size != 0U)) || (data_size > 255U) ||
        (i2c_is_valid_address(hi2c, dev_address) == 0U)) {
        return NEON_INVALID_PARAM;
    }
    if (i2c_active_config.mode != NEON_I2C_MODE_MASTER) {
        return NEON_NOT_SUPPORTED;
    }

    status = i2c_acquire(hi2c, NEON_I2C_STATE_BUSY_TX);
    if (status != NEON_OK) {
        return status;
    }
    hi2c->error_code = NEON_I2C_ERROR_NONE;
    status = i2c_master_write_core(hi2c, dev_address, 1U, register_address, data, data_size, timeout_polls);
    i2c_release(hi2c);
    return status;
}

/** @copydoc neon_i2c_slave_transmit */
neon_status_t neon_i2c_slave_transmit(neon_i2c_handle_t *hi2c, const uint8_t *data, uint16_t size,
                                      uint32_t timeout_polls) {
    if (i2c_is_valid_handle(hi2c) == 0U || data == NULL || size == 0U || size > NEON_I2C_MAX_TRANSFER_SIZE) {
        return NEON_INVALID_PARAM;
    }
    if (i2c_active_config.mode != NEON_I2C_MODE_SLAVE) { return NEON_NOT_SUPPORTED; }
    neon_status_t status = i2c_acquire(hi2c, NEON_I2C_STATE_BUSY_TX);
    if (status != NEON_OK) { return status; }
    /* The master must wait until this blocking operation has been armed. */
    if ((hi2c->instance->ISR & I2C_ISR_BUS_BUSY_Msk) != 0U) {
        i2c_release(hi2c);
        return NEON_BUSY;
    }
    uint32_t irq_enabled = NVIC_GetEnableIRQ(I2C0_IRQn);
    uint32_t irq_pending = NVIC_GetPendingIRQ(I2C0_IRQn);
    NVIC_DisableIRQ(I2C0_IRQn);
    uint32_t saved_interrupts;
    hi2c->error_code = NEON_I2C_ERROR_NONE;
    i2c_prepare_slave(hi2c, &saved_interrupts);
    uint32_t queued = 0U;
    uint32_t stop_settled = 0U;
    for (;;) {
        status = i2c_check_transfer_error(hi2c);
        if (status != NEON_OK) { break; }
        uint32_t stopped = hi2c->instance->IITR & I2C_IITR_STOP_DEC_Msk;
        uint32_t flags = hi2c->instance->ISR;
        if (stopped != 0U) {
            /* Allow the FIFO read pointer to cross its clock boundary after STOP. */
            if (++stop_settled >= 8U) {
                if (queued < size || (flags & I2C_ISR_TXFF_EMPTY_Msk) == 0U) {
                    hi2c->error_code |= NEON_I2C_ERROR_SHORT_TRANSFER;
                    status = NEON_ERROR;
                }
                break;
            }
        } else if (queued < size && (flags & I2C_ISR_TXFF_FULL_Msk) == 0U) {
            uint32_t word = 0U;
            memcpy(&word, &data[queued++], 1U);
            hi2c->instance->ITDR = word;
        }
        status = i2c_consume_timeout(hi2c, &timeout_polls);
        if (status != NEON_OK) { break; }
    }
    neon_status_t cleanup = i2c_finish_slave(hi2c);
    if (status == NEON_OK) { status = cleanup; }
    i2c_restore_polling_irq(hi2c, saved_interrupts, irq_enabled, irq_pending);
    i2c_release(hi2c);
    return status;
}

/** @copydoc neon_i2c_slave_receive */
neon_status_t neon_i2c_slave_receive(neon_i2c_handle_t *hi2c, uint8_t *data, uint16_t size, uint32_t timeout_polls) {
    uint32_t received;
    return neon_i2c_slave_receive_ex(hi2c, data, size, &received, timeout_polls);
}

/** @copydoc neon_i2c_slave_receive_ex */
neon_status_t neon_i2c_slave_receive_ex(neon_i2c_handle_t *hi2c, uint8_t *data, uint16_t size,
                                      uint32_t *received_count, uint32_t timeout_polls) {
    if (i2c_is_valid_handle(hi2c) == 0U || data == NULL || size == 0U ||
        size > NEON_I2C_MAX_TRANSFER_SIZE || received_count == NULL) {
        return NEON_INVALID_PARAM;
    }
    *received_count = 0U;
    if (i2c_active_config.mode != NEON_I2C_MODE_SLAVE) { return NEON_NOT_SUPPORTED; }
    neon_status_t status = i2c_acquire(hi2c, NEON_I2C_STATE_BUSY_RX);
    if (status != NEON_OK) { return status; }
    if ((hi2c->instance->ISR & I2C_ISR_BUS_BUSY_Msk) != 0U) {
        i2c_release(hi2c);
        return NEON_BUSY;
    }
    uint32_t irq_enabled = NVIC_GetEnableIRQ(I2C0_IRQn);
    uint32_t irq_pending = NVIC_GetPendingIRQ(I2C0_IRQn);
    NVIC_DisableIRQ(I2C0_IRQn);
    uint32_t saved_interrupts;
    hi2c->error_code = NEON_I2C_ERROR_NONE;
    i2c_prepare_slave(hi2c, &saved_interrupts);
    uint32_t received = 0U;
    uint32_t stop_settled = 0U;
    for (;;) {
        status = i2c_check_transfer_error(hi2c);
        if (status != NEON_OK) { break; }
        uint32_t stopped = hi2c->instance->IITR & I2C_IITR_STOP_DEC_Msk;
        uint32_t flags = hi2c->instance->ISR;
        if ((flags & I2C_ISR_RXFF_EMPTY_Msk) == 0U) {
            const uint32_t word = hi2c->instance->IRDR & I2C_IRDR_RX_DATA_Msk;
            if (received < size) {
                memcpy(&data[received++], &word, 1U);
            } else {
                hi2c->error_code |= NEON_I2C_ERROR_RX_OVERFLOW;
            }
            stop_settled = 0U;
        } else if (stopped != 0U && ++stop_settled >= 8U) {
            if (received < size) { hi2c->error_code |= NEON_I2C_ERROR_SHORT_TRANSFER; }
            if (hi2c->error_code != NEON_I2C_ERROR_NONE) { status = NEON_ERROR; }
            break;
        }
        status = i2c_consume_timeout(hi2c, &timeout_polls);
        if (status != NEON_OK) { break; }
    }
    *received_count = received;
    neon_status_t cleanup = i2c_finish_slave(hi2c);
    if (status == NEON_OK) { status = cleanup; }
    i2c_restore_polling_irq(hi2c, saved_interrupts, irq_enabled, irq_pending);
    i2c_release(hi2c);
    return status;
}

/** @copydoc neon_i2c_write_data */
neon_status_t neon_i2c_write_data(neon_i2c_handle_t *hi2c, uint8_t data) {
    neon_status_t status;
    if ((i2c_is_valid_handle(hi2c) == 0U) || (fu2_i2c_owner != hi2c)) {
        return NEON_INVALID_PARAM;
    }
    if (i2c_active_config.mode != NEON_I2C_MODE_SLAVE) {
        return NEON_NOT_SUPPORTED;
    }
    status = i2c_acquire(hi2c, NEON_I2C_STATE_BUSY_TX);
    if (status != NEON_OK) {
        return status;
    }
    if ((hi2c->instance->ISR & I2C_ISR_TXFF_FULL_Msk) != 0U) {
        status = NEON_BUSY;
    } else {
        hi2c->instance->ITDR = data;
    }
    i2c_release(hi2c);
    return status;
}

/** @copydoc neon_i2c_read_data */
neon_status_t neon_i2c_read_data(neon_i2c_handle_t *hi2c, uint8_t *data) {
    neon_status_t status;
    if ((i2c_is_valid_handle(hi2c) == 0U) || (data == NULL) || (fu2_i2c_owner != hi2c)) {
        return NEON_INVALID_PARAM;
    }
    if (i2c_active_config.mode != NEON_I2C_MODE_SLAVE) {
        return NEON_NOT_SUPPORTED;
    }
    status = i2c_acquire(hi2c, NEON_I2C_STATE_BUSY_RX);
    if (status != NEON_OK) {
        return status;
    }
    if ((hi2c->instance->ISR & I2C_ISR_RXFF_EMPTY_Msk) != 0U) {
        status = NEON_BUSY;
    } else {
        const uint32_t result_word = (uint8_t)(hi2c->instance->IRDR & I2C_IRDR_RX_DATA_Msk);
        memcpy(data, &result_word, 1U);
    }
    i2c_release(hi2c);
    return status;
}

/** @copydoc neon_i2c_get_slave_status */
neon_status_t neon_i2c_get_slave_status(const neon_i2c_handle_t *hi2c, neon_i2c_slave_status_t *status) {
    uint32_t snapshot;

    if ((i2c_is_valid_handle(hi2c) == 0U) || (status == NULL) || (fu2_i2c_owner != hi2c) ||
        (hi2c->state == NEON_I2C_STATE_RESET)) {
        return NEON_INVALID_PARAM;
    }
    if (i2c_active_config.mode != NEON_I2C_MODE_SLAVE) {
        return NEON_NOT_SUPPORTED;
    }

    snapshot = hi2c->instance->ISR;
    status->matched_address = (uint8_t)((snapshot & I2C_ISR_ADDR_MATCH_Msk) >> I2C_ISR_ADDR_MATCH_Pos);
    status->direction = ((snapshot & I2C_ISR_SLAVE_DIR_Msk) != 0U) ? NEON_I2C_DIRECTION_READ : NEON_I2C_DIRECTION_WRITE;
    status->pec = (uint8_t)((snapshot & I2C_ISR_PEC_DATA_Msk) >> I2C_ISR_PEC_DATA_Pos);
    return NEON_OK;
}

/** @copydoc neon_i2c_enable_interrupts */
neon_status_t neon_i2c_enable_interrupts(neon_i2c_handle_t *hi2c, uint32_t interrupts) {
    neon_status_t status;

    if ((interrupts == 0U) || ((interrupts & ~I2C_INTERRUPT_MASK) != 0U)) {
        return NEON_INVALID_PARAM;
    }
    status = i2c_acquire(hi2c, NEON_I2C_STATE_BUSY_CONFIG);
    if (status != NEON_OK) {
        return status;
    }
    hi2c->instance->IIER |= interrupts;
    i2c_release(hi2c);
    return NEON_OK;
}

/** @copydoc neon_i2c_disable_interrupts */
neon_status_t neon_i2c_disable_interrupts(neon_i2c_handle_t *hi2c, uint32_t interrupts) {
    neon_status_t status;

    if ((interrupts == 0U) || ((interrupts & ~I2C_INTERRUPT_MASK) != 0U)) {
        return NEON_INVALID_PARAM;
    }
    status = i2c_acquire(hi2c, NEON_I2C_STATE_BUSY_CONFIG);
    if (status != NEON_OK) {
        return status;
    }
    hi2c->instance->IIER &= ~interrupts;
    i2c_release(hi2c);
    return NEON_OK;
}

/** @copydoc neon_i2c_get_interrupt_status */
neon_status_t neon_i2c_get_interrupt_status(const neon_i2c_handle_t *hi2c, uint32_t *interrupts) {
    if ((i2c_is_valid_handle(hi2c) == 0U) || (interrupts == NULL) || (fu2_i2c_owner != hi2c) ||
        (hi2c->state == NEON_I2C_STATE_RESET)) {
        return NEON_INVALID_PARAM;
    }
    *interrupts = hi2c->instance->IITR & I2C_INTERRUPT_MASK;
    return NEON_OK;
}

/** @copydoc neon_i2c_clear_interrupts */
neon_status_t neon_i2c_clear_interrupts(neon_i2c_handle_t *hi2c, uint32_t interrupts) {
    neon_status_t status;
    if ((i2c_is_valid_handle(hi2c) == 0U) || (interrupts == 0U) || ((interrupts & ~I2C_INTERRUPT_MASK) != 0U) ||
        (fu2_i2c_owner != hi2c) || (hi2c->state == NEON_I2C_STATE_RESET)) {
        return NEON_INVALID_PARAM;
    }
    status = i2c_acquire(hi2c, NEON_I2C_STATE_BUSY_CONFIG);
    if (status != NEON_OK) {
        return status;
    }
    i2c_pulse_interrupt_clear(hi2c->instance, interrupts);
    i2c_release(hi2c);
    return NEON_OK;
}

/** @copydoc neon_i2c_get_flags */
neon_status_t neon_i2c_get_flags(const neon_i2c_handle_t *hi2c, uint32_t *flags) {
    if ((i2c_is_valid_handle(hi2c) == 0U) || (flags == NULL) || (fu2_i2c_owner != hi2c) ||
        (hi2c->state == NEON_I2C_STATE_RESET)) {
        return NEON_INVALID_PARAM;
    }
    *flags = i2c_logical_flags(hi2c->instance);
    return NEON_OK;
}

/** @copydoc neon_i2c_is_active_flag */
neon_status_t neon_i2c_is_active_flag(const neon_i2c_handle_t *hi2c, uint32_t flags, uint8_t *active) {
    if ((i2c_is_valid_handle(hi2c) == 0U) || (active == NULL) || (flags == 0U) || ((flags & ~I2C_FLAG_MASK) != 0U) ||
        (fu2_i2c_owner != hi2c) || (hi2c->state == NEON_I2C_STATE_RESET)) {
        return NEON_INVALID_PARAM;
    }
    const uint32_t result_word = ((i2c_logical_flags(hi2c->instance) & flags) == flags) ? 1U : 0U;
    memcpy(active, &result_word, 1U);
    return NEON_OK;
}

/** @copydoc neon_i2c_set_dma_config */
neon_status_t neon_i2c_set_dma_config(neon_i2c_handle_t *hi2c, neon_i2c_dma_direction_t direction,
                                    const neon_i2c_dma_config_t *config) {
    if ((uint32_t)direction > NEON_I2C_DMA_RX || config == NULL ||
        (uint32_t)config->mode > NEON_I2C_DMA_BURST ||
        (uint32_t)config->burst_length > NEON_I2C_DMA_BURST_16 ||
        config->total_bytes == 0U || config->total_bytes > 255U) {
        return NEON_INVALID_PARAM;
    }
    neon_status_t status = i2c_acquire(hi2c, NEON_I2C_STATE_BUSY_CONFIG);
    if (status != NEON_OK) { return status; }
    I2C_TypeDef *instance = hi2c->instance;
    if ((instance->ISR & (I2C_ISR_BUS_BUSY_Msk | I2C_DMA_BUSY_MASK)) != 0U ||
        (instance->ICTLR & (I2C_ICTLR_START_Msk | I2C_ICTLR_STOP_Msk)) != 0U ||
        ((instance->IDTR | instance->IDRR) & I2C_IDTR_ENABLE_Msk) != 0U) {
        i2c_release(hi2c);
        return NEON_BUSY;
    }
    volatile uint32_t *control = direction == NEON_I2C_DMA_TX ? &instance->IDTR : &instance->IDRR;
    /* IDTR and IDRR have identical field layouts. Keep requests disabled. */
    *control = ((uint32_t)config->burst_length << I2C_IDTR_MAX_BURST_LEN_Pos) |
               ((uint32_t)config->mode << I2C_IDTR_BURST_MODE_Pos) |
               ((uint32_t)config->total_bytes << I2C_IDTR_TOTAL_BYTES_Pos);
    i2c_pulse_interrupt_clear(instance, direction == NEON_I2C_DMA_TX ?
        NEON_I2C_INTERRUPT_DMA_TX_DONE : NEON_I2C_INTERRUPT_DMA_RX_DONE);
    i2c_release(hi2c);
    return NEON_OK;
}

/** @copydoc neon_i2c_configure_dma */
neon_status_t neon_i2c_configure_dma(neon_i2c_handle_t *hi2c, neon_i2c_dma_direction_t direction, uint8_t enable) {
    if ((uint32_t)direction > NEON_I2C_DMA_RX || enable > 1U) { return NEON_INVALID_PARAM; }
    neon_status_t status = i2c_acquire(hi2c, NEON_I2C_STATE_BUSY_CONFIG);
    if (status != NEON_OK) { return status; }
    I2C_TypeDef *instance = hi2c->instance;
    volatile uint32_t *control = direction == NEON_I2C_DMA_TX ? &instance->IDTR : &instance->IDRR;
    if (enable != 0U) {
        if ((*control & I2C_IDTR_TOTAL_BYTES_Msk) == 0U) {
            i2c_release(hi2c);
            return NEON_INVALID_PARAM;
        }
        if ((*control & I2C_IDTR_ENABLE_Msk) != 0U ||
            (instance->ISR & (I2C_ISR_BUS_BUSY_Msk | I2C_DMA_BUSY_MASK)) != 0U ||
            (instance->ICTLR & (I2C_ICTLR_START_Msk | I2C_ICTLR_STOP_Msk)) != 0U) {
            i2c_release(hi2c);
            return NEON_BUSY;
        }
        uint32_t done = direction == NEON_I2C_DMA_TX ?
            NEON_I2C_INTERRUPT_DMA_TX_DONE : NEON_I2C_INTERRUPT_DMA_RX_DONE;
        i2c_pulse_interrupt_clear(instance, done);
        /* IITR reports enabled events; also makes DONE available to polling. */
        instance->IIER |= done;
        *control |= I2C_IDTR_ENABLE_Msk;
    } else {
        *control &= ~I2C_IDTR_ENABLE_Msk;
    }
    i2c_release(hi2c);
    return NEON_OK;
}

/** @copydoc neon_i2c_is_dma_complete */
neon_status_t neon_i2c_is_dma_complete(const neon_i2c_handle_t *hi2c, neon_i2c_dma_direction_t direction,
                                     uint8_t *complete) {
    if (i2c_is_valid_handle(hi2c) == 0U || fu2_i2c_owner != hi2c || complete == NULL ||
        (uint32_t)direction > NEON_I2C_DMA_RX) {
        return NEON_INVALID_PARAM;
    }
    uint32_t done = direction == NEON_I2C_DMA_TX ?
        NEON_I2C_INTERRUPT_DMA_TX_DONE : NEON_I2C_INTERRUPT_DMA_RX_DONE;
    uint32_t busy = direction == NEON_I2C_DMA_TX ? I2C_ISR_DMA_BUSY_TX_Msk : I2C_ISR_DMA_BUSY_RX_Msk;
    const uint32_t result = (hi2c->instance->IITR & done) != 0U && (hi2c->instance->ISR & busy) == 0U;
    memcpy(complete, &result, 1U);
    return NEON_OK;
}

/** @copydoc neon_i2c_request_bus_clear */
neon_status_t neon_i2c_request_bus_clear(neon_i2c_handle_t *hi2c) {
    neon_status_t status;

    if (i2c_is_valid_handle(hi2c) == 0U) {
        return NEON_INVALID_PARAM;
    }
    if (i2c_active_config.mode != NEON_I2C_MODE_MASTER) {
        return NEON_NOT_SUPPORTED;
    }
    status = i2c_acquire(hi2c, NEON_I2C_STATE_BUSY_CONFIG);
    if (status != NEON_OK) {
        return status;
    }
    hi2c->instance->IBCER |= I2C_IBCER_BUS_CLR_Msk;
    i2c_release(hi2c);
    return NEON_OK;
}

/** @copydoc neon_i2c_generate_stop */
neon_status_t neon_i2c_generate_stop(neon_i2c_handle_t *hi2c) {
    neon_status_t status;
    if ((i2c_is_valid_handle(hi2c) == 0U) || (fu2_i2c_owner != hi2c) || (hi2c->state == NEON_I2C_STATE_RESET)) {
        return NEON_INVALID_PARAM;
    }
    if (i2c_active_config.mode != NEON_I2C_MODE_MASTER) {
        return NEON_NOT_SUPPORTED;
    }
    status = i2c_acquire(hi2c, NEON_I2C_STATE_BUSY_CONFIG);
    if (status != NEON_OK) {
        return status;
    }
    hi2c->instance->ICTLR |= I2C_ICTLR_STOP_Msk;
    i2c_release(hi2c);
    return NEON_OK;
}

/** @copydoc neon_i2c_configure_smbus */
neon_status_t neon_i2c_configure_smbus(neon_i2c_handle_t *hi2c, const neon_i2c_smbus_config_t *config) {
    neon_status_t status;
    uint32_t value = 0U;

    if ((config == NULL) || (config->enable > 1U) || (config->pec_enable > 1U) || (config->spike_filter_enable > 1U) ||
        (config->quick_command_enable > 1U) || (config->alert_enable > 1U)) {
        return NEON_INVALID_PARAM;
    }
    status = i2c_acquire(hi2c, NEON_I2C_STATE_BUSY_CONFIG);
    if (status != NEON_OK) {
        return status;
    }
    if (config->enable != 0U) {
        value |= I2C_ISMBR_SMB_EN_Msk;
    }
    if (config->pec_enable != 0U) {
        value |= I2C_ISMBR_PEC_EN_Msk;
    }
    if (config->spike_filter_enable != 0U) {
        value |= I2C_ISMBR_SF_EN_Msk;
    }
    if (config->quick_command_enable != 0U) {
        value |= I2C_ISMBR_QCMD_EN_Msk;
    }
    if (config->alert_enable != 0U) {
        value |= I2C_ISMBR_ALERT_EN_Msk;
    }
    hi2c->instance->ISMBR = value;
    i2c_release(hi2c);
    return NEON_OK;
}

/** @copydoc neon_i2c_clear_smbus_flags */
neon_status_t neon_i2c_clear_smbus_flags(neon_i2c_handle_t *hi2c, uint8_t clear_alert, uint8_t clear_pec) {
    neon_status_t status;
    uint32_t clear_mask = 0U;
    uint32_t config;

    if ((i2c_is_valid_handle(hi2c) == 0U) || (clear_alert > 1U) || (clear_pec > 1U) ||
        ((clear_alert == 0U) && (clear_pec == 0U)) || (fu2_i2c_owner != hi2c) ||
        (hi2c->state == NEON_I2C_STATE_RESET)) {
        return NEON_INVALID_PARAM;
    }
    status = i2c_acquire(hi2c, NEON_I2C_STATE_BUSY_CONFIG);
    if (status != NEON_OK) {
        return status;
    }
    if (clear_alert != 0U) {
        clear_mask |= I2C_ISMBR_ALERT_DEC_CLR_Msk;
    }
    if (clear_pec != 0U) {
        clear_mask |= I2C_ISMBR_PEC_ERR_CLR_Msk;
    }
    config = hi2c->instance->ISMBR & ~(I2C_ISMBR_ALERT_DEC_CLR_Msk | I2C_ISMBR_PEC_ERR_CLR_Msk);
    hi2c->instance->ISMBR = config | clear_mask;
    hi2c->instance->ISMBR = config;
    i2c_pulse_interrupt_clear(hi2c->instance, ((clear_alert != 0U) ? I2C_IICR_ALERT_DEC_CLR_Msk : 0U) |
                                                  ((clear_pec != 0U) ? I2C_IICR_PEC_ERR_CLR_Msk : 0U));
    i2c_release(hi2c);
    return NEON_OK;
}

/** @copydoc neon_i2c_set_device_id */
neon_status_t neon_i2c_set_device_id(neon_i2c_handle_t *hi2c, uint32_t device_id) {
    neon_status_t status;

    if ((device_id & ~I2C_ISDIR_DEV_ID_Msk) != 0U) {
        return NEON_INVALID_PARAM;
    }
    if (i2c_is_valid_handle(hi2c) == 0U) {
        return NEON_INVALID_PARAM;
    }
    if (i2c_active_config.mode != NEON_I2C_MODE_SLAVE) {
        return NEON_NOT_SUPPORTED;
    }
    status = i2c_acquire(hi2c, NEON_I2C_STATE_BUSY_CONFIG);
    if (status != NEON_OK) {
        return status;
    }
    hi2c->instance->ISDIR = device_id & I2C_ISDIR_DEV_ID_Msk;
    i2c_release(hi2c);
    return NEON_OK;
}

/** @copydoc neon_i2c_enable_device_id_read */
neon_status_t neon_i2c_enable_device_id_read(neon_i2c_handle_t *hi2c, uint8_t enable) {
    neon_status_t status;

    if (enable > 1U) {
        return NEON_INVALID_PARAM;
    }
    if (i2c_is_valid_handle(hi2c) == 0U) {
        return NEON_INVALID_PARAM;
    }
    if (i2c_active_config.mode != NEON_I2C_MODE_MASTER) {
        return NEON_NOT_SUPPORTED;
    }
    status = i2c_acquire(hi2c, NEON_I2C_STATE_BUSY_CONFIG);
    if (status != NEON_OK) {
        return status;
    }
    hi2c->instance->IDIER = (enable != 0U) ? I2C_IDIER_DEV_ID_Msk : 0U;
    i2c_release(hi2c);
    return NEON_OK;
}

/** @copydoc neon_i2c_enable_general_call_reset */
neon_status_t neon_i2c_enable_general_call_reset(neon_i2c_handle_t *hi2c, uint8_t enable) {
    neon_status_t status;

    if (enable > 1U) {
        return NEON_INVALID_PARAM;
    }
    if (i2c_is_valid_handle(hi2c) == 0U) {
        return NEON_INVALID_PARAM;
    }
    status = i2c_acquire(hi2c, NEON_I2C_STATE_BUSY_CONFIG);
    if (status != NEON_OK) {
        return status;
    }
    hi2c->instance->ISRER = (enable != 0U) ? I2C_ISRER_SW_RESET_Msk : 0U;
    i2c_release(hi2c);
    return NEON_OK;
}

/** @copydoc neon_i2c_get_clock_status */
neon_status_t neon_i2c_get_clock_status(neon_i2c_handle_t *hi2c, neon_i2c_clock_status_t *status) {
    uint32_t snapshot;

    if ((i2c_is_valid_handle(hi2c) == 0U) || (status == NULL) || (fu2_i2c_owner != hi2c) ||
        (hi2c->state == NEON_I2C_STATE_RESET)) {
        return NEON_INVALID_PARAM;
    }

    snapshot = hi2c->instance->ICLFT;
    status->clock_failure = ((snapshot & I2C_ICLFT_CLK_FAILURE_Msk) != 0U) ? 1U : 0U;
    status->failure_address = (uint16_t)((snapshot & I2C_ICLFT_ADDR_FAILURE_Msk) >> I2C_ICLFT_ADDR_FAILURE_Pos);
    status->failed_direction =
        ((snapshot & I2C_ICLFT_CMD_FAILURE_Msk) != 0U) ? NEON_I2C_DIRECTION_READ : NEON_I2C_DIRECTION_WRITE;
    if (status->clock_failure != 0U) {
        hi2c->error_code |= NEON_I2C_ERROR_CLOCK;
    }
    return NEON_OK;
}

/*==================================================================================================
*                                        PRIVATE FUNCTIONS
==================================================================================================*/

static neon_status_t i2c_apply_config(neon_i2c_handle_t *hi2c, const neon_i2c_config_t *settings) {
    neon_status_t status;
    uint32_t primask;
    uint32_t config;

    if ((i2c_is_valid_handle(hi2c) == 0U) || (i2c_is_valid_config(settings) == 0U)) {
        return NEON_INVALID_PARAM;
    }

    if (settings->pin_config == NEON_I2C_PIN_CONFIG_AUTO) {
        status = i2c_validate_pin_route();
        if (status != NEON_OK) {
            return status;
        }
    }

    primask = i2c_enter_critical();
    if ((fu2_i2c_owner != NULL) || (hi2c->state != NEON_I2C_STATE_INITIALIZED)) {
        i2c_exit_critical(primask);
        return NEON_BUSY;
    }
    fu2_i2c_owner = hi2c;
    memcpy(&i2c_active_config, settings, sizeof(*settings));
    hi2c->state = NEON_I2C_STATE_BUSY_CONFIG;
    hi2c->error_code = NEON_I2C_ERROR_NONE;
    hi2c->private_context = 0U;
    i2c_exit_critical(primask);

    status = neon_crm_peripheral_clock_enable(NEON_CRM_PERIPHERAL_I2C0);
    if (status == NEON_OK) {
        status = neon_crm_peripheral_reset(NEON_CRM_PERIPHERAL_I2C0);
    }
    if ((status == NEON_OK) && (settings->pin_config == NEON_I2C_PIN_CONFIG_AUTO)) {
        status = i2c_configure_pin_route();
        if (status == NEON_OK) {
            hi2c->private_context |= I2C_CONTEXT_PIN_AUTO;
        }
    }
    if (status != NEON_OK) {
        (void)neon_crm_peripheral_clock_disable(NEON_CRM_PERIPHERAL_I2C0);
        primask = i2c_enter_critical();
        fu2_i2c_owner = NULL;
        hi2c->state = NEON_I2C_STATE_RESET;
        i2c_exit_critical(primask);
        return status;
    }

    hi2c->instance->IER = 0U;
    config = ((uint32_t)settings->speed_mode << I2C_ICFGR_SPEED_MODE_Pos) & I2C_ICFGR_SPEED_MODE_Msk;
    if (settings->mode == NEON_I2C_MODE_MASTER) {
        config |= I2C_ICFGR_MASTER_SLAVE_Msk;
    }
    if (settings->addressing_mode == NEON_I2C_ADDRESSING_10BIT) {
        config |= I2C_ICFGR_ADDR_MODE_Msk;
    }
    if (settings->clock_stretch == NEON_I2C_CLOCK_STRETCH_ENABLE) {
        config |= I2C_ICFGR_STRETCH_EN_Msk;
    }
    hi2c->instance->ICFGR = config;
    hi2c->instance->ITSAR = ((uint32_t)settings->own_address << I2C_ITSAR_SLAVE_ADDR_Pos) & I2C_ITSAR_SLAVE_ADDR_Msk;
    hi2c->instance->ISPTR = settings->timing.spike_count;
    hi2c->instance->ISLTR = ((uint32_t)settings->timing.scl_low_count << I2C_ISLTR_SCL_LOW_CNT_Pos) |
                            ((uint32_t)settings->timing.scl_low_count_high_speed << I2C_ISLTR_SCL_LOW_CNT_HS_Pos);
    hi2c->instance->ISHTR = ((uint32_t)settings->timing.scl_high_count << I2C_ISHTR_SCL_HIGH_CNT_Pos) |
                            ((uint32_t)settings->timing.scl_high_count_high_speed << I2C_ISHTR_SCL_HIGH_CNT_HS_Pos);
    hi2c->instance->ISSTR = settings->timing.start_setup_count;
    hi2c->instance->ISTHTR = settings->timing.start_hold_count;
    hi2c->instance->ISPSTR = settings->timing.stop_setup_count;
    hi2c->instance->ISDHTR = settings->timing.sda_hold_count;
    hi2c->instance->ITTR = settings->timing.bus_free_count;
    hi2c->instance->ISCSLTR = settings->timing.scl_stuck_low_count;
    hi2c->instance->ISDSLTR = settings->timing.sda_stuck_low_count;
    hi2c->instance->ITFTLR = ((uint32_t)settings->fifo.tx_full_level << I2C_ITFTLR_PROG_FLVL_Pos) |
                             ((uint32_t)settings->fifo.tx_empty_level << I2C_ITFTLR_PROG_ELVL_Pos);
    hi2c->instance->IRFTLR = ((uint32_t)settings->fifo.rx_full_level << I2C_IRFTLR_PROG_FLVL_Pos) |
                             ((uint32_t)settings->fifo.rx_empty_level << I2C_IRFTLR_PROG_ELVL_Pos);
    hi2c->instance->IIER = 0U;
    hi2c->instance->IICR = 0U;
    hi2c->instance->IBCER = 0U;
    hi2c->instance->IDIER = 0U;
    hi2c->instance->ISRER = 0U;
    hi2c->instance->IDTR = 0U;
    hi2c->instance->IDRR = 0U;
    hi2c->instance->ISDIR = 0U;
    hi2c->instance->IIOCR = 0U;
    hi2c->instance->ISMBR = 0U;
    hi2c->instance->IER = I2C_IER_ENABLE_Msk;

    primask = i2c_enter_critical();
    hi2c->state = NEON_I2C_STATE_READY;
    i2c_exit_critical(primask);
    return NEON_OK;
}

static uint32_t i2c_enter_critical(void) {
    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    return primask;
}

static void i2c_exit_critical(uint32_t primask) {
    __set_PRIMASK(primask);
}

static uint8_t i2c_is_valid_handle(const neon_i2c_handle_t *hi2c) {
    return ((hi2c != NULL) && (hi2c->instance == I2C0)) ? 1U : 0U;
}

static uint8_t i2c_is_valid_address(const neon_i2c_handle_t *hi2c, uint16_t address) {
    if (i2c_active_config.addressing_mode == NEON_I2C_ADDRESSING_10BIT) {
        return (address <= 0x03FFU) ? 1U : 0U;
    }
    return (address <= 0x007FU) ? 1U : 0U;
}

static uint8_t i2c_is_valid_config(const neon_i2c_config_t *config) {
    if ((config == NULL) || ((uint32_t)config->mode > NEON_I2C_MODE_MASTER) ||
        ((uint32_t)config->speed_mode > NEON_I2C_SPEED_ULTRA_FAST) ||
        ((uint32_t)config->addressing_mode > NEON_I2C_ADDRESSING_10BIT) ||
        ((uint32_t)config->clock_stretch > NEON_I2C_CLOCK_STRETCH_ENABLE) ||
        ((uint32_t)config->pin_config > NEON_I2C_PIN_CONFIG_KEEP) || (config->timing.scl_low_count == 0U) ||
        (config->timing.scl_high_count == 0U) || (config->timing.scl_low_count > 0xFFFFU) ||
        (config->timing.scl_high_count > 0xFFFFU) || (config->timing.scl_low_count_high_speed > 0xFFFFU) ||
        (config->timing.scl_high_count_high_speed > 0xFFFFU) || (config->timing.spike_count > 0xFFFFU) ||
        (config->timing.start_setup_count > 0xFFFFU) || (config->timing.start_hold_count > 0xFFFFU) ||
        (config->timing.stop_setup_count > 0xFFFFU) || (config->timing.sda_hold_count > 0xFFFFU) ||
        (config->timing.bus_free_count > 0xFFFFU) || (config->timing.scl_stuck_low_count > 0xFFFFU) ||
        (config->timing.sda_stuck_low_count > 0xFFFFU) || (config->fifo.tx_full_level > 0x0FU) ||
        (config->fifo.tx_empty_level > 0x0FU) || (config->fifo.rx_full_level > 0x0FU) ||
        (config->fifo.rx_empty_level > 0x0FU)) {
        return 0U;
    }

    if (config->mode == NEON_I2C_MODE_SLAVE) {
        if (((config->addressing_mode == NEON_I2C_ADDRESSING_7BIT) && (config->own_address > 0x007FU)) ||
            ((config->addressing_mode == NEON_I2C_ADDRESSING_10BIT) && (config->own_address > 0x03FFU))) {
            return 0U;
        }
    }
    if ((config->speed_mode == NEON_I2C_SPEED_HIGH) &&
        ((config->timing.scl_low_count_high_speed == 0U) || (config->timing.scl_high_count_high_speed == 0U))) {
        return 0U;
    }
    return 1U;
}

static neon_status_t i2c_validate_pin_route(void) {
    fu2_syscsr_physical_pad_map_t pad_map;
    fu2_syscsr_pin_function_map_t function_map;
    neon_status_t status;
    uint32_t index;
    uint8_t found;

    status = fu2_syscsr_get_physical_pad_map(SYSCSR_PIN_18, &pad_map);
    if (status != NEON_OK) {
        return status;
    }
    status = fu2_syscsr_get_pin_function_map(SYSCSR_PIN_18, &function_map);
    if (status != NEON_OK) {
        return status;
    }
    found = 0U;
    for (index = 0U; index < 4U; index++) {
        if (function_map.mux_function[index] == SYSCSR_PIN_FUNCTION_I2C0_SDA) {
            found = 1U;
        }
    }
    if (found == 0U) {
        return NEON_NOT_MAPPED;
    }

    status = fu2_syscsr_get_physical_pad_map(SYSCSR_PIN_19, &pad_map);
    if (status != NEON_OK) {
        return status;
    }
    status = fu2_syscsr_get_pin_function_map(SYSCSR_PIN_19, &function_map);
    if (status != NEON_OK) {
        return status;
    }
    found = 0U;
    for (index = 0U; index < 4U; index++) {
        if (function_map.mux_function[index] == SYSCSR_PIN_FUNCTION_I2C0_SCL) {
            found = 1U;
        }
    }
    return (found != 0U) ? NEON_OK : NEON_NOT_MAPPED;
}

static neon_status_t i2c_configure_pin_route(void) {
    static const fu2_syscsr_pin_route_t routes[] = {
        {SYSCSR_PIN_18, SYSCSR_PIN_FUNCTION_I2C0_SDA},
        {SYSCSR_PIN_19, SYSCSR_PIN_FUNCTION_I2C0_SCL},
    };
    /* SDA/SCL need input feedback and open-drain; use external bus pull-ups. */
    return fu2_syscsr_configure_pins(&i2c_pins, routes, 2U, 1U);
}

static neon_status_t i2c_reset_pin_route(void) {
    fu2_syscsr_release_pins(&i2c_pins);
    return NEON_OK;
}

static void i2c_pulse_interrupt_clear(I2C_TypeDef *instance, uint32_t interrupts) {
    uint32_t primask = i2c_enter_critical();
    /* All valid bits of the IICR are clear pulses, with no field configuration to retain. DTI requires
   * write 1 and returns 0. */
    instance->IICR = interrupts & I2C_INTERRUPT_MASK;
    instance->IICR = 0U;
    i2c_exit_critical(primask);
}

static uint32_t i2c_error_to_clear_mask(uint32_t status) {
    uint32_t clear_mask = 0U;

    if ((status & I2C_ISR_ARB_LOST_Msk) != 0U) {
        clear_mask |= I2C_IICR_ARB_LOST_CLR_Msk;
    }
    if ((status & I2C_ISR_SCL_STUCK_LOW_Msk) != 0U) {
        clear_mask |= I2C_IICR_SCL_STUCK_LOW_CLR_Msk;
    }
    if ((status & I2C_ISR_SDA_STUCK_LOW_Msk) != 0U) {
        clear_mask |= I2C_IICR_SDA_STUCK_LOW_CLR_Msk;
    }
    return clear_mask;
}

static uint32_t i2c_error_code(uint32_t status) {
    uint32_t error = NEON_I2C_ERROR_NONE;

    if ((status & I2C_ISR_ARB_LOST_Msk) != 0U) {
        error |= NEON_I2C_ERROR_ARBITRATION_LOST;
    }
    if ((status & I2C_ISR_SCL_STUCK_LOW_Msk) != 0U) {
        error |= NEON_I2C_ERROR_SCL_STUCK_LOW;
    }
    if ((status & I2C_ISR_SDA_STUCK_LOW_Msk) != 0U) {
        error |= NEON_I2C_ERROR_SDA_STUCK_LOW;
    }
    return error;
}

static uint32_t i2c_logical_flags(const I2C_TypeDef *instance) {
    uint32_t raw = instance->ISR;
    uint32_t interrupt = instance->IITR;
    /* Keep public logical flag values while translating the hardware ISR. */
    uint32_t flags = (raw & 7U) | ((raw & 0x3FF8U) << 2);
    if ((interrupt & I2C_IITR_TRANSFER_DONE_Msk) != 0U) { flags |= NEON_I2C_FLAG_TRANSFER_DONE; }
    if ((interrupt & I2C_IITR_ADDR_NACK_Msk) != 0U) { flags |= NEON_I2C_FLAG_ADDRESS_NACK; }
    if ((interrupt & I2C_IITR_DATA_NACK_Msk) != 0U) { flags |= NEON_I2C_FLAG_DATA_NACK; }
    return flags;
}

static neon_status_t i2c_check_transfer_error(neon_i2c_handle_t *hi2c) {
    uint32_t status = hi2c->instance->ISR;
    uint32_t interrupt = hi2c->instance->IITR;
    uint32_t errors = status & I2C_TRANSFER_ERROR_FLAGS;
    uint32_t clear = i2c_error_to_clear_mask(errors);
    uint32_t code = i2c_error_code(errors);
    if ((interrupt & I2C_IITR_ADDR_NACK_Msk) != 0U) {
        code |= NEON_I2C_ERROR_ADDRESS_NACK;
        clear |= I2C_IICR_ADDR_NACK_CLR_Msk;
    }
    if ((interrupt & I2C_IITR_DATA_NACK_Msk) != 0U) {
        code |= NEON_I2C_ERROR_DATA_NACK;
        clear |= I2C_IICR_DATA_NACK_CLR_Msk;
    }
    if ((interrupt & I2C_IITR_PEC_ERR_Msk) != 0U) {
        code |= NEON_I2C_ERROR_PEC;
        clear |= I2C_IICR_PEC_ERR_CLR_Msk;
    }
    if (code == 0U) { return NEON_OK; }
    i2c_error_isr = status;
    i2c_error_iitr = interrupt;
    hi2c->error_code |= code;
    i2c_pulse_interrupt_clear(hi2c->instance, clear);
    return NEON_ERROR;
}

static void i2c_abort_master(neon_i2c_handle_t *hi2c, uint8_t transfer_started) {
    /* Never send STOP to a bus owned by a different master. */
    if ((transfer_started != 0U) && ((hi2c->error_code & NEON_I2C_ERROR_ARBITRATION_LOST) == 0U)) {
        hi2c->instance->ICTLR |= I2C_ICTLR_STOP_Msk;
        /* The transfer budget may already be zero. Cleanup has its own bounded budget. */
        uint32_t recovery = I2C_RECOVERY_POLLS;
        while (((hi2c->instance->ICTLR & (I2C_ICTLR_START_Msk | I2C_ICTLR_STOP_Msk)) != 0U) ||
               ((hi2c->instance->ISR & I2C_ISR_BUS_BUSY_Msk) != 0U)) {
            if (recovery-- == 0U) { i2c_mark_fault(hi2c); return; }
        }
        i2c_flush_fifos(hi2c->instance);
    } else if ((hi2c->instance->ICTLR & (I2C_ICTLR_START_Msk | I2C_ICTLR_STOP_Msk)) != 0U) {
        i2c_mark_fault(hi2c);
    }
}

static void i2c_mark_fault(neon_i2c_handle_t *hi2c) {
    hi2c->error_code |= NEON_I2C_ERROR_RECOVERY_REQUIRED;
    hi2c->state = NEON_I2C_STATE_FAULT;
    hi2c->instance->IIER = 0U;
    if (NVIC_GetEnableIRQ(I2C0_IRQn) != 0U) { hi2c->private_context |= I2C_CONTEXT_RESTORE_IRQ; }
    NVIC_DisableIRQ(I2C0_IRQn);
}

static void i2c_restore_polling_irq(neon_i2c_handle_t *hi2c, uint32_t saved_interrupts,
                                  uint32_t irq_enabled, uint32_t irq_pending) {
    if (hi2c->state == NEON_I2C_STATE_FAULT) {
        if (irq_enabled != 0U) { hi2c->private_context |= I2C_CONTEXT_RESTORE_IRQ; }
        return;
    }
    hi2c->instance->IIER = saved_interrupts;
    if (irq_pending == 0U) { NVIC_ClearPendingIRQ(I2C0_IRQn); }
    if (irq_enabled != 0U) { NVIC_EnableIRQ(I2C0_IRQn); }
}

static neon_status_t i2c_consume_timeout(neon_i2c_handle_t *hi2c, uint32_t *timeout_polls) {
    if (*timeout_polls == 0U) {
        hi2c->error_code |= NEON_I2C_ERROR_TIMEOUT;
        return NEON_TIMEOUT;
    }
    (*timeout_polls)--;
    return NEON_OK;
}

static neon_status_t i2c_wait_bus_idle(neon_i2c_handle_t *hi2c, uint32_t *timeout_polls, uint8_t check_error) {
    neon_status_t status;

    while ((hi2c->instance->ISR & I2C_ISR_BUS_BUSY_Msk) != 0U) {
        if (check_error != 0U) {
            status = i2c_check_transfer_error(hi2c);
            if (status != NEON_OK) {
                return status;
            }
        }
        status = i2c_consume_timeout(hi2c, timeout_polls);
        if (status != NEON_OK) {
            return status;
        }
    }
    return (check_error != 0U) ? i2c_check_transfer_error(hi2c) : NEON_OK;
}

static neon_status_t i2c_wait_start_consumed(neon_i2c_handle_t *hi2c, uint32_t *timeout_polls) {
    neon_status_t status;

    while ((hi2c->instance->ICTLR & I2C_ICTLR_START_Msk) != 0U) {
        status = i2c_check_transfer_error(hi2c);
        if (status != NEON_OK) {
            return status;
        }
        status = i2c_consume_timeout(hi2c, timeout_polls);
        if (status != NEON_OK) {
            return status;
        }
    }
    return i2c_check_transfer_error(hi2c);
}

static neon_status_t i2c_wait_tx_space(neon_i2c_handle_t *hi2c, uint32_t *timeout_polls) {
    neon_status_t status;

    while ((hi2c->instance->ISR & I2C_ISR_TXFF_FULL_Msk) != 0U) {
        status = i2c_check_transfer_error(hi2c);
        if (status != NEON_OK) {
            return status;
        }
        status = i2c_consume_timeout(hi2c, timeout_polls);
        if (status != NEON_OK) {
            return status;
        }
    }
    return i2c_check_transfer_error(hi2c);
}

static neon_status_t i2c_wait_rx_data(neon_i2c_handle_t *hi2c, uint32_t *timeout_polls,
                                     uint32_t *transfer_started) {
    neon_status_t status;

    for (;;) {
        status = i2c_check_transfer_error(hi2c);
        if (status != NEON_OK) {
            return status;
        }
        /* DONE counts bytes received from the bus, not bytes copied to RAM.
         * Release SCL promptly; STOP preserves the unread RX FIFO contents. */
        if (*transfer_started != 0U &&
            (((hi2c->instance->ISR & I2C_ISR_TRANSFER_DONE_Msk) != 0U) ||
             ((hi2c->instance->IITR & I2C_IITR_TRANSFER_DONE_Msk) != 0U))) {
            status = i2c_stop_master(hi2c, timeout_polls, transfer_started);
            if (status != NEON_OK) { return status; }
        }
        if ((hi2c->instance->ISR & I2C_ISR_RXFF_EMPTY_Msk) == 0U) {
            return NEON_OK;
        }
        status = i2c_consume_timeout(hi2c, timeout_polls);
        if (status != NEON_OK) {
            return status;
        }
    }
}

static neon_status_t i2c_wait_transfer_done(neon_i2c_handle_t *hi2c, uint32_t *timeout_polls) {
    neon_status_t status;

    while ((hi2c->instance->ISR & I2C_ISR_TRANSFER_DONE_Msk) == 0U &&
           (hi2c->instance->IITR & I2C_IITR_TRANSFER_DONE_Msk) == 0U) {
        status = i2c_check_transfer_error(hi2c);
        if (status != NEON_OK) {
            return status;
        }
        status = i2c_consume_timeout(hi2c, timeout_polls);
        if (status != NEON_OK) {
            return status;
        }
    }
    return i2c_check_transfer_error(hi2c);
}

static neon_status_t i2c_acquire(neon_i2c_handle_t *hi2c, neon_i2c_state_t state) {
    uint32_t primask;

    if (i2c_is_valid_handle(hi2c) == 0U) {
        return NEON_INVALID_PARAM;
    }

    primask = i2c_enter_critical();
    if (fu2_i2c_owner != hi2c) {
        i2c_exit_critical(primask);
        return NEON_ERROR;
    }
    if (hi2c->state == NEON_I2C_STATE_FAULT) {
        i2c_exit_critical(primask);
        return NEON_ERROR;
    }
    if (hi2c->state != NEON_I2C_STATE_READY) {
        i2c_exit_critical(primask);
        return NEON_BUSY;
    }
    /* DMA requests must be disabled before software touches the FIFOs. */
    if (((state == NEON_I2C_STATE_BUSY_TX) || (state == NEON_I2C_STATE_BUSY_RX)) &&
        (((hi2c->instance->IDTR & I2C_IDTR_ENABLE_Msk) |
          (hi2c->instance->IDRR & I2C_IDRR_ENABLE_Msk)) != 0U ||
         (hi2c->instance->ISR & I2C_DMA_BUSY_MASK) != 0U)) {
        i2c_exit_critical(primask);
        return NEON_BUSY;
    }
    hi2c->state = state;
    i2c_exit_critical(primask);
    return NEON_OK;
}

static void i2c_release(neon_i2c_handle_t *hi2c) {
    uint32_t primask = i2c_enter_critical();
    if (hi2c->state != NEON_I2C_STATE_FAULT) { hi2c->state = NEON_I2C_STATE_READY; }
    i2c_exit_critical(primask);
}

static void i2c_flush_fifos(I2C_TypeDef *instance) {
    /* DTI specifies that turning off the controller will reset the read/write pointer of both FIFOs. */
    instance->IER &= ~I2C_IER_ENABLE_Msk;
    instance->IER |= I2C_IER_ENABLE_Msk;
}

static neon_status_t i2c_prepare_master(neon_i2c_handle_t *hi2c, uint32_t address, uint32_t size,
                                        neon_i2c_direction_t direction, uint32_t use_sub_address, uint32_t sub_address,
                                        uint32_t *timeout_polls, uint32_t *saved_interrupts) {
    neon_status_t status;
    uint32_t config;

    /* A previous timeout_polls can leave START/STOP pending even before BUS_BUSY
   * asserts. Do not overwrite its address/count or flush its FIFO. */
    while ((hi2c->instance->ICTLR & (I2C_ICTLR_START_Msk | I2C_ICTLR_STOP_Msk)) != 0U) {
        status = i2c_consume_timeout(hi2c, timeout_polls);
        if (status != NEON_OK) {
            return status;
        }
    }
    status = i2c_wait_bus_idle(hi2c, timeout_polls, 0U);
    if (status != NEON_OK) {
        return status;
    }

    i2c_flush_fifos(hi2c->instance);
    i2c_pulse_interrupt_clear(hi2c->instance, NEON_I2C_INTERRUPT_ALL);
    *saved_interrupts = hi2c->instance->IIER;
    /* Poll DONE/NACK in IITR and keep the bus until software sends STOP. */
    hi2c->instance->IIER = *saved_interrupts | I2C_IIER_TRANSFER_DONE_EN_Msk |
        I2C_IIER_ADDR_NACK_EN_Msk | I2C_IIER_DATA_NACK_EN_Msk;
    if ((hi2c->instance->ISMBR & I2C_ISMBR_PEC_EN_Msk) != 0U) {
        hi2c->instance->IIER |= I2C_IIER_PEC_ERR_EN_Msk;
    }

    hi2c->instance->ITSAR = ((uint32_t)address << I2C_ITSAR_SLAVE_ADDR_Pos) & I2C_ITSAR_SLAVE_ADDR_Msk;
    hi2c->instance->IBCR = (uint32_t)size & I2C_IBCR_BYTE_COUNT_Msk;
    if (use_sub_address != 0U) {
        hi2c->instance->ISRAR = ((uint32_t)sub_address << I2C_ISRAR_SLAVE_REG_ADDR_Pos) & I2C_ISRAR_SLAVE_REG_ADDR_Msk;
    }

    config = hi2c->instance->ICFGR & ~(I2C_ICFGR_READ_WRITE_Msk | I2C_ICFGR_SUB_ADDR_EN_Msk);
    if (direction == NEON_I2C_DIRECTION_READ) {
        config |= I2C_ICFGR_READ_WRITE_Msk;
    }
    if (use_sub_address != 0U) {
        config |= I2C_ICFGR_SUB_ADDR_EN_Msk;
    }
    hi2c->instance->ICFGR = config & ~I2C_ICFGR_HW_AUTO_END_Msk;
    /* Caller fills TX FIFO before issuing START; RX can start immediately. */
    return NEON_OK;
}

static neon_status_t i2c_stop_master(neon_i2c_handle_t *hi2c, uint32_t *timeout_polls,
                                    uint32_t *transfer_started) {
    hi2c->instance->ICTLR |= I2C_ICTLR_STOP_Msk;
    while ((hi2c->instance->ICTLR & I2C_ICTLR_STOP_Msk) != 0U) {
        neon_status_t status = i2c_consume_timeout(hi2c, timeout_polls);
        if (status != NEON_OK) { return status; }
    }
    *transfer_started = 0U;
    return i2c_wait_bus_idle(hi2c, timeout_polls, 1U);
}

static neon_status_t i2c_finish_polling_transfer(neon_i2c_handle_t *hi2c, uint32_t *timeout_polls,
                                                 uint32_t *transfer_started) {
    neon_status_t status = NEON_OK;

    /* RX may have already stopped on DONE, then drained the remaining FIFO. */
    if (*transfer_started != 0U) {
        status = i2c_wait_transfer_done(hi2c, timeout_polls);
        if (status == NEON_OK) {
            status = i2c_stop_master(hi2c, timeout_polls, transfer_started);
        }
    }
    i2c_pulse_interrupt_clear(hi2c->instance, I2C_IICR_TRANSFER_DONE_CLR_Msk);
    return status;
}

static neon_status_t i2c_master_write_core(neon_i2c_handle_t *hi2c, uint32_t address, uint32_t prefix_valid,
                                           uint32_t prefix, const uint8_t *data, uint32_t data_size,
                                           uint32_t timeout_polls) {
    neon_status_t status;
    uint32_t saved_interrupts = hi2c->instance->IIER;
    uint32_t saved_auto_end = hi2c->instance->ICFGR & I2C_ICFGR_HW_AUTO_END_Msk;
    uint32_t irq_enabled = NVIC_GetEnableIRQ(I2C0_IRQn);
    uint32_t irq_pending = NVIC_GetPendingIRQ(I2C0_IRQn);
    /* IITR events must remain enabled for polling without invoking a user ISR. */
    NVIC_DisableIRQ(I2C0_IRQn);
    uint16_t index = 0U;
    uint16_t total_size = (uint16_t)(data_size + ((prefix_valid != 0U) ? 1U : 0U));
    uint32_t transfer_started = 0U;

    status = i2c_prepare_master(hi2c, address, total_size, NEON_I2C_DIRECTION_WRITE, 0U, 0U, &timeout_polls,
                                &saved_interrupts);
    if ((status == NEON_OK) && (prefix_valid != 0U)) {
        status = i2c_wait_tx_space(hi2c, &timeout_polls);
        if (status == NEON_OK) {
            hi2c->instance->ITDR = prefix;
        }
    }

    /* Queue the prefix and up to one FIFO of data before the bus can consume it. */
    uint32_t prefill_size = NEON_I2C_FIFO_DEPTH - (prefix_valid != 0U ? 1U : 0U);
    if (prefill_size > data_size) { prefill_size = data_size; }
    for (; (index < prefill_size) && (status == NEON_OK); index++) {
        status = i2c_wait_tx_space(hi2c, &timeout_polls);
        if (status == NEON_OK) {
            uint32_t data_word = 0U;
            memcpy(&data_word, &data[index], sizeof(data[index]));
            hi2c->instance->ITDR = data_word;
        }
    }
    if (status == NEON_OK) {
        hi2c->instance->ICTLR |= I2C_ICTLR_START_Msk;
        transfer_started = 1U;
        status = i2c_wait_start_consumed(hi2c, &timeout_polls);
    }
    for (; (index < data_size) && (status == NEON_OK); index++) {
        status = i2c_wait_tx_space(hi2c, &timeout_polls);
        if (status == NEON_OK) {
            uint32_t data_word = 0U;
            memcpy(&data_word, &data[index], sizeof(data[index]));
            hi2c->instance->ITDR = data_word;
        }
    }
    if (status == NEON_OK) {
        status = i2c_finish_polling_transfer(hi2c, &timeout_polls, &transfer_started);
    }
    if (status != NEON_OK) {
        i2c_abort_master(hi2c, transfer_started);
    }
    if (hi2c->state != NEON_I2C_STATE_FAULT) {
        hi2c->instance->ICFGR = (hi2c->instance->ICFGR & ~I2C_ICFGR_HW_AUTO_END_Msk) | saved_auto_end;
    }
    i2c_restore_polling_irq(hi2c, saved_interrupts, irq_enabled, irq_pending);
    return status;
}

static neon_status_t i2c_master_read_core(neon_i2c_handle_t *hi2c, uint32_t address, uint32_t use_sub_address,
                                          uint32_t sub_address, uint8_t *data, uint32_t size, uint32_t timeout_polls) {
    neon_status_t status;
    uint32_t saved_interrupts = hi2c->instance->IIER;
    uint32_t saved_auto_end = hi2c->instance->ICFGR & I2C_ICFGR_HW_AUTO_END_Msk;
    uint32_t irq_enabled = NVIC_GetEnableIRQ(I2C0_IRQn);
    uint32_t irq_pending = NVIC_GetPendingIRQ(I2C0_IRQn);
    /* IITR events must remain enabled for polling without invoking a user ISR. */
    NVIC_DisableIRQ(I2C0_IRQn);
    uint16_t index = 0U;
    uint32_t transfer_started = 0U;
    uint32_t failed_ctrl = 0U, failed_cfg = 0U, failed_count = 0U;
    i2c_error_isr = 0U;
    i2c_error_iitr = 0U;

    status = i2c_prepare_master(hi2c, address, size, NEON_I2C_DIRECTION_READ, use_sub_address, sub_address,
                                &timeout_polls, &saved_interrupts);
    if (status == NEON_OK) {
        hi2c->instance->ICTLR |= I2C_ICTLR_START_Msk;
        transfer_started = 1U;
        status = i2c_wait_start_consumed(hi2c, &timeout_polls);
    }
    while ((index < size) && (status == NEON_OK)) {
        status = i2c_wait_rx_data(hi2c, &timeout_polls, &transfer_started);
        if (status == NEON_OK) {
            const uint32_t result_word = (uint8_t)(hi2c->instance->IRDR & I2C_IRDR_RX_DATA_Msk);
            memcpy(&data[index], &result_word, 1U);
            ++index;
        }
    }
    if (status == NEON_OK) {
        status = i2c_finish_polling_transfer(hi2c, &timeout_polls, &transfer_started);
    }
    if (status != NEON_OK) {
        if (i2c_error_isr == 0U && i2c_error_iitr == 0U) {
            i2c_error_isr = hi2c->instance->ISR;
            i2c_error_iitr = hi2c->instance->IITR;
        }
        failed_ctrl = hi2c->instance->ICTLR;
        failed_cfg = hi2c->instance->ICFGR;
        failed_count = hi2c->instance->IBCR;
        i2c_abort_master(hi2c, transfer_started);
    }
    if (hi2c->state != NEON_I2C_STATE_FAULT) {
        hi2c->instance->ICFGR = (hi2c->instance->ICFGR & ~I2C_ICFGR_HW_AUTO_END_Msk) | saved_auto_end;
    }
    i2c_restore_polling_irq(hi2c, saved_interrupts, irq_enabled, irq_pending);
    /* Print only after STOP/recovery so UART logging does not delay FIFO service. */
    if (status != NEON_OK && (hi2c->error_code & ~NEON_I2C_ERROR_ADDRESS_NACK) != 0U) {
        printf("[I2C RX] received=%lu/%lu ISR=%08lX IITR=%08lX CTRL=%08lX\n",
            (unsigned long)index, (unsigned long)size, (unsigned long)i2c_error_isr,
            (unsigned long)i2c_error_iitr, (unsigned long)failed_ctrl);
        printf("[I2C RX] CFG=%08lX COUNT=%lu SCL_LIMIT=%lu SDA_LIMIT=%lu\n",
            (unsigned long)failed_cfg, (unsigned long)failed_count,
            (unsigned long)hi2c->instance->ISCSLTR, (unsigned long)hi2c->instance->ISDSLTR);
    }
    return status;
}

static void i2c_prepare_slave(neon_i2c_handle_t *hi2c, uint32_t *saved_interrupts) {
    i2c_flush_fifos(hi2c->instance);
    i2c_pulse_interrupt_clear(hi2c->instance, NEON_I2C_INTERRUPT_ALL);
    *saved_interrupts = hi2c->instance->IIER;
    hi2c->instance->IIER = *saved_interrupts | I2C_IIER_STOP_DEC_EN_Msk;
    if ((hi2c->instance->ISMBR & I2C_ISMBR_PEC_EN_Msk) != 0U) {
        hi2c->instance->IIER |= I2C_IIER_PEC_ERR_EN_Msk;
    }
}

/* A slave cannot issue STOP. Only reset/reconfigure can recover a stuck active bus. */
static neon_status_t i2c_finish_slave(neon_i2c_handle_t *hi2c) {
    uint32_t recovery = I2C_RECOVERY_POLLS;
    while ((hi2c->instance->ISR & I2C_ISR_BUS_BUSY_Msk) != 0U) {
        if (recovery-- == 0U) {
            hi2c->error_code |= NEON_I2C_ERROR_TIMEOUT;
            i2c_mark_fault(hi2c);
            return NEON_TIMEOUT;
        }
    }
    i2c_flush_fifos(hi2c->instance);
    i2c_pulse_interrupt_clear(hi2c->instance, I2C_IICR_STOP_DEC_CLR_Msk);
    return NEON_OK;
}
