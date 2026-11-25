#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

typedef struct {
    uint32_t B_w, B_h;        /* kernel spatial           */
    uint32_t d, s;            /* dilation, stride         */
    uint32_t c_til;           /* input channels in tile   */
    uint32_t k_til;           /* output channels in tile  */
    uint32_t h_til, w_til;    /* output tile height/width */
    uint32_t X_used, Y_used;
    uint32_t preload_en;
} npu_layer_desc_t;

typedef struct {
    uint32_t incntlim, act_reps, wei_reps;
    uint32_t xlim, xstep, ylim, ystep, chlim, chstep;
    uint32_t til_xlim, til_xstep, til_ylim, til_ystep;
    uint32_t dil_pat, rows_active;
    uint32_t wlim, wstep, klim, kstep, til_klim, til_kstep;
    uint32_t cols_active, waligned;
    uint32_t ncontexts, cxlim, cxstep, cklim, ckstep;
    uint32_t til_cylim, til_cystep, til_cklim;
} npu_core_fields_t;

void npu_compute_core_fields(const npu_layer_desc_t *c, npu_core_fields_t *f);

#ifdef __cplusplus
}
#endif
