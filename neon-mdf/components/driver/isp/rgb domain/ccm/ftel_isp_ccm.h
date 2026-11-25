/*
 * Cấu hình ma trận hiệu chỉnh màu có dấu 3x3 và offset RGB.
 * Khai báo API và cấu trúc dữ liệu.
 */
#ifndef FTEL_ISP_CCM_H
#define FTEL_ISP_CCM_H
#include "common/ftel_isp_common.h"

/* Matrix and offsets are signed 12-bit Q3.9 per CSR/MAS 2026-11-24. */
typedef struct {
    bool enable;
    int16_t matrix[9];
    int16_t offset[3]; /* Signed Q3.9: code 512 adds one pixel level. */
} ccm_config_t;

void ccm_get_default_config(ccm_config_t *cfg);
isp_status_t ccm_validate_config(const ccm_config_t *cfg);
isp_status_t ccm_configure(const ccm_config_t *cfg);

#endif
