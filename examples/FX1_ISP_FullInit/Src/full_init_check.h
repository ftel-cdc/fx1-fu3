/* Kiểm chứng init tổng trên đích, không nạp RAW hay cấp quyền bộ đệm DMA. */
#ifndef ISP_FULL_INIT_CHECK_H
#define ISP_FULL_INIT_CHECK_H

#include "init_check.h"

/* Báo riêng giới hạn profile 0 của VP; không biến cấu hình mặc định lỗi thành READY. */
int isp_full_init_check(const init_test_platform_t *platform, uint32_t timeout_us);

#endif
