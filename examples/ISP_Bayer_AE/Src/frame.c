/* Bước 4–5: RAW tổng hợp trong DDR -> HAL -> VP -> AEC; không chạy AE.
 * Oracle tính trực tiếp từ dữ liệu đầu vào/config, không lấy số đo VP làm chuẩn. */
#include "ftel_isp.h"
#include "neon_freertos_fx1.h"
#include "task.h"

#define WIDTH 64u
#define HEIGHT 48u
#define RAW_STRIDE 160u
#define Y_STRIDE 80u
#define UV_STRIDE 96u
#define GUARD 64u
#define RAW_ARENA FX1_FW_RAW_BUF_BASE
#define OUT_ARENA FX1_FW_YUV_BUF_BASE
#define RAW_ADDR (RAW_ARENA + GUARD)
#define Y_ADDR (OUT_ARENA + GUARD)
#define UV_ADDR (OUT_ARENA + 0x4000u + GUARD)
#define CASES 7u

static unsigned checks;
static ftel_isp_aec_stats_t stats; /* Tránh đặt snapshot histogram trên stack task. */

static void check(bool ok, const char *name)
{
    ++checks;
    fx1_puts(ok ? "[frame] PASS " : "[frame] FAIL ");
    fx1_puts(name); fx1_putc('\n');
    if (!ok) fx1_fail(0xec00u + checks);
}

static uint16_t sample(unsigned test, unsigned x, unsigned y)
{
    static const uint16_t constant[CASES] = {0, 1024, 4095, 257, 257, 0, 3000};
    return test == 5 ? (uint16_t)(100u + 200u * (x & 1u) + 400u * (y & 1u)) : constant[test];
}

static void fill(uintptr_t address, unsigned bytes)
{
    volatile uint8_t *p = (volatile uint8_t *)address;
    for (unsigned i = 0; i < bytes; ++i) p[i] = 0xa5u;
}

static bool guard(uintptr_t address)
{
    const volatile uint8_t *p = (const volatile uint8_t *)address;
    for (unsigned i = 0; i < GUARD; ++i) if (p[i] != 0xa5u) return false;
    return true;
}

static bool padding(uintptr_t address, unsigned stride, unsigned width, unsigned height)
{
    const volatile uint8_t *p = (const volatile uint8_t *)address;
    for (unsigned y = 0; y < height; ++y)
        for (unsigned x = width; x < stride; ++x) if (p[y * stride + x] != 0xa5u) return false;
    return guard(address - GUARD) && guard(address + stride * height);
}

static bool input_unchanged(unsigned test)
{
    for (unsigned y = 0; y < HEIGHT; ++y) {
        const volatile uint16_t *row = (const volatile uint16_t *)(RAW_ADDR + y * RAW_STRIDE);
        for (unsigned x = 0; x < WIDTH; ++x) if (row[x] != sample(test, x, y)) return false;
    }
    return padding(RAW_ADDR, RAW_STRIDE, WIDTH * 2u, HEIGHT);
}

static void expected(const ftel_isp_config_t *c, unsigned test, uint16_t w, uint16_t h,
                      uint64_t *sum, uint64_t *ue, uint64_t *oe, uint64_t histogram[64])
{
    *sum = *ue = *oe = 0;
    for (unsigned i = 0; i < 64; ++i) histogram[i] = 0;
    unsigned skip_x = (unsigned)c->bayer.input.pattern & 1u;
    unsigned skip_y = ((unsigned)c->bayer.input.pattern >> 1) & 1u;
    for (unsigned y = 0; y < h; ++y) for (unsigned x = 0; x < w; ++x) {
        uint32_t v = sample(test, x + skip_x, y + skip_y);
        if (c->bayer.input.raw_depth == FTEL_ISP_RAW10) v *= 4u;
        if (c->bayer.blc.enable) v = v > c->bayer.blc.black_level ? v - c->bayer.blc.black_level : 0;
        if (c->aec.source == FTEL_ISP_AEC_AFTER_DG) {
            if (c->bayer.wb.enable) {
                uint16_t gain = (x & 1u) == (y & 1u) ?
                    ((y & 1u) ? c->bayer.wb.b_q8 : c->bayer.wb.r_q8) : c->bayer.wb.g_q8;
                v = v * gain / 256u;
                if (v > 4095u) v = 4095u;
            }
            if (c->bayer.dg.enable) {
                v = v * c->bayer.dg.gain_q8 / 256u;
                if (v > 4095u) v = 4095u;
            }
        }
        *sum += v;
        *ue += v < 272u;
        *oe += v > 3744u;
        ++histogram[v >> 6];
    }
}

static void frame_task(void *unused)
{
    (void)unused;
    for (unsigned test = 0; test < CASES; ++test) {
        fx1_puts("FRAME_CASE="); fx1_putdec(test); fx1_putc('\n');
        ftel_isp_config_t config;
        ftel_isp_init_result_t init;
        ftel_isp_get_default_config(&config);
        config.aec.source = FTEL_ISP_AEC_AFTER_BPC;
        config.aec.context_id = 0x46583200u + test;
        if (test == 3 || test == 4) {
            config.bayer.input.raw_depth = FTEL_ISP_RAW10;
            config.bayer.blc.enable = true; config.bayer.blc.black_level = 64;
            config.bayer.wb.enable = true; config.bayer.wb.r_q8 = 384; config.bayer.wb.b_q8 = 512;
            config.bayer.dg.enable = true; config.bayer.dg.gain_q8 = 384;
            if (test == 3) config.aec.source = FTEL_ISP_AEC_AFTER_DG;
        }
        if (test == 5) config.bayer.input.pattern = FTEL_ISP_BGGR;
        if (test == 6) {
            config.bayer.dg.enable = true; config.bayer.dg.gain_q8 = 512;
            config.aec.source = FTEL_ISP_AEC_AFTER_DG;
        }
        check(ftel_isp_init_bayer_aec(&config, &init) == FTEL_ISP_OK, "init before each one-shot frame");
        check(ftel_isp_read_aec(&stats) == FTEL_ISP_E_NO_DATA, "no stats before first frame");
        fill(RAW_ARENA, 0x4000u); fill(OUT_ARENA, 0x8000u);
        for (unsigned y = 0; y < HEIGHT; ++y) {
            volatile uint16_t *row = (volatile uint16_t *)(RAW_ADDR + y * RAW_STRIDE);
            for (unsigned x = 0; x < WIDTH; ++x) row[x] = sample(test, x, y);
        }
        ftel_isp_frame_t frame = {
            .raw = {RAW_ADDR, RAW_STRIDE, RAW_STRIDE * HEIGHT},
            .y = {Y_ADDR, Y_STRIDE, Y_STRIDE * init.active_height},
            .uv = {UV_ADDR, UV_STRIDE, UV_STRIDE * (init.active_height / 2u)},
            .tag = 0xcafe0000u + test, .timestamp = 1000u + test,
        };
        ftel_isp_frame_result_t result;
        ftel_isp_info_t before;
        check(ftel_isp_get_info(&before) == FTEL_ISP_OK && !before.busy, "snapshot DMA counts before submit");
        ftel_isp_frame_t bad = frame; bad.raw.address += 2;
        check(ftel_isp_run_frame(&bad, 20000u, &result) == FTEL_ISP_E_CONFIG,
              "unaligned buffer rejected before DMA");
        bad = frame; bad.uv.address = frame.y.address;
        check(ftel_isp_run_frame(&bad, 20000u, &result) == FTEL_ISP_E_CONFIG,
              "overlapping output rejected before DMA");
        if (test == 0) {
            bad = frame; --bad.raw.bytes;
            check(ftel_isp_run_frame(&bad, 20000u, &result) == FTEL_ISP_E_CONFIG,
                  "short RAW buffer rejected before DMA");
            bad = frame; bad.y.address = FX1_FW_IMAGE_BASE;
            check(ftel_isp_run_frame(&bad, 20000u, &result) == FTEL_ISP_E_CONFIG,
                  "output cannot overwrite firmware/shared DDR");
        }
        ftel_isp_status_t status = ftel_isp_run_frame(&frame, 20000u, &result);
        fx1_log_hex("FRAME_STATUS", (uint32_t)status);
        check(status == FTEL_ISP_OK, "RAW -> IDMA -> ISP -> CH0 complete");
        check(result.input_count == before.input_count + 1u &&
              result.output_count == before.output_count + 1u && result.frame_id == test + 1u &&
              result.tag == frame.tag && result.timestamp == frame.timestamp,
              "one input/output and matching frame identity");
        fx1_log_hex("DMA_PAUSE_SNAPSHOT", result.dma_errors);
        check(ftel_isp_read_aec(&stats) == FTEL_ISP_OK, "AEC acquire/read/release through HAL");
        check(stats.frame_id == result.frame_id && stats.context_id == config.aec.context_id &&
              stats.source == config.aec.source && !stats.overflow, "AEC frame/context/source and overflow");
        uint64_t sum, ue, oe, hist[64];
        expected(&config, test, init.active_width, init.active_height, &sum, &ue, &oe, hist);
        uint64_t count = (uint64_t)init.active_width * init.active_height;
        check(stats.sum == sum && stats.count == count && stats.ue_count == ue && stats.oe_count == oe,
              "AEC sum/count/UE/OE match independent RAW oracle");
        bool histogram_ok = true;
        for (unsigned i = 0; i < 64; ++i) if (stats.histogram[i] != hist[i]) histogram_ok = false;
        check(histogram_ok, "all 64 histogram bins match RAW oracle");
        fx1_log_hex("AEC_SUM", (uint32_t)stats.sum);
        fx1_log_hex("AEC_COUNT", (uint32_t)stats.count);
        check(ftel_isp_read_aec(&stats) == FTEL_ISP_E_NO_DATA && stats.sum == sum,
              "result consumed once, no stale stats republished");
        check(input_unchanged(test), "RAW pixels/padding/guards unchanged");
        check(padding(Y_ADDR, Y_STRIDE, init.active_width, init.active_height) &&
              padding(UV_ADDR, UV_STRIDE, init.active_width, init.active_height / 2u),
              "output padding/guards preserved");
        check(ftel_isp_run_frame(&frame, 20000u, &result) == FTEL_ISP_E_STATE,
              "second submit requires explicit re-init");
        ftel_isp_info_t info;
        check(ftel_isp_get_info(&info) == FTEL_ISP_OK && !info.busy, "frame returns idle ISP/DMA");
    }
    fx1_puts("FRAME_AEC PASS frames="); fx1_putdec(CASES);
    fx1_puts(" checks="); fx1_putdec(checks); fx1_putc('\n');
    fx1_pass();
}

int main(void)
{
    NEON_FX1_FreeRTOS_Init();
    configASSERT(xTaskCreate(frame_task, "bayer_aec", 2048, NULL, 2, NULL) == pdPASS);
    vTaskStartScheduler();
    fx1_fail(0xecff);
}
