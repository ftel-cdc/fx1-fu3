/*
 * Cấu hình gain cân bằng trắng R/Gr/Gb/B; thuật toán tự chọn gain là phần riêng.
 * Hiện thực và luồng xử lý.
 */
#include "bayer domain/wb/ftel_isp_wb.h"

void wb_get_default_config(wb_config_t *cfg)
{
    if (!cfg) return;
    cfg->enable = false;                 /* WB_CTRL có giá trị sau reset 0 */
    cfg->gain_r = GAIN_UNITY;            /* WB_GAIN_R có giá trị sau reset 0x100 */
    cfg->gain_g = GAIN_UNITY;            /* WB_GAIN_G có giá trị sau reset 0x100 */
    cfg->gain_b = GAIN_UNITY;            /* WB_GAIN_B có giá trị sau reset 0x100 */
}

isp_status_t wb_validate_config(const wb_config_t *cfg)
{
    return cfg && cfg->gain_r <= GAIN_MAX && cfg->gain_g <= GAIN_MAX && cfg->gain_b <= GAIN_MAX ? ISP_OK : ISP_E_PARAM;
}

isp_status_t wb_configure(const wb_config_t *cfg)
{
    if (wb_validate_config(cfg) != ISP_OK) return ISP_E_PARAM;

    isp_status_t st = isp_prepare_config();
    if (st != ISP_OK) return st;

    REG_UPD(R_WB_GAIN_R, GAIN_MASK, cfg->gain_r);
    REG_UPD(R_WB_GAIN_G, GAIN_MASK, cfg->gain_g);
    REG_UPD(R_WB_GAIN_B, GAIN_MASK, cfg->gain_b);

    isp_barrier();
    if (cfg->enable) REG_SET(R_WB_CTRL, WB_CTRL_EN);
    else             REG_CLR(R_WB_CTRL, WB_CTRL_EN);

    isp_barrier();
    st = isp_config_status();
    if (st != ISP_OK) return st;
    if (!isp_reg_matches(R_WB_GAIN_R, GAIN_MASK, cfg->gain_r) || !isp_reg_matches(R_WB_GAIN_G, GAIN_MASK, cfg->gain_g) || !isp_reg_matches(R_WB_GAIN_B, GAIN_MASK, cfg->gain_b) || !isp_reg_matches(R_WB_CTRL, 1u, cfg->enable ? 1u : 0u)) return ISP_E_IO;
    return ISP_OK;
}
