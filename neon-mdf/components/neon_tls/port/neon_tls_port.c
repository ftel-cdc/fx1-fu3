/**
 * @file    neon_tls_port.c
 * @brief   mbedTLS platform services on FX1: FreeRTOS heap, entropy source, C library helpers.
 * @details Entropy: mbedtls_hardware_poll() (MBEDTLS_ENTROPY_HARDWARE_ALT) calls
 *          neon_tls_port_entropy(), a weak function. A board with a TRNG overrides it. Without an
 *          override, CONFIG_NEON_MDF_TLS_DEV_ENTROPY supplies timer-jitter bytes that are predictable
 *          (deterministic on the VP); otherwise seeding fails and no TLS session can start.
 */

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include "neon_tls_port.h"
#include "FreeRTOS.h"
#include "fx1_fw.h"
#include "sdkconfig.h"
#include <stddef.h>
#include <stdint.h>

/*==================================================================================================
*                                       DEFINES AND MACROS
==================================================================================================*/
#define PORT_ENTROPY_FAILED (-1)

/*==================================================================================================
*                                        LOCAL VARIABLES
==================================================================================================*/
#ifdef CONFIG_NEON_MDF_TLS_DEV_ENTROPY
static uint64_t port_entropy_state;
static uint32_t port_entropy_warned;
#endif

/*==================================================================================================
*                                         GLOBAL FUNCTIONS
==================================================================================================*/
/** @copydoc neon_tls_port_calloc */
void *neon_tls_port_calloc(size_t count, size_t size) {
    if (count == 0U || size == 0U) {
        return NULL;
    }
    /* heap_4 pvPortCalloc checks count * size for overflow and zeroes the block. */
    return pvPortCalloc(count, size);
}

/** @copydoc neon_tls_port_free */
void neon_tls_port_free(void *pointer) {
    if (pointer != NULL) {
        vPortFree(pointer);
    }
}

/** @copydoc neon_tls_port_entropy */
__attribute__((weak)) int neon_tls_port_entropy(uint8_t *output, size_t size_bytes) {
#ifdef CONFIG_NEON_MDF_TLS_DEV_ENTROPY
    if (port_entropy_warned == 0U) {
        port_entropy_warned = 1U;
        fx1_puts("[NEON_TLS] WARNING: development entropy (timer jitter), keys are predictable\n");
    }
    for (size_t i = 0U; i < size_bytes; ++i) {
        /* Mix mtime around a data-dependent busy loop; splitmix64 finalizer per byte. */
        uint64_t x = port_entropy_state + fx1_mtime() + 0x9E3779B97F4A7C15ULL;
        for (uint32_t spin = (uint32_t)(x & 0x1FU); spin > 0U; --spin) {
            x ^= fx1_mtime() << (spin & 31U);
        }
        x = (x ^ (x >> 30)) * 0xBF58476D1CE4E5B9ULL;
        x = (x ^ (x >> 27)) * 0x94D049BB133111EBULL;
        x ^= x >> 31;
        port_entropy_state = x;
        output[i] = (uint8_t)(x >> 56);
    }
    return 0;
#else
    (void)output;
    (void)size_bytes;
    return PORT_ENTROPY_FAILED;
#endif
}

/**
 * @brief C library strchr() for mbedTLS (x509_crt.c); the FX1 image links no C library.
 * @param[in] text NUL-terminated string.
 * @param[in] c Character to find (converted to char); '\0' finds the terminator.
 * @return Pointer to the first occurrence, or NULL.
 */
char *strchr(const char *text, int c) {
    const char wanted = (char)c;
    for (;; ++text) {
        if (*text == wanted) {
            return (char *)text;
        }
        if (*text == '\0') {
            return NULL;
        }
    }
}

/**
 * @brief C library strstr() for mbedTLS (PEM and certificate parsing).
 * @param[in] haystack NUL-terminated string to search.
 * @param[in] needle NUL-terminated string to find; empty matches at haystack.
 * @return Pointer to the first occurrence, or NULL.
 */
char *strstr(const char *haystack, const char *needle) {
    if (*needle == '\0') {
        return (char *)haystack;
    }
    for (; *haystack != '\0'; ++haystack) {
        const char *h = haystack;
        const char *n = needle;
        while (*n != '\0' && *h == *n) {
            ++h;
            ++n;
        }
        if (*n == '\0') {
            return (char *)haystack;
        }
    }
    return NULL;
}

/**
 * @brief mbedTLS MBEDTLS_ENTROPY_HARDWARE_ALT contract.
 * @param[in] data Unused.
 * @param[out] output Entropy bytes.
 * @param[in] len Requested bytes.
 * @param[out] olen Bytes produced.
 * @return 0, or MBEDTLS_ERR_ENTROPY_SOURCE_FAILED (-0x003C) when no source is available.
 */
int mbedtls_hardware_poll(void *data, unsigned char *output, size_t len, size_t *olen) {
    (void)data;
    if (neon_tls_port_entropy(output, len) != 0) {
        *olen = 0U;
        return -0x003C;
    }
    *olen = len;
    return 0;
}
