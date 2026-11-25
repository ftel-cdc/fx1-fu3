/**
 * @file    neon_net.h
 * @brief   Neon network manager: one Ethernet interface on lwIP 2.2.1 and FreeRTOS.
 * @details The manager owns the lwIP tcpip thread, the neon_eth handle and the lwIP netif. Every
 *          driver call runs in the tcpip thread or under the lwIP core lock, which is the single
 *          serialized owner neon_eth requires. The PHY link is polled; on link up the MAC rate
 *          follows the PHY and DHCP (or the static address) brings IPv4 up. Applications then use the
 *          lwIP socket (`lwip_*`), netconn or raw APIs. All functions are for FreeRTOS tasks on
 *          hart 0 after the scheduler started; none is ISR-safe. One instance (NEON_NET0).
 */
#ifndef __NEON_NET_H__
#define __NEON_NET_H__

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include "neon_types.h"
#include "eth/neon_eth.h"

#ifdef __cplusplus
extern "C" {
#endif

/*==================================================================================================
*                                       DEFINES AND MACROS
==================================================================================================*/
/** Longest DHCP host name kept by the manager, without the terminator. */
#define NEON_NET_HOSTNAME_MAX_BYTES 31U

/** Event bits passed to the event callback. */
#define NEON_NET_EVENT_LINK_UP (1UL << 0)
#define NEON_NET_EVENT_LINK_DOWN (1UL << 1)
#define NEON_NET_EVENT_IPV4_UP (1UL << 2)
#define NEON_NET_EVENT_IPV4_DOWN (1UL << 3)

/*==================================================================================================
*                                  STRUCTURES AND OTHER TYPEDEFS
==================================================================================================*/
typedef struct neon_net_handle_s neon_net_handle_t;

typedef enum { NEON_NET0 = 0 } neon_net_t;

typedef enum {
    NEON_NET_STATE_RESET = 0,
    NEON_NET_STATE_READY,   /**< Initialized or stopped. */
    NEON_NET_STATE_RUNNING, /**< Interface up; link/IPv4 may still be pending. */
    NEON_NET_STATE_FAULTED  /**< Ethernet driver faulted; stop then start again. */
} neon_net_state_t;

typedef enum { NEON_NET_IPV4_DHCP = 0, NEON_NET_IPV4_STATIC = 1 } neon_net_ipv4_mode_t;

/** Interface settings. Begin from neon_net_get_default_config(). Addresses are in wire order. */
typedef struct neon_net_config_s {
    neon_eth_config_t eth;                           /**< MAC address, PHY address, autonegotiation, rate. */
    neon_net_ipv4_mode_t ipv4_mode;                  /**< DHCP needs CONFIG_NEON_MDF_NET_DHCP. */
    uint8_t ipv4_address[4];                         /**< Static mode only. */
    uint8_t ipv4_netmask[4];                         /**< Static mode only. */
    uint8_t ipv4_gateway[4];                         /**< Static mode only; 0.0.0.0 for none. */
    uint8_t dns_server[4];                           /**< Static mode; with DHCP the server's option wins. */
    char hostname[NEON_NET_HOSTNAME_MAX_BYTES + 1U]; /**< DHCP option 12; empty for none. */
} neon_net_config_t;

/** Snapshot of the interface. */
typedef struct neon_net_info_s {
    uint32_t link_up;
    uint32_t speed_mbps;
    uint32_t ipv4_up;
    uint8_t mac_address[6];
    uint8_t ipv4_address[4];
    uint8_t ipv4_netmask[4];
    uint8_t ipv4_gateway[4];
} neon_net_info_t;

/** Counters of the manager and its Ethernet driver. */
typedef struct neon_net_stats_s {
    neon_eth_stats_t eth;   /**< Driver counters. */
    uint32_t rx_no_buffer;  /**< RX frames dropped: no lwIP pbuf. */
    uint32_t rx_queue_full; /**< RX frames dropped: tcpip mailbox full. */
    uint32_t tx_ring_full;  /**< TX frames dropped: descriptor ring still full after reclaim. */
    uint32_t tx_errors;     /**< TX frames refused by the driver otherwise. */
    uint32_t link_changes;  /**< Link up/down transitions seen by the poll. */
    uint32_t rate_errors;   /**< Failed MAC rate switches after autonegotiation. */
} neon_net_stats_t;

/**
 * @brief Event callback, called from the tcpip thread with the core lock held.
 * @param[in] context Opaque value given at registration.
 * @param[in] events  NEON_NET_EVENT_* bits that just happened.
 * @warning Must not block or call neon_net_*; may use lwIP raw APIs.
 */
typedef void (*neon_net_event_callback_t)(void *context, uint32_t events);

/*==================================================================================================
*                                       FUNCTION PROTOTYPES
==================================================================================================*/
/**
 * @brief Fill the default settings: neon_eth defaults, DHCP, no host name, no static addresses.
 * @param[out] config Settings to fill.
 * @return NEON_OK or NEON_INVALID_PARAM.
 */
neon_status_t neon_net_get_default_config(neon_net_config_t *config);
/**
 * @brief Start the lwIP tcpip thread once and acquire the Ethernet driver.
 * @param[out] handle Receives the manager handle.
 * @param[in] instance NEON_NET0.
 * @return NEON_OK, NEON_BUSY if already acquired, NEON_ERROR or NEON_INVALID_PARAM.
 * @note Task context, scheduler running. The tcpip thread is never deleted (lwIP has no deinit).
 */
neon_status_t neon_net_init(neon_net_handle_t **handle, neon_net_t instance);
/**
 * @brief Copy interface settings; applied by the next start.
 * @param[in,out] handle Initialized handle, not running.
 * @param[in] config Settings; rejected when inconsistent (static mode without address/netmask).
 * @return NEON_OK, NEON_BUSY while running, NEON_NOT_SUPPORTED (DHCP disabled) or NEON_INVALID_PARAM.
 */
neon_status_t neon_net_set_config(neon_net_handle_t *handle, const neon_net_config_t *config);
/**
 * @brief Register the event callback; NULL removes it.
 * @param[in,out] handle Initialized handle.
 * @param[in] callback Event callback.
 * @param[in] context Opaque value passed back.
 * @return NEON_OK or NEON_INVALID_PARAM.
 */
neon_status_t neon_net_register_callback(neon_net_handle_t *handle, neon_net_event_callback_t callback, void *context);
/**
 * @brief Start the MAC/PHY, add the lwIP interface and begin link polling and DHCP.
 * @param[in,out] handle Initialized handle.
 * @return NEON_OK, NEON_BUSY if running, NEON_ERROR/NEON_TIMEOUT from the driver, or NEON_INVALID_PARAM.
 * @note Returns before the link is up; use neon_net_wait_ready(). Holds the core lock during the
 *       PHY bring-up (up to about 600 ms of busy wait).
 */
neon_status_t neon_net_start(neon_net_handle_t *handle);
/**
 * @brief Wait until the link is up and the interface has an IPv4 address.
 * @param[in] handle Running handle.
 * @param[in] timeout_ms Maximum wait; 0 only checks.
 * @return NEON_OK, NEON_TIMEOUT, NEON_BUSY if not running, or NEON_INVALID_PARAM.
 */
neon_status_t neon_net_wait_ready(neon_net_handle_t *handle, uint32_t timeout_ms);
/**
 * @brief Read the interface state.
 * @param[in] handle Initialized handle.
 * @param[out] info Snapshot.
 * @return NEON_OK or NEON_INVALID_PARAM.
 */
neon_status_t neon_net_get_info(neon_net_handle_t *handle, neon_net_info_t *info);
/**
 * @brief Read manager and driver counters.
 * @param[in] handle Initialized handle.
 * @param[out] stats Snapshot.
 * @return NEON_OK or NEON_INVALID_PARAM.
 */
neon_status_t neon_net_get_stats(neon_net_handle_t *handle, neon_net_stats_t *stats);
/**
 * @brief Get the manager state.
 * @param[in] handle Handle.
 * @return Current state, RESET for an invalid handle.
 */
neon_net_state_t neon_net_get_state(neon_net_handle_t *handle);
/**
 * @brief Remove the interface from lwIP and stop the MAC/DMA.
 * @param[in,out] handle Initialized handle.
 * @return NEON_OK, NEON_TIMEOUT if the driver did not reach quiescence (FAULTED, retry later),
 *         or NEON_INVALID_PARAM.
 * @note Open sockets on the interface see their routes vanish; close them first.
 */
neon_status_t neon_net_stop(neon_net_handle_t *handle);
/**
 * @brief Release a stopped manager and its Ethernet handle.
 * @param[in,out] handle Initialized, stopped handle.
 * @return NEON_OK, NEON_BUSY while running, or NEON_INVALID_PARAM.
 */
neon_status_t neon_net_deinit(neon_net_handle_t *handle);

#ifdef __cplusplus
}
#endif
#endif /* __NEON_NET_H__ */
