/**
 * @file sdk_irq.h
 * @brief Public declarations for IRQ.
 * @details
 * PFIC interrupt control and CSR mstatus critical sections for WCH RISC-V.
 * RISC-V applications and backends use these APIs for interrupt control.
 * Cortex-M backends use their CMSIS NVIC/PRIMASK APIs directly.
 */

#ifndef SDK_IRQ_H
#define SDK_IRQ_H

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*==================================================================================================
*                                       FUNCTION PROTOTYPES
==================================================================================================*/
/**
 * @brief enable IRQ.
 * @param[in] irqn Device interrupt number.
 */
void SDK_IRQ_Enable(int32_t irqn);
/**
 * @brief Disable IRQ.
 * @param[in] irqn Device interrupt number.
 */
void SDK_IRQ_Disable(int32_t irqn);
/**
 * @brief Set IRQ priority.
 * @param[in] irqn Device interrupt number.
 * @param[in] priority interrupt priority.
 */
void SDK_IRQ_SetPriority(int32_t irqn, uint32_t priority);

/**
 * @brief Mask interrupts and return the previous interrupt-mask state.
 * @return The requested value or operation result.
 * @details
 * Disable interrupt, return previous state to pass into SDK_IRQ_ExitCritical().
 */
uint32_t SDK_IRQ_EnterCritical(void);
/**
 * @brief Restore the interrupt-mask state saved on entry to a critical section.
 * @param[in] saved_state interrupt state returned by SDK_IRQ_EnterCritical.
 * @details
 * Restore interrupt state saved from SDK_IRQ_EnterCritical().
 */
void SDK_IRQ_ExitCritical(uint32_t saved_state);

#ifdef __cplusplus
}
#endif

#endif /* SDK_IRQ_H */
