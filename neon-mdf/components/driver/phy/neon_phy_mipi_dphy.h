/**
 * @file neon_phy_mipi_dphy.h
 * @brief MIPI D-PHY settings of the generic PHY interface (neon_phy.h), after include/linux/phy/phy-mipi-dphy.h.
 * @details
 * What a client of a D-PHY gives neon_phy_configure() and gets back from neon_phy_take_status(). The values
 * are those of the link and the board (lanes, line rate, wiring) and the error indications of the PPI
 * (D-PHY specification, Annex A); each PHY driver turns them into its own register values. Included by
 * neon_phy.h; a client includes neon_phy.h.
 */

#ifndef __NEON_PHY_MIPI_DPHY_H__
#define __NEON_PHY_MIPI_DPHY_H__

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include <stdbool.h>
#include <stdint.h>
#include "neon_types.h"
#ifdef __cplusplus
extern "C" {
#endif

/*==================================================================================================
*                                        DEFINES AND MACROS
==================================================================================================*/
/* Capacity of the structures: the most data lanes any supported D-PHY has. */
#define NEON_PHY_MIPI_DPHY_MAX_LANES 4U

/* Submodes of NEON_PHY_MODE_MIPI_DPHY, the values of Linux PHY_MIPI_DPHY_SUBMODE_*. */
#define NEON_PHY_MIPI_DPHY_SUBMODE_TX 0   /* Master side: DSI host, CSI-2 transmitter. */
#define NEON_PHY_MIPI_DPHY_SUBMODE_RX 1   /* Slave side: CSI-2 receiver, DSI peripheral. */

/*==================================================================================================
*                                  STRUCTURES AND OTHER TYPEDEFS
==================================================================================================*/
/** D-PHY link settings, given to neon_phy_configure() in NEON_PHY_MODE_MIPI_DPHY. */
typedef struct {
    uint8_t lane_count;                             /* Data lanes in use: logical lanes 0 to lane_count - 1. */
    uint32_t lane_rate_bps;                         /* Line rate of each data lane, in bit/s. */
    bool continuous_clock;                          /* The clock lane stays in High-Speed mode between bursts. */
    uint8_t lane_map[NEON_PHY_MIPI_DPHY_MAX_LANES]; /* Physical data lane of logical lane n; together a permutation. */
    uint8_t lane_polarity_mask;                     /* Bit n set: Dp and Dn of physical data lane n are swapped on the board. */
    uint32_t hs_rx_timeout_ns;                      /* Receive side: longest High-Speed burst plus margin; 0 keeps the PHY default. */
} neon_phy_mipi_dphy_opts_t;

/**
 * D-PHY errors since the previous neon_phy_take_status(). Bit n of a mask is logical data lane n. The
 * named fields are the error signals of the PPI; a PHY that does not raise one leaves it at 0.
 */
typedef struct {
    uint8_t sot_error_mask;                         /* ErrSotHS: SoT leader with a single-bit error; the burst is usable. */
    uint8_t sot_sync_error_mask;                    /* ErrSotSyncHS: no SoT leader found; the burst carried no data. */
    uint8_t control_error_mask;                     /* ErrControl: line states in a sequence the protocol does not allow. */
    uint8_t escape_error_mask;                      /* ErrEsc: unknown escape-mode entry command. */
    uint8_t escape_sync_error_mask;                 /* ErrSyncEsc: Low-Power Data Transmission ended off a byte boundary. */
    uint32_t vendor;                                /* Errors only this PHY reports; bits in its phy/platform/<family>/neon_phy_<family>.h. */
} neon_phy_mipi_dphy_status_t;

/*==================================================================================================
*                                       FUNCTION PROTOTYPES
==================================================================================================*/
/**
 * @brief Fill D-PHY settings with the defaults: continuous clock, straight lane map, no polarity swap, the
 * PHY's own High-Speed receive timeout.
 * @param[in] lane_count Data lanes in use.
 * @param[in] lane_rate_bps Line rate of each data lane.
 * @param[out] opts Settings. Ignored when NULL.
 */
void neon_phy_mipi_dphy_get_default_opts(uint8_t lane_count, uint32_t lane_rate_bps, neon_phy_mipi_dphy_opts_t *opts);

/**
 * @brief Check the settings that do not depend on the PHY: a lane count from 1 to max_lanes, a line rate,
 * a lane map that is a permutation of the max_lanes physical lanes, and polarity bits of those lanes only.
 * For PHY drivers; the limits of one PHY are checked by its validate operation.
 * @param[in] opts Settings.
 * @param[in] max_lanes Data lanes the PHY has, at most NEON_PHY_MIPI_DPHY_MAX_LANES.
 * @return NEON_OK, or NEON_INVALID_PARAM.
 */
neon_status_t neon_phy_mipi_dphy_check_opts(const neon_phy_mipi_dphy_opts_t *opts, uint8_t max_lanes);

#ifdef __cplusplus
}
#endif

#endif /* __NEON_PHY_MIPI_DPHY_H__ */
