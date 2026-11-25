/*
 * Cấu hình và đọc thống kê sáng AEC: quyền đọc bank, tổng/count, vùng và histogram.
 * Khai báo API và cấu trúc dữ liệu.
 */
#ifndef FTEL_ISP_AEC_H
#define FTEL_ISP_AEC_H

#include "common/ftel_isp_common.h"

/*
 * AEC: khối thống kê đo sáng. Ghi AEC_* rồi phát AEC_CTRL.commit; cấu hình đệm có hiệu lực tại SOF
 * đầu vào được tiếp nhận kế tiếp. Đọc kết quả qua quyền đọc hai bank TUNE_AEC_*: xin quyền, đọc,
 * giải phóng.
 */
typedef struct {
    uint8_t  num_zone_x;    /* 1..32                                  */
    uint8_t  num_zone_y;    /* 1..24                                  */
    uint16_t zone_width;    /* 1..4095; zone cuối phủ phần ảnh còn lại. */
    uint16_t zone_height;   /* 1..4095                                */
} aec_zone_config_t;

typedef enum {
    AEC_SOURCE_BPC = 0,        /* Mặc định đo sau BPC. */
    AEC_SOURCE_DG  = 1,        /* Đo sau WB và DG. */
} aec_source_t;

typedef struct {
    bool     enable;           /* AEC_CTRL.en                        */
    aec_source_t source;       /* Chọn điểm lấy dữ liệu đo qua TUNE_AEC_SOURCE. */
    uint32_t context_id;       /* Nhãn phần mềm do bên gọi đặt, được xuất cùng thống kê. */
    aec_zone_config_t zones;
    uint16_t sample_min_clip;  /* Giới hạn mẫu được chấp nhận, tính cả hai đầu mút, trong 0..4095. */
    uint16_t sample_max_clip;  /* Giới hạn mẫu được chấp nhận, tính cả hai đầu mút, trong 0..4095. */
    uint16_t thresh_ue;        /* Đếm thiếu sáng UE khi sample < thresh_ue. */
    uint16_t thresh_oe;        /* Đếm quá sáng OE khi sample > thresh_oe. */
} aec_config_t;

/*
 * Thống kê một zone đọc từ bản ghi 64 bit đầy đủ (MAS, Bảng 194, 0x200 + 8*z), thay cho cửa sổ hẹp
 * modulo. Ở 4K, zone có thể vượt cả giới hạn 2^16 mẫu hoặc tổng 2^28.
 */
typedef struct {
    uint64_t sum;         /* Tổng giá trị Bayer của các mẫu đủ điều kiện. */
    uint64_t count;       /* Số mẫu đủ điều kiện. */
    uint64_t ue_count;    /* Số mẫu dưới TH_UE. */
    uint64_t oe_count;    /* Số mẫu trên TH_OE. */
    uint16_t min;         /* Zone rỗng đọc về 4095. */
    uint16_t max;         /* Zone rỗng đọc về 0. */
    bool     empty;       /* Zone rỗng khi count == 0. */
    bool     overflow;    /* Cờ tràn được chốt theo từng kết quả. */
} aec_zone_stats_t;

/* Kết quả toàn ảnh và thông tin nhận diện frame của bank đang giữ quyền đọc. */
typedef struct {
    uint32_t frame_id;
    uint32_t context_id;
    uint64_t global_sum;    /* Tổng đầy đủ 64 bit. */
    uint64_t global_count;  /* Số mẫu đầy đủ 64 bit. */
    uint64_t oe_count, ue_count;
    uint16_t oe_percent_q8, ue_percent_q8; /* 100 percent = 0x6400; leased frame. */
    bool     overflow;      /* Cờ tràn được chốt của kết quả này. */
} aec_global_stats_t;

/*
 * Điền mặc định theo đặc tả. active_w/active_h là kích thước hữu dụng do ifmt_active_size() suy
 * ra; giảm lưới 32x24 nếu ảnh nhỏ không thỏa quy tắc phủ ảnh.
 */
void          aec_get_default_config(aec_config_t *cfg,
                                     uint16_t active_w, uint16_t active_h);

/*
 * Kiểm tra, ghi đủ các trường cấu hình đệm rồi COMMIT. Phải thỏa (Nx-1)*zone_width < active_w <=
 * Nx*zone_width, và tương tự theo chiều Y.
 */
isp_status_t  aec_validate_config(const aec_config_t *cfg,
                            uint16_t active_w, uint16_t active_h); /* Không truy cập MMIO. */
isp_status_t  aec_configure(const aec_config_t *cfg,
                            uint16_t active_w, uint16_t active_h);

/* Không chặn: xin quyền đọc bank READY hoàn tất sớm nhất. */
isp_status_t  aec_acquire(void);

/* Giải phóng quyền đọc hiện tại; nếu chưa giữ bank thì trả ISP_E_NOT_IDLE và không ghi lệnh. */
isp_status_t  aec_release(void);

/* Trả 1 khi đang giữ quyền đọc một bank. */
bool          aec_lease_valid(void);

/* Đọc nhận diện frame và kết quả toàn ảnh; yêu cầu đang giữ quyền đọc. */
isp_status_t  aec_read_global(aec_global_stats_t *st);

/*
 * Đọc zone của bank đang giữ quyền: index = y*num_zone_x + x. Giới hạn theo lưới được chốt cùng
 * bank ở TUNE_AE_GRID.
 */
isp_status_t  aec_read_zone(uint16_t zone_index, aec_zone_stats_t *st);

/*
 * Chọn điểm đo BPC hoặc DG trước frame. Nguồn đo thuộc tập cấu hình đệm AEC nên hàm cũng phát
 * COMMIT.
 */
isp_status_t  aec_set_source(aec_source_t source);

/* Đọc điểm đo đã tạo thống kê của frame đang giữ quyền; yêu cầu có quyền đọc. */
isp_status_t  aec_get_result_source(aec_source_t *source);

/*
 * Đọc số mẫu của bin histogram 0..63 từ bản ghi 64 bit tại 0x100 + bin. API 32 bit này kẹp các giá
 * trị vượt 2^32-1 về cực đại.
 */
isp_status_t  aec_read_hist(uint8_t bin, uint32_t *count);

/* API histogram 64 bit không mất dữ liệu; API 32 bit ở trên giữ cơ chế kẹp để tương thích. */
isp_status_t aec_read_hist64(uint8_t bin, uint64_t *count);
typedef struct {
    aec_global_stats_t global;
    aec_source_t source;
    uint64_t histogram[AEC_HIST_BINS];
} aec_frame_stats_t;

/*
 * Không chặn: xin quyền, đọc và giải phóng một bản sao thống kê toàn frame. Chưa có READY trả
 * NO_DATA; quyền đọc đã tồn tại trả NOT_IDLE và được giữ nguyên. Sau khi tự xin được quyền, hàm
 * luôn thử giải phóng kể cả khi đọc lỗi. Chỉ xuất bản sao khi đọc và giải phóng đều thành công;
 * bên gọi tuần tự hóa truy cập.
 */
isp_status_t aec_get_stats(aec_frame_stats_t *stats);

#endif /* FTEL_ISP_AEC_H */
