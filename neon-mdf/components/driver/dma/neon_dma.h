/**
 * @file neon_dma.h
 * @brief Public declarations for DMA.
 * @details
 * GT32U2086 FTEL DMA: 8 channels, command registers at a 0x100-byte stride.
 * @note Blocking polling/configuration operations are intended for task context.
 * Serialize configuration/teardown with other users of the same peripheral;
 * timeout_polls values are polling budgets, not milliseconds, where applicable.
 */

#ifndef __NEON_DMA_H__
#define __NEON_DMA_H__

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include "neon_types.h"
#include "mcu_device.h"
#ifdef __cplusplus
extern "C" {
#endif

/*==================================================================================================
*                                        DEFINES AND MACROS
==================================================================================================*/
#define NEON_DMA_CONFIG_M2M_WORD 0x84010004UL
#define NEON_DMA_CONFIG_FIXED_WORD 0x04010004UL
#define NEON_DMA_CONTROL_M2M 0x3UL
#define NEON_DMA_MAX_TRANSFER_SIZE 1020UL

/*==================================================================================================
*                                  STRUCTURES AND OTHER TYPEDEFS
==================================================================================================*/
typedef struct {
    uint32_t src_addr;
    uint32_t dst_addr;
    uint32_t transfer_size; /* Bytes, divisible by four, 4..1020. */
    uint32_t channel;       /* 0..7. */
    uint32_t read_config;   /* Burst size in bytes must be divisible by four. */
    uint32_t write_config;
    uint32_t control;
    uint32_t peripheral_config;
} neon_dma_config_t;

/*==================================================================================================
*                                       FUNCTION PROTOTYPES
==================================================================================================*/
/**
 * @brief Deinitialize DMA.
 * @param[in] instance Peripheral register instance.
 * @return NEON_OK on success, or a module error/status code.
 */
neon_status_t neon_dma_deinit(DMA_TypeDef *instance);
/**
 * @brief Start DMA channel.
 * @param[in] instance Peripheral register instance.
 * @param[in] config Module configuration.
 * @return NEON_OK on success, or a module error/status code.
 */
neon_status_t neon_dma_start_channel(DMA_TypeDef *instance, const neon_dma_config_t *config);
/**
 * @brief Dma wait channel.
 * @param[in] instance Peripheral register instance.
 * @param[in] channel channel.
 * @param[in] timeout_cycles timeout_polls cycles.
 * @return NEON_OK on success, or a module error/status code.
 * @details
 * Wait for CMD_DONE; timeout_cycles=0 waits indefinitely.
 */
neon_status_t neon_dma_wait_channel(DMA_TypeDef *instance, uint8_t channel, uint32_t timeout_cycles);
/**
 * @brief Check completion without waiting.
 * @return NEON_OK when done, idle and no accepted AXI transactions remain;
 * NEON_BUSY while pending, or an error.
 * @details Bus/FIFO/AXI timeout errors fail the transfer. The no-progress
 * watchdog is advisory for peripheral-paced commands, which may legitimately
 * wait for UART characters longer than its window; it remains fatal for M2M.
 */
neon_status_t neon_dma_poll_channel(DMA_TypeDef *instance, uint8_t channel);
/**
 * @brief Dma fire channel.
 * @param[in] instance Peripheral register instance.
 * @param[in] channel channel.
 * @return NEON_OK on success, or a module error/status code.
 * @details
 * Start a previously configured native channel. No PL330 microprogram is used.
 */
neon_status_t neon_dma_fire_channel(DMA_TypeDef *instance, uint8_t channel);
/**
 * @brief Dma mem cpy.
 * @param[in] instance Peripheral register instance.
 * @param[in] channel channel.
 * @param[in] dst Dst.
 * @param[in] src Src.
 * @param[in] byte_count Byte count.
 * @param[in] timeout_cycles timeout_polls cycles.
 * @return NEON_OK on success, or a module error/status code.
 * @details
 * Aligned word copy, byte_count divisible by four and <=1020. Clock/reset
 * must be enabled/released through CRM before calling.
 * timeout_cycles is a poll budget; zero explicitly waits indefinitely.
 */
neon_status_t neon_dma_memcpy(DMA_TypeDef *instance, uint8_t channel, uint32_t dst, uint32_t src, uint32_t byte_count,
                              uint32_t timeout_cycles);
#ifdef __cplusplus
}
#endif
#endif /* __NEON_DMA_H__ */
