/**
 * @file fx1_csi_simulation.h
 * @brief Test hooks of the in-RAM CSI-2 receiver backend.
 * @details
 * Models the ping-pong DMA of MAS V1.1 chapter 9: two buffers that are hardware owned until a frame
 * completes, the error policies, frame drops while both sides are held by software, the per-capture
 * context and the sticky IRQ_STATUS bits. No memory is written: the sim only tracks ownership and
 * context, so a consumer that needs pixels fills the DDR area itself. Not part of the production API.
 */

#ifndef __FX1_CSI_SIMULATION_H__
#define __FX1_CSI_SIMULATION_H__

/*==================================================================================================
*                                          INCLUDE FILES                                          
==================================================================================================*/
#include "csi/neon_csi.h"
#ifdef __cplusplus
extern "C" {
#endif

/*==================================================================================================
*                                  STRUCTURES AND OTHER TYPEDEFS                                  
==================================================================================================*/
/** One frame arriving from the link. A zero-initialised value is a good frame on VC0 at time zero. */
typedef struct {
    uint64_t timestamp_ns;
    bool fault;                  /* The frame is damaged (buffer error); the error policy decides what happens. */
    uint32_t last_valid_line;    /* Lines written before the fault. */
    uint32_t extra_events;       /* NEON_CSI_EVENT_* raised together with the frame. */
    uint32_t ecc_errors;
    uint32_t crc_errors;
    uint8_t vc_mask;             /* Bit n: VC n carried the frame. 0 means VC0. */
    uint32_t extracted[NEON_CSI_MAX_VCS][NEON_CSI_MAX_EXTRACT_SLOTS];
} fx1_csi_sim_frame_t;

typedef struct {
    uint32_t starts;
    uint32_t stops;
    uint32_t soft_resets;
    uint32_t configures;
} fx1_csi_sim_counters_t;

/*==================================================================================================
*                                       FUNCTION PROTOTYPES                                       
==================================================================================================*/
/**
 * @brief The DMA completes one frame, or drops it.
 * @param[in,out] handle Receiver handle.
 * @param[in] frame Frame description.
 * @return NEON_OK when a buffer completed, or when a faulted frame was rewound by the discard policy;
 * NEON_BUSY when both buffers were software owned and the frame was dropped; NEON_ERROR when the
 * receiver is not running; NEON_INVALID_PARAM for a NULL pointer.
 * @note The 14-bit frame number advances for every frame, dropped ones included.
 */
neon_status_t fx1_csi_sim_push_frame(neon_csi_handle_t *handle, const fx1_csi_sim_frame_t *frame);
/** Set the next 14-bit frame number, to exercise wrap-around. */
void fx1_csi_sim_set_next_frame_id(neon_csi_handle_t *handle, uint32_t frame_id);
/** Raise sticky events as the hardware would. */
void fx1_csi_sim_raise_events(neon_csi_handle_t *handle, uint32_t events);
/** Queue a generic short packet for neon_csi_read_short_packet(). One packet is kept. */
void fx1_csi_sim_push_short_packet(neon_csi_handle_t *handle, const neon_csi_short_packet_t *packet);
/** Make neon_csi_hw_stop() fail, to exercise the retry path. */
void fx1_csi_sim_set_stop_fails(neon_csi_handle_t *handle, bool fails);
void fx1_csi_sim_get_counters(const neon_csi_handle_t *handle, fx1_csi_sim_counters_t *counters);
/** Record id that the next capture is stamped with. */
uint32_t fx1_csi_sim_get_record_id(const neon_csi_handle_t *handle);
/** Event mask last written to the backend. */
uint32_t fx1_csi_sim_get_event_mask(const neon_csi_handle_t *handle);

#ifdef __cplusplus
}
#endif

#endif /* __FX1_CSI_SIMULATION_H__ */
