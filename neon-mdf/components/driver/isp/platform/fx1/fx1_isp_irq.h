#ifndef FX1_ISP_IRQ_H
#define FX1_ISP_IRQ_H
#include <stdbool.h>
#include <stdint.h>
/* Offsets, never absolute addresses. All hooks are mandatory; barrier must order
 * device accesses. Only gate() is ISR-safe. Worker rearm/gate is serialized with
 * that ISR by the hart-0 port critical section. No volatile shared gate flag. */
typedef struct {
    void *user;
    uint32_t (*read)(void *, uint32_t);
    void (*write)(void *, uint32_t, uint32_t);
    void (*barrier)(void *);
} fx1_isp_io_t;
typedef struct { uint32_t common, dma, output; } fx1_isp_irq_bits_t;
/* DMA bit meanings in MAS 6.26.5.2 / CSR IRQ field (which is an 8-bit vector). */
enum {
    FX1_IRQ_CFG = 1u, FX1_IRQ_UNDERRUN = 2u, FX1_IRQ_INPUT_DONE = 4u,
    FX1_IRQ_INPUT_START = 8u, FX1_IRQ_OUTPUT_DONE = 16u,
    FX1_IRQ_OVERFLOW = 32u, FX1_IRQ_AXI = 64u, FX1_IRQ_STREAM = 128u
};
void fx1_isp_irq_gate(const fx1_isp_io_t *);
fx1_isp_irq_bits_t fx1_isp_irq_read(const fx1_isp_io_t *);
/* Intersection of captured + serviced + canonical W1C only. No ownership writes. */
void fx1_isp_irq_ack(const fx1_isp_io_t *, fx1_isp_irq_bits_t captured,
                     fx1_isp_irq_bits_t serviced);
/* Caller holds port critical section. Recheck after enabling; if pending, leave
 * sources masked and run another worker turn. Event after recheck wakes via ISR. */
bool fx1_isp_irq_rearm(const fx1_isp_io_t *, fx1_isp_irq_bits_t desired);
fx1_isp_irq_bits_t fx1_isp_irq_replay_masks(uint32_t hardware_caps);
#endif
