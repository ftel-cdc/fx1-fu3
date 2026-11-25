/**
 * @file neon_eth.h
 * @brief Ethernet MAC frame I/O and PHY link management, independent of an OS or TCP/IP stack.
 * @details One serialized owner on hart 0 calls every API except irq_handler.
 * RX callbacks run inside process(), borrow the frame until return and must not
 * reenter the driver. TX copies the frame before returning. No heap allocation.
 * FX1 currently provides one instance, fixed 16-entry rings and 2048-byte slots.
 * The MAC is RMII 10/100 full duplex only; the PHY layer advertises nothing else.
 */
#ifndef __NEON_ETH_H__
#define __NEON_ETH_H__

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
/** Config phy_address value: use the lowest MDIO address (0..31) whose PHY answers. */
#define NEON_ETH_PHY_ADDRESS_SCAN 0xFFFFFFFFUL
/** Smallest frame accepted by transmit (DA, SA, EtherType); the MAC pads to 60 bytes + FCS. */
#define NEON_ETH_FRAME_MIN_BYTES 14U
/** Largest frame accepted by transmit and delivered by RX (DA..payload, no FCS). */
#define NEON_ETH_FRAME_MAX_BYTES 1514U

/** RX descriptor status (callback descriptor_status, DES0): error bits [18:16]. */
#define NEON_ETH_RX_STATUS_ERROR_MASK 0x00070000UL
/** RX checksum verdict bits of DES0 (MAS Table 6-29). Valid only when the CHECKED bit is set. */
#define NEON_ETH_RX_STATUS_IPV4_CHECKED (1UL << 25)
#define NEON_ETH_RX_STATUS_IPV4_ERROR (1UL << 24)
#define NEON_ETH_RX_STATUS_L4_CHECKED (1UL << 23)
#define NEON_ETH_RX_STATUS_L4_ERROR (1UL << 22)

/*==================================================================================================
*                                  STRUCTURES AND OTHER TYPEDEFS
==================================================================================================*/
typedef struct neon_eth_handle_s neon_eth_handle_t;

typedef enum { NEON_ETH0 = 0 } neon_eth_t;

typedef enum {
    NEON_ETH_STATE_RESET = 0,
    NEON_ETH_STATE_READY,
    NEON_ETH_STATE_RUNNING,
    NEON_ETH_STATE_STOPPING,
    NEON_ETH_STATE_FAULTED
} neon_eth_state_t;

/** Start-up settings. Begin from neon_eth_get_default_config(); unknown fields keep their default. */
typedef struct neon_eth_config_s {
    uint8_t mac_address[6];
    uint32_t speed_mbps;      /**< 10 or 100. Forced PHY/MAC rate, or the MAC rate until the link resolves. */
    uint32_t autonegotiation; /**< 1: advertise 10/100 full duplex; the owner follows neon_eth_get_link(). */
    uint32_t phy_address;     /**< 0..31, or NEON_ETH_PHY_ADDRESS_SCAN. */
    uint32_t phy_loopback;    /**< 0 or 1; forces speed_mbps, needs no line peer. */
} neon_eth_config_t;

typedef struct {
    uint32_t mac_id;
    uint32_t build_config;
    uint32_t phy_address;
    uint16_t phy_id1;
    uint16_t phy_id2;
    uint32_t link_up;
} neon_eth_info_t;

/** Link state of the PHY. */
typedef struct neon_eth_link_s {
    uint32_t up;          /**< 1 when the link is up (autonegotiation complete if enabled). */
    uint32_t speed_mbps;  /**< 10 or 100. */
    uint32_t full_duplex; /**< 0 means the partner only offers half duplex, which the MAC cannot run. */
} neon_eth_link_t;

typedef struct {
    uint32_t tx_frames; /**< Frames submitted, not proof of delivery on the line. */
    uint32_t rx_frames;
    uint32_t tx_errors;
    uint32_t rx_errors;
    uint32_t tx_dropped;
    uint32_t interrupts;
    uint32_t late_writebacks;
    uint32_t recoveries;
} neon_eth_stats_t;

typedef void (*neon_eth_rx_callback_t)(void *context, const uint8_t *frame, uint32_t size_bytes,
                                       uint32_t descriptor_status);

/*==================================================================================================
*                                       FUNCTION PROTOTYPES
==================================================================================================*/
/**
 * @brief Fill the default start-up settings.
 * @param[out] config Locally administered MAC 02:00:46:58:31:01, 100 Mbps, autonegotiation on,
 *             PHY address scan, no loopback.
 * @return NEON_OK or NEON_INVALID_PARAM.
 */
neon_status_t neon_eth_get_default_config(neon_eth_config_t *config);
/**
 * @brief Acquire a driver-owned handle with default settings, without hardware access.
 * @param[out] handle Receives the opaque handle. Valid until successful deinit.
 * @param[in] instance MAC instance, currently NEON_ETH0.
 * @return NEON_OK, NEON_BUSY if already acquired, or NEON_INVALID_PARAM.
 */
neon_status_t neon_eth_init(neon_eth_handle_t **handle, neon_eth_t instance);
/**
 * @brief Apply start-up settings before the first start.
 * @param[in,out] handle Acquired handle.
 * @param[in] config Unicast, nonzero MAC, 10/100 Mbps, valid PHY address; copied by driver.
 * @return NEON_OK, NEON_BUSY after start (deinit/init to reconfigure), or NEON_INVALID_PARAM.
 */
neon_status_t neon_eth_set_config(neon_eth_handle_t *handle, const neon_eth_config_t *config);
/**
 * @brief Register an RX callback; NULL disables delivery to the application.
 * @param[in,out] handle Acquired handle.
 * @param[in] callback Receives DA..payload without FCS and raw RX DES0 status.
 * @param[in] context Opaque application data, retained until replaced/deinit.
 * @return NEON_OK or NEON_INVALID_PARAM.
 */
neon_status_t neon_eth_register_callback(neon_eth_handle_t *handle, neon_eth_rx_callback_t callback, void *context);
/**
 * @brief Read MAC identity and find the PHY over bounded, polled MDIO transactions.
 * @param[in,out] handle Acquired handle.
 * @param[out] info Receives identity and link snapshot; invalid on error.
 * @return NEON_OK, NEON_BUSY in FAULTED state, NEON_ERROR (wrong MAC or no PHY) or NEON_INVALID_PARAM.
 * @note Releases only the MDIO block from reset when the MAC is still held.
 */
neon_status_t neon_eth_probe(neon_eth_handle_t *handle, neon_eth_info_t *info);
/**
 * @brief Start/restart descriptor DMA. The first start also finds and prepares the PHY.
 * @param[in,out] handle Acquired handle; FAULTED requires recover first.
 * @return NEON_OK, NEON_BUSY, NEON_TIMEOUT, NEON_ERROR or NEON_INVALID_PARAM.
 * @note Bounded busy waits (up to about 600 ms for a PHY soft reset), task/main context only.
 *       Driver preserves the PLIC enable. With autonegotiation the line rate is known only later:
 *       poll neon_eth_get_link() and apply it with neon_eth_set_speed().
 */
neon_status_t neon_eth_start(neon_eth_handle_t *handle);
/**
 * @brief Copy and queue one frame; completion is asynchronous.
 * @param[in,out] handle Running handle.
 * @param[in] frame DA..payload, no FCS; copied before return.
 * @param[in] size_bytes NEON_ETH_FRAME_MIN_BYTES..NEON_ETH_FRAME_MAX_BYTES; shorter than 60 is
 *            zero-padded by the MAC.
 * @return NEON_OK, NEON_BUSY if full/halted, or NEON_INVALID_PARAM.
 */
neon_status_t neon_eth_transmit(neon_eth_handle_t *handle, const uint8_t *frame, uint32_t size_bytes);
/**
 * @brief Service completion/errors, reclaim TX and deliver RX callbacks.
 * @param[in,out] handle Acquired handle.
 * @param[out] next_poll_ms Maximum delay before the next call even without IRQ;
 *             zero only when not running. Polling callers may call more often.
 * @return NEON_OK, NEON_ERROR if FAULTED, or NEON_INVALID_PARAM.
 * @note Call after IRQ notification and after TX; never from ISR.
 */
neon_status_t neon_eth_process(neon_eth_handle_t *handle, uint32_t *next_poll_ms);
/**
 * @brief Take/ack MAC status and mask active level errors, without waiting.
 * @param[in,out] handle Acquired handle.
 * @return NEON_OK or NEON_INVALID_PARAM.
 * @note Call in the application's PLIC handler before PLIC complete, or from
 * process() in polling mode. Does not install a trap or enable CPU/PLIC IRQs.
 */
neon_status_t neon_eth_irq_handler(neon_eth_handle_t *handle);
/**
 * @brief Read the PHY link state (two BMSR reads, then the autonegotiation result).
 * @param[in,out] handle Handle after a successful probe or start.
 * @param[out] link Link state.
 * @return NEON_OK, NEON_ERROR (no PHY found yet, MDIO failure), NEON_BUSY in FAULTED state,
 *         or NEON_INVALID_PARAM.
 * @note About 5 MDIO frames (26 us each at the reset MDC divider); task/main context only.
 */
neon_status_t neon_eth_get_link(neon_eth_handle_t *handle, neon_eth_link_t *link);
/**
 * @brief Set the MAC line rate (ETH_RMII_CFG) to match the PHY; does not reprogram the PHY.
 * @param[in,out] handle Acquired handle, READY or RUNNING.
 * @param[in] speed_mbps 10 or 100.
 * @return NEON_OK, NEON_TIMEOUT if the MACs do not go idle (rate unchanged, MACs re-enabled),
 *         NEON_BUSY while STOPPING/FAULTED, or NEON_INVALID_PARAM.
 * @note RUNNING: MAS Table 9-7 sequence; both MAC enables drop for up to about 3 ms, a frame being
 *       received is lost, DMA keeps running. Task/main context only.
 */
neon_status_t neon_eth_set_speed(neon_eth_handle_t *handle, uint32_t speed_mbps);
/**
 * @brief Read a Clause 22 PHY register (board bring-up, vendor registers).
 * @param[in,out] handle Acquired handle.
 * @param[in] phy_address 0..31.
 * @param[in] reg 0..31.
 * @param[out] value Register value.
 * @return NEON_OK, NEON_ERROR (no response), NEON_TIMEOUT, NEON_BUSY in FAULTED state,
 *         or NEON_INVALID_PARAM.
 */
neon_status_t neon_eth_mdio_read(neon_eth_handle_t *handle, uint32_t phy_address, uint32_t reg, uint16_t *value);
/**
 * @brief Write a Clause 22 PHY register.
 * @param[in,out] handle Acquired handle.
 * @param[in] phy_address 0..31.
 * @param[in] reg 0..31; BMCR (0) is refused while RUNNING/STOPPING.
 * @param[in] value Register value.
 * @return NEON_OK, NEON_ERROR, NEON_TIMEOUT, NEON_BUSY or NEON_INVALID_PARAM.
 */
neon_status_t neon_eth_mdio_write(neon_eth_handle_t *handle, uint32_t phy_address, uint32_t reg, uint16_t value);
/**
 * @brief Drain, halt DMA and stop the MAC before returning buffer ownership.
 * @param[in,out] handle Acquired handle.
 * @param[in] drain_us TX drain budget in microseconds; zero skips drain only.
 * @return NEON_OK when quiescent, NEON_TIMEOUT/NEON_BUSY if FAULTED, or NEON_INVALID_PARAM.
 * @warning Timeout retains DMA memory/handle. Recover before restart or deinit.
 */
neon_status_t neon_eth_stop(neon_eth_handle_t *handle, uint32_t drain_us);
/**
 * @brief Recover halted directions or retry quiescence from FAULTED.
 * @param[in,out] handle Acquired handle.
 * @return NEON_OK, NEON_TIMEOUT, NEON_ERROR or NEON_INVALID_PARAM.
 * @note Bounded waits, task/main context only; may restart a FAULTED MAC.
 */
neon_status_t neon_eth_recover(neon_eth_handle_t *handle);
/**
 * @brief Get a software state snapshot.
 * @param[in] handle Acquired handle.
 * @return Current state, RESET for an invalid/released handle.
 */
neon_eth_state_t neon_eth_get_state(const neon_eth_handle_t *handle);
/**
 * @brief Read driver counters under the owner's serialization.
 * @param[in] handle Acquired handle.
 * @param[out] stats Counter snapshot.
 * @return NEON_OK or NEON_INVALID_PARAM.
 */
neon_status_t neon_eth_get_stats(const neon_eth_handle_t *handle, neon_eth_stats_t *stats);
/**
 * @brief Release a stopped handle; never resets a live or faulted DMA.
 * @param[in,out] handle Acquired handle.
 * @return NEON_OK, NEON_BUSY until safely stopped, or NEON_INVALID_PARAM.
 */
neon_status_t neon_eth_deinit(neon_eth_handle_t *handle);

#ifdef __cplusplus
}
#endif
#endif /* __NEON_ETH_H__ */
