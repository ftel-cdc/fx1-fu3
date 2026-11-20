#include "npu_v4_native_driver.h"
#include "../FX1V42Regs.h"
#include "soc/regs/soc_regs_npu_v4.h"

extern void uart_puts(const char *s);

static inline void npu_write32(uint32_t offset, uint32_t val) {
    *(volatile uint32_t *)(uintptr_t)(CDC_NPU0_BASE + offset) = val;
}

static inline uint32_t npu_read32(uint32_t offset) {
    return *(volatile uint32_t *)(uintptr_t)(CDC_NPU0_BASE + offset);
}

void npu_native_set_profile(uint32_t profile) {
    npu_write32(FX1_REG_NATIVE_CFG_PROFILE, profile);
}

void npu_native_soft_reset(void) {
    npu_write32(FX1_REG_NATIVE_CONTROL, FX1_NATIVE_RESET_BYTE(0xFF));
}

uint64_t npu_perf_read(uint32_t low_offset) {
    /* Two counter blocks with different LO->HI strides:
     *   0x1200..0x1228 -> HI at LO + 0x40
     *   0x1280..0x12A8 -> HI at LO + 0x80
     * See NPU_V4.4_INTEGRATION.md "Raw V4.4 Performance Counters". */
    const uint32_t stride = (low_offset >= FX1_PERF_EXTENDED_LO_FIRST)
                                ? FX1_PERF_HI_STRIDE_EXTENDED
                                : FX1_PERF_HI_STRIDE_LEGACY;
    const uint32_t lo = npu_read32(low_offset);
    const uint32_t hi = npu_read32(low_offset + stride);
    return ((uint64_t)hi << 32) | (uint64_t)lo;
}

/* ---- Per-job perf accumulation ---------------------------------------------
 * The VP NPU model clears its perf counters when a job starts (CDC-VP npu_tlm.cpp:
 * perf.reset() on a native start pulse and on a rich push), so the registers only
 * ever hold the LAST job. A conv fragment is thousands of jobs (per 32x32 tile, up
 * to 8 x {staging GEMM + native run}), so a register read at fragment end saw one
 * tail job. Every start in this firmware goes through npu_rich_push() /
 * npu_native_start() and every completion through the matching *_wait_done(), so
 * the totals are accumulated here, per counter and per job:
 *   pre = before the start write, st = right after it, post = after done
 *   st < pre : the start cleared the counter -> the job counted from 0
 *   else     : the counter kept running      -> the job counted from pre
 * which is right for reset-on-start and free-running counters alike. Counts that
 * land between jobs (late retirement) are folded in at the next job or read.
 * The SRAM staging pass is a GEMM_FUSED used as a DMA loader: its m*k*n
 * pseudo-MACs are kept out of MAC_OPS / ACTIVE_PE / TOTAL_PE / RICH_M/K/N.
 * RICH_M/K/N are a shape, not a count: the last rich job's value is kept.
 * Build with -DNPU_PERF_ACC=0 to read the raw registers instead. */
#ifndef NPU_PERF_ACC
#define NPU_PERF_ACC 1
#endif

/* Order == NPU_PERF_* (npu_v4_native_driver.h). */
static const uint32_t k_perf_lo[NPU_PERF_COUNT] = {
    FX1_PERF_EXEC_CYCLES_LO,  FX1_PERF_STALL_CYCLES_LO, FX1_PERF_MAC_OPS_LO,   FX1_PERF_ACTIVE_PE_LO,   /* 0x1200.. */
    FX1_PERF_TOTAL_PE_LO,     FX1_PERF_TOTAL_CYCLES_LO, FX1_PERF_RICH_M_LO,    FX1_PERF_RICH_K_LO,
    FX1_PERF_RICH_N_LO,       FX1_PERF_SA_ACTIVE_LO,    FX1_PERF_OBP_ACTIVE_LO,                          /* ..0x1228 */
    FX1_PERF_PROCESSING_CYCLES_LO, FX1_PERF_TRANSFER_CYCLES_LO, FX1_PERF_MAC_ENGINE_CYCLES_LO,           /* 0x1280.. */
    FX1_PERF_DMA_ENGINE_CYCLES_LO, FX1_PERF_ACT_ENGINE_CYCLES_LO, FX1_PERF_POOL_ENGINE_CYCLES_LO,
    FX1_PERF_RED_ENGINE_CYCLES_LO, FX1_PERF_DDR_READ_BYTES_LO,    FX1_PERF_DDR_WRITE_BYTES_LO,
    FX1_PERF_DMA_READ_CYCLES_LO,   FX1_PERF_DMA_WRITE_CYCLES_LO,                                         /* ..0x12A8 */
};

#if NPU_PERF_ACC
static uint64_t s_perf_acc[NPU_PERF_COUNT];   /* totals since boot */
static uint64_t s_perf_last[NPU_PERF_COUNT];  /* register value already credited to s_perf_acc */
static uint64_t s_perf_base[NPU_PERF_COUNT];  /* in-flight job: value it counts from */
static uint32_t s_perf_hw_jobs, s_perf_stage_jobs;
static uint8_t  s_perf_ready;                 /* s_perf_last initialised */
static uint8_t  s_perf_in_flight;
static uint8_t  s_perf_next_stage;            /* set by npu_native_stage_operands() */
static uint8_t  s_perf_cur_stage;             /* class of the in-flight (or last) job */

static int perf_is_shape(uint32_t i) {
    return i == NPU_PERF_RICH_M || i == NPU_PERF_RICH_K || i == NPU_PERF_RICH_N;
}

/* Counters the current job class must not be credited with. */
static int perf_skip(uint32_t i) {
    return s_perf_cur_stage && (perf_is_shape(i) || i == NPU_PERF_MAC_OPS ||
                                i == NPU_PERF_ACTIVE_PE || i == NPU_PERF_TOTAL_PE);
}

static void perf_init(void) {
    if (s_perf_ready) return;
    for (uint32_t i = 0; i < NPU_PERF_COUNT; ++i) s_perf_last[i] = npu_perf_read(k_perf_lo[i]);
    s_perf_ready = 1;
}

/* Credit what a counter gained outside a job window (late retirement of the last job). */
static void perf_fold_one(uint32_t i, uint64_t cur) {
    if (!perf_is_shape(i) && !perf_skip(i) && cur > s_perf_last[i])
        s_perf_acc[i] += cur - s_perf_last[i];
    s_perf_last[i] = cur;
}

static void perf_job_end(void) {              /* after done, error or timeout */
    if (!s_perf_in_flight) return;
    for (uint32_t i = 0; i < NPU_PERF_COUNT; ++i) {
        const uint64_t post = npu_perf_read(k_perf_lo[i]);
        const uint64_t job  = (post >= s_perf_base[i]) ? post - s_perf_base[i] : post;
        if (!perf_skip(i)) {
            if (perf_is_shape(i)) { if (job) s_perf_acc[i] = job; }
            else                  s_perf_acc[i] += job;
        }
        s_perf_last[i] = post;
    }
    ++s_perf_hw_jobs;
    if (s_perf_cur_stage) ++s_perf_stage_jobs;
    s_perf_in_flight = 0;
}

static void perf_job_begin(void) {            /* before the start write */
    if (s_perf_in_flight) perf_job_end();     /* previous start was never waited for */
    perf_init();
    for (uint32_t i = 0; i < NPU_PERF_COUNT; ++i) {
        const uint64_t pre = npu_perf_read(k_perf_lo[i]);
        perf_fold_one(i, pre);                /* still uses the PREVIOUS job's class */
        s_perf_base[i] = pre;
    }
    s_perf_cur_stage  = s_perf_next_stage;
    s_perf_next_stage = 0;
}

static void perf_job_started(void) {          /* right after the start write */
    for (uint32_t i = 0; i < NPU_PERF_COUNT; ++i)
        if (npu_perf_read(k_perf_lo[i]) < s_perf_base[i]) s_perf_base[i] = 0;   /* cleared by the start */
    s_perf_in_flight = 1;
}

#define PERF_JOB_BEGIN()      perf_job_begin()
#define PERF_JOB_STARTED()    perf_job_started()
#define PERF_JOB_END()        perf_job_end()
#define PERF_NEXT_IS_STAGE()  (s_perf_next_stage = 1)
#else
#define PERF_JOB_BEGIN()      ((void)0)
#define PERF_JOB_STARTED()    ((void)0)
#define PERF_JOB_END()        ((void)0)
#define PERF_NEXT_IS_STAGE()  ((void)0)
#endif

void npu_perf_acc_snapshot(uint64_t out[NPU_PERF_COUNT], uint32_t *hw_jobs, uint32_t *stage_jobs) {
#if NPU_PERF_ACC
    perf_init();
    for (uint32_t i = 0; i < NPU_PERF_COUNT; ++i) {
        if (!s_perf_in_flight) perf_fold_one(i, npu_perf_read(k_perf_lo[i]));
        out[i] = s_perf_acc[i];
    }
    if (hw_jobs)    *hw_jobs    = s_perf_hw_jobs;
    if (stage_jobs) *stage_jobs = s_perf_stage_jobs;
#else
    for (uint32_t i = 0; i < NPU_PERF_COUNT; ++i) out[i] = npu_perf_read(k_perf_lo[i]);
    if (hw_jobs)    *hw_jobs    = 0;
    if (stage_jobs) *stage_jobs = 0;
#endif
}

uint64_t npu_perf_acc_get(uint32_t idx) {
    if (idx >= NPU_PERF_COUNT) return 0;
#if NPU_PERF_ACC
    perf_init();
    if (!s_perf_in_flight) perf_fold_one(idx, npu_perf_read(k_perf_lo[idx]));
    return s_perf_acc[idx];
#else
    return npu_perf_read(k_perf_lo[idx]);
#endif
}

void npu_rich_program(const NeonRichInstr *instr) {
    if (!instr) return;

    uint8_t op_val = (uint8_t)instr->opcode;

    if (op_val == FX1_RICH_OP_GEMM_FUSED) {
        npu_write32(FX1_RICH_IN_ADDR, (uint32_t)instr->in_addr);
        npu_write32(FX1_RICH_WEIGHT_ADDR, (uint32_t)instr->w_addr);
        npu_write32(FX1_RICH_OUT_ADDR, (uint32_t)instr->out_addr);
        npu_write32(FX1_RICH_BIAS_ADDR, (uint32_t)instr->bias_addr);
        npu_write32(FX1_RICH_M, instr->m);
        npu_write32(FX1_RICH_K, instr->k);
        npu_write32(FX1_RICH_N, instr->n);
        npu_write32(FX1_RICH_KERNEL_H, instr->kh);
        npu_write32(FX1_RICH_KERNEL_W, instr->kw);
        npu_write32(FX1_RICH_STRIDE, instr->stride);
        npu_write32(FX1_RICH_PADDING, instr->pad);
        npu_write32(FX1_RICH_ACT_TYPE, (uint32_t)instr->act_type);
        npu_write32(FX1_RICH_HAS_SKIP, (uint32_t)instr->has_skip);
        npu_write32(FX1_RICH_SKIP_ADDR, (uint32_t)instr->skip_addr);
        npu_write32(FX1_RICH_IN_SCALE, instr->in_scale);
        npu_write32(FX1_RICH_WEIGHT_SCALE, instr->w_scale);
        npu_write32(FX1_RICH_OUT_SCALE, instr->out_scale);
    } else if (op_val == FX1_RICH_OP_ELEM_WISE) {
        npu_write32(FX1_RICH_OUT_ADDR, (uint32_t)instr->out_addr);
        npu_write32(FX1_RICH_Q_GAMMA_A_ADDR, (uint32_t)instr->q_gamma_a_addr);
        npu_write32(FX1_RICH_K_B_ADDR, (uint32_t)instr->k_b_addr);

        npu_write32(FX1_RICH_SEQ_LEN, instr->len);

        uint32_t dim_val = (instr->dim > 0) ? ((uint32_t)instr->dim & 0xFFu) : 1u;
        npu_write32(FX1_RICH_HEADS_DIM_MODE,
                    (((uint32_t)instr->elem_mode & 0xFFu) << 24) |
                    (dim_val << 16) |
                    ((uint32_t)instr->num_heads & 0xFFFFu));

        npu_write32(FX1_RICH_A_LEN, instr->a_len ? instr->a_len : instr->len);
        npu_write32(FX1_RICH_B_LEN, instr->b_len ? instr->b_len : instr->len);

        npu_write32(FX1_RICH_HAS_SKIP, 0u);

        npu_write32(FX1_RICH_HEAD_DIM_EPS, instr->scale_a);
        npu_write32(FX1_RICH_ATTN_SCALE, instr->scale_b);
        npu_write32(FX1_RICH_SCALE_OUT, instr->scale_out);
    } else if (op_val == FX1_RICH_OP_LAYERNORM) {
        npu_write32(FX1_RICH_IN_ADDR, (uint32_t)instr->in_addr);
        npu_write32(FX1_RICH_OUT_ADDR, (uint32_t)instr->out_addr);
        npu_write32(FX1_RICH_Q_GAMMA_A_ADDR, (uint32_t)instr->q_gamma_a_addr);
        npu_write32(FX1_RICH_V_BETA_ADDR, (uint32_t)instr->v_beta_addr);
        npu_write32(FX1_RICH_SEQ_LEN, instr->len);
        npu_write32(FX1_RICH_HEADS_DIM_MODE, instr->dim);
        npu_write32(FX1_RICH_HEAD_DIM_EPS, instr->eps_shift);
        npu_write32(FX1_RICH_HAS_SKIP, 0u);
    } else if (op_val == FX1_RICH_OP_FUSED_ATTN) {
        npu_write32(FX1_RICH_OUT_ADDR, (uint32_t)instr->out_addr);
        npu_write32(FX1_RICH_Q_GAMMA_A_ADDR, (uint32_t)instr->q_gamma_a_addr);
        npu_write32(FX1_RICH_K_B_ADDR, (uint32_t)instr->k_b_addr);
        npu_write32(FX1_RICH_V_BETA_ADDR, (uint32_t)instr->v_beta_addr);
        npu_write32(FX1_RICH_SEQ_LEN, instr->seq_len);
        npu_write32(FX1_RICH_HEADS_DIM_MODE, instr->num_heads);
        npu_write32(FX1_RICH_HEAD_DIM_EPS, instr->head_dim);
        npu_write32(FX1_RICH_ATTN_SCALE, instr->attn_scale);
        npu_write32(FX1_RICH_HAS_SKIP, 0u);
    }
}

void npu_rich_push(uint8_t opcode, uint8_t lane) {
    PERF_JOB_BEGIN();
    if (lane == 0) {
        npu_write32(FX1_RICH_PUSH_A, (uint32_t)opcode);
    } else {
        npu_write32(FX1_RICH_PUSH_B, (uint32_t)opcode);
    }
    PERF_JOB_STARTED();
}

static int rich_wait_done_poll(uint32_t max_iters) {
    if (max_iters == 0) max_iters = 400000;
    for (uint32_t i = 0; i < max_iters; ++i) {
        uint32_t sts = npu_read32(CDC_NPU_STATUS);
        if ((sts & CDC_NPU_STATUS_DONE) != 0) {
            npu_write32(CDC_NPU_STATUS, CDC_NPU_STATUS_DONE);
            return 1; // success
        }
        if ((sts & CDC_NPU_STATUS_ERROR) != 0) {
            uint32_t err = npu_read32(CDC_NPU_LAST_ERROR);
            npu_write32(CDC_NPU_STATUS, CDC_NPU_STATUS_ERROR);
            return (err != 0) ? -(int)err : -1; // return negative error code
        }
        uint32_t ctrl = npu_read32(FX1_REG_NATIVE_CONTROL);
        if ((ctrl & FX1_NATIVE_STATUS_DONE) != 0) {
            npu_write32(FX1_REG_NATIVE_CONTROL, FX1_NATIVE_CTRL_CLEAR_DONE);
            return 1; // success
        }
    }
    return 0; // timeout
}

int npu_rich_wait_done(uint32_t max_iters) {
    const int rc = rich_wait_done_poll(max_iters);
    PERF_JOB_END();
    return rc;
}

/* HasNpuTop padding extension registers (SW_INTEGRATION_GUIDE.md §3.3, model offsets
 * 0x4E8..0x4F8). The VP forwards the whole 4 KiB rich alias window to the model, so they sit
 * at the same alias offsets as the rest of the extension bank; the SDK's soc_regs_npu_v4.h
 * stops at MASK_ADDR (0x4E4), hence the local definitions. */
#ifndef CDC_NPU_VP_RICH_PAD_VALUE
#define CDC_NPU_VP_RICH_PAD_VALUE (CDC_NPU_VP_RICH_ALIAS_BASE + 0x4E8u)
#define CDC_NPU_VP_RICH_PAD_T     (CDC_NPU_VP_RICH_ALIAS_BASE + 0x4ECu)
#define CDC_NPU_VP_RICH_PAD_L     (CDC_NPU_VP_RICH_ALIAS_BASE + 0x4F0u)
#define CDC_NPU_VP_RICH_PAD_B     (CDC_NPU_VP_RICH_ALIAS_BASE + 0x4F4u)
#define CDC_NPU_VP_RICH_PAD_R     (CDC_NPU_VP_RICH_ALIAS_BASE + 0x4F8u)
#endif

int npu_has_gemm_conv(const npu_has_gemm_t *g) {
    /* DRAM window: the VP stages it into the model before the push and copies it back
     * before STATUS.DONE. */
    npu_write32(CDC_NPU_RICH_WINDOW_BASE, g->win_base);
    npu_write32(CDC_NPU_RICH_WINDOW_SIZE, g->win_size);
    /* Any write clears a sticky HasNpuTop error left by an earlier instruction. */
    npu_write32(CDC_NPU_VP_RICH_STATUS, 0u);

    /* Legacy registers are sticky: write every one GEMM_FUSED reads (M/K/N are ignored
     * once IN_C is written). */
    npu_write32(FX1_RICH_IN_ADDR,     g->in_addr);
    npu_write32(FX1_RICH_WEIGHT_ADDR, g->w_addr);
    npu_write32(FX1_RICH_OUT_ADDR,    g->out_addr);
    npu_write32(FX1_RICH_BIAS_ADDR,   g->bias_addr);
    npu_write32(FX1_RICH_KERNEL_H,    g->kh);
    npu_write32(FX1_RICH_KERNEL_W,    g->kw);
    npu_write32(FX1_RICH_STRIDE,      g->stride);
    npu_write32(FX1_RICH_PADDING,     g->pad);
    npu_write32(FX1_RICH_ACT_TYPE,    g->act_type);
    npu_write32(FX1_RICH_HAS_SKIP,    0u);

    /* Extension bank (one-shot, cleared by the push). Writing it routes the push to
     * HasNpuTop; TILE_CIN below IN_C enables the input-channel split with SRAM-C preload. */
    npu_write32(CDC_NPU_VP_RICH_IN_C,       g->in_c);
    npu_write32(CDC_NPU_VP_RICH_IN_H,       g->in_h);
    npu_write32(CDC_NPU_VP_RICH_IN_W,       g->in_w);
    npu_write32(CDC_NPU_VP_RICH_OUT_C,      g->out_c);
    npu_write32(CDC_NPU_VP_RICH_OUT_H,      g->out_h);
    npu_write32(CDC_NPU_VP_RICH_OUT_W,      g->out_w);
    npu_write32(CDC_NPU_VP_RICH_TILE_COUT,  g->tile_cout);
    npu_write32(CDC_NPU_VP_RICH_TILE_H,     g->tile_h);
    npu_write32(CDC_NPU_VP_RICH_TILE_W,     g->tile_w);
    npu_write32(CDC_NPU_VP_RICH_SCALE_ADDR, g->scale_addr);
    npu_write32(CDC_NPU_VP_RICH_SHIFT_ADDR, g->shift_addr);
    if (g->act_type != 0u)
        npu_write32(CDC_NPU_VP_RICH_LUT_ADDR, g->lut_addr);
    npu_write32(CDC_NPU_VP_RICH_ZP_OUT,     (uint32_t)g->zp_out);
    npu_write32(CDC_NPU_VP_RICH_FLAGS,      g->flags);
    if (g->tile_cin != 0u && g->tile_cin < g->in_c)
        npu_write32(CDC_NPU_VP_RICH_TILE_CIN, g->tile_cin);
    if (g->y_used != 0u)
        npu_write32(CDC_NPU_VP_RICH_Y_USED, g->y_used);
    /* Hardware padding of the unpadded input. Writing any side register replaces the legacy
     * PAD for all four sides (unwritten sides = 0), so all four are written together; the
     * value is the int8 input zero point in the low byte (two's complement). */
    if ((g->pad_t | g->pad_l | g->pad_b | g->pad_r) != 0u) {
        npu_write32(CDC_NPU_VP_RICH_PAD_VALUE, (uint32_t)(uint8_t)(int8_t)g->pad_value);
        npu_write32(CDC_NPU_VP_RICH_PAD_T, g->pad_t);
        npu_write32(CDC_NPU_VP_RICH_PAD_L, g->pad_l);
        npu_write32(CDC_NPU_VP_RICH_PAD_B, g->pad_b);
        npu_write32(CDC_NPU_VP_RICH_PAD_R, g->pad_r);
    }

    npu_rich_push(FX1_RICH_OP_GEMM_FUSED, 0);
    /* A whole layer on the RTL-accurate core can outlast the default 400k polls. */
    int rc = npu_rich_wait_done(0xFFFFFFFFu);
    const uint32_t st = npu_read32(CDC_NPU_VP_RICH_STATUS);
    if (rc > 0 && (st & 4u) != 0u)                  /* bit 2 sticky error, code in [31:16] */
        rc = -(int)((st >> 16) & 0xFFFFu);
    return rc;
}

/* Push one ELEM_WISE and wait without a poll limit (the RTL-accurate model runs the whole tensor),
 * then report a sticky HasNpuTop error as -(code) like npu_has_gemm_conv. */
static int has_elem_push_wait(void) {
    npu_rich_push(FX1_RICH_OP_ELEM_WISE, 0);
    int rc = npu_rich_wait_done(0xFFFFFFFFu);
    const uint32_t st = npu_read32(CDC_NPU_VP_RICH_STATUS);
    if (rc > 0 && (st & 4u) != 0u)                  /* bit 2 sticky error, code in [31:16] */
        rc = -(int)((st >> 16) & 0xFFFFu);
    return rc;
}

int npu_has_elem_add(const npu_has_add_t *a) {
    npu_write32(CDC_NPU_RICH_WINDOW_BASE, a->win_base);
    npu_write32(CDC_NPU_RICH_WINDOW_SIZE, a->win_size);
    npu_write32(CDC_NPU_VP_RICH_STATUS, 0u);
    /* Legacy registers are sticky: write every one the ADD reads (§4.2). Mode 0 as a plain value. */
    npu_write32(FX1_RICH_Q_GAMMA_A_ADDR, a->a_addr);
    npu_write32(FX1_RICH_K_B_ADDR,       a->b_addr);
    npu_write32(FX1_RICH_OUT_ADDR,       a->out_addr);
    npu_write32(FX1_RICH_SEQ_LEN,        a->len);
    npu_write32(FX1_RICH_A_LEN,          a->len);
    npu_write32(FX1_RICH_B_LEN,          a->len);
    npu_write32(FX1_RICH_HEADS_DIM_MODE, FX1_RICH_ELEM_ADD);
    npu_write32(FX1_RICH_HAS_SKIP,       0u);
    /* Integer extension (one-shot, cleared by the push); writing it routes the push to HasNpuTop. */
    npu_write32(CDC_NPU_VP_RICH_ZP_A, (uint32_t)a->zp_a);
    npu_write32(CDC_NPU_VP_RICH_ZP_B, (uint32_t)a->zp_b);
    npu_write32(CDC_NPU_VP_RICH_ZP_O, (uint32_t)a->zp_o);
    npu_write32(CDC_NPU_VP_RICH_SA,   a->sa);
    npu_write32(CDC_NPU_VP_RICH_SHA,  a->sha);
    npu_write32(CDC_NPU_VP_RICH_SB,   a->sb);
    npu_write32(CDC_NPU_VP_RICH_SHB,  a->shb);
    npu_write32(CDC_NPU_VP_RICH_SO,   a->so);
    npu_write32(CDC_NPU_VP_RICH_SHO,  a->sho);
    return has_elem_push_wait();
}

int npu_has_maxpool(const npu_has_pool_t *p) {
    npu_write32(CDC_NPU_RICH_WINDOW_BASE, p->win_base);
    npu_write32(CDC_NPU_RICH_WINDOW_SIZE, p->win_size);
    npu_write32(CDC_NPU_VP_RICH_STATUS, 0u);
    npu_write32(FX1_RICH_Q_GAMMA_A_ADDR, p->in_addr);
    npu_write32(FX1_RICH_OUT_ADDR,       p->out_addr);
    npu_write32(FX1_RICH_STRIDE,         p->stride);
    npu_write32(FX1_RICH_HEADS_DIM_MODE, FX1_RICH_ELEM_MAXPOOL);
    npu_write32(FX1_RICH_HAS_SKIP,       0u);
    npu_write32(CDC_NPU_VP_RICH_IN_C,      p->c);
    npu_write32(CDC_NPU_VP_RICH_IN_H,      p->h);
    npu_write32(CDC_NPU_VP_RICH_IN_W,      p->w);
    npu_write32(CDC_NPU_VP_RICH_POOL_K,    p->k);
    npu_write32(CDC_NPU_VP_RICH_POOL_P,    p->pad);
    npu_write32(CDC_NPU_VP_RICH_POOL_MODE, p->mode);
    return has_elem_push_wait();
}

// --- Stage 3.1: SRAM staging via the rich DMA loader ---
int npu_native_stage_operands(uint32_t act_phys, uint32_t wgt_phys,
                              uint32_t scratch_phys,
                              uint32_t m, uint32_t k, uint32_t n)
{
    npu_write32(FX1_RICH_IN_ADDR, act_phys);
    npu_write32(FX1_RICH_WEIGHT_ADDR, wgt_phys);
    npu_write32(FX1_RICH_OUT_ADDR, scratch_phys);
    npu_write32(FX1_RICH_BIAS_ADDR, 0u);
    npu_write32(FX1_RICH_HAS_SKIP, 0u);
    npu_write32(FX1_RICH_M, m);
    npu_write32(FX1_RICH_K, k);
    npu_write32(FX1_RICH_N, n);
    npu_write32(FX1_RICH_KERNEL_H, 1u);
    npu_write32(FX1_RICH_KERNEL_W, 1u);
    npu_write32(FX1_RICH_STRIDE, 1u);
    npu_write32(FX1_RICH_PADDING, 0u);
    npu_write32(FX1_RICH_ACT_TYPE, 0u);
    npu_write32(FX1_RICH_IN_SCALE, 0x3f800000u);     /* 1.0f IEEE-754 */
    npu_write32(FX1_RICH_WEIGHT_SCALE, 0x3f800000u); /* 1.0f IEEE-754 */
    npu_write32(FX1_RICH_OUT_SCALE, 0x3f800000u);    /* 1.0f IEEE-754 */

    PERF_NEXT_IS_STAGE();      /* DMA loader, not model compute: see the perf block above */
    npu_rich_push(FX1_RICH_OP_GEMM_FUSED, 0);
    return npu_rich_wait_done(400000);
}

static uint32_t gcd_u32(uint32_t a, uint32_t b) {
    while (b != 0) {
        uint32_t t = b;
        b = a % b;
        a = t;
    }
    return a;
}

int npu_native_stage_operands_sized(uint32_t act_phys, uint32_t act_bytes,
                                    uint32_t wgt_phys, uint32_t wgt_bytes,
                                    uint32_t scratch_phys)
{
    if (act_bytes == 0 || wgt_bytes == 0) return 1;
    // Native SRAM B is organized as 32 columns (FX1_SA_X = 32).
    // Staging must keep N=32 so weights are written with 32 columns per row in SRAM B.
    uint32_t n = 32;
    uint32_t k = (n != 0) ? (wgt_bytes / n) : 0;
    uint32_t m = (k != 0) ? (act_bytes / k) : 0;
    if (k == 0 || m * k != act_bytes || k * n != wgt_bytes) {
        // Fallback to GCD if not cleanly divisible. After
        // NPUV4Runtime::triggerNative() started staging K_padded (a multiple of
        // 32) instead of K_exact, this branch should no longer be reachable for
        // any conv in the compiled model set -- if it fires, SRAM rows will NOT
        // land on 32-byte word boundaries and the native datapath is
        // known-corrupt. See IMPLEMENTATION_PLAN_SRAMA_STAGING_SHAPE.md.
        uart_puts("[npu_native_stage] WARNING: falling back to gcd shape; "
                  "SRAM rows will not be 32B-aligned\r\n");
        uint32_t g = gcd_u32(act_bytes, wgt_bytes);
        m = act_bytes / g;
        k = g;
        n = wgt_bytes / g;
    }
    return npu_native_stage_operands(act_phys, wgt_phys, scratch_phys, m, k, n);
}

// --- Stage 3.2: Program core registers ---
void npu_native_program_core(const npu_layer_desc_t *d,
                             const npu_core_fields_t *f,
                             uint32_t k_dim)
{
    (void)d;
    npu_native_set_profile(FX1_PROFILE_V1_SAURIA);      /* profile FIRST */

    npu_write32(FX1_NATIVE_CON_INCNTLIM,  k_dim);
    npu_write32(FX1_NATIVE_CON_ACT_REPS,  f->act_reps);
    npu_write32(FX1_NATIVE_CON_WEI_REPS,  f->wei_reps);

    npu_write32(FX1_NATIVE_ACT_ROWS_ACTIVE, f->rows_active);
    npu_write32(FX1_NATIVE_ACT_INCNTLIM,  k_dim);
    npu_write32(FX1_NATIVE_ACT_INCNTSTEP, f->xstep);
    npu_write32(FX1_NATIVE_ACT_XLIM,      f->xlim);
    npu_write32(FX1_NATIVE_ACT_XSTEP,     f->xstep);
    npu_write32(FX1_NATIVE_ACT_YLIM,      f->ylim);
    npu_write32(FX1_NATIVE_ACT_YSTEP,     f->ystep);
    npu_write32(FX1_NATIVE_ACT_CHLIM,     f->chlim);
    npu_write32(FX1_NATIVE_ACT_CHSTEP,    f->chstep);
    npu_write32(FX1_NATIVE_ACT_DIL_PAT,   f->dil_pat);
    npu_write32(FX1_NATIVE_ACT_TIL_XLIM,  f->til_xlim);
    npu_write32(FX1_NATIVE_ACT_TIL_XSTEP, f->til_xstep);
    npu_write32(FX1_NATIVE_ACT_TIL_YLIM,  f->til_ylim);
    npu_write32(FX1_NATIVE_ACT_TIL_YSTEP, f->til_ystep);

    npu_write32(FX1_NATIVE_WEI_INCNTLIM,  f->wlim);
    npu_write32(FX1_NATIVE_WEI_INCNTSTEP, f->wstep);
    npu_write32(FX1_NATIVE_WEI_WLIM,      f->wlim);
    npu_write32(FX1_NATIVE_WEI_WSTEP,     f->wstep);
    npu_write32(FX1_NATIVE_WEI_KLIM,      f->klim);
    npu_write32(FX1_NATIVE_WEI_KSTEP,     k_dim);      /* also sets mvm_k sideband */
    npu_write32(FX1_NATIVE_WEI_TIL_XLIM,  f->til_klim);
    npu_write32(FX1_NATIVE_WEI_TIL_XSTEP, f->til_kstep);
    npu_write32(FX1_NATIVE_WEI_COLS_ACTIVE, f->cols_active);
    npu_write32(FX1_NATIVE_WEI_WALIGNED,  f->waligned);

    npu_write32(FX1_NATIVE_NCONTEXTS,     f->ncontexts); /* sets total_contexts sideband */
    npu_write32(FX1_NATIVE_OUT_CXLIM,     f->cxlim);
    npu_write32(FX1_NATIVE_OUT_CXSTEP,    f->cxstep);
    npu_write32(FX1_NATIVE_OUT_CKLIM,     f->cklim);
    npu_write32(FX1_NATIVE_OUT_CKSTEP,    f->ckstep);
    npu_write32(FX1_NATIVE_TIL_CYLIM,     f->til_cylim);
    npu_write32(FX1_NATIVE_TIL_CYSTEP,    f->til_cystep);
    npu_write32(FX1_NATIVE_TIL_CKLIM,     f->til_cklim);

    npu_write32(FX1_NATIVE_CFG_ACT_BASE_ADDR, 0u);
    npu_write32(FX1_NATIVE_CFG_WEI_BASE_ADDR, 0u);
    npu_write32(FX1_NATIVE_CFG_OUT_BASE_ADDR, 0u);

    /* DO NOT write 0x0820/0x0824/0x0828 as TIL_CKSTEP/INACTIVE_COLS/PRELOAD_EN —
       they alias onto OBP-A, which wins the decode (Stage 0.2). OBP is programmed
       last, by npu_native_program_obp(), immediately before the start pulse. */
}

// --- Stage 3.3: OBP Programming ---
void npu_native_program_obp(const int32_t *bias, const int32_t *multiplier,
                            const int32_t *shift, const uint8_t *lut,
                            uint32_t lanes, uint32_t cfg)
{
    if (lanes > FX1_OBP_LANES) lanes = FX1_OBP_LANES;

    if (bias && (cfg & FX1_OBP_CFG_BIAS_EN)) {
        npu_obp_load_bias_a(bias, lanes);
    }
    if (multiplier && shift && (cfg & FX1_OBP_CFG_REQUANT_EN)) {
        uint32_t scale_obp[FX1_OBP_LANES];
        uint32_t shift_obp[FX1_OBP_LANES];
        for (uint32_t l = 0; l < lanes; ++l) {
            scale_obp[l] = (uint32_t)multiplier[l];
            // TFLite shift is <= 0 for right shifts; OBP shift is right shift amount (e.g. 30)
            shift_obp[l] = (shift[l] < 0) ? (uint32_t)(-shift[l]) : (uint32_t)shift[l];
        }
        npu_obp_load_scale_a(scale_obp, lanes);
        npu_obp_load_shift_a(shift_obp, lanes);
    }
    if (lut && (cfg & FX1_OBP_CFG_LUT_EN)) {
        npu_obp_load_lut_a(lut, lanes);
    }

    // Program OBP CFG LAST
    npu_obp_set_cfg_a(cfg);
}

// --- Stage 3.4: Start, wait, and read back ---
void npu_native_start(void) {
    __asm__ volatile("fence" ::: "memory");
    PERF_JOB_BEGIN();
    npu_write32(FX1_REG_NATIVE_CONTROL, FX1_NATIVE_CTRL_START_PULSE);
    PERF_JOB_STARTED();
}

static int native_wait_done_poll(uint32_t max_iters) {
    if (max_iters == 0) max_iters = 400000;
    for (uint32_t i = 0; i < max_iters; ++i) {
        uint32_t ctrl = npu_read32(FX1_REG_NATIVE_CONTROL);
        if ((ctrl & FX1_NATIVE_STATUS_DONE) != 0) {
            npu_write32(FX1_REG_NATIVE_CONTROL, FX1_NATIVE_CTRL_CLEAR_DONE);
            return 1; // success
        }
        uint32_t sts = npu_read32(CDC_NPU_STATUS);
        if ((sts & CDC_NPU_STATUS_ERROR) != 0) {
            uint32_t err = npu_read32(CDC_NPU_LAST_ERROR);
            npu_write32(CDC_NPU_STATUS, CDC_NPU_STATUS_ERROR);
            return (err != 0) ? -(int)err : -1; // return negative error code
        }
        if ((sts & CDC_NPU_STATUS_DONE) != 0) {
            npu_write32(CDC_NPU_STATUS, CDC_NPU_STATUS_DONE);
            return 1; // success
        }
    }
    return 0; // timeout
}

int npu_native_wait_done(uint32_t max_iters) {
    const int rc = native_wait_done_poll(max_iters);
    PERF_JOB_END();
    return rc;
}

void npu_native_read_sramc(int8_t *dst, uint32_t channels, uint32_t rows,
                           uint32_t row_stride) {
    if (!dst || row_stride == 0u) return;
    if (channels > FX1_SA_X) channels = FX1_SA_X;

    /* V4.5 host SRAM encoding: word_index = (physical_row << 3) | subword,
       subword 0..7, 4 int8 lanes per word. Only subword 0 is 4-byte aligned AND
       lane-contiguous, so at most FX1_NATIVE_Y_USED (=4) rows per context. */
    const uint32_t ncontexts = (rows + FX1_NATIVE_Y_USED - 1u) / FX1_NATIVE_Y_USED;
    for (uint32_t col = 0; col < channels; ++col) {
        for (uint32_t ctx = 0; ctx < ncontexts; ++ctx) {
            uint32_t phys = col * ncontexts + ctx;
            uint32_t word = npu_read32(FX1_NATIVE_SRAMC_BASE +
                                       (phys << FX1_SRAM_SUBWORD_SHIFT));   /* was << 4 */
            for (uint32_t lane = 0; lane < FX1_SRAM_LANES_PER_WORD; ++lane) {
                uint32_t r = ctx * FX1_SRAM_LANES_PER_WORD + lane;
                if (r < rows) {
                    int8_t val = (int8_t)((word >> (lane * 8)) & 0xFFu);
                    /* Transpose native [channel][row] to row-major dst. */
                    dst[r * row_stride + col] = val;
                }
            }
        }
    }
}

// --- OBP Hardware Control (Lane A) ---
void npu_obp_set_cfg_a(uint32_t cfg) {
    npu_write32(FX1_REG_OBP_CFG_A, cfg);
}

void npu_obp_set_default_requant_a(uint32_t scale, uint32_t shift) {
    npu_write32(FX1_REG_REQUANT_SCALE_A, scale);
    npu_write32(FX1_REG_REQUANT_SHIFT_A, shift);
}

void npu_obp_load_bias_a(const int32_t *v, uint32_t lanes) {
    if (!v) return;
    if (lanes > FX1_OBP_LANES) lanes = FX1_OBP_LANES;
    for (uint32_t l = 0; l < lanes; ++l) {
        npu_write32(FX1_OBP_A_BIAS_BASE + l * sizeof(int32_t), (uint32_t)v[l]);
    }
}

void npu_obp_load_scale_a(const uint32_t *v, uint32_t lanes) {
    if (!v) return;
    if (lanes > FX1_OBP_LANES) lanes = FX1_OBP_LANES;
    for (uint32_t l = 0; l < lanes; ++l) {
        npu_write32(FX1_OBP_A_SCALE_BASE + l * sizeof(uint32_t), v[l]);
    }
}

void npu_obp_load_shift_a(const uint32_t *v, uint32_t lanes) {
    if (!v) return;
    if (lanes > FX1_OBP_LANES) lanes = FX1_OBP_LANES;
    for (uint32_t l = 0; l < lanes; ++l) {
        npu_write32(FX1_OBP_A_SHIFT_BASE + l * sizeof(uint32_t), v[l]);
    }
}

void npu_obp_load_lut_a(const uint8_t *lut, uint32_t lanes) {
    if (!lut) return;
    if (lanes > FX1_OBP_LANES) lanes = FX1_OBP_LANES;
    // OBP A LUT: 32 lanes x 256 bytes = 8192 bytes (alias window 0x20000..0x21FFF)
    for (uint32_t l = 0; l < lanes; ++l) {
        uint32_t lane_offset = FX1_OBP_A_LUT_BASE + l * 256;
        for (uint32_t i = 0; i < 256; i += 4) {
            uint32_t word = (uint32_t)lut[l * 256 + i + 0] |
                            ((uint32_t)lut[l * 256 + i + 1] << 8) |
                            ((uint32_t)lut[l * 256 + i + 2] << 16) |
                            ((uint32_t)lut[l * 256 + i + 3] << 24);
            npu_write32(lane_offset + i, word);
        }
    }
}

// --- RCE Hardware Control (Lane A) ---
void npu_rce_load_exp_a(const uint8_t *lut_exp) {
    if (!lut_exp) return;
    for (uint32_t i = 0; i < 256; i += 4) {
        uint32_t word = (uint32_t)lut_exp[i + 0] |
                        ((uint32_t)lut_exp[i + 1] << 8) |
                        ((uint32_t)lut_exp[i + 2] << 16) |
                        ((uint32_t)lut_exp[i + 3] << 24);
        npu_write32(FX1_RCE_A_EXP_BASE + i, word);
    }
}

void npu_rce_load_recip_a(const uint8_t *lut_recip) {
    if (!lut_recip) return;
    for (uint32_t i = 0; i < 512; i += 4) {
        uint32_t word = (uint32_t)lut_recip[i + 0] |
                        ((uint32_t)lut_recip[i + 1] << 8) |
                        ((uint32_t)lut_recip[i + 2] << 16) |
                        ((uint32_t)lut_recip[i + 3] << 24);
        npu_write32(FX1_RCE_A_RECIP_BASE + i, word);
    }
}

void npu_rce_load_rsqrt_a(const uint16_t *lut_rsqrt) {
    if (!lut_rsqrt) return;
    for (uint32_t i = 0; i < 1024; i += 2) {
        uint32_t word = (uint32_t)lut_rsqrt[i + 0] |
                        ((uint32_t)lut_rsqrt[i + 1] << 16);
        npu_write32(FX1_RCE_A_RSQRT_BASE + i * sizeof(uint16_t), word);
    }
}
