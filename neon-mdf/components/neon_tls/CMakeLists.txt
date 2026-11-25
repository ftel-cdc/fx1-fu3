# Neon TLS: mbedTLS 3.6.8 (third_party/mbedtls, unmodified) configured by port/include/
# neon_tls_mbedtls_config.h, over lwIP sockets of neon_net. Added by neon_mdf_add_tls().

set(MBEDTLS_DIR ${CMAKE_CURRENT_LIST_DIR}/third_party/mbedtls)
# Every library source; modules the configuration leaves out compile to nothing.
file(GLOB NEON_TLS_MBEDTLS_SOURCES CONFIGURE_DEPENDS ${MBEDTLS_DIR}/library/*.c)

add_library(neon_mdf_mbedtls STATIC ${NEON_TLS_MBEDTLS_SOURCES})
target_include_directories(neon_mdf_mbedtls PUBLIC
  ${CMAKE_CURRENT_LIST_DIR}/port/include
  ${MBEDTLS_DIR}/include
)
target_include_directories(neon_mdf_mbedtls PRIVATE ${MBEDTLS_DIR}/library)
target_compile_definitions(neon_mdf_mbedtls PUBLIC MBEDTLS_CONFIG_FILE="neon_tls_mbedtls_config.h")
target_link_libraries(neon_mdf_mbedtls PUBLIC neon_mdf_kconfig)
target_link_libraries(neon_mdf_mbedtls PRIVATE neon_mdf::net)
# Upstream code is compiled as released.
target_compile_options(neon_mdf_mbedtls PRIVATE -w)

add_library(neon_mdf_tls STATIC
  src/neon_tls.c
  port/neon_tls_port.c
)
add_library(neon_mdf::tls ALIAS neon_mdf_tls)
target_include_directories(neon_mdf_tls PUBLIC ${CMAKE_CURRENT_LIST_DIR})
target_link_libraries(neon_mdf_tls PUBLIC neon_mdf_mbedtls neon_mdf::net)
target_compile_features(neon_mdf_tls PUBLIC c_std_11)
target_compile_options(neon_mdf_tls PRIVATE -Wall -Wextra)
# mbedTLS calls the port (calloc/free, entropy, libc helpers).
target_link_libraries(neon_mdf_mbedtls PUBLIC neon_mdf_tls)
