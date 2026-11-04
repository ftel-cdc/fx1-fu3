/**
 * @file neon_qspi.c
 * @brief Implementation for QSPI.
 * @details
 * Implements module behavior and target-specific integration.
 */

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include "qspi/neon_qspi.h"
#include "crm/neon_crm.h"
#include "syscsr/platform/fu2/neon_syscsr_internal.h"

static fu2_syscsr_pin_group_t qspi_pins;
static const fu2_syscsr_pin_route_t qspi_routes[] = {
    {SYSCSR_PIN_68, SYSCSR_PIN_FUNCTION_QSPI_SCK}, {SYSCSR_PIN_72, SYSCSR_PIN_FUNCTION_QSPI_SSN},
    {SYSCSR_PIN_71, SYSCSR_PIN_FUNCTION_QSPI_D0}, {SYSCSR_PIN_70, SYSCSR_PIN_FUNCTION_QSPI_D1},
    {SYSCSR_PIN_69, SYSCSR_PIN_FUNCTION_QSPI_D2}, {SYSCSR_PIN_64, SYSCSR_PIN_FUNCTION_QSPI_D3},
};

/*==================================================================================================
*                                        DEFINES AND MACROS
==================================================================================================*/
/* Bit vị trí/mask THẬT (inc_fu2/fu2_dti_qspi.h). */
#define QSPI_SCLK_DIV_MASK 0x7U

#define QSPI_TREG0_T_SS2CK_POS 0U
#define QSPI_TREG0_T_CK2SS_POS 16U

#define QSPI_CFG_RESET_VALUE 0x00001C30U /* FU2.yaml: mode 0, SDR. */
#define QSPI_CFG_FMODE_POS 4U
#define QSPI_CFG_FMODE_INDIRECT (0x1U << QSPI_CFG_FMODE_POS)
#define QSPI_CFG_FMODE_MASK (0x3U << QSPI_CFG_FMODE_POS)

#define QSPI_CCR_INST_POS 0U
#define QSPI_CCR_INST_MODE_POS 8U
#define QSPI_CCR_ADDR_MODE_POS 10U
#define QSPI_CCR_ADSIZE_POS 12U
#define QSPI_CCR_ADSIZE_24BIT (0x2U << QSPI_CCR_ADSIZE_POS) /* 24-bit địa chỉ - phổ biến nhất cho NOR flash */
#define QSPI_CCR_DCYC_POS 18U
#define QSPI_CCR_DATA_MODE_POS 23U
#define QSPI_CCR_RX_EN (1U << 25)

#define QSPI_STT_BUSY (1U << 0)

/*==================================================================================================
*                                   PRIVATE FUNCTION PROTOTYPES
==================================================================================================*/

static uint32_t qspi_enter_critical(void);
static void qspi_exit_critical(uint32_t primask);

/*==================================================================================================
*                                         GLOBAL FUNCTIONS
==================================================================================================*/

/** @copydoc neon_qspi_get_default_config */
void neon_qspi_get_default_config(neon_qspi_config_t *config) {
    if (config != 0) {
        config->clock_divider = NEON_QSPI_CLK_DIV_2;
        config->pin_config = NEON_QSPI_PIN_CONFIG_AUTO;
    }
}

/** @copydoc neon_qspi_init */
neon_status_t neon_qspi_init(QSPI_TypeDef *instance, const neon_qspi_config_t *init) {
    uint32_t saved_state;

    if (instance != QSPI_APB || init == 0 || init->clock_divider > QSPI_SCLK_DIV_MASK ||
        (uint32_t)init->pin_config > NEON_QSPI_PIN_CONFIG_KEEP) {
        return NEON_INVALID_PARAM;
    }

    neon_status_t status = neon_crm_peripheral_clock_enable(NEON_CRM_PERIPHERAL_QSPI);
    if (status == NEON_OK) {
        status = neon_crm_peripheral_release_reset(NEON_CRM_PERIPHERAL_QSPI);
    }
    if (status != NEON_OK) {
        return status;
    }

    saved_state = qspi_enter_critical();
    /* Phải gọi khi BUSY=0 - khớp fu2_dti_qspi.c::LL_QSPI_Init */
    if (instance->STT & QSPI_STT_BUSY) {
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

    instance->SCLK = init->clock_divider & QSPI_SCLK_DIV_MASK;
    instance->TREG0 = 0; /* SS2CK/CK2SS delay = 0, đơn giản hoá - mở rộng struct config nếu cần chỉnh */

    /* CFG: CPOL=0/CPHA=0/SDR/FMODE=Indirect - đơn giản hoá cho use-case đọc/ghi
   * flash NOR chuẩn (chế độ phổ biến nhất). Mở rộng struct config trước nếu
   * cần CPOL/CPHA/DDR khác (theo guide/DriverImplement.md mục 2). */
    instance->CFG = (QSPI_CFG_RESET_VALUE & ~QSPI_CFG_FMODE_MASK) | QSPI_CFG_FMODE_INDIRECT;
    qspi_exit_critical(saved_state);

    return NEON_OK;
}

/** @copydoc neon_qspi_deinit */
neon_status_t neon_qspi_deinit(QSPI_TypeDef *instance) {
    /* Keep the reset mapping inside the CRM backend; FU2 v8 moved this bit. */
    if (instance != QSPI_APB) {
        return NEON_INVALID_PARAM;
    }

    neon_status_t status = neon_crm_peripheral_reset(NEON_CRM_PERIPHERAL_QSPI);
    if (status == NEON_OK) {
        fu2_syscsr_release_pins(&qspi_pins);
    }
    return status;
}

/** @copydoc neon_qspi_send_command */
neon_status_t neon_qspi_send_command(QSPI_TypeDef *instance, const neon_qspi_command_t *cmd) {
    uint32_t ccr;
    uint32_t saved_state;

    if (instance != QSPI_APB || cmd == 0 || cmd->instruction > 0xFFU || cmd->instruction_lines > NEON_QSPI_LINES_4 ||
        cmd->address_lines > NEON_QSPI_LINES_4 || cmd->data_lines > NEON_QSPI_LINES_4 || cmd->dummy_cycles > 31U ||
        cmd->receive_enable > 1U || (cmd->address_lines != NEON_QSPI_LINES_NONE && cmd->address > 0xFFFFFFU) ||
        (cmd->data_lines != NEON_QSPI_LINES_NONE && cmd->data_length == 0U)) {
        return NEON_INVALID_PARAM;
    }

    saved_state = qspi_enter_critical();
    if ((instance->STT & QSPI_STT_BUSY) != 0U) {
        qspi_exit_critical(saved_state);
        return NEON_BUSY;
    }

    if (cmd->address_lines != NEON_QSPI_LINES_NONE) {
        instance->AR = cmd->address;
    }
    if (cmd->data_lines != NEON_QSPI_LINES_NONE) {
        instance->DLR = cmd->data_length;
    }

    /* Ghi CCR cuối cùng - đây là lệnh THẬT SỰ BẮT ĐẦU transfer ở chế độ
   * Indirect, khớp thứ tự thật trong fu2_dti_qspi.c::LL_QSPI_ConfigCommand
   * (AR/ABR/DLR trước, CCR sau cùng). */
    ccr = ((uint32_t)cmd->instruction << QSPI_CCR_INST_POS) | (cmd->instruction_lines << QSPI_CCR_INST_MODE_POS) |
          (cmd->address_lines << QSPI_CCR_ADDR_MODE_POS) |
          ((cmd->address_lines != NEON_QSPI_LINES_NONE) ? QSPI_CCR_ADSIZE_24BIT : 0U) |
          ((cmd->dummy_cycles & 0x1FU) << QSPI_CCR_DCYC_POS) | (cmd->data_lines << QSPI_CCR_DATA_MODE_POS) |
          (cmd->receive_enable ? QSPI_CCR_RX_EN : 0U);

    instance->CCR = ccr;
    qspi_exit_critical(saved_state);

    return NEON_OK;
}

/** @copydoc neon_qspi_write_data */
neon_status_t neon_qspi_write_data(QSPI_TypeDef *instance, uint32_t data) {
    if (instance != QSPI_APB) {
        return NEON_INVALID_PARAM;
    }
    instance->TX = data;
    return NEON_OK;
}

/** @copydoc neon_qspi_read_data */
uint32_t neon_qspi_read_data(QSPI_TypeDef *instance) {
    return (instance == QSPI_APB) ? instance->RX : 0U;
}

/** @copydoc neon_qspi_is_busy */
uint32_t neon_qspi_is_busy(QSPI_TypeDef *instance) {
    return (instance == QSPI_APB && (instance->STT & QSPI_STT_BUSY)) ? 1U : 0U;
}

/** @copydoc neon_qspi_wait_ready */
neon_status_t neon_qspi_wait_ready(QSPI_TypeDef *instance, uint32_t timeout_polls) {
    uint32_t remaining;

    if (instance != QSPI_APB) {
        return NEON_INVALID_PARAM;
    }

    remaining = timeout_polls;
    while (instance->STT & QSPI_STT_BUSY) {
        if (remaining-- == 0) {
            return NEON_TIMEOUT;
        }
    }
    return NEON_OK;
}

/*==================================================================================================
*                                        PRIVATE FUNCTIONS
==================================================================================================*/

static uint32_t qspi_enter_critical(void) {
    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    return primask;
}

static void qspi_exit_critical(uint32_t primask) {
    __set_PRIMASK(primask);
}
