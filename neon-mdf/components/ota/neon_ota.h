/**
 * @file neon_ota.h
 * @brief Public declarations for OTA.
 * @details
 * * neon_ota.h - doc/ghi "otadata" (trang thai OTA: slot nao active, da xac
 * nhan tot chua, da thu boot may lan) - dung CHUNG boi CA bootloader
 * (neon_bootloader_main.c, doc luc boot de quyet dinh nhay vao slot nao) LAN
 * app (goi neon_ota_confirm_valid() sau khi chay on dinh) - TACH RIENG thanh
 * component cua no (khong nam trong components/bootloader/) de app link
 * duoc ma khong keo theo main() cua bootloader, cung ly do voi
 * neon_mdf_bootloader_uart.
 *
 * CHI duoc them vao build khi app CO flashmap.csv (xem cmake/flashmap.cmake,
 * neon_mdf_process_flashmap() tu add_subdirectory() component nay) - dung
 * NEON_MDF_FLASHMAP_OTADATA_BASE (macro sinh boi tools/gen_flashmap.py,
 * "flashmap.h") nen KHONG the dung cho app khong co flashmap.
 *
 * Mo hinh OTA BAT DOI XUNG (KHONG phai A/B doi xung kieu ping-pong) - xem
 * memory project_fu2_bootloader_ota_direction + plan file cho quyet dinh
 * thiet ke day du:
 *   - slot "factory" (active_slot=0) - nap cung bootloader luc dau, KHONG
 *     BAO GIO bi OTA dung toi - luon la phao cuu sinh chac chan tot.
 *   - slot "ota" (active_slot=1) - CHI OTA moi ghi vao.
 * Co che an toan: xac nhan + tu dong rollback - app moi (chay tu slot ota)
 * PHAI tu goi neon_ota_confirm_valid() sau khi chay on dinh; neu chua xac
 * nhan ma bi reset lap lai vuot nguong, bootloader tu dong quay ve factory.
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

#define NEON_OTA_STATE_VALID 0U          /* slot active da xac nhan tot, khong rollback nua */
#define NEON_OTA_STATE_PENDING_VERIFY 1U /* slot ota moi ghi, dang cho app tu confirm */
#define NEON_OTA_STATE_INVALID 2U        /* slot ota bi tu choi (CRC sai hoac het luot thu) - da rollback ve factory */

/* Nguong so lan boot-chua-xac-nhan truoc khi TU DONG rollback ve factory -
 * xem NEON_OTA_STATE_PENDING_VERIFY trong neon_bootloader_main.c. */
#define NEON_OTA_MAX_BOOT_ATTEMPTS 3U

/*==================================================================================================
*                                  STRUCTURES AND OTHER TYPEDEFS
==================================================================================================*/
typedef struct {
    uint32_t magic;         /* PHAI = NEON_OTA_MAGIC de coi la otadata hop le */
    uint32_t active_slot;   /* NEON_OTA_SLOT_FACTORY hoac NEON_OTA_SLOT_OTA */
    uint32_t state;         /* NEON_OTA_STATE_* */
    uint32_t boot_attempts; /* so lan da boot slot active MA CHUA xac nhan tot */
    uint32_t crc32;         /* CRC32 tren 4 field tren (16 byte dau), phat hien otadata hong/ghi do */
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
 * Doc otadata that tu flash - tra ve 0 (KHONG ghi *out) neu magic/CRC khong
 * hop le (chua tung OTA lan nao - truong hop nay coi nhu
 * active_slot=FACTORY, state=VALID o noi goi). Tra ve 1 neu hop le.
 */
int neon_ota_read_data(neon_ota_data_t *out);

/* Ghi otadata that vao flash (tu dong tinh lai magic/crc32, khong can nguoi
 * goi tu dien 2 field do) - erase+ghi lai TOAN BO 1 sector otadata moi lan
 * goi (KHONG dung ky thuat log-structured - tan suat ghi thuc te thap, xem
 * plan file). */
/* Trả 1 khi erase/program hoàn tất, 0 nếu Flash HAL báo lỗi. */
/**
 * @brief Erase and program OTA metadata, including its magic and CRC32.
 * @param[in] data data for this operation.
 * @return 1 on success, 0 if validation or the Flash operation fails.
 */
int neon_ota_write_data(const neon_ota_data_t *data);

/**
 * @brief Confirm the running OTA image to prevent rollback on subsequent boots.
 * @details
 * App TU GOI ham nay sau khi da chay on dinh 1 luc (vd sau vai giay/1 vong
 * lap chinh) - danh dau slot dang chay la VALID, KHONG rollback nua cho toi
 * lan OTA sau. Khong lam gi (return ngay) neu chua tung co otadata hop le
 * (dang chay factory mac dinh, khong can xac nhan) hoac da VALID roi (tranh
 * erase thua khong can thiet).
 */
void neon_ota_confirm_valid(void);

#ifdef __cplusplus
}
#endif

#endif /* __NEON_OTA_H__ */
