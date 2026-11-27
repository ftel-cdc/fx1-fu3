#include "camera_internal.h"
void camera_history_add(neon_camera_session_t *s, const neon_camera_frame_t *f)
{
    /* This is a bounded completed trace, not a pending transaction store.
     * Live records stay pinned in frames[] and are never evicted here. */
    s->history[s->history_cursor] = *f;
    s->history_cursor = (uint8_t)((s->history_cursor + 1u) % NEON_CAMERA_HISTORY_SIZE);
}
neon_camera_result_t neon_camera_session_history(const neon_camera_session_t *s, uint64_t generation, uint32_t frame, neon_camera_frame_t *out)
{
    if (!s || !out || !generation) return NEON_CAMERA_INVALID;
    for (unsigned i = 0; i < NEON_ISP_SESSION_SLOTS; ++i)
        if ((s->buffers[i] == NEON_CAMERA_BUFFER_READY || s->buffers[i] == NEON_CAMERA_BUFFER_CONSUMER) &&
            s->frames[i].generation == generation && s->frames[i].identity.ingress_frame == frame) {
            *out = s->frames[i]; return NEON_CAMERA_OK;
        }
    for (unsigned n = 0; n < NEON_CAMERA_HISTORY_SIZE; ++n) {
        unsigned i = (s->history_cursor + NEON_CAMERA_HISTORY_SIZE - 1u - n) % NEON_CAMERA_HISTORY_SIZE;
        const neon_camera_frame_t *f = &s->history[i];
        if (f->generation == generation && f->identity.ingress_frame == frame) {
            *out = *f; return NEON_CAMERA_OK;
        }
    }
    return NEON_CAMERA_NO_DATA;
}
