/* Real service owner turns + real register backend; deterministic fake kernel,
 * PLIC lifecycle and MMIO. No claim that this proves RTOS/PLIC hardware timing. */
#include "test_common.h"
#include "isp/neon_isp_claim.h"
#include "host/isp_runtime_mmio.h"
#include "../../rtos/services/camera/neon_camera_freertos.c"
#include <stdlib.h>
typedef struct { uint8_t data[8]; unsigned count, head, tail; } fake_queue_t;
static unsigned critical, notifications, registrations, unregistrations;
static TickType_t ticks;
static NEON_FX1_IRQ_Handler handler;
static void *handler_user;
static bool fail_register, fail_task, fail_queue, registration_interrupt;
static isp_mmio_t hw;
void fake_enter(void) { ++critical; }
void fake_exit(void) { CHECK(critical != 0); --critical; }
QueueHandle_t xQueueCreate(unsigned n, size_t size)
{ CHECK_EQ(n,8); CHECK_EQ(size,1); return fail_queue ? NULL : calloc(1,sizeof(fake_queue_t)); }
void vQueueDelete(QueueHandle_t q) { free(q); }
BaseType_t xQueueSend(QueueHandle_t q, const void *p, TickType_t t)
{
    fake_queue_t *f=q; (void)t;
    if (f->count==8) return pdFALSE;
    f->data[f->tail++ % 8]=*(const uint8_t *)p; ++f->count; return pdTRUE;
}
BaseType_t xQueueReceive(QueueHandle_t q, void *p, TickType_t t)
{
    fake_queue_t *f=q; (void)t;
    if (!f->count) return pdFALSE;
    *(uint8_t *)p=f->data[f->head++ % 8]; --f->count; return pdTRUE;
}
BaseType_t xTaskCreate(void (*entry)(void *), const char *name, uint32_t stack, void *user, uint32_t priority, TaskHandle_t *task)
{ (void)entry;(void)name;(void)stack;(void)priority; if (fail_task) return pdFAIL; *task=user; return pdPASS; }
void vTaskDelay(TickType_t n) { ticks+=n; }
TickType_t xTaskGetTickCount(void) { return ticks; }
void vTaskNotifyGiveFromISR(TaskHandle_t task, BaseType_t *woken)
{ CHECK(task != NULL); ++notifications; *woken=pdTRUE; }
void xTaskNotifyGive(TaskHandle_t task) { CHECK(task != NULL); ++notifications; }
uint32_t ulTaskNotifyTake(BaseType_t clear, TickType_t wait)
{
    CHECK(clear); unsigned n=notifications; notifications=0;
    if (!n) ticks+=wait;
    return n;
}
BaseType_t NEON_FX1_IRQ_Register(uint32_t source, uint32_t priority, NEON_FX1_IRQ_Handler h, void *user)
{
    CHECK_EQ(source,FX1_IRQ_ISP); CHECK(priority != 0); CHECK(!handler);
    CHECK_EQ(hw.reg[R(COMMON_IRQ_EN)/4] | hw.reg[R(DMA_IRQ_EN)/4] | hw.reg[R(OUTPUT_IRQ_EN)/4],0);
    if (fail_register) return pdFAIL;
    handler=h; handler_user=user; ++registrations;
    if (registration_interrupt) CHECK_EQ(handler(handler_user),pdTRUE);
    return pdPASS;
}
BaseType_t NEON_FX1_IRQ_Unregister(uint32_t source, NEON_FX1_IRQ_Handler h, void *user)
{
    CHECK_EQ(source,FX1_IRQ_ISP); CHECK(handler==h); CHECK(handler_user==user);
    CHECK_EQ(hw.reg[R(COMMON_IRQ_EN)/4] | hw.reg[R(DMA_IRQ_EN)/4] | hw.reg[R(OUTPUT_IRQ_EN)/4],0);
    CHECK(!neon_isp_device_claim(&hw)); /* detach occurs before shared release */
    handler=NULL; handler_user=NULL; ++unregistrations; return pdPASS;
}
static uint32_t service_read(void *u,uint32_t r) { (void)u; return mmio_read(&hw,r); }
static void service_write(void *u,uint32_t r,uint32_t v) { (void)u; mmio_write(&hw,r,v); }
static void service_prepare(void *u,uint32_t a,uint32_t n,bool o) { (void)u; mmio_prepare(&hw,a,n,o); }
static void service_complete(void *u,uint32_t a,uint32_t n) { (void)u; mmio_complete(&hw,a,n); }
static const fx1_isp_scalar_profile_t scalar=FX1_ISP_SCALAR_PROFILE_INITIALIZER;
static neon_camera_freertos_config_t configuration(void)
{
    neon_camera_freertos_config_t c={.irq_priority=2,.worker_priority=3,.stack_words=512,.poll_ticks=1,.timeout_ticks=20,.storm_budget=3,
        .session={.baseline={&scalar,sizeof(scalar),7},.stream={.width=64,.height=48,.raw_stride=128,.y_stride=64,.uv_stride=64,.raw_depth=12,.dma_alignment=16}}};
    for (unsigned i=0;i<4;++i) {
        uint32_t a=0x100000u+i*0x10000u;
        c.session.stream.buffers[i]=(neon_isp_session_buffer_t){{a,6144},{a+0x2000u,3072},{a+0x4000u,1536}};
    }
    return c;
}
static void setup(neon_camera_freertos_t *s)
{
    ticks=0; notifications=0; mmio_init(&hw);
    neon_camera_freertos_config_t c=configuration();
    CHECK_EQ(neon_camera_freertos_create(s,&c),NEON_CAMERA_OK);
    s->backend.port.io=(fx1_isp_io_t){s,service_read,service_write,mmio_barrier};
    s->backend.port.prepare=service_prepare; s->backend.port.complete=service_complete;
    neon_camera_ticket_t ticket; neon_camera_reply_t reply;
    CHECK_EQ(neon_camera_freertos_enqueue(s,&(neon_camera_command_t){.kind=NEON_CAMERA_CMD_START},&ticket),NEON_CAMERA_OK);
    CHECK_EQ(neon_camera_freertos_collect(s,ticket,&reply),NEON_CAMERA_NO_DATA);
    (void)worker_turn(s);
    CHECK_EQ(neon_camera_freertos_collect(s,ticket,&reply),NEON_CAMERA_OK);
    CHECK_EQ(reply.result,NEON_CAMERA_PENDING);
    mmio_recovery_done(&hw,0);
    for (unsigned n=0;n<9;++n) (void)worker_turn(s);
    hw.reset_requests=0;
    CHECK_EQ(s->session.status.state,NEON_CAMERA_STARTING);
}
static neon_camera_reply_t command(neon_camera_freertos_t *s,neon_camera_command_t c)
{
    neon_camera_ticket_t t; neon_camera_reply_t reply;
    CHECK_EQ(neon_camera_freertos_enqueue(s,&c,&t),NEON_CAMERA_OK);
    (void)worker_turn(s);
    CHECK_EQ(neon_camera_freertos_collect(s,t,&reply),NEON_CAMERA_OK);
    CHECK_EQ(neon_camera_freertos_collect(s,t,&reply),NEON_CAMERA_STALE);
    return reply;
}
static void shutdown_service(neon_camera_freertos_t *s)
{
    CHECK_EQ(neon_camera_freertos_stop(s,s->cached_status.generation),NEON_CAMERA_PENDING);
    for (unsigned n=0;n<4;++n) {
        (void)worker_turn(s); mmio_recovery_done(&hw,0); (void)worker_turn(s);
    }
    CHECK(!s->session.status.claimed); CHECK(!handler);
    vQueueDelete(s->commands);
}
static void queue_saturation_stop_and_stale(void)
{
    neon_camera_freertos_t s={0}; setup(&s);
    neon_camera_ticket_t tickets[8],extra; neon_camera_reply_t reply;
    for (unsigned n=0;n<8;++n) CHECK_EQ(neon_camera_freertos_enqueue(&s,&(neon_camera_command_t){.kind=NEON_CAMERA_CMD_STATUS},&tickets[n]),NEON_CAMERA_OK);
    CHECK_EQ(neon_camera_freertos_enqueue(&s,&(neon_camera_command_t){.kind=NEON_CAMERA_CMD_RETURN},&extra),NEON_CAMERA_BUSY);
    CHECK_EQ(neon_camera_freertos_stop(&s,99),NEON_CAMERA_STALE);
    CHECK_EQ(neon_camera_freertos_stop(&s,1),NEON_CAMERA_PENDING);
    (void)worker_turn(&s); CHECK(s.session.stop_requested); CHECK_EQ(hw.reset_requests,1);
    mmio_recovery_done(&hw,0); (void)worker_turn(&s); CHECK(!s.session.status.claimed);
    for (unsigned n=0;n<4;++n) (void)worker_turn(&s);
    for (unsigned n=0;n<8;++n) CHECK_EQ(neon_camera_freertos_collect(&s,tickets[n],&reply),NEON_CAMERA_OK);
    reply=command(&s,(neon_camera_command_t){.kind=NEON_CAMERA_CMD_START}); CHECK_EQ(reply.result,NEON_CAMERA_PENDING);
    reply=command(&s,(neon_camera_command_t){.kind=NEON_CAMERA_CMD_SUBMIT,.generation=1}); CHECK_EQ(reply.result,NEON_CAMERA_STALE);
    CHECK_EQ(s.session.status.generation,2); shutdown_service(&s);
}
static void lost_wakeup_coalescing_and_consumer_stop(void)
{
    neon_camera_freertos_t s={0}; registration_interrupt=true; setup(&s); registration_interrupt=false;
    CHECK(notifications>0); CHECK_EQ(ulTaskNotifyTake(pdTRUE,10)>0,true); CHECK_EQ(ticks,0);
    neon_camera_reply_t r=command(&s,(neon_camera_command_t){.kind=NEON_CAMERA_CMD_SUBMIT,.generation=1,.tag=42});
    CHECK_EQ(r.result,NEON_CAMERA_OK); mmio_admit(&hw); (void)worker_turn(&s);
    mmio_retire(&hw,0,1,7); CHECK(handler); CHECK_EQ(handler(handler_user),pdTRUE);
    CHECK_EQ(hw.ack_output,0); /* ISR masked but did not ACK */
    for (unsigned n=0;n<10;++n) xTaskNotifyGive(s.worker);
    (void)worker_turn(&s); CHECK_EQ(s.session.status.retired,1);
    CHECK_EQ(ulTaskNotifyTake(pdTRUE,10)>0,true); CHECK_EQ(ticks,0);
    (void)worker_turn(&s); CHECK_EQ(s.session.status.retired,1);
    r=command(&s,(neon_camera_command_t){.kind=NEON_CAMERA_CMD_ACQUIRE,.generation=1}); CHECK_EQ(r.result,NEON_CAMERA_OK);
    neon_camera_frame_t frame=r.frame;
    CHECK_EQ(neon_camera_freertos_stop(&s,1),NEON_CAMERA_PENDING); (void)worker_turn(&s);
    mmio_recovery_done(&hw,0); (void)worker_turn(&s); CHECK(s.session.status.claimed); CHECK(handler);
    r=command(&s,(neon_camera_command_t){.kind=NEON_CAMERA_CMD_RETURN,.generation=1,.frame=frame});
    CHECK_EQ(r.result,NEON_CAMERA_OK); CHECK(!s.session.status.claimed); CHECK(!handler);
    vQueueDelete(s.commands);
}
static void timer_progress_timeout_and_storm(void)
{
    neon_camera_freertos_t s={0}; setup(&s);
    CHECK(s.deadline_active); TickType_t deadline=s.deadline;
    hw.reg[R(TUNE_STATUS)/4] |= M(TUNE_STATUS_CHECK_BUSY);
    for (unsigned n=0;n<6;++n) { ++ticks; xTaskNotifyGive(s.worker); (void)worker_turn(&s); }
    CHECK_EQ(s.deadline,deadline); CHECK_EQ(s.session.status.state,NEON_CAMERA_STARTING);
    ticks=deadline; (void)worker_turn(&s); CHECK_EQ(s.session.status.last_error,NEON_CAMERA_TIMEOUT);
    CHECK(s.session.status.claimed); CHECK(!hw.reg[R(DMA_IRQ_EN)/4]);
    hw.reg[R(TUNE_STATUS)/4] &= ~M(TUNE_STATUS_CHECK_BUSY); shutdown_service(&s);
    s=(neon_camera_freertos_t){0}; setup(&s);
    hw.stuck_irq=true; hw.set_bits=1; hw.reg[R(OUTPUT_IRQ_STATUS)/4]=1;
    for (unsigned n=0;n<4;++n) (void)worker_turn(&s);
    CHECK(s.storm); CHECK(s.session.status.claimed); CHECK_EQ(hw.reg[R(OUTPUT_IRQ_EN)/4],0);
    hw.stuck_irq=false; hw.set_bits=0; shutdown_service(&s);
}
static void startup_failure(void)
{
    neon_camera_freertos_t s={0}; neon_camera_freertos_config_t c=configuration();
    fail_queue=true; CHECK_EQ(neon_camera_freertos_create(&s,&c),NEON_CAMERA_BUSY); fail_queue=false;
    fail_task=true; CHECK_EQ(neon_camera_freertos_create(&s,&c),NEON_CAMERA_BUSY); CHECK(!s.commands); fail_task=false;
    setup(&s); shutdown_service(&s);
    /* Failure after claim but before registration must remain quarantined. */
    s=(neon_camera_freertos_t){0}; mmio_init(&hw); CHECK_EQ(neon_camera_freertos_create(&s,&c),NEON_CAMERA_OK);
    s.backend.port.io=(fx1_isp_io_t){&s,service_read,service_write,mmio_barrier};
    s.backend.port.prepare=service_prepare; s.backend.port.complete=service_complete;
    fail_register=true;
    neon_camera_reply_t r=command(&s,(neon_camera_command_t){.kind=NEON_CAMERA_CMD_START});
    CHECK_EQ(r.result,NEON_CAMERA_HW_ERROR); CHECK(s.session.status.claimed); CHECK(!handler); fail_register=false;
    shutdown_service(&s);
}
int main(void)
{
    /* Exercise fixture attach/detach helpers too, then use real service hooks. */
    isp_mmio_t probe; mmio_init(&probe); fx1_isp_backend_port_t p=mmio_port(&probe);
    CHECK(p.attach(&probe)); p.detach(&probe);
    RUN(queue_saturation_stop_and_stale); RUN(lost_wakeup_coalescing_and_consumer_stop);
    RUN(timer_progress_timeout_and_storm); RUN(startup_failure);
    CHECK_EQ(registrations,unregistrations); CHECK_EQ(critical,0);
    return TEST_REPORT();
}
