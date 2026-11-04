/*
 * Cấu hình gain số toàn ảnh trong miền Bayer; gain Q8 dùng 256 cho hệ số 1x.
 * Hiện thực và luồng xử lý.
 */
#include "bayer domain/dg/ftel_isp_dg.h"

void dg_get_default_config(dg_config_t *cfg)
{
    if (!cfg) return;
    cfg->enable = false;        /* DG_CTRL có giá trị sau reset 0 */
    cfg->gain   = GAIN_UNITY;   /* DG_GAIN có giá trị sau reset 0x100 */
}

isp_status_t dg_validate_config(const dg_config_t *cfg)
{
    return cfg && cfg->gain <= GAIN_MAX ? ISP_OK : ISP_E_PARAM;
}

isp_status_t dg_configure(const dg_config_t *cfg)
{
    if (dg_validate_config(cfg) != ISP_OK) return ISP_E_PARAM;

    isp_status_t st = isp_prepare_config();
    if (st != ISP_OK) return st;

    REG_UPD(R_DG_GAIN, GAIN_MASK, cfg->gain);

    isp_barrier();
    if (cfg->enable) REG_SET(R_DG_CTRL, DG_CTRL_EN);
    else             REG_CLR(R_DG_CTRL, DG_CTRL_EN);

    isp_barrier();
    st = isp_config_status();
    if (st != ISP_OK) return st;
    if (!isp_reg_matches(R_DG_GAIN, GAIN_MASK, cfg->gain) || !isp_reg_matches(R_DG_CTRL, 1u, cfg->enable ? 1u : 0u)) return ISP_E_IO;
    return ISP_OK;
}
