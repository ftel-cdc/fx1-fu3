/*
 * Cấu hình thống kê lấy nét legacy; chưa triển khai thuật toán AF hay lệnh chạy thấu kính.
 * Khai báo API và cấu trúc dữ liệu.
 */
#ifndef FTEL_ISP_AF_H
#define FTEL_ISP_AF_H
#include "common/ftel_isp_common.h"

/* Chỉ cấu hình thống kê AF: legacy dùng Sobel cố định, lưới đều 4x4. Các trường tương thích lens/search/metric/zone mask không tác động xử lý. */
typedef struct {
    bool enable;
    uint32_t context_id;
} af_config_t;

void af_get_default_config(af_config_t *cfg);
isp_status_t af_validate_config(const af_config_t *cfg);
isp_status_t af_configure(const af_config_t *cfg);

#endif
