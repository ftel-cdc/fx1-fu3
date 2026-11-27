#ifndef NEON_CAMERA_RUNTIME_H
#define NEON_CAMERA_RUNTIME_H
#include "neon_camera_frame.h"
#include "neon_camera_status.h"
#define NEON_CAMERA_HISTORY_SIZE 8u
#define NEON_CAMERA_SESSION_INITIALIZER {0}

typedef struct {
    void *device;
    const neon_isp_session_backend_t *backend;
    neon_isp_session_config_t stream;
    neon_isp_session_profile_t baseline;
} neon_camera_session_config_t;

typedef enum {
    NEON_CAMERA_BUFFER_FREE, NEON_CAMERA_BUFFER_HARDWARE,
    NEON_CAMERA_BUFFER_READY, NEON_CAMERA_BUFFER_CONSUMER,
    NEON_CAMERA_BUFFER_QUARANTINED
} neon_camera_buffer_state_t;
/* Caller-owned, fixed storage. Treat fields as private; never copy a live session.
 * Initialize storage with INITIALIZER once. Reconfigure through init only after
 * CONFIGURED/unclaimed, preserving monotonic generation and lease counters.
 * All APIs, including lease return, must run in the serialized owner context.
 * Other tasks queue commands/returns to that owner through the optional camera
 * FreeRTOS service. Core itself has no RTOS or register access.
 */
typedef struct {
    neon_camera_status_t status;
    neon_camera_session_config_t config;
    neon_camera_buffer_state_t buffers[NEON_ISP_SESSION_SLOTS];
    neon_camera_frame_t frames[NEON_ISP_SESSION_SLOTS];
    neon_camera_frame_t history[NEON_CAMERA_HISTORY_SIZE];
    uint64_t next_lease;
    uint8_t cursor, history_cursor, in_flight;
    bool stop_requested;
    neon_isp_session_transaction_t pending;
} neon_camera_session_t;

neon_camera_result_t neon_camera_session_validate(const neon_camera_session_config_t *);
neon_camera_result_t neon_camera_session_init(neon_camera_session_t *, const neon_camera_session_config_t *);
neon_camera_result_t neon_camera_session_start(neon_camera_session_t *);
/* One authoritative poll; call until NO_DATA, yielding/deadline in the service.
 * IRQ notification is only a hint to call this function. */
neon_camera_result_t neon_camera_session_poll(neon_camera_session_t *);
neon_camera_result_t neon_camera_session_submit(neon_camera_session_t *, uint32_t tag);
neon_camera_result_t neon_camera_session_acquire(neon_camera_session_t *, neon_camera_frame_t *);
neon_camera_result_t neon_camera_session_return(neon_camera_session_t *, const neon_camera_frame_t *);
neon_camera_result_t neon_camera_session_apply(neon_camera_session_t *, const neon_isp_session_transaction_t *, uint32_t observed_next_frame);
neon_camera_result_t neon_camera_session_stop(neon_camera_session_t *);
/* Caller reports deadline expiry; keeps claim, HW ownership and diagnostics. */
neon_camera_result_t neon_camera_session_timeout(neon_camera_session_t *);
neon_camera_result_t neon_camera_session_status(const neon_camera_session_t *, neon_camera_status_t *);
neon_camera_result_t neon_camera_session_history(const neon_camera_session_t *, uint64_t generation, uint32_t ingress_frame, neon_camera_frame_t *);
/* SW precheck only; backend must repeat using NEXT_FRAME at command acceptance. */
neon_camera_result_t neon_camera_target_validate(neon_isp_apply_mode_t, uint32_t target, uint32_t next, bool active);
#endif
