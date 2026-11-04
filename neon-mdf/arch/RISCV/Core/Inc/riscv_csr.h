/**
 * @file riscv_csr.h
 * @brief Public declarations for riscv csr.
 * @details
 * * riscv_csr.h - macro doc/ghi CSR (Control and Status Register) toi thieu,
 * tuong duong vai tro cua CMSIS-Core (core_cmX.h) ben Cortex-M nhung tu
 * viet (ARM khong cap CMSIS cho RISC-V - cong dong RISC-V thuong dung
 * `riscv-csr-encoding.h`-style macro nhu duoi day, xem
 * arch/RISCV/README.md).
 *
 * CHI can inline asm chuan cua GCC (khong phu thuoc extension nao ngoai
 * Zicsr, da bat qua -march=..._zicsr, xem cmake/arch/riscv.cmake).
 */

#ifndef RISCV_CSR_H
#define RISCV_CSR_H

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include <stdint.h>

/*==================================================================================================
*                                        DEFINES AND MACROS
==================================================================================================*/
#define RISCV_CSR_READ(csr)                                    \
  ({                                                            \
    uint32_t __v;                                               \
    __asm volatile("csrr %0, " #csr : "=r"(__v));                \
    __v;                                                        \
  })

#define RISCV_CSR_WRITE(csr, val) __asm volatile("csrw " #csr ", %0" ::"r"((uint32_t)(val)))
#define RISCV_CSR_SET(csr, val)   __asm volatile("csrs " #csr ", %0" ::"r"((uint32_t)(val)))
#define RISCV_CSR_CLEAR(csr, val) __asm volatile("csrc " #csr ", %0" ::"r"((uint32_t)(val)))

/* mstatus.MIE (bit 3) - co bat/tat ngat toan cuc CHUAN cua RISC-V (khong
 * phai vendor-specific, khac PFIC/CSR 0x804 chi dieu khien nesting/HPE) -
 * dung boi arch/RISCV/Port/Src/sdk_irq.c cho EnterCritical/ExitCritical. */
#define RISCV_MSTATUS_MIE (1u << 3)

/* __NOP() - CUNG TEN voi macro CMSIS ben Cortex-M (core_cmX.h) - de code
 * app dung chung (vd Blinky/Src/main.c's delay_busy()) khong can #ifdef
 * rieng theo kien truc chi vi 1 lenh NOP. */
#define __NOP() __asm volatile("nop")

#endif /* RISCV_CSR_H */
