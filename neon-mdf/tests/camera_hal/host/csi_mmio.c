#include "csi_mmio.h"
#include "fx1_fw.h"
#include <assert.h>
#include <string.h>

static uint32_t registers[1024];
static unsigned index_of(uint32_t offset)
{
    assert(offset % 4U == 0U && offset / 4U < 1024U);
    return offset / 4U;
}
void csi_mmio_set(uint32_t offset, uint32_t value) { registers[index_of(offset)] = value; }
uint32_t csi_mmio_get(uint32_t offset) { return registers[index_of(offset)]; }
void csi_mmio_reset(void)
{
    memset(registers, 0, sizeof(registers));
    CSI_SET_REG(IP_VERSION, NEON_FX1_MIPI_CSI2_IP_VERSION_RESET);
}
uint32_t fx1_read32(uintptr_t address)
{
    assert(address >= NEON_FX1_MIPI_CSI2_BASE && address < NEON_FX1_MIPI_CSI2_BASE + sizeof(registers));
    uint32_t offset = (uint32_t)(address - NEON_FX1_MIPI_CSI2_BASE);
    uint32_t value = csi_mmio_get(offset);
    if (offset == NEON_FX1_MIPI_CSI2_GENERIC_SHORT_PACKET_OFFSET) csi_mmio_set(offset, 0U);
    return value;
}
void fx1_write32(uintptr_t address, uint32_t value)
{
    assert(address >= NEON_FX1_MIPI_CSI2_BASE && address < NEON_FX1_MIPI_CSI2_BASE + sizeof(registers));
    uint32_t offset = (uint32_t)(address - NEON_FX1_MIPI_CSI2_BASE);
    if (offset == NEON_FX1_MIPI_CSI2_IRQ_STATUS_OFFSET) {
        CSI_SET_REG(IRQ_STATUS, CSI_GET_REG(IRQ_STATUS) & ~value);
        return;
    }
    if (offset == NEON_FX1_MIPI_CSI2_BUFFER_RELEASE_OFFSET) {
        uint32_t owners = CSI_GET_REG(BUFFER_OWNERSHIP);
        if (value & 1U) owners &= ~3U;
        if (value & 2U) owners &= ~12U;
        CSI_SET_REG(BUFFER_OWNERSHIP, owners);
    }
    if (offset == NEON_FX1_MIPI_CSI2_TPG_CONTROL_OFFSET) {
        uint32_t active = NEON_FX1_MIPI_CSI2_CORE_STATUS_ACTIVE_SRC_MASK;
        uint32_t status = CSI_GET_REG(CORE_STATUS) & ~active;
        if (value & NEON_FX1_MIPI_CSI2_TPG_CONTROL_TPG_ENABLE_MASK) status |= active;
        CSI_SET_REG(CORE_STATUS, status);
    }
    csi_mmio_set(offset, value);
}
void csi_mmio_complete(unsigned slot, uint32_t frame_id, int error)
{
    assert(slot < 2U && frame_id > 0U && frame_id <= 0x3fffU);
    unsigned shift = 2U * slot;
    uint32_t owners = CSI_GET_REG(BUFFER_OWNERSHIP);
    assert(((owners >> shift) & 3U) == 0U);
    CSI_SET_REG(BUFFER_OWNERSHIP, (owners & ~(3U << shift)) | ((error ? 2U : 1U) << shift));
    shift = 16U * slot;
    CSI_SET_REG(FRAME_ID, (CSI_GET_REG(FRAME_ID) & ~(0x3fffU << shift)) | (frame_id << shift));
    CSI_SET_REG(IRQ_STATUS, CSI_GET_REG(IRQ_STATUS) | (1U << (error ? 17U : 16U)));
    CSI_SET_REG(VC0_IMAGE_INFO1, (32U << 16) | 96U);
    CSI_SET_REG(VC0_IMAGE_INFO2, 0x2cU);
}
