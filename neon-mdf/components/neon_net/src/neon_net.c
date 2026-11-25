/**
 * @file    neon_net.c
 * @brief   Neon network manager: lwIP tcpip thread, neon_eth ownership, link and IPv4 tracking.
 * @details Threading: every neon_eth call except the IRQ handler runs with the lwIP core lock held,
 *          either in the tcpip thread (timers, the IRQ-scheduled service) or in a task through
 *          LOCK_TCPIP_CORE() (this API, lwIP sockets reaching linkoutput). The PLIC handler only takes
 *          the MAC status (neon_eth_irq_handler) and posts one service message to the tcpip mailbox.
 *          The driver's returned poll interval and the PHY link poll are lwIP sys_timeouts.
 */

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include "neon_net.h"
#include "neon_net_internal.h"
#include "neon_net_port_internal.h"
#include "neon_freertos_fx1.h"
#include "task.h"
#include "lwip/dhcp.h"
#include "lwip/dns.h"
#include "lwip/ip4_addr.h"
#include "lwip/timeouts.h"
#include <stddef.h>
#include <string.h>

/*==================================================================================================
*                                       DEFINES AND MACROS
==================================================================================================*/
#define NET_IRQ_PRIORITY 2U
/* TX drain budget of a stop: a full ring of 1514-byte frames at 10 Mbps is about 20 ms. */
#define NET_STOP_DRAIN_US 25000U

/*==================================================================================================
*                                        LOCAL VARIABLES
==================================================================================================*/
static neon_net_handle_t net_handle;
static uint32_t net_tcpip_started;
static sys_sem_t net_tcpip_ready;

/*==================================================================================================
*                                   PRIVATE FUNCTION PROTOTYPES
==================================================================================================*/
static int net_valid(const neon_net_handle_t *handle);
static void net_tcpip_init_done(void *arg);
static void net_service_message(void *arg);
static void net_service_timer(void *arg);
static void net_link_timer(void *arg);
static void net_link_poll(neon_net_handle_t *handle);
static void net_status_changed(struct netif *netif);
static void net_notify(neon_net_handle_t *handle, uint32_t events);
static BaseType_t net_irq(void *user);
static int net_ipv4_nonzero(const uint8_t address[4]);
static int net_terminated(const char *text, uint32_t size_bytes);
static void net_copy_ipv4(uint8_t out[4], const ip4_addr_t *address);

/*==================================================================================================
*                                         GLOBAL FUNCTIONS
==================================================================================================*/
/** @copydoc neon_net_get_default_config */
neon_status_t neon_net_get_default_config(neon_net_config_t *config) {
    if (config == NULL) {
        return NEON_INVALID_PARAM;
    }
    memset(config, 0, sizeof(*config));
    (void)neon_eth_get_default_config(&config->eth);
    config->ipv4_mode = NEON_NET_IPV4_DHCP;
    return NEON_OK;
}

/** @copydoc neon_net_init */
neon_status_t neon_net_init(neon_net_handle_t **handle, neon_net_t instance) {
    if (handle == NULL || instance != NEON_NET0 || xTaskGetSchedulerState() != taskSCHEDULER_RUNNING) {
        return NEON_INVALID_PARAM;
    }
    if (net_handle.acquired != 0U) {
        return NEON_BUSY;
    }
    if (net_tcpip_started == 0U) {
        if (sys_sem_new(&net_tcpip_ready, 0U) != ERR_OK) {
            return NEON_ERROR;
        }
        tcpip_init(net_tcpip_init_done, NULL);
        (void)sys_arch_sem_wait(&net_tcpip_ready, 0U);
        net_tcpip_started = 1U;
    }
    if (net_handle.service_msg == NULL) {
        net_handle.service_msg = tcpip_callbackmsg_new(net_service_message, &net_handle);
        if (net_handle.service_msg == NULL || sys_sem_new(&net_handle.ready_sem, 0U) != ERR_OK) {
            return NEON_ERROR;
        }
    }
    LOCK_TCPIP_CORE();
    neon_status_t status = neon_eth_init(&net_handle.eth, NEON_ETH0);
    if (status == NEON_OK) {
        (void)neon_net_get_default_config(&net_handle.config);
        net_handle.callback = NULL;
        net_handle.callback_context = NULL;
        memset(&net_handle.counters, 0, sizeof(net_handle.counters));
        net_handle.state = NEON_NET_STATE_READY;
        net_handle.acquired = 1U;
        *handle = &net_handle;
    }
    UNLOCK_TCPIP_CORE();
    return status;
}

/** @copydoc neon_net_set_config */
neon_status_t neon_net_set_config(neon_net_handle_t *handle, const neon_net_config_t *config) {
    if (!net_valid(handle) || config == NULL ||
        (config->ipv4_mode != NEON_NET_IPV4_DHCP && config->ipv4_mode != NEON_NET_IPV4_STATIC) ||
        !net_terminated(config->hostname, sizeof(config->hostname))) {
        return NEON_INVALID_PARAM;
    }
    if (config->ipv4_mode == NEON_NET_IPV4_STATIC &&
        (!net_ipv4_nonzero(config->ipv4_address) || !net_ipv4_nonzero(config->ipv4_netmask))) {
        return NEON_INVALID_PARAM;
    }
#if !LWIP_DHCP
    if (config->ipv4_mode == NEON_NET_IPV4_DHCP) {
        return NEON_NOT_SUPPORTED;
    }
#endif
    LOCK_TCPIP_CORE();
    neon_status_t status = NEON_BUSY;
    if (handle->state == NEON_NET_STATE_READY) {
        /* The Ethernet handle is in its initial state here (stop re-acquires it). */
        status = neon_eth_set_config(handle->eth, &config->eth);
        if (status == NEON_OK) {
            handle->config = *config;
        }
    }
    UNLOCK_TCPIP_CORE();
    return status;
}

/** @copydoc neon_net_register_callback */
neon_status_t neon_net_register_callback(neon_net_handle_t *handle, neon_net_event_callback_t callback, void *context) {
    if (!net_valid(handle)) {
        return NEON_INVALID_PARAM;
    }
    LOCK_TCPIP_CORE();
    handle->callback = callback;
    handle->callback_context = context;
    UNLOCK_TCPIP_CORE();
    return NEON_OK;
}

/** @copydoc neon_net_start */
neon_status_t neon_net_start(neon_net_handle_t *handle) {
    if (!net_valid(handle)) {
        return NEON_INVALID_PARAM;
    }
    LOCK_TCPIP_CORE();
    if (handle->state != NEON_NET_STATE_READY) {
        UNLOCK_TCPIP_CORE();
        return NEON_BUSY;
    }
    neon_net_port_seed(handle->config.eth.mac_address, sizeof(handle->config.eth.mac_address));
    neon_status_t status = neon_eth_register_callback(handle->eth, neon_net_netif_receive, handle);
    if (status == NEON_OK) {
        status = neon_eth_start(handle->eth);
    }
    if (status != NEON_OK) {
        if (neon_eth_get_state(handle->eth) == NEON_ETH_STATE_FAULTED) {
            handle->state = NEON_NET_STATE_FAULTED;
        }
        UNLOCK_TCPIP_CORE();
        return status;
    }
    ip4_addr_t address;
    ip4_addr_t netmask;
    ip4_addr_t gateway;
    ip4_addr_set_zero(&address);
    ip4_addr_set_zero(&netmask);
    ip4_addr_set_zero(&gateway);
    if (handle->config.ipv4_mode == NEON_NET_IPV4_STATIC) {
        const neon_net_config_t *c = &handle->config;
        IP4_ADDR(&address, c->ipv4_address[0], c->ipv4_address[1], c->ipv4_address[2], c->ipv4_address[3]);
        IP4_ADDR(&netmask, c->ipv4_netmask[0], c->ipv4_netmask[1], c->ipv4_netmask[2], c->ipv4_netmask[3]);
        IP4_ADDR(&gateway, c->ipv4_gateway[0], c->ipv4_gateway[1], c->ipv4_gateway[2], c->ipv4_gateway[3]);
    }
    handle->link_up = 0U;
    handle->ipv4_up = 0U;
    handle->speed_mbps = handle->config.eth.speed_mbps;
    handle->service_pending = 0U;
    (void)netif_add(&handle->netif, &address, &netmask, &gateway, handle, neon_net_netif_init, tcpip_input);
    netif_set_status_callback(&handle->netif, net_status_changed);
    netif_set_default(&handle->netif);
    netif_set_up(&handle->netif);
#if LWIP_DNS
    if (handle->config.ipv4_mode == NEON_NET_IPV4_STATIC && net_ipv4_nonzero(handle->config.dns_server)) {
        ip_addr_t dns;
        const uint8_t *d = handle->config.dns_server;
        IP_ADDR4(&dns, d[0], d[1], d[2], d[3]);
        dns_setserver(0U, &dns);
    }
#endif
#if LWIP_DHCP
    if (handle->config.ipv4_mode == NEON_NET_IPV4_DHCP) {
        /* Waits in INIT until netif_set_link_up() from the link poll. */
        (void)dhcp_start(&handle->netif);
    }
#endif
    handle->state = NEON_NET_STATE_RUNNING;
    (void)NEON_FX1_IRQ_Register((uint32_t)FX1_IRQ_ETH, NET_IRQ_PRIORITY, net_irq, handle);
    net_link_poll(handle);
    neon_net_service(handle);
    UNLOCK_TCPIP_CORE();
    return NEON_OK;
}

/** @copydoc neon_net_wait_ready */
neon_status_t neon_net_wait_ready(neon_net_handle_t *handle, uint32_t timeout_ms) {
    if (!net_valid(handle)) {
        return NEON_INVALID_PARAM;
    }
    const uint32_t start_ms = sys_now();
    for (;;) {
        LOCK_TCPIP_CORE();
        const neon_net_state_t state = handle->state;
        const uint32_t ready = handle->link_up != 0U && handle->ipv4_up != 0U;
        UNLOCK_TCPIP_CORE();
        if (state != NEON_NET_STATE_RUNNING) {
            return NEON_BUSY;
        }
        if (ready) {
            return NEON_OK;
        }
        const uint32_t elapsed_ms = sys_now() - start_ms;
        if (elapsed_ms >= timeout_ms) {
            return NEON_TIMEOUT;
        }
        (void)sys_arch_sem_wait(&handle->ready_sem, timeout_ms - elapsed_ms);
    }
}

/** @copydoc neon_net_get_info */
neon_status_t neon_net_get_info(neon_net_handle_t *handle, neon_net_info_t *info) {
    if (!net_valid(handle) || info == NULL) {
        return NEON_INVALID_PARAM;
    }
    memset(info, 0, sizeof(*info));
    LOCK_TCPIP_CORE();
    info->link_up = handle->link_up;
    info->speed_mbps = handle->speed_mbps;
    info->ipv4_up = handle->ipv4_up;
    memcpy(info->mac_address, handle->config.eth.mac_address, sizeof(info->mac_address));
    if (handle->state == NEON_NET_STATE_RUNNING) {
        net_copy_ipv4(info->ipv4_address, netif_ip4_addr(&handle->netif));
        net_copy_ipv4(info->ipv4_netmask, netif_ip4_netmask(&handle->netif));
        net_copy_ipv4(info->ipv4_gateway, netif_ip4_gw(&handle->netif));
    }
    UNLOCK_TCPIP_CORE();
    return NEON_OK;
}

/** @copydoc neon_net_get_stats */
neon_status_t neon_net_get_stats(neon_net_handle_t *handle, neon_net_stats_t *stats) {
    if (!net_valid(handle) || stats == NULL) {
        return NEON_INVALID_PARAM;
    }
    LOCK_TCPIP_CORE();
    *stats = handle->counters;
    neon_status_t status = neon_eth_get_stats(handle->eth, &stats->eth);
    UNLOCK_TCPIP_CORE();
    return status;
}

/** @copydoc neon_net_get_state */
neon_net_state_t neon_net_get_state(neon_net_handle_t *handle) {
    if (!net_valid(handle)) {
        return NEON_NET_STATE_RESET;
    }
    LOCK_TCPIP_CORE();
    neon_net_state_t state = handle->state;
    UNLOCK_TCPIP_CORE();
    return state;
}

/** @copydoc neon_net_stop */
neon_status_t neon_net_stop(neon_net_handle_t *handle) {
    if (!net_valid(handle)) {
        return NEON_INVALID_PARAM;
    }
    LOCK_TCPIP_CORE();
    if (handle->state == NEON_NET_STATE_READY) {
        UNLOCK_TCPIP_CORE();
        return NEON_OK;
    }
    if (handle->state == NEON_NET_STATE_RUNNING) {
        sys_untimeout(net_service_timer, handle);
        sys_untimeout(net_link_timer, handle);
#if LWIP_DHCP
        dhcp_stop(&handle->netif);
#endif
        netif_set_down(&handle->netif);
        netif_remove(&handle->netif);
        handle->link_up = 0U;
        handle->ipv4_up = 0U;
    }
    SDK_IRQ_Disable((int32_t)FX1_IRQ_ETH);
    if (neon_eth_get_state(handle->eth) == NEON_ETH_STATE_FAULTED) {
        (void)neon_eth_recover(handle->eth);
    }
    neon_status_t status = neon_eth_stop(handle->eth, NET_STOP_DRAIN_US);
    if (status == NEON_OK) {
        /* Re-acquire so the next start is a cold init that applies a new configuration. */
        status = neon_eth_deinit(handle->eth);
        if (status == NEON_OK) {
            status = neon_eth_init(&handle->eth, NEON_ETH0);
        }
        if (status == NEON_OK) {
            status = neon_eth_set_config(handle->eth, &handle->config.eth);
        }
    }
    handle->state = status == NEON_OK ? NEON_NET_STATE_READY : NEON_NET_STATE_FAULTED;
    UNLOCK_TCPIP_CORE();
    return status == NEON_OK ? NEON_OK : NEON_TIMEOUT;
}

/** @copydoc neon_net_deinit */
neon_status_t neon_net_deinit(neon_net_handle_t *handle) {
    if (!net_valid(handle)) {
        return NEON_INVALID_PARAM;
    }
    LOCK_TCPIP_CORE();
    neon_status_t status = NEON_BUSY;
    if (handle->state == NEON_NET_STATE_READY) {
        status = neon_eth_deinit(handle->eth);
        if (status == NEON_OK) {
            handle->eth = NULL;
            handle->state = NEON_NET_STATE_RESET;
            handle->acquired = 0U;
        }
    }
    UNLOCK_TCPIP_CORE();
    return status;
}

/** @copydoc neon_net_service */
void neon_net_service(neon_net_handle_t *handle) {
    if (handle->state != NEON_NET_STATE_RUNNING) {
        return;
    }
    /* Clear first: an interrupt during the pass posts the message again. */
    handle->service_pending = 0U;
    uint32_t next_poll_ms = 0U;
    neon_status_t status = neon_eth_process(handle->eth, &next_poll_ms);
    if (status == NEON_ERROR) {
        /* FAULTED: a stop or recovery did not reach quiescence. One retry per pass. */
        if (neon_eth_recover(handle->eth) != NEON_OK) {
            handle->state = NEON_NET_STATE_FAULTED;
            handle->link_up = 0U;
            netif_set_link_down(&handle->netif);
            net_notify(handle, NEON_NET_EVENT_LINK_DOWN);
            return;
        }
        (void)neon_eth_process(handle->eth, &next_poll_ms);
    }
    sys_untimeout(net_service_timer, handle);
    if (next_poll_ms != 0U) {
        sys_timeout(next_poll_ms, net_service_timer, handle);
    }
}

/*==================================================================================================
*                                        PRIVATE FUNCTIONS
==================================================================================================*/
static int net_valid(const neon_net_handle_t *handle) {
    return handle == &net_handle && net_handle.acquired != 0U;
}

static void net_tcpip_init_done(void *arg) {
    (void)arg;
    sys_sem_signal(&net_tcpip_ready);
}

static void net_service_message(void *arg) {
    neon_net_service((neon_net_handle_t *)arg);
}

static void net_service_timer(void *arg) {
    neon_net_service((neon_net_handle_t *)arg);
}

static void net_link_timer(void *arg) {
    neon_net_handle_t *handle = (neon_net_handle_t *)arg;
    if (handle->state == NEON_NET_STATE_RUNNING) {
        net_link_poll(handle);
    }
}

/* PHY link poll: the MAC rate follows autonegotiation before lwIP sees the link up. */
static void net_link_poll(neon_net_handle_t *handle) {
    neon_eth_link_t link;
    if (neon_eth_get_link(handle->eth, &link) == NEON_OK) {
        if (link.up != 0U && link.speed_mbps != handle->speed_mbps) {
            if (neon_eth_set_speed(handle->eth, link.speed_mbps) == NEON_OK) {
                handle->speed_mbps = link.speed_mbps;
            } else {
                ++handle->counters.rate_errors;
                link.up = 0U;
            }
        }
        if (link.up != 0U && handle->link_up == 0U) {
            handle->link_up = 1U;
            ++handle->counters.link_changes;
            netif_set_link_up(&handle->netif);
            net_notify(handle, NEON_NET_EVENT_LINK_UP);
            net_status_changed(&handle->netif);
        } else if (link.up == 0U && handle->link_up != 0U) {
            handle->link_up = 0U;
            ++handle->counters.link_changes;
            netif_set_link_down(&handle->netif);
            net_notify(handle, NEON_NET_EVENT_LINK_DOWN);
            net_status_changed(&handle->netif);
        }
    }
    sys_timeout(CONFIG_NEON_MDF_NET_LINK_POLL_MS, net_link_timer, handle);
}

/* IPv4 is "up" while the interface is up, linked and has a nonzero address. */
static void net_status_changed(struct netif *netif) {
    neon_net_handle_t *handle = (neon_net_handle_t *)netif->state;
    const uint32_t up = netif_is_up(netif) && handle->link_up != 0U && !ip4_addr_isany_val(*netif_ip4_addr(netif));
    if (up != handle->ipv4_up) {
        handle->ipv4_up = up;
        net_notify(handle, up != 0U ? NEON_NET_EVENT_IPV4_UP : NEON_NET_EVENT_IPV4_DOWN);
        if (up != 0U) {
            sys_sem_signal(&handle->ready_sem);
        }
    }
}

static void net_notify(neon_net_handle_t *handle, uint32_t events) {
    if (handle->callback != NULL) {
        handle->callback(handle->callback_context, events);
    }
}

/* PLIC source 6. Take/ack the MAC status (lines fall before PLIC complete), post the service once. */
static BaseType_t net_irq(void *user) {
    neon_net_handle_t *handle = (neon_net_handle_t *)user;
    (void)neon_eth_irq_handler(handle->eth);
    if (handle->service_pending != 0U) {
        return pdFALSE;
    }
    handle->service_pending = 1U;
    err_t err = tcpip_callbackmsg_trycallback_fromisr(handle->service_msg);
    if (err == ERR_NEED_SCHED) {
        return pdTRUE;
    }
    if (err != ERR_OK) {
        /* Mailbox full: the driver's poll timer still runs the service. */
        handle->service_pending = 0U;
    }
    return pdFALSE;
}

static int net_ipv4_nonzero(const uint8_t address[4]) {
    return (address[0] | address[1] | address[2] | address[3]) != 0U;
}

/* The FX1 image has no memchr. */
static int net_terminated(const char *text, uint32_t size_bytes) {
    for (uint32_t i = 0U; i < size_bytes; ++i) {
        if (text[i] == '\0') {
            return 1;
        }
    }
    return 0;
}

static void net_copy_ipv4(uint8_t out[4], const ip4_addr_t *address) {
    out[0] = ip4_addr1(address);
    out[1] = ip4_addr2(address);
    out[2] = ip4_addr3(address);
    out[3] = ip4_addr4(address);
}
