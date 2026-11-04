/*
 * Điều khiển ISP dùng chung: ABI, trạng thái rỗi, hàng rào MMIO và chờ có giới hạn.
 * Khai báo API và cấu trúc dữ liệu.
 */
#ifndef FTEL_ISP_COMMON_H
#define FTEL_ISP_COMMON_H

#include "registers/ftel_isp_regs.h"
#include <stddef.h>

/*
 * Mã trạng thái trả về của driver.
 */
typedef enum {
    ISP_OK          =  0,
    ISP_E_PARAM     = -1,
    ISP_E_BAD_ABI   = -2,
    ISP_E_NOT_IDLE  = -3,
    ISP_E_TIMEOUT   = -4,
    ISP_E_IO        = -5,
    ISP_E_UNSUPPORTED = -6,
    ISP_E_NO_DATA   = -7,
    ISP_E_OVERFLOW  = -8,
    ISP_E_STALE     = -9,
    ISP_E_STATE     = -10,
} isp_status_t;

/*
 * Các hàm hỗ trợ dùng chung giữa các khối.
 */
/* Bảo đảm thứ tự truy cập MMIO: dùng fence trên RISC-V, DSB trên ARM. */
void          isp_barrier(void);

/*
 * Hàng rào bộ nhớ ngăn CPU/bus sắp xếp lại truy cập: DSB trên ARM, fence trên RISC-V; nền tảng
 * khác dùng hàng rào của trình biên dịch.
 */
void          isp_memory_barrier(void);

/* So khớp R_COMMON_VER_ID với ISP_ABI_VERSION. */
isp_status_t  isp_check_abi(void);

/* Trả true khi pipeline đang bận (COMMON_STATUS.BUSY). */
bool          isp_pipeline_busy(void);

/*
 * Trả true khi pipeline, DMA, RAW đã xếp hàng hoặc recovery chưa rỗi; dùng để bảo vệ các ghi thuộc
 * lớp IDLE của MAS 9.9.3.
 */
bool          isp_not_idle(void);

/*
 * Bên gọi ngừng xếp RAW trước. Giữ ISP bật để xử lý hết công việc còn lại, rồi tắt DMA/ISP.
 * timeout đếm lần thăm dò; hết hạn giữ nguyên các bit bật, không hủy frame.
 */
isp_status_t  isp_stop_and_wait(uint32_t timeout);

/*
 * Xóa các lỗi đã chốt trong TUNE_ERROR bằng W1C. Hàm không tự cấu hình; bên gọi kiểm tra
 * isp_config_status() sau chuỗi ghi.
 */
isp_status_t  isp_tune_error_clear(void);

/* Trả true khi TUNE_ERROR còn ít nhất một lỗi đã chốt. */
bool          isp_tune_error_set(void);

/* Ghi nguyên từ COMMON_CTRL; tránh đọc-sửa-ghi vì có các bit lệnh W1S/W1SC. */
isp_status_t  isp_clear_common_ctrl(void);
isp_status_t  isp_enable_pipeline(void);

/* Trả true khi ISP đang ở chế độ profile của phần mở rộng TUNE. */
bool          isp_in_profile_mode(void);

/* Kiểm tra ABI, định danh TUNE và toàn bộ bit tính năng được yêu cầu. */
isp_status_t  isp_check_tune_cap(uint32_t cap_bit);

/* Hỗ trợ cấu hình ở chế độ legacy; bên gọi phải tuần tự hóa truy cập cùng vùng thanh ghi ISP. */
isp_status_t  isp_require_legacy(void);
isp_status_t  isp_prepare_config(void);
isp_status_t  isp_config_status(void);
bool          isp_reg_matches(uint32_t address, uint32_t mask, uint32_t value);
isp_status_t  isp_write_checked(uint32_t address, uint32_t mask, uint32_t value);
typedef struct {
    uint32_t address, mask, value;
} isp_reg_setting_t;

isp_status_t isp_configure_registers(const isp_reg_setting_t *list, size_t count);

typedef struct {
    uint64_t (*time_us)(void *user); /* NULL: chờ theo số lần thăm dò có giới hạn. */
    void *user;
    uint32_t timeout_us;           /* Giới hạn thời gian khi có time_us. */
    uint32_t poll_limit;           /* Luôn giới hạn số lần thăm dò, kể cả khi đồng hồ đứng. */
} isp_wait_t;

void isp_get_default_wait(isp_wait_t *wait);
isp_status_t isp_wait_register(uint32_t address, uint32_t mask, uint32_t value,
                               const isp_wait_t *wait);

#endif /* FTEL_ISP_COMMON_H */
