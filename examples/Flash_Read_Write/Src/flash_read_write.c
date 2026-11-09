/**
 * @file flash_read_write.c
 * @brief Implementation for Flash read write.
 * @details
 * FU2 main Flash smoke test using the public driver API.
 */

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/

#include "flash_read_write.h"
#include <stdint.h>
#include <stdio.h>

#include "flash/neon_flash.h"

_Static_assert(NEON_FLASH_SECTOR_SIZE == 0x200U, "Flash_Read_Write linker reservation must match sector size");

/*==================================================================================================
*                                         LOCAL VARIABLES
==================================================================================================*/
extern uint32_t __flash_rw_start__;

/*==================================================================================================
*                                   PRIVATE FUNCTION PROTOTYPES
==================================================================================================*/

static uint32_t flash_read_write_pattern(uint32_t index);

/*==================================================================================================
*                                         GLOBAL FUNCTIONS
==================================================================================================*/

/** @copydoc flash_read_write_run */
void flash_read_write_run(void) {
    const uint32_t scratch = (uint32_t)(uintptr_t)&__flash_rw_start__;
    const uint32_t words = NEON_FLASH_SECTOR_SIZE / sizeof(uint32_t);
    uint32_t address = scratch;
    uint32_t actual = 0U;
    uint32_t expected = 0xFFFFFFFFU;
    const char *operation = "config";
    neon_flash_config_t config;
    neon_flash_get_default_config(&config);

    printf("[INFO] Flash scratch: 0x%08lX, %lu bytes; contents will be erased.\n", (unsigned long)scratch,
           (unsigned long)NEON_FLASH_SECTOR_SIZE);
    neon_status_t status = neon_flash_init(&config);
    if (status != NEON_OK) {
        goto error;
    }

    operation = "EraseSector";
    printf("[INFO] Erasing scratch sector.\n");
    status = neon_flash_erase_sector(scratch);
    if (status != NEON_OK) {
        goto error;
    }

    operation = "Read erased sector";
    for (uint32_t index = 0U; index < words; ++index) {
        address = scratch + index * sizeof(uint32_t);
        status = neon_flash_read_word(address, &actual);
        if (status != NEON_OK) {
            goto error;
        }
        if (actual != expected) {
            goto mismatch;
        }
    }
    printf("[PASS] Erase read-back: %lu words are 0xFFFFFFFF.\n", (unsigned long)words);

    operation = "ProgramWord";
    printf("[INFO] Programming %lu pattern words.\n", (unsigned long)words);
    for (uint32_t index = 0U; index < words; ++index) {
        address = scratch + index * sizeof(uint32_t);
        status = neon_flash_program_word(address, flash_read_write_pattern(index));
        if (status != NEON_OK) {
            goto error;
        }
    }

    operation = "Read pattern";
    for (uint32_t index = 0U; index < words; ++index) {
        address = scratch + index * sizeof(uint32_t);
        expected = flash_read_write_pattern(index);
        status = neon_flash_read_word(address, &actual);
        if (status != NEON_OK) {
            goto error;
        }
        if (actual != expected) {
            goto mismatch;
        }
    }
    printf("[PASS] Flash_Read_Write: %lu/%lu words match.\n", (unsigned long)words, (unsigned long)words);
    goto cleanup;

error:
    printf("[FAIL] Flash %s at 0x%08lX: status=%lu error=0x%08lX\n", operation, (unsigned long)address,
           (unsigned long)status, (unsigned long)neon_flash_get_error());
    goto cleanup;

mismatch:
    printf("[FAIL] Flash %s at 0x%08lX: expected=0x%08lX actual=0x%08lX\n", operation, (unsigned long)address,
           (unsigned long)expected, (unsigned long)actual);

cleanup:
    neon_flash_deinit();
}

/*==================================================================================================
*                                        PRIVATE FUNCTIONS
==================================================================================================*/

static uint32_t flash_read_write_pattern(uint32_t index) {
    static const uint32_t patterns[] = {
        0x00000000U, 0xFFFFFFFFU, 0xAAAAAAAAU, 0x55555555U, 0x01234567U, 0x89ABCDEFU, 0xA5A5A5A5U, 0x5A5A5A5AU,
    };
    /* Change each group too, so an address alias cannot pass read-back. */
    return patterns[index % 8U] ^ (index / 8U);
}
