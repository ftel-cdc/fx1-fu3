/**
 * @file stdlib.h
 * @brief Public declarations for stdlib.
 * @details
 * * stdlib.h - empty shim for FreeRTOS-Kernel core on RISC-V - see string.h scene this file for reason
 * that's enough (no real libc for ABI
 * ilp32e). Verified by searching all of list.c/tasks.c/queue.c/timers.c/
 * heap_4.c includes <stdlib.h> but does not call any functions from it
 * (malloc/free/abs/qsort... both unused - FreeRTOS from private heap management via heap_4.c,
 * unused malloc() heap) - just #include is "successful" (no preprocessor errors), no further
 * declaration needed.
 */

#ifndef NEON_MDF_RISCV_LIBC_SHIM_STDLIB_H
#define NEON_MDF_RISCV_LIBC_SHIM_STDLIB_H

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include <stddef.h> /* size_t/NULL - in case later code is needed */

#endif /* NEON_MDF_RISCV_LIBC_SHIM_STDLIB_H */
