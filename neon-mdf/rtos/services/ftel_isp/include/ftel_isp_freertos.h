#ifndef FTEL_ISP_FREERTOS_H
#define FTEL_ISP_FREERTOS_H

#include "ftel_isp.h"
#include "FreeRTOS.h"
#include "pipeline/ftel_isp_stream.h"

#ifdef __cplusplus
extern "C" {
#endif

#define FTEL_ISP_FREERTOS_MAX_JOBS 3u

typedef struct {
    uint32_t slot, generation;
} ftel_isp_ticket_t;

typedef struct {
    configSTACK_DEPTH_TYPE stack_words;
    UBaseType_t task_priority;
    uint32_t irq_priority;
    uint32_t recovery_timeout_us;
} ftel_isp_freertos_config_t;

typedef struct {
    ftel_isp_status_t status, recovery_status;
    ftel_isp_init_result_t init;
    ftel_isp_frame_result_t frame;
    ftel_isp_aec_stats_t aec;
    uint32_t irq_count, common_irq, dma_irq, output_irq, dma_errors;
    bool frame_submitted, stats_valid, recovered, buffers_released;
} ftel_isp_job_result_t;

/* Một job toàn pipeline chạy liên tiếp 1..4 slot, chỉ init một lần.
 * Giữ trạng thái giữa các frame (ví dụ GTM auto), chưa có vòng kín 3A. */
#define FTEL_ISP_FREERTOS_MAX_FRAMES 4u
typedef struct {
    const isp_config_t *config;
    uint32_t frame_count, frame_timeout_us;
    uint32_t tag[FTEL_ISP_FREERTOS_MAX_FRAMES];
    uint32_t timestamp[FTEL_ISP_FREERTOS_MAX_FRAMES];
} ftel_isp_pipeline_request_t;

typedef struct {
    ftel_isp_job_result_t job; /* status/IRQ/recovery/buffer; không dùng init/frame/aec ở đây. */
    isp_init_result_t init;
    uint32_t completed_frames;
    isp_frame_result_t frame[FTEL_ISP_FREERTOS_MAX_FRAMES];
} ftel_isp_pipeline_result_t;

/* Copy config/metadata, không copy RAW, output hoặc bảng LSC được trỏ tới.
 * Caller giữ bảng LSC hợp lệ và bất biến, giữ toàn bộ 4 slot RAW/output của
 * các kênh bật, tới khi release ticket (kể cả slot chưa chạy).
 * Các vùng phải nằm trong DDR dữ liệu, không chồng nhau hoặc ticket khác.
 * Nhận tối đa hai bảng LSC; service có thể đổi resident vật lý để tránh nạp
 * vào active resident còn giữ sau reset. Hệ số không đổi; init.lsc.profile
 * báo resident vật lý. Ba bảng trả E_UNSUPPORTED trước khi queue.
 * Dùng cùng worker/queue/recovery với Bayer/AEC. Không gọi legacy API từ
 * task khác khi service đã start. E_CONFIG: chưa xếp job/chưa ghi MMIO. */
ftel_isp_status_t ftel_isp_freertos_submit_pipeline(const ftel_isp_pipeline_request_t *request,
                                                  ftel_isp_ticket_t *ticket);
/* Chỉ dùng với ticket pipeline; sai loại trả E_STATE, không tiêu thụ kết quả.
 * wait hết hạn không hủy job; cùng quy tắc caller/release như API Bayer/AEC. */
ftel_isp_status_t ftel_isp_freertos_wait_pipeline(ftel_isp_ticket_t ticket,
                                                TickType_t wait_ticks,
                                                ftel_isp_pipeline_result_t *result);

void ftel_isp_freertos_get_default_config(ftel_isp_freertos_config_t *config);
/* Gọi từ task hart 0 khi scheduler đang chạy, một lần mỗi boot. Worker nhận
 * quyền ISP; các API thuần truy cập MMIO từ task khác sẽ trả E_BUSY. */
ftel_isp_status_t ftel_isp_freertos_start(const ftel_isp_freertos_config_t *config);
/* Copy config/descriptor, không copy pixel. Từ khi submit thành công tới khi
 * release ticket, caller giữ cả RAW/Y/UV, không đọc/ghi hoặc gửi chúng nơi khác.
 * Tối đa MAX_JOBS ticket, một frame phần cứng đang chạy; init lại mỗi job. */
ftel_isp_status_t ftel_isp_freertos_submit(const ftel_isp_config_t *config,
                                         const ftel_isp_frame_t *frame,
                                         uint32_t frame_timeout_us,
                                         ftel_isp_ticket_t *ticket);
/* Các API service chỉ gọi từ task, không gọi từ ISR. Task submit phải sống
 * tới khi release ticket. Chỉ task đã submit được wait/release.
 * E_TIMEOUT ở wait chỉ hết hạn chờ của
 * caller: job vẫn sống và buffer vẫn được giữ, có thể wait lại cùng ticket.
 * OK nghĩa đã lấy result; result.status là kết quả xử lý frame. */
ftel_isp_status_t ftel_isp_freertos_wait(ftel_isp_ticket_t ticket,
                                       TickType_t wait_ticks,
                                       ftel_isp_job_result_t *result);
/* E_BUSY khi job chưa xong hoặc recovery chưa trả buffer. Sau release, ticket
 * cũ trở thành E_STALE; chỉ lúc này caller được tái sử dụng buffer. */
ftel_isp_status_t ftel_isp_freertos_release(ftel_isp_ticket_t ticket);
/* Thử lại recovery bằng worker nếu auto-recovery hết hạn. Trong thời gian
 * faulted, từ chối job mới; job chưa chạy được hủy an toàn. Khi recovery thành
 * công, wait lại để lấy buffers_released=true rồi release ticket. */
ftel_isp_status_t ftel_isp_freertos_recover(TickType_t wait_ticks);

#ifdef __cplusplus
}
#endif
#endif
