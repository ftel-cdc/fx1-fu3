/* Bare-metal comparison bridge using a bundled profile-mode reference driver.
 * Neon owns startup/linker/runtime. No external job.bin/profile.bin is loaded.
 */
#include "fx1_fw.h"
#include "isp_demo_setup.h"
#include "profile_data.h"
#define ISP_DMA_ERR_FATAL (FX1_ISP_DMA_ERR_ERR_ALIGN_OR_GEOMETRY_BIT | FX1_ISP_DMA_ERR_ERR_IDMA_AXI_BIT | FX1_ISP_DMA_ERR_ERR_ODMA_AXI_BIT)
static fx1_isp_dev dev;
static uint32_t read32(void *ctx, uint32_t off)
{ (void)ctx; return fx1_read32(FX1_ISP_CSR_BASE + off); }
static void write32(void *ctx, uint32_t off, uint32_t value)
{ (void)ctx; fx1_write32(FX1_ISP_CSR_BASE + off, value); fx1_mb(); }
static void delay(void *ctx, uint32_t cycles)
{ (void)ctx; while (cycles--) __asm__ volatile("nop"); }
static void show(const char *name, uint32_t value)
{ fx1_puts(name); fx1_puts(" = "); fx1_puthex(value); fx1_puts("\n"); }
#ifndef FX1_PROFILE_DISABLE_ID
#define FX1_PROFILE_DISABLE_ID 0
#endif
static const struct { const char *name; uint32_t offset; } block_controls[] = {
    {"BLC", FX1_ISP_BLC_CTRL_OFFSET},
    {"LSC", FX1_ISP_LSC_CTRL_OFFSET},
    {"BPC", FX1_ISP_BPC_CTRL_OFFSET},
    {"WB", FX1_ISP_WB_CTRL_OFFSET},
    {"DG", FX1_ISP_DG_CTRL_OFFSET},
    {"CCM", FX1_ISP_CCM_CTRL_OFFSET},
    {"GAMMA", FX1_ISP_GAMMA_CTRL_OFFSET},
    {"GTM", FX1_ISP_GTM_CTRL_OFFSET},
    {"NR_2D", FX1_ISP_NR_2D_CTRL_OFFSET},
    {"EE", FX1_ISP_EE_CTRL_OFFSET},
    {"CNF", FX1_ISP_CNF_CTRL_OFFSET},
    {"AEC", FX1_ISP_AEC_CTRL_OFFSET},
    {"AWB", FX1_ISP_AWB_CTRL_OFFSET},
    {"AF", FX1_ISP_AF_CTRL_OFFSET},
    {"RESIZER", FX1_ISP_RESIZER_CTRL_OFFSET},
};
int main(void)
{
    fx1_puts("[PASS] FW_BOOT: Neon embedded full D65 profile, reference replay bridge\n");
    fx1_isp_hal hal = {read32, write32, delay, NULL};
    int rc = fx1_isp_init(&dev, &hal);
    show("INIT_RC", rc);
    if (rc) return 1;
    const fx1_isp_reg_write *selected_profile = profile;
#if defined(FX1_PROFILE_BYPASS_BLC_LSC) || defined(FX1_PROFILE_BYPASS_LSC)
    static fx1_isp_reg_write bypass_profile[sizeof profile / sizeof profile[0]];
    unsigned changed = 0;
    for (unsigned i = 0; i < sizeof profile / sizeof profile[0]; ++i) {
        bypass_profile[i] = profile[i];
        if (profile[i].offset == FX1_ISP_LSC_CTRL_OFFSET
#ifdef FX1_PROFILE_BYPASS_BLC_LSC
            || profile[i].offset == FX1_ISP_BLC_CTRL_OFFSET
#endif
        ) {
            bypass_profile[i].value &= ~1u;
            ++changed;
        }
    }
#ifdef FX1_PROFILE_BYPASS_BLC_LSC
    if (changed != 2u) return 9;
#else
    if (changed != 1u) return 9;
#endif
    selected_profile = bypass_profile;
#ifdef FX1_PROFILE_BYPASS_BLC_LSC
    fx1_puts("[INFO] Profile override: BLC=OFF LSC=OFF; all other writes unchanged\n");
#else
    fx1_puts("[INFO] Profile override: BLC=ON LSC=OFF; all other writes unchanged\n");
#endif
#endif
#if FX1_PROFILE_DISABLE_ID > 0
    static fx1_isp_reg_write single_profile[sizeof profile / sizeof profile[0]];
    uint32_t target = block_controls[FX1_PROFILE_DISABLE_ID - 1].offset;
    unsigned found = 0;
    for (unsigned i = 0; i < sizeof profile / sizeof profile[0]; ++i) {
        single_profile[i] = profile[i];
        if (profile[i].offset == target) {
            single_profile[i].value &= ~1u;
            ++found;
        }
    }
    if (!found) return 11;
    selected_profile = single_profile;
    fx1_puts("DISABLED_BLOCK=");
    fx1_puts(block_controls[FX1_PROFILE_DISABLE_ID - 1].name); fx1_puts("\n");
#endif
    uint32_t error = 0;
    rc = isp_demo_configure(&dev, &job, selected_profile, isp_demo_mode(&job), &error);
    show("CONFIG_RC", rc); show("TUNE_ERROR", error);
    if (rc) return 2;
    show("BLC_CTRL", read32(NULL, FX1_ISP_BLC_CTRL_OFFSET));
    show("LSC_CTRL", read32(NULL, FX1_ISP_LSC_CTRL_OFFSET));
#ifdef FX1_PROFILE_BYPASS_BLC_LSC
    if ((read32(NULL, FX1_ISP_BLC_CTRL_OFFSET) & 1u) ||
        (read32(NULL, FX1_ISP_LSC_CTRL_OFFSET) & 1u)) return 10;
#endif
#ifdef FX1_PROFILE_BYPASS_LSC
    if (!(read32(NULL, FX1_ISP_BLC_CTRL_OFFSET) & 1u) ||
        (read32(NULL, FX1_ISP_LSC_CTRL_OFFSET) & 1u)) return 10;
#endif
#if FX1_PROFILE_DISABLE_ID > 0
    for (unsigned i = 0; i < sizeof block_controls / sizeof block_controls[0]; ++i) {
        uint32_t enabled = read32(NULL, block_controls[i].offset) & 1u;
        show(block_controls[i].name, enabled);
        if (enabled != (i == FX1_PROFILE_DISABLE_ID - 1 ? 0u : 1u)) return 12;
    }
    fx1_puts("[PASS] Exactly one block OFF, other 14 ON\n");
#endif
    fx1_isp_buffers buffers;
    for (unsigned i = 0; i < 4; ++i) {
        buffers.in[i] = job.in_addr;
        buffers.y[i] = job.y_addr;
        buffers.uv[i] = job.uv_addr;
    }
    buffers.in_stride = job.in_stride;
    buffers.y_stride = buffers.uv_stride = job.out_stride;
    buffers.max_burst_beats = 16;
    rc = fx1_isp_start(&dev, &buffers);
    show("START_RC", rc);
    if (rc) return 3;
    show("OUTPUT_WIDTH", read32(NULL, FX1_ISP_RESIZER_OUT_W_OFFSET));
    show("OUTPUT_HEIGHT", read32(NULL, FX1_ISP_RESIZER_OUT_H_OFFSET));
    for (unsigned frame = 0; frame < job.frames; ++frame) {
        fx1_dma_prepare((void *)(uintptr_t)job.in_addr, job.in_stride * job.height, FX1_DMA_TO_DEVICE);
        if (!fx1_isp_input_ready(&dev) || fx1_isp_queue_frame(&dev)) return 4;
        uint64_t deadline = fx1_mtime() + FX1_MTIME_HZ;
        unsigned slot = 0;
        int done = 0;
        do {
            fx1_isp_events events;
            fx1_isp_irq(&dev, &events);
            if (events.dma_err & ISP_DMA_ERR_FATAL) return 5;
            rc = fx1_isp_next_done(&dev, &slot);
            if (rc == FX1_ISP_OK) { done = 1; break; }
        } while (fx1_mtime() < deadline);
        if (!done) return 6;
        show("FRAME_DONE", frame + 1);
        uint32_t profile_id = 0;
        fx1_isp_output_meta(&dev, 0, slot, NULL, NULL, &profile_id);
        show("PROFILE_ID", profile_id);
        if (profile_id != job.profile_id) return 7;
    }
    fx1_dma_complete((void *)(uintptr_t)job.y_addr, job.out_stride * job.height, FX1_DMA_FROM_DEVICE);
    fx1_dma_complete((void *)(uintptr_t)job.uv_addr, job.out_stride * job.height / 2, FX1_DMA_FROM_DEVICE);
    if (fx1_isp_stop(&dev)) return 8;
    fx1_puts("PROFILE_REPLAY_RESULT=PASS\n");
    return 0;
}
