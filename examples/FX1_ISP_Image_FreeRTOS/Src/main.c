/* Demo RAW/profile local trên FreeRTOS, theo thứ tự của FX1_ISP_Image/Src/main.c.
 * Khác biệt ở cách chạy: client gửi job, worker giữ ISP và chờ frame bằng IRQ.
 * Runner và thư viện driver giữ nguyên. */
#include "fx1_fw.h"
#include "ftel_isp_freertos.h"
#include "neon_freertos_fx1.h"
#include "task.h"
#include "profile.h"
#ifdef PIPELINE_CONTRACT_TEST
#include "pipeline_checks.h"
#endif

#define WIDTH 2688u
#define HEIGHT 1520u
#define RAW_BASE 0x81000000u
#define OUTPUT_BASE 0x88000000u
#define SLOT_BYTES 0x00800000u
#ifndef FRAME_COUNT
#define FRAME_COUNT 2u
#endif
_Static_assert(FRAME_COUNT >= 1 && FRAME_COUNT <= 4, "1..4 frame slots");
_Static_assert(WIDTH * HEIGHT * 2u < SLOT_BYTES, "RAW slot capacity");
_Static_assert(RAW_BASE + 4u * SLOT_BYTES <= FX1_FW_YUV_BUF_BASE, "RAW region");
_Static_assert(OUTPUT_BASE + 4u * SLOT_BYTES < FX1_FW_TEST_AREA_BASE, "output region");
static uint32_t checks;
static volatile uint32_t heartbeat;

static void show(const char *name, uint32_t value)
{
    fx1_puts(name); fx1_puts(" = "); fx1_puthex(value); fx1_putc('\n');
}
static void check(bool ok, const char *name)
{
    ++checks;
    fx1_puts(ok ? "[PASS] " : "[FAIL] "); fx1_puts(name); fx1_putc('\n');
    if (!ok) fx1_fail(0xf800u + checks);
}
static void background(void *unused)
{
    (void)unused;
    for (;;) { ++heartbeat; vTaskDelay(1); }
}

static void image_task(void *unused)
{
    (void)unused;
    static isp_config_t cfg;
    static ftel_isp_pipeline_result_t result;
    /* 1. Giống bare-metal: mặc định -> input RAW -> profile trong RAM. */
    isp_get_default_config(&cfg);
    cfg.ifmt = (ifmt_config_t){WIDTH, HEIGHT, BAYER_BGGR, INPUT_DEPTH_RAW12};
    output_get_default_config(&cfg.output, WIDTH, HEIGHT);
    apply_profile(&cfg);
    show("FRAME_COUNT", FRAME_COUNT);
    /* 2. Giống bare-metal: tính output, stride và bốn slot DDR. */
    output_geometry_t geometry[3];
    for (unsigned c = 0; c < 3; ++c)
        check(output_compute_geometry(&cfg.output.channel[c], c, WIDTH, HEIGHT,
                                      &geometry[c]) == ISP_OK, "profile output geometry");
    dma_get_default_config(&cfg.dma, WIDTH, geometry);
    cfg.dma.input_stride = WIDTH * 2u;
    cfg.dma.output[0].y_stride = cfg.dma.output[0].uv_stride = WIDTH;
    for (unsigned i = 0; i < 4; ++i) {
        cfg.dma.input[i] = RAW_BASE + i * SLOT_BYTES;
        cfg.dma.output[0].y[i] = OUTPUT_BASE + i * SLOT_BYTES;
        cfg.dma.output[0].uv[i] = cfg.dma.output[0].y[i] + 0x00400000u;
    }
    /* 3. Worker thực hiện isp_init/stream; client không đọc/ghi CSR ISP.
     * RAW được runner nạp vào DDR, submit chỉ gửi config và descriptor. */
    check(ftel_isp_freertos_start(NULL) == FTEL_ISP_OK, "start exclusive ISP worker");
    ftel_isp_pipeline_request_t request = {
        .config = &cfg, .frame_count = FRAME_COUNT, .frame_timeout_us = 2000000u,
    };
    for (unsigned i = 0; i < FRAME_COUNT; ++i) {
        request.tag[i] = 0x1000u + i; request.timestamp[i] = 1000u + i;
    }
#ifdef PIPELINE_CONTRACT_TEST
    pipeline_checks_before_submit(&cfg, &request, check, show);
#endif
    ftel_isp_ticket_t ticket;
    check(ftel_isp_freertos_submit_pipeline(&request, &ticket) == FTEL_ISP_OK, "submit full profile");
#ifdef PIPELINE_CONTRACT_TEST
    pipeline_checks_after_submit(&cfg, &request);
#endif
    /* 4. Chờ worker bằng semaphore thay cho vòng polling ở bare-metal. */
    check(ftel_isp_freertos_wait_pipeline(ticket, pdMS_TO_TICKS(10000), &result) == FTEL_ISP_OK,
          "receive pipeline result");
    show("JOB_STATUS", result.job.status); show("INIT_STATUS", result.init.status);
    show("INIT_STAGE", result.init.stage); show("IRQ_COUNT", result.job.irq_count);
    show("LSC_STAGE", result.init.lsc.stage); show("LSC_PROFILE", result.init.lsc.profile);
    show("LSC_STATUS", result.init.lsc.profile_status); show("LSC_ERROR", result.init.lsc.errors);
    check(result.job.status == FTEL_ISP_OK && result.init.stage == ISP_INIT_READY &&
          result.completed_frames == FRAME_COUNT && result.job.buffers_released &&
          result.job.irq_count > 0, "all frames complete through ISR/worker");
    show("OUTPUT_WIDTH", geometry[0].width); show("OUTPUT_HEIGHT", geometry[0].height);
    /* 5. Giống bare-metal: kiểm slot, count, metadata và lỗi DMA mỗi frame. */
    for (unsigned i = 0; i < FRAME_COUNT; ++i) {
        const isp_frame_result_t *f = &result.frame[i];
        show("FRAME_SLOT", f->slot); show("DMA_ERRORS", f->dma.errors);
        show("INPUT_COUNT", f->dma.input_count);
        show("OUTPUT_COUNT", f->dma.output_count[0]); show("OUTPUT_TAG", f->tag[0]);
        check(f->slot == i && f->dma.input_count == result.frame[0].dma.input_count + i &&
              f->dma.output_count[0] == result.frame[0].dma.output_count[0] + i &&
              f->tag[0] == 0x1000u + i && f->timestamp[0] == 1000u + i &&
              !(f->dma.errors & ~DMA_ERROR_INPUT_PAUSED), "slot/counter/metadata match frame");
    }
    /* 6. Trả ticket sau khi worker đã release slot/stop stream an toàn. */
    check(ftel_isp_freertos_release(ticket) == FTEL_ISP_OK, "release output for client");
    check(heartbeat > 0 && uxTaskGetStackHighWaterMark(NULL) > 64,
          "background task runs; client stack has margin");
    /* 7. Giống bare-metal: xuất NV12 của frame cuối cho cùng runner. */
    show("NV12_BASE", OUTPUT_BASE + (FRAME_COUNT - 1u) * SLOT_BYTES);
    show("NV12_BYTES", (uint32_t)geometry[0].width * geometry[0].height * 3u / 2u);
    fx1_puts("PIPELINE_FREERTOS PASS checks="); fx1_putdec(checks); fx1_putc('\n');
    fx1_puts("IMAGE_RESULT=PASS\n");
    fx1_pass();
}
int main(void)
{
    fx1_puts("FW_BOOT: Neon ISP full pipeline / FreeRTOS\n");
    NEON_FX1_FreeRTOS_Init();
    check(xTaskCreate(image_task, "image_client", 4096, NULL, 3, NULL) == pdPASS,
          "create image client");
    check(xTaskCreate(background, "background", 256, NULL, 1, NULL) == pdPASS,
          "create background task");
    vTaskStartScheduler();
    fx1_fail(0xf8ff);
}
