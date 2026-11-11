/**
 * @file spi_dma.c
 * @brief SPI0 DMA write/read of 32 bytes in W5500 socket 0 TX SRAM.
 * @details
 * SPI0 TX/RX DMA tests using a W5500 socket TX SRAM buffer.
 */

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/

#include "spi_dma.h"
#include "crm/neon_crm.h"
#include "dma/neon_dma.h"
#include "spi/neon_spi.h"
#include "sdkconfig.h"
#include <stdio.h>

/*==================================================================================================
*                                        DEFINES AND MACROS
==================================================================================================*/

#if !defined(CONFIG_NEON_MDF_DRIVER_SPI) || !defined(CONFIG_NEON_MDF_DRIVER_DMA)
#error "SPI_DMA requires both SPI and DMA drivers"
#endif

#define SPI_TIMEOUT CONFIG_SPI_DMA_TIMEOUT_POLLS
#define DATA_WORDS 8U
#define DATA_BYTES (DATA_WORDS * sizeof(uint32_t))
#define SPI0_DMA_SLOT 1U

/*==================================================================================================
*                                         LOCAL VARIABLES
==================================================================================================*/

/* A failed DMA command has no acknowledged abort. Keep its RAM alive until reset. */
static uint32_t dma_pending;

/*==================================================================================================
*                                        PRIVATE FUNCTIONS
==================================================================================================*/

/**
 * @brief Hold CS and send a socket-0 TX SRAM header before the DMA payload.
 * @param[in] write Nonzero for WRITE, zero for READ.
 * @return SPI configuration or transfer status.
 */
static neon_status_t w5500_sram_begin(uint32_t write) {
    /* Socket 0 TX buffer, offset 0: 00 00 14 (write) or 00 00 10 (read). */
    uint32_t header = write != 0U ? 0x000014U : 0x000010U;
    neon_status_t status = neon_spi_set_slave_select_control(SPI0, NEON_SPI_SS_MODE_MANUAL,
        NEON_SPI_SS_FORCE_HOLD, NEON_SPI_SS_LONG_DISABLE);
    if (status == NEON_OK) { status = neon_spi_set_data_width(SPI0, NEON_SPI_DATA_WIDTH_24BIT); }
    if (status == NEON_OK) { status = neon_spi_transmit_words(SPI0, &header, 1U, SPI_TIMEOUT); }
    if (status == NEON_OK) { status = neon_spi_set_data_width(SPI0, NEON_SPI_DATA_WIDTH_32BIT); }
    return status;
}

/**
 * @brief Disable SPI DMA requests and release CS after the payload finishes.
 * @return SPI completion or configuration status.
 */
static neon_status_t w5500_sram_end(void) {
    /* The payload has finished on the wire before this releases CS. */
    neon_status_t status = neon_spi_disable_dma(SPI0);
    if (status == NEON_OK) {
        status = neon_spi_set_slave_select_control(SPI0, NEON_SPI_SS_MODE_MANUAL,
            NEON_SPI_SS_FORCE_RELEASE, NEON_SPI_SS_LONG_DISABLE);
    }
    if (status == NEON_OK) { status = neon_spi_wait_tx_complete(SPI0, SPI_TIMEOUT); }
    if (status == NEON_OK) {
        status = neon_spi_set_slave_select_control(SPI0, NEON_SPI_SS_MODE_NORMAL,
            NEON_SPI_SS_FORCE_HOLD, NEON_SPI_SS_LONG_DISABLE);
    }
    return status;
}

/**
 * @brief Move the SRAM payload with TX DMA and optional RX DMA.
 * @param[in] tx Aligned TX words; also supply dummy words for an RX transfer.
 * @param[out] rx Aligned RX words, or NULL for TX only.
 * @return DMA/SPI completion status, transfer error or timeout.
 * @details
 * TX always runs: RX needs dummy TX words to generate SCK. NULL rx means TX only.
 */
static neon_status_t spi_dma_transfer(const uint32_t *tx, uint32_t *rx) {
    neon_spi_dma_config_t request = {
        .state = NEON_SPI_DMA_ENABLE,
        .transfer_mode = NEON_SPI_DMA_SINGLE,
        .burst_length = NEON_SPI_DMA_BURST_2, /* Ignored in SINGLE mode. */
        .total_bytes = DATA_BYTES,
    };
    neon_dma_config_t tx_dma = {
        .src_addr = (uint32_t)(uintptr_t)tx,
        .dst_addr = (uint32_t)(uintptr_t)&SPI0->TX,
        .transfer_size = DATA_BYTES,
        .channel = 0U,
        .read_config = NEON_DMA_CONFIG_M2M_WORD,
        .write_config = NEON_DMA_CONFIG_FIXED_WORD,
        .control = NEON_DMA_CONTROL_M2M,
        .peripheral_config = SPI0_DMA_SLOT << 16,
    };
    neon_dma_config_t rx_dma = {
        .src_addr = (uint32_t)(uintptr_t)&SPI0->RX,
        .dst_addr = (uint32_t)(uintptr_t)rx,
        .transfer_size = DATA_BYTES,
        .channel = 1U,
        .read_config = NEON_DMA_CONFIG_FIXED_WORD,
        .write_config = NEON_DMA_CONFIG_M2M_WORD,
        .control = NEON_DMA_CONTROL_M2M,
        .peripheral_config = SPI0_DMA_SLOT,
    };
    neon_status_t status = neon_spi_set_receive_capture(SPI0, rx != NULL);
    if (status == NEON_OK && rx != NULL) { status = neon_spi_configure_dma(SPI0, NEON_SPI_DMA_RX, &request); }
    if (status == NEON_OK) { status = neon_spi_configure_dma(SPI0, NEON_SPI_DMA_TX, &request); }
    if (status != NEON_OK) { return status; }

    __DMB();
    dma_pending = 1U;
    /* Arm RX first so it can drain the FIFO as soon as TX starts clocking. */
    if (rx != NULL) { status = neon_dma_start_channel(DMA0, &rx_dma); }
    if (status == NEON_OK) { status = neon_dma_start_channel(DMA0, &tx_dma); }
    if (status != NEON_OK) { return status; }

    uint32_t done_mask = SPI_STT_DMA_TRAN_DONE_TX_Msk;
    if (rx != NULL) { done_mask |= SPI_STT_DMA_TRAN_DONE_RX_Msk; }
    status = NEON_TIMEOUT;
    uint32_t work = 0U;
    while (work < SPI_TIMEOUT) {
        ++work; /* The CPU can do other work; DMA moves the payload. */
        neon_status_t tx_status = neon_dma_poll_channel(DMA0, 0U);
        neon_status_t rx_status = rx != NULL ? neon_dma_poll_channel(DMA0, 1U) : NEON_OK;
        if (tx_status != NEON_OK && tx_status != NEON_BUSY) { status = tx_status; break; }
        if (rx_status != NEON_OK && rx_status != NEON_BUSY) { status = rx_status; break; }
        if (tx_status == NEON_OK && rx_status == NEON_OK) {
            dma_pending = 0U; /* Both DMAC commands, including bus responses, have retired. */
            if ((SPI0->STT_DMA & done_mask) == done_mask) { status = NEON_OK; break; }
        }
    }
    printf("[MAIN] Iterations while DMA active: %lu\n", (unsigned long)work);
    /* DMA TX done means data reached the FIFO; the shifter may still be busy. */
    if (status == NEON_OK) { status = neon_spi_wait_tx_complete(SPI0, SPI_TIMEOUT); }
    return status;
}

/*==================================================================================================
*                                         GLOBAL FUNCTIONS
==================================================================================================*/

/** @copydoc spi_dma_run */
void spi_dma_run(void) {
    /* Each word carries four serial bytes, most significant byte first. */
    static uint32_t pattern[DATA_WORDS], dummy[DATA_WORDS], received[DATA_WORDS];
    if (dma_pending != 0U) {
        printf("[FAIL] Previous DMA command is incomplete. Reset FPGA before retry.\n");
        return;
    }
    /* This standalone example owns DMA0; refuse to reset another active user. */
    if (neon_dma_deinit(DMA0) != NEON_OK || neon_crm_peripheral_reset(NEON_CRM_PERIPHERAL_DMA0) != NEON_OK) {
        printf("[FAIL] DMA0 is busy or could not be initialized.\n");
        return;
    }
    neon_spi_config_t config;
    neon_spi_get_default_config(&config);
    config.clock_polarity = NEON_SPI_POLARITY_HIGH;
    config.clock_phase = NEON_SPI_PHASE_2EDGE;
    config.data_width = NEON_SPI_DATA_WIDTH_32BIT;
    config.slave_select = NEON_SPI_SLAVE_SELECT_0;
    uint32_t divisor = (SystemCoreClock + 2U * CONFIG_SPI_DMA_SCK_HZ - 1U) / (2U * CONFIG_SPI_DMA_SCK_HZ);
    config.clock_divider = divisor > 0U ? divisor - 1U : 0U;
    config.timing.slave_select_to_clock = (SystemCoreClock + 999999U) / 1000000U;
    config.timing.clock_to_slave_select = config.timing.slave_select_to_clock;
    const char *stage = "SPI0 init";
    neon_status_t status = neon_spi_init(SPI0, &config);
    if (status != NEON_OK) { goto failed; }
    uint32_t actual_hz = 0U;
    neon_spi_get_clock_frequency(SPI0, &actual_hz);
    printf("[SPI DMA] SPI0 mode 3, SCK=%lu Hz, B2B=%lu SCK periods.\n",
        (unsigned long)actual_hz, (unsigned long)config.timing.between_frames);
    printf("[SPI DMA] DMA0: TX channel 0, RX channel 1, peripheral slot 1, SINGLE 32-bit.\n");
    printf("[PINS] SCK=PMOD_IO5 MOSI=IO4 MISO=IO1 CS=IO0; common GND.\n");
    /* Conservative power-on wait, at least 560 ms with the correct core clock. */
    for (uint32_t ms = 0U; ms < 560U; ++ms) {
        for (uint32_t cycles = SystemCoreClock / 1000U; cycles != 0U; --cycles) { __NOP(); }
    }
    stage = "VERSION read";
    uint32_t command = 0x00390000U, version = 0U;
    status = neon_spi_transmit_receive_words(SPI0, &command, &version, 1U, SPI_TIMEOUT);
    if (status != NEON_OK) { goto failed; }
    printf("[W5500] VERSION=%02lX %s\n", (unsigned long)(version & 0xFFU),
        (version & 0xFFU) == 0x04U ? "[PASS]" : "[FAIL] expected 04; check SPI wiring/bitstream/power");
    if ((version & 0xFFU) != 0x04U) { goto cleanup; }

    for (uint32_t i = 0U; i < DATA_WORDS; ++i) {
        uint32_t b = 4U * i;
        pattern[i] = (b << 24) | ((b + 1U) << 16) | ((b + 2U) << 8) | (b + 3U);
        dummy[i] = 0U;
        received[i] = 0U;
    }
    stage = "DMA TX";
    printf("[DMA TX] Write 32 bytes to socket 0 TX SRAM: 00 01 ... 1F.\n");
    status = w5500_sram_begin(1U);
    if (status == NEON_OK) { status = spi_dma_transfer(pattern, NULL); }
    if (status == NEON_OK) { status = w5500_sram_end(); }
    if (status != NEON_OK) { goto failed; }

    stage = "polling readback";
    status = w5500_sram_begin(0U);
    if (status == NEON_OK) { status = neon_spi_receive_words(SPI0, received, DATA_WORDS, 0U, SPI_TIMEOUT); }
    if (status == NEON_OK) { status = w5500_sram_end(); }
    if (status != NEON_OK) { goto failed; }
    for (uint32_t i = 0U; i < 32U; ++i) {
        uint32_t value = (received[i / 4U] >> (24U - 8U * (i % 4U))) & 0xFFU;
        if (value != i) {
            printf("[DMA TX] [FAIL] byte %lu: expected=%02lX received=%02lX\n",
                (unsigned long)i, (unsigned long)i, (unsigned long)value);
            goto cleanup;
        }
    }
    printf("[DMA TX] Polling readback: 32/32 matched [PASS]\n");

    stage = "DMA RX";
    printf("[DMA RX] Read 32 bytes; DMA TX sends dummy words to generate SCK.\n");
    for (uint32_t i = 0U; i < DATA_WORDS; ++i) { received[i] = 0xA5A5A5A5U; }
    status = w5500_sram_begin(0U);
    if (status == NEON_OK) { status = spi_dma_transfer(dummy, received); }
    if (status == NEON_OK) { status = w5500_sram_end(); }
    if (status != NEON_OK) { goto failed; }
    for (uint32_t i = 0U; i < 32U; ++i) {
        uint32_t value = (received[i / 4U] >> (24U - 8U * (i % 4U))) & 0xFFU;
        if (value != i) {
            printf("[DMA RX] [FAIL] byte %lu: expected=%02lX received=%02lX\n",
                (unsigned long)i, (unsigned long)i, (unsigned long)value);
            goto cleanup;
        }
    }
    printf("[DMA RX] Compare: 32/32 matched [PASS]\n");
    printf("[SPI DMA] TX and RX [PASS]\n");
    goto cleanup;

failed:
    printf("[FAIL] %s status=%lu.\n", stage, (unsigned long)status);
cleanup:
    if (dma_pending != 0U) {
        neon_spi_disable_dma(SPI0);
        /* Park new service; this does not acknowledge an abort of accepted AXI traffic. */
        DMA0->CH0_ENABLE = 0U;
        DMA0->CH1_ENABLE = 0U;
        __DSB();
        printf("[FAIL] DMA incomplete; buffers/SPI retained. Reset FPGA before retry.\n");
        return;
    }
    neon_spi_deinit(SPI0);
    neon_dma_deinit(DMA0);
}
