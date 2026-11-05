/**
 * @file riscv_libc_shim.c
 * @brief Implementation for riscv libc shim.
 * @details
 * * riscv_libc_shim.c implements libc_shim/string.h. FreeRTOS-Kernel needs memcpy/memset/strlen, but
 * the bare-metal RISC-V ilp32e toolchain has neither libc nor a matching multilib. Simple byte
 * loops work across supported ISAs without assuming alignment, word size, or instructions beyond
 * rv32ec. Like riscv_soft_math.c, these helpers avoid assuming hardware extensions absent from
 * CH32V003F4P6.
 *
 * Intentionally unoptimized: no lwu or word-copy path. FreeRTOS_Blinky copies small TCB/queue items,
 * so correctness matters more than speed. Any later optimization must verify alignment
 * requirements; RVE does not relax RV32I alignment rules. Before switching to
 * word-copy.
 */

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/

#include "string.h"

/*==================================================================================================
*                                         GLOBAL FUNCTIONS
==================================================================================================*/

/** @copydoc memcpy */
void *memcpy(void *dst, const void *src, size_t n)
{
  unsigned char *d = (unsigned char *)dst;
  const unsigned char *s = (const unsigned char *)src;
  while (n--) {
    *d++ = *s++;
  }
  return dst;
}

/** @copydoc memset */
void *memset(void *dst, int c, size_t n)
{
  unsigned char *d = (unsigned char *)dst;
  unsigned char v = (unsigned char)c;
  while (n--) {
    *d++ = v;
  }
  return dst;
}

/** @copydoc memmove */
void *memmove(void *dst, const void *src, size_t n)
{
  unsigned char *d = (unsigned char *)dst;
  const unsigned char *s = (const unsigned char *)src;
  if (d == s || n == 0) {
    return dst;
  }
  if (d < s) {
    while (n--) {
      *d++ = *s++;
    }
  } else {
    d += n;
    s += n;
    while (n--) {
      *--d = *--s;
    }
  }
  return dst;
}

/** @copydoc strlen */
size_t strlen(const char *s)
{
  size_t n = 0;
  while (s[n] != '\0') {
    n++;
  }
  return n;
}
