# Neon network manager (`neon_net`)

One Ethernet interface on lwIP 2.2.1 and FreeRTOS, over the `neon_eth` driver.
Include `neon_net.h`; use the lwIP socket (`lwip_*`), netconn or raw APIs for
traffic.

```text
neon_net/
  neon_net.h                  public API (init/set_config/start/wait_ready/info/stats/stop/deinit)
  src/neon_net.c              manager: tcpip thread, ETH ownership, IRQ, link poll, IPv4 events
  src/neon_net_netif.c        lwIP netif on neon_eth (copy TX/RX)
  port/include/lwipopts.h     lwIP options from Kconfig; an app may add lwipopts_app.h
  port/include/arch/cc.h      compiler/platform glue (no C library needed)
  port/neon_net_port.c        LWIP_RAND, per-task errno, atoi, diagnostics, assert
  third_party/lwip/           lwIP STABLE-2_2_1_RELEASE, unmodified (see below)
```

## Build

```cmake
include(${NEON_MDF_PATH}/cmake/freertos.cmake)
include(${NEON_MDF_PATH}/cmake/neon_net.cmake)
neon_mdf_add_freertos(${NEON_MDF_TARGET} ${CMAKE_CURRENT_SOURCE_DIR}/Inc)
neon_mdf_add_net()                       # needs CONFIG_NEON_MDF_NET=y
target_link_libraries(app PRIVATE neon_mdf::net neon_mdf::freertos)
```

FX1 only. `FreeRTOSConfig.h` needs `configUSE_MUTEXES`, `configUSE_COUNTING_SEMAPHORES`,
`INCLUDE_vTaskSuspend`, `INCLUDE_vTaskDelay`, `INCLUDE_xTaskGetSchedulerState`,
32-bit ticks and a heap large enough for the tcpip thread (stack from Kconfig) and
the mailboxes. Set `configUSE_POSIX_ERRNO 1` so socket `errno` is per task.
`configMAX_PRIORITIES` must exceed `CONFIG_NEON_MDF_NET_TCPIP_PRIORITY`.

Kconfig (menu "FTEL FX1 network stack"): tcpip priority/stack, lwIP heap,
RX pbuf pool, TCP window, DHCP, DNS, link poll period, debug/statistics, and
`NEON_MDF_NET_LOOPBACK` (see Testing).

## Lifecycle

```c
neon_net_config_t config;
neon_net_get_default_config(&config);      /* neon_eth defaults, DHCP */
neon_net_init(&net, NEON_NET0);            /* starts tcpip once, acquires ETH0 */
neon_net_set_config(net, &config);         /* static IPv4, host name, MAC, PHY */
neon_net_register_callback(net, on_event, NULL);
neon_net_start(net);                       /* PHY autoneg, MAC/DMA, netif, DHCP */
neon_net_wait_ready(net, 10000U);          /* link up and IPv4 address */
...
neon_net_stop(net);                        /* netif removed, MAC stopped, ETH re-acquired */
neon_net_deinit(net);                      /* tcpip thread stays: lwIP has no deinit */
```

All calls are from FreeRTOS tasks on hart 0 after the scheduler started.

## Threading and ownership

`neon_eth` needs one serialized owner. Here that owner is **the lwIP core
lock** (`LWIP_TCPIP_CORE_LOCKING=1`): driver calls run in the tcpip thread
(timers, IRQ-scheduled service) or in a task that holds the lock (this API,
socket calls that reach `linkoutput`).

- PLIC source 6 (registered with `NEON_FX1_IRQ_Register`, priority 2):
  `neon_eth_irq_handler()` only, then one `tcpip_callbackmsg` posted from the
  ISR to run `neon_eth_process()` in the tcpip thread.
- `neon_eth_process()` returns its next poll interval (late DES0 write-back,
  error watch, idle 100 ms); it becomes an lwIP `sys_timeout`.
- RX frames are copied into `PBUF_POOL` buffers and queued with `tcpip_input`
  (`LWIP_TCPIP_CORE_LOCKING_INPUT=0`), so frames delivered while `linkoutput`
  reclaims a full ring never reenter the stack.
- TX copies the pbuf chain into one frame; on a full ring the service runs
  once and the frame is retried, else dropped (`tx_ring_full`).
- The PHY link is polled (`CONFIG_NEON_MDF_NET_LINK_POLL_MS`); on link up the
  MAC rate follows autonegotiation (`neon_eth_set_speed`) **before**
  `netif_set_link_up()`, which also starts DHCP.

Event callbacks run in the tcpip thread with the core lock held: no blocking,
no `neon_net_*` calls.

## lwIP configuration notes

- `ETH_PAD_SIZE 2`: IPv4 headers 4-byte aligned in pbufs.
- Checksums: generated and checked in software. The MAC's TX offload
  (`ETH_CSUM_TX_CFG` reset 3) re-inserts the same IPv4/TCP/UDP values, and does
  not cover fragments or ICMP (MAS 6.9), so software stays authoritative.
- `LWIP_COMPAT_SOCKETS 0`: use `lwip_socket()`, `lwip_connect()`, ... BSD-name
  macros clash with newlib headers and with struct members named `connect`.
- The FX1 image links no C library: `LWIP_NO_CTYPE_H`, own `atoi`,
  `__errno()` (FreeRTOS per-task errno), minimal printf for `LWIP_PLATFORM_DIAG`.
- `LWIP_RAND()` is a xorshift seeded from the MAC address and mtime —
  **not cryptographic** (TCP ISN, DNS IDs). Replace with a TRNG when FX1 has one.
- Assertions are on (`LWIP_PLATFORM_ASSERT` prints and calls `fx1_fail`).

## Testing

`examples/NET_Loopback` (VP, `--eth-peer loopback`) builds with
`CONFIG_NEON_MDF_NET_LOOPBACK=n`: traffic to the own address leaves through the
MAC/DMA/PHY and comes back from the line, so ARP, ICMP echo, UDP (with IPv4
fragmentation) and a TCP echo run over the real driver. The VP has no host
network (`vp_fx1/vp/doc/VP_FX1_V3.0/eth/README.md`, "Limits"): DHCP, LAN peers
and the Internet need the FPGA.

## Third party

`third_party/lwip` is lwIP `STABLE-2_2_1_RELEASE` (commit
`77dcd25a72509eb83f72b033d219b1d40cd8eb95`, https://github.com/lwip-tcpip/lwip):
`src/` and `contrib/ports/freertos/` copied unmodified, with `COPYING`
(BSD-3-Clause) and `README`. It is excluded from clang-format. Its own
`src/Filelists.cmake` is not included because it writes into the source tree;
the source list is in `CMakeLists.txt`.
