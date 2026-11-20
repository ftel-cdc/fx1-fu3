/**
     * @file aes_known_vector.c
     * @brief AES-128 ECB encryption and decryption known-answer checks.
     * @details
     * Run from the standalone application UART terminal.
     */

/*==================================================================================================
    *                                          INCLUDE FILES
    ==================================================================================================*/
#include "aes_known_vector.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "aes/neon_aes.h"
#include "crm/neon_crm.h"

#define TEST_AES_INSTANCE AES
#define AES_TEST_TIMEOUT_POLLS 1000000U

/*==================================================================================================
    *                                         GLOBAL FUNCTIONS
    ==================================================================================================*/

/* FIPS 197 Appendix C.1 AES-128 vector, represented low register word first. */
static const uint32_t test_key[4] = {
    0x0C0D0E0FU,
    0x08090A0BU,
    0x04050607U,
    0x00010203U,
};
static const uint32_t test_plaintext[4] = {
    0xCCDDEEFFU,
    0x8899AABBU,
    0x44556677U,
    0x00112233U,
};
static const uint32_t expected_ciphertext[4] = {
    0x70B4C55AU,
    0xD8CDB780U,
    0x6A7B0430U,
    0x69C4E0D8U,
};

static bool words_equal(const uint32_t actual[4], const uint32_t expected[4]) {
    for (uint32_t i = 0U; i < 4U; ++i) {
        if (actual[i] != expected[i]) {
            return false;
        }
    }
    return true;
}

static void print_block(const char *label, const uint32_t words[4]) {
    /* Display bytes in AES's conventional left-to-right order. */
    printf("  %s: %08lX %08lX %08lX %08lX\n", label, (unsigned long)words[3], (unsigned long)words[2],
           (unsigned long)words[1], (unsigned long)words[0]);
}

static bool process_known_block(const char *label, uint32_t direction, const uint32_t input[4],
                                const uint32_t expected[4]) {
    const neon_aes_config_t config = {
        .mode = NEON_AES_MODE_ECB,
        .direction = direction,
    };
    uint32_t output[4] = {0U, 0U, 0U, 0U};
    bool initialized = false;
    bool passed = false;
    neon_status_t status;

    printf("\n--- %s ---\n", label);
    print_block("Input    ", input);
    print_block("Expected ", expected);

    status = neon_aes_init(TEST_AES_INSTANCE, &config);
    if (status != NEON_OK) {
        printf("[AES][FAIL] init status=%d\n", (int)status);
        return false;
    }
    initialized = true;

    status = neon_aes_set_key128(TEST_AES_INSTANCE, test_key);
    if (status != NEON_OK) {
        printf("[AES][FAIL] set key status=%d\n", (int)status);
        goto cleanup;
    }

    status = neon_aes_process_block(TEST_AES_INSTANCE, input, output, AES_TEST_TIMEOUT_POLLS);
    if (status != NEON_OK) {
        printf("[AES][FAIL] process block %s (status=%d, budget=%lu polls)\n",
               (status == NEON_TIMEOUT) ? "timed out" : "failed", (int)status, (unsigned long)AES_TEST_TIMEOUT_POLLS);
        goto cleanup;
    }

    print_block("Actual   ", output);
    passed = words_equal(output, expected);
    printf("%s\n",
           passed ? "[AES][PASS] output matches known vector." : "[AES][FAIL] output differs from known vector.");

cleanup:
    if (initialized) {
        status = neon_aes_deinit(TEST_AES_INSTANCE);
        if (status != NEON_OK) {
            printf("[AES][FAIL] deinit status=%d\n", (int)status);
            passed = false;
        }
    }
    return passed;
}

void aes_known_vector_run(void) {
    bool passed = true;
    neon_status_t status;

    printf("\n=== AES-128 ECB Known-Answer Test ===\n");
    print_block("Key      ", test_key);
    printf("[AES] Single AES engine; ECB one-block test; timeout=%lu polls per operation.\n",
           (unsigned long)AES_TEST_TIMEOUT_POLLS);

    status = neon_crm_peripheral_clock_enable(NEON_CRM_PERIPHERAL_AES);
    if (status != NEON_OK) {
        printf("[AES][FAIL] CRM clock enable status=%d\n", (int)status);
        return;
    }

    status = neon_crm_peripheral_reset(NEON_CRM_PERIPHERAL_AES);
    if (status != NEON_OK) {
        printf("[AES][FAIL] CRM reset status=%d\n", (int)status);
        (void)neon_crm_peripheral_clock_disable(NEON_CRM_PERIPHERAL_AES);
        return;
    }

    printf("[AES] CRM clock and reset setup complete.\n");
    if (!process_known_block("Encrypt known plaintext", NEON_AES_DIRECTION_ENCRYPT, test_plaintext,
                             expected_ciphertext)) {
        passed = false;
    }
    if (!process_known_block("Decrypt known ciphertext", NEON_AES_DIRECTION_DECRYPT, expected_ciphertext,
                             test_plaintext)) {
        passed = false;
    }

    /* Reset the singleton before clock gating, including after a timeout. */
    status = neon_crm_peripheral_reset(NEON_CRM_PERIPHERAL_AES);
    if (status != NEON_OK) {
        printf("[AES][FAIL] final CRM reset status=%d\n", (int)status);
        passed = false;
    }
    status = neon_crm_peripheral_clock_disable(NEON_CRM_PERIPHERAL_AES);
    if (status != NEON_OK) {
        printf("[AES][FAIL] CRM clock disable status=%d\n", (int)status);
        passed = false;
    }

    printf("%s\n", passed ? "[AES][PASS] encrypt and decrypt known-answer checks passed."
                          : "[AES][FAIL] one or more known-answer checks failed.");
}
