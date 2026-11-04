/* Điểm vào VP của bộ kiểm chứng init tổng; chưa chạy frame hay thuật toán 3A. */
#include "fx1_fw.h"
#include "common/ftel_isp_common.h"
#include "full_init_check.h"

_Static_assert(ISP_BASE == FX1_ISP_CSR_BASE, "test ISP base must match BSP");

static uint64_t time_us(void)
{
    return fx1_mtime() / 20u;
}

static bool starts_with(const char *text, const char *prefix)
{
    while (*prefix) {
        if (*text++ != *prefix++) return false;
    }
    return true;
}

static void report(const char *message, uint32_t value)
{
    fx1_puts(message);
    /* Giữ dòng kết quả máy đọc được; các bản ghi thanh ghi vẫn in dạng hex. */
    if (!starts_with(message, "INIT_CASE ") && !starts_with(message, "FULL_CHECKS ")) {
        fx1_puts(" = ");
        fx1_puthex(value);
    }
    fx1_puts("\n");
}

int main(void)
{
    fx1_puts("[PASS] FW_BOOT: full ISP init verification\n");
    fx1_puts("[INFO] Init/register checks only. No RAW, DMA frame, or pixel verdict.\n");
    const init_test_platform_t platform = {time_us, report};
    int status = isp_full_init_check(&platform, 1000);
    report("[INFO] full init check status", (uint32_t)status);
    fx1_puts(status == ISP_OK ? "INIT_FULL_RESULT=PASS\n" : "INIT_FULL_RESULT=FAIL\n");
    return status == ISP_OK ? 0 : 0x33;
}
