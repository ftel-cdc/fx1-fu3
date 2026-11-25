/**
 * @file    lwipopts.h
 * @brief   lwIP 2.2.1 options for the Neon network manager on FreeRTOS.
 * @details Sizes come from Kconfig (sdkconfig.h). An application may override any option by placing
 *          `lwipopts_app.h` on its include path: it is included first and every default below is
 *          guarded with #ifndef. Options that the manager's threading model depends on (NO_SYS,
 *          core locking, ETH_PAD_SIZE) are not overridable.
 */
#ifndef __NEON_NET_LWIPOPTS_H__
#define __NEON_NET_LWIPOPTS_H__

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include "sdkconfig.h"

#if defined(__has_include)
#if __has_include("lwipopts_app.h")
#include "lwipopts_app.h"
#endif
#endif

/*==================================================================================================
*                                  FIXED BY THE MANAGER (neon_net.c)
==================================================================================================*/
/* The tcpip thread, or a task holding the core lock, is the single owner of neon_eth. */
#define NO_SYS 0
#define SYS_LIGHTWEIGHT_PROT 1
#define LWIP_TCPIP_CORE_LOCKING 1
/* RX goes through the tcpip mailbox: the driver delivers frames from inside linkoutput too. */
#define LWIP_TCPIP_CORE_LOCKING_INPUT 0
/* RX buffers start 2 bytes into a slot so the IPv4 header is 4-byte aligned (rx_lead 2). */
#define ETH_PAD_SIZE 2
#define LWIP_ETHERNET 1
#define LWIP_ARP 1
#define LWIP_IPV4 1
#define LWIP_NETIF_STATUS_CALLBACK 1
#define LWIP_NETIF_LINK_CALLBACK 1
#define LWIP_NETIF_HOSTNAME 1
#define LWIP_TIMERS 1
/* Each pass of the driver service and the link poll owns one sys_timeout. */
#define NEON_NET_SYS_TIMEOUTS 2

/*==================================================================================================
*                                       THREADS AND MAILBOXES
==================================================================================================*/
#define TCPIP_THREAD_NAME "tcpip"
#ifndef TCPIP_THREAD_STACKSIZE
#define TCPIP_THREAD_STACKSIZE CONFIG_NEON_MDF_NET_TCPIP_STACK_BYTES
#endif
#ifndef TCPIP_THREAD_PRIO
#define TCPIP_THREAD_PRIO CONFIG_NEON_MDF_NET_TCPIP_PRIORITY
#endif
#ifndef TCPIP_MBOX_SIZE
#define TCPIP_MBOX_SIZE 32
#endif
#ifndef DEFAULT_THREAD_STACKSIZE
#define DEFAULT_THREAD_STACKSIZE 2048
#endif
#ifndef DEFAULT_THREAD_PRIO
#define DEFAULT_THREAD_PRIO 2
#endif
#ifndef DEFAULT_RAW_RECVMBOX_SIZE
#define DEFAULT_RAW_RECVMBOX_SIZE 8
#endif
#ifndef DEFAULT_UDP_RECVMBOX_SIZE
#define DEFAULT_UDP_RECVMBOX_SIZE 8
#endif
#ifndef DEFAULT_TCP_RECVMBOX_SIZE
#define DEFAULT_TCP_RECVMBOX_SIZE 16
#endif
#ifndef DEFAULT_ACCEPTMBOX_SIZE
#define DEFAULT_ACCEPTMBOX_SIZE 4
#endif

/*==================================================================================================
*                                              MEMORY
==================================================================================================*/
#ifndef MEM_ALIGNMENT
#define MEM_ALIGNMENT 4
#endif
#ifndef MEM_SIZE
#define MEM_SIZE CONFIG_NEON_MDF_NET_MEM_SIZE
#endif
#ifndef PBUF_POOL_SIZE
#define PBUF_POOL_SIZE CONFIG_NEON_MDF_NET_PBUF_POOL_SIZE
#endif
#ifndef MEMP_NUM_PBUF
#define MEMP_NUM_PBUF 16
#endif
#ifndef MEMP_NUM_RAW_PCB
#define MEMP_NUM_RAW_PCB 4
#endif
#ifndef MEMP_NUM_UDP_PCB
#define MEMP_NUM_UDP_PCB 8
#endif
#ifndef MEMP_NUM_TCP_PCB
#define MEMP_NUM_TCP_PCB 8
#endif
#ifndef MEMP_NUM_TCP_PCB_LISTEN
#define MEMP_NUM_TCP_PCB_LISTEN 4
#endif
#ifndef MEMP_NUM_TCP_SEG
#define MEMP_NUM_TCP_SEG 32
#endif
#ifndef MEMP_NUM_NETBUF
#define MEMP_NUM_NETBUF 16
#endif
#ifndef MEMP_NUM_NETCONN
#define MEMP_NUM_NETCONN 16
#endif
/* One RX ring (16 frames) can be queued to the tcpip thread at once. */
#ifndef MEMP_NUM_TCPIP_MSG_INPKT
#define MEMP_NUM_TCPIP_MSG_INPKT 24
#endif
#ifndef MEMP_NUM_SYS_TIMEOUT
#define MEMP_NUM_SYS_TIMEOUT (LWIP_NUM_SYS_TIMEOUT_INTERNAL + NEON_NET_SYS_TIMEOUTS + 4)
#endif

/*==================================================================================================
*                                            IPv4 / LINK
==================================================================================================*/
#ifndef LWIP_IPV6
#define LWIP_IPV6 0
#endif
#ifndef IP_REASSEMBLY
#define IP_REASSEMBLY 1
#endif
#ifndef IP_FRAG
#define IP_FRAG 1
#endif
#ifndef LWIP_ICMP
#define LWIP_ICMP 1
#endif
#ifndef LWIP_RAW
#define LWIP_RAW 1
#endif
#ifndef LWIP_IGMP
#define LWIP_IGMP 0
#endif
#ifndef LWIP_AUTOIP
#define LWIP_AUTOIP 0
#endif
#ifndef LWIP_DHCP
#ifdef CONFIG_NEON_MDF_NET_DHCP
#define LWIP_DHCP 1
#else
#define LWIP_DHCP 0
#endif
#endif
/* ARP probing of the offered address delays every lease by seconds; off by default. */
#ifndef LWIP_DHCP_DOES_ACD_CHECK
#define LWIP_DHCP_DOES_ACD_CHECK 0
#endif
#ifndef LWIP_DNS
#ifdef CONFIG_NEON_MDF_NET_DNS
#define LWIP_DNS 1
#else
#define LWIP_DNS 0
#endif
#endif
#ifndef ARP_TABLE_SIZE
#define ARP_TABLE_SIZE 10
#endif
#ifndef ARP_QUEUEING
#define ARP_QUEUEING 1
#endif
#ifndef LWIP_NETIF_LOOPBACK
#ifdef CONFIG_NEON_MDF_NET_LOOPBACK
#define LWIP_NETIF_LOOPBACK 1
#else
#define LWIP_NETIF_LOOPBACK 0
#endif
#endif
#ifndef LWIP_HAVE_LOOPIF
#define LWIP_HAVE_LOOPIF 0
#endif

/*==================================================================================================
*                                                TCP / UDP
==================================================================================================*/
#ifndef LWIP_UDP
#define LWIP_UDP 1
#endif
#ifndef LWIP_TCP
#define LWIP_TCP 1
#endif
#ifndef TCP_MSS
#define TCP_MSS 1460
#endif
#ifndef TCP_WND
#define TCP_WND (CONFIG_NEON_MDF_NET_TCP_WND_MSS * TCP_MSS)
#endif
#ifndef TCP_SND_BUF
#define TCP_SND_BUF (CONFIG_NEON_MDF_NET_TCP_WND_MSS * TCP_MSS)
#endif
#ifndef TCP_SND_QUEUELEN
#define TCP_SND_QUEUELEN ((4 * TCP_SND_BUF + (TCP_MSS - 1)) / TCP_MSS)
#endif
#ifndef TCP_QUEUE_OOSEQ
#define TCP_QUEUE_OOSEQ 1
#endif
#ifndef LWIP_TCP_KEEPALIVE
#define LWIP_TCP_KEEPALIVE 1
#endif

/*==================================================================================================
*                                          SEQUENTIAL APIS
==================================================================================================*/
#ifndef LWIP_NETCONN
#define LWIP_NETCONN 1
#endif
#ifndef LWIP_SOCKET
#define LWIP_SOCKET 1
#endif
/* lwip_* names only: BSD-name macros/functions clash with newlib headers and struct members. */
#ifndef LWIP_COMPAT_SOCKETS
#define LWIP_COMPAT_SOCKETS 0
#endif
#ifndef LWIP_POSIX_SOCKETS_IO_NAMES
#define LWIP_POSIX_SOCKETS_IO_NAMES 0
#endif
#ifndef LWIP_SO_RCVTIMEO
#define LWIP_SO_RCVTIMEO 1
#endif
#ifndef LWIP_SO_SNDTIMEO
#define LWIP_SO_SNDTIMEO 1
#endif
#ifndef SO_REUSE
#define SO_REUSE 1
#endif
#ifndef LWIP_SOCKET_SELECT
#define LWIP_SOCKET_SELECT 1
#endif
#ifndef LWIP_SOCKET_POLL
#define LWIP_SOCKET_POLL 1
#endif

/*==================================================================================================
*                                      CHECKSUMS, STATISTICS, DEBUG
==================================================================================================*/
/* Software generates and checks every checksum. The MAC re-inserts the same IPv4/TCP/UDP values
 * (ETH_CSUM_TX_CFG reset 3, the field is masked while it computes), so this stays correct for
 * fragments and ICMP, which the offload does not cover (MAS 6.9). */
#ifndef LWIP_STATS
#ifdef CONFIG_NEON_MDF_NET_DEBUG
#define LWIP_STATS 1
#else
#define LWIP_STATS 0
#endif
#endif
/* Assertions stay on (bring-up); LWIP_NOASSERT in lwipopts_app.h removes them. LWIP_DEBUG only
 * enables the per-module *_DEBUG switches an application turns on. */
#ifdef CONFIG_NEON_MDF_NET_DEBUG
#ifndef LWIP_DEBUG
#define LWIP_DEBUG 1
#endif
#endif

#endif /* __NEON_NET_LWIPOPTS_H__ */
