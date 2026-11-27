#include "camera_internal.h"
neon_camera_result_t neon_camera_session_submit(neon_camera_session_t *s, uint32_t tag)
{
    neon_camera_result_t r;
    if (!s) return NEON_CAMERA_INVALID;
    /* Baseline needs ingress to become active; do not deadlock STARTING. */
    if ((s->status.state != NEON_CAMERA_RUNNING && s->status.state != NEON_CAMERA_STARTING) || s->in_flight)
        return NEON_CAMERA_BUSY;
    for (unsigned n = 0; n < NEON_ISP_SESSION_SLOTS; ++n) {
        uint8_t i = (uint8_t)((s->cursor + n) % NEON_ISP_SESSION_SLOTS);
        if (s->buffers[i] != NEON_CAMERA_BUFFER_FREE) continue;
        if (s->next_lease == UINT64_MAX) return NEON_CAMERA_INVALID;
        s->frames[i] = (neon_camera_frame_t){.generation = s->status.generation,
            .lease = ++s->next_lease, .slot = i, .replay = true};
        s->frames[i].identity.tag = tag;
        r = s->config.backend->submit(s->config.device, i, tag);
        if (r != NEON_CAMERA_OK && r != NEON_CAMERA_PENDING) {
            if (r == NEON_CAMERA_HW_ERROR || r == NEON_CAMERA_TIMEOUT) {
                s->buffers[i] = NEON_CAMERA_BUFFER_QUARANTINED;
                return camera_fail(s, r);
            }
            return r;
        }
        s->buffers[i] = NEON_CAMERA_BUFFER_HARDWARE;
        s->in_flight = 1;
        ++s->status.submitted;
        s->cursor = (uint8_t)((i + 1u) % NEON_ISP_SESSION_SLOTS);
        return NEON_CAMERA_OK;
    }
    return NEON_CAMERA_BUSY;
}
neon_camera_result_t neon_camera_session_acquire(neon_camera_session_t *s, neon_camera_frame_t *out)
{
    if (!s || !out) return NEON_CAMERA_INVALID;
    unsigned oldest = NEON_ISP_SESSION_SLOTS;
    for (unsigned i = 0; i < NEON_ISP_SESSION_SLOTS; ++i)
        if (s->buffers[i] == NEON_CAMERA_BUFFER_READY &&
            (oldest == NEON_ISP_SESSION_SLOTS || s->frames[i].lease < s->frames[oldest].lease)) oldest = i;
    if (oldest != NEON_ISP_SESSION_SLOTS) {
        s->buffers[oldest] = NEON_CAMERA_BUFFER_CONSUMER;
        --s->status.ready; ++s->status.consumer_held;
        *out = s->frames[oldest];
        return NEON_CAMERA_OK;
    }
    return NEON_CAMERA_NO_DATA;
}
neon_camera_result_t neon_camera_session_return(neon_camera_session_t *s, const neon_camera_frame_t *f)
{
    neon_camera_result_t r;
    if (!s || !f || f->slot >= NEON_ISP_SESSION_SLOTS) return NEON_CAMERA_INVALID;
    if (f->generation != s->status.generation ||
        s->buffers[f->slot] != NEON_CAMERA_BUFFER_CONSUMER ||
        f->lease != s->frames[f->slot].lease) return NEON_CAMERA_STALE;
    r = s->config.backend->release_output(s->config.device, f->slot);
    if (r != NEON_CAMERA_OK) return r;
    s->buffers[f->slot] = NEON_CAMERA_BUFFER_FREE;
    --s->status.consumer_held;
    (void)camera_finish_stop(s);
    return NEON_CAMERA_OK;
}
