/*
 * Cấu hình và đọc thống kê màu AWB: quyền đọc bank, tổng R/G/B và từng vùng.
 * Khai báo API và cấu trúc dữ liệu.
 */
#ifndef FTEL_ISP_AWB_H
#define FTEL_ISP_AWB_H

#include "common/ftel_isp_common.h"

/*
 * AWB: thống kê ô Bayer sau BPC. Mỗi ô RGGB 2x2 đủ điều kiện góp R, trung bình G và B. Kết quả nằm
 * trong hai bank; xin quyền đọc, đọc rồi giải phóng.
 */
typedef struct {
    bool     enable;              /* AWB_CTRL.en                    */
    uint8_t  num_zone_x;          /* 1..64 cột ô Bayer. */
    uint8_t  num_zone_y;          /* 1..32 hàng ô Bayer. */
    uint16_t underexposed_limit;  /* Loại cả ô nếu có mẫu <= v. */
    uint16_t saturation_limit;    /* Loại cả ô nếu có mẫu >= v. */
    uint32_t context_id;          /* Nhãn phần mềm do bên gọi đặt, được chốt tại SOF. */
} awb_stat_config_t;

/* Tổng từng màu trên các ô đủ điều kiện. */
typedef struct {
    uint64_t sum_r;     /* Tổng ô Bayer không dấu 35 bit. */
    uint64_t sum_g;
    uint64_t sum_b;
    uint32_t count;
} awb_zone_stats_t;

typedef struct {
    uint32_t frame_id;
    uint32_t context_id;
    awb_zone_stats_t global;
} awb_global_stats_t;

typedef struct {
    uint8_t  ready_count;   /* 0..2 bank đã hoàn tất, đang READY. */
    bool     leased;        /* Phần mềm đang giữ quyền đọc một bank. */
    bool     overrun;       /* Cờ đã chốt do hết bank hoặc hình học sai. */
    uint8_t  num_zone_x;    /* Lưới được chốt cùng bank đang giữ quyền đọc. */
    uint8_t  num_zone_y;
    uint32_t drop_count;
} awb_lease_status_t;

/*
 * Điền giá trị sau reset: tắt, giới hạn đầy đủ, lưới num_zone_x/y = 0. Lưới 0/0 không hợp lệ; bên
 * gọi phải đặt cả hai chiều trước awb_configure().
 */
void          awb_get_default_config(awb_stat_config_t *cfg);

/*
 * Kiểm tra rồi ghi cấu hình, bật sau cùng. Lưới phải vừa số ô Bayer của active_w/active_h (NX <=
 * W/2, NY <= H/2), tối đa 2048 zone. Truyền 0/0 để bỏ kiểm tra kích thước, chỉ khi bên gọi đã bảo
 * đảm hình học phù hợp.
 */
isp_status_t  awb_validate_config(const awb_stat_config_t *cfg,
                            uint16_t active_w, uint16_t active_h); /* Không truy cập MMIO. */
isp_status_t  awb_configure(const awb_stat_config_t *cfg,
                            uint16_t active_w, uint16_t active_h);

/* Thăm dò bank READY qua AWB_RESULT_STATUS. */
void          awb_get_lease_status(awb_lease_status_t *st);

/* Không chặn: xin quyền đọc bank READY hoàn tất sớm nhất. */
isp_status_t  awb_acquire(void);

/* Giải phóng quyền đọc hiện tại. */
isp_status_t  awb_release(void);

/* Đọc nhận diện frame và tổng các màu toàn ảnh; yêu cầu có quyền đọc. */
isp_status_t  awb_read_global(awb_global_stats_t *st);

/*
 * Đọc zone của bank đang giữ quyền theo index = y*read_num_zone_x + x, dùng lưới chốt trong
 * AWB_RESULT_STATUS. Từ chối chỉ số ngoài lưới hoặc vượt địa chỉ 11 bit.
 */
isp_status_t  awb_read_zone(uint16_t zone_index, awb_zone_stats_t *st);

/* Xóa báo cáo mất phép đo đã chốt và DROP_COUNT. */
void          awb_clear_report(void);

typedef struct {
    awb_global_stats_t global;
    bool overrun;         /* Báo mất phép đo, không phải tràn số học. */
    uint32_t drop_count;
} awb_frame_stats_t;

/*
 * Quy tắc sở hữu như aec_get_stats(). count = 0 là dữ liệu rỗng hợp lệ. Đọc báo cáo không tự xóa;
 * bank đang giữ quyền vẫn có thể hợp lệ khi có OVERRUN.
 */
isp_status_t awb_get_stats(awb_frame_stats_t *stats);

#endif /* FTEL_ISP_AWB_H */
