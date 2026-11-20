/**
 * @file neon_phy_mipi_dphy.c
 * @brief MIPI D-PHY settings: defaults and the checks that do not depend on the PHY.
 */

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include "phy/neon_phy_mipi_dphy.h"
#include <stddef.h>
#include <string.h>

/*==================================================================================================
*                                         GLOBAL FUNCTIONS
==================================================================================================*/
void neon_phy_mipi_dphy_get_default_opts(uint8_t lane_count, uint32_t lane_rate_bps, neon_phy_mipi_dphy_opts_t *opts)
{
    uint8_t lane;

    if (opts == NULL) {
        return;
    }
    (void)memset(opts, 0, sizeof(*opts));
    opts->lane_count = lane_count;
    opts->lane_rate_bps = lane_rate_bps;
    opts->continuous_clock = true;
    for (lane = 0U; lane < (uint8_t)NEON_PHY_MIPI_DPHY_MAX_LANES; lane++) {
        opts->lane_map[lane] = lane;
    }
}

neon_status_t neon_phy_mipi_dphy_check_opts(const neon_phy_mipi_dphy_opts_t *opts, uint8_t max_lanes)
{
    uint32_t seen = 0U;
    uint8_t lane;

    if ((opts == NULL) || (max_lanes == 0U) || (max_lanes > NEON_PHY_MIPI_DPHY_MAX_LANES)) {
        return NEON_INVALID_PARAM;
    }
    if ((opts->lane_count == 0U) || (opts->lane_count > max_lanes) || (opts->lane_rate_bps == 0U)) {
        return NEON_INVALID_PARAM;
    }
    /* Every logical lane of the PHY is mapped, used or not, so the map must cover all max_lanes. */
    for (lane = 0U; lane < max_lanes; lane++) {
        if ((opts->lane_map[lane] >= max_lanes) || ((seen & (1U << opts->lane_map[lane])) != 0U)) {
            return NEON_INVALID_PARAM;
        }
        seen |= 1U << opts->lane_map[lane];
    }
    if (((uint32_t)opts->lane_polarity_mask >> max_lanes) != 0U) {
        return NEON_INVALID_PARAM;
    }
    return NEON_OK;
}
