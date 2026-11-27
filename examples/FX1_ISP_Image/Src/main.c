/* MAS v1.07 scalar profile replay through the camera owner worker. Input DDR
 * must be populated before running. This example does not load the old csrw/IQ
 * profile; table consumers and 3A are disabled in this minimal register backend. */
#include "neon_camera_freertos.h"
#include "neon_freertos_fx1.h"
#include "fx1_fw.h"
#ifndef WIDTH
#define WIDTH 2688u
#endif
#ifndef HEIGHT
#define HEIGHT 1520u
#endif
#define RAW_BASE 0x81000000u
#define OUTPUT_BASE 0x88000000u
#define SLOT_BYTES 0x00800000u
#ifndef FRAME_COUNT
#define FRAME_COUNT 8u
#endif
static neon_camera_freertos_t camera;
static const fx1_isp_scalar_profile_t profile = FX1_ISP_SCALAR_PROFILE_INITIALIZER;
static neon_camera_reply_t call(neon_camera_command_t c)
{
    neon_camera_reply_t reply;
    neon_camera_ticket_t ticket;
    configASSERT(neon_camera_freertos_enqueue(&camera, &c, &ticket) == NEON_CAMERA_OK);
    while (neon_camera_freertos_collect(&camera, ticket, &reply) == NEON_CAMERA_NO_DATA) vTaskDelay(1);
    return reply;
}
static void application(void *unused)
{
    (void)unused;
    neon_camera_reply_t r = call((neon_camera_command_t){.kind = NEON_CAMERA_CMD_START});
    fx1_log_hex("START_RESULT", r.result);
    fx1_log_hex("ISP_VERSION", r.device_version);
    fx1_log_hex("TUNE_ID", r.tune_id);
    fx1_log_hex("TUNE_CAPS", r.device_capabilities);
    if (r.result != NEON_CAMERA_PENDING) fx1_fail(0xca01);
    uint64_t generation = r.status.generation;
    for (uint32_t n = 0; n < FRAME_COUNT; ++n) {
        do {
            r = call((neon_camera_command_t){.kind = NEON_CAMERA_CMD_SUBMIT, .generation = generation, .tag = n + 0x1000u});
            if (r.status.state == NEON_CAMERA_FAULTED) fx1_fail(0xca02);
            if (r.result == NEON_CAMERA_BUSY) vTaskDelay(1);
        } while (r.result == NEON_CAMERA_BUSY);
        if (r.result != NEON_CAMERA_OK) fx1_fail(0xca03);
        do {
            r = call((neon_camera_command_t){.kind = NEON_CAMERA_CMD_ACQUIRE, .generation = generation});
            if (r.status.state == NEON_CAMERA_FAULTED) fx1_fail(0xca04);
            if (r.result == NEON_CAMERA_NO_DATA) vTaskDelay(1);
        } while (r.result == NEON_CAMERA_NO_DATA);
        if (r.result != NEON_CAMERA_OK || r.frame.identity.tag != n + 0x1000u || r.frame.identity.profile_id != 7)
            fx1_fail(0xca05);
        fx1_log_hex("RETIRED_FRAME", r.frame.identity.ingress_frame);
        fx1_log_hex("OUTPUT_SLOT", r.frame.slot);
        neon_camera_frame_t frame = r.frame;
        r = call((neon_camera_command_t){.kind = NEON_CAMERA_CMD_RETURN, .generation = generation, .frame = frame});
        if (r.result != NEON_CAMERA_OK) fx1_fail(0xca06);
    }
    configASSERT(neon_camera_freertos_stop(&camera, generation) == NEON_CAMERA_PENDING);
    neon_camera_status_t status;
    do {
        vTaskDelay(1); neon_camera_freertos_status(&camera, &status);
        if (status.last_error != NEON_CAMERA_OK) fx1_fail(0xca07);
    } while (status.claimed);
    fx1_puts("[PASS] CAMERA_SCALAR_REPLAY: retired metadata, lease return, safe stop\n");
    fx1_pass();
}
int main(void)
{
    fx1_puts("Camera MAS v1.07 scalar profile / deferred IRQ worker\n");
    NEON_FX1_FreeRTOS_Init();
    neon_camera_freertos_config_t c = {.irq_priority = 2, .worker_priority = 3,
        .stack_words = 2048, .storm_budget = 32, .poll_ticks = 1, .timeout_ticks = pdMS_TO_TICKS(5000),
        .session = {.baseline = {&profile, sizeof(profile), 7},
            .stream = {.width = WIDTH, .height = HEIGHT, .raw_stride = WIDTH * 2u,
                .y_stride = WIDTH, .uv_stride = WIDTH, .dma_alignment = 16, .raw_depth = 12, .bayer_pattern = 3}}};
    for (unsigned i = 0; i < 4; ++i) {
        c.session.stream.buffers[i] = (neon_isp_session_buffer_t){
            {RAW_BASE + i * SLOT_BYTES, WIDTH * HEIGHT * 2u},
            {OUTPUT_BASE + i * SLOT_BYTES, WIDTH * HEIGHT},
            {OUTPUT_BASE + i * SLOT_BYTES + 0x00400000u, WIDTH * HEIGHT / 2u}};
    }
    configASSERT(neon_camera_freertos_create(&camera, &c) == NEON_CAMERA_OK);
    configASSERT(xTaskCreate(application, "replay", 1536, NULL, 1, NULL) == pdPASS);
    vTaskStartScheduler();
    fx1_fail(0xca00);
}
