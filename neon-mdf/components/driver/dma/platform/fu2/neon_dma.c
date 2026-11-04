/**
 * @file neon_dma.c
 * @brief Implementation for DMA.
 * @details
 * Implements module behavior and target-specific integration.
 */

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include "dma/neon_dma.h"

/*==================================================================================================
*                                        DEFINES AND MACROS
==================================================================================================*/
#define DMA_CHANNEL_STRIDE 0x100UL
#define DMA_INTERRUPT_MASK 0x1FFFUL

/*==================================================================================================
*                                   PRIVATE FUNCTION PROTOTYPES
==================================================================================================*/

static uint32_t dma_valid_instance(DMA_TypeDef *instance);
static volatile uint32_t *dma_channel_reg(DMA_TypeDef *instance, uint32_t channel, uint32_t offset);

/*==================================================================================================
*                                         GLOBAL FUNCTIONS
==================================================================================================*/

/** @copydoc neon_dma_deinit */
neon_status_t neon_dma_deinit(DMA_TypeDef *instance) {
    if (!dma_valid_instance(instance)) {
        return NEON_INVALID_PARAM;
    }
    /* A disabled active channel is parked, not aborted: require completion first. */
    for (uint32_t channel = 0U; channel < 8U; ++channel) {
        if (*dma_channel_reg(instance, channel, 0x48U) != 0U) {
            return NEON_BUSY;
        }
    }
    for (uint32_t channel = 0U; channel < 8U; ++channel) {
        *dma_channel_reg(instance, channel, 0x40U) = 0U;
        *dma_channel_reg(instance, channel, 0xA8U) = 0U;
        *dma_channel_reg(instance, channel, 0xA4U) = DMA_INTERRUPT_MASK;
    }
    return NEON_OK;
}

/** @copydoc neon_dma_fire_channel */
neon_status_t neon_dma_fire_channel(DMA_TypeDef *instance, uint8_t channel) {
    if (!dma_valid_instance(instance) || channel > 7U) {
        return NEON_INVALID_PARAM;
    }
    if (*dma_channel_reg(instance, channel, 0x48U) != 0U) {
        return NEON_BUSY;
    }
    *dma_channel_reg(instance, channel, 0xA4U) = DMA_INTERRUPT_MASK;
    *dma_channel_reg(instance, channel, 0x40U) = 1U;
    __DSB();
    *dma_channel_reg(instance, channel, 0x44U) = 1U;
    return NEON_OK;
}

/** @copydoc neon_dma_start_channel */
neon_status_t neon_dma_start_channel(DMA_TypeDef *instance, const neon_dma_config_t *config) {
    if (!dma_valid_instance(instance) || config == 0 || config->channel > 7U || config->transfer_size == 0U ||
        config->transfer_size > NEON_DMA_MAX_TRANSFER_SIZE ||
        ((config->src_addr | config->dst_addr | config->transfer_size | config->read_config | config->write_config) &
         3U) != 0U ||
        (config->read_config & 0x7FU) == 0U || (config->write_config & 0x7FU) == 0U) {
        return NEON_INVALID_PARAM;
    }
    uint32_t channel = config->channel;
    if (*dma_channel_reg(instance, channel, 0x48U) != 0U) {
        return NEON_BUSY;
    }
    *dma_channel_reg(instance, channel, 0x40U) = 0U;
    *dma_channel_reg(instance, channel, 0xA8U) = 0U;
    *dma_channel_reg(instance, channel, 0x00U) = config->src_addr;
    *dma_channel_reg(instance, channel, 0x04U) = config->dst_addr;
    *dma_channel_reg(instance, channel, 0x08U) = config->transfer_size;
    *dma_channel_reg(instance, channel, 0x0CU) = config->control;
    *dma_channel_reg(instance, channel, 0x10U) = config->read_config;
    *dma_channel_reg(instance, channel, 0x14U) = config->write_config;
    *dma_channel_reg(instance, channel, 0x18U) = 0U;
    *dma_channel_reg(instance, channel, 0x20U) = config->peripheral_config;
    return neon_dma_fire_channel(instance, (uint8_t)channel);
}

/** @copydoc neon_dma_wait_channel */
neon_status_t neon_dma_wait_channel(DMA_TypeDef *instance, uint8_t channel, uint32_t timeout_cycles) {
    if (!dma_valid_instance(instance) || channel > 7U) {
        return NEON_INVALID_PARAM;
    }
    uint32_t remaining = timeout_cycles;
    do {
        uint32_t status = *dma_channel_reg(instance, channel, 0xA0U);
        if ((status & (DMA_INTERRUPT_MASK & ~1UL)) != 0U) {
            return NEON_ERROR;
        }
        if ((status & 1U) != 0U) {
            return NEON_OK;
        }
    } while (timeout_cycles == 0U || --remaining != 0U);
    return NEON_TIMEOUT;
}

/** @copydoc neon_dma_memcpy */
neon_status_t neon_dma_memcpy(DMA_TypeDef *instance, uint8_t channel, uint32_t dst, uint32_t src, uint32_t byte_count,
                              uint32_t timeout_cycles) {
    if (((src | dst | byte_count) & 3U) != 0U) {
        return NEON_INVALID_PARAM;
    }
    neon_dma_config_t config = {
        .src_addr = src,
        .dst_addr = dst,
        .transfer_size = byte_count,
        .channel = channel,
        .read_config = NEON_DMA_CONFIG_M2M_WORD,
        .write_config = NEON_DMA_CONFIG_M2M_WORD,
        .control = NEON_DMA_CONTROL_M2M,
        .peripheral_config = 0U,
    };
    neon_status_t status = neon_dma_start_channel(instance, &config);
    return status == NEON_OK ? neon_dma_wait_channel(instance, channel, timeout_cycles) : status;
}

/*==================================================================================================
*                                        PRIVATE FUNCTIONS
==================================================================================================*/

static uint32_t dma_valid_instance(DMA_TypeDef *instance) {
    return instance == DMA0 || instance == DMA1;
}

static volatile uint32_t *dma_channel_reg(DMA_TypeDef *instance, uint32_t channel, uint32_t offset) {
    return (volatile uint32_t *)((uintptr_t)instance + channel * DMA_CHANNEL_STRIDE + offset);
}
