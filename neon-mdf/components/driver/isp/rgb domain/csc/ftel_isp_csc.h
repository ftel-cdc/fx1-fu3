/*
 * Chọn chuẩn chuyển đổi RGB/YUV dùng chung cho CSC chính và CSC ngược tại CH2.
 * Khai báo API và cấu trúc dữ liệu.
 */
#ifndef FTEL_ISP_CSC_H
#define FTEL_ISP_CSC_H
#include "common/ftel_isp_common.h"

/* Chuyển RGB sang YUV bắt buộc; standard 0=BT.601, 1=BT.709, toàn dải trong legacy. CSC ngược của CH2 dùng chung lựa chọn này. */
typedef struct {
    uint8_t standard;
} csc_config_t;

void csc_get_default_config(csc_config_t *cfg);
isp_status_t csc_validate_config(const csc_config_t *cfg);
isp_status_t csc_configure(const csc_config_t *cfg);

#endif
