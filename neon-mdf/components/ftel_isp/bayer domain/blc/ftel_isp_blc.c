/*
 * Cấu hình bù mức đen: bốn bộ offset từng màu, hệ số giãn dải và bộ đang chọn.
 * Hiện thực và luồng xử lý.
 */
#include "bayer domain/blc/ftel_isp_blc.h"

/* Bảng địa chỉ thanh ghi theo mức gain_level. */
static const uint16_t k_ofs_dft[4] = {
    R_BLC_OFS_G0_DFT, R_BLC_OFS_G1_DFT, R_BLC_OFS_G2_DFT, R_BLC_OFS_G3_DFT };
static const uint16_t k_ofs_r[4]   = {
    R_BLC_OFS_G0_R,   R_BLC_OFS_G1_R,   R_BLC_OFS_G2_R,   R_BLC_OFS_G3_R   };
static const uint16_t k_ofs_gr[4]  = {
    R_BLC_OFS_G0_GR,  R_BLC_OFS_G1_GR,  R_BLC_OFS_G2_GR,  R_BLC_OFS_G3_GR  };
static const uint16_t k_ofs_gb[4]  = {
    R_BLC_OFS_G0_GB,  R_BLC_OFS_G1_GB,  R_BLC_OFS_G2_GB,  R_BLC_OFS_G3_GB  };
static const uint16_t k_ofs_b[4]   = {
    R_BLC_OFS_G0_B,   R_BLC_OFS_G1_B,   R_BLC_OFS_G2_B,   R_BLC_OFS_G3_B   };
static const uint16_t k_scale[4]   = {
    R_BLC_SCALE_G0,   R_BLC_SCALE_G1,   R_BLC_SCALE_G2,   R_BLC_SCALE_G3   };

uint32_t blc_calc_scale(uint16_t pedestal)
{
    if (pedestal == 0u || pedestal >= BLC_PEDESTAL_MAX)
        return BLC_SCALE_UNITY;

    uint64_t num = (uint64_t)BLC_PEDESTAL_MAX << 16;   /* 4095 * 65536 */
    uint64_t den = (uint64_t)(BLC_PEDESTAL_MAX - pedestal);
    uint64_t s   = num / den;

    if (s > BLC_SCALE_MAX) s = BLC_SCALE_MAX;
    return (uint32_t)s;
}

void blc_get_default_config(blc_config_t *cfg)
{
    if (!cfg) return;

    cfg->enable         = false;          /* BLC_CTRL có giá trị sau reset 0 */
    cfg->gain_level     = 0;              /* BLC_GAIN_SEL[1:0] có giá trị sau reset 0 */
    cfg->range_scale_en = true;           /* BLC_GAIN_SEL[8] có giá trị sau reset 1 */

    for (int i = 0; i < 4; i++) {
        cfg->profile[i].dft   = 200;              /* Giá trị sau reset 0xC8 */
        cfg->profile[i].r     = 0;
        cfg->profile[i].gr    = 0;
        cfg->profile[i].gb    = 0;
        cfg->profile[i].b     = 0;
        /*
         * 0 là tự tính: blc_configure() suy ra scale từ dft. Đặt sẵn 0x10000 ở đây sẽ cố định mọi profile
         * ở 1x dù range_scale_en bật.
         */
        cfg->profile[i].scale = BLC_SCALE_AUTO;
    }
}

isp_status_t blc_validate_config(const blc_config_t *cfg)
{
    if (!cfg || cfg->gain_level > 3u) return ISP_E_PARAM;
    for (unsigned i = 0; i < 4; i++) {
        const blc_profile_config_t *p = &cfg->profile[i];
        if (p->dft > 4095 || p->r > 4095 || p->gr > 4095 || p->gb > 4095 || p->b > 4095)
            return ISP_E_PARAM;
    }
    return ISP_OK;
}

isp_status_t blc_configure(const blc_config_t *cfg)
{
    if (blc_validate_config(cfg) != ISP_OK) return ISP_E_PARAM;
    isp_status_t st = isp_prepare_config();
    if (st != ISP_OK) return st;

    /* Bước 1: ghi đủ bốn bộ tham số trước khi chọn/bật. */
    for (int i = 0; i < 4; i++) {
        const blc_profile_config_t *p = &cfg->profile[i];

        REG_UPD(k_ofs_dft[i], 0xFFFu, p->dft);
        REG_UPD(k_ofs_r  [i], 0xFFFu, p->r);
        REG_UPD(k_ofs_gr [i], 0xFFFu, p->gr);
        REG_UPD(k_ofs_gb [i], 0xFFFu, p->gb);
        REG_UPD(k_ofs_b  [i], 0xFFFu, p->b);

        uint32_t scale = p->scale ? p->scale : blc_calc_scale(p->dft);
        if (scale > BLC_SCALE_MAX) scale = BLC_SCALE_MAX;
        REG_UPD(k_scale[i], BLC_SCALE_MAX, scale);
    }
    isp_barrier();      /* Các offset phải sẵn sàng trước khi chọn bộ tham số. */

    /* Bước 2: chọn bộ tham số và bit range_scale_en. */
    uint32_t sel = BLC_GAIN_SEL_GAIN_LVL(cfg->gain_level);
    if (cfg->range_scale_en) sel |= BLC_GAIN_SEL_RANGE_SCALE;
    REG_UPD(R_BLC_GAIN_SEL, 0x103u, sel);

    /* Bước 3: bật/tắt BLC sau cùng. */
    if (cfg->enable) REG_SET(R_BLC_CTRL, BLC_CTRL_EN);
    else             REG_CLR(R_BLC_CTRL, BLC_CTRL_EN);

    isp_barrier();
    st = isp_config_status();
    if (st != ISP_OK) return st;
    for (unsigned i = 0; i < 4; i++) {
        const blc_profile_config_t *p = &cfg->profile[i];
        uint32_t scale = p->scale ? p->scale : blc_calc_scale(p->dft);
        if (scale > BLC_SCALE_MAX) scale = BLC_SCALE_MAX;
        if (!isp_reg_matches(k_ofs_dft[i], 0xfffu, p->dft) ||
            !isp_reg_matches(k_ofs_r[i], 0xfffu, p->r) ||
            !isp_reg_matches(k_ofs_gr[i], 0xfffu, p->gr) ||
            !isp_reg_matches(k_ofs_gb[i], 0xfffu, p->gb) ||
            !isp_reg_matches(k_ofs_b[i], 0xfffu, p->b) ||
            !isp_reg_matches(k_scale[i], BLC_SCALE_MAX, scale)) return ISP_E_IO;
    }
    if (!isp_reg_matches(R_BLC_GAIN_SEL, 0x103u, sel) ||
        !isp_reg_matches(R_BLC_CTRL, 1u, cfg->enable ? 1u : 0u)) return ISP_E_IO;
    return ISP_OK;
}
