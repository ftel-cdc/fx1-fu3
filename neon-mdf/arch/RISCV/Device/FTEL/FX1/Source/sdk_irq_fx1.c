/* Port API IRQ của Neon sang PLIC, context M-mode của hart hiện tại. */
#include "sdk_irq.h"
#include "fx1_fw.h"

static int valid_irq(int32_t irqn)
{
    return irqn > 0 && (uint32_t)irqn < FX1_PLIC_NUM_SOURCES;
}

void SDK_IRQ_Enable(int32_t irqn)
{
    if (!valid_irq(irqn)) return;
    uint32_t saved = SDK_IRQ_EnterCritical();
    fx1_plic_enable(FX1_PLIC_CONTEXT_HART_M(fx1_hartid()), (uint32_t)irqn, 1);
    fx1_mb();
    SDK_IRQ_ExitCritical(saved);
}

void SDK_IRQ_Disable(int32_t irqn)
{
    if (!valid_irq(irqn)) return;
    uint32_t saved = SDK_IRQ_EnterCritical();
    fx1_plic_enable(FX1_PLIC_CONTEXT_HART_M(fx1_hartid()), (uint32_t)irqn, 0);
    fx1_mb();
    SDK_IRQ_ExitCritical(saved);
}

void SDK_IRQ_SetPriority(int32_t irqn, uint32_t priority)
{
    if (!valid_irq(irqn) || priority > FX1_PLIC_PRIORITY_MAX) return;
    fx1_plic_set_priority((uint32_t)irqn, priority);
    fx1_mb();
}

uint32_t SDK_IRQ_EnterCritical(void)
{
    return fx1_irq_save();
}

void SDK_IRQ_ExitCritical(uint32_t saved_state)
{
    fx1_irq_restore(saved_state);
}
