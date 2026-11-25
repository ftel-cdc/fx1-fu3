/**
 * @file main.c
 * @brief Check neon_net (lwIP 2.2.1 on FreeRTOS) end to end on the VP line loopback.
 * @details CONFIG_NEON_MDF_NET_LOOPBACK is off, so traffic to the interface's own address leaves
 *          through the MAC, DMA and PHY and comes back from `--eth-peer loopback`: ARP resolves the
 *          own address, ICMP echo, UDP (with IPv4 fragmentation) and a TCP echo run over the
 *          real driver. Then stop, restart and release.
 */
#include "neon_net.h"
#include "neon_freertos_fx1.h"
#include "task.h"
#include "lwip/etharp.h"
#include "lwip/inet_chksum.h"
#include "lwip/prot/icmp.h"
#include "lwip/sockets.h"
#include "lwip/tcpip.h"
#include <stddef.h>
#include <string.h>

#define TEST_UDP_PORT 5000U
#define TEST_TCP_PORT 7000U
#define TEST_TCP_BYTES 16384U
#define TEST_ICMP_ID 0xAFAFU
#define TEST_TIMEOUT_MS 5000U

static const uint8_t own_mac[6] = {0x02U, 0x00U, 0x46U, 0x58U, 0x31U, 0x01U};
static const uint8_t own_ip[4] = {192U, 168U, 10U, 2U};

static neon_net_handle_t *net;
static uint32_t checks;
static volatile uint32_t events_seen;
static TaskHandle_t test_task_handle;
static uint8_t tx_buffer[TEST_TCP_BYTES];
static uint8_t rx_buffer[TEST_TCP_BYTES];

static void check(int ok, const char *name) {
    ++checks;
    fx1_puts(ok ? "[NEON_NET] PASS " : "[NEON_NET] FAIL ");
    fx1_putdec(checks);
    fx1_puts(" ");
    fx1_puts(name);
    fx1_putc('\n');
    if (!ok) {
        fx1_fail(0xe000U + checks);
    }
}

static void log_ms(const char *what, uint64_t since) {
    fx1_puts("[NEON_NET] ");
    fx1_puts(what);
    fx1_puts(" took ");
    fx1_putdec((uint32_t)((fx1_mtime() - since) / (FX1_MTIME_HZ / 1000U)));
    fx1_puts(" ms (simulated)\n");
}

static void on_event(void *context, uint32_t events) {
    (void)context;
    events_seen |= events;
}

static void own_address(struct sockaddr_in *address, uint16_t port) {
    memset(address, 0, sizeof(*address));
    address->sin_len = (u8_t)sizeof(*address);
    address->sin_family = AF_INET;
    address->sin_port = lwip_htons(port);
    address->sin_addr.s_addr = PP_HTONL(LWIP_MAKEU32(own_ip[0], own_ip[1], own_ip[2], own_ip[3]));
}

static void set_timeout(int socket, uint32_t timeout_ms) {
    struct timeval tv = {.tv_sec = (time_t)(timeout_ms / 1000U),
                         .tv_usec = (suseconds_t)((timeout_ms % 1000U) * 1000U)};
    (void)lwip_setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
}

static void fill_pattern(uint8_t *buffer, uint32_t size_bytes, uint32_t seed) {
    for (uint32_t i = 0U; i < size_bytes; ++i) {
        buffer[i] = (uint8_t)(i * 31U + seed * 7U + (i >> 8));
    }
}

/* One ICMP echo to the own address; the raw socket also sees our request come back. */
static int ping_self(uint16_t sequence) {
    struct sockaddr_in to;
    uint8_t packet[sizeof(struct icmp_echo_hdr) + 32U];
    uint8_t reply[128];
    int s = lwip_socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (s < 0) {
        return 0;
    }
    set_timeout(s, TEST_TIMEOUT_MS);
    struct icmp_echo_hdr *echo = (struct icmp_echo_hdr *)packet;
    memset(packet, 0, sizeof(packet));
    ICMPH_TYPE_SET(echo, ICMP_ECHO);
    ICMPH_CODE_SET(echo, 0U);
    echo->id = lwip_htons(TEST_ICMP_ID);
    echo->seqno = lwip_htons(sequence);
    fill_pattern(packet + sizeof(*echo), 32U, sequence);
    echo->chksum = inet_chksum(packet, (u16_t)sizeof(packet));
    own_address(&to, 0U);
    int ok = lwip_sendto(s, packet, sizeof(packet), 0, (struct sockaddr *)&to, sizeof(to)) == (int)sizeof(packet);
    int answered = 0;
    while (ok && !answered) {
        int n = lwip_recv(s, reply, sizeof(reply), 0);
        if (n <= 0) {
            break;
        }
        uint32_t ihl = (uint32_t)(reply[0] & 0x0FU) * 4U;
        if ((uint32_t)n < ihl + sizeof(*echo)) {
            continue;
        }
        const struct icmp_echo_hdr *got = (const struct icmp_echo_hdr *)(reply + ihl);
        if (ICMPH_TYPE(got) == ICMP_ER && got->id == echo->id && got->seqno == echo->seqno &&
            (uint32_t)n == ihl + sizeof(packet) &&
            memcmp(reply + ihl + sizeof(*echo), packet + sizeof(*echo), 32U) == 0) {
            answered = 1;
        }
    }
    lwip_close(s);
    return ok && answered;
}

static int udp_self(uint32_t size_bytes, uint32_t seed) {
    struct sockaddr_in address;
    int rx = lwip_socket(AF_INET, SOCK_DGRAM, 0);
    int tx = lwip_socket(AF_INET, SOCK_DGRAM, 0);
    int ok = rx >= 0 && tx >= 0;
    own_address(&address, TEST_UDP_PORT);
    ok = ok && lwip_bind(rx, (struct sockaddr *)&address, sizeof(address)) == 0;
    set_timeout(rx, TEST_TIMEOUT_MS);
    fill_pattern(tx_buffer, size_bytes, seed);
    ok = ok &&
         lwip_sendto(tx, tx_buffer, size_bytes, 0, (struct sockaddr *)&address, sizeof(address)) == (int)size_bytes;
    ok = ok && lwip_recv(rx, rx_buffer, sizeof(rx_buffer), 0) == (int)size_bytes &&
         memcmp(tx_buffer, rx_buffer, size_bytes) == 0;
    if (rx >= 0) {
        lwip_close(rx);
    }
    if (tx >= 0) {
        lwip_close(tx);
    }
    return ok;
}

/* TCP echo server: one connection, echoes until the peer closes. */
static void tcp_echo_task(void *argument) {
    (void)argument;
    struct sockaddr_in address;
    static uint8_t echo_buffer[1460];
    int listener = lwip_socket(AF_INET, SOCK_STREAM, 0);
    own_address(&address, TEST_TCP_PORT);
    int ok = listener >= 0 && lwip_bind(listener, (struct sockaddr *)&address, sizeof(address)) == 0 &&
             lwip_listen(listener, 1) == 0;
    xTaskNotify(test_task_handle, ok ? 1U : 2U, eSetValueWithOverwrite);
    if (ok) {
        int peer = lwip_accept(listener, NULL, NULL);
        for (;;) {
            int n = peer >= 0 ? lwip_recv(peer, echo_buffer, sizeof(echo_buffer), 0) : -1;
            if (n <= 0 || lwip_send(peer, echo_buffer, (size_t)n, 0) != n) {
                break;
            }
        }
        if (peer >= 0) {
            lwip_close(peer);
        }
    }
    if (listener >= 0) {
        lwip_close(listener);
    }
    vTaskDelete(NULL);
}

static int tcp_self(void) {
    struct sockaddr_in address;
    uint32_t listening = 0U;
    if (xTaskCreate(tcp_echo_task, "tcp_echo", 1024, NULL, 2, NULL) != pdPASS ||
        xTaskNotifyWait(0U, 0xFFFFFFFFU, &listening, pdMS_TO_TICKS(TEST_TIMEOUT_MS)) != pdTRUE || listening != 1U) {
        return 0;
    }
    int s = lwip_socket(AF_INET, SOCK_STREAM, 0);
    own_address(&address, TEST_TCP_PORT);
    int ok = s >= 0 && lwip_connect(s, (struct sockaddr *)&address, sizeof(address)) == 0;
    set_timeout(s, 2U * TEST_TIMEOUT_MS);
    fill_pattern(tx_buffer, TEST_TCP_BYTES, 99U);
    memset(rx_buffer, 0, sizeof(rx_buffer));
    uint32_t sent = 0U;
    uint32_t received = 0U;
    while (ok && received < TEST_TCP_BYTES) {
        if (sent < TEST_TCP_BYTES) {
            uint32_t chunk = TEST_TCP_BYTES - sent < 2048U ? TEST_TCP_BYTES - sent : 2048U;
            int n = lwip_send(s, tx_buffer + sent, chunk, 0);
            ok = n > 0;
            sent += ok ? (uint32_t)n : 0U;
        }
        int n = lwip_recv(s, rx_buffer + received, TEST_TCP_BYTES - received, sent < TEST_TCP_BYTES ? MSG_DONTWAIT : 0);
        if (n > 0) {
            received += (uint32_t)n;
        } else if (n == 0 || (n < 0 && errno != EWOULDBLOCK && errno != EAGAIN)) {
            ok = 0;
        }
    }
    ok = ok && memcmp(tx_buffer, rx_buffer, TEST_TCP_BYTES) == 0;
    if (s >= 0) {
        lwip_close(s);
    }
    vTaskDelay(pdMS_TO_TICKS(100U));
    return ok;
}

static int arp_has_own_address(void) {
    ip4_addr_t address;
    struct eth_addr *mac = NULL;
    const ip4_addr_t *found = NULL;
    IP4_ADDR(&address, own_ip[0], own_ip[1], own_ip[2], own_ip[3]);
    LOCK_TCPIP_CORE();
    int ok = etharp_find_addr(NULL, &address, &mac, &found) >= 0 && mac != NULL && memcmp(mac->addr, own_mac, 6U) == 0;
    UNLOCK_TCPIP_CORE();
    return ok;
}

static void test_task(void *argument) {
    (void)argument;
    neon_net_config_t config;
    neon_net_info_t info;
    neon_net_stats_t stats;
    neon_net_handle_t *duplicate = NULL;
    uint64_t t0;

    check(neon_net_get_default_config(&config) == NEON_OK && config.ipv4_mode == NEON_NET_IPV4_DHCP &&
              config.eth.autonegotiation == 1U,
          "default config: DHCP, autonegotiation");
    check(neon_net_init(&net, NEON_NET0) == NEON_OK, "init: tcpip thread and ETH0");
    check(neon_net_init(&duplicate, NEON_NET0) == NEON_BUSY, "reject second owner");
    config.ipv4_mode = NEON_NET_IPV4_STATIC;
    check(neon_net_set_config(net, &config) == NEON_INVALID_PARAM, "reject static mode without address");
    memcpy(config.ipv4_address, own_ip, 4U);
    config.ipv4_netmask[0] = 255U;
    config.ipv4_netmask[1] = 255U;
    config.ipv4_netmask[2] = 255U;
    memcpy(config.hostname, "fx1-vp", 7U);
    check(neon_net_set_config(net, &config) == NEON_OK, "static 192.168.10.2/24");
    check(neon_net_register_callback(net, on_event, NULL) == NEON_OK, "register event callback");

    t0 = fx1_mtime();
    check(neon_net_start(net) == NEON_OK, "start: PHY autonegotiation, MAC, netif");
    check(neon_net_start(net) == NEON_BUSY, "reject start while running");
    check(neon_net_wait_ready(net, TEST_TIMEOUT_MS) == NEON_OK, "link up and IPv4 ready");
    log_ms("start to ready", t0);
    check((events_seen & (NEON_NET_EVENT_LINK_UP | NEON_NET_EVENT_IPV4_UP)) ==
              (NEON_NET_EVENT_LINK_UP | NEON_NET_EVENT_IPV4_UP),
          "events LINK_UP and IPV4_UP");
    check(neon_net_get_info(net, &info) == NEON_OK && info.link_up == 1U && info.speed_mbps == 100U &&
              info.ipv4_up == 1U && memcmp(info.ipv4_address, own_ip, 4U) == 0 &&
              memcmp(info.mac_address, own_mac, 6U) == 0,
          "info: 100 Mbps, address, MAC");

    t0 = fx1_mtime();
    for (uint16_t sequence = 1U; sequence <= 3U; ++sequence) {
        check(ping_self(sequence), "ICMP echo to own address over the line");
    }
    log_ms("3 pings", t0);
    check(arp_has_own_address(), "ARP resolved own address to own MAC through the line");

    t0 = fx1_mtime();
    check(udp_self(1U, 1U), "UDP 1 byte");
    check(udp_self(512U, 2U), "UDP 512 bytes");
    check(udp_self(1472U, 3U), "UDP 1472 bytes (largest unfragmented)");
    check(udp_self(4000U, 4U), "UDP 4000 bytes (3 IPv4 fragments, reassembled)");
    check(udp_self(8000U, 5U), "UDP 8000 bytes (6 IPv4 fragments, reassembled)");
    log_ms("UDP", t0);

    t0 = fx1_mtime();
    check(tcp_self(), "TCP 16 KiB echo through listen/accept/connect");
    log_ms("TCP 16 KiB echo", t0);

    check(neon_net_get_stats(net, &stats) == NEON_OK, "read stats");
    fx1_puts("[NEON_NET] eth tx_frames ");
    fx1_putdec(stats.eth.tx_frames);
    fx1_puts(" rx_frames ");
    fx1_putdec(stats.eth.rx_frames);
    fx1_puts(" interrupts ");
    fx1_putdec(stats.eth.interrupts);
    fx1_putc('\n');
    check(stats.eth.tx_frames >= 40U && stats.eth.tx_frames == stats.eth.rx_frames, "every frame sent came back");
    check(stats.eth.tx_errors == 0U && stats.eth.rx_errors == 0U && stats.eth.tx_dropped == 0U,
          "no descriptor errors or dropped TX");
    check(stats.rx_no_buffer == 0U && stats.rx_queue_full == 0U && stats.tx_ring_full == 0U && stats.tx_errors == 0U,
          "no buffer, mailbox or ring exhaustion");
    check(stats.eth.interrupts > 0U, "PLIC source 6 drives the service");

    check(neon_net_stop(net) == NEON_OK && neon_net_get_state(net) == NEON_NET_STATE_READY, "stop");
    check(neon_net_wait_ready(net, 0U) == NEON_BUSY, "not ready after stop");
    events_seen = 0U;
    check(neon_net_start(net) == NEON_OK && neon_net_wait_ready(net, TEST_TIMEOUT_MS) == NEON_OK, "restart");
    check(udp_self(3000U, 6U), "UDP after restart (ARP learned again)");
    check(ping_self(4U), "ICMP after restart");
    check(neon_net_stop(net) == NEON_OK, "stop again");
    check(neon_net_deinit(net) == NEON_OK && neon_net_get_state(net) == NEON_NET_STATE_RESET, "release");

    fx1_puts("[NEON_NET] SUMMARY checks=");
    fx1_putdec(checks);
    fx1_puts(" failed=0\n");
    fx1_pass();
}

int main(void) {
    NEON_FX1_FreeRTOS_Init();
    if (xTaskCreate(test_task, "net_test", 2048, NULL, 2, &test_task_handle) != pdPASS) {
        fx1_fail(0xe0ffU);
    }
    vTaskStartScheduler();
    fx1_fail(0xe0feU);
}
