# Neon network manager: lwIP 2.2.1 (third_party/lwip, unmodified) on FreeRTOS over neon_eth.
# Added by neon_mdf_add_net() (cmake/neon_net.cmake) after neon_mdf_add_freertos(): the lwIP port
# needs the FreeRTOS kernel target and the application's FreeRTOSConfig.h.

set(LWIP_DIR ${CMAKE_CURRENT_LIST_DIR}/third_party/lwip)
# Same groups as lwIP's src/Filelists.cmake (core, core/ipv4, api, netif/ethernet), listed here
# because that file also writes init.h/Doxyfile into the source tree.
set(NEON_NET_LWIP_SOURCES
  core/init.c core/def.c core/dns.c core/inet_chksum.c core/ip.c core/mem.c core/memp.c core/netif.c
  core/pbuf.c core/raw.c core/stats.c core/sys.c core/altcp.c core/altcp_alloc.c core/altcp_tcp.c
  core/tcp.c core/tcp_in.c core/tcp_out.c core/timeouts.c core/udp.c
  core/ipv4/acd.c core/ipv4/autoip.c core/ipv4/dhcp.c core/ipv4/etharp.c core/ipv4/icmp.c
  core/ipv4/igmp.c core/ipv4/ip4_frag.c core/ipv4/ip4.c core/ipv4/ip4_addr.c
  api/api_lib.c api/api_msg.c api/err.c api/if_api.c api/netbuf.c api/netdb.c api/netifapi.c
  api/sockets.c api/tcpip.c
  netif/ethernet.c
)
list(TRANSFORM NEON_NET_LWIP_SOURCES PREPEND ${LWIP_DIR}/src/)

add_library(neon_mdf_lwip STATIC
  ${NEON_NET_LWIP_SOURCES}
  ${LWIP_DIR}/contrib/ports/freertos/sys_arch.c
)
target_include_directories(neon_mdf_lwip PUBLIC
  ${CMAKE_CURRENT_LIST_DIR}/port/include
  ${LWIP_DIR}/src/include
  ${LWIP_DIR}/contrib/ports/freertos/include
)
target_link_libraries(neon_mdf_lwip PUBLIC neon_mdf::freertos neon_mdf_kconfig)
# Upstream code is compiled as released; its warnings are not ours to fix.
target_compile_options(neon_mdf_lwip PRIVATE -w)

add_library(neon_mdf_net STATIC
  src/neon_net.c
  src/neon_net_netif.c
  port/neon_net_port.c
)
add_library(neon_mdf::net ALIAS neon_mdf_net)
target_include_directories(neon_mdf_net PUBLIC ${CMAKE_CURRENT_LIST_DIR})
target_include_directories(neon_mdf_net PRIVATE ${CMAKE_CURRENT_LIST_DIR}/port)
target_link_libraries(neon_mdf_net PUBLIC neon_mdf_lwip neon_mdf_driver neon_mdf::freertos)
target_compile_features(neon_mdf_net PUBLIC c_std_11)
target_compile_options(neon_mdf_net PRIVATE -Wall -Wextra)
# lwIP references the port (LWIP_RAND, errno, assert); the archives resolve each other.
target_link_libraries(neon_mdf_lwip PUBLIC neon_mdf_net)
