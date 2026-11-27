#include "camera_internal.h"
neon_camera_result_t neon_camera_target_validate(neon_isp_apply_mode_t mode, uint32_t target, uint32_t next, bool active)
{
    uint32_t d = target - next;
    if (mode == NEON_ISP_APPLY_NEXT) return NEON_CAMERA_OK;
    if (mode != NEON_ISP_APPLY_EXACT || d >= UINT32_C(0x80000000) || (!active && d))
        return NEON_CAMERA_INVALID;
    return NEON_CAMERA_OK;
}
neon_camera_result_t neon_camera_session_apply(neon_camera_session_t *s, const neon_isp_session_transaction_t *t, uint32_t next)
{
    neon_camera_result_t r;
    if (!s || !t || !t->profile.data || !t->profile.bytes) return NEON_CAMERA_INVALID;
    if (s->status.state != NEON_CAMERA_RUNNING || s->status.profile_pending) return NEON_CAMERA_BUSY;
    r = neon_camera_target_validate(t->mode, t->target_frame, next, true);
    if (r != NEON_CAMERA_OK) return r;
    r = s->config.backend->apply(s->config.device, t);
    if (r == NEON_CAMERA_OK || r == NEON_CAMERA_PENDING) {
        s->pending = *t;
        s->status.profile_pending = true;
        return NEON_CAMERA_PENDING;
    }
    if (r == NEON_CAMERA_HW_ERROR || r == NEON_CAMERA_TIMEOUT) return camera_fail(s, r);
    return r;
}
