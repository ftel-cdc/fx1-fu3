#include "test_common.h"
#include "neon_camera_runtime.h"
#include "isp/neon_isp_claim.h"
#include "isp/platform/fx1/fx1_isp_profile_txn.h"
#include "host/isp_runtime_mmio.h"
static fx1_isp_scalar_profile_t scalar = FX1_ISP_SCALAR_PROFILE_INITIALIZER;
static neon_camera_session_config_t configuration(fx1_isp_backend_t *b)
{
    neon_camera_session_config_t c = {.backend = fx1_isp_backend_ops(), .device = b,
        .baseline = {&scalar, sizeof(scalar), 7},
        .stream = {.width = 64, .height = 48, .raw_stride = 128, .y_stride = 64, .uv_stride = 64,
            .dma_alignment = 16, .raw_depth = 12}};
    for (unsigned i = 0; i < 4; ++i) {
        uint32_t base = 0x100000u + i * 0x10000u;
        c.stream.buffers[i] = (neon_isp_session_buffer_t){{base,6144},{base+0x2000u,3072},{base+0x4000u,1536}};
    }
    return c;
}
static void setup(neon_camera_session_t *s, fx1_isp_backend_t *b, isp_mmio_t *m)
{
    mmio_init(m); fx1_isp_backend_port_t p = mmio_port(m);
    CHECK_EQ(fx1_isp_backend_init(b, &p), NEON_CAMERA_OK);
    neon_camera_session_config_t c = configuration(b);
    CHECK_EQ(neon_camera_session_init(s, &c), NEON_CAMERA_OK);
    m->owner = s;
    CHECK_EQ(neon_camera_session_start(s), NEON_CAMERA_PENDING);
    mmio_recovery_done(m, 0);
    for (unsigned i = 0; i < 9; ++i) (void)neon_camera_session_poll(s);
    m->reset_requests = 0;
    CHECK_EQ(m->reg[R(TUNE_STATUS)/4] & M(TUNE_STATUS_PENDING), M(TUNE_STATUS_PENDING));
    CHECK_EQ(s->status.state, NEON_CAMERA_STARTING);
}
static void shutdown_session(neon_camera_session_t *s, isp_mmio_t *m)
{
    CHECK_EQ(neon_camera_session_stop(s), NEON_CAMERA_PENDING);
    for (unsigned n = 0; n < 3; ++n) (void)neon_camera_session_poll(s);
    for (unsigned n = 0; n < 4; ++n) {
        mmio_recovery_done(m, 0); (void)neon_camera_session_poll(s);
    }
    neon_camera_frame_t f;
    while (neon_camera_session_acquire(s, &f) == NEON_CAMERA_OK)
        CHECK_EQ(neon_camera_session_return(s, &f), NEON_CAMERA_OK);
    CHECK(!s->status.claimed);
    CHECK_EQ(m->detaches, m->attaches);
}
static void irq_w1c_and_set_wins(void)
{
    isp_mmio_t m; mmio_init(&m); fx1_isp_backend_port_t p = mmio_port(&m);
    m.reg[R(COMMON_IRQ_STATUS)/4] = 7; m.reg[R(DMA_IRQ_STAT)/4] = 255; m.reg[R(OUTPUT_IRQ_STATUS)/4] = 0x77;
    fx1_isp_irq_gate(&p.io);
    CHECK_EQ(m.reg[R(COMMON_IRQ_STATUS)/4], 7);
    fx1_isp_irq_bits_t c = fx1_isp_irq_read(&p.io);
    m.set_offset = R(DMA_IRQ_STAT); m.set_bits = FX1_IRQ_OUTPUT_DONE;
    fx1_isp_irq_ack(&p.io, c, (fx1_isp_irq_bits_t){1,FX1_IRQ_OUTPUT_DONE | FX1_IRQ_STREAM,1});
    CHECK_EQ(m.ack_common, 1); CHECK_EQ(m.ack_dma, FX1_IRQ_OUTPUT_DONE | FX1_IRQ_STREAM); CHECK_EQ(m.ack_output, 1);
    CHECK_EQ(m.reg[R(DMA_IRQ_STAT)/4], 127);
    CHECK_EQ(m.reg[R(COMMON_IRQ_STATUS)/4], 6);
    CHECK(fx1_isp_irq_rearm(&p.io, fx1_isp_irq_replay_masks(FX1_ISP_TUNE_CAPS_RO_MASK)));
    CHECK_EQ(m.reg[R(COMMON_IRQ_EN)/4], 0); CHECK_EQ(m.reg[R(DMA_IRQ_EN)/4], 0);
    m.reg[R(COMMON_IRQ_STATUS)/4] = m.reg[R(DMA_IRQ_STAT)/4] = m.reg[R(OUTPUT_IRQ_STATUS)/4] = 0;
    m.arm_offset = R(OUTPUT_IRQ_STATUS); m.arm_bits = 1;
    CHECK(fx1_isp_irq_rearm(&p.io, fx1_isp_irq_replay_masks(FX1_ISP_TUNE_CAPS_RO_MASK)));
    m.reg[R(OUTPUT_IRQ_STATUS)/4] = 0;
    CHECK(!fx1_isp_irq_rearm(&p.io, (fx1_isp_irq_bits_t){7,255,0x77}));
    CHECK_EQ(m.reg[R(DMA_IRQ_EN)/4] & FX1_IRQ_UNDERRUN, 0);
    CHECK(m.reg[R(DMA_IRQ_EN)/4] & FX1_IRQ_STREAM);
    /* An unobserved late bit must never be acknowledged by an old snapshot. */
    fx1_isp_irq_ack(&p.io, (fx1_isp_irq_bits_t){0,0,0}, (fx1_isp_irq_bits_t){UINT32_MAX,UINT32_MAX,UINT32_MAX});
    CHECK_EQ(m.ack_dma, FX1_IRQ_OUTPUT_DONE | FX1_IRQ_STREAM);
}
static void profile_rotation_and_leases(void)
{
    neon_camera_session_t s = {0}; fx1_isp_backend_t b; isp_mmio_t m;
    setup(&s, &b, &m);
    neon_camera_frame_t held[4];
    for (uint8_t i = 0; i < 4; ++i) {
        CHECK_EQ(neon_camera_session_submit(&s, 500u+i), NEON_CAMERA_OK);
        mmio_admit(&m); (void)neon_camera_session_poll(&s);
        mmio_retire(&m, i, 1u+i, 7);
        CHECK_EQ(neon_camera_session_poll(&s), NEON_CAMERA_OK);
        CHECK_EQ(neon_camera_session_acquire(&s, &held[i]), NEON_CAMERA_OK);
        CHECK_EQ(held[i].identity.ingress_frame, 1u+i);
        CHECK_EQ(held[i].identity.profile_id, 7);
        CHECK_EQ(neon_camera_timestamp_span(held[i].identity.output_sof, held[i].identity.output_eof), 4);
        CHECK_EQ(neon_camera_session_poll(&s), NEON_CAMERA_NO_DATA);
        CHECK_EQ(s.status.retired, 1u+i);
    }
    CHECK_EQ(m.completed, 8); CHECK_EQ(m.prepared, 12);
    CHECK_EQ(m.reg[R(ODMA_BUF_DONE)/4], 15);
    CHECK_EQ(neon_camera_session_submit(&s, 99), NEON_CAMERA_BUSY);
    /* Returning slot 1 cannot skip consumer-held head 0. */
    CHECK_EQ(neon_camera_session_return(&s, &held[1]), NEON_CAMERA_OK);
    CHECK_EQ(neon_camera_session_submit(&s, 99), NEON_CAMERA_BUSY);
    CHECK_EQ(neon_camera_session_return(&s, &held[0]), NEON_CAMERA_OK);
    CHECK_EQ(neon_camera_session_return(&s, &held[0]), NEON_CAMERA_STALE);
    CHECK_EQ(neon_camera_session_return(&s, &held[2]), NEON_CAMERA_OK);
    CHECK_EQ(neon_camera_session_return(&s, &held[3]), NEON_CAMERA_OK);
    neon_isp_session_transaction_t t = {{&scalar,sizeof(scalar),8},NEON_ISP_APPLY_NEXT,0};
    CHECK_EQ(neon_camera_session_apply(&s, &t, 5), NEON_CAMERA_PENDING);
    for (unsigned n = 0; n < 4; ++n) (void)neon_camera_session_poll(&s);
    CHECK_EQ(neon_camera_session_submit(&s, 99), NEON_CAMERA_OK);
    mmio_admit(&m); CHECK_EQ(neon_camera_session_poll(&s), NEON_CAMERA_OK);
    CHECK(!s.status.profile_pending);
    mmio_retire(&m, 0, 5, 8); (void)neon_camera_session_poll(&s);
    shutdown_session(&s, &m);
    CHECK_EQ(neon_camera_session_start(&s), NEON_CAMERA_PENDING);
    mmio_recovery_done(&m, 0);
    for (unsigned n = 0; n < 9; ++n) (void)neon_camera_session_poll(&s);
    CHECK_EQ(s.cursor, 0); CHECK_EQ(neon_camera_session_return(&s, &held[0]), NEON_CAMERA_STALE);
    shutdown_session(&s, &m); /* Second detach expected separately. */
}
static void fault_preserves_done_and_stable_records(void)
{
    neon_camera_session_t s = {0}; fx1_isp_backend_t b; isp_mmio_t m;
    setup(&s, &b, &m); CHECK_EQ(neon_camera_session_submit(&s, 11), NEON_CAMERA_OK);
    mmio_admit(&m); (void)neon_camera_session_poll(&s); mmio_retire(&m, 0, 1, 7);
    m.reg[R(DMA_ERR)/4] = 8; m.reg[R(DMA_IRQ_STAT)/4] |= FX1_IRQ_AXI;
    m.reg[R(COMMON_IRQ_STATUS)/4] |= 2; m.reg[R(COMMON_RECOVERY_SEQUENCE)/4] = UINT32_MAX;
    m.reg[R(COMMON_RECOVERY_STATUS)/4] = M(COMMON_RECOVERY_STATUS_BUSY);
    CHECK_EQ(neon_camera_session_poll(&s), NEON_CAMERA_HW_ERROR);
    CHECK_EQ(s.status.retired, 1); CHECK_EQ(s.status.state, NEON_CAMERA_FAULTED);
    neon_camera_frame_t f; CHECK_EQ(neon_camera_session_acquire(&s, &f), NEON_CAMERA_OK);
    CHECK_EQ(f.identity.ingress_frame, 1); CHECK_EQ(m.reset_requests, 0);
    CHECK_EQ(neon_camera_session_stop(&s), NEON_CAMERA_PENDING);
    for (unsigned n = 0; n < 10; ++n) (void)neon_camera_session_poll(&s);
    CHECK_EQ(m.reset_requests, 0); CHECK(s.status.claimed);
    m.reg[R(COMMON_ABORT0_FRAME)/4] = 2; m.reg[R(COMMON_ABORT0_OUTCOME)/4] = 0x101;
    mmio_recovery_done(&m, 1); m.unstable_sequence = true;
    (void)neon_camera_session_poll(&s);
    CHECK(s.status.hardware.recovery_sequence != UINT32_MAX); /* Previous stable copy survives. */
    CHECK_EQ(m.reset_requests, 0);
    m.unstable_sequence = false; m.reg[R(COMMON_RECOVERY_SEQUENCE)/4] = UINT32_MAX;
    (void)neon_camera_session_poll(&s);
    CHECK_EQ(s.status.hardware.recovery_records[0].ingress_frame, 2);
    CHECK_EQ(m.reset_requests, 1); CHECK_EQ(m.reg[R(COMMON_RECOVERY_SEQUENCE)/4], 0);
    mmio_recovery_done(&m, 0); (void)neon_camera_session_poll(&s);
    CHECK(s.status.claimed); CHECK(s.status.drained);
    CHECK_EQ(neon_camera_session_return(&s, &f), NEON_CAMERA_OK);
    CHECK(!s.status.claimed); CHECK_EQ(m.detaches, 1);
}
static void stats_discard_and_timeout(void)
{
    neon_camera_session_t s = {0}; fx1_isp_backend_t b; isp_mmio_t m;
    setup(&s, &b, &m);
    m.ready[0]=2; m.ready[1]=1; m.ready[2]=2;
    m.reg[R(TUNE_AEC_STATUS)/4]=4; m.reg[R(AWB_RESULT_STATUS)/4]=1; m.reg[R(TUNE_AF_STATUS)/4]=4;
    m.reg[R(TUNE_AEC_FRAME)/4]=20; m.reg[R(TUNE_AF_FRAME)/4]=22;
    m.reg[R(AWB_FRAME_ID)/4]=21; m.reg[R(COMMON_IRQ_STATUS)/4]=4;
    CHECK_EQ(neon_camera_session_poll(&s), NEON_CAMERA_OK);
    CHECK_EQ(b.stats_discarded[0],2); CHECK_EQ(b.stats_discarded[1],1); CHECK_EQ(b.stats_discarded[2],2);
    CHECK_EQ(b.stats_frame[0],20); CHECK_EQ(b.stats_frame[1],21); CHECK_EQ(b.stats_frame[2],22);
    CHECK_EQ(m.releases[0],2); CHECK_EQ(m.releases[1],1); CHECK_EQ(m.releases[2],2);
    (void)neon_camera_session_poll(&s); CHECK_EQ(b.stats_discarded[0],2); CHECK_EQ(s.status.retired,0);
    CHECK_EQ(neon_camera_session_submit(&s,33),NEON_CAMERA_OK);
    CHECK_EQ(neon_camera_session_timeout(&s),NEON_CAMERA_TIMEOUT); CHECK(s.status.claimed);
    CHECK_EQ(s.buffers[0],NEON_CAMERA_BUFFER_QUARANTINED);
    CHECK(!neon_isp_device_claim(&m));
    /* Timeout is not drain; backend keeps outstanding IDMA_VALID until real
     * completion or recovery. Supply an authoritative automatic abort. */
    CHECK_EQ(neon_camera_session_stop(&s),NEON_CAMERA_PENDING);
    (void)neon_camera_session_poll(&s); CHECK_EQ(m.reset_requests,0);
    ++m.reg[R(COMMON_RECOVERY_SEQUENCE)/4];
    mmio_recovery_done(&m,1); m.reg[R(COMMON_ABORT0_FRAME)/4]=1;
    (void)neon_camera_session_poll(&s); CHECK_EQ(s.status.retired,0);
    mmio_recovery_done(&m,0); (void)neon_camera_session_poll(&s); CHECK(!s.status.claimed);
}
static void capability_and_claim_exclusion(void)
{
    neon_camera_session_t s={0}; fx1_isp_backend_t b; isp_mmio_t m; mmio_init(&m);
    fx1_isp_backend_port_t p=mmio_port(&m); CHECK_EQ(fx1_isp_backend_init(&b,&p),NEON_CAMERA_OK);
    neon_camera_session_config_t c=configuration(&b); CHECK_EQ(neon_camera_session_init(&s,&c),NEON_CAMERA_OK);
    CHECK(neon_isp_device_claim(&m)); CHECK_EQ(neon_camera_session_start(&s),NEON_CAMERA_BUSY);
    CHECK_EQ(m.writes,0); CHECK(neon_isp_device_release(&m));
    m.reg[R(TUNE_CAPS)/4] &= ~M(TUNE_CAPS_FRAME_ID);
    CHECK_EQ(neon_camera_session_start(&s),NEON_CAMERA_UNSUPPORTED); CHECK_EQ(m.writes,0); CHECK(!s.status.claimed);
    /* RAW replay doesn't require INPUT_STREAM capability. */
    m.reg[R(TUNE_CAPS)/4] = FX1_ISP_TUNE_CAPS_RO_MASK & ~M(TUNE_CAPS_INPUT_STREAM);
    CHECK_EQ(neon_camera_session_start(&s),NEON_CAMERA_PENDING);
    CHECK(!neon_isp_device_claim(&m));
    shutdown_session(&s,&m);
}
static void preparation_and_nonterminal_sources(void)
{
    neon_camera_session_t s={0}; fx1_isp_backend_t b; isp_mmio_t m; mmio_init(&m);
    fx1_isp_backend_port_t p=mmio_port(&m); CHECK_EQ(fx1_isp_backend_init(&b,&p),NEON_CAMERA_OK);
    neon_camera_session_config_t c=configuration(&b); CHECK_EQ(neon_camera_session_init(&s,&c),NEON_CAMERA_OK);
    CHECK_EQ(neon_camera_session_start(&s),NEON_CAMERA_PENDING);
    /* Startup pointer recovery may take arbitrary time without another IRQ. */
    for (unsigned n=0;n<8;++n) (void)neon_camera_session_poll(&s);
    CHECK_EQ(m.reset_requests,1); CHECK_EQ(m.reg[R(COMMON_FRAME_WIDTH)/4],0);
    mmio_recovery_done(&m,0); (void)neon_camera_session_poll(&s);
    (void)neon_camera_session_poll(&s); CHECK_EQ(m.reg[R(COMMON_FRAME_WIDTH)/4],64);
    m.reg[R(LSC_PROFILE_STATUS)/4]=M(LSC_PROFILE_STATUS_PREP_BUSY);
    unsigned writes=m.writes;
    for (unsigned n=0;n<8;++n) (void)neon_camera_session_poll(&s);
    CHECK_EQ(m.writes,writes); CHECK_EQ(m.reg[R(COMMON_FRAME_HEIGHT)/4],0);
    m.reg[R(LSC_PROFILE_STATUS)/4]=0;
    for (unsigned n=0;n<8;++n) (void)neon_camera_session_poll(&s);
    CHECK_EQ(m.reg[R(COMMON_FRAME_HEIGHT)/4],48);
    m.reg[R(DMA_ERR)/4]=2u|4u|32u;
    m.reg[R(DMA_IRQ_STAT)/4]=FX1_IRQ_UNDERRUN|FX1_IRQ_OVERFLOW|FX1_IRQ_STREAM|FX1_IRQ_INPUT_DONE;
    m.reg[R(INPUT_STREAM_ERR)/4]=M(INPUT_STREAM_ERR_DROP);
    m.reg[R(INPUT_STREAM_DROP_COUNT)/4]=3;
    m.reg[R(COMMON_IRQ_STATUS)/4]=1; /* Frame IRQ alone is never OUTPUT. */
    CHECK_EQ(neon_camera_session_poll(&s),NEON_CAMERA_OK);
    CHECK_EQ(s.status.retired,0); CHECK_EQ(s.status.state,NEON_CAMERA_STARTING);
    CHECK_EQ(s.status.hardware.stream_drops,3); CHECK_EQ(s.status.hardware.stream_error,M(INPUT_STREAM_ERR_DROP));
    CHECK_EQ(m.reg[R(DMA_IRQ_STAT)/4],FX1_IRQ_UNDERRUN);
    /* Masked stats status still gets serviced without STATS_READY/any IRQ. */
    m.ready[2]=1; m.reg[R(TUNE_AF_STATUS)/4]=2;
    (void)neon_camera_session_poll(&s); CHECK_EQ(b.stats_discarded[2],1); CHECK_EQ(s.status.retired,0);
    shutdown_session(&s,&m);
}
static void seal_error_waits_for_check_completion(void)
{
    neon_camera_session_t s={0}; fx1_isp_backend_t b; isp_mmio_t m; mmio_init(&m);
    fx1_isp_backend_port_t p=mmio_port(&m); CHECK_EQ(fx1_isp_backend_init(&b,&p),NEON_CAMERA_OK);
    neon_camera_session_config_t c=configuration(&b); CHECK_EQ(neon_camera_session_init(&s,&c),NEON_CAMERA_OK);
    CHECK_EQ(neon_camera_session_start(&s),NEON_CAMERA_PENDING); mmio_recovery_done(&m,0);
    for (unsigned n=0;n<10 && b.phase!=FX1_PROFILE_SEAL;++n) (void)neon_camera_session_poll(&s);
    CHECK_EQ(b.phase,FX1_PROFILE_SEAL);
    m.reg[R(TUNE_STATUS)/4] |= M(TUNE_STATUS_CHECK_BUSY);
    m.reg[R(TUNE_ERROR)/4]=M(TUNE_ERROR_INVALID);
    (void)neon_camera_session_poll(&s);
    CHECK_EQ(s.status.state,NEON_CAMERA_STARTING); CHECK_EQ(s.status.last_error,NEON_CAMERA_OK);
    CHECK_EQ(m.reset_requests,1); /* Startup reset only. No tune rejection reset. */
    m.reg[R(TUNE_STATUS)/4] &= ~M(TUNE_STATUS_CHECK_BUSY);
    CHECK_EQ(neon_camera_session_poll(&s),NEON_CAMERA_HW_ERROR);
    CHECK_EQ(s.status.hardware.tune_error,M(TUNE_ERROR_INVALID)); CHECK_EQ(m.reset_requests,1);
    shutdown_session(&s,&m);
}
int main(void)
{
    RUN(irq_w1c_and_set_wins); RUN(profile_rotation_and_leases);
    RUN(fault_preserves_done_and_stable_records); RUN(stats_discard_and_timeout); RUN(capability_and_claim_exclusion); RUN(preparation_and_nonterminal_sources); RUN(seal_error_waits_for_check_completion);
    return TEST_REPORT();
}
