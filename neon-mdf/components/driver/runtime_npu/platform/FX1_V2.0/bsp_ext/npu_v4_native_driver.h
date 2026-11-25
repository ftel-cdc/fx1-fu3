#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "core/NPU_ISA.h"
#include "npu_v4_native_cfg.h"
#include <stdint.h>
#include <stddef.h>

void npu_native_set_profile(uint32_t profile);
void npu_rich_program(const NeonRichInstr *instr);
void npu_rich_push(uint8_t opcode, uint8_t lane);
int  npu_rich_wait_done(uint32_t max_iters);

/* One whole-layer GEMM_FUSED on the V4.7 HasNpuTop controller (extended instruction,
 * SW_INTEGRATION_GUIDE.md §4.1). Every address is a system physical address inside
 * [win_base, win_base + win_size). tile_cin < in_c splits K into in_c / tile_cin core
 * passes that accumulate in SRAM-C (in_c % tile_cin == 0). Returns 1 on success, 0 on
 * timeout, or a negative error (wrapper LAST_ERROR or -(HasNpuTop error code)).
 *
 * Convolution padding (SW_INTEGRATION_GUIDE.md §3.3, §4.7): in_h / in_w are the UNPADDED
 * input; the hardware inserts pad_value around it while it gathers the kernel windows
 * (DMA path, or the IFMAP feeder with the VP's HAS_FEEDER_PAD=1). pad_t/l/b/r = 0 and
 * pad_value = 0: no padding field is written (VALID / 1x1). `pad` is the legacy
 * all-sides PAD register; leave it 0 when the side fields are used. */
typedef struct {
    uint32_t win_base, win_size;
    uint32_t in_addr, w_addr, out_addr, bias_addr, scale_addr, shift_addr, lut_addr;
    uint32_t in_c, in_h, in_w, out_c, out_h, out_w;
    uint32_t kh, kw, stride, pad;
    uint32_t pad_t, pad_l, pad_b, pad_r; /* per-side padding of the unpadded input (PAD_T/L/B/R) */
    int32_t  pad_value;                /* int8 value of the padded positions (PAD_VALUE): input zero point */
    uint32_t act_type;                 /* 0 none, 2 LUT */
    int32_t  zp_out;
    uint32_t flags;
    uint32_t tile_cout, tile_h, tile_w, tile_cin, y_used;
} npu_has_gemm_t;

int npu_has_gemm_conv(const npu_has_gemm_t *g);

/* Whole-tensor ELEM_WISE on the V4.7 HasNpuTop vector unit (SW_INTEGRATION_GUIDE.md §4.2, §4.3), run by
 * the data flow controller on GvuTop. Same DRAM-window contract and return codes as npu_has_gemm_conv:
 * every address lies in [win_base, win_base + win_size); 1 = done, 0 = timeout, < 0 = error. */
typedef struct {
    uint32_t win_base, win_size;
    uint32_t a_addr, b_addr, out_addr;  /* b_addr may equal a_addr */
    uint32_t len;                       /* elements of A, B and OUT (no broadcast) */
    int32_t  zp_a, zp_b, zp_o;          /* int8 zero points */
    uint32_t sa, sb, so;                /* raw 32-bit multipliers (signed, SCALE_FMT I32) */
    uint32_t sha, shb, sho;             /* round-shifts 0..63 */
} npu_has_add_t;
int npu_has_elem_add(const npu_has_add_t *a);

typedef struct {
    uint32_t win_base, win_size;
    uint32_t in_addr, out_addr;
    uint32_t c, h, w;                   /* input [C][H][W] int8 */
    uint32_t k, stride, pad;            /* square window, one stride, pad value -128 on all sides */
    uint32_t mode;                      /* POOL_MODE: 0 direct window, 1 separable (same result) */
} npu_has_pool_t;
int npu_has_maxpool(const npu_has_pool_t *p);
void npu_native_soft_reset(void);
uint64_t npu_perf_read(uint32_t low_offset);

/* Per-job accumulated perf counters (see npu_v4_native_driver.c). Index order ==
 * NPUV4Runtime::PerfIdx == kPerfKeys in main.cpp == NPU_V4.7_INTEGRATION.md
 * "Raw V4.7 Performance Counters". */
enum {
  NPU_PERF_EXEC, NPU_PERF_STALL, NPU_PERF_MAC_OPS, NPU_PERF_ACTIVE_PE, NPU_PERF_TOTAL_PE, NPU_PERF_TOTAL,
  NPU_PERF_RICH_M, NPU_PERF_RICH_K, NPU_PERF_RICH_N, NPU_PERF_SA, NPU_PERF_OBP,
  NPU_PERF_PROC, NPU_PERF_XFER, NPU_PERF_MAC_ENG, NPU_PERF_DMA_ENG, NPU_PERF_ACT_ENG,
  NPU_PERF_POOL_ENG, NPU_PERF_RED_ENG, NPU_PERF_DDR_RD, NPU_PERF_DDR_WR,
  NPU_PERF_DMA_RD_CYC, NPU_PERF_DMA_WR_CYC, NPU_PERF_COUNT
};
/* Totals over every NPU job since boot: monotonic, except RICH_M/K/N (shape of the
 * last rich job). hw_jobs / stage_jobs may be NULL. */
void     npu_perf_acc_snapshot(uint64_t out[NPU_PERF_COUNT], uint32_t *hw_jobs, uint32_t *stage_jobs);
uint64_t npu_perf_acc_get(uint32_t idx);

// --- Native Driver API (Stage 3) ---
int  npu_native_stage_operands(uint32_t act_phys, uint32_t wgt_phys,
                               uint32_t scratch_phys,
                               uint32_t m, uint32_t k, uint32_t n);
                               
int  npu_native_stage_operands_sized(uint32_t act_phys, uint32_t act_bytes,
                                     uint32_t wgt_phys, uint32_t wgt_bytes,
                                     uint32_t scratch_phys);

void npu_native_program_core(const npu_layer_desc_t *d,
                             const npu_core_fields_t *f,
                             uint32_t k_dim);

void npu_native_program_obp(const int32_t *bias, const int32_t *multiplier,
                            const int32_t *shift, const uint8_t *lut,
                            uint32_t lanes, uint32_t cfg);

void npu_native_start(void);
int  npu_native_wait_done(uint32_t max_iters);
/* Drains one native job's OBP output out of the SRAM C host window.
 *   dst        : destination for INT8 results
 *   channels   : number of active OC lanes in this tile (<= FX1_SA_X)
 *   rows       : number of spatial rows produced (<= FX1_NATIVE_Y_USED per ctx)
 *   row_stride : bytes between consecutive destination rows
 *                (CONV2D: FX1_SA_TILE for the tile-local scratch;
 *                 FC:     the full output channel count N)
 */
void npu_native_read_sramc(int8_t *dst, uint32_t channels, uint32_t rows,
                           uint32_t row_stride);

// --- OBP Hardware Control (Lane A) ---
void npu_obp_set_cfg_a(uint32_t cfg);
void npu_obp_set_default_requant_a(uint32_t scale, uint32_t shift);
void npu_obp_load_bias_a(const int32_t *v, uint32_t lanes);
void npu_obp_load_scale_a(const uint32_t *v, uint32_t lanes);
void npu_obp_load_shift_a(const uint32_t *v, uint32_t lanes);
void npu_obp_load_lut_a(const uint8_t *lut, uint32_t lanes);

// --- RCE Hardware Control (Lane A) ---
void npu_rce_load_exp_a(const uint8_t *lut_exp);
void npu_rce_load_recip_a(const uint8_t *lut_recip);
void npu_rce_load_rsqrt_a(const uint16_t *lut_rsqrt);

#ifdef __cplusplus
}
#endif
