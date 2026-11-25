/*
 * Cấu hình ngưỡng khử nhiễu sắc độ UV có hướng dẫn bởi độ sáng Y.
 * Khai báo API và cấu trúc dữ liệu.
 */
#ifndef FTEL_ISP_CNF_H
#define FTEL_ISP_CNF_H
#include "common/ftel_isp_common.h"

/* Ngưỡng CNF ở miền YUV8. Trường STRENGTH dự phòng không được xem là điều khiển có tác dụng. */
typedef struct {
    bool enable;
    uint16_t gf_eps; /* 8-bit luma squared; 0..65535, reset 200. */
} cnf_config_t;

void cnf_get_default_config(cnf_config_t *cfg);
isp_status_t cnf_validate_config(const cnf_config_t *cfg);
isp_status_t cnf_configure(const cnf_config_t *cfg);

#endif
