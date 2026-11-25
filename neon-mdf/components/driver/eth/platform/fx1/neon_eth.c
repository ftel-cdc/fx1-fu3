/**
 * @file neon_eth.c
 * @brief Neon Ethernet API for the FTEL FX1 MAC and its descriptor DMA.
 * @details Reference core is isolated in reference/; no RTOS or VP dependency. The PHY is owned by
 * this layer (platform/common/neon_eth_phy.c), so the reference core is told to leave it alone
 * (cfg.phy_addr = -1) and only programs ETH_RMII_CFG from the rate chosen here.
 */

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include "eth/neon_eth.h"
#include "reference/fx1_eth.h"
#include "reference/eth_regs.h"
#include "../common/neon_eth_phy_internal.h"
#include "fx1_fw.h"
#include "sdk_irq.h"
#include <stddef.h>

/*==================================================================================================
*                                       DEFINES AND MACROS
==================================================================================================*/
#define ETH_CSR(offset) (FX1_ETH_CSR_BASE + (uintptr_t)(offset))
/* MAS Table 9-7: after clearing the MAC enables wait >= 13 RMII cycles (260 ns), then for BUSY = 0.
 * A 1518-byte frame takes 1.22 ms at 10 Mbps; leave margin for one in flight. */
#define ETH_RATE_IDLE_TIMEOUT_US 3000U
#define ETH_MAC_ID 0x45544801UL

/*==================================================================================================
*                                  STRUCTURES AND OTHER TYPEDEFS
==================================================================================================*/
struct neon_eth_handle_s {
    fx1_eth mac;
    neon_eth_config_t config;
    eth_phy_t phy;
    uint32_t phy_found;
    uint32_t acquired;
    uint32_t interrupts;
};

/*==================================================================================================
*                                        LOCAL VARIABLES
==================================================================================================*/
static neon_eth_handle_t eth_handle;

static const neon_eth_config_t eth_default_config = {
    .mac_address = {0x02U, 0x00U, 0x46U, 0x58U, 0x31U, 0x01U},
    .speed_mbps = 100U,
    .autonegotiation = 1U,
    .phy_address = NEON_ETH_PHY_ADDRESS_SCAN,
    .phy_loopback = 0U,
};

/*==================================================================================================
*                                   PRIVATE FUNCTION PROTOTYPES
==================================================================================================*/
static int eth_valid(const neon_eth_handle_t *handle);
static neon_status_t eth_status(int result);
static uint32_t eth_mask_irq(void);
static void eth_restore_irq(uint32_t enabled);
static void eth_delay_us(void *context, uint32_t delay_us);
static neon_status_t eth_bus_read(void *context, uint32_t phy_address, uint32_t reg, uint16_t *value);
static neon_status_t eth_bus_write(void *context, uint32_t phy_address, uint32_t reg, uint16_t value);
static void eth_bus_bind(neon_eth_handle_t *handle, eth_phy_bus_t *bus);
static void eth_release_mdio(void);
static neon_status_t eth_find_phy(neon_eth_handle_t *handle);
static neon_status_t eth_prepare_phy(neon_eth_handle_t *handle);
static neon_status_t eth_switch_rate(uint32_t speed100);
static void eth_apply_config(neon_eth_handle_t *handle);

/*==================================================================================================
*                                         GLOBAL FUNCTIONS
==================================================================================================*/
/** @copydoc neon_eth_get_default_config */
neon_status_t neon_eth_get_default_config(neon_eth_config_t *config) {
    if (config == NULL) {
        return NEON_INVALID_PARAM;
    }
    *config = eth_default_config;
    return NEON_OK;
}

/** @copydoc neon_eth_init */
neon_status_t neon_eth_init(neon_eth_handle_t **handle, neon_eth_t instance) {
    if (handle == NULL || instance != NEON_ETH0 || fx1_hartid() != 0U) {
        return NEON_INVALID_PARAM;
    }
    if (eth_handle.acquired != 0U) {
        return NEON_BUSY;
    }
    fx1_eth_setup(&eth_handle.mac);
    eth_handle.config = eth_default_config;
    eth_handle.phy_found = 0U;
    eth_apply_config(&eth_handle);
    eth_handle.acquired = 1U;
    eth_handle.interrupts = 0U;
    *handle = &eth_handle;
    return NEON_OK;
}

/** @copydoc neon_eth_set_config */
neon_status_t neon_eth_set_config(neon_eth_handle_t *handle, const neon_eth_config_t *config) {
    if (!eth_valid(handle) || config == NULL || (config->speed_mbps != 10U && config->speed_mbps != 100U) ||
        config->phy_loopback > 1U || config->autonegotiation > 1U ||
        (config->phy_address > ETH_PHY_ADDRESS_MAX && config->phy_address != NEON_ETH_PHY_ADDRESS_SCAN) ||
        (config->mac_address[0] & 1U) != 0U) {
        return NEON_INVALID_PARAM;
    }
    uint32_t nonzero = 0U;
    for (uint32_t i = 0U; i < 6U; ++i) {
        nonzero |= config->mac_address[i];
    }
    if (nonzero == 0U) {
        return NEON_INVALID_PARAM;
    }
    if (handle->mac.state != FX1_ETH_UNINIT) {
        return NEON_BUSY;
    }
    handle->config = *config;
    handle->phy_found = 0U;
    eth_apply_config(handle);
    return NEON_OK;
}

/** @copydoc neon_eth_register_callback */
neon_status_t neon_eth_register_callback(neon_eth_handle_t *handle, neon_eth_rx_callback_t callback, void *context) {
    if (!eth_valid(handle)) {
        return NEON_INVALID_PARAM;
    }
    handle->mac.rx_cb = callback;
    handle->mac.rx_ctx = context;
    return NEON_OK;
}

/** @copydoc neon_eth_probe */
neon_status_t neon_eth_probe(neon_eth_handle_t *handle, neon_eth_info_t *info) {
    if (!eth_valid(handle) || info == NULL) {
        return NEON_INVALID_PARAM;
    }
    if (handle->mac.state == FX1_ETH_FAULTED) {
        return NEON_BUSY;
    }
    info->mac_id = fx1_read32(ETH_CSR(ETH_ID));
    info->build_config = fx1_read32(ETH_CSR(ETH_DMA_BUILDCFG));
    if (info->mac_id != ETH_MAC_ID || info->build_config != ETH_DRV_BUILDCFG) {
        return NEON_ERROR;
    }
    eth_release_mdio();
    neon_status_t status = eth_find_phy(handle);
    if (status != NEON_OK) {
        return status;
    }
    neon_eth_link_t link;
    status = neon_eth_get_link(handle, &link);
    if (status == NEON_OK) {
        info->phy_address = handle->phy.address;
        info->phy_id1 = handle->phy.id1;
        info->phy_id2 = handle->phy.id2;
        info->link_up = link.up;
    }
    return status;
}

/** @copydoc neon_eth_start */
neon_status_t neon_eth_start(neon_eth_handle_t *handle) {
    if (!eth_valid(handle)) {
        return NEON_INVALID_PARAM;
    }
    if (handle->mac.state == FX1_ETH_UNINIT) {
        /* Cold start: the PHY must run RMII at a rate the MAC is told before RX_EN/TX_EN. */
        neon_status_t status = eth_prepare_phy(handle);
        if (status != NEON_OK) {
            return status;
        }
    }
    uint32_t enabled = eth_mask_irq();
    int result = fx1_eth_start(&handle->mac);
    /* Failed initialization may have issued DMA writes. Retain memory until
     * recovery proves quiescence; never allow setup/deinit to reuse it. */
    if (result != ETH_OK) {
        handle->mac.state = FX1_ETH_FAULTED;
    }
    eth_restore_irq(enabled);
    return eth_status(result);
}

/** @copydoc neon_eth_transmit */
neon_status_t neon_eth_transmit(neon_eth_handle_t *handle, const uint8_t *frame, uint32_t size_bytes) {
    if (!eth_valid(handle) || frame == NULL || size_bytes < NEON_ETH_FRAME_MIN_BYTES ||
        size_bytes > NEON_ETH_FRAME_MAX_BYTES) {
        return NEON_INVALID_PARAM;
    }
    return eth_status(fx1_eth_send(&handle->mac, frame, size_bytes));
}

/** @copydoc neon_eth_process */
neon_status_t neon_eth_process(neon_eth_handle_t *handle, uint32_t *next_poll_ms) {
    if (!eth_valid(handle) || next_poll_ms == NULL) {
        return NEON_INVALID_PARAM;
    }
    /* Supports polling with the same take/ack ordering as the IRQ path. */
    if (handle->mac.state == FX1_ETH_RUNNING) {
        uint32_t enabled = eth_mask_irq();
        fx1_eth_isr(&handle->mac);
        eth_restore_irq(enabled);
    }
    *next_poll_ms = (uint32_t)fx1_eth_service(&handle->mac);
    return handle->mac.state == FX1_ETH_FAULTED ? NEON_ERROR : NEON_OK;
}

/** @copydoc neon_eth_irq_handler */
neon_status_t neon_eth_irq_handler(neon_eth_handle_t *handle) {
    if (!eth_valid(handle)) {
        return NEON_INVALID_PARAM;
    }
    fx1_eth_isr(&handle->mac);
    ++handle->interrupts;
    return NEON_OK;
}

/** @copydoc neon_eth_get_link */
neon_status_t neon_eth_get_link(neon_eth_handle_t *handle, neon_eth_link_t *link) {
    if (!eth_valid(handle) || link == NULL) {
        return NEON_INVALID_PARAM;
    }
    if (handle->mac.state == FX1_ETH_FAULTED) {
        return NEON_BUSY;
    }
    if (handle->phy_found == 0U) {
        return NEON_ERROR;
    }
    eth_phy_bus_t bus;
    eth_phy_link_t phy_link;
    eth_bus_bind(handle, &bus);
    neon_status_t status = eth_phy_get_link(&bus, &handle->phy, &phy_link);
    if (status == NEON_OK) {
        link->up = phy_link.up;
        link->speed_mbps = phy_link.speed_mbps;
        link->full_duplex = phy_link.full_duplex;
    }
    return status;
}

/** @copydoc neon_eth_set_speed */
neon_status_t neon_eth_set_speed(neon_eth_handle_t *handle, uint32_t speed_mbps) {
    if (!eth_valid(handle) || (speed_mbps != 10U && speed_mbps != 100U)) {
        return NEON_INVALID_PARAM;
    }
    const int speed100 = speed_mbps == 100U ? 1 : 0;
    neon_status_t status = NEON_OK;
    switch (handle->mac.state) {
        case FX1_ETH_UNINIT:
            /* eth_init() writes ETH_RMII_CFG from this before releasing the MAC. */
            break;
        case FX1_ETH_STOPPED:
            /* Both MACs disabled and idle after the quiesce: the window is open (Table 9-7). */
            fx1_write32(ETH_CSR(ETH_RMII_CFG), speed100 != 0 ? ETH_RMII_CFG_SPEED_MASK : 0U);
            break;
        case FX1_ETH_RUNNING:
            if (handle->mac.speed100 != speed100) {
                status = eth_switch_rate((uint32_t)speed100);
            }
            break;
        default:
            return NEON_BUSY;
    }
    if (status == NEON_OK) {
        handle->mac.speed100 = speed100;
        handle->mac.cfg.speed100 = speed100;
        handle->mac.dev.cfg.speed100 = speed100;
    }
    return status;
}

/** @copydoc neon_eth_mdio_read */
neon_status_t neon_eth_mdio_read(neon_eth_handle_t *handle, uint32_t phy_address, uint32_t reg, uint16_t *value) {
    if (!eth_valid(handle) || phy_address > ETH_PHY_ADDRESS_MAX || reg > 31U || value == NULL) {
        return NEON_INVALID_PARAM;
    }
    if (handle->mac.state == FX1_ETH_FAULTED) {
        return NEON_BUSY;
    }
    eth_release_mdio();
    return eth_bus_read(handle, phy_address, reg, value);
}

/** @copydoc neon_eth_mdio_write */
neon_status_t neon_eth_mdio_write(neon_eth_handle_t *handle, uint32_t phy_address, uint32_t reg, uint16_t value) {
    if (!eth_valid(handle) || phy_address > ETH_PHY_ADDRESS_MAX || reg > 31U) {
        return NEON_INVALID_PARAM;
    }
    if (handle->mac.state == FX1_ETH_FAULTED) {
        return NEON_BUSY;
    }
    eth_release_mdio();
    return eth_bus_write(handle, phy_address, reg, value);
}

/** @copydoc neon_eth_stop */
neon_status_t neon_eth_stop(neon_eth_handle_t *handle, uint32_t drain_us) {
    if (!eth_valid(handle)) {
        return NEON_INVALID_PARAM;
    }
    uint32_t enabled = eth_mask_irq();
    int result = fx1_eth_stop(&handle->mac, drain_us);
    eth_restore_irq(enabled);
    return eth_status(result);
}

/** @copydoc neon_eth_recover */
neon_status_t neon_eth_recover(neon_eth_handle_t *handle) {
    if (!eth_valid(handle)) {
        return NEON_INVALID_PARAM;
    }
    uint32_t enabled = eth_mask_irq();
    int result = fx1_eth_recover(&handle->mac);
    eth_restore_irq(enabled);
    return eth_status(result);
}

/** @copydoc neon_eth_get_state */
neon_eth_state_t neon_eth_get_state(const neon_eth_handle_t *handle) {
    if (!eth_valid(handle)) {
        return NEON_ETH_STATE_RESET;
    }
    switch (handle->mac.state) {
        case FX1_ETH_UNINIT:
        case FX1_ETH_STOPPED:
            return NEON_ETH_STATE_READY;
        case FX1_ETH_RUNNING:
            return NEON_ETH_STATE_RUNNING;
        case FX1_ETH_STOPPING:
            return NEON_ETH_STATE_STOPPING;
        default:
            return NEON_ETH_STATE_FAULTED;
    }
}

/** @copydoc neon_eth_get_stats */
neon_status_t neon_eth_get_stats(const neon_eth_handle_t *handle, neon_eth_stats_t *stats) {
    if (!eth_valid(handle) || stats == NULL) {
        return NEON_INVALID_PARAM;
    }
    stats->tx_frames = handle->mac.c.tx_frames;
    stats->rx_frames = handle->mac.c.rx_frames;
    stats->tx_errors = handle->mac.c.tx_desc_errors;
    stats->rx_errors = handle->mac.c.rx_desc_errors;
    stats->tx_dropped = handle->mac.c.tx_dropped;
    stats->interrupts = handle->interrupts;
    stats->late_writebacks = handle->mac.c.wb_timeouts;
    stats->recoveries = handle->mac.c.recoveries;
    return NEON_OK;
}

/** @copydoc neon_eth_deinit */
neon_status_t neon_eth_deinit(neon_eth_handle_t *handle) {
    if (!eth_valid(handle)) {
        return NEON_INVALID_PARAM;
    }
    if (handle->mac.state != FX1_ETH_UNINIT && handle->mac.state != FX1_ETH_STOPPED) {
        return NEON_BUSY;
    }
    /* Prevent a stale pending PLIC delivery from touching a released handle. */
    SDK_IRQ_Disable((int32_t)FX1_IRQ_ETH);
    if (handle->mac.state == FX1_ETH_STOPPED) {
        /* Both engines have acknowledged HALTED. Cold init on the next
         * acquisition expects reset IDLE rather than the stopped state. */
        fx1_write32(ETH_CSR(ETH_CTRL), 0U);
        fx1_mb();
    }
    handle->acquired = 0U;
    return NEON_OK;
}

/*==================================================================================================
*                                        PRIVATE FUNCTIONS
==================================================================================================*/
static int eth_valid(const neon_eth_handle_t *handle) {
    return handle == &eth_handle && eth_handle.acquired != 0U && fx1_hartid() == 0U;
}

static neon_status_t eth_status(int result) {
    switch (result) {
        case ETH_OK:
            return NEON_OK;
        case ETH_E_STATE:
        case ETH_E_FULL:
            return NEON_BUSY;
        case ETH_E_TIMEOUT:
            return NEON_TIMEOUT;
        case ETH_E_LEN:
        case ETH_E_CFG:
            return NEON_INVALID_PARAM;
        default:
            return NEON_ERROR;
    }
}

static uint32_t eth_mask_irq(void) {
    uint32_t saved = SDK_IRQ_EnterCritical();
    uint32_t enabled = fx1_read32(FX1_PLIC_BASE + FX1_PLIC_ENABLE_OFF) & (1UL << FX1_IRQ_ETH);
    SDK_IRQ_Disable((int32_t)FX1_IRQ_ETH);
    SDK_IRQ_ExitCritical(saved);
    return enabled;
}

static void eth_restore_irq(uint32_t enabled) {
    if (enabled != 0U) {
        SDK_IRQ_Enable((int32_t)FX1_IRQ_ETH);
    }
}

static void eth_delay_us(void *context, uint32_t delay_us) {
    (void)context;
    const uint64_t end = fx1_mtime() + (uint64_t)delay_us * (FX1_MTIME_HZ / 1000000U);
    while (fx1_mtime() < end) {
    }
}

static neon_status_t eth_bus_read(void *context, uint32_t phy_address, uint32_t reg, uint16_t *value) {
    neon_eth_handle_t *handle = (neon_eth_handle_t *)context;
    return eth_status(fx1_eth_mdio_read(&handle->mac, phy_address, reg, value));
}

static neon_status_t eth_bus_write(void *context, uint32_t phy_address, uint32_t reg, uint16_t value) {
    neon_eth_handle_t *handle = (neon_eth_handle_t *)context;
    return eth_status(fx1_eth_mdio_write(&handle->mac, phy_address, reg, value));
}

static void eth_bus_bind(neon_eth_handle_t *handle, eth_phy_bus_t *bus) {
    bus->context = handle;
    bus->read = eth_bus_read;
    bus->write = eth_bus_write;
    bus->delay_us = eth_delay_us;
}

/* MDIO runs only with ETH_CTRL[0] and [3] released, else START is dropped silently (MAS Table 6-42
 * row 6). Releasing them leaves every DMA/MAC block in reset; eth_init() releases the rest. */
static void eth_release_mdio(void) {
    const uint32_t mdio_up = ETH_CTRL_RST_N_MASK | ETH_CTRL_MDIO_RST_N_MASK;
    const uint32_t ctrl = fx1_read32(ETH_CSR(ETH_CTRL));
    if ((ctrl & mdio_up) != mdio_up) {
        fx1_write32(ETH_CSR(ETH_CTRL), ctrl | mdio_up);
    }
}

static neon_status_t eth_find_phy(neon_eth_handle_t *handle) {
    if (handle->phy_found != 0U) {
        return NEON_OK;
    }
    eth_phy_bus_t bus;
    eth_bus_bind(handle, &bus);
    neon_status_t status = eth_phy_find(&bus, handle->config.phy_address, &handle->phy);
    if (status == NEON_OK) {
        handle->phy_found = 1U;
    }
    return status;
}

static neon_status_t eth_prepare_phy(neon_eth_handle_t *handle) {
    eth_release_mdio();
    neon_status_t status = eth_find_phy(handle);
    if (status != NEON_OK) {
        return status;
    }
    eth_phy_bus_t bus;
    eth_bus_bind(handle, &bus);
    const eth_phy_mode_t mode = {
        .autonegotiation = handle->config.autonegotiation,
        .speed_mbps = handle->config.speed_mbps,
        .loopback = handle->config.phy_loopback,
    };
    return eth_phy_setup(&bus, &handle->phy, &mode);
}

/* MAS Table 9-7 (ETH_RMII_CFG): MAC enables off, >= 13 RMII cycles, BUSY = 0, rate, enables on.
 * The RMII_CFG write is refused (store fault on the VP) unless both MACs are disabled and idle,
 * so on timeout the rate is left unchanged. */
static neon_status_t eth_switch_rate(uint32_t speed100) {
    const uint32_t busy = ETH_STATUS_TX_BUSY_MASK | ETH_STATUS_RX_BUSY_MASK;
    neon_status_t status = NEON_TIMEOUT;
    fx1_write32(ETH_CSR(ETH_RX_MAC_CFG), 0U);
    fx1_write32(ETH_CSR(ETH_TX_MAC_CFG), 0U);
    eth_delay_us(NULL, 1U);
    for (uint32_t waited_us = 0U; waited_us <= ETH_RATE_IDLE_TIMEOUT_US; ++waited_us) {
        if ((fx1_read32(ETH_CSR(ETH_STATUS)) & busy) == 0U) {
            fx1_write32(ETH_CSR(ETH_RMII_CFG), speed100 != 0U ? ETH_RMII_CFG_SPEED_MASK : 0U);
            status = NEON_OK;
            break;
        }
        eth_delay_us(NULL, 1U);
    }
    fx1_write32(ETH_CSR(ETH_RX_MAC_CFG), ETH_RX_MAC_CFG_RX_EN_MASK);
    fx1_write32(ETH_CSR(ETH_TX_MAC_CFG), ETH_TX_MAC_CFG_TX_EN_MASK);
    return status;
}

static void eth_apply_config(neon_eth_handle_t *handle) {
    for (uint32_t i = 0U; i < 6U; ++i) {
        handle->mac.cfg.mac[i] = handle->config.mac_address[i];
    }
    handle->mac.speed100 = handle->config.speed_mbps == 100U ? 1 : 0;
    handle->mac.cfg.phy_loopback = (int)handle->config.phy_loopback;
    /* The PHY belongs to this layer; eth_init() must not rewrite BMCR. */
    handle->mac.cfg.phy_addr = -1;
}
