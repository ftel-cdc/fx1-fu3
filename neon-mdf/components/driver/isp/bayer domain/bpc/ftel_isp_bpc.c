/*
 * Cấu hình sửa pixel lỗi bằng ngưỡng không gian; không có lịch sử lỗi qua frame.
 * Hiện thực và luồng xử lý.
 */
#include "bayer domain/bpc/ftel_isp_bpc.h"

void bpc_get_default_config(bpc_config_t *cfg)
{
    if (!cfg) return;
    cfg->enable       = false;                       /* BPC_CTRL có giá trị sau reset 0 */
    cfg->thresh_k     = BPC_THRESH_K_DEFAULT;        /* 0x4D */
    cfg->thresh_floor = BPC_THRESH_FLOOR_DEFAULT;    /* 0x192 */
}

isp_status_t bpc_validate_config(const bpc_config_t *cfg)
{
    return cfg ? ISP_OK : ISP_E_PARAM;
}

isp_status_t bpc_configure(const bpc_config_t *cfg)
{
    if (bpc_validate_config(cfg) != ISP_OK) return ISP_E_PARAM;

    isp_status_t st = isp_prepare_config();
    if (st != ISP_OK) return st;

    REG_WR(R_BPC_THRESH,
           BPC_THRESH_K(cfg->thresh_k) | BPC_THRESH_FLOOR(cfg->thresh_floor));

    isp_barrier();
    if (cfg->enable) REG_SET(R_BPC_CTRL, BPC_CTRL_EN);
    else             REG_CLR(R_BPC_CTRL, BPC_CTRL_EN);

    isp_barrier();
    st = isp_config_status();
    if (st != ISP_OK) return st;
    if (REG_RD(R_BPC_THRESH) != (BPC_THRESH_K(cfg->thresh_k) | BPC_THRESH_FLOOR(cfg->thresh_floor)) || !isp_reg_matches(R_BPC_CTRL, 1u, cfg->enable ? 1u : 0u)) return ISP_E_IO;
    return ISP_OK;
}

void bpc_get_stats(bpc_stats_t *st)
{
    if (!st) return;
    st->candidates = (uint16_t)(REG_RD(R_BPC_NUM_CANDIDATES) & 0x3FFFu);
    st->defective  = (uint16_t)(REG_RD(R_BPC_NUM_DEFECTIVE)  & 0x3FFFu);
}
