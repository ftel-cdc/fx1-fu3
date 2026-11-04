/**
 * @file image_header.h
 * @brief Public declarations for image header.
 * @details
 * * image_header.h - dinh dang header firmware image dung chung giua
 * neon_bootloader_main.c (doc/verify) va tools/send_firmware.py (host, ghep
 * vao dau file gui qua UART). Doi 1 ben thi PHAI doi ben kia theo.
 *
 * Layout THAT tren flash: [16 byte image_header_t][padding toi
 * NEON_MDF_APP_VECTOR_BASE][vector table cua app][code/data cua app...].
 * IMAGE_HEADER_SIZE (macro nay, =16, sizeof(image_header_t) THAT) khac voi
 * NEON_MDF_IMAGE_HEADER_SIZE (macro sinh tu mcu-description, vd 0x200) -
 * cai sau la "khe" du lon de thoa yeu cau align SCB->VTOR (xem file device
 * YAML tuong ung trong mcu-description/devices, muc `bootloader.image_header_size` +
 * loader.py._check_bootloader_vtor_alignment), KHONG PHAI kich thuoc header
 * that su - phan giua 16 byte header va NEON_MDF_APP_VECTOR_BASE la vung
 * dem chua dung.
 */

#ifndef __IMAGE_HEADER_H__
#define __IMAGE_HEADER_H__

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include <stdint.h>

/*==================================================================================================
*                                        DEFINES AND MACROS
==================================================================================================*/
#define IMAGE_MAGIC 0x4E454F4EU /* "NEON" - PLACEHOLDER minh hoa, khong phai chuan chinh thuc */

/*==================================================================================================
*                                  STRUCTURES AND OTHER TYPEDEFS
==================================================================================================*/
typedef struct {
    uint32_t magic;   /* PHAI = IMAGE_MAGIC de coi la anh hop le */
    uint32_t version; /* app tu dat, bootloader hien khong so sanh (chua co logic downgrade-protect) */
    uint32_t size;    /* kich thuoc anh app THAT (vector table + code + data), KHONG tinh header nay */
    uint32_t
        crc32; /* CRC32 (poly 0xEDB88320, giong zlib/PNG) tinh tren dung `size` byte tai NEON_MDF_APP_VECTOR_BASE */
} image_header_t;

#define IMAGE_HEADER_SIZE 16U /* sizeof(image_header_t) THAT - CA 2 ben (bootloader/host) deu gia dinh gia tri nay */

#endif /* __IMAGE_HEADER_H__ */
