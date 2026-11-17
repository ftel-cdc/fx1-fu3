/**
 * @file rtc_calendar.c
 * @brief Functional testcase for RTC_Calendar.
 * @details
 * Verifies RTC initialization via CRM, date/time readback, 1-second tick progression,
 * minute/day/leap-year rollover, and boundary input validation.
 */

/*==================================================================================================
-                                          INCLUDE FILES
==================================================================================================*/
#include "rtc_calendar.h"
#include <stdbool.h>
#include <stdio.h>
#include "console/neon_console.h"
#include "crm/neon_crm.h"
#include "rtc/neon_rtc.h"
#include "sdkconfig.h"
#include "uart/neon_uart.h"

/*==================================================================================================
-                                        DEFINES AND MACROS
==================================================================================================*/
#ifndef NEON_MAX_DELAY
#define NEON_MAX_DELAY 0xFFFFFFFFU
#endif

/*
 * FU2 CRM Spec v3.3 Section 3.7: RTC core clock operates at 36 kHz[cite: 67].
 * DTI RTC UG Section 5: Divisor = 36,000 Hz / 100 Hz = 360[cite: 16].
 */
#ifndef CONFIG_RTC_CALENDAR_PRESCALER
#define CONFIG_RTC_CALENDAR_PRESCALER 360U
#endif

#define TEST_RTC_INSTANCE        RTC
#define RTC_CPSR_EXPECTED_RESET  0x00002710U
#define RTC_WAIT_POLL_LIMIT      100000000U

/*==================================================================================================
-                                   PRIVATE FUNCTION PROTOTYPES
==================================================================================================*/
static bool time_equal(const neon_rtc_time_t *a, const neon_rtc_time_t *b);
static bool date_equal(const neon_rtc_date_t *a, const neon_rtc_date_t *b);
static neon_status_t read_consistent_datetime(neon_rtc_date_t *date, neon_rtc_time_t *time);
static bool wait_for_time(const neon_rtc_time_t *expected, neon_rtc_time_t *actual);
static bool wait_for_date(const neon_rtc_date_t *expected, neon_rtc_date_t *actual);
static void print_hardware_registers(void);
static bool set_and_check_time(const neon_rtc_time_t *expected);
static bool set_and_check_date(const neon_rtc_date_t *expected);
static bool wait_for_next_second(const neon_rtc_time_t *previous, neon_rtc_time_t *actual);
static bool wait_for_datetime_rollover(const neon_rtc_date_t *target_date,
                                       const neon_rtc_time_t *target_time,
                                       neon_rtc_date_t *actual_date,
                                       neon_rtc_time_t *actual_time);
static void run_basic_readback_test(void);
static void run_live_tracking_test(void);
static void run_rollover_test(void);
static void run_validation_test(void);
static void print_rtc_menu(void);

/*==================================================================================================
-                                        PRIVATE FUNCTIONS
==================================================================================================*/
static bool time_equal(const neon_rtc_time_t *a, const neon_rtc_time_t *b) {
    return a->hour == b->hour && a->minute == b->minute && a->second == b->second;
}

static bool date_equal(const neon_rtc_date_t *a, const neon_rtc_date_t *b) {
    return a->year == b->year && a->month == b->month && a->date == b->date;
}

/* Prevents midnight rollover race condition between RDATE and RTIME reads */
static neon_status_t read_consistent_datetime(neon_rtc_date_t *date, neon_rtc_time_t *time) {
    neon_rtc_date_t date_before = {0};
    neon_rtc_date_t date_after = {0};
    neon_status_t status;

    for (uint32_t retry = 0U; retry < 3U; ++retry) {
        status = neon_rtc_get_date(TEST_RTC_INSTANCE, &date_before);
        if (status != NEON_OK) {
            return status;
        }
        status = neon_rtc_get_time(TEST_RTC_INSTANCE, time);
        if (status != NEON_OK) {
            return status;
        }
        status = neon_rtc_get_date(TEST_RTC_INSTANCE, &date_after);
        if (status != NEON_OK) {
            return status;
        }
        if (date_equal(&date_before, &date_after)) {
            *date = date_after;
            return NEON_OK;
        }
    }

    *date = date_after;
    return NEON_OK;
}

static bool wait_for_time(const neon_rtc_time_t *expected, neon_rtc_time_t *actual) {
    for (uint32_t poll = 0U; poll < RTC_WAIT_POLL_LIMIT; ++poll) {
        const neon_status_t status = neon_rtc_get_time(TEST_RTC_INSTANCE, actual);
        if (status != NEON_OK) {
            printf("[ERROR] neon_rtc_get_time failed (Status=%d).\n", (int)status);
            return false;
        }
        if (time_equal(actual, expected)) {
            return true;
        }
    }
    return false;
}

static bool wait_for_date(const neon_rtc_date_t *expected, neon_rtc_date_t *actual) {
    for (uint32_t poll = 0U; poll < RTC_WAIT_POLL_LIMIT; ++poll) {
        const neon_status_t status = neon_rtc_get_date(TEST_RTC_INSTANCE, actual);
        if (status != NEON_OK) {
            printf("[ERROR] neon_rtc_get_date failed (Status=%d).\n", (int)status);
            return false;
        }
        if (date_equal(actual, expected)) {
            return true;
        }
    }
    return false;
}

static void print_hardware_registers(void) {
    const uint32_t cpsr = TEST_RTC_INSTANCE->CPSR;
    printf("[RTC REG] CPSR=0x%08lX (EN=%s DIV=%lu) RTIME=0x%08lX RDATE=0x%08lX\n",
           (unsigned long)cpsr, ((cpsr & (1UL << 31)) != 0U) ? "ON" : "OFF",
           (unsigned long)(cpsr & 0x03FFFFFFU), (unsigned long)TEST_RTC_INSTANCE->RTIME,
           (unsigned long)TEST_RTC_INSTANCE->RDATE);
}

static bool set_and_check_time(const neon_rtc_time_t *expected) {
    neon_rtc_time_t actual = {0};
    const neon_status_t status = neon_rtc_set_time(TEST_RTC_INSTANCE, expected);
    if (status != NEON_OK) {
        printf("[ERROR] neon_rtc_set_time failed (Status=%d).\n", (int)status);
        return false;
    }
    if (!wait_for_time(expected, &actual)) {
        printf("[FAIL] Timed out waiting for RTC time latch. Last=%02lu:%02lu:%02lu\n",
               (unsigned long)actual.hour, (unsigned long)actual.minute,
               (unsigned long)actual.second);
        return false;
    }
    return true;
}

static bool set_and_check_date(const neon_rtc_date_t *expected) {
    neon_rtc_date_t actual = {0};
    const neon_status_t status = neon_rtc_set_date(TEST_RTC_INSTANCE, expected);
    if (status != NEON_OK) {
        printf("[ERROR] neon_rtc_set_date failed (Status=%d).\n", (int)status);
        return false;
    }
    if (!wait_for_date(expected, &actual)) {
        printf("[FAIL] Timed out waiting for RTC date latch. Last=%04lu-%02lu-%02lu\n",
               (unsigned long)actual.year, (unsigned long)actual.month,
               (unsigned long)actual.date);
        return false;
    }
    return true;
}

static bool wait_for_next_second(const neon_rtc_time_t *previous, neon_rtc_time_t *actual) {
    neon_rtc_time_t expected = *previous;
    if (++expected.second == 60U) {
        expected.second = 0U;
        if (++expected.minute == 60U) {
            expected.minute = 0U;
            expected.hour = (uint8_t)((expected.hour + 1U) % 24U);
        }
    }

    for (uint32_t poll = 0U; poll < RTC_WAIT_POLL_LIMIT; ++poll) {
        const neon_status_t status = neon_rtc_get_time(TEST_RTC_INSTANCE, actual);
        if (status != NEON_OK) {
            printf("[ERROR] neon_rtc_get_time failed (Status=%d).\n", (int)status);
            return false;
        }
        if (!time_equal(actual, previous)) {
            return time_equal(actual, &expected);
        }
    }
    return false;
}

static bool wait_for_datetime_rollover(const neon_rtc_date_t *target_date,
                                       const neon_rtc_time_t *target_time,
                                       neon_rtc_date_t *actual_date,
                                       neon_rtc_time_t *actual_time) {
    for (uint32_t poll = 0U; poll < RTC_WAIT_POLL_LIMIT; ++poll) {
        const neon_status_t status = read_consistent_datetime(actual_date, actual_time);
        if (status != NEON_OK) {
            printf("[ERROR] read_consistent_datetime failed (Status=%d).\n", (int)status);
            return false;
        }
        if (date_equal(actual_date, target_date) && time_equal(actual_time, target_time)) {
            return true;
        }
    }
    return false;
}

static void run_basic_readback_test(void) {
    const neon_rtc_date_t expected_date = {.year = 2026U, .month = 11U, .date = 6U};
    const neon_rtc_time_t expected_time = {.hour = 14U, .minute = 30U, .second = 20U};
    neon_rtc_date_t actual_date = {0};
    neon_rtc_time_t actual_time = {0};

    printf("\n[TEST 1] Set/get date and time\n");
    const bool date_ok = set_and_check_date(&expected_date);
    const bool time_ok = set_and_check_time(&expected_time);
    if (!date_ok || !time_ok || read_consistent_datetime(&actual_date, &actual_time) != NEON_OK) {
        printf("[FAIL] RTC set/get operation failed.\n");
        return;
    }

    printf("  Expected: %04lu-%02lu-%02lu %02lu:%02lu:%02lu\n",
           (unsigned long)expected_date.year, (unsigned long)expected_date.month,
           (unsigned long)expected_date.date, (unsigned long)expected_time.hour,
           (unsigned long)expected_time.minute, (unsigned long)expected_time.second);
    printf("  Actual  : %04lu-%02lu-%02lu %02lu:%02lu:%02lu (weekday %lu)\n",
           (unsigned long)actual_date.year, (unsigned long)actual_date.month,
           (unsigned long)actual_date.date, (unsigned long)actual_time.hour,
           (unsigned long)actual_time.minute, (unsigned long)actual_time.second,
           (unsigned long)actual_date.day);

    if (date_equal(&actual_date, &expected_date) && time_equal(&actual_time, &expected_time)) {
        printf("[PASS] Date and time readback matched.\n");
    } else {
        printf("[FAIL] Date/time readback mismatch.\n");
    }
    print_hardware_registers();
}

static void run_live_tracking_test(void) {
    neon_rtc_time_t previous = {0};
    neon_rtc_time_t actual = {0};
    neon_status_t status = neon_rtc_get_time(TEST_RTC_INSTANCE, &previous);
    if (status != NEON_OK) {
        printf("[ERROR] Initial RTC read failed (Status=%d).\n", (int)status);
        return;
    }

    printf("\n[TEST 2] Verify five consecutive RTC second increments\n");
    for (uint32_t i = 0U; i < 5U; ++i) {
        if (!wait_for_next_second(&previous, &actual)) {
            printf("[FAIL] RTC did not advance by exactly one second; last=%02lu:%02lu:%02lu\n",
                   (unsigned long)actual.hour, (unsigned long)actual.minute,
                   (unsigned long)actual.second);
            print_hardware_registers();
            return;
        }
        printf("  [%lu/5] %02lu:%02lu:%02lu\n", (unsigned long)(i + 1U),
               (unsigned long)actual.hour, (unsigned long)actual.minute,
               (unsigned long)actual.second);
        previous = actual;
    }
    printf("[PASS] RTC advanced through five consecutive second values.\n");
}

static void run_rollover_test(void) {
    const neon_rtc_time_t minute_start = {.hour = 10U, .minute = 59U, .second = 58U};
    const neon_rtc_time_t minute_target = {.hour = 11U, .minute = 0U, .second = 0U};

    const neon_rtc_date_t leap_start_date = {.year = 2028U, .month = 2U, .date = 28U};
    const neon_rtc_date_t leap_target_date = {.year = 2028U, .month = 2U, .date = 29U};

    const neon_rtc_date_t year_end_date = {.year = 2026U, .month = 12U, .date = 31U};
    const neon_rtc_date_t year_target_date = {.year = 2027U, .month = 1U, .date = 1U};

    const neon_rtc_time_t midnight_start = {.hour = 23U, .minute = 59U, .second = 58U};
    const neon_rtc_time_t midnight_target = {.hour = 0U, .minute = 0U, .second = 0U};

    neon_rtc_time_t actual_time = {0};
    neon_rtc_date_t actual_date = {0};

    printf("\n[TEST 3] Minute, leap-day, and year rollover\n");

    /* 3a. Minute & hour rollover (10:59:58 -> 11:00:00) */
    if (!set_and_check_time(&minute_start)) {
        printf("[FAIL] Could not set minute rollover start time.\n");
        return;
    }
    if (!wait_for_time(&minute_target, &actual_time)) {
        printf("[FAIL] Minute rollover timeout/mismatch; last=%02lu:%02lu:%02lu\n",
               (unsigned long)actual_time.hour, (unsigned long)actual_time.minute,
               (unsigned long)actual_time.second);
        return;
    }
    printf("  [PASS] Minute rollover reached %02lu:%02lu:%02lu.\n",
           (unsigned long)actual_time.hour, (unsigned long)actual_time.minute,
           (unsigned long)actual_time.second);

    /* 3b. Leap day rollover (2028-02-28 23:59:58 -> 2028-02-29 00:00:00) */
    if (!set_and_check_date(&leap_start_date) || !set_and_check_time(&midnight_start)) {
        printf("[FAIL] Could not set leap-day rollover start date/time.\n");
        return;
    }
    if (!wait_for_datetime_rollover(&leap_target_date, &midnight_target, &actual_date, &actual_time)) {
        printf("[FAIL] Leap-day rollover timeout/mismatch; last=%04lu-%02lu-%02lu %02lu:%02lu:%02lu\n",
               (unsigned long)actual_date.year, (unsigned long)actual_date.month,
               (unsigned long)actual_date.date, (unsigned long)actual_time.hour,
               (unsigned long)actual_time.minute, (unsigned long)actual_time.second);
        return;
    }
    printf("  [PASS] Leap-day rollover reached %04lu-%02lu-%02lu %02lu:%02lu:%02lu.\n",
           (unsigned long)actual_date.year, (unsigned long)actual_date.month,
           (unsigned long)actual_date.date, (unsigned long)actual_time.hour,
           (unsigned long)actual_time.minute, (unsigned long)actual_time.second);

    /* 3c. Year rollover (2026-12-31 23:59:58 -> 2027-01-01 00:00:00) */
    if (!set_and_check_date(&year_end_date) || !set_and_check_time(&midnight_start)) {
        printf("[FAIL] Could not set year rollover start date/time.\n");
        return;
    }
    if (!wait_for_datetime_rollover(&year_target_date, &midnight_target, &actual_date, &actual_time)) {
        printf("[FAIL] Year rollover timeout/mismatch; last=%04lu-%02lu-%02lu %02lu:%02lu:%02lu\n",
               (unsigned long)actual_date.year, (unsigned long)actual_date.month,
               (unsigned long)actual_date.date, (unsigned long)actual_time.hour,
               (unsigned long)actual_time.minute, (unsigned long)actual_time.second);
        return;
    }
    printf("  [PASS] Year rollover reached %04lu-%02lu-%02lu %02lu:%02lu:%02lu.\n",
           (unsigned long)actual_date.year, (unsigned long)actual_date.month,
           (unsigned long)actual_date.date, (unsigned long)actual_time.hour,
           (unsigned long)actual_time.minute, (unsigned long)actual_time.second);

    print_hardware_registers();
}

static void run_validation_test(void) {
    const neon_rtc_date_t valid_leap_date = {.year = 2028U, .month = 2U, .date = 29U};
    const neon_rtc_date_t invalid_leap_date = {.year = 2026U, .month = 2U, .date = 29U};
    const neon_rtc_time_t invalid_time = {.hour = 25U, .minute = 61U, .second = 70U};
    neon_rtc_date_t actual_date = {0};
    const neon_status_t invalid_date_status = neon_rtc_set_date(TEST_RTC_INSTANCE, &invalid_leap_date);
    const neon_status_t invalid_time_status = neon_rtc_set_time(TEST_RTC_INSTANCE, &invalid_time);

    printf("\n[TEST 4] Leap year and invalid input validation\n");
    const bool valid_date_ok = set_and_check_date(&valid_leap_date);
    const bool invalid_date_ok = (invalid_date_status == NEON_INVALID_PARAM);
    const bool invalid_time_ok = (invalid_time_status == NEON_INVALID_PARAM);
    if (neon_rtc_get_date(TEST_RTC_INSTANCE, &actual_date) != NEON_OK) {
        printf("[ERROR] Could not read date after validation checks.\n");
        return;
    }

    printf("  Valid leap date: %s\n", valid_date_ok ? "accepted and read back" : "FAIL");
    printf("  Invalid date rejected: %s (status=%d)\n", invalid_date_ok ? "yes" : "NO",
           (int)invalid_date_status);
    printf("  Invalid time rejected: %s (status=%d)\n", invalid_time_ok ? "yes" : "NO",
           (int)invalid_time_status);
    if (valid_date_ok && invalid_date_ok && invalid_time_ok && date_equal(&actual_date, &valid_leap_date)) {
        printf("[PASS] Leap date accepted; invalid date/time rejected.\n");
    } else {
        printf("[FAIL] RTC input validation mismatch.\n");
    }
}

static void print_rtc_menu(void) {
    printf("\n=================== 12. RTC CALENDAR TEST MENU ===================\n");
    printf("  [1] Set/get date and time\n");
    printf("  [2] Verify five consecutive RTC second increments\n");
    printf("  [3] Check minute, leap-day, and year rollover\n");
    printf("  [4] Check leap year and invalid inputs\n");
    printf("  [q] Deinitialize RTC and exit\n");
    printf("==================================================================\n");
}

/*==================================================================================================
-                                        GLOBAL FUNCTIONS
==================================================================================================*/

/** @copydoc rtc_calendar_run */
void rtc_calendar_run(void) {
    neon_status_t status;
    neon_uart_handle_t *uart = neon_console_get_uart();
    neon_rtc_config_t rtc_config = {
        .clock_frequency_hz = 0U,
        .prescaler = (uint32_t)CONFIG_RTC_CALENDAR_PRESCALER,
    };
    uint8_t reset_asserted = 1U;
    bool running = true;

    if (uart == NULL) {
        printf("[ERROR] Terminal UART handle is unavailable.\n");
        return;
    }

    status = neon_crm_peripheral_clock_enable(NEON_CRM_PERIPHERAL_RTC);
    if (status != NEON_OK) {
        printf("[ERROR] RTC CRM clock enable failed (Status=%d).\n", (int)status);
        return;
    }
    printf("[RTC] CRM clock OK.\n");

    status = neon_crm_peripheral_reset(NEON_CRM_PERIPHERAL_RTC);
    if (status != NEON_OK) {
        printf("[ERROR] RTC CRM reset failed (Status=%d).\n", (int)status);
        return;
    }
    (void)neon_crm_is_peripheral_reset_asserted(NEON_CRM_PERIPHERAL_RTC, &reset_asserted);
    printf("[RTC] CRM reset OK (SW_RST_N_1=0x%08lX, asserted=%u).\n",
           (unsigned long)CRM->SW_RST_N_1, (unsigned int)reset_asserted);

    printf("[RTC] Reading CPSR reset value at 0x%08lX...\n",
           (unsigned long)(uintptr_t)&TEST_RTC_INSTANCE->CPSR);
    const uint32_t rtc_cpsr_reset_value = TEST_RTC_INSTANCE->CPSR;
    printf("[RTC] CPSR after reset=0x%08lX (expected 0x%08lX).\n",
           (unsigned long)rtc_cpsr_reset_value, (unsigned long)RTC_CPSR_EXPECTED_RESET);
    if (rtc_cpsr_reset_value != RTC_CPSR_EXPECTED_RESET) {
        printf("[WARN] RTC CPSR reset-value mismatch; continuing with explicit init.\n");
    }

    status = neon_rtc_init(TEST_RTC_INSTANCE, &rtc_config);
    if (status != NEON_OK) {
        printf("[ERROR] neon_rtc_init failed (Status=%d).\n", (int)status);
        return;
    }
    printf("[RTC] Init OK (DIV=%lu).\n", (unsigned long)rtc_config.prescaler);
    print_hardware_registers();

    while (running) {
        uint32_t rx_word = 0U;
        print_rtc_menu();
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
                run_basic_readback_test();
                break;
            case '2':
                run_live_tracking_test();
                break;
            case '3':
                run_rollover_test();
                break;
            case '4':
                run_validation_test();
                break;
            case 'q':
            case 'Q':
            case 0x03:
                running = false;
                break;
            default:
                printf("[WARN] Invalid option. Select [1, 2, 3, 4, q].\n");
                break;
        }
    }

    status = neon_rtc_deinit(TEST_RTC_INSTANCE);
    if (status == NEON_OK) {
        printf("[RTC] Deinitialized; RTC counter/configuration stopped.\n");
    } else {
        printf("[ERROR] neon_rtc_deinit failed (Status=%d).\n", (int)status);
    }
}