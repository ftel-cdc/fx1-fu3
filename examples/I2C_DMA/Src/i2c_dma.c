/**
 * @file i2c_dma.c
 * @brief I2C0 DMA write/read of 16 bytes in an AT24C256 EEPROM.
 * @details
 * Write a fixed pattern at offset 0, wait for the EEPROM, then read and compare.
 */

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include "i2c_dma.h"
#include "crm/neon_crm.h"
#include "dma/neon_dma.h"
#include "i2c/neon_i2c.h"
#include "sdkconfig.h"
#include <stdio.h>

/*==================================================================================================
*                                        DEFINES AND MACROS
==================================================================================================*/

#if !defined(CONFIG_NEON_MDF_DRIVER_I2C) || !defined(CONFIG_NEON_MDF_DRIVER_DMA)
#error "I2C_DMA requires both I2C and DMA drivers"
#endif

#define TIMEOUT CONFIG_I2C_DMA_TIMEOUT_POLLS
#define DEVICE_ADDRESS CONFIG_I2C_DMA_ADDRESS
#define DATA_SIZE 16U
#define I2C0_DMA_SLOT 4U
#define BUS_PENDING (I2C_ICTLR_START_Msk | I2C_ICTLR_STOP_Msk)
#define TRANSFER_ERRORS (NEON_I2C_FLAG_ADDRESS_NACK | NEON_I2C_FLAG_DATA_NACK | \
    NEON_I2C_FLAG_ARBITRATION_LOST | NEON_I2C_FLAG_SCL_STUCK_LOW | NEON_I2C_FLAG_SDA_STUCK_LOW)

/*==================================================================================================
*                                         LOCAL VARIABLES
==================================================================================================*/

static neon_i2c_handle_t i2c;
/* A parked DMA command can still own RAM. Retain the handle and buffers until reset. */
static uint32_t reset_required;

/*==================================================================================================
*                                        PRIVATE FUNCTIONS
==================================================================================================*/

/**
 * @brief Request STOP and wait for both the command and bus to become idle.
 * @return I2C status or NEON_TIMEOUT.
 */
static neon_status_t stop_bus(void) {
    if ((I2C0->ICTLR & BUS_PENDING) == 0U && (I2C0->ISR & I2C_ISR_BUS_BUSY_Msk) == 0U) {
        return NEON_OK;
    }
    neon_status_t status = neon_i2c_generate_stop(&i2c);
    if (status != NEON_OK) { return status; }
    for (uint32_t poll = 0U; poll < TIMEOUT; ++poll) {
        if ((I2C0->ICTLR & BUS_PENDING) == 0U && (I2C0->ISR & I2C_ISR_BUS_BUSY_Msk) == 0U) {
            return NEON_OK;
        }
    }
    return NEON_TIMEOUT;
}

/**
 * @brief Move the 16-byte EEPROM payload using DMA1.
 * @param[in,out] data One logical I2C byte in the low byte of each aligned word.
 * @param[in] direction TX uses channel 0; RX uses channel 1.
 * @return NEON_OK after DMA, I2C byte count and STOP all complete, otherwise an error.
 */
static neon_status_t transfer(uint32_t *data, neon_i2c_dma_direction_t direction) {
    uint32_t receiving = direction == NEON_I2C_DMA_RX;
    neon_i2c_dma_config_t request = {
        .mode = NEON_I2C_DMA_SINGLE,
        .burst_length = NEON_I2C_DMA_BURST_2, /* Ignored in SINGLE mode. */
        .total_bytes = DATA_SIZE,
    };
    neon_dma_config_t dma = {
        .src_addr = (uint32_t)(uintptr_t)(receiving != 0U ? &I2C0->IRDR : data),
        .dst_addr = (uint32_t)(uintptr_t)(receiving != 0U ? data : &I2C0->ITDR),
        .transfer_size = DATA_SIZE * sizeof(uint32_t),
        .channel = receiving != 0U ? 1U : 0U,
        .read_config = receiving != 0U ? NEON_DMA_CONFIG_FIXED_WORD : NEON_DMA_CONFIG_M2M_WORD,
        .write_config = receiving != 0U ? NEON_DMA_CONFIG_M2M_WORD : NEON_DMA_CONFIG_FIXED_WORD,
        .control = NEON_DMA_CONTROL_M2M,
        .peripheral_config = receiving != 0U ? I2C0_DMA_SLOT : I2C0_DMA_SLOT << 16,
    };
    neon_status_t status = neon_i2c_clear_interrupts(&i2c, NEON_I2C_INTERRUPT_ALL);
    if (status == NEON_OK) { status = neon_i2c_set_dma_config(&i2c, direction, &request); }
    if (status != NEON_OK) { return status; }

    /* The HAL configures DMA requests but has no DMA transaction START API.
     * This standalone application owns I2C0 and programs the bus frame here. */
    I2C0->ITSAR = DEVICE_ADDRESS;
    I2C0->IBCR = DATA_SIZE + (receiving != 0U ? 0U : 2U);
    I2C0->ICFGR &= ~(I2C_ICFGR_READ_WRITE_Msk | I2C_ICFGR_SUB_ADDR_EN_Msk | I2C_ICFGR_HW_AUTO_END_Msk);
    if (receiving != 0U) {
        I2C0->ICFGR |= I2C_ICFGR_READ_WRITE_Msk;
    } else {
        /* AT24C256 offset 0000: queue its two address bytes before enabling DMA. */
        I2C0->ITDR = 0U;
        I2C0->ITDR = 0U;
    }
    status = neon_i2c_enable_interrupts(&i2c, NEON_I2C_INTERRUPT_TRANSFER_DONE |
        NEON_I2C_INTERRUPT_ADDRESS_NACK | NEON_I2C_INTERRUPT_DATA_NACK);
    if (status == NEON_OK) { status = neon_i2c_configure_dma(&i2c, direction, 1U); }
    if (status != NEON_OK) { return status; }
    __DMB();
    status = neon_dma_start_channel(DMA1, &dma);
    if (status != NEON_OK) { return status; }
    reset_required = 1U;
    I2C0->ICTLR |= I2C_ICTLR_START_Msk;

    uint32_t stop_sent = 0U;
    for (uint32_t poll = 0U; poll < TIMEOUT; ++poll) {
        neon_status_t dma_status = neon_dma_poll_channel(DMA1, receiving != 0U ? 1U : 0U);
        if (dma_status == NEON_OK) { reset_required = 0U; }
        else if (dma_status != NEON_BUSY) { return dma_status; }

        uint32_t flags = 0U, complete = 0U;
        status = neon_i2c_get_flags(&i2c, &flags);
        if (status != NEON_OK) { return status; }
        if ((flags & TRANSFER_ERRORS) != 0U) { return NEON_ERROR; }
        /* DONE holds SCL low. Send STOP even if RX DMA is still draining the FIFO. */
        if ((flags & NEON_I2C_FLAG_TRANSFER_DONE) != 0U && stop_sent == 0U) {
            status = neon_i2c_generate_stop(&i2c);
            if (status != NEON_OK) { return status; }
            stop_sent = 1U;
        }
        /* The byte output owns a complete word, as required by the FU2 runtime. */
        status = neon_i2c_is_dma_complete(&i2c, direction, (uint8_t *)&complete);
        if (status != NEON_OK) { return status; }
        if (dma_status == NEON_OK && complete != 0U && stop_sent != 0U &&
            (I2C0->ICTLR & BUS_PENDING) == 0U && (flags & NEON_I2C_FLAG_BUS_BUSY) == 0U) {
            return neon_i2c_configure_dma(&i2c, direction, 0U);
        }
    }
    return NEON_TIMEOUT;
}

/**
 * @brief Wait at least six milliseconds for the EEPROM write cycle.
 * @details Busy delay; SystemCoreClock must match the actual FPGA clock.
 */
static void write_delay(void) {
    for (uint32_t ms = 0U; ms < 6U; ++ms) {
        for (uint32_t cycles = SystemCoreClock / 1000U; cycles != 0U; --cycles) { __NOP(); }
    }
}

/*==================================================================================================
*                                         GLOBAL FUNCTIONS
==================================================================================================*/

/** @copydoc i2c_dma_run */
void i2c_dma_run(void) {
    /* Each 32-bit FIFO access moves one I2C byte, unlike packed byte buffers. */
    static uint32_t tx[DATA_SIZE], rx[DATA_SIZE];
    if (reset_required != 0U) {
        printf("[FAIL] Previous transfer is incomplete. Reset FPGA before retry.\n");
        return;
    }
    /* DMA1 belongs to this example; do not reset another active user. */
    if (neon_dma_deinit(DMA1) != NEON_OK || neon_crm_peripheral_reset(NEON_CRM_PERIPHERAL_DMA1) != NEON_OK) {
        printf("[FAIL] DMA1 is busy or could not be initialized.\n");
        return;
    }
    neon_i2c_config_t config;
    neon_i2c_get_default_config(&config);
    config.clock_stretch = NEON_I2C_CLOCK_STRETCH_ENABLE;
    config.fifo.tx_empty_level = 15U;
    config.fifo.rx_full_level = 0U; /* Request RX as soon as one byte is available. */
    neon_status_t status = neon_i2c_init(&i2c, NEON_I2C0);
    if (status != NEON_OK) {
        printf("[FAIL] I2C0 handle status=%lu.\n", (unsigned long)status);
        return;
    }
    status = neon_i2c_set_config(&i2c, &config);
    if (status != NEON_OK) {
        printf("[FAIL] I2C0 config status=%lu.\n", (unsigned long)status);
        neon_i2c_deinit(&i2c);
        return;
    }
    printf("[I2C DMA] I2C0 master, target SCL=100 kHz, AT24C256 address=%02X.\n", DEVICE_ADDRESS);
    printf("[I2C DMA] DMA1: TX channel 0, RX channel 1, request slot 4, SINGLE mode.\n");
    printf("[PINS] SDA=PMOD_IO6 SCL=PMOD_IO2; pull-ups, common GND, WP LOW.\n");
    printf("[WRITE] Overwrite EEPROM 0000..000F with 00 01 ... 0F; no restore.\n");
    for (uint32_t i = 0U; i < DATA_SIZE; ++i) {
        tx[i] = i;
        rx[i] = 0xA5U;
    }

    const char *stage = "DMA TX";
    status = transfer(tx, NEON_I2C_DMA_TX);
    if (status != NEON_OK) { goto failed; }
    write_delay();
    printf("[DMA TX] 16 bytes sent; EEPROM write delay complete.\n");

    stage = "EEPROM read pointer";
    /* The polling byte API expects packed bytes: this word sends 00 00. */
    uint32_t address = 0U;
    status = neon_i2c_master_transmit(&i2c, DEVICE_ADDRESS, (const uint8_t *)&address, 2U, TIMEOUT);
    if (status != NEON_OK) { goto failed; }
    stage = "DMA RX";
    status = transfer(rx, NEON_I2C_DMA_RX);
    if (status != NEON_OK) { goto failed; }

    printf("[DMA RX]");
    uint32_t matched = 0U;
    for (uint32_t i = 0U; i < DATA_SIZE; ++i) {
        printf(" %02lX", (unsigned long)(rx[i] & 0xFFU));
        if ((rx[i] & 0xFFU) == tx[i]) { ++matched; }
    }
    printf("\n[VERIFY] %lu/16 bytes matched %s\n", (unsigned long)matched,
        matched == DATA_SIZE ? "[PASS]" : "[FAIL]");
    goto cleanup;

failed:
    printf("[FAIL] %s status=%lu ISR=%08lX IITR=%08lX error=%08lX.\n", stage,
        (unsigned long)status, (unsigned long)I2C0->ISR, (unsigned long)I2C0->IITR,
        (unsigned long)neon_i2c_get_error(&i2c));
    printf("[INFO] Check address, wiring, pull-ups, WP and the DMA1 bitstream route.\n");
cleanup:
    neon_i2c_configure_dma(&i2c, NEON_I2C_DMA_TX, 0U);
    neon_i2c_configure_dma(&i2c, NEON_I2C_DMA_RX, 0U);
    if (status != NEON_OK && stop_bus() != NEON_OK) { reset_required = 1U; }
    if (reset_required != 0U) {
        /* Disable parks new service; it does not abort accepted AXI traffic. */
        DMA1->CH0_ENABLE = 0U;
        DMA1->CH1_ENABLE = 0U;
        __DSB();
        printf("[FAIL] Transfer incomplete; buffers retained. Reset FPGA before retry.\n");
        return;
    }
    if (neon_i2c_deinit(&i2c) != NEON_OK || neon_dma_deinit(DMA1) != NEON_OK) {
        reset_required = 1U;
        printf("[FAIL] Peripheral cleanup incomplete. Reset FPGA before retry.\n");
    }
}
