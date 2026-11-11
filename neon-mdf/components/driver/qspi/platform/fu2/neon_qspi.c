/**
 * @file neon_qspi.c
 * @brief FU2 QSPI SDR commands, FIFO polling and mapped reads.
 * @details
 * Configuration is changed only while idle. CCR starts indirect commands;
 * AHB reads start mapped commands. Abort flushes the controller FIFOs.
 */

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include "qspi/neon_qspi.h"
#include "crm/neon_crm.h"
#include "syscsr/platform/fu2/neon_syscsr_internal.h"

/*==================================================================================================
*                                        DEFINES AND MACROS
==================================================================================================*/
#define QSPI_SCLK_DIV_MASK 0x7U
#define QSPI_CFG_RESET_VALUE 0x00001C30U
#define QSPI_CFG_FMODE_MASK (3U << 4)
#define QSPI_CFG_INDIRECT (1U << 4)
#define QSPI_CFG_MEMORY_MAPPED (3U << 4)
#define QSPI_CFG_DMA_ENABLE (1U << 7)
#define QSPI_CFG_TIMEOUT_ENABLE (1U << 8)
#define QSPI_CFG_SAMPLE_SHIFT (1U << 13)
#define QSPI_CAR_ABORT (1U << 0)
#define QSPI_STT_BUSY (1U << 0)
#define QSPI_RISR_TRANSFER_COMPLETE (1U << 1)
#define QSPI_RISR_TX_FULL (1U << 2)
#define QSPI_RISR_RX_EMPTY (1U << 5)
#define QSPI_RISR_CMD_FULL (1U << 6)
#define QSPI_ICR_CLEAR_ALL 7U

/*==================================================================================================
*                                         LOCAL VARIABLES
==================================================================================================*/
static fu2_syscsr_pin_group_t qspi_pins;
static const fu2_syscsr_pin_route_t qspi_routes[] = {
    {SYSCSR_PIN_68, SYSCSR_PIN_FUNCTION_QSPI_SCK}, {SYSCSR_PIN_72, SYSCSR_PIN_FUNCTION_QSPI_SSN},
    {SYSCSR_PIN_71, SYSCSR_PIN_FUNCTION_QSPI_D0}, {SYSCSR_PIN_70, SYSCSR_PIN_FUNCTION_QSPI_D1},
    {SYSCSR_PIN_69, SYSCSR_PIN_FUNCTION_QSPI_D2}, {SYSCSR_PIN_64, SYSCSR_PIN_FUNCTION_QSPI_D3},
};

/* One command at a time; callers serialize task/interrupt access to this state. */
static uint32_t qspi_pending;
static uint32_t qspi_mapped;
static uint32_t qspi_receive;
static uint32_t qspi_bytes_remaining;
static uint32_t qspi_has_data;
static uint32_t qspi_seen_busy;
static uint32_t qspi_saved_clock;

/*==================================================================================================
*                                   PRIVATE FUNCTION PROTOTYPES
==================================================================================================*/
static uint32_t qspi_enter_critical(void);
static void qspi_exit_critical(uint32_t primask);
static neon_status_t qspi_build_command(const neon_qspi_command_t *cmd, uint32_t mapped, uint32_t *ccr);
static uint32_t qspi_unavailable(QSPI_TypeDef *instance);
static void qspi_finish_command(QSPI_TypeDef *instance);

/*==================================================================================================
*                                         GLOBAL FUNCTIONS
==================================================================================================*/

/** @copydoc neon_qspi_get_default_config */
void neon_qspi_get_default_config(neon_qspi_config_t *config) {
    if (config != 0) {
        *config = (neon_qspi_config_t){0};
        config->clock_divider = NEON_QSPI_CLK_DIV_2;
        config->pin_config = NEON_QSPI_PIN_CONFIG_AUTO;
    }
}

/** @copydoc neon_qspi_init */
neon_status_t neon_qspi_init(QSPI_TypeDef *instance, const neon_qspi_config_t *init) {
    if (instance != QSPI_APB || init == 0 || init->clock_divider > QSPI_SCLK_DIV_MASK ||
        (uint32_t)init->pin_config > NEON_QSPI_PIN_CONFIG_KEEP ||
        (init->clock_mode != NEON_QSPI_CLOCK_MODE_0 && init->clock_mode != NEON_QSPI_CLOCK_MODE_3) ||
        (uint32_t)init->slave_select > NEON_QSPI_SLAVE_SELECT_1 || init->sample_shift > 1U ||
        init->timing.slave_select_to_clock > 0xFFFFU || init->timing.clock_to_slave_select > 0xFFFFU ||
        init->timing.between_commands > 0xFFFFU || init->timing.memory_mapped_timeout > 0xFFFFU) {
        return NEON_INVALID_PARAM;
    }
    if (init->slave_select == NEON_QSPI_SLAVE_SELECT_1 && init->pin_config == NEON_QSPI_PIN_CONFIG_AUTO) {
        return NEON_NOT_MAPPED;
    }

    neon_status_t status = neon_crm_peripheral_clock_enable(NEON_CRM_PERIPHERAL_QSPI);
    if (status == NEON_OK) {
        status = neon_crm_peripheral_release_reset(NEON_CRM_PERIPHERAL_QSPI);
    }
    if (status != NEON_OK) {
        return status;
    }

    uint32_t saved_state = qspi_enter_critical();
    if (qspi_unavailable(instance) || (instance->CFG & QSPI_CFG_DMA_ENABLE)) {
        qspi_exit_critical(saved_state);
        return NEON_BUSY;
    }
    if (init->pin_config == NEON_QSPI_PIN_CONFIG_AUTO) {
        status = fu2_syscsr_configure_pins(&qspi_pins, qspi_routes, 6U, 0U);
    } else {
        fu2_syscsr_release_pins(&qspi_pins);
    }
    if (status != NEON_OK) {
        qspi_exit_critical(saved_state);
        return status;
    }

    instance->SCLK = init->clock_divider;
    instance->SSR = init->slave_select == NEON_QSPI_SLAVE_SELECT_0 ? 2U : 1U;
    instance->TREG0 = init->timing.slave_select_to_clock | (init->timing.clock_to_slave_select << 16);
    instance->TREG1 = init->timing.between_commands;
    /* The upper half contains an unused DLL field; keep it unchanged. */
    instance->TREG2 = (instance->TREG2 & 0xFFFF0000U) | init->timing.memory_mapped_timeout;
    instance->IMR = 0U;
    instance->DMACTRL = 0U;
    instance->ICR = QSPI_ICR_CLEAR_ALL;
    instance->CFG = (QSPI_CFG_RESET_VALUE & ~QSPI_CFG_FMODE_MASK) | QSPI_CFG_INDIRECT |
                    (uint32_t)init->clock_mode | (init->sample_shift ? QSPI_CFG_SAMPLE_SHIFT : 0U) |
                    (init->timing.memory_mapped_timeout ? QSPI_CFG_TIMEOUT_ENABLE : 0U);
    qspi_exit_critical(saved_state);
    return NEON_OK;
}

/** @copydoc neon_qspi_deinit */
neon_status_t neon_qspi_deinit(QSPI_TypeDef *instance) {
    if (instance != QSPI_APB) {
        return NEON_INVALID_PARAM;
    }
    uint32_t saved_state = qspi_enter_critical();
    if (qspi_unavailable(instance) || (instance->CFG & QSPI_CFG_FMODE_MASK) != QSPI_CFG_INDIRECT ||
        (instance->CFG & QSPI_CFG_DMA_ENABLE)) {
        qspi_exit_critical(saved_state);
        return NEON_BUSY;
    }
    neon_status_t status = neon_crm_peripheral_reset(NEON_CRM_PERIPHERAL_QSPI);
    if (status == NEON_OK) {
        fu2_syscsr_release_pins(&qspi_pins);
    }
    qspi_exit_critical(saved_state);
    return status;
}

/** @copydoc neon_qspi_send_command */
neon_status_t neon_qspi_send_command(QSPI_TypeDef *instance, const neon_qspi_command_t *cmd) {
    uint32_t ccr;
    if (instance != QSPI_APB || qspi_build_command(cmd, 0U, &ccr) != NEON_OK) {
        return NEON_INVALID_PARAM;
    }
    uint32_t saved_state = qspi_enter_critical();
    if (qspi_unavailable(instance) || (instance->CFG & QSPI_CFG_FMODE_MASK) != QSPI_CFG_INDIRECT ||
        (instance->RISR & (QSPI_RISR_CMD_FULL | QSPI_RISR_RX_EMPTY)) != QSPI_RISR_RX_EMPTY) {
        qspi_exit_critical(saved_state);
        return NEON_BUSY;
    }
    if (instance->CFG & QSPI_CFG_DMA_ENABLE) {
        qspi_exit_critical(saved_state);
        return NEON_NOT_SUPPORTED;
    }

    instance->ICR = QSPI_RISR_TRANSFER_COMPLETE;
    instance->AR = cmd->address_lines ? cmd->address : 0U;
    instance->ABR = cmd->alternate_byte_lines ? cmd->alternate_bytes : 0U;
    instance->DLR = cmd->data_lines ? cmd->data_length - 1U : 0U;
    qspi_pending = 1U;
    qspi_receive = cmd->receive_enable;
    qspi_has_data = cmd->data_lines != NEON_QSPI_LINES_NONE;
    qspi_bytes_remaining = qspi_has_data ? cmd->data_length : 0U;
    qspi_saved_clock = instance->SCLK;
    if (!qspi_has_data && (qspi_saved_clock & QSPI_SCLK_DIV_MASK) < NEON_QSPI_CLK_DIV_32) {
        /* Keep the short command-only BUSY pulse observable to software. */
        instance->SCLK = (qspi_saved_clock & ~QSPI_SCLK_DIV_MASK) | NEON_QSPI_CLK_DIV_32;
    }
    instance->CCR = ccr;
    qspi_seen_busy = (instance->STT & QSPI_STT_BUSY) != 0U;
    qspi_exit_critical(saved_state);
    if (!qspi_has_data) {
        /* Observe the start now, even if the caller waits much later. Do not
         * hold the interrupt mask during a potentially stalled hardware start. */
        uint32_t remaining = NEON_QSPI_COMMAND_START_TIMEOUT_POLLS;
        while (!qspi_seen_busy && !(instance->RISR & QSPI_RISR_TRANSFER_COMPLETE)) {
            qspi_seen_busy = (instance->STT & QSPI_STT_BUSY) != 0U;
            if (qspi_seen_busy) {
                break;
            }
            if (remaining-- == 0U) {
                return NEON_TIMEOUT;
            }
        }
    }
    return NEON_OK;
}

/** @copydoc neon_qspi_write_data */
neon_status_t neon_qspi_write_data(QSPI_TypeDef *instance, uint32_t data) {
    if (instance != QSPI_APB) {
        return NEON_INVALID_PARAM;
    }
    if (!qspi_pending || qspi_receive || !qspi_bytes_remaining) {
        return NEON_ERROR;
    }
    if (instance->RISR & QSPI_RISR_TX_FULL) {
        return NEON_BUSY;
    }
    instance->TX = data;
    qspi_bytes_remaining -= qspi_bytes_remaining > 4U ? 4U : qspi_bytes_remaining;
    return NEON_OK;
}

/** @copydoc neon_qspi_try_read_data */
neon_status_t neon_qspi_try_read_data(QSPI_TypeDef *instance, uint32_t *data) {
    if (instance != QSPI_APB || data == 0 || ((uintptr_t)data & 3U)) {
        return NEON_INVALID_PARAM;
    }
    if (!qspi_pending || !qspi_receive || !qspi_bytes_remaining) {
        return NEON_ERROR;
    }
    if (instance->RISR & QSPI_RISR_RX_EMPTY) {
        return NEON_BUSY;
    }
    uint32_t value = instance->RX;
    if (qspi_bytes_remaining < 4U) {
        value &= (1U << (8U * qspi_bytes_remaining)) - 1U;
    }
    *data = value;
    qspi_bytes_remaining -= qspi_bytes_remaining > 4U ? 4U : qspi_bytes_remaining;
    return NEON_OK;
}

/** @copydoc neon_qspi_read_data */
uint32_t neon_qspi_read_data(QSPI_TypeDef *instance) {
    uint32_t data = 0U;
    (void)neon_qspi_try_read_data(instance, &data);
    return data;
}

/** @copydoc neon_qspi_transmit_words */
neon_status_t neon_qspi_transmit_words(QSPI_TypeDef *instance, const uint32_t *data, uint32_t timeout_polls) {
    if (instance != QSPI_APB || data == 0 || ((uintptr_t)data & 3U)) {
        return NEON_INVALID_PARAM;
    }
    if (!qspi_pending || qspi_receive || !qspi_bytes_remaining) {
        return NEON_ERROR;
    }
    uint32_t index = 0U, remaining = timeout_polls;
    while (qspi_bytes_remaining) {
        neon_status_t status = neon_qspi_write_data(instance, data[index]);
        if (status == NEON_OK) {
            ++index;
        } else if (status != NEON_BUSY) {
            return status;
        } else if (remaining-- == 0U) {
            return NEON_TIMEOUT;
        }
    }
    return NEON_OK;
}

/** @copydoc neon_qspi_receive_words */
neon_status_t neon_qspi_receive_words(QSPI_TypeDef *instance, uint32_t *data, uint32_t timeout_polls) {
    if (instance != QSPI_APB || data == 0 || ((uintptr_t)data & 3U)) {
        return NEON_INVALID_PARAM;
    }
    if (!qspi_pending || !qspi_receive || !qspi_bytes_remaining) {
        return NEON_ERROR;
    }
    uint32_t index = 0U, remaining = timeout_polls;
    while (qspi_bytes_remaining) {
        neon_status_t status = neon_qspi_try_read_data(instance, &data[index]);
        if (status == NEON_OK) {
            ++index;
        } else if (status != NEON_BUSY) {
            return status;
        } else if (remaining-- == 0U) {
            return NEON_TIMEOUT;
        }
    }
    return NEON_OK;
}

/** @copydoc neon_qspi_is_busy */
uint32_t neon_qspi_is_busy(QSPI_TypeDef *instance) {
    return (instance == QSPI_APB && (instance->STT & QSPI_STT_BUSY)) ? 1U : 0U;
}

/** @copydoc neon_qspi_wait_complete */
neon_status_t neon_qspi_wait_complete(QSPI_TypeDef *instance, uint32_t timeout_polls) {
    if (instance != QSPI_APB) {
        return NEON_INVALID_PARAM;
    }
    if (!qspi_pending) {
        return NEON_ERROR;
    }
    if (qspi_bytes_remaining) {
        return NEON_BUSY;
    }
    uint32_t remaining = timeout_polls;
    for (;;) {
        uint32_t flags = instance->RISR;
        uint32_t busy = instance->STT & QSPI_STT_BUSY;
        qspi_seen_busy |= busy;
        if (!busy && ((flags & QSPI_RISR_TRANSFER_COMPLETE) || (!qspi_has_data && qspi_seen_busy))) {
            qspi_finish_command(instance);
            return NEON_OK;
        }
        if (remaining-- == 0U) {
            return NEON_TIMEOUT;
        }
    }
}

/** @copydoc neon_qspi_wait_ready */
neon_status_t neon_qspi_wait_ready(QSPI_TypeDef *instance, uint32_t timeout_polls) {
    if (instance != QSPI_APB) {
        return NEON_INVALID_PARAM;
    }
    if (qspi_pending) {
        return neon_qspi_wait_complete(instance, timeout_polls);
    }
    uint32_t remaining = timeout_polls;
    while (instance->STT & QSPI_STT_BUSY) {
        if (remaining-- == 0U) {
            return NEON_TIMEOUT;
        }
    }
    return NEON_OK;
}

/** @copydoc neon_qspi_enable_memory_mapped */
neon_status_t neon_qspi_enable_memory_mapped(QSPI_TypeDef *instance, const neon_qspi_command_t *cmd) {
    uint32_t ccr;
    if (instance != QSPI_APB || qspi_build_command(cmd, 1U, &ccr) != NEON_OK) {
        return NEON_INVALID_PARAM;
    }
    uint32_t saved_state = qspi_enter_critical();
    if (qspi_unavailable(instance) || (instance->CFG & QSPI_CFG_FMODE_MASK) != QSPI_CFG_INDIRECT ||
        (instance->RISR & (QSPI_RISR_CMD_FULL | QSPI_RISR_RX_EMPTY)) != QSPI_RISR_RX_EMPTY) {
        qspi_exit_critical(saved_state);
        return NEON_BUSY;
    }
    if (instance->CFG & QSPI_CFG_DMA_ENABLE) {
        qspi_exit_critical(saved_state);
        return NEON_NOT_SUPPORTED;
    }
    instance->ICR = QSPI_RISR_TRANSFER_COMPLETE;
    instance->ABR = cmd->alternate_byte_lines ? cmd->alternate_bytes : 0U;
    /* Switch mode before CCR so this write cannot start an indirect transaction. */
    instance->CFG = (instance->CFG & ~QSPI_CFG_FMODE_MASK) | QSPI_CFG_MEMORY_MAPPED;
    instance->CCR = ccr;
    qspi_mapped = 1U;
    __DSB();
    __ISB();
    qspi_exit_critical(saved_state);
    return NEON_OK;
}

/** @copydoc neon_qspi_abort */
neon_status_t neon_qspi_abort(QSPI_TypeDef *instance, uint32_t timeout_polls) {
    if (instance != QSPI_APB) {
        return NEON_INVALID_PARAM;
    }
    if (instance->CFG & QSPI_CFG_DMA_ENABLE) {
        return NEON_BUSY;
    }
    instance->CAR = QSPI_CAR_ABORT;
    uint32_t remaining = timeout_polls;
    while ((instance->CAR & QSPI_CAR_ABORT) || (instance->STT & QSPI_STT_BUSY)) {
        if (remaining-- == 0U) {
            return NEON_TIMEOUT;
        }
    }
    instance->CFG = (instance->CFG & ~QSPI_CFG_FMODE_MASK) | QSPI_CFG_INDIRECT;
    if (qspi_pending) {
        qspi_finish_command(instance);
    }
    instance->ICR = QSPI_ICR_CLEAR_ALL;
    qspi_bytes_remaining = 0U;
    qspi_mapped = 0U;
    __DSB();
    return NEON_OK;
}

/*==================================================================================================
*                                        PRIVATE FUNCTIONS
==================================================================================================*/

/**
 * @brief Mask CPU interrupts during short configuration updates.
 * @return Previous PRIMASK value.
 */
static uint32_t qspi_enter_critical(void) {
    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    return primask;
}

/**
 * @brief Restore the caller's interrupt mask.
 * @param[in] primask Saved PRIMASK value.
 */
static void qspi_exit_critical(uint32_t primask) {
    __set_PRIMASK(primask);
}

/**
 * @brief Validate the frame and compose CCR without starting a transaction.
 * @param[in] cmd Logical frame settings.
 * @param[in] mapped Nonzero for a memory-mapped read frame.
 * @param[out] ccr Validated command register value.
 * @return NEON_OK or NEON_INVALID_PARAM.
 */
static neon_status_t qspi_build_command(const neon_qspi_command_t *cmd, uint32_t mapped, uint32_t *ccr) {
    if (cmd == 0 || cmd->instruction > 0xFFU || cmd->instruction_lines > NEON_QSPI_LINES_4 ||
        cmd->address_lines > NEON_QSPI_LINES_4 || cmd->alternate_byte_lines > NEON_QSPI_LINES_4 ||
        cmd->data_lines > NEON_QSPI_LINES_4 || cmd->dummy_cycles > 31U || cmd->receive_enable > 1U ||
        cmd->instruction_once > 1U || cmd->address_size > 4U || cmd->alternate_byte_size > 4U ||
        (!cmd->instruction_lines && !cmd->address_lines && !cmd->alternate_byte_lines && !cmd->data_lines) ||
        (mapped && (!cmd->receive_enable || !cmd->data_lines)) ||
        (!mapped && (cmd->instruction_once || (cmd->data_lines && !cmd->data_length))) ||
        (!cmd->data_lines && cmd->receive_enable)) {
        return NEON_INVALID_PARAM;
    }
    uint32_t address_size = cmd->address_size ? cmd->address_size : 3U;
    uint32_t alternate_size = cmd->alternate_byte_size ? cmd->alternate_byte_size : 1U;
    if ((!mapped && cmd->address_lines && address_size < 4U && (cmd->address >> (8U * address_size))) ||
        (cmd->alternate_byte_lines && alternate_size < 4U && (cmd->alternate_bytes >> (8U * alternate_size)))) {
        return NEON_INVALID_PARAM;
    }
    *ccr = cmd->instruction | (cmd->instruction_lines << 8) | (cmd->address_lines << 10) |
           (cmd->address_lines ? (address_size - 1U) << 12 : 0U) |
           (cmd->alternate_byte_lines << 14) |
           (cmd->alternate_byte_lines ? (alternate_size - 1U) << 16 : 0U) |
           (cmd->dummy_cycles << 18) | (cmd->data_lines << 23) |
           (cmd->receive_enable << 25) | (cmd->instruction_once << 26);
    return NEON_OK;
}

/**
 * @brief Check whether reconfiguration would disturb a pending operation.
 * @param[in] instance QSPI_APB.
 * @return Nonzero when a command, mapping or abort is active.
 */
static uint32_t qspi_unavailable(QSPI_TypeDef *instance) {
    return qspi_pending || qspi_mapped || (instance->STT & QSPI_STT_BUSY) || (instance->CAR & QSPI_CAR_ABORT);
}

/**
 * @brief Acknowledge completion and restore the divider after a command-only frame.
 * @param[in] instance QSPI_APB.
 */
static void qspi_finish_command(QSPI_TypeDef *instance) {
    instance->ICR = QSPI_RISR_TRANSFER_COMPLETE;
    if (!qspi_has_data) {
        instance->SCLK = qspi_saved_clock;
    }
    qspi_pending = 0U;
}
