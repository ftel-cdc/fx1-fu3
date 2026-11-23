/* Host tests of the MAS v1.4 production CSI driver with a register fixture. */
#include "test_common.h"
#include "host/csi_mmio.h"
#include "csi/platform/fx1/neon_csi_fx1.h"
#include "mipi/platform/fx1/fx1_mipi_simulation.h"

typedef struct {
    neon_mipi_handle_t link;
    neon_csi_handle_t csi;
    neon_csi_config_t config;
} env_t;

static void setup(env_t *e)
{
    neon_mipi_config_t link = {NEON_MIPI_DIRECTION_RX, 4U, 1000000000UL, true};
    csi_mmio_reset();
    CHECK_EQ(neon_mipi_init(&e->link), NEON_OK);
    CHECK_EQ(neon_mipi_acquire(&e->link, &link), NEON_OK);
    CHECK_EQ(neon_csi_init(&e->csi, &e->link), NEON_OK);
    neon_csi_get_default_config(&e->config);
    e->config.width = 64U;
    e->config.height = 32U;
    e->config.pixel_packing = NEON_CSI_PACKING_UNPACKED16;
    e->config.dma.base_addr[0] = 0x80000000U;
    e->config.dma.base_addr[1] = 0x80010000U;
    e->config.dma.buffer_size_bytes = 4096U;
}
static void start(env_t *e)
{
    setup(e);
    CHECK_EQ(neon_csi_configure(&e->csi, &e->config), NEON_OK);
    CHECK_EQ(neon_csi_start(&e->csi), NEON_OK);
}
static void test_version_and_caps(void)
{
    env_t e;
    neon_csi_caps_t caps;
    setup(&e);
    CHECK_EQ(neon_csi_get_caps(NULL), NEON_INVALID_PARAM);
    CHECK_EQ(neon_csi_get_caps(&caps), NEON_OK);
    CHECK_EQ(caps.buffer_count, 2U);
    CHECK_EQ(caps.vc_count, 2U);
    CHECK_EQ(caps.supported_raw_formats, NEON_CSI_RAW_FORMAT_BIT(NEON_CSI_RAW10) |
                                       NEON_CSI_RAW_FORMAT_BIT(NEON_CSI_RAW12));
    CHECK_EQ(caps.supported_sub_events[NEON_CSI_EVENT_FRAME] & NEON_CSI_SUB_EVENT_FRAME_START, 0U);
    CSI_SET_REG(IP_VERSION, 0U);
    CHECK_EQ(neon_csi_init(&e.csi, &e.link), NEON_NOT_SUPPORTED);
    CHECK_EQ(neon_csi_init(NULL, &e.link), NEON_INVALID_PARAM);
}
static void test_layout_and_validation(void)
{
    env_t e;
    neon_csi_layout_t layout;
    setup(&e);
    CHECK_EQ(neon_csi_calculate_layout(&e.config, &layout), NEON_OK);
    CHECK_EQ(layout.bytes_per_line, 128U);
    CHECK_EQ(layout.frame_bytes, 4096U);
    CHECK_EQ(layout.buffer_bytes, 8192U); /* VC1 reserved even with VC0 only. */
    CHECK_EQ(layout.sample_align, NEON_CSI_ALIGN_RIGHT);
    e.config.metadata.enable = true;
    e.config.metadata.area_size_bytes = 256U;
    CHECK_EQ(neon_csi_calculate_layout(&e.config, &layout), NEON_OK);
    CHECK_EQ(layout.buffer_bytes, 8704U);
    e.config.metadata.area_size_bytes = 120U;
    CHECK_EQ(neon_csi_calculate_layout(&e.config, &layout), NEON_INVALID_PARAM);
    e.config.metadata.enable = false;
    e.config.dma.line_pitch_bytes = 129U;
    CHECK_EQ(neon_csi_configure(&e.csi, &e.config), NEON_INVALID_PARAM);
    CHECK_EQ(CSI_GET_REG(DMA_BUFFER_SIZE), 0U); /* No writes on validation failure. */
    e.config.dma.line_pitch_bytes = 384U; /* Padding > 255 bytes. */
    CHECK_EQ(neon_csi_calculate_layout(&e.config, &layout), NEON_INVALID_PARAM);
    e.config.dma.line_pitch_bytes = 0U;
    e.config.dma.base_addr[0] += 4U;
    CHECK_EQ(neon_csi_configure(&e.csi, &e.config), NEON_INVALID_PARAM);
    e.config.raw_format = NEON_CSI_RAW16;
    CHECK_EQ(neon_csi_calculate_layout(&e.config, &layout), NEON_NOT_SUPPORTED);
}
static void test_start_stop_and_dma_timeout(void)
{
    env_t e;
    setup(&e);
    CHECK_EQ(neon_csi_start(&e.csi), NEON_ERROR);
    CSI_SET_REG(IRQ_STATUS, 0xffffffU);
    CHECK_EQ(neon_csi_configure(&e.csi, &e.config), NEON_OK);
    CHECK_EQ(CSI_GET_REG(IRQ_STATUS), 0U);
    CHECK_EQ(CSI_GET_REG(LINE_PITCH), 128U);
    CHECK_EQ(CSI_GET_REG(DMA_BUFFER_SIZE), 4096U);
    CHECK_EQ(neon_csi_start(&e.csi), NEON_OK);
    CHECK(e.csi.link_locked);
    CHECK_EQ(neon_csi_configure(&e.csi, &e.config), NEON_BUSY);
    CHECK_EQ(neon_csi_deinit(&e.csi), NEON_BUSY);
    CSI_SET_REG(DMA_STATUS, NEON_FX1_MIPI_CSI2_DMA_STATUS_DMA_BUSY_MASK);
    CHECK_EQ(neon_csi_stop(&e.csi), NEON_TIMEOUT);
    CSI_SET_REG(DMA_STATUS, 0U);
    CHECK_EQ(neon_csi_stop(&e.csi), NEON_OK);
    CHECK(!e.csi.link_locked);
    CHECK_EQ(CSI_GET_REG(IRQ_ENABLE), 0U);
    CHECK_EQ(CSI_GET_REG(GLOBAL_IRQ_ENABLE), 0U);
    CHECK_EQ(neon_csi_deinit(&e.csi), NEON_OK);
}
static void test_buffer_order_wrap_and_release(void)
{
    env_t e;
    neon_csi_frame_t frame;
    start(&e);
    CHECK_EQ(neon_csi_acquire_frame(&e.csi, &frame), NEON_BUSY);
    csi_mmio_complete(0U, 1U, 0);
    csi_mmio_complete(1U, 0x3fffU, 1);
    CSI_SET_REG(BUFFER_ERROR_INFO, 0xffff0000U);
    CHECK_EQ(neon_csi_acquire_frame(&e.csi, &frame), NEON_OK);
    CHECK_EQ(frame.buffer_index, 1U); /* 0x3fff is older than 1. */
    CHECK_EQ(frame.state, NEON_CSI_BUFFER_SW_ERROR);
    CHECK_EQ(frame.last_valid_line, UINT32_MAX);
    CHECK_EQ(frame.pixel_addr[0], e.config.dma.base_addr[1]);
    CHECK_EQ(frame.pixel_addr[1], e.config.dma.base_addr[1] + 4096U);
    CHECK_EQ(neon_csi_acquire_frame(&e.csi, &frame), NEON_OK);
    CHECK_EQ(frame.buffer_index, 0U);
    CHECK_EQ(neon_csi_acquire_frame(&e.csi, &frame), NEON_BUSY);
    CHECK_EQ(neon_csi_release_frame(&e.csi, 1U), NEON_OK);
    CHECK_EQ(CSI_GET_REG(BUFFER_OWNERSHIP), 1U);
    CHECK_EQ(neon_csi_release_frame(&e.csi, 1U), NEON_INVALID_PARAM);
    CHECK_EQ(neon_csi_release_frame(&e.csi, 0U), NEON_OK);
    CHECK_EQ(CSI_GET_REG(BUFFER_OWNERSHIP), 0U);
    CHECK_EQ(neon_csi_stop(&e.csi), NEON_OK);
}
static void test_irq_w1c_and_recovery(void)
{
    env_t e;
    uint32_t events, pending;
    start(&e);
    CSI_SET_REG(IRQ_STATUS, (1U << 4) | (1U << 18)); /* CRC + AXI error. */
    CHECK_EQ(neon_csi_get_events(&e.csi, &events), NEON_OK);
    CHECK(events & NEON_CSI_EVENT_BIT(NEON_CSI_EVENT_PACKET));
    CHECK(events & NEON_CSI_EVENT_BIT(NEON_CSI_EVENT_SYSTEM));
    CHECK_EQ(neon_csi_take_sub_events(&e.csi, NEON_CSI_EVENT_PACKET, &pending), NEON_OK);
    CHECK_EQ(pending, NEON_CSI_SUB_EVENT_PACKET_CRC_ERROR);
    CHECK_EQ(CSI_GET_REG(IRQ_STATUS), 1U << 18); /* Preserve unrelated IRQ. */
    CHECK_EQ(neon_csi_recover(&e.csi), NEON_ERROR);
    fx1_mipi_sim_set_hs_active(&e.link, true);
    csi_mmio_complete(0U, 10U, 0);
    CHECK_EQ(neon_csi_recover(&e.csi), NEON_OK);
    CHECK_EQ(CSI_GET_REG(IRQ_STATUS), 0U);
    CHECK_EQ(CSI_GET_REG(BUFFER_OWNERSHIP), 0U);
    e.link.config_epoch++;
    CHECK_EQ(neon_csi_get_events(&e.csi, &events), NEON_OK);
    CHECK_EQ(e.csi.state, NEON_CSI_STATE_IDLE);
    CHECK(!e.csi.link_locked);
    CHECK_EQ(neon_csi_take_sub_events(&e.csi, NEON_CSI_EVENT_SYSTEM, &pending), NEON_OK);
    CHECK(pending & NEON_CSI_SUB_EVENT_SYSTEM_LINK_RESET);
}
static void test_fx1_tpg(void)
{
    env_t e;
    setup(&e);
    e.config.test_pattern.enable = true;
    e.config.test_pattern.width = 67U;
    e.config.test_pattern.height = 32U;
    CHECK_EQ(neon_csi_configure(&e.csi, &e.config), NEON_OK);
    CHECK_EQ(neon_csi_fx1_set_lead_vc(&e.csi, 1U), NEON_INVALID_PARAM);
    CHECK_EQ(neon_csi_fx1_set_record_id(&e.csi, 0x1234U), NEON_OK);
    CHECK_EQ(CSI_GET_REG(METADATA_RECORD_ID), 0x1234U);
    CHECK_EQ(neon_csi_start(&e.csi), NEON_OK);
    CHECK(CSI_GET_REG(CORE_STATUS) & NEON_FX1_MIPI_CSI2_CORE_STATUS_ACTIVE_SRC_MASK);
    CHECK_EQ(neon_csi_stop(&e.csi), NEON_OK);
    CHECK_EQ(CSI_GET_REG(CORE_STATUS) & NEON_FX1_MIPI_CSI2_CORE_STATUS_ACTIVE_SRC_MASK, 0U);
}
int main(void)
{
    RUN(test_version_and_caps);
    RUN(test_layout_and_validation);
    RUN(test_start_stop_and_dma_timeout);
    RUN(test_buffer_order_wrap_and_release);
    RUN(test_irq_w1c_and_recovery);
    RUN(test_fx1_tpg);
    return TEST_REPORT();
}
