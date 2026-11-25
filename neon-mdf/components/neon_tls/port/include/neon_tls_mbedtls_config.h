/**
 * @file    neon_tls_mbedtls_config.h
 * @brief   mbedTLS 3.6.8 configuration of the Neon TLS component (MBEDTLS_CONFIG_FILE).
 * @details TLS 1.2 client and server over lwIP sockets on FX1, software crypto:
 *          ECDHE-ECDSA/ECDHE-RSA with AES-GCM or ChaCha20-Poly1305, PSK for tests; X.509 with
 *          RSA and ECDSA (P-256, P-384), X25519. The FX1 image links no C library: memory goes to
 *          the FreeRTOS heap, snprintf to the lwIP port formatter, entropy to neon_tls_port.c.
 *          No time source yet: certificate validity dates are NOT checked (MBEDTLS_HAVE_TIME_DATE
 *          off) until SNTP/RTC time exists. An application may add `neon_tls_mbedtls_config_app.h`
 *          (included last) to adjust options.
 * @note    Hardware crypto (FX1 crypto blocks, documents pending) would plug in through the
 *          MBEDTLS_*_ALT / *_PROCESS_ALT options here and an entropy source in neon_tls_port.c.
 */
#ifndef __NEON_TLS_MBEDTLS_CONFIG_H__
#define __NEON_TLS_MBEDTLS_CONFIG_H__

/*==================================================================================================
*                                        PLATFORM / SYSTEM
==================================================================================================*/
#include <stdarg.h>
#include <stddef.h>

#define MBEDTLS_PLATFORM_C
#define MBEDTLS_PLATFORM_MEMORY
#define MBEDTLS_PLATFORM_CALLOC_MACRO neon_tls_port_calloc
#define MBEDTLS_PLATFORM_FREE_MACRO neon_tls_port_free
#define MBEDTLS_PLATFORM_SNPRINTF_MACRO neon_net_port_snprintf
#define MBEDTLS_PLATFORM_VSNPRINTF_MACRO neon_net_port_vsnprintf
#define MBEDTLS_PLATFORM_PRINTF_MACRO neon_net_port_printf
#define MBEDTLS_NO_PLATFORM_ENTROPY
#define MBEDTLS_ENTROPY_HARDWARE_ALT
#define MBEDTLS_DEPRECATED_REMOVED

#include "neon_tls_port.h"
/* Same declarations as neon_net's arch/cc.h, without pulling lwIP into every mbedTLS source. */
int neon_net_port_snprintf(char *buffer, size_t size, const char *format, ...);
int neon_net_port_vsnprintf(char *buffer, size_t size, const char *format, va_list args);
void neon_net_port_printf(const char *format, ...);

/*==================================================================================================
*                                           CRYPTO PRIMITIVES
==================================================================================================*/
#define MBEDTLS_AES_C
#define MBEDTLS_AES_ROM_TABLES
#define MBEDTLS_AES_FEWER_TABLES
#define MBEDTLS_GCM_C
#define MBEDTLS_CHACHA20_C
#define MBEDTLS_POLY1305_C
#define MBEDTLS_CHACHAPOLY_C
#define MBEDTLS_CIPHER_C
#define MBEDTLS_MD_C
#define MBEDTLS_SHA224_C
#define MBEDTLS_SHA256_C
#define MBEDTLS_SHA384_C
#define MBEDTLS_SHA512_C

#define MBEDTLS_BIGNUM_C
#define MBEDTLS_ECP_C
#define MBEDTLS_ECP_NIST_OPTIM
#define MBEDTLS_ECP_DP_SECP256R1_ENABLED
#define MBEDTLS_ECP_DP_SECP384R1_ENABLED
#define MBEDTLS_ECP_DP_CURVE25519_ENABLED
#define MBEDTLS_ECDH_C
#define MBEDTLS_ECDSA_C
#define MBEDTLS_ECDSA_DETERMINISTIC
#define MBEDTLS_RSA_C
#define MBEDTLS_PKCS1_V15
#define MBEDTLS_PKCS1_V21

#define MBEDTLS_ENTROPY_C
#define MBEDTLS_CTR_DRBG_C
#define MBEDTLS_HMAC_DRBG_C

/*==================================================================================================
*                                       KEYS AND CERTIFICATES
==================================================================================================*/
#define MBEDTLS_ASN1_PARSE_C
#define MBEDTLS_ASN1_WRITE_C
#define MBEDTLS_OID_C
#define MBEDTLS_BASE64_C
#define MBEDTLS_PEM_PARSE_C
#define MBEDTLS_PK_C
#define MBEDTLS_PK_PARSE_C
#define MBEDTLS_X509_USE_C
#define MBEDTLS_X509_CRT_PARSE_C

/*==================================================================================================
*                                                TLS
==================================================================================================*/
#define MBEDTLS_SSL_TLS_C
#define MBEDTLS_SSL_CLI_C
#define MBEDTLS_SSL_SRV_C
#define MBEDTLS_SSL_PROTO_TLS1_2
#define MBEDTLS_KEY_EXCHANGE_ECDHE_ECDSA_ENABLED
#define MBEDTLS_KEY_EXCHANGE_ECDHE_RSA_ENABLED
#define MBEDTLS_KEY_EXCHANGE_PSK_ENABLED
#define MBEDTLS_SSL_SERVER_NAME_INDICATION
#define MBEDTLS_SSL_ALPN
#define MBEDTLS_SSL_MAX_FRAGMENT_LENGTH
#define MBEDTLS_SSL_ENCRYPT_THEN_MAC
#define MBEDTLS_SSL_EXTENDED_MASTER_SECRET
/* Servers may send full 16 KiB records; outgoing records are kept small. */
#define MBEDTLS_SSL_IN_CONTENT_LEN 16384
#define MBEDTLS_SSL_OUT_CONTENT_LEN 4096
#define MBEDTLS_ERROR_C

/*==================================================================================================
*                                        APPLICATION OVERRIDES
==================================================================================================*/
#if defined(__has_include)
#if __has_include("neon_tls_mbedtls_config_app.h")
#include "neon_tls_mbedtls_config_app.h"
#endif
#endif

#endif /* __NEON_TLS_MBEDTLS_CONFIG_H__ */
