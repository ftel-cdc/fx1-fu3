/*
 * Kiểm chứng init tổng bằng driver thật: đọc lại cấu hình, trạng thái bảng,
 * hình học, reset lại và đường lỗi. Không tạo frame để né giới hạn của VP.
 */
#include "full_init_check.h"
#include "pipeline/ftel_isp_init.h"

typedef struct {
    const init_test_platform_t *platform;
    unsigned passed, failed, known_limits;
} full_test_t;

static uint64_t platform_time(void *user)
{
    const init_test_platform_t *platform = user;
    return platform->time_us();
}

static char *append(char *out, const char *text)
{
    while (*text) *out++ = *text++;
    *out = '\0';
    return out;
}

static char *append_unsigned(char *out, unsigned value)
{
    char reversed[10];
    unsigned count = 0;
    do {
        reversed[count++] = (char)('0' + value % 10u);
        value /= 10u;
    } while (value);
    while (count) *out++ = reversed[--count];
    *out = '\0';
    return out;
}

static bool check(full_test_t *test, const char *name, bool good)
{
    char message[128];
    char *out = append(message, good ? "[PASS] " : "[FAIL] ");
    append(out, name);
    test->platform->report(message, good ? 0u : 1u);
    if (good) test->passed++;
    else test->failed++;
    return good;
}

static void case_result(full_test_t *test, const char *name, const char *verdict)
{
    char message[128];
    char *out = append(message, "INIT_CASE ");
    out = append(out, name);
    out = append(out, " ");
    append(out, verdict);
    test->platform->report(message, 0);
}

static bool matches(uint32_t offset, uint32_t mask, uint32_t expected)
{
    return (REG_RD(offset) & mask) == (expected & mask);
}

/* Các thanh ghi điều khiển cố định; không đọc cổng stats/LUT có giao thức riêng. */
static const uint32_t snapshot_offsets[] = {
    R_COMMON_CTRL, R_COMMON_FRAME_WIDTH, R_COMMON_FRAME_HEIGHT,
    R_COMMON_BAYER, R_COMMON_INPUT_FORMAT, R_COMMON_RECOVERY_SEQUENCE,
    R_BLC_CTRL, R_BLC_GAIN_SEL, R_LSC_CTRL, R_LSC_MESH_NODES,
    R_LSC_STRENGTH, R_LSC_PROFILE_SEL, R_LSC_PROFILE_STATUS,
    R_BPC_CTRL, R_BPC_THRESH, R_WB_CTRL, R_WB_GAIN_R, R_WB_GAIN_G,
    R_WB_GAIN_B, R_DG_CTRL, R_DG_GAIN, FX1_ISP_CCM_CTRL_OFFSET,
    FX1_ISP_GAMMA_CTRL_OFFSET, FX1_ISP_CSC_CTRL_OFFSET,
    FX1_ISP_GTM_CTRL_OFFSET, FX1_ISP_NR_2D_CTRL_OFFSET,
    FX1_ISP_EE_CTRL_OFFSET, FX1_ISP_CNF_CTRL_OFFSET,
    R_AEC_CTRL, R_AEC_CONTEXT_ID, R_AWB_CTRL, R_AWB_CONTEXT_ID,
    FX1_ISP_AF_CTRL_OFFSET, FX1_ISP_AF_CONTEXT_ID_OFFSET,
    FX1_ISP_OUTPUT_ENABLE_OFFSET, FX1_ISP_RESIZER_CTRL_OFFSET,
    FX1_ISP_CH1_RESIZER_CTRL_OFFSET, FX1_ISP_CH2_RESIZER_CTRL_OFFSET,
    R_DMA_CTRL, FX1_ISP_IDMA_STRIDE_OFFSET,
    FX1_ISP_ODMA_Y_STRIDE_OFFSET, FX1_ISP_CH1_ODMA_Y_STRIDE_OFFSET,
    FX1_ISP_CH2_ODMA_Y_STRIDE_OFFSET
};
#define SNAPSHOT_COUNT (sizeof(snapshot_offsets) / sizeof(snapshot_offsets[0]))

static bool unarmed_and_no_frames(full_test_t *test)
{
    bool good = true;
    good &= check(test, "ISP and DMA remain stopped",
        !(REG_RD(R_COMMON_CTRL) & COMMON_CTRL_ISP_EN) &&
        !(REG_RD(R_DMA_CTRL) & 3u) && !isp_not_idle());
    good &= check(test, "No input or output buffer ownership granted",
        !(REG_RD(R_IDMA_BUF_VALID) & IDMA_BUF_VALID_MASK) &&
        !(REG_RD(FX1_ISP_ODMA_BUF_FREE_OFFSET) & 15u) &&
        !(REG_RD(FX1_ISP_CH1_ODMA_BUF_FREE_OFFSET) & 15u) &&
        !(REG_RD(FX1_ISP_CH2_ODMA_BUF_FREE_OFFSET) & 15u));
    good &= check(test, "No IDMA or ODMA frame completed",
        !REG_RD(FX1_ISP_IDMA_FRAME_COUNT_OFFSET) &&
        !REG_RD(FX1_ISP_ODMA_FRAME_COUNT_OFFSET) &&
        !REG_RD(FX1_ISP_CH1_ODMA_FRAME_COUNT_OFFSET) &&
        !REG_RD(FX1_ISP_CH2_ODMA_FRAME_COUNT_OFFSET));
    return good;
}

static bool verify_configuration(full_test_t *test, const isp_config_t *cfg,
                                 const isp_init_result_t *result)
{
    bool good = true;
    const uint32_t all_stages = (1u << (ISP_INIT_READY + 1u)) - 1u;
    good &= check(test, "All 25 init stages completed through READY",
        result->status == ISP_OK && result->stage == ISP_INIT_READY &&
        result->completed == all_stages);
    for (unsigned stage = 0; stage <= ISP_INIT_READY; stage++)
        test->platform->report(isp_init_stage_name((isp_init_stage_t)stage),
                              !!(result->completed & (1u << stage)));

    uint16_t width = 0, height = 0;
    good &= check(test, "IFMT fields and active dimensions",
        ifmt_active_size(&cfg->ifmt, &width, &height) == 0 &&
        result->active_width == width && result->active_height == height &&
        matches(R_COMMON_FRAME_WIDTH, 0xffffu, cfg->ifmt.width) &&
        matches(R_COMMON_FRAME_HEIGHT, 0xffffu, cfg->ifmt.height) &&
        matches(R_COMMON_BAYER, 3u, cfg->ifmt.bayer) &&
        matches(R_COMMON_INPUT_FORMAT, 3u, cfg->ifmt.depth));

    bool blc = matches(R_BLC_CTRL, 1u, cfg->blc.enable) &&
        matches(R_BLC_GAIN_SEL, 0x103u, cfg->blc.gain_level |
                (cfg->blc.range_scale_en ? 0x100u : 0u));
    static const uint32_t blc_base[4] = {
        R_BLC_OFS_G0_DFT, R_BLC_OFS_G1_DFT, R_BLC_OFS_G2_DFT, R_BLC_OFS_G3_DFT};
    static const uint32_t blc_scale[4] = {
        R_BLC_SCALE_G0, R_BLC_SCALE_G1, R_BLC_SCALE_G2, R_BLC_SCALE_G3};
    for (unsigned profile = 0; profile < 4; profile++) {
        const blc_profile_config_t *p = &cfg->blc.profile[profile];
        uint32_t scale = p->scale ? p->scale : blc_calc_scale(p->dft);
        if (scale > BLC_SCALE_MAX) scale = BLC_SCALE_MAX;
        blc &= matches(blc_base[profile], 0xfffu, p->dft) &&
               matches(blc_base[profile]+4u, 0xfffu, p->r) &&
               matches(blc_base[profile]+8u, 0xfffu, p->gr) &&
               matches(blc_base[profile]+12u, 0xfffu, p->gb) &&
               matches(blc_base[profile]+16u, 0xfffu, p->b) &&
               matches(blc_scale[profile], BLC_SCALE_MAX, scale);
    }
    good &= check(test, "BLC four resident sets and enable", blc);

    unsigned valid_profiles = 0;
    for (unsigned profile = 0; profile < LSC_NUM_PROFILES; profile++)
        if (cfg->lsc.coefficients[profile]) valid_profiles |= 1u << profile;
    good &= check(test, "LSC requested residents valid and selector configured",
        matches(R_LSC_PROFILE_STATUS, 7u, valid_profiles) &&
        matches(R_LSC_PROFILE_SEL, 3u, cfg->lsc.profile) &&
        matches(R_LSC_CTRL, 1u, cfg->lsc.enable) &&
        matches(R_LSC_MESH_NODES, 0xffffu,
                cfg->lsc.mesh.nx | ((uint32_t)cfg->lsc.mesh.ny << 8)) &&
        matches(R_LSC_STRENGTH, LSC_STRENGTH_MASK, cfg->lsc.mesh.strength) &&
        !(REG_RD(R_LSC_LOAD_STATUS) & LSC_LOAD_STATUS_BUSY) && !REG_RD(R_LSC_ERROR));
    good &= check(test, "BPC threshold and enable",
        matches(R_BPC_CTRL, 1u, cfg->bpc.enable) &&
        matches(R_BPC_THRESH, 0xffffffffu,
                BPC_THRESH_K(cfg->bpc.thresh_k) | BPC_THRESH_FLOOR(cfg->bpc.thresh_floor)));
    good &= check(test, "WB three gains and enable",
        matches(R_WB_CTRL, 1u, cfg->wb.enable) &&
        matches(R_WB_GAIN_R, 0xfffu, cfg->wb.gain_r) &&
        matches(R_WB_GAIN_G, 0xfffu, cfg->wb.gain_g) &&
        matches(R_WB_GAIN_B, 0xfffu, cfg->wb.gain_b));
    good &= check(test, "Digital gain and enable",
        matches(R_DG_CTRL, 1u, cfg->dg.enable) && matches(R_DG_GAIN, 0xfffu, cfg->dg.gain));
    good &= check(test, "Demosaic fixed legacy margin",
        !REG_RD(FX1_ISP_TUNE_DEMOSAIC_MARGIN_OFFSET));

    bool ccm = matches(FX1_ISP_CCM_CTRL_OFFSET, 1u, cfg->ccm.enable);
    for (unsigned i = 0; i < 9; i++)
        ccm &= matches(FX1_ISP_CCM_CRR_OFFSET+4u*i, 0xfffu, (uint32_t)cfg->ccm.matrix[i]);
    for (unsigned i = 0; i < 3; i++)
        ccm &= matches(FX1_ISP_CCM_OFS_R_OFFSET+4u*i, 0xfffu, (uint32_t)cfg->ccm.offset[i]);
    good &= check(test, "CCM matrix offsets and enable", ccm);
    good &= check(test, "Gamma LUT loaded and enable",
        matches(FX1_ISP_GAMMA_STATUS_OFFSET, 3u, 2u) &&
        matches(FX1_ISP_GAMMA_CTRL_OFFSET, 1u, cfg->gamma.enable));
    good &= check(test, "CSC standard",
        matches(FX1_ISP_CSC_CTRL_OFFSET, 1u, cfg->csc.standard));
    good &= check(test, "GTM scalars mode and enable",
        matches(FX1_ISP_GTM_KEY_OFFSET, 0xffu, cfg->gtm.key) &&
        matches(FX1_ISP_GTM_LWHITE_OFFSET, 0xffffu, cfg->gtm.white) &&
        matches(FX1_ISP_GTM_ROI_LOG2_OFFSET, 0x1fu, cfg->gtm.roi_log2) &&
        matches(FX1_ISP_GTM_CTRL_OFFSET, 3u,
                cfg->gtm.enable | ((uint32_t)cfg->gtm.manual << 1)));
    good &= check(test, "2DNR enable",
        matches(FX1_ISP_NR_2D_CTRL_OFFSET, 1u, cfg->nr.enable));
    good &= check(test, "EE all resident LUTs valid and selected profile",
        matches(FX1_ISP_EE_STATUS_OFFSET, 0x73u, 0x70u) &&
        matches(FX1_ISP_EE_CTRL_OFFSET, 0x3fu,
                ((uint32_t)cfg->ee.profile << 4) | ((uint32_t)cfg->ee.profile << 2) |
                (cfg->ee.enable ? 1u : 0u)));
    good &= check(test, "CNF thresholds and enable",
        matches(FX1_ISP_CNF_CTRL_OFFSET, 1u, cfg->cnf.enable) &&
        matches(FX1_ISP_CNF_CHROMA_TH_OFFSET, 0x1ffu, cfg->cnf.chroma_threshold) &&
        matches(FX1_ISP_CNF_LUMA_TH_OFFSET, 0xffu, cfg->cnf.luma_threshold));
    good &= check(test, "AEC config and measurement source",
        matches(R_AEC_CTRL, 1u, cfg->aec.enable) &&
        matches(R_AEC_CONTEXT_ID, 0xffffffffu, cfg->aec.context_id) &&
        matches(R_AEC_ZONE_CFG, 0x1f3fu,
            cfg->aec.zones.num_zone_x | ((uint32_t)cfg->aec.zones.num_zone_y << 8)) &&
        matches(R_AEC_ZONE_SIZE, 0x0fff0fffu,
            cfg->aec.zones.zone_width | ((uint32_t)cfg->aec.zones.zone_height << 16)) &&
        matches(R_AEC_SAMPLE_CLIP, 0x0fff0fffu,
            cfg->aec.sample_min_clip | ((uint32_t)cfg->aec.sample_max_clip << 16)) &&
        matches(R_AEC_THRESH, 0x0fff0fffu,
            cfg->aec.thresh_ue | ((uint32_t)cfg->aec.thresh_oe << 16)) &&
        matches(R_TUNE_AEC_SOURCE, 1u, cfg->aec.source));
    good &= check(test, "AWB config and thresholds",
        matches(R_AWB_CTRL, 0x3fffu, AWB_CTRL_NX(cfg->awb.num_zone_x) |
            AWB_CTRL_NY(cfg->awb.num_zone_y) | (cfg->awb.enable ? AWB_CTRL_EN : 0u)) &&
        matches(R_AWB_CONTEXT_ID, 0xffffffffu, cfg->awb.context_id) &&
        matches(R_AWB_UNDEREXPOSED_LIMIT, 0xfffu, cfg->awb.underexposed_limit) &&
        matches(R_AWB_SATURATION_LIMIT, 0xfffu, cfg->awb.saturation_limit));
    good &= check(test, "AF config without lens movement commands",
        matches(FX1_ISP_AF_CTRL_OFFSET, 0x19u, cfg->af.enable) &&
        matches(FX1_ISP_AF_CONTEXT_ID_OFFSET, 0xffffffffu, cfg->af.context_id));

    static const uint32_t resizer[3] = {
        FX1_ISP_RESIZER_CTRL_OFFSET, FX1_ISP_CH1_RESIZER_CTRL_OFFSET,
        FX1_ISP_CH2_RESIZER_CTRL_OFFSET};
    static const uint32_t crop[3] = {
        FX1_ISP_CH0_CROP_CTRL_OFFSET, FX1_ISP_CH1_CROP_CTRL_OFFSET,
        FX1_ISP_CH2_CROP_CTRL_OFFSET};
    static const uint32_t y_stride[3] = {
        FX1_ISP_ODMA_Y_STRIDE_OFFSET, FX1_ISP_CH1_ODMA_Y_STRIDE_OFFSET,
        FX1_ISP_CH2_ODMA_Y_STRIDE_OFFSET};
    static const char *const width_label[3] = {"CH0_OUT_W", "CH1_OUT_W", "CH2_OUT_W"};
    static const char *const height_label[3] = {"CH0_OUT_H", "CH1_OUT_H", "CH2_OUT_H"};
    bool output = true, dma = true;
    unsigned enabled_outputs = 0;
    for (unsigned channel = 0; channel < 3; channel++) {
        const output_channel_config_t *c = &cfg->output.channel[channel];
        output_geometry_t g;
        output &= output_compute_geometry(c, channel, width, height, &g) == ISP_OK &&
            matches(crop[channel], 1u, c->crop.enable) &&
            matches(crop[channel]+4u, 0x1fff1fffu,
                c->crop.x | ((uint32_t)c->crop.y << 16)) &&
            matches(crop[channel]+8u, 0x1fff1fffu,
                c->crop.width | ((uint32_t)c->crop.height << 16)) &&
            matches(resizer[channel], 0xf9u,
                ((uint32_t)c->resizer.preset << 3) | (c->resizer.enable ? 1u : 0u));
        if (c->enable) {
            enabled_outputs |= 1u << channel;
            test->platform->report(width_label[channel], REG_RD(resizer[channel]+0x14u) & 0x1fffu);
            test->platform->report(height_label[channel], REG_RD(resizer[channel]+0x18u) & 0x1fffu);
            /* OUT_W/H được kiểm tra khi GEOMETRY chạy; không đòi VALID tồn tại sau tắt ISP. */
            output &= matches(resizer[channel]+0x14u, 0x1fffu, g.width) &&
                      matches(resizer[channel]+0x18u, 0x1fffu, g.height);
        }
        dma &= matches(y_stride[channel], 0xffffffffu, cfg->dma.output[channel].y_stride);
        if (channel < 2)
            dma &= matches(y_stride[channel]+4u, 0xffffffffu, cfg->dma.output[channel].uv_stride);
    }
    good &= check(test, "Output crop resizers dimensions and enabled channels",
        output && matches(FX1_ISP_OUTPUT_ENABLE_OFFSET, 7u, enabled_outputs));
    good &= check(test, "DMA burst and strides for all channels",
        dma && matches(R_DMA_CTRL, 0xff03u,
                       ((uint32_t)cfg->dma.max_burst_beats-1u) << 8) &&
        matches(FX1_ISP_IDMA_STRIDE_OFFSET, 0xffffffffu, cfg->dma.input_stride));
    good &= check(test, "No configuration or DMA errors remain",
        !REG_RD(R_TUNE_ERROR) && !REG_RD(R_LSC_ERROR) && !REG_RD(R_DMA_ERR) &&
        !(REG_RD(FX1_ISP_EE_STATUS_OFFSET) & 2u) &&
        !(REG_RD(R_COMMON_STATUS) & COMMON_STATUS_ERROR));
    good &= unarmed_and_no_frames(test);
    return good;
}

static void compatible_defaults(isp_config_t *cfg)
{
    isp_get_default_config(cfg);
    cfg->lsc.coefficients[0] = NULL;
    cfg->lsc.coefficient_count[0] = 0;
    cfg->lsc.profile = 1;
}

static bool run_success(full_test_t *test, const char *name, const isp_config_t *cfg,
                        const isp_wait_t *wait)
{
    isp_init_result_t result;
    uint32_t sequence = REG_RD(R_COMMON_RECOVERY_SEQUENCE);
    isp_status_t status = isp_init(cfg, wait, &result);
    test->platform->report("INIT_STATUS", (uint32_t)status);
    test->platform->report("INIT_STAGE", result.stage);
    test->platform->report("INIT_COMPLETED", result.completed);
    bool good = check(test, "isp_init returns ISP_OK", status == ISP_OK);
    if (status != ISP_OK) {
        test->platform->report("LSC_STEP", result.lsc.stage);
        test->platform->report("LSC_PROFILE", result.lsc.profile);
        test->platform->report("LSC_ERROR", result.lsc.errors);
    } else {
        good &= check(test, "One new recovery completed for this init",
            (uint32_t)(REG_RD(R_COMMON_RECOVERY_SEQUENCE)-sequence) == 1u);
        good &= verify_configuration(test, cfg, &result);
    }
    case_result(test, name, good ? "PASS" : "FAIL");
    return good;
}

int isp_full_init_check(const init_test_platform_t *platform, uint32_t timeout_us)
{
    if (!platform || !platform->time_us || !platform->report || !timeout_us)
        return ISP_E_PARAM;
    full_test_t test = {platform, 0, 0, 0};
    isp_wait_t wait = {platform_time, (void *)platform, timeout_us, 100000};
    isp_config_t cfg;
    isp_init_result_t result;
    isp_get_default_config(&cfg);
    platform->report("COMMON_VER_ID", REG_RD(R_COMMON_VER_ID));
    platform->report("TUNE_REV", REG_RD(R_TUNE_REV));

    isp_status_t status = isp_init(&cfg, &wait, &result);
    platform->report("DEFAULT_INIT_STATUS", (uint32_t)status);
    platform->report("DEFAULT_INIT_STAGE", result.stage);
    platform->report("DEFAULT_LSC_PROFILE", result.lsc.profile);
    platform->report("DEFAULT_LSC_ERROR", result.lsc.errors);
    if (status == ISP_OK) {
        bool good = verify_configuration(&test, &cfg, &result);
        case_result(&test, "DEFAULT_THREE_PROFILES", good ? "PASS" : "FAIL");
        if (!good) goto finished;
    } else {
        bool known = status == ISP_E_IO && result.stage == ISP_INIT_LSC &&
            result.lsc.stage == LSC_STAGE_BEGIN && result.lsc.profile == 0 &&
            result.lsc.errors == LSC_ERROR_ACTIVE_LOAD_REJECT &&
            !(result.completed & (1u << ISP_INIT_READY));
        if (!known || !unarmed_and_no_frames(&test)) {
            check(&test, "Unexpected strict default init failure", false);
            case_result(&test, "DEFAULT_THREE_PROFILES", "FAIL");
            goto finished;
        }
        test.known_limits++;
        platform->report("[KNOWN_LIMIT] DEFAULT_LSC_PROFILE0", result.lsc.errors);
        case_result(&test, "DEFAULT_THREE_PROFILES", "KNOWN_LIMIT");
    }

    compatible_defaults(&cfg);
    if (!run_success(&test, "COMPAT_DEFAULT", &cfg, &wait)) goto finished;
    if (!run_success(&test, "REPEAT_IDLE", &cfg, &wait)) goto finished;

    cfg.blc.enable = cfg.lsc.enable = cfg.bpc.enable = cfg.wb.enable = cfg.dg.enable = true;
    cfg.ccm.enable = cfg.gamma.enable = cfg.gtm.enable = cfg.nr.enable = true;
    cfg.ee.enable = cfg.cnf.enable = true;
    cfg.aec.enable = cfg.awb.enable = cfg.af.enable = true;
    cfg.lsc.profile = cfg.ee.profile = 2;
    cfg.wb.gain_r = 0x180u;
    cfg.wb.gain_g = 0x140u;
    cfg.wb.gain_b = 0x120u;
    cfg.dg.gain = 0x180u;
    cfg.aec.context_id = 0xaec1u;
    cfg.awb.context_id = 0xab01u;
    cfg.af.context_id = 0xafc1u;
    cfg.aec.source = AEC_SOURCE_DG;
    cfg.output.channel[1].enable = cfg.output.channel[2].enable = true;
    cfg.output.channel[1].resizer = (resizer_config_t){true, 8};
    cfg.output.channel[2].resizer = (resizer_config_t){true, 21};
    output_geometry_t geometry[3];
    for (unsigned channel = 0; channel < 3; channel++) {
        if (!check(&test, "Compute enabled output geometry",
            output_compute_geometry(&cfg.output.channel[channel], channel, 1920, 1080,
                                    &geometry[channel]) == ISP_OK)) {
            case_result(&test, "ALL_ENABLED_THREE_OUTPUTS", "FAIL");
            goto finished;
        }
    }
    dma_get_default_config(&cfg.dma, cfg.ifmt.width, geometry);
    if (!run_success(&test, "ALL_ENABLED_THREE_OUTPUTS", &cfg, &wait)) goto finished;

    uint32_t snapshot[SNAPSHOT_COUNT];
    for (unsigned i = 0; i < SNAPSHOT_COUNT; i++) snapshot[i] = REG_RD(snapshot_offsets[i]);
    cfg.ee.resident[2].luma[63] = 0x8001u;
    status = isp_init(&cfg, &wait, &result);
    bool unchanged = true;
    for (unsigned i = 0; i < SNAPSHOT_COUNT; i++)
        unchanged &= snapshot[i] == REG_RD(snapshot_offsets[i]);
    bool rejected = check(&test, "Invalid final EE table entry rejected before reset",
        status == ISP_E_PARAM && result.stage == ISP_INIT_EE && !result.completed);
    rejected &= check(&test, "Recovery sequence and sampled configuration unchanged", unchanged);
    rejected &= unarmed_and_no_frames(&test);
    case_result(&test, "INVALID_CONFIG_REJECT", rejected ? "PASS" : "FAIL");
    if (!rejected) goto finished;

    compatible_defaults(&cfg);
    run_success(&test, "RECOVER_DEFAULT", &cfg, &wait);

finished:
    ;
    char summary[100];
    char *out = append(summary, "FULL_CHECKS passed=");
    out = append_unsigned(out, test.passed);
    out = append(out, " failed=");
    out = append_unsigned(out, test.failed);
    out = append(out, " known_limits=");
    append_unsigned(out, test.known_limits);
    platform->report(summary, 0);
    return test.failed ? INIT_TEST_E_READBACK : ISP_OK;
}
