/* App M1: RAW12 DDR -> driver team -> CH0 NV12; polling và giữ output tới khi kiểm tra xong. */
#include "fx1_fw.h"
#include "pipeline/ftel_isp_stream.h"
#include "frame_fixture.h"

_Static_assert(FRAME_RAW_BASE >= FX1_FW_RAW_BUF_BASE, "RAW must be outside firmware");
_Static_assert(FRAME_RAW_BASE + FRAME_ARENA_BYTES <= FX1_FW_YUV_BUF_BASE, "RAW overlaps output");
_Static_assert(FRAME_OUTPUT_BASE >= FX1_FW_YUV_BUF_BASE, "output must be in buffer region");
_Static_assert(FRAME_OUTPUT_BASE + FRAME_ARENA_BYTES <= FX1_FW_TEST_AREA_BASE, "output outside region");

static unsigned passed, failed;
static uint64_t time_us(void *unused)
{
    (void)unused;
    return fx1_mtime() / (FX1_MTIME_HZ / 1000000u);
}
static void show(const char *name, uint32_t value)
{
    fx1_puts(name); fx1_puts(" = "); fx1_puthex(value); fx1_puts("\n");
}
static bool check(const char *name, bool ok)
{
    fx1_puts(ok ? "[PASS] " : "[FAIL] "); fx1_puts(name); fx1_puts("\n");
    if (ok) passed++; else failed++;
    return ok;
}
static bool plane_equals(uint64_t base, unsigned stride, unsigned rows, uint8_t active)
{
    const volatile uint8_t *p = (const volatile uint8_t *)(uintptr_t)base;
    for (unsigned y = 0; y < rows; y++) for (unsigned x = 0; x < stride; x++) {
        /* MAS 6.25.3.8: formatter tích hợp không phát padding, ODMA giữ byte DDR ngoài vùng active. */
        uint8_t expected = x < FRAME_WIDTH ? active : 0xa5u;
        if (p[y * stride + x] != expected) {
            show("FIRST_BAD_BYTE_OFFSET", y * stride + x);
            show("EXPECTED_BYTE", expected); show("ACTUAL_BYTE", p[y * stride + x]);
            return false;
        }
    }
    return true;
}
static bool guard_equals(uint64_t base, unsigned bytes)
{
    const volatile uint8_t *p = (const volatile uint8_t *)(uintptr_t)base;
    for (unsigned i = 0; i < bytes; i++) if (p[i] != 0xa5u) return false;
    return true;
}

/* Local fixture: no VP --load or external RAW file is required. */
static void seed_buffers(void)
{
    volatile uint8_t *raw = (volatile uint8_t *)(uintptr_t)FRAME_RAW_BASE;
    volatile uint8_t *output = (volatile uint8_t *)(uintptr_t)FRAME_OUTPUT_BASE;
    static const uint16_t samples[4] = {0, 1024, 2048, 4095};
    for (unsigned i = 0; i < FRAME_ARENA_BYTES; ++i) {
        raw[i] = 0xa5u;
        output[i] = 0xa5u;
    }
    for (unsigned slot = 0; slot < 4; ++slot) {
        for (unsigned row = 0; row < FRAME_HEIGHT; ++row) {
            volatile uint16_t *line = (volatile uint16_t *)(uintptr_t)(
                FRAME_RAW_BASE + slot * FRAME_SLOT_BYTES + FRAME_GUARD_BYTES +
                row * FRAME_INPUT_STRIDE);
            for (unsigned col = 0; col < FRAME_WIDTH; ++col) line[col] = samples[slot];
        }
    }
    fx1_dma_prepare((const volatile void *)(uintptr_t)FRAME_RAW_BASE, FRAME_ARENA_BYTES, FX1_DMA_TO_DEVICE);
    fx1_dma_prepare((const volatile void *)(uintptr_t)FRAME_OUTPUT_BASE, FRAME_ARENA_BYTES, FX1_DMA_FROM_DEVICE);
}

int main(void)
{
    fx1_puts("[PASS] FW_BOOT: RAW12 to NV12 through team driver\n");
    fx1_puts("[INFO] One in-flight frame; explicit LSC tables 1/2, LSC disabled.\n");
    seed_buffers();
    isp_config_t cfg;
    isp_get_default_config(&cfg);
    cfg.ifmt = (ifmt_config_t){FRAME_WIDTH, FRAME_HEIGHT, BAYER_RGGB, INPUT_DEPTH_RAW12};
    cfg.lsc.coefficients[0] = NULL; cfg.lsc.coefficient_count[0] = 0;
    cfg.lsc.profile = 1;
    aec_get_default_config(&cfg.aec, FRAME_WIDTH, FRAME_HEIGHT);
    cfg.aec.enable = cfg.awb.enable = cfg.af.enable = false;
    output_get_default_config(&cfg.output, FRAME_WIDTH, FRAME_HEIGHT);
    output_geometry_t geometry[3];
    for (unsigned c = 0; c < 3; c++)
        (void)output_compute_geometry(&cfg.output.channel[c], c, FRAME_WIDTH, FRAME_HEIGHT, &geometry[c]);
    dma_get_default_config(&cfg.dma, FRAME_WIDTH, geometry);
    cfg.dma.input_stride = FRAME_INPUT_STRIDE;
    cfg.dma.output[0].y_stride = FRAME_Y_STRIDE;
    cfg.dma.output[0].uv_stride = FRAME_UV_STRIDE;
    for (unsigned i = 0; i < 4; i++) {
        cfg.dma.input[i] = FRAME_RAW_BASE + i * FRAME_SLOT_BYTES + FRAME_GUARD_BYTES;
        cfg.dma.output[0].y[i] = FRAME_OUTPUT_BASE + i * FRAME_SLOT_BYTES + FRAME_GUARD_BYTES;
        cfg.dma.output[0].uv[i] = cfg.dma.output[0].y[i] + FRAME_UV_OFFSET;
    }
    isp_wait_t wait = {time_us, NULL, 10000, 1000000};
    isp_init_result_t init;
    isp_status_t st = isp_init(&cfg, &wait, &init);
    show("INIT_STATUS", (uint32_t)st); show("INIT_STAGE", init.stage);
    if (!check("INIT reaches READY", st == ISP_OK && init.stage == ISP_INIT_READY)) goto finish;
    isp_stream_t stream = {0};
    st = isp_stream_start(&stream, &cfg, &wait);
    if (!check("START verifies geometry without queuing DMA", st == ISP_OK &&
               !REG_RD(R_IDMA_BUF_VALID) && !(REG_RD(R_DMA_CTRL) & 3u))) goto finish;
    /* Giá trị mong đợi từ CSC full-range Q16 theo MAS, không lấy output VP làm golden. */
    static const uint16_t samples[4] = {0, 1024, 2048, 4095};
    for (unsigned frame = 0; frame < 5; frame++) {
        uint8_t slot = frame & 3u;
        uint32_t tag = 0xf100u + frame, timestamp = 1000u + 10u * frame;
        fx1_puts("FRAME sequence="); fx1_putdec(frame); fx1_puts(" slot="); fx1_putdec(slot); fx1_puts("\n");
        fx1_dma_prepare((const volatile void *)(uintptr_t)(FRAME_OUTPUT_BASE + slot * FRAME_SLOT_BYTES),
                        FRAME_SLOT_BYTES, FX1_DMA_FROM_DEVICE);
        st = isp_stream_submit(&stream, tag, timestamp);
        if (!check("SUBMIT accepts next rotating slot", st == ISP_OK && stream.next_slot == slot)) break;
        isp_frame_result_t result;
        st = isp_stream_wait(&stream, &wait, &result);
        show("WAIT_STATUS", (uint32_t)st);
        if (st != ISP_OK) {
            show("DMA_STAT", REG_RD(R_DMA_STAT)); show("DMA_ERR", REG_RD(R_DMA_ERR));
            show("IDMA_COUNT", REG_RD(FX1_ISP_IDMA_FRAME_COUNT_OFFSET));
            show("ODMA_COUNT", REG_RD(FX1_ISP_ODMA_FRAME_COUNT_OFFSET));
            show("ODMA_DONE", REG_RD(FX1_ISP_ODMA_BUF_DONE_OFFSET));
        }
        if (!check("WAIT receives complete frame", st == ISP_OK)) break;
        show("DMA_ERROR_BEFORE_PAUSE_CLEAR", result.dma.errors);
        show("IDMA_FRAME_COUNT", result.dma.input_count);
        show("ODMA_FRAME_COUNT", result.dma.output_count[0]);
        check("Completion counts/slot/sequence match", result.slot == slot && result.sequence == frame &&
              result.dma.input_count == frame + 1u && result.dma.output_count[0] == frame + 1u &&
              result.dma.output_done[0] == (1u << slot) && !result.dma.input_valid);
        check("Published tag/timestamp identify submitted frame", result.tag[0] == tag &&
              result.timestamp[0] == timestamp && result.eof_timestamp[0] >= result.sof_timestamp[0]);
        fx1_dma_complete((const volatile void *)(uintptr_t)(FRAME_OUTPUT_BASE + slot * FRAME_SLOT_BYTES),
                         FRAME_SLOT_BYTES, FX1_DMA_FROM_DEVICE);
        uint8_t y = (uint8_t)(((uint32_t)samples[slot] * 4081u + 32768u) >> 16);
        check("Y pixels and row padding match independent formula",
              plane_equals(cfg.dma.output[0].y[slot], FRAME_Y_STRIDE, FRAME_HEIGHT, y));
        check("UV pixels are neutral and row padding remains intact",
              plane_equals(cfg.dma.output[0].uv[slot], FRAME_UV_STRIDE, FRAME_HEIGHT / 2u, 128));
        check("Output guards remain intact",
              guard_equals(cfg.dma.output[0].y[slot] - FRAME_GUARD_BYTES, FRAME_GUARD_BYTES) &&
              guard_equals(cfg.dma.output[0].y[slot] + FRAME_Y_STRIDE * FRAME_HEIGHT, FRAME_GUARD_BYTES) &&
              guard_equals(cfg.dma.output[0].uv[slot] - FRAME_GUARD_BYTES, FRAME_GUARD_BYTES) &&
              guard_equals(cfg.dma.output[0].uv[slot] + FRAME_UV_STRIDE * FRAME_HEIGHT / 2u, FRAME_GUARD_BYTES));
        check("HELD output cannot be submitted or stopped",
              isp_stream_submit(&stream, 0, 0) == ISP_E_STATE &&
              isp_stream_stop(&stream, 100) == ISP_E_STATE &&
              REG_RD(FX1_ISP_ODMA_BUF_DONE_OFFSET) == (1u << slot));
        st = isp_stream_release(&stream);
        if (!check("RELEASE ACKs DONE without rearming FREE", st == ISP_OK &&
                   !REG_RD(FX1_ISP_ODMA_BUF_DONE_OFFSET) && !REG_RD(FX1_ISP_ODMA_BUF_FREE_OFFSET))) break;
    }
    check("Five frames include slot rotation 0-1-2-3-0", stream.sequence == 5);
    st = isp_stream_stop(&stream, 100000);
    check("STOP leaves ISP/DMA disabled and no errors", st == ISP_OK &&
          !(REG_RD(R_COMMON_CTRL) & 1u) && !(REG_RD(R_DMA_CTRL) & 3u) &&
          !REG_RD(R_DMA_ERR) && !REG_RD(R_IDMA_BUF_VALID) && !isp_tune_error_set());
finish:
    fx1_puts("FRAME_CHECKS passed="); fx1_putdec(passed);
    fx1_puts(" failed="); fx1_putdec(failed); fx1_puts("\n");
    fx1_puts(failed ? "FRAME_RESULT=FAIL\n" : "FRAME_RESULT=PASS\n");
    return failed ? 0x34 : 0;
}
