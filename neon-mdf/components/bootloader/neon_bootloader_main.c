/**
 * @file neon_bootloader_main.c
 * @brief Implementation for bootloader main.
 * @details
 * * neon_bootloader_main.c - bootloader 2nd-stage qua UART, TONG QUAT tu
 * Bootloader_WeAct/Src/bootloader_main.c (vi du dau tien, WEACT_F412) thanh
 * 1 phan cua SDK - dung macro NEON_MDF_APP_BASE/APP_VECTOR_BASE/APP_SIZE
 * sinh tu mcu-description (thay vi #define hardcode) + NEON_FLASH_*
 * (components/driver/flash) + NEON_BOOTLOADER_UART_* (neon_bootloader_uart.h)
 * + NEON_BOOTLOADER_ARCH_* (neon_bootloader_arch.h) nen KHONG con gia dinh
 * dia chi/family/kien truc CPU cu the nao trong file nay - device nao co
 * khai `bootloader:` trong YAML + implementation UART/flash/arch that cho
 * family do (xem components/driver/CMakeLists.txt,
 * components/bootloader/CMakeLists.txt) la dung duoc file nay nguyen ven.
 *
 * PHAN DAC THU KIEN TRUC CPU (SysTick-tuong-duong/jump-to-app/system-reset)
 * tach het ra neon_bootloader_arch_<family>.c (xem neon_bootloader_arch.h) -
 * file NAY 100% dung chung moi kien truc/family, KHONG co #ifdef theo
 * arch/family nao ca (2026-08-10, tach ra tu 1 ban co #if defined(NEON_MDF_ARCH_RISCV)
 * chen truc tiep - user de nghi tach rieng vi ly do de quan ly khi them
 * nhieu device sau nay, GIU LAI phan logic OTA/rollback DUNG CHUNG trong 1
 * cho duy nhat, khong copy-paste sang nhieu file - xem
 * platform/weact_f412/neon_bootloader_arch.c/platform/ch32v003/neon_bootloader_arch.c cho vi
 * du implementation THAT).
 *
 * PHAM VI + GIOI HAN CO CHU DICH (khong phai thieu sot):
 * - KHONG co secure boot/chu ky - chi CRC32 (phat hien loi truyen, KHONG
 *   chong gia mao). Muon that can chu ky (vd ECDSA) - ngoai pham vi hien tai.
 * - KHONG co downgrade protection (version trong header chua duoc so
 *   sanh/kiem tra).
 * - Single-bank (khong co A/B partition/staging) - xem huong OTA rong hon
 *   cua team (memory project_fu2_bootloader_ota_direction) cho tuong lai.
 *
 * GIAO THUC UART (xem neon_bootloader_uart_<family>.c cho tham so that):
 *   1. Boot -> doi ~3s 1 byte SYNC (0x55 'U'). Nhan duoc -> vao UPDATE MODE.
 *      Khong nhan duoc (timeout) -> kiem tra app hien co, nhay vao neu hop
 *      le, khong thi dung yen (KHONG co recovery UI o day).
 *   2. UPDATE MODE: gui 'R' (0x52, ready) -> nhan 16 byte header
 *      (image_header_t) -> kiem tra magic -> gui ACK (0x06) hoac NAK (0x15).
 *   3. Xoa vung [APP_BASE, APP_VECTOR_BASE + hdr.size) qua neon_flash_erase_region
 *      (tu dong erase moi sector/page ma vung nay cham toi, khong con gioi
 *      han 1 sector duy nhat nhu ban dau).
 *   4. Nhan data theo chunk 256 byte (byte cuoi pad bang 0xFF neu size
 *      khong chia het cho 256) - ghi tung word (4 byte) vao flash ngay sau
 *      khe header - gui ACK sau MOI chunk.
 *   5. Tinh CRC32 tren dung `size` byte vua ghi, so sanh voi header.crc32 -
 *      khop -> gui ACK cuoi + nhay vao app (co VTOR/mtvec relocation That).
 *      Khong khop -> gui NAK, dung yen (KHONG nhay vao anh sai).
 */

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/

#include "neon_bootloader_uart.h"
#include "neon_bootloader_crc32.h"
#include "neon_bootloader_arch.h"
#include "image_header.h"
#include "flash/neon_flash.h"
#include "mcu_device.h"

#if defined(NEON_MDF_FLASHMAP_ENABLED)
/* Logic partition-switch/OTA MOI - CHI compile khi app co flashmap.csv (xem
 * cmake/flashmap.cmake, components/bootloader/CMakeLists.txt). KHONG anh
 * huong app khong co flashmap.csv (Blinky, FreeRTOS_Blinky) - nhanh #else o
 * cuoi file nay (xem run_update_mode()/main() ben duoi) la CODE CU KHONG
 * DOI 1 KY TU nao, da xac nhan bang so sanh hash bootloader.bin truoc/sau -
 * xem plan file cho ly do + thiet ke day du. */
#include "flashmap.h"
#include "neon_ota.h"
#endif

/*==================================================================================================
*                                        DEFINES AND MACROS
==================================================================================================*/
#define NEON_BOOTLOADER_CHUNK_SIZE 256U

#define NEON_BOOTLOADER_SYNC_BYTE 0x55U
#define NEON_BOOTLOADER_READY_BYTE 0x52U
#define NEON_BOOTLOADER_ACK_BYTE 0x06U
#define NEON_BOOTLOADER_NAK_BYTE 0x15U

/* ~3s cho doi SYNC luc boot - dung SysTick-tuong-duong THAT (qua
 * neon_bootloader_arch.h, KHONG con truc tiep dung CMSIS SysTick trong file
 * nay) de dem thoi gian, KHONG dem so vong lap.
 *
 * LICH SU (2026-08-09): ban dau dung 1 bien dem so vong lap busy-wait
 * (SYNC_TIMEOUT_POLLS) roi giam dan moi lan RXNE chua len - do THAT bang
 * SWD (halt tai cac moc thoi gian sau reset) cho ket qua RAT KHONG ON DINH
 * giua cac lan do (co lan do ra ~2.8s, co lan van dang chay o ~8s+, cung 1
 * gia tri hang so). Nghiem trong hon: khi KHONG gan debugger (SWD hoan toan
 * thao roi qua "shutdown"), doc UART thuan tuy cho thay app cu da chay
 * NGAY TU t=0.007s sau reset (gan nhu tuc thi, bat ke gia tri
 * SYNC_TIMEOUT_POLLS la bao nhieu) - trong khi dat breakpoint qua SWD ngay
 * sau loi goi cho thay CPU van con "ket" trong ham hon 2s. Ket luan: dem so
 * vong lap CPU thuan tuy la CO CHE KHONG DANG TIN CAY tren phan cung nay -
 * co dang ket noi debugger (halt/resume, polling nen) anh huong ro ret den
 * thoi gian THAT cua 1 vong lap don gian, theo huong khong doan truoc duoc.
 *
 * FIX THAT: doi sang dong ho phan cung chuan (SysTick/STK tuy kien truc,
 * hoan toan doc lap voi viec co dang debug hay khong, vi no la 1 peripheral
 * rieng, khong phai suy dien tu so chu ky lenh CPU thuc thi). */
#define NEON_BOOTLOADER_SYNC_TIMEOUT_MS 3000U

/*==================================================================================================
*                                   PRIVATE FUNCTION PROTOTYPES
==================================================================================================*/

static int bootloader_main_app_header_valid(void);
static void bootloader_main_run_update_mode(void);

/*==================================================================================================
*                                        PRIVATE FUNCTIONS
==================================================================================================*/

static int bootloader_main_app_header_valid(void) {
    const image_header_t *hdr = (const image_header_t *)NEON_MDF_APP_BASE;
    return (hdr->magic == IMAGE_MAGIC) && (hdr->size > 0U) && (hdr->size <= NEON_MDF_APP_SIZE);
}

static void bootloader_main_run_update_mode(void) {
    neon_bootloader_uart_send_byte(NEON_BOOTLOADER_READY_BYTE);

    uint8_t hdr_bytes[IMAGE_HEADER_SIZE];
    for (uint32_t i = 0; i < IMAGE_HEADER_SIZE; i++) {
        hdr_bytes[i] = neon_bootloader_uart_recv_byte();
    }
    image_header_t hdr;
    __builtin_memcpy(&hdr, hdr_bytes, sizeof(hdr));

    if (hdr.magic != IMAGE_MAGIC || hdr.size == 0U || hdr.size > NEON_MDF_APP_SIZE) {
        neon_bootloader_uart_send_byte(NEON_BOOTLOADER_NAK_BYTE);
        return;
    }

    /* Xoa tu APP_BASE (noi header 16 byte se duoc ghi) den het anh app that -
   * neon_flash_erase_region tu tinh sector/page nao can erase, khong con gioi
   * han 1 sector co dinh nhu ban dau (xem components/driver/flash). */
    if (neon_flash_erase_region(NEON_MDF_APP_BASE, NEON_MDF_IMAGE_HEADER_SIZE + hdr.size) != NEON_OK) {
        neon_bootloader_uart_send_byte(NEON_BOOTLOADER_NAK_BYTE);
        return;
    }

    uint32_t header_words[4];
    __builtin_memcpy(header_words, &hdr, sizeof(hdr));
    for (uint32_t i = 0; i < 4U; i++) {
        if (neon_flash_program_word(NEON_MDF_APP_BASE + i * 4U, header_words[i]) != NEON_OK) {
            neon_bootloader_uart_send_byte(NEON_BOOTLOADER_NAK_BYTE);
            return;
        }
    }

    /* ACK SAU KHI erase+ghi header xong, KHONG phai ngay sau khi validate
   * header - erase la thao tac cham (hang tram ms), bootloader KHONG poll
   * UART trong luc do. Neu ACK som, host se bat dau gui chunk data ngay
   * trong luc bootloader con dang ban erase -> mat byte dau -> bootloader
   * (cho byte tiep, khong timeout) va host (cho ACK, co timeout) deadlock
   * voi nhau. Loi nay tung xay ra that luc test tren phan cung that. */
    neon_bootloader_uart_send_byte(NEON_BOOTLOADER_ACK_BYTE);

    uint32_t remaining = hdr.size;
    uint32_t write_addr = NEON_MDF_APP_VECTOR_BASE;
    while (remaining > 0U) {
        uint8_t chunk[NEON_BOOTLOADER_CHUNK_SIZE];
        uint32_t this_chunk = (remaining < NEON_BOOTLOADER_CHUNK_SIZE) ? remaining : NEON_BOOTLOADER_CHUNK_SIZE;
        for (uint32_t i = 0; i < NEON_BOOTLOADER_CHUNK_SIZE; i++) {
            chunk[i] = (i < this_chunk) ? neon_bootloader_uart_recv_byte()
                                        : 0xFFU; /* pad chunk cuoi bang 0xFF (giong trang thai flash da xoa) */
        }
        for (uint32_t i = 0; i < NEON_BOOTLOADER_CHUNK_SIZE; i += 4U) {
            uint32_t word;
            __builtin_memcpy(&word, &chunk[i], 4U);
            if (neon_flash_program_word(write_addr + i, word) != NEON_OK) {
                neon_bootloader_uart_send_byte(NEON_BOOTLOADER_NAK_BYTE);
                return;
            }
        }
        write_addr += NEON_BOOTLOADER_CHUNK_SIZE;
        remaining = (remaining > NEON_BOOTLOADER_CHUNK_SIZE) ? (remaining - NEON_BOOTLOADER_CHUNK_SIZE) : 0U;
        neon_bootloader_uart_send_byte(NEON_BOOTLOADER_ACK_BYTE);
    }

    uint32_t crc = neon_bootloader_crc32((const void *)NEON_MDF_APP_VECTOR_BASE, hdr.size);
    if (crc == hdr.crc32) {
        neon_bootloader_uart_send_byte(NEON_BOOTLOADER_ACK_BYTE);
        neon_bootloader_arch_jump_to_app(NEON_MDF_APP_VECTOR_BASE);
    } else {
        neon_bootloader_uart_send_byte(NEON_BOOTLOADER_NAK_BYTE);
        /* Anh sai - KHONG nhay vao. Dung yen, cho lan update tiep theo (reset
     * board de thu lai tu dau, vi vong lap chinh cua bootloader chi chay
     * qua giai doan SYNC 1 lan luc boot). */
        for (;;) {
        }
    }
}

#if defined(NEON_MDF_FLASHMAP_ENABLED)
/*
 * ==========================================================================
 * Logic partition-switch/OTA MOI (chi khi flashmap.csv co that) - xem
 * neon_ota.h + plan file cho thiet ke day du. Mo hinh BAT DOI XUNG:
 *   - factory (NEON_OTA_SLOT_FACTORY) - KHONG BAO GIO bi OTA dung toi.
 *   - ota (NEON_OTA_SLOT_OTA) - CHI OTA moi ghi vao qua run_update_mode_flashmap().
 * An toan: xac nhan + tu dong rollback - xem boot_flashmap().
 * ==========================================================================
 */

/* Verify CA magic/size (giong app_header_valid() ban goc) LAN CRC32 (ban
 * goc KHONG check CRC luc cold-boot, chi check luc UART nhan xong) - o day
 * check CRC LUON, ca cho factory, vi day la nhanh MOI, doi hoi an toan cao
 * hon (phat hien factory bi hong flash ngoai y muon, khong chi phat hien
 * OTA loi). */
static int bootloader_main_image_header_valid_at(uint32_t base, uint32_t vector_base, uint32_t max_code_size) {
    const image_header_t *hdr = (const image_header_t *)base;
    if (hdr->magic != IMAGE_MAGIC || hdr->size == 0U || hdr->size > max_code_size) {
        return 0;
    }
    uint32_t crc = neon_bootloader_crc32((const void *)vector_base, hdr->size);
    return (crc == hdr->crc32) ? 1 : 0;
}

static void bootloader_main_jump_factory_or_halt(void) {
    if (bootloader_main_image_header_valid_at(NEON_MDF_FLASHMAP_APP_BASE, NEON_MDF_FLASHMAP_APP_VECTOR_BASE,
                                              NEON_MDF_FLASHMAP_APP_CODE_SIZE)) {
        neon_bootloader_arch_jump_to_app(NEON_MDF_FLASHMAP_APP_VECTOR_BASE);
    }
    /* Factory hong - truong hop CUC HIEM (khong bao gio bi OTA dung toi,
   * chi co the hong do JTAG ghi sai) - khong con noi de lui, dung yen
   * giong het hanh vi "khong co app hop le" cua ban khong-flashmap. */
    for (;;) {
    }
}

static void bootloader_main_rollback_to_factory(void) {
    neon_ota_data_t d;
    d.active_slot = NEON_OTA_SLOT_FACTORY;
    d.state = NEON_OTA_STATE_VALID;
    d.boot_attempts = 0;
    if (!neon_ota_write_data(&d)) {
        for (;;) {
        }
    }
}

static void bootloader_main_boot_flashmap(void) {
    neon_ota_data_t d;
    if (!neon_ota_read_data(&d) || d.active_slot == NEON_OTA_SLOT_FACTORY) {
        /* Chua tung OTA (otadata rong/hong - coi nhu factory mac dinh, KHONG
     * ghi otadata gi ca luc nay - tranh erase flash thua o lan boot dau
     * tien), HOAC da o factory san (sau rollback lan truoc) - nhay factory. */
        bootloader_main_jump_factory_or_halt();
        return; /* khong bao gio toi day */
    }

    /* active_slot == NEON_OTA_SLOT_OTA */
    if (!bootloader_main_image_header_valid_at(NEON_MDF_FLASHMAP_APP_OTA_BASE, NEON_MDF_FLASHMAP_APP_OTA_VECTOR_BASE,
                                               NEON_MDF_FLASHMAP_APP_OTA_CODE_SIZE)) {
        /* CRC/magic sai - rollback NGAY, khong doi het luot thu (loi toan ven
     * du lieu ro rang, khac voi "logic loi nhung CRC dung" ma boot_attempts
     * o duoi xu ly). */
        bootloader_main_rollback_to_factory();
        bootloader_main_jump_factory_or_halt();
        return;
    }

    if (d.state == NEON_OTA_STATE_PENDING_VERIFY) {
        d.boot_attempts++;
        if (d.boot_attempts > NEON_OTA_MAX_BOOT_ATTEMPTS) {
            bootloader_main_rollback_to_factory();
            bootloader_main_jump_factory_or_halt();
            return;
        }
        if (!neon_ota_write_data(&d)) {
            for (;;) {
            }
        }
    }
    /* d.state == VALID -> da xac nhan tot roi, nhay thang, khong ghi otadata
   * gi them (tranh erase flash moi lan boot binh thuong). */

    neon_bootloader_arch_jump_to_app(NEON_MDF_FLASHMAP_APP_OTA_VECTOR_BASE);
}

/* Ban SAO CHINH SUA cua run_update_mode() (KHONG goi lai ham goc, tranh
 * tham so hoa ham goc lam anh huong nhanh #else) - 2 khac biet CHINH:
 *   1. Ghi vao slot OTA (NEON_MDF_FLASHMAP_APP_OTA_*) thay vi
 *      NEON_MDF_APP_BASE co dinh - slot OTA la DUY NHAT OTA tung dung toi,
 *      khong co khai niem "slot dang trong" phai tinh (khac A/B doi xung).
 *   2. Sau khi CRC dung: GHI OTADATA (pending_verify) + RESET, thay vi nhay
 *      thang - di lai DUNG 1 duong verify+switch qua boot_flashmap() o
 *      tren, tranh trung lap logic rollback/threshold. */
static void bootloader_main_run_update_mode_flashmap(void) {
    neon_bootloader_uart_send_byte(NEON_BOOTLOADER_READY_BYTE);

    uint8_t hdr_bytes[IMAGE_HEADER_SIZE];
    for (uint32_t i = 0; i < IMAGE_HEADER_SIZE; i++) {
        hdr_bytes[i] = neon_bootloader_uart_recv_byte();
    }
    image_header_t hdr;
    __builtin_memcpy(&hdr, hdr_bytes, sizeof(hdr));

    if (hdr.magic != IMAGE_MAGIC || hdr.size == 0U || hdr.size > NEON_MDF_FLASHMAP_APP_OTA_CODE_SIZE) {
        neon_bootloader_uart_send_byte(NEON_BOOTLOADER_NAK_BYTE);
        return;
    }

    if (neon_flash_erase_region(NEON_MDF_FLASHMAP_APP_OTA_BASE, NEON_MDF_IMAGE_HEADER_SIZE + hdr.size) != NEON_OK) {
        neon_bootloader_uart_send_byte(NEON_BOOTLOADER_NAK_BYTE);
        return;
    }

    uint32_t header_words[4];
    __builtin_memcpy(header_words, &hdr, sizeof(hdr));
    for (uint32_t i = 0; i < 4U; i++) {
        if (neon_flash_program_word(NEON_MDF_FLASHMAP_APP_OTA_BASE + i * 4U, header_words[i]) != NEON_OK) {
            neon_bootloader_uart_send_byte(NEON_BOOTLOADER_NAK_BYTE);
            return;
        }
    }

    /* ACK SAU KHI erase+ghi header xong - xem comment tuong tu trong
   * run_update_mode() ban goc, ly do giong het (tranh deadlock luc erase). */
    neon_bootloader_uart_send_byte(NEON_BOOTLOADER_ACK_BYTE);

    uint32_t remaining = hdr.size;
    uint32_t write_addr = NEON_MDF_FLASHMAP_APP_OTA_VECTOR_BASE;
    while (remaining > 0U) {
        uint8_t chunk[NEON_BOOTLOADER_CHUNK_SIZE];
        uint32_t this_chunk = (remaining < NEON_BOOTLOADER_CHUNK_SIZE) ? remaining : NEON_BOOTLOADER_CHUNK_SIZE;
        for (uint32_t i = 0; i < NEON_BOOTLOADER_CHUNK_SIZE; i++) {
            chunk[i] = (i < this_chunk) ? neon_bootloader_uart_recv_byte() : 0xFFU;
        }
        for (uint32_t i = 0; i < NEON_BOOTLOADER_CHUNK_SIZE; i += 4U) {
            uint32_t word;
            __builtin_memcpy(&word, &chunk[i], 4U);
            if (neon_flash_program_word(write_addr + i, word) != NEON_OK) {
                neon_bootloader_uart_send_byte(NEON_BOOTLOADER_NAK_BYTE);
                return;
            }
        }
        write_addr += NEON_BOOTLOADER_CHUNK_SIZE;
        remaining = (remaining > NEON_BOOTLOADER_CHUNK_SIZE) ? (remaining - NEON_BOOTLOADER_CHUNK_SIZE) : 0U;
        neon_bootloader_uart_send_byte(NEON_BOOTLOADER_ACK_BYTE);
    }

    uint32_t crc = neon_bootloader_crc32((const void *)NEON_MDF_FLASHMAP_APP_OTA_VECTOR_BASE, hdr.size);
    if (crc == hdr.crc32) {
        neon_ota_data_t d;
        d.active_slot = NEON_OTA_SLOT_OTA;
        d.state = NEON_OTA_STATE_PENDING_VERIFY;
        d.boot_attempts = 0;
        if (!neon_ota_write_data(&d)) {
            neon_bootloader_uart_send_byte(NEON_BOOTLOADER_NAK_BYTE);
            return;
        }

        neon_bootloader_uart_send_byte(NEON_BOOTLOADER_ACK_BYTE);
        neon_bootloader_arch_system_reset();
    } else {
        neon_bootloader_uart_send_byte(NEON_BOOTLOADER_NAK_BYTE);
        for (;;) {
        }
    }
}
#endif /*==================================================================================================
*                                         GLOBAL FUNCTIONS
==================================================================================================*/

/**
 * @brief Initialize the application and run its main loop.
 * @return The application normally runs indefinitely.
 * @details
 * NEON_MDF_FLASHMAP_ENABLED
 */
int main(void) {
    neon_bootloader_uart_init();

    neon_bootloader_arch_systick_start1ms();
    uint32_t elapsed_ms = 0U;
    uint8_t b = 0U;
    int got_byte = 0;
    while (elapsed_ms < NEON_BOOTLOADER_SYNC_TIMEOUT_MS) {
        if (neon_bootloader_uart_try_recv_byte(&b)) {
            got_byte = 1;
            break;
        }
        if (neon_bootloader_arch_systick1ms_elapsed()) {
            elapsed_ms++;
        }
    }

#if defined(NEON_MDF_FLASHMAP_ENABLED)
    if (got_byte && b == NEON_BOOTLOADER_SYNC_BYTE) {
        bootloader_main_run_update_mode_flashmap();
        /* run_update_mode_flashmap() hoac RESET (khong quay lai day), hoac dung
     * yen vinh vien luc CRC sai (for(;;) o trong ham). */
    }

    bootloader_main_boot_flashmap(); /* khong bao gio quay lai day - neon_bootloader_arch_jump_to_app()/jump_factory_or_halt() la diem cuoi */
#else
    if (got_byte && b == NEON_BOOTLOADER_SYNC_BYTE) {
        bootloader_main_run_update_mode();
        /* run_update_mode() hoac nhay vao app (khong quay lai day), hoac dung
     * yen vinh vien luc CRC sai (for(;;) o tren). */
    }

    if (bootloader_main_app_header_valid()) {
        neon_bootloader_arch_jump_to_app(NEON_MDF_APP_VECTOR_BASE);
    }

    /* Khong co app hop le va khong update - dung yen (KHONG co UI recovery
   * o day). */
    for (;;) {
    }
#endif
}
