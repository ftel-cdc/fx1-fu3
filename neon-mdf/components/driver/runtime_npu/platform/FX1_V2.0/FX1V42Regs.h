#pragma once

#include <stdint.h>

// ============================================================
// FX1 SAURIA V4.4 Hardware MMIO Registers Map
//
// Base: CDC_NPU0_BASE (0x10200000)
// Aperture: 1 MiB (0x10200000..0x102FFFFF)
// ============================================================

#ifndef CDC_NPU0_BASE
#define CDC_NPU0_BASE 0x10200000UL
#endif

// ============================================================
// SAURIA V4.5 "Neo Core Lite 32" geometry.
// Authority: npu_tlm.cpp (kArrayRows/kArrayCols = 32),
//            v4.5_model/npu_top.h, v4.5_model/sram/sram_top.h.
// ============================================================
#define FX1_SA_X                      32u      // X_DIM — PE columns / output channels
#define FX1_SA_Y                      32u      // Y_DIM — PE rows / spatial pixels
#define FX1_SA_TILE                   32u      // X_DIM == Y_DIM

#define FX1_SRAMA_BANK_BYTES          (79u * 1024u)    // 80896 — IFmap, per ping-pong bank
#define FX1_SRAMB_BANK_BYTES          (81u * 1024u)    // 82944 — Weight, per bank
#define FX1_SRAMC_BANK_BYTES          (96u * 1024u)    // 98304 — PSum, per bank
#define FX1_SCRATCH_BANK_BYTES        (24u * 1024u)    // 24576 — RE/LUT scratch, per lane
#define FX1_SRAM_TOTAL_BYTES          (560u * 1024u)   // 158 + 162 + 192 + 48 KiB

#define FX1_OBP_LANES                 32u       // V4.5 32-mode channel OBP tables
#define FX1_OBP_LUT_BYTES_PER_LANE    256u
#define FX1_OBP_LUT_TOTAL_BYTES       (FX1_OBP_LANES * FX1_OBP_LUT_BYTES_PER_LANE)  // 8192

// --- Native Common & Profile Registers ---
#define FX1_REG_NATIVE_CONTROL        0x0000UL
#define FX1_REG_NATIVE_CFG_PROFILE    0x0004UL

#define FX1_PROFILE_V1_SAURIA         0u
#define FX1_PROFILE_V4_LINEAR         1u

// --- OBP & Requantization Configuration Registers (Lane A) ---
#define FX1_REG_OBP_CFG_A             0x0820UL
#define FX1_REG_REQUANT_SCALE_A       0x0824UL
#define FX1_REG_REQUANT_SHIFT_A       0x0828UL

// --- Controller + activation-feeder registers (PROFILE_V1_SAURIA) ---
// Implicit-im2col programming interface. Authority: NPU_V4.4_INTEGRATION.md,
// "Implicit im2col Programming" + "Activation Feeder".
#define FX1_NATIVE_CON_INCNTLIM       0x0200UL
#define FX1_NATIVE_CON_ACT_REPS       0x0204UL
#define FX1_NATIVE_CON_WEI_REPS       0x0208UL
#define FX1_NATIVE_CON_NSPLIT         0x0214UL

#define FX1_NATIVE_ACT_ROWS_ACTIVE    0x0400UL
#define FX1_NATIVE_ACT_INCNTLIM       0x0404UL
#define FX1_NATIVE_ACT_INCNTSTEP      0x0408UL
#define FX1_NATIVE_ACT_XLIM           0x0414UL
#define FX1_NATIVE_ACT_XSTEP          0x0418UL
#define FX1_NATIVE_ACT_YLIM           0x041CUL
#define FX1_NATIVE_ACT_YSTEP          0x0420UL
#define FX1_NATIVE_ACT_CHLIM          0x0424UL
#define FX1_NATIVE_ACT_DIL_PAT        0x0428UL
#define FX1_NATIVE_ACT_CHSTEP         0x042CUL
#define FX1_NATIVE_ACT_TIL_XLIM       0x0430UL
#define FX1_NATIVE_ACT_TIL_XSTEP      0x0434UL
#define FX1_NATIVE_ACT_TIL_YLIM       0x0438UL
#define FX1_NATIVE_ACT_TIL_YSTEP      0x043CUL
#define FX1_NATIVE_CFG_ACT_BASE_ADDR  0x0480UL

// --- Weight feeder (0x0600 block) ---
#define FX1_NATIVE_WEI_INCNTLIM       0x0604UL
#define FX1_NATIVE_WEI_INCNTSTEP      0x0608UL
#define FX1_NATIVE_WEI_WLIM           0x0610UL
#define FX1_NATIVE_WEI_WSTEP          0x0614UL
#define FX1_NATIVE_WEI_KLIM           0x0618UL
#define FX1_NATIVE_WEI_KSTEP          0x061CUL
#define FX1_NATIVE_WEI_TIL_XLIM       0x0620UL
#define FX1_NATIVE_WEI_TIL_XSTEP      0x0624UL
#define FX1_NATIVE_WEI_COLS_ACTIVE    0x0628UL
#define FX1_NATIVE_WEI_WALIGNED       0x062CUL
#define FX1_NATIVE_CFG_WEI_BASE_ADDR  0x0680UL

// --- Output / PSM (0x0800 block) ---
#define FX1_NATIVE_NCONTEXTS          0x0800UL
#define FX1_NATIVE_OUT_CXLIM          0x0804UL
#define FX1_NATIVE_OUT_CXSTEP         0x0808UL
#define FX1_NATIVE_OUT_CKLIM          0x080CUL
#define FX1_NATIVE_OUT_CKSTEP         0x0810UL
#define FX1_NATIVE_TIL_CYLIM          0x0814UL
#define FX1_NATIVE_TIL_CYSTEP         0x0818UL
#define FX1_NATIVE_TIL_CKLIM          0x081CUL
#define FX1_NATIVE_CFG_OUT_BASE_ADDR  0x0880UL
// NOTE: 0x0820/0x0824/0x0828 are F_TIL_CKSTEP/F_INACTIVE_COLS/F_PRELOAD_EN in the
// V4.4 field list, but the OBP-A decode branches win there (integration doc lines
// 288-290). NEVER write them as tiling fields — they ARE OBP_CFG_A /
// REQUANT_SCALE_A / REQUANT_SHIFT_A.

// --- Native SRAM host windows ---
// Byte offset inside a window == model WORD index: (phys_row << 3) | subword,
// subword 0..7, each word carrying 4 int8 lanes (sram_top.h: SUBWORDS = DIM/4).
// The 4-byte MMIO alignment rule makes only subword 0 and 4 reachable, i.e.
// lanes 0-3 and 16-19. Reads of SRAM C therefore use subword 0 only.
#define FX1_NATIVE_SRAMA_BASE         0x40000UL
#define FX1_NATIVE_SRAMB_BASE         0x80000UL
#define FX1_NATIVE_SRAMC_BASE         0xC0000UL
#define FX1_SRAM_SUBWORDS_PER_ROW     8u        // was 16
#define FX1_SRAM_SUBWORD_SHIFT        3u        // was implicit 4
#define FX1_SRAM_LANES_PER_WORD       4u

/* Only sub_word 0 is both 4-byte aligned and lane-contiguous, so the host can
   read back at most 4 consecutive PSUM rows per physical row. Keep Y_used = 4. */
#define FX1_NATIVE_Y_USED             4u
#define FX1_NATIVE_X_USED             FX1_SA_X   // was 64

// --- OBP lane B mirrors (V4.5) ---
#define FX1_REG_OBP_CFG_B             0x0830UL
#define FX1_REG_REQUANT_SCALE_B       0x0834UL
#define FX1_REG_REQUANT_SHIFT_B       0x0838UL

// OBP Config bitmask (obp_cfg_unpack)
#define FX1_OBP_CFG_BIAS_EN           (1u << 0)
#define FX1_OBP_CFG_REQUANT_EN        (1u << 1)
#define FX1_OBP_CFG_LUT_EN            (1u << 2)
#define FX1_OBP_CFG_RESIDUAL_EN       (1u << 3)
// V4.5: per-vector channel auto-indexing (OS scan-chain drain emits 1 channel
// per vector transaction). Mirrors CDC_NPU_OBP_CFG_VEC_CHANNEL_MODE.
#define FX1_OBP_CFG_VEC_CHANNEL_MODE  (1u << 8)

// Native Control register bit definitions
#define FX1_NATIVE_CTRL_START_PULSE   (1u << 0)
#define FX1_NATIVE_CTRL_CLEAR_DONE    0u
#define FX1_NATIVE_STATUS_DONE        (1u << 1)
#define FX1_NATIVE_STATUS_IDLE        (1u << 2)
#define FX1_NATIVE_STATUS_READY       (1u << 3)
#define FX1_NATIVE_RESET_BYTE(val)    (((val) & 0xFFu) << 16)

// --- Native Performance Counters (64-bit: Low / High pair) ---
#define FX1_PERF_EXEC_CYCLES_LO       0x1200UL
#define FX1_PERF_EXEC_CYCLES_HI       0x1240UL
#define FX1_PERF_STALL_CYCLES_LO      0x1204UL
#define FX1_PERF_STALL_CYCLES_HI      0x1244UL
#define FX1_PERF_MAC_OPS_LO           0x1208UL
#define FX1_PERF_MAC_OPS_HI           0x1248UL
#define FX1_PERF_ACTIVE_PE_LO         0x120CUL
#define FX1_PERF_ACTIVE_PE_HI         0x124CUL
#define FX1_PERF_TOTAL_PE_LO          0x1210UL
#define FX1_PERF_TOTAL_PE_HI          0x1250UL
#define FX1_PERF_TOTAL_CYCLES_LO      0x1214UL
#define FX1_PERF_TOTAL_CYCLES_HI      0x1254UL
#define FX1_PERF_SA_ACTIVE_LO         0x1224UL
#define FX1_PERF_SA_ACTIVE_HI         0x1264UL
#define FX1_PERF_OBP_ACTIVE_LO        0x1228UL
#define FX1_PERF_OBP_ACTIVE_HI        0x1268UL
// Accumulated rich-operation GEMM dimensions (GEMM_FUSED/FUSED_ATTN/LAYERNORM/
// ELEM_WISE), summed by V4.5 across every rich instruction executed since the
// last soft reset. Same legacy LO+0x40 stride as the rest of this block.
#define FX1_PERF_RICH_M_LO            0x1218UL
#define FX1_PERF_RICH_M_HI            0x1258UL
#define FX1_PERF_RICH_K_LO            0x121CUL
#define FX1_PERF_RICH_K_HI            0x125CUL
#define FX1_PERF_RICH_N_LO            0x1220UL
#define FX1_PERF_RICH_N_HI            0x1260UL

/* Expanded raw counters added by v4.4_model_25Aug. NOTE the HI block for these
 * is at LO + 0x80, NOT the LO + 0x40 used by the 0x1200-block — see
 * npu_perf_read() in bsp_ext/npu_v4_native_driver.c. */
#define FX1_PERF_PROCESSING_CYCLES_LO   0x1280u
#define FX1_PERF_TRANSFER_CYCLES_LO     0x1284u
#define FX1_PERF_MAC_ENGINE_CYCLES_LO   0x1288u
#define FX1_PERF_DMA_ENGINE_CYCLES_LO   0x128Cu
#define FX1_PERF_ACT_ENGINE_CYCLES_LO   0x1290u
#define FX1_PERF_POOL_ENGINE_CYCLES_LO  0x1294u
#define FX1_PERF_RED_ENGINE_CYCLES_LO   0x1298u
#define FX1_PERF_DDR_READ_BYTES_LO      0x129Cu
#define FX1_PERF_DDR_WRITE_BYTES_LO     0x12A0u
#define FX1_PERF_DMA_READ_CYCLES_LO     0x12A4u
#define FX1_PERF_DMA_WRITE_CYCLES_LO    0x12A8u

#define FX1_PERF_HI_STRIDE_LEGACY       0x40u   /* 0x1200-block: HI = LO + 0x40 */
#define FX1_PERF_HI_STRIDE_EXTENDED     0x80u   /* 0x1280-block: HI = LO + 0x80 */
#define FX1_PERF_EXTENDED_LO_FIRST      0x1280u

// --- Rich Instruction Push & Config Registers ---
// Note: Push rich instructions by writing opcode into PUSH_A / PUSH_B
#define FX1_RICH_INST_LO_A            0x10300UL
#define FX1_RICH_INST_HI_A            0x10304UL
#define FX1_RICH_INST_LO_B            0x10308UL
#define FX1_RICH_INST_HI_B            0x1030CUL
#define FX1_RICH_PUSH_A               0x10310UL
#define FX1_RICH_PUSH_B               0x10314UL

// Rich Instruction Field Registers (0x10400..0x10460)
#define FX1_RICH_IN_ADDR              0x10400UL
#define FX1_RICH_WEIGHT_ADDR          0x10404UL
#define FX1_RICH_OUT_ADDR             0x10408UL
#define FX1_RICH_BIAS_ADDR            0x1040CUL
#define FX1_RICH_M                    0x10410UL
#define FX1_RICH_K                    0x10414UL
#define FX1_RICH_N                    0x10418UL
#define FX1_RICH_KERNEL_H             0x1041CUL
#define FX1_RICH_KERNEL_W             0x10420UL
#define FX1_RICH_STRIDE               0x10424UL
#define FX1_RICH_PADDING              0x10428UL
#define FX1_RICH_ACT_TYPE             0x1042CUL
#define FX1_RICH_HAS_SKIP             0x10430UL
#define FX1_RICH_SKIP_ADDR            0x10434UL
#define FX1_RICH_IN_SCALE             0x10438UL
#define FX1_RICH_WEIGHT_SCALE         0x1043CUL
#define FX1_RICH_OUT_SCALE            0x10440UL

// Aliased fields across opcodes
#define FX1_RICH_Q_GAMMA_A_ADDR       0x10444UL // q_addr (ATTN), gamma_addr (LAYERNORM), a_addr (ELEM_WISE)
#define FX1_RICH_K_B_ADDR             0x10448UL // k_addr (ATTN), b_addr (ELEM_WISE)
#define FX1_RICH_V_BETA_ADDR          0x1044CUL // v_addr (ATTN), beta_addr (LAYERNORM)
#define FX1_RICH_SEQ_LEN              0x10450UL // seq_len (ATTN), len (ELEM_WISE)
#define FX1_RICH_HEADS_DIM_MODE       0x10454UL // num_heads (ATTN), dim (LAYERNORM), mode (ELEM_WISE)
#define FX1_RICH_HEAD_DIM_EPS         0x10458UL // head_dim (ATTN), eps_shift (LAYERNORM), scale_a (ELEM_WISE)
#define FX1_RICH_ATTN_SCALE           0x1045CUL // attn_scale (ATTN), scale_b (ELEM_WISE)
#define FX1_RICH_SCALE_OUT            0x10460UL // scale_out (ELEM_WISE)

/* 25Aug rich ABI: explicit ELEM_WISE source lengths (VP 0x10464/0x10468). */
#define FX1_RICH_A_LEN                0x10464u
#define FX1_RICH_B_LEN                0x10468u

// --- Compact OBP & RCE Aliases (V4.5: 32-lane OBP) ---
#define FX1_OBP_A_LUT_BASE            0x20000UL // 32 lanes x 256 B = 8 KB (window 0x20000..0x21FFF)
#define FX1_OBP_A_BIAS_BASE           0x24000UL // 32 x int32 (0x24000..0x2407F)
#define FX1_OBP_A_SCALE_BASE          0x25000UL
#define FX1_OBP_A_SHIFT_BASE          0x26000UL
#define FX1_OBP_B_LUT_BASE            0x28000UL // 32 lanes x 256 B = 8 KB
#define FX1_OBP_B_BIAS_BASE           0x2C000UL
#define FX1_OBP_B_SCALE_BASE          0x2D000UL
#define FX1_OBP_B_SHIFT_BASE          0x2E000UL

#define FX1_RCE_A_EXP_BASE            0x32000UL
#define FX1_RCE_A_RECIP_BASE          0x33000UL
#define FX1_RCE_A_RSQRT_BASE          0x34000UL

#define FX1_RCE_B_EXP_BASE            0x35000UL
#define FX1_RCE_B_RECIP_BASE          0x36000UL
#define FX1_RCE_B_RSQRT_BASE          0x37000UL

// --- Rich Opcodes ---
#define FX1_RICH_OP_SET_NSPLIT        0x05u
#define FX1_RICH_OP_GEMM_FUSED        0x12u
#define FX1_RICH_OP_FUSED_ATTN        0x13u
#define FX1_RICH_OP_LAYERNORM         0x14u
#define FX1_RICH_OP_ELEM_WISE         0x15u

// Rich Activation types
#define FX1_RICH_ACT_NONE             0u
#define FX1_RICH_ACT_RELU             1u
#define FX1_RICH_ACT_SILU             2u
#define FX1_RICH_ACT_GELU             3u

// Rich Element-wise modes
#define FX1_RICH_ELEM_ADD             0u
#define FX1_RICH_ELEM_MAXPOOL         1u
#define FX1_RICH_ELEM_MUL             2u
#define FX1_RICH_ELEM_SUB             3u
#define FX1_RICH_ELEM_DIV             4u

/* ---- V4.7 HasNpuTop extended GEMM_FUSED (HAS_CONV2D, K x C_in split) ----
 * Private DRAM window of one extended instruction. The VP copies the whole window
 * into the NPU model before the push and the whole window back to RAM after
 * completion (npu_tlm.cpp prepare_rich_job / service_rich_completions), so it must
 * never overlap firmware code, stack, heap, the tensor arena (0x8F000000) or the
 * model blob (0x8C000000). 0x8B000000..0x8BFFFFFF is the unused top 16 MiB of
 * CDC_NPU_WORK0 (native scratch at its bottom, rich im2col staging at 0x8A000000).
 * Contents are transient: staged and consumed within one HAS_CONV2D. */
#define FX1_HAS_WIN_BASE   0x8B000000UL
#define FX1_HAS_WIN_SIZE   0x01000000UL

