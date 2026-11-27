#include "fx1_isp_irq.h"
#include "fx1_isp/fx1_isp_csr.h"
#define R(name) FX1_ISP_##name##_OFFSET
void fx1_isp_irq_gate(const fx1_isp_io_t *io)
{
    io->write(io->user, R(COMMON_IRQ_EN), 0);
    io->write(io->user, R(DMA_IRQ_EN), 0);
    io->write(io->user, R(OUTPUT_IRQ_EN), 0);
    io->barrier(io->user);
}
fx1_isp_irq_bits_t fx1_isp_irq_read(const fx1_isp_io_t *io)
{
    return (fx1_isp_irq_bits_t){io->read(io->user, R(COMMON_IRQ_STATUS)),
        io->read(io->user, R(DMA_IRQ_STAT)), io->read(io->user, R(OUTPUT_IRQ_STATUS))};
}
void fx1_isp_irq_ack(const fx1_isp_io_t *io, fx1_isp_irq_bits_t c, fx1_isp_irq_bits_t s)
{
    uint32_t common = c.common & s.common & FX1_ISP_COMMON_IRQ_STATUS_W1C_MASK;
    uint32_t dma = c.dma & s.dma & FX1_ISP_DMA_IRQ_STAT_W1C_MASK;
    uint32_t output = c.output & s.output & FX1_ISP_OUTPUT_IRQ_STATUS_W1C_MASK;
    if (common) io->write(io->user, R(COMMON_IRQ_STATUS), common);
    if (dma) io->write(io->user, R(DMA_IRQ_STAT), dma);
    if (output) io->write(io->user, R(OUTPUT_IRQ_STATUS), output);
    io->barrier(io->user);
}
bool fx1_isp_irq_rearm(const fx1_isp_io_t *io, fx1_isp_irq_bits_t d)
{
    d.common &= FX1_ISP_COMMON_IRQ_EN_RW_MASK;
    d.dma &= FX1_ISP_DMA_IRQ_EN_RW_MASK & ~(uint32_t)FX1_IRQ_UNDERRUN;
    d.output &= FX1_ISP_OUTPUT_IRQ_EN_RW_MASK;
    io->write(io->user, R(COMMON_IRQ_EN), d.common);
    io->write(io->user, R(DMA_IRQ_EN), d.dma);
    io->write(io->user, R(OUTPUT_IRQ_EN), d.output);
    io->barrier(io->user);
    fx1_isp_irq_bits_t p = fx1_isp_irq_read(io);
    if ((p.common & d.common) || (p.dma & d.dma) || (p.output & d.output)) {
        fx1_isp_irq_gate(io);
        return true;
    }
    return false;
}
fx1_isp_irq_bits_t fx1_isp_irq_replay_masks(uint32_t hardware_caps)
{
    /* Statistics are disabled in the scalar profile. Still poll every engine;
     * no undrainable STATS_READY or flow-control level source on the pin. */
    return (fx1_isp_irq_bits_t){FX1_ISP_COMMON_IRQ_EN_ERROR_EN_MASK,
        FX1_IRQ_CFG | FX1_IRQ_INPUT_DONE | FX1_IRQ_OUTPUT_DONE | FX1_IRQ_AXI |
        ((hardware_caps & FX1_ISP_TUNE_CAPS_INPUT_STREAM_MASK) ? FX1_IRQ_STREAM : 0u),
        (FX1_ISP_OUTPUT_IRQ_EN_DONE_MASK & 1u) | (FX1_ISP_OUTPUT_IRQ_EN_ERROR_MASK & 16u)};
}
