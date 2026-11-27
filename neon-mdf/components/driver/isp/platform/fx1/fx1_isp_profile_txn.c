#include "fx1_isp_profile_txn.h"
#include "fx1_isp/fx1_isp_csr.h"
#include <string.h>
#define R(n) FX1_ISP_##n##_OFFSET
#define M(n) FX1_ISP_##n##_MASK
static uint32_t rd(fx1_isp_backend_t *b, uint32_t r) { return b->port.io.read(b->port.io.user, r); }
static void wr(fx1_isp_backend_t *b, uint32_t r, uint32_t v) { b->port.io.write(b->port.io.user, r, v); }
static void barrier(fx1_isp_backend_t *b) { b->port.io.barrier(b->port.io.user); }
bool fx1_isp_profile_valid(const neon_isp_session_profile_t *p)
{
    fx1_isp_scalar_profile_t v;
    if (!p || !p->data || p->bytes != sizeof(v)) return false;
    memcpy(&v, p->data, sizeof(v));
    return v.abi == FX1_ISP_SCALAR_PROFILE_ABI && v.wb_r <= 4095 &&
        v.wb_g <= 4095 && v.wb_b <= 4095 && v.digital_gain <= 4095;
}
neon_camera_result_t fx1_isp_profile_queue(fx1_isp_backend_t *b, const neon_isp_session_transaction_t *t)
{
    if (!fx1_isp_profile_valid(&t->profile)) return NEON_CAMERA_UNSUPPORTED;
    if (b->phase || b->stopping || b->faulted) return NEON_CAMERA_BUSY;
    uint32_t s = rd(b, R(TUNE_STATUS));
    if (s & (M(TUNE_STATUS_PENDING) | M(TUNE_STATUS_CHECK_BUSY))) return NEON_CAMERA_BUSY;
    if (t->mode != NEON_ISP_APPLY_NEXT && t->mode != NEON_ISP_APPLY_EXACT) return NEON_CAMERA_INVALID;
    if (t->mode == NEON_ISP_APPLY_EXACT) {
        uint32_t d = t->target_frame - rd(b, R(TUNE_NEXT_FRAME));
        if (d >= UINT32_C(0x80000000) || (!(s & M(TUNE_STATUS_ACTIVE_VALID)) && d))
            return NEON_CAMERA_INVALID;
    }
    b->bank = (uint8_t)((s & M(TUNE_STATUS_ACTIVE_VALID)) ? (s & 1u) ^ 1u : 0u);
    uint32_t busy = b->bank ? M(TUNE_STATUS_BANK1_BUSY) | M(TUNE_STATUS_BANK1_PENDING) | M(TUNE_STATUS_BANK1_REFERENCED)
                            : M(TUNE_STATUS_BANK0_BUSY) | M(TUNE_STATUS_BANK0_PENDING) | M(TUNE_STATUS_BANK0_REFERENCED);
    if (s & busy) return NEON_CAMERA_BUSY;
    b->transaction = *t;
    b->phase = FX1_PROFILE_LOAD;
    return NEON_CAMERA_PENDING;
}
static void load_scalars(fx1_isp_backend_t *b)
{
    fx1_isp_scalar_profile_t p;
    memcpy(&p, b->transaction.profile.data, sizeof(p));
    const uint32_t disabled[] = {R(BLC_CTRL), R(LSC_CTRL), R(BPC_CTRL), R(CCM_CTRL),
        R(GAMMA_CTRL), R(GTM_CTRL), R(NR_2D_CTRL), R(EE_CTRL), R(CNF_CTRL),
        R(AEC_CTRL), R(AWB_CTRL), R(AF_CTRL), R(TUNE_NR_CTRL), R(TUNE_CSC_CTRL)};
    for (unsigned i = 0; i < sizeof(disabled) / sizeof(disabled[0]); ++i) wr(b, disabled[i], 0);
    wr(b, R(WB_GAIN_R), p.wb_r); wr(b, R(WB_GAIN_G), p.wb_g); wr(b, R(WB_GAIN_B), p.wb_b);
    wr(b, R(TUNE_WB_GB), p.wb_g);
    wr(b, R(LSC_PROFILE_SEL), 0);
    wr(b, R(WB_CTRL), 1); wr(b, R(DG_GAIN), p.digital_gain); wr(b, R(DG_CTRL), 1);
    wr(b, R(CSC_CTRL), 0); /* BT.601, limited range. */
    wr(b, R(TUNE_PROFILE_ID), b->transaction.profile.id);
}
neon_camera_result_t fx1_isp_profile_progress(fx1_isp_backend_t *b, neon_isp_session_observation_t *o)
{
    uint32_t s = rd(b, R(TUNE_STATUS));
    if (s & M(TUNE_STATUS_CHECK_BUSY)) return NEON_CAMERA_PENDING;
    if (rd(b, R(TUNE_ERROR))) return NEON_CAMERA_HW_ERROR;
    switch (b->phase) {
    case FX1_PROFILE_IDLE: return NEON_CAMERA_NO_DATA;
    case FX1_PROFILE_LOAD:
        wr(b, R(TUNE_BANK), b->bank);
        if (s & (b->bank ? M(TUNE_STATUS_BANK1_VALID) : M(TUNE_STATUS_BANK0_VALID))) {
            wr(b, R(TUNE_CMD), M(TUNE_CMD_UNSEAL)); barrier(b);
            if (rd(b, R(TUNE_ERROR))) return NEON_CAMERA_HW_ERROR;
        }
        wr(b, R(TUNE_CMD), M(TUNE_CMD_INIT)); barrier(b);
        if (rd(b, R(TUNE_ERROR))) return NEON_CAMERA_HW_ERROR;
        load_scalars(b);
        barrier(b);
        if (rd(b, R(TUNE_ERROR))) return NEON_CAMERA_HW_ERROR;
        wr(b, R(TUNE_CMD), M(TUNE_CMD_SEAL)); barrier(b);
        b->phase = FX1_PROFILE_SEAL;
        return NEON_CAMERA_PENDING;
    case FX1_PROFILE_SEAL:
        if (!(s & (b->bank ? M(TUNE_STATUS_BANK1_VALID) : M(TUNE_STATUS_BANK0_VALID))))
            return NEON_CAMERA_HW_ERROR;
        b->phase = FX1_PROFILE_APPLY;
        return NEON_CAMERA_PENDING;
    case FX1_PROFILE_APPLY: {
        uint32_t cmd = M(TUNE_CMD_APPLY);
        /* Hardware performs the final acceptance check; this second precheck
         * avoids emitting a known missed/ambiguous exact request. */
        if (b->transaction.mode == NEON_ISP_APPLY_EXACT) {
            uint32_t d = b->transaction.target_frame - rd(b, R(TUNE_NEXT_FRAME));
            if (d >= UINT32_C(0x80000000) || (!(s & M(TUNE_STATUS_ACTIVE_VALID)) && d))
                return NEON_CAMERA_HW_ERROR;
            wr(b, R(TUNE_APPLY_FRAME), b->transaction.target_frame);
            cmd |= M(TUNE_CMD_EXACT);
        }
        wr(b, R(TUNE_BANK), b->bank);
        wr(b, R(TUNE_CMD), cmd); barrier(b);
        if (rd(b, R(TUNE_ERROR))) return NEON_CAMERA_HW_ERROR;
        b->phase = FX1_PROFILE_WAIT;
        return NEON_CAMERA_PENDING;
    }
    case FX1_PROFILE_WAIT:
        if (s & M(TUNE_STATUS_PENDING)) return NEON_CAMERA_PENDING;
        if (!(s & M(TUNE_STATUS_ACTIVE_VALID)) || (s & 1u) != b->bank ||
            rd(b, R(TUNE_ACTIVE_ID)) != b->transaction.profile.id) return NEON_CAMERA_HW_ERROR;
        o->flags |= NEON_ISP_OBS_APPLIED;
        o->applied_id = rd(b, R(TUNE_ACTIVE_ID));
        o->applied_frame = rd(b, R(TUNE_ACTIVE_FRAME));
        b->phase = FX1_PROFILE_IDLE;
        return NEON_CAMERA_OK;
    default: return NEON_CAMERA_PENDING;
    }
}
