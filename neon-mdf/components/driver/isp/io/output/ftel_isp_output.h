/*
 * Cấu hình crop/thu nhỏ/định dạng ba kênh và kiểm tra hình học đầu ra đã hợp lệ.
 * Khai báo API và cấu trúc dữ liệu.
 */
#ifndef FTEL_ISP_OUTPUT_H
#define FTEL_ISP_OUTPUT_H
#include "common/ftel_isp_common.h"

typedef struct {
    bool enable;
    uint16_t x, y, width, height;
} crop_config_t;

typedef struct {
    bool enable;
    uint8_t preset;
} resizer_config_t;

typedef struct {
    bool enable;
    crop_config_t crop;
    resizer_config_t resizer;
} output_channel_config_t;

typedef struct {
    output_channel_config_t channel[3];
    uint8_t done_irq, error_irq;
} output_config_t;

typedef enum {
    OUTPUT_NV12 = 0,
    OUTPUT_RGB888 = 1,
} output_format_t;

typedef struct {
    uint16_t width, height;
    output_format_t format;
} output_geometry_t;

/* CH0/CH1 cố định NV12, CH2 cố định RGB888. Formatter/join/FIFO không có bit bật CSR riêng. */
void output_get_default_config(output_config_t *cfg, uint16_t active_w, uint16_t active_h);
isp_status_t output_compute_geometry(const output_channel_config_t *cfg, unsigned channel,
                                     uint16_t active_w, uint16_t active_h, output_geometry_t *geometry);
isp_status_t output_validate_config(const output_config_t *cfg, uint16_t active_w, uint16_t active_h);
isp_status_t output_configure(const output_config_t *cfg, uint16_t active_w, uint16_t active_h);
/* Sau khi bật ISP, trước khi cấp quyền DMA: chờ VALID và đối chiếu OUT_W/H của mọi kênh đang bật. */
isp_status_t output_wait_geometry(const output_config_t *cfg, uint16_t active_w, uint16_t active_h,
                                  const isp_wait_t *wait);
#endif
