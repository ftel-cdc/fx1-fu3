/**
 * @file neon_flash.c
 * @brief Implementation for Flash.
 * @details
 * GT32U2086 Flash backend for the Common-Spec-v8 VP.
 *
 * The main array is memory mapped: normal stores perform programming and can
 * only change bits from 1 to 0. Erase and initialization use the command
 * engine at FLASH_CTRL + 0x60..0x6C.
 */

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/

#include "flash/neon_flash.h"
#include <string.h>

#include "crm/neon_crm.h"

#include <stddef.h>
#include <stdint.h>

#if defined(__GNUC__)

/*==================================================================================================
*                                        DEFINES AND MACROS
==================================================================================================*/
#define FLASH_RAMFUNC __attribute__((section(".ramfunc"), noinline))
#else
#define FLASH_RAMFUNC
#endif

#define FLASH_REG32(address) (*(volatile uint32_t *)(uintptr_t)(address))

#define FLASH_FLASHC_COMMAND_VALUE(op)                                                                                 \
    ((((uint32_t)(op) << FLASH_COMMAND_CMD_OPCODE_Pos) & FLASH_COMMAND_CMD_OPCODE_Msk) | FLASH_COMMAND_START_Msk)
#define FLASH_FLASHC_OP_INIT 1U
#define FLASH_FLASHC_OP_SECTOR_ERASE 2U
#define FLASH_FLASHC_OP_CHIP_ERASE 3U

/*==================================================================================================
*                                         LOCAL VARIABLES
==================================================================================================*/
static neon_flash_state_t g_flash_state = NEON_FLASH_STATE_RESET;
static uint32_t g_flash_error = NEON_FLASH_ERROR_NONE;
static neon_flash_config_t g_flash_config = {NEON_FLASH_DEFAULT_TIMEOUT, 1U};

/*==================================================================================================
*                                   PRIVATE FUNCTION PROTOTYPES
==================================================================================================*/

static int flash_range_valid(uint32_t address, uint32_t base, uint32_t total_size, uint32_t size);
static FLASH_RAMFUNC neon_status_t flash_timeout(void);
static FLASH_RAMFUNC neon_status_t flash_wait_command_complete(uint32_t done_mask);
static neon_status_t flash_ensure_initialized(void);
static FLASH_RAMFUNC uint32_t flash_enter_critical(void);
static FLASH_RAMFUNC void flash_exit_critical(uint32_t primask);
static FLASH_RAMFUNC neon_status_t flash_run_command(uint32_t opcode, uint32_t done_mask,
                                                     neon_flash_state_t busy_state);

/*==================================================================================================
*                                         GLOBAL FUNCTIONS
==================================================================================================*/

/** @copydoc neon_flash_get_default_config */
void neon_flash_get_default_config(neon_flash_config_t *config) {
    if (config != NULL) {
        config->operation_timeout_polls = NEON_FLASH_DEFAULT_TIMEOUT;
        config->verify_after_write = 1U;
    }
}

/** @copydoc neon_flash_init */
neon_status_t neon_flash_init(const neon_flash_config_t *config) {
    neon_status_t status;
    uint32_t primask;

    if ((config == NULL) || (config->operation_timeout_polls == 0U) || (config->verify_after_write > 1U)) {
        return NEON_INVALID_PARAM;
    }
    if (g_flash_state == NEON_FLASH_STATE_BUSY_PROGRAM || g_flash_state == NEON_FLASH_STATE_BUSY_ERASE ||
        g_flash_state == NEON_FLASH_STATE_BUSY_COMMAND) {
        return NEON_BUSY;
    }
    if (g_flash_state == NEON_FLASH_STATE_DEEP_POWER_STANDBY) {
        return NEON_ERROR;
    }
    if (g_flash_state == NEON_FLASH_STATE_ERROR) {
        return NEON_ERROR;
    }

    status = neon_crm_peripheral_clock_enable(NEON_CRM_PERIPHERAL_FLASH);
    if (status == NEON_OK) {
        status = neon_crm_peripheral_release_reset(NEON_CRM_PERIPHERAL_FLASH);
    }
    if (status != NEON_OK) {
        return status;
    }
    primask = flash_enter_critical();
    g_flash_config = *config;
    g_flash_error = NEON_FLASH_ERROR_NONE;

    /* Spec v1.4 section 9.1: wait for the controller to become idle first.
   * Keep the wait in SRAM and prevent ISR instruction fetches from Flash. */
    status = flash_wait_command_complete(0U);
    /* Boot ROM normally ran INIT already. Reuse it only after BUSY and START
   * are both low; INIT_DONE alone does not mean both banks are ready. */
    if ((status == NEON_OK) && ((FLASH_CTRL->STATUS & FLASH_STATUS_INIT_DONE_Msk) == 0U)) {
        status = flash_run_command(FLASH_FLASHC_OP_INIT, FLASH_STATUS_INIT_DONE_Msk, NEON_FLASH_STATE_BUSY_COMMAND);
    }

    if (status == NEON_OK) {
        g_flash_state = NEON_FLASH_STATE_READY;
    }
    flash_exit_critical(primask);
    return status;
}

/** @copydoc neon_flash_deinit */
neon_status_t neon_flash_deinit(void) {
    if (g_flash_state == NEON_FLASH_STATE_BUSY_PROGRAM || g_flash_state == NEON_FLASH_STATE_BUSY_ERASE ||
        g_flash_state == NEON_FLASH_STATE_BUSY_COMMAND) {
        return NEON_BUSY;
    }
    if (g_flash_state == NEON_FLASH_STATE_DEEP_POWER_STANDBY) {
        return NEON_ERROR;
    }
    g_flash_state = NEON_FLASH_STATE_RESET;
    g_flash_error = NEON_FLASH_ERROR_NONE;
    neon_flash_get_default_config(&g_flash_config);
    return NEON_OK;
}

/** @copydoc neon_flash_get_config */
neon_status_t neon_flash_get_config(neon_flash_config_t *config) {
    if (config == NULL) {
        return NEON_INVALID_PARAM;
    }
    *config = g_flash_config;
    return NEON_OK;
}

/** @copydoc neon_flash_clear_error */
neon_status_t neon_flash_clear_error(void) {
    if (g_flash_state == NEON_FLASH_STATE_DEEP_POWER_STANDBY) {
        return NEON_ERROR;
    }
    if ((FLASH_CTRL->STATUS & FLASH_STATUS_BUSY_Msk) != 0U) {
        return NEON_BUSY;
    }
    /* A timeout_polls caused by a disabled/rejected command engine can leave START
   * asserted forever. Disarm it so a later valid command is not blocked. */
    FLASH_CTRL->COMMAND = 0U;
    FLASH_CTRL->SLOT_COMMAND = 0U;
    g_flash_error = NEON_FLASH_ERROR_NONE;
    g_flash_state =
        ((FLASH_CTRL->STATUS & FLASH_STATUS_INIT_DONE_Msk) != 0U) ? NEON_FLASH_STATE_READY : NEON_FLASH_STATE_RESET;
    return NEON_OK;
}

/** @copydoc neon_flash_get_state */
neon_flash_state_t neon_flash_get_state(void) {
    return g_flash_state;
}

/** @copydoc neon_flash_get_error */
uint32_t neon_flash_get_error(void) {
    return g_flash_error;
}

/** @copydoc neon_flash_get_version */
uint32_t neon_flash_get_version(void) {
    return FLASH_CTRL->VERSION;
}

/** @copydoc neon_flash_get_flags */
uint32_t neon_flash_get_flags(void) {
    return FLASH_CTRL->STATUS & NEON_FLASH_FLAG_ALL;
}

/** @copydoc neon_flash_is_active_flag */
neon_status_t neon_flash_is_active_flag(uint32_t flags, uint8_t *active) {
    if ((active == NULL) || (flags == 0U) || ((flags & ~NEON_FLASH_FLAG_ALL) != 0U)) {
        return NEON_INVALID_PARAM;
    }
    const uint32_t result_word = ((FLASH_CTRL->STATUS & flags) == flags) ? 1U : 0U;
    memcpy(active, &result_word, 1U);
    return NEON_OK;
}

/** @copydoc neon_flash_read_word */
neon_status_t neon_flash_read_word(uint32_t address, uint32_t *data) {
    neon_status_t status;

    if ((data == NULL) || ((address & 0x3U) != 0U)) {
        return NEON_INVALID_PARAM;
    }
    if (!flash_range_valid(address, FLASH_BASE, NEON_FLASH_TOTAL_SIZE, sizeof(uint32_t))) {
        g_flash_error |= NEON_FLASH_ERROR_RANGE;
        return NEON_INVALID_PARAM;
    }
    status = flash_ensure_initialized();
    if (status != NEON_OK) {
        return status;
    }
    *data = FLASH_REG32(address);
    return NEON_OK;
}

/** @copydoc neon_flash_program_word */
FLASH_RAMFUNC neon_status_t neon_flash_program_word(uint32_t address, uint32_t data) {
    neon_status_t status;
    uint32_t current;
    uint32_t primask;

    if ((address & 0x3U) != 0U) {
        g_flash_error |= NEON_FLASH_ERROR_ALIGNMENT;
        return NEON_INVALID_PARAM;
    }
    if (!flash_range_valid(address, FLASH_BASE, NEON_FLASH_TOTAL_SIZE, sizeof(uint32_t))) {
        g_flash_error |= NEON_FLASH_ERROR_RANGE;
        return NEON_INVALID_PARAM;
    }

    status = flash_ensure_initialized();
    if (status != NEON_OK) {
        return status;
    }

    primask = flash_enter_critical();
    g_flash_state = NEON_FLASH_STATE_BUSY_PROGRAM;
    current = FLASH_REG32(address);
    if ((current & data) != data) {
        g_flash_error |= NEON_FLASH_ERROR_NOT_ERASED;
        g_flash_state = NEON_FLASH_STATE_READY;
        flash_exit_critical(primask);
        return NEON_ERROR;
    }

    FLASH_REG32(address) = data;
    __DSB();

    if (g_flash_config.verify_after_write != 0U && FLASH_REG32(address) != data) {
        g_flash_error |= NEON_FLASH_ERROR_VERIFY;
        g_flash_state = NEON_FLASH_STATE_READY;
        flash_exit_critical(primask);
        return NEON_ERROR;
    }

    g_flash_state = NEON_FLASH_STATE_READY;
    flash_exit_critical(primask);
    return NEON_OK;
}

/** @copydoc neon_flash_erase_sector */
FLASH_RAMFUNC neon_status_t neon_flash_erase_sector(uint32_t address) {
    neon_status_t status;
    uint32_t sector_base;
    uint32_t primask;

    if (!flash_range_valid(address, FLASH_BASE, NEON_FLASH_TOTAL_SIZE, 1U)) {
        g_flash_error |= NEON_FLASH_ERROR_RANGE;
        return NEON_INVALID_PARAM;
    }
    status = flash_ensure_initialized();
    if (status != NEON_OK) {
        return status;
    }

    sector_base = address & ~(NEON_FLASH_SECTOR_SIZE - 1U);
    primask = flash_enter_critical();
    FLASH_CTRL->CMD_ADDR = sector_base;
    status = flash_run_command(FLASH_FLASHC_OP_SECTOR_ERASE, FLASH_STATUS_SECTOR_ERASE_DONE_Msk,
                               NEON_FLASH_STATE_BUSY_ERASE);
    flash_exit_critical(primask);
    if (status != NEON_OK) {
        return status;
    }

    if (g_flash_config.verify_after_write != 0U) {
        for (uint32_t offset = 0U; offset < NEON_FLASH_SECTOR_SIZE; offset += 4U) {
            if (FLASH_REG32(sector_base + offset) != 0xFFFFFFFFU) {
                g_flash_error |= NEON_FLASH_ERROR_VERIFY;
                g_flash_state = NEON_FLASH_STATE_READY;
                return NEON_ERROR;
            }
        }
    }

    g_flash_state = NEON_FLASH_STATE_READY;
    return NEON_OK;
}

/** @copydoc neon_flash_erase_region */
neon_status_t neon_flash_erase_region(uint32_t address, uint32_t size) {
    uint32_t first_sector;
    uint32_t last_sector;

    if ((size == 0U) || !flash_range_valid(address, FLASH_BASE, NEON_FLASH_TOTAL_SIZE, size)) {
        g_flash_error |= NEON_FLASH_ERROR_RANGE;
        return NEON_INVALID_PARAM;
    }

    first_sector = address & ~(NEON_FLASH_SECTOR_SIZE - 1U);
    last_sector = (address + size - 1U) & ~(NEON_FLASH_SECTOR_SIZE - 1U);
    for (uint32_t sector = first_sector;; sector += NEON_FLASH_SECTOR_SIZE) {
        neon_status_t status = neon_flash_erase_sector(sector);
        if (status != NEON_OK) {
            return status;
        }
        if (sector == last_sector) {
            break;
        }
    }
    return NEON_OK;
}

/** @copydoc neon_flash_erase_chip */
FLASH_RAMFUNC neon_status_t neon_flash_erase_chip(void) {
    neon_status_t status = flash_ensure_initialized();
    if (status != NEON_OK) {
        return status;
    }
    status =
        flash_run_command(FLASH_FLASHC_OP_CHIP_ERASE, FLASH_STATUS_CHIP_ERASE_DONE_Msk, NEON_FLASH_STATE_BUSY_ERASE);
    if ((status == NEON_OK) && (g_flash_config.verify_after_write != 0U)) {
        for (uint32_t offset = 0U; offset < NEON_FLASH_TOTAL_SIZE; offset += sizeof(uint32_t)) {
            if (FLASH_REG32(FLASH_BASE + offset) != 0xFFFFFFFFU) {
                g_flash_error |= NEON_FLASH_ERROR_VERIFY;
                return NEON_ERROR;
            }
        }
    }
    return status;
}

/** @copydoc neon_flash_enter_deep_power_standby */
FLASH_RAMFUNC neon_status_t neon_flash_enter_deep_power_standby(void) {
    /* Spec v1.4 section 3 marks this unsupported, despite section 6.13. */
    return NEON_NOT_SUPPORTED;
}

/** @copydoc neon_flash_exit_deep_power_standby */
FLASH_RAMFUNC neon_status_t neon_flash_exit_deep_power_standby(void) {
    return NEON_NOT_SUPPORTED;
}

/** @copydoc neon_flash_read_nvr_word */
neon_status_t neon_flash_read_nvr_word(uint32_t address, uint32_t *data) {
    neon_status_t status;

    if ((data == NULL) || ((address & 0x3U) != 0U)) {
        return NEON_INVALID_PARAM;
    }
    if (!flash_range_valid(address, NEON_FLASH_NVR_BASE, NEON_FLASH_NVR_SIZE, sizeof(uint32_t))) {
        g_flash_error |= NEON_FLASH_ERROR_RANGE;
        return NEON_INVALID_PARAM;
    }
    status = flash_ensure_initialized();
    if (status != NEON_OK) {
        return status;
    }
    *data = FLASH_REG32(address);
    return NEON_OK;
}

/** @copydoc neon_flash_program_nvr_word */
FLASH_RAMFUNC neon_status_t neon_flash_program_nvr_word(uint32_t address, uint32_t data) {
    neon_status_t status;
    uint32_t current;
    uint32_t slot_base;
    uint32_t metadata;
    uint32_t primask;

    if ((address & 0x3U) != 0U) {
        g_flash_error |= NEON_FLASH_ERROR_ALIGNMENT;
        return NEON_INVALID_PARAM;
    }
    if (!flash_range_valid(address, NEON_FLASH_NVR_BASE, NEON_FLASH_NVR_SIZE, sizeof(uint32_t))) {
        g_flash_error |= NEON_FLASH_ERROR_RANGE;
        return NEON_INVALID_PARAM;
    }
    slot_base = address & ~(NEON_FLASH_NVR_SLOT_SIZE - 1U);
    if (address == slot_base) {
        g_flash_error |= NEON_FLASH_ERROR_PROTECTED;
        return NEON_LOCKED;
    }
    status = flash_ensure_initialized();
    if (status != NEON_OK) {
        return status;
    }
    primask = flash_enter_critical();
    g_flash_state = NEON_FLASH_STATE_BUSY_PROGRAM;
    metadata = FLASH_REG32(slot_base);
    if ((metadata & 0x3U) != 0x3U) {
        g_flash_error |= NEON_FLASH_ERROR_PROTECTED;
        g_flash_state = NEON_FLASH_STATE_READY;
        flash_exit_critical(primask);
        return NEON_LOCKED;
    }
    current = FLASH_REG32(address);
    if ((current & data) != data) {
        g_flash_error |= NEON_FLASH_ERROR_NOT_ERASED;
        g_flash_state = NEON_FLASH_STATE_READY;
        flash_exit_critical(primask);
        return NEON_ERROR;
    }

    FLASH_REG32(address) = data;
    __DSB();
    if ((g_flash_config.verify_after_write != 0U) && (FLASH_REG32(address) != data)) {
        g_flash_error |= NEON_FLASH_ERROR_VERIFY;
        g_flash_state = NEON_FLASH_STATE_READY;
        flash_exit_critical(primask);
        return NEON_ERROR;
    }
    g_flash_state = NEON_FLASH_STATE_READY;
    flash_exit_critical(primask);
    return NEON_OK;
}

/** @copydoc neon_flash_erase_nvr_sector */
FLASH_RAMFUNC neon_status_t neon_flash_erase_nvr_sector(uint32_t address) {
    neon_status_t status;
    uint32_t sector_base;
    uint32_t primask;

    if (!flash_range_valid(address, NEON_FLASH_NVR_BASE, NEON_FLASH_NVR_SIZE, 1U)) {
        g_flash_error |= NEON_FLASH_ERROR_RANGE;
        return NEON_INVALID_PARAM;
    }
    status = flash_ensure_initialized();
    if (status != NEON_OK) {
        return status;
    }
    sector_base = address & ~(NEON_FLASH_NVR_SLOT_SIZE - 1U);
    primask = flash_enter_critical();
    FLASH_CTRL->CMD_ADDR = sector_base;
    status = flash_run_command(FLASH_FLASHC_OP_SECTOR_ERASE, FLASH_STATUS_SECTOR_ERASE_DONE_Msk,
                               NEON_FLASH_STATE_BUSY_ERASE);
    flash_exit_critical(primask);
    if ((status == NEON_OK) && (g_flash_config.verify_after_write != 0U)) {
        for (uint32_t offset = 0U; offset < NEON_FLASH_NVR_SLOT_SIZE; offset += sizeof(uint32_t)) {
            if (FLASH_REG32(sector_base + offset) != 0xFFFFFFFFU) {
                g_flash_error |= NEON_FLASH_ERROR_VERIFY;
                return NEON_ERROR;
            }
        }
    }
    return status;
}

/** @copydoc neon_flash_issue_nvr_slot */
FLASH_RAMFUNC neon_status_t neon_flash_issue_nvr_slot(uint8_t slot, uint8_t mark_programmed, uint8_t lock) {
    neon_status_t status;
    uint32_t command;
    uint32_t primask;

    if ((slot >= NEON_FLASH_NVR_SLOT_COUNT) || (mark_programmed > 1U) || (lock > 1U) ||
        ((mark_programmed == 0U) && (lock == 0U))) {
        return NEON_INVALID_PARAM;
    }
    status = flash_ensure_initialized();
    if (status != NEON_OK) {
        return status;
    }
    command = FLASH_SLOT_COMMAND_START_Msk |
              (((uint32_t)slot << FLASH_SLOT_COMMAND_SLOT_INDEX_Pos) & FLASH_SLOT_COMMAND_SLOT_INDEX_Msk);
    if (mark_programmed != 0U) {
        command |= FLASH_SLOT_COMMAND_PROGRAMMED_Msk;
    }
    if (lock != 0U) {
        command |= FLASH_SLOT_COMMAND_LOCK_Msk;
    }

    primask = flash_enter_critical();
    if (((FLASH_CTRL->STATUS & FLASH_STATUS_BUSY_Msk) != 0U) ||
        ((FLASH_CTRL->COMMAND & FLASH_COMMAND_START_Msk) != 0U) ||
        ((FLASH_CTRL->SLOT_COMMAND & FLASH_SLOT_COMMAND_START_Msk) != 0U)) {
        flash_exit_critical(primask);
        return NEON_BUSY;
    }
    g_flash_state = NEON_FLASH_STATE_BUSY_COMMAND;
    FLASH_CTRL->SLOT_COMMAND = command;
    for (uint32_t i = 0U; i < g_flash_config.operation_timeout_polls; ++i) {
        if (((FLASH_CTRL->SLOT_COMMAND & FLASH_SLOT_COMMAND_START_Msk) == 0U) &&
            ((FLASH_CTRL->STATUS & FLASH_STATUS_BUSY_Msk) == 0U) &&
            ((FLASH_CTRL->STATUS & FLASH_STATUS_SLOT_ISSUE_DONE_Msk) != 0U)) {
            g_flash_state = NEON_FLASH_STATE_READY;
            flash_exit_critical(primask);
            return NEON_OK;
        }
    }
    flash_exit_critical(primask);
    return flash_timeout();
}

/** @copydoc neon_flash_erase_bank */
neon_status_t neon_flash_erase_bank(uint32_t bank, uint8_t include_redundancy, uint8_t include_nvr) {
    (void)bank;
    (void)include_redundancy;
    (void)include_nvr;
    return NEON_NOT_SUPPORTED;
}

/** @copydoc neon_flash_write_config_register */
neon_status_t neon_flash_write_config_register(uint32_t bank, uint32_t address, uint32_t data) {
    (void)bank;
    (void)address;
    (void)data;
    return NEON_NOT_SUPPORTED;
}

/** @copydoc neon_flash_read_config_register */
neon_status_t neon_flash_read_config_register(uint32_t bank, uint32_t address, uint32_t *data) {
    (void)bank;
    (void)address;
    (void)data;
    return NEON_NOT_SUPPORTED;
}

/** @copydoc neon_flash_set_bad_sector_records */
neon_status_t neon_flash_set_bad_sector_records(uint32_t record0, uint32_t record1) {
    (void)record0;
    (void)record1;
    return NEON_NOT_SUPPORTED;
}

/** @copydoc neon_flash_get_bad_sector_records */
neon_status_t neon_flash_get_bad_sector_records(uint32_t *record0, uint32_t *record1) {
    (void)record0;
    (void)record1;
    return NEON_NOT_SUPPORTED;
}

/** @copydoc neon_flash_enable_bad_sector_remap */
neon_status_t neon_flash_enable_bad_sector_remap(uint8_t enable) {
    (void)enable;
    return NEON_NOT_SUPPORTED;
}

/** @copydoc neon_flash_enable_nvr_sector */
neon_status_t neon_flash_enable_nvr_sector(uint8_t enable) {
    (void)enable;
    return NEON_NOT_SUPPORTED;
}

/*==================================================================================================
*                                        PRIVATE FUNCTIONS
==================================================================================================*/

static int flash_range_valid(uint32_t address, uint32_t base, uint32_t total_size, uint32_t size) {
    uint32_t offset;

    if (address < base) {
        return 0;
    }
    offset = address - base;
    return (offset <= total_size) && (size <= (total_size - offset));
}

static FLASH_RAMFUNC neon_status_t flash_timeout(void) {
    g_flash_error |= NEON_FLASH_ERROR_TIMEOUT;
    g_flash_state = NEON_FLASH_STATE_ERROR;
    return NEON_TIMEOUT;
}

static FLASH_RAMFUNC neon_status_t flash_wait_command_complete(uint32_t done_mask) {
    for (uint32_t i = 0U; i < g_flash_config.operation_timeout_polls; ++i) {
        uint32_t command = FLASH_CTRL->COMMAND;
        uint32_t status = FLASH_CTRL->STATUS;

        if (((command & FLASH_COMMAND_START_Msk) == 0U) && ((status & FLASH_STATUS_BUSY_Msk) == 0U) &&
            ((done_mask == 0U) || ((status & done_mask) != 0U))) {
            return NEON_OK;
        }
    }
    return flash_timeout();
}

static neon_status_t flash_ensure_initialized(void) {
    if ((g_flash_state == NEON_FLASH_STATE_READY) && ((FLASH_CTRL->STATUS & FLASH_STATUS_INIT_DONE_Msk) != 0U)) {
        return NEON_OK;
    }
    if ((g_flash_state == NEON_FLASH_STATE_BUSY_PROGRAM) || (g_flash_state == NEON_FLASH_STATE_BUSY_ERASE) ||
        (g_flash_state == NEON_FLASH_STATE_BUSY_COMMAND)) {
        return NEON_BUSY;
    }
    if (g_flash_state == NEON_FLASH_STATE_ERROR) {
        return NEON_ERROR;
    }
    if (g_flash_state == NEON_FLASH_STATE_DEEP_POWER_STANDBY) {
        return NEON_ERROR;
    }

    return neon_flash_init(&g_flash_config);
}

static FLASH_RAMFUNC uint32_t flash_enter_critical(void) {
    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    return primask;
}

static FLASH_RAMFUNC void flash_exit_critical(uint32_t primask) {
    __set_PRIMASK(primask);
}

static FLASH_RAMFUNC neon_status_t flash_run_command(uint32_t opcode, uint32_t done_mask,
                                                     neon_flash_state_t busy_state) {
    neon_status_t status;
    uint32_t primask;

    primask = flash_enter_critical();
    status = flash_wait_command_complete(0U);
    if (status != NEON_OK) {
        flash_exit_critical(primask);
        return status;
    }
    g_flash_state = busy_state;
    uint32_t command = FLASH_FLASHC_COMMAND_VALUE(opcode);
    /* Select the opcode with START=0, then assert START (spec v1.4, 9.1). */
    FLASH_CTRL->COMMAND = command & ~FLASH_COMMAND_START_Msk;
    __DSB();
    FLASH_CTRL->COMMAND = command;
    status = flash_wait_command_complete(done_mask);
    if (status == NEON_OK) {
        g_flash_state = NEON_FLASH_STATE_READY;
    }
    flash_exit_critical(primask);
    return status;
}
