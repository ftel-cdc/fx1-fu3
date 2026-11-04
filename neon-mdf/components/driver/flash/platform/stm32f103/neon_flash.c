/**
 * @file neon_flash.c
 * @brief Implementation for Flash.
 * @details
 * * STM32F103 medium-density FLASH backend (RM0008, Flash memory interface).
 * Erase granularity is one 1 KiB page. Programming uses two 16-bit
 * half-words because STM32F1 has no STM32F4-style 32-bit PSIZE mode.
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

#define FLASH_SR_BSY (1U << 0)
#define FLASH_SR_PGERR (1U << 2)
#define FLASH_SR_WRPRTERR (1U << 4)
#define FLASH_SR_EOP (1U << 5)

#define FLASH_CR_PG (1U << 0)
#define FLASH_CR_PER (1U << 1)
#define FLASH_CR_STRT (1U << 6)
#define FLASH_CR_LOCK (1U << 7)

#define FLASH_STM32F103_FLASH_BASE 0x08000000U
#define FLASH_STM32F103_FLASH_END 0x08020000U
#define FLASH_STM32F103_PAGE_SIZE 0x400U

/*==================================================================================================
*                                   PRIVATE FUNCTION PROTOTYPES
==================================================================================================*/

static void flash_wait_busy(void);
static void flash_unlock(void);
static void flash_clear_status(void);
static void flash_erase_page(uint32_t page_address);
static void flash_program_half_word(uint32_t address, uint16_t data);

/*==================================================================================================
*                                         GLOBAL FUNCTIONS
==================================================================================================*/

/** @copydoc neon_flash_erase_region */
neon_status_t neon_flash_erase_region(uint32_t address, uint32_t size) {
    if (size == 0U || address < FLASH_STM32F103_FLASH_BASE || address >= FLASH_STM32F103_FLASH_END ||
        size > (FLASH_STM32F103_FLASH_END - address)) {
        return NEON_INVALID_PARAM;
    }

    uint32_t first_page = address & ~(FLASH_STM32F103_PAGE_SIZE - 1U);
    uint32_t last_address = address + size - 1U;
    uint32_t last_page = last_address & ~(FLASH_STM32F103_PAGE_SIZE - 1U);

    for (uint32_t page = first_page; page <= last_page; page += FLASH_STM32F103_PAGE_SIZE) {
        flash_erase_page(page);
    }
    return NEON_OK;
}

/** @copydoc neon_flash_program_word */
neon_status_t neon_flash_program_word(uint32_t address, uint32_t data) {
    if (((address & 0x3U) != 0U) || (address < FLASH_STM32F103_FLASH_BASE) ||
        (address > (FLASH_STM32F103_FLASH_END - sizeof(uint32_t)))) {
        return NEON_INVALID_PARAM;
    }
    flash_program_half_word(address, (uint16_t)data);
    flash_program_half_word(address + 2U, (uint16_t)(data >> 16U));
    return (*((volatile const uint32_t *)address) == data) ? NEON_OK : NEON_ERROR;
}

/*==================================================================================================
*                                        PRIVATE FUNCTIONS
==================================================================================================*/

static void flash_wait_busy(void) {
    while ((FLASH->SR & FLASH_SR_BSY) != 0U) {
    }
}

static void flash_unlock(void) {
    if ((FLASH->CR & FLASH_CR_LOCK) != 0U) {
        FLASH->KEYR = FLASH_KEY1;
        FLASH->KEYR = FLASH_KEY2;
    }
}

static void flash_clear_status(void) {
    FLASH->SR = FLASH_SR_EOP | FLASH_SR_PGERR | FLASH_SR_WRPRTERR;
}

static void flash_erase_page(uint32_t page_address) {
    flash_wait_busy();
    flash_unlock();
    flash_clear_status();

    FLASH->CR = FLASH_CR_PER;
    FLASH->AR = page_address;
    FLASH->CR |= FLASH_CR_STRT;
    flash_wait_busy();
    FLASH->CR &= ~FLASH_CR_PER;
}

static void flash_program_half_word(uint32_t address, uint16_t data) {
    flash_wait_busy();
    flash_unlock();
    flash_clear_status();

    FLASH->CR = FLASH_CR_PG;
    *((volatile uint16_t *)address) = data;
    flash_wait_busy();
    FLASH->CR &= ~FLASH_CR_PG;
}
