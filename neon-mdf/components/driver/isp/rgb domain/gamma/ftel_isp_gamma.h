/*
 * Nạp, đọc lại và xác nhận đủ 256 điểm bảng gamma trước khi bật khối.
 * Khai báo API và cấu trúc dữ liệu.
 */
#ifndef FTEL_ISP_GAMMA_H
#define FTEL_ISP_GAMMA_H
#include "common/ftel_isp_common.h"
#define GAMMA_LUT_SIZE 256u
typedef struct {
    bool enable;
    uint16_t lut[GAMMA_LUT_SIZE];
} gamma_config_t;

/* Mặc định tắt, LUT tăng tuyến tính; vẫn phải nạp RAM, đọc kiểm tra và xác nhận. */
void gamma_get_default_config(gamma_config_t *cfg);
isp_status_t gamma_validate_config(const gamma_config_t *cfg);
isp_status_t gamma_configure(const gamma_config_t *cfg);
#endif
