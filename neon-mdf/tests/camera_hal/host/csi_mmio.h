/* Register fixture for the production CSI driver, not a CSI HAL implementation.
 * Models immediate IRQ W1C, buffer release, source switching and FIFO pop only;
 * no DMA writes or clock-domain timing are simulated. */
#ifndef TEST_CSI_MMIO_H
#define TEST_CSI_MMIO_H
#include <stdint.h>
#include "fx1_registers.h"
void csi_mmio_reset(void);
void csi_mmio_set(uint32_t offset, uint32_t value);
uint32_t csi_mmio_get(uint32_t offset);
void csi_mmio_complete(unsigned slot, uint32_t frame_id, int error);
#define CSI_GET_REG(name) csi_mmio_get(NEON_FX1_MIPI_CSI2_##name##_OFFSET)
#define CSI_SET_REG(name, value) csi_mmio_set(NEON_FX1_MIPI_CSI2_##name##_OFFSET, (value))
#endif
