/**
 * @file neon_flash.c
 * @brief Implementation for Flash.
 * @details
 * * platform/weact_f412/neon_flash.c - real implementation for STM32F412 FLASH
 * controller; generalized from Bootloader_WeAct/Src/bootloader_flash.c
 * (initial only erase/write with sector specifier 2) to erase-by-area-dia-only periodic, using struct
 * FLASH_TypeDef generated from mcu-description (see mcu-description/devices/WEACT_F412.yaml,
 * peripheral "FLASH") seen because 1 separately struct hand-roll as before.
 *
 * register/confirm value via real source (no guess):
 *   - FLASH_KEY1=0x45670123, FLASH_KEY2=0xCDEF89AB
 *     (STMicroelectronics/stm32f4xx_hal_driver, Inc/stm32f4xx_hal_flash.h)
 *   - Sector map STM32F412 (RM0402): sector 0-3 = 16KB, sector 4 = 64KB,
 *     sector 5-7 = 128KB - confirmed via WebSearch, not automatically guess F0/L0 (2 dynamics that use
 * page-based uniformity, F4 use irregular sector). by sector is HARDCODE with translation (not
 * generated from mcu-description) - map irregular sector of STM32F4 can not be filled in through
 * `memory:` don current time, the same way GPIO driver still hand-write logic register separately
 * for each family to see because generator from generating.
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

#define FLASH_SR_EOP (1U << 0)
#define FLASH_SR_BSY (1U << 16)

#define FLASH_CR_PG (1U << 0)
#define FLASH_CR_SER (1U << 1)
#define FLASH_CR_SNB_POS 3
#define FLASH_CR_PSIZE_WORD                                                                                            \
    (2U << 8) /* PSIZE=10b selects 32-bit programming. The board supply is 3.3V (>2.7V; see RM0402) */
#define FLASH_CR_STRT (1U << 16)
#define FLASH_CR_LOCK (1U << 31)

/*==================================================================================================
*                                  STRUCTURES AND OTHER TYPEDEFS
==================================================================================================*/
typedef struct {
    uint32_t base;
    uint32_t size;
} flash_sector_t;

/*==================================================================================================
*                                         LOCAL VARIABLES
==================================================================================================*/
static const flash_sector_t SECTORS[8] = {
    {0x08000000U, 0x04000U}, /* sector 0, 16KB */
    {0x08004000U, 0x04000U}, /* sector 1, 16KB */
    {0x08008000U, 0x04000U}, /* sector 2, 16KB */
    {0x0800C000U, 0x04000U}, /* sector 3, 16KB */
    {0x08010000U, 0x10000U}, /* sector 4, 64KB */
    {0x08020000U, 0x20000U}, /* sector 5, 128KB */
    {0x08040000U, 0x20000U}, /* sector 6, 128KB */
    {0x08060000U, 0x20000U}, /* sector 7, 128KB */
};

/*==================================================================================================
*                                   PRIVATE FUNCTION PROTOTYPES
==================================================================================================*/

static void flash_wait_busy(void);
static void flash_unlock(void);
static void flash_erase_sector(uint32_t index);

/*==================================================================================================
*                                         GLOBAL FUNCTIONS
==================================================================================================*/

/** @copydoc neon_flash_erase_region */
neon_status_t neon_flash_erase_region(uint32_t address, uint32_t size) {
    if ((size == 0U) || (address < FLASH_MEM_BASE) || (address >= (FLASH_MEM_BASE + 0x80000U)) ||
        (size > ((FLASH_MEM_BASE + 0x80000U) - address))) {
        return NEON_INVALID_PARAM;
    }
    uint32_t end = address + size;
    for (uint32_t i = 0; i < 8U; i++) {
        uint32_t sec_start = SECTORS[i].base;
        uint32_t sec_end = sec_start + SECTORS[i].size;
        if (sec_start < end && sec_end > address) {
            flash_erase_sector(i);
        }
    }
    return NEON_OK;
}

/** @copydoc neon_flash_program_word */
neon_status_t neon_flash_program_word(uint32_t address, uint32_t data) {
    if (((address & 0x3U) != 0U) || (address < FLASH_MEM_BASE) ||
        (address > (FLASH_MEM_BASE + 0x80000U - sizeof(uint32_t)))) {
        return NEON_INVALID_PARAM;
    }
    flash_wait_busy();
    flash_unlock(); /* in case of calling this function without pre-erasing */

    FLASH->CR = FLASH_CR_PG | FLASH_CR_PSIZE_WORD;
    *((volatile uint32_t *)address) = data;
    flash_wait_busy();
    FLASH->CR &= ~FLASH_CR_PG;
    return (*((volatile const uint32_t *)address) == data) ? NEON_OK : NEON_ERROR;
}

/*==================================================================================================
*                                        PRIVATE FUNCTIONS
==================================================================================================*/

static void flash_wait_busy(void) {
    while (FLASH->SR & FLASH_SR_BSY) {
        /* poll */
    }
}

static void flash_unlock(void) {
    if (FLASH->CR & FLASH_CR_LOCK) {
        FLASH->KEYR = FLASH_KEY1;
        FLASH->KEYR = FLASH_KEY2;
    }
}

static void flash_erase_sector(uint32_t index) {
    flash_wait_busy();
    flash_unlock();

    FLASH->SR = FLASH_SR_EOP; /* clear has old EOP (write 1 to clear, STM32 standard) */
    FLASH->CR = FLASH_CR_SER | (index << FLASH_CR_SNB_POS);
    FLASH->CR |= FLASH_CR_STRT;
    flash_wait_busy();
    FLASH->CR &= ~FLASH_CR_SER;
    /* do not re-lock FLASH->CR here - erase multiple sectors in a row (loop below) or the next write word
   * requires unlocking CR. Reset/app handoff relocks it; CR resets with LOCK=1
   * after reset - true silicon behavior, not assumption). */
}
