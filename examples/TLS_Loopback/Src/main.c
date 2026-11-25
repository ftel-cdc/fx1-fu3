/**
 * @file main.c
 * @brief Check neon_tls (mbedTLS 3.6.8) on the VP: a TLS server and client on the own address.
 * @details As NET_Loopback, CONFIG_NEON_MDF_NET_LOOPBACK is off and `--eth-peer loopback` returns
 *          every frame, so both TLS endpoints talk through MAC, DMA and PHY. Sessions: PSK, then
 *          certificates (RSA root -> ECDSA P-256 server, ECDHE-ECDSA), then a host name mismatch and
 *          an untrusted root, each of which the client must reject. Echoes are compared byte for byte.
 */
#include "neon_net.h"
#include "neon_tls.h"
#include "neon_freertos_fx1.h"
#include "task.h"
#include "test_pki.h"
#include "lwip/sockets.h"
#include "mbedtls/x509.h"
#include <stddef.h>
#include <string.h>

#define TEST_PORT 4433U
#define TEST_TIMEOUT_MS 120000U
#define TEST_TASK_STACK_WORDS 4096U

typedef enum { MODE_PSK = 0, MODE_CERT = 1 } test_mode_t;

typedef struct {
    test_mode_t mode;
    uint32_t echo_bytes;
    neon_status_t accept_status;
    neon_status_t echo_status;
} server_job_t;

static const uint8_t own_ip[4] = {192U, 168U, 10U, 2U};
static const uint8_t test_psk[16] = {0x4eU, 0x45U, 0x4fU, 0x4eU, 0x2dU, 0x70U, 0x73U, 0x6bU,
                                     0x01U, 0x23U, 0x45U, 0x67U, 0x89U, 0xabU, 0xcdU, 0xefU};
static uint32_t checks;
static TaskHandle_t test_task_handle;
static uint8_t client_tx[4096];
static uint8_t client_rx[4096];
static uint8_t server_buffer[1024];

static void check(int ok, const char *name) {
    ++checks;
    fx1_puts(ok ? "[NEON_TLS] PASS " : "[NEON_TLS] FAIL ");
    fx1_putdec(checks);
    fx1_puts(" ");
    fx1_puts(name);
    fx1_putc('\n');
    if (!ok) {
        fx1_fail(0xe000U + checks);
    }
}

static void log_ms(const char *what, uint64_t since) {
    fx1_puts("[NEON_TLS] ");
    fx1_puts(what);
    fx1_puts(" took ");
    fx1_putdec((uint32_t)((fx1_mtime() - since) / (FX1_MTIME_HZ / 1000U)));
    fx1_puts(" ms (simulated)\n");
}

static void log_error(const char *what, neon_tls_handle_t *tls) {
    neon_tls_error_t error;
    if (neon_tls_get_error(tls, &error) == NEON_OK) {
        fx1_puts("[NEON_TLS] ");
        fx1_puts(what);
        fx1_log_hex(" mbedtls error", (uint32_t)error.mbedtls_error);
        fx1_log_hex("[NEON_TLS] verify flags", error.verify_flags);
    }
}

static int listen_socket(void) {
    struct sockaddr_in address;
    memset(&address, 0, sizeof(address));
    address.sin_len = (u8_t)sizeof(address);
    address.sin_family = AF_INET;
    address.sin_port = lwip_htons(TEST_PORT);
    address.sin_addr.s_addr = PP_HTONL(LWIP_MAKEU32(own_ip[0], own_ip[1], own_ip[2], own_ip[3]));
    int s = lwip_socket(AF_INET, SOCK_STREAM, 0);
    int reuse = 1;
    (void)lwip_setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
    if (s < 0 || lwip_bind(s, (struct sockaddr *)&address, sizeof(address)) != 0 || lwip_listen(s, 1) != 0) {
        if (s >= 0) {
            lwip_close(s);
        }
        return -1;
    }
    return s;
}

/* One TLS server session: accept, echo echo_bytes, close. Notifies the test task when listening and
 * when done; the job carries the results. */
static void server_task(void *argument) {
    server_job_t *job = (server_job_t *)argument;
    neon_tls_handle_t *tls = NULL;
    neon_tls_config_t config;
    int s = listen_socket();
    job->accept_status = NEON_ERROR;
    job->echo_status = NEON_ERROR;
    (void)neon_tls_get_default_config(&config);
    config.role = NEON_TLS_ROLE_SERVER;
    config.verify = NEON_TLS_VERIFY_NONE; /* no client certificate */
    config.timeout_ms = TEST_TIMEOUT_MS;
    if (job->mode == MODE_PSK) {
        config.psk = test_psk;
        config.psk_size = sizeof(test_psk);
        config.psk_identity = "fx1-client";
    } else {
        config.own_cert = (const uint8_t *)test_server_cert_pem;
        config.own_cert_size = test_server_cert_pem_size;
        config.own_key = (const uint8_t *)test_server_key_pem;
        config.own_key_size = test_server_key_pem_size;
    }
    int ready = s >= 0 && neon_tls_init(&tls) == NEON_OK && neon_tls_set_config(tls, &config) == NEON_OK;
    xTaskNotify(test_task_handle, ready ? 1U : 2U, eSetValueWithOverwrite);
    if (ready) {
        job->accept_status = neon_tls_accept(tls, s);
        if (job->accept_status != NEON_OK) {
            log_error("server handshake", tls);
        }
        uint32_t echoed = 0U;
        neon_status_t status = job->accept_status;
        while (status == NEON_OK && echoed < job->echo_bytes) {
            uint32_t got = 0U;
            status = neon_tls_read(tls, server_buffer, sizeof(server_buffer), &got);
            if (status == NEON_OK && got == 0U) {
                status = NEON_ERROR;
            }
            if (status == NEON_OK) {
                status = neon_tls_write(tls, server_buffer, got, NULL);
                echoed += got;
            }
        }
        job->echo_status = status;
        if (status == NEON_OK) {
            /* Wait for the client's close_notify before closing. */
            uint32_t got = 0U;
            (void)neon_tls_read(tls, server_buffer, sizeof(server_buffer), &got);
        }
    }
    if (tls != NULL) {
        (void)neon_tls_deinit(tls);
    }
    if (s >= 0) {
        lwip_close(s);
    }
    xTaskNotify(test_task_handle, 3U, eSetValueWithOverwrite);
    vTaskDelete(NULL);
}

static int start_server(server_job_t *job) {
    uint32_t value = 0U;
    if (xTaskCreate(server_task, "tls_server", TEST_TASK_STACK_WORDS, job, 2, NULL) != pdPASS) {
        return 0;
    }
    return xTaskNotifyWait(0U, 0xFFFFFFFFU, &value, portMAX_DELAY) == pdTRUE && value == 1U;
}

static int wait_server(void) {
    uint32_t value = 0U;
    return xTaskNotifyWait(0U, 0xFFFFFFFFU, &value, pdMS_TO_TICKS(2U * TEST_TIMEOUT_MS)) == pdTRUE && value == 3U;
}

static int echo(neon_tls_handle_t *tls, uint32_t size_bytes, uint32_t seed) {
    for (uint32_t i = 0U; i < size_bytes; ++i) {
        client_tx[i] = (uint8_t)(i * 13U + seed);
    }
    memset(client_rx, 0, size_bytes);
    if (neon_tls_write(tls, client_tx, size_bytes, NULL) != NEON_OK) {
        return 0;
    }
    uint32_t received = 0U;
    while (received < size_bytes) {
        uint32_t got = 0U;
        if (neon_tls_read(tls, client_rx + received, size_bytes - received, &got) != NEON_OK || got == 0U) {
            return 0;
        }
        received += got;
    }
    return memcmp(client_tx, client_rx, size_bytes) == 0;
}

static void client_config(neon_tls_config_t *config, const char *ca, size_t ca_size, const char *name) {
    (void)neon_tls_get_default_config(config);
    config->ca = (const uint8_t *)ca;
    config->ca_size = ca_size;
    config->server_name = name;
    config->timeout_ms = TEST_TIMEOUT_MS;
}

static void start_network(void) {
    neon_net_handle_t *net = NULL;
    neon_net_config_t config;
    (void)neon_net_get_default_config(&config);
    config.ipv4_mode = NEON_NET_IPV4_STATIC;
    memcpy(config.ipv4_address, own_ip, 4U);
    config.ipv4_netmask[0] = 255U;
    config.ipv4_netmask[1] = 255U;
    config.ipv4_netmask[2] = 255U;
    check(neon_net_init(&net, NEON_NET0) == NEON_OK && neon_net_set_config(net, &config) == NEON_OK &&
              neon_net_start(net) == NEON_OK && neon_net_wait_ready(net, 5000U) == NEON_OK,
          "network up: 192.168.10.2 over the VP line loopback");
}

static void test_task(void *argument) {
    (void)argument;
    neon_tls_handle_t *tls = NULL;
    neon_tls_handle_t *extra[CONFIG_NEON_MDF_TLS_MAX_SESSIONS];
    neon_tls_config_t config;
    neon_tls_error_t error;
    server_job_t job;
    uint64_t t0;

    start_network();
    const size_t heap_before = xPortGetFreeHeapSize();

    /* Pool and argument checks. */
    uint32_t taken = 0U;
    while (taken < CONFIG_NEON_MDF_TLS_MAX_SESSIONS && neon_tls_init(&extra[taken]) == NEON_OK) {
        ++taken;
    }
    check(taken == CONFIG_NEON_MDF_TLS_MAX_SESSIONS && neon_tls_init(&tls) == NEON_BUSY, "handle pool bounded");
    for (uint32_t i = 0U; i < taken; ++i) {
        (void)neon_tls_deinit(extra[i]);
    }
    check(neon_tls_init(&tls) == NEON_OK, "init client handle (entropy, CTR-DRBG)");
    client_config(&config, NULL, 0U, NULL);
    check(neon_tls_set_config(tls, &config) == NEON_OK &&
              neon_tls_connect(tls, "192.168.10.2", TEST_PORT) == NEON_INVALID_PARAM,
          "verification required without CA or PSK is refused");
    config.ca = (const uint8_t *)"not a certificate";
    config.ca_size = 18U;
    check(neon_tls_set_config(tls, &config) == NEON_ERROR, "malformed CA rejected at set_config");

    /* 1. PSK session. */
    job.mode = MODE_PSK;
    job.echo_bytes = 4096U;
    check(start_server(&job), "PSK server listening");
    (void)neon_tls_get_default_config(&config);
    config.psk = test_psk;
    config.psk_size = sizeof(test_psk);
    config.psk_identity = "fx1-client";
    config.timeout_ms = TEST_TIMEOUT_MS;
    t0 = fx1_mtime();
    check(neon_tls_set_config(tls, &config) == NEON_OK && neon_tls_connect(tls, "192.168.10.2", TEST_PORT) == NEON_OK,
          "PSK handshake (TLS 1.2, PSK-AES-GCM)");
    log_ms("PSK handshake", t0);
    check(neon_tls_get_state(tls) == NEON_TLS_STATE_CONNECTED, "state CONNECTED");
    t0 = fx1_mtime();
    check(echo(tls, 4096U, 1U), "PSK 4 KiB echo");
    log_ms("PSK 4 KiB echo", t0);
    check(neon_tls_close(tls) == NEON_OK && wait_server() && job.accept_status == NEON_OK && job.echo_status == NEON_OK,
          "PSK close_notify, server saw the full echo");

    /* 2. Certificate session: RSA root -> ECDSA P-256 server, ECDHE-ECDSA. */
    job.mode = MODE_CERT;
    job.echo_bytes = 2048U;
    check(start_server(&job), "certificate server listening");
    client_config(&config, test_ca_pem, test_ca_pem_size, "fx1-tls.test");
    t0 = fx1_mtime();
    neon_status_t status = neon_tls_set_config(tls, &config);
    if (status == NEON_OK) {
        status = neon_tls_connect(tls, "192.168.10.2", TEST_PORT);
    }
    if (status != NEON_OK) {
        log_error("client handshake", tls);
    }
    check(status == NEON_OK, "certificate handshake, chain and host name verified");
    log_ms("ECDHE-ECDSA handshake", t0);
    check(echo(tls, 2048U, 2U), "certificate session 2 KiB echo");
    check(neon_tls_close(tls) == NEON_OK && wait_server() && job.echo_status == NEON_OK, "certificate session closed");

    /* 3. Host name mismatch must fail verification. */
    job.echo_bytes = 0U;
    check(start_server(&job), "server for host name test");
    client_config(&config, test_ca_pem, test_ca_pem_size, "wrong.test");
    t0 = fx1_mtime();
    check(neon_tls_set_config(tls, &config) == NEON_OK &&
              neon_tls_connect(tls, "192.168.10.2", TEST_PORT) == NEON_ERROR,
          "host name mismatch rejected");
    log_ms("rejected handshake", t0);
    check(neon_tls_get_error(tls, &error) == NEON_OK && (error.verify_flags & MBEDTLS_X509_BADCERT_CN_MISMATCH) != 0U,
          "verify flags report CN mismatch");
    check(wait_server() && job.accept_status != NEON_OK, "server saw the client abort");

    /* 4. Untrusted root must fail verification. */
    check(start_server(&job), "server for untrusted root test");
    client_config(&config, test_wrong_ca_pem, test_wrong_ca_pem_size, "fx1-tls.test");
    check(neon_tls_set_config(tls, &config) == NEON_OK &&
              neon_tls_connect(tls, "192.168.10.2", TEST_PORT) == NEON_ERROR,
          "untrusted root rejected");
    check(neon_tls_get_error(tls, &error) == NEON_OK && (error.verify_flags & MBEDTLS_X509_BADCERT_NOT_TRUSTED) != 0U,
          "verify flags report NOT_TRUSTED");
    check(wait_server() && job.accept_status != NEON_OK, "server saw the second abort");

    check(neon_tls_deinit(tls) == NEON_OK && neon_tls_get_state(tls) == NEON_TLS_STATE_RESET, "deinit client");
    vTaskDelay(pdMS_TO_TICKS(200U));
    const size_t heap_after = xPortGetFreeHeapSize();
    fx1_puts("[NEON_TLS] heap free before ");
    fx1_putdec((uint32_t)heap_before);
    fx1_puts(" after ");
    fx1_putdec((uint32_t)heap_after);
    fx1_puts(", minimum ever ");
    fx1_putdec((uint32_t)xPortGetMinimumEverFreeHeapSize());
    fx1_putc('\n');
    check(heap_after + 512U >= heap_before, "TLS sessions return their heap");

    fx1_puts("[NEON_TLS] SUMMARY checks=");
    fx1_putdec(checks);
    fx1_puts(" failed=0\n");
    fx1_pass();
}

int main(void) {
    NEON_FX1_FreeRTOS_Init();
    if (xTaskCreate(test_task, "tls_test", TEST_TASK_STACK_WORDS, NULL, 2, &test_task_handle) != pdPASS) {
        fx1_fail(0xe0ffU);
    }
    vTaskStartScheduler();
    fx1_fail(0xe0feU);
}
