#include "camera_internal.h"
neon_camera_result_t camera_fail(neon_camera_session_t *s, neon_camera_result_t r)
{
    s->status.state = NEON_CAMERA_FAULTED;
    s->status.last_error = r;
    for (unsigned i = 0; i < NEON_ISP_SESSION_SLOTS; ++i)
        if (s->buffers[i] == NEON_CAMERA_BUFFER_HARDWARE)
            s->buffers[i] = NEON_CAMERA_BUFFER_QUARANTINED;
    return r;
}
neon_camera_result_t camera_finish_stop(neon_camera_session_t *s)
{
    if (!s->stop_requested || !s->status.drained) return NEON_CAMERA_OK;
    if (s->status.ready || s->status.consumer_held) return NEON_CAMERA_PENDING;
    s->config.backend->release(s->config.device, s);
    s->status.claimed = false;
    s->status.state = NEON_CAMERA_CONFIGURED;
    s->stop_requested = false;
    return NEON_CAMERA_OK;
}
neon_camera_result_t neon_camera_session_stop(neon_camera_session_t *s)
{
    neon_camera_result_t r;
    if (!s) return NEON_CAMERA_INVALID;
    if (!s->status.claimed) return s->status.state == NEON_CAMERA_CONFIGURED ? NEON_CAMERA_OK : NEON_CAMERA_INVALID;
    if (s->stop_requested) return NEON_CAMERA_PENDING;
    r = s->config.backend->stop(s->config.device);
    if (r != NEON_CAMERA_OK && r != NEON_CAMERA_PENDING) return camera_fail(s, r);
    s->stop_requested = true;
    s->status.state = NEON_CAMERA_STOPPING;
    return NEON_CAMERA_PENDING;
}
neon_camera_result_t neon_camera_session_timeout(neon_camera_session_t *s)
{
    if (!s) return NEON_CAMERA_INVALID;
    if (!s->status.claimed) return NEON_CAMERA_INVALID;
    return camera_fail(s, NEON_CAMERA_TIMEOUT);
}
