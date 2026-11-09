/**
 * @file neon_bootloader_crc32.h
 * @brief Public declarations for bootloader crc32.
 * @details
 * * neon_bootloader_crc32.h - CRC32 phan mem, THUAT TOAN CHUAN zlib/PNG (poly
 * 0xEDB88320, khong dao bit) - de khop voi Python `zlib.crc32()` o host
 * (tools/send_firmware.py), khong dung CRC peripheral phan cung cua STM32
 * (tranh phai xac nhan them 1 peripheral nua, va CRC phan cung cua STM32
 * dung poly 0x04C11DB7 KHONG khop voi zlib mac dinh - de danh gia don gian,
 * chon phan mem). Thuat toan pure-software, khong phu thuoc chip nao ca -
 * dung chung cho moi family, khong can dispatch theo family nhu UART/flash.
 */

#ifndef __NEON_BOOTLOADER_CRC32_H__
#define __NEON_BOOTLOADER_CRC32_H__

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/*==================================================================================================
*                                       FUNCTION PROTOTYPES
==================================================================================================*/
/**
 * @brief Calculate the software CRC32 used by firmware images and OTA metadata.
 * @param[in] data data for this operation.
 * @param[in] len Number of bytes to process.
 * @return CRC32 of the input bytes.
 */
uint32_t neon_bootloader_crc32(const void *data, size_t len);

#ifdef __cplusplus
}
#endif

#endif /* __NEON_BOOTLOADER_CRC32_H__ */
