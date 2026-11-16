/**
 * @file neon_csi_events.h
 * @brief Event bits reported by the CSI-2 receiver.
 * @details
 * Kept apart from neon_csi.h so that the main header holds only the contract of the calls. Callers need
 * not include this file themselves: neon_csi.h does. The bits are a logical numbering owned by the HAL;
 * which of them a chip can raise is neon_csi_caps_t.supported_events.
 */

#ifndef __NEON_CSI_EVENTS_H__
#define __NEON_CSI_EVENTS_H__

#ifdef __cplusplus
extern "C" {
#endif

/*==================================================================================================
*                                        DEFINES AND MACROS                                       
==================================================================================================*/
/*
 * Event bits, numbered by this HAL and not by any chip's interrupt register. A chip may never raise some
 * of them: neon_csi_caps_t.supported_events says which can occur, and a backend translates its own
 * status register to these bits explicitly. FX1 happens to use the IRQ_STATUS positions 0 to 23
 * (MAS 6.13); verify them against the register workbook before the register backend is written.
 */
#define NEON_CSI_EVENT_FRAME_ERROR_VC0 (1UL << 0)
#define NEON_CSI_EVENT_FRAME_SYNC_VC0 (1UL << 1)
#define NEON_CSI_EVENT_FRAME_ERROR_VC1 (1UL << 2)
#define NEON_CSI_EVENT_FRAME_SYNC_VC1 (1UL << 3)
#define NEON_CSI_EVENT_CRC_ERROR (1UL << 4)
#define NEON_CSI_EVENT_ECC_CORRECTED (1UL << 5)
#define NEON_CSI_EVENT_ECC_UNCORRECTABLE (1UL << 6)
#define NEON_CSI_EVENT_UNSUPPORTED_DT (1UL << 7)
#define NEON_CSI_EVENT_WC_CORRUPTION (1UL << 8)
#define NEON_CSI_EVENT_SOT_ERROR (1UL << 9)
#define NEON_CSI_EVENT_SOT_SYNC_ERROR (1UL << 10)
#define NEON_CSI_EVENT_LANE_STOP_STATE (1UL << 11)
#define NEON_CSI_EVENT_LINE_BUFFER_FULL (1UL << 12)
#define NEON_CSI_EVENT_LINE_BUFFER_OVERFLOW (1UL << 13)
#define NEON_CSI_EVENT_FRAME_DROPPED (1UL << 14)
#define NEON_CSI_EVENT_FRAME_RECEIVED (1UL << 15)
#define NEON_CSI_EVENT_BUFFER_VALID (1UL << 16)
#define NEON_CSI_EVENT_BUFFER_ERROR (1UL << 17)
#define NEON_CSI_EVENT_AXI_ERROR (1UL << 18)
#define NEON_CSI_EVENT_LANE_ABORT (1UL << 19)
#define NEON_CSI_EVENT_RESOLUTION_MISMATCH (1UL << 20)
#define NEON_CSI_EVENT_WC_ALIGN_ERROR (1UL << 21)
#define NEON_CSI_EVENT_FORMAT_BRIDGE_OVERFLOW (1UL << 22)
#define NEON_CSI_EVENT_FRAME_NUMBER_ERROR (1UL << 23)
/* Raised by the HAL on every chip, not by hardware: the link was reset while this receiver was running. */
#define NEON_CSI_EVENT_LINK_RESET (1UL << 31)

#ifdef __cplusplus
}
#endif

#endif /* __NEON_CSI_EVENTS_H__ */
