/**
 * @file sdk_irq.c
 * @brief Implementation for IRQ.
 * @details
 * * sdk_irq.c - implementation THAT cho WCH PFIC (Programmable Fast
 * interrupt Controller) - vendor-specific, KHONG PHAI CLINT/PLIC chuan
 * RISC-V (dung dung nhu chinh sach RESERVED cu da canh bao, xem
 * arch/RISCV/README.md) - dia chi thanh ghi THAT tu CH32V003RM.PDF V1.9
 * chuong 6.5 "Register Description":
 *   IENR1/IENR2 (enable set)   0xE000E100 / 0xE000E104
 *   IRER1/IRER2 (enable clear) 0xE000E180 / 0xE000E184
 *   IPRIORx (priority, 1 byte/IRQ) 0xE000E400 + irqn
 * PFIC la core-private peripheral (giong NVIC cua Cortex-M) - KHONG khai
 * trong mcu-description/devices/CH32V003F4P6.yaml's `peripherals:` (xem
 * comment dau file YAML do), truy cap thang bang dia chi cung o day.
 */

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/

#include "sdk_irq.h"
#include "riscv_csr.h"

/*==================================================================================================
*                                        DEFINES AND MACROS
==================================================================================================*/
#define PFIC_IENR1 (*(volatile uint32_t *)0xE000E100UL)
#define PFIC_IENR2 (*(volatile uint32_t *)0xE000E104UL)
#define PFIC_IRER1 (*(volatile uint32_t *)0xE000E180UL)
#define PFIC_IRER2 (*(volatile uint32_t *)0xE000E184UL)
#define PFIC_IPRIOR_BASE 0xE000E400UL

/*==================================================================================================
*                                         GLOBAL FUNCTIONS
==================================================================================================*/

/** @copydoc SDK_IRQ_Enable */
void SDK_IRQ_Enable(int32_t irqn) {
    if (irqn < 0) {
        return; /* NMI/HardFault (entry 2/3) khong qua PFIC enable - luon bat */
    }
    if (irqn < 32) {
        PFIC_IENR1 = (1U << irqn);
    } else {
        PFIC_IENR2 = (1U << (irqn - 32));
    }
}

/** @copydoc SDK_IRQ_Disable */
void SDK_IRQ_Disable(int32_t irqn) {
    if (irqn < 0) {
        return;
    }
    if (irqn < 32) {
        PFIC_IRER1 = (1U << irqn);
    } else {
        PFIC_IRER2 = (1U << (irqn - 32));
    }
}

/** @copydoc SDK_IRQ_SetPriority */
void SDK_IRQ_SetPriority(int32_t irqn, uint32_t priority) {
    if (irqn < 0) {
        return;
    }
    volatile uint8_t *reg = (volatile uint8_t *)(PFIC_IPRIOR_BASE + (uint32_t)irqn);
    *reg = (uint8_t)priority;
}

/** @copydoc SDK_IRQ_EnterCritical */
uint32_t SDK_IRQ_EnterCritical(void) {
    uint32_t saved = RISCV_CSR_READ(mstatus);
    RISCV_CSR_CLEAR(mstatus, RISCV_MSTATUS_MIE);
    return saved;
}

/** @copydoc SDK_IRQ_ExitCritical */
void SDK_IRQ_ExitCritical(uint32_t saved_state) {
    if (saved_state & RISCV_MSTATUS_MIE) {
        RISCV_CSR_SET(mstatus, RISCV_MSTATUS_MIE);
    }
}
