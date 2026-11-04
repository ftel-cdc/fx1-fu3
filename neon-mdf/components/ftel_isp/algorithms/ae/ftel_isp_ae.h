/*
 * Thuật toán AE tham chiếu chạy CPU: tính sáng trung bình và đề xuất tham số cho frame sau.
 * Khai báo API và cấu trúc dữ liệu.
 */
#ifndef FTEL_ISP_AE_H
#define FTEL_ISP_AE_H
#include "stats/aec/ftel_isp_aec.h"

typedef struct {
    uint32_t exposure_us; /* Thời gian phơi sáng, đơn vị micro giây. */
    uint16_t analog_gain_q8, digital_gain_q8; /* 256 biểu diễn hệ số 1x. */
} ae_params_t;

typedef enum {
    AE_CONTROL_DIGITAL_GAIN,
    AE_CONTROL_EXPOSURE,
    AE_CONTROL_ANALOG_GAIN,
} ae_control_t;

typedef struct {
    ae_params_t initial, minimum, maximum; /* Bộ ban đầu và giới hạn từng tham số. */
    ae_control_t control; /* Mỗi lần xử lý chỉ điều chỉnh một tham số được chọn. */
    uint16_t target, tolerance; /* Mức đo RAW12 đã chuẩn hóa, không phải độ sáng Y8. */
    uint16_t max_step_q8;       /* Từ 256 đến 4096; giới hạn tỷ số tăng/giảm mỗi bước. */
} ae_config_t;

/* current mô tả tham số đã áp thành công; ae_process() không tự sửa trạng thái này. */
typedef struct {
    ae_config_t config;
    ae_params_t current;
} ae_context_t;

void ae_get_default_config(ae_config_t *config);
isp_status_t ae_validate_params(const ae_config_t *config, const ae_params_t *params);
isp_status_t ae_validate_config(const ae_config_t *config);
isp_status_t ae_init(ae_context_t *context, const ae_config_t *config);
isp_status_t ae_statistics_mean(const aec_global_stats_t *stats, uint16_t *mean);
/*
 * Bộ điều khiển AE tham chiếu chỉ chạy CPU, không MMIO/số thực/cấp phát động. Hàm trả tham số đề
 * xuất; current chỉ đổi khi bộ lập lịch áp dụng thành công. Thống kê rỗng/tràn/sai dữ liệu trả lỗi
 * và giữ nguyên đầu ra.
 */
isp_status_t ae_process(const ae_context_t *context, const aec_global_stats_t *stats,
                        ae_params_t *params);
#endif
