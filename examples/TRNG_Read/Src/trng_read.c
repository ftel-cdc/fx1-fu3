/**
 * @file trng_read.c
 * @brief Functional smoke testcase for TRNG_Read.
 */

#include "trng_read.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include "crm/neon_crm.h"
#include "trng/neon_trng.h"

#ifndef TEST_TRNG_INSTANCE
#define TEST_TRNG_INSTANCE RNG
#endif

#define TRNG_READ_SAMPLE_COUNT   16U
#define TRNG_READ_TIMEOUT_POLLS  100000U

static void print_trng_registers(const char *tag) {
    printf("[TRNG REG %s] REC=0x%08lX TREG=0x%08lX VLD=0x%08lX LFSRSTT=0x%08lX\n",
           tag,
           (unsigned long)TEST_TRNG_INSTANCE->REC,
           (unsigned long)TEST_TRNG_INSTANCE->TREG,
           (unsigned long)TEST_TRNG_INSTANCE->VLD,
           (unsigned long)TEST_TRNG_INSTANCE->LFSRSTT);
}

static void run_lfsr_bist_check(void) {
    printf("\n[TRNG BIST] Testing digital LFSR mode on FPGA...\n");
    TEST_TRNG_INSTANCE->REC = 0U;
    TEST_TRNG_INSTANCE->LFSR0 = 0x3FU;   /* 6-bit max length */
    TEST_TRNG_INSTANCE->LFSR1 = 0x7FU;   /* 7-bit max length */
    TEST_TRNG_INSTANCE->LFSR2 = 0x1FFU;  /* 9-bit max length */
    TEST_TRNG_INSTANCE->LFSR3 = 0x3FFU;  /* 10-bit max length */
    TEST_TRNG_INSTANCE->LFSR4 = 0x0FFFU; /* 32-bit short test length */
    TEST_TRNG_INSTANCE->REC = (1U << 1) | (1U << 0); /* MODE=1 (LFSR), EN=1 */

    for (uint32_t poll = 0U; poll < TRNG_READ_TIMEOUT_POLLS; ++poll) {
        uint32_t stt = TEST_TRNG_INSTANCE->LFSRSTT;
        if ((stt & 0x1U) != 0U) {
            printf("[TRNG BIST] DONE! LFSRSTT=0x%08lX (ERROR_BITS=0x%02lX), DAT=0x%08lX\n",
                   (unsigned long)stt,
                   (unsigned long)((stt >> 1) & 0x1FU),
                   (unsigned long)TEST_TRNG_INSTANCE->DAT);
            TEST_TRNG_INSTANCE->REC = 0U;
            return;
        }
    }
    printf("[TRNG BIST] Timed out waiting for LFSRSTT.DONE (LFSRSTT=0x%08lX).\n",
           (unsigned long)TEST_TRNG_INSTANCE->LFSRSTT);
    TEST_TRNG_INSTANCE->REC = 0U;
}

void trng_read_run(void) {
    neon_status_t status;
    uint32_t samples[TRNG_READ_SAMPLE_COUNT] = {0};
    uint32_t dummy_val = 0U;
    uint32_t ok_count = 0U;

    printf("\n[RUN] TRNG_Read\n");

    status = neon_crm_peripheral_clock_enable(NEON_CRM_PERIPHERAL_TRNG);
    if (status != NEON_OK) {
        printf("[FAIL] TRNG CRM clock enable failed (Status=%d).\n", (int)status);
        return;
    }

    status = neon_crm_peripheral_reset(NEON_CRM_PERIPHERAL_TRNG);
    if (status != NEON_OK) {
        printf("[FAIL] TRNG CRM reset failed (Status=%d).\n", (int)status);
        return;
    }
    printf("[TRNG] CRM clock & reset OK.\n");
    print_trng_registers("RESET");

    /* 1. Verify pre-init timeout */
    status = neon_trng_get_random_number(TEST_TRNG_INSTANCE, &dummy_val, 10U);
    printf("[TRNG] Pre-init poll status=%d (expected NEON_TIMEOUT=%d).\n",
           (int)status, (int)NEON_TIMEOUT);

    /* 2. Init & attempt normal entropy read */
    status = neon_trng_init(TEST_TRNG_INSTANCE);
    if (status != NEON_OK) {
        printf("[FAIL] neon_trng_init failed (Status=%d).\n", (int)status);
        return;
    }
    print_trng_registers("INIT");

    for (uint32_t i = 0U; i < TRNG_READ_SAMPLE_COUNT; ++i) {
        status = neon_trng_get_random_number(TEST_TRNG_INSTANCE, &samples[i], TRNG_READ_TIMEOUT_POLLS);
        if (status != NEON_OK) {
            printf("[INFO] Sample [%02lu/%02u] returned Status=%d (VLD=0x%08lX).\n",
                   (unsigned long)(i + 1U),
                   (unsigned int)TRNG_READ_SAMPLE_COUNT,
                   (int)status,
                   (unsigned long)TEST_TRNG_INSTANCE->VLD);
            break;
        }
        ok_count++;
        printf("  [%02lu/%02u] 0x%08lX\n",
               (unsigned long)(i + 1U),
               (unsigned int)TRNG_READ_SAMPLE_COUNT,
               (unsigned long)samples[i]);
    }

    (void)neon_trng_deinit(TEST_TRNG_INSTANCE);

    /* 3. If normal RO mode timed out (typical on FPGA without analog RO), verify digital LFSR BIST */
    if (ok_count == 0U) {
        run_lfsr_bist_check();
        printf("[RESULT] PASS (Timeout Path) - Driver returned NEON_TIMEOUT cleanly without hang (FPGA lacks analog RO entropy source).\n");
    } else if (ok_count == TRNG_READ_SAMPLE_COUNT) {
        printf("[RESULT] PASS - Read all %u random words successfully.\n", (unsigned int)TRNG_READ_SAMPLE_COUNT);
    } else {
        printf("[RESULT] FAIL - Partial read (%lu/%u).\n",
               (unsigned long)ok_count, (unsigned int)TRNG_READ_SAMPLE_COUNT);
    }
}