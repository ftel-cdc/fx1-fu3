/**
 * @file test_pki.h
 * @brief TEST ONLY PKI of TLS_Loopback (PEM, sizes include the terminating NUL).
 */
#ifndef TEST_PKI_H
#define TEST_PKI_H

#include <stddef.h>

extern const char test_ca_pem[];
extern const size_t test_ca_pem_size;
extern const char test_server_cert_pem[];
extern const size_t test_server_cert_pem_size;
extern const char test_server_key_pem[];
extern const size_t test_server_key_pem_size;
extern const char test_wrong_ca_pem[];
extern const size_t test_wrong_ca_pem_size;

#endif
