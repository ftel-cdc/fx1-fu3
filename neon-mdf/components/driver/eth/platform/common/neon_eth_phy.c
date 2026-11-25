/**
 * @file    neon_eth_phy.c
 * @brief   Clause 22 PHY management shared by the Ethernet backends.
 * @details Generic IEEE 802.3 clause 22 handling plus one chip quirk: the Microchip VSC8531-02 of the
 *          FPGA FMC+ card must run RMII, which in managed mode only software can select
 *          (datasheet VMDS-10514 3.15.1: register 23[12:11] = 01, then soft reset). The FX1 MAC is
 *          RMII 10/100 full duplex only (MAS 1.2), so 1000BASE-T and half duplex are never advertised.
 */

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include "neon_eth_phy_internal.h"
#include <stddef.h>

/*==================================================================================================
*                                       DEFINES AND MACROS
==================================================================================================*/
/* BMSR bit 8: extended status register present (1000BASE-T capable PHY). */
#define PHY_BMSR_EXTENDED_STATUS (1UL << 8)
/* 1000BASE-T control: advertise 1000 full/half duplex. */
#define PHY_GBCR_ADV_1000 ((1UL << 9) | (1UL << 8))
/* ANAR technology field [12:5] (also clears pause bits: the MAC ignores PAUSE frames). */
#define PHY_ANAR_TECHNOLOGY_MASK 0x1FE0U

/* Microchip VSC8531: OUI/model match on ID2[15:4]; revision in ID2[3:0]. */
#define PHY_VSC8531_ID1 0x0007U
#define PHY_VSC8531_ID2_MODEL 0x0570U
#define PHY_VSC8531_ID2_MODEL_MASK 0xFFF0U
#define PHY_VSC8531_REG_PAGE 31U
#define PHY_VSC8531_REG_EXT_CTRL1 23U
#define PHY_VSC8531_MAC_IF_MASK 0x1800U
#define PHY_VSC8531_MAC_IF_RMII 0x0800U

/* Soft reset completion: bounded poll, 1 ms steps. */
#define PHY_RESET_POLL_US 1000U
#define PHY_RESET_TIMEOUT_POLLS 600U

/*==================================================================================================
*                                   PRIVATE FUNCTION PROTOTYPES
==================================================================================================*/
static int phy_bus_valid(const eth_phy_bus_t *bus);
static int phy_is_vsc8531(const eth_phy_t *phy);
static neon_status_t phy_soft_reset(const eth_phy_bus_t *bus, uint32_t address);
static neon_status_t phy_vsc8531_select_rmii(const eth_phy_bus_t *bus, uint32_t address);
static uint32_t phy_resolve(uint16_t common, uint32_t *full_duplex);

/*==================================================================================================
*                                         GLOBAL FUNCTIONS
==================================================================================================*/
/** @copydoc eth_phy_find */
neon_status_t eth_phy_find(const eth_phy_bus_t *bus, uint32_t address, eth_phy_t *phy) {
    if (!phy_bus_valid(bus) || phy == NULL) {
        return NEON_INVALID_PARAM;
    }
    uint32_t first = address;
    uint32_t last = address;
    if (address > ETH_PHY_ADDRESS_MAX) {
        first = 0U;
        last = ETH_PHY_ADDRESS_MAX;
    }
    for (uint32_t candidate = first; candidate <= last; ++candidate) {
        uint16_t id1 = 0U;
        uint16_t id2 = 0U;
        if (bus->read(bus->context, candidate, ETH_PHY_REG_ID1, &id1) != NEON_OK) {
            continue;
        }
        if (id1 == 0x0000U || id1 == 0xFFFFU) {
            continue;
        }
        if (bus->read(bus->context, candidate, ETH_PHY_REG_ID2, &id2) != NEON_OK) {
            continue;
        }
        phy->address = candidate;
        phy->id1 = id1;
        phy->id2 = id2;
        return NEON_OK;
    }
    return NEON_ERROR;
}

/** @copydoc eth_phy_setup */
neon_status_t eth_phy_setup(const eth_phy_bus_t *bus, const eth_phy_t *phy, const eth_phy_mode_t *mode) {
    if (!phy_bus_valid(bus) || phy == NULL || mode == NULL || phy->address > ETH_PHY_ADDRESS_MAX ||
        (mode->speed_mbps != 10U && mode->speed_mbps != 100U)) {
        return NEON_INVALID_PARAM;
    }
    neon_status_t status = NEON_OK;
    if (phy_is_vsc8531(phy)) {
        status = phy_vsc8531_select_rmii(bus, phy->address);
        if (status != NEON_OK) {
            return status;
        }
    }
    const uint16_t forced =
        (uint16_t)(ETH_PHY_BMCR_FULL_DUPLEX | (mode->speed_mbps == 100U ? ETH_PHY_BMCR_SPEED_100 : 0U));
    if (mode->loopback != 0U) {
        return bus->write(bus->context, phy->address, ETH_PHY_REG_BMCR, (uint16_t)(forced | ETH_PHY_BMCR_LOOPBACK));
    }
    if (mode->autonegotiation == 0U) {
        return bus->write(bus->context, phy->address, ETH_PHY_REG_BMCR, forced);
    }
    uint16_t bmsr = 0U;
    uint16_t anar = 0U;
    status = bus->read(bus->context, phy->address, ETH_PHY_REG_BMSR, &bmsr);
    if (status == NEON_OK && (bmsr & PHY_BMSR_EXTENDED_STATUS) != 0U) {
        uint16_t gbcr = 0U;
        status = bus->read(bus->context, phy->address, ETH_PHY_REG_GBCR, &gbcr);
        if (status == NEON_OK) {
            status = bus->write(bus->context, phy->address, ETH_PHY_REG_GBCR, (uint16_t)(gbcr & ~PHY_GBCR_ADV_1000));
        }
    }
    if (status == NEON_OK) {
        status = bus->read(bus->context, phy->address, ETH_PHY_REG_ANAR, &anar);
    }
    if (status == NEON_OK) {
        anar = (uint16_t)((anar & ~PHY_ANAR_TECHNOLOGY_MASK) | ETH_PHY_ADV_10_FD | ETH_PHY_ADV_100_FD);
        status = bus->write(bus->context, phy->address, ETH_PHY_REG_ANAR, anar);
    }
    if (status == NEON_OK) {
        status = bus->write(bus->context, phy->address, ETH_PHY_REG_BMCR,
                            (uint16_t)(ETH_PHY_BMCR_AN_ENABLE | ETH_PHY_BMCR_AN_RESTART));
    }
    return status;
}

/** @copydoc eth_phy_get_link */
neon_status_t eth_phy_get_link(const eth_phy_bus_t *bus, const eth_phy_t *phy, eth_phy_link_t *link) {
    if (!phy_bus_valid(bus) || phy == NULL || link == NULL || phy->address > ETH_PHY_ADDRESS_MAX) {
        return NEON_INVALID_PARAM;
    }
    uint16_t bmcr = 0U;
    uint16_t bmsr = 0U;
    /* Link status latches low until read: the second read is the current state. */
    neon_status_t status = bus->read(bus->context, phy->address, ETH_PHY_REG_BMSR, &bmsr);
    if (status == NEON_OK) {
        status = bus->read(bus->context, phy->address, ETH_PHY_REG_BMSR, &bmsr);
    }
    if (status == NEON_OK) {
        status = bus->read(bus->context, phy->address, ETH_PHY_REG_BMCR, &bmcr);
    }
    if (status != NEON_OK) {
        return status;
    }
    link->up = 0U;
    link->speed_mbps = (bmcr & ETH_PHY_BMCR_SPEED_100) != 0U ? 100U : 10U;
    link->full_duplex = (bmcr & ETH_PHY_BMCR_FULL_DUPLEX) != 0U ? 1U : 0U;
    if ((bmcr & ETH_PHY_BMCR_LOOPBACK) != 0U) {
        /* The loop is inside the PHY; the line state does not matter. */
        link->up = 1U;
        return NEON_OK;
    }
    if ((bmsr & ETH_PHY_BMSR_LINK) == 0U) {
        return NEON_OK;
    }
    if ((bmcr & ETH_PHY_BMCR_AN_ENABLE) == 0U) {
        link->up = 1U;
        return NEON_OK;
    }
    if ((bmsr & ETH_PHY_BMSR_AN_COMPLETE) == 0U) {
        return NEON_OK;
    }
    uint16_t anar = 0U;
    uint16_t anlpar = 0U;
    status = bus->read(bus->context, phy->address, ETH_PHY_REG_ANAR, &anar);
    if (status == NEON_OK) {
        status = bus->read(bus->context, phy->address, ETH_PHY_REG_ANLPAR, &anlpar);
    }
    if (status != NEON_OK) {
        return status;
    }
    link->speed_mbps = phy_resolve((uint16_t)(anar & anlpar), &link->full_duplex);
    link->up = link->speed_mbps != 0U ? 1U : 0U;
    if (link->speed_mbps == 0U) {
        link->speed_mbps = 10U;
    }
    return NEON_OK;
}

/*==================================================================================================
*                                        PRIVATE FUNCTIONS
==================================================================================================*/
static int phy_bus_valid(const eth_phy_bus_t *bus) {
    return bus != NULL && bus->read != NULL && bus->write != NULL && bus->delay_us != NULL;
}

static int phy_is_vsc8531(const eth_phy_t *phy) {
    return phy->id1 == PHY_VSC8531_ID1 && (phy->id2 & PHY_VSC8531_ID2_MODEL_MASK) == PHY_VSC8531_ID2_MODEL;
}

static neon_status_t phy_soft_reset(const eth_phy_bus_t *bus, uint32_t address) {
    neon_status_t status = bus->write(bus->context, address, ETH_PHY_REG_BMCR, (uint16_t)ETH_PHY_BMCR_RESET);
    for (uint32_t poll = 0U; status == NEON_OK && poll < PHY_RESET_TIMEOUT_POLLS; ++poll) {
        uint16_t bmcr = 0U;
        bus->delay_us(bus->context, PHY_RESET_POLL_US);
        status = bus->read(bus->context, address, ETH_PHY_REG_BMCR, &bmcr);
        if (status == NEON_OK && (bmcr & ETH_PHY_BMCR_RESET) == 0U) {
            return NEON_OK;
        }
    }
    return status == NEON_OK ? NEON_TIMEOUT : status;
}

/* Managed mode boots RGMII (register 23[12:11] reset 10); unmanaged mode takes RMII from the strap.
 * Register 23 is on the main page: select page 0 first. */
static neon_status_t phy_vsc8531_select_rmii(const eth_phy_bus_t *bus, uint32_t address) {
    uint16_t ext_ctrl1 = 0U;
    neon_status_t status = bus->write(bus->context, address, PHY_VSC8531_REG_PAGE, 0U);
    if (status == NEON_OK) {
        status = bus->read(bus->context, address, PHY_VSC8531_REG_EXT_CTRL1, &ext_ctrl1);
    }
    if (status != NEON_OK || (ext_ctrl1 & PHY_VSC8531_MAC_IF_MASK) == PHY_VSC8531_MAC_IF_RMII) {
        return status;
    }
    ext_ctrl1 = (uint16_t)((ext_ctrl1 & ~PHY_VSC8531_MAC_IF_MASK) | PHY_VSC8531_MAC_IF_RMII);
    status = bus->write(bus->context, address, PHY_VSC8531_REG_EXT_CTRL1, ext_ctrl1);
    if (status == NEON_OK) {
        /* The MAC interface mode takes effect at the next soft reset (VMDS-10514 3.15.1). */
        status = phy_soft_reset(bus, address);
    }
    return status;
}

/* Highest common technology, IEEE 802.3 Annex 28B.3 priority. Returns 0 when nothing is common. */
static uint32_t phy_resolve(uint16_t common, uint32_t *full_duplex) {
    if ((common & ETH_PHY_ADV_100_FD) != 0U) {
        *full_duplex = 1U;
        return 100U;
    }
    if ((common & ETH_PHY_ADV_100_HD) != 0U) {
        *full_duplex = 0U;
        return 100U;
    }
    if ((common & ETH_PHY_ADV_10_FD) != 0U) {
        *full_duplex = 1U;
        return 10U;
    }
    if ((common & ETH_PHY_ADV_10_HD) != 0U) {
        *full_duplex = 0U;
        return 10U;
    }
    *full_duplex = 0U;
    return 0U;
}
