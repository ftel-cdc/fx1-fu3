/*
 * Cấu hình RAW10/RAW12, pha Bayer và kích thước hữu dụng sau chuẩn hóa về RGGB.
 * Khai báo API và cấu trúc dữ liệu.
 */
#ifndef FTEL_ISP_IFMT_H
#define FTEL_ISP_IFMT_H

#include "common/ftel_isp_common.h"

/*
 * IFMT: định dạng đầu vào và chuẩn hóa pha Bayer.
 */
typedef enum { IFMT_SOURCE_DDR = 0, IFMT_SOURCE_STREAM = 1 } ifmt_source_t;

typedef struct {
    uint16_t         width;      /* Chiều rộng ngoài: số chẵn, 32..3842. */
    uint16_t         height;     /* Chiều cao ngoài: số chẵn, 32..2162. */
    bayer_pattern_t  bayer;      /* RGGB/GRBG/GBRG/BGGR              */
    uint8_t          depth;      /* INPUT_DEPTH_RAW12 hoặc RAW10     */
    bool crop;                  /* false: same-CFA padding; true: legacy crop. */
    ifmt_source_t source;       /* Stream cannot replay the trailing parity row. */
    bool drop;                 /* Stream: drop at SOF when admission is unavailable. */
} ifmt_config_t;

/* Điền các giá trị mặc định theo đặc tả vào cfg. */
void          ifmt_get_default_config(ifmt_config_t *cfg);
isp_status_t  ifmt_validate_config(const ifmt_config_t *cfg); /* Không truy cập MMIO. */

/*
 * Kích thước hữu dụng sau khi IFMT bỏ cột/hàng đầu theo pha Bayer và làm tròn xuống số chẵn:
 * Crop: ((W-SH)&~1, (H-SV)&~1). Pad: capped even W/H; stream H excludes SV. LSC/AEC/AWB phải dùng kích thước này thay cho kích thước ngoài. Trả -1
 * nếu cấu hình rỗng hoặc không hợp lệ.
 */
int           ifmt_active_size(const ifmt_config_t *cfg,
                               uint16_t *active_w, uint16_t *active_h);

/*
 * Kiểm tra và ghi phần cứng; đổi đầu vào yêu cầu đã dừng/xử lý hết dữ liệu. Trả ISP_E_NOT_IDLE nếu
 * phần cứng từ chối ghi lớp IDLE dù AXI vẫn trả OKAY.
 */
isp_status_t  ifmt_configure(const ifmt_config_t *cfg);

typedef struct {
    uint32_t status, errors, frame_count, drop_count;
} ifmt_stream_status_t;
/* Read live adapter state. Counter values are independent live snapshots. */
isp_status_t ifmt_read_stream_status(ifmt_stream_status_t *status);
/* W1C only; DMA_ERR.STREAM is a separate sticky flag. */
isp_status_t ifmt_clear_stream_errors(uint32_t errors);

#endif /* FTEL_ISP_IFMT_H */
