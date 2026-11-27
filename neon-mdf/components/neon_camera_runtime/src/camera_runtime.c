#include "camera_internal.h"

neon_camera_result_t neon_camera_session_start(neon_camera_session_t *s)
{
    neon_camera_result_t r;
    if (!s) return NEON_CAMERA_INVALID;
    if (s->status.state != NEON_CAMERA_CONFIGURED || s->status.claimed) return NEON_CAMERA_BUSY;
    if (s->status.generation == UINT64_MAX) return NEON_CAMERA_INVALID;
    r = s->config.backend->claim(s->config.device, s);
    if (r != NEON_CAMERA_OK) return r;
    ++s->status.generation;
    s->status.claimed = true;
    s->status.drained = false;
    s->status.profile_pending = true;
    s->status.hardware = (neon_isp_hw_snapshot_t){0};
    s->status.submitted = s->status.retired = 0;
    s->status.state = NEON_CAMERA_STARTING;
    s->stop_requested = false;
    s->cursor = 0; /* A drained backend begins a fresh rotation at hardware slot 0. */
    s->pending = (neon_isp_session_transaction_t){.profile = s->config.baseline, .mode = NEON_ISP_APPLY_NEXT};
    r = s->config.backend->begin(s->config.device, &s->config.stream, &s->config.baseline);
    if (r == NEON_CAMERA_UNSUPPORTED) {
        s->config.backend->release(s->config.device, s);
        s->status.claimed = s->status.profile_pending = false;
        s->status.state = NEON_CAMERA_CONFIGURED;
    } else if (r != NEON_CAMERA_OK && r != NEON_CAMERA_PENDING) return camera_fail(s, r);
    s->status.last_error = r == NEON_CAMERA_UNSUPPORTED ? r : NEON_CAMERA_OK;
    return r == NEON_CAMERA_OK ? NEON_CAMERA_PENDING : r;
}

neon_camera_result_t neon_camera_session_poll(neon_camera_session_t *s)
{
    neon_isp_session_observation_t o = {0};
    neon_camera_result_t r;
    if (!s) return NEON_CAMERA_INVALID;
    if (!s->status.claimed) return NEON_CAMERA_NO_DATA;
    r = s->config.backend->poll(s->config.device, &o);
    if (o.hardware.valid) s->status.hardware = o.hardware;
    if (r == NEON_CAMERA_NO_DATA || r == NEON_CAMERA_PENDING) return r;
    if (r != NEON_CAMERA_OK) return camera_fail(s, r);
    r = camera_observe(s, &o);
    if (r != NEON_CAMERA_OK) return camera_fail(s, r);
    return camera_finish_stop(s);
}

neon_camera_result_t neon_camera_session_status(const neon_camera_session_t *s, neon_camera_status_t *out)
{
    if (!s || !out) return NEON_CAMERA_INVALID;
    *out = s->status;
    return NEON_CAMERA_OK;
}
