#include "neon_camera_freertos.h"
#include "neon_freertos_fx1.h"
#include "fx1_fw.h"
#include "fx1/fx1_memory_map.h"
#if configUSE_TASK_NOTIFICATIONS != 1 || configSUPPORT_DYNAMIC_ALLOCATION != 1
#error "Camera service requires task notifications and startup allocation"
#endif
static uint32_t read_reg(void *user, uint32_t offset)
{
    (void)user;
    return *(volatile uint32_t *)(uintptr_t)(FX1_ISP_CSR_BASE + offset);
}
static void write_reg(void *user, uint32_t offset, uint32_t value)
{
    (void)user;
    *(volatile uint32_t *)(uintptr_t)(FX1_ISP_CSR_BASE + offset) = value;
}
static void barrier(void *user) { (void)user; fx1_mb(); }
static void prepare(void *user, uint32_t a, uint32_t n, bool output)
{
    (void)user;
    fx1_dma_prepare((void *)(uintptr_t)a, n, output ? FX1_DMA_FROM_DEVICE : FX1_DMA_TO_DEVICE);
}
static void complete(void *user, uint32_t a, uint32_t n)
{
    (void)user;
    fx1_dma_complete((void *)(uintptr_t)a, n, FX1_DMA_FROM_DEVICE);
}
static BaseType_t camera_isr(void *user)
{
    neon_camera_freertos_t *s = user;
    BaseType_t woken = pdFALSE;
    fx1_isp_irq_gate(&s->backend.port.io);
    vTaskNotifyGiveFromISR(s->worker, &woken);
    return woken;
}
static bool attach(void *user)
{
    neon_camera_freertos_t *s = user;
    return NEON_FX1_IRQ_Register(FX1_IRQ_ISP, s->config.irq_priority, camera_isr, s) == pdPASS;
}
static void detach(void *user)
{
    configASSERT(NEON_FX1_IRQ_Unregister(FX1_IRQ_ISP, camera_isr, user) == pdPASS);
}
static void execute(neon_camera_freertos_t *s, uint8_t index)
{
    neon_camera_command_t *c = &s->requests[index].command;
    neon_camera_reply_t reply = {0};
    if (c->kind != NEON_CAMERA_CMD_START && c->kind != NEON_CAMERA_CMD_STATUS &&
        c->generation != s->session.status.generation) reply.result = NEON_CAMERA_STALE;
    else switch (c->kind) {
    case NEON_CAMERA_CMD_START:
        reply.result = neon_camera_session_start(&s->session);
        if (reply.result == NEON_CAMERA_PENDING) {
            s->storm = s->timed_out = s->deadline_active = false; s->unproductive = 0;
        }
        break;
    case NEON_CAMERA_CMD_SUBMIT: reply.result = neon_camera_session_submit(&s->session, c->tag); break;
    case NEON_CAMERA_CMD_APPLY:
        reply.result = neon_camera_session_apply(&s->session, &c->transaction, s->session.status.hardware.next_frame); break;
    case NEON_CAMERA_CMD_ACQUIRE: reply.result = neon_camera_session_acquire(&s->session, &reply.frame); break;
    case NEON_CAMERA_CMD_RETURN: reply.result = neon_camera_session_return(&s->session, &c->frame); break;
    case NEON_CAMERA_CMD_STATUS: reply.result = NEON_CAMERA_OK; break;
    default: reply.result = NEON_CAMERA_INVALID; break;
    }
    reply.device_version = s->backend.version; reply.device_capabilities = s->backend.capabilities;
    reply.tune_id = s->backend.tune_id;
    (void)neon_camera_session_status(&s->session, &reply.status);
    taskENTER_CRITICAL();
    s->requests[index].reply = reply;
    s->requests[index].state = 2;
    s->cached_status = reply.status;
    taskEXIT_CRITICAL();
}
static bool deadline_expired(TickType_t now, TickType_t deadline)
{
    const TickType_t half = (TickType_t)(portMAX_DELAY / 2u + 1u);
    return (TickType_t)(now - deadline) < half;
}
/* One bounded owner turn is also exercised by the host fake port. */
static bool worker_turn(neon_camera_freertos_t *s)
{
    bool stop; uint64_t generation;
    taskENTER_CRITICAL();
    stop = s->stop_pending; generation = s->stop_generation; s->stop_pending = false;
    taskEXIT_CRITICAL();
    if (stop && generation == s->session.status.generation) (void)neon_camera_session_stop(&s->session);
    uint8_t index;
    for (unsigned n = 0; n < 2; ++n)
        if (xQueueReceive(s->commands, &index, 0) == pdTRUE) execute(s, index);
        else break;
    uint32_t before = s->session.status.retired;
    for (unsigned n = 0; n < 4 && s->session.status.claimed; ++n) {
        neon_camera_result_t r = neon_camera_session_poll(&s->session);
        if (r != NEON_CAMERA_OK) break;
    }
    bool pending = s->session.status.claimed && (s->session.in_flight || s->session.status.profile_pending || s->session.stop_requested);
    TickType_t now = xTaskGetTickCount();
    if (pending && !s->deadline_active && !s->timed_out) {
        s->deadline = now + s->config.timeout_ticks; s->deadline_active = true;
    }
    if (s->deadline_active && !s->timed_out && deadline_expired(now, s->deadline)) {
        (void)neon_camera_session_timeout(&s->session); s->timed_out = true;
    }
    if (!pending) s->deadline_active = false;
    bool immediate = false;
    if (s->session.status.claimed && s->backend.attached) {
        taskENTER_CRITICAL();
        if (s->session.status.state != NEON_CAMERA_FAULTED && !s->storm)
            immediate = fx1_isp_irq_rearm(&s->backend.port.io, fx1_isp_irq_replay_masks(s->backend.capabilities));
        else fx1_isp_irq_gate(&s->backend.port.io);
        taskEXIT_CRITICAL();
    }
    if (immediate && before == s->session.status.retired) ++s->unproductive;
    else s->unproductive = 0;
    if (s->unproductive >= s->config.storm_budget) {
        fx1_isp_irq_gate(&s->backend.port.io); s->storm = true;
        (void)neon_camera_session_timeout(&s->session); s->timed_out = true;
        immediate = false;
    }
    taskENTER_CRITICAL(); s->cached_status = s->session.status; taskEXIT_CRITICAL();
    return immediate;
}
static void worker(void *user)
{
    neon_camera_freertos_t *s = user;
    for (;;) {
        bool immediate = worker_turn(s);
        /* Timed polling also reconciles quarantine/recovery with no IRQ. A
         * notification arriving between recheck and wait remains pending. Never
         * clear notifications after rearm except by this atomic wait operation. */
        if (immediate) { vTaskDelay(1); continue; }
        TickType_t wait = s->config.poll_ticks;
        if (s->deadline_active && !s->timed_out) {
            TickType_t now = xTaskGetTickCount();
            TickType_t remaining = deadline_expired(now, s->deadline) ? 0 : s->deadline - now;
            if (remaining < wait) wait = remaining;
        }
        (void)ulTaskNotifyTake(pdTRUE, wait);
    }
}
neon_camera_result_t neon_camera_freertos_create(neon_camera_freertos_t *s, const neon_camera_freertos_config_t *c)
{
    if (!s || !c || s->worker || s->commands || fx1_hartid() != 0 || !c->poll_ticks || !c->timeout_ticks ||
        c->timeout_ticks > portMAX_DELAY / 2u || c->poll_ticks > c->timeout_ticks ||
        !c->storm_budget || !c->stack_words || !c->irq_priority || c->irq_priority > FX1_PLIC_PRIORITY_MAX ||
        c->worker_priority >= configMAX_PRIORITIES) return NEON_CAMERA_INVALID;
    s->config = *c;
    fx1_isp_backend_port_t p = {{s, read_reg, write_reg, barrier}, prepare, complete, attach, detach};
    neon_camera_result_t r = fx1_isp_backend_init(&s->backend, &p);
    if (r != NEON_CAMERA_OK) return r;
    s->config.session.backend = fx1_isp_backend_ops(); s->config.session.device = &s->backend;
    r = neon_camera_session_init(&s->session, &s->config.session);
    if (r != NEON_CAMERA_OK) return r;
    s->commands = xQueueCreate(NEON_CAMERA_SERVICE_REQUESTS, sizeof(uint8_t));
    if (!s->commands) return NEON_CAMERA_BUSY;
    /* Worker cannot execute START until creation returns and client enqueues it.
     * No IRQ registration or MMIO occurs until that worker claims the device. */
    if (xTaskCreate(worker, "camera", c->stack_words, s, c->worker_priority, &s->worker) != pdPASS) {
        vQueueDelete(s->commands); s->commands = NULL; return NEON_CAMERA_BUSY;
    }
    s->cached_status = s->session.status;
    return NEON_CAMERA_OK;
}
neon_camera_result_t neon_camera_freertos_enqueue(neon_camera_freertos_t *s, const neon_camera_command_t *c, neon_camera_ticket_t *ticket)
{
    if (!s || !c || !ticket || !s->worker || fx1_hartid() != 0) return NEON_CAMERA_INVALID;
    taskENTER_CRITICAL();
    if (s->serial == UINT64_MAX) { taskEXIT_CRITICAL(); return NEON_CAMERA_INVALID; }
    for (uint8_t i = 0; i < NEON_CAMERA_SERVICE_REQUESTS; ++i) if (!s->requests[i].state) {
        s->requests[i].state = 1; s->requests[i].serial = ++s->serial;
        s->requests[i].command = *c;
        if (xQueueSend(s->commands, &i, 0) != pdTRUE) {
            s->requests[i].state = 0; taskEXIT_CRITICAL(); return NEON_CAMERA_BUSY;
        }
        *ticket = (neon_camera_ticket_t){s->serial, i};
        taskEXIT_CRITICAL(); xTaskNotifyGive(s->worker); return NEON_CAMERA_OK;
    }
    taskEXIT_CRITICAL(); return NEON_CAMERA_BUSY;
}
neon_camera_result_t neon_camera_freertos_collect(neon_camera_freertos_t *s, neon_camera_ticket_t t, neon_camera_reply_t *r)
{
    if (!s || !r || t.index >= NEON_CAMERA_SERVICE_REQUESTS) return NEON_CAMERA_INVALID;
    taskENTER_CRITICAL();
    if (!s->requests[t.index].state || s->requests[t.index].serial != t.serial) {
        taskEXIT_CRITICAL(); return NEON_CAMERA_STALE;
    }
    if (s->requests[t.index].state != 2) { taskEXIT_CRITICAL(); return NEON_CAMERA_NO_DATA; }
    *r = s->requests[t.index].reply; s->requests[t.index].state = 0;
    taskEXIT_CRITICAL(); return NEON_CAMERA_OK;
}
neon_camera_result_t neon_camera_freertos_stop(neon_camera_freertos_t *s, uint64_t generation)
{
    if (!s || !s->worker) return NEON_CAMERA_INVALID;
    taskENTER_CRITICAL();
    if (generation != s->cached_status.generation) { taskEXIT_CRITICAL(); return NEON_CAMERA_STALE; }
    s->stop_generation = generation; s->stop_pending = true;
    taskEXIT_CRITICAL(); xTaskNotifyGive(s->worker); return NEON_CAMERA_PENDING;
}
void neon_camera_freertos_status(neon_camera_freertos_t *s, neon_camera_status_t *out)
{
    if (!s || !out) return;
    taskENTER_CRITICAL(); *out = s->cached_status; taskEXIT_CRITICAL();
}
