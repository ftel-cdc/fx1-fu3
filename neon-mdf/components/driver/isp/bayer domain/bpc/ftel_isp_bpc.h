/*
 * Cấu hình sửa pixel lỗi bằng ngưỡng không gian; không có lịch sử lỗi qua frame.
 * Khai báo API và cấu trúc dữ liệu.
 */
#ifndef FTEL_ISP_BPC_H
#define FTEL_ISP_BPC_H

#include "common/ftel_isp_common.h"

/*
 * BPC: sửa pixel lỗi bằng lân cận không gian.
 */
typedef struct {
    bool     enable;        /* BPC_CTRL.en                                    */
    uint16_t thresh_k;      /* Hệ số ngưỡng tương đối, BPC_THRESH[31:16], sau reset là 0x4D. */
    uint16_t thresh_floor;  /* Ngưỡng sàn tuyệt đối, BPC_THRESH[15:0], sau reset là 0x192. */
} bpc_config_t;

/* Bộ đếm theo frame; cả hai kẹp tối đa 16383. */
typedef struct {
    uint16_t candidates;    /* Số pixel bị đánh dấu nghi lỗi. */
    uint16_t defective;     /* Số pixel thực sự được sửa. */
} bpc_stats_t;

/* Điền mặc định theo đặc tả: tắt khối, ngưỡng bằng giá trị sau reset. */
void          bpc_get_default_config(bpc_config_t *cfg);

/* Kiểm tra, ghi ngưỡng rồi bật sau cùng. */
isp_status_t  bpc_validate_config(const bpc_config_t *cfg); /* Không truy cập MMIO. */
isp_status_t  bpc_configure(const bpc_config_t *cfg);

/* Đọc bộ đếm của frame vừa hoàn tất gần nhất. */
void          bpc_get_stats(bpc_stats_t *st);

#endif /* FTEL_ISP_BPC_H */
