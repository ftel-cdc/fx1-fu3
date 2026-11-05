/**
 * @file neon_bootloader_crc32.c
 * @brief Implementation for bootloader crc32.
 * @details
 * * neon_bootloader_crc32.c - CRC32 chuan zlib, thuat toan bit-by-bit don gian
 * (khong dung bang tra - it code hon, chap nhan cham hon vi bootloader chi
 * chay 1 lan luc update, khong phai hot path).
 */

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/

#include "neon_bootloader_crc32.h"

/*==================================================================================================
*                                         GLOBAL FUNCTIONS
==================================================================================================*/

/** @copydoc neon_bootloader_crc32 */
uint32_t neon_bootloader_crc32(const void *data, size_t len) {
    const uint8_t *bytes = (const uint8_t *)data;
    uint32_t crc = 0xFFFFFFFFU;

    for (size_t i = 0; i < len; i++) {
        crc ^= bytes[i];
        for (int bit = 0; bit < 8; bit++) {
            if (crc & 1U) {
                crc = (crc >> 1) ^ 0xEDB88320U;
            } else {
                crc = crc >> 1;
            }
        }
    }

    return crc ^ 0xFFFFFFFFU;
}
