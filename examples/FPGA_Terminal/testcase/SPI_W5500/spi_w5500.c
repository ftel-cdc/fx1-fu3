/**
 * @file spi_w5500.c
 * @brief SPI0 master <-> W5500 register and SRAM checks.
 * @details Uses Neon SPI APIs; the driver configures the SPI0 pins.
 * SPI checks only: no IP address, sockets, DMA, IRQ or LAN cable required.
 */
#include "spi_w5500.h"
#include "sdkconfig.h"
#include "crm/neon_crm.h"
#include "spi/neon_spi.h"
#include <stdio.h>

#ifndef CONFIG_NEON_MDF_DRIVER_SPI
#error "SPI_W5500 requires CONFIG_NEON_MDF_DRIVER_SPI=y"
#endif

#define SPI_TIMEOUT CONFIG_SPI_W5500_TIMEOUT_POLLS
#define W5500_MR 0x0000U
#define W5500_RTR 0x0019U
#define W5500_RCR 0x001BU
#define W5500_PHYCFGR 0x002EU
#define W5500_VERSIONR 0x0039U
#define SYSTICK_MASK 0x00FFFFFFU

static neon_spi_config_t g_spi_config;
static uint32_t g_mode;
static uint32_t g_sck_hz;
static uint32_t g_last_tick;
static uint32_t g_elapsed;

/* SysTick runs without interrupts. Sample more often than a 24-bit wrap. */
static uint32_t spi_w5500_ticks(void) {
    uint32_t now = SysTick->VAL & SYSTICK_MASK;
    g_elapsed += (g_last_tick - now) & SYSTICK_MASK;
    g_last_tick = now;
    return g_elapsed;
}

static int spi_w5500_delay_ms(uint32_t ms) {
    while (ms-- != 0U) {
        uint32_t start = spi_w5500_ticks();
        uint32_t polls = SPI_TIMEOUT;
        while ((spi_w5500_ticks() - start) < SystemCoreClock / 1000U) {
            if (polls-- == 0U) {
                printf("[FAIL] SysTick is not advancing.\n");
                return 0;
            }
        }
    }
    return 1;
}

static int spi_w5500_configure(uint32_t mode, uint32_t requested_hz) {
    neon_spi_get_default_config(&g_spi_config);
    g_spi_config.clock_polarity = mode == 3U ? NEON_SPI_POLARITY_HIGH : NEON_SPI_POLARITY_LOW;
    g_spi_config.clock_phase = mode == 3U ? NEON_SPI_PHASE_2EDGE : NEON_SPI_PHASE_1EDGE;
    g_spi_config.data_width = NEON_SPI_DATA_WIDTH_32BIT;
    g_spi_config.slave_select = NEON_SPI_SLAVE_SELECT_0;
    uint32_t divisor = (SystemCoreClock + 2U * requested_hz - 1U) / (2U * requested_hz);
    g_spi_config.clock_divider = divisor > 0U ? divisor - 1U : 0U;
    g_spi_config.timing.slave_select_to_clock = (SystemCoreClock + 999999U) / 1000000U;
    g_spi_config.timing.clock_to_slave_select = g_spi_config.timing.slave_select_to_clock;
    neon_status_t status = neon_spi_init(SPI0, &g_spi_config);
    if (status != NEON_OK) {
        printf("[FAIL] SPI0 init status=%lu.\n", (unsigned long)status);
        return 0;
    }
    g_mode = mode;
    g_sck_hz = requested_hz;
    uint32_t actual = 0U;
    neon_spi_get_clock_frequency(SPI0, &actual);
    printf("[SPI0] mode=%lu MSB SCK=%lu Hz, hardware SS0, CTRL=0x%08lX\n",
           (unsigned long)mode, (unsigned long)actual, (unsigned long)SPI0->CTRL);
    printf("[SPI0] B2B=%lu SCK periods, T_B2B=%lu core cycles.\n",
           (unsigned long)g_spi_config.timing.between_frames,
           (unsigned long)((SPI0->TREG1 & SPI_TREG1_T_B2B_Msk) >> SPI_TREG1_T_B2B_Pos));
    return 1;
}

/* Common-register VDM frame: addr_hi, addr_lo, control, data/dummy.
 * One 32-bit transfer holds SS0 LOW across all four bytes. Write RX is not ACK. */
static int w5500_access8(uint32_t address, uint32_t write, uint32_t data, uint32_t *result) {
    uint32_t tx = (address << 16) | (write != 0U ? 0x0400U : 0U) | (data & 0xFFU);
    uint32_t rx = 0U;
    spi_w5500_ticks();
    printf("[FU2 -> W5500] %s addr=0x%04lX TX=0x%08lX\n", write != 0U ? "WRITE" : "READ",
           (unsigned long)address, (unsigned long)tx);
    neon_status_t status = neon_spi_transmit_receive_words(SPI0, &tx, &rx, 1U, SPI_TIMEOUT);
    if (status != NEON_OK) {
        printf("[FAIL] SPI status=%lu; resetting SPI0 before retry.\n", (unsigned long)status);
        neon_spi_init(SPI0, &g_spi_config);
        return 0;
    }
    printf("[W5500 -> FU2] RX=0x%08lX DATA=0x%02lX%s\n", (unsigned long)rx,
           (unsigned long)(rx & 0xFFU), write != 0U ? " (ignored for WRITE)" : "");
    *result = rx & 0xFFU;
    spi_w5500_ticks();
    return 1;
}

static int w5500_version(void) {
    uint32_t version = 0U;
    if (!w5500_access8(W5500_VERSIONR, 0U, 0U, &version)) {
        return 0;
    }
    printf("[W5500] VERSIONR=0x%02lX %s\n", (unsigned long)version,
           version == 0x04U ? "[OK]" : "[FAIL] expected 0x04");
    return version == 0x04U;
}

static int w5500_soft_reset(void) {
    uint32_t mr = 0U;
    printf("[RESET] W5500 software reset via SPI, MR=0x80.\n");
    /* Reset SPI0 first to release CS and clear its FIFOs. Keep the selected timing. */
    neon_status_t status = neon_spi_init(SPI0, &g_spi_config);
    if (status != NEON_OK) {
        printf("[RESET] [FAIL] SPI0 init status=%lu.\n", (unsigned long)status);
        return 0;
    }
    /* Explicit reset can be attempted even when a previous VERSIONR read failed. */
    if (!w5500_access8(W5500_MR, 1U, 0x80U, &mr)) {
        return 0;
    }
    for (uint32_t poll = 0U; poll < 20U; ++poll) {
        if (!spi_w5500_delay_ms(1U) || !w5500_access8(W5500_MR, 0U, 0U, &mr)) {
            return 0;
        }
        if (mr == 0U) {
            if (!w5500_version()) {
                printf("[RESET] [FAIL] Reset not confirmed; check SPI or reset the module via RSTn.\n");
                return 0;
            }
            printf("[RESET] MR=0x00, VERSIONR=0x04 [PASS]\n");
            return 1;
        }
    }
    printf("[RESET] [FAIL] MR did not clear, MR=0x%02lX.\n", (unsigned long)mr);
    return 0;
}

static int w5500_init_chip(void) {
    printf("[INIT] Wait 560 ms for module power-on/reset circuitry.\n");
    if (!spi_w5500_delay_ms(560U) || !w5500_version()) {
        return 0;
    }
    return w5500_soft_reset();
}

static void w5500_info(void) {
    uint32_t mr, phy, high, low, rcr;
    if (!w5500_version() || !w5500_access8(W5500_MR, 0U, 0U, &mr) ||
        !w5500_access8(W5500_PHYCFGR, 0U, 0U, &phy) || !w5500_access8(W5500_RTR, 0U, 0U, &high) ||
        !w5500_access8(W5500_RTR + 1U, 0U, 0U, &low) || !w5500_access8(W5500_RCR, 0U, 0U, &rcr)) {
        return;
    }
    printf("[INFO] MR=0x%02lX PHYCFGR=0x%02lX LINK=%s RTR=0x%04lX RCR=0x%02lX\n",
           (unsigned long)mr, (unsigned long)phy, (phy & 1U) != 0U ? "UP" : "DOWN",
           (unsigned long)((high << 8) | low), (unsigned long)rcr);
}

static void w5500_ping_pong(void) {
    uint32_t saved = 0U, first = 0U, second = 0U, restored = 0U, ignored = 0U;
    if (!w5500_version() || !w5500_access8(W5500_RCR, 0U, 0U, &saved)) {
        return;
    }
    int transferred = w5500_access8(W5500_RCR, 1U, 0x55U, &ignored) &&
                      w5500_access8(W5500_RCR, 0U, 0U, &first) &&
                      w5500_access8(W5500_RCR, 1U, 0xAAU, &ignored) &&
                      w5500_access8(W5500_RCR, 0U, 0U, &second);
    /* Restore even after a failed/mismatching pattern transfer. */
    int restore_ok = w5500_access8(W5500_RCR, 1U, saved, &ignored) &&
                     w5500_access8(W5500_RCR, 0U, 0U, &restored) && restored == saved;
    printf("[PING/PONG] write=55 read=%02lX; write=AA read=%02lX; RCR restored=%02lX %s\n",
           (unsigned long)first, (unsigned long)second, (unsigned long)restored,
           transferred && first == 0x55U && second == 0xAAU && restore_ok ? "[PASS]" : "[FAIL]");
}

static int w5500_read_sram32(uint32_t *data) {
    uint32_t tx[8] = {0x00001000U}, rx[8];
    neon_status_t status = neon_spi_set_slave_select_control(SPI0, NEON_SPI_SS_MODE_MANUAL,
                                                           NEON_SPI_SS_FORCE_HOLD, NEON_SPI_SS_LONG_DISABLE);
    /* Eight 32-bit words carry the 3-byte header and the first 29 data bytes.
     * Hardware B2B separates bytes inside each word; CS stays LOW throughout. */
    if (status != NEON_OK || neon_spi_transmit_receive_words(SPI0, tx, rx, 8U, SPI_TIMEOUT) != NEON_OK) {
        goto failed;
    }
    for (uint32_t index = 0U; index < 29U; ++index) {
        uint32_t position = index + 3U;
        data[index] = (rx[position / 4U] >> (24U - 8U * (position % 4U))) & 0xFFU;
    }
    spi_w5500_ticks();
    /* Finish with three dummy bytes and release CS after the last bit. */
    tx[0] = 0U;
    if (neon_spi_set_data_width(SPI0, NEON_SPI_DATA_WIDTH_24BIT) != NEON_OK ||
        neon_spi_set_slave_select_control(SPI0, NEON_SPI_SS_MODE_MANUAL,
            NEON_SPI_SS_FORCE_RELEASE, NEON_SPI_SS_LONG_DISABLE) != NEON_OK ||
        neon_spi_transmit_receive_words(SPI0, tx, rx, 1U, SPI_TIMEOUT) != NEON_OK) {
        goto failed;
    }
    data[29] = (rx[0] >> 16) & 0xFFU;
    data[30] = (rx[0] >> 8) & 0xFFU;
    data[31] = rx[0] & 0xFFU;
    if (neon_spi_set_data_width(SPI0, NEON_SPI_DATA_WIDTH_32BIT) != NEON_OK ||
        neon_spi_set_slave_select_control(SPI0, NEON_SPI_SS_MODE_NORMAL,
            NEON_SPI_SS_FORCE_HOLD, NEON_SPI_SS_LONG_DISABLE) != NEON_OK) {
        goto failed;
    }
    return 1;
failed:
    neon_spi_init(SPI0, &g_spi_config);
    return 0;
}

static void w5500_read32(void) {
    uint32_t data[32];
    if (!w5500_read_sram32(data)) {
        printf("[FAIL] SPI burst incomplete; SPI0 reset attempted, no partial dump.\n");
        return;
    }
    printf("[FU2 -> W5500] READ32 TX=00 00 10 + 32 dummy bytes, one CS assertion\n");
    printf("[W5500 -> FU2] socket 0 TX SRAM, offset 0x0000 (contents are not a test pattern)\n");
    for (uint32_t index = 0U; index < 32U; ++index) {
        if ((index % 16U) == 0U) {
            printf("%04lX: ", (unsigned long)index);
        }
        printf("%02lX%c", (unsigned long)data[index], (index % 16U) == 15U ? '\n' : ' ');
    }
}

/* CRC-32/ISO-HDLC over the 32 received bytes, stored as aligned words. */
static uint32_t w5500_crc32(const uint32_t *data) {
    uint32_t crc = 0xFFFFFFFFU;
    for (uint32_t index = 0U; index < 32U; ++index) {
        crc ^= data[index];
        for (uint32_t bit = 0U; bit < 8U; ++bit) {
            crc = (crc >> 1) ^ ((crc & 1U) != 0U ? 0xEDB88320U : 0U);
        }
    }
    return ~crc;
}

/* Returns 1 after a full sweep, 0 on an unstable baseline/configuration error,
 * and -1 when the user cancels. All cases share the first validated CRC. */
static int w5500_b2b_sweep(neon_uart_handle_t *uart, uint32_t *reference_crc, uint32_t make_reference) {
    uint32_t data[32], have_reference = make_reference == 0U, minimum = 11U;
    const uint32_t tries = CONFIG_SPI_W5500_B2B_TRIES;
    for (int32_t gap = 10; gap >= 0; --gap) {
        g_spi_config.timing.between_frames = (uint32_t)gap;
        g_spi_config.timing.between_frames_unit = NEON_SPI_B2B_SCK_CYCLES;
        if (neon_spi_set_timing(SPI0, &g_spi_config.timing) != NEON_OK) {
            printf("[B2B] Cannot configure %ld SCK periods; no minimum reported.\n", (long)gap);
            return 0;
        }
        uint32_t failed = 0U, crc_errors = 0U, id_errors = 0U, io_errors = 0U, first_bad_crc = 0U;
        for (uint32_t attempt = 0U; attempt < tries; ++attempt) {
            uint32_t key = 0U;
            if (neon_uart_try_receive(uart, &key) == NEON_OK &&
                (key == 'q' || key == 'Q' || key == 3U)) {
                printf("[B2B] Cancelled; partial sweep has no minimum result.\n");
                return -1;
            }
            uint32_t tx = 0x00390000U, rx = 0U;
            spi_w5500_ticks();
            if (neon_spi_transmit_receive_words(SPI0, &tx, &rx, 1U, SPI_TIMEOUT) != NEON_OK) {
                ++io_errors;
                ++failed;
                neon_spi_init(SPI0, &g_spi_config);
                continue;
            }
            uint32_t bad_id = (rx & 0xFFU) != 0x04U;
            id_errors += bad_id;
            if (!w5500_read_sram32(data)) {
                ++io_errors;
                ++failed;
                continue;
            }
            uint32_t crc = w5500_crc32(data);
            if (make_reference != 0U && gap == 10 && attempt == 0U && bad_id == 0U) {
                *reference_crc = crc;
                have_reference = 1U;
            }
            uint32_t bad_crc = have_reference == 0U || crc != *reference_crc;
            if (bad_crc != 0U) {
                if (crc_errors == 0U) { first_bad_crc = crc; }
                ++crc_errors;
            }
            failed += bad_id != 0U || bad_crc != 0U;
            spi_w5500_ticks();
        }
        uint32_t percent = (failed * 10000U) / tries;
        printf("[B2B] mode=%lu SCK=%lu Hz gap=%ld T_B2B=%lu: fail=%lu/%lu (%lu.%02lu%%) CRC=%lu ID=%lu IO=%lu %s\n",
               (unsigned long)g_mode, (unsigned long)g_sck_hz, (long)gap,
               (unsigned long)((SPI0->TREG1 & SPI_TREG1_T_B2B_Msk) >> SPI_TREG1_T_B2B_Pos),
               (unsigned long)failed, (unsigned long)tries, (unsigned long)(percent / 100U),
               (unsigned long)(percent % 100U), (unsigned long)crc_errors,
               (unsigned long)id_errors, (unsigned long)io_errors, failed == 0U ? "[PASS]" : "[FAIL]");
        if (crc_errors != 0U && have_reference != 0U) {
            printf("[B2B] Expected CRC32=%08lX; first different CRC32=%08lX.\n",
                   (unsigned long)*reference_crc, (unsigned long)first_bad_crc);
        }
        if (gap == 10) {
            if (failed != 0U) {
                printf("[B2B] mode=%lu SCK=%lu Hz fails at 10 SCK periods; no minimum reported.\n",
                       (unsigned long)g_mode, (unsigned long)g_sck_hz);
                return 0;
            }
            printf("[B2B] Reference CRC32=%08lX verified over %lu reads.\n",
                   (unsigned long)*reference_crc, (unsigned long)tries);
        }
        if (failed == 0U) { minimum = (uint32_t)gap; }
    }
    printf("[B2B] mode=%lu SCK=%lu Hz: smallest gap with zero errors = %lu SCK periods.\n",
           (unsigned long)g_mode, (unsigned long)g_sck_hz, (unsigned long)minimum);
    return 1;
}

static void w5500_b2b_test(neon_uart_handle_t *uart) {
    neon_spi_config_t saved_config = g_spi_config;
    uint32_t saved_mode = g_mode, saved_hz = g_sck_hz;
    static const uint32_t speeds[] = {1000000U, 3000000U, 5000000U, 10000000U};
    uint32_t reference_crc = 0U, reference_ready = 0U;
    printf("[B2B] Read-only mode 0/3 sweep, requested SCK 1/3/5/10 MHz.\n");
    printf("[B2B] Each case tests gaps 10..0 SCK periods, %lu reads per gap.\n",
           (unsigned long)CONFIG_SPI_W5500_B2B_TRIES);
    printf("[B2B] Shared CRC reference starts at mode 0 / 1 MHz / gap 10; VERSIONR must be 04.\n");
    printf("[B2B] Unavailable exact frequencies are skipped; q/Ctrl+C cancels to the menu.\n");
    for (uint32_t mode = 0U; mode <= 3U; mode += 3U) {
        for (uint32_t speed = 0U; speed < 4U; ++speed) {
            uint32_t actual = 0U;
            if (!spi_w5500_configure(mode, speeds[speed]) ||
                neon_spi_get_clock_frequency(SPI0, &actual) != NEON_OK) {
                printf("[B2B] Cannot configure mode=%lu requested=%lu Hz.\n",
                       (unsigned long)mode, (unsigned long)speeds[speed]);
                if (reference_ready == 0U) { goto restore; }
                continue;
            }
            if (actual != speeds[speed]) {
                printf("[B2B] SKIP mode=%lu requested=%lu Hz: divider gives %lu Hz at core=%lu Hz.\n",
                       (unsigned long)mode, (unsigned long)speeds[speed],
                       (unsigned long)actual, (unsigned long)SystemCoreClock);
                if (reference_ready == 0U) { goto restore; }
                continue;
            }
            int result = w5500_b2b_sweep(uart, &reference_crc, reference_ready == 0U);
            if (result < 0) { goto restore; }
            if (result == 0 && reference_ready == 0U) {
                printf("[B2B] Reference is unstable; remaining mode/speed cases cancelled.\n");
                goto restore;
            }
            reference_ready = 1U;
        }
    }
    printf("[B2B] Mode/speed sweep finished; results apply to this sample count.\n");
restore:
    g_spi_config = saved_config;
    g_mode = saved_mode;
    g_sck_hz = saved_hz;
    if (neon_spi_init(SPI0, &g_spi_config) != NEON_OK) {
        printf("[FAIL] Could not restore SPI0 after B2B test.\n");
    } else {
        printf("[B2B] Original SPI configuration restored; auto is OFF.\n");
    }
}

static void w5500_diagnose(void) {
    uint32_t saved_mode = g_mode, saved_hz = g_sck_hz, value = 0U;
    printf("[DIAG] Read-only probes, mode 0/3 at 1 MHz/100 kHz.\n");
    for (uint32_t mode = 0U; mode <= 3U; mode += 3U) {
        for (uint32_t speed = 0U; speed < 2U; ++speed) {
            if (spi_w5500_configure(mode, speed == 0U ? 1000000U : 100000U)) {
                w5500_access8(W5500_VERSIONR, 0U, 0U, &value);
                w5500_access8(W5500_RCR, 0U, 0U, &value);
                w5500_access8(W5500_PHYCFGR, 0U, 0U, &value);
            }
        }
    }
    spi_w5500_configure(saved_mode, saved_hz);
    printf("[DIAG] Selected mode/speed restored.\n");
}

static void w5500_debug_registers(const char *stage) {
    printf("[DBG] %s core=%lu CTRL=%08lX CLK=%08lX CMD=%08lX FIFO=%08lX\n", stage,
           (unsigned long)SystemCoreClock, (unsigned long)SPI0->CTRL, (unsigned long)SPI0->CLK,
           (unsigned long)SPI0->CMDCONF, (unsigned long)SPI0->FIFO);
    printf("[DBG] TREG0=%08lX TREG1=%08lX STT=%08lX CLKS=%08lX DMA=%08lX/%08lX/%08lX\n",
           (unsigned long)SPI0->TREG0, (unsigned long)SPI0->TREG1, (unsigned long)SPI0->STT,
           (unsigned long)SPI0->CLKS, (unsigned long)SPI0->CTRL_RX,
           (unsigned long)SPI0->CTRL_TX, (unsigned long)SPI0->STT_DMA);
    printf("[DBG] CRM gate1=%08lX reset_n2=%08lX (SPI0 gate bit10/reset bit0 must be 1)\n",
           (unsigned long)CRM->CLK_GATE_1, (unsigned long)CRM->SW_RST_N_2);
    printf("[DBG] SCK right21: mux=%lu IE=%lu DR=%lu OS=%lu OD=%lu expected=1/1/0/0/0\n",
           (unsigned long)((SYSCSR->RIGHT_IO_CFG_EXT >> 10) & 3U),
           (unsigned long)((SYSCSR->RIGHT_PAD_CTRL0 >> 21) & 1U),
           (unsigned long)((SYSCSR->RIGHT_PAD_CTRL5 >> 21) & 1U),
           (unsigned long)((SYSCSR->RIGHT_PAD_CTRL6 >> 21) & 1U),
           (unsigned long)((SYSCSR->RIGHT_PAD_CTRL7 >> 21) & 1U));
    printf("[DBG] MISO/MOSI/CS top0..2: mux=%02lX IE=%lX DR=%lX OS=%lX OD=%lX expected=15/7/0/0/0\n",
           (unsigned long)(SYSCSR->TOP_IO_CFG & 0x3FU), (unsigned long)(SYSCSR->TOP_PAD_CTRL0 & 7U),
           (unsigned long)(SYSCSR->TOP_PAD_CTRL5 & 7U), (unsigned long)(SYSCSR->TOP_PAD_CTRL6 & 7U),
           (unsigned long)(SYSCSR->TOP_PAD_CTRL7 & 7U));
}

static int w5500_debug_wait(uint32_t cycles) {
    uint32_t start = spi_w5500_ticks(), polls = SPI_TIMEOUT;
    while (spi_w5500_ticks() - start < cycles) {
        if (polls-- == 0U) { return 0; }
    }
    return 1;
}

/* Debug-only access: no UART output until the entire CS frame has finished. */
static int w5500_debug_mmio(uint32_t *result, uint32_t bytewise, uint32_t log) {
    uint32_t drained = 0U, stale = 0U, raw = 0U, ready = 0U;
    uint32_t before = SPI0->STT, start = spi_w5500_ticks();
    while ((SPI0->STT & SPI_STT_INT_RXFF_EMPTY_Msk) == 0U && drained < NEON_SPI_FIFO_DEPTH) {
        stale = SPI0->RX;
        ++drained;
    }
    uint32_t ctrl = SPI0->CTRL & ~(SPI_CTRL_SS_MODE_Msk | SPI_CTRL_SS_FORCE_Msk | SPI_CTRL_SS_LONG_Msk);
    SPI0->CTRL = ctrl | (bytewise != 0U ? SPI_CTRL_SS_MODE_Msk : 0U);
    uint32_t guard = 2U * ((SPI0->CLK & SPI_CLK_DIV_Msk) + 1U) +
                     (SPI0->TREG0 >> SPI_TREG0_T_CK2SS_Pos) + (SPI0->TREG1 & SPI_TREG1_T_B2B_Msk);
    for (uint32_t part = 0U; part < (bytewise != 0U ? 4U : 1U); ++part) {
        uint32_t polls = SPI_TIMEOUT;
        while ((SPI0->STT & SPI_STT_INT_TXFF_FULL_Msk) != 0U) {
            if (polls-- == 0U) { goto failed; }
        }
        if (bytewise != 0U && part == 3U) { SPI0->CTRL |= SPI_CTRL_SS_FORCE_Msk; }
        SPI0->CMDCONF = (bytewise != 0U ? 7U : 31U) | SPI_CMDCONF_RD_Msk |
                        (1U << SPI_CMDCONF_SLAVE_EN_Pos);
        SPI0->TX = bytewise != 0U ? (0x00390000U >> (24U - 8U * part)) & 0xFFU : 0x00390000U;
        polls = SPI_TIMEOUT;
        do {
            ready = SPI0->STT;
            if (polls-- == 0U) { goto failed; }
        } while ((ready & SPI_STT_INT_RXFF_EMPTY_Msk) != 0U);
        uint32_t rx = SPI0->RX;
        raw = bytewise != 0U ? (raw << 8) | (rx & 0xFFU) : rx;
        if (!w5500_debug_wait(guard)) { goto failed; }
    }
    *result = raw;
    uint32_t after = SPI0->STT, elapsed = spi_w5500_ticks() - start, clock = SPI0->CLKS;
    if (log != 0U) {
        printf("[DBG] MMIO%lu first: STT before=%08lX ready=%08lX after=%08lX drained=%lu stale=%08lX\n",
               bytewise != 0U ? 8UL : 32UL, (unsigned long)before, (unsigned long)ready,
               (unsigned long)after, (unsigned long)drained, (unsigned long)stale);
        printf("[DBG] MMIO frame ticks=%lu CLKS=%08lX (no delay before reading RX)\n",
               (unsigned long)elapsed, (unsigned long)clock);
    } else if (clock != 0U) {
        printf("[DBG] MMIO CLKS=%08lX\n", (unsigned long)clock);
    }
    return 1;
failed:
    printf("[DBG] MMIO timeout: partial_RX=%08lX STT=%08lX CLKS=%08lX\n",
           (unsigned long)raw, (unsigned long)SPI0->STT, (unsigned long)SPI0->CLKS);
    return 0;
}

/* GPIOC4=SCK, C5=MISO input, C6=MOSI, C7=CS. Mode 0, about 10 kHz or slower. */
static int w5500_debug_gpio_read(uint32_t address, uint32_t *result) {
    uint32_t tx = address << 16, rx = 0U;
    uint32_t half_period = (SystemCoreClock + 19999U) / 20000U;
    GPIOC->OUT &= ~((1U << 4) | (1U << 7));
    for (uint32_t bit = 0U; bit < 32U; ++bit) {
        GPIOC->OUT = (GPIOC->OUT & ~(1U << 6)) | ((tx >> 25) & (1U << 6));
        if (!w5500_debug_wait(half_period)) { goto failed; }
        GPIOC->OUT |= 1U << 4;
        if (!w5500_debug_wait(half_period)) { goto failed; }
        rx = (rx << 1) | ((GPIOC->IN >> 5) & 1U);
        GPIOC->OUT &= ~(1U << 4);
        tx <<= 1;
    }
    if (!w5500_debug_wait(half_period)) { goto failed; }
    GPIOC->OUT |= 1U << 7;
    if (!w5500_debug_wait(half_period)) { return 0; }
    *result = rx;
    return 1;
failed:
    GPIOC->OUT |= 1U << 7;
    return 0;
}

/* path: 0=public API, 1=MMIO32, 2=MMIO8 under one CS, 3=GPIO software SPI. */
static void w5500_debug_reads(const char *name, uint32_t path) {
    uint32_t good = 0U, bad = 0U, io = 0U, attempts = 0U, samples[4] = {0U};
    for (uint32_t trial = 0U; trial < 16U; ++trial) {
        uint32_t tx = 0x00390000U, rx = 0U;
        int ok;
        if (path == 0U) {
            neon_status_t status = neon_spi_transmit_receive_words(SPI0, &tx, &rx, 1U, SPI_TIMEOUT);
            ok = status == NEON_OK;
            uint32_t clock = SPI0->CLKS;
            if (!ok || clock != 0U) {
                printf("[DBG] API status=%lu STT=%08lX CLKS=%08lX\n", (unsigned long)status,
                       (unsigned long)SPI0->STT, (unsigned long)clock);
            }
        } else if (path < 3U) {
            ok = w5500_debug_mmio(&rx, path == 2U, trial == 0U);
        } else {
            ok = w5500_debug_gpio_read(W5500_VERSIONR, &rx);
        }
        ++attempts;
        if (!ok) { ++io; break; }
        if (trial < 4U) { samples[trial] = rx; }
        if ((rx & 0xFFU) == 0x04U) { ++good; } else { ++bad; }
        if (!spi_w5500_delay_ms(1U)) { ++io; break; }
    }
    printf("[DBG] %s VERSION: OK=%lu BAD=%lu IO=%lu attempts=%lu\n", name,
           (unsigned long)good, (unsigned long)bad, (unsigned long)io, (unsigned long)attempts);
    printf("[DBG] First raw RX: %08lX %08lX %08lX %08lX (only low byte is VERSION)\n",
           (unsigned long)samples[0], (unsigned long)samples[1],
           (unsigned long)samples[2], (unsigned long)samples[3]);
}

static void w5500_debug_gpio(void) {
    uint32_t clock = CRM->CLK_GATE_1 & CRM_CLK_GATE_1_GPIO2_OSC_CLK_EN_Msk;
    if ((CRM->SW_RST_N_2 & CRM_SW_RST_N_2_GPIO2_OSC_CLK_SWRST_Msk) == 0U ||
        neon_crm_peripheral_clock_enable(NEON_CRM_PERIPHERAL_GPIOC) != NEON_OK) {
        printf("[DBG] GPIO SKIP: GPIOC held in reset or clock unavailable.\n");
        return;
    }
    if ((GPIOC->INT_EN & 0xF0U) != 0U) {
        printf("[DBG] GPIO SKIP: GPIOC4..7 interrupts are in use.\n");
        goto clock_restore;
    }
    uint32_t oe = GPIOC->OE, out = GPIOC->OUT, ctl = GPIOC->CTL;
    uint32_t pending = GPIOC->RAW_INT & 0xF0U;
    uint32_t right_mux = SYSCSR->RIGHT_IO_CFG_EXT & (3U << 10);
    uint32_t top_mux = SYSCSR->TOP_IO_CFG & 0x3FU;
    GPIOC->OE &= ~0xF0U;
    GPIOC->OUT = (GPIOC->OUT & ~0xF0U) | (1U << 7);
    GPIOC->CTL &= ~0xF0U;
    SYSCSR->RIGHT_IO_CFG_EXT &= ~(3U << 10);
    SYSCSR->TOP_IO_CFG &= ~0x3FU;
    GPIOC->OE = (GPIOC->OE & ~0xF0U) | 0xD0U;
    printf("[DBG] GPIO mode0: PC4=SCK PC5=MISO PC6=MOSI PC7=CS, MISO stays INPUT.\n");
    printf("[DBG] GPIO OE=%08lX IN=%08lX OUT=%08lX; CS is HIGH before reads.\n",
           (unsigned long)GPIOC->OE, (unsigned long)GPIOC->IN, (unsigned long)GPIOC->OUT);
    if (spi_w5500_delay_ms(1U)) {
        w5500_debug_reads("GPIO-mode0", 3U);
        static const uint32_t addresses[] = {W5500_MR, W5500_RCR, W5500_PHYCFGR};
        for (uint32_t i = 0U; i < 3U; ++i) {
            uint32_t rx = 0U;
            if (!w5500_debug_gpio_read(addresses[i], &rx)) { break; }
            printf("[DBG] GPIO addr=%04lX raw=%08lX data=%02lX\n", (unsigned long)addresses[i],
                   (unsigned long)rx, (unsigned long)(rx & 0xFFU));
        }
    }
    GPIOC->OUT |= 1U << 7;
    GPIOC->OE &= ~0xF0U;
    SYSCSR->RIGHT_IO_CFG_EXT = (SYSCSR->RIGHT_IO_CFG_EXT & ~(3U << 10)) | right_mux;
    SYSCSR->TOP_IO_CFG = (SYSCSR->TOP_IO_CFG & ~0x3FU) | top_mux;
    GPIOC->OUT = (GPIOC->OUT & ~0xF0U) | (out & 0xF0U);
    GPIOC->CTL = (GPIOC->CTL & ~0xF0U) | (ctl & 0xF0U);
    GPIOC->RAW_INT = (GPIOC->RAW_INT & 0xF0U) & ~pending;
    GPIOC->OE = (GPIOC->OE & ~0xF0U) | (oe & 0xF0U);
clock_restore:
    if (clock == 0U) { neon_crm_peripheral_clock_disable(NEON_CRM_PERIPHERAL_GPIOC); }
}

static void w5500_debug(void) {
    printf("[DBG] BEGIN W5500-v2: read-only, auto OFF; no W5500 reset/write.\n");
    printf("[DBG] Each path reads VERSION 16 times, with 1 ms BETWEEN frames.\n");
    w5500_debug_registers("live");
    w5500_debug_reads("API-live", 0U);
    static const char *const names[] = {"API-reset", "MMIO32", "MMIO8-CS"};
    for (uint32_t path = 0U; path < 3U; ++path) {
        if (neon_spi_init(SPI0, &g_spi_config) != NEON_OK) { goto restore; }
        if (path == 0U) { w5500_debug_registers("after SPI0 reset"); }
        w5500_debug_reads(names[path], path);
    }
    if (neon_spi_init(SPI0, &g_spi_config) == NEON_OK) { w5500_debug_gpio(); }
restore:
    if (neon_spi_init(SPI0, &g_spi_config) != NEON_OK) {
        printf("[DBG] FAIL: SPI0 configuration restore failed.\n");
    } else {
        printf("[DBG] SPI0 configuration restored; auto stays OFF.\n");
    }
    printf("[DBG] END. Capture CS/SCK/MOSI/MISO if all paths fail; GPIO failure alone is inconclusive.\n");
}

static void spi_w5500_help(void) {
    printf("\n i: init/reset W5500 | t: info | p: SPI write/read/restore RCR\n");
    printf(" s: software reset W5500 via SPI (no VERSION precheck, auto OFF)\n");
    printf(" r: read 32 SRAM bytes | d: read-only diagnostics | h/?: help\n");
    printf(" v: debug VERSION via API/MMIO/GPIO, dump registers/pins (auto OFF)\n");
    printf(" b: read-only B2B 10..0, modes 0/3, SCK 1/3/5/10 MHz (exact only, auto OFF)\n");
    printf(" 0..4: mode 0 at 100/500/1000/2500/5000 kHz\n");
    printf(" 5..9: mode 3 at 100/500/1000/2500/5000 kHz\n");
    printf(" Preset runs ping/pong now, then every 3 s; a: toggle auto (starts OFF)\n");
    printf(" Ctrl+C or q: stop and return to fpga>\n\n");
}

void spi_w5500_run(neon_uart_handle_t *uart) {
    /* Borrow an idle SysTick without changing the CPU interrupt mask. */
    uint32_t saved_ctrl = SysTick->CTRL;
    uint32_t saved_load = SysTick->LOAD;
    if ((saved_ctrl & SysTick_CTRL_ENABLE_Msk) != 0U) {
        printf("[ERROR] SysTick is in use; SPI_W5500 needs an idle SysTick.\n");
        return;
    }
    SysTick->LOAD = SYSTICK_MASK;
    SysTick->VAL = 0U;
    SysTick->CTRL = SysTick_CTRL_ENABLE_Msk | SysTick_CTRL_CLKSOURCE_Msk;
    g_last_tick = SysTick->VAL;
    g_elapsed = 0U;
    if (!spi_w5500_configure(3U, CONFIG_SPI_W5500_SCK_HZ)) {
        goto cleanup;
    }
    printf("[INFO] SPI0 <-> W5500: SCK=PMOD_IO5 MOSI=IO4 MISO=IO1 CS=IO0.\n");
    if (w5500_init_chip()) {
        w5500_info();
    }
    spi_w5500_help();
    printf("[READY] SPI_W5500 menu, no Enter needed.\n");

    uint32_t automatic = 0U, last = spi_w5500_ticks();
    static const uint32_t speeds[] = {100000U, 500000U, 1000000U, 2500000U, 5000000U};
    for (;;) {
        uint32_t key = 0U;
        neon_status_t status = neon_uart_receive(uart, (uint8_t *)&key, 1U, CONFIG_FPGA_TERMINAL_RX_TIMEOUT_POLLS);
        if (status == NEON_OK) {
            if (key == 3U || key == 'q' || key == 'Q') {
                break;
            }
            if (key >= '0' && key <= '9') {
                uint32_t preset = key - '0';
                if (spi_w5500_configure(preset < 5U ? 0U : 3U, speeds[preset % 5U])) {
                    automatic = 1U;
                    printf("[AUTO] ON, test now then every 3 s.\n");
                    w5500_ping_pong();
                }
                last = spi_w5500_ticks();
            } else {
                switch (key) {
                    case 'i':
                        if (w5500_init_chip()) { w5500_info(); }
                        last = spi_w5500_ticks();
                        break;
                    case 's':
                        automatic = 0U;
                        printf("[AUTO] OFF.\n");
                        w5500_soft_reset();
                        last = spi_w5500_ticks();
                        break;
                    case 't': w5500_info(); break;
                    case 'p': w5500_ping_pong(); last = spi_w5500_ticks(); break;
                    case 'r': w5500_read32(); last = spi_w5500_ticks(); break;
                    case 'd': w5500_diagnose(); last = spi_w5500_ticks(); break;
                    case 'v':
                        automatic = 0U;
                        w5500_debug();
                        last = spi_w5500_ticks();
                        break;
                    case 'b':
                        automatic = 0U;
                        w5500_b2b_test(uart);
                        last = spi_w5500_ticks();
                        break;
                    case 'a':
                        automatic ^= 1U;
                        last = spi_w5500_ticks();
                        printf("[AUTO] %s (period 3 s).\n", automatic != 0U ? "ON" : "OFF");
                        break;
                    case 'h': case '?': spi_w5500_help(); break;
                    default: break; /* Ignore CR/LF left by the terminal command. */
                }
            }
        }
        uint32_t now = spi_w5500_ticks();
        if (automatic != 0U && now - last >= 3U * SystemCoreClock) {
            w5500_ping_pong();
            last = spi_w5500_ticks();
        }
    }
cleanup:
    neon_spi_deinit(SPI0);
    SysTick->CTRL = 0U;
    SysTick->LOAD = saved_load;
    SysTick->VAL = 0U;
    SysTick->CTRL = saved_ctrl;
    printf("\n[INFO] SPI_W5500 stopped; SPI0 released.\n");
}
