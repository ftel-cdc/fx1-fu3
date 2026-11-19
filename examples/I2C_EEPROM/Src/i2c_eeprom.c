/**
 * @file i2c_eeprom.c
 * @brief I2C0 master checks with an AT24C256 EEPROM.
 * @details
 * I2C0 EEPROM scan, bounded reads and a one-KiB write/read verification test.
 */

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/

#include "i2c_eeprom.h"
#include "i2c/neon_i2c.h"
#include "sdkconfig.h"
#include <stdio.h>

/*==================================================================================================
*                                        DEFINES AND MACROS
==================================================================================================*/

#ifndef CONFIG_NEON_MDF_DRIVER_I2C
#error "I2C_EEPROM requires the I2C driver"
#endif

#define TIMEOUT CONFIG_I2C_EEPROM_TIMEOUT_POLLS
#define DEVICE_ADDRESS CONFIG_I2C_EEPROM_ADDRESS
#define EEPROM_SIZE 32768U
#define READ_TEST_SIZE 256U
#define TEST_SIZE 1024U
#define READ_SIZE 64U
#define WRITE_SIZE 32U
#define TICK_MASK 0x00FFFFFFU

/*==================================================================================================
*                                         LOCAL VARIABLES
==================================================================================================*/

static neon_i2c_handle_t i2c;

/*==================================================================================================
*                                        PRIVATE FUNCTIONS
==================================================================================================*/

/**
 * @brief Wait six milliseconds for an EEPROM write cycle.
 * @return NEON_OK after the delay, or NEON_TIMEOUT if the polling budget expires.
 */
static neon_status_t write_delay(void) {
    uint32_t start = SysTick->VAL;
    uint32_t cycles = (SystemCoreClock / 1000U) * 6U;
    uint32_t polls = TIMEOUT;
    while (((start - SysTick->VAL) & TICK_MASK) < cycles) {
        if (polls-- == 0U) { return NEON_TIMEOUT; }
    }
    return NEON_OK;
}

/**
 * @brief Read bytes using a two-byte EEPROM memory address.
 * @param[in] address First EEPROM byte offset.
 * @param[out] data Aligned word array; each element receives one logical byte.
 * @param[in] count Number of bytes, at most READ_SIZE.
 * @return I2C transfer status.
 * @details
 * Logical bytes occupy uint32_t elements. Pack/unpack only at the byte-API boundary.
 */
static neon_status_t eeprom_read(uint32_t address, uint32_t *data, uint32_t count) {
    uint32_t pointer = (address >> 8) | ((address & 0xFFU) << 8);
    uint32_t packed[READ_SIZE / 4U] = {0U};
    neon_status_t status = neon_i2c_master_transmit(&i2c, DEVICE_ADDRESS,
        (const uint8_t *)&pointer, 2U, TIMEOUT);
    if (status == NEON_OK) {
        status = neon_i2c_master_receive(&i2c, DEVICE_ADDRESS, (uint8_t *)packed, count, TIMEOUT);
    }
    if (status == NEON_OK) {
        for (uint32_t i = 0U; i < count; ++i) {
            data[i] = (packed[i / 4U] >> (8U * (i % 4U))) & 0xFFU;
        }
    }
    return status;
}

/**
 * @brief Write an EEPROM block within one page and wait for its write cycle.
 * @param[in] address First EEPROM byte offset.
 * @param[in] data Word array containing one logical byte per element.
 * @param[in] count Payload bytes, at most WRITE_SIZE and within one EEPROM page.
 * @return Transfer error, or the write-delay status after a successful transfer.
 */
static neon_status_t eeprom_write(uint32_t address, const uint32_t *data, uint32_t count) {
    /* Callers use at most 32 bytes, aligned within the EEPROM's 64-byte pages. */
    uint32_t packed[(WRITE_SIZE + 2U + 3U) / 4U] = {0U};
    packed[0] = (address >> 8) | ((address & 0xFFU) << 8);
    for (uint32_t i = 0U; i < count; ++i) {
        uint32_t position = i + 2U;
        packed[position / 4U] |= (data[i] & 0xFFU) << (8U * (position % 4U));
    }
    neon_status_t status = neon_i2c_master_transmit(&i2c, DEVICE_ADDRESS,
        (const uint8_t *)packed, count + 2U, TIMEOUT);
    /* A partial write can also start an internal write cycle after STOP. */
    neon_status_t delay = write_delay();
    return status == NEON_OK ? delay : status;
}

/**
 * @brief Print logical bytes as sixteen-column hexadecimal rows.
 * @param[in] address EEPROM offset printed at the start of each row.
 * @param[in] data Word array containing one logical byte per element.
 * @param[in] count Number of logical bytes to print.
 */
static void dump(uint32_t address, const uint32_t *data, uint32_t count) {
    for (uint32_t i = 0U; i < count; ++i) {
        if ((i % 16U) == 0U) { printf("%04lX: ", (unsigned long)(address + i)); }
        printf("%02lX%c", (unsigned long)data[i],
            (i % 16U) == 15U || i + 1U == count ? '\n' : ' ');
    }
}

/**
 * @brief Consume one pending console character and detect cancellation.
 * @param[in,out] uart Initialized console UART handle.
 * @return Nonzero for q or Ctrl+C; other received characters are consumed and ignored.
 */
static uint32_t cancelled(neon_uart_handle_t *uart) {
    uint32_t key = 0U;
    return neon_uart_try_receive(uart, &key) == NEON_OK && (key == 'q' || key == 3U);
}

/**
 * @brief Advance a xorshift32 sequence and return its low byte.
 * @param[in,out] state Nonzero generator state.
 * @return Next logical byte in the reproducible sequence.
 */
static uint32_t next_byte(uint32_t *state) {
    uint32_t x = *state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    *state = x;
    return x & 0xFFU;
}

/**
 * @brief Probe all seven-bit I2C addresses with one-byte reads.
 * @return NEON_OK when scanning finishes, otherwise the transport error.
 */
static neon_status_t scan(void) {
    uint32_t byte = 0U, ack = 0U;
    printf("[SCAN] 7-bit 00..7F; one-byte reads, hex=ACK, --=NACK.\n");
    printf("     0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F\n");
    for (uint32_t address = 0U; address < 128U; ++address) {
        if ((address % 16U) == 0U) { printf("%02lX: ", (unsigned long)address); }
        neon_status_t status = neon_i2c_master_receive(&i2c, address, (uint8_t *)&byte, 1U, TIMEOUT);
        if (status == NEON_OK) {
            ++ack;
            printf("%02lX ", (unsigned long)address);
        } else if (status == NEON_ERROR && neon_i2c_get_error(&i2c) == NEON_I2C_ERROR_ADDRESS_NACK) {
            printf("-- ");
        } else {
            printf("\n[SCAN] stopped at %02lX, status=%lu error=0x%08lX.\n",
                (unsigned long)address, (unsigned long)status, (unsigned long)neon_i2c_get_error(&i2c));
            return status;
        }
        if ((address % 16U) == 15U) { printf("\n"); }
    }
    printf("[SCAN] Probed=128 ACK=%lu NACK=%lu %s\n", (unsigned long)ack,
        (unsigned long)(128U - ack), ack != 0U ? "[PASS]" : "[FAIL] no device found");
    return NEON_OK;
}

/**
 * @brief Write and verify the first TEST_SIZE bytes with a reproducible pattern.
 * @param[in,out] uart Console UART used to check q or Ctrl+C between blocks.
 * @return Transport status; verification and cancellation results are printed.
 * @details
 * Overwrites the test region without restoring it. Only a sample and the first eight
 * mismatches are printed; every byte in the test region is compared.
 */
static neon_status_t write_read_test(neon_uart_handle_t *uart) {
    uint32_t data[READ_SIZE], expected[WRITE_SIZE];
    uint32_t seed = SysTick->VAL;
    if (seed == 0U) { seed = 0x46553201U; }
    uint32_t state = seed, written = 0U, checked = 0U, mismatches = 0U, complete = 0U;
    neon_status_t status = NEON_OK;
    printf("[TEST] Write/verify %lu bytes at 0000..%04lX, no restore.\n",
        (unsigned long)TEST_SIZE, (unsigned long)(TEST_SIZE - 1U));
    printf("[TEST] q/Ctrl+C aborts after a block.\n");
    printf("[WRITE] xorshift32 seed=%08lX, 32-byte blocks, 6 ms write delay.\n", (unsigned long)seed);
    for (uint32_t address = 0U; address < TEST_SIZE; address += WRITE_SIZE) {
        if (cancelled(uart)) { goto aborted; }
        for (uint32_t i = 0U; i < WRITE_SIZE; ++i) { expected[i] = next_byte(&state); }
        status = eeprom_write(address, expected, WRITE_SIZE);
        if (status != NEON_OK) { goto finished; }
        written += WRITE_SIZE;
        if ((written % 256U) == 0U) {
            printf("[WRITE] %lu/%lu\n", (unsigned long)written, (unsigned long)TEST_SIZE);
        }
    }
    state = seed;
    printf("[VERIFY] Read 64-byte blocks and compare every byte.\n");
    for (uint32_t address = 0U; address < TEST_SIZE; address += READ_SIZE) {
        if (cancelled(uart)) { goto aborted; }
        status = eeprom_read(address, data, READ_SIZE);
        if (status != NEON_OK) { goto finished; }
        if (address == 0U) {
            printf("[SAMPLE] First 64 bytes:\n");
            dump(address, data, READ_SIZE);
        }
        for (uint32_t i = 0U; i < READ_SIZE; ++i) {
            uint32_t wanted = next_byte(&state);
            if (data[i] != wanted) {
                ++mismatches;
                if (mismatches <= 8U) {
                    printf("[FAIL] mem=%04lX expected=%02lX received=%02lX\n",
                        (unsigned long)(address + i), (unsigned long)wanted, (unsigned long)data[i]);
                } else if (mismatches == 9U) {
                    printf("[VERIFY] Further mismatch details omitted; all bytes are still checked.\n");
                }
            }
        }
        checked += READ_SIZE;
    }
    complete = 1U;
    if (mismatches == 0U) {
        printf("[VERIFY] [PASS] %lu bytes match\n", (unsigned long)TEST_SIZE);
    } else {
        printf("[VERIFY] [FAIL] data mismatch\n");
    }
    goto finished;
aborted:
    printf("[ABORT] EEPROM test incomplete; written bytes retain the test pattern.\n");
finished:
    printf("[RESULT] written=%lu checked=%lu mismatches=%lu transport=%lu %s\n",
        (unsigned long)written, (unsigned long)checked, (unsigned long)mismatches, (unsigned long)status,
        complete != 0U && mismatches == 0U && status == NEON_OK ? "[PASS]" : "[FAIL/incomplete]");
    return status;
}

/*==================================================================================================
*                                         GLOBAL FUNCTIONS
==================================================================================================*/

/** @copydoc i2c_eeprom_run */
void i2c_eeprom_run(neon_uart_handle_t *uart) {
    if ((SysTick->CTRL & SysTick_CTRL_ENABLE_Msk) != 0U) {
        printf("[FAIL] SysTick is already in use.\n");
        return;
    }
    uint32_t saved_load = SysTick->LOAD, saved_ctrl = SysTick->CTRL;
    neon_i2c_config_t config;
    neon_i2c_get_default_config(&config);
    neon_status_t status = neon_i2c_init(&i2c, NEON_I2C0);
    if (status != NEON_OK) { printf("[FAIL] I2C0 handle status=%lu.\n", (unsigned long)status); return; }
    status = neon_i2c_set_config(&i2c, &config);
    if (status != NEON_OK) {
        printf("[FAIL] I2C0 config status=%lu.\n", (unsigned long)status);
        neon_i2c_deinit(&i2c);
        return;
    }
    SysTick->LOAD = TICK_MASK;
    SysTick->VAL = 0U;
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_ENABLE_Msk;
    printf("[I2C0] Master, target SCL=100 kHz, EEPROM address=%02X.\n", DEVICE_ADDRESS);
    printf("[EEPROM] Capacity=%lu bytes; quick tests use only the beginning of memory.\n",
        (unsigned long)EEPROM_SIZE);
    printf("[PINS] SDA=PMOD_IO6 SCL=PMOD_IO2; pull-ups, common GND, WP LOW for writes.\n");
    printf(" s: scan 00..7F | a: write/read 2A 00 at 0000\n");
    printf(" r: read 256 bytes once | t: write/verify first 1 KiB, no restore\n");
    printf(" q/Ctrl+C: stop a long test or return to fpga:>>\n");
    printf("[READY] I2C_EEPROM menu, no Enter needed.\n");
    for (;;) {
        uint32_t key = 0U;
        if (neon_uart_try_receive(uart, &key) != NEON_OK) { continue; }
        if (key == 'q' || key == 3U) { break; }
        status = NEON_OK;
        if (key == 's') { status = scan(); }
        else if (key == 'a') {
            uint32_t tx[2] = {0x2AU, 0x00U}, rx[2] = {0U};
            printf("[WRITE] mem=0000 data=2A 00; wait 6 ms, then read.\n");
            status = eeprom_write(0U, tx, 2U);
            if (status == NEON_OK) { status = eeprom_read(0U, rx, 2U); }
            if (status == NEON_OK) {
                printf("[READ] %02lX %02lX %s\n", (unsigned long)rx[0], (unsigned long)rx[1],
                    rx[0] == tx[0] && rx[1] == tx[1] ? "[PASS]" : "[FAIL] expected 2A 00");
            }
        } else if (key == 'r') {
            uint32_t data[READ_SIZE], read = 0U;
            printf("[READ] 0000..%04lX once, 64-byte blocks; q/Ctrl+C stops.\n",
                (unsigned long)(READ_TEST_SIZE - 1U));
            for (uint32_t address = 0U; address < READ_TEST_SIZE; address += READ_SIZE) {
                if (cancelled(uart)) { break; }
                status = eeprom_read(address, data, READ_SIZE);
                if (status != NEON_OK) { break; }
                dump(address, data, READ_SIZE);
                read += READ_SIZE;
            }
            printf("[READ] Completed bytes=%lu/%lu.\n", (unsigned long)read, (unsigned long)READ_TEST_SIZE);
        } else if (key == 't') { status = write_read_test(uart); }
        else { continue; }
        if (status != NEON_OK) {
            printf("[FAIL] I2C status=%lu error=0x%08lX; check address, wiring, pull-ups and WP.\n",
                (unsigned long)status, (unsigned long)neon_i2c_get_error(&i2c));
        }
        printf("[MENU] s/a/r/t or q to exit.\n");
    }
    neon_i2c_deinit(&i2c);
    SysTick->CTRL = 0U;
    SysTick->LOAD = saved_load;
    SysTick->VAL = 0U;
    SysTick->CTRL = saved_ctrl & 7U;
    printf("[INFO] I2C_EEPROM stopped; I2C0 released.\n");
}
