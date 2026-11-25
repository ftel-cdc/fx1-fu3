/**
 * @file    neon_eth_phy_internal.h
 * @brief   Clause 22 PHY management shared by the Ethernet backends.
 * @details @internal Backends supply bounded MDIO transfers and a busy-wait delay; this layer finds the
 *          PHY, applies chip quirks (VSC8531 RMII mode in managed mode), advertises only 10/100 full
 *          duplex (the MAC has no half-duplex path) and resolves the negotiated link. No RTOS dependency.
 *          Every call runs in the driver owner's context; waits are bounded busy loops.
 */
#ifndef __NEON_ETH_PHY_INTERNAL_H__
#define __NEON_ETH_PHY_INTERNAL_H__

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include "neon_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/*==================================================================================================
*                                       DEFINES AND MACROS
==================================================================================================*/
/** Clause 22 register numbers (IEEE 802.3 clause 22.2.4). */
#define ETH_PHY_REG_BMCR 0U
#define ETH_PHY_REG_BMSR 1U
#define ETH_PHY_REG_ID1 2U
#define ETH_PHY_REG_ID2 3U
#define ETH_PHY_REG_ANAR 4U
#define ETH_PHY_REG_ANLPAR 5U
#define ETH_PHY_REG_GBCR 9U

#define ETH_PHY_BMCR_RESET (1UL << 15)
#define ETH_PHY_BMCR_LOOPBACK (1UL << 14)
#define ETH_PHY_BMCR_SPEED_100 (1UL << 13)
#define ETH_PHY_BMCR_AN_ENABLE (1UL << 12)
#define ETH_PHY_BMCR_AN_RESTART (1UL << 9)
#define ETH_PHY_BMCR_FULL_DUPLEX (1UL << 8)

#define ETH_PHY_BMSR_AN_COMPLETE (1UL << 5)
#define ETH_PHY_BMSR_LINK (1UL << 2)

#define ETH_PHY_ADV_SELECTOR_8023 0x0001U
#define ETH_PHY_ADV_10_FD (1UL << 6)
#define ETH_PHY_ADV_100_HD (1UL << 7)
#define ETH_PHY_ADV_100_FD (1UL << 8)
#define ETH_PHY_ADV_10_HD (1UL << 5)

/** Highest MDIO address (5-bit PHYAD). */
#define ETH_PHY_ADDRESS_MAX 31U

/*==================================================================================================
*                                  STRUCTURES AND OTHER TYPEDEFS
==================================================================================================*/
/** MDIO access supplied by the backend; both calls are bounded and return NEON_* status. */
typedef struct eth_phy_bus_s {
    void *context;
    neon_status_t (*read)(void *context, uint32_t phy_address, uint32_t reg, uint16_t *value);
    neon_status_t (*write)(void *context, uint32_t phy_address, uint32_t reg, uint16_t value);
    void (*delay_us)(void *context, uint32_t delay_us);
} eth_phy_bus_t;

/** PHY found on the bus. */
typedef struct eth_phy_s {
    uint32_t address;
    uint16_t id1;
    uint16_t id2;
} eth_phy_t;

/** How the PHY is brought up. */
typedef struct eth_phy_mode_s {
    uint32_t autonegotiation; /**< 1: advertise 10/100 full duplex and restart autonegotiation. */
    uint32_t speed_mbps;      /**< Forced rate (10 or 100) when autonegotiation is 0. */
    uint32_t loopback;        /**< 1: PHY loopback, forced rate, autonegotiation off. */
} eth_phy_mode_t;

/** Link state read from the PHY. */
typedef struct eth_phy_link_s {
    uint32_t up;
    uint32_t speed_mbps;
    uint32_t full_duplex;
} eth_phy_link_t;

/*==================================================================================================
*                                       FUNCTION PROTOTYPES
==================================================================================================*/
/**
 * @brief Find a PHY at a fixed address or the lowest responding address.
 * @param[in] bus MDIO access.
 * @param[in] address 0..31, or a value above ETH_PHY_ADDRESS_MAX to scan 0..31.
 * @param[out] phy Address and identifiers of the PHY; unchanged on error.
 * @return NEON_OK, NEON_ERROR when no PHY answers, or NEON_INVALID_PARAM.
 * @note ID1 of 0x0000 or 0xFFFF counts as absent: without a pull-up on MDIO an absent PHY may still
 *       complete the frame without NO_RESPONSE (MAS 6.11).
 */
neon_status_t eth_phy_find(const eth_phy_bus_t *bus, uint32_t address, eth_phy_t *phy);

/**
 * @brief Prepare the PHY for the MAC (RMII, 10/100, full duplex) and start the link.
 * @param[in] bus MDIO access.
 * @param[in] phy PHY returned by eth_phy_find().
 * @param[in] mode Autonegotiation, forced rate or loopback.
 * @return NEON_OK, NEON_TIMEOUT if a soft reset does not complete, NEON_ERROR or NEON_INVALID_PARAM.
 * @note Busy-waits up to about 600 ms (VSC8531 soft reset); call before the MAC starts.
 */
neon_status_t eth_phy_setup(const eth_phy_bus_t *bus, const eth_phy_t *phy, const eth_phy_mode_t *mode);

/**
 * @brief Read the current link: BMSR twice (link status latches low), then the resolved rate.
 * @param[in] bus MDIO access.
 * @param[in] phy PHY returned by eth_phy_find().
 * @param[out] link Link state; up is 0 until autonegotiation completes.
 * @return NEON_OK, NEON_ERROR or NEON_INVALID_PARAM.
 */
neon_status_t eth_phy_get_link(const eth_phy_bus_t *bus, const eth_phy_t *phy, eth_phy_link_t *link);

#ifdef __cplusplus
}
#endif
#endif /* __NEON_ETH_PHY_INTERNAL_H__ */
