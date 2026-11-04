/**
 * @file stdlib.h
 * @brief Public declarations for stdlib.
 * @details
 * * stdlib.h - shim RONG cho FreeRTOS-Kernel core tren RISC-V - xem
 * string.h canh file nay cho ly do day du (khong co libc that cho ABI
 * ilp32e). Da ra soat THAT (grep) toan bo list.c/tasks.c/queue.c/timers.c/
 * heap_4.c - #include <stdlib.h> co mat nhung KHONG thuc su goi ham nao tu
 * no (malloc/free/abs/qsort... deu KHONG dung - FreeRTOS tu quan ly heap
 * rieng qua heap_4.c, khong dung malloc() he thong) - chi can #include nay
 * "thanh cong" (khong loi preprocessor) la du, khong can khai bao gi them.
 */

#ifndef NEON_MDF_RISCV_LIBC_SHIM_STDLIB_H
#define NEON_MDF_RISCV_LIBC_SHIM_STDLIB_H

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include <stddef.h> /* size_t/NULL - phong truong hop code sau nay can */

#endif /* NEON_MDF_RISCV_LIBC_SHIM_STDLIB_H */
