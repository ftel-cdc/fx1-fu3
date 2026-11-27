#include "fx1_isp_backend.h"
#include "fx1_isp_profile_txn.h"
#include "isp/neon_isp_claim.h"
#include "fx1_isp/fx1_isp_csr.h"
#define R(n) FX1_ISP_##n##_OFFSET
#define M(n) FX1_ISP_##n##_MASK
static uint32_t rd(fx1_isp_backend_t *b, uint32_t r) { return b->port.io.read(b->port.io.user, r); }
static void wr(fx1_isp_backend_t *b, uint32_t r, uint32_t v) { b->port.io.write(b->port.io.user, r, v); }
static void barrier(fx1_isp_backend_t *b) { b->port.io.barrier(b->port.io.user); }
/* Engines ordered AEC/AWB/AF throughout diagnostics. */
static const uint32_t stat_owner[] = {R(TUNE_AEC_STATUS), R(AWB_RESULT_STATUS), R(TUNE_AF_STATUS)};
static const uint32_t stat_event[] = {R(AEC_STATUS), R(AWB_STATUS), R(AF_STATUS)};
static const uint32_t stat_drop[] = {R(TUNE_AEC_DROPS), R(AWB_DROP_COUNT), R(TUNE_AF_DROPS)};
static const uint32_t stat_cmd[] = {R(TUNE_AEC_CMD), R(AWB_RESULT_CTRL), R(TUNE_AF_CMD)};
static bool stats_empty(fx1_isp_backend_t *b)
{
    for (unsigned i = 0; i < 3; ++i)
        if (rd(b, stat_owner[i]) & (i == 1 ? 0x27u : 0x0fu)) return false;
    return true;
}
static bool idle(fx1_isp_backend_t *b)
{
    /* COMMON BUSY aggregates unretired contexts/output masters. Builder/prep
     * checks are additional requirements of IDLE writes, not timeout guesses. */
    return !(rd(b, R(COMMON_STATUS)) & M(COMMON_STATUS_BUSY)) &&
        !rd(b, R(DMA_STAT)) && !(rd(b, R(IDMA_BUF_VALID)) & 15u) &&
        !(rd(b, R(COMMON_RECOVERY_STATUS)) & M(COMMON_RECOVERY_STATUS_BUSY)) &&
        !(rd(b, R(LSC_PROFILE_STATUS)) & M(LSC_PROFILE_STATUS_PREP_BUSY)) &&
        !(rd(b, R(GTM_STATUS)) & M(GTM_STATUS_BUSY)) &&
        !(rd(b, R(NR_2D_STATUS)) & M(NR_2D_STATUS_BUSY)) &&
        !(rd(b, R(TUNE_STATUS)) & M(TUNE_STATUS_CHECK_BUSY));
}
static neon_camera_result_t claim(void *device, const void *owner)
{
    fx1_isp_backend_t *b = device;
    if (!b || b->owner || !neon_isp_device_claim(owner)) return NEON_CAMERA_BUSY;
    b->owner = owner;
    return NEON_CAMERA_OK;
}
static void release(void *device, const void *owner)
{
    fx1_isp_backend_t *b = device;
    if (b->owner != owner) return;
    if (b->attached) {
        fx1_isp_irq_gate(&b->port.io);
        b->port.detach(b->port.io.user);
        b->attached = false;
    }
    (void)neon_isp_device_release(owner);
    b->owner = NULL;
}
static bool recovery_snapshot(fx1_isp_backend_t *b);
static neon_camera_result_t begin(void *device, const neon_isp_session_config_t *c,
                                   const neon_isp_session_profile_t *p)
{
    fx1_isp_backend_t *b = device;
    if (!b->owner) return NEON_CAMERA_INVALID;
    const uint32_t required = M(TUNE_CAPS_PROFILE) | M(TUNE_CAPS_RECOVERY_REPORT) |
        M(TUNE_CAPS_FRAME_ID) | M(TUNE_CAPS_WB_GB) | M(TUNE_CAPS_INPUT_PAD) | M(TUNE_CAPS_AEC_LEASE) | M(TUNE_CAPS_AF_LEASE);
    b->capabilities = rd(b, R(TUNE_CAPS));
    b->version = rd(b, R(COMMON_VER_ID)); b->tune_id = rd(b, R(TUNE_ID));
    if (!fx1_isp_profile_valid(p) || c->raw_depth != 12 || c->width < 32 || c->height < 32 || c->width > 3840 || c->height > 2160 ||
        c->dma_alignment < 16 || (c->raw_stride | c->y_stride | c->uv_stride) & 15u ||
        b->version != FX1_ISP_COMMON_VER_ID_RESET ||
        b->tune_id != FX1_ISP_TUNE_ID_RESET || (b->capabilities & required) != required)
        return NEON_CAMERA_UNSUPPORTED; /* No MMIO mutation before this return. */
    for (unsigned i = 0; i < 4; ++i)
        if ((c->buffers[i].raw.address | c->buffers[i].y.address | c->buffers[i].uv.address) & 15u)
            return NEON_CAMERA_UNSUPPORTED;
    /* Do not steal residual hardware ownership. A dirty device stays claimed
     * for explicit stop/recovery; never blanket-reset someone else's results. */
    if (!idle(b) || !stats_empty(b) || rd(b, R(ODMA_BUF_DONE)) || rd(b, R(ODMA_BUF_FREE)) ||
        rd(b, R(CH1_ODMA_BUF_DONE)) || rd(b, R(CH2_ODMA_BUF_DONE)) ||
        rd(b, R(DMA_CTRL)) & (M(DMA_CTRL_IDMA_EN) | M(DMA_CTRL_ODMA_EN))) return NEON_CAMERA_HW_ERROR;
    b->config = *c;
    b->published = b->submitted = 0; b->head = 0;
    b->faulted = b->stopping = b->reset_requested = false;
    b->recovery_valid = false;
    b->evidence = (neon_isp_hw_snapshot_t){0};
    b->transaction = (neon_isp_session_transaction_t){.profile = *p, .mode = NEON_ISP_APPLY_NEXT};
    fx1_isp_irq_gate(&b->port.io);
    if (!b->port.attach(b->port.io.user)) return NEON_CAMERA_HW_ERROR;
    b->attached = true;
    /* A clean ownership bitmap does not prove rotation pointer zero. Copy any
     * previous recovery before requesting a stopped DMA-scope recovery; it is
     * the authoritative pointer/AXI/context reset prerequisite, not a delay. */
    if (!recovery_snapshot(b)) return NEON_CAMERA_HW_ERROR;
    b->setup_sequence = rd(b, R(COMMON_RECOVERY_SEQUENCE));
    wr(b, R(COMMON_CTRL), 0);
    wr(b, R(DMA_CTRL), M(DMA_CTRL_SOFT_RESET)); barrier(b);
    b->phase = FX1_SETUP_RECOVERY;
    return NEON_CAMERA_PENDING;
}
static neon_camera_result_t progress_setup(fx1_isp_backend_t *b)
{
    if (b->phase == FX1_SETUP_RECOVERY) {
        if (!b->recovery_valid || b->recovery_sequence != b->setup_sequence + 1u || !idle(b))
            return NEON_CAMERA_PENDING;
        /* A new generation may acknowledge residual sticky errors only after
         * copying them and proving quiescence. Direct writes, never blanket
         * clear or RMW mixed command/status registers. */
        uint32_t tune = rd(b, R(TUNE_ERROR)) & FX1_ISP_TUNE_ERROR_W1C_MASK;
        uint32_t dma = rd(b, R(DMA_ERR)) & FX1_ISP_DMA_ERR_W1C_MASK;
        uint32_t lsc = rd(b, R(LSC_ERROR)) & FX1_ISP_LSC_ERROR_W1C_MASK;
        uint32_t ee = rd(b, R(EE_STATUS)) & FX1_ISP_EE_STATUS_W1C_MASK;
        b->evidence.tune_error = tune; b->evidence.dma_error = dma;
        b->evidence.lsc_error = lsc; b->evidence.ee_status = ee;
        if (tune) wr(b, R(TUNE_ERROR), tune);
        if (dma) wr(b, R(DMA_ERR), dma);
        if (lsc) wr(b, R(LSC_ERROR), lsc);
        if (ee) wr(b, R(EE_STATUS), ee);
        wr(b, R(DMA_CTRL), 0); wr(b, R(TUNE_MODE), 0); barrier(b);
        b->phase = FX1_SETUP_WIDTH;
        return NEON_CAMERA_PENDING;
    }
    if (rd(b, R(TUNE_ERROR))) return NEON_CAMERA_HW_ERROR;
    if (rd(b, R(LSC_PROFILE_STATUS)) & M(LSC_PROFILE_STATUS_PREP_BUSY)) return NEON_CAMERA_PENDING;
    if (b->phase == FX1_SETUP_WIDTH) {
        wr(b, R(COMMON_FRAME_WIDTH), b->config.width); b->phase = FX1_SETUP_HEIGHT;
    } else if (b->phase == FX1_SETUP_HEIGHT) {
        wr(b, R(COMMON_FRAME_HEIGHT), b->config.height); b->phase = FX1_SETUP_REST;
    } else {
        wr(b, R(COMMON_BAYER), b->config.bayer_pattern);
        wr(b, R(COMMON_INPUT_FORMAT), 0); /* RAW12, same-CFA padding. */
        wr(b, R(INPUT_SRC_CTRL), 0); /* DDR replay, never stream fallback. */
        wr(b, R(CH0_CROP_CTRL), 0); wr(b, R(RESIZER_CTRL), 0);
        wr(b, R(OUTPUT_ENABLE), 1); wr(b, R(OFMT_CTRL), 0);
        wr(b, R(IDMA_STRIDE), b->config.raw_stride);
        wr(b, R(ODMA_Y_STRIDE), b->config.y_stride); wr(b, R(ODMA_UV_STRIDE), b->config.uv_stride);
        for (unsigned i = 0; i < 4; ++i) {
            const neon_isp_session_buffer_t *v = &b->config.buffers[i];
            wr(b, R(IDMA_BUF_ADDR_L0) + 8u * i, v->raw.address);
            wr(b, R(IDMA_BUF_ADDR_H0) + 8u * i, 0);
            wr(b, R(ODMA_Y_ADDR_L0) + 16u * i, v->y.address);
            wr(b, R(ODMA_Y_ADDR_H0) + 16u * i, 0);
            wr(b, R(ODMA_UV_ADDR_L0) + 16u * i, v->uv.address);
            wr(b, R(ODMA_UV_ADDR_H0) + 16u * i, 0);
        }
        wr(b, R(TUNE_MODE), M(TUNE_MODE_ENABLE)); barrier(b);
        b->phase = FX1_PROFILE_LOAD; b->bank = 0;
    }
    barrier(b);
    return NEON_CAMERA_PENDING;
}
/* Stable copy; never request another recovery while BUSY. Evidence persists
 * after DONE W1C, including across subsequent NO_DATA polls. */
static bool recovery_snapshot(fx1_isp_backend_t *b)
{
    uint32_t seq = rd(b, R(COMMON_RECOVERY_SEQUENCE));
    uint32_t s = rd(b, R(COMMON_RECOVERY_STATUS));
    b->evidence.recovery_live_status = s; b->evidence.recovery_live_sequence = seq;
    b->evidence.valid |= NEON_ISP_DIAG_RECOVERY_LIVE;
    if (s & M(COMMON_RECOVERY_STATUS_BUSY)) return false;
    if (!(s & M(COMMON_RECOVERY_STATUS_DONE))) return true;
    neon_isp_hw_snapshot_t copy = b->evidence;
    copy.recovery_status = s; copy.recovery_sequence = seq;
    copy.recovery_count = (s & M(COMMON_RECOVERY_STATUS_COUNT)) >> FX1_ISP_COMMON_RECOVERY_STATUS_COUNT_SHIFT;
    if (copy.recovery_count > 4) return false;
    for (unsigned i = 0; i < copy.recovery_count; ++i) {
        copy.recovery_records[i].ingress_frame = rd(b, R(COMMON_ABORT0_FRAME) + 8u * i);
        copy.recovery_records[i].outcome = rd(b, R(COMMON_ABORT0_OUTCOME) + 8u * i);
    }
    barrier(b);
    if (rd(b, R(COMMON_RECOVERY_SEQUENCE)) != seq || rd(b, R(COMMON_RECOVERY_STATUS)) != s) return false;
    copy.valid |= NEON_ISP_DIAG_RECOVERY;
    b->evidence = copy; b->recovery_sequence = seq; b->recovery_valid = true;
    wr(b, R(COMMON_RECOVERY_STATUS), M(COMMON_RECOVERY_STATUS_DONE)); barrier(b);
    return true;
}
static bool discard_stats(fx1_isp_backend_t *b)
{
    bool empty = true;
    const uint32_t frame[] = {R(TUNE_AEC_FRAME), R(AWB_FRAME_ID), R(TUNE_AF_FRAME)};
    const uint32_t profile[] = {R(TUNE_AEC_PROFILE), R(TUNE_AWB_PROFILE), R(TUNE_AF_PROFILE)};
    const uint32_t event_mask[] = {FX1_ISP_AEC_STATUS_W1C_MASK, FX1_ISP_AWB_STATUS_W1C_MASK, FX1_ISP_AF_STATUS_W1C_MASK};
    for (unsigned i = 0; i < 3; ++i) {
        b->evidence.stats_status[i] = rd(b, stat_event[i]);
        b->evidence.stats_drop[i] = rd(b, stat_drop[i]);
        /* Two banks: bounded drain. Discard intentionally copies identity, not
         * statistics payload. No algorithm is enabled on this profile. */
        for (unsigned n = 0; n < 2; ++n) {
            uint32_t s = rd(b, stat_owner[i]);
            uint32_t lease = i == 1 ? 4u : 1u, ready = i == 1 ? 3u : 6u;
            if (!(s & lease)) {
                if (!(s & ready)) break;
                wr(b, stat_cmd[i], 1); barrier(b); s = rd(b, stat_owner[i]);
                if (!(s & lease)) { empty = false; break; }
            }
            b->stats_frame[i] = rd(b, frame[i]); b->stats_profile[i] = rd(b, profile[i]);
            ++b->stats_discarded[i];
            wr(b, stat_cmd[i], 2); barrier(b);
        }
        if (rd(b, stat_owner[i]) & 7u) empty = false;
        uint32_t event = b->evidence.stats_status[i] & event_mask[i];
        if (event) wr(b, stat_event[i], event);
    }
    b->evidence.valid |= NEON_ISP_DIAG_STATS;
    return empty;
}
static void snapshot(fx1_isp_backend_t *b, fx1_isp_irq_bits_t irq)
{
    neon_isp_hw_snapshot_t *e = &b->evidence;
    e->valid |= NEON_ISP_DIAG_COMMON | NEON_ISP_DIAG_DMA | NEON_ISP_DIAG_OUTPUT | NEON_ISP_DIAG_TUNE;
    e->common_status = rd(b, R(COMMON_STATUS)); e->common_irq = irq.common;
    e->dma_status = rd(b, R(DMA_STAT)); e->dma_error = rd(b, R(DMA_ERR)); e->dma_irq = irq.dma;
    e->output_status[0] = rd(b, R(ODMA_BUF_DONE));
    e->output_status[1] = rd(b, R(CH1_ODMA_BUF_DONE)); e->output_status[2] = rd(b, R(CH2_ODMA_BUF_DONE));
    e->output_geometry[0] = rd(b, R(CH0_GEOMETRY_STATUS));
    e->output_geometry[1] = rd(b, R(CH1_GEOMETRY_STATUS));
    e->output_geometry[2] = rd(b, R(CH2_GEOMETRY_STATUS));
    e->output_irq = irq.output; e->tune_status = rd(b, R(TUNE_STATUS)); e->tune_error = rd(b, R(TUNE_ERROR));
    e->active_id = rd(b, R(TUNE_ACTIVE_ID)); e->active_frame = rd(b, R(TUNE_ACTIVE_FRAME));
    e->next_frame = rd(b, R(TUNE_NEXT_FRAME));
    if (b->capabilities & M(TUNE_CAPS_INPUT_STREAM)) {
        b->stream_error = rd(b, R(INPUT_STREAM_ERR)); b->stream_drops = rd(b, R(INPUT_STREAM_DROP_COUNT));
        e->valid |= NEON_ISP_DIAG_STREAM;
    } else {
        b->stream_error = b->stream_drops = 0; e->valid &= ~NEON_ISP_DIAG_STREAM;
    }
    b->lsc_error = rd(b, R(LSC_ERROR)); b->ee_status = rd(b, R(EE_STATUS));
    e->lsc_error = b->lsc_error; e->ee_status = b->ee_status;
    e->stream_error = b->stream_error; e->stream_drops = b->stream_drops;
    e->valid |= NEON_ISP_DIAG_AUX;
}
static neon_camera_result_t poll(void *device, neon_isp_session_observation_t *o)
{
    fx1_isp_backend_t *b = device;
    if (!b->owner) return NEON_CAMERA_INVALID;
    *o = (neon_isp_session_observation_t){0};
    fx1_isp_irq_bits_t irq = fx1_isp_irq_read(&b->port.io), served = irq;
    snapshot(b, irq);
    bool recovered = recovery_snapshot(b);
    bool stats_drained = discard_stats(b);
    if (!stats_drained) served.common &= ~M(COMMON_IRQ_STATUS_STATS_READY_IRQ);
    /* Classify separately: flow-control and stream/drop are diagnostics only;
     * AXI terminal recovery is automatic, tune rejection never issues reset. */
    bool fatal = (b->evidence.dma_error & (1u | 8u | 16u)) ||
        (b->evidence.tune_error && !(b->evidence.tune_status & M(TUNE_STATUS_CHECK_BUSY))) ||
        b->lsc_error || (b->ee_status & M(EE_STATUS_ERROR)) ||
        (b->recovery_valid && b->evidence.recovery_count != 0);
    if (b->phase == FX1_SETUP_RECOVERY) fatal = false; /* prior generation, copied before clear */
    if (fatal && !b->faulted) { b->faulted = true; o->flags |= NEON_ISP_OBS_FAULT; }
    if (!b->stopping && !b->faulted) {
        neon_camera_result_t r = b->phase >= FX1_SETUP_RECOVERY && b->phase <= FX1_SETUP_REST ?
            progress_setup(b) : fx1_isp_profile_progress(b, o);
        if (r == NEON_CAMERA_HW_ERROR) {
            b->faulted = true; o->flags |= NEON_ISP_OBS_FAULT;
            snapshot(b, irq); /* Keep rejection produced during this turn. */
        }
    }
    /* DONE is durable while consumer holds it. IRQ acknowledgement never clears
     * DONE or grants FREE. Metadata read only under successful retirement. */
    uint32_t done = rd(b, R(ODMA_BUF_DONE)) & 15u;
    uint32_t unseen = done & ~b->published;
    for (uint8_t i = 0; i < 4; ++i) if (unseen & (1u << i)) {
        if (!(b->submitted & (1u << i))) { b->faulted = true; o->flags |= NEON_ISP_OBS_FAULT; break; }
        neon_isp_frame_identity_t f = {.valid = NEON_ISP_ID_FRAME | NEON_ISP_ID_PROFILE | NEON_ISP_ID_OUTPUT_TIME,
            .ingress_frame = rd(b, R(TUNE_CH0_BUF0_FRAME) + 4u * i),
            .profile_id = rd(b, R(TUNE_CH0_BUF0_PROFILE) + 4u * i),
            .tag = rd(b, R(ODMA_BUF_TAG0) + 4u * i),
            .output_sof = rd(b, R(ODMA_BUF_SOF_TS0) + 4u * i),
            .output_eof = rd(b, R(ODMA_BUF_EOF_TS0) + 4u * i)};
        if (f.tag != b->tags[i]) { b->faulted = true; o->flags |= NEON_ISP_OBS_FAULT; break; }
        bool aborted = false;
        for (unsigned n = 0; n < 4; ++n)
            if ((b->published & (1u << n)) &&
                rd(b, R(TUNE_CH0_BUF0_FRAME) + 4u * n) == f.ingress_frame) aborted = true;
        if (b->recovery_valid) for (unsigned n = 0; n < b->evidence.recovery_count; ++n)
            aborted |= b->evidence.recovery_records[n].ingress_frame == f.ingress_frame;
        if (aborted) { b->faulted = true; o->flags |= NEON_ISP_OBS_FAULT; break; }
        const neon_isp_session_buffer_t *v = &b->config.buffers[i];
        b->port.complete(b->port.io.user, v->y.address, v->y.bytes);
        b->port.complete(b->port.io.user, v->uv.address, v->uv.bytes); barrier(b);
        o->flags |= NEON_ISP_OBS_OUTPUT; o->slot = i; o->output = f;
        b->published |= 1u << i; b->submitted &= ~(1u << i);
        break;
    }
    if (done & ~b->published) {
        served.output &= ~1u; served.dma &= ~(uint32_t)FX1_IRQ_OUTPUT_DONE;
        served.common &= ~M(COMMON_IRQ_STATUS_FRAME_DONE_IRQ);
    }
    /* Do not ACK recovery-related causes while its evidence is unstable. */
    if (!recovered) {
        served.common &= ~M(COMMON_IRQ_STATUS_ERROR_IRQ);
        served.dma &= ~(uint32_t)FX1_IRQ_AXI;
        served.output &= ~M(OUTPUT_IRQ_STATUS_ERROR);
    }
    if (!(rd(b, R(IDMA_BUF_VALID)) & (1u << b->head)))
        served.dma &= ~(uint32_t)FX1_IRQ_UNDERRUN;
    if (b->stopping && recovered && stats_drained) {
        if (!b->reset_requested && idle(b)) {
            b->stop_sequence = rd(b, R(COMMON_RECOVERY_SEQUENCE));
            /* CANCEL/admission race: record applied identity above; no new
             * admission from here. Recovery safely cancels remaining pending. */
            wr(b, R(TUNE_CMD), M(TUNE_CMD_CANCEL));
            wr(b, R(DMA_CTRL), M(DMA_CTRL_SOFT_RESET)); barrier(b);
            b->reset_requested = true;
        } else if (b->reset_requested && b->recovery_valid &&
                   b->recovery_sequence == b->stop_sequence + 1u && idle(b) && stats_empty(b) &&
                   !(rd(b, R(ODMA_BUF_DONE)) & ~b->published & 15u)) {
            wr(b, R(DMA_CTRL), 0); wr(b, R(COMMON_CTRL), 0); barrier(b);
            b->submitted = 0; b->phase = FX1_PROFILE_IDLE;
            o->flags |= NEON_ISP_OBS_DRAINED;
        }
    }
    o->hardware = b->evidence;
    fx1_isp_irq_ack(&b->port.io, irq, served);
    /* Always return the snapshot, including fault/recovery progress without IRQ.
     * The service bounds batches; zero flags is a diagnostic observation. */
    return o->flags || irq.common || (irq.dma & ~(uint32_t)FX1_IRQ_UNDERRUN) || irq.output ?
        NEON_CAMERA_OK : NEON_CAMERA_NO_DATA;
}
static neon_camera_result_t submit(void *device, uint8_t slot, uint32_t tag)
{
    fx1_isp_backend_t *b = device;
    if (!b->owner || slot >= 4) return NEON_CAMERA_INVALID;
    if (b->stopping || b->faulted || b->submitted || slot != b->head ||
        (b->phase && b->phase != FX1_PROFILE_WAIT)) return NEON_CAMERA_BUSY;
    if (rd(b, R(COMMON_RECOVERY_STATUS)) & M(COMMON_RECOVERY_STATUS_BUSY)) return NEON_CAMERA_BUSY;
    uint32_t bit = 1u << slot;
    if ((rd(b, R(ODMA_BUF_DONE)) | b->published) & bit) return NEON_CAMERA_BUSY;
    if ((rd(b, R(CH0_GEOMETRY_STATUS)) & (M(CH0_GEOMETRY_STATUS_VALID) | M(CH0_GEOMETRY_STATUS_BUSY))) !=
        M(CH0_GEOMETRY_STATUS_VALID)) return NEON_CAMERA_BUSY;
    if (rd(b, R(RESIZER_OUT_W)) != b->config.width || rd(b, R(RESIZER_OUT_H)) != b->config.height)
        return NEON_CAMERA_BUSY;
    const neon_isp_session_buffer_t *v = &b->config.buffers[slot];
    b->port.prepare(b->port.io.user, v->raw.address, v->raw.bytes, false);
    b->port.prepare(b->port.io.user, v->y.address, v->y.bytes, true);
    b->port.prepare(b->port.io.user, v->uv.address, v->uv.bytes, true);
    wr(b, R(IDMA_BUF_TAG0) + 4u * slot, tag);
    wr(b, R(IDMA_BUF_TS0) + 4u * slot, rd(b, R(FRAME_COUNTER)));
    barrier(b); wr(b, R(ODMA_BUF_FREE), bit); barrier(b);
    wr(b, R(COMMON_CTRL), M(COMMON_CTRL_ISP_EN));
    wr(b, R(IDMA_BUF_VALID), bit); barrier(b);
    /* Direct value, never RMW the SOFT_RESET command. */
    wr(b, R(DMA_CTRL), (63u << 8) | M(DMA_CTRL_IDMA_EN) | M(DMA_CTRL_ODMA_EN)); barrier(b);
    b->tags[slot] = tag; b->submitted |= bit; b->head = (uint8_t)((slot + 1u) % 4u);
    return NEON_CAMERA_OK;
}
static neon_camera_result_t release_output(void *device, uint8_t slot)
{
    fx1_isp_backend_t *b = device;
    if (!b->owner || slot >= 4 || !(b->published & (1u << slot))) return NEON_CAMERA_STALE;
    wr(b, R(ODMA_BUF_DONE), 1u << slot); barrier(b);
    if (rd(b, R(ODMA_BUF_DONE)) & (1u << slot)) return NEON_CAMERA_BUSY;
    b->published &= ~(1u << slot);
    /* FREE is deliberately deferred to submit of the rotation head, after cache
     * prepare. Returning an arbitrary consumer lease cannot arm that buffer. */
    return NEON_CAMERA_OK;
}
static neon_camera_result_t apply(void *device, const neon_isp_session_transaction_t *t)
{
    fx1_isp_backend_t *b = device;
    return b->owner ? fx1_isp_profile_queue(b, t) : NEON_CAMERA_INVALID;
}
static neon_camera_result_t stop(void *device)
{
    fx1_isp_backend_t *b = device;
    if (!b->owner) return NEON_CAMERA_INVALID;
    b->stopping = true;
    return NEON_CAMERA_PENDING;
}
neon_camera_result_t fx1_isp_backend_init(fx1_isp_backend_t *b, const fx1_isp_backend_port_t *p)
{
    if (!b || !p || !p->io.read || !p->io.write || !p->io.barrier ||
        !p->prepare || !p->complete || !p->attach || !p->detach) return NEON_CAMERA_INVALID;
    *b = (fx1_isp_backend_t){.port = *p};
    return NEON_CAMERA_OK;
}
const neon_isp_session_backend_t *fx1_isp_backend_ops(void)
{
    static const neon_isp_session_backend_t ops = {
        NEON_ISP_SESSION_ABI, NEON_ISP_SESSION_REQUIRED_CAPS,
        claim, release, begin, poll, submit, release_output, apply, stop};
    return &ops;
}
