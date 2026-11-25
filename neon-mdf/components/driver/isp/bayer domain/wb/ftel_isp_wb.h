/*
 * Cấu hình gain cân bằng trắng R/Gr/Gb/B; thuật toán tự chọn gain là phần riêng.
 * Khai báo API và cấu trúc dữ liệu.
 */
#ifndef FTEL_ISP_WB_H
#define FTEL_ISP_WB_H

#include "common/ftel_isp_common.h"

/*
 * WB: cân bằng trắng bằng gain từng màu.
 */
typedef struct {
    bool     enable;    /* WB_CTRL.en                       */
    uint16_t gain_r;    /* UQ4.8; 0x100 = 1x, cực đại 0xFFF. */
    uint16_t gain_g;    /* UQ4.8; legacy dùng chung gain G cho cả Gr và Gb. */
    uint16_t gain_b;    /* UQ4.8                            */
} wb_config_t;

/* Điền mặc định theo đặc tả: tắt, mọi gain = 1x. */
void          wb_get_default_config(wb_config_t *cfg);

/* Kiểm tra, ghi gain từng màu rồi bật sau cùng. */
isp_status_t  wb_validate_config(const wb_config_t *cfg); /* Không truy cập MMIO. */
isp_status_t  wb_configure(const wb_config_t *cfg);

#endif /* FTEL_ISP_WB_H */
