/* D65 BGGR RAW12 loaded at RAW_BASE; packed NV12 dumped at OUTPUT_BASE. */
#include "fx1_fw.h"
#include "pipeline/ftel_isp_stream.h"
#ifdef FX1_IMAGE_D65_PROFILE
#include "d65_profile.h"
#endif
#define WIDTH 2688u
#define HEIGHT 1520u
#define RAW_BASE 0x81000000u
#define OUTPUT_BASE 0x88000000u
#define SLOT_BYTES 0x00800000u
#define Y_BYTES (WIDTH * HEIGHT)
#define NV12_BYTES (Y_BYTES * 3u / 2u)
_Static_assert(WIDTH * HEIGHT * 2u < SLOT_BYTES, "RAW slot capacity");
_Static_assert(RAW_BASE + 4u * SLOT_BYTES <= FX1_FW_YUV_BUF_BASE, "RAW region");
_Static_assert(OUTPUT_BASE + 4u * SLOT_BYTES < FX1_FW_TEST_AREA_BASE, "output region");
static uint64_t time_us(void *unused)
{
    (void)unused;
    return fx1_mtime() / (FX1_MTIME_HZ / 1000000u);
}
static void show(const char *name, uint32_t value)
{
    fx1_puts(name); fx1_puts(" = "); fx1_puthex(value); fx1_puts("\n");
}
int main(void)
{
#if FX1_IMAGE_ALL_BLOCKS
    fx1_puts("[PASS] FW_BOOT: Neon D65 all supported blocks ON\n");
#else
    fx1_puts("[PASS] FW_BOOT: Neon D65 image BLC=OFF LSC=OFF\n");
#endif
    static isp_config_t cfg;
    isp_get_default_config(&cfg);
    cfg.ifmt = (ifmt_config_t){WIDTH, HEIGHT, BAYER_BGGR, INPUT_DEPTH_RAW12};
    cfg.blc.enable = FX1_IMAGE_ALL_BLOCKS;
    cfg.lsc.enable = FX1_IMAGE_ALL_BLOCKS;
#if FX1_IMAGE_ALL_BLOCKS
    cfg.bpc.enable = cfg.wb.enable = cfg.dg.enable = true;
    cfg.ccm.enable = cfg.gamma.enable = cfg.gtm.enable = true;
    cfg.nr.enable = cfg.ee.enable = cfg.cnf.enable = true;
#endif
    /* Avoid the known LSC table-0 init limitation; driver defaults unchanged. */
    cfg.lsc.coefficients[0] = NULL;
    cfg.lsc.coefficient_count[0] = 0;
    cfg.lsc.profile = 1;
    aec_get_default_config(&cfg.aec, WIDTH, HEIGHT);
    cfg.aec.enable = cfg.awb.enable = cfg.af.enable = FX1_IMAGE_ALL_BLOCKS;
#ifdef FX1_IMAGE_D65_PROFILE
    apply_d65_profile(&cfg);
#endif
    output_get_default_config(&cfg.output, WIDTH, HEIGHT);
    output_geometry_t geometry[3];
    for (unsigned c = 0; c < 3; ++c)
        if (output_compute_geometry(&cfg.output.channel[c], c, WIDTH, HEIGHT, &geometry[c]) != ISP_OK)
            return 1;
    dma_get_default_config(&cfg.dma, WIDTH, geometry);
    cfg.dma.input_stride = WIDTH * 2u;
    cfg.dma.output[0].y_stride = cfg.dma.output[0].uv_stride = WIDTH;
    for (unsigned slot = 0; slot < 4; ++slot) {
        cfg.dma.input[slot] = RAW_BASE + slot * SLOT_BYTES;
        cfg.dma.output[0].y[slot] = OUTPUT_BASE + slot * SLOT_BYTES;
        cfg.dma.output[0].uv[slot] = cfg.dma.output[0].y[slot] + Y_BYTES;
    }
    isp_wait_t wait = {time_us, NULL, 2000000, 10000000};
    isp_init_result_t init;
    isp_status_t st = isp_init(&cfg, &wait, &init);
    show("INIT_STATUS", st); show("INIT_STAGE", init.stage);
    if (st != ISP_OK) return 2;
    show("BLC_CTRL", REG_RD(R_BLC_CTRL));
    show("LSC_CTRL", REG_RD(R_LSC_CTRL));
    if ((REG_RD(R_BLC_CTRL) & 1u) != FX1_IMAGE_ALL_BLOCKS ||
        (REG_RD(R_LSC_CTRL) & 1u) != FX1_IMAGE_ALL_BLOCKS) return 3;
#if FX1_IMAGE_ALL_BLOCKS
    static const struct { const char *name; uint32_t offset; } controls[] = {
        {"BLC", FX1_ISP_BLC_CTRL_OFFSET}, {"LSC", FX1_ISP_LSC_CTRL_OFFSET},
        {"BPC", FX1_ISP_BPC_CTRL_OFFSET}, {"WB", FX1_ISP_WB_CTRL_OFFSET},
        {"DG", FX1_ISP_DG_CTRL_OFFSET}, {"CCM", FX1_ISP_CCM_CTRL_OFFSET},
        {"GAMMA", FX1_ISP_GAMMA_CTRL_OFFSET}, {"GTM", FX1_ISP_GTM_CTRL_OFFSET},
        {"NR_2D", FX1_ISP_NR_2D_CTRL_OFFSET}, {"EE", FX1_ISP_EE_CTRL_OFFSET},
        {"CNF", FX1_ISP_CNF_CTRL_OFFSET}, {"AEC", FX1_ISP_AEC_CTRL_OFFSET},
        {"AWB", FX1_ISP_AWB_CTRL_OFFSET}, {"AF", FX1_ISP_AF_CTRL_OFFSET}
    };
    for (unsigned i = 0; i < sizeof controls / sizeof controls[0]; ++i) {
        uint32_t value = REG_RD(controls[i].offset);
        show(controls[i].name, value);
        if (!(value & 1u)) return 10;
    }
    fx1_puts("[PASS] All 14 supported block enable bits read back ON\n");
#else
    fx1_puts("[PASS] BLC and LSC enable bits are zero\n");
#endif
#ifdef FX1_IMAGE_D65_PROFILE
    show("PROFILE_WB_R_Q8", REG_RD(FX1_ISP_WB_GAIN_R_OFFSET));
    show("PROFILE_WB_G_Q8", REG_RD(FX1_ISP_WB_GAIN_G_OFFSET));
    show("PROFILE_WB_B_Q8", REG_RD(FX1_ISP_WB_GAIN_B_OFFSET));
    show("PROFILE_DG_Q8", REG_RD(FX1_ISP_DG_GAIN_OFFSET));
    if (REG_RD(FX1_ISP_WB_GAIN_R_OFFSET) != 591u ||
        REG_RD(FX1_ISP_WB_GAIN_G_OFFSET) != 256u ||
        REG_RD(FX1_ISP_WB_GAIN_B_OFFSET) != 393u ||
        REG_RD(FX1_ISP_DG_GAIN_OFFSET) != 557u) return 11;
    for (unsigned i = 0; i < 9; ++i) {
        uint32_t value = REG_RD(FX1_ISP_CCM_CRR_OFFSET + 4u * i) & 0xfffu;
        show("PROFILE_CCM_Q9", value);
        if (value != ((uint32_t)cfg.ccm.matrix[i] & 0xfffu)) return 12;
    }
    fx1_puts("[PASS] D65 WB/DG/CCM profile readback\n");
#endif
    fx1_dma_prepare((void *)(uintptr_t)RAW_BASE, WIDTH * HEIGHT * 2u, FX1_DMA_TO_DEVICE);
    fx1_dma_prepare((void *)(uintptr_t)OUTPUT_BASE, NV12_BYTES, FX1_DMA_FROM_DEVICE);
    isp_stream_t stream = {0};
    st = isp_stream_start(&stream, &cfg, &wait);
    if (st != ISP_OK) return 4;
    st = isp_stream_submit(&stream, 0xd650u, 1000u);
    if (st != ISP_OK) return 5;
    isp_frame_result_t frame;
    st = isp_stream_wait(&stream, &wait, &frame);
    show("WAIT_STATUS", st);
    show("FRAME_SLOT", frame.slot); show("DMA_ERRORS", frame.dma.errors);
    show("INPUT_COUNT", frame.dma.input_count); show("OUTPUT_COUNT", frame.dma.output_count[0]);
    show("OUTPUT_TAG", frame.tag[0]);
    if (st != ISP_OK || frame.slot != 0 ||
        (frame.dma.errors & ~DMA_ERROR_INPUT_PAUSED) ||
        frame.dma.input_count != 1 || frame.dma.output_count[0] != 1 ||
        frame.tag[0] != 0xd650u) return 6;
    fx1_dma_complete((void *)(uintptr_t)OUTPUT_BASE, NV12_BYTES, FX1_DMA_FROM_DEVICE);
    if (isp_stream_release(&stream) != ISP_OK) return 7;
    if (isp_stream_stop(&stream, 100000) != ISP_OK) return 8;
    if ((REG_RD(R_BLC_CTRL) & 1u) != FX1_IMAGE_ALL_BLOCKS ||
        (REG_RD(R_LSC_CTRL) & 1u) != FX1_IMAGE_ALL_BLOCKS || REG_RD(R_DMA_ERR)) return 9;
    show("NV12_BASE", OUTPUT_BASE); show("NV12_BYTES", NV12_BYTES);
    fx1_puts("IMAGE_RESULT=PASS\n");
    return 0;
}
