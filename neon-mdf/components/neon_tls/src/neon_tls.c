/**
 * @file    neon_tls.c
 * @brief   Neon TLS sessions: mbedTLS 3.6.8 over blocking lwIP sockets with bounded waits.
 * @details Each pool handle owns its entropy/CTR-DRBG pair, so handles need no locking between
 *          tasks (MBEDTLS_THREADING_C is off). The BIO maps socket timeouts to MBEDTLS_ERR_SSL_TIMEOUT;
 *          a write timeout or fatal error leaves the session CLOSED (a record may be half sent).
 */

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include "neon_tls.h"
#include "FreeRTOS.h"
#include "task.h"
#include "lwip/inet.h"
#include "lwip/netdb.h"
#include "lwip/sockets.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/entropy.h"
#include "mbedtls/error.h"
#include "mbedtls/net_sockets.h"
#include "mbedtls/pk.h"
#include "mbedtls/ssl.h"
#include "mbedtls/x509_crt.h"
#include <string.h>

/*==================================================================================================
*                                       DEFINES AND MACROS
==================================================================================================*/
#define TLS_DEFAULT_TIMEOUT_MS 10000U

/*==================================================================================================
*                                  STRUCTURES AND OTHER TYPEDEFS
==================================================================================================*/
struct neon_tls_handle_s {
    uint32_t acquired;
    neon_tls_state_t state;
    neon_tls_role_t role;
    neon_tls_verify_t verify;
    uint32_t timeout_ms;
    uint32_t socket_timeout_ms; /**< SO_RCVTIMEO currently set on the socket. */
    int socket;
    uint32_t has_ca;
    uint32_t has_psk;
    uint32_t configured; /**< mbedtls_ssl_setup() succeeded for the current configuration. */
    mbedtls_entropy_context entropy;
    mbedtls_ctr_drbg_context drbg;
    mbedtls_ssl_config conf;
    mbedtls_ssl_context ssl;
    mbedtls_x509_crt ca;
    mbedtls_x509_crt own_cert;
    mbedtls_pk_context own_key;
    char server_name[NEON_TLS_SERVER_NAME_MAX_BYTES + 1U];
    char alpn[NEON_TLS_ALPN_MAX_BYTES + 1U];
    const char *alpn_list[2];
    neon_tls_error_t error;
};

/*==================================================================================================
*                                        LOCAL VARIABLES
==================================================================================================*/
static neon_tls_handle_t tls_pool[CONFIG_NEON_MDF_TLS_MAX_SESSIONS];

/* PSK-only sessions offer only PSK suites (no certificate is configured to authenticate). */
static const int tls_psk_suites[] = {
    MBEDTLS_TLS_PSK_WITH_AES_128_GCM_SHA256,
    MBEDTLS_TLS_PSK_WITH_AES_256_GCM_SHA384,
    MBEDTLS_TLS_PSK_WITH_CHACHA20_POLY1305_SHA256,
    0,
};

/*==================================================================================================
*                                   PRIVATE FUNCTION PROTOTYPES
==================================================================================================*/
static int tls_valid(const neon_tls_handle_t *handle);
static void tls_contexts_init(neon_tls_handle_t *handle);
static void tls_contexts_free(neon_tls_handle_t *handle);
static int tls_copy_text(char *destination, size_t capacity, const char *source);
static neon_status_t tls_fail(neon_tls_handle_t *handle, int mbedtls_error);
static void tls_close_socket(neon_tls_handle_t *handle);
static int tls_set_socket_timeouts(neon_tls_handle_t *handle, uint32_t timeout_ms);
static neon_status_t tls_tcp_connect(neon_tls_handle_t *handle, const char *host, uint16_t port);
static neon_status_t tls_handshake(neon_tls_handle_t *handle, const char *host);
static int tls_bio_send(void *context, const unsigned char *buffer, size_t size_bytes);
static int tls_bio_recv(void *context, unsigned char *buffer, size_t size_bytes, uint32_t timeout_ms);

/*==================================================================================================
*                                         GLOBAL FUNCTIONS
==================================================================================================*/
/** @copydoc neon_tls_get_default_config */
neon_status_t neon_tls_get_default_config(neon_tls_config_t *config) {
    if (config == NULL) {
        return NEON_INVALID_PARAM;
    }
    memset(config, 0, sizeof(*config));
    config->role = NEON_TLS_ROLE_CLIENT;
    config->verify = NEON_TLS_VERIFY_REQUIRED;
    config->timeout_ms = TLS_DEFAULT_TIMEOUT_MS;
    return NEON_OK;
}

/** @copydoc neon_tls_init */
neon_status_t neon_tls_init(neon_tls_handle_t **handle) {
    if (handle == NULL) {
        return NEON_INVALID_PARAM;
    }
    neon_tls_handle_t *slot = NULL;
    uint32_t index = 0U;
    taskENTER_CRITICAL();
    for (; index < CONFIG_NEON_MDF_TLS_MAX_SESSIONS; ++index) {
        if (tls_pool[index].acquired == 0U) {
            slot = &tls_pool[index];
            slot->acquired = 1U;
            break;
        }
    }
    taskEXIT_CRITICAL();
    if (slot == NULL) {
        return NEON_BUSY;
    }
    memset(&slot->error, 0, sizeof(slot->error));
    slot->socket = -1;
    slot->state = NEON_TLS_STATE_RESET;
    mbedtls_entropy_init(&slot->entropy);
    mbedtls_ctr_drbg_init(&slot->drbg);
    tls_contexts_init(slot);
    const unsigned char personal[] = {'n', 'e', 'o', 'n', '_', 't', 'l', 's', (unsigned char)index};
    int ret = mbedtls_ctr_drbg_seed(&slot->drbg, mbedtls_entropy_func, &slot->entropy, personal, sizeof(personal));
    neon_tls_config_t defaults;
    (void)neon_tls_get_default_config(&defaults);
    slot->state = NEON_TLS_STATE_READY;
    if (ret != 0 || neon_tls_set_config(slot, &defaults) != NEON_OK) {
        slot->error.mbedtls_error = ret != 0 ? ret : slot->error.mbedtls_error;
        tls_contexts_free(slot);
        mbedtls_ctr_drbg_free(&slot->drbg);
        mbedtls_entropy_free(&slot->entropy);
        slot->state = NEON_TLS_STATE_RESET;
        slot->acquired = 0U;
        return NEON_ERROR;
    }
    *handle = slot;
    return NEON_OK;
}

/** @copydoc neon_tls_set_config */
neon_status_t neon_tls_set_config(neon_tls_handle_t *handle, const neon_tls_config_t *config) {
    if (!tls_valid(handle) || config == NULL ||
        (config->role != NEON_TLS_ROLE_CLIENT && config->role != NEON_TLS_ROLE_SERVER) ||
        (config->verify != NEON_TLS_VERIFY_REQUIRED && config->verify != NEON_TLS_VERIFY_NONE) ||
        config->timeout_ms == 0U || (config->ca != NULL && config->ca_size == 0U) ||
        ((config->own_cert == NULL) != (config->own_key == NULL)) ||
        (config->psk != NULL && (config->psk_size == 0U || config->psk_identity == NULL))) {
        return NEON_INVALID_PARAM;
    }
    if ((config->server_name != NULL && strlen(config->server_name) > NEON_TLS_SERVER_NAME_MAX_BYTES) ||
        (config->alpn != NULL && strlen(config->alpn) > NEON_TLS_ALPN_MAX_BYTES) ||
        (config->psk != NULL &&
         (config->psk_identity[0] == '\0' || strlen(config->psk_identity) > NEON_TLS_PSK_IDENTITY_MAX_BYTES))) {
        return NEON_INVALID_PARAM;
    }
    if (handle->state == NEON_TLS_STATE_CONNECTED) {
        return NEON_BUSY;
    }
    handle->configured = 0U;
    tls_close_socket(handle);
    tls_contexts_free(handle);
    tls_contexts_init(handle);
    handle->role = config->role;
    handle->verify = config->verify;
    handle->timeout_ms = config->timeout_ms;
    handle->has_ca = 0U;
    handle->has_psk = 0U;
    handle->server_name[0] = '\0';
    handle->alpn[0] = '\0';
    if (!tls_copy_text(handle->server_name, sizeof(handle->server_name), config->server_name) ||
        !tls_copy_text(handle->alpn, sizeof(handle->alpn), config->alpn)) {
        return NEON_INVALID_PARAM;
    }
    int ret = mbedtls_ssl_config_defaults(
        &handle->conf, config->role == NEON_TLS_ROLE_CLIENT ? MBEDTLS_SSL_IS_CLIENT : MBEDTLS_SSL_IS_SERVER,
        MBEDTLS_SSL_TRANSPORT_STREAM, MBEDTLS_SSL_PRESET_DEFAULT);
    if (ret == 0) {
        mbedtls_ssl_conf_rng(&handle->conf, mbedtls_ctr_drbg_random, &handle->drbg);
        mbedtls_ssl_conf_authmode(&handle->conf, config->verify == NEON_TLS_VERIFY_NONE ? MBEDTLS_SSL_VERIFY_NONE
                                                                                        : MBEDTLS_SSL_VERIFY_REQUIRED);
        mbedtls_ssl_conf_read_timeout(&handle->conf, config->timeout_ms);
    }
    if (ret == 0 && config->ca != NULL) {
        ret = mbedtls_x509_crt_parse(&handle->ca, config->ca, config->ca_size);
        if (ret == 0) {
            mbedtls_ssl_conf_ca_chain(&handle->conf, &handle->ca, NULL);
            handle->has_ca = 1U;
        }
    }
    if (ret == 0 && config->own_cert != NULL) {
        ret = mbedtls_x509_crt_parse(&handle->own_cert, config->own_cert, config->own_cert_size);
        if (ret == 0) {
            ret = mbedtls_pk_parse_key(&handle->own_key, config->own_key, config->own_key_size, NULL, 0U,
                                       mbedtls_ctr_drbg_random, &handle->drbg);
        }
        if (ret == 0) {
            ret = mbedtls_ssl_conf_own_cert(&handle->conf, &handle->own_cert, &handle->own_key);
        }
    }
    if (ret == 0 && config->psk != NULL) {
        size_t identity_size = strlen(config->psk_identity);
        ret = mbedtls_ssl_conf_psk(&handle->conf, config->psk, config->psk_size,
                                   (const unsigned char *)config->psk_identity, identity_size);
        if (ret == 0 && config->ca == NULL && config->own_cert == NULL) {
            mbedtls_ssl_conf_ciphersuites(&handle->conf, tls_psk_suites);
        }
        handle->has_psk = ret == 0 ? 1U : 0U;
    }
    if (ret == 0 && handle->alpn[0] != '\0') {
        handle->alpn_list[0] = handle->alpn;
        handle->alpn_list[1] = NULL;
        ret = mbedtls_ssl_conf_alpn_protocols(&handle->conf, handle->alpn_list);
    }
    if (ret == 0) {
        ret = mbedtls_ssl_setup(&handle->ssl, &handle->conf);
    }
    handle->state = NEON_TLS_STATE_READY;
    if (ret != 0) {
        handle->error.mbedtls_error = ret;
        return NEON_ERROR;
    }
    handle->configured = 1U;
    return NEON_OK;
}

/** @copydoc neon_tls_connect */
neon_status_t neon_tls_connect(neon_tls_handle_t *handle, const char *host, uint16_t port) {
    if (!tls_valid(handle) || host == NULL || host[0] == '\0' || port == 0U || handle->role != NEON_TLS_ROLE_CLIENT) {
        return NEON_INVALID_PARAM;
    }
    if (handle->state != NEON_TLS_STATE_READY || handle->configured == 0U) {
        return NEON_BUSY;
    }
    if (handle->verify == NEON_TLS_VERIFY_REQUIRED && handle->has_ca == 0U && handle->has_psk == 0U) {
        return NEON_INVALID_PARAM;
    }
    memset(&handle->error, 0, sizeof(handle->error));
    neon_status_t status = tls_tcp_connect(handle, host, port);
    if (status == NEON_OK) {
        status = tls_handshake(handle, handle->server_name[0] != '\0' ? handle->server_name : host);
    }
    return status;
}

/** @copydoc neon_tls_accept */
neon_status_t neon_tls_accept(neon_tls_handle_t *handle, int listen_socket) {
    if (!tls_valid(handle) || listen_socket < 0 || handle->role != NEON_TLS_ROLE_SERVER) {
        return NEON_INVALID_PARAM;
    }
    if (handle->state != NEON_TLS_STATE_READY || handle->configured == 0U) {
        return NEON_BUSY;
    }
    memset(&handle->error, 0, sizeof(handle->error));
    fd_set readable;
    FD_ZERO(&readable);
    FD_SET(listen_socket, &readable);
    struct timeval wait = {.tv_sec = (time_t)(handle->timeout_ms / 1000U),
                           .tv_usec = (suseconds_t)((handle->timeout_ms % 1000U) * 1000U)};
    int ready = lwip_select(listen_socket + 1, &readable, NULL, NULL, &wait);
    if (ready <= 0) {
        handle->error.socket_errno = ready < 0 ? errno : 0;
        return ready == 0 ? NEON_TIMEOUT : NEON_ERROR;
    }
    handle->socket = lwip_accept(listen_socket, NULL, NULL);
    if (handle->socket < 0 || !tls_set_socket_timeouts(handle, handle->timeout_ms)) {
        handle->error.socket_errno = errno;
        tls_close_socket(handle);
        return NEON_ERROR;
    }
    return tls_handshake(handle, NULL);
}

/** @copydoc neon_tls_write */
neon_status_t neon_tls_write(neon_tls_handle_t *handle, const uint8_t *data, uint32_t size_bytes,
                             uint32_t *written_bytes) {
    if (!tls_valid(handle) || (data == NULL && size_bytes != 0U)) {
        return NEON_INVALID_PARAM;
    }
    if (written_bytes != NULL) {
        *written_bytes = 0U;
    }
    if (handle->state != NEON_TLS_STATE_CONNECTED) {
        return NEON_BUSY;
    }
    uint32_t done = 0U;
    while (done < size_bytes) {
        int ret = mbedtls_ssl_write(&handle->ssl, data + done, size_bytes - done);
        if (ret > 0) {
            done += (uint32_t)ret;
            if (written_bytes != NULL) {
                *written_bytes = done;
            }
        } else if (ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE) {
            handle->state = NEON_TLS_STATE_CLOSED;
            return tls_fail(handle, ret);
        }
    }
    return NEON_OK;
}

/** @copydoc neon_tls_read */
neon_status_t neon_tls_read(neon_tls_handle_t *handle, uint8_t *data, uint32_t size_bytes, uint32_t *read_bytes) {
    if (!tls_valid(handle) || data == NULL || size_bytes == 0U || read_bytes == NULL) {
        return NEON_INVALID_PARAM;
    }
    *read_bytes = 0U;
    if (handle->state != NEON_TLS_STATE_CONNECTED) {
        return handle->state == NEON_TLS_STATE_CLOSED ? NEON_OK : NEON_BUSY;
    }
    for (;;) {
        int ret = mbedtls_ssl_read(&handle->ssl, data, size_bytes);
        if (ret > 0) {
            *read_bytes = (uint32_t)ret;
            return NEON_OK;
        }
        if (ret == 0 || ret == MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY) {
            handle->state = NEON_TLS_STATE_CLOSED;
            return NEON_OK;
        }
        if (ret == MBEDTLS_ERR_SSL_TIMEOUT) {
            handle->error.mbedtls_error = ret;
            return NEON_TIMEOUT;
        }
        if (ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE) {
            handle->state = NEON_TLS_STATE_CLOSED;
            return tls_fail(handle, ret);
        }
    }
}

/** @copydoc neon_tls_close */
neon_status_t neon_tls_close(neon_tls_handle_t *handle) {
    if (!tls_valid(handle)) {
        return NEON_INVALID_PARAM;
    }
    if (handle->state == NEON_TLS_STATE_CONNECTED) {
        (void)mbedtls_ssl_close_notify(&handle->ssl);
    }
    tls_close_socket(handle);
    if (handle->configured != 0U) {
        (void)mbedtls_ssl_session_reset(&handle->ssl);
    }
    handle->state = NEON_TLS_STATE_READY;
    return NEON_OK;
}

/** @copydoc neon_tls_get_state */
neon_tls_state_t neon_tls_get_state(const neon_tls_handle_t *handle) {
    return tls_valid(handle) ? handle->state : NEON_TLS_STATE_RESET;
}

/** @copydoc neon_tls_get_error */
neon_status_t neon_tls_get_error(const neon_tls_handle_t *handle, neon_tls_error_t *error) {
    if (!tls_valid(handle) || error == NULL) {
        return NEON_INVALID_PARAM;
    }
    *error = handle->error;
    return NEON_OK;
}

/** @copydoc neon_tls_deinit */
neon_status_t neon_tls_deinit(neon_tls_handle_t *handle) {
    if (!tls_valid(handle)) {
        return NEON_INVALID_PARAM;
    }
    (void)neon_tls_close(handle);
    tls_contexts_free(handle);
    mbedtls_ctr_drbg_free(&handle->drbg);
    mbedtls_entropy_free(&handle->entropy);
    handle->state = NEON_TLS_STATE_RESET;
    taskENTER_CRITICAL();
    handle->acquired = 0U;
    taskEXIT_CRITICAL();
    return NEON_OK;
}

/*==================================================================================================
*                                        PRIVATE FUNCTIONS
==================================================================================================*/
static int tls_valid(const neon_tls_handle_t *handle) {
    return handle != NULL && handle >= &tls_pool[0] && handle < &tls_pool[CONFIG_NEON_MDF_TLS_MAX_SESSIONS] &&
           handle->acquired != 0U && handle->state != NEON_TLS_STATE_RESET;
}

static void tls_contexts_init(neon_tls_handle_t *handle) {
    mbedtls_ssl_config_init(&handle->conf);
    mbedtls_ssl_init(&handle->ssl);
    mbedtls_x509_crt_init(&handle->ca);
    mbedtls_x509_crt_init(&handle->own_cert);
    mbedtls_pk_init(&handle->own_key);
}

static void tls_contexts_free(neon_tls_handle_t *handle) {
    mbedtls_ssl_free(&handle->ssl);
    mbedtls_ssl_config_free(&handle->conf);
    mbedtls_x509_crt_free(&handle->ca);
    mbedtls_x509_crt_free(&handle->own_cert);
    mbedtls_pk_free(&handle->own_key);
}

/* Copies a NUL-terminated string; NULL copies as empty. Returns 0 when it does not fit. */
static int tls_copy_text(char *destination, size_t capacity, const char *source) {
    if (source == NULL) {
        destination[0] = '\0';
        return 1;
    }
    size_t size = strlen(source);
    if (size >= capacity) {
        return 0;
    }
    memcpy(destination, source, size + 1U);
    return 1;
}

static neon_status_t tls_fail(neon_tls_handle_t *handle, int mbedtls_error) {
    handle->error.mbedtls_error = mbedtls_error;
    return mbedtls_error == MBEDTLS_ERR_SSL_TIMEOUT ? NEON_TIMEOUT : NEON_ERROR;
}

static void tls_close_socket(neon_tls_handle_t *handle) {
    if (handle->socket >= 0) {
        (void)lwip_close(handle->socket);
        handle->socket = -1;
    }
}

static int tls_set_socket_timeouts(neon_tls_handle_t *handle, uint32_t timeout_ms) {
    struct timeval tv = {.tv_sec = (time_t)(timeout_ms / 1000U),
                         .tv_usec = (suseconds_t)((timeout_ms % 1000U) * 1000U)};
    if (lwip_setsockopt(handle->socket, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) != 0 ||
        lwip_setsockopt(handle->socket, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv)) != 0) {
        return 0;
    }
    handle->socket_timeout_ms = timeout_ms;
    return 1;
}

/* Resolve (dotted IPv4 or DNS), then a non-blocking connect bounded by the session timeout. */
static neon_status_t tls_tcp_connect(neon_tls_handle_t *handle, const char *host, uint16_t port) {
    struct sockaddr_in address;
    memset(&address, 0, sizeof(address));
    address.sin_len = (u8_t)sizeof(address);
    address.sin_family = AF_INET;
    address.sin_port = lwip_htons(port);
    if (inet_aton(host, &address.sin_addr) == 0) {
#if LWIP_DNS
        struct addrinfo hints;
        struct addrinfo *result = NULL;
        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;
        int ret = lwip_getaddrinfo(host, NULL, &hints, &result);
        if (ret != 0 || result == NULL) {
            handle->error.mbedtls_error = MBEDTLS_ERR_NET_UNKNOWN_HOST;
            return NEON_ERROR;
        }
        address.sin_addr = ((struct sockaddr_in *)result->ai_addr)->sin_addr;
        lwip_freeaddrinfo(result);
#else
        handle->error.mbedtls_error = MBEDTLS_ERR_NET_UNKNOWN_HOST;
        return NEON_ERROR;
#endif
    }
    handle->socket = lwip_socket(AF_INET, SOCK_STREAM, 0);
    if (handle->socket < 0) {
        handle->error.socket_errno = errno;
        handle->error.mbedtls_error = MBEDTLS_ERR_NET_SOCKET_FAILED;
        return NEON_ERROR;
    }
    int flags = lwip_fcntl(handle->socket, F_GETFL, 0);
    (void)lwip_fcntl(handle->socket, F_SETFL, flags | O_NONBLOCK);
    int ret = lwip_connect(handle->socket, (struct sockaddr *)&address, sizeof(address));
    if (ret != 0 && errno == EINPROGRESS) {
        fd_set writable;
        FD_ZERO(&writable);
        FD_SET(handle->socket, &writable);
        struct timeval wait = {.tv_sec = (time_t)(handle->timeout_ms / 1000U),
                               .tv_usec = (suseconds_t)((handle->timeout_ms % 1000U) * 1000U)};
        ret = lwip_select(handle->socket + 1, NULL, &writable, NULL, &wait);
        if (ret == 0) {
            tls_close_socket(handle);
            handle->error.mbedtls_error = MBEDTLS_ERR_NET_CONNECT_FAILED;
            return NEON_TIMEOUT;
        }
        int socket_error = 0;
        socklen_t length = sizeof(socket_error);
        ret = ret < 0 ? -1 : lwip_getsockopt(handle->socket, SOL_SOCKET, SO_ERROR, &socket_error, &length);
        if (ret == 0 && socket_error != 0) {
            errno = socket_error;
            ret = -1;
        }
    }
    if (ret != 0) {
        handle->error.socket_errno = errno;
        handle->error.mbedtls_error = MBEDTLS_ERR_NET_CONNECT_FAILED;
        tls_close_socket(handle);
        return NEON_ERROR;
    }
    (void)lwip_fcntl(handle->socket, F_SETFL, flags & ~O_NONBLOCK);
    if (!tls_set_socket_timeouts(handle, handle->timeout_ms)) {
        handle->error.socket_errno = errno;
        tls_close_socket(handle);
        return NEON_ERROR;
    }
    return NEON_OK;
}

static neon_status_t tls_handshake(neon_tls_handle_t *handle, const char *host) {
    int ret = mbedtls_ssl_session_reset(&handle->ssl);
    if (ret == 0 && handle->role == NEON_TLS_ROLE_CLIENT) {
        /* SNI and the certificate name check (required by mbedTLS 3.6 with VERIFY_REQUIRED). */
        ret = mbedtls_ssl_set_hostname(&handle->ssl, host);
    }
    if (ret == 0) {
        mbedtls_ssl_set_bio(&handle->ssl, handle, tls_bio_send, NULL, tls_bio_recv);
        do {
            ret = mbedtls_ssl_handshake(&handle->ssl);
        } while (ret == MBEDTLS_ERR_SSL_WANT_READ || ret == MBEDTLS_ERR_SSL_WANT_WRITE);
    }
    handle->error.verify_flags = mbedtls_ssl_get_verify_result(&handle->ssl);
    if (ret != 0) {
        tls_close_socket(handle);
        (void)mbedtls_ssl_session_reset(&handle->ssl);
        return tls_fail(handle, ret);
    }
    handle->state = NEON_TLS_STATE_CONNECTED;
    return NEON_OK;
}

static int tls_bio_send(void *context, const unsigned char *buffer, size_t size_bytes) {
    neon_tls_handle_t *handle = (neon_tls_handle_t *)context;
    int sent = lwip_send(handle->socket, buffer, size_bytes, 0);
    if (sent >= 0) {
        return sent;
    }
    handle->error.socket_errno = errno;
    if (errno == EAGAIN || errno == EWOULDBLOCK) {
        return MBEDTLS_ERR_SSL_TIMEOUT;
    }
    if (errno == EPIPE || errno == ECONNRESET) {
        return MBEDTLS_ERR_NET_CONN_RESET;
    }
    return MBEDTLS_ERR_NET_SEND_FAILED;
}

static int tls_bio_recv(void *context, unsigned char *buffer, size_t size_bytes, uint32_t timeout_ms) {
    neon_tls_handle_t *handle = (neon_tls_handle_t *)context;
    if (timeout_ms == 0U) {
        timeout_ms = handle->timeout_ms;
    }
    if (timeout_ms != handle->socket_timeout_ms && !tls_set_socket_timeouts(handle, timeout_ms)) {
        handle->error.socket_errno = errno;
        return MBEDTLS_ERR_NET_RECV_FAILED;
    }
    int received = lwip_recv(handle->socket, buffer, size_bytes, 0);
    if (received >= 0) {
        return received;
    }
    handle->error.socket_errno = errno;
    if (errno == EAGAIN || errno == EWOULDBLOCK) {
        return MBEDTLS_ERR_SSL_TIMEOUT;
    }
    if (errno == ECONNRESET) {
        return MBEDTLS_ERR_NET_CONN_RESET;
    }
    return MBEDTLS_ERR_NET_RECV_FAILED;
}
