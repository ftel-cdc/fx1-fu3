/**
 * @file neon_flash.c
 * @brief Implementation for Flash.
 * @details
 * * platform/ch32v003/neon_flash.c - implementation THAT cho CH32V003 FLASH controller
 * (RM chuong 16 "Flash Memory and User Option Bytes", V1.9 - KHONG doan).
 *
 * KHAC BIET QUAN TRONG voi platform/weact_f412/neon_flash.c (STM32F4):
 *   - Erase: 16KB flash = 16 sector 1KB DEU NHAU (RM 16.2.2: "write-protected
 *     in 1K byte (16 pages) units") - KHONG co bang sector khong deu nhau
 *     nhu STM32F4, chi can vong lap don gian theo boi so 1KB.
 *   - Program: "Standard programming" ghi TUNG 2-BYTE (halfword) 1 lan (RM
 *     16.2.1: "CPU performs programming in single 2-byte mode") - KHAC
 *     STM32F4 ghi duoc ca 32-bit/lan (PSIZE=word) - neon_flash_program_word()
 *     van giu API ghi 1 word 32-bit ben ngoai (khong doi contract chung),
 *     nhung BEN TRONG tach thanh 2 lan ghi 16-bit.
 *   - Thanh ghi/bit KHAC dia chi voi STM32F4: STATR.BUSY=bit0 (khong phai
 *     SR.BSY=bit16), STATR.EOP=bit5 (khong phai SR.EOP=bit0), CTLR.LOCK=bit7
 *     (khong phai CR.LOCK=bit31), CTLR.PER=bit1 (erase 1 sector 1KB),
 *     CTLR.STRT=bit6, CTLR.PG=bit0 - dung ADDR (offset 0x14) de chi dinh
 *     dia chi erase/program (KHONG dung so hieu sector nhu STM32F4's SNB).
 *   - KEY1/KEY2 unlock GIONG HET STM32 (0x45670123/0xCDEF89AB - RM 16.3.2,
 *     xac nhan trung, khong phai trung hop ngau nhien - cung IP goc).
 */

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/

#include "flash/neon_flash.h"
#include "mcu_device.h"

/*==================================================================================================
*                                        DEFINES AND MACROS
==================================================================================================*/
#define FLASH_KEY1 0x45670123UL
#define FLASH_KEY2 0xCDEF89ABUL

#define FLASH_STATR_BUSY (1U << 0)
#define FLASH_STATR_EOP (1U << 5)

#define FLASH_CTLR_PG (1U << 0)
#define FLASH_CTLR_PER (1U << 1)
#define FLASH_CTLR_STRT (1U << 6)
#define FLASH_CTLR_LOCK (1U << 7)

#define FLASH_SECTOR_SIZE 0x400U /* 1KB - THAT, RM 16.2.2 */

/*==================================================================================================
*                                   PRIVATE FUNCTION PROTOTYPES
==================================================================================================*/

static void flash_wait_busy(void);
static void flash_unlock(void);
static void flash_erase_sector(uint32_t sector_addr);

/*==================================================================================================
*                                         GLOBAL FUNCTIONS
==================================================================================================*/

/** @copydoc neon_flash_erase_region */
neon_status_t neon_flash_erase_region(uint32_t address, uint32_t size) {
    if ((size == 0U) || (address >= (FLASH_MEM_BASE + 0x4000U)) || (size > ((FLASH_MEM_BASE + 0x4000U) - address))) {
        return NEON_INVALID_PARAM;
    }
    uint32_t end = address + size;
    uint32_t sector_addr = address - (address % FLASH_SECTOR_SIZE);
    for (; sector_addr < end; sector_addr += FLASH_SECTOR_SIZE) {
        flash_erase_sector(sector_addr);
    }
    return NEON_OK;
}

/** @copydoc neon_flash_program_word */
neon_status_t neon_flash_program_word(uint32_t address, uint32_t data) {
    if (((address & 0x3U) != 0U) || (address > (FLASH_MEM_BASE + 0x4000U - sizeof(uint32_t)))) {
        return NEON_INVALID_PARAM;
    }
    flash_wait_busy();
    flash_unlock();

    FLASH->CTLR |= FLASH_CTLR_PG;

    /* THAT: standard programming CHI ghi duoc 2-byte/lan (RM 16.2.1) - tach
   * 1 word 32-bit thanh 2 lan ghi halfword, doi BUSY giua 2 lan. */
    *((volatile uint16_t *)address) = (uint16_t)(data & 0xFFFFU);
    flash_wait_busy();
    *((volatile uint16_t *)(address + 2U)) = (uint16_t)(data >> 16);
    flash_wait_busy();

    FLASH->CTLR &= ~FLASH_CTLR_PG;
    return (*((volatile const uint32_t *)address) == data) ? NEON_OK : NEON_ERROR;
}

/*==================================================================================================
*                                        PRIVATE FUNCTIONS
==================================================================================================*/

static void flash_wait_busy(void) {
    while (FLASH->STATR & FLASH_STATR_BUSY) {
        /* poll */
    }
}

static void flash_unlock(void) {
    if (FLASH->CTLR & FLASH_CTLR_LOCK) {
        FLASH->KEYR = FLASH_KEY1;
        FLASH->KEYR = FLASH_KEY2;
    }
}

static void flash_erase_sector(uint32_t sector_addr) {
    flash_wait_busy();
    flash_unlock();

    FLASH->STATR = FLASH_STATR_EOP; /* clear co EOP cu (ghi 1 de clear) */
    FLASH->CTLR |= FLASH_CTLR_PER;
    FLASH->ADDR = sector_addr;
    FLASH->CTLR |= FLASH_CTLR_STRT;
    flash_wait_busy();
    FLASH->CTLR &= ~FLASH_CTLR_PER;
    /* KHONG lock lai FLASH->CTLR o day - cung ly do voi ban STM32F4 (erase
   * nhieu sector lien tiep / ghi word tiep theo can CTLR mo khoa san). */
}
