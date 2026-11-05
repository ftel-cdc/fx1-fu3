/**
 * @file riscv_csr.h
 * @brief Public declarations for riscv csr.
 * @details
 * * riscv_csr.h provides minimal CSR (Control and Status Register) read/write macros, serving a role
 * similar to CMSIS-Core core_cmX.h on Cortex-M. ARM does not provide CMSIS for RISC-V; these GCC
 * macros follow the community `riscv-csr-encoding.h` style. See
 * arch/RISCV/README.md).
 *
 * Uses standard GCC inline assembly and only the Zicsr extension, enabled through -march=..._zicsr;
 * see cmake/arch/riscv.cmake.
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

/* mstatus.MIE (bit 3) is the standard RISC-V global interrupt enable, distinct from vendor PFIC/CSR
 * 0x804 nesting/HPE controls. arch/RISCV/Port/Src/sdk_irq.c uses it for EnterCritical/ExitCritical. */
#define RISCV_MSTATUS_MIE (1u << 3)

/* __NOP() matches the CMSIS name in Cortex-M core_cmX.h, allowing shared apps such as
 * Blinky/Src/main.c delay_busy() to issue NOP without architecture-specific #ifdef branches. */
#define __NOP() __asm volatile("nop")

#endif /* RISCV_CSR_H */
