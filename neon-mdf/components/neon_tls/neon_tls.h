/**
 * @file    neon_tls.h
 * @brief   Neon TLS: TLS 1.2 sessions (mbedTLS 3.6.8) over lwIP sockets of neon_net.
 * @details A session is a client (neon_tls_connect) or a server (neon_tls_accept) on one TCP socket.
 *          Certificates/keys are PEM or DER, parsed and copied at set_config; certificate validity
 *          dates are not checked until FX1 has a time source. Crypto is software only. Handles come
 *          from a static pool (CONFIG_NEON_MDF_TLS_MAX_SESSIONS); records use the FreeRTOS heap.
 *          One task uses a handle at a time; different handles may run in different tasks.
 *          FreeRTOS task context only, never from an ISR.
 * @warning With CONFIG_NEON_MDF_TLS_DEV_ENTROPY the random numbers are predictable (bring-up only).
 */
#ifndef __NEON_TLS_H__
#define __NEON_TLS_H__

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include <stddef.h>
#include "neon_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/*==================================================================================================
*                                       DEFINES AND MACROS
==================================================================================================*/
/** Longest server name (SNI, certificate host name check), without the terminator. */
#define NEON_TLS_SERVER_NAME_MAX_BYTES 127U
/** Longest ALPN protocol name, without the terminator. */
#define NEON_TLS_ALPN_MAX_BYTES 31U
/** Longest PSK identity, without the terminator. */
#define NEON_TLS_PSK_IDENTITY_MAX_BYTES 63U

/*==================================================================================================
*                                  STRUCTURES AND OTHER TYPEDEFS
==================================================================================================*/
typedef struct neon_tls_handle_s neon_tls_handle_t;

typedef enum { NEON_TLS_ROLE_CLIENT = 0, NEON_TLS_ROLE_SERVER = 1 } neon_tls_role_t;

typedef enum {
    NEON_TLS_VERIFY_REQUIRED = 0, /**< Peer certificate must chain to ca and match server_name. */
    NEON_TLS_VERIFY_NONE = 1      /**< No peer authentication: tests only. */
} neon_tls_verify_t;

typedef enum {
    NEON_TLS_STATE_RESET = 0,
    NEON_TLS_STATE_READY,     /**< Initialized/configured, no connection. */
    NEON_TLS_STATE_CONNECTED, /**< Handshake complete. */
    NEON_TLS_STATE_CLOSED     /**< Peer closed or fatal error; call close before reuse. */
} neon_tls_state_t;

/**
 * @brief Session settings. Begin from neon_tls_get_default_config(). PEM buffers must include the
 *        terminating NUL in their size (mbedTLS rule); DER buffers are passed as they are.
 */
typedef struct neon_tls_config_s {
    neon_tls_role_t role;
    neon_tls_verify_t verify; /**< Server role: REQUIRED asks for a client certificate. */
    const uint8_t *ca;        /**< Trusted CA chain (PEM/DER); required with VERIFY_REQUIRED. */
    size_t ca_size;
    const uint8_t *own_cert; /**< Server certificate, or client certificate (mutual TLS). */
    size_t own_cert_size;
    const uint8_t *own_key; /**< Private key of own_cert (unencrypted PEM/DER). */
    size_t own_key_size;
    const uint8_t *psk; /**< Pre-shared key instead of certificates (PSK suites only). */
    size_t psk_size;
    const char *psk_identity;
    const char *server_name; /**< Client: SNI and host name check; NULL uses the connect host. */
    const char *alpn;        /**< One ALPN protocol ("mqtt", "http/1.1"), NULL for none. */
    uint32_t timeout_ms;     /**< Each handshake/read/write wait; also the TCP connect bound. */
} neon_tls_config_t;

/** Last failure details for diagnostics. */
typedef struct neon_tls_error_s {
    int32_t mbedtls_error; /**< Negative mbedTLS/lwIP-BIO code of the last failed call, 0 if none. */
    uint32_t verify_flags; /**< MBEDTLS_X509_BADCERT_* flags of the last handshake. */
    int32_t socket_errno;  /**< errno of the last failed socket call. */
} neon_tls_error_t;

/*==================================================================================================
*                                       FUNCTION PROTOTYPES
==================================================================================================*/
/**
 * @brief Fill default settings: client, verification required, 10 s timeout, nothing loaded.
 * @param[out] config Settings to fill.
 * @return NEON_OK or NEON_INVALID_PARAM.
 */
neon_status_t neon_tls_get_default_config(neon_tls_config_t *config);
/**
 * @brief Take a handle from the pool and seed its random generator.
 * @param[out] handle Receives the handle.
 * @return NEON_OK, NEON_BUSY if the pool is empty, NEON_ERROR (no entropy) or NEON_INVALID_PARAM.
 */
neon_status_t neon_tls_init(neon_tls_handle_t **handle);
/**
 * @brief Parse and keep certificates/keys, apply role, verification, PSK, ALPN and timeouts.
 * @param[in,out] handle READY handle.
 * @param[in] config Settings; buffers may be released after the call.
 * @return NEON_OK, NEON_BUSY while connected, NEON_ERROR (parse failure, see get_error) or
 *         NEON_INVALID_PARAM.
 */
neon_status_t neon_tls_set_config(neon_tls_handle_t *handle, const neon_tls_config_t *config);
/**
 * @brief Client: resolve host, open TCP to port and complete the handshake.
 * @param[in,out] handle Configured client handle, READY.
 * @param[in] host Dotted IPv4 or DNS name.
 * @param[in] port TCP port.
 * @return NEON_OK, NEON_TIMEOUT, NEON_ERROR (DNS, TCP, handshake or verification; see get_error),
 *         NEON_BUSY if not READY, or NEON_INVALID_PARAM.
 */
neon_status_t neon_tls_connect(neon_tls_handle_t *handle, const char *host, uint16_t port);
/**
 * @brief Server: accept one TCP connection on a listening lwIP socket and complete the handshake.
 * @param[in,out] handle Configured server handle, READY.
 * @param[in] listen_socket Socket from lwip_socket/lwip_bind/lwip_listen, owned by the caller.
 * @return NEON_OK, NEON_TIMEOUT, NEON_ERROR, NEON_BUSY or NEON_INVALID_PARAM.
 */
neon_status_t neon_tls_accept(neon_tls_handle_t *handle, int listen_socket);
/**
 * @brief Send application data; returns when everything is written or on error.
 * @param[in,out] handle CONNECTED handle.
 * @param[in] data Bytes to send.
 * @param[in] size_bytes Number of bytes.
 * @param[out] written_bytes Bytes accepted by TLS (all of them on NEON_OK); may be NULL.
 * @return NEON_OK, NEON_TIMEOUT, NEON_ERROR (state becomes CLOSED), NEON_BUSY or NEON_INVALID_PARAM.
 */
neon_status_t neon_tls_write(neon_tls_handle_t *handle, const uint8_t *data, uint32_t size_bytes,
                             uint32_t *written_bytes);
/**
 * @brief Receive application data, waiting up to the configured timeout.
 * @param[in,out] handle CONNECTED handle.
 * @param[out] data Destination.
 * @param[in] size_bytes Capacity.
 * @param[out] read_bytes Bytes received; 0 with NEON_OK means the peer closed (state CLOSED).
 * @return NEON_OK, NEON_TIMEOUT (nothing arrived), NEON_ERROR, NEON_BUSY or NEON_INVALID_PARAM.
 */
neon_status_t neon_tls_read(neon_tls_handle_t *handle, uint8_t *data, uint32_t size_bytes, uint32_t *read_bytes);
/**
 * @brief Send close_notify if connected, close the socket, keep the configuration for reuse.
 * @param[in,out] handle Initialized handle.
 * @return NEON_OK or NEON_INVALID_PARAM.
 */
neon_status_t neon_tls_close(neon_tls_handle_t *handle);
/**
 * @brief Get the session state.
 * @param[in] handle Handle.
 * @return Current state, RESET for an invalid handle.
 */
neon_tls_state_t neon_tls_get_state(const neon_tls_handle_t *handle);
/**
 * @brief Get the last failure details.
 * @param[in] handle Initialized handle.
 * @param[out] error Snapshot.
 * @return NEON_OK or NEON_INVALID_PARAM.
 */
neon_status_t neon_tls_get_error(const neon_tls_handle_t *handle, neon_tls_error_t *error);
/**
 * @brief Close if needed, free parsed certificates/keys and return the handle to the pool.
 * @param[in,out] handle Initialized handle; invalid afterwards.
 * @return NEON_OK or NEON_INVALID_PARAM.
 */
neon_status_t neon_tls_deinit(neon_tls_handle_t *handle);

#ifdef __cplusplus
}
#endif
#endif /* __NEON_TLS_H__ */
