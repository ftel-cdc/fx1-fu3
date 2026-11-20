/**
 * @file fx1_csidphy.c
 * @brief Driver of csi-dphy0, the MIPI D-PHY RX of FX1 (ftel_dphy_rx, HAS draft v0.18).
 * @details
 * Notes for clients are in neon_phy_fx1.h.
 *
 * The IP has no register block yet (Block No. 8 absent, rtl_notes D-2; configuration path OPEN K3): its
 * timer inputs are tied at integration. The register base comes from the instance (phy->priv):
 * - base != 0 (FX1_CSIDPHY_BASE given at build time): the registers of fx1_csidphy_regs.h, the software
 *   side's DRAFT map, are written and read.
 * - base == 0 (today): the settings are still validated and the timer values computed, but nothing is
 *   written, and take_status() reports no error. The tied values are those of 1500 Mb/s (HAS Table 3-2):
 *   at another rate the link may lose leaders or payload bytes, which no software can fix.
 *
 * Rules of this PHY that the generic layer does not know (neon_phy_fx1.h): configure() only while powered
 * off, power_on() only once configured, mode MIPI D-PHY RX only.
 *
 * Timer formulas (HAS Appendix B, Table 3-2). They depend on how the IP is built, so they live here and not
 * in the generic D-PHY code:
 *   hs_settle_cyc  = round((115 ns + 9.5 UI + (L - 0.5) x 8 UI - T_term) / 5 ns), the centre of the window
 *                    that starts the sync search after THS-SETTLE,min and before the first 1 of the leader;
 *                    T_term = (LP_SYNC + LP_FILTER + 3) x 5 ns = 35 ns, the latest entry into DL_HS_TERM.
 *   eot_skip_bytes = max(floor((THS-TRAIL,min + T_lp11) / 8 UI) - 2, 0), one byte of margin to the payload;
 *                    THS-TRAIL,min = max(8 UI, 60 ns + 4 UI), T_lp11 = (LP_SYNC + LP_FILTER + 1) x 5 ns - 1 ns.
 * L is the word latency of Block No. 1 (2, OPEN ADR-007). Values: 80 Mb/s 70 / 0, 750 Mb/s 22 / 6,
 * 1500 Mb/s 19 / 14 (the reset values).
 *
 * Hardware traps to respect in this file (register map present):
 * - The timer fields, LANE_CFG included, are read by core_clk and rxbyteclkhs without synchronization:
 *   write them only with every lane disabled (HAS §4.1, Table 4-2). neon_phy.c refuses configure() while
 *   powered, and the client powers on only after it has disabled the lanes.
 * - The IP has no interrupt: the sticky flags of EVT_FLAGS are read by take_status().
 * - A flag raised between the read and the write-1-to-clear of take_status() stays set: only the bits read
 *   are written back.
 */

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include "phy/platform/fx1/fx1_csidphy.h"
#include "phy/platform/fx1/fx1_csidphy_regs.h"
#include "fx1_fw.h"
#include <stddef.h>

/*==================================================================================================
*                                        LOCAL MACROS
==================================================================================================*/
#define CSIDPHY_PS_PER_S            1000000000000ULL /* UI in ps = this / lane_rate_bps. */
#define CSIDPHY_CORE_CLK_PS         5000U            /* core_clk, 200 MHz (PARA_CORE_CLK_MHZ). */
#define CSIDPHY_LP_SYNC_STAGES      2U               /* PARA_LP_SYNC_STAGES. */
#define CSIDPHY_LP_FILTER_CYC       2U               /* PARA_LP_FILTER_CYC. */
#define CSIDPHY_WORD_LATENCY        2U               /* L, word latency of Block No. 1 (OPEN ADR-007). */

/* Latest entry into DL_HS_TERM after the start of LP-00, and shortest delay from LP-11 to the EoT request. */
#define CSIDPHY_T_TERM_PS           ((CSIDPHY_LP_SYNC_STAGES + CSIDPHY_LP_FILTER_CYC + 3U) * CSIDPHY_CORE_CLK_PS)
#define CSIDPHY_T_LP11_PS           (((CSIDPHY_LP_SYNC_STAGES + CSIDPHY_LP_FILTER_CYC + 1U) * CSIDPHY_CORE_CLK_PS) - 1000U)

/* Twice the UI factor of hs_settle_cyc: 2 x (9.5 + (L - 0.5) x 8) = 19 + (2L - 1) x 8, 43 with L = 2. */
#define CSIDPHY_SETTLE_UI_X2        (19U + (((2U * CSIDPHY_WORD_LATENCY) - 1U) * 8U))
#define CSIDPHY_SETTLE_CENTRE_PS    115000U          /* 115 ns: the constant part of the window centre. */
#define CSIDPHY_EOT_MARGIN_BYTES    2U               /* One byte below the value that cuts the payload, plus one. */
#define CSIDPHY_TRAIL_MIN_PS        60000U           /* THS-TRAIL,min = max(8 UI, 60 ns + 4 UI). */

/* Rate-independent fields: the reset values of HAS Table 3-2. */
#define CSIDPHY_CLK_SETTLE_CYC      33U              /* TCLK-SETTLE centre, 197.5 ns. */
#define CSIDPHY_TINIT_CYC           20000U           /* TINIT,SLAVE = 100 us. */
#define CSIDPHY_HS_RX_TIMEOUT_CYC   0x40000UL        /* 2^18 = 1.31 ms (OPEN K8). */
#define CSIDPHY_ESC_TIMEOUT_CYC     0x40000UL        /* 2^18. */

/* Field widths (config/ftel_dphy_rx.yaml). */
#define CSIDPHY_HS_SETTLE_MAX       0xFFU            /* 8 bits. */
#define CSIDPHY_EOT_SKIP_MAX        0x3FU            /* 6 bits. */
#define CSIDPHY_HS_RX_TIMEOUT_MAX   0xFFFFFUL        /* 20 bits. */

/* Register of the instance; base is that of csidphy_instance(). */
#define CSIDPHY_REG(base, name) ((base) + FX1_CSIDPHY_##name##_OFFSET)

/*==================================================================================================
*                                    LOCAL TYPEDEFS (STRUCTURES, UNIONS, ENUMS)
==================================================================================================*/
/** Driver state, in phy->backend_words (cleared by the first neon_phy_init()). */
typedef struct {
    bool configured;                           /* configure() succeeded since the first init. */
} csidphy_state_t;

_Static_assert(sizeof(csidphy_state_t) <= sizeof(((neon_phy_t *)0)->backend_words),
               "csidphy_state_t does not fit in the handle");

/*==================================================================================================
*                                         GLOBAL FUNCTIONS
==================================================================================================*/
neon_status_t neon_phy_fx1_csidphy_calc_timing(const neon_phy_mipi_dphy_opts_t *opts,
                                               neon_phy_fx1_csidphy_timing_t *timing)
{
    uint64_t rate;
    uint64_t settle_x2_ps;
    uint64_t settle;
    uint64_t trail_bits;
    uint64_t eot;
    uint64_t timeout;

    if ((opts == NULL) || (timing == NULL)) {
        return NEON_INVALID_PARAM;
    }
    if ((opts->lane_rate_bps < NEON_PHY_FX1_CSIDPHY_MIN_RATE_BPS) ||
        (opts->lane_rate_bps > NEON_PHY_FX1_CSIDPHY_MAX_RATE_BPS)) {
        return NEON_NOT_SUPPORTED;
    }
    rate = opts->lane_rate_bps;

    /* hs_settle_cyc, in twice the time to keep the half UI exact: round(x2 / (2 x 5 ns)). */
    settle_x2_ps = (2ULL * (CSIDPHY_SETTLE_CENTRE_PS - CSIDPHY_T_TERM_PS)) +
                   (((uint64_t)CSIDPHY_SETTLE_UI_X2 * CSIDPHY_PS_PER_S) / rate);
    settle = (settle_x2_ps + CSIDPHY_CORE_CLK_PS) / (2ULL * CSIDPHY_CORE_CLK_PS);

    /* eot_skip_bytes. (THS-TRAIL,min + T_lp11) x rate is in bit x 10^12; one byte is 8 x 10^12. */
    if (((uint64_t)CSIDPHY_TRAIL_MIN_PS * rate) >= (4ULL * CSIDPHY_PS_PER_S)) {
        /* 60 ns + 4 UI is the longer: the trail lasts (60 ns + T_lp11) x rate + 4 bits. */
        trail_bits = ((uint64_t)(CSIDPHY_TRAIL_MIN_PS + CSIDPHY_T_LP11_PS) * rate) + (4ULL * CSIDPHY_PS_PER_S);
    } else {
        trail_bits = ((uint64_t)CSIDPHY_T_LP11_PS * rate) + (8ULL * CSIDPHY_PS_PER_S);
    }
    eot = trail_bits / (8ULL * CSIDPHY_PS_PER_S);
    eot = (eot > CSIDPHY_EOT_MARGIN_BYTES) ? (eot - CSIDPHY_EOT_MARGIN_BYTES) : 0U;

    /* hs_rx_timeout_cyc: rounded up so that a burst of exactly hs_rx_timeout_ns is not cut. */
    timeout = CSIDPHY_HS_RX_TIMEOUT_CYC;
    if (opts->hs_rx_timeout_ns != 0U) {
        timeout = (((uint64_t)opts->hs_rx_timeout_ns * 1000ULL) + (CSIDPHY_CORE_CLK_PS - 1U)) / CSIDPHY_CORE_CLK_PS;
        if (timeout > CSIDPHY_HS_RX_TIMEOUT_MAX) {
            return NEON_INVALID_PARAM;
        }
    }

    /* Both stay far inside their fields from 80 Mb/s (70 and 0) to 1500 Mb/s (19 and 14). */
    if ((settle > CSIDPHY_HS_SETTLE_MAX) || (eot > CSIDPHY_EOT_SKIP_MAX)) {
        return NEON_NOT_SUPPORTED;
    }
    timing->hs_settle_cyc = (uint8_t)settle;
    timing->eot_skip_bytes = (uint8_t)eot;
    timing->clk_settle_cyc = (uint8_t)CSIDPHY_CLK_SETTLE_CYC;
    timing->tinit_cyc = (uint16_t)CSIDPHY_TINIT_CYC;
    timing->hs_rx_timeout_cyc = (uint32_t)timeout;
    timing->esc_timeout_cyc = (uint32_t)CSIDPHY_ESC_TIMEOUT_CYC;
    return NEON_OK;
}

/*==================================================================================================
*                                       PRIVATE FUNCTIONS
==================================================================================================*/
static csidphy_state_t *csidphy_state(neon_phy_t *phy)
{
    return (csidphy_state_t *)(void *)phy->backend_words;
}

/* Register base of the instance, 0 when the IP has no register block. */
static uintptr_t csidphy_base(const neon_phy_t *phy)
{
    const fx1_csidphy_instance_t *instance = (const fx1_csidphy_instance_t *)phy->priv;

    return (instance != NULL) ? instance->base : 0U;
}

/* One per-lane field of EVT_FLAGS, bit n = logical lane n. */
static uint8_t csidphy_lane_field(uint32_t flags, uint32_t shift)
{
    return (uint8_t)((flags >> shift) & FX1_CSIDPHY_EVT_LANE_FIELD_MASK);
}

static neon_status_t csidphy_init(neon_phy_t *phy)
{
    uintptr_t base = csidphy_base(phy);

    if (base == 0U) {
        return NEON_OK; /* No register block to check. */
    }
    /* Refuse an IP this driver was not written for, and a build with fewer lane slices. */
    if (fx1_read32(CSIDPHY_REG(base, IP_VERSION)) != FX1_CSIDPHY_IP_VERSION_EXPECTED) {
        return NEON_NOT_SUPPORTED;
    }
    if ((fx1_read32(CSIDPHY_REG(base, IP_PARAM)) & FX1_CSIDPHY_IP_PARAM_NUM_LANES_MASK) != NEON_PHY_FX1_CSIDPHY_LANES) {
        return NEON_NOT_SUPPORTED;
    }
    return NEON_OK;
}

/* The generic checks, the limits of R1 and the timer ranges. Touches no hardware. */
static neon_status_t csidphy_validate(const neon_phy_t *phy, const neon_phy_configure_opts_t *opts)
{
    neon_phy_fx1_csidphy_timing_t timing;
    neon_status_t status;

    (void)phy;
    status = neon_phy_mipi_dphy_check_opts(&opts->mipi_dphy, (uint8_t)NEON_PHY_FX1_CSIDPHY_LANES);
    if (status != NEON_OK) {
        return status;
    }
    /* The EoT delay line drains only while rxbyteclkhs runs (HAS §4.6). */
    if (!opts->mipi_dphy.continuous_clock) {
        return NEON_NOT_SUPPORTED;
    }
    return neon_phy_fx1_csidphy_calc_timing(&opts->mipi_dphy, &timing);
}

static neon_status_t csidphy_configure(neon_phy_t *phy, const neon_phy_configure_opts_t *opts)
{
    const neon_phy_mipi_dphy_opts_t *dphy = &opts->mipi_dphy;
    neon_phy_fx1_csidphy_timing_t timing;
    uintptr_t base = csidphy_base(phy);
    neon_status_t status;
    uint32_t lane_cfg;
    uint32_t lane;

    /* ATTENTION: the timers are read without synchronization (HAS §4.1): only with every lane disabled. */
    if (phy->power_count != 0U) {
        return NEON_BUSY;
    }
    status = neon_phy_fx1_csidphy_calc_timing(dphy, &timing);
    if (status != NEON_OK) {
        return status;
    }
    if (base != 0U) {
        fx1_write32(CSIDPHY_REG(base, TIMING_HS),
                    (((uint32_t)timing.hs_settle_cyc << FX1_CSIDPHY_TIMING_HS_SETTLE_SHIFT) & FX1_CSIDPHY_TIMING_HS_SETTLE_MASK) |
                        (((uint32_t)timing.eot_skip_bytes << FX1_CSIDPHY_TIMING_HS_EOT_SKIP_SHIFT) &
                         FX1_CSIDPHY_TIMING_HS_EOT_SKIP_MASK));
        fx1_write32(CSIDPHY_REG(base, TIMING_CLK), (uint32_t)timing.clk_settle_cyc & FX1_CSIDPHY_TIMING_CLK_SETTLE_MASK);
        fx1_write32(CSIDPHY_REG(base, TIMING_TINIT), (uint32_t)timing.tinit_cyc & FX1_CSIDPHY_TIMING_TINIT_MASK);
        fx1_write32(CSIDPHY_REG(base, HS_RX_TIMEOUT), timing.hs_rx_timeout_cyc & FX1_CSIDPHY_HS_RX_TIMEOUT_MASK);
        fx1_write32(CSIDPHY_REG(base, ESC_TIMEOUT), timing.esc_timeout_cyc & FX1_CSIDPHY_ESC_TIMEOUT_MASK);

        /* Every logical lane is mapped, used or not: the map is a permutation (checked by validate). */
        lane_cfg = ((uint32_t)dphy->lane_polarity_mask << FX1_CSIDPHY_LANE_CFG_POL_SHIFT) & FX1_CSIDPHY_LANE_CFG_POL_MASK;
        for (lane = 0U; lane < NEON_PHY_FX1_CSIDPHY_LANES; lane++) {
            lane_cfg |= ((uint32_t)dphy->lane_map[lane] & FX1_CSIDPHY_LANE_CFG_MAP_FIELD_MASK)
                        << FX1_CSIDPHY_LANE_CFG_MAP_SHIFT(lane);
        }
        fx1_write32(CSIDPHY_REG(base, LANE_CFG), lane_cfg);
    }
    /* base == 0: no register block, the timer inputs and the lane map are tied at integration. */
    csidphy_state(phy)->configured = true;
    return NEON_OK;
}

static neon_status_t csidphy_power_on(neon_phy_t *phy)
{
    uintptr_t base = csidphy_base(phy);

    /* The timers depend on the line rate: never run the lanes on values computed for no rate. */
    if (!csidphy_state(phy)->configured) {
        return NEON_ERROR;
    }
    if (base != 0U) {
        /* Start the session clean: flags and counters left by an earlier link are not this one's. */
        fx1_write32(CSIDPHY_REG(base, EVT_FLAGS), FX1_CSIDPHY_EVT_ALL_MASK);
        fx1_write32(CSIDPHY_REG(base, EVT_CNT_CLR), FX1_CSIDPHY_EVT_ALL_MASK);
    }
    return NEON_OK;
}

static neon_status_t csidphy_take_status(neon_phy_t *phy, neon_phy_status_t *status)
{
    neon_phy_mipi_dphy_status_t *dphy = &status->mipi_dphy;
    uintptr_t base = csidphy_base(phy);
    uint32_t flags = 0U;

    /* base == 0: the status toggles of the IP reach no register; the SoT errors are still in LANE_STATUS
     * of the CSI-2 controller. */
    if (base != 0U) {
        flags = fx1_read32(CSIDPHY_REG(base, EVT_FLAGS)) & FX1_CSIDPHY_EVT_ALL_MASK;
        if (flags != 0U) {
            fx1_write32(CSIDPHY_REG(base, EVT_FLAGS), flags); /* Exactly the bits read. */
        }
    }
    *dphy = (neon_phy_mipi_dphy_status_t){0};
    dphy->sot_error_mask = csidphy_lane_field(flags, FX1_CSIDPHY_EVT_ERRSOTHS_SHIFT);
    dphy->sot_sync_error_mask = csidphy_lane_field(flags, FX1_CSIDPHY_EVT_ERRSOTSYNCHS_SHIFT);
    dphy->escape_error_mask = csidphy_lane_field(flags, FX1_CSIDPHY_EVT_ERRESC_SHIFT);
    dphy->vendor = ((uint32_t)csidphy_lane_field(flags, FX1_CSIDPHY_EVT_ERREOTSYNC_SHIFT) << NEON_PHY_FX1_CSIDPHY_EOT_SYNC_SHIFT) |
                   ((uint32_t)csidphy_lane_field(flags, FX1_CSIDPHY_EVT_HS_RX_TIMEOUT_SHIFT)
                    << NEON_PHY_FX1_CSIDPHY_HS_RX_TIMEOUT_SHIFT) |
                   ((uint32_t)csidphy_lane_field(flags, FX1_CSIDPHY_EVT_ESC_TIMEOUT_SHIFT) << NEON_PHY_FX1_CSIDPHY_ESC_TIMEOUT_SHIFT) |
                   (((flags & FX1_CSIDPHY_EVT_CL_ABORT_MASK) != 0U) ? NEON_PHY_FX1_CSIDPHY_CLOCK_ABORT : 0U);
    return NEON_OK;
}

/*==================================================================================================
*                                      GLOBAL VARIABLES
==================================================================================================*/
const neon_phy_ops_t fx1_csidphy_ops = {
    .mode = NEON_PHY_MODE_MIPI_DPHY,           /* MIPI D-PHY ... */
    .submode = NEON_PHY_MIPI_DPHY_SUBMODE_RX,  /* ... receive side, the only one it has. */
    .init = csidphy_init,                      /* IP_VERSION and IP_PARAM check. */
    .exit = NULL,                              /* Nothing to release: the client disables the lanes. */
    .set_mode = NULL,                          /* Only the mode above is accepted. */
    .validate = csidphy_validate,              /* Generic checks, R1 limits, timer ranges. */
    .configure = csidphy_configure,            /* Timers and LANE_CFG, while powered off. */
    .power_on = csidphy_power_on,              /* Needs a configuration; clears EVT_FLAGS and the counters. */
    .power_off = NULL,                         /* The lane enables are in the CSI-2 controller. */
    .take_status = csidphy_take_status,        /* EVT_FLAGS, read and cleared. */
};
