/*
 * Cấu hình gain số toàn ảnh trong miền Bayer; gain Q8 dùng 256 cho hệ số 1x.
 * Khai báo API và cấu trúc dữ liệu.
 */
#ifndef FTEL_ISP_DG_H
#define FTEL_ISP_DG_H

#include "common/ftel_isp_common.h"

/*
 * DG: gain số chung, nhân Bayer sau WB.
 */
typedef struct {
    bool     enable;    /* DG_CTRL.en                    */
    uint16_t gain;      /* UQ4.8; 0x100 = 1x, cực đại 0xFFF. */
} dg_config_t;

/* Điền mặc định theo đặc tả: tắt, gain = 1x. */
void          dg_get_default_config(dg_config_t *cfg);

/* Kiểm tra, ghi gain rồi bật sau cùng. */
isp_status_t  dg_validate_config(const dg_config_t *cfg); /* Không truy cập MMIO. */
isp_status_t  dg_configure(const dg_config_t *cfg);

#endif /* FTEL_ISP_DG_H */
