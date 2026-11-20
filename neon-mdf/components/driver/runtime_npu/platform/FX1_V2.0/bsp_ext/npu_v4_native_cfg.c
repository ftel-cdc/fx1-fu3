#include "npu_v4_native_cfg.h"
#include "../FX1V42Regs.h"

static inline uint32_t iceil(uint32_t a, uint32_t b) {
    if (b == 0) return 0;
    return (a + b - 1u) / b;
}

void npu_compute_core_fields(const npu_layer_desc_t *c, npu_core_fields_t *f) {
    /* SRAM element counts are HW-intrinsic (libsauria_cfg.h):
       SRAMA_N = Y_DIM, SRAMB_N = X_DIM, SRAMC_N = Y_DIM.
       DILP_W stays 64 — it is a fixed SAURIA constant (sauria_types.h),
       independent of the array geometry. 
    */
    const uint32_t X       = FX1_SA_X;      /* 32 */
    const uint32_t SRAMA_N = FX1_SA_Y;      /* 32 — was 64 */
    const uint32_t SRAMB_N = FX1_SA_X;      /* 32 — was 64 */
    const uint32_t SRAMC_N = FX1_SA_Y;      /* 32 — was 64 */
    const uint32_t DILP_W  = 64u;           /* UNCHANGED */

    const uint32_t B_w = (c->B_w > 0) ? c->B_w : 1u;
    const uint32_t B_h = (c->B_h > 0) ? c->B_h : 1u;
    const uint32_t d   = (c->d > 0) ? c->d : 1u;
    const uint32_t s   = (c->s > 0) ? c->s : 1u;
    const uint32_t c_til = c->c_til;
    const uint32_t k_til = c->k_til;
    const uint32_t h_til = (c->h_til > 0) ? c->h_til : 1u;
    const uint32_t w_til = (c->w_til > 0) ? c->w_til : 1u;
    const uint32_t X_used = (c->X_used > 0) ? c->X_used : FX1_SA_X;   /* was 64u */
    const uint32_t Y_used = (c->Y_used > 0) ? c->Y_used : FX1_NATIVE_Y_USED;

    const uint32_t B_w_eff = 1u + (B_w - 1u) * d;
    const uint32_t B_h_eff = 1u + (B_h - 1u) * d;
    const uint32_t C_w_eff_til = 1u + (w_til - 1u) * s;
    const uint32_t C_h_eff_til = 1u + (h_til - 1u) * s;
    const uint32_t A_w_til = C_w_eff_til + B_w_eff - 1u;
    const uint32_t A_h_til = C_h_eff_til + B_h_eff - 1u;

    f->incntlim  = B_w * B_h * c_til - 1u;
    f->act_reps  = iceil(k_til, X_used);
    f->wei_reps  = iceil(w_til, Y_used) * h_til;

    /* XLIM has a fast path for 1x1/stride-1 and a general form. */
    if (s == 1u && d == 1u && B_w == 1u && B_h == 1u &&
        (c_til % SRAMA_N) == 0u && (Y_used % SRAMA_N) == 0u) {
        f->xlim = Y_used;
    } else {
        f->xlim = (1u + (Y_used - 1u) * s) + B_w_eff + 1u
                  - (B_w_eff % 2u) + SRAMA_N;
    }
    f->xstep     = SRAMA_N;                     /* NOT dilation */
    f->ylim      = A_w_til * B_h_eff;
    f->ystep     = A_w_til * d;
    f->chlim     = A_w_til * A_h_til * c_til;
    f->chstep    = A_w_til * A_h_til;
    f->til_xlim  = iceil(w_til, Y_used) * (Y_used * s);
    f->til_xstep = Y_used * s;
    f->til_ylim  = h_til * (A_w_til * s);
    f->til_ystep = A_w_til * s;

    /* dil_pat: MSB-first over 64 bits, bit (63 - i) set when i%d==0 && i/d < B_w.
       Only the low 32 bits reach the 32-bit MMIO register. */
    uint64_t dil_pat = 0;
    for (uint32_t i = 0; i < DILP_W; i++) {
        if ((i % d == 0u) && (i / d < B_w)) {
            dil_pat |= (1ULL << (DILP_W - 1u - i));
        }
    }
    f->dil_pat = (uint32_t)(dil_pat & 0xFFFFFFFFu);

    /* rows_active: LSB-first, (1 << Y_used) - 1. With Y_used = 4 this is 0xF. */
    f->rows_active = (Y_used >= 32u) ? 0xFFFFFFFFu : ((1u << Y_used) - 1u);

    f->wlim      = k_til * B_w * B_h * c_til;
    f->wstep     = k_til;
    f->waligned  = ((k_til % SRAMB_N) == 0u) && (X_used == SRAMB_N);
    f->klim      = f->waligned ? 1u : (SRAMB_N + 1u);
    f->kstep     = SRAMB_N;
    f->til_klim  = k_til;
    f->til_kstep = X_used;

    /* cols_active: MSB-first over X bits, packed as-is (cfg_decode does NOT
       reverse it). libsauria_cfg.h:
           xm = (X_used >= 64) ? ~0 : ((1 << X_used) - 1)
           f  = (X >= 64) ? xm : (xm << (X - X_used))
       With X = 32 the shift branch is now the live one — the old code took the
       X >= 64 branch and produced a right-aligned mask, which misplaces every
       partial (X_used < 32) OC tile. */
    {
        uint64_t xm = (X_used >= 64u) ? ~0ULL : ((1ULL << X_used) - 1ULL);
        uint64_t ca = (X >= 64u) ? xm
                                 : ((X_used >= X) ? xm : (xm << (X - X_used)));
        f->cols_active = (uint32_t)(ca & 0xFFFFFFFFu);
    }

    f->ncontexts = iceil(w_til, Y_used) * h_til * iceil(k_til, X_used);
    f->cxlim     = Y_used + SRAMC_N;
    f->cxstep    = SRAMC_N;
    f->cklim     = w_til * h_til * X_used;
    f->ckstep    = w_til * h_til;
    f->til_cylim = w_til * h_til;
    f->til_cystep= Y_used;
    f->til_cklim = w_til * h_til * k_til;
}
