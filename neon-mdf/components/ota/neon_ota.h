/**
 * @file neon_ota.h
 * @brief Public declarations for OTA.
 * @details
 * * neon_ota.h reads/writes otadata: active slot, confirmation state and boot-attempt count. Shared by
 * the bootloader (neon_bootloader_main.c, selecting a slot at boot) and app (calling
 * neon_ota_confirm_valid() after stable operation). Kept separate as
 * component of no (not male in components/bootloader/) so that app link can be without glue
 * according to the bootloader's main(), same reason as
 * neon_mdf_bootloader_uart.
 *
 * Only added to builds with flashmap.csv (see cmake/flashmap.cmake, neon_mdf_process_flashmap() from
 * this add_subdirectory() component) - uses NEON_MDF_FLASHMAP_OTADATA_BASE (macro created by
 * tools/gen_flashmap.py, "flashmap.h") so cannot be used for app without flashmap.
 *
 * open image OTA enable pulse change (not A/B pulse change kieu ping-pong) - see memory
 * project_fu2_bootloader_ota_direction + plan file for specified
 * Complete design:
 *   - factory slot (active_slot=0): flashed with the bootloader initially and never overwritten by OTA; always the final
 *     buoy to create tot pins.
 *   - slot "ota" (active_slot=1) - only OTA each write vao. has safety cover: confirm + automatically
 * rollback - new app (running from ota slot) must be from calling neon_ota_confirm_valid() after
 * running on specified; if unconfirmed but bi reset repeated beyond the range, bootloader
 * automatically returns to factory.
 */

#ifndef __NEON_OTA_H__
#define __NEON_OTA_H__

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include <stdint.h>

/*==================================================================================================
*                                        DEFINES AND MACROS
==================================================================================================*/
#define NEON_OTA_MAGIC 0x4154414FU /* "OTA" + 1 byte padding, LE - PLACEHOLDER minh hoa */

#define NEON_OTA_SLOT_FACTORY 0U
#define NEON_OTA_SLOT_OTA 1U

#define NEON_OTA_STATE_VALID 0U          /* slot active confirmed tot, no old rollback */
#define NEON_OTA_STATE_PENDING_VERIFY 1U /* slot ota each write, are giving app from confirm */
#define NEON_OTA_STATE_INVALID 2U        /* OTA slot rejected because of a bad CRC or exhausted boot attempts; rolled back to factory */

/* The number of unknown boots before automatic rollback to factory - see NEON_OTA_STATE_PENDING_VERIFY
 * in neon_bootloader_main.c. */
#define NEON_OTA_MAX_BOOT_ATTEMPTS 3U

/*==================================================================================================
*                                  STRUCTURES AND OTHER TYPEDEFS
==================================================================================================*/
typedef struct {
    uint32_t magic;         /* must = NEON_OTA_MAGIC to be considered otadata le */
    uint32_t active_slot;   /* NEON_OTA_SLOT_FACTORY Or NEON_OTA_SLOT_OTA */
    uint32_t state;         /* NEON_OTA_STATE_* */
    uint32_t boot_attempts; /* number of times have boot slot active without tot confirmation */
    uint32_t crc32;         /* CRC32 over the preceding four fields (first 16 bytes), detecting corrupted or partially written otadata */
} neon_ota_data_t;

#ifdef __cplusplus
extern "C" {
#endif

/*==================================================================================================
*                                       FUNCTION PROTOTYPES
==================================================================================================*/
/**
 * @brief Read and validate the OTA metadata stored in Flash.
 * @param[out] out Destination for the validated result.
 * @return 1 on success, 0 if validation or the Flash operation fails.
 * @details
 * read real otadata from flash - check to 0 (not write *out) if magic/CRC fails (not every OTA - this
 * is considered active_slot=FACTORY, state=VALID in our call). check to 1 if it does.
 */
int neon_ota_read_data(neon_ota_data_t *out);

/* write otadata real and flash (automatically recalculate magic/crc32, no call from filling those 2
 * fields) - erase+write all 1 otadata sector per call (do not use log-structured frequency, see
 * plan file). */
/* Pay 1 when erase/program is complete, 0 if HAL Flash gives error. */
/**
 * @brief Erase and program OTA metadata, including its magic and CRC32.
 * @param[in] data data for this operation.
 * @return 1 on success, 0 if validation or the Flash operation fails.
 */
int neon_ota_write_data(const neon_ota_data_t *data);

/**
 * @brief Confirm the running OTA image to prevent rollback on subsequent boots.
 * @details
 * The app calls this function after stable operation, for example a few seconds or one main-loop cycle -
 * the current slot header is VALID, no rollback until the next OTA. do not return immediately if
 * there has never been a consistent otadata (running default factory, no confirmation needed) or
 * already VALID (unnecessary losing erase battles).
 */
void neon_ota_confirm_valid(void);

#ifdef __cplusplus
}
#endif

#endif /* __NEON_OTA_H__ */
