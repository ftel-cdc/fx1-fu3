/*
 * Cấu hình ngưỡng khử nhiễu sắc độ UV có hướng dẫn bởi độ sáng Y.
 * Hiện thực và luồng xử lý.
 */
#include "yuv domain/cnf/ftel_isp_cnf.h"
#include "registers/ftel_isp_regs.h"

void cnf_get_default_config(cnf_config_t *cfg)
{
    if (!cfg) return;
    cfg->enable = false;
    cfg->gf_eps = 200;
}

isp_status_t cnf_validate_config(const cnf_config_t *cfg)
{
    if (!cfg) return ISP_E_PARAM;
    return ISP_OK;
}

isp_status_t cnf_configure(const cnf_config_t *cfg)
{
    isp_status_t st = cnf_validate_config(cfg);
    if (st != ISP_OK) return st;
    const isp_reg_setting_t words[] = {
        {FX1_ISP_CNF_GF_EPS_OFFSET, 0xffffu, cfg->gf_eps},
        {FX1_ISP_CNF_CTRL_OFFSET, 1u, cfg->enable ? 1u : 0u}
    };
    return isp_configure_registers(words, sizeof(words) / sizeof(words[0]));
}
