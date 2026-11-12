/**
 * @file qspi_flash_read_write.c
 * @brief Indirect Single/Dual/Quad NOR erase, program and byte verification.
 * @details
 * Runs from internal Flash. Uses a dedicated external NOR sector and aligned
 * packed-word buffers. Write commands erase the test sector without restoring it.
 */

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/

#include "qspi_flash_read_write.h"
#include "qspi/neon_qspi.h"
#include "sdkconfig.h"
#include <stdio.h>

/*==================================================================================================
*                                        DEFINES AND MACROS
==================================================================================================*/

#if !defined(CONFIG_NEON_MDF_DRIVER_QSPI)
#error "This example requires the QSPI driver"
#endif

#define TEST_OFFSET CONFIG_QSPI_FLASH_READ_WRITE_TEST_OFFSET
#define TRANSFER_POLLS CONFIG_QSPI_FLASH_READ_WRITE_TIMEOUT_POLLS
#define READY_POLLS CONFIG_QSPI_FLASH_READ_WRITE_READY_POLLS
#define SECTOR_SIZE 4096U
#define PAGE_SIZE 256U
#define MAX_SIZE 2048U
#define SAMPLE_SIZE 64U
#define READ_SIZE 4U

_Static_assert(TEST_OFFSET >= SECTOR_SIZE && TEST_OFFSET <= 0x3F000U,
    "Reserve a test sector inside the 256-KiB QSPI window, after sector zero");
_Static_assert((TEST_OFFSET % SECTOR_SIZE) == 0U, "Test offset must be sector aligned");

/*==================================================================================================
*                                         LOCAL VARIABLES
==================================================================================================*/

static uint32_t write_data[MAX_SIZE / 4U];
static uint32_t read_data[MAX_SIZE / 4U];
static uint32_t last_size;
static uint32_t selected_level;
static uint32_t random_state;
static uint32_t selected_mode;
static uint32_t io_failed;
static uint32_t recovery_required;
static uint32_t stop_requested;
static const uint32_t fixed_pattern[] = {
    0x00000000U, 0xFFFFFFFFU, 0xAAAAAAAAU, 0x55555555U,
    0x01234567U, 0x89ABCDEFU, 0xA5A5A5A5U, 0x5A5A5A5AU,
};

/*==================================================================================================
*                                        PRIVATE FUNCTIONS
==================================================================================================*/

/**
 * @brief Detect a stop request between complete NOR operations.
 * @param[in,out] uart Initialized console UART.
 * @return Nonzero after the stop key or Ctrl+C is received.
 */
static uint32_t cancelled(neon_uart_handle_t *uart) {
    uint32_t key = 0U;
    if (neon_uart_try_receive(uart, &key) == NEON_OK && (key == 'x' || key == 3U)) {
        stop_requested = 1U;
    }
    return stop_requested;
}

/**
 * @brief Send a NOR frame, service its data FIFO and wait for controller completion.
 * @param[in] command Frame description.
 * @param[in,out] data Packed words for the data phase, or NULL without data.
 * @return QSPI status. A transport error ends the current menu session.
 */
static neon_status_t nor_transfer(const neon_qspi_command_t *command, uint32_t *data) {
    neon_status_t status = neon_qspi_send_command(QSPI_APB, command);
    if (status == NEON_OK && command->data_lines != NEON_QSPI_LINES_NONE) {
        status = command->receive_enable != 0U ?
            neon_qspi_receive_words(QSPI_APB, data, TRANSFER_POLLS) :
            neon_qspi_transmit_words(QSPI_APB, data, TRANSFER_POLLS);
    }
    if (status == NEON_OK) {
        status = neon_qspi_wait_complete(QSPI_APB, TRANSFER_POLLS);
    }
    if (status != NEON_OK) {
        io_failed = 1U;
        printf("[FAIL] QSPI opcode=%02lX status=%lu.\n",
            (unsigned long)command->instruction, (unsigned long)status);
        if (neon_qspi_abort(QSPI_APB, TRANSFER_POLLS) != NEON_OK) {
            recovery_required = 1U;
            printf("[FAIL] QSPI abort failed; reset the FPGA before another run.\n");
        }
    }
    return status;
}

/**
 * @brief Send a one-lane NOR command without a data phase.
 * @param[in] opcode Eight-bit NOR instruction.
 * @return Controller transfer status.
 */
static neon_status_t nor_command(uint32_t opcode) {
    neon_qspi_command_t command = {
        .instruction = opcode,
        .instruction_lines = NEON_QSPI_LINES_1,
    };
    return nor_transfer(&command, NULL);
}

/**
 * @brief Read a NOR status register or its three-byte JEDEC ID.
 * @param[in] opcode Read-register instruction.
 * @param[out] value Packed result, low wire byte first.
 * @param[in] count One, two or three bytes.
 * @return Controller transfer status.
 */
static neon_status_t nor_register(uint32_t opcode, uint32_t *value, uint32_t count) {
    neon_qspi_command_t command = {
        .instruction = opcode,
        .instruction_lines = NEON_QSPI_LINES_1,
        .data_lines = NEON_QSPI_LINES_1,
        .data_length = count,
        .receive_enable = 1U,
    };
    return nor_transfer(&command, value);
}

/**
 * @brief Poll NOR WIP; controller TC alone does not finish erase/program.
 * @return NEON_OK, a transport error or NEON_TIMEOUT.
 */
static neon_status_t nor_wait_ready(void) {
    for (uint32_t n = 0U; n < READY_POLLS; ++n) {
        uint32_t status_register = 0U;
        neon_status_t status = nor_register(0x05U, &status_register, 1U);
        if (status != NEON_OK) { return status; }
        if ((status_register & 1U) == 0U) { return NEON_OK; }
    }
    io_failed = 1U;
    printf("[FAIL] NOR WIP did not clear within the polling budget.\n");
    return NEON_TIMEOUT;
}

/**
 * @brief Request write-enable and confirm WEL before changing the NOR.
 * @return NEON_OK, a transport error or NEON_TIMEOUT.
 */
static neon_status_t nor_write_enable(void) {
    for (uint32_t n = 0U; n < READY_POLLS; ++n) {
        uint32_t sr1 = 0U;
        neon_status_t status = nor_command(0x06U);
        if (status == NEON_OK) { status = nor_register(0x05U, &sr1, 1U); }
        if (status != NEON_OK) { return status; }
        if ((sr1 & 3U) == 2U) { return NEON_OK; }
    }
    io_failed = 1U;
    printf("[FAIL] NOR write-enable latch did not set.\n");
    return NEON_TIMEOUT;
}

/**
 * @brief Apply the requested divider and mode through the QSPI driver.
 * @param[in] divider Encoded divider 0..7 for /2.. /256.
 * @return Initialization status.
 */
static neon_status_t configure_controller(uint32_t divider) {
    neon_qspi_config_t config;
    neon_qspi_get_default_config(&config);
    config.clock_divider = divider;
#ifdef CONFIG_QSPI_FLASH_READ_WRITE_MODE_3
    config.clock_mode = NEON_QSPI_CLOCK_MODE_3;
#endif
    neon_status_t status = neon_qspi_init(QSPI_APB, &config);
    if (status == NEON_BUSY) {
        status = neon_qspi_abort(QSPI_APB, TRANSFER_POLLS);
        if (status == NEON_OK) { status = neon_qspi_init(QSPI_APB, &config); }
        else { recovery_required = 1U; }
    }
    if (status == NEON_OK) {
        printf("[QSPI] mode=%lu divider=/%lu SCK=%lu Hz (calculated), CS0.\n",
            (unsigned long)config.clock_mode, (unsigned long)(2U << divider),
            (unsigned long)(SystemCoreClock >> (divider + 1U)));
    }
    return status;
}

/**
 * @brief Print NOR identification/status and validate the supported protocol and range.
 * @return NEON_OK, NEON_NOT_SUPPORTED, NEON_INVALID_PARAM or a transfer error.
 */
static neon_status_t nor_info(void) {
    uint32_t id = 0U, sr1 = 0U, sr2 = 0U;
    neon_status_t status = nor_register(0x9FU, &id, 3U);
    if (status == NEON_OK) { status = nor_register(0x05U, &sr1, 1U); }
    if (status == NEON_OK) { status = nor_register(0x35U, &sr2, 1U); }
    if (status != NEON_OK) { return status; }
    uint32_t manufacturer = id & 0xFFU, density = (id >> 16) & 0xFFU;
    printf("[NOR] JEDEC=%02lX %02lX %02lX SR1=%02lX SR2=%02lX QE=%lu.\n",
        (unsigned long)manufacturer, (unsigned long)((id >> 8) & 0xFFU),
        (unsigned long)density, (unsigned long)sr1, (unsigned long)sr2,
        (unsigned long)((sr2 >> 1) & 1U));
    if ((manufacturer != 0xEFU && manufacturer != 0xC8U) ||
        ((id >> 8) & 0xFFU) != 0x40U || density < 0x12U || density > 0x18U) {
        io_failed = 1U;
        printf("[FAIL] Expected a supported EF/C8 40 12..18 NOR; no test data written.\n");
        return NEON_NOT_SUPPORTED;
    }
    if (TEST_OFFSET + SECTOR_SIZE > (1U << density)) {
        io_failed = 1U;
        printf("[FAIL] Test sector exceeds the detected NOR capacity.\n");
        return NEON_INVALID_PARAM;
    }
    printf("[NOR] Capacity=%lu bytes, reserved test sector=%08lX..%08lX.\n",
        (unsigned long)(1U << density), (unsigned long)TEST_OFFSET,
        (unsigned long)(TEST_OFFSET + SECTOR_SIZE - 1U));
    return NEON_OK;
}

/**
 * @brief Exit continuous read, wake/reset the NOR at /32 and identify it.
 * @return NOR initialization status; no array erase/program is issued here.
 */
static neon_status_t nor_initialize(void) {
    neon_qspi_command_t leave_read = {
        .instruction = 0xFFU,
        .instruction_lines = NEON_QSPI_LINES_4,
        .address_lines = NEON_QSPI_LINES_4,
        .address = 0xFFFFFFU,
    };
    neon_status_t status = nor_transfer(&leave_read, NULL);
    if (status == NEON_OK) { status = nor_command(0xABU); }
    if (status == NEON_OK) { status = nor_wait_ready(); }
    if (status == NEON_OK) { status = nor_command(0x66U); }
    if (status == NEON_OK) { status = nor_command(0x99U); }
    if (status == NEON_OK) { status = nor_wait_ready(); }
    if (status == NEON_OK) { status = nor_info(); }
    return status;
}

/**
 * @brief Enable Quad data while preserving status/protection bits.
 * @return NEON_OK, NEON_ERROR for failed QE readback, or a transport error.
 */
static neon_status_t nor_enable_quad(void) {
    uint32_t sr1 = 0U, sr2 = 0U;
    neon_status_t status = nor_register(0x05U, &sr1, 1U);
    if (status == NEON_OK) { status = nor_register(0x35U, &sr2, 1U); }
    if (status != NEON_OK || (sr2 & 2U) != 0U) { return status; }
    uint32_t value = (sr1 & 0xFCU) | ((sr2 | 2U) << 8);
    neon_qspi_command_t command = {
        .instruction = 0x01U,
        .instruction_lines = NEON_QSPI_LINES_1,
        .data_lines = NEON_QSPI_LINES_1,
        .data_length = 2U,
    };
    status = nor_write_enable();
    if (status == NEON_OK) { status = nor_transfer(&command, &value); }
    if (status == NEON_OK) { status = nor_wait_ready(); }
    if (status == NEON_OK) { status = nor_register(0x35U, &sr2, 1U); }
    if (status == NEON_OK && (sr2 & 2U) == 0U) {
        printf("[FAIL] QE did not set; check NOR status protection and wiring.\n");
        status = NEON_ERROR;
    }
    return status;
}

/**
 * @brief Read one four-byte frame through the selected Single/Dual/Quad protocol.
 * @param[in] address NOR byte offset.
 * @param[out] data Packed receive words.
 * @param[in] mode Zero=Single, one=Dual, two=Quad.
 * @return Transfer status.
 */
static neon_status_t nor_read_frame(uint32_t address, uint32_t *data, uint32_t mode) {
    neon_qspi_command_t command = {
        .instruction = mode == 0U ? 0x03U : (mode == 1U ? 0x3BU : 0x6BU),
        .instruction_lines = NEON_QSPI_LINES_1,
        .address_lines = NEON_QSPI_LINES_1,
        .address = address,
        .dummy_cycles = mode == 0U ? 0U : 8U,
        .data_lines = mode == 0U ? NEON_QSPI_LINES_1 :
            (mode == 1U ? NEON_QSPI_LINES_2 : NEON_QSPI_LINES_4),
        .data_length = READ_SIZE,
        .receive_enable = 1U,
    };
    return nor_transfer(&command, data);
}

/**
 * @brief Read an array range using four-byte frames, one packed word per frame.
 * @param[in,out] uart Console used to cancel between frames.
 * @param[in] address NOR byte offset.
 * @param[out] data Packed receive words.
 * @param[in] count Byte count, divisible by four.
 * @param[in] mode Zero=Single, one=Dual, two=Quad.
 * @return Transfer status or NEON_BUSY on cancellation.
 */
static neon_status_t nor_read(neon_uart_handle_t *uart, uint32_t address,
    uint32_t *data, uint32_t count, uint32_t mode) {
    neon_status_t status = nor_wait_ready();
    if (status != NEON_OK) { return status; }
    for (uint32_t offset = 0U; offset < count; offset += READ_SIZE) {
        if (cancelled(uart)) { return NEON_BUSY; }
        status = nor_read_frame(address + offset, &data[offset / 4U], mode);
        if (status != NEON_OK) { return status; }
    }
    return NEON_OK;
}

/**
 * @brief Erase exactly the reserved sector and check all 4096 bytes are FF.
 * @param[in,out] uart Console used for cancellation between readback blocks.
 * @return NEON_OK, NEON_LOCKED, NEON_BUSY on cancellation, or an operation error.
 */
static neon_status_t nor_erase(neon_uart_handle_t *uart) {
    uint32_t sr1 = 0U, sr2 = 0U, erased[READ_SIZE / 4U];
    neon_status_t status = nor_wait_ready();
    if (status == NEON_OK) { status = nor_register(0x05U, &sr1, 1U); }
    if (status == NEON_OK) { status = nor_register(0x35U, &sr2, 1U); }
    if (status != NEON_OK) { return status; }
    if ((sr1 & 0x1CU) != 0U || (sr2 & 0xC0U) != 0U) {
        printf("[FAIL] NOR is protected or suspended; status bits are preserved.\n");
        return NEON_LOCKED;
    }
    neon_qspi_command_t command = {
        .instruction = 0x20U,
        .instruction_lines = NEON_QSPI_LINES_1,
        .address_lines = NEON_QSPI_LINES_1,
        .address = TEST_OFFSET,
    };
    status = nor_write_enable();
    if (status == NEON_OK) { status = nor_transfer(&command, NULL); }
    if (status == NEON_OK) { status = nor_wait_ready(); }
    if (status != NEON_OK) { return status; }
    for (uint32_t offset = 0U; offset < SECTOR_SIZE; offset += sizeof(erased)) {
        if (cancelled(uart)) { return NEON_BUSY; }
        status = nor_read_frame(TEST_OFFSET + offset, erased, 0U);
        if (status != NEON_OK) { return status; }
        for (uint32_t i = 0U; i < sizeof(erased) / sizeof(erased[0]); ++i) {
            if (erased[i] != 0xFFFFFFFFU) {
                printf("[FAIL] Erase readback at NOR=%08lX is not FFFFFFFF.\n",
                    (unsigned long)(TEST_OFFSET + offset + 4U * i));
                return NEON_ERROR;
            }
        }
    }
    return NEON_OK;
}

/**
 * @brief Program the test pattern without crossing 256-byte page boundaries.
 * @param[in,out] uart Console used for cancellation between pages.
 * @param[in] size Number of bytes starting at TEST_OFFSET.
 * @return NEON_OK, NEON_BUSY on cancellation, or an operation error.
 */
static neon_status_t nor_program(neon_uart_handle_t *uart, uint32_t size) {
    for (uint32_t offset = 0U; offset < size; offset += PAGE_SIZE) {
        if (cancelled(uart)) { return NEON_BUSY; }
        uint32_t count = size - offset;
        if (count > PAGE_SIZE) { count = PAGE_SIZE; }
        neon_qspi_command_t command = {
            .instruction = selected_mode == 2U ? 0x32U : 0x02U,
            .instruction_lines = NEON_QSPI_LINES_1,
            .address_lines = NEON_QSPI_LINES_1,
            .address = TEST_OFFSET + offset,
            .data_lines = selected_mode == 2U ? NEON_QSPI_LINES_4 : NEON_QSPI_LINES_1,
            .data_length = count,
        };
        neon_status_t status = nor_write_enable();
        if (status == NEON_OK) { status = nor_transfer(&command, &write_data[offset / 4U]); }
        if (status == NEON_OK) { status = nor_wait_ready(); }
        if (status != NEON_OK) { return status; }
    }
    return NEON_OK;
}

/**
 * @brief Fill the expected buffer using a fixed pattern or the next xorshift sequence.
 * @param[in] size Number of test bytes.
 */
static void fill_pattern(uint32_t size) {
    for (uint32_t i = 0U; i < size / 4U; ++i) {
        if (selected_level >= 5U) {
            write_data[i] = fixed_pattern[i % (sizeof(fixed_pattern) / sizeof(fixed_pattern[0]))];
        } else {
            random_state ^= random_state << 13;
            random_state ^= random_state >> 17;
            random_state ^= random_state << 5;
            write_data[i] = random_state;
        }
    }
}

/**
 * @brief Extract a byte after an aligned word load.
 * @param[in] data Packed word buffer; volatile access preserves the FU2 word ABI.
 * @param[in] offset Logical byte offset.
 * @return The selected byte.
 */
static uint32_t byte_at(const volatile uint32_t *data, uint32_t offset) {
    uint32_t word = data[offset / 4U];
    return (word >> (8U * (offset % 4U))) & 0xFFU;
}

/**
 * @brief Dump at most the first 64 bytes as sixteen-column hexadecimal rows.
 * @param[in] data Packed words containing the sample.
 * @param[in] size Total test size in bytes.
 */
static void dump_sample(const uint32_t *data, uint32_t size) {
    uint32_t count = size < SAMPLE_SIZE ? size : SAMPLE_SIZE;
    printf("[SAMPLE] First %lu of %lu bytes:\n", (unsigned long)count, (unsigned long)size);
    for (uint32_t offset = 0U; offset < count; ++offset) {
        if ((offset % 16U) == 0U) { printf("%08lX: ", (unsigned long)(TEST_OFFSET + offset)); }
        printf("%02lX%c", (unsigned long)byte_at(data, offset),
            (offset % 16U) == 15U || offset + 1U == count ? '\n' : ' ');
    }
}

/**
 * @brief Compare every test byte and print at most the first eight mismatches.
 * @param[in] size Number of bytes to compare.
 * @return Total mismatching byte count.
 */
static uint32_t compare_data(uint32_t size) {
    uint32_t mismatches = 0U;
    for (uint32_t offset = 0U; offset < size; ++offset) {
        uint32_t expected = byte_at(write_data, offset);
        uint32_t actual = byte_at(read_data, offset);
        if (expected != actual) {
            ++mismatches;
            if (mismatches <= 8U) {
                printf("[MISMATCH] NOR=%08lX expected=%02lX actual=%02lX.\n",
                    (unsigned long)(TEST_OFFSET + offset), (unsigned long)expected, (unsigned long)actual);
            }
        }
    }
    return mismatches;
}

/**
 * @brief Read and compare the complete pattern from the most recent successful program.
 * @param[in,out] uart Console used to cancel between frames.
 * @return NEON_OK, NEON_ERROR for mismatches, or a transport error.
 */
static neon_status_t read_verify(neon_uart_handle_t *uart) {
    if (last_size == 0U) { printf("[INFO] Write a pattern first using 0..9 or w.\n"); return NEON_OK; }
    neon_status_t status = nor_read(uart, TEST_OFFSET, read_data, last_size, selected_mode);
    if (status != NEON_OK) { return status; }
    dump_sample(read_data, last_size);
    uint32_t mismatches = compare_data(last_size);
    printf("[VERIFY] bytes=%lu mismatches=%lu %s\n", (unsigned long)last_size,
        (unsigned long)mismatches, mismatches == 0U ? "[PASS]" : "[FAIL]");
    return mismatches == 0U ? NEON_OK : NEON_ERROR;
}

/**
 * @brief Erase the reserved sector, program a selected pattern and verify every byte.
 * @param[in,out] uart Console used to cancel between complete operations.
 * @return Test or transport status; no PASS is printed on cancellation.
 */
static neon_status_t write_verify(neon_uart_handle_t *uart) {
    uint32_t size = 128U << (selected_level % 5U);
    if (cancelled(uart)) { return NEON_BUSY; }
    last_size = 0U;
    fill_pattern(size);
    printf("[WRITE] level=%lu bytes=%lu pattern=%s mode=%lu; erase sector, no restore.\n",
        (unsigned long)selected_level, (unsigned long)size,
        selected_level < 5U ? "xorshift32" : "fixed", (unsigned long)selected_mode);
    neon_status_t status = nor_erase(uart);
    if (status == NEON_OK) { status = nor_program(uart, size); }
    if (status != NEON_OK) { return status; }
    last_size = size;
    return read_verify(uart);
}

/** @brief Print test sizes, mode selection and stop commands. */
static void print_help(void) {
    printf("\n i: JEDEC/status | s: Single | d: Dual read | q: Quad read/write\n");
    printf(" 0..4: random, 5..9: fixed; each group uses 128/256/512/1024/2048 bytes\n");
    printf(" w: repeat write/test | r: read/verify last write | a: all 10 levels\n");
    printf(" h/?: help | x/Ctrl+C: stop or return to fpga:>>\n");
    printf(" [WRITE] commands erase all 4096 bytes at %08lX; no restore.\n", (unsigned long)TEST_OFFSET);
    printf(" [MODE] 0=Single 02/03, 1=Single write 02 + Dual read 3B, 2=Quad 32/6B.\n");
}

/*==================================================================================================
*                                         GLOBAL FUNCTIONS
==================================================================================================*/

/** @copydoc qspi_flash_read_write_run */
void qspi_flash_read_write_run(neon_uart_handle_t *uart) {
    if (recovery_required != 0U) {
        printf("[FAIL] QSPI recovery required; reset the FPGA before another run.\n");
        return;
    }
    last_size = 0U;
    selected_level = 0U;
    random_state = 0x6D2B79F5U;
    selected_mode = 0U;
    io_failed = 0U;
    stop_requested = 0U;
    neon_status_t status = configure_controller(NEON_QSPI_CLK_DIV_32);
    if (status != NEON_OK) {
        printf("[FAIL] QSPI initialization status=%lu.\n", (unsigned long)status);
        return;
    }
    status = nor_initialize();
    if (status == NEON_OK) { status = configure_controller(CONFIG_QSPI_FLASH_READ_WRITE_DIVIDER); }
    if (status == NEON_OK) {
        print_help();
        printf("[READY] QSPI_Flash_Read_Write menu, no Enter needed.\n");
        for (;;) {
            uint32_t key = 0U;
            if (neon_uart_try_receive(uart, &key) != NEON_OK) { continue; }
            if (key == 'x' || key == 3U) { break; }
            stop_requested = 0U;
            status = NEON_OK;
            if (key >= '0' && key <= '9') {
                selected_level = key - '0';
                status = write_verify(uart);
            } else if (key == 'w') { status = write_verify(uart); }
            else if (key == 'r') { status = read_verify(uart); }
            else if (key == 'i') { status = nor_info(); }
            else if (key == 'h' || key == '?') { print_help(); }
            else if (key == 'a') {
                uint32_t passed = 0U, failed = 0U;
                for (selected_level = 0U; selected_level < 10U; ++selected_level) {
                    if (cancelled(uart)) { break; }
                    status = write_verify(uart);
                    if (stop_requested != 0U || io_failed != 0U) { break; }
                    if (status == NEON_OK) { ++passed; } else { ++failed; }
                }
                selected_level = 0U;
                printf("[SWEEP] passed=%lu failed=%lu completed=%lu/10 %s\n",
                    (unsigned long)passed, (unsigned long)failed, (unsigned long)(passed + failed),
                    passed == 10U ? "[PASS]" : "[FAIL/incomplete]");
            } else if (key == 's' || key == 'd' || key == 'q') {
                uint32_t mode = key == 's' ? 0U : (key == 'd' ? 1U : 2U);
                if (mode == 2U) { status = nor_enable_quad(); }
                if (status == NEON_OK) {
                    selected_mode = mode;
                    printf("[MODE] %lu selected; array data unchanged.\n", (unsigned long)mode);
                }
            } else { continue; }
            if (stop_requested != 0U) { printf("[ABORT] Test incomplete; written data is retained.\n"); }
            else if (status != NEON_OK) { printf("[FAIL] Operation status=%lu.\n", (unsigned long)status); }
            if (io_failed != 0U || recovery_required != 0U) { break; }
            printf("[MENU] i/s/d/q/0..9/w/r/a/h or x to exit.\n");
        }
    } else { printf("[FAIL] NOR setup status=%lu.\n", (unsigned long)status); }
    if (recovery_required == 0U) {
        status = neon_qspi_abort(QSPI_APB, TRANSFER_POLLS);
        if (status == NEON_OK) { status = neon_qspi_deinit(QSPI_APB); }
        if (status != NEON_OK) {
            recovery_required = 1U;
            printf("[FAIL] QSPI cleanup status=%lu; reset required.\n", (unsigned long)status);
        }
    }
    printf("[INFO] QSPI_Flash_Read_Write stopped.\n");
}
