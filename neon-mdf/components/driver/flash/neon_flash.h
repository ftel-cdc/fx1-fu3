/**
 * @file neon_flash.h
 * @brief Public declarations for Flash.
 * @details
 * NEON Flash HAL. FU2 definitions follow the Common-Spec-v8 VP contract.
 * @note Blocking polling/configuration operations are intended for task context.
 * Serialize configuration/teardown with other users of the same peripheral;
 * timeout_polls values are polling budgets, not milliseconds, where applicable.
 */

#ifndef __NEON_FLASH_H__
#define __NEON_FLASH_H__

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include "neon_types.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*==================================================================================================
*                                       FUNCTION PROTOTYPES
==================================================================================================*/
/**
 * @brief Erase Flash region.
 * @param[in] address address used by this operation.
 * @param[in] size Number of transfer elements.
 * @return NEON_OK on success, or a module error/status code.
 */
neon_status_t neon_flash_erase_region(uint32_t address, uint32_t size);
/**
 * @brief Program Flash word.
 * @param[in] address address used by this operation.
 * @param[in] data data for this operation.
 * @return NEON_OK on success, or a module error/status code.
 */
neon_status_t neon_flash_program_word(uint32_t address, uint32_t data);

#if defined(FU2)

#include "mcu_device.h"

/*==================================================================================================
*                                        DEFINES AND MACROS
==================================================================================================*/
/* FU2 v8 main array: two 256-KB banks and 512-byte erase sectors. */
#define NEON_FLASH_BANK_COUNT 2U
#define NEON_FLASH_BANK_SIZE 0x40000U
#define NEON_FLASH_TOTAL_SIZE 0x80000U
#define NEON_FLASH_SECTOR_SIZE 0x200U
#define NEON_FLASH_SECTORS_PER_BANK 512U
#define NEON_FLASH_WORDS_PER_BANK 65536U
#define NEON_FLASH_CPU_WORD_SIZE 0x4U
#define NEON_FLASH_NVR_BASE 0x00010000U
#define NEON_FLASH_NVR_SLOT_COUNT 14U
#define NEON_FLASH_NVR_SLOT_SIZE 0x200U
#define NEON_FLASH_NVR_SIZE 0x1C00U
#define NEON_FLASH_DEFAULT_TIMEOUT 200000U

#if (NEON_FLASH_BANK_COUNT * NEON_FLASH_BANK_SIZE) != NEON_FLASH_TOTAL_SIZE
#error "FU2 v8 Flash bank geometry does not match total size"
#endif
#if (NEON_FLASH_SECTORS_PER_BANK * NEON_FLASH_SECTOR_SIZE) != NEON_FLASH_BANK_SIZE
#error "FU2 v8 Flash sector geometry does not match bank size"
#endif

#define NEON_FLASH_ERROR_NONE 0x00000000U
#define NEON_FLASH_ERROR_ALIGNMENT 0x00000001U
#define NEON_FLASH_ERROR_RANGE 0x00000002U
#define NEON_FLASH_ERROR_NOT_ERASED 0x00000004U
#define NEON_FLASH_ERROR_VERIFY 0x00000008U
#define NEON_FLASH_ERROR_TIMEOUT 0x00000010U
#define NEON_FLASH_ERROR_PROTECTED 0x00000020U

/*==================================================================================================
*                                  STRUCTURES AND OTHER TYPEDEFS
==================================================================================================*/
typedef enum {
    NEON_FLASH_STATE_RESET = 0,
    NEON_FLASH_STATE_READY = 1,
    NEON_FLASH_STATE_BUSY_PROGRAM = 2,
    NEON_FLASH_STATE_BUSY_ERASE = 3,
    NEON_FLASH_STATE_BUSY_COMMAND = 4,
    NEON_FLASH_STATE_DEEP_POWER_STANDBY = 5,
    NEON_FLASH_STATE_ERROR = 6,
} neon_flash_state_t;

typedef uint32_t neon_flash_flag_t;
#define NEON_FLASH_FLAG_BUSY ((1U << 0U))
#define NEON_FLASH_FLAG_SLOT_ISSUE_DONE ((1U << 1U))
#define NEON_FLASH_FLAG_INIT_DONE ((1U << 2U))
#define NEON_FLASH_FLAG_SECTOR_ERASE_DONE ((1U << 3U))
#define NEON_FLASH_FLAG_CHIP_ERASE_DONE ((1U << 4U))
#define NEON_FLASH_FLAG_DPSTB_DONE ((1U << 5U))
#define NEON_FLASH_FLAG_ALL (0x0000003FU)

typedef struct {
    uint32_t operation_timeout_polls; /* Poll iterations, not milliseconds. */
    neon_field8_t verify_after_write;
} neon_flash_config_t;

/**
 * @brief Fill the default configuration for Flash.
 * @param[out] config Module configuration.
 */
void neon_flash_get_default_config(neon_flash_config_t *config);
/**
 * @brief Initialize Flash.
 * @param[in] config Module configuration.
 * @return NEON_OK on success, or a module error/status code.
 * @details
 * FU2 waits for BUSY=0 and START=0 before reusing BootROM initialization or
 * launching INIT. A hardware wait exceeding operation_timeout_polls returns
 * NEON_TIMEOUT; an ongoing operation owned by the driver returns NEON_BUSY.
 */
neon_status_t neon_flash_init(const neon_flash_config_t *config);
/**
 * @brief Deinitialize Flash.
 * @return NEON_OK on success, or a module error/status code.
 */
neon_status_t neon_flash_deinit(void);
/**
 * @brief Read Flash config.
 * @param[out] config Module configuration.
 * @return NEON_OK on success, or a module error/status code.
 */
neon_status_t neon_flash_get_config(neon_flash_config_t *config);
/**
 * @brief Clear Flash error.
 * @return NEON_OK on success, or a module error/status code.
 */
neon_status_t neon_flash_clear_error(void);

/**
 * @brief Read Flash state.
 * @return Current Flash driver state.
 */
neon_flash_state_t neon_flash_get_state(void);
/**
 * @brief Read Flash error.
 * @return Module error bitmask.
 */
uint32_t neon_flash_get_error(void);
/**
 * @brief Read Flash version.
 * @return The requested value or operation result.
 */
uint32_t neon_flash_get_version(void);
/**
 * @brief Read Flash flags.
 * @return Current hardware status-flag bitmask.
 */
uint32_t neon_flash_get_flags(void);
/**
 * @brief Check the selected status flags of Flash.
 * @param[in] flags Status-flag bitmask.
 * @param[out] active Receives whether the selected flag is active.
 * @return NEON_OK on success, or a module error/status code.
 */
neon_status_t neon_flash_is_active_flag(uint32_t flags, uint8_t *active);

/**
 * @brief Read Flash word.
 * @param[in] address address used by this operation.
 * @param[out] data data for this operation.
 * @return NEON_OK on success, or a module error/status code.
 */
neon_status_t neon_flash_read_word(uint32_t address, uint32_t *data);
/**
 * @brief Erase Flash sector.
 * @param[in] address address used by this operation.
 * @return NEON_OK on success, or a module error/status code.
 */
neon_status_t neon_flash_erase_sector(uint32_t address);
/**
 * @brief Erase Flash chip.
 * @return NEON_OK on success, or a module error/status code.
 * @details
 * Erases the complete 512-KB main array. The function body runs from SRAM,
 * but the caller and all post-return code must also reside outside that array.
 */
neon_status_t neon_flash_erase_chip(void);
/**
 * @brief Enter Flash deep power standby.
 * @return NEON_NOT_SUPPORTED on FU2.
 * @details
 * Spec v1.4 section 3 lists Deep Standby as unsupported, although section
 * 6.13 describes it. These APIs issue no command until support is confirmed.
 */
neon_status_t neon_flash_enter_deep_power_standby(void);
/**
 * @brief Exit Flash deep power standby.
 * @return NEON_NOT_SUPPORTED on FU2.
 */
neon_status_t neon_flash_exit_deep_power_standby(void);

/**
 * @brief Read Flash NVR word.
 * @param[in] address address used by this operation.
 * @param[out] data data for this operation.
 * @return NEON_OK on success, or a module error/status code.
 * @details
 * NVR consists of 14 slots of 512 bytes. Direct programming of slot word 0
 * is blocked by hardware; lifecycle bits are changed with IssueNVRSlot().
 */
neon_status_t neon_flash_read_nvr_word(uint32_t address, uint32_t *data);
/**
 * @brief Program Flash NVR word.
 * @param[in] address address used by this operation.
 * @param[in] data data for this operation.
 * @return NEON_OK on success, or a module error/status code.
 */
neon_status_t neon_flash_program_nvr_word(uint32_t address, uint32_t data);
/**
 * @brief Erase Flash NVR sector.
 * @param[in] address address used by this operation.
 * @return NEON_OK on success, or a module error/status code.
 */
neon_status_t neon_flash_erase_nvr_sector(uint32_t address);
/**
 * @brief Issue Flash NVR slot.
 * @param[in] slot Flash NVR slot selection.
 * @param[in] mark_programmed Nonzero to mark the slot as programmed.
 * @param[in] lock Nonzero to lock the selected resource.
 * @return NEON_OK on success, or a module error/status code.
 */
neon_status_t neon_flash_issue_nvr_slot(uint8_t slot, uint8_t mark_programmed, uint8_t lock);

/**
 * @brief Erase Flash bank.
 * @param[in] bank Flash bank selection.
 * @param[in] include_redundancy Nonzero to include the redundant Flash area.
 * @param[in] include_nvr Nonzero to include the nonvolatile register area.
 * @return NEON_OK on success, or a module error/status code.
 * @details
 * Deprecated compatibility API: the v8 command engine chip-erases the complete array rather than one bank.
 * Retained for source compatibility; per-bank erase returns NOT_SUPPORTED.
 */
neon_status_t neon_flash_erase_bank(uint32_t bank, uint8_t include_redundancy, uint8_t include_nvr);

/**
 * @brief Write Flash config register.
 * @param[in] bank Flash bank selection.
 * @param[in] address address used by this operation.
 * @param[in] data data for this operation.
 * @return NEON_OK on success, or a module error/status code.
 * @details
 * Deprecated compatibility APIs: these v1.2 operations are absent from the
 * FU2 v8 VP register contract and always return NEON_NOT_SUPPORTED.
 */
neon_status_t neon_flash_write_config_register(uint32_t bank, uint32_t address, uint32_t data);
/**
 * @brief Read Flash config register.
 * @param[in] bank Flash bank selection.
 * @param[in] address address used by this operation.
 * @param[out] data data for this operation.
 * @return NEON_OK on success, or a module error/status code.
 */
neon_status_t neon_flash_read_config_register(uint32_t bank, uint32_t address, uint32_t *data);
/**
 * @brief Set Flash bad sector records.
 * @param[in] record0 First bad-sector record.
 * @param[in] record1 second bad-sector record.
 * @return NEON_OK on success, or a module error/status code.
 */
neon_status_t neon_flash_set_bad_sector_records(uint32_t record0, uint32_t record1);
/**
 * @brief Read Flash bad sector records.
 * @param[out] record0 First bad-sector record.
 * @param[out] record1 second bad-sector record.
 * @return NEON_OK on success, or a module error/status code.
 */
neon_status_t neon_flash_get_bad_sector_records(uint32_t *record0, uint32_t *record1);
/**
 * @brief enable Flash bad sector remap.
 * @param[in] enable Nonzero to enable, zero to disable.
 * @return NEON_OK on success, or a module error/status code.
 */
neon_status_t neon_flash_enable_bad_sector_remap(uint8_t enable);
/**
 * @brief enable Flash NVR sector.
 * @param[in] enable Nonzero to enable, zero to disable.
 * @return NEON_OK on success, or a module error/status code.
 */
neon_status_t neon_flash_enable_nvr_sector(uint8_t enable);

#endif /* FU2 */

#ifdef __cplusplus
}
#endif

#endif /* __NEON_FLASH_H__ */
