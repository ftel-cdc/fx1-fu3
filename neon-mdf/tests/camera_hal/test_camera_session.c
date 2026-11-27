/* Session policy tests with an explicit fake backend, not an MAS register oracle. */
#include "neon_camera_runtime.h"
#include "isp/neon_isp_claim.h"
#include "test_common.h"

typedef struct {
    neon_isp_session_observation_t observation;
    neon_camera_result_t begin_result, submit_result, release_result, apply_result;
    unsigned polls, releases, submits, applies, stops;
    uint8_t slot;
    uint32_t tag;
} fake_t;
static const uint32_t profile = 123;
static neon_camera_result_t claim(void *d, const void *owner)
{ (void)d; return neon_isp_device_claim(owner) ? NEON_CAMERA_OK : NEON_CAMERA_BUSY; }
static void release(void *d, const void *owner)
{ ++((fake_t *)d)->releases; CHECK(neon_isp_device_release(owner)); }
static neon_camera_result_t begin(void *d, const neon_isp_session_config_t *c, const neon_isp_session_profile_t *p)
{ (void)c; (void)p; return ((fake_t *)d)->begin_result; }
static neon_camera_result_t poll(void *d, neon_isp_session_observation_t *o)
{
    fake_t *f = d; ++f->polls;
    if (!f->observation.flags) return NEON_CAMERA_NO_DATA;
    *o = f->observation; f->observation = (neon_isp_session_observation_t){0}; return NEON_CAMERA_OK;
}
static neon_camera_result_t submit(void *d, uint8_t slot, uint32_t tag)
{ fake_t *f = d; ++f->submits; f->slot = slot; f->tag = tag; return f->submit_result; }
static neon_camera_result_t release_output(void *d, uint8_t slot)
{ (void)slot; return ((fake_t *)d)->release_result; }
static neon_camera_result_t apply(void *d, const neon_isp_session_transaction_t *t)
{ fake_t *f = d; (void)t; ++f->applies; return f->apply_result; }
static neon_camera_result_t stop(void *d)
{ ++((fake_t *)d)->stops; return NEON_CAMERA_PENDING; }
static const neon_isp_session_backend_t backend = {
    .abi = NEON_ISP_SESSION_ABI, .capabilities = NEON_ISP_SESSION_REQUIRED_CAPS,
    .claim = claim, .release = release, .begin = begin, .poll = poll,
    .submit = submit, .release_output = release_output, .apply = apply, .stop = stop
};
static neon_camera_session_config_t config(fake_t *f)
{
    neon_camera_session_config_t c = {.device = f, .backend = &backend,
        .baseline = {&profile, sizeof(profile), 7},
        .stream = {.width = 64, .height = 48, .raw_depth = 12, .dma_alignment = 16,
            .raw_stride = 128, .y_stride = 64, .uv_stride = 64}};
    for (unsigned i = 0; i < 4; ++i) {
        uint32_t base = 0x100000u + i * 0x10000u;
        c.stream.buffers[i] = (neon_isp_session_buffer_t){
            .raw = {base, 6144}, .y = {base + 0x2000u, 3072}, .uv = {base + 0x4000u, 1536}};
    }
    return c;
}
static void setup(neon_camera_session_t *s, fake_t *f)
{
    neon_camera_session_config_t c = config(f);
    CHECK_EQ(neon_camera_session_init(s, &c), NEON_CAMERA_OK);
    CHECK_EQ(neon_camera_session_start(s), NEON_CAMERA_PENDING);
}
static void activate(neon_camera_session_t *s, fake_t *f, uint32_t id, uint32_t frame)
{
    f->observation = (neon_isp_session_observation_t){.flags = NEON_ISP_OBS_APPLIED,
        .applied_id = id, .applied_frame = frame};
    CHECK_EQ(neon_camera_session_poll(s), NEON_CAMERA_OK);
}
static void complete(neon_camera_session_t *s, fake_t *f, uint32_t frame, uint32_t id)
{
    f->observation = (neon_isp_session_observation_t){.flags = NEON_ISP_OBS_OUTPUT,
        .slot = f->slot, .output = {.valid = NEON_ISP_ID_FRAME | NEON_ISP_ID_PROFILE,
            .ingress_frame = frame, .profile_id = id, .tag = f->tag}};
    CHECK_EQ(neon_camera_session_poll(s), NEON_CAMERA_OK);
}
static void drain(neon_camera_session_t *s, fake_t *f, neon_camera_result_t expected)
{
    CHECK_EQ(neon_camera_session_stop(s), NEON_CAMERA_PENDING);
    f->observation.flags = NEON_ISP_OBS_DRAINED;
    CHECK_EQ(neon_camera_session_poll(s), expected);
}
static void validation(void)
{
    fake_t f = {0}; neon_camera_session_config_t c = config(&f), bad;
    neon_isp_session_backend_t b = backend;
    CHECK_EQ(neon_camera_session_validate(&c), NEON_CAMERA_OK);
    bad = c; bad.stream.width = 63; CHECK_EQ(neon_camera_session_validate(&bad), NEON_CAMERA_INVALID);
    bad = c; bad.stream.raw_stride = UINT32_MAX; CHECK_EQ(neon_camera_session_validate(&bad), NEON_CAMERA_INVALID);
    bad = c; bad.stream.buffers[3].uv.address = c.stream.buffers[0].raw.address;
    CHECK_EQ(neon_camera_session_validate(&bad), NEON_CAMERA_INVALID);
    bad = c; bad.stream.buffers[2].raw.address = 0xfffffff0u;
    CHECK_EQ(neon_camera_session_validate(&bad), NEON_CAMERA_INVALID);
    bad = c; bad.stream.buffers[1].y.bytes = 1;
    CHECK_EQ(neon_camera_session_validate(&bad), NEON_CAMERA_INVALID);
    bad = c; bad.stream.raw_depth = 10; CHECK_EQ(neon_camera_session_validate(&bad), NEON_CAMERA_UNSUPPORTED);
    b.capabilities = 0; bad = c; bad.backend = &b;
    CHECK_EQ(neon_camera_session_validate(&bad), NEON_CAMERA_UNSUPPORTED);
    b = backend; b.poll = NULL;
    CHECK_EQ(neon_camera_session_validate(&bad), NEON_CAMERA_INVALID);
    CHECK_EQ(neon_camera_target_validate(NEON_ISP_APPLY_EXACT, 0, UINT32_MAX, true), NEON_CAMERA_OK);
    CHECK_EQ(neon_camera_target_validate(NEON_ISP_APPLY_EXACT, 0x80000000u, 0, true), NEON_CAMERA_INVALID);
    CHECK_EQ(neon_camera_target_validate(NEON_ISP_APPLY_EXACT, UINT32_MAX, 0, true), NEON_CAMERA_INVALID);
    CHECK_EQ(neon_camera_target_validate(NEON_ISP_APPLY_EXACT, 2, 1, false), NEON_CAMERA_INVALID);
    CHECK_EQ(neon_camera_target_validate(NEON_ISP_APPLY_EXACT, 1, 1, false), NEON_CAMERA_OK);
    CHECK_EQ(neon_camera_timestamp_span(UINT32_MAX - 1u, 2), 4);
}
static void claim_and_baseline(void)
{
    neon_camera_session_t s = {0}, other = {0}; fake_t f = {0}, g = {0};
    neon_camera_session_config_t c = config(&g);
    setup(&s, &f);
    CHECK_EQ(s.status.state, NEON_CAMERA_STARTING);
    CHECK_EQ(neon_camera_session_init(&other, &c), NEON_CAMERA_OK);
    CHECK_EQ(neon_camera_session_start(&other), NEON_CAMERA_BUSY);
    CHECK(!neon_isp_device_claim(&other));
    CHECK(!neon_isp_device_release(&other));
    CHECK_EQ(neon_camera_session_init(&s, &c), NEON_CAMERA_BUSY);
    /* Ingress must be allowed before baseline activation: baseline needs SOF. */
    CHECK_EQ(neon_camera_session_submit(&s, 77), NEON_CAMERA_OK);
    CHECK_EQ(neon_camera_session_submit(&s, 88), NEON_CAMERA_BUSY);
    activate(&s, &f, 7, 0xffffffffu);
    CHECK_EQ(s.status.state, NEON_CAMERA_RUNNING);
    drain(&s, &f, NEON_CAMERA_OK);
    CHECK_EQ(f.releases, 1);
    CHECK_EQ(s.status.retired, 0); /* Aborted/discarded work is never successful output. */
    CHECK_EQ(neon_camera_session_start(&other), NEON_CAMERA_PENDING);
    drain(&other, &g, NEON_CAMERA_OK);
}
static void leases_and_history(void)
{
    neon_camera_session_t s = {0}; fake_t f = {0};
    neon_camera_frame_t held[4], out, old;
    setup(&s, &f); activate(&s, &f, 7, 1);
    for (uint32_t n = 0; n < 4; ++n) {
        CHECK_EQ(neon_camera_session_submit(&s, 100u + n), NEON_CAMERA_OK);
        CHECK_EQ(f.slot, n);
        complete(&s, &f, n, 7);
        CHECK_EQ(neon_camera_session_acquire(&s, &held[n]), NEON_CAMERA_OK);
        CHECK_EQ(held[n].identity.ingress_frame, n); /* 0 is valid at wrap. */
        CHECK(!held[n].observed_applied_valid);
        CHECK_EQ(held[n].effective.valid, 0);
    }
    CHECK_EQ(neon_camera_session_submit(&s, 9), NEON_CAMERA_BUSY);
    old = held[0];
    CHECK_EQ(neon_camera_session_return(&s, &held[0]), NEON_CAMERA_OK);
    CHECK_EQ(neon_camera_session_return(&s, &held[0]), NEON_CAMERA_STALE);
    /* Many rotations must preserve a consumer-held record outside trace capacity. */
    for (uint32_t n = 4; n < 24; ++n) {
        CHECK_EQ(neon_camera_session_submit(&s, n + 100u), NEON_CAMERA_OK);
        complete(&s, &f, n, 3); /* Output carries old profile independent of activation. */
        CHECK_EQ(neon_camera_session_acquire(&s, &out), NEON_CAMERA_OK);
        CHECK_EQ(out.identity.profile_id, 3);
        CHECK_EQ(neon_camera_session_return(&s, &out), NEON_CAMERA_OK);
    }
    CHECK_EQ(neon_camera_session_history(&s, s.status.generation, 1, &out), NEON_CAMERA_OK);
    CHECK_EQ(out.lease, held[1].lease);
    drain(&s, &f, NEON_CAMERA_PENDING);
    CHECK_EQ(f.releases, 0); /* Drained hardware does not revoke consumer leases. */
    for (unsigned i = 1; i < 4; ++i) CHECK_EQ(neon_camera_session_return(&s, &held[i]), NEON_CAMERA_OK);
    CHECK_EQ(f.releases, 1);
    CHECK_EQ(neon_camera_session_start(&s), NEON_CAMERA_PENDING);
    CHECK_EQ(neon_camera_session_return(&s, &old), NEON_CAMERA_STALE);
    drain(&s, &f, NEON_CAMERA_OK);
}
static void pending_and_timeout(void)
{
    neon_camera_session_t s = {0}; fake_t f = {0}; neon_camera_frame_t out;
    neon_isp_session_transaction_t t = {.profile = {&profile, sizeof(profile), 11},
        .mode = NEON_ISP_APPLY_EXACT, .target_frame = 0};
    setup(&s, &f); activate(&s, &f, 7, UINT32_MAX);
    CHECK_EQ(neon_camera_session_apply(&s, &t, UINT32_MAX), NEON_CAMERA_PENDING);
    CHECK_EQ(neon_camera_session_apply(&s, &t, UINT32_MAX), NEON_CAMERA_BUSY);
    CHECK_EQ(f.applies, 1);
    activate(&s, &f, 11, 0);
    CHECK(!s.status.profile_pending);
    CHECK_EQ(neon_camera_session_submit(&s, 4), NEON_CAMERA_OK);
    CHECK_EQ(neon_camera_session_timeout(&s), NEON_CAMERA_TIMEOUT);
    CHECK_EQ(s.status.state, NEON_CAMERA_FAULTED);
    CHECK_EQ(s.buffers[f.slot], NEON_CAMERA_BUFFER_QUARANTINED);
    CHECK_EQ(neon_camera_session_acquire(&s, &out), NEON_CAMERA_NO_DATA);
    CHECK_EQ(neon_camera_session_submit(&s, 5), NEON_CAMERA_BUSY);
    CHECK_EQ(f.releases, 0);
    drain(&s, &f, NEON_CAMERA_OK);
    CHECK_EQ(f.releases, 1);
    CHECK_EQ(s.status.last_error, NEON_CAMERA_TIMEOUT);
}
static void unsupported_and_bad_identity(void)
{
    neon_camera_session_t s = {0}; fake_t f = {.begin_result = NEON_CAMERA_UNSUPPORTED};
    neon_camera_session_config_t c = config(&f);
    CHECK_EQ(neon_camera_session_init(&s, &c), NEON_CAMERA_OK);
    CHECK_EQ(neon_camera_session_start(&s), NEON_CAMERA_UNSUPPORTED);
    CHECK(!s.status.claimed); CHECK_EQ(s.status.state, NEON_CAMERA_CONFIGURED);
    f.begin_result = NEON_CAMERA_OK;
    CHECK_EQ(neon_camera_session_start(&s), NEON_CAMERA_PENDING);
    activate(&s, &f, 7, 1);
    CHECK_EQ(neon_camera_session_submit(&s, 5), NEON_CAMERA_OK);
    f.observation = (neon_isp_session_observation_t){.flags = NEON_ISP_OBS_OUTPUT,
        .slot = f.slot, .output = {.valid = NEON_ISP_ID_PROFILE, .tag = 5},
        .hardware = {.valid = NEON_ISP_DIAG_DMA, .dma_error = 0x21}};
    CHECK_EQ(neon_camera_session_poll(&s), NEON_CAMERA_HW_ERROR);
    CHECK_EQ(s.status.hardware.dma_error, 0x21);
    CHECK_EQ(s.status.retired, 0);
    CHECK(s.status.claimed);
    drain(&s, &f, NEON_CAMERA_OK);
}
static void ready_stop_and_failed_return(void)
{
    neon_camera_session_t s = {0}; fake_t f = {0}; neon_camera_frame_t out;
    setup(&s, &f); activate(&s, &f, 7, 1);
    CHECK_EQ(neon_camera_session_poll(&s), NEON_CAMERA_NO_DATA);
    CHECK_EQ(neon_camera_session_poll(&s), NEON_CAMERA_NO_DATA);
    CHECK_EQ(s.status.retired, 0); /* Wakeups cannot manufacture completions. */
    CHECK_EQ(neon_camera_session_submit(&s, 8), NEON_CAMERA_OK);
    complete(&s, &f, 1, 7);
    drain(&s, &f, NEON_CAMERA_PENDING);
    CHECK_EQ(s.status.ready, 1);
    CHECK_EQ(neon_camera_session_acquire(&s, &out), NEON_CAMERA_OK);
    f.release_result = NEON_CAMERA_BUSY;
    CHECK_EQ(neon_camera_session_return(&s, &out), NEON_CAMERA_BUSY);
    CHECK_EQ(s.status.consumer_held, 1);
    CHECK_EQ(f.releases, 0);
    f.release_result = NEON_CAMERA_OK;
    CHECK_EQ(neon_camera_session_return(&s, &out), NEON_CAMERA_OK);
    CHECK_EQ(f.releases, 1);
}
static void missed_exact_and_failed_begin(void)
{
    neon_camera_session_t s = {0}; fake_t f = {.begin_result = NEON_CAMERA_HW_ERROR};
    neon_camera_session_config_t c = config(&f);
    neon_isp_session_transaction_t t = {.profile = {&profile, sizeof(profile), 8},
        .mode = NEON_ISP_APPLY_EXACT, .target_frame = 3};
    CHECK_EQ(neon_camera_session_init(&s, &c), NEON_CAMERA_OK);
    CHECK_EQ(neon_camera_session_start(&s), NEON_CAMERA_HW_ERROR);
    CHECK(s.status.claimed); CHECK_EQ(f.releases, 0);
    drain(&s, &f, NEON_CAMERA_OK);
    f.begin_result = NEON_CAMERA_OK;
    CHECK_EQ(neon_camera_session_start(&s), NEON_CAMERA_PENDING);
    activate(&s, &f, 7, 1);
    CHECK_EQ(neon_camera_session_apply(&s, &t, 2), NEON_CAMERA_PENDING);
    f.observation = (neon_isp_session_observation_t){.flags = NEON_ISP_OBS_APPLIED,
        .applied_id = 8, .applied_frame = 4};
    CHECK_EQ(neon_camera_session_poll(&s), NEON_CAMERA_HW_ERROR);
    CHECK(s.status.profile_pending); /* Never report a late EXACT as on target. */
    drain(&s, &f, NEON_CAMERA_OK);
}
static void ready_fifo_across_slot_wrap(void)
{
    neon_camera_session_t s = {0}; fake_t f = {0}; neon_camera_frame_t a, b;
    setup(&s, &f); activate(&s, &f, 7, 1);
    for (uint32_t n = 0; n < 3; ++n) {
        CHECK_EQ(neon_camera_session_submit(&s, n), NEON_CAMERA_OK);
        complete(&s, &f, n, 7);
        CHECK_EQ(neon_camera_session_acquire(&s, &a), NEON_CAMERA_OK);
        CHECK_EQ(neon_camera_session_return(&s, &a), NEON_CAMERA_OK);
    }
    CHECK_EQ(neon_camera_session_submit(&s, 3), NEON_CAMERA_OK);
    CHECK_EQ(f.slot, 3); complete(&s, &f, 3, 7);
    CHECK_EQ(neon_camera_session_submit(&s, 4), NEON_CAMERA_OK);
    CHECK_EQ(f.slot, 0); complete(&s, &f, 4, 7);
    CHECK_EQ(neon_camera_session_acquire(&s, &a), NEON_CAMERA_OK);
    CHECK_EQ(neon_camera_session_acquire(&s, &b), NEON_CAMERA_OK);
    CHECK_EQ(a.identity.tag, 3); CHECK_EQ(b.identity.tag, 4);
    CHECK_EQ(neon_camera_session_return(&s, &a), NEON_CAMERA_OK);
    CHECK_EQ(neon_camera_session_return(&s, &b), NEON_CAMERA_OK);
    drain(&s, &f, NEON_CAMERA_OK);
}
static void batch_fault_and_invalid_apply_preserve_retirement(void)
{
    neon_camera_session_t s = {0}; fake_t f = {0}; neon_camera_frame_t output;
    setup(&s, &f); activate(&s, &f, 7, 1);
    CHECK_EQ(neon_camera_session_submit(&s, 99), NEON_CAMERA_OK);
    neon_isp_session_transaction_t t = {{&profile,sizeof(profile),8},NEON_ISP_APPLY_EXACT,2};
    CHECK_EQ(neon_camera_session_apply(&s, &t, 2), NEON_CAMERA_PENDING);
    f.observation = (neon_isp_session_observation_t){
        .flags = NEON_ISP_OBS_FAULT | NEON_ISP_OBS_APPLIED | NEON_ISP_OBS_OUTPUT,
        .applied_id = 8, .applied_frame = 3, .slot = f.slot,
        .output = {.valid = NEON_ISP_ID_FRAME | NEON_ISP_ID_PROFILE, .ingress_frame = 1, .profile_id = 7, .tag = 99},
        .hardware = {.valid = NEON_ISP_DIAG_RECOVERY | NEON_ISP_DIAG_DMA,
            .dma_error = 8, .recovery_sequence = UINT32_MAX, .recovery_count = 1,
            .recovery_records = {{2,1}}}};
    CHECK_EQ(neon_camera_session_poll(&s), NEON_CAMERA_HW_ERROR);
    CHECK_EQ(s.status.state, NEON_CAMERA_FAULTED); CHECK_EQ(s.status.retired, 1);
    CHECK(s.status.profile_pending); /* Invalid EXACT cannot hide preserved output. */
    CHECK_EQ(neon_camera_session_acquire(&s, &output), NEON_CAMERA_OK);
    CHECK(output.recovery_sequence_valid); CHECK_EQ(output.recovery_sequence, UINT32_MAX);
    CHECK_EQ(s.status.hardware.recovery_records[0].ingress_frame, 2);
    drain(&s, &f, NEON_CAMERA_PENDING); CHECK(s.status.claimed);
    CHECK_EQ(neon_camera_session_return(&s, &output), NEON_CAMERA_OK); CHECK(!s.status.claimed);
}
int main(void)
{
    RUN(validation); RUN(claim_and_baseline); RUN(leases_and_history);
    RUN(pending_and_timeout); RUN(unsupported_and_bad_identity);
    RUN(ready_stop_and_failed_return); RUN(missed_exact_and_failed_begin);
    RUN(ready_fifo_across_slot_wrap); RUN(batch_fault_and_invalid_apply_preserve_retirement);
    return TEST_REPORT();
}
