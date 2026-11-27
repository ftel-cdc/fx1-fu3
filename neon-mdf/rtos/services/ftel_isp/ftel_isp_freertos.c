/* Một worker sở hữu ISP; queue giữ descriptor, notification nối ISR với task.
 * Nguồn local, không phụ thuộc firmware/driver tham chiếu của VP. */
#include "ftel_isp_freertos.h"
#include "ftel_isp_private.h"
#include "platform/fx1/ftel_isp_platform.h"
#include "neon_freertos_fx1.h"
#include "queue.h"
#include "semphr.h"
#include "task.h"
#include "fx1_fw.h"

#if configUSE_MUTEXES != 1 || configUSE_TASK_NOTIFICATIONS != 1 || configSUPPORT_DYNAMIC_ALLOCATION != 1 || \
    INCLUDE_xTaskGetSchedulerState != 1 || INCLUDE_vTaskDelay != 1 || INCLUDE_vTaskDelete != 1
#error "ISP FreeRTOS can mutex, dynamic allocation, scheduler state, delay va task delete"
#endif

#define RECOVER_COMMAND UINT32_MAX
#define DMA_IRQ_FATAL 0x61u
#define DMA_IRQ_DONE 0x10u

typedef enum { JOB_FREE, JOB_QUEUED, JOB_ACTIVE, JOB_DONE } job_state_t;
#define MAX_SPANS 28u /* 4 RAW + 4*(Y/UV CH0 + Y/UV CH1 + RGB CH2). */
typedef struct {
    isp_config_t config;
    isp_init_result_t init;
    isp_frame_result_t frame[FTEL_ISP_FREERTOS_MAX_FRAMES];
    uint32_t count, completed, tag[4], timestamp[4];
} pipeline_job_t;

typedef struct {
    job_state_t state;
    bool pipeline;
    unsigned span_count;
    ftel_isp_buffer_t span[MAX_SPANS];
    pipeline_job_t full;
    uint32_t generation, timeout_us;
    TaskHandle_t caller;
    SemaphoreHandle_t done;
    ftel_isp_config_t config;
    ftel_isp_frame_t frame;
    ftel_isp_job_result_t result;
} job_t;

static struct {
    unsigned state; /* 0 chưa start, 1 đang tạo, 2 hoạt động, 3 startup lỗi. */
    bool faulted, recovery_pending;
    SemaphoreHandle_t lock, startup, recovery_done;
    QueueHandle_t queue;
    TaskHandle_t worker;
    ftel_isp_freertos_config_t config;
    ftel_isp_status_t startup_status, recovery_status;
    job_t jobs[FTEL_ISP_FREERTOS_MAX_JOBS];
    volatile uint32_t irq_count, common_irq, dma_irq, output_irq, dma_error;
} service;

static void lock(void) { configASSERT(xSemaphoreTake(service.lock, portMAX_DELAY) == pdTRUE); }
static void unlock(void) { xSemaphoreGive(service.lock); }

static bool worker_owns_isp(void *unused)
{
    (void)unused;
    return fx1_hartid() == 0 && xTaskGetCurrentTaskHandle() == service.worker;
}

static void mask_irq(void)
{
    SDK_IRQ_Disable(FX1_IRQ_ISP);
    REG_WR(R_COMMON_IRQ_EN, 0);
    REG_WR(R_DMA_IRQ_EN, 0);
    REG_WR(FX1_ISP_OUTPUT_IRQ_EN_OFFSET, 0);
    isp_barrier();
}

static void ack_irq(void)
{
    REG_WR(R_COMMON_IRQ_STATUS, REG_RD(R_COMMON_IRQ_STATUS) & 7u);
    REG_WR(FX1_ISP_DMA_IRQ_STAT_OFFSET, REG_RD(FX1_ISP_DMA_IRQ_STAT_OFFSET) & 0x7fu);
    REG_WR(FX1_ISP_OUTPUT_IRQ_STATUS_OFFSET, REG_RD(FX1_ISP_OUTPUT_IRQ_STATUS_OFFSET) & 0x77u);
    isp_barrier();
}

static BaseType_t isp_isr(void *unused)
{
    (void)unused;
    BaseType_t woken = pdFALSE;
    uint32_t common = REG_RD(R_COMMON_IRQ_STATUS) & 7u;
    uint32_t dma = REG_RD(FX1_ISP_DMA_IRQ_STAT_OFFSET) & 0x7fu;
    uint32_t output = REG_RD(FX1_ISP_OUTPUT_IRQ_STATUS_OFFSET) & 0x77u;
    service.common_irq |= common;
    service.dma_irq |= dma;
    service.output_irq |= output;
    service.dma_error |= REG_RD(R_DMA_ERR);
    ++service.irq_count;
    /* Chỉ ACK snapshot, giữ nguyên lỗi gốc và quyền FREE/DONE của buffer. */
    REG_WR(R_COMMON_IRQ_STATUS, common);
    REG_WR(FX1_ISP_DMA_IRQ_STAT_OFFSET, dma);
    REG_WR(FX1_ISP_OUTPUT_IRQ_STATUS_OFFSET, output);
    isp_barrier();
    vTaskNotifyGiveFromISR(service.worker, &woken);
    return woken;
}

static TickType_t wait_ticks(uint64_t us)
{
    uint64_t ticks = (us * configTICK_RATE_HZ + 999999u) / 1000000u;
    if (!ticks) ticks = 1;
    if (ticks >= portMAX_DELAY) ticks = portMAX_DELAY - 1u;
    return (TickType_t)ticks;
}

static ftel_isp_status_t recover_hardware(void)
{
    mask_irq();
    ftel_isp_status_t status;
    if (!ftel_isp_fx1_session.recovering) {
        status = ftel_isp_fx1_recovery_begin();
        if (status != FTEL_ISP_OK) return status;
    }
    uint64_t start = ftel_isp_platform_time_us(NULL);
    do {
        status = ftel_isp_fx1_recovery_poll();
        if (status != FTEL_ISP_E_NO_DATA) return status;
        vTaskDelay(1);
    } while (ftel_isp_platform_time_us(NULL) - start < service.config.recovery_timeout_us);
    return FTEL_ISP_E_TIMEOUT;
}

static ftel_isp_status_t wait_frame(uint64_t start, uint32_t timeout,
                                   ftel_isp_frame_result_t *frame,
                                   isp_stream_t *stream, isp_frame_result_t *full_frame)
{
    bool completion_seen = false;
    for (;;) {
        uint64_t elapsed = ftel_isp_platform_time_us(NULL) - start;
        if (elapsed >= timeout) return FTEL_ISP_E_TIMEOUT;
        if (!completion_seen) {
            if (ulTaskNotifyTake(pdTRUE, wait_ticks(timeout - elapsed)) == 0)
                continue;
        }
        if (ftel_isp_platform_time_us(NULL) - start >= timeout) return FTEL_ISP_E_TIMEOUT;
        taskENTER_CRITICAL();
        uint32_t common = service.common_irq, dma = service.dma_irq;
        uint32_t output = service.output_irq, error = service.dma_error;
        taskEXIT_CRITICAL();
        if ((common & 2u) || (dma & DMA_IRQ_FATAL) || (output & 0x70u) ||
            (error & ~(1u << 1))) return FTEL_ISP_E_IO;
        completion_seen |= (common & 1u) || (dma & DMA_IRQ_DONE) || (output & 7u);
        if (completion_seen) {
            ftel_isp_status_t status = stream ?
                ftel_isp_fx1_status(isp_stream_poll(stream, full_frame)) :
                ftel_isp_fx1_collect(frame);
            if (status != FTEL_ISP_E_NO_DATA) return status;
            /* IRQ DONE có thể tới trước lúc BUSY hạ. Nhường CPU khi drain,
             * vẫn dùng deadline tuyệt đối, không gia hạn sau mỗi lần thức. */
            vTaskDelay(1);
        }
    }
}

static void process_job(job_t *job)
{
    ftel_isp_job_result_t *r = &job->result;
    *r = (ftel_isp_job_result_t){.buffers_released = true};
    mask_irq();
    r->status = ftel_isp_init_bayer_aec(&job->config, &r->init);
    if (r->status == FTEL_ISP_E_CONFIG) return;
    if (r->status == FTEL_ISP_OK) {
        ack_irq();
        (void)ulTaskNotifyTake(pdTRUE, 0);
        taskENTER_CRITICAL();
        service.irq_count = service.common_irq = service.dma_irq = 0;
        service.output_irq = service.dma_error = 0;
        taskEXIT_CRITICAL();
        uint64_t start = ftel_isp_platform_time_us(NULL);
        r->buffers_released = false;
        r->status = ftel_isp_fx1_submit(&job->frame, true, 1000u);
        r->frame_submitted = r->status == FTEL_ISP_OK;
        if (r->status == FTEL_ISP_OK) {
            SDK_IRQ_Enable(FX1_IRQ_ISP);
            r->status = wait_frame(start, job->timeout_us, &r->frame, NULL, NULL);
        }
        mask_irq();
        r->irq_count = service.irq_count;
        r->common_irq = service.common_irq;
        r->dma_irq = service.dma_irq;
        r->output_irq = service.output_irq;
        r->dma_errors = service.dma_error | REG_RD(R_DMA_ERR);
        if (r->status == FTEL_ISP_OK) {
            r->buffers_released = true;
            r->status = ftel_isp_read_aec(&r->aec);
            r->stats_valid = r->status == FTEL_ISP_OK && !r->aec.overflow;
            if (r->status == FTEL_ISP_OK && !r->stats_valid) r->status = FTEL_ISP_E_IO;
        }
    }
    if (r->status != FTEL_ISP_OK) {
        r->recovery_status = recover_hardware();
        r->recovered = r->recovery_status == FTEL_ISP_OK;
        r->buffers_released = r->recovered;
    }
}

/* Cache hooks nằm ở worker: client không đụng pixel khi ticket đang giữ buffer.
 * BSP VP hiện chỉ barrier; silicon có cache phải hiện thực clean/invalidate. */
static void cache_slot(const pipeline_job_t *p, unsigned slot, bool complete)
{
    const isp_config_t *cfg = &p->config;
    if (!complete)
        fx1_dma_prepare((void *)(uintptr_t)cfg->dma.input[slot],
                        cfg->dma.input_stride * cfg->ifmt.height, FX1_DMA_TO_DEVICE);
    uint16_t w, h;
    (void)ifmt_active_size(&cfg->ifmt, &w, &h);
    for (unsigned c = 0; c < 3; ++c) if (cfg->output.channel[c].enable) {
        output_geometry_t g;
        (void)output_compute_geometry(&cfg->output.channel[c], c, w, h, &g);
        const dma_output_config_t *out = &cfg->dma.output[c];
        if (complete) {
            fx1_dma_complete((void *)(uintptr_t)out->y[slot], out->y_stride * g.height, FX1_DMA_FROM_DEVICE);
            if (c < 2) fx1_dma_complete((void *)(uintptr_t)out->uv[slot],
                                       out->uv_stride * (g.height / 2u), FX1_DMA_FROM_DEVICE);
        } else {
            fx1_dma_prepare((void *)(uintptr_t)out->y[slot], out->y_stride * g.height, FX1_DMA_FROM_DEVICE);
            if (c < 2) fx1_dma_prepare((void *)(uintptr_t)out->uv[slot],
                                      out->uv_stride * (g.height / 2u), FX1_DMA_FROM_DEVICE);
        }
    }
}

/* Resident là vùng lưu bảng, không phải bộ hệ số riêng cho một ảnh.
 * Soft reset không xóa active LSC. Đổi nơi nạp sang resident không active,
 * giữ nguyên toàn bộ hệ số/strength; caller vẫn dùng profile logic ban đầu. */
static void map_lsc_residents(lsc_init_config_t *cfg)
{
    lsc_init_config_t source = *cfg;
    unsigned active = (REG_RD(R_LSC_PROFILE_STATUS) &
        FX1_ISP_LSC_PROFILE_STATUS_ACTIVE_PROFILE_MASK) >>
        FX1_ISP_LSC_PROFILE_STATUS_ACTIVE_PROFILE_SHIFT;
    if (active >= 3 || !source.coefficients[active]) return;
    bool used[3] = {false, false, false};
    used[active] = true;
    for (unsigned i = 0; i < 3; ++i)
        if (i != active && source.coefficients[i]) used[i] = true;
    static const unsigned order[] = {1, 2, 0};
    for (unsigned i = 0; i < 3; ++i) {
        unsigned dest = order[i];
        if (used[dest]) continue;
        cfg->coefficients[dest] = source.coefficients[active];
        cfg->coefficient_count[dest] = source.coefficient_count[active];
        cfg->coefficients[active] = NULL;
        cfg->coefficient_count[active] = 0;
        if (cfg->profile == active) cfg->profile = dest;
        return;
    }
}

static void process_pipeline(job_t *job)
{
    pipeline_job_t *p = &job->full;
    ftel_isp_job_result_t *r = &job->result;
    *r = (ftel_isp_job_result_t){.buffers_released = true};
    mask_irq();
    map_lsc_residents(&p->config.lsc);
    isp_wait_t wait = {ftel_isp_platform_time_us, NULL, 2000000u, 10000000u};
    r->status = ftel_isp_fx1_status(isp_init(&p->config, &wait, &p->init));
    if (r->status != FTEL_ISP_OK) goto failed;
    ftel_isp_fx1_session = (ftel_isp_fx1_session_t){0};
    isp_stream_t stream = {0};
    r->status = ftel_isp_fx1_status(isp_stream_start(&stream, &p->config, &wait));
    if (r->status != FTEL_ISP_OK) goto failed;
    service.irq_count = 0;
    for (unsigned i = 0; i < p->count; ++i) {
        ack_irq();
        (void)ulTaskNotifyTake(pdTRUE, 0);
        service.common_irq = service.dma_irq = service.output_irq = service.dma_error = 0;
        cache_slot(p, i, false);
        REG_WR(R_COMMON_IRQ_EN, 3u);
        REG_WR(R_DMA_IRQ_EN, 0x71u);
        REG_WR(FX1_ISP_OUTPUT_IRQ_EN_OFFSET, 0x70u | stream.outputs);
        isp_barrier();
        uint64_t start = ftel_isp_platform_time_us(NULL);
        r->buffers_released = false;
        r->status = ftel_isp_fx1_status(isp_stream_submit(&stream, p->tag[i], p->timestamp[i]));
        if (r->status == FTEL_ISP_OK) {
            r->frame_submitted = true;
            SDK_IRQ_Enable(FX1_IRQ_ISP);
            r->status = wait_frame(start, job->timeout_us, NULL, &stream, &p->frame[i]);
        }
        mask_irq();
        r->irq_count = service.irq_count;
        r->common_irq |= service.common_irq;
        r->dma_irq |= service.dma_irq;
        r->output_irq |= service.output_irq;
        r->dma_errors |= service.dma_error | REG_RD(R_DMA_ERR);
        if (r->status != FTEL_ISP_OK) goto failed;
        for (unsigned c = 0; c < 3; ++c) if (stream.outputs & (1u << c)) {
            if (p->frame[i].tag[c] != p->tag[i] || p->frame[i].timestamp[c] != p->timestamp[i]) {
                r->status = FTEL_ISP_E_STALE;
                goto failed;
            }
        }
        cache_slot(p, i, true);
        r->status = ftel_isp_fx1_status(isp_stream_release(&stream));
        if (r->status != FTEL_ISP_OK) goto failed;
        ++p->completed;
    }
    r->status = ftel_isp_fx1_status(isp_stream_stop(&stream, 100000u));
    if (r->status == FTEL_ISP_OK) { r->buffers_released = true; return; }
failed:
    /* Không trả quyền pixel khi DMA chưa drain, kể cả submit lỗi một phần. */
    r->recovery_status = recover_hardware();
    r->recovered = r->recovery_status == FTEL_ISP_OK;
    r->buffers_released = r->recovered;
}

static void worker(void *unused)
{
    (void)unused;
    /* start() claimed before allocating the worker. Only this task may enter. */
    service.startup_status = NEON_FX1_IRQ_Register(FX1_IRQ_ISP,
        service.config.irq_priority, isp_isr, NULL) == pdPASS ? FTEL_ISP_OK : FTEL_ISP_E_CONFIG;
    if (service.startup_status == FTEL_ISP_OK) mask_irq();
    else (void)ftel_isp_fx1_set_owner(NULL, NULL);
    xSemaphoreGive(service.startup);
    if (service.startup_status != FTEL_ISP_OK) vTaskDelete(NULL);
    for (;;) {
        uint32_t index;
        configASSERT(xQueueReceive(service.queue, &index, portMAX_DELAY) == pdTRUE);
        if (index == RECOVER_COMMAND) {
            ftel_isp_status_t status = recover_hardware();
            lock();
            service.recovery_status = status;
            service.recovery_pending = false;
            service.faulted = status != FTEL_ISP_OK;
            if (status == FTEL_ISP_OK) {
                for (unsigned i = 0; i < FTEL_ISP_FREERTOS_MAX_JOBS; ++i) {
                    job_t *j = &service.jobs[i];
                    if (j->state == JOB_DONE && !j->result.buffers_released) {
                        j->result.buffers_released = true;
                        j->result.recovered = true;
                        j->result.recovery_status = FTEL_ISP_OK;
                    }
                }
            }
            unlock();
            xSemaphoreGive(service.recovery_done);
            continue;
        }
        configASSERT(index < FTEL_ISP_FREERTOS_MAX_JOBS);
        job_t *j = &service.jobs[index];
        lock();
        j->state = JOB_ACTIVE;
        bool faulted = service.faulted;
        unlock();
        if (faulted) {
            j->result = (ftel_isp_job_result_t){.status = FTEL_ISP_E_STATE, .buffers_released = true};
        } else if (j->pipeline) process_pipeline(j);
        else process_job(j);
        lock();
        if (!j->result.buffers_released) service.faulted = true;
        j->state = JOB_DONE;
        unlock();
        xSemaphoreGive(j->done);
    }
}

void ftel_isp_freertos_get_default_config(ftel_isp_freertos_config_t *c)
{
    if (c) *c = (ftel_isp_freertos_config_t){2048, 2, 1, 20000};
}

ftel_isp_status_t ftel_isp_freertos_start(const ftel_isp_freertos_config_t *config)
{
    ftel_isp_freertos_config_t c;
    if (config) c = *config; else ftel_isp_freertos_get_default_config(&c);
    if (fx1_hartid() != 0 || xTaskGetSchedulerState() != taskSCHEDULER_RUNNING)
        return FTEL_ISP_E_STATE;
    if (c.stack_words < 512 || !c.task_priority || c.task_priority >= configMAX_PRIORITIES ||
        !c.irq_priority || c.irq_priority > FX1_PLIC_PRIORITY_MAX || !c.recovery_timeout_us)
        return FTEL_ISP_E_CONFIG;
    taskENTER_CRITICAL();
    bool started = service.state != 0;
    if (!started) service.state = 1;
    taskEXIT_CRITICAL();
    if (started) return FTEL_ISP_E_BUSY;
    ftel_isp_info_t info;
    ftel_isp_status_t status = ftel_isp_get_info(&info);
    if (status == FTEL_ISP_OK && (info.busy || aec_lease_valid())) status = FTEL_ISP_E_BUSY;
    if (status != FTEL_ISP_OK) { service.state = 0; return status; }
    /* Atomic claim closes the preflight race with camera sessions. A rejection
     * leaves the service retryable and does not allocate a worker or touch IRQ. */
    if (!ftel_isp_fx1_set_owner(worker_owns_isp, NULL)) {
        service.state = 0;
        return FTEL_ISP_E_BUSY;
    }
    service.config = c;
    service.lock = xSemaphoreCreateMutex();
    service.queue = xQueueCreate(FTEL_ISP_FREERTOS_MAX_JOBS + 1u, sizeof(uint32_t));
    service.startup = xSemaphoreCreateBinary();
    service.recovery_done = xSemaphoreCreateBinary();
    configASSERT(service.lock && service.queue && service.startup && service.recovery_done);
    for (unsigned i = 0; i < FTEL_ISP_FREERTOS_MAX_JOBS; ++i) {
        service.jobs[i].done = xSemaphoreCreateBinary();
        configASSERT(service.jobs[i].done);
    }
    configASSERT(xTaskCreate(worker, "isp_worker", c.stack_words, NULL,
                            c.task_priority, &service.worker) == pdPASS);
    configASSERT(xSemaphoreTake(service.startup, portMAX_DELAY) == pdTRUE);
    service.state = service.startup_status == FTEL_ISP_OK ? 2u : 3u;
    return service.startup_status;
}

static bool span_valid(const ftel_isp_buffer_t *b)
{
    uint64_t end = (uint64_t)b->address + b->bytes;
    return b->bytes && b->stride && !(b->address & 15u) && !(b->stride & 15u) &&
           b->address >= FX1_FW_RAW_BUF_BASE && end <= (uint64_t)FX1_DDR_BASE + FX1_DDR_SIZE;
}
static bool overlap(const ftel_isp_buffer_t *a, const ftel_isp_buffer_t *b)
{
    return a->address < (uint64_t)b->address + b->bytes &&
           b->address < (uint64_t)a->address + a->bytes;
}
static bool spans_overlap(const ftel_isp_buffer_t *span, unsigned count, const job_t *job)
{
    for (unsigned i = 0; i < count; ++i)
        for (unsigned j = 0; j < job->span_count; ++j)
            if (overlap(&span[i], &job->span[j])) return true;
    return false;
}

/* lock đã được giữ; ticket DONE vẫn giữ toàn bộ vùng nhớ tới release. */
static job_t *reserve_job(const ftel_isp_buffer_t *span, unsigned count, unsigned *index)
{
    job_t *free_job = NULL;
    for (unsigned i = 0; i < FTEL_ISP_FREERTOS_MAX_JOBS; ++i) {
        job_t *j = &service.jobs[i];
        if (j->state == JOB_FREE) { free_job = j; *index = i; }
        else if (spans_overlap(span, count, j)) return NULL;
    }
    if (free_job) {
        free_job->span_count = count;
        for (unsigned i = 0; i < count; ++i) free_job->span[i] = span[i];
    }
    return free_job;
}

static ftel_isp_status_t enqueue_job(job_t *j, unsigned index, uint32_t timeout,
                                    ftel_isp_ticket_t *ticket)
{
    j->timeout_us = timeout;
    j->caller = xTaskGetCurrentTaskHandle();
    if (++j->generation == 0) ++j->generation;
    j->state = JOB_QUEUED;
    (void)xSemaphoreTake(j->done, 0);
    uint32_t command = index;
    if (xQueueSend(service.queue, &command, 0) != pdTRUE) {
        j->state = JOB_FREE;
        return FTEL_ISP_E_BUSY;
    }
    *ticket = (ftel_isp_ticket_t){index, j->generation};
    return FTEL_ISP_OK;
}

ftel_isp_status_t ftel_isp_freertos_submit(const ftel_isp_config_t *config,
                                         const ftel_isp_frame_t *frame,
                                         uint32_t timeout, ftel_isp_ticket_t *ticket)
{
    if (fx1_hartid() != 0 || service.state != 2) return FTEL_ISP_E_STATE;
    if (!config || !config->aec.enable || !frame || !ticket || !timeout || !span_valid(&frame->raw) ||
        !span_valid(&frame->y) || !span_valid(&frame->uv) ||
        overlap(&frame->raw, &frame->y) || overlap(&frame->raw, &frame->uv) ||
        overlap(&frame->y, &frame->uv)) return FTEL_ISP_E_CONFIG;
    lock();
    ftel_isp_status_t status = FTEL_ISP_E_BUSY;
    if (service.faulted) { status = FTEL_ISP_E_STATE; goto out; }
    const ftel_isp_buffer_t span[] = {frame->raw, frame->y, frame->uv};
    unsigned index;
    job_t *j = reserve_job(span, 3, &index);
    if (!j) goto out;
    j->pipeline = false;
    j->config = *config; j->frame = *frame;
    status = enqueue_job(j, index, timeout, ticket);
out:
    unlock();
    return status;
}

static ftel_isp_status_t find_job(ftel_isp_ticket_t t, job_t **job)
{
    if (t.slot >= FTEL_ISP_FREERTOS_MAX_JOBS || !t.generation) return FTEL_ISP_E_STALE;
    job_t *j = &service.jobs[t.slot];
    if (j->state == JOB_FREE || j->generation != t.generation) return FTEL_ISP_E_STALE;
    if (j->caller != xTaskGetCurrentTaskHandle()) return FTEL_ISP_E_BUSY;
    *job = j;
    return FTEL_ISP_OK;
}

static ftel_isp_status_t wait_job(ftel_isp_ticket_t t, TickType_t ticks, bool pipeline,
                                  ftel_isp_job_result_t *basic,
                                  ftel_isp_pipeline_result_t *full)
{
    if (fx1_hartid() != 0 || service.state != 2) return FTEL_ISP_E_STATE;
    if ((!pipeline && !basic) || (pipeline && !full)) return FTEL_ISP_E_CONFIG;
    lock();
    job_t *j;
    ftel_isp_status_t status = find_job(t, &j);
    if (status == FTEL_ISP_OK && j->pipeline != pipeline) status = FTEL_ISP_E_STATE;
    if (status != FTEL_ISP_OK) { unlock(); return status; }
    bool done = j->state == JOB_DONE;
    unlock();
    if (!done) (void)xSemaphoreTake(j->done, ticks);
    lock();
    status = find_job(t, &j);
    if (status == FTEL_ISP_OK) {
        if (j->state != JOB_DONE) status = FTEL_ISP_E_TIMEOUT;
        else if (!pipeline) *basic = j->result;
        else {
            full->job = j->result;
            full->init = j->full.init;
            full->completed_frames = j->full.completed;
            for (unsigned i = 0; i < FTEL_ISP_FREERTOS_MAX_FRAMES; ++i)
                full->frame[i] = j->full.frame[i];
        }
    }
    unlock();
    return status;
}

ftel_isp_status_t ftel_isp_freertos_wait(ftel_isp_ticket_t t, TickType_t ticks,
                                       ftel_isp_job_result_t *result)
{
    return wait_job(t, ticks, false, result, NULL);
}

ftel_isp_status_t ftel_isp_freertos_wait_pipeline(ftel_isp_ticket_t t, TickType_t ticks,
                                                ftel_isp_pipeline_result_t *result)
{
    return wait_job(t, ticks, true, NULL, result);
}

/* Từ chối địa chỉ ngoài DDR/padding chồng lấn trước khi queue hoặc ghi MMIO. */
static bool add_span(ftel_isp_buffer_t *span, unsigned *count,
                      uint64_t address, uint32_t stride, uint32_t rows)
{
    uint64_t bytes = (uint64_t)stride * rows;
    if (!address || address > UINTPTR_MAX || !bytes || bytes > UINT32_MAX) return false;
    ftel_isp_buffer_t b = {(uintptr_t)address, stride, (uint32_t)bytes};
    if (!span_valid(&b)) return false;
    for (unsigned i = 0; i < *count; ++i) if (overlap(&b, &span[i])) return false;
    if (*count >= MAX_SPANS) return false;
    span[(*count)++] = b;
    return true;
}

ftel_isp_status_t ftel_isp_freertos_submit_pipeline(const ftel_isp_pipeline_request_t *request,
                                                  ftel_isp_ticket_t *ticket)
{
    if (fx1_hartid() != 0 || service.state != 2) return FTEL_ISP_E_STATE;
    if (!request || !ticket || !request->config || !request->frame_timeout_us ||
        !request->frame_count || request->frame_count > FTEL_ISP_FREERTOS_MAX_FRAMES)
        return FTEL_ISP_E_CONFIG;
    const isp_config_t *cfg = request->config;
    if (isp_validate_config(cfg) != ISP_OK) return FTEL_ISP_E_CONFIG;
    unsigned lsc_tables = 0;
    for (unsigned i = 0; i < 3; ++i) if (cfg->lsc.coefficients[i]) ++lsc_tables;
    /* Luôn cần ít nhất một resident trống để tránh ghi đè bảng active. */
    if (lsc_tables > 2) return FTEL_ISP_E_UNSUPPORTED;
    ftel_isp_buffer_t span[MAX_SPANS];
    unsigned count = 0;
    for (unsigned i = 0; i < 4; ++i)
        if (!add_span(span, &count, cfg->dma.input[i], cfg->dma.input_stride, cfg->ifmt.height))
            return FTEL_ISP_E_CONFIG;
    uint16_t w, h;
    (void)ifmt_active_size(&cfg->ifmt, &w, &h);
    unsigned outputs = 0;
    for (unsigned c = 0; c < 3; ++c) if (cfg->output.channel[c].enable) {
        ++outputs;
        output_geometry_t g;
        (void)output_compute_geometry(&cfg->output.channel[c], c, w, h, &g);
        for (unsigned i = 0; i < 4; ++i) {
            if (!add_span(span, &count, cfg->dma.output[c].y[i], cfg->dma.output[c].y_stride, g.height) ||
                (c < 2 && !add_span(span, &count, cfg->dma.output[c].uv[i],
                                   cfg->dma.output[c].uv_stride, g.height / 2u)))
                return FTEL_ISP_E_CONFIG;
        }
    }
    if (!outputs) return FTEL_ISP_E_CONFIG;
    lock();
    ftel_isp_status_t status = FTEL_ISP_E_BUSY;
    if (service.faulted) { status = FTEL_ISP_E_STATE; goto out; }
    unsigned index;
    job_t *j = reserve_job(span, count, &index);
    if (!j) goto out;
    j->pipeline = true;
    j->full = (pipeline_job_t){.config = *cfg, .count = request->frame_count};
    for (unsigned i = 0; i < request->frame_count; ++i) {
        j->full.tag[i] = request->tag[i];
        j->full.timestamp[i] = request->timestamp[i];
    }
    status = enqueue_job(j, index, request->frame_timeout_us, ticket);
out:
    unlock();
    return status;
}

ftel_isp_status_t ftel_isp_freertos_release(ftel_isp_ticket_t t)
{
    if (fx1_hartid() != 0 || service.state != 2) return FTEL_ISP_E_STATE;
    lock();
    job_t *j;
    ftel_isp_status_t status = find_job(t, &j);
    if (status == FTEL_ISP_OK) {
        if (j->state != JOB_DONE || !j->result.buffers_released) status = FTEL_ISP_E_BUSY;
        else { j->state = JOB_FREE; j->caller = NULL; }
    }
    unlock();
    return status;
}

ftel_isp_status_t ftel_isp_freertos_recover(TickType_t ticks)
{
    if (fx1_hartid() != 0 || service.state != 2) return FTEL_ISP_E_STATE;
    lock();
    if (!service.faulted) { unlock(); return FTEL_ISP_OK; }
    if (service.recovery_pending) { unlock(); return FTEL_ISP_E_BUSY; }
    service.recovery_pending = true;
    (void)xSemaphoreTake(service.recovery_done, 0);
    uint32_t command = RECOVER_COMMAND;
    configASSERT(xQueueSend(service.queue, &command, 0) == pdTRUE);
    unlock();
    if (xSemaphoreTake(service.recovery_done, ticks) != pdTRUE) return FTEL_ISP_E_TIMEOUT;
    lock();
    ftel_isp_status_t status = service.recovery_status;
    unlock();
    return status;
}
