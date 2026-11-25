/**
 * @file    neon_net_internal.h
 * @brief   @internal Manager state shared by neon_net.c and the lwIP netif glue.
 */
#ifndef __NEON_NET_INTERNAL_H__
#define __NEON_NET_INTERNAL_H__

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include "neon_net.h"
#include "lwip/netif.h"
#include "lwip/sys.h"
#include "lwip/tcpip.h"

#ifdef __cplusplus
extern "C" {
#endif

/*==================================================================================================
*                                  STRUCTURES AND OTHER TYPEDEFS
==================================================================================================*/
struct neon_net_handle_s {
    uint32_t acquired;
    neon_net_state_t state;
    neon_net_config_t config;
    neon_eth_handle_t *eth;
    struct netif netif;
    uint32_t netif_added;
    neon_net_event_callback_t callback;
    void *callback_context;
    uint32_t link_up;
    uint32_t speed_mbps;
    uint32_t ipv4_up;
    sys_sem_t ready_sem;
    struct tcpip_callback_msg *service_msg;
    volatile uint32_t service_pending;
    neon_net_stats_t counters; /**< eth member unused; driver counters are read on demand. */
    uint8_t tx_frame[NEON_ETH_FRAME_MAX_BYTES];
};

/*==================================================================================================
*                                       FUNCTION PROTOTYPES
==================================================================================================*/
/**
 * @brief netif_add() init function: Ethernet interface on the manager's neon_eth handle.
 * @param[in,out] netif Interface whose state is the manager handle.
 * @return ERR_OK.
 */
err_t neon_net_netif_init(struct netif *netif);

/**
 * @brief neon_eth RX callback: copy the frame into a pbuf and queue it to the tcpip thread.
 * @param[in] context Manager handle.
 * @param[in] frame DA..payload without FCS, borrowed.
 * @param[in] size_bytes Frame size.
 * @param[in] descriptor_status RX DES0.
 */
void neon_net_netif_receive(void *context, const uint8_t *frame, uint32_t size_bytes, uint32_t descriptor_status);

/**
 * @brief Run the driver service once and re-arm its timer (core lock held).
 * @param[in,out] handle Manager handle.
 */
void neon_net_service(neon_net_handle_t *handle);

#ifdef __cplusplus
}
#endif
#endif /* __NEON_NET_INTERNAL_H__ */
