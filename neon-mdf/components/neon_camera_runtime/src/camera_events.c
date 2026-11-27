#include "camera_internal.h"
neon_camera_result_t camera_observe(neon_camera_session_t *s, const neon_isp_session_observation_t *o)
{
    neon_camera_result_t result = (o->flags & NEON_ISP_OBS_FAULT) ? NEON_CAMERA_HW_ERROR : NEON_CAMERA_OK;
    if (o->flags & NEON_ISP_OBS_APPLIED) {
        if (!s->status.profile_pending || o->applied_id != s->pending.profile.id ||
            (s->pending.mode == NEON_ISP_APPLY_EXACT && o->applied_frame != s->pending.target_frame))
            result = NEON_CAMERA_HW_ERROR;
        else {
            s->status.profile_pending = false;
            if (s->status.state == NEON_CAMERA_STARTING) s->status.state = NEON_CAMERA_RUNNING;
        }
        /* Applied frame belongs to this transaction, never to a guessed sensor latency. */
    }
    if (o->flags & NEON_ISP_OBS_OUTPUT) {
        neon_camera_frame_t *f;
        const uint32_t required = NEON_ISP_ID_FRAME | NEON_ISP_ID_PROFILE;
        if (o->slot >= NEON_ISP_SESSION_SLOTS ||
            (o->output.valid & required) != required ||
            (s->buffers[o->slot] != NEON_CAMERA_BUFFER_HARDWARE &&
             s->buffers[o->slot] != NEON_CAMERA_BUFFER_QUARANTINED)) return NEON_CAMERA_HW_ERROR;
        for (unsigned i = 0; i < NEON_ISP_SESSION_SLOTS; ++i)
            if ((s->buffers[i] == NEON_CAMERA_BUFFER_READY || s->buffers[i] == NEON_CAMERA_BUFFER_CONSUMER) &&
                s->frames[i].identity.ingress_frame == o->output.ingress_frame)
                return NEON_CAMERA_HW_ERROR; /* No two live interpretations across ID wrap. */
        f = &s->frames[o->slot];
        if (o->output.tag != f->identity.tag) return NEON_CAMERA_HW_ERROR;
        f->identity = o->output; /* Do not substitute current ACTIVE_ID. */
        f->recovery_sequence_valid = (o->hardware.valid & NEON_ISP_DIAG_RECOVERY) != 0;
        if (f->recovery_sequence_valid) f->recovery_sequence = o->hardware.recovery_sequence;
        s->buffers[o->slot] = NEON_CAMERA_BUFFER_READY;
        s->in_flight = 0;
        ++s->status.retired; ++s->status.ready;
        camera_history_add(s, f);
    }
    if (o->flags & NEON_ISP_OBS_DRAINED) {
        if (!s->stop_requested) return NEON_CAMERA_HW_ERROR;
        s->status.drained = true;
        s->status.profile_pending = false;
        s->in_flight = 0;
        for (unsigned i = 0; i < NEON_ISP_SESSION_SLOTS; ++i)
            if (s->buffers[i] == NEON_CAMERA_BUFFER_HARDWARE || s->buffers[i] == NEON_CAMERA_BUFFER_QUARANTINED)
                s->buffers[i] = NEON_CAMERA_BUFFER_FREE;
        s->status.state = NEON_CAMERA_DRAINING;
    }
    return result;
}
