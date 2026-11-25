# Neon TLS (`neon_tls`)

TLS 1.2 sessions with mbedTLS 3.6.8 LTS over lwIP sockets of `neon_net`.
Include `neon_tls.h`.

```text
neon_tls/
  neon_tls.h                        public API (client connect / server accept, read/write/close)
  src/neon_tls.c                    sessions: handle pool, mbedTLS contexts, socket BIO with timeouts
  port/include/neon_tls_mbedtls_config.h   MBEDTLS_CONFIG_FILE (+ optional neon_tls_mbedtls_config_app.h)
  port/include/neon_tls_port.h      platform hooks (heap, entropy)
  port/neon_tls_port.c              FreeRTOS heap, entropy (weak hook), strchr/strstr
  third_party/mbedtls/              mbedTLS 3.6.8 include/ + library/, unmodified (see below)
```

## Build

```cmake
include(${NEON_MDF_PATH}/cmake/neon_net.cmake)
include(${NEON_MDF_PATH}/cmake/neon_tls.cmake)
neon_mdf_add_freertos(${NEON_MDF_TARGET} ${CMAKE_CURRENT_SOURCE_DIR}/Inc)
neon_mdf_add_net()
neon_mdf_add_tls()                       # needs CONFIG_NEON_MDF_TLS=y
target_link_libraries(app PRIVATE neon_mdf::tls neon_mdf::net neon_mdf::freertos)
```

Kconfig (menu "FTEL FX1 network stack"): `NEON_MDF_TLS`, `NEON_MDF_TLS_MAX_SESSIONS`
(static pool), `NEON_MDF_TLS_DEV_ENTROPY`.

Resources per live session: about 48 KiB of FreeRTOS heap at the peak of an ECDHE
handshake (16 KiB input record, 4 KiB output record, handshake/ECP state, parsed
certificates), measured on the VP with `TLS_Loopback`. TLS tasks need a deep
stack: 16 KiB (`4096` words) is used in `examples/TLS_Loopback`. The library adds
about 240 KiB of code.

## Usage

```c
neon_tls_config_t config;
neon_tls_get_default_config(&config);        /* client, verification required, 10 s */
config.ca = ca_pem;          config.ca_size = sizeof(ca_pem);   /* PEM size includes the NUL */
config.server_name = "broker.example.com";    /* SNI + certificate host name */
config.alpn = "mqtt";                          /* optional */
neon_tls_init(&tls);
neon_tls_set_config(tls, &config);            /* parses and copies certificates/keys */
neon_tls_connect(tls, "broker.example.com", 8883U);
neon_tls_write(tls, data, size, NULL);
neon_tls_read(tls, buffer, sizeof(buffer), &got);   /* got == 0: peer closed */
neon_tls_close(tls);                           /* close_notify; handle reusable */
neon_tls_deinit(tls);
```

Server role: `config.role = NEON_TLS_ROLE_SERVER`, `own_cert`/`own_key`, then
`neon_tls_accept(tls, listen_socket)`. Mutual TLS: `own_cert`/`own_key` on the
client, `verify = REQUIRED` + `ca` on the server. PSK: `psk`, `psk_size`,
`psk_identity` (PSK-only sessions offer only PSK suites).

Failures: `neon_tls_get_error()` gives the mbedTLS error code, the X.509 verify
flags (`MBEDTLS_X509_BADCERT_*`) and the socket errno.

## mbedTLS configuration

- TLS 1.2 only, client and server. Key exchanges ECDHE-ECDSA, ECDHE-RSA, PSK;
  AES-128/256-GCM and ChaCha20-Poly1305; SHA-224/256/384/512; curves P-256,
  P-384, X25519; RSA PKCS#1 v1.5/PSS verification and signing; X.509 parse/verify;
  SNI, ALPN, max fragment length, encrypt-then-MAC, extended master secret.
- Records: 16 KiB in (servers may send full records), 4 KiB out.
- No PSA crypto, no file system, no `net_sockets.c` (the BIO is in `neon_tls.c`).
- **No time source**: `MBEDTLS_HAVE_TIME(_DATE)` off, so certificate validity
  dates are not checked. Add it with SNTP/RTC before production.
- **Entropy**: `MBEDTLS_ENTROPY_HARDWARE_ALT` → `neon_tls_port_entropy()` (weak).
  `CONFIG_NEON_MDF_TLS_DEV_ENTROPY` supplies timer jitter: **predictable**, and on
  the VP fully deterministic; a warning is printed once. A board with a TRNG
  overrides `neon_tls_port_entropy()`.
- Hardware crypto: none yet (FX1 crypto documents pending). It would plug in
  through `MBEDTLS_*_ALT` options in the configuration and a TRNG entropy hook.
- The FX1 image links no C library: heap → `pvPortCalloc`/`vPortFree`,
  `snprintf`/`vsnprintf`/`printf` → the `neon_net` port formatter,
  `strchr`/`strstr` in `neon_tls_port.c`.

## Threading

One task uses a handle at a time; handles are independent (each owns its
entropy pool and CTR-DRBG; `MBEDTLS_THREADING_C` is off). Socket waits are
bounded by `timeout_ms`; a write timeout or fatal error leaves the session
`CLOSED` (call `close` before reuse).

## Testing

`examples/TLS_Loopback` runs a TLS server and client on the VP's own address
over the MAC/DMA/PHY line loopback: PSK session, certificate session
(RSA root → ECDSA P-256 server, ECDHE-ECDSA), host name mismatch and untrusted
root rejections, heap accounting.

## Third party and support window

`third_party/mbedtls` is Mbed TLS 3.6.8 from the release archive
`mbedtls-3.6.8.tar.bz2` (SHA-256 `bc54b818e87f49c0b3281dd85d24adaede0e63b4e822e3fbd49c005a38b9595b`,
https://github.com/Mbed-TLS/mbedtls/releases/tag/mbedtls-3.6.8): `include/`,
`library/` (without its CMakeLists.txt/Makefile), `LICENSE` (Apache-2.0 OR
GPL-2.0-or-later), `README.md`, `ChangeLog`; no file modified. The 3.6 LTS branch
is supported until **March 2027** (`BRANCHES.md` upstream); the 4.x LTS removes
the legacy crypto and `*_ALT` APIs, so plan the migration before release.
