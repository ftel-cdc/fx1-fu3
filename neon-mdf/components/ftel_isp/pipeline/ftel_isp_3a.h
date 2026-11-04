/*
 * Khung điều phối 3A theo từng frame: áp tham số, ghép thống kê và giữ đề xuất kế tiếp.
 * Khai báo API và cấu trúc dữ liệu.
 */
#ifndef FTEL_ISP_3A_H
#define FTEL_ISP_3A_H
#include "algorithms/ae/ftel_isp_ae.h"
#include "stats/awb/ftel_isp_awb.h"

/*
 * Hàm gọi lại phải bảo đảm giá trị có hiệu lực cho frame đầu vào được chỉ định trước khi trả OK.
 * Lớp nối cảm biến xử lý độ trễ/group-hold. Thiếu hàm cảm biến nghĩa là tham số đó đã được cấu
 * hình cố định bên ngoài. Lỗi áp dụng có thể xảy ra giữa chừng: context chuyển FAULT và chặn tự
 * thử lại.
 */
typedef isp_status_t (*isp_3a_apply_fn)(void *user, uint32_t frame_id, uint32_t context_id, uint32_t value);
typedef struct {
    /* value: gain số Q8, thời gian phơi sáng micro giây hoặc gain tương tự Q8. */
    isp_3a_apply_fn digital_gain, exposure, analog_gain;
    /* NULL dùng AE tham chiếu; hàm thay thế chỉ tính đề xuất, không áp tham số. */
    isp_status_t (*process_ae)(void *user, const ae_context_t *ae,
                             const aec_global_stats_t *stats, ae_params_t *proposal);
} isp_3a_ops_t;

typedef struct {
    ae_config_t ae;
    aec_config_t aec;
    awb_stat_config_t awb;
    uint16_t active_width, active_height; /* Kích thước hữu dụng sau IFMT. */
    bool collect_awb; /* Thu thập thống kê AWB; chưa có thuật toán tự tính gain WB. */
    uint32_t context_seed; /* Chọn giá trị khởi đầu mới sau khôi phục/khởi động lại. */
} isp_3a_config_t;

typedef struct {
    uint32_t frame_id, context_id; /* Số thứ tự đầu vào phần cứng và nhãn cấu hình phần mềm. */
    bool aec_valid, awb_valid; /* Đã sao chép đúng frame/nhãn; count vẫn có thể bằng 0. */
    isp_status_t ae_status, awb_status; /* Trạng thái sử dụng số đo, khác mã trả về của poll. */
    aec_frame_stats_t aec;
    awb_frame_stats_t awb;
    ae_params_t applied, next; /* Bộ đang áp cho frame này và bộ chờ áp ở prepare kế tiếp. */
} isp_3a_result_t;

typedef enum {
    ISP_3A_UNINITIALIZED,
    ISP_3A_READY,
    ISP_3A_WAIT_STATS,
    ISP_3A_FAULT,
} isp_3a_state_t;

typedef struct {
    isp_3a_state_t state;
    isp_3a_config_t config;
    isp_3a_ops_t ops;
    void *user;
    ae_context_t ae;
    ae_params_t pending; /* Chỉ cập nhật current sau khi mọi hàm áp dụng trả thành công. */
    isp_3a_result_t result;
    uint32_t token;
    bool have_frame, aec_done, awb_done; /* Mỗi bank của một frame chỉ thu thập một lần. */
} isp_3a_context_t;

void isp_3a_get_default_config(isp_3a_config_t *config, uint16_t active_w, uint16_t active_h);
void isp_3a_get_default_ops(isp_3a_ops_t *ops);
isp_status_t isp_3a_apply_digital_gain(void *user, uint32_t frame_id, uint32_t context_id, uint32_t gain_q8);
/* Chỉ tạo trạng thái phần mềm; gọi isp_init() của driver trước khi chuẩn bị frame đầu. */
isp_status_t isp_3a_init(isp_3a_context_t *context, const isp_3a_config_t *config,
                        const isp_3a_ops_t *ops, void *user);
/*
 * Lập lịch tuần tự, không có RAW đang chạy/đã xếp hàng. Ghi cấu hình thống kê/context và áp dụng
 * tham số ban đầu/đang chờ. Sau thành công, bên gọi mới gửi đúng frame này. Hàm không cấp phát bộ
 * đệm, cấp quyền DMA hay tạo FRAME_START.
 */
isp_status_t isp_3a_prepare_frame(isp_3a_context_t *context, uint32_t frame_id);
/*
 * Không chặn. NO_DATA: tiếp tục chờ; STALE: đã bỏ kết quả cũ/không khớp và vẫn chờ. OK: đã thu
 * thập đủ frame; xem result.ae_status vì NO_DATA/OVERFLOW giữ tham số cũ. AEC/AWB có thể sẵn sàng
 * ở hai lần thăm dò khác nhau; mỗi kết quả chỉ sao chép/giải phóng một lần.
 */
isp_status_t isp_3a_poll(isp_3a_context_t *context, isp_3a_result_t *result);
/*
 * Sau khi frame xử lý hết, bỏ qua thống kê bị thiếu và giữ gain. Kết quả đến muộn sẽ bị lọc theo
 * frame/context ở các lần thăm dò sau.
 */
isp_status_t isp_3a_skip_statistics(isp_3a_context_t *context);
#endif
