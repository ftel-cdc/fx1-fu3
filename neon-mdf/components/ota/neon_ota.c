/**
 * @file neon_ota.c
 * @brief Implementation for OTA.
 * @details
 * * neon_ota.c - xem neon_ota.h cho thiet ke tong the. Dung lai
 * neon_flash_erase_region/ProgramWord (components/driver/flash) +
 * neon_bootloader_crc32 (components/bootloader, tach rieng thanh target
 * neon_mdf_crc32 - xem components/bootloader/CMakeLists.txt - de component
 * nay link duoc ma khong keo theo main() cua bootloader) - KHONG viet lai
 * logic flash/CRC nao ca.
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
/* __builtin_memcpy (khong #include <string.h>) - toolchain RISC-V cua SDK
 * nay (riscv64-elf-gcc, xem cmake/toolchain/riscv-none-elf.cmake) KHONG kem
 * newlib nen KHONG co <string.h> that - builtin cua GCC luon co san bat ke
 * kien truc/libc, an toan dung chung cho ca Cortex-M (van co newlib) lan
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
        /* Chua tung co otadata hop le - dang chay factory mac dinh, khong co
     * gi de xac nhan. */
        return;
    }
    if (data.state == NEON_OTA_STATE_VALID) {
        /* Da xac nhan roi - khong ghi lai (tranh erase+ghi flash thua). */
        return;
    }

    data.state = NEON_OTA_STATE_VALID;
    data.boot_attempts = 0;
    /* API public hiện giữ kiểu void; lần boot sau vẫn thấy trạng thái cũ nếu
   * ghi thất bại và cơ chế rollback sẽ xử lý an toàn. */
    (void)neon_ota_write_data(&data);
}
