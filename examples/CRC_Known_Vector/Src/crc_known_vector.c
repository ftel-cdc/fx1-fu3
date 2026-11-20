/**
 * @file crc_known_vector.c
 * @brief CRC-8 known vectors and length boundary checks.
 * @details
 * Run from the standalone application UART terminal.
 */

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include "crc_known_vector.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "console/neon_console.h"
#include "crm/neon_crm.h"
#include "crc/neon_crc.h"
#include "uart/neon_uart.h"

#ifndef NEON_MAX_DELAY
#define NEON_MAX_DELAY 0xFFFFFFFFU
#endif

#define TEST_CRC_INSTANCE CRC
#define TEST_CRC_POLYNOMIAL NEON_CRC_POLY_SMBUS
#define TEST_CRC_TIMEOUT_POLLS 1000000U
/*==================================================================================================
*                                         GLOBAL FUNCTIONS
==================================================================================================*/

static uint8_t crc8_reference_words(const uint32_t *words, uint32_t word_count) {
    uint8_t crc = 0U;

    for (uint32_t i = 0U; i < word_count; ++i) {
        for (int bit = 31; bit >= 0; --bit) {
            const uint8_t input_bit = (uint8_t)((words[i] >> bit) & 1U);
            const uint8_t feedback = (uint8_t)(((crc >> 7) & 1U) ^ input_bit);
            crc = (uint8_t)(crc << 1);
            if (feedback != 0U) {
                crc ^= (uint8_t)TEST_CRC_POLYNOMIAL;
            }
        }
    }
    return crc;
}

static bool run_vector_test(const char *name, const uint32_t *words, uint32_t word_count, uint8_t expected_crc) {
    uint32_t calculated_crc = 0U;
    const uint8_t reference_crc = crc8_reference_words(words, word_count);
    const neon_status_t status = neon_crc_calculate(TEST_CRC_INSTANCE, TEST_CRC_POLYNOMIAL, words, word_count,
                                                    (uint8_t *)&calculated_crc, TEST_CRC_TIMEOUT_POLLS);

    printf("\n--- %s ---\n", name);
    printf("  Poly=0x%02X Init=0x00 RefIn=FALSE RefOut=FALSE XorOut=0x00\n", (unsigned int)TEST_CRC_POLYNOMIAL);
    printf("  Input (%lu 32-bit words): ", (unsigned long)word_count);
    const uint32_t words_to_print = (word_count <= 16U) ? word_count : 8U;
    for (uint32_t i = 0U; i < words_to_print; ++i) {
        printf("%08lX ", (unsigned long)words[i]);
    }
    if (word_count > 16U) {
        printf("... ");
        for (uint32_t i = word_count - 4U; i < word_count; ++i) {
            printf("%08lX ", (unsigned long)words[i]);
        }
    }
    printf("\n  Expected=0x%02X SoftwareRef=0x%02X Hardware=0x%02X\n", (unsigned int)expected_crc,
           (unsigned int)reference_crc, (unsigned int)calculated_crc);

    if (status != NEON_OK) {
        printf("  [FAIL] CRC driver status=%d\n", (int)status);
        return false;
    }
    if (reference_crc != expected_crc) {
        printf("  [FAIL] Software reference does not match the fixed expected value.\n");
        return false;
    }
    if (calculated_crc != expected_crc) {
        printf("  [FAIL] Hardware CRC does not match the fixed expected value.\n");
        return false;
    }
    printf("  [PASS] Hardware and software reference match the fixed expected value.\n");
    return true;
}

static bool run_vector_1_ascii_words(void) {
    /* Each ASCII code occupies the low byte of a separate 32-bit word. */
    static const uint32_t words[] = {'1', '2', '3', '4', '5', '6', '7', '8', '9'};
    return run_vector_test("ASCII codes in low byte of 9 words", words, (uint32_t)(sizeof(words) / sizeof(words[0])),
                           0x5BU);
}

static bool run_vector_2_packed_bytes(void) {
    /* The four bytes are packed in the order processed by the MSB-first engine. */
    static const uint32_t words[] = {0x00FFAA55U};
    return run_vector_test("Packed byte stream: 00 FF AA 55", words, (uint32_t)(sizeof(words) / sizeof(words[0])),
                           0x1DU);
}

static bool run_vector_3_odd_word_count(void) {
    /* Seven complete words exercise an odd word count and word boundaries. */
    static const uint32_t words[] = {
        0xA1B2C3D4U, 0xE5F60718U, 0x293A4B5CU, 0x6D7E8F90U, 0x10213243U, 0x54657687U, 0x98A9BACBU,
    };
    return run_vector_test("7-word fixed pattern (odd word count)", words, (uint32_t)(sizeof(words) / sizeof(words[0])),
                           0xD1U);
}

static bool run_minimum_length_test(void) {
    static const uint32_t word = 0x12345678U;
    return run_vector_test("Minimum valid length: 1 word", &word, 1U, 0x1CU);
}

static bool run_maximum_length_test(void) {
    static uint32_t words[NEON_CRC_DATA_LENGTH_MAX];
    static uint32_t initialized;

    if (!initialized) {
        for (uint32_t i = 0U; i < NEON_CRC_DATA_LENGTH_MAX; ++i) {
            /* Deterministic low-byte payload; high 24 bits are zero. */
            words[i] = (i * 37U + 0x5AU) & 0xFFU;
        }
        initialized = 1U;
    }
    return run_vector_test("Maximum valid length: 255 words", words, NEON_CRC_DATA_LENGTH_MAX, 0xE1U);
}

static bool run_invalid_length_test(void) {
    static const uint32_t word = 0x12345678U;
    uint32_t result = 0U;
    const neon_status_t zero_status = neon_crc_calculate(TEST_CRC_INSTANCE, TEST_CRC_POLYNOMIAL, &word, 0U,
                                                         (uint8_t *)&result, TEST_CRC_TIMEOUT_POLLS);
    const neon_status_t oversized_status =
        neon_crc_calculate(TEST_CRC_INSTANCE, TEST_CRC_POLYNOMIAL, &word, NEON_CRC_DATA_LENGTH_MAX + 1U,
                           (uint8_t *)&result, TEST_CRC_TIMEOUT_POLLS);

    printf("\n--- Invalid length boundaries ---\n");
    printf("  length=0 status=%d; length=%u status=%d; expected=%d\n", (int)zero_status,
           (unsigned int)(NEON_CRC_DATA_LENGTH_MAX + 1U), (int)oversized_status, (int)NEON_INVALID_PARAM);
    if (zero_status != NEON_INVALID_PARAM || oversized_status != NEON_INVALID_PARAM) {
        printf("  [FAIL] CRC API did not reject an invalid length.\n");
        return false;
    }
    printf("  [PASS] Both invalid lengths were rejected.\n");
    return true;
}

static bool run_all_vectors(void) {
    uint32_t passed = 0U;
    const uint32_t total = 6U;

    passed += run_vector_1_ascii_words() ? 1U : 0U;
    passed += run_vector_2_packed_bytes() ? 1U : 0U;
    passed += run_vector_3_odd_word_count() ? 1U : 0U;
    passed += run_minimum_length_test() ? 1U : 0U;
    passed += run_maximum_length_test() ? 1U : 0U;
    passed += run_invalid_length_test() ? 1U : 0U;

    printf("\n[SUMMARY] %lu/%lu CRC checks passed.\n", (unsigned long)passed, (unsigned long)total);
    return passed == total;
}

void crc_known_vector_run(void) {
    neon_status_t status;
    neon_uart_handle_t *uart = neon_console_get_uart();
    bool running = true;

    if (uart == NULL) {
        printf("[ERROR] Terminal UART handle is unavailable.\n");
        return;
    }
    printf("\n[INFO] Starting CRC Known Vector Testcase...\n");
    printf("[CRC-8] Polynomial=0x%02X Init=0x00 RefIn=FALSE RefOut=FALSE XorOut=0x00\n",
           (unsigned int)TEST_CRC_POLYNOMIAL);
    printf("[NOTE] API length counts 32-bit words; every word contributes all 32 bits, MSB first.\n");

    status = neon_crm_peripheral_clock_enable(NEON_CRM_PERIPHERAL_CRC);
    if (status != NEON_OK) {
        printf("[ERROR] CRC clock enable failed (Status=%d).\n", (int)status);
        return;
    }
    status = neon_crm_peripheral_reset(NEON_CRM_PERIPHERAL_CRC);
    if (status != NEON_OK) {
        printf("[ERROR] CRC reset failed (Status=%d).\n", (int)status);
        (void)neon_crm_peripheral_clock_disable(NEON_CRM_PERIPHERAL_CRC);
        return;
    }

    printf("[INFO] CRC clock enabled and peripheral reset released.\n");
    printf("\n=================== CRC-8 KNOWN VECTOR TEST MENU ===================\n");
    printf("  [1] ASCII codes in low byte of 9 words\n");
    printf("  [2] Packed 4-byte stream: 00 FF AA 55\n");
    printf("  [3] Fixed pattern: 7-word odd-length check\n");
    printf("  [4] Minimum valid API length (1 word)\n");
    printf("  [5] Maximum valid API length (255 words)\n");
    printf("  [6] Reject invalid API lengths (0 and 256)\n");
    printf("  [a] Run all checks\n");
    printf("  [q] Quit testcase\n");
    printf("=====================================================================\n");

    while (running) {
        uint32_t rx_word = 0U;
        printf("Select option > ");

        status = neon_uart_receive(uart, (uint8_t *)&rx_word, 1U, NEON_MAX_DELAY);
        if (status != NEON_OK) {
            continue;
        }
        const uint8_t input = (uint8_t)(rx_word & 0xFFU);
        if (input == '\r' || input == '\n') {
            continue;
        }
        printf("%c\n", input);

        switch (input) {
            case '1':
                (void)run_vector_1_ascii_words();
                break;
            case '2':
                (void)run_vector_2_packed_bytes();
                break;
            case '3':
                (void)run_vector_3_odd_word_count();
                break;
            case '4':
                (void)run_minimum_length_test();
                break;
            case '5':
                (void)run_maximum_length_test();
                break;
            case '6':
                (void)run_invalid_length_test();
                break;
            case 'a':
            case 'A':
                (void)run_all_vectors();
                break;
            case 'q':
            case 'Q':
            case 0x03:
                running = false;
                break;
            default:
                printf("[WARN] Invalid option. Select [1-6, a, q].\n");
                break;
        }
    }

    status = neon_crm_peripheral_clock_disable(NEON_CRM_PERIPHERAL_CRC);
    if (status != NEON_OK) {
        printf("[ERROR] CRC clock disable failed (Status=%d).\n", (int)status);
    } else {
        printf("[INFO] CRC clock disabled; testcase exited.\n");
    }
}
