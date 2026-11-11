/** @file i2c_eeprom.c
 * @brief I2C0 master checks with an AT24C256 EEPROM.
 */
#include "i2c_eeprom.h"
#include "i2c/neon_i2c.h"
#include "sdkconfig.h"
#include <stdio.h>

#ifndef CONFIG_NEON_MDF_DRIVER_I2C
#error "I2C_EEPROM requires the I2C driver"
#endif

#define TIMEOUT CONFIG_I2C_EEPROM_TIMEOUT_POLLS
#define DEVICE_ADDRESS CONFIG_I2C_EEPROM_ADDRESS
#define EEPROM_SIZE 32768U
#define READ_SIZE 64U
#define WRITE_SIZE 32U
#define TICK_MASK 0x00FFFFFFU

static neon_i2c_handle_t i2c;

static neon_status_t write_delay(void) {
    uint32_t start = SysTick->VAL;
    uint32_t cycles = (SystemCoreClock / 1000U) * 6U;
    uint32_t polls = TIMEOUT;
    while (((start - SysTick->VAL) & TICK_MASK) < cycles) {
        if (polls-- == 0U) { return NEON_TIMEOUT; }
    }
    return NEON_OK;
}

/* Logical bytes occupy uint32_t elements. Pack/unpack only at the byte-API boundary. */
static neon_status_t eeprom_read(uint32_t address, uint32_t *data, uint32_t count) {
    uint32_t pointer = (address >> 8) | ((address & 0xFFU) << 8);
    uint32_t packed[READ_SIZE / 4U] = {0U};
    neon_status_t status = neon_i2c_master_transmit(&i2c, DEVICE_ADDRESS,
        (const uint8_t *)&pointer, 2U, TIMEOUT);
    if (status != NEON_OK) {
        printf("[READ] Pointer write failed at %04lX.\n", (unsigned long)address);
    }
    if (status == NEON_OK) {
        status = neon_i2c_master_receive(&i2c, DEVICE_ADDRESS, (uint8_t *)packed, count, TIMEOUT);
        if (status != NEON_OK) {
            printf("[READ] RX failed at %04lX, requested=%lu bytes.\n",
                (unsigned long)address, (unsigned long)count);
        }
    }
    if (status == NEON_OK) {
        for (uint32_t i = 0U; i < count; ++i) {
            data[i] = (packed[i / 4U] >> (8U * (i % 4U))) & 0xFFU;
        }
    }
    return status;
}

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

static void dump(uint32_t address, const uint32_t *data, uint32_t count) {
    for (uint32_t i = 0U; i < count; ++i) {
        if ((i % 16U) == 0U) { printf("%04lX: ", (unsigned long)(address + i)); }
        printf("%02lX%c", (unsigned long)data[i],
            (i % 16U) == 15U || i + 1U == count ? '\n' : ' ');
    }
}

static uint32_t cancelled(neon_uart_handle_t *uart) {
    uint32_t key = 0U;
    return neon_uart_try_receive(uart, &key) == NEON_OK && (key == 'q' || key == 3U);
}

static uint32_t next_byte(uint32_t *state) {
    uint32_t x = *state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    *state = x;
    return x & 0xFFU;
}

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

static neon_status_t address_probe(void) {
    uint32_t pattern_a[4] = {0xA5U, 0x3CU, 0x69U, 0x96U};
    uint32_t pattern_b[4] = {0x5AU, 0xC3U, 0x96U, 0x69U};
    uint32_t readback[2][4] = {{0U}};
    printf("[ADDR] Test two-byte memory addressing on device %02X.\n", DEVICE_ADDRESS);
    printf("[ADDR] Overwrites 0010..0013 and 0020..0023 on a two-byte-address EEPROM;\n");
    printf("[ADDR] or 0000..0004 on a one-byte-address EEPROM. No restore.\n");
    printf("[ADDR] WRITE A:\n");
    dump(0x10U, pattern_a, 4U);
    neon_status_t status = eeprom_write(0x10U, pattern_a, 4U);
    if (status != NEON_OK) { return status; }
    printf("[ADDR] WRITE B:\n");
    dump(0x20U, pattern_b, 4U);
    status = eeprom_write(0x20U, pattern_b, 4U);
    if (status != NEON_OK) { return status; }

    for (uint32_t test = 0U; test < 2U; ++test) {
        uint32_t address = test == 0U ? 0x10U : 0x20U;
        uint32_t pointer = address << 8U; /* Send 00, then 10 or 20. */
        uint32_t packed = 0U;
        printf("[ADDR] READ %04lX:\n", (unsigned long)address);
        status = neon_i2c_master_transmit(&i2c, DEVICE_ADDRESS,
            (const uint8_t *)&pointer, 2U, TIMEOUT);
        /* On a one-byte-address EEPROM the second pointer byte writes data.
         * Wait even after an error, in case STOP committed that byte. */
        neon_status_t delay = write_delay();
        if (status != NEON_OK) { return status; }
        if (delay != NEON_OK) { return delay; }
        status = neon_i2c_master_receive(&i2c, DEVICE_ADDRESS, (uint8_t *)&packed, 4U, TIMEOUT);
        if (status != NEON_OK) { return status; }
        for (uint32_t i = 0U; i < 4U; ++i) {
            readback[test][i] = (packed >> (8U * i)) & 0xFFU;
        }
        dump(address, readback[test], 4U);
    }

    uint32_t matches_a = 1U, matches_b = 1U, first_is_b = 1U;
    for (uint32_t i = 0U; i < 4U; ++i) {
        if (readback[0][i] != pattern_a[i]) { matches_a = 0U; }
        if (readback[1][i] != pattern_b[i]) { matches_b = 0U; }
        if (readback[0][i] != pattern_b[i]) { first_is_b = 0U; }
    }
    if (matches_a && matches_b) {
        printf("[ADDR] RESULT: 2-BYTE addressing works at both tested locations.\n");
    } else if (first_is_b && matches_b) {
        printf("[ADDR] RESULT: both locations returned B; likely 1-BYTE addressing.\n");
        printf("[ADDR] Memory/bus aliasing can give the same result; confirm the IC marking.\n");
    } else {
        printf("[ADDR] RESULT: INCONCLUSIVE; data mismatch, check IC, WP and I2C signals.\n");
    }
    printf("[ADDR] This test does not identify capacity/page size or change the configuration.\n");
    return NEON_OK;
}

static neon_status_t read_lengths(void) {
    const uint32_t sizes[] = {1U, 2U, 4U, 8U, 16U, 31U, 32U, 33U, 64U};
    uint32_t data[READ_SIZE];
    printf("[LENGTH] Two-byte pointer at 0000, then read each size; no payload writes.\n");
    for (uint32_t i = 0U; i < sizeof(sizes) / sizeof(sizes[0]); ++i) {
        uint32_t start = SysTick->VAL;
        neon_status_t status = eeprom_read(0U, data, sizes[i]);
        uint32_t ticks = (start - SysTick->VAL) & TICK_MASK;
        printf("[LENGTH] bytes=%lu status=%lu error=%08lX elapsed_ticks=%lu\n",
            (unsigned long)sizes[i], (unsigned long)status,
            (unsigned long)neon_i2c_get_error(&i2c), (unsigned long)ticks);
        if (status != NEON_OK) { return status; }
    }
    return NEON_OK;
}

static neon_status_t full_test(neon_uart_handle_t *uart) {
    uint32_t data[READ_SIZE], expected[WRITE_SIZE];
    uint32_t seed = SysTick->VAL;
    if (seed == 0U) { seed = 0x46553201U; }
    uint32_t state = seed, written = 0U, checked = 0U, mismatches = 0U, complete = 0U;
    neon_status_t status = NEON_OK;
    printf("[TEST] Overwrite ALL 32 KiB, no restore; q/Ctrl+C aborts after a block.\n");
    printf("[READ ORIGINAL] 0000..7FFF, 64-byte blocks.\n");
    for (uint32_t address = 0U; address < EEPROM_SIZE; address += READ_SIZE) {
        if (cancelled(uart)) { goto aborted; }
        status = eeprom_read(address, data, READ_SIZE);
        if (status != NEON_OK) { goto finished; }
        dump(address, data, READ_SIZE);
    }
    printf("[WRITE] xorshift32 seed=%08lX, 32-byte blocks, 6 ms write delay.\n", (unsigned long)seed);
    for (uint32_t address = 0U; address < EEPROM_SIZE; address += WRITE_SIZE) {
        if (cancelled(uart)) { goto aborted; }
        for (uint32_t i = 0U; i < WRITE_SIZE; ++i) { expected[i] = next_byte(&state); }
        status = eeprom_write(address, expected, WRITE_SIZE);
        if (status != NEON_OK) { goto finished; }
        written += WRITE_SIZE;
        if ((written % 1024U) == 0U) { printf("[WRITE] %lu/32768\n", (unsigned long)written); }
    }
    state = seed;
    printf("[VERIFY] Read 64-byte blocks and compare every byte.\n");
    for (uint32_t address = 0U; address < EEPROM_SIZE; address += READ_SIZE) {
        if (cancelled(uart)) { goto aborted; }
        status = eeprom_read(address, data, READ_SIZE);
        if (status != NEON_OK) { goto finished; }
        dump(address, data, READ_SIZE);
        for (uint32_t i = 0U; i < READ_SIZE; ++i) {
            uint32_t wanted = next_byte(&state);
            if (data[i] != wanted) {
                ++mismatches;
                printf("[FAIL] mem=%04lX expected=%02lX received=%02lX\n",
                    (unsigned long)(address + i), (unsigned long)wanted, (unsigned long)data[i]);
            }
        }
        checked += READ_SIZE;
    }
    complete = 1U;
    printf("[VERIFY] %s\n", mismatches == 0U ? "[PASS] 32768 bytes match" : "[FAIL] data mismatch");
    goto finished;
aborted:
    printf("[ABORT] EEPROM test incomplete; written bytes retain the test pattern.\n");
finished:
    printf("[RESULT] written=%lu checked=%lu mismatches=%lu transport=%lu %s\n",
        (unsigned long)written, (unsigned long)checked, (unsigned long)mismatches, (unsigned long)status,
        complete != 0U && mismatches == 0U && status == NEON_OK ? "[PASS]" : "[FAIL/incomplete]");
    return status;
}

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
    printf("[PINS] SDA=PMOD_IO6 SCL=PMOD_IO2; pull-ups, common GND, WP LOW for writes.\n");
    printf(" s: scan 00..7F | a: write/read 2A 00 at 0000\n");
    printf(" d: memory address-width test (overwrites small regions, no restore)\n");
    printf(" l: read-length diagnostics, 1/2/4/8/16/31/32/33/64 bytes\n");
    printf(" r: continuous 64-byte read | t: overwrite/verify ALL 32 KiB\n");
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
            uint32_t data[READ_SIZE], address = 0U, blocks = 0U;
            printf("[READ LOOP] 0000..7FFF, blocks of 64; q/Ctrl+C stops.\n");
            while (!cancelled(uart)) {
                status = eeprom_read(address, data, READ_SIZE);
                if (status != NEON_OK) { break; }
                dump(address, data, READ_SIZE);
                ++blocks;
                address = (address + READ_SIZE) % EEPROM_SIZE;
            }
            printf("[READ LOOP] Completed blocks=%lu.\n", (unsigned long)blocks);
        } else if (key == 't') { status = full_test(uart); }
        else if (key == 'd') { status = address_probe(); }
        else if (key == 'l') { status = read_lengths(); }
        else { continue; }
        if (status != NEON_OK) {
            printf("[FAIL] I2C status=%lu error=0x%08lX; check address, wiring, pull-ups and WP.\n",
                (unsigned long)status, (unsigned long)neon_i2c_get_error(&i2c));
        }
        printf("[MENU] s/a/d/l/r/t or q to exit.\n");
    }
    neon_i2c_deinit(&i2c);
    SysTick->CTRL = 0U;
    SysTick->LOAD = saved_load;
    SysTick->VAL = 0U;
    SysTick->CTRL = saved_ctrl & 7U;
    printf("[INFO] I2C_EEPROM stopped; I2C0 released.\n");
}
