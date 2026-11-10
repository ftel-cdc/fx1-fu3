/* Kiểm chứng RTOS/service bằng firmware: không sửa VP/RTL/testbench. */
#include "ftel_isp_freertos.h"
#include "neon_freertos_fx1.h"
#include "task.h"
#include "semphr.h"

#define W 64u
#define H 48u
#define STRIDE 144u
#define ARENA_SIZE 0x10000u
static unsigned checks;
static volatile uint32_t heartbeat, clients_passed;
static volatile bool delay_recovery_ack;
static SemaphoreHandle_t client_done;
static TaskHandle_t manager_handle;
static ftel_isp_job_result_t result;
static ftel_isp_ticket_t foreign_ticket;
static volatile bool foreign_rejected;

/* Wrap chỉ trong ELF test, trì hoãn công bố recovery hoàn tất cho adapter. */
extern ftel_isp_status_t __real_ftel_isp_fx1_recovery_poll(void);
ftel_isp_status_t __wrap_ftel_isp_fx1_recovery_poll(void)
{
    return delay_recovery_ack ? FTEL_ISP_E_NO_DATA : __real_ftel_isp_fx1_recovery_poll();
}

static void check(bool ok, const char *name)
{
    ++checks;
    fx1_puts(ok ? "[service] PASS " : "[service] FAIL ");
    fx1_puts(name); fx1_putc('\n');
    if (!ok) fx1_fail(0xee00u + checks);
}

static ftel_isp_frame_t make_frame(unsigned arena, uint16_t sample)
{
    uintptr_t base = FX1_FW_RAW_BUF_BASE + 0x100000u + arena * ARENA_SIZE;
    ftel_isp_frame_t f = {
        .raw = {base + 0x100u, STRIDE, STRIDE * H},
        .y = {base + 0x4000u, STRIDE, STRIDE * H},
        .uv = {base + 0x8000u, STRIDE, STRIDE * H / 2u},
        .tag = 0xaec000u + arena, .timestamp = 500u + arena,
    };
    const ftel_isp_buffer_t *buffers[] = {&f.raw, &f.y, &f.uv};
    for (unsigned b = 0; b < 3; ++b) {
        volatile uint8_t *p = (volatile uint8_t *)buffers[b]->address;
        p[-1] = 0xa5;
        for (unsigned i = 0; i <= buffers[b]->bytes; ++i) p[i] = 0xa5;
    }
    for (unsigned y = 0; y < H; ++y) {
        volatile uint16_t *row = (volatile uint16_t *)(f.raw.address + y * STRIDE);
        for (unsigned x = 0; x < W; ++x) row[x] = sample;
    }
    return f;
}

static bool guards(const ftel_isp_frame_t *f, uint16_t sample)
{
    for (unsigned y = 0; y < H; ++y) {
        volatile uint16_t *row = (volatile uint16_t *)(f->raw.address + y * STRIDE);
        for (unsigned x = 0; x < W; ++x) if (row[x] != sample) return false;
    }
    const ftel_isp_buffer_t *buffers[] = {&f->raw, &f->y, &f->uv};
    unsigned widths[] = {2u * W, W, W};
    unsigned heights[] = {H, H, H / 2u};
    for (unsigned b = 0; b < 3; ++b) {
        volatile uint8_t *p = (volatile uint8_t *)buffers[b]->address;
        if (p[-1] != 0xa5 || p[buffers[b]->bytes] != 0xa5) return false;
        for (unsigned y = 0; y < heights[b]; ++y)
            for (unsigned x = widths[b]; x < STRIDE; ++x)
                if (p[y * STRIDE + x] != 0xa5) return false;
    }
    return true;
}

static bool stats_ok(const ftel_isp_job_result_t *r, const ftel_isp_frame_t *f,
                      uint32_t context, unsigned sample)
{
    if (r->status != FTEL_ISP_OK || !r->frame_submitted || !r->stats_valid ||
        !r->buffers_released || !r->irq_count || r->aec.overflow ||
        r->frame.tag != f->tag || r->frame.timestamp != f->timestamp ||
        r->aec.frame_id != r->frame.frame_id || r->aec.context_id != context ||
        r->aec.count != W * H || r->aec.sum != (uint64_t)sample * W * H) return false;
    for (unsigned i = 0; i < 64; ++i)
        if (r->aec.histogram[i] != (i == (sample >> 6) ? W * H : 0u)) return false;
    return guards(f, (uint16_t)sample);
}

static void background(void *unused)
{
    (void)unused;
    for (;;) { ++heartbeat; vTaskDelay(1); }
}

static void foreign_client(void *unused)
{
    (void)unused;
    ftel_isp_job_result_t r;
    foreign_rejected = ftel_isp_freertos_wait(foreign_ticket, 0, &r) == FTEL_ISP_E_BUSY &&
                       ftel_isp_freertos_release(foreign_ticket) == FTEL_ISP_E_BUSY;
    xTaskNotifyGive(manager_handle);
    vTaskDelete(NULL);
}

static void client(void *arg)
{
    unsigned id = (unsigned)(uintptr_t)arg;
    ftel_isp_job_result_t r;
    for (unsigned round = 0; round < 8; ++round) {
        unsigned sample = 512u + 64u * round + id * 128u;
        ftel_isp_frame_t f = make_frame(5u + id, (uint16_t)sample);
        ftel_isp_config_t c; ftel_isp_get_default_config(&c);
        c.aec.context_id = 0xface0000u + 16u * id + round;
        ftel_isp_ticket_t ticket;
        if (ftel_isp_freertos_submit(&c, &f, 50000u, &ticket) != FTEL_ISP_OK ||
            ftel_isp_freertos_wait(ticket, pdMS_TO_TICKS(100), &r) != FTEL_ISP_OK ||
            !stats_ok(&r, &f, c.aec.context_id, sample) ||
            ftel_isp_freertos_release(ticket) != FTEL_ISP_OK) fx1_fail(0xef01u + id);
        vTaskDelay(1);
    }
    taskENTER_CRITICAL(); ++clients_passed; taskEXIT_CRITICAL();
    xSemaphoreGive(client_done);
    vTaskDelete(NULL);
}

static void manager(void *unused)
{
    (void)unused;
    manager_handle = xTaskGetCurrentTaskHandle();
    ftel_isp_freertos_config_t options;
    ftel_isp_freertos_get_default_config(&options);
    options.recovery_timeout_us = 3000u;
    check(ftel_isp_freertos_start(&options) == FTEL_ISP_OK, "start owner task and register ISP PLIC IRQ");
    check(ftel_isp_freertos_start(&options) == FTEL_ISP_E_BUSY, "duplicate start rejected");
    ftel_isp_config_t c; ftel_isp_get_default_config(&c); c.aec.context_id = 0x12345678u;
    ftel_isp_info_t info;
    check(ftel_isp_get_info(&info) == FTEL_ISP_E_BUSY &&
          ftel_isp_init_bayer_aec(&c, NULL) == FTEL_ISP_E_BUSY,
          "only worker may access ISP through pure API");

    ftel_isp_frame_t frames[4]; ftel_isp_ticket_t tickets[4];
    for (unsigned i = 0; i < 4; ++i) frames[i] = make_frame(i, 1024);
    check(ftel_isp_freertos_submit(&c, &frames[0], 50000u, &tickets[0]) == FTEL_ISP_OK,
          "submit copies config and descriptor");
    check(ftel_isp_freertos_submit(&c, &frames[0], 50000u, &tickets[3]) == FTEL_ISP_E_BUSY,
          "overlapping leased buffers rejected");
    result.status = FTEL_ISP_E_ABI;
    check(ftel_isp_freertos_wait(tickets[0], 0, &result) == FTEL_ISP_E_TIMEOUT &&
          result.status == FTEL_ISP_E_ABI, "caller wait timeout keeps ticket and result unchanged");
    check(ftel_isp_freertos_release(tickets[0]) == FTEL_ISP_E_BUSY, "cannot release queued or active job");
    foreign_ticket = tickets[0];
    configASSERT(xTaskCreate(foreign_client, "foreign_client", 1024, NULL, 4, NULL) == pdPASS);
    check(ulTaskNotifyTake(pdTRUE, 0) == 1 && foreign_rejected, "another task cannot wait or release caller ticket");
    for (unsigned i = 1; i < 3; ++i)
        check(ftel_isp_freertos_submit(&c, &frames[i], 50000u, &tickets[i]) == FTEL_ISP_OK,
              "queue independent frame");
    check(ftel_isp_freertos_submit(&c, &frames[3], 50000u, &tickets[3]) == FTEL_ISP_E_BUSY,
          "ticket pool bounded");
    c.aec.context_id = 0; /* Không làm đổi bản copy đang nằm trong queue. */
    for (unsigned i = 0; i < 3; ++i) {
        check(ftel_isp_freertos_wait(tickets[i], pdMS_TO_TICKS(100), &result) == FTEL_ISP_OK &&
              stats_ok(&result, &frames[i], 0x12345678u, 1024), "IRQ frame result stats identity and DDR guards");
        check(ftel_isp_freertos_release(tickets[i]) == FTEL_ISP_OK, "return buffer lease after completion");
        check(ftel_isp_freertos_wait(tickets[i], 0, &result) == FTEL_ISP_E_STALE,
              "released ticket rejected");
    }
    ftel_isp_ticket_t stale = tickets[0];
    c.aec.context_id = 0x12345678u;
    c.bayer.dg.gain_q8 = 0xffffu;
    check(ftel_isp_freertos_submit(&c, &frames[0], 50000u, &tickets[0]) == FTEL_ISP_OK,
          "invalid config processed by owner");
    check(ftel_isp_freertos_wait(stale, 0, &result) == FTEL_ISP_E_STALE,
          "generation rejects stale ticket after slot reuse");
    check(ftel_isp_freertos_wait(tickets[0], pdMS_TO_TICKS(100), &result) == FTEL_ISP_OK &&
          result.status == FTEL_ISP_E_CONFIG && result.buffers_released && !result.frame_submitted,
          "validation failure returns buffers without DMA");
    check(ftel_isp_freertos_release(tickets[0]) == FTEL_ISP_OK, "release failed validation job");
    c.bayer.dg.gain_q8 = 256;

    delay_recovery_ack = true;
    check(ftel_isp_freertos_submit(&c, &frames[0], 1u, &tickets[0]) == FTEL_ISP_OK,
          "submit job with expired hardware deadline");
    check(ftel_isp_freertos_submit(&c, &frames[1], 50000u, &tickets[1]) == FTEL_ISP_OK,
          "queue frame behind failing job");
    check(ftel_isp_freertos_wait(tickets[0], pdMS_TO_TICKS(100), &result) == FTEL_ISP_OK &&
          result.status == FTEL_ISP_E_TIMEOUT && result.frame_submitted &&
          result.recovery_status == FTEL_ISP_E_TIMEOUT && !result.buffers_released,
          "recovery timeout quarantines DMA buffers");
    check(ftel_isp_freertos_release(tickets[0]) == FTEL_ISP_E_BUSY &&
          ftel_isp_freertos_submit(&c, &frames[2], 50000u, &tickets[2]) == FTEL_ISP_E_STATE,
          "faulted service rejects release and new jobs");
    check(ftel_isp_freertos_wait(tickets[1], pdMS_TO_TICKS(100), &result) == FTEL_ISP_OK &&
          result.status == FTEL_ISP_E_STATE && result.buffers_released && !result.frame_submitted &&
          ftel_isp_freertos_release(tickets[1]) == FTEL_ISP_OK,
          "faulted service cancels queued frame without DMA");
    delay_recovery_ack = false;
    check(ftel_isp_freertos_recover(pdMS_TO_TICKS(100)) == FTEL_ISP_OK, "explicit recovery confirms hardware drain");
    check(ftel_isp_freertos_wait(tickets[0], 0, &result) == FTEL_ISP_OK && result.recovered &&
          result.buffers_released && result.status == FTEL_ISP_E_TIMEOUT,
          "late recovery returns buffer but preserves original frame error");
    check(ftel_isp_freertos_release(tickets[0]) == FTEL_ISP_OK, "release quarantined ticket after recovery");
    check(ftel_isp_freertos_recover(0) == FTEL_ISP_OK, "healthy service recovery is no-op");
    check(ftel_isp_freertos_submit(&c, &frames[0], 1u, &tickets[0]) == FTEL_ISP_OK &&
          ftel_isp_freertos_wait(tickets[0], pdMS_TO_TICKS(100), &result) == FTEL_ISP_OK &&
          result.status == FTEL_ISP_E_TIMEOUT && result.frame_submitted && result.recovered &&
          result.recovery_status == FTEL_ISP_OK && result.buffers_released &&
          ftel_isp_freertos_release(tickets[0]) == FTEL_ISP_OK,
          "auto-recovery confirms drain after frame timeout");

    client_done = xSemaphoreCreateCounting(2, 0); configASSERT(client_done);
    uint32_t before = heartbeat;
    configASSERT(xTaskCreate(client, "isp_clientA", 2048, (void *)0, 2, NULL) == pdPASS);
    configASSERT(xTaskCreate(client, "isp_clientB", 2048, (void *)1, 2, NULL) == pdPASS);
    check(xSemaphoreTake(client_done, pdMS_TO_TICKS(1000)) == pdTRUE &&
          xSemaphoreTake(client_done, pdMS_TO_TICKS(1000)) == pdTRUE && clients_passed == 2,
          "two client tasks each process 8 IRQ frames after recovery");
    check(heartbeat > before, "background task progresses while ISP service runs");
    check(uxTaskGetStackHighWaterMark(manager_handle) > 64, "manager stack margin");
    fx1_puts("ISP_FREERTOS_SERVICE PASS checks="); fx1_putdec(checks); fx1_putc('\n');
    fx1_pass();
}

int main(void)
{
    NEON_FX1_FreeRTOS_Init();
    configASSERT(xTaskCreate(background, "background", 256, NULL, 1, NULL) == pdPASS);
    configASSERT(xTaskCreate(manager, "isp_manager", 2048, NULL, 3, NULL) == pdPASS);
    vTaskStartScheduler();
    fx1_fail(0xefff);
}
