/*
 * Bài kiểm tra trên đích gọi đúng isp_init() của driver và báo kết quả từng giai đoạn.
 * Khai báo API và cấu trúc dữ liệu.
 */
#ifndef ISP_INIT_CHECK_H
#define ISP_INIT_CHECK_H

#include <stdint.h>

/* Các hàm gọi lại phục vụ ứng dụng test, không phải API driver sản phẩm mới. */
typedef struct {
    uint64_t (*time_us)(void);
    void (*report)(const char *message, uint32_t value);
} init_test_platform_t;

#define INIT_TEST_E_READBACK (-10)

/*
 * Gọi đúng isp_init() của FX1_ISP_SDK với cấu hình đầy đủ mặc định. Khởi tạo điều
 * khiển/LUT/thống kê/output/DMA ở legacy, chưa gửi frame. Hàm báo cáo liệt kê các bước đã hoàn
 * tất.
 */
int isp_init_check(const init_test_platform_t *platform, uint32_t timeout_us);

#endif
