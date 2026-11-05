/**
 * @file neon_ota.c
 * @brief Implementation for OTA.
 * @details
 * * neon_ota.c - see neon_ota.h for reusable synthetic devices
 * neon_flash_erase_region/ProgramWord (components/driver/flash) +
 * neon_bootloader_crc32 (components/bootloader, separately tailored to target neon_mdf_crc32 - see
 * components/bootloader/CMakeLists.txt - let component link without glue according to bootloader's
 * main()) - not rewritten
 * flash/CRC logic.
 */

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/

#include "neon_ota.h"
#include "neon_bootloader_crc32.h"
#include "flash/neon_flash.h"
#include "flashmap.h"

#include <stddef.h>

/*==================================================================================================
*                                        DEFINES AND MACROS
==================================================================================================*/
/* __builtin_memcpy (not #include <string.h>) - the RISC-V toolchain of this SDK (riscv64-elf-gcc, see
 * cmake/toolchain/riscv-none-elf.cmake) does not include newlib, so no real <string.h> - builtin
 * GCC provides this independently of architecture/libc, so it works with both Cortex-M/newlib and
 * RISC-V. */
#define NEON_OTA_DATA_CRC_LENGTH offsetof(neon_ota_data_t, crc32)

/*==================================================================================================
*                                         GLOBAL FUNCTIONS
==================================================================================================*/

/** @copydoc neon_ota_read_data */
int neon_ota_read_data(neon_ota_data_t *out) {
    neon_ota_data_t stored;
    __builtin_memcpy(&stored, (const void *)NEON_MDF_FLASHMAP_OTADATA_BASE, sizeof(stored));

    if (stored.magic != NEON_OTA_MAGIC) {
        return 0;
    }
    uint32_t crc = neon_bootloader_crc32(&stored, NEON_OTA_DATA_CRC_LENGTH);
    if (crc != stored.crc32) {
        return 0;
    }

    *out = stored;
    return 1;
}

/** @copydoc neon_ota_write_data */
int neon_ota_write_data(const neon_ota_data_t *data) {
    neon_ota_data_t tmp = *data;
    tmp.magic = NEON_OTA_MAGIC;
    tmp.crc32 = neon_bootloader_crc32(&tmp, NEON_OTA_DATA_CRC_LENGTH);

    if (neon_flash_erase_region(NEON_MDF_FLASHMAP_OTADATA_BASE, sizeof(tmp)) != NEON_OK) {
        return 0;
    }

    const uint32_t *words = (const uint32_t *)&tmp;
    for (uint32_t i = 0; i < sizeof(tmp) / 4U; i++) {
        if (neon_flash_program_word(NEON_MDF_FLASHMAP_OTADATA_BASE + i * 4U, words[i]) != NEON_OK) {
            return 0;
        }
    }
    return 1;
}

/** @copydoc neon_ota_confirm_valid */
void neon_ota_confirm_valid(void) {
    neon_ota_data_t data;
    if (!neon_ota_read_data(&data)) {
        /* unmatched otadata - running default factory, nothing to confirm. */
        return;
    }
    if (data.state == NEON_OTA_STATE_VALID) {
        /* confirmed - do not rewrite (erase+write flash loses). */
        return;
    }

    data.state = NEON_OTA_STATE_VALID;
    data.boot_attempts = 0;
    /* The public API currently keeps the void type; next boot will still see the old state if write fails
   * and the rollback mechanism will handle it safely. */
    (void)neon_ota_write_data(&data);
}
