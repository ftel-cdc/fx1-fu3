/* Contract tests for CSR/MAS 2026-11-24; MMIO storage is a host fixture, not a VP. */
#include "test_common.h"
#include "bayer domain/ifmt/ftel_isp_ifmt.h"
#include "bayer domain/lsc/ftel_isp_lsc.h"
#include "yuv domain/cnf/ftel_isp_cnf.h"
#include "rgb domain/ccm/ftel_isp_ccm.h"
#include "io/dma/ftel_isp_dma.h"
#include "stats/aec/ftel_isp_aec.h"
uint32_t ftel_isp_test_registers[0x10000 / 4];

static void test_input_geometry(void)
{
    ifmt_config_t c;
    ifmt_get_default_config(&c);
    c.width = 2688; c.height = 1520;
    for (unsigned phase = 0; phase < 4; ++phase) {
        c.bayer = (bayer_pattern_t)phase;
        for (unsigned source = 0; source < 2; ++source) {
            c.source = (ifmt_source_t)source;
            for (unsigned crop = 0; crop < 2; ++crop) {
                c.crop = crop != 0;
                uint16_t w, h;
                CHECK_EQ(ifmt_active_size(&c, &w, &h), 0);
                CHECK_EQ(w, crop ? 2688u - 2u * (phase & 1u) : 2688u);
                CHECK_EQ(h, (crop || source) ? 1520u - 2u * (phase >> 1) : 1520u);
            }
        }
    }
    c.source = IFMT_SOURCE_DDR; c.crop = false; c.bayer = BAYER_BGGR;
    c.width = 3842; c.height = 2162;
    uint16_t w, h;
    CHECK_EQ(ifmt_active_size(&c, &w, &h), 0);
    CHECK_EQ(w, 3840); CHECK_EQ(h, 2160);
    c.width = 32; c.height = 32;
    CHECK_EQ(ifmt_active_size(&c, &w, &h), 0);
    c.crop = true;
    CHECK_EQ(ifmt_active_size(&c, &w, &h), -1);
    c.source = (ifmt_source_t)2;
    CHECK_EQ(ifmt_validate_config(&c), ISP_E_PARAM);
}

static void test_coefficient_units(void)
{
    lsc_init_config_t c;
    lsc_get_default_init_config(&c);
    uint32_t mesh[c.coefficient_count[0]];
    for (unsigned i = 0; i < c.coefficient_count[0]; ++i) mesh[i] = LSC_COEF_UNITY;
    for (unsigned p = 0; p < 3; ++p) c.coefficients[p] = mesh;
    CHECK_EQ(lsc_validate_init_config(&c, 1920, 1080), ISP_OK);
    mesh[0] = 0;
    CHECK_EQ(lsc_validate_init_config(&c, 1920, 1080), ISP_E_PARAM);
    mesh[0] = 0x1fffff;
    CHECK_EQ(lsc_validate_init_config(&c, 1920, 1080), ISP_OK);
    mesh[0] = 0x200000;
    CHECK_EQ(lsc_validate_init_config(&c, 1920, 1080), ISP_E_PARAM);
    ccm_config_t ccm; ccm_get_default_config(&ccm);
    ccm.offset[0] = 512;
    CHECK_EQ(ccm_validate_config(&ccm), ISP_OK);
    ccm.offset[0] = 2048;
    CHECK_EQ(ccm_validate_config(&ccm), ISP_E_PARAM);
    cnf_config_t cnf; cnf_get_default_config(&cnf);
    CHECK_EQ(cnf.gf_eps, 200);
    cnf.gf_eps = 65535;
    CHECK_EQ(cnf_validate_config(&cnf), ISP_OK);
}

static void test_async_validation_and_caps(void)
{
    memset(ftel_isp_test_registers, 0, sizeof(ftel_isp_test_registers));
    REG_WR(R_COMMON_VER_ID, ISP_ABI_VERSION);
    REG_WR(R_TUNE_ID, TUNE_ID_MAGIC);
    REG_WR(R_TUNE_CAPS, 0x003fffff);
    CHECK_EQ(isp_check_abi(), ISP_E_BAD_ABI);
    REG_WR(R_TUNE_CAPS, 0x00ffffff);
    CHECK_EQ(isp_check_abi(), ISP_OK);
    isp_wait_t wait = {.poll_limit = 3};
    REG_WR(FX1_ISP_TUNE_STATUS_OFFSET, FX1_ISP_TUNE_STATUS_CHECK_BUSY_MASK);
    CHECK_EQ(isp_wait_checks(&wait), ISP_E_TIMEOUT);
    REG_WR(FX1_ISP_TUNE_STATUS_OFFSET, 0);
    CHECK_EQ(isp_wait_checks(&wait), ISP_OK);
    REG_WR(R_TUNE_ERROR, 1);
    CHECK_EQ(isp_wait_checks(&wait), ISP_E_IO);
    REG_WR(R_TUNE_ERROR, 0);
    REG_WR(R_LSC_PROFILE_STATUS, FX1_ISP_LSC_PROFILE_STATUS_PREP_BUSY_MASK);
    CHECK_EQ(isp_wait_register(R_LSC_PROFILE_STATUS, FX1_ISP_LSC_PROFILE_STATUS_PREP_BUSY_MASK, 0, &wait), ISP_E_TIMEOUT);
}

static void test_stream_and_aec_readout(void)
{
    ifmt_stream_status_t state;
    REG_WR(FX1_ISP_INPUT_STREAM_STATUS_OFFSET, 2);
    REG_WR(FX1_ISP_INPUT_STREAM_ERR_OFFSET, 0x24);
    REG_WR(FX1_ISP_INPUT_STREAM_FRAME_COUNT_OFFSET, 5);
    REG_WR(FX1_ISP_INPUT_STREAM_DROP_COUNT_OFFSET, 1);
    CHECK_EQ(ifmt_read_stream_status(&state), ISP_OK);
    CHECK_EQ(state.status, 2); CHECK_EQ(state.errors, 0x24);
    CHECK_EQ(state.frame_count, 5); CHECK_EQ(state.drop_count, 1);
    CHECK_EQ(ifmt_clear_stream_errors(0x40), ISP_E_PARAM);
    aec_global_stats_t stats;
    REG_WR(R_TUNE_AEC_STATUS, 0);
    CHECK_EQ(aec_read_global(&stats), ISP_E_NOT_IDLE);
    REG_WR(R_TUNE_AEC_STATUS, TUNE_AEC_STATUS_LEASE_VALID);
    REG_WR(FX1_ISP_AEC_GLOBAL_OE_UE_PERCENT_OFFSET, (0x6400u << 16) | 0x100u);
    CHECK_EQ(aec_read_global(&stats), ISP_OK);
    CHECK_EQ(stats.ue_percent_q8, 256); CHECK_EQ(stats.oe_percent_q8, 25600);
    output_geometry_t g[3] = {{.width=1920,.height=1080,.format=OUTPUT_NV12},
                             {.width=1920,.height=1080,.format=OUTPUT_NV12},
                             {.width=640,.height=480,.format=OUTPUT_RGB888}};
    dma_config_t dma; dma_get_default_config(&dma, 1920, g);
    dma.irq_enable = 0x80;
    CHECK_EQ(dma_validate_config(&dma, 1920, 1080, g), ISP_OK);
    dma.irq_enable = 0x100;
    CHECK_EQ(dma_validate_config(&dma, 1920, 1080, g), ISP_E_PARAM);
    dma.irq_enable = 0x80; dma.input_stride = 0;
    CHECK_EQ(dma_validate_config_for_source(&dma, 1920, 1080, g, true), ISP_OK);
    CHECK_EQ(dma_validate_config(&dma, 1920, 1080, g), ISP_E_PARAM);
}
int main(void)
{
    RUN(test_input_geometry); RUN(test_coefficient_units);
    RUN(test_async_validation_and_caps); RUN(test_stream_and_aec_readout);
    return TEST_REPORT();
}
