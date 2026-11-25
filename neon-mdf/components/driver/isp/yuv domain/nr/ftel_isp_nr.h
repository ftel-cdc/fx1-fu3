/*
 * Cấu hình bật khử nhiễu không gian 2D; bộ ước lượng nhiễu legacy là chỉ đọc.
 * Khai báo API và cấu trúc dữ liệu.
 */
#ifndef FTEL_ISP_NR_H
#define FTEL_ISP_NR_H
#include "common/ftel_isp_common.h"

/* 2DNR legacy tự ước lượng nhiễu. EST_VAR chỉ đọc; không ghi giá trị khởi tạo phần mềm vào đó. */
typedef struct {
    bool enable;
} nr_config_t;

void nr_get_default_config(nr_config_t *cfg);
isp_status_t nr_validate_config(const nr_config_t *cfg);
isp_status_t nr_configure(const nr_config_t *cfg);

#endif
