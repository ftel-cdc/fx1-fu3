/**
 * @file test_pki.c
 * @brief TEST ONLY certificates and key for TLS_Loopback. Never trust them anywhere.
 * @details Generated with OpenSSL 3.5 (see README.md): an RSA-2048 root signs an ECDSA P-256 server
 *          certificate for fx1-tls.test / 192.168.10.2, so the device verifies an RSA signature and
 *          runs ECDHE-ECDSA without an RSA private-key operation. A second (EC) root is the
 *          untrusted CA of the negative test. Validity 2026-2046 (dates are not checked yet).
 */
#include "test_pki.h"

/* NEON Test Root CA, RSA-2048, self-signed */
const char test_ca_pem[] =
    "-----BEGIN CERTIFICATE-----\n"
    "MIIDZTCCAk2gAwIBAgIUPd00m7DDaabwV1nOYojtEC62aF8wDQYJKoZIhvcNAQEL\n"
    "BQAwOjEcMBoGA1UECgwTRlRFTCBORU9OIFRFU1QgT05MWTEaMBgGA1UEAwwRTkVP\n"
    "TiBUZXN0IFJvb3QgQ0EwHhcNMjYxMTI1MjEyNjM5WhcNNDYxMTIwMjEyNjM5WjA6\n"
    "MRwwGgYDVQQKDBNGVEVMIE5FT04gVEVTVCBPTkxZMRowGAYDVQQDDBFORU9OIFRl\n"
    "c3QgUm9vdCBDQTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBAL5jHtOT\n"
    "GNzPaki+Wvdzz0qecOd07Ar40w79MZchzAzbuAjmjM0dSBwFYFlHU7b6Jsbdj7Rx\n"
    "cgxlhrUiIpesKEzkesnPyJdEbMeiD7aKzVEPCxijECXQ4syCsu1CnmMPLT9JOYye\n"
    "7vvAAF9IoBmYvYf1Ti68O2E9mFVQhgEpNIqJvEHsUepBUrpmokaa00S2zYKYSXzD\n"
    "DiY7QUpUG9UEHCOUfwpRUcEzCjRAhkUHJ13gJlVB0AYttjSz519A8omcb3hCdCmu\n"
    "nN36diy9kJfD5yYYQaPDAdH2WCk6dXn/0nOrvHCem3lLdYjoFOVSansgQ6ygBwXU\n"
    "niz6D3zzUpDX63UCAwEAAaNjMGEwHQYDVR0OBBYEFB1wwJFIbxm9FoVm/6sWPZ6+\n"
    "OsZfMB8GA1UdIwQYMBaAFB1wwJFIbxm9FoVm/6sWPZ6+OsZfMA8GA1UdEwEB/wQF\n"
    "MAMBAf8wDgYDVR0PAQH/BAQDAgEGMA0GCSqGSIb3DQEBCwUAA4IBAQAYU5Vpm3an\n"
    "2p4g9EMKzk8ADcCeM+m89d2pZMZA7RKl+JP+RCpqtQhg63kqttKrjHCnzvke+byo\n"
    "+ud0yhiWqtCDOSEkSxDPlW0VZGPQa/Y8mwEsxgcqdZNDoj0JMFx/ufqEo9YMCeHU\n"
    "YWHlH7wz4J9Zrt+i7lHv9gsAzZh732b2qNI8rFs+UbDlklCuaym3Rtr/OpbaIQt/\n"
    "IpNaqxCOg8VBWci3HZFoP6p2nI4u4gxBj4egiykrn5Y2ViUHl/LaDVIqXBKeO5e0\n"
    "P6zXZJRgXGVmQOaxSxSVEuhwAOoyXn0I10IHXKJ7beg9qk32jS+/qww/JxQkWyrv\n"
    "7cQ76Ls+B2Xh\n"
    "-----END CERTIFICATE-----\n";
const size_t test_ca_pem_size = sizeof(test_ca_pem);

/* fx1-tls.test, ECDSA P-256, issued by NEON Test Root CA */
const char test_server_cert_pem[] =
    "-----BEGIN CERTIFICATE-----\n"
    "MIICxTCCAa2gAwIBAgIURLa0KYanWZS9t5F7C/Vt314vDcIwDQYJKoZIhvcNAQEL\n"
    "BQAwOjEcMBoGA1UECgwTRlRFTCBORU9OIFRFU1QgT05MWTEaMBgGA1UEAwwRTkVP\n"
    "TiBUZXN0IFJvb3QgQ0EwHhcNMjYxMTI1MjEyNjM5WhcNNDYxMTIwMjEyNjM5WjA1\n"
    "MRwwGgYDVQQKDBNGVEVMIE5FT04gVEVTVCBPTkxZMRUwEwYDVQQDDAxmeDEtdGxz\n"
    "LnRlc3QwWTATBgcqhkjOPQIBBggqhkjOPQMBBwNCAAS1riCgg7HdOhWyODOLdosJ\n"
    "e+5/FQ0ZpV+wjAyTa6OjdX8cet15lJMNf0Z+iFKcCpITy8FpeO0TsCM1odtCQs7a\n"
    "o4GSMIGPMAkGA1UdEwQCMAAwDgYDVR0PAQH/BAQDAgeAMBMGA1UdJQQMMAoGCCsG\n"
    "AQUFBwMBMB0GA1UdEQQWMBSCDGZ4MS10bHMudGVzdIcEwKgKAjAdBgNVHQ4EFgQU\n"
    "n5e26am9iMAut3LyIZ0jita6dvgwHwYDVR0jBBgwFoAUHXDAkUhvGb0WhWb/qxY9\n"
    "nr46xl8wDQYJKoZIhvcNAQELBQADggEBAH3i4QvO4Kd/VSsjJaU77jlp1fXtOc3H\n"
    "/16JTRJeCJKodi2knwe65+KBcflTTKRHVuH8ETnbXi24SIA1I859TV17iNOC19p3\n"
    "FBh8p50rEj/iIeAiCoUMWtoc9tRQRXIlC0IuTRo1pwAYmVI711tUZpIFZI69t01g\n"
    "U9ZM6WGHd7BwnKrPv8+mZnoG+vKl3nRuRF8GGQN9wZPZBmtfX2YkMQgAj4RSD7ra\n"
    "birN7oEDaY46DyooQFrbeP2qLqBO6oWMef/5Bwc/xie9olz8uY9njB8x6EZFeVsg\n"
    "9F5aeObXocWb63hQXw7A5/OwDtwduj32hB/5PySH2af1DzdjlDZe1Hc=\n"
    "-----END CERTIFICATE-----\n";
const size_t test_server_cert_pem_size = sizeof(test_server_cert_pem);

/* Private key of test_server_cert_pem (PKCS#8, unencrypted) - TEST ONLY */
const char test_server_key_pem[] =
    "-----BEGIN PRIVATE KEY-----\n"
    "MIGHAgEAMBMGByqGSM49AgEGCCqGSM49AwEHBG0wawIBAQQgIesGACzTFkFLgVLu\n"
    "1t5Sf5k+oDrdI+fmRucD3rm4EwyhRANCAAS1riCgg7HdOhWyODOLdosJe+5/FQ0Z\n"
    "pV+wjAyTa6OjdX8cet15lJMNf0Z+iFKcCpITy8FpeO0TsCM1odtCQs7a\n"
    "-----END PRIVATE KEY-----\n";
const size_t test_server_key_pem_size = sizeof(test_server_key_pem);

/* NEON Wrong CA, ECDSA P-256, unrelated root */
const char test_wrong_ca_pem[] =
    "-----BEGIN CERTIFICATE-----\n"
    "MIIBwTCCAWegAwIBAgIUEtuIDpfdoxhXyEY3bG22JCR0KtowCgYIKoZIzj0EAwIw\n"
    "NjEcMBoGA1UECgwTRlRFTCBORU9OIFRFU1QgT05MWTEWMBQGA1UEAwwNTkVPTiBX\n"
    "cm9uZyBDQTAeFw0yNjExMjUyMTI2MzlaFw00NjExMjAyMTI2MzlaMDYxHDAaBgNV\n"
    "BAoME0ZURUwgTkVPTiBURVNUIE9OTFkxFjAUBgNVBAMMDU5FT04gV3JvbmcgQ0Ew\n"
    "WTATBgcqhkjOPQIBBggqhkjOPQMBBwNCAATdhuwkSWiRbB167BbUa5yKSAlzz3mv\n"
    "bS9Bo1PgfgKSEEWpPzaisoe9fPp9gaBZBuoOpAPaXKlJPSznUNdRJ0vho1MwUTAd\n"
    "BgNVHQ4EFgQU/oCtYluQYzWcRw3aZZHEM/PDrpEwHwYDVR0jBBgwFoAU/oCtYluQ\n"
    "YzWcRw3aZZHEM/PDrpEwDwYDVR0TAQH/BAUwAwEB/zAKBggqhkjOPQQDAgNIADBF\n"
    "AiBkTbAiJh+HiB2KyQvYOUC/5MqdrgneFRhMtbxHj5E08QIhAOMJult15OKn341o\n"
    "spl7MGktQrwsnEJLCxBlMdAtLJ1C\n"
    "-----END CERTIFICATE-----\n";
const size_t test_wrong_ca_pem_size = sizeof(test_wrong_ca_pem);
