/**
 * @file neon_flash.c
 * @brief Implementation for Flash.
 * @details
 * * platform/ch32v003/neon_flash.c - real implementation for CH32V003 FLASH controller (RM chapter 16
 * "Flash Memory and User Option Bytes", V1.9 - no guessing).
 *
 * other know IMPORTANT with platform/weact_f412/neon_flash.c(STM32F4):
 *   - Erase: 16KB flash consists of 16 equal 1KB sectors (RM 16.2.2: "write-protected
 *     in 1K bytes (16 pages) units") - not equal to irregular sectors like STM32F4, just loop spaced by
 *     the number of 1KB.
 *   - Program: "Standard programming" write each 2-BYTE (halfword) 1 time (RM
 *     16.2.1: "CPU performs programming in single 2-byte mode") - unlike STM32F4 which writes both
 *     32 bits per operation (PSIZE=word). neon_flash_program_word() retains the public 32-bit API, but
 *     internally splits each word into two 16-bit writes.
 *   - Register addresses and bits differ from STM32F4: STATR.BUSY=bit0 rather than SR.BSY=bit16, and STATR.EOP=bit5 rather than
 *     SR.EOP=bit0), CTLR.LOCK=bit7 (not CR.LOCK=bit31), CTLR.PER=bit1 (erase 1 sector 1KB),
 *     CTLR.STRT=bit6, CTLR.PG=bit0 - use ADDR (offset 0x14) to specify address erase/program (do
 *     does not use sector-number encoding like STM32F4 SNB).
 *   - KEY1/KEY2 unlocking matches STM32: 0x45670123/0xCDEF89AB (RM 16.3.2,
 *     confirms the middle, not the middle of the spectrum - same underlying IP).
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

    /* Standard programming writes 2 bytes per operation (RM 16.2.1), so split a 32-bit word into two writes
   * halfword, change BUSY between 2 times. */
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

    FLASH->STATR = FLASH_STATR_EOP; /* clear has old EOP (write 1 to clear) */
    FLASH->CTLR |= FLASH_CTLR_PER;
    FLASH->ADDR = sector_addr;
    FLASH->CTLR |= FLASH_CTLR_STRT;
    flash_wait_busy();
    FLASH->CTLR &= ~FLASH_CTLR_PER;
    /* Do not relock FLASH->CTLR here. Like STM32F4, callers may erase multiple sectors or write
   * next word that requires CTLR to open journal). */
}
