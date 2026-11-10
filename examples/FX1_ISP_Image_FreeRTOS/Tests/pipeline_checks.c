/* Chỉ liên kết vào fx1_isp_pipeline_checks; demo ảnh không chứa fault injection. */
#include "pipeline_checks.h"
/* Bố trí DDR giống Src/main.c và example bare-metal. */
#define WIDTH 2688u
#define HEIGHT 1520u
#define RAW_BASE 0x81000000u
#define OUTPUT_BASE 0x88000000u
#ifndef FRAME_COUNT
#define FRAME_COUNT 2u
#endif
static ftel_isp_pipeline_result_t result;
static bool hold_recovery;
extern ftel_isp_status_t __real_ftel_isp_fx1_recovery_poll(void);
ftel_isp_status_t __wrap_ftel_isp_fx1_recovery_poll(void)
{
    return hold_recovery ? FTEL_ISP_E_NO_DATA : __real_ftel_isp_fx1_recovery_poll();
}
void pipeline_checks_before_submit(isp_config_t *cfg, ftel_isp_pipeline_request_t *request,
                                   pipeline_check_fn_t check, pipeline_show_fn_t show)
{
    ftel_isp_ticket_t ticket;
    lsc_init_config_t original_lsc = cfg->lsc;
    for (unsigned i = 0; i < 3; ++i) {
        cfg->lsc.coefficients[i] = original_lsc.coefficients[original_lsc.profile];
        cfg->lsc.coefficient_count[i] = original_lsc.coefficient_count[original_lsc.profile];
    }
    check(ftel_isp_freertos_submit_pipeline(request, &ticket) == FTEL_ISP_E_UNSUPPORTED,
          "three LSC tables require a free resident");
    cfg->lsc = original_lsc;
    uint64_t original = cfg->dma.input[1];
    cfg->dma.input[1] = cfg->dma.input[0];
    check(ftel_isp_freertos_submit_pipeline(request, &ticket) == FTEL_ISP_E_CONFIG,
          "reject overlapping DMA slots before queue");
    cfg->dma.input[1] = original;
    cfg->dma.input[0] = 0x80000000u;
    check(ftel_isp_freertos_submit_pipeline(request, &ticket) == FTEL_ISP_E_CONFIG,
          "protect firmware DDR");
    cfg->dma.input[0] = RAW_BASE;
    request->frame_count = 0;
    check(ftel_isp_freertos_submit_pipeline(request, &ticket) == FTEL_ISP_E_CONFIG,
          "reject zero frame count");
    request->frame_count = FRAME_COUNT;
    ftel_isp_config_t basic;
    ftel_isp_get_default_config(&basic);
    basic.bayer.input.width = 64; basic.bayer.input.height = 48;
    basic.aec.enable = true;
    ftel_isp_frame_t frame = {
        .raw = {RAW_BASE, WIDTH * 2, WIDTH * HEIGHT * 2},
        .y = {OUTPUT_BASE, WIDTH, WIDTH * HEIGHT},
        .uv = {OUTPUT_BASE + 0x400000u, WIDTH, WIDTH * HEIGHT / 2},
    };
    ftel_isp_job_result_t basic_result;
    check(ftel_isp_freertos_submit(&basic, &frame, 2000000u, &ticket) == FTEL_ISP_OK,
          "queue Bayer job in shared service");
    ftel_isp_ticket_t other;
    check(ftel_isp_freertos_submit_pipeline(request, &other) == FTEL_ISP_E_BUSY,
          "pipeline cannot alias queued Bayer buffers");
    check(ftel_isp_freertos_wait_pipeline(ticket, 0, &result) == FTEL_ISP_E_STATE,
          "pipeline wait rejects Bayer ticket");
    check(ftel_isp_freertos_wait(ticket, pdMS_TO_TICKS(5000), &basic_result) == FTEL_ISP_OK &&
          basic_result.status == FTEL_ISP_OK, "Bayer job completes before full pipeline");
    check(ftel_isp_freertos_submit_pipeline(request, &other) == FTEL_ISP_E_BUSY,
          "DONE Bayer ticket still owns buffers");
    check(ftel_isp_freertos_release(ticket) == FTEL_ISP_OK, "release Bayer ticket");
    request->frame_timeout_us = 1;
    hold_recovery = true;
    check(ftel_isp_freertos_submit_pipeline(request, &ticket) == FTEL_ISP_OK,
          "queue pipeline timeout test");
    check(ftel_isp_freertos_submit(&basic, &frame, 2000000u, &other) == FTEL_ISP_E_BUSY,
          "Bayer job cannot alias queued pipeline buffers");
    check(ftel_isp_freertos_submit_pipeline(request, &other) == FTEL_ISP_E_BUSY,
          "pipeline job cannot alias another ticket");
    check(ftel_isp_freertos_wait(ticket, 0, &basic_result) == FTEL_ISP_E_STATE,
          "reject wrong wait type");
    result.completed_frames = 0xabcdefu;
    check(ftel_isp_freertos_wait_pipeline(ticket, 0, &result) == FTEL_ISP_E_TIMEOUT &&
          result.completed_frames == 0xabcdefu, "client timeout preserves result and ticket");
    check(ftel_isp_freertos_release(ticket) == FTEL_ISP_E_BUSY, "queued buffer remains leased");
    check(ftel_isp_freertos_wait_pipeline(ticket, pdMS_TO_TICKS(5000), &result) == FTEL_ISP_OK,
          "wait faulted pipeline result");
    show("FAULT_STATUS", result.job.status); show("FAULT_RECOVERY", result.job.recovery_status);
    show("FAULT_INIT", result.init.status); show("FAULT_STAGE", result.init.stage);
    show("FAULT_SUBMITTED", result.job.frame_submitted);
    check(result.job.status == FTEL_ISP_E_TIMEOUT && result.job.frame_submitted &&
          result.init.stage == ISP_INIT_READY && !result.job.buffers_released &&
          result.job.recovery_status == FTEL_ISP_E_TIMEOUT, "failed recovery quarantines buffers");
    check(ftel_isp_freertos_release(ticket) == FTEL_ISP_E_BUSY &&
          ftel_isp_freertos_submit_pipeline(request, &other) == FTEL_ISP_E_STATE,
          "faulted service blocks reuse and new jobs");
    hold_recovery = false;
    check(ftel_isp_freertos_recover(pdMS_TO_TICKS(1000)) == FTEL_ISP_OK,
          "explicit recovery drains hardware");
    check(ftel_isp_freertos_wait_pipeline(ticket, 0, &result) == FTEL_ISP_OK &&
          result.job.buffers_released && result.job.recovered, "recovered ticket returns buffer rights");
    check(ftel_isp_freertos_release(ticket) == FTEL_ISP_OK &&
          ftel_isp_freertos_wait_pipeline(ticket, 0, &result) == FTEL_ISP_E_STALE,
          "release invalidates pipeline ticket");
    request->frame_timeout_us = 2000000u;
    check(ftel_isp_freertos_submit_pipeline(request, &ticket) == FTEL_ISP_OK,
          "restart pipeline after recovery");
    check(ftel_isp_freertos_wait_pipeline(ticket, pdMS_TO_TICKS(10000), &result) == FTEL_ISP_OK &&
          result.job.status == FTEL_ISP_OK && result.completed_frames == FRAME_COUNT,
          "restarted job completes with identical tuning");
    check(ftel_isp_freertos_release(ticket) == FTEL_ISP_OK,
          "release restarted pipeline before another job");
}
void pipeline_checks_after_submit(isp_config_t *cfg, ftel_isp_pipeline_request_t *request)
{
    /* Kiểm chứng config/metadata đã được copy; không đổi các bảng LSC. */
    cfg->wb.gain_r = 256;
    request->tag[0] = 0xbadu;
}
