#ifndef NEON_CAMERA_FREERTOS_H
#define NEON_CAMERA_FREERTOS_H
#include "neon_camera_runtime.h"
#include "isp/platform/fx1/fx1_isp_backend.h"
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
#define NEON_CAMERA_SERVICE_REQUESTS 8u
typedef enum { NEON_CAMERA_CMD_START, NEON_CAMERA_CMD_SUBMIT, NEON_CAMERA_CMD_APPLY,
    NEON_CAMERA_CMD_ACQUIRE, NEON_CAMERA_CMD_RETURN, NEON_CAMERA_CMD_STATUS } neon_camera_command_kind_t;
typedef struct {
    neon_camera_command_kind_t kind;
    uint64_t generation; /* Required except START/STATUS. Reject stale queued work. */
    uint32_t tag;
    neon_isp_session_transaction_t transaction;
    neon_camera_frame_t frame;
} neon_camera_command_t;
typedef struct {
    neon_camera_result_t result;
    uint32_t device_version, device_capabilities, tune_id; /* saved probe, no client MMIO */
    neon_camera_frame_t frame;
    neon_camera_status_t status;
} neon_camera_reply_t;
typedef struct { uint64_t serial; uint8_t index; } neon_camera_ticket_t;
typedef struct {
    neon_camera_session_config_t session; /* backend/device overridden with FX1 backend */
    uint32_t irq_priority, worker_priority, stack_words, storm_budget;
    TickType_t poll_ticks, timeout_ticks; /* 1..half tick range, absolute deadline */
} neon_camera_freertos_config_t;
/* Caller-owned stable storage, zero-initialize ONCE. Do not free or move while
 * worker exists, even after stop. Worker persists to support restart. Requests
 * own copies of descriptors; profile payload remains borrowed until applied or
 * drained. Collect every accepted ticket; full storage/queue returns BUSY without
 * transfer. STOP has its own mailbox, independent of request/result saturation. */
typedef struct {
    neon_camera_session_t session;
    fx1_isp_backend_t backend;
    neon_camera_freertos_config_t config;
    QueueHandle_t commands;
    TaskHandle_t worker;
    struct {
        uint64_t serial;
        unsigned state; /* 0 free, 1 queued/active, 2 completed */
        neon_camera_command_t command;
        neon_camera_reply_t reply;
    } requests[NEON_CAMERA_SERVICE_REQUESTS];
    uint64_t serial, stop_generation;
    neon_camera_status_t cached_status;
    bool stop_pending, deadline_active, timed_out, storm;
    TickType_t deadline;
    uint32_t unproductive;
} neon_camera_freertos_t;
neon_camera_result_t neon_camera_freertos_create(neon_camera_freertos_t *, const neon_camera_freertos_config_t *);
neon_camera_result_t neon_camera_freertos_enqueue(neon_camera_freertos_t *, const neon_camera_command_t *, neon_camera_ticket_t *);
neon_camera_result_t neon_camera_freertos_collect(neon_camera_freertos_t *, neon_camera_ticket_t, neon_camera_reply_t *);
neon_camera_result_t neon_camera_freertos_stop(neon_camera_freertos_t *, uint64_t generation);
/* Coherent cached status, no MMIO/core access in client task. */
void neon_camera_freertos_status(neon_camera_freertos_t *, neon_camera_status_t *);
#endif
