/**
 * @file neon_csi_hw.h
 * @brief @internal Register-level contract of the CSI backends.
 * @details
 * neon_csi.c holds every rule of the module, including validation, ownership bookkeeping and the link
 * epoch checks, and is the same for every chip. A chip supplies these functions in exactly one backend: the
 * in-RAM simulation (fx1_csi_simulation.c) or its register backend (the FX1 one needs the final register
 * workbook and the DMA and metadata register definitions, and does not exist yet), plus its limits through
 * neon_csi_hw_get_caps(). Applications must not include this file.
 */

#ifndef __NEON_CSI_HW_H__
#define __NEON_CSI_HW_H__

/*==================================================================================================
*                                          INCLUDE FILES                                          
==================================================================================================*/
#include "csi/neon_csi.h"
#ifdef __cplusplus
extern "C" {
#endif

/*==================================================================================================
*                                       FUNCTION PROTOTYPES                                       
==================================================================================================*/
/** Limits of the chip; FX1 defines it in fx1_csi_caps.c, shared by its backends. */
neon_status_t neon_csi_hw_get_caps(neon_csi_caps_t *caps);
/** The handle holds the caps already. */
neon_status_t neon_csi_hw_init(neon_csi_handle_t *handle);
/** Program PROTOCOL, STREAM, DMA, metadata and test pattern registers. Leaves the core disabled. */
neon_status_t neon_csi_hw_configure(neon_csi_handle_t *handle, const neon_csi_config_t *config, const neon_csi_layout_t *layout);
/** Arm both sides as hardware owned, then dma_enable and dma_start, then core_enable. */
neon_status_t neon_csi_hw_start(neon_csi_handle_t *handle);
/** dma_abort, wait for the drain, then clear core_enable. */
neon_status_t neon_csi_hw_stop(neon_csi_handle_t *handle);
/**
 * CORE_CONFIG.soft_reset, then poll CORE_STATUS.reset_in_progress. Afterwards release both buffers
 * explicitly, so that both are hardware owned whatever the reset does to the ownership register.
 */
neon_status_t neon_csi_hw_soft_reset(neon_csi_handle_t *handle);
neon_status_t neon_csi_hw_read_events(neon_csi_handle_t *handle, uint32_t *events);
/** Write one to the given IRQ_STATUS bits. */
neon_status_t neon_csi_hw_clear_events(neon_csi_handle_t *handle, uint32_t events);
neon_status_t neon_csi_hw_set_event_mask(neon_csi_handle_t *handle, uint32_t event_mask);
/** BUFFER_OWNERSHIP. */
neon_status_t neon_csi_hw_read_buffer_state(neon_csi_handle_t *handle, neon_csi_buffer_state_t state[NEON_CSI_MAX_BUFFERS]);
/**
 * Frame context of one software-owned buffer: frame_id, timestamp, record_id, per-VC info, extracted
 * values and last_valid_line. Sets frame->context_valid. The caller fills buffer_index, state, layout
 * and addresses.
 */
neon_status_t neon_csi_hw_read_frame(neon_csi_handle_t *handle, uint8_t buffer_index, neon_csi_frame_t *frame);
/** BUFFER_RELEASE. */
neon_status_t neon_csi_hw_release_buffer(neon_csi_handle_t *handle, uint8_t buffer_index);
/** Counters and Control FSM state of the status; leaves state, buffer_state and irq_pending alone. */
neon_status_t neon_csi_hw_read_counters(neon_csi_handle_t *handle, neon_csi_status_t *status);
neon_status_t neon_csi_hw_set_record_id(neon_csi_handle_t *handle, uint32_t record_id);
neon_status_t neon_csi_hw_read_short_packet(neon_csi_handle_t *handle, neon_csi_short_packet_t *packet);

#ifdef __cplusplus
}
#endif

#endif /* __NEON_CSI_HW_H__ */
