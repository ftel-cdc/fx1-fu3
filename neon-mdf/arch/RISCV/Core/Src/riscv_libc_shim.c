/**
 * @file riscv_libc_shim.c
 * @brief Implementation for riscv libc shim.
 * @details
 * * riscv_libc_shim.c - implementation THAT cho libc_shim/string.h (xem file
 * do cho ly do day du: FreeRTOS-Kernel core can memcpy/memset/strlen ma
 * toolchain RISC-V bare-metal (ABI ilp32e, khong multilib khop) khong co
 * libc that de cung cap). Tu viet DON GIAN, AN TOAN cho MOI ISA - chi dung
 * vong lap byte-by-byte (khong dung lenh nao ngoai rv32ec/tao gia dinh ve
 * alignment/word size), giong tinh than riscv_soft_math.c (tu cung cap
 * thay vi doan sai extension phan cung khong co that tren CH32V003F4P6).
 *
 * KHONG toi uu (khong dung lwu/word-copy) - demo FreeRTOS_Blinky chi copy
 * vai chuc byte (TCB/queue item nho) nen hieu nang khong quan trong bang
 * DUNG/AN TOAN. Neu sau nay can toi uu, phai kiem tra ky alignment that
 * (RVE khong co gi dam bao khac RV32I ve alignment) truoc khi doi sang
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
