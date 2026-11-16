/**
 * @file fx1_csi_caps.c
 * @brief What the FX1 CSI-2 controller provides (MAS V1.1). Facts of the IP, shared by every FX1 backend.
 */

/*==================================================================================================
*                                          INCLUDE FILES                                          
==================================================================================================*/
#include "csi/neon_csi.h"
#include "csi/platform/common/neon_csi_hw.h"

/*==================================================================================================
*                                        DEFINES AND MACROS                                       
==================================================================================================*/
#define FX1_CSI_BUFFER_COUNT 2U            /* Ping and pong (MAS 6.20). */
#define FX1_CSI_VC_COUNT 2U                /* VC0 and VC1; other virtual channels are dropped. */
#define FX1_CSI_EXTRACT_SLOTS 2U           /* N_EXTRACT. */
#define FX1_CSI_FRAME_ID_BITS 14U
#define FX1_CSI_MAX_OUTSTANDING 127U
#define FX1_CSI_MAX_QOS 15U
#define FX1_CSI_MAX_PROT 7U
#define FX1_CSI_MAX_BURST_BEATS 256U
#define FX1_CSI_MAX_LINE_PADDING_BYTES 255U
#define FX1_CSI_MIN_TPG_DIMENSION 8U
#define FX1_CSI_TPG_WIDTH_GRANULARITY 4U
#define FX1_CSI_MAX_BYTES_PER_LINE 0xFFFFUL /* LINE_PITCH.bytes_per_line is 16 bits wide. */
#define FX1_CSI_META_RECORD_MIN_BYTES 128UL
/* IRQ_STATUS bits 0 to 23 map one to one onto NEON_CSI_EVENT_* bits 0 to 23. */
#define FX1_CSI_EVENTS 0x00FFFFFFUL
/* The ISP IDMA needs base and stride in multiples of its AXI beat, 16 bytes by default (ISP MAS table 154). */
#ifndef FX1_CSI_DMA_ALIGN_BYTES
#define FX1_CSI_DMA_ALIGN_BYTES 16UL
#endif

_Static_assert(FX1_CSI_BUFFER_COUNT <= NEON_CSI_MAX_BUFFERS, "raise NEON_CSI_MAX_BUFFERS");
_Static_assert(FX1_CSI_VC_COUNT <= NEON_CSI_MAX_VCS, "raise NEON_CSI_MAX_VCS");
_Static_assert(FX1_CSI_EXTRACT_SLOTS <= NEON_CSI_MAX_EXTRACT_SLOTS, "raise NEON_CSI_MAX_EXTRACT_SLOTS");

/*==================================================================================================
*                                         GLOBAL FUNCTIONS                                        
==================================================================================================*/
neon_status_t neon_csi_hw_get_caps(neon_csi_caps_t *caps)
{
    caps->buffer_count = FX1_CSI_BUFFER_COUNT;
    caps->vc_count = FX1_CSI_VC_COUNT;
    caps->extract_slots = FX1_CSI_EXTRACT_SLOTS;
    caps->frame_id_bits = FX1_CSI_FRAME_ID_BITS;
    caps->max_outstanding = FX1_CSI_MAX_OUTSTANDING;
    caps->max_qos = FX1_CSI_MAX_QOS;
    caps->max_prot = FX1_CSI_MAX_PROT;
    caps->max_burst_beats = FX1_CSI_MAX_BURST_BEATS;
    caps->max_line_padding_bytes = FX1_CSI_MAX_LINE_PADDING_BYTES;
    caps->min_tpg_dimension = FX1_CSI_MIN_TPG_DIMENSION;
    caps->tpg_width_granularity = FX1_CSI_TPG_WIDTH_GRANULARITY;
    caps->test_pattern = true;
    caps->max_bytes_per_line = FX1_CSI_MAX_BYTES_PER_LINE;
    caps->dma_align_bytes = FX1_CSI_DMA_ALIGN_BYTES;
    caps->meta_record_min_bytes = FX1_CSI_META_RECORD_MIN_BYTES;
    caps->supported_events = FX1_CSI_EVENTS;
    return NEON_OK;
}
