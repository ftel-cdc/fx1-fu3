# TLS_Loopback

Validates `neon_tls` (mbedTLS 3.6.8 LTS, TLS 1.2) on the FX1 VP. A TLS server
task and the test (client) task use the interface's own address 192.168.10.2;
with `CONFIG_NEON_MDF_NET_LOOPBACK=n` and `--eth-peer loopback` every record goes
through the MAC, descriptor DMA and PHY (see `NET_Loopback`).

| Step | What it proves |
|---|---|
| handle pool, missing CA, malformed CA | bounded pool, argument and parse errors |
| PSK session + 4 KiB echo + close_notify | TLS 1.2 PSK-AES-GCM, record layer both ways, orderly close |
| certificate session + 2 KiB echo | RSA-2048 root verifies an ECDSA P-256 server certificate (SAN `fx1-tls.test`), ECDHE-ECDSA handshake |
| host name `wrong.test` | client rejects, `MBEDTLS_X509_BADCERT_CN_MISMATCH` |
| untrusted root | client rejects, `MBEDTLS_X509_BADCERT_NOT_TRUSTED` |
| heap before/after | sessions return their FreeRTOS heap |

```sh
. scripts/env/env.sh                     # from the fx1-driver workspace
cd neon_mdf/examples/TLS_Loopback
cmake --preset FX1 -DNEON_FX1_VP_EXECUTABLE="$NEON_FX1_VP_EXECUTABLE"
cmake --build --preset FX1 --target run-vp
```

The asymmetric handshakes are slow on the VP (about 1 MIPS of host speed): see
`build/FX1/vp/result.json` for the measured times.

`Src/test_pki.c` holds **TEST ONLY** certificates and a private key generated
with OpenSSL 3.5:

```sh
openssl req -x509 -newkey rsa:2048 -nodes -keyout ca.key -out ca.crt -days 7300 -sha256 \
  -subj "/O=FTEL NEON TEST ONLY/CN=NEON Test Root CA" \
  -addext "basicConstraints=critical,CA:TRUE" -addext "keyUsage=critical,keyCertSign,cRLSign"
openssl ecparam -name prime256v1 -genkey -noout | openssl pkcs8 -topk8 -nocrypt -out server.key
openssl req -new -key server.key -out server.csr -subj "/O=FTEL NEON TEST ONLY/CN=fx1-tls.test"
openssl x509 -req -in server.csr -CA ca.crt -CAkey ca.key -CAcreateserial -out server.crt \
  -days 7300 -sha256 -extfile server.ext   # CA:FALSE, digitalSignature, serverAuth,
                                           # SAN DNS:fx1-tls.test, IP:192.168.10.2
openssl req -x509 -newkey ec -pkeyopt ec_paramgen_curve:prime256v1 -nodes -keyout wrong_ca.key \
  -out wrong_ca.crt -days 7300 -sha256 -subj "/O=FTEL NEON TEST ONLY/CN=NEON Wrong CA"
```

The build uses `CONFIG_NEON_MDF_TLS_DEV_ENTROPY` (predictable, deterministic on
the VP) and does not check certificate dates (no time source yet). Real servers
(HTTPS, EMQX over MQTTS) need the FPGA and are Phase 1C.
