/*
 * Cấu hình bù mức đen: bốn bộ offset từng màu, hệ số giãn dải và bộ đang chọn.
 * Khai báo API và cấu trúc dữ liệu.
 */
#ifndef FTEL_ISP_BLC_H
#define FTEL_ISP_BLC_H

#include "common/ftel_isp_common.h"

/* ============================================================
 *  BLC
 * ============================================================ */
typedef struct {
    uint16_t dft;      /* Mức đen nền dùng chung, 0..4095. */
    uint16_t r;        /* Hiệu chỉnh mức đen R, 0..4095. */
    uint16_t gr;       /* Hiệu chỉnh mức đen Gr, 0..4095. */
    uint16_t gb;       /* Hiệu chỉnh mức đen Gb, 0..4095. */
    uint16_t b;        /* Hiệu chỉnh mức đen B, 0..4095. */
    uint32_t scale;    /* UQ2.16; BLC_SCALE_AUTO(0) yêu cầu tự tính từ dft. */
} blc_profile_config_t;

typedef struct {
    bool                 enable;          /* BLC_CTRL.en         */
    uint8_t              gain_level;      /* 0..3                */
    bool                 range_scale_en;  /* BLC_GAIN_SEL[8]     */
    blc_profile_config_t profile[4];      /* G0..G3              */
} blc_config_t;

/*
 * Điền mặc định cho bốn bộ tham số giống nhau; scale = 0 để blc_configure() tự suy ra từ dft.
 */
void          blc_get_default_config(blc_config_t *cfg);

/* Kiểm tra toàn bộ, ghi bốn bộ tham số, chọn rồi bật theo đúng thứ tự. */
isp_status_t  blc_validate_config(const blc_config_t *cfg); /* Không truy cập MMIO. */
isp_status_t  blc_configure(const blc_config_t *cfg);

/*
 * Các hàm hỗ trợ công khai.
 */
#define BLC_SCALE_AUTO  0u    /* profile.scale được tính từ dft. */

/* UQ2.16 = floor(4095*65536 / (4095 - pedestal)); kẹp tối đa 0x3FFFF. */
uint32_t      blc_calc_scale(uint16_t pedestal);

#endif /* FTEL_ISP_BLC_H */
