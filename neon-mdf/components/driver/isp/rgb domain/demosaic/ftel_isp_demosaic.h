/*
 * Kiểm tra đường nội suy Bayer sang RGB cố định ở legacy; không thêm bit bật giả.
 * Khai báo API và cấu trúc dữ liệu.
 */
#ifndef FTEL_ISP_DEMOSAIC_H
#define FTEL_ISP_DEMOSAIC_H
#include "common/ftel_isp_common.h"

/* Phép biến đổi bắt buộc, không có bit bật riêng. Legacy chỉ dùng margin = 0; margin khác 0 cần API chế độ profile. */
typedef struct {
    uint16_t green_margin, rb_margin;
} demosaic_config_t;

void demosaic_get_default_config(demosaic_config_t *cfg);
isp_status_t demosaic_validate_config(const demosaic_config_t *cfg);
isp_status_t demosaic_configure(const demosaic_config_t *cfg);

#endif
