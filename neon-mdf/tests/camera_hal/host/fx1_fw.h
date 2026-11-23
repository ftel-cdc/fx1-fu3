/* Host-only replacement for BSP MMIO and RISC-V fences. Selected by the test Makefile. */
#ifndef FX1_FW_H
#define FX1_FW_H
#include <stdint.h>
uint32_t fx1_read32(uintptr_t address);
void fx1_write32(uintptr_t address, uint32_t value);
typedef enum {
    FX1_DMA_TO_DEVICE = 1, FX1_DMA_FROM_DEVICE = 2, FX1_DMA_BIDIRECTIONAL = 3
} fx1_dma_dir_t;
static inline void fx1_dma_complete(const volatile void *addr, uint32_t bytes, fx1_dma_dir_t dir)
{ (void)addr; (void)bytes; (void)dir; }
#endif
