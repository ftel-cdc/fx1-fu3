/**
 * @file    neon_net_netif.c
 * @brief   lwIP Ethernet netif on neon_eth.
 * @details Runs in the tcpip thread or under the core lock, the serialized owner neon_eth needs.
 *          TX copies the pbuf chain into one frame (neon_eth copies it again into its DMA slot);
 *          RX copies the borrowed frame into a PBUF_POOL buffer and queues it through tcpip_input,
 *          so a frame delivered while linkoutput reclaims the ring never reenters the stack.
 */

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include "neon_net_internal.h"
#include "lwip/etharp.h"
#include "lwip/pbuf.h"
#include "lwip/snmp.h"
#include "lwip/stats.h"
#include "netif/ethernet.h"
#include <stddef.h>

/*==================================================================================================
*                                       DEFINES AND MACROS
==================================================================================================*/
#define NETIF_MTU_BYTES 1500U

/*==================================================================================================
*                                   PRIVATE FUNCTION PROTOTYPES
==================================================================================================*/
static err_t netif_link_output(struct netif *netif, struct pbuf *p);

/*==================================================================================================
*                                         GLOBAL FUNCTIONS
==================================================================================================*/
/** @copydoc neon_net_netif_init */
err_t neon_net_netif_init(struct netif *netif) {
    neon_net_handle_t *handle = (neon_net_handle_t *)netif->state;
    netif->name[0] = 'e';
    netif->name[1] = '0';
#if LWIP_NETIF_HOSTNAME
    netif->hostname = handle->config.hostname[0] != '\0' ? handle->config.hostname : NULL;
#endif
    netif->output = etharp_output;
    netif->linkoutput = netif_link_output;
    netif->mtu = (u16_t)NETIF_MTU_BYTES;
    netif->hwaddr_len = ETH_HWADDR_LEN;
    for (uint32_t i = 0U; i < ETH_HWADDR_LEN; ++i) {
        netif->hwaddr[i] = handle->config.eth.mac_address[i];
    }
    netif->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_ETHERNET;
    MIB2_INIT_NETIF(netif, snmp_ifType_ethernet_csmacd, 100000000U);
    return ERR_OK;
}

/** @copydoc neon_net_netif_receive */
void neon_net_netif_receive(void *context, const uint8_t *frame, uint32_t size_bytes, uint32_t descriptor_status) {
    neon_net_handle_t *handle = (neon_net_handle_t *)context;
    if ((descriptor_status & NEON_ETH_RX_STATUS_ERROR_MASK) != 0U || size_bytes < SIZEOF_ETH_HDR) {
        /* Truncated or DMA error: the driver counted it; it is not a valid packet. */
        LINK_STATS_INC(link.drop);
        return;
    }
    struct pbuf *p = pbuf_alloc(PBUF_RAW, (u16_t)(size_bytes + ETH_PAD_SIZE), PBUF_POOL);
    if (p == NULL) {
        ++handle->counters.rx_no_buffer;
        LINK_STATS_INC(link.memerr);
        return;
    }
    (void)pbuf_take_at(p, frame, (u16_t)size_bytes, ETH_PAD_SIZE);
    LINK_STATS_INC(link.recv);
    if (handle->netif.input(p, &handle->netif) != ERR_OK) {
        ++handle->counters.rx_queue_full;
        LINK_STATS_INC(link.drop);
        pbuf_free(p);
    }
}

/*==================================================================================================
*                                        PRIVATE FUNCTIONS
==================================================================================================*/
static err_t netif_link_output(struct netif *netif, struct pbuf *p) {
    neon_net_handle_t *handle = (neon_net_handle_t *)netif->state;
    LWIP_ASSERT_CORE_LOCKED();
    if ((uint32_t)p->tot_len <= ETH_PAD_SIZE || (uint32_t)p->tot_len - ETH_PAD_SIZE > NEON_ETH_FRAME_MAX_BYTES) {
        ++handle->counters.tx_errors;
        LINK_STATS_INC(link.err);
        return ERR_IF;
    }
    const uint32_t size_bytes = (uint32_t)p->tot_len - ETH_PAD_SIZE;
    (void)pbuf_copy_partial(p, handle->tx_frame, (u16_t)size_bytes, ETH_PAD_SIZE);
    neon_status_t status = neon_eth_transmit(handle->eth, handle->tx_frame, size_bytes);
    if (status == NEON_BUSY) {
        /* Ring full: reclaim written-back descriptors, then try once more. */
        neon_net_service(handle);
        status = neon_eth_transmit(handle->eth, handle->tx_frame, size_bytes);
    }
    if (status != NEON_OK) {
        if (status == NEON_BUSY) {
            ++handle->counters.tx_ring_full;
        } else {
            ++handle->counters.tx_errors;
        }
        LINK_STATS_INC(link.drop);
        return status == NEON_BUSY ? ERR_MEM : ERR_IF;
    }
    LINK_STATS_INC(link.xmit);
    return ERR_OK;
}
