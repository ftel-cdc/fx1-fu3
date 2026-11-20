/**
 * @file neon_phy_fx1.h
 * @brief PHYs of FX1 and their limits.
 * @details
 * "csi-dphy0": the MIPI D-PHY receiver under the CSI-2 controller, IP ftel_dphy_rx, release R1 (HAS draft
 * v0.18, repository ftel-cdc/fu3_dphy, branch dev). It has no PDF document: the HAS is doc/spec.md there.
 *
 * Notes for FX1 clients of csi-dphy0:
 * - The reset, the lane enables, Stop State and active_lanes of this D-PHY are registers of the CSI-2
 *   controller (PHY_CONFIG, PHY_STATUS, LANE_STATUS, PROTOCOL_CONFIG); neon_csi drives them around the
 *   neon_phy calls. The D-PHY itself holds only its timers, the lane map and its error flags.
 * - Mode MIPI D-PHY, submode RX only; neon_phy_set_mode() accepts nothing else.
 * - neon_phy_configure() only while powered off (NEON_BUSY otherwise), that is while PHY_CONFIG.cl_enable and
 *   dl_enable are 0: the D-PHY reads its timers without synchronization (HAS §4.1). neon_phy_power_on()
 *   needs a configuration first (NEON_ERROR otherwise) and comes before the lanes are enabled.
 * - Line rate 80 to 1500 Mb/s. hs_settle_cyc and eot_skip_bytes depend on it (HAS Appendix B): a value
 *   computed for another rate loses the leader or cuts payload bytes.
 * - Continuous clock only (HAS §4.6). After the last burst the D-PHY needs 288 UI of clock to drain its EoT
 *   delay line (open item S12): with the TCLK-POST minimum the last packet stays in the D-PHY. neon_csi
 *   stops at once and never waits for that packet (decision of 2026-11-20). The tail is flushed by the
 *   D-PHY reset of the next neon_csi_configure(); a start() straight after stop() may instead receive it
 *   when the clock runs again, as one Frame End without its Frame Start.
 * - The D-PHY has no register block yet (OPEN K3): its timer inputs are tied at integration, to the values
 *   of 1500 Mb/s. The driver writes registers only when FX1_CSIDPHY_BASE is defined, with the software
 *   side's draft map (FX1_DPHY_RX_CSR_DRAFT_v0.1). Without it the settings are validated and nothing is
 *   written, and neon_phy_take_status() reports no error.
 * - Errors: sot_error_mask, sot_sync_error_mask and escape_error_mask of the PPI, plus the vendor bits
 *   below. control_error_mask is not reported (no status register in the IP), escape_sync_error_mask
 *   never (no Low-Power Data Transmission in R1).
 */

#ifndef __NEON_PHY_FX1_H__
#define __NEON_PHY_FX1_H__

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include "phy/neon_phy.h"
#ifdef __cplusplus
extern "C" {
#endif

/*==================================================================================================
*                                        DEFINES AND MACROS
==================================================================================================*/
/* Names for neon_phy_get(). */
#define NEON_PHY_FX1_CSI_DPHY0              "csi-dphy0"   /* MIPI D-PHY RX of the CSI-2 controller. */

/* Limits of csi-dphy0, release R1 (HAS v0.18 Table 1-3, Table 3-1). */
#define NEON_PHY_FX1_CSIDPHY_LANES          4U            /* Data lane slices built (PARA_NUM_LANES). */
#define NEON_PHY_FX1_CSIDPHY_MIN_RATE_BPS   80000000UL    /* Lowest line rate of D-PHY v1.1. */
#define NEON_PHY_FX1_CSIDPHY_MAX_RATE_BPS   1500000000UL  /* R1; 1792 Mb/s needs the deskew of R2. */

/* Bits of neon_phy_mipi_dphy_status_t.vendor for csi-dphy0. A per-lane field holds bit n = logical lane n. */
#define NEON_PHY_FX1_CSIDPHY_EOT_SYNC_SHIFT       0U          /* [3:0]   a burst did not end on a byte boundary; its last byte was dropped. */
#define NEON_PHY_FX1_CSIDPHY_HS_RX_TIMEOUT_SHIFT  4U          /* [7:4]   a burst lasted longer than hs_rx_timeout_ns and was cut. */
#define NEON_PHY_FX1_CSIDPHY_ESC_TIMEOUT_SHIFT    8U          /* [11:8]  an escape-mode sequence did not complete. */
#define NEON_PHY_FX1_CSIDPHY_LANE_FIELD_MASK      0xFUL       /* Width of one per-lane field. */
#define NEON_PHY_FX1_CSIDPHY_CLOCK_ABORT          (1UL << 12) /* [12]    the clock lane left a request unfinished. */

/*==================================================================================================
*                                  STRUCTURES AND OTHER TYPEDEFS
==================================================================================================*/
/** Timer values of csi-dphy0 (HAS Table 3-2), in core_clk cycles of 5 ns unless stated. */
typedef struct {
    uint8_t hs_settle_cyc;                     /* Length of DL_HS_TERM; places the start of the sync search after THS-SETTLE. */
    uint8_t eot_skip_bytes;                    /* Bytes discarded before the last-transition search at EoT. */
    uint8_t clk_settle_cyc;                    /* Minimum length of CL_HS_TERM, the centre of TCLK-SETTLE. */
    uint16_t tinit_cyc;                        /* LP-11 time a lane needs before it leaves its INIT state. */
    uint32_t hs_rx_timeout_cyc;                /* Longest time in DL_HS_RX before the burst is cut. */
    uint32_t esc_timeout_cyc;                  /* Escape-mode watchdog. */
} neon_phy_fx1_csidphy_timing_t;

/*==================================================================================================
*                                       FUNCTION PROTOTYPES
==================================================================================================*/
/**
 * @brief Timer values that csi-dphy0 is programmed with for these settings. Needs no hardware.
 * @param[in] opts D-PHY settings; only lane_rate_bps and hs_rx_timeout_ns are used.
 * @param[out] timing Timer values.
 * @return NEON_OK, NEON_INVALID_PARAM for a NULL pointer or a timeout the field cannot hold, or
 * NEON_NOT_SUPPORTED for a line rate outside 80 to 1500 Mb/s.
 */
neon_status_t neon_phy_fx1_csidphy_calc_timing(const neon_phy_mipi_dphy_opts_t *opts,
                                               neon_phy_fx1_csidphy_timing_t *timing);

#ifdef __cplusplus
}
#endif

#endif /* __NEON_PHY_FX1_H__ */
