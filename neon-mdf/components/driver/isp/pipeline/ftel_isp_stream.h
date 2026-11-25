/* Điều hành frame bằng polling: một frame đang chạy, bốn slot xoay vòng, CPU giữ output tới release. */
#ifndef FTEL_ISP_STREAM_H
#define FTEL_ISP_STREAM_H
#include "pipeline/ftel_isp_init.h"

typedef enum {
    ISP_STREAM_CLOSED = 0, ISP_STREAM_READY, ISP_STREAM_RUNNING,
    ISP_STREAM_HELD, ISP_STREAM_STOPPED, ISP_STREAM_FAULT
} isp_stream_state_t;

typedef struct {
    uint32_t sequence;          /* Thứ tự của phần mềm; không đồng nghĩa AEC frame_id. */
    uint8_t slot, outputs;
    uint32_t tag[3], timestamp[3], sof_timestamp[3], eof_timestamp[3];
    dma_frame_status_t dma;     /* Snapshot trước khi xóa báo thiếu input do chủ động dừng giữa frame. */
} isp_frame_result_t;

typedef struct {
    isp_stream_state_t state;
    uint8_t next_slot, outputs;
    uint32_t sequence, recovery_sequence, input_count, output_count[3];
    isp_frame_result_t completed;
} isp_stream_t;

/*
 * Khởi tạo context bằng {0}. Gọi sau isp_init() thành công, trước frame đầu tiên của lần reset.
 * cfg phải là cấu hình vừa init; bốn buffer mỗi loại cần địa chỉ thật, không chồng lấn.
 * Caller sở hữu duy nhất ISP và cấu hình; không gọi reset/config/ISR khác trong stream.
 * Start bật ISP và kiểm tra geometry, chưa cấp quyền buffer. Không tự init lại hay bỏ bảng LSC.
 */
isp_status_t isp_stream_start(isp_stream_t *stream, const isp_config_t *cfg, const isp_wait_t *wait);
/* Caller ghi RAW vào next_slot trước submit; output chỉ đọc sau poll/wait trả ISP_OK. */
isp_status_t isp_stream_submit(isp_stream_t *stream, uint32_t tag, uint32_t timestamp);
isp_status_t isp_stream_poll(isp_stream_t *stream, isp_frame_result_t *result);
/* Timeout giữ nguyên quyền DMA/trạng thái RUNNING; không được sửa hoặc tái sử dụng buffer. */
isp_status_t isp_stream_wait(isp_stream_t *stream, const isp_wait_t *wait, isp_frame_result_t *result);
/* Caller đã đọc/dump xong output: ACK DONE, chuyển slot tiếp; chưa cấp FREE lần nữa. */
isp_status_t isp_stream_release(isp_stream_t *stream);
/* Chỉ stop khi READY, không có frame hay output đang giữ. Start lần nữa cần isp_init() và context mới. */
isp_status_t isp_stream_stop(isp_stream_t *stream, uint32_t poll_limit);
#endif
