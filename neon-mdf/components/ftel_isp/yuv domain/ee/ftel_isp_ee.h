/*
 * Cấu hình tăng cường biên và nạp/xác nhận bảng của ba bộ tham số lưu trú.
 * Khai báo API và cấu trúc dữ liệu.
 */
#ifndef FTEL_ISP_EE_H
#define FTEL_ISP_EE_H
#include "common/ftel_isp_common.h"

typedef struct {
    uint16_t alpha, beta; /* UQ1.15, từ 0 đến 0x8000. */
    uint8_t clamp_positive, clamp_negative, features; /* Các bit chức năng features[2:0]. */
    uint16_t center_x2, center_y2;
    uint32_t radius2[3];
    uint16_t radial_gain[4]; /* UQ1.15, từ 0 đến 0xc000. */
    uint16_t luma[64], activity[64], contrast_positive[32], contrast_negative[32];
} ee_profile_config_t;

typedef struct {
    bool enable;
    uint8_t profile;
    ee_profile_config_t resident[3];
} ee_config_t;

void ee_get_default_config(ee_config_t *cfg);
isp_status_t ee_validate_config(const ee_config_t *cfg, uint16_t active_w, uint16_t active_h);
/* Ghi, đọc kiểm tra và xác nhận đủ ba bộ tham số lưu trú trước khi chọn/bật. */
isp_status_t ee_configure(const ee_config_t *cfg, uint16_t active_w, uint16_t active_h);
#endif
