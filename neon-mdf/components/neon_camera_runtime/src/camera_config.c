#include "camera_internal.h"

neon_camera_result_t neon_camera_session_validate(const neon_camera_session_config_t *c)
{
    const neon_isp_session_config_t *v;
    neon_isp_session_span_t spans[NEON_ISP_SESSION_SLOTS * 3u];
    const neon_isp_session_backend_t *b;
    if (!c || !c->device || !c->backend || !c->baseline.data || !c->baseline.bytes)
        return NEON_CAMERA_INVALID;
    b = c->backend;
    if (b->abi != NEON_ISP_SESSION_ABI ||
        (b->capabilities & NEON_ISP_SESSION_REQUIRED_CAPS) != NEON_ISP_SESSION_REQUIRED_CAPS)
        return NEON_CAMERA_UNSUPPORTED;
    if (!b->claim || !b->release || !b->begin || !b->poll || !b->submit ||
        !b->release_output || !b->apply || !b->stop) return NEON_CAMERA_INVALID;
    v = &c->stream;
    if (!v->width || !v->height || (v->width & 1u) || (v->height & 1u) ||
        v->width > UINT16_MAX || v->height > UINT16_MAX ||
        !v->dma_alignment || v->bayer_pattern > 3u) return NEON_CAMERA_INVALID;
    if (v->raw_depth != 12u) return NEON_CAMERA_UNSUPPORTED;
    if ((uint64_t)v->raw_stride < (uint64_t)v->width * 2u ||
        v->y_stride < v->width || v->uv_stride < v->width ||
        v->raw_stride % v->dma_alignment || v->y_stride % v->dma_alignment ||
        v->uv_stride % v->dma_alignment) return NEON_CAMERA_INVALID;
    for (unsigned i = 0; i < NEON_ISP_SESSION_SLOTS; ++i) {
        const neon_isp_session_buffer_t *p = &v->buffers[i];
        if ((uint64_t)p->raw.bytes < (uint64_t)v->raw_stride * v->height ||
            (uint64_t)p->y.bytes < (uint64_t)v->y_stride * v->height ||
            (uint64_t)p->uv.bytes < (uint64_t)v->uv_stride * (v->height / 2u)) return NEON_CAMERA_INVALID;
        spans[i * 3u] = p->raw; spans[i * 3u + 1u] = p->y; spans[i * 3u + 2u] = p->uv;
    }
    for (unsigned i = 0; i < NEON_ISP_SESSION_SLOTS * 3u; ++i) {
        uint64_t end = (uint64_t)spans[i].address + spans[i].bytes;
        if (end > (uint64_t)UINT32_MAX + 1u || spans[i].address % v->dma_alignment)
            return NEON_CAMERA_INVALID;
        for (unsigned j = 0; j < i; ++j)
            if ((uint64_t)spans[i].address < (uint64_t)spans[j].address + spans[j].bytes &&
                (uint64_t)spans[j].address < end) return NEON_CAMERA_INVALID;
    }
    return NEON_CAMERA_OK;
}

neon_camera_result_t neon_camera_session_init(neon_camera_session_t *s, const neon_camera_session_config_t *c)
{
    neon_camera_result_t r;
    if (!s) return NEON_CAMERA_INVALID;
    if (s->status.claimed || (s->status.state != NEON_CAMERA_UNINITIALIZED &&
        s->status.state != NEON_CAMERA_CONFIGURED)) return NEON_CAMERA_BUSY;
    r = neon_camera_session_validate(c);
    if (r != NEON_CAMERA_OK) return r;
    s->config = *c;
    s->status.state = NEON_CAMERA_CONFIGURED;
    s->status.last_error = NEON_CAMERA_OK;
    return NEON_CAMERA_OK;
}
