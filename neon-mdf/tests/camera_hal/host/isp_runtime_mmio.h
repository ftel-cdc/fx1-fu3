#ifndef ISP_RUNTIME_MMIO_H
#define ISP_RUNTIME_MMIO_H
#include "isp/platform/fx1/fx1_isp_backend.h"
#include "fx1_isp/fx1_isp_csr.h"
#define R(n) FX1_ISP_##n##_OFFSET
#define M(n) FX1_ISP_##n##_MASK
/* Behavioral fixture: sticky W1C/set-wins, profile command acceptance and
 * deferred recovery. This does not model pixel datapaths or prove MAS hardware. */
typedef struct {
    uint32_t reg[0x9000 / 4];
    uint32_t writes, ack_common, ack_dma, ack_output, reset_requests;
    uint32_t set_offset, set_bits, arm_offset, arm_bits;
    unsigned prepared, completed, attaches, detaches;
    unsigned ready[3], leased[3], releases[3];
    bool attach_fail, unstable_sequence, stuck_irq;
    const void *owner;
} isp_mmio_t;
static uint32_t mmio_read(void *user, uint32_t offset)
{
    isp_mmio_t *m = user;
    if (m->unstable_sequence && offset == R(COMMON_RECOVERY_SEQUENCE)) return ++m->reg[offset / 4];
    return m->reg[offset / 4];
}
static void mmio_write(void *user, uint32_t offset, uint32_t value)
{
    isp_mmio_t *m = user;
    ++m->writes;
    bool w1c = offset == R(COMMON_IRQ_STATUS) || offset == R(DMA_IRQ_STAT) || offset == R(OUTPUT_IRQ_STATUS) ||
        offset == R(ODMA_BUF_DONE) || offset == R(TUNE_ERROR) || offset == R(COMMON_RECOVERY_STATUS) ||
        offset == R(AEC_STATUS) || offset == R(AWB_STATUS) || offset == R(AF_STATUS);
    if (offset == R(COMMON_IRQ_STATUS)) m->ack_common = value;
    if (offset == R(DMA_IRQ_STAT)) m->ack_dma = value;
    if (offset == R(OUTPUT_IRQ_STATUS)) m->ack_output = value;
    if (w1c) m->reg[offset / 4] &= ~value;
    else if (offset == R(IDMA_BUF_VALID) || offset == R(ODMA_BUF_FREE)) m->reg[offset / 4] |= value;
    else m->reg[offset / 4] = value;
    if (offset == m->set_offset || (m->stuck_irq && offset == R(OUTPUT_IRQ_STATUS)))
        m->reg[offset / 4] |= m->set_bits;
    if (offset == R(OUTPUT_IRQ_EN) && value && m->arm_bits) {
        m->reg[m->arm_offset / 4] |= m->arm_bits; m->arm_bits = 0;
    }
    if (offset == R(COMMON_FRAME_WIDTH)) m->reg[R(RESIZER_OUT_W) / 4] = value;
    if (offset == R(COMMON_FRAME_HEIGHT)) m->reg[R(RESIZER_OUT_H) / 4] = value;
    if (offset == R(TUNE_MODE) && !value) m->reg[R(TUNE_STATUS) / 4] = 0;
    if (offset == R(TUNE_CMD)) {
        uint32_t bank = m->reg[R(TUNE_BANK) / 4] & 1u;
        if (value & M(TUNE_CMD_UNSEAL)) m->reg[R(TUNE_STATUS) / 4] &= ~(bank ? M(TUNE_STATUS_BANK1_VALID) : M(TUNE_STATUS_BANK0_VALID));
        if (value & M(TUNE_CMD_SEAL)) m->reg[R(TUNE_STATUS) / 4] |= bank ? M(TUNE_STATUS_BANK1_VALID) : M(TUNE_STATUS_BANK0_VALID);
        if (value & M(TUNE_CMD_APPLY)) m->reg[R(TUNE_STATUS) / 4] |= M(TUNE_STATUS_PENDING);
        if (value & M(TUNE_CMD_CANCEL)) m->reg[R(TUNE_STATUS) / 4] &= ~M(TUNE_STATUS_PENDING);
    }
    if (offset == R(DMA_CTRL) && (value & M(DMA_CTRL_SOFT_RESET))) {
        ++m->reset_requests;
        ++m->reg[R(COMMON_RECOVERY_SEQUENCE) / 4];
        m->reg[R(COMMON_RECOVERY_STATUS) / 4] = M(COMMON_RECOVERY_STATUS_BUSY) | 16u;
    }
    const uint32_t cmd[] = {R(TUNE_AEC_CMD), R(AWB_RESULT_CTRL), R(TUNE_AF_CMD)};
    const uint32_t status[] = {R(TUNE_AEC_STATUS), R(AWB_RESULT_STATUS), R(TUNE_AF_STATUS)};
    for (unsigned i = 0; i < 3; ++i) if (offset == cmd[i]) {
        if (value == 1 && m->ready[i] && !m->leased[i]) { --m->ready[i]; m->leased[i] = 1; }
        if (value == 2 && m->leased[i]) { ++m->releases[i]; m->leased[i] = 0; }
        m->reg[status[i] / 4] = i == 1 ? m->ready[i] | (m->leased[i] << 2) : (m->ready[i] << 1) | m->leased[i];
    }
}
static void mmio_barrier(void *user) { (void)user; }
static void mmio_prepare(void *user, uint32_t a, uint32_t n, bool output)
{ (void)a; (void)n; (void)output; ++((isp_mmio_t *)user)->prepared; }
static void mmio_complete(void *user, uint32_t a, uint32_t n)
{ (void)a; (void)n; ++((isp_mmio_t *)user)->completed; }
static bool mmio_attach(void *user)
{ isp_mmio_t *m = user; ++m->attaches; return !m->attach_fail; }
static void mmio_detach(void *user)
{
    isp_mmio_t *m = user; ++m->detaches;
    /* The shared claim must still belong to this owner during IRQ detach. */
    if (m->owner) CHECK(neon_isp_device_available_to(m->owner));
}
static fx1_isp_backend_port_t mmio_port(isp_mmio_t *m)
{
    return (fx1_isp_backend_port_t){{m, mmio_read, mmio_write, mmio_barrier},
        mmio_prepare, mmio_complete, mmio_attach, mmio_detach};
}
static void mmio_init(isp_mmio_t *m)
{
    *m = (isp_mmio_t){0};
    m->reg[R(COMMON_VER_ID) / 4] = FX1_ISP_COMMON_VER_ID_RESET;
    m->reg[R(TUNE_ID) / 4] = FX1_ISP_TUNE_ID_RESET;
    m->reg[R(TUNE_CAPS) / 4] = FX1_ISP_TUNE_CAPS_RO_MASK;
    m->reg[R(TUNE_NEXT_FRAME) / 4] = 1;
    m->reg[R(CH0_GEOMETRY_STATUS) / 4] = M(CH0_GEOMETRY_STATUS_VALID);
}
static void mmio_admit(isp_mmio_t *m)
{
    uint32_t frame = m->reg[R(TUNE_NEXT_FRAME) / 4]++;
    if (m->reg[R(TUNE_STATUS) / 4] & M(TUNE_STATUS_PENDING)) {
        m->reg[R(TUNE_STATUS) / 4] &= ~(M(TUNE_STATUS_PENDING) | M(TUNE_STATUS_ACTIVE_BANK));
        m->reg[R(TUNE_STATUS) / 4] |= M(TUNE_STATUS_ACTIVE_VALID) | m->reg[R(TUNE_BANK) / 4];
        m->reg[R(TUNE_ACTIVE_ID) / 4] = m->reg[R(TUNE_PROFILE_ID) / 4];
        m->reg[R(TUNE_ACTIVE_FRAME) / 4] = frame;
    }
    m->reg[R(COMMON_STATUS) / 4] = M(COMMON_STATUS_BUSY);
    m->reg[R(DMA_STAT) / 4] = 3;
}
static void mmio_retire(isp_mmio_t *m, uint8_t slot, uint32_t frame, uint32_t profile)
{
    m->reg[R(IDMA_BUF_VALID) / 4] &= ~(1u << slot);
    m->reg[R(ODMA_BUF_FREE) / 4] &= ~(1u << slot);
    m->reg[R(ODMA_BUF_DONE) / 4] |= 1u << slot;
    m->reg[R(TUNE_CH0_BUF0_FRAME) / 4 + slot] = frame;
    m->reg[R(TUNE_CH0_BUF0_PROFILE) / 4 + slot] = profile;
    m->reg[R(ODMA_BUF_TAG0) / 4 + slot] = m->reg[R(IDMA_BUF_TAG0) / 4 + slot];
    m->reg[R(ODMA_BUF_SOF_TS0) / 4 + slot] = UINT32_MAX - 1u;
    m->reg[R(ODMA_BUF_EOF_TS0) / 4 + slot] = 2;
    m->reg[R(COMMON_STATUS) / 4] = 0; m->reg[R(DMA_STAT) / 4] = 0;
    m->reg[R(COMMON_IRQ_STATUS) / 4] |= 1;
    m->reg[R(DMA_IRQ_STAT) / 4] |= FX1_IRQ_OUTPUT_DONE;
    m->reg[R(OUTPUT_IRQ_STATUS) / 4] |= 1;
}
static void mmio_recovery_done(isp_mmio_t *m, uint32_t count)
{
    m->reg[R(COMMON_RECOVERY_STATUS) / 4] = M(COMMON_RECOVERY_STATUS_DONE) | 16u | count << 16;
    m->reg[R(IDMA_BUF_VALID) / 4] = m->reg[R(ODMA_BUF_FREE) / 4] = 0;
    m->reg[R(COMMON_STATUS) / 4] = m->reg[R(DMA_STAT) / 4] = 0;
    m->reg[R(DMA_CTRL) / 4] = 0;
}
#endif
