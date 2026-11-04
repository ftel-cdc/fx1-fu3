/*
 * Cấu hình ánh xạ tông toàn cục và nạp/đọc lại bảng 65 tỷ số.
 * Khai báo API và cấu trúc dữ liệu.
 */
#ifndef FTEL_ISP_GTM_H
#define FTEL_ISP_GTM_H
#include "common/ftel_isp_common.h"
#define GTM_LUT_SIZE 65u
typedef struct {
    bool enable, manual;
    uint16_t key, white; /* Hệ số key: UQ0.8, 0..255; mức trắng white: UQ8.8, 1..65535. */
    uint8_t roi_log2;    /* 0..31 */
    uint16_t lut[GTM_LUT_SIZE]; /* Tỷ số UQ8.8; 256 biểu diễn hệ số 1x. */
} gtm_config_t;

void gtm_get_default_config(gtm_config_t *cfg);
isp_status_t gtm_validate_config(const gtm_config_t *cfg);
/* Luôn nạp và đọc kiểm tra bank thủ công 0, sau đó chọn chế độ được yêu cầu. */
isp_status_t gtm_configure(const gtm_config_t *cfg);
#endif
