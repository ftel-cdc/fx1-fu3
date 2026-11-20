/**
 * @file fx1_csidphy_regs.h
 * @brief @internal Registers of csi-dphy0, from FX1_DPHY_RX_CSR_DRAFT_v0.1 (fx1/DOCS).
 * @details
 * DRAFT: the IP has no register block yet (Block No. 8 absent, rtl_notes D-2; configuration path OPEN
 * K3 / ADR-005). The map is the software side's proposal: an APB subordinate of its own, 256 B. Offsets
 * and fields change with the answer of the D-PHY team; the base address is not assigned, so
 * FX1_CSIDPHY_BASE has no default and the instance base is 0 (no access) until it is given.
 */

#ifndef __FX1_CSIDPHY_REGS_H__
#define __FX1_CSIDPHY_REGS_H__

/*==================================================================================================
*                                        DEFINES AND MACROS
==================================================================================================*/
/* IP_VERSION, 0x00 (RO): HAS version major [23:16], minor [15:8], patch [7:0]. */
#define FX1_CSIDPHY_IP_VERSION_OFFSET           0x00U
#define FX1_CSIDPHY_IP_VERSION_EXPECTED         0x00001200U /* HAS draft v0.18. */

/* IP_PARAM, 0x04 (RO): synthesis parameters. */
#define FX1_CSIDPHY_IP_PARAM_OFFSET             0x04U
#define FX1_CSIDPHY_IP_PARAM_NUM_LANES_MASK     0x00000007U /* [2:0] PARA_NUM_LANES. */

/* TIMING_HS, 0x10 (RW): hs_settle_cyc [7:0], eot_skip_bytes [13:8]. */
#define FX1_CSIDPHY_TIMING_HS_OFFSET            0x10U
#define FX1_CSIDPHY_TIMING_HS_SETTLE_SHIFT      0U
#define FX1_CSIDPHY_TIMING_HS_SETTLE_MASK       0x000000FFU
#define FX1_CSIDPHY_TIMING_HS_EOT_SKIP_SHIFT    8U
#define FX1_CSIDPHY_TIMING_HS_EOT_SKIP_MASK     0x00003F00U

/* TIMING_CLK, 0x14 (RW): clk_settle_cyc [7:0]. */
#define FX1_CSIDPHY_TIMING_CLK_OFFSET           0x14U
#define FX1_CSIDPHY_TIMING_CLK_SETTLE_MASK      0x000000FFU

/* TIMING_TINIT, 0x18 (RW): tinit_cyc [15:0]. */
#define FX1_CSIDPHY_TIMING_TINIT_OFFSET         0x18U
#define FX1_CSIDPHY_TIMING_TINIT_MASK           0x0000FFFFU

/* HS_RX_TIMEOUT, 0x1C (RW): hs_rx_timeout_cyc [19:0]. */
#define FX1_CSIDPHY_HS_RX_TIMEOUT_OFFSET        0x1CU
#define FX1_CSIDPHY_HS_RX_TIMEOUT_MASK          0x000FFFFFU

/* ESC_TIMEOUT, 0x20 (RW): esc_timeout_cyc [19:0]. */
#define FX1_CSIDPHY_ESC_TIMEOUT_OFFSET          0x20U
#define FX1_CSIDPHY_ESC_TIMEOUT_MASK            0x000FFFFFU

/* LANE_CFG, 0x24 (RW): laneN_map at [2N+1:2N], lane_pol [11:8]. Reset 0xE4: straight, no inversion. */
#define FX1_CSIDPHY_LANE_CFG_OFFSET             0x24U
#define FX1_CSIDPHY_LANE_CFG_MAP_SHIFT(lane)    (2U * (uint32_t)(lane))
#define FX1_CSIDPHY_LANE_CFG_MAP_FIELD_MASK     0x3U
#define FX1_CSIDPHY_LANE_CFG_POL_SHIFT          8U
#define FX1_CSIDPHY_LANE_CFG_POL_MASK           0x00000F00U

/* EVT_FLAGS, 0x50 (RW1C) and EVT_CNT_CLR, 0x54 (WO): one bit per event and logical lane. */
#define FX1_CSIDPHY_EVT_FLAGS_OFFSET            0x50U
#define FX1_CSIDPHY_EVT_CNT_CLR_OFFSET          0x54U
#define FX1_CSIDPHY_EVT_ERRSOTHS_SHIFT          0U           /* [3:0]   errsoths. */
#define FX1_CSIDPHY_EVT_ERRSOTSYNCHS_SHIFT      4U           /* [7:4]   errsotsynchs. */
#define FX1_CSIDPHY_EVT_ERREOTSYNC_SHIFT        8U           /* [11:8]  EoT sync error. */
#define FX1_CSIDPHY_EVT_HS_RX_TIMEOUT_SHIFT     12U          /* [15:12] HS RX timeout. */
#define FX1_CSIDPHY_EVT_ERRESC_SHIFT            16U          /* [19:16] erresc. */
#define FX1_CSIDPHY_EVT_ESC_TIMEOUT_SHIFT       20U          /* [23:20] escape watchdog. */
#define FX1_CSIDPHY_EVT_LANE_FIELD_MASK         0xFU        /* Width of one per-lane field. */
#define FX1_CSIDPHY_EVT_CL_ABORT_MASK           0x01000000U /* [24] aborted clock-lane request. */
#define FX1_CSIDPHY_EVT_ALL_MASK                0x01FFFFFFU

#endif /* __FX1_CSIDPHY_REGS_H__ */
