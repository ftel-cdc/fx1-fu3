/* Init tổng với lựa chọn tường minh của ứng dụng: chỉ nạp LSC 1/2, không yêu cầu bảng 0. */
#include "fx1_fw.h"
#include "pipeline/ftel_isp_init.h"

static uint64_t time_us(void *user)
{
    (void)user;
    return fx1_mtime() / 20u;
}

static void show(const char *name, uint32_t value)
{
    fx1_puts(name); fx1_puts(" = "); fx1_puthex(value); fx1_puts("\n");
}

int main(void)
{
    fx1_puts("[PASS] FW_BOOT: ISP init with explicit LSC tables 1/2\n");
    fx1_puts("[INFO] This configuration does not request LSC table 0. No RAW/DMA frame.\n");
    isp_config_t cfg;
    isp_get_default_config(&cfg);
    cfg.lsc.coefficients[0] = NULL;
    cfg.lsc.coefficient_count[0] = 0;
    cfg.lsc.profile = 1;
    cfg.lsc.enable = false;

    isp_wait_t wait = {time_us, NULL, 1000, 100000};
    isp_init_result_t result;
    isp_status_t status = isp_init(&cfg, &wait, &result);
    for (unsigned stage = 0; stage <= ISP_INIT_READY; stage++) {
        if (result.completed & (1u << stage))
            show(isp_init_stage_name((isp_init_stage_t)stage), 0);
    }
    show("INIT_STATUS", (uint32_t)status);
    show("INIT_STAGE", result.stage);
    if (status != ISP_OK) {
        show("LSC_STEP", result.lsc.stage);
        show("LSC_ERROR_BEFORE_ABORT", result.lsc.errors);
    }
    uint32_t valid = REG_RD(R_LSC_PROFILE_STATUS) & LSC_PROFILE_STATUS_VALID_MASK;
    show("LSC_LOADED_VALID_MASK", valid);
    bool ready = status == ISP_OK && valid == 6u &&
        (REG_RD(R_LSC_PROFILE_SEL) & 3u) == 1u && !(REG_RD(R_LSC_CTRL) & 1u) &&
        !(REG_RD(R_COMMON_CTRL) & COMMON_CTRL_ISP_EN) &&
        !(REG_RD(R_DMA_CTRL) & 3u) && !(REG_RD(R_IDMA_BUF_VALID) & IDMA_BUF_VALID_MASK);
    fx1_puts(ready ? "INIT_LSC12_RESULT=PASS\n" : "INIT_LSC12_RESULT=FAIL\n");
    return ready ? 0 : 0x32;
}
