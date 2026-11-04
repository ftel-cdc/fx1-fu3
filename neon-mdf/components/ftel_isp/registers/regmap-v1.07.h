/*
 * Bản đồ thanh ghi FTEL FX1 ISP theo đặc tả v1.07. Nguồn: FTEL_IP_ISP_CSR_v1.07.xlsm, các sheet Flat
 * Registers và Summary. Địa chỉ lệch tính theo byte từ ISP_BASE do SoC quy định; bus AXI4-Lite, từ
 * 32 bit. Tệp không quy định địa chỉ vật lý CPU, clock, hard reset hoặc bộ nhận MIPI-CSI-2. Hằng
 * *_RESET của trường là giá trị chưa dịch bit; *_RESET của thanh ghi là cả từ 32 bit. RO: chỉ đọc;
 * RW: đọc/ghi; W1C: ghi 1 để xóa; W1S: ghi 1 để đặt yêu cầu; W1P: ghi 1 phát xung, đọc 0; W1SC: ghi
 * 1 tự xóa, đọc 0. Không đọc-sửa-ghi tùy tiện thanh ghi lệnh/trộn nhiều kiểu truy cập. Giá trị CSR
 * sau reset không khởi tạo SRAM/bảng. Luồng profile đầy đủ là phần riêng. Chú thích đã Việt hóa để
 * bàn giao; mã khai báo và hằng số giữ nguyên bản đồ nguồn.
 */
#ifndef FTEL_FX1_ISP_REGMAP_V107_H
#define FTEL_FX1_ISP_REGMAP_V107_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define FX1_ISP_REG_COUNT 573U
#define FX1_ISP_FIELD_COUNT 819U
#define FX1_ISP_MAX_REGISTER_OFFSET 0x8C90U
#define FX1_ISP_EXPECTED_VER_ID 0x00010007U
#define FX1_ISP_EXPECTED_VER_DATE 0x18102026U
#define FX1_ISP_EXPECTED_TUNE_REV 0x00010007U

#define FX1_ISP_ACCESS_RO 0U
#define FX1_ISP_ACCESS_RW 1U
#define FX1_ISP_ACCESS_W1C 2U
#define FX1_ISP_ACCESS_W1S 3U
#define FX1_ISP_ACCESS_W1P 4U
#define FX1_ISP_ACCESS_W1SC 5U
#define FX1_ISP_ACCESS_MIXED 6U

/* Hàm MMIO cơ bản; BSP phải ánh xạ đúng vùng CSR và bảo đảm thứ tự truy cập. */
static inline uint32_t fx1_isp_mmio_read32(uintptr_t base, uint32_t offset)
{
    return *(volatile const uint32_t *)(base + (uintptr_t)offset);
}
static inline void fx1_isp_mmio_write32(uintptr_t base, uint32_t offset, uint32_t value)
{
    *(volatile uint32_t *)(base + (uintptr_t)offset) = value;
}
static inline uint32_t fx1_isp_field_get(uint32_t word, uint32_t mask, uint32_t shift)
{
    return (word & mask) >> shift;
}
static inline uint32_t fx1_isp_field_prep(uint32_t value, uint32_t mask, uint32_t shift)
{
    return (value << shift) & mask;
}

/* Toàn bộ thanh ghi trong đặc tả, mặt nạ truy cập và các trường bit thành phần. */

/**
 * COMMON_VER_DATE: địa chỉ lệch 0x0000, dòng bảng 2, kiểu truy cập RO
 * Ngày phiên bản regmap cố định theo BCD, không phải thời điểm biên dịch.
 */
#define FX1_ISP_COMMON_VER_DATE_OFFSET 0x0000U
#define REG_COMMON_VER_DATE FX1_ISP_COMMON_VER_DATE_OFFSET
#define FX1_ISP_COMMON_VER_DATE_RESET 0x18102026U
#define FX1_ISP_COMMON_VER_DATE_ACCESS FX1_ISP_ACCESS_RO
#define FX1_ISP_COMMON_VER_DATE_RO_MASK 0xFFFFFFFFU
#define FX1_ISP_COMMON_VER_DATE_RW_MASK 0x00000000U
#define FX1_ISP_COMMON_VER_DATE_W1C_MASK 0x00000000U
#define FX1_ISP_COMMON_VER_DATE_W1S_MASK 0x00000000U
#define FX1_ISP_COMMON_VER_DATE_W1P_MASK 0x00000000U
#define FX1_ISP_COMMON_VER_DATE_W1SC_MASK 0x00000000U
/**
 * year [15:0], dòng bảng 3, kiểu truy cập RO
 * Năm phiên bản dạng BCD; bản đồ hiện tại đọc 0x2026.
 */
#define FX1_ISP_COMMON_VER_DATE_YEAR_SHIFT 0U
#define FX1_ISP_COMMON_VER_DATE_YEAR_WIDTH 16U
#define FX1_ISP_COMMON_VER_DATE_YEAR_MASK 0x0000FFFFU
#define FX1_ISP_COMMON_VER_DATE_YEAR_RESET 0x00002026U
#define FX1_ISP_COMMON_VER_DATE_YEAR_ACCESS FX1_ISP_ACCESS_RO
/**
 * month [23:16], dòng bảng 4, kiểu truy cập RO
 * Tháng phiên bản dạng BCD.
 */
#define FX1_ISP_COMMON_VER_DATE_MONTH_SHIFT 16U
#define FX1_ISP_COMMON_VER_DATE_MONTH_WIDTH 8U
#define FX1_ISP_COMMON_VER_DATE_MONTH_MASK 0x00FF0000U
#define FX1_ISP_COMMON_VER_DATE_MONTH_RESET 0x00000010U
#define FX1_ISP_COMMON_VER_DATE_MONTH_ACCESS FX1_ISP_ACCESS_RO
/**
 * day [31:24], dòng bảng 5, kiểu truy cập RO
 * Ngày trong tháng của phiên bản, dạng BCD.
 */
#define FX1_ISP_COMMON_VER_DATE_DAY_SHIFT 24U
#define FX1_ISP_COMMON_VER_DATE_DAY_WIDTH 8U
#define FX1_ISP_COMMON_VER_DATE_DAY_MASK 0xFF000000U
#define FX1_ISP_COMMON_VER_DATE_DAY_RESET 0x00000018U
#define FX1_ISP_COMMON_VER_DATE_DAY_ACCESS FX1_ISP_ACCESS_RO

/**
 * COMMON_VER_ID: địa chỉ lệch 0x0004, dòng bảng 6, kiểu truy cập RO
 * ABI lập trình pipeline 1.07: thống kê Bayer RAW12, resize theo mã kích thước định sẵn, crop từng
 * kênh và tính CSC với trung gian đủ rộng.
 */
#define FX1_ISP_COMMON_VER_ID_OFFSET 0x0004U
#define REG_COMMON_VER_ID FX1_ISP_COMMON_VER_ID_OFFSET
#define FX1_ISP_COMMON_VER_ID_RESET 0x00010007U
#define FX1_ISP_COMMON_VER_ID_ACCESS FX1_ISP_ACCESS_RO
#define FX1_ISP_COMMON_VER_ID_RO_MASK 0xFFFFFFFFU
#define FX1_ISP_COMMON_VER_ID_RW_MASK 0x00000000U
#define FX1_ISP_COMMON_VER_ID_W1C_MASK 0x00000000U
#define FX1_ISP_COMMON_VER_ID_W1S_MASK 0x00000000U
#define FX1_ISP_COMMON_VER_ID_W1P_MASK 0x00000000U
#define FX1_ISP_COMMON_VER_ID_W1SC_MASK 0x00000000U
/**
 * version [31:0], dòng bảng 7, kiểu truy cập RO
 * ABI pipeline 0x00010007; không có thanh ghi phiên bản riêng cho từng khối.
 */
#define FX1_ISP_COMMON_VER_ID_VERSION_SHIFT 0U
#define FX1_ISP_COMMON_VER_ID_VERSION_WIDTH 32U
#define FX1_ISP_COMMON_VER_ID_VERSION_MASK 0xFFFFFFFFU
#define FX1_ISP_COMMON_VER_ID_VERSION_RESET 0x00010007U
#define FX1_ISP_COMMON_VER_ID_VERSION_ACCESS FX1_ISP_ACCESS_RO

/**
 * COMMON_CTRL: địa chỉ lệch 0x0008, dòng bảng 8, kiểu truy cập Mixed
 * COMMON: điều khiển toàn pipeline. ISP_EN cho phép vận hành; khối tùy chọn còn có bit bật riêng.
 */
#define FX1_ISP_COMMON_CTRL_OFFSET 0x0008U
#define REG_COMMON_CTRL FX1_ISP_COMMON_CTRL_OFFSET
#define FX1_ISP_COMMON_CTRL_RESET 0x00000000U
#define FX1_ISP_COMMON_CTRL_ACCESS FX1_ISP_ACCESS_MIXED
#define FX1_ISP_COMMON_CTRL_RO_MASK 0x00000000U
#define FX1_ISP_COMMON_CTRL_RW_MASK 0x00000001U
#define FX1_ISP_COMMON_CTRL_W1C_MASK 0x00000000U
#define FX1_ISP_COMMON_CTRL_W1S_MASK 0x00000004U
#define FX1_ISP_COMMON_CTRL_W1P_MASK 0x00000000U
#define FX1_ISP_COMMON_CTRL_W1SC_MASK 0x00000002U
/**
 * isp_en [0:0], dòng bảng 9, kiểu truy cập RW
 * ISP_EN=1 cho phép pipeline hoạt động. Cấu hình hình học/tuning trước, bật ISP để hình học đầu ra
 * hợp lệ, rồi mới cấp quyền/bật DMA.
 */
#define FX1_ISP_COMMON_CTRL_ISP_EN_SHIFT 0U
#define FX1_ISP_COMMON_CTRL_ISP_EN_WIDTH 1U
#define FX1_ISP_COMMON_CTRL_ISP_EN_MASK 0x00000001U
#define FX1_ISP_COMMON_CTRL_ISP_EN_RESET 0x00000000U
#define FX1_ISP_COMMON_CTRL_ISP_EN_ACCESS FX1_ISP_ACCESS_RW
/**
 * soft_rst [1:1], dòng bảng 10, kiểu truy cập W1SC
 * Yêu cầu khôi phục nhóm: ngừng tiếp nhận, xử lý hết bus/nội bộ rồi phát xung reset 15 chu kỳ. Thăm
 * dò COMMON_RECOVERY_STATUS.BUSY/DONE; sao chép bản ghi hủy trước khi chạy lại. Yêu cầu khi đang bận
 * bị từ chối bằng TUNE_ERROR.PENDING.
 */
#define FX1_ISP_COMMON_CTRL_SOFT_RST_SHIFT 1U
#define FX1_ISP_COMMON_CTRL_SOFT_RST_WIDTH 1U
#define FX1_ISP_COMMON_CTRL_SOFT_RST_MASK 0x00000002U
#define FX1_ISP_COMMON_CTRL_SOFT_RST_RESET 0x00000000U
#define FX1_ISP_COMMON_CTRL_SOFT_RST_ACCESS FX1_ISP_ACCESS_W1SC
/**
 * frame_start [2:2], dòng bảng 11, kiểu truy cập W1S
 * Lệnh kích frame phần mềm chỉ giữ tương thích: ghi 1 được chấp nhận, tự xóa nhưng không tác động
 * IDMA, tiếp nhận hay trạng thái khác. IDMA bắt đầu khi bộ đệm đầu hàng hợp lệ; chạy một frame thì
 * chỉ đánh dấu đúng một bộ đệm đầu vào VALID.
 */
#define FX1_ISP_COMMON_CTRL_FRAME_START_SHIFT 2U
#define FX1_ISP_COMMON_CTRL_FRAME_START_WIDTH 1U
#define FX1_ISP_COMMON_CTRL_FRAME_START_MASK 0x00000004U
#define FX1_ISP_COMMON_CTRL_FRAME_START_RESET 0x00000000U
#define FX1_ISP_COMMON_CTRL_FRAME_START_ACCESS FX1_ISP_ACCESS_W1S

/**
 * COMMON_STATUS: địa chỉ lệch 0x000C, dòng bảng 12, kiểu truy cập RO
 * COMMON: trạng thái toàn pipeline, chỉ đọc.
 */
#define FX1_ISP_COMMON_STATUS_OFFSET 0x000CU
#define REG_COMMON_STATUS FX1_ISP_COMMON_STATUS_OFFSET
#define FX1_ISP_COMMON_STATUS_RESET 0x00000000U
#define FX1_ISP_COMMON_STATUS_ACCESS FX1_ISP_ACCESS_RO
#define FX1_ISP_COMMON_STATUS_RO_MASK 0x00000007U
#define FX1_ISP_COMMON_STATUS_RW_MASK 0x00000000U
#define FX1_ISP_COMMON_STATUS_W1C_MASK 0x00000000U
#define FX1_ISP_COMMON_STATUS_W1S_MASK 0x00000000U
#define FX1_ISP_COMMON_STATUS_W1P_MASK 0x00000000U
#define FX1_ISP_COMMON_STATUS_W1SC_MASK 0x00000000U
/**
 * busy [0:0], dòng bảng 13, kiểu truy cập RO
 * BUSY=1 khi còn frame đang xử lý ở bất kỳ vị trí nào trong pipeline.
 */
#define FX1_ISP_COMMON_STATUS_BUSY_SHIFT 0U
#define FX1_ISP_COMMON_STATUS_BUSY_WIDTH 1U
#define FX1_ISP_COMMON_STATUS_BUSY_MASK 0x00000001U
#define FX1_ISP_COMMON_STATUS_BUSY_RESET 0x00000000U
#define FX1_ISP_COMMON_STATUS_BUSY_ACCESS FX1_ISP_ACCESS_RO
/**
 * frame_done [1:1], dòng bảng 14, kiểu truy cập RO
 * Frame được nhận gần nhất đã kết thúc ở mọi đầu ra bật, thành công hoặc qua recovery. Xóa tại SOF
 * đầu vào được nhận kế tiếp và khi reset.
 */
#define FX1_ISP_COMMON_STATUS_FRAME_DONE_SHIFT 1U
#define FX1_ISP_COMMON_STATUS_FRAME_DONE_WIDTH 1U
#define FX1_ISP_COMMON_STATUS_FRAME_DONE_MASK 0x00000002U
#define FX1_ISP_COMMON_STATUS_FRAME_DONE_RESET 0x00000000U
#define FX1_ISP_COMMON_STATUS_FRAME_DONE_ACCESS FX1_ISP_ACCESS_RO
/**
 * error [2:2], dòng bảng 15, kiểu truy cập RO
 * Tổng OR trực tiếp của DMA_ERR ALIGN/AXI_RD/AXI_WR, LSC_ERROR, EE_STATUS.ERROR và TUNE_ERROR; không
 * tính điều tiết luồng hoặc mất thống kê. Đọc các thanh ghi đó để biết nguyên nhân.
 */
#define FX1_ISP_COMMON_STATUS_ERROR_SHIFT 2U
#define FX1_ISP_COMMON_STATUS_ERROR_WIDTH 1U
#define FX1_ISP_COMMON_STATUS_ERROR_MASK 0x00000004U
#define FX1_ISP_COMMON_STATUS_ERROR_RESET 0x00000000U
#define FX1_ISP_COMMON_STATUS_ERROR_ACCESS FX1_ISP_ACCESS_RO

/**
 * COMMON_IRQ_STATUS: địa chỉ lệch 0x0010, dòng bảng 16, kiểu truy cập W1C
 * COMMON: trạng thái ngắt được chốt; ghi 1 để xóa bit. Sự kiện vẫn đặt bit dù COMMON_IRQ_EN tắt; mặt
 * nạ chỉ điều khiển chân ngắt.
 */
#define FX1_ISP_COMMON_IRQ_STATUS_OFFSET 0x0010U
#define REG_COMMON_IRQ_STATUS FX1_ISP_COMMON_IRQ_STATUS_OFFSET
#define FX1_ISP_COMMON_IRQ_STATUS_RESET 0x00000000U
#define FX1_ISP_COMMON_IRQ_STATUS_ACCESS FX1_ISP_ACCESS_W1C
#define FX1_ISP_COMMON_IRQ_STATUS_RO_MASK 0x00000000U
#define FX1_ISP_COMMON_IRQ_STATUS_RW_MASK 0x00000000U
#define FX1_ISP_COMMON_IRQ_STATUS_W1C_MASK 0x00000007U
#define FX1_ISP_COMMON_IRQ_STATUS_W1S_MASK 0x00000000U
#define FX1_ISP_COMMON_IRQ_STATUS_W1P_MASK 0x00000000U
#define FX1_ISP_COMMON_IRQ_STATUS_W1SC_MASK 0x00000000U
/**
 * frame_done_irq [0:0], dòng bảng 17, kiểu truy cập W1C
 * Ngắt frame hoàn tất: đặt khi COMMON_STATUS.FRAME_DONE tăng; ghi 1 để xóa.
 */
#define FX1_ISP_COMMON_IRQ_STATUS_FRAME_DONE_IRQ_SHIFT 0U
#define FX1_ISP_COMMON_IRQ_STATUS_FRAME_DONE_IRQ_WIDTH 1U
#define FX1_ISP_COMMON_IRQ_STATUS_FRAME_DONE_IRQ_MASK 0x00000001U
#define FX1_ISP_COMMON_IRQ_STATUS_FRAME_DONE_IRQ_RESET 0x00000000U
#define FX1_ISP_COMMON_IRQ_STATUS_FRAME_DONE_IRQ_ACCESS FX1_ISP_ACCESS_W1C
/**
 * error_irq [1:1], dòng bảng 18, kiểu truy cập W1C
 * Ngắt lỗi: đặt khi các lỗi ALIGN/AXI_RD/AXI_WR, LSC_ERROR, EE_STATUS.ERROR hoặc TUNE_ERROR chuyển 0
 * sang 1, và mỗi recovery được nhận. Không đặt do IDMA_UNDERRUN, ODMA_OVERFLOW hoặc mất thống kê;
 * ghi 1 để xóa.
 */
#define FX1_ISP_COMMON_IRQ_STATUS_ERROR_IRQ_SHIFT 1U
#define FX1_ISP_COMMON_IRQ_STATUS_ERROR_IRQ_WIDTH 1U
#define FX1_ISP_COMMON_IRQ_STATUS_ERROR_IRQ_MASK 0x00000002U
#define FX1_ISP_COMMON_IRQ_STATUS_ERROR_IRQ_RESET 0x00000000U
#define FX1_ISP_COMMON_IRQ_STATUS_ERROR_IRQ_ACCESS FX1_ISP_ACCESS_W1C
/**
 * stats_ready_irq [2:2], dòng bảng 19, kiểu truy cập W1C
 * Ngắt thống kê sẵn sàng: đặt khi một khối công bố kết quả frame hoàn chỉnh; ghi 1 để xóa.
 */
#define FX1_ISP_COMMON_IRQ_STATUS_STATS_READY_IRQ_SHIFT 2U
#define FX1_ISP_COMMON_IRQ_STATUS_STATS_READY_IRQ_WIDTH 1U
#define FX1_ISP_COMMON_IRQ_STATUS_STATS_READY_IRQ_MASK 0x00000004U
#define FX1_ISP_COMMON_IRQ_STATUS_STATS_READY_IRQ_RESET 0x00000000U
#define FX1_ISP_COMMON_IRQ_STATUS_STATS_READY_IRQ_ACCESS FX1_ISP_ACCESS_W1C

/**
 * COMMON_IRQ_EN: địa chỉ lệch 0x0014, dòng bảng 20, kiểu truy cập RW
 * COMMON: mặt nạ cho phép từng bit COMMON_IRQ_STATUS tác động chân ngắt; không ngăn bit trạng thái
 * được đặt.
 */
#define FX1_ISP_COMMON_IRQ_EN_OFFSET 0x0014U
#define REG_COMMON_IRQ_EN FX1_ISP_COMMON_IRQ_EN_OFFSET
#define FX1_ISP_COMMON_IRQ_EN_RESET 0x00000000U
#define FX1_ISP_COMMON_IRQ_EN_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_COMMON_IRQ_EN_RO_MASK 0x00000000U
#define FX1_ISP_COMMON_IRQ_EN_RW_MASK 0x00000007U
#define FX1_ISP_COMMON_IRQ_EN_W1C_MASK 0x00000000U
#define FX1_ISP_COMMON_IRQ_EN_W1S_MASK 0x00000000U
#define FX1_ISP_COMMON_IRQ_EN_W1P_MASK 0x00000000U
#define FX1_ISP_COMMON_IRQ_EN_W1SC_MASK 0x00000000U
/**
 * frame_done_en [0:0], dòng bảng 21, kiểu truy cập RW
 * Cho phép sự kiện frame hoàn tất tác động chân ngắt.
 */
#define FX1_ISP_COMMON_IRQ_EN_FRAME_DONE_EN_SHIFT 0U
#define FX1_ISP_COMMON_IRQ_EN_FRAME_DONE_EN_WIDTH 1U
#define FX1_ISP_COMMON_IRQ_EN_FRAME_DONE_EN_MASK 0x00000001U
#define FX1_ISP_COMMON_IRQ_EN_FRAME_DONE_EN_RESET 0x00000000U
#define FX1_ISP_COMMON_IRQ_EN_FRAME_DONE_EN_ACCESS FX1_ISP_ACCESS_RW
/**
 * error_en [1:1], dòng bảng 22, kiểu truy cập RW
 * Cho phép sự kiện lỗi tác động chân ngắt.
 */
#define FX1_ISP_COMMON_IRQ_EN_ERROR_EN_SHIFT 1U
#define FX1_ISP_COMMON_IRQ_EN_ERROR_EN_WIDTH 1U
#define FX1_ISP_COMMON_IRQ_EN_ERROR_EN_MASK 0x00000002U
#define FX1_ISP_COMMON_IRQ_EN_ERROR_EN_RESET 0x00000000U
#define FX1_ISP_COMMON_IRQ_EN_ERROR_EN_ACCESS FX1_ISP_ACCESS_RW
/**
 * stats_ready_en [2:2], dòng bảng 23, kiểu truy cập RW
 * Cho phép sự kiện thống kê sẵn sàng tác động chân ngắt.
 */
#define FX1_ISP_COMMON_IRQ_EN_STATS_READY_EN_SHIFT 2U
#define FX1_ISP_COMMON_IRQ_EN_STATS_READY_EN_WIDTH 1U
#define FX1_ISP_COMMON_IRQ_EN_STATS_READY_EN_MASK 0x00000004U
#define FX1_ISP_COMMON_IRQ_EN_STATS_READY_EN_RESET 0x00000000U
#define FX1_ISP_COMMON_IRQ_EN_STATS_READY_EN_ACCESS FX1_ISP_ACCESS_RW

/**
 * COMMON_FRAME_WIDTH: địa chỉ lệch 0x0018, dòng bảng 24, kiểu truy cập RW
 * Chiều rộng ảnh ngoài trước chuẩn hóa pha Bayer. IFMT có thể bỏ cột đầu rồi làm tròn xuống chẵn.
 * Đổi miền đầu vào chỉ khi legacy đã dừng/xử lý hết dữ liệu; profile bị từ chối với MODE và giữ
 * nguyên trạng thái. Thay đổi được nhận làm mất hiệu lực các bộ LSC/EE và hình học đầu ra phụ thuộc.
 */
#define FX1_ISP_COMMON_FRAME_WIDTH_OFFSET 0x0018U
#define REG_COMMON_FRAME_WIDTH FX1_ISP_COMMON_FRAME_WIDTH_OFFSET
#define FX1_ISP_COMMON_FRAME_WIDTH_RESET 0x00000780U
#define FX1_ISP_COMMON_FRAME_WIDTH_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_COMMON_FRAME_WIDTH_RO_MASK 0x00000000U
#define FX1_ISP_COMMON_FRAME_WIDTH_RW_MASK 0x0000FFFFU
#define FX1_ISP_COMMON_FRAME_WIDTH_W1C_MASK 0x00000000U
#define FX1_ISP_COMMON_FRAME_WIDTH_W1S_MASK 0x00000000U
#define FX1_ISP_COMMON_FRAME_WIDTH_W1P_MASK 0x00000000U
#define FX1_ISP_COMMON_FRAME_WIDTH_W1SC_MASK 0x00000000U
/**
 * h_active [15:0], dòng bảng 25, kiểu truy cập RW
 * Số mẫu ngoài mỗi dòng, số chẵn 32..3842. Chiều rộng hữu dụng W=(Win-SH)&~1 phải trong 32..3840.
 * IDMA đọc chiều rộng ngoài; tọa độ phía sau dùng W hữu dụng.
 */
#define FX1_ISP_COMMON_FRAME_WIDTH_H_ACTIVE_SHIFT 0U
#define FX1_ISP_COMMON_FRAME_WIDTH_H_ACTIVE_WIDTH 16U
#define FX1_ISP_COMMON_FRAME_WIDTH_H_ACTIVE_MASK 0x0000FFFFU
#define FX1_ISP_COMMON_FRAME_WIDTH_H_ACTIVE_RESET 0x00000780U
#define FX1_ISP_COMMON_FRAME_WIDTH_H_ACTIVE_ACCESS FX1_ISP_ACCESS_RW

/**
 * COMMON_FRAME_HEIGHT: địa chỉ lệch 0x001C, dòng bảng 26, kiểu truy cập RW
 * Chiều cao ảnh ngoài trước chuẩn hóa pha Bayer. IFMT có thể bỏ hàng đầu rồi làm tròn xuống chẵn;
 * IDMA yêu cầu chiều cao ngoài chẵn. Đổi đầu vào cần legacy đã dừng/xử lý hết; profile từ chối MODE,
 * giữ trạng thái. Thay đổi làm mất hiệu lực các bộ LSC/EE và hình học đầu ra phụ thuộc.
 */
#define FX1_ISP_COMMON_FRAME_HEIGHT_OFFSET 0x001CU
#define REG_COMMON_FRAME_HEIGHT FX1_ISP_COMMON_FRAME_HEIGHT_OFFSET
#define FX1_ISP_COMMON_FRAME_HEIGHT_RESET 0x00000438U
#define FX1_ISP_COMMON_FRAME_HEIGHT_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_COMMON_FRAME_HEIGHT_RO_MASK 0x00000000U
#define FX1_ISP_COMMON_FRAME_HEIGHT_RW_MASK 0x0000FFFFU
#define FX1_ISP_COMMON_FRAME_HEIGHT_W1C_MASK 0x00000000U
#define FX1_ISP_COMMON_FRAME_HEIGHT_W1S_MASK 0x00000000U
#define FX1_ISP_COMMON_FRAME_HEIGHT_W1P_MASK 0x00000000U
#define FX1_ISP_COMMON_FRAME_HEIGHT_W1SC_MASK 0x00000000U
/**
 * v_active [15:0], dòng bảng 27, kiểu truy cập RW
 * Số dòng ngoài mỗi frame, số chẵn 32..2162. Hữu dụng H=(Hin-SV)&~1 phải trong 32..2160. IDMA đọc
 * chiều cao ngoài; phía sau dùng H hữu dụng.
 */
#define FX1_ISP_COMMON_FRAME_HEIGHT_V_ACTIVE_SHIFT 0U
#define FX1_ISP_COMMON_FRAME_HEIGHT_V_ACTIVE_WIDTH 16U
#define FX1_ISP_COMMON_FRAME_HEIGHT_V_ACTIVE_MASK 0x0000FFFFU
#define FX1_ISP_COMMON_FRAME_HEIGHT_V_ACTIVE_RESET 0x00000438U
#define FX1_ISP_COMMON_FRAME_HEIGHT_V_ACTIVE_ACCESS FX1_ISP_ACCESS_RW

/**
 * COMMON_BAYER: địa chỉ lệch 0x0020, dòng bảng 28, kiểu truy cập RW
 * Pha Bayer đầu vào. IFMT chuẩn hóa RGGB bằng bỏ cột/hàng đầu nếu cần rồi làm tròn mỗi chiều xuống
 * chẵn. Đổi đầu vào cần legacy đã dừng/xử lý hết; profile từ chối MODE. Thay đổi làm mất hiệu lực
 * các bộ LSC/EE và hình học đầu ra phụ thuộc.
 */
#define FX1_ISP_COMMON_BAYER_OFFSET 0x0020U
#define REG_COMMON_BAYER FX1_ISP_COMMON_BAYER_OFFSET
#define FX1_ISP_COMMON_BAYER_RESET 0x00000000U
#define FX1_ISP_COMMON_BAYER_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_COMMON_BAYER_RO_MASK 0x00000000U
#define FX1_ISP_COMMON_BAYER_RW_MASK 0x00000003U
#define FX1_ISP_COMMON_BAYER_W1C_MASK 0x00000000U
#define FX1_ISP_COMMON_BAYER_W1S_MASK 0x00000000U
#define FX1_ISP_COMMON_BAYER_W1P_MASK 0x00000000U
#define FX1_ISP_COMMON_BAYER_W1SC_MASK 0x00000000U
/**
 * pattern [1:0], dòng bảng 29, kiểu truy cập RW
 * Mẫu lọc màu Bayer: bit [1:0] là [dịch dọc, dịch ngang]. 0=RGGB, 1=GRBG, 2=GBRG, 3=BGGR.
 */
#define FX1_ISP_COMMON_BAYER_PATTERN_SHIFT 0U
#define FX1_ISP_COMMON_BAYER_PATTERN_WIDTH 2U
#define FX1_ISP_COMMON_BAYER_PATTERN_MASK 0x00000003U
#define FX1_ISP_COMMON_BAYER_PATTERN_RESET 0x00000000U
#define FX1_ISP_COMMON_BAYER_PATTERN_ACCESS FX1_ISP_ACCESS_RW

/**
 * COMMON_SCRATCH: địa chỉ lệch 0x0028, dòng bảng 30, kiểu truy cập RW
 * Thanh ghi nháp RW 32 bit, không tác động phần cứng; dùng kiểm tra bus thanh ghi trước khi tin các
 * giá trị đọc khác.
 */
#define FX1_ISP_COMMON_SCRATCH_OFFSET 0x0028U
#define REG_COMMON_SCRATCH FX1_ISP_COMMON_SCRATCH_OFFSET
#define FX1_ISP_COMMON_SCRATCH_RESET 0x00000000U
#define FX1_ISP_COMMON_SCRATCH_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_COMMON_SCRATCH_RO_MASK 0x00000000U
#define FX1_ISP_COMMON_SCRATCH_RW_MASK 0xFFFFFFFFU
#define FX1_ISP_COMMON_SCRATCH_W1C_MASK 0x00000000U
#define FX1_ISP_COMMON_SCRATCH_W1S_MASK 0x00000000U
#define FX1_ISP_COMMON_SCRATCH_W1P_MASK 0x00000000U
#define FX1_ISP_COMMON_SCRATCH_W1SC_MASK 0x00000000U
/**
 * scratch [31:0], dòng bảng 31, kiểu truy cập RW
 * Đọc lại đúng giá trị đã ghi vào thanh ghi nháp.
 */
#define FX1_ISP_COMMON_SCRATCH_SCRATCH_SHIFT 0U
#define FX1_ISP_COMMON_SCRATCH_SCRATCH_WIDTH 32U
#define FX1_ISP_COMMON_SCRATCH_SCRATCH_MASK 0xFFFFFFFFU
#define FX1_ISP_COMMON_SCRATCH_SCRATCH_RESET 0x00000000U
#define FX1_ISP_COMMON_SCRATCH_SCRATCH_ACCESS FX1_ISP_ACCESS_RW

/**
 * BLC_CTRL: địa chỉ lệch 0x0200, dòng bảng 32, kiểu truy cập RW
 * BLC: trừ mức đen nền cảm biến rồi có thể nhân giãn dải còn lại để khôi phục cực đại.
 */
#define FX1_ISP_BLC_CTRL_OFFSET 0x0200U
#define REG_BLC_CTRL FX1_ISP_BLC_CTRL_OFFSET
#define FX1_ISP_BLC_CTRL_RESET 0x00000000U
#define FX1_ISP_BLC_CTRL_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_BLC_CTRL_RO_MASK 0x00000000U
#define FX1_ISP_BLC_CTRL_RW_MASK 0x00000001U
#define FX1_ISP_BLC_CTRL_W1C_MASK 0x00000000U
#define FX1_ISP_BLC_CTRL_W1S_MASK 0x00000000U
#define FX1_ISP_BLC_CTRL_W1P_MASK 0x00000000U
#define FX1_ISP_BLC_CTRL_W1SC_MASK 0x00000000U
/**
 * en [0:0], dòng bảng 33, kiểu truy cập RW
 * Bật BLC: 1 trừ mức đen, 0 truyền nguyên giá trị.
 */
#define FX1_ISP_BLC_CTRL_EN_SHIFT 0U
#define FX1_ISP_BLC_CTRL_EN_WIDTH 1U
#define FX1_ISP_BLC_CTRL_EN_MASK 0x00000001U
#define FX1_ISP_BLC_CTRL_EN_RESET 0x00000000U
#define FX1_ISP_BLC_CTRL_EN_ACCESS FX1_ISP_ACCESS_RW

/**
 * BLC_GAIN_SEL: địa chỉ lệch 0x0204, dòng bảng 34, kiểu truy cập RW
 * Chọn một trong bốn bộ offset BLC. Mức đen thay đổi theo analog gain của cảm biến; giữ sẵn bốn bộ
 * để chọn thay vì ghi lại mỗi frame.
 */
#define FX1_ISP_BLC_GAIN_SEL_OFFSET 0x0204U
#define REG_BLC_GAIN_SEL FX1_ISP_BLC_GAIN_SEL_OFFSET
#define FX1_ISP_BLC_GAIN_SEL_RESET 0x00000100U
#define FX1_ISP_BLC_GAIN_SEL_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_BLC_GAIN_SEL_RO_MASK 0x00000000U
#define FX1_ISP_BLC_GAIN_SEL_RW_MASK 0x00000103U
#define FX1_ISP_BLC_GAIN_SEL_W1C_MASK 0x00000000U
#define FX1_ISP_BLC_GAIN_SEL_W1S_MASK 0x00000000U
#define FX1_ISP_BLC_GAIN_SEL_W1P_MASK 0x00000000U
#define FX1_ISP_BLC_GAIN_SEL_W1SC_MASK 0x00000000U
/**
 * gain_level [1:0], dòng bảng 35, kiểu truy cập RW
 * Chọn BLC_OFS_Gn_* và BLC_SCALE_Gn: mã 0,1,2,3 tương ứng G0,G1,G2,G3.
 */
#define FX1_ISP_BLC_GAIN_SEL_GAIN_LEVEL_SHIFT 0U
#define FX1_ISP_BLC_GAIN_SEL_GAIN_LEVEL_WIDTH 2U
#define FX1_ISP_BLC_GAIN_SEL_GAIN_LEVEL_MASK 0x00000003U
#define FX1_ISP_BLC_GAIN_SEL_GAIN_LEVEL_RESET 0x00000000U
#define FX1_ISP_BLC_GAIN_SEL_GAIN_LEVEL_ACCESS FX1_ISP_ACCESS_RW
/**
 * range_scale_en [8:8], dòng bảng 36, kiểu truy cập RW
 * Bật giãn dải: 1 nhân BLC_SCALE_Gn sau trừ mức đen, 0 dùng chính xác hệ số 1x. Sau reset là 1; được
 * chốt theo frame.
 */
#define FX1_ISP_BLC_GAIN_SEL_RANGE_SCALE_EN_SHIFT 8U
#define FX1_ISP_BLC_GAIN_SEL_RANGE_SCALE_EN_WIDTH 1U
#define FX1_ISP_BLC_GAIN_SEL_RANGE_SCALE_EN_MASK 0x00000100U
#define FX1_ISP_BLC_GAIN_SEL_RANGE_SCALE_EN_RESET 0x00000001U
#define FX1_ISP_BLC_GAIN_SEL_RANGE_SCALE_EN_ACCESS FX1_ISP_ACCESS_RW

/**
 * BLC_OFS_G0_DFT: địa chỉ lệch 0x0208, dòng bảng 37, kiểu truy cập RW
 * BLC G0: mức đen dùng chung trừ ở mọi màu; offset từng màu được cộng thêm để hiệu chỉnh.
 */
#define FX1_ISP_BLC_OFS_G0_DFT_OFFSET 0x0208U
#define REG_BLC_OFS_G0_DFT FX1_ISP_BLC_OFS_G0_DFT_OFFSET
#define FX1_ISP_BLC_OFS_G0_DFT_RESET 0x000000C8U
#define FX1_ISP_BLC_OFS_G0_DFT_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_BLC_OFS_G0_DFT_RO_MASK 0x00000000U
#define FX1_ISP_BLC_OFS_G0_DFT_RW_MASK 0x00000FFFU
#define FX1_ISP_BLC_OFS_G0_DFT_W1C_MASK 0x00000000U
#define FX1_ISP_BLC_OFS_G0_DFT_W1S_MASK 0x00000000U
#define FX1_ISP_BLC_OFS_G0_DFT_W1P_MASK 0x00000000U
#define FX1_ISP_BLC_OFS_G0_DFT_W1SC_MASK 0x00000000U
/**
 * ofs_g0_dft [11:0], dòng bảng 38, kiểu truy cập RW
 * Mức đen/hiệu chỉnh của bộ G0, mã Bayer không dấu 12 bit 0..4095. Cộng mức đen chung và offset màu
 * chính xác ở 13 bit trước khi trừ; không cuộn vòng/kẹp tổng, phần dư kẹp về 0. Sau reset là 200.
 */
#define FX1_ISP_BLC_OFS_G0_DFT_OFS_G0_DFT_SHIFT 0U
#define FX1_ISP_BLC_OFS_G0_DFT_OFS_G0_DFT_WIDTH 12U
#define FX1_ISP_BLC_OFS_G0_DFT_OFS_G0_DFT_MASK 0x00000FFFU
#define FX1_ISP_BLC_OFS_G0_DFT_OFS_G0_DFT_RESET 0x000000C8U
#define FX1_ISP_BLC_OFS_G0_DFT_OFS_G0_DFT_ACCESS FX1_ISP_ACCESS_RW

/**
 * BLC_OFS_G0_R: địa chỉ lệch 0x020C, dòng bảng 39, kiểu truy cập RW
 * BLC G0: hiệu chỉnh mức đen màu đỏ.
 */
#define FX1_ISP_BLC_OFS_G0_R_OFFSET 0x020CU
#define REG_BLC_OFS_G0_R FX1_ISP_BLC_OFS_G0_R_OFFSET
#define FX1_ISP_BLC_OFS_G0_R_RESET 0x00000000U
#define FX1_ISP_BLC_OFS_G0_R_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_BLC_OFS_G0_R_RO_MASK 0x00000000U
#define FX1_ISP_BLC_OFS_G0_R_RW_MASK 0x00000FFFU
#define FX1_ISP_BLC_OFS_G0_R_W1C_MASK 0x00000000U
#define FX1_ISP_BLC_OFS_G0_R_W1S_MASK 0x00000000U
#define FX1_ISP_BLC_OFS_G0_R_W1P_MASK 0x00000000U
#define FX1_ISP_BLC_OFS_G0_R_W1SC_MASK 0x00000000U
/**
 * ofs_g0_r [11:0], dòng bảng 40, kiểu truy cập RW
 * Mức đen/hiệu chỉnh của bộ G0, mã Bayer không dấu 12 bit 0..4095. Cộng mức đen chung và offset màu
 * chính xác ở 13 bit trước khi trừ; không cuộn vòng/kẹp tổng, phần dư kẹp về 0. Sau reset là 0.
 */
#define FX1_ISP_BLC_OFS_G0_R_OFS_G0_R_SHIFT 0U
#define FX1_ISP_BLC_OFS_G0_R_OFS_G0_R_WIDTH 12U
#define FX1_ISP_BLC_OFS_G0_R_OFS_G0_R_MASK 0x00000FFFU
#define FX1_ISP_BLC_OFS_G0_R_OFS_G0_R_RESET 0x00000000U
#define FX1_ISP_BLC_OFS_G0_R_OFS_G0_R_ACCESS FX1_ISP_ACCESS_RW

/**
 * BLC_OFS_G0_GR: địa chỉ lệch 0x0210, dòng bảng 41, kiểu truy cập RW
 * BLC G0: hiệu chỉnh mức đen màu Gr (xanh lá trên hàng đỏ).
 */
#define FX1_ISP_BLC_OFS_G0_GR_OFFSET 0x0210U
#define REG_BLC_OFS_G0_GR FX1_ISP_BLC_OFS_G0_GR_OFFSET
#define FX1_ISP_BLC_OFS_G0_GR_RESET 0x00000000U
#define FX1_ISP_BLC_OFS_G0_GR_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_BLC_OFS_G0_GR_RO_MASK 0x00000000U
#define FX1_ISP_BLC_OFS_G0_GR_RW_MASK 0x00000FFFU
#define FX1_ISP_BLC_OFS_G0_GR_W1C_MASK 0x00000000U
#define FX1_ISP_BLC_OFS_G0_GR_W1S_MASK 0x00000000U
#define FX1_ISP_BLC_OFS_G0_GR_W1P_MASK 0x00000000U
#define FX1_ISP_BLC_OFS_G0_GR_W1SC_MASK 0x00000000U
/**
 * ofs_g0_gr [11:0], dòng bảng 42, kiểu truy cập RW
 * Mức đen/hiệu chỉnh của bộ G0, mã Bayer không dấu 12 bit 0..4095. Cộng mức đen chung và offset màu
 * chính xác ở 13 bit trước khi trừ; không cuộn vòng/kẹp tổng, phần dư kẹp về 0. Sau reset là 0.
 */
#define FX1_ISP_BLC_OFS_G0_GR_OFS_G0_GR_SHIFT 0U
#define FX1_ISP_BLC_OFS_G0_GR_OFS_G0_GR_WIDTH 12U
#define FX1_ISP_BLC_OFS_G0_GR_OFS_G0_GR_MASK 0x00000FFFU
#define FX1_ISP_BLC_OFS_G0_GR_OFS_G0_GR_RESET 0x00000000U
#define FX1_ISP_BLC_OFS_G0_GR_OFS_G0_GR_ACCESS FX1_ISP_ACCESS_RW

/**
 * BLC_OFS_G0_GB: địa chỉ lệch 0x0214, dòng bảng 43, kiểu truy cập RW
 * BLC G0: hiệu chỉnh mức đen màu Gb (xanh lá trên hàng xanh dương).
 */
#define FX1_ISP_BLC_OFS_G0_GB_OFFSET 0x0214U
#define REG_BLC_OFS_G0_GB FX1_ISP_BLC_OFS_G0_GB_OFFSET
#define FX1_ISP_BLC_OFS_G0_GB_RESET 0x00000000U
#define FX1_ISP_BLC_OFS_G0_GB_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_BLC_OFS_G0_GB_RO_MASK 0x00000000U
#define FX1_ISP_BLC_OFS_G0_GB_RW_MASK 0x00000FFFU
#define FX1_ISP_BLC_OFS_G0_GB_W1C_MASK 0x00000000U
#define FX1_ISP_BLC_OFS_G0_GB_W1S_MASK 0x00000000U
#define FX1_ISP_BLC_OFS_G0_GB_W1P_MASK 0x00000000U
#define FX1_ISP_BLC_OFS_G0_GB_W1SC_MASK 0x00000000U
/**
 * ofs_g0_gb [11:0], dòng bảng 44, kiểu truy cập RW
 * Mức đen/hiệu chỉnh của bộ G0, mã Bayer không dấu 12 bit 0..4095. Cộng mức đen chung và offset màu
 * chính xác ở 13 bit trước khi trừ; không cuộn vòng/kẹp tổng, phần dư kẹp về 0. Sau reset là 0.
 */
#define FX1_ISP_BLC_OFS_G0_GB_OFS_G0_GB_SHIFT 0U
#define FX1_ISP_BLC_OFS_G0_GB_OFS_G0_GB_WIDTH 12U
#define FX1_ISP_BLC_OFS_G0_GB_OFS_G0_GB_MASK 0x00000FFFU
#define FX1_ISP_BLC_OFS_G0_GB_OFS_G0_GB_RESET 0x00000000U
#define FX1_ISP_BLC_OFS_G0_GB_OFS_G0_GB_ACCESS FX1_ISP_ACCESS_RW

/**
 * BLC_OFS_G0_B: địa chỉ lệch 0x0218, dòng bảng 45, kiểu truy cập RW
 * BLC G0: hiệu chỉnh mức đen màu xanh dương.
 */
#define FX1_ISP_BLC_OFS_G0_B_OFFSET 0x0218U
#define REG_BLC_OFS_G0_B FX1_ISP_BLC_OFS_G0_B_OFFSET
#define FX1_ISP_BLC_OFS_G0_B_RESET 0x00000000U
#define FX1_ISP_BLC_OFS_G0_B_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_BLC_OFS_G0_B_RO_MASK 0x00000000U
#define FX1_ISP_BLC_OFS_G0_B_RW_MASK 0x00000FFFU
#define FX1_ISP_BLC_OFS_G0_B_W1C_MASK 0x00000000U
#define FX1_ISP_BLC_OFS_G0_B_W1S_MASK 0x00000000U
#define FX1_ISP_BLC_OFS_G0_B_W1P_MASK 0x00000000U
#define FX1_ISP_BLC_OFS_G0_B_W1SC_MASK 0x00000000U
/**
 * ofs_g0_b [11:0], dòng bảng 46, kiểu truy cập RW
 * Mức đen/hiệu chỉnh của bộ G0, mã Bayer không dấu 12 bit 0..4095. Cộng mức đen chung và offset màu
 * chính xác ở 13 bit trước khi trừ; không cuộn vòng/kẹp tổng, phần dư kẹp về 0. Sau reset là 0.
 */
#define FX1_ISP_BLC_OFS_G0_B_OFS_G0_B_SHIFT 0U
#define FX1_ISP_BLC_OFS_G0_B_OFS_G0_B_WIDTH 12U
#define FX1_ISP_BLC_OFS_G0_B_OFS_G0_B_MASK 0x00000FFFU
#define FX1_ISP_BLC_OFS_G0_B_OFS_G0_B_RESET 0x00000000U
#define FX1_ISP_BLC_OFS_G0_B_OFS_G0_B_ACCESS FX1_ISP_ACCESS_RW

/**
 * BLC_OFS_G1_DFT: địa chỉ lệch 0x021C, dòng bảng 47, kiểu truy cập RW
 * BLC G1: mức đen dùng chung trừ ở mọi màu; offset từng màu được cộng thêm để hiệu chỉnh.
 */
#define FX1_ISP_BLC_OFS_G1_DFT_OFFSET 0x021CU
#define REG_BLC_OFS_G1_DFT FX1_ISP_BLC_OFS_G1_DFT_OFFSET
#define FX1_ISP_BLC_OFS_G1_DFT_RESET 0x000000C8U
#define FX1_ISP_BLC_OFS_G1_DFT_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_BLC_OFS_G1_DFT_RO_MASK 0x00000000U
#define FX1_ISP_BLC_OFS_G1_DFT_RW_MASK 0x00000FFFU
#define FX1_ISP_BLC_OFS_G1_DFT_W1C_MASK 0x00000000U
#define FX1_ISP_BLC_OFS_G1_DFT_W1S_MASK 0x00000000U
#define FX1_ISP_BLC_OFS_G1_DFT_W1P_MASK 0x00000000U
#define FX1_ISP_BLC_OFS_G1_DFT_W1SC_MASK 0x00000000U
/**
 * ofs_g1_dft [11:0], dòng bảng 48, kiểu truy cập RW
 * Mức đen/hiệu chỉnh của bộ G1, mã Bayer không dấu 12 bit 0..4095. Cộng mức đen chung và offset màu
 * chính xác ở 13 bit trước khi trừ; không cuộn vòng/kẹp tổng, phần dư kẹp về 0. Sau reset là 200.
 */
#define FX1_ISP_BLC_OFS_G1_DFT_OFS_G1_DFT_SHIFT 0U
#define FX1_ISP_BLC_OFS_G1_DFT_OFS_G1_DFT_WIDTH 12U
#define FX1_ISP_BLC_OFS_G1_DFT_OFS_G1_DFT_MASK 0x00000FFFU
#define FX1_ISP_BLC_OFS_G1_DFT_OFS_G1_DFT_RESET 0x000000C8U
#define FX1_ISP_BLC_OFS_G1_DFT_OFS_G1_DFT_ACCESS FX1_ISP_ACCESS_RW

/**
 * BLC_OFS_G1_R: địa chỉ lệch 0x0220, dòng bảng 49, kiểu truy cập RW
 * BLC G1: hiệu chỉnh mức đen màu đỏ.
 */
#define FX1_ISP_BLC_OFS_G1_R_OFFSET 0x0220U
#define REG_BLC_OFS_G1_R FX1_ISP_BLC_OFS_G1_R_OFFSET
#define FX1_ISP_BLC_OFS_G1_R_RESET 0x00000000U
#define FX1_ISP_BLC_OFS_G1_R_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_BLC_OFS_G1_R_RO_MASK 0x00000000U
#define FX1_ISP_BLC_OFS_G1_R_RW_MASK 0x00000FFFU
#define FX1_ISP_BLC_OFS_G1_R_W1C_MASK 0x00000000U
#define FX1_ISP_BLC_OFS_G1_R_W1S_MASK 0x00000000U
#define FX1_ISP_BLC_OFS_G1_R_W1P_MASK 0x00000000U
#define FX1_ISP_BLC_OFS_G1_R_W1SC_MASK 0x00000000U
/**
 * ofs_g1_r [11:0], dòng bảng 50, kiểu truy cập RW
 * Mức đen/hiệu chỉnh của bộ G1, mã Bayer không dấu 12 bit 0..4095. Cộng mức đen chung và offset màu
 * chính xác ở 13 bit trước khi trừ; không cuộn vòng/kẹp tổng, phần dư kẹp về 0. Sau reset là 0.
 */
#define FX1_ISP_BLC_OFS_G1_R_OFS_G1_R_SHIFT 0U
#define FX1_ISP_BLC_OFS_G1_R_OFS_G1_R_WIDTH 12U
#define FX1_ISP_BLC_OFS_G1_R_OFS_G1_R_MASK 0x00000FFFU
#define FX1_ISP_BLC_OFS_G1_R_OFS_G1_R_RESET 0x00000000U
#define FX1_ISP_BLC_OFS_G1_R_OFS_G1_R_ACCESS FX1_ISP_ACCESS_RW

/**
 * BLC_OFS_G1_GR: địa chỉ lệch 0x0224, dòng bảng 51, kiểu truy cập RW
 * BLC G1: hiệu chỉnh mức đen màu Gr (xanh lá trên hàng đỏ).
 */
#define FX1_ISP_BLC_OFS_G1_GR_OFFSET 0x0224U
#define REG_BLC_OFS_G1_GR FX1_ISP_BLC_OFS_G1_GR_OFFSET
#define FX1_ISP_BLC_OFS_G1_GR_RESET 0x00000000U
#define FX1_ISP_BLC_OFS_G1_GR_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_BLC_OFS_G1_GR_RO_MASK 0x00000000U
#define FX1_ISP_BLC_OFS_G1_GR_RW_MASK 0x00000FFFU
#define FX1_ISP_BLC_OFS_G1_GR_W1C_MASK 0x00000000U
#define FX1_ISP_BLC_OFS_G1_GR_W1S_MASK 0x00000000U
#define FX1_ISP_BLC_OFS_G1_GR_W1P_MASK 0x00000000U
#define FX1_ISP_BLC_OFS_G1_GR_W1SC_MASK 0x00000000U
/**
 * ofs_g1_gr [11:0], dòng bảng 52, kiểu truy cập RW
 * Mức đen/hiệu chỉnh của bộ G1, mã Bayer không dấu 12 bit 0..4095. Cộng mức đen chung và offset màu
 * chính xác ở 13 bit trước khi trừ; không cuộn vòng/kẹp tổng, phần dư kẹp về 0. Sau reset là 0.
 */
#define FX1_ISP_BLC_OFS_G1_GR_OFS_G1_GR_SHIFT 0U
#define FX1_ISP_BLC_OFS_G1_GR_OFS_G1_GR_WIDTH 12U
#define FX1_ISP_BLC_OFS_G1_GR_OFS_G1_GR_MASK 0x00000FFFU
#define FX1_ISP_BLC_OFS_G1_GR_OFS_G1_GR_RESET 0x00000000U
#define FX1_ISP_BLC_OFS_G1_GR_OFS_G1_GR_ACCESS FX1_ISP_ACCESS_RW

/**
 * BLC_OFS_G1_GB: địa chỉ lệch 0x0228, dòng bảng 53, kiểu truy cập RW
 * BLC G1: hiệu chỉnh mức đen màu Gb (xanh lá trên hàng xanh dương).
 */
#define FX1_ISP_BLC_OFS_G1_GB_OFFSET 0x0228U
#define REG_BLC_OFS_G1_GB FX1_ISP_BLC_OFS_G1_GB_OFFSET
#define FX1_ISP_BLC_OFS_G1_GB_RESET 0x00000000U
#define FX1_ISP_BLC_OFS_G1_GB_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_BLC_OFS_G1_GB_RO_MASK 0x00000000U
#define FX1_ISP_BLC_OFS_G1_GB_RW_MASK 0x00000FFFU
#define FX1_ISP_BLC_OFS_G1_GB_W1C_MASK 0x00000000U
#define FX1_ISP_BLC_OFS_G1_GB_W1S_MASK 0x00000000U
#define FX1_ISP_BLC_OFS_G1_GB_W1P_MASK 0x00000000U
#define FX1_ISP_BLC_OFS_G1_GB_W1SC_MASK 0x00000000U
/**
 * ofs_g1_gb [11:0], dòng bảng 54, kiểu truy cập RW
 * Mức đen/hiệu chỉnh của bộ G1, mã Bayer không dấu 12 bit 0..4095. Cộng mức đen chung và offset màu
 * chính xác ở 13 bit trước khi trừ; không cuộn vòng/kẹp tổng, phần dư kẹp về 0. Sau reset là 0.
 */
#define FX1_ISP_BLC_OFS_G1_GB_OFS_G1_GB_SHIFT 0U
#define FX1_ISP_BLC_OFS_G1_GB_OFS_G1_GB_WIDTH 12U
#define FX1_ISP_BLC_OFS_G1_GB_OFS_G1_GB_MASK 0x00000FFFU
#define FX1_ISP_BLC_OFS_G1_GB_OFS_G1_GB_RESET 0x00000000U
#define FX1_ISP_BLC_OFS_G1_GB_OFS_G1_GB_ACCESS FX1_ISP_ACCESS_RW

/**
 * BLC_OFS_G1_B: địa chỉ lệch 0x022C, dòng bảng 55, kiểu truy cập RW
 * BLC G1: hiệu chỉnh mức đen màu xanh dương.
 */
#define FX1_ISP_BLC_OFS_G1_B_OFFSET 0x022CU
#define REG_BLC_OFS_G1_B FX1_ISP_BLC_OFS_G1_B_OFFSET
#define FX1_ISP_BLC_OFS_G1_B_RESET 0x00000000U
#define FX1_ISP_BLC_OFS_G1_B_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_BLC_OFS_G1_B_RO_MASK 0x00000000U
#define FX1_ISP_BLC_OFS_G1_B_RW_MASK 0x00000FFFU
#define FX1_ISP_BLC_OFS_G1_B_W1C_MASK 0x00000000U
#define FX1_ISP_BLC_OFS_G1_B_W1S_MASK 0x00000000U
#define FX1_ISP_BLC_OFS_G1_B_W1P_MASK 0x00000000U
#define FX1_ISP_BLC_OFS_G1_B_W1SC_MASK 0x00000000U
/**
 * ofs_g1_b [11:0], dòng bảng 56, kiểu truy cập RW
 * Mức đen/hiệu chỉnh của bộ G1, mã Bayer không dấu 12 bit 0..4095. Cộng mức đen chung và offset màu
 * chính xác ở 13 bit trước khi trừ; không cuộn vòng/kẹp tổng, phần dư kẹp về 0. Sau reset là 0.
 */
#define FX1_ISP_BLC_OFS_G1_B_OFS_G1_B_SHIFT 0U
#define FX1_ISP_BLC_OFS_G1_B_OFS_G1_B_WIDTH 12U
#define FX1_ISP_BLC_OFS_G1_B_OFS_G1_B_MASK 0x00000FFFU
#define FX1_ISP_BLC_OFS_G1_B_OFS_G1_B_RESET 0x00000000U
#define FX1_ISP_BLC_OFS_G1_B_OFS_G1_B_ACCESS FX1_ISP_ACCESS_RW

/**
 * BLC_OFS_G2_DFT: địa chỉ lệch 0x0230, dòng bảng 57, kiểu truy cập RW
 * BLC G2: mức đen dùng chung trừ ở mọi màu; offset từng màu được cộng thêm để hiệu chỉnh.
 */
#define FX1_ISP_BLC_OFS_G2_DFT_OFFSET 0x0230U
#define REG_BLC_OFS_G2_DFT FX1_ISP_BLC_OFS_G2_DFT_OFFSET
#define FX1_ISP_BLC_OFS_G2_DFT_RESET 0x000000C8U
#define FX1_ISP_BLC_OFS_G2_DFT_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_BLC_OFS_G2_DFT_RO_MASK 0x00000000U
#define FX1_ISP_BLC_OFS_G2_DFT_RW_MASK 0x00000FFFU
#define FX1_ISP_BLC_OFS_G2_DFT_W1C_MASK 0x00000000U
#define FX1_ISP_BLC_OFS_G2_DFT_W1S_MASK 0x00000000U
#define FX1_ISP_BLC_OFS_G2_DFT_W1P_MASK 0x00000000U
#define FX1_ISP_BLC_OFS_G2_DFT_W1SC_MASK 0x00000000U
/**
 * ofs_g2_dft [11:0], dòng bảng 58, kiểu truy cập RW
 * Mức đen/hiệu chỉnh của bộ G2, mã Bayer không dấu 12 bit 0..4095. Cộng mức đen chung và offset màu
 * chính xác ở 13 bit trước khi trừ; không cuộn vòng/kẹp tổng, phần dư kẹp về 0. Sau reset là 200.
 */
#define FX1_ISP_BLC_OFS_G2_DFT_OFS_G2_DFT_SHIFT 0U
#define FX1_ISP_BLC_OFS_G2_DFT_OFS_G2_DFT_WIDTH 12U
#define FX1_ISP_BLC_OFS_G2_DFT_OFS_G2_DFT_MASK 0x00000FFFU
#define FX1_ISP_BLC_OFS_G2_DFT_OFS_G2_DFT_RESET 0x000000C8U
#define FX1_ISP_BLC_OFS_G2_DFT_OFS_G2_DFT_ACCESS FX1_ISP_ACCESS_RW

/**
 * BLC_OFS_G2_R: địa chỉ lệch 0x0234, dòng bảng 59, kiểu truy cập RW
 * BLC G2: hiệu chỉnh mức đen màu đỏ.
 */
#define FX1_ISP_BLC_OFS_G2_R_OFFSET 0x0234U
#define REG_BLC_OFS_G2_R FX1_ISP_BLC_OFS_G2_R_OFFSET
#define FX1_ISP_BLC_OFS_G2_R_RESET 0x00000000U
#define FX1_ISP_BLC_OFS_G2_R_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_BLC_OFS_G2_R_RO_MASK 0x00000000U
#define FX1_ISP_BLC_OFS_G2_R_RW_MASK 0x00000FFFU
#define FX1_ISP_BLC_OFS_G2_R_W1C_MASK 0x00000000U
#define FX1_ISP_BLC_OFS_G2_R_W1S_MASK 0x00000000U
#define FX1_ISP_BLC_OFS_G2_R_W1P_MASK 0x00000000U
#define FX1_ISP_BLC_OFS_G2_R_W1SC_MASK 0x00000000U
/**
 * ofs_g2_r [11:0], dòng bảng 60, kiểu truy cập RW
 * Mức đen/hiệu chỉnh của bộ G2, mã Bayer không dấu 12 bit 0..4095. Cộng mức đen chung và offset màu
 * chính xác ở 13 bit trước khi trừ; không cuộn vòng/kẹp tổng, phần dư kẹp về 0. Sau reset là 0.
 */
#define FX1_ISP_BLC_OFS_G2_R_OFS_G2_R_SHIFT 0U
#define FX1_ISP_BLC_OFS_G2_R_OFS_G2_R_WIDTH 12U
#define FX1_ISP_BLC_OFS_G2_R_OFS_G2_R_MASK 0x00000FFFU
#define FX1_ISP_BLC_OFS_G2_R_OFS_G2_R_RESET 0x00000000U
#define FX1_ISP_BLC_OFS_G2_R_OFS_G2_R_ACCESS FX1_ISP_ACCESS_RW

/**
 * BLC_OFS_G2_GR: địa chỉ lệch 0x0238, dòng bảng 61, kiểu truy cập RW
 * BLC G2: hiệu chỉnh mức đen màu Gr (xanh lá trên hàng đỏ).
 */
#define FX1_ISP_BLC_OFS_G2_GR_OFFSET 0x0238U
#define REG_BLC_OFS_G2_GR FX1_ISP_BLC_OFS_G2_GR_OFFSET
#define FX1_ISP_BLC_OFS_G2_GR_RESET 0x00000000U
#define FX1_ISP_BLC_OFS_G2_GR_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_BLC_OFS_G2_GR_RO_MASK 0x00000000U
#define FX1_ISP_BLC_OFS_G2_GR_RW_MASK 0x00000FFFU
#define FX1_ISP_BLC_OFS_G2_GR_W1C_MASK 0x00000000U
#define FX1_ISP_BLC_OFS_G2_GR_W1S_MASK 0x00000000U
#define FX1_ISP_BLC_OFS_G2_GR_W1P_MASK 0x00000000U
#define FX1_ISP_BLC_OFS_G2_GR_W1SC_MASK 0x00000000U
/**
 * ofs_g2_gr [11:0], dòng bảng 62, kiểu truy cập RW
 * Mức đen/hiệu chỉnh của bộ G2, mã Bayer không dấu 12 bit 0..4095. Cộng mức đen chung và offset màu
 * chính xác ở 13 bit trước khi trừ; không cuộn vòng/kẹp tổng, phần dư kẹp về 0. Sau reset là 0.
 */
#define FX1_ISP_BLC_OFS_G2_GR_OFS_G2_GR_SHIFT 0U
#define FX1_ISP_BLC_OFS_G2_GR_OFS_G2_GR_WIDTH 12U
#define FX1_ISP_BLC_OFS_G2_GR_OFS_G2_GR_MASK 0x00000FFFU
#define FX1_ISP_BLC_OFS_G2_GR_OFS_G2_GR_RESET 0x00000000U
#define FX1_ISP_BLC_OFS_G2_GR_OFS_G2_GR_ACCESS FX1_ISP_ACCESS_RW

/**
 * BLC_OFS_G2_GB: địa chỉ lệch 0x023C, dòng bảng 63, kiểu truy cập RW
 * BLC G2: hiệu chỉnh mức đen màu Gb (xanh lá trên hàng xanh dương).
 */
#define FX1_ISP_BLC_OFS_G2_GB_OFFSET 0x023CU
#define REG_BLC_OFS_G2_GB FX1_ISP_BLC_OFS_G2_GB_OFFSET
#define FX1_ISP_BLC_OFS_G2_GB_RESET 0x00000000U
#define FX1_ISP_BLC_OFS_G2_GB_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_BLC_OFS_G2_GB_RO_MASK 0x00000000U
#define FX1_ISP_BLC_OFS_G2_GB_RW_MASK 0x00000FFFU
#define FX1_ISP_BLC_OFS_G2_GB_W1C_MASK 0x00000000U
#define FX1_ISP_BLC_OFS_G2_GB_W1S_MASK 0x00000000U
#define FX1_ISP_BLC_OFS_G2_GB_W1P_MASK 0x00000000U
#define FX1_ISP_BLC_OFS_G2_GB_W1SC_MASK 0x00000000U
/**
 * ofs_g2_gb [11:0], dòng bảng 64, kiểu truy cập RW
 * Mức đen/hiệu chỉnh của bộ G2, mã Bayer không dấu 12 bit 0..4095. Cộng mức đen chung và offset màu
 * chính xác ở 13 bit trước khi trừ; không cuộn vòng/kẹp tổng, phần dư kẹp về 0. Sau reset là 0.
 */
#define FX1_ISP_BLC_OFS_G2_GB_OFS_G2_GB_SHIFT 0U
#define FX1_ISP_BLC_OFS_G2_GB_OFS_G2_GB_WIDTH 12U
#define FX1_ISP_BLC_OFS_G2_GB_OFS_G2_GB_MASK 0x00000FFFU
#define FX1_ISP_BLC_OFS_G2_GB_OFS_G2_GB_RESET 0x00000000U
#define FX1_ISP_BLC_OFS_G2_GB_OFS_G2_GB_ACCESS FX1_ISP_ACCESS_RW

/**
 * BLC_OFS_G2_B: địa chỉ lệch 0x0240, dòng bảng 65, kiểu truy cập RW
 * BLC G2: hiệu chỉnh mức đen màu xanh dương.
 */
#define FX1_ISP_BLC_OFS_G2_B_OFFSET 0x0240U
#define REG_BLC_OFS_G2_B FX1_ISP_BLC_OFS_G2_B_OFFSET
#define FX1_ISP_BLC_OFS_G2_B_RESET 0x00000000U
#define FX1_ISP_BLC_OFS_G2_B_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_BLC_OFS_G2_B_RO_MASK 0x00000000U
#define FX1_ISP_BLC_OFS_G2_B_RW_MASK 0x00000FFFU
#define FX1_ISP_BLC_OFS_G2_B_W1C_MASK 0x00000000U
#define FX1_ISP_BLC_OFS_G2_B_W1S_MASK 0x00000000U
#define FX1_ISP_BLC_OFS_G2_B_W1P_MASK 0x00000000U
#define FX1_ISP_BLC_OFS_G2_B_W1SC_MASK 0x00000000U
/**
 * ofs_g2_b [11:0], dòng bảng 66, kiểu truy cập RW
 * Mức đen/hiệu chỉnh của bộ G2, mã Bayer không dấu 12 bit 0..4095. Cộng mức đen chung và offset màu
 * chính xác ở 13 bit trước khi trừ; không cuộn vòng/kẹp tổng, phần dư kẹp về 0. Sau reset là 0.
 */
#define FX1_ISP_BLC_OFS_G2_B_OFS_G2_B_SHIFT 0U
#define FX1_ISP_BLC_OFS_G2_B_OFS_G2_B_WIDTH 12U
#define FX1_ISP_BLC_OFS_G2_B_OFS_G2_B_MASK 0x00000FFFU
#define FX1_ISP_BLC_OFS_G2_B_OFS_G2_B_RESET 0x00000000U
#define FX1_ISP_BLC_OFS_G2_B_OFS_G2_B_ACCESS FX1_ISP_ACCESS_RW

/**
 * BLC_OFS_G3_DFT: địa chỉ lệch 0x0244, dòng bảng 67, kiểu truy cập RW
 * BLC G3: mức đen dùng chung trừ ở mọi màu; offset từng màu được cộng thêm để hiệu chỉnh.
 */
#define FX1_ISP_BLC_OFS_G3_DFT_OFFSET 0x0244U
#define REG_BLC_OFS_G3_DFT FX1_ISP_BLC_OFS_G3_DFT_OFFSET
#define FX1_ISP_BLC_OFS_G3_DFT_RESET 0x000000C8U
#define FX1_ISP_BLC_OFS_G3_DFT_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_BLC_OFS_G3_DFT_RO_MASK 0x00000000U
#define FX1_ISP_BLC_OFS_G3_DFT_RW_MASK 0x00000FFFU
#define FX1_ISP_BLC_OFS_G3_DFT_W1C_MASK 0x00000000U
#define FX1_ISP_BLC_OFS_G3_DFT_W1S_MASK 0x00000000U
#define FX1_ISP_BLC_OFS_G3_DFT_W1P_MASK 0x00000000U
#define FX1_ISP_BLC_OFS_G3_DFT_W1SC_MASK 0x00000000U
/**
 * ofs_g3_dft [11:0], dòng bảng 68, kiểu truy cập RW
 * Mức đen/hiệu chỉnh của bộ G3, mã Bayer không dấu 12 bit 0..4095. Cộng mức đen chung và offset màu
 * chính xác ở 13 bit trước khi trừ; không cuộn vòng/kẹp tổng, phần dư kẹp về 0. Sau reset là 200.
 */
#define FX1_ISP_BLC_OFS_G3_DFT_OFS_G3_DFT_SHIFT 0U
#define FX1_ISP_BLC_OFS_G3_DFT_OFS_G3_DFT_WIDTH 12U
#define FX1_ISP_BLC_OFS_G3_DFT_OFS_G3_DFT_MASK 0x00000FFFU
#define FX1_ISP_BLC_OFS_G3_DFT_OFS_G3_DFT_RESET 0x000000C8U
#define FX1_ISP_BLC_OFS_G3_DFT_OFS_G3_DFT_ACCESS FX1_ISP_ACCESS_RW

/**
 * BLC_OFS_G3_R: địa chỉ lệch 0x0248, dòng bảng 69, kiểu truy cập RW
 * BLC G3: hiệu chỉnh mức đen màu đỏ.
 */
#define FX1_ISP_BLC_OFS_G3_R_OFFSET 0x0248U
#define REG_BLC_OFS_G3_R FX1_ISP_BLC_OFS_G3_R_OFFSET
#define FX1_ISP_BLC_OFS_G3_R_RESET 0x00000000U
#define FX1_ISP_BLC_OFS_G3_R_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_BLC_OFS_G3_R_RO_MASK 0x00000000U
#define FX1_ISP_BLC_OFS_G3_R_RW_MASK 0x00000FFFU
#define FX1_ISP_BLC_OFS_G3_R_W1C_MASK 0x00000000U
#define FX1_ISP_BLC_OFS_G3_R_W1S_MASK 0x00000000U
#define FX1_ISP_BLC_OFS_G3_R_W1P_MASK 0x00000000U
#define FX1_ISP_BLC_OFS_G3_R_W1SC_MASK 0x00000000U
/**
 * ofs_g3_r [11:0], dòng bảng 70, kiểu truy cập RW
 * Mức đen/hiệu chỉnh của bộ G3, mã Bayer không dấu 12 bit 0..4095. Cộng mức đen chung và offset màu
 * chính xác ở 13 bit trước khi trừ; không cuộn vòng/kẹp tổng, phần dư kẹp về 0. Sau reset là 0.
 */
#define FX1_ISP_BLC_OFS_G3_R_OFS_G3_R_SHIFT 0U
#define FX1_ISP_BLC_OFS_G3_R_OFS_G3_R_WIDTH 12U
#define FX1_ISP_BLC_OFS_G3_R_OFS_G3_R_MASK 0x00000FFFU
#define FX1_ISP_BLC_OFS_G3_R_OFS_G3_R_RESET 0x00000000U
#define FX1_ISP_BLC_OFS_G3_R_OFS_G3_R_ACCESS FX1_ISP_ACCESS_RW

/**
 * BLC_OFS_G3_GR: địa chỉ lệch 0x024C, dòng bảng 71, kiểu truy cập RW
 * BLC G3: hiệu chỉnh mức đen màu Gr (xanh lá trên hàng đỏ).
 */
#define FX1_ISP_BLC_OFS_G3_GR_OFFSET 0x024CU
#define REG_BLC_OFS_G3_GR FX1_ISP_BLC_OFS_G3_GR_OFFSET
#define FX1_ISP_BLC_OFS_G3_GR_RESET 0x00000000U
#define FX1_ISP_BLC_OFS_G3_GR_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_BLC_OFS_G3_GR_RO_MASK 0x00000000U
#define FX1_ISP_BLC_OFS_G3_GR_RW_MASK 0x00000FFFU
#define FX1_ISP_BLC_OFS_G3_GR_W1C_MASK 0x00000000U
#define FX1_ISP_BLC_OFS_G3_GR_W1S_MASK 0x00000000U
#define FX1_ISP_BLC_OFS_G3_GR_W1P_MASK 0x00000000U
#define FX1_ISP_BLC_OFS_G3_GR_W1SC_MASK 0x00000000U
/**
 * ofs_g3_gr [11:0], dòng bảng 72, kiểu truy cập RW
 * Mức đen/hiệu chỉnh của bộ G3, mã Bayer không dấu 12 bit 0..4095. Cộng mức đen chung và offset màu
 * chính xác ở 13 bit trước khi trừ; không cuộn vòng/kẹp tổng, phần dư kẹp về 0. Sau reset là 0.
 */
#define FX1_ISP_BLC_OFS_G3_GR_OFS_G3_GR_SHIFT 0U
#define FX1_ISP_BLC_OFS_G3_GR_OFS_G3_GR_WIDTH 12U
#define FX1_ISP_BLC_OFS_G3_GR_OFS_G3_GR_MASK 0x00000FFFU
#define FX1_ISP_BLC_OFS_G3_GR_OFS_G3_GR_RESET 0x00000000U
#define FX1_ISP_BLC_OFS_G3_GR_OFS_G3_GR_ACCESS FX1_ISP_ACCESS_RW

/**
 * BLC_OFS_G3_GB: địa chỉ lệch 0x0250, dòng bảng 73, kiểu truy cập RW
 * BLC G3: hiệu chỉnh mức đen màu Gb (xanh lá trên hàng xanh dương).
 */
#define FX1_ISP_BLC_OFS_G3_GB_OFFSET 0x0250U
#define REG_BLC_OFS_G3_GB FX1_ISP_BLC_OFS_G3_GB_OFFSET
#define FX1_ISP_BLC_OFS_G3_GB_RESET 0x00000000U
#define FX1_ISP_BLC_OFS_G3_GB_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_BLC_OFS_G3_GB_RO_MASK 0x00000000U
#define FX1_ISP_BLC_OFS_G3_GB_RW_MASK 0x00000FFFU
#define FX1_ISP_BLC_OFS_G3_GB_W1C_MASK 0x00000000U
#define FX1_ISP_BLC_OFS_G3_GB_W1S_MASK 0x00000000U
#define FX1_ISP_BLC_OFS_G3_GB_W1P_MASK 0x00000000U
#define FX1_ISP_BLC_OFS_G3_GB_W1SC_MASK 0x00000000U
/**
 * ofs_g3_gb [11:0], dòng bảng 74, kiểu truy cập RW
 * Mức đen/hiệu chỉnh của bộ G3, mã Bayer không dấu 12 bit 0..4095. Cộng mức đen chung và offset màu
 * chính xác ở 13 bit trước khi trừ; không cuộn vòng/kẹp tổng, phần dư kẹp về 0. Sau reset là 0.
 */
#define FX1_ISP_BLC_OFS_G3_GB_OFS_G3_GB_SHIFT 0U
#define FX1_ISP_BLC_OFS_G3_GB_OFS_G3_GB_WIDTH 12U
#define FX1_ISP_BLC_OFS_G3_GB_OFS_G3_GB_MASK 0x00000FFFU
#define FX1_ISP_BLC_OFS_G3_GB_OFS_G3_GB_RESET 0x00000000U
#define FX1_ISP_BLC_OFS_G3_GB_OFS_G3_GB_ACCESS FX1_ISP_ACCESS_RW

/**
 * BLC_OFS_G3_B: địa chỉ lệch 0x0254, dòng bảng 75, kiểu truy cập RW
 * BLC G3: hiệu chỉnh mức đen màu xanh dương.
 */
#define FX1_ISP_BLC_OFS_G3_B_OFFSET 0x0254U
#define REG_BLC_OFS_G3_B FX1_ISP_BLC_OFS_G3_B_OFFSET
#define FX1_ISP_BLC_OFS_G3_B_RESET 0x00000000U
#define FX1_ISP_BLC_OFS_G3_B_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_BLC_OFS_G3_B_RO_MASK 0x00000000U
#define FX1_ISP_BLC_OFS_G3_B_RW_MASK 0x00000FFFU
#define FX1_ISP_BLC_OFS_G3_B_W1C_MASK 0x00000000U
#define FX1_ISP_BLC_OFS_G3_B_W1S_MASK 0x00000000U
#define FX1_ISP_BLC_OFS_G3_B_W1P_MASK 0x00000000U
#define FX1_ISP_BLC_OFS_G3_B_W1SC_MASK 0x00000000U
/**
 * ofs_g3_b [11:0], dòng bảng 76, kiểu truy cập RW
 * Mức đen/hiệu chỉnh của bộ G3, mã Bayer không dấu 12 bit 0..4095. Cộng mức đen chung và offset màu
 * chính xác ở 13 bit trước khi trừ; không cuộn vòng/kẹp tổng, phần dư kẹp về 0. Sau reset là 0.
 */
#define FX1_ISP_BLC_OFS_G3_B_OFS_G3_B_SHIFT 0U
#define FX1_ISP_BLC_OFS_G3_B_OFS_G3_B_WIDTH 12U
#define FX1_ISP_BLC_OFS_G3_B_OFS_G3_B_MASK 0x00000FFFU
#define FX1_ISP_BLC_OFS_G3_B_OFS_G3_B_RESET 0x00000000U
#define FX1_ISP_BLC_OFS_G3_B_OFS_G3_B_ACCESS FX1_ISP_ACCESS_RW

/**
 * BLC_SCALE_G0: địa chỉ lệch 0x0258, dòng bảng 77, kiểu truy cập RW
 * BLC G0: giãn dải sau phép trừ để khôi phục mức cực đại.
 */
#define FX1_ISP_BLC_SCALE_G0_OFFSET 0x0258U
#define REG_BLC_SCALE_G0 FX1_ISP_BLC_SCALE_G0_OFFSET
#define FX1_ISP_BLC_SCALE_G0_RESET 0x00010000U
#define FX1_ISP_BLC_SCALE_G0_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_BLC_SCALE_G0_RO_MASK 0x00000000U
#define FX1_ISP_BLC_SCALE_G0_RW_MASK 0x0003FFFFU
#define FX1_ISP_BLC_SCALE_G0_W1C_MASK 0x00000000U
#define FX1_ISP_BLC_SCALE_G0_W1S_MASK 0x00000000U
#define FX1_ISP_BLC_SCALE_G0_W1P_MASK 0x00000000U
#define FX1_ISP_BLC_SCALE_G0_W1SC_MASK 0x00000000U
/**
 * scale_g0 [17:0], dòng bảng 78, kiểu truy cập RW
 * Hệ số giãn dải G0 dạng UQ2.16; 0x10000=1x. Hệ số khôi phục toàn dải là 4095/(4095-pedestal).
 */
#define FX1_ISP_BLC_SCALE_G0_SCALE_G0_SHIFT 0U
#define FX1_ISP_BLC_SCALE_G0_SCALE_G0_WIDTH 18U
#define FX1_ISP_BLC_SCALE_G0_SCALE_G0_MASK 0x0003FFFFU
#define FX1_ISP_BLC_SCALE_G0_SCALE_G0_RESET 0x00010000U
#define FX1_ISP_BLC_SCALE_G0_SCALE_G0_ACCESS FX1_ISP_ACCESS_RW

/**
 * BLC_SCALE_G1: địa chỉ lệch 0x025C, dòng bảng 79, kiểu truy cập RW
 * BLC G1: giãn dải sau phép trừ để khôi phục mức cực đại.
 */
#define FX1_ISP_BLC_SCALE_G1_OFFSET 0x025CU
#define REG_BLC_SCALE_G1 FX1_ISP_BLC_SCALE_G1_OFFSET
#define FX1_ISP_BLC_SCALE_G1_RESET 0x00010000U
#define FX1_ISP_BLC_SCALE_G1_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_BLC_SCALE_G1_RO_MASK 0x00000000U
#define FX1_ISP_BLC_SCALE_G1_RW_MASK 0x0003FFFFU
#define FX1_ISP_BLC_SCALE_G1_W1C_MASK 0x00000000U
#define FX1_ISP_BLC_SCALE_G1_W1S_MASK 0x00000000U
#define FX1_ISP_BLC_SCALE_G1_W1P_MASK 0x00000000U
#define FX1_ISP_BLC_SCALE_G1_W1SC_MASK 0x00000000U
/**
 * scale_g1 [17:0], dòng bảng 80, kiểu truy cập RW
 * Hệ số giãn dải G1 dạng UQ2.16; 0x10000=1x. Hệ số khôi phục toàn dải là 4095/(4095-pedestal).
 */
#define FX1_ISP_BLC_SCALE_G1_SCALE_G1_SHIFT 0U
#define FX1_ISP_BLC_SCALE_G1_SCALE_G1_WIDTH 18U
#define FX1_ISP_BLC_SCALE_G1_SCALE_G1_MASK 0x0003FFFFU
#define FX1_ISP_BLC_SCALE_G1_SCALE_G1_RESET 0x00010000U
#define FX1_ISP_BLC_SCALE_G1_SCALE_G1_ACCESS FX1_ISP_ACCESS_RW

/**
 * BLC_SCALE_G2: địa chỉ lệch 0x0260, dòng bảng 81, kiểu truy cập RW
 * BLC G2: giãn dải sau phép trừ để khôi phục mức cực đại.
 */
#define FX1_ISP_BLC_SCALE_G2_OFFSET 0x0260U
#define REG_BLC_SCALE_G2 FX1_ISP_BLC_SCALE_G2_OFFSET
#define FX1_ISP_BLC_SCALE_G2_RESET 0x00010000U
#define FX1_ISP_BLC_SCALE_G2_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_BLC_SCALE_G2_RO_MASK 0x00000000U
#define FX1_ISP_BLC_SCALE_G2_RW_MASK 0x0003FFFFU
#define FX1_ISP_BLC_SCALE_G2_W1C_MASK 0x00000000U
#define FX1_ISP_BLC_SCALE_G2_W1S_MASK 0x00000000U
#define FX1_ISP_BLC_SCALE_G2_W1P_MASK 0x00000000U
#define FX1_ISP_BLC_SCALE_G2_W1SC_MASK 0x00000000U
/**
 * scale_g2 [17:0], dòng bảng 82, kiểu truy cập RW
 * Hệ số giãn dải G2 dạng UQ2.16; 0x10000=1x. Hệ số khôi phục toàn dải là 4095/(4095-pedestal).
 */
#define FX1_ISP_BLC_SCALE_G2_SCALE_G2_SHIFT 0U
#define FX1_ISP_BLC_SCALE_G2_SCALE_G2_WIDTH 18U
#define FX1_ISP_BLC_SCALE_G2_SCALE_G2_MASK 0x0003FFFFU
#define FX1_ISP_BLC_SCALE_G2_SCALE_G2_RESET 0x00010000U
#define FX1_ISP_BLC_SCALE_G2_SCALE_G2_ACCESS FX1_ISP_ACCESS_RW

/**
 * BLC_SCALE_G3: địa chỉ lệch 0x0264, dòng bảng 83, kiểu truy cập RW
 * BLC G3: giãn dải sau phép trừ để khôi phục mức cực đại.
 */
#define FX1_ISP_BLC_SCALE_G3_OFFSET 0x0264U
#define REG_BLC_SCALE_G3 FX1_ISP_BLC_SCALE_G3_OFFSET
#define FX1_ISP_BLC_SCALE_G3_RESET 0x00010000U
#define FX1_ISP_BLC_SCALE_G3_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_BLC_SCALE_G3_RO_MASK 0x00000000U
#define FX1_ISP_BLC_SCALE_G3_RW_MASK 0x0003FFFFU
#define FX1_ISP_BLC_SCALE_G3_W1C_MASK 0x00000000U
#define FX1_ISP_BLC_SCALE_G3_W1S_MASK 0x00000000U
#define FX1_ISP_BLC_SCALE_G3_W1P_MASK 0x00000000U
#define FX1_ISP_BLC_SCALE_G3_W1SC_MASK 0x00000000U
/**
 * scale_g3 [17:0], dòng bảng 84, kiểu truy cập RW
 * Hệ số giãn dải G3 dạng UQ2.16; 0x10000=1x. Hệ số khôi phục toàn dải là 4095/(4095-pedestal).
 */
#define FX1_ISP_BLC_SCALE_G3_SCALE_G3_SHIFT 0U
#define FX1_ISP_BLC_SCALE_G3_SCALE_G3_WIDTH 18U
#define FX1_ISP_BLC_SCALE_G3_SCALE_G3_MASK 0x0003FFFFU
#define FX1_ISP_BLC_SCALE_G3_SCALE_G3_RESET 0x00010000U
#define FX1_ISP_BLC_SCALE_G3_SCALE_G3_ACCESS FX1_ISP_ACCESS_RW

/**
 * LSC_CTRL: địa chỉ lệch 0x0400, dòng bảng 85, kiểu truy cập RW
 * LSC: nhân từng mẫu Bayer với gain nội suy từ lưới để bù suy giảm ánh sáng ở góc ống kính.
 */
#define FX1_ISP_LSC_CTRL_OFFSET 0x0400U
#define REG_LSC_CTRL FX1_ISP_LSC_CTRL_OFFSET
#define FX1_ISP_LSC_CTRL_RESET 0x00000000U
#define FX1_ISP_LSC_CTRL_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_LSC_CTRL_RO_MASK 0x00000000U
#define FX1_ISP_LSC_CTRL_RW_MASK 0x00000001U
#define FX1_ISP_LSC_CTRL_W1C_MASK 0x00000000U
#define FX1_ISP_LSC_CTRL_W1S_MASK 0x00000000U
#define FX1_ISP_LSC_CTRL_W1P_MASK 0x00000000U
#define FX1_ISP_LSC_CTRL_W1SC_MASK 0x00000000U
/**
 * en [0:0], dòng bảng 86, kiểu truy cập RW
 * Bật LSC: 1 áp gain lưới, 0 truyền nguyên. Chỉ có tác dụng khi profile đã nạp và xác nhận hợp lệ;
 * xem LSC_PROFILE_STATUS.
 */
#define FX1_ISP_LSC_CTRL_EN_SHIFT 0U
#define FX1_ISP_LSC_CTRL_EN_WIDTH 1U
#define FX1_ISP_LSC_CTRL_EN_MASK 0x00000001U
#define FX1_ISP_LSC_CTRL_EN_RESET 0x00000000U
#define FX1_ISP_LSC_CTRL_EN_ACCESS FX1_ISP_ACCESS_RW

/**
 * LSC_MESH_NODES: địa chỉ lệch 0x0404, dòng bảng 87, kiểu truy cập RW
 * Hình học lưới LSC dùng chung mọi profile; 0 sau reset cố ý chưa hợp lệ. Ngoài TUNE_BANK; chỉ đổi
 * sau khi xử lý hết, thoát chế độ profile và giải phóng mọi tham chiếu cư trú đã SEAL. Thay đổi xóa
 * xác nhận lưới.
 */
#define FX1_ISP_LSC_MESH_NODES_OFFSET 0x0404U
#define REG_LSC_MESH_NODES FX1_ISP_LSC_MESH_NODES_OFFSET
#define FX1_ISP_LSC_MESH_NODES_RESET 0x00000000U
#define FX1_ISP_LSC_MESH_NODES_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_LSC_MESH_NODES_RO_MASK 0x00000000U
#define FX1_ISP_LSC_MESH_NODES_RW_MASK 0x0000FFFFU
#define FX1_ISP_LSC_MESH_NODES_W1C_MASK 0x00000000U
#define FX1_ISP_LSC_MESH_NODES_W1S_MASK 0x00000000U
#define FX1_ISP_LSC_MESH_NODES_W1P_MASK 0x00000000U
#define FX1_ISP_LSC_MESH_NODES_W1SC_MASK 0x00000000U
/**
 * mesh_nx [7:0], dòng bảng 88, kiểu truy cập RW
 * Số nút ngang 2..32, không vượt chiều rộng hữu dụng. Phân hoạch width-1 trên nx-1 khoảng; khoảng
 * cách nguyên có thể lệch nhau một mẫu. Đổi hình học làm mất hiệu lực các profile cư trú.
 */
#define FX1_ISP_LSC_MESH_NODES_MESH_NX_SHIFT 0U
#define FX1_ISP_LSC_MESH_NODES_MESH_NX_WIDTH 8U
#define FX1_ISP_LSC_MESH_NODES_MESH_NX_MASK 0x000000FFU
#define FX1_ISP_LSC_MESH_NODES_MESH_NX_RESET 0x00000000U
#define FX1_ISP_LSC_MESH_NODES_MESH_NX_ACCESS FX1_ISP_ACCESS_RW
/**
 * mesh_ny [15:8], dòng bảng 89, kiểu truy cập RW
 * Số nút dọc 2..32, không vượt chiều cao hữu dụng. Phân hoạch height-1 trên ny-1 khoảng; khoảng cách
 * nguyên có thể lệch nhau một dòng.
 */
#define FX1_ISP_LSC_MESH_NODES_MESH_NY_SHIFT 8U
#define FX1_ISP_LSC_MESH_NODES_MESH_NY_WIDTH 8U
#define FX1_ISP_LSC_MESH_NODES_MESH_NY_MASK 0x0000FF00U
#define FX1_ISP_LSC_MESH_NODES_MESH_NY_RESET 0x00000000U
#define FX1_ISP_LSC_MESH_NODES_MESH_NY_ACCESS FX1_ISP_ACCESS_RW

/**
 * LSC_STRENGTH: địa chỉ lệch 0x0408, dòng bảng 90, kiểu truy cập RW
 * Cường độ LSC trộn giữa không sửa và áp toàn lưới; giảm mức bù mà không cần nạp lại dữ liệu hiệu
 * chuẩn.
 */
#define FX1_ISP_LSC_STRENGTH_OFFSET 0x0408U
#define REG_LSC_STRENGTH FX1_ISP_LSC_STRENGTH_OFFSET
#define FX1_ISP_LSC_STRENGTH_RESET 0x00000000U
#define FX1_ISP_LSC_STRENGTH_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_LSC_STRENGTH_RO_MASK 0x00000000U
#define FX1_ISP_LSC_STRENGTH_RW_MASK 0x0001FFFFU
#define FX1_ISP_LSC_STRENGTH_W1C_MASK 0x00000000U
#define FX1_ISP_LSC_STRENGTH_W1S_MASK 0x00000000U
#define FX1_ISP_LSC_STRENGTH_W1P_MASK 0x00000000U
#define FX1_ISP_LSC_STRENGTH_W1SC_MASK 0x00000000U
/**
 * strength [16:0], dòng bảng 91, kiểu truy cập RW
 * Cường độ UQ1.16 hợp lệ 0..0x10000: 0 cho hệ số 1x, 0x10000 áp đủ lưới. Mã biểu diễn lớn hơn ngoại
 * suy mức sửa; gain hiệu dụng âm kẹp pixel về 0.
 */
#define FX1_ISP_LSC_STRENGTH_STRENGTH_SHIFT 0U
#define FX1_ISP_LSC_STRENGTH_STRENGTH_WIDTH 17U
#define FX1_ISP_LSC_STRENGTH_STRENGTH_MASK 0x0001FFFFU
#define FX1_ISP_LSC_STRENGTH_STRENGTH_RESET 0x00000000U
#define FX1_ISP_LSC_STRENGTH_STRENGTH_ACCESS FX1_ISP_ACCESS_RW

/**
 * LSC_PROFILE_SEL: địa chỉ lệch 0x040C, dòng bảng 92, kiểu truy cập RW
 * Chọn một trong ba profile LSC cư trú theo nguồn sáng. Legacy từ chối profile chưa nạp và đặt
 * profile_sel_reject. Profile mode chỉ lưu giá trị theo bank; SEAL từ chối mã 3 bằng INVALID hoặc
 * chọn profile bật nhưng chưa hợp lệ bằng NOT_READY.
 */
#define FX1_ISP_LSC_PROFILE_SEL_OFFSET 0x040CU
#define REG_LSC_PROFILE_SEL FX1_ISP_LSC_PROFILE_SEL_OFFSET
#define FX1_ISP_LSC_PROFILE_SEL_RESET 0x00000000U
#define FX1_ISP_LSC_PROFILE_SEL_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_LSC_PROFILE_SEL_RO_MASK 0x00000000U
#define FX1_ISP_LSC_PROFILE_SEL_RW_MASK 0x00000003U
#define FX1_ISP_LSC_PROFILE_SEL_W1C_MASK 0x00000000U
#define FX1_ISP_LSC_PROFILE_SEL_W1S_MASK 0x00000000U
#define FX1_ISP_LSC_PROFILE_SEL_W1P_MASK 0x00000000U
#define FX1_ISP_LSC_PROFILE_SEL_W1SC_MASK 0x00000000U
/**
 * profile_sel [1:0], dòng bảng 93, kiểu truy cập RW
 * Chọn profile 0..2. Mã 3 bị từ chối ngay khi ghi ở legacy hoặc khi SEAL ở profile mode.
 */
#define FX1_ISP_LSC_PROFILE_SEL_PROFILE_SEL_SHIFT 0U
#define FX1_ISP_LSC_PROFILE_SEL_PROFILE_SEL_WIDTH 2U
#define FX1_ISP_LSC_PROFILE_SEL_PROFILE_SEL_MASK 0x00000003U
#define FX1_ISP_LSC_PROFILE_SEL_PROFILE_SEL_RESET 0x00000000U
#define FX1_ISP_LSC_PROFILE_SEL_PROFILE_SEL_ACCESS FX1_ISP_ACCESS_RW

/**
 * LSC_LOAD_CTRL: địa chỉ lệch 0x0410, dòng bảng 94, kiểu truy cập Mixed
 * Nạp lưới tuần tự vào profile không hoạt động: BEGIN, đúng 4*nx*ny hệ số, VALIDATE. Ưu tiên ABORT,
 * VALIDATE, BEGIN; bắt đầu khi bận/xác nhận khi rỗi đặt LOAD_PROTOCOL. Profile mode còn đặt LOCKED
 * với đích đã SEAL/đang chờ/đang xử lý; đích đang xử lý đặt cả ACTIVE_LOAD_REJECT.
 */
#define FX1_ISP_LSC_LOAD_CTRL_OFFSET 0x0410U
#define REG_LSC_LOAD_CTRL FX1_ISP_LSC_LOAD_CTRL_OFFSET
#define FX1_ISP_LSC_LOAD_CTRL_RESET 0x00000000U
#define FX1_ISP_LSC_LOAD_CTRL_ACCESS FX1_ISP_ACCESS_MIXED
#define FX1_ISP_LSC_LOAD_CTRL_RO_MASK 0x00000000U
#define FX1_ISP_LSC_LOAD_CTRL_RW_MASK 0x00000003U
#define FX1_ISP_LSC_LOAD_CTRL_W1C_MASK 0x00000000U
#define FX1_ISP_LSC_LOAD_CTRL_W1S_MASK 0x00000700U
#define FX1_ISP_LSC_LOAD_CTRL_W1P_MASK 0x00000000U
#define FX1_ISP_LSC_LOAD_CTRL_W1SC_MASK 0x00000000U
/**
 * load_profile [1:0], dòng bảng 95, kiểu truy cập RW
 * Đích nạp profile 0..2. Từ chối profile đang được frame chưa kết thúc dùng với LSC bật; ở legacy
 * còn gồm profile được chọn khi EN=1 và ACTIVE_VALID=1. Sau reset chưa có profile hoạt động.
 */
#define FX1_ISP_LSC_LOAD_CTRL_LOAD_PROFILE_SHIFT 0U
#define FX1_ISP_LSC_LOAD_CTRL_LOAD_PROFILE_WIDTH 2U
#define FX1_ISP_LSC_LOAD_CTRL_LOAD_PROFILE_MASK 0x00000003U
#define FX1_ISP_LSC_LOAD_CTRL_LOAD_PROFILE_RESET 0x00000000U
#define FX1_ISP_LSC_LOAD_CTRL_LOAD_PROFILE_ACCESS FX1_ISP_ACCESS_RW
/**
 * load_begin [8:8], dòng bảng 96, kiểu truy cập W1S
 * Ghi 1 để bắt đầu nạp hệ số; bit tự xóa.
 */
#define FX1_ISP_LSC_LOAD_CTRL_LOAD_BEGIN_SHIFT 8U
#define FX1_ISP_LSC_LOAD_CTRL_LOAD_BEGIN_WIDTH 1U
#define FX1_ISP_LSC_LOAD_CTRL_LOAD_BEGIN_MASK 0x00000100U
#define FX1_ISP_LSC_LOAD_CTRL_LOAD_BEGIN_RESET 0x00000000U
#define FX1_ISP_LSC_LOAD_CTRL_LOAD_BEGIN_ACCESS FX1_ISP_ACCESS_W1S
/**
 * load_validate [9:9], dòng bảng 97, kiểu truy cập W1S
 * Ghi 1 để kết thúc/xác nhận; chỉ đặt hợp lệ nếu số hệ số khớp.
 */
#define FX1_ISP_LSC_LOAD_CTRL_LOAD_VALIDATE_SHIFT 9U
#define FX1_ISP_LSC_LOAD_CTRL_LOAD_VALIDATE_WIDTH 1U
#define FX1_ISP_LSC_LOAD_CTRL_LOAD_VALIDATE_MASK 0x00000200U
#define FX1_ISP_LSC_LOAD_CTRL_LOAD_VALIDATE_RESET 0x00000000U
#define FX1_ISP_LSC_LOAD_CTRL_LOAD_VALIDATE_ACCESS FX1_ISP_ACCESS_W1S
/**
 * load_abort [10:10], dòng bảng 98, kiểu truy cập W1S
 * Ghi 1 để hủy phần nạp dở và về rỗi; bit tự xóa. Đích giữ không hợp lệ đến khi nạp/xác nhận lại;
 * profile đang hoạt động không đổi.
 */
#define FX1_ISP_LSC_LOAD_CTRL_LOAD_ABORT_SHIFT 10U
#define FX1_ISP_LSC_LOAD_CTRL_LOAD_ABORT_WIDTH 1U
#define FX1_ISP_LSC_LOAD_CTRL_LOAD_ABORT_MASK 0x00000400U
#define FX1_ISP_LSC_LOAD_CTRL_LOAD_ABORT_RESET 0x00000000U
#define FX1_ISP_LSC_LOAD_CTRL_LOAD_ABORT_ACCESS FX1_ISP_ACCESS_W1S

/**
 * LSC_COEF_DATA: địa chỉ lệch 0x0414, dòng bảng 99, kiểu truy cập RW
 * Cổng ghi hệ số LSC tự tiến địa chỉ theo R,Gr,Gb,B tại mỗi nút, rồi X, rồi Y. Profile mode từ chối
 * đích cư trú đã SEAL, đang chờ hoặc đang xử lý bằng TUNE_ERROR.LOCKED.
 */
#define FX1_ISP_LSC_COEF_DATA_OFFSET 0x0414U
#define REG_LSC_COEF_DATA FX1_ISP_LSC_COEF_DATA_OFFSET
#define FX1_ISP_LSC_COEF_DATA_RESET 0x00000000U
#define FX1_ISP_LSC_COEF_DATA_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_LSC_COEF_DATA_RO_MASK 0x00000000U
#define FX1_ISP_LSC_COEF_DATA_RW_MASK 0x001FFFFFU
#define FX1_ISP_LSC_COEF_DATA_W1C_MASK 0x00000000U
#define FX1_ISP_LSC_COEF_DATA_W1S_MASK 0x00000000U
#define FX1_ISP_LSC_COEF_DATA_W1P_MASK 0x00000000U
#define FX1_ISP_LSC_COEF_DATA_W1SC_MASK 0x00000000U
/**
 * coef_data [20:0], dòng bảng 100, kiểu truy cập RW
 * Hệ số UQ3.18 hợp lệ 0..4, mã 0..0x100000; 1x là 0x040000. Vượt 4 vẫn được đếm nhưng đặt COEF_RANGE
 * và làm lần nạp không hợp lệ. Ghi sau LOAD_FULL bị bỏ và đặt LOAD_PROTOCOL. Thứ tự R,Gr,Gb,B tại
 * từng nút, rồi X, Y.
 */
#define FX1_ISP_LSC_COEF_DATA_COEF_DATA_SHIFT 0U
#define FX1_ISP_LSC_COEF_DATA_COEF_DATA_WIDTH 21U
#define FX1_ISP_LSC_COEF_DATA_COEF_DATA_MASK 0x001FFFFFU
#define FX1_ISP_LSC_COEF_DATA_COEF_DATA_RESET 0x00000000U
#define FX1_ISP_LSC_COEF_DATA_COEF_DATA_ACCESS FX1_ISP_ACCESS_RW

/**
 * LSC_LOAD_STATUS: địa chỉ lệch 0x0418, dòng bảng 101, kiểu truy cập RO
 * Tiến độ nạp LSC hiện tại, chỉ đọc.
 */
#define FX1_ISP_LSC_LOAD_STATUS_OFFSET 0x0418U
#define REG_LSC_LOAD_STATUS FX1_ISP_LSC_LOAD_STATUS_OFFSET
#define FX1_ISP_LSC_LOAD_STATUS_RESET 0x00000000U
#define FX1_ISP_LSC_LOAD_STATUS_ACCESS FX1_ISP_ACCESS_RO
#define FX1_ISP_LSC_LOAD_STATUS_RO_MASK 0x000FFFFFU
#define FX1_ISP_LSC_LOAD_STATUS_RW_MASK 0x00000000U
#define FX1_ISP_LSC_LOAD_STATUS_W1C_MASK 0x00000000U
#define FX1_ISP_LSC_LOAD_STATUS_W1S_MASK 0x00000000U
#define FX1_ISP_LSC_LOAD_STATUS_W1P_MASK 0x00000000U
#define FX1_ISP_LSC_LOAD_STATUS_W1SC_MASK 0x00000000U
/**
 * load_addr [15:0], dòng bảng 102, kiểu truy cập RO
 * Số hệ số đã được chấp nhận trong lần nạp này.
 */
#define FX1_ISP_LSC_LOAD_STATUS_LOAD_ADDR_SHIFT 0U
#define FX1_ISP_LSC_LOAD_STATUS_LOAD_ADDR_WIDTH 16U
#define FX1_ISP_LSC_LOAD_STATUS_LOAD_ADDR_MASK 0x0000FFFFU
#define FX1_ISP_LSC_LOAD_STATUS_LOAD_ADDR_RESET 0x00000000U
#define FX1_ISP_LSC_LOAD_STATUS_LOAD_ADDR_ACCESS FX1_ISP_ACCESS_RO
/**
 * load_full [16:16], dòng bảng 103, kiểu truy cập RO
 * LOAD_FULL=1 khi đã ghi đủ 4*nx*ny hệ số.
 */
#define FX1_ISP_LSC_LOAD_STATUS_LOAD_FULL_SHIFT 16U
#define FX1_ISP_LSC_LOAD_STATUS_LOAD_FULL_WIDTH 1U
#define FX1_ISP_LSC_LOAD_STATUS_LOAD_FULL_MASK 0x00010000U
#define FX1_ISP_LSC_LOAD_STATUS_LOAD_FULL_RESET 0x00000000U
#define FX1_ISP_LSC_LOAD_STATUS_LOAD_FULL_ACCESS FX1_ISP_ACCESS_RO
/**
 * load_busy [17:17], dòng bảng 104, kiểu truy cập RO
 * LOAD_BUSY=1 khi đang có chuỗi nạp.
 */
#define FX1_ISP_LSC_LOAD_STATUS_LOAD_BUSY_SHIFT 17U
#define FX1_ISP_LSC_LOAD_STATUS_LOAD_BUSY_WIDTH 1U
#define FX1_ISP_LSC_LOAD_STATUS_LOAD_BUSY_MASK 0x00020000U
#define FX1_ISP_LSC_LOAD_STATUS_LOAD_BUSY_RESET 0x00000000U
#define FX1_ISP_LSC_LOAD_STATUS_LOAD_BUSY_ACCESS FX1_ISP_ACCESS_RO
/**
 * load_profile [19:18], dòng bảng 105, kiểu truy cập RO
 * Chỉ số profile đang được nạp.
 */
#define FX1_ISP_LSC_LOAD_STATUS_LOAD_PROFILE_SHIFT 18U
#define FX1_ISP_LSC_LOAD_STATUS_LOAD_PROFILE_WIDTH 2U
#define FX1_ISP_LSC_LOAD_STATUS_LOAD_PROFILE_MASK 0x000C0000U
#define FX1_ISP_LSC_LOAD_STATUS_LOAD_PROFILE_RESET 0x00000000U
#define FX1_ISP_LSC_LOAD_STATUS_LOAD_PROFILE_ACCESS FX1_ISP_ACCESS_RO

/**
 * LSC_PROFILE_STATUS: địa chỉ lệch 0x041C, dòng bảng 106, kiểu truy cập RO
 * Các profile LSC có dữ liệu hợp lệ, chỉ đọc.
 */
#define FX1_ISP_LSC_PROFILE_STATUS_OFFSET 0x041CU
#define REG_LSC_PROFILE_STATUS FX1_ISP_LSC_PROFILE_STATUS_OFFSET
#define FX1_ISP_LSC_PROFILE_STATUS_RESET 0x00000000U
#define FX1_ISP_LSC_PROFILE_STATUS_ACCESS FX1_ISP_ACCESS_RO
#define FX1_ISP_LSC_PROFILE_STATUS_RO_MASK 0x000000F7U
#define FX1_ISP_LSC_PROFILE_STATUS_RW_MASK 0x00000000U
#define FX1_ISP_LSC_PROFILE_STATUS_W1C_MASK 0x00000000U
#define FX1_ISP_LSC_PROFILE_STATUS_W1S_MASK 0x00000000U
#define FX1_ISP_LSC_PROFILE_STATUS_W1P_MASK 0x00000000U
#define FX1_ISP_LSC_PROFILE_STATUS_W1SC_MASK 0x00000000U
/**
 * profile_valid [2:0], dòng bảng 107, kiểu truy cập RO
 * Mỗi bit xác nhận một profile đã nạp/kiểm tra: bit 0=PROFILE_0, bit 1=PROFILE_1, bit 2=PROFILE_2.
 */
#define FX1_ISP_LSC_PROFILE_STATUS_PROFILE_VALID_SHIFT 0U
#define FX1_ISP_LSC_PROFILE_STATUS_PROFILE_VALID_WIDTH 3U
#define FX1_ISP_LSC_PROFILE_STATUS_PROFILE_VALID_MASK 0x00000007U
#define FX1_ISP_LSC_PROFILE_STATUS_PROFILE_VALID_RESET 0x00000000U
#define FX1_ISP_LSC_PROFILE_STATUS_PROFILE_VALID_ACCESS FX1_ISP_ACCESS_RO
/**
 * active_profile [5:4], dòng bảng 108, kiểu truy cập RO
 * Profile pipeline đang sử dụng.
 */
#define FX1_ISP_LSC_PROFILE_STATUS_ACTIVE_PROFILE_SHIFT 4U
#define FX1_ISP_LSC_PROFILE_STATUS_ACTIVE_PROFILE_WIDTH 2U
#define FX1_ISP_LSC_PROFILE_STATUS_ACTIVE_PROFILE_MASK 0x00000030U
#define FX1_ISP_LSC_PROFILE_STATUS_ACTIVE_PROFILE_RESET 0x00000000U
#define FX1_ISP_LSC_PROFILE_STATUS_ACTIVE_PROFILE_ACCESS FX1_ISP_ACCESS_RO
/**
 * active_valid [6:6], dòng bảng 109, kiểu truy cập RO
 * ACTIVE_VALID=1 khi profile đang chọn có dữ liệu hợp lệ; nếu 0 thì không áp sửa dù LSC_CTRL.en bật.
 */
#define FX1_ISP_LSC_PROFILE_STATUS_ACTIVE_VALID_SHIFT 6U
#define FX1_ISP_LSC_PROFILE_STATUS_ACTIVE_VALID_WIDTH 1U
#define FX1_ISP_LSC_PROFILE_STATUS_ACTIVE_VALID_MASK 0x00000040U
#define FX1_ISP_LSC_PROFILE_STATUS_ACTIVE_VALID_RESET 0x00000000U
#define FX1_ISP_LSC_PROFILE_STATUS_ACTIVE_VALID_ACCESS FX1_ISP_ACCESS_RO
/**
 * geometry_valid [7:7], dòng bảng 110, kiểu truy cập RO
 * GEOM_OK=1 khi LSC_MESH_NODES mô tả lưới dùng được.
 */
#define FX1_ISP_LSC_PROFILE_STATUS_GEOMETRY_VALID_SHIFT 7U
#define FX1_ISP_LSC_PROFILE_STATUS_GEOMETRY_VALID_WIDTH 1U
#define FX1_ISP_LSC_PROFILE_STATUS_GEOMETRY_VALID_MASK 0x00000080U
#define FX1_ISP_LSC_PROFILE_STATUS_GEOMETRY_VALID_RESET 0x00000000U
#define FX1_ISP_LSC_PROFILE_STATUS_GEOMETRY_VALID_ACCESS FX1_ISP_ACCESS_RO

/**
 * LSC_ERROR: địa chỉ lệch 0x0420, dòng bảng 111, kiểu truy cập W1C
 * Các lỗi LSC được chốt, ghi 1 để xóa. Lỗi giữa chuỗi nạp dài phải giữ đến khi phần mềm kiểm tra.
 */
#define FX1_ISP_LSC_ERROR_OFFSET 0x0420U
#define REG_LSC_ERROR FX1_ISP_LSC_ERROR_OFFSET
#define FX1_ISP_LSC_ERROR_RESET 0x00000000U
#define FX1_ISP_LSC_ERROR_ACCESS FX1_ISP_ACCESS_W1C
#define FX1_ISP_LSC_ERROR_RO_MASK 0x00000000U
#define FX1_ISP_LSC_ERROR_RW_MASK 0x00000000U
#define FX1_ISP_LSC_ERROR_W1C_MASK 0x0000003FU
#define FX1_ISP_LSC_ERROR_W1S_MASK 0x00000000U
#define FX1_ISP_LSC_ERROR_W1P_MASK 0x00000000U
#define FX1_ISP_LSC_ERROR_W1SC_MASK 0x00000000U
/**
 * active_load_reject [0:0], dòng bảng 112, kiểu truy cập W1C
 * Đã nạp nhầm vào profile đang sử dụng.
 */
#define FX1_ISP_LSC_ERROR_ACTIVE_LOAD_REJECT_SHIFT 0U
#define FX1_ISP_LSC_ERROR_ACTIVE_LOAD_REJECT_WIDTH 1U
#define FX1_ISP_LSC_ERROR_ACTIVE_LOAD_REJECT_MASK 0x00000001U
#define FX1_ISP_LSC_ERROR_ACTIVE_LOAD_REJECT_RESET 0x00000000U
#define FX1_ISP_LSC_ERROR_ACTIVE_LOAD_REJECT_ACCESS FX1_ISP_ACCESS_W1C
/**
 * profile_sel_reject [1:1], dòng bảng 113, kiểu truy cập W1C
 * LSC_PROFILE_SEL chọn profile sai hoặc chưa nạp.
 */
#define FX1_ISP_LSC_ERROR_PROFILE_SEL_REJECT_SHIFT 1U
#define FX1_ISP_LSC_ERROR_PROFILE_SEL_REJECT_WIDTH 1U
#define FX1_ISP_LSC_ERROR_PROFILE_SEL_REJECT_MASK 0x00000002U
#define FX1_ISP_LSC_ERROR_PROFILE_SEL_REJECT_RESET 0x00000000U
#define FX1_ISP_LSC_ERROR_PROFILE_SEL_REJECT_ACCESS FX1_ISP_ACCESS_W1C
/**
 * coef_range [2:2], dòng bảng 114, kiểu truy cập W1C
 * Hệ số ghi hoặc phát hiện khi VALIDATE vượt dải 0..4 UQ3.18. Chuỗi giữ không hợp lệ đến BEGIN/ABORT
 * tiếp; xóa cờ không làm bảng hợp lệ. W1C; lỗi mới đồng thời được ưu tiên hơn xóa.
 */
#define FX1_ISP_LSC_ERROR_COEF_RANGE_SHIFT 2U
#define FX1_ISP_LSC_ERROR_COEF_RANGE_WIDTH 1U
#define FX1_ISP_LSC_ERROR_COEF_RANGE_MASK 0x00000004U
#define FX1_ISP_LSC_ERROR_COEF_RANGE_RESET 0x00000000U
#define FX1_ISP_LSC_ERROR_COEF_RANGE_ACCESS FX1_ISP_ACCESS_W1C
/**
 * coef_count [3:3], dòng bảng 115, kiểu truy cập W1C
 * Số hệ số tại VALIDATE khác 4*nx*ny, hoặc ghi thừa sau LOAD_FULL đã bị bỏ.
 */
#define FX1_ISP_LSC_ERROR_COEF_COUNT_SHIFT 3U
#define FX1_ISP_LSC_ERROR_COEF_COUNT_WIDTH 1U
#define FX1_ISP_LSC_ERROR_COEF_COUNT_MASK 0x00000008U
#define FX1_ISP_LSC_ERROR_COEF_COUNT_RESET 0x00000000U
#define FX1_ISP_LSC_ERROR_COEF_COUNT_ACCESS FX1_ISP_ACCESS_W1C
/**
 * geometry_invalid [4:4], dòng bảng 116, kiểu truy cập W1C
 * Hình học lưới vượt giới hạn số nút hỗ trợ.
 */
#define FX1_ISP_LSC_ERROR_GEOMETRY_INVALID_SHIFT 4U
#define FX1_ISP_LSC_ERROR_GEOMETRY_INVALID_WIDTH 1U
#define FX1_ISP_LSC_ERROR_GEOMETRY_INVALID_MASK 0x00000010U
#define FX1_ISP_LSC_ERROR_GEOMETRY_INVALID_RESET 0x00000000U
#define FX1_ISP_LSC_ERROR_GEOMETRY_INVALID_ACCESS FX1_ISP_ACCESS_W1C
/**
 * load_protocol [5:5], dòng bảng 117, kiểu truy cập W1C
 * Vi phạm thứ tự nạp: ghi dữ liệu khi chưa BEGIN/đã FULL, hoặc VALIDATE khi chưa BEGIN.
 */
#define FX1_ISP_LSC_ERROR_LOAD_PROTOCOL_SHIFT 5U
#define FX1_ISP_LSC_ERROR_LOAD_PROTOCOL_WIDTH 1U
#define FX1_ISP_LSC_ERROR_LOAD_PROTOCOL_MASK 0x00000020U
#define FX1_ISP_LSC_ERROR_LOAD_PROTOCOL_RESET 0x00000000U
#define FX1_ISP_LSC_ERROR_LOAD_PROTOCOL_ACCESS FX1_ISP_ACCESS_W1C

/**
 * LSC_STAT_OVF_CNT_R: địa chỉ lệch 0x0424, dòng bảng 118, kiểu truy cập RO
 * LSC: đếm mẫu màu tương ứng bị gain lưới đẩy tới bão hòa trong frame đã hoàn tất; số lớn cho thấy
 * vùng sáng bị nâng vượt cực đại.
 */
#define FX1_ISP_LSC_STAT_OVF_CNT_R_OFFSET 0x0424U
#define REG_LSC_STAT_OVF_CNT_R FX1_ISP_LSC_STAT_OVF_CNT_R_OFFSET
#define FX1_ISP_LSC_STAT_OVF_CNT_R_RESET 0x00000000U
#define FX1_ISP_LSC_STAT_OVF_CNT_R_ACCESS FX1_ISP_ACCESS_RO
#define FX1_ISP_LSC_STAT_OVF_CNT_R_RO_MASK 0x001FFFFFU
#define FX1_ISP_LSC_STAT_OVF_CNT_R_RW_MASK 0x00000000U
#define FX1_ISP_LSC_STAT_OVF_CNT_R_W1C_MASK 0x00000000U
#define FX1_ISP_LSC_STAT_OVF_CNT_R_W1S_MASK 0x00000000U
#define FX1_ISP_LSC_STAT_OVF_CNT_R_W1P_MASK 0x00000000U
#define FX1_ISP_LSC_STAT_OVF_CNT_R_W1SC_MASK 0x00000000U
/**
 * ovf_cnt_r [20:0], dòng bảng 119, kiểu truy cập RO
 * Số mẫu của màu tương ứng bị bão hòa trong frame hoàn tất gần nhất.
 */
#define FX1_ISP_LSC_STAT_OVF_CNT_R_OVF_CNT_R_SHIFT 0U
#define FX1_ISP_LSC_STAT_OVF_CNT_R_OVF_CNT_R_WIDTH 21U
#define FX1_ISP_LSC_STAT_OVF_CNT_R_OVF_CNT_R_MASK 0x001FFFFFU
#define FX1_ISP_LSC_STAT_OVF_CNT_R_OVF_CNT_R_RESET 0x00000000U
#define FX1_ISP_LSC_STAT_OVF_CNT_R_OVF_CNT_R_ACCESS FX1_ISP_ACCESS_RO

/**
 * LSC_STAT_OVF_CNT_GR: địa chỉ lệch 0x0428, dòng bảng 120, kiểu truy cập RO
 * LSC: đếm mẫu màu tương ứng bị gain lưới đẩy tới bão hòa trong frame đã hoàn tất; số lớn cho thấy
 * vùng sáng bị nâng vượt cực đại.
 */
#define FX1_ISP_LSC_STAT_OVF_CNT_GR_OFFSET 0x0428U
#define REG_LSC_STAT_OVF_CNT_GR FX1_ISP_LSC_STAT_OVF_CNT_GR_OFFSET
#define FX1_ISP_LSC_STAT_OVF_CNT_GR_RESET 0x00000000U
#define FX1_ISP_LSC_STAT_OVF_CNT_GR_ACCESS FX1_ISP_ACCESS_RO
#define FX1_ISP_LSC_STAT_OVF_CNT_GR_RO_MASK 0x001FFFFFU
#define FX1_ISP_LSC_STAT_OVF_CNT_GR_RW_MASK 0x00000000U
#define FX1_ISP_LSC_STAT_OVF_CNT_GR_W1C_MASK 0x00000000U
#define FX1_ISP_LSC_STAT_OVF_CNT_GR_W1S_MASK 0x00000000U
#define FX1_ISP_LSC_STAT_OVF_CNT_GR_W1P_MASK 0x00000000U
#define FX1_ISP_LSC_STAT_OVF_CNT_GR_W1SC_MASK 0x00000000U
/**
 * ovf_cnt_gr [20:0], dòng bảng 121, kiểu truy cập RO
 * Số mẫu của màu tương ứng bị bão hòa trong frame hoàn tất gần nhất.
 */
#define FX1_ISP_LSC_STAT_OVF_CNT_GR_OVF_CNT_GR_SHIFT 0U
#define FX1_ISP_LSC_STAT_OVF_CNT_GR_OVF_CNT_GR_WIDTH 21U
#define FX1_ISP_LSC_STAT_OVF_CNT_GR_OVF_CNT_GR_MASK 0x001FFFFFU
#define FX1_ISP_LSC_STAT_OVF_CNT_GR_OVF_CNT_GR_RESET 0x00000000U
#define FX1_ISP_LSC_STAT_OVF_CNT_GR_OVF_CNT_GR_ACCESS FX1_ISP_ACCESS_RO

/**
 * LSC_STAT_OVF_CNT_GB: địa chỉ lệch 0x042C, dòng bảng 122, kiểu truy cập RO
 * LSC: đếm mẫu màu tương ứng bị gain lưới đẩy tới bão hòa trong frame đã hoàn tất; số lớn cho thấy
 * vùng sáng bị nâng vượt cực đại.
 */
#define FX1_ISP_LSC_STAT_OVF_CNT_GB_OFFSET 0x042CU
#define REG_LSC_STAT_OVF_CNT_GB FX1_ISP_LSC_STAT_OVF_CNT_GB_OFFSET
#define FX1_ISP_LSC_STAT_OVF_CNT_GB_RESET 0x00000000U
#define FX1_ISP_LSC_STAT_OVF_CNT_GB_ACCESS FX1_ISP_ACCESS_RO
#define FX1_ISP_LSC_STAT_OVF_CNT_GB_RO_MASK 0x001FFFFFU
#define FX1_ISP_LSC_STAT_OVF_CNT_GB_RW_MASK 0x00000000U
#define FX1_ISP_LSC_STAT_OVF_CNT_GB_W1C_MASK 0x00000000U
#define FX1_ISP_LSC_STAT_OVF_CNT_GB_W1S_MASK 0x00000000U
#define FX1_ISP_LSC_STAT_OVF_CNT_GB_W1P_MASK 0x00000000U
#define FX1_ISP_LSC_STAT_OVF_CNT_GB_W1SC_MASK 0x00000000U
/**
 * ovf_cnt_gb [20:0], dòng bảng 123, kiểu truy cập RO
 * Số mẫu của màu tương ứng bị bão hòa trong frame hoàn tất gần nhất.
 */
#define FX1_ISP_LSC_STAT_OVF_CNT_GB_OVF_CNT_GB_SHIFT 0U
#define FX1_ISP_LSC_STAT_OVF_CNT_GB_OVF_CNT_GB_WIDTH 21U
#define FX1_ISP_LSC_STAT_OVF_CNT_GB_OVF_CNT_GB_MASK 0x001FFFFFU
#define FX1_ISP_LSC_STAT_OVF_CNT_GB_OVF_CNT_GB_RESET 0x00000000U
#define FX1_ISP_LSC_STAT_OVF_CNT_GB_OVF_CNT_GB_ACCESS FX1_ISP_ACCESS_RO

/**
 * LSC_STAT_OVF_CNT_B: địa chỉ lệch 0x0430, dòng bảng 124, kiểu truy cập RO
 * LSC: đếm mẫu màu tương ứng bị gain lưới đẩy tới bão hòa trong frame đã hoàn tất; số lớn cho thấy
 * vùng sáng bị nâng vượt cực đại.
 */
#define FX1_ISP_LSC_STAT_OVF_CNT_B_OFFSET 0x0430U
#define REG_LSC_STAT_OVF_CNT_B FX1_ISP_LSC_STAT_OVF_CNT_B_OFFSET
#define FX1_ISP_LSC_STAT_OVF_CNT_B_RESET 0x00000000U
#define FX1_ISP_LSC_STAT_OVF_CNT_B_ACCESS FX1_ISP_ACCESS_RO
#define FX1_ISP_LSC_STAT_OVF_CNT_B_RO_MASK 0x001FFFFFU
#define FX1_ISP_LSC_STAT_OVF_CNT_B_RW_MASK 0x00000000U
#define FX1_ISP_LSC_STAT_OVF_CNT_B_W1C_MASK 0x00000000U
#define FX1_ISP_LSC_STAT_OVF_CNT_B_W1S_MASK 0x00000000U
#define FX1_ISP_LSC_STAT_OVF_CNT_B_W1P_MASK 0x00000000U
#define FX1_ISP_LSC_STAT_OVF_CNT_B_W1SC_MASK 0x00000000U
/**
 * ovf_cnt_b [20:0], dòng bảng 125, kiểu truy cập RO
 * Số mẫu của màu tương ứng bị bão hòa trong frame hoàn tất gần nhất.
 */
#define FX1_ISP_LSC_STAT_OVF_CNT_B_OVF_CNT_B_SHIFT 0U
#define FX1_ISP_LSC_STAT_OVF_CNT_B_OVF_CNT_B_WIDTH 21U
#define FX1_ISP_LSC_STAT_OVF_CNT_B_OVF_CNT_B_MASK 0x001FFFFFU
#define FX1_ISP_LSC_STAT_OVF_CNT_B_OVF_CNT_B_RESET 0x00000000U
#define FX1_ISP_LSC_STAT_OVF_CNT_B_OVF_CNT_B_ACCESS FX1_ISP_ACCESS_RO

/**
 * BPC_CTRL: địa chỉ lệch 0x0600, dòng bảng 126, kiểu truy cập RW
 * BPC: sửa pixel sai lệch so với các láng giềng cùng màu.
 */
#define FX1_ISP_BPC_CTRL_OFFSET 0x0600U
#define REG_BPC_CTRL FX1_ISP_BPC_CTRL_OFFSET
#define FX1_ISP_BPC_CTRL_RESET 0x00000000U
#define FX1_ISP_BPC_CTRL_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_BPC_CTRL_RO_MASK 0x00000000U
#define FX1_ISP_BPC_CTRL_RW_MASK 0x00000001U
#define FX1_ISP_BPC_CTRL_W1C_MASK 0x00000000U
#define FX1_ISP_BPC_CTRL_W1S_MASK 0x00000000U
#define FX1_ISP_BPC_CTRL_W1P_MASK 0x00000000U
#define FX1_ISP_BPC_CTRL_W1SC_MASK 0x00000000U
/**
 * en [0:0], dòng bảng 127, kiểu truy cập RW
 * Bật BPC: 1 phát hiện/sửa, 0 truyền nguyên.
 */
#define FX1_ISP_BPC_CTRL_EN_SHIFT 0U
#define FX1_ISP_BPC_CTRL_EN_WIDTH 1U
#define FX1_ISP_BPC_CTRL_EN_MASK 0x00000001U
#define FX1_ISP_BPC_CTRL_EN_RESET 0x00000000U
#define FX1_ISP_BPC_CTRL_EN_ACCESS FX1_ISP_ACCESS_RW

/**
 * BPC_MODE: địa chỉ lệch 0x0604, dòng bảng 128, kiểu truy cập RW
 * BPC_MODE chỉ giữ tương thích. Đặc tả chỉ có BPC không gian sửa ngay, không giữ lịch sử pixel lỗi
 * qua frame; giá trị ghi không bật phát hiện theo thời gian.
 */
#define FX1_ISP_BPC_MODE_OFFSET 0x0604U
#define REG_BPC_MODE FX1_ISP_BPC_MODE_OFFSET
#define FX1_ISP_BPC_MODE_RESET 0x00000001U
#define FX1_ISP_BPC_MODE_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_BPC_MODE_RO_MASK 0x00000000U
#define FX1_ISP_BPC_MODE_RW_MASK 0x00000001U
#define FX1_ISP_BPC_MODE_W1C_MASK 0x00000000U
#define FX1_ISP_BPC_MODE_W1S_MASK 0x00000000U
#define FX1_ISP_BPC_MODE_W1P_MASK 0x00000000U
#define FX1_ISP_BPC_MODE_W1SC_MASK 0x00000000U
/**
 * dynamic_det_en [0:0], dòng bảng 129, kiểu truy cập RW
 * Trường bật phát hiện động dự phòng, không tác động BPC không gian của đặc tả này.
 */
#define FX1_ISP_BPC_MODE_DYNAMIC_DET_EN_SHIFT 0U
#define FX1_ISP_BPC_MODE_DYNAMIC_DET_EN_WIDTH 1U
#define FX1_ISP_BPC_MODE_DYNAMIC_DET_EN_MASK 0x00000001U
#define FX1_ISP_BPC_MODE_DYNAMIC_DET_EN_RESET 0x00000001U
#define FX1_ISP_BPC_MODE_DYNAMIC_DET_EN_ACCESS FX1_ISP_ACCESS_RW

/**
 * BPC_TEMPORAL_VAR: địa chỉ lệch 0x0608, dòng bảng 130, kiểu truy cập RW
 * BPC_TEMPORAL_VAR chỉ giữ tương thích; không có lịch sử lỗi qua frame hoặc phát hiện theo thời gian
 * dù giá trị ghi được lưu.
 */
#define FX1_ISP_BPC_TEMPORAL_VAR_OFFSET 0x0608U
#define REG_BPC_TEMPORAL_VAR FX1_ISP_BPC_TEMPORAL_VAR_OFFSET
#define FX1_ISP_BPC_TEMPORAL_VAR_RESET 0x00000100U
#define FX1_ISP_BPC_TEMPORAL_VAR_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_BPC_TEMPORAL_VAR_RO_MASK 0x00000000U
#define FX1_ISP_BPC_TEMPORAL_VAR_RW_MASK 0x0000FFFFU
#define FX1_ISP_BPC_TEMPORAL_VAR_W1C_MASK 0x00000000U
#define FX1_ISP_BPC_TEMPORAL_VAR_W1S_MASK 0x00000000U
#define FX1_ISP_BPC_TEMPORAL_VAR_W1P_MASK 0x00000000U
#define FX1_ISP_BPC_TEMPORAL_VAR_W1SC_MASK 0x00000000U
/**
 * temporal_var [15:0], dòng bảng 131, kiểu truy cập RW
 * Dung sai giữa frame dự phòng, không tác động BPC không gian hiện tại.
 */
#define FX1_ISP_BPC_TEMPORAL_VAR_TEMPORAL_VAR_SHIFT 0U
#define FX1_ISP_BPC_TEMPORAL_VAR_TEMPORAL_VAR_WIDTH 16U
#define FX1_ISP_BPC_TEMPORAL_VAR_TEMPORAL_VAR_MASK 0x0000FFFFU
#define FX1_ISP_BPC_TEMPORAL_VAR_TEMPORAL_VAR_RESET 0x00000100U
#define FX1_ISP_BPC_TEMPORAL_VAR_TEMPORAL_VAR_ACCESS FX1_ISP_ACCESS_RW

/**
 * BPC_THRESH: địa chỉ lệch 0x060C, dòng bảng 132, kiểu truy cập RW
 * Ngưỡng BPC: ứng viên khi lệch trung bình láng giềng lớn hơn max(THRESH_FLOOR, THRESH_K*activity).
 * activity là độ phân tán của tám láng giềng cùng màu; thành phần tương đối tránh sửa quá mức vùng
 * nhiều chi tiết, ngưỡng sàn luôn áp dụng.
 */
#define FX1_ISP_BPC_THRESH_OFFSET 0x060CU
#define REG_BPC_THRESH FX1_ISP_BPC_THRESH_OFFSET
#define FX1_ISP_BPC_THRESH_RESET 0x004D0192U
#define FX1_ISP_BPC_THRESH_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_BPC_THRESH_RO_MASK 0x00000000U
#define FX1_ISP_BPC_THRESH_RW_MASK 0xFFFFFFFFU
#define FX1_ISP_BPC_THRESH_W1C_MASK 0x00000000U
#define FX1_ISP_BPC_THRESH_W1S_MASK 0x00000000U
#define FX1_ISP_BPC_THRESH_W1P_MASK 0x00000000U
#define FX1_ISP_BPC_THRESH_W1SC_MASK 0x00000000U
/**
 * thresh_k [31:16], dòng bảng 133, kiểu truy cập RW
 * Hệ số ngưỡng tương đối UQ8.8: 256=1x, sau reset 77 xấp xỉ 0,30. Giá trị 0 tắt thành phần tương
 * đối, chỉ giữ sàn.
 */
#define FX1_ISP_BPC_THRESH_THRESH_K_SHIFT 16U
#define FX1_ISP_BPC_THRESH_THRESH_K_WIDTH 16U
#define FX1_ISP_BPC_THRESH_THRESH_K_MASK 0xFFFF0000U
#define FX1_ISP_BPC_THRESH_THRESH_K_RESET 0x0000004DU
#define FX1_ISP_BPC_THRESH_THRESH_K_ACCESS FX1_ISP_ACCESS_RW
/**
 * thresh_floor [15:0], dòng bảng 134, kiểu truy cập RW
 * Ngưỡng sàn tuyệt đối theo mã pixel 12 bit của pipeline, luôn áp dụng.
 */
#define FX1_ISP_BPC_THRESH_THRESH_FLOOR_SHIFT 0U
#define FX1_ISP_BPC_THRESH_THRESH_FLOOR_WIDTH 16U
#define FX1_ISP_BPC_THRESH_THRESH_FLOOR_MASK 0x0000FFFFU
#define FX1_ISP_BPC_THRESH_THRESH_FLOOR_RESET 0x00000192U
#define FX1_ISP_BPC_THRESH_THRESH_FLOOR_ACCESS FX1_ISP_ACCESS_RW

/**
 * BPC_PIXEL_AGE: địa chỉ lệch 0x0610, dòng bảng 135, kiểu truy cập RW
 * BPC_PIXEL_AGE chỉ lưu giá trị tương thích; không giữ lịch sử lỗi hoặc bật phát hiện theo thời
 * gian.
 */
#define FX1_ISP_BPC_PIXEL_AGE_OFFSET 0x0610U
#define REG_BPC_PIXEL_AGE FX1_ISP_BPC_PIXEL_AGE_OFFSET
#define FX1_ISP_BPC_PIXEL_AGE_RESET 0x000004B0U
#define FX1_ISP_BPC_PIXEL_AGE_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_BPC_PIXEL_AGE_RO_MASK 0x00000000U
#define FX1_ISP_BPC_PIXEL_AGE_RW_MASK 0x0000FFFFU
#define FX1_ISP_BPC_PIXEL_AGE_W1C_MASK 0x00000000U
#define FX1_ISP_BPC_PIXEL_AGE_W1S_MASK 0x00000000U
#define FX1_ISP_BPC_PIXEL_AGE_W1P_MASK 0x00000000U
#define FX1_ISP_BPC_PIXEL_AGE_W1SC_MASK 0x00000000U
/**
 * pixel_age [15:0], dòng bảng 136, kiểu truy cập RW
 * Số frame xác nhận là trường dự phòng, không ảnh hưởng BPC không gian hiện tại.
 */
#define FX1_ISP_BPC_PIXEL_AGE_PIXEL_AGE_SHIFT 0U
#define FX1_ISP_BPC_PIXEL_AGE_PIXEL_AGE_WIDTH 16U
#define FX1_ISP_BPC_PIXEL_AGE_PIXEL_AGE_MASK 0x0000FFFFU
#define FX1_ISP_BPC_PIXEL_AGE_PIXEL_AGE_RESET 0x000004B0U
#define FX1_ISP_BPC_PIXEL_AGE_PIXEL_AGE_ACCESS FX1_ISP_ACCESS_RW

/**
 * BPC_NUM_CANDIDATES: địa chỉ lệch 0x0614, dòng bảng 137, kiểu truy cập RO
 * Số ứng viên BPC theo frame, chỉ đọc; kẹp tối đa 16383.
 */
#define FX1_ISP_BPC_NUM_CANDIDATES_OFFSET 0x0614U
#define REG_BPC_NUM_CANDIDATES FX1_ISP_BPC_NUM_CANDIDATES_OFFSET
#define FX1_ISP_BPC_NUM_CANDIDATES_RESET 0x00000000U
#define FX1_ISP_BPC_NUM_CANDIDATES_ACCESS FX1_ISP_ACCESS_RO
#define FX1_ISP_BPC_NUM_CANDIDATES_RO_MASK 0x00003FFFU
#define FX1_ISP_BPC_NUM_CANDIDATES_RW_MASK 0x00000000U
#define FX1_ISP_BPC_NUM_CANDIDATES_W1C_MASK 0x00000000U
#define FX1_ISP_BPC_NUM_CANDIDATES_W1S_MASK 0x00000000U
#define FX1_ISP_BPC_NUM_CANDIDATES_W1P_MASK 0x00000000U
#define FX1_ISP_BPC_NUM_CANDIDATES_W1SC_MASK 0x00000000U
/**
 * candidates [13:0], dòng bảng 138, kiểu truy cập RO
 * Đếm ứng viên frame hoàn tất gần nhất: sai lệch tuyệt đối so với trung bình cắt phần lẻ của tám
 * láng giềng phải vượt sàn. Dùng THRESH_FLOOR hoặc sàn theo màu khi COLOR_THRESH_EN=1. Công bố cuối
 * frame, giữ đến frame sau hoàn tất; kẹp 16383.
 */
#define FX1_ISP_BPC_NUM_CANDIDATES_CANDIDATES_SHIFT 0U
#define FX1_ISP_BPC_NUM_CANDIDATES_CANDIDATES_WIDTH 14U
#define FX1_ISP_BPC_NUM_CANDIDATES_CANDIDATES_MASK 0x00003FFFU
#define FX1_ISP_BPC_NUM_CANDIDATES_CANDIDATES_RESET 0x00000000U
#define FX1_ISP_BPC_NUM_CANDIDATES_CANDIDATES_ACCESS FX1_ISP_ACCESS_RO

/**
 * BPC_NUM_DEFECTIVE: địa chỉ lệch 0x0618, dòng bảng 139, kiểu truy cập RO
 * Số pixel BPC đã sửa theo frame, chỉ đọc; kẹp tối đa 16383.
 */
#define FX1_ISP_BPC_NUM_DEFECTIVE_OFFSET 0x0618U
#define REG_BPC_NUM_DEFECTIVE FX1_ISP_BPC_NUM_DEFECTIVE_OFFSET
#define FX1_ISP_BPC_NUM_DEFECTIVE_RESET 0x00000000U
#define FX1_ISP_BPC_NUM_DEFECTIVE_ACCESS FX1_ISP_ACCESS_RO
#define FX1_ISP_BPC_NUM_DEFECTIVE_RO_MASK 0x00003FFFU
#define FX1_ISP_BPC_NUM_DEFECTIVE_RW_MASK 0x00000000U
#define FX1_ISP_BPC_NUM_DEFECTIVE_W1C_MASK 0x00000000U
#define FX1_ISP_BPC_NUM_DEFECTIVE_W1S_MASK 0x00000000U
#define FX1_ISP_BPC_NUM_DEFECTIVE_W1P_MASK 0x00000000U
#define FX1_ISP_BPC_NUM_DEFECTIVE_W1SC_MASK 0x00000000U
/**
 * defective [13:0], dòng bảng 140, kiểu truy cập RO
 * Số pixel đã sửa của frame hoàn tất gần nhất, gồm cả vị trí trong danh sách lỗi nhà máy. Công bố
 * cùng số ứng viên ở cuối frame, giữ đến frame sau; kẹp 16383.
 */
#define FX1_ISP_BPC_NUM_DEFECTIVE_DEFECTIVE_SHIFT 0U
#define FX1_ISP_BPC_NUM_DEFECTIVE_DEFECTIVE_WIDTH 14U
#define FX1_ISP_BPC_NUM_DEFECTIVE_DEFECTIVE_MASK 0x00003FFFU
#define FX1_ISP_BPC_NUM_DEFECTIVE_DEFECTIVE_RESET 0x00000000U
#define FX1_ISP_BPC_NUM_DEFECTIVE_DEFECTIVE_ACCESS FX1_ISP_ACCESS_RO

/**
 * BPC_STATUS: địa chỉ lệch 0x061C, dòng bảng 141, kiểu truy cập W1C
 * Trạng thái BPC đã chốt; ghi 1 để xóa.
 */
#define FX1_ISP_BPC_STATUS_OFFSET 0x061CU
#define REG_BPC_STATUS FX1_ISP_BPC_STATUS_OFFSET
#define FX1_ISP_BPC_STATUS_RESET 0x00000000U
#define FX1_ISP_BPC_STATUS_ACCESS FX1_ISP_ACCESS_W1C
#define FX1_ISP_BPC_STATUS_RO_MASK 0x00000000U
#define FX1_ISP_BPC_STATUS_RW_MASK 0x00000000U
#define FX1_ISP_BPC_STATUS_W1C_MASK 0x00000001U
#define FX1_ISP_BPC_STATUS_W1S_MASK 0x00000000U
#define FX1_ISP_BPC_STATUS_W1P_MASK 0x00000000U
#define FX1_ISP_BPC_STATUS_W1SC_MASK 0x00000000U
/**
 * cand_reject_ovf [0:0], dòng bảng 142, kiểu truy cập W1C
 * Cờ tràn bảng ứng viên dành cho cơ chế theo thời gian chưa triển khai. BPC không gian không giữ
 * bảng giữa frame; các bộ đếm âm thầm kẹp 16383 mà không đặt cờ này.
 */
#define FX1_ISP_BPC_STATUS_CAND_REJECT_OVF_SHIFT 0U
#define FX1_ISP_BPC_STATUS_CAND_REJECT_OVF_WIDTH 1U
#define FX1_ISP_BPC_STATUS_CAND_REJECT_OVF_MASK 0x00000001U
#define FX1_ISP_BPC_STATUS_CAND_REJECT_OVF_RESET 0x00000000U
#define FX1_ISP_BPC_STATUS_CAND_REJECT_OVF_ACCESS FX1_ISP_ACCESS_W1C

/**
 * WB_CTRL: địa chỉ lệch 0x0800, dòng bảng 143, kiểu truy cập RW
 * WB: nhân gain từng màu Bayer để vật trung tính cho mức R,G,B bằng nhau.
 */
#define FX1_ISP_WB_CTRL_OFFSET 0x0800U
#define REG_WB_CTRL FX1_ISP_WB_CTRL_OFFSET
#define FX1_ISP_WB_CTRL_RESET 0x00000000U
#define FX1_ISP_WB_CTRL_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_WB_CTRL_RO_MASK 0x00000000U
#define FX1_ISP_WB_CTRL_RW_MASK 0x00000001U
#define FX1_ISP_WB_CTRL_W1C_MASK 0x00000000U
#define FX1_ISP_WB_CTRL_W1S_MASK 0x00000000U
#define FX1_ISP_WB_CTRL_W1P_MASK 0x00000000U
#define FX1_ISP_WB_CTRL_W1SC_MASK 0x00000000U
/**
 * en [0:0], dòng bảng 144, kiểu truy cập RW
 * Bật WB: 1 áp gain từng màu, 0 truyền nguyên.
 */
#define FX1_ISP_WB_CTRL_EN_SHIFT 0U
#define FX1_ISP_WB_CTRL_EN_WIDTH 1U
#define FX1_ISP_WB_CTRL_EN_MASK 0x00000001U
#define FX1_ISP_WB_CTRL_EN_RESET 0x00000000U
#define FX1_ISP_WB_CTRL_EN_ACCESS FX1_ISP_ACCESS_RW

/**
 * WB_GAIN_R: địa chỉ lệch 0x0804, dòng bảng 145, kiểu truy cập RW
 * Gain màu đỏ của WB.
 */
#define FX1_ISP_WB_GAIN_R_OFFSET 0x0804U
#define REG_WB_GAIN_R FX1_ISP_WB_GAIN_R_OFFSET
#define FX1_ISP_WB_GAIN_R_RESET 0x00000100U
#define FX1_ISP_WB_GAIN_R_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_WB_GAIN_R_RO_MASK 0x00000000U
#define FX1_ISP_WB_GAIN_R_RW_MASK 0x00000FFFU
#define FX1_ISP_WB_GAIN_R_W1C_MASK 0x00000000U
#define FX1_ISP_WB_GAIN_R_W1S_MASK 0x00000000U
#define FX1_ISP_WB_GAIN_R_W1P_MASK 0x00000000U
#define FX1_ISP_WB_GAIN_R_W1SC_MASK 0x00000000U
/**
 * gain_r [11:0], dòng bảng 146, kiểu truy cập RW
 * Gain R UQ4.8, 0x100=1x. Legacy dùng WB_GAIN_G cho cả hai màu xanh lá; profile mode dùng WB_GAIN_G
 * cho Gr, TUNE_WB_GB cho Gb.
 */
#define FX1_ISP_WB_GAIN_R_GAIN_R_SHIFT 0U
#define FX1_ISP_WB_GAIN_R_GAIN_R_WIDTH 12U
#define FX1_ISP_WB_GAIN_R_GAIN_R_MASK 0x00000FFFU
#define FX1_ISP_WB_GAIN_R_GAIN_R_RESET 0x00000100U
#define FX1_ISP_WB_GAIN_R_GAIN_R_ACCESS FX1_ISP_ACCESS_RW

/**
 * WB_GAIN_G: địa chỉ lệch 0x0808, dòng bảng 147, kiểu truy cập RW
 * Gain G dùng chung Gr/Gb ở legacy; chỉ Gr ở profile mode.
 */
#define FX1_ISP_WB_GAIN_G_OFFSET 0x0808U
#define REG_WB_GAIN_G FX1_ISP_WB_GAIN_G_OFFSET
#define FX1_ISP_WB_GAIN_G_RESET 0x00000100U
#define FX1_ISP_WB_GAIN_G_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_WB_GAIN_G_RO_MASK 0x00000000U
#define FX1_ISP_WB_GAIN_G_RW_MASK 0x00000FFFU
#define FX1_ISP_WB_GAIN_G_W1C_MASK 0x00000000U
#define FX1_ISP_WB_GAIN_G_W1S_MASK 0x00000000U
#define FX1_ISP_WB_GAIN_G_W1P_MASK 0x00000000U
#define FX1_ISP_WB_GAIN_G_W1SC_MASK 0x00000000U
/**
 * gain_g [11:0], dòng bảng 148, kiểu truy cập RW
 * Gain G UQ4.8, 0x100=1x. Legacy áp cho Gr và Gb; profile mode áp cho Gr, còn Gb dùng TUNE_WB_GB
 * riêng.
 */
#define FX1_ISP_WB_GAIN_G_GAIN_G_SHIFT 0U
#define FX1_ISP_WB_GAIN_G_GAIN_G_WIDTH 12U
#define FX1_ISP_WB_GAIN_G_GAIN_G_MASK 0x00000FFFU
#define FX1_ISP_WB_GAIN_G_GAIN_G_RESET 0x00000100U
#define FX1_ISP_WB_GAIN_G_GAIN_G_ACCESS FX1_ISP_ACCESS_RW

/**
 * WB_GAIN_B: địa chỉ lệch 0x080C, dòng bảng 149, kiểu truy cập RW
 * Gain màu xanh dương của WB.
 */
#define FX1_ISP_WB_GAIN_B_OFFSET 0x080CU
#define REG_WB_GAIN_B FX1_ISP_WB_GAIN_B_OFFSET
#define FX1_ISP_WB_GAIN_B_RESET 0x00000100U
#define FX1_ISP_WB_GAIN_B_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_WB_GAIN_B_RO_MASK 0x00000000U
#define FX1_ISP_WB_GAIN_B_RW_MASK 0x00000FFFU
#define FX1_ISP_WB_GAIN_B_W1C_MASK 0x00000000U
#define FX1_ISP_WB_GAIN_B_W1S_MASK 0x00000000U
#define FX1_ISP_WB_GAIN_B_W1P_MASK 0x00000000U
#define FX1_ISP_WB_GAIN_B_W1SC_MASK 0x00000000U
/**
 * gain_b [11:0], dòng bảng 150, kiểu truy cập RW
 * Gain B UQ4.8, 0x100=1x. Legacy dùng chung WB_GAIN_G cho hai màu xanh lá; profile mode tách Gr và
 * Gb qua TUNE_WB_GB.
 */
#define FX1_ISP_WB_GAIN_B_GAIN_B_SHIFT 0U
#define FX1_ISP_WB_GAIN_B_GAIN_B_WIDTH 12U
#define FX1_ISP_WB_GAIN_B_GAIN_B_MASK 0x00000FFFU
#define FX1_ISP_WB_GAIN_B_GAIN_B_RESET 0x00000100U
#define FX1_ISP_WB_GAIN_B_GAIN_B_ACCESS FX1_ISP_ACCESS_RW

/**
 * DG_CTRL: địa chỉ lệch 0x0A00, dòng bảng 151, kiểu truy cập RW
 * DG: hệ số nhân chung sau WB. Khác analog gain cảm biến, DG nhân cả nhiễu lẫn tín hiệu nên không
 * cải thiện tỷ số tín hiệu/nhiễu; ưu tiên tăng gain cảm biến trước khi cần.
 */
#define FX1_ISP_DG_CTRL_OFFSET 0x0A00U
#define REG_DG_CTRL FX1_ISP_DG_CTRL_OFFSET
#define FX1_ISP_DG_CTRL_RESET 0x00000000U
#define FX1_ISP_DG_CTRL_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_DG_CTRL_RO_MASK 0x00000000U
#define FX1_ISP_DG_CTRL_RW_MASK 0x00000001U
#define FX1_ISP_DG_CTRL_W1C_MASK 0x00000000U
#define FX1_ISP_DG_CTRL_W1S_MASK 0x00000000U
#define FX1_ISP_DG_CTRL_W1P_MASK 0x00000000U
#define FX1_ISP_DG_CTRL_W1SC_MASK 0x00000000U
/**
 * en [0:0], dòng bảng 152, kiểu truy cập RW
 * Bật DG: 1 áp gain, 0 truyền nguyên.
 */
#define FX1_ISP_DG_CTRL_EN_SHIFT 0U
#define FX1_ISP_DG_CTRL_EN_WIDTH 1U
#define FX1_ISP_DG_CTRL_EN_MASK 0x00000001U
#define FX1_ISP_DG_CTRL_EN_RESET 0x00000000U
#define FX1_ISP_DG_CTRL_EN_ACCESS FX1_ISP_ACCESS_RW

/**
 * DG_GAIN: địa chỉ lệch 0x0A04, dòng bảng 153, kiểu truy cập RW
 * Gain số toàn ảnh của DG.
 */
#define FX1_ISP_DG_GAIN_OFFSET 0x0A04U
#define REG_DG_GAIN FX1_ISP_DG_GAIN_OFFSET
#define FX1_ISP_DG_GAIN_RESET 0x00000100U
#define FX1_ISP_DG_GAIN_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_DG_GAIN_RO_MASK 0x00000000U
#define FX1_ISP_DG_GAIN_RW_MASK 0x00000FFFU
#define FX1_ISP_DG_GAIN_W1C_MASK 0x00000000U
#define FX1_ISP_DG_GAIN_W1S_MASK 0x00000000U
#define FX1_ISP_DG_GAIN_W1P_MASK 0x00000000U
#define FX1_ISP_DG_GAIN_W1SC_MASK 0x00000000U
/**
 * gain [11:0], dòng bảng 154, kiểu truy cập RW
 * Gain UQ4.8, 0x100=1x; cực đại 0xFFF nhỏ hơn 16x một bước.
 */
#define FX1_ISP_DG_GAIN_GAIN_SHIFT 0U
#define FX1_ISP_DG_GAIN_GAIN_WIDTH 12U
#define FX1_ISP_DG_GAIN_GAIN_MASK 0x00000FFFU
#define FX1_ISP_DG_GAIN_GAIN_RESET 0x00000100U
#define FX1_ISP_DG_GAIN_GAIN_ACCESS FX1_ISP_ACCESS_RW

/**
 * D_WDR_CTRL: địa chỉ lệch 0x0C00, dòng bảng 155, kiểu truy cập RW
 * D_WDR dự phòng cho chức năng tương lai; có thanh ghi lưu nhưng ghi không ảnh hưởng xử lý ảnh trong
 * đặc tả này.
 */
#define FX1_ISP_D_WDR_CTRL_OFFSET 0x0C00U
#define REG_D_WDR_CTRL FX1_ISP_D_WDR_CTRL_OFFSET
#define FX1_ISP_D_WDR_CTRL_RESET 0x00000000U
#define FX1_ISP_D_WDR_CTRL_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_D_WDR_CTRL_RO_MASK 0x00000000U
#define FX1_ISP_D_WDR_CTRL_RW_MASK 0x00000001U
#define FX1_ISP_D_WDR_CTRL_W1C_MASK 0x00000000U
#define FX1_ISP_D_WDR_CTRL_W1S_MASK 0x00000000U
#define FX1_ISP_D_WDR_CTRL_W1P_MASK 0x00000000U
#define FX1_ISP_D_WDR_CTRL_W1SC_MASK 0x00000000U
/**
 * en [0:0], dòng bảng 156, kiểu truy cập RW
 * Bit bật Digital WDR chưa triển khai, không tác động xử lý.
 */
#define FX1_ISP_D_WDR_CTRL_EN_SHIFT 0U
#define FX1_ISP_D_WDR_CTRL_EN_WIDTH 1U
#define FX1_ISP_D_WDR_CTRL_EN_MASK 0x00000001U
#define FX1_ISP_D_WDR_CTRL_EN_RESET 0x00000000U
#define FX1_ISP_D_WDR_CTRL_EN_ACCESS FX1_ISP_ACCESS_RW

/**
 * D_WDR_STRENGTH: địa chỉ lệch 0x0C04, dòng bảng 157, kiểu truy cập RW
 * Cường độ nén tông D_WDR chưa triển khai; xem D_WDR_CTRL.
 */
#define FX1_ISP_D_WDR_STRENGTH_OFFSET 0x0C04U
#define REG_D_WDR_STRENGTH FX1_ISP_D_WDR_STRENGTH_OFFSET
#define FX1_ISP_D_WDR_STRENGTH_RESET 0x00000000U
#define FX1_ISP_D_WDR_STRENGTH_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_D_WDR_STRENGTH_RO_MASK 0x00000000U
#define FX1_ISP_D_WDR_STRENGTH_RW_MASK 0x000000FFU
#define FX1_ISP_D_WDR_STRENGTH_W1C_MASK 0x00000000U
#define FX1_ISP_D_WDR_STRENGTH_W1S_MASK 0x00000000U
#define FX1_ISP_D_WDR_STRENGTH_W1P_MASK 0x00000000U
#define FX1_ISP_D_WDR_STRENGTH_W1SC_MASK 0x00000000U
/**
 * strength [7:0], dòng bảng 158, kiểu truy cập RW
 * Cường độ nén tông dự phòng, không tác động xử lý.
 */
#define FX1_ISP_D_WDR_STRENGTH_STRENGTH_SHIFT 0U
#define FX1_ISP_D_WDR_STRENGTH_STRENGTH_WIDTH 8U
#define FX1_ISP_D_WDR_STRENGTH_STRENGTH_MASK 0x000000FFU
#define FX1_ISP_D_WDR_STRENGTH_STRENGTH_RESET 0x00000000U
#define FX1_ISP_D_WDR_STRENGTH_STRENGTH_ACCESS FX1_ISP_ACCESS_RW

/**
 * DEMOSAIC_STATUS: địa chỉ lệch 0x0E00, dòng bảng 159, kiểu truy cập RO
 * Trạng thái khối Demosaic.
 */
#define FX1_ISP_DEMOSAIC_STATUS_OFFSET 0x0E00U
#define REG_DEMOSAIC_STATUS FX1_ISP_DEMOSAIC_STATUS_OFFSET
#define FX1_ISP_DEMOSAIC_STATUS_RESET 0x00000000U
#define FX1_ISP_DEMOSAIC_STATUS_ACCESS FX1_ISP_ACCESS_RO
#define FX1_ISP_DEMOSAIC_STATUS_RO_MASK 0x00000001U
#define FX1_ISP_DEMOSAIC_STATUS_RW_MASK 0x00000000U
#define FX1_ISP_DEMOSAIC_STATUS_W1C_MASK 0x00000000U
#define FX1_ISP_DEMOSAIC_STATUS_W1S_MASK 0x00000000U
#define FX1_ISP_DEMOSAIC_STATUS_W1P_MASK 0x00000000U
#define FX1_ISP_DEMOSAIC_STATUS_W1SC_MASK 0x00000000U
/**
 * busy [0:0], dòng bảng 160, kiểu truy cập RO
 * BUSY=1 khi khối còn giữ pixel đang xử lý. Tuân thủ trình tự cấu hình/nạp LUT riêng; chỉ thấy
 * BUSY=0 chưa đủ quyền ghi đè bảng đang dùng.
 */
#define FX1_ISP_DEMOSAIC_STATUS_BUSY_SHIFT 0U
#define FX1_ISP_DEMOSAIC_STATUS_BUSY_WIDTH 1U
#define FX1_ISP_DEMOSAIC_STATUS_BUSY_MASK 0x00000001U
#define FX1_ISP_DEMOSAIC_STATUS_BUSY_RESET 0x00000000U
#define FX1_ISP_DEMOSAIC_STATUS_BUSY_ACCESS FX1_ISP_ACCESS_RO

/**
 * CCM_CTRL: địa chỉ lệch 0x1000, dòng bảng 161, kiểu truy cập RW
 * CCM: ma trận 3x3 và offset từng màu chuyển đáp ứng màu cảm biến sang không gian làm việc. Tắt chỉ
 * áp ma trận đơn vị, không bỏ độ trễ của khối trên đường dữ liệu.
 */
#define FX1_ISP_CCM_CTRL_OFFSET 0x1000U
#define REG_CCM_CTRL FX1_ISP_CCM_CTRL_OFFSET
#define FX1_ISP_CCM_CTRL_RESET 0x00000000U
#define FX1_ISP_CCM_CTRL_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_CCM_CTRL_RO_MASK 0x00000000U
#define FX1_ISP_CCM_CTRL_RW_MASK 0x00000005U
#define FX1_ISP_CCM_CTRL_W1C_MASK 0x00000000U
#define FX1_ISP_CCM_CTRL_W1S_MASK 0x00000000U
#define FX1_ISP_CCM_CTRL_W1P_MASK 0x00000000U
#define FX1_ISP_CCM_CTRL_W1SC_MASK 0x00000000U
/**
 * en [0:0], dòng bảng 162, kiểu truy cập RW
 * Bật ma trận màu: 1 áp ma trận, 0 truyền nguyên.
 */
#define FX1_ISP_CCM_CTRL_EN_SHIFT 0U
#define FX1_ISP_CCM_CTRL_EN_WIDTH 1U
#define FX1_ISP_CCM_CTRL_EN_MASK 0x00000001U
#define FX1_ISP_CCM_CTRL_EN_RESET 0x00000000U
#define FX1_ISP_CCM_CTRL_EN_ACCESS FX1_ISP_ACCESS_RW
/**
 * updated [2:2], dòng bảng 163, kiểu truy cập RW
 * Trường cập nhật chỉ giữ tương thích đọc/ghi. Cấu hình được chốt ở SOF được nhận; không cần lệnh
 * cập nhật riêng.
 */
#define FX1_ISP_CCM_CTRL_UPDATED_SHIFT 2U
#define FX1_ISP_CCM_CTRL_UPDATED_WIDTH 1U
#define FX1_ISP_CCM_CTRL_UPDATED_MASK 0x00000004U
#define FX1_ISP_CCM_CTRL_UPDATED_RESET 0x00000000U
#define FX1_ISP_CCM_CTRL_UPDATED_ACCESS FX1_ISP_ACCESS_RW

/**
 * CCM_CRR: địa chỉ lệch 0x1004, dòng bảng 164, kiểu truy cập RW
 * CCM: phần tử ma trận tại hàng 0, cột 0 (thứ tự màu R,G,B).
 */
#define FX1_ISP_CCM_CRR_OFFSET 0x1004U
#define REG_CCM_CRR FX1_ISP_CCM_CRR_OFFSET
#define FX1_ISP_CCM_CRR_RESET 0x00000200U
#define FX1_ISP_CCM_CRR_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_CCM_CRR_RO_MASK 0x00000000U
#define FX1_ISP_CCM_CRR_RW_MASK 0x00000FFFU
#define FX1_ISP_CCM_CRR_W1C_MASK 0x00000000U
#define FX1_ISP_CCM_CRR_W1S_MASK 0x00000000U
#define FX1_ISP_CCM_CRR_W1P_MASK 0x00000000U
#define FX1_ISP_CCM_CRR_W1SC_MASK 0x00000000U
/**
 * coefrr [11:0], dòng bảng 165, kiểu truy cập RW
 * Hệ số/offset có dấu bù hai Q3.9 rộng 12 bit; 1x=0x200, dải -4 đến 4-1/512. Offset dùng cùng tỷ lệ
 * phần lẻ với bộ tích lũy và được cộng trước lần dịch cuối.
 */
#define FX1_ISP_CCM_CRR_COEFRR_SHIFT 0U
#define FX1_ISP_CCM_CRR_COEFRR_WIDTH 12U
#define FX1_ISP_CCM_CRR_COEFRR_MASK 0x00000FFFU
#define FX1_ISP_CCM_CRR_COEFRR_RESET 0x00000200U
#define FX1_ISP_CCM_CRR_COEFRR_ACCESS FX1_ISP_ACCESS_RW

/**
 * CCM_CRG: địa chỉ lệch 0x1008, dòng bảng 166, kiểu truy cập RW
 * CCM: phần tử ma trận tại hàng 0, cột 1 (thứ tự màu R,G,B).
 */
#define FX1_ISP_CCM_CRG_OFFSET 0x1008U
#define REG_CCM_CRG FX1_ISP_CCM_CRG_OFFSET
#define FX1_ISP_CCM_CRG_RESET 0x00000000U
#define FX1_ISP_CCM_CRG_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_CCM_CRG_RO_MASK 0x00000000U
#define FX1_ISP_CCM_CRG_RW_MASK 0x00000FFFU
#define FX1_ISP_CCM_CRG_W1C_MASK 0x00000000U
#define FX1_ISP_CCM_CRG_W1S_MASK 0x00000000U
#define FX1_ISP_CCM_CRG_W1P_MASK 0x00000000U
#define FX1_ISP_CCM_CRG_W1SC_MASK 0x00000000U
/**
 * coefrg [11:0], dòng bảng 167, kiểu truy cập RW
 * Hệ số/offset có dấu bù hai Q3.9 rộng 12 bit; 1x=0x200, dải -4 đến 4-1/512. Offset dùng cùng tỷ lệ
 * phần lẻ với bộ tích lũy và được cộng trước lần dịch cuối.
 */
#define FX1_ISP_CCM_CRG_COEFRG_SHIFT 0U
#define FX1_ISP_CCM_CRG_COEFRG_WIDTH 12U
#define FX1_ISP_CCM_CRG_COEFRG_MASK 0x00000FFFU
#define FX1_ISP_CCM_CRG_COEFRG_RESET 0x00000000U
#define FX1_ISP_CCM_CRG_COEFRG_ACCESS FX1_ISP_ACCESS_RW

/**
 * CCM_CRB: địa chỉ lệch 0x100C, dòng bảng 168, kiểu truy cập RW
 * CCM: phần tử ma trận tại hàng 0, cột 2 (thứ tự màu R,G,B).
 */
#define FX1_ISP_CCM_CRB_OFFSET 0x100CU
#define REG_CCM_CRB FX1_ISP_CCM_CRB_OFFSET
#define FX1_ISP_CCM_CRB_RESET 0x00000000U
#define FX1_ISP_CCM_CRB_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_CCM_CRB_RO_MASK 0x00000000U
#define FX1_ISP_CCM_CRB_RW_MASK 0x00000FFFU
#define FX1_ISP_CCM_CRB_W1C_MASK 0x00000000U
#define FX1_ISP_CCM_CRB_W1S_MASK 0x00000000U
#define FX1_ISP_CCM_CRB_W1P_MASK 0x00000000U
#define FX1_ISP_CCM_CRB_W1SC_MASK 0x00000000U
/**
 * coefrb [11:0], dòng bảng 169, kiểu truy cập RW
 * Hệ số/offset có dấu bù hai Q3.9 rộng 12 bit; 1x=0x200, dải -4 đến 4-1/512. Offset dùng cùng tỷ lệ
 * phần lẻ với bộ tích lũy và được cộng trước lần dịch cuối.
 */
#define FX1_ISP_CCM_CRB_COEFRB_SHIFT 0U
#define FX1_ISP_CCM_CRB_COEFRB_WIDTH 12U
#define FX1_ISP_CCM_CRB_COEFRB_MASK 0x00000FFFU
#define FX1_ISP_CCM_CRB_COEFRB_RESET 0x00000000U
#define FX1_ISP_CCM_CRB_COEFRB_ACCESS FX1_ISP_ACCESS_RW

/**
 * CCM_CGR: địa chỉ lệch 0x1010, dòng bảng 170, kiểu truy cập RW
 * CCM: phần tử ma trận tại hàng 1, cột 0 (thứ tự màu R,G,B).
 */
#define FX1_ISP_CCM_CGR_OFFSET 0x1010U
#define REG_CCM_CGR FX1_ISP_CCM_CGR_OFFSET
#define FX1_ISP_CCM_CGR_RESET 0x00000000U
#define FX1_ISP_CCM_CGR_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_CCM_CGR_RO_MASK 0x00000000U
#define FX1_ISP_CCM_CGR_RW_MASK 0x00000FFFU
#define FX1_ISP_CCM_CGR_W1C_MASK 0x00000000U
#define FX1_ISP_CCM_CGR_W1S_MASK 0x00000000U
#define FX1_ISP_CCM_CGR_W1P_MASK 0x00000000U
#define FX1_ISP_CCM_CGR_W1SC_MASK 0x00000000U
/**
 * coefgr [11:0], dòng bảng 171, kiểu truy cập RW
 * Hệ số/offset có dấu bù hai Q3.9 rộng 12 bit; 1x=0x200, dải -4 đến 4-1/512. Offset dùng cùng tỷ lệ
 * phần lẻ với bộ tích lũy và được cộng trước lần dịch cuối.
 */
#define FX1_ISP_CCM_CGR_COEFGR_SHIFT 0U
#define FX1_ISP_CCM_CGR_COEFGR_WIDTH 12U
#define FX1_ISP_CCM_CGR_COEFGR_MASK 0x00000FFFU
#define FX1_ISP_CCM_CGR_COEFGR_RESET 0x00000000U
#define FX1_ISP_CCM_CGR_COEFGR_ACCESS FX1_ISP_ACCESS_RW

/**
 * CCM_CGG: địa chỉ lệch 0x1014, dòng bảng 172, kiểu truy cập RW
 * CCM: phần tử ma trận tại hàng 1, cột 1 (thứ tự màu R,G,B).
 */
#define FX1_ISP_CCM_CGG_OFFSET 0x1014U
#define REG_CCM_CGG FX1_ISP_CCM_CGG_OFFSET
#define FX1_ISP_CCM_CGG_RESET 0x00000200U
#define FX1_ISP_CCM_CGG_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_CCM_CGG_RO_MASK 0x00000000U
#define FX1_ISP_CCM_CGG_RW_MASK 0x00000FFFU
#define FX1_ISP_CCM_CGG_W1C_MASK 0x00000000U
#define FX1_ISP_CCM_CGG_W1S_MASK 0x00000000U
#define FX1_ISP_CCM_CGG_W1P_MASK 0x00000000U
#define FX1_ISP_CCM_CGG_W1SC_MASK 0x00000000U
/**
 * coefgg [11:0], dòng bảng 173, kiểu truy cập RW
 * Hệ số/offset có dấu bù hai Q3.9 rộng 12 bit; 1x=0x200, dải -4 đến 4-1/512. Offset dùng cùng tỷ lệ
 * phần lẻ với bộ tích lũy và được cộng trước lần dịch cuối.
 */
#define FX1_ISP_CCM_CGG_COEFGG_SHIFT 0U
#define FX1_ISP_CCM_CGG_COEFGG_WIDTH 12U
#define FX1_ISP_CCM_CGG_COEFGG_MASK 0x00000FFFU
#define FX1_ISP_CCM_CGG_COEFGG_RESET 0x00000200U
#define FX1_ISP_CCM_CGG_COEFGG_ACCESS FX1_ISP_ACCESS_RW

/**
 * CCM_CGB: địa chỉ lệch 0x1018, dòng bảng 174, kiểu truy cập RW
 * CCM: phần tử ma trận tại hàng 1, cột 2 (thứ tự màu R,G,B).
 */
#define FX1_ISP_CCM_CGB_OFFSET 0x1018U
#define REG_CCM_CGB FX1_ISP_CCM_CGB_OFFSET
#define FX1_ISP_CCM_CGB_RESET 0x00000000U
#define FX1_ISP_CCM_CGB_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_CCM_CGB_RO_MASK 0x00000000U
#define FX1_ISP_CCM_CGB_RW_MASK 0x00000FFFU
#define FX1_ISP_CCM_CGB_W1C_MASK 0x00000000U
#define FX1_ISP_CCM_CGB_W1S_MASK 0x00000000U
#define FX1_ISP_CCM_CGB_W1P_MASK 0x00000000U
#define FX1_ISP_CCM_CGB_W1SC_MASK 0x00000000U
/**
 * coefgb [11:0], dòng bảng 175, kiểu truy cập RW
 * Hệ số/offset có dấu bù hai Q3.9 rộng 12 bit; 1x=0x200, dải -4 đến 4-1/512. Offset dùng cùng tỷ lệ
 * phần lẻ với bộ tích lũy và được cộng trước lần dịch cuối.
 */
#define FX1_ISP_CCM_CGB_COEFGB_SHIFT 0U
#define FX1_ISP_CCM_CGB_COEFGB_WIDTH 12U
#define FX1_ISP_CCM_CGB_COEFGB_MASK 0x00000FFFU
#define FX1_ISP_CCM_CGB_COEFGB_RESET 0x00000000U
#define FX1_ISP_CCM_CGB_COEFGB_ACCESS FX1_ISP_ACCESS_RW

/**
 * CCM_CBR: địa chỉ lệch 0x101C, dòng bảng 176, kiểu truy cập RW
 * CCM: phần tử ma trận tại hàng 2, cột 0 (thứ tự màu R,G,B).
 */
#define FX1_ISP_CCM_CBR_OFFSET 0x101CU
#define REG_CCM_CBR FX1_ISP_CCM_CBR_OFFSET
#define FX1_ISP_CCM_CBR_RESET 0x00000000U
#define FX1_ISP_CCM_CBR_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_CCM_CBR_RO_MASK 0x00000000U
#define FX1_ISP_CCM_CBR_RW_MASK 0x00000FFFU
#define FX1_ISP_CCM_CBR_W1C_MASK 0x00000000U
#define FX1_ISP_CCM_CBR_W1S_MASK 0x00000000U
#define FX1_ISP_CCM_CBR_W1P_MASK 0x00000000U
#define FX1_ISP_CCM_CBR_W1SC_MASK 0x00000000U
/**
 * coefbr [11:0], dòng bảng 177, kiểu truy cập RW
 * Hệ số/offset có dấu bù hai Q3.9 rộng 12 bit; 1x=0x200, dải -4 đến 4-1/512. Offset dùng cùng tỷ lệ
 * phần lẻ với bộ tích lũy và được cộng trước lần dịch cuối.
 */
#define FX1_ISP_CCM_CBR_COEFBR_SHIFT 0U
#define FX1_ISP_CCM_CBR_COEFBR_WIDTH 12U
#define FX1_ISP_CCM_CBR_COEFBR_MASK 0x00000FFFU
#define FX1_ISP_CCM_CBR_COEFBR_RESET 0x00000000U
#define FX1_ISP_CCM_CBR_COEFBR_ACCESS FX1_ISP_ACCESS_RW

/**
 * CCM_CBG: địa chỉ lệch 0x1020, dòng bảng 178, kiểu truy cập RW
 * CCM: phần tử ma trận tại hàng 2, cột 1 (thứ tự màu R,G,B).
 */
#define FX1_ISP_CCM_CBG_OFFSET 0x1020U
#define REG_CCM_CBG FX1_ISP_CCM_CBG_OFFSET
#define FX1_ISP_CCM_CBG_RESET 0x00000000U
#define FX1_ISP_CCM_CBG_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_CCM_CBG_RO_MASK 0x00000000U
#define FX1_ISP_CCM_CBG_RW_MASK 0x00000FFFU
#define FX1_ISP_CCM_CBG_W1C_MASK 0x00000000U
#define FX1_ISP_CCM_CBG_W1S_MASK 0x00000000U
#define FX1_ISP_CCM_CBG_W1P_MASK 0x00000000U
#define FX1_ISP_CCM_CBG_W1SC_MASK 0x00000000U
/**
 * coefbg [11:0], dòng bảng 179, kiểu truy cập RW
 * Hệ số/offset có dấu bù hai Q3.9 rộng 12 bit; 1x=0x200, dải -4 đến 4-1/512. Offset dùng cùng tỷ lệ
 * phần lẻ với bộ tích lũy và được cộng trước lần dịch cuối.
 */
#define FX1_ISP_CCM_CBG_COEFBG_SHIFT 0U
#define FX1_ISP_CCM_CBG_COEFBG_WIDTH 12U
#define FX1_ISP_CCM_CBG_COEFBG_MASK 0x00000FFFU
#define FX1_ISP_CCM_CBG_COEFBG_RESET 0x00000000U
#define FX1_ISP_CCM_CBG_COEFBG_ACCESS FX1_ISP_ACCESS_RW

/**
 * CCM_CBB: địa chỉ lệch 0x1024, dòng bảng 180, kiểu truy cập RW
 * CCM: phần tử ma trận tại hàng 2, cột 2 (thứ tự màu R,G,B).
 */
#define FX1_ISP_CCM_CBB_OFFSET 0x1024U
#define REG_CCM_CBB FX1_ISP_CCM_CBB_OFFSET
#define FX1_ISP_CCM_CBB_RESET 0x00000200U
#define FX1_ISP_CCM_CBB_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_CCM_CBB_RO_MASK 0x00000000U
#define FX1_ISP_CCM_CBB_RW_MASK 0x00000FFFU
#define FX1_ISP_CCM_CBB_W1C_MASK 0x00000000U
#define FX1_ISP_CCM_CBB_W1S_MASK 0x00000000U
#define FX1_ISP_CCM_CBB_W1P_MASK 0x00000000U
#define FX1_ISP_CCM_CBB_W1SC_MASK 0x00000000U
/**
 * coefbb [11:0], dòng bảng 181, kiểu truy cập RW
 * Hệ số/offset có dấu bù hai Q3.9 rộng 12 bit; 1x=0x200, dải -4 đến 4-1/512. Offset dùng cùng tỷ lệ
 * phần lẻ với bộ tích lũy và được cộng trước lần dịch cuối.
 */
#define FX1_ISP_CCM_CBB_COEFBB_SHIFT 0U
#define FX1_ISP_CCM_CBB_COEFBB_WIDTH 12U
#define FX1_ISP_CCM_CBB_COEFBB_MASK 0x00000FFFU
#define FX1_ISP_CCM_CBB_COEFBB_RESET 0x00000200U
#define FX1_ISP_CCM_CBB_COEFBB_ACCESS FX1_ISP_ACCESS_RW

/**
 * CCM_OFS_R: địa chỉ lệch 0x1028, dòng bảng 182, kiểu truy cập RW
 * CCM: offset của màu tương ứng, cộng sau phép nhân ma trận.
 */
#define FX1_ISP_CCM_OFS_R_OFFSET 0x1028U
#define REG_CCM_OFS_R FX1_ISP_CCM_OFS_R_OFFSET
#define FX1_ISP_CCM_OFS_R_RESET 0x00000000U
#define FX1_ISP_CCM_OFS_R_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_CCM_OFS_R_RO_MASK 0x00000000U
#define FX1_ISP_CCM_OFS_R_RW_MASK 0x00000FFFU
#define FX1_ISP_CCM_OFS_R_W1C_MASK 0x00000000U
#define FX1_ISP_CCM_OFS_R_W1S_MASK 0x00000000U
#define FX1_ISP_CCM_OFS_R_W1P_MASK 0x00000000U
#define FX1_ISP_CCM_OFS_R_W1SC_MASK 0x00000000U
/**
 * offset_r [11:0], dòng bảng 183, kiểu truy cập RW
 * Hệ số/offset có dấu bù hai Q3.9 rộng 12 bit; 1x=0x200, dải -4 đến 4-1/512. Offset dùng cùng tỷ lệ
 * phần lẻ với bộ tích lũy và được cộng trước lần dịch cuối.
 */
#define FX1_ISP_CCM_OFS_R_OFFSET_R_SHIFT 0U
#define FX1_ISP_CCM_OFS_R_OFFSET_R_WIDTH 12U
#define FX1_ISP_CCM_OFS_R_OFFSET_R_MASK 0x00000FFFU
#define FX1_ISP_CCM_OFS_R_OFFSET_R_RESET 0x00000000U
#define FX1_ISP_CCM_OFS_R_OFFSET_R_ACCESS FX1_ISP_ACCESS_RW

/**
 * CCM_OFS_G: địa chỉ lệch 0x102C, dòng bảng 184, kiểu truy cập RW
 * CCM: offset của màu tương ứng, cộng sau phép nhân ma trận.
 */
#define FX1_ISP_CCM_OFS_G_OFFSET 0x102CU
#define REG_CCM_OFS_G FX1_ISP_CCM_OFS_G_OFFSET
#define FX1_ISP_CCM_OFS_G_RESET 0x00000000U
#define FX1_ISP_CCM_OFS_G_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_CCM_OFS_G_RO_MASK 0x00000000U
#define FX1_ISP_CCM_OFS_G_RW_MASK 0x00000FFFU
#define FX1_ISP_CCM_OFS_G_W1C_MASK 0x00000000U
#define FX1_ISP_CCM_OFS_G_W1S_MASK 0x00000000U
#define FX1_ISP_CCM_OFS_G_W1P_MASK 0x00000000U
#define FX1_ISP_CCM_OFS_G_W1SC_MASK 0x00000000U
/**
 * offset_g [11:0], dòng bảng 185, kiểu truy cập RW
 * Hệ số/offset có dấu bù hai Q3.9 rộng 12 bit; 1x=0x200, dải -4 đến 4-1/512. Offset dùng cùng tỷ lệ
 * phần lẻ với bộ tích lũy và được cộng trước lần dịch cuối.
 */
#define FX1_ISP_CCM_OFS_G_OFFSET_G_SHIFT 0U
#define FX1_ISP_CCM_OFS_G_OFFSET_G_WIDTH 12U
#define FX1_ISP_CCM_OFS_G_OFFSET_G_MASK 0x00000FFFU
#define FX1_ISP_CCM_OFS_G_OFFSET_G_RESET 0x00000000U
#define FX1_ISP_CCM_OFS_G_OFFSET_G_ACCESS FX1_ISP_ACCESS_RW

/**
 * CCM_OFS_B: địa chỉ lệch 0x1030, dòng bảng 186, kiểu truy cập RW
 * CCM: offset của màu tương ứng, cộng sau phép nhân ma trận.
 */
#define FX1_ISP_CCM_OFS_B_OFFSET 0x1030U
#define REG_CCM_OFS_B FX1_ISP_CCM_OFS_B_OFFSET
#define FX1_ISP_CCM_OFS_B_RESET 0x00000000U
#define FX1_ISP_CCM_OFS_B_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_CCM_OFS_B_RO_MASK 0x00000000U
#define FX1_ISP_CCM_OFS_B_RW_MASK 0x00000FFFU
#define FX1_ISP_CCM_OFS_B_W1C_MASK 0x00000000U
#define FX1_ISP_CCM_OFS_B_W1S_MASK 0x00000000U
#define FX1_ISP_CCM_OFS_B_W1P_MASK 0x00000000U
#define FX1_ISP_CCM_OFS_B_W1SC_MASK 0x00000000U
/**
 * offset_b [11:0], dòng bảng 187, kiểu truy cập RW
 * Hệ số/offset có dấu bù hai Q3.9 rộng 12 bit; 1x=0x200, dải -4 đến 4-1/512. Offset dùng cùng tỷ lệ
 * phần lẻ với bộ tích lũy và được cộng trước lần dịch cuối.
 */
#define FX1_ISP_CCM_OFS_B_OFFSET_B_SHIFT 0U
#define FX1_ISP_CCM_OFS_B_OFFSET_B_WIDTH 12U
#define FX1_ISP_CCM_OFS_B_OFFSET_B_MASK 0x00000FFFU
#define FX1_ISP_CCM_OFS_B_OFFSET_B_RESET 0x00000000U
#define FX1_ISP_CCM_OFS_B_OFFSET_B_ACCESS FX1_ISP_ACCESS_RW

/**
 * CCM_STATUS: địa chỉ lệch 0x1034, dòng bảng 188, kiểu truy cập RO
 * Trạng thái CCM, chỉ đọc.
 */
#define FX1_ISP_CCM_STATUS_OFFSET 0x1034U
#define REG_CCM_STATUS FX1_ISP_CCM_STATUS_OFFSET
#define FX1_ISP_CCM_STATUS_RESET 0x00000000U
#define FX1_ISP_CCM_STATUS_ACCESS FX1_ISP_ACCESS_RO
#define FX1_ISP_CCM_STATUS_RO_MASK 0x00000001U
#define FX1_ISP_CCM_STATUS_RW_MASK 0x00000000U
#define FX1_ISP_CCM_STATUS_W1C_MASK 0x00000000U
#define FX1_ISP_CCM_STATUS_W1S_MASK 0x00000000U
#define FX1_ISP_CCM_STATUS_W1P_MASK 0x00000000U
#define FX1_ISP_CCM_STATUS_W1SC_MASK 0x00000000U
/**
 * busy [0:0], dòng bảng 189, kiểu truy cập RO
 * BUSY=1 khi khối còn giữ pixel đang xử lý. Tuân thủ trình tự cấu hình/nạp LUT riêng; chỉ thấy
 * BUSY=0 chưa đủ quyền ghi đè bảng đang dùng.
 */
#define FX1_ISP_CCM_STATUS_BUSY_SHIFT 0U
#define FX1_ISP_CCM_STATUS_BUSY_WIDTH 1U
#define FX1_ISP_CCM_STATUS_BUSY_MASK 0x00000001U
#define FX1_ISP_CCM_STATUS_BUSY_RESET 0x00000000U
#define FX1_ISP_CCM_STATUS_BUSY_ACCESS FX1_ISP_ACCESS_RO

/**
 * GAMMA_CTRL: địa chỉ lệch 0x1200, dòng bảng 190, kiểu truy cập Mixed
 * GAMMA: bật và xác nhận nạp SRAM. Hard reset và common soft reset ở profile mode xóa tính hợp lệ;
 * legacy common soft reset giữ lại. Ghi làm mất hiệu lực đích; nạp/đọc kiểm tra đầy đủ trước xác
 * nhận, theo quyền sở hữu của chế độ điều khiển.
 */
#define FX1_ISP_GAMMA_CTRL_OFFSET 0x1200U
#define REG_GAMMA_CTRL FX1_ISP_GAMMA_CTRL_OFFSET
#define FX1_ISP_GAMMA_CTRL_RESET 0x00000000U
#define FX1_ISP_GAMMA_CTRL_ACCESS FX1_ISP_ACCESS_MIXED
#define FX1_ISP_GAMMA_CTRL_RO_MASK 0x00000000U
#define FX1_ISP_GAMMA_CTRL_RW_MASK 0x00000071U
#define FX1_ISP_GAMMA_CTRL_W1C_MASK 0x00000000U
#define FX1_ISP_GAMMA_CTRL_W1S_MASK 0x00000000U
#define FX1_ISP_GAMMA_CTRL_W1P_MASK 0x00000100U
#define FX1_ISP_GAMMA_CTRL_W1SC_MASK 0x00000000U
/**
 * en [0:0], dòng bảng 191, kiểu truy cập RW
 * Chỉ áp đường gamma của frame khi bật và LUT_LOADED đã đặt; nếu chưa nạp thì truyền nguyên.
 */
#define FX1_ISP_GAMMA_CTRL_EN_SHIFT 0U
#define FX1_ISP_GAMMA_CTRL_EN_WIDTH 1U
#define FX1_ISP_GAMMA_CTRL_EN_MASK 0x00000001U
#define FX1_ISP_GAMMA_CTRL_EN_RESET 0x00000000U
#define FX1_ISP_GAMMA_CTRL_EN_ACCESS FX1_ISP_ACCESS_RW
/**
 * gamma_value [6:4], dòng bảng 192, kiểu truy cập RW
 * Lựa chọn đường cong định sẵn chỉ giữ tương thích, không tác động xử lý; phần mềm tự tạo và nạp qua
 * cổng LUT.
 */
#define FX1_ISP_GAMMA_CTRL_GAMMA_VALUE_SHIFT 4U
#define FX1_ISP_GAMMA_CTRL_GAMMA_VALUE_WIDTH 3U
#define FX1_ISP_GAMMA_CTRL_GAMMA_VALUE_MASK 0x00000070U
#define FX1_ISP_GAMMA_CTRL_GAMMA_VALUE_RESET 0x00000000U
#define FX1_ISP_GAMMA_CTRL_GAMMA_VALUE_ACCESS FX1_ISP_ACCESS_RW
/**
 * lut_load_confirm [8:8], dòng bảng 193, kiểu truy cập W1P
 * Xác nhận đã ghi/đọc kiểm tra đủ 256 điểm khi Gamma tắt và rỗi. Đọc bit trả 0. Ghi LUT làm mất hiệu
 * lực bảng và được ưu tiên hơn xác nhận cùng lúc.
 */
#define FX1_ISP_GAMMA_CTRL_LUT_LOAD_CONFIRM_SHIFT 8U
#define FX1_ISP_GAMMA_CTRL_LUT_LOAD_CONFIRM_WIDTH 1U
#define FX1_ISP_GAMMA_CTRL_LUT_LOAD_CONFIRM_MASK 0x00000100U
#define FX1_ISP_GAMMA_CTRL_LUT_LOAD_CONFIRM_RESET 0x00000000U
#define FX1_ISP_GAMMA_CTRL_LUT_LOAD_CONFIRM_ACCESS FX1_ISP_ACCESS_W1P

/**
 * GAMMA_LUT_ADDR: địa chỉ lệch 0x1204, dòng bảng 194, kiểu truy cập RW
 * Chỉ số LUT Gamma tự tăng mỗi lần ghi DATA; nạp đủ bảng bằng ghi địa chỉ một lần rồi 256 dữ liệu.
 */
#define FX1_ISP_GAMMA_LUT_ADDR_OFFSET 0x1204U
#define REG_GAMMA_LUT_ADDR FX1_ISP_GAMMA_LUT_ADDR_OFFSET
#define FX1_ISP_GAMMA_LUT_ADDR_RESET 0x00000000U
#define FX1_ISP_GAMMA_LUT_ADDR_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_GAMMA_LUT_ADDR_RO_MASK 0x00000000U
#define FX1_ISP_GAMMA_LUT_ADDR_RW_MASK 0x000000FFU
#define FX1_ISP_GAMMA_LUT_ADDR_W1C_MASK 0x00000000U
#define FX1_ISP_GAMMA_LUT_ADDR_W1S_MASK 0x00000000U
#define FX1_ISP_GAMMA_LUT_ADDR_W1P_MASK 0x00000000U
#define FX1_ISP_GAMMA_LUT_ADDR_W1SC_MASK 0x00000000U
/**
 * addr [7:0], dòng bảng 195, kiểu truy cập RW
 * Chỉ số 0..255. Điểm i biểu diễn đầu vào i/255 toàn dải; tám bit chỉ số là tám bit cao của thành
 * phần 12 bit.
 */
#define FX1_ISP_GAMMA_LUT_ADDR_ADDR_SHIFT 0U
#define FX1_ISP_GAMMA_LUT_ADDR_ADDR_WIDTH 8U
#define FX1_ISP_GAMMA_LUT_ADDR_ADDR_MASK 0x000000FFU
#define FX1_ISP_GAMMA_LUT_ADDR_ADDR_RESET 0x00000000U
#define FX1_ISP_GAMMA_LUT_ADDR_ADDR_ACCESS FX1_ISP_ACCESS_RW

/**
 * GAMMA_LUT_DATA: địa chỉ lệch 0x1208, dòng bảng 196, kiểu truy cập RW
 * Cổng ghi LUT Gamma ghi cùng giá trị cho cả ba màu; phần cứng không có đường cong riêng từng màu.
 */
#define FX1_ISP_GAMMA_LUT_DATA_OFFSET 0x1208U
#define REG_GAMMA_LUT_DATA FX1_ISP_GAMMA_LUT_DATA_OFFSET
#define FX1_ISP_GAMMA_LUT_DATA_RESET 0x00000000U
#define FX1_ISP_GAMMA_LUT_DATA_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_GAMMA_LUT_DATA_RO_MASK 0x00000000U
#define FX1_ISP_GAMMA_LUT_DATA_RW_MASK 0x00000FFFU
#define FX1_ISP_GAMMA_LUT_DATA_W1C_MASK 0x00000000U
#define FX1_ISP_GAMMA_LUT_DATA_W1S_MASK 0x00000000U
#define FX1_ISP_GAMMA_LUT_DATA_W1P_MASK 0x00000000U
#define FX1_ISP_GAMMA_LUT_DATA_W1SC_MASK 0x00000000U
/**
 * data [11:0], dòng bảng 197, kiểu truy cập RW
 * Giá trị đầu ra Gamma 12 bit 0..4095. Đọc DATA chỉ trả lần ghi cuối, không phải RAM; dùng LUT_RDATA
 * để kiểm tra bảng.
 */
#define FX1_ISP_GAMMA_LUT_DATA_DATA_SHIFT 0U
#define FX1_ISP_GAMMA_LUT_DATA_DATA_WIDTH 12U
#define FX1_ISP_GAMMA_LUT_DATA_DATA_MASK 0x00000FFFU
#define FX1_ISP_GAMMA_LUT_DATA_DATA_RESET 0x00000000U
#define FX1_ISP_GAMMA_LUT_DATA_DATA_ACCESS FX1_ISP_ACCESS_RW

/**
 * GAMMA_LUT_RDATA: địa chỉ lệch 0x120C, dòng bảng 198, kiểu truy cập RO
 * Đọc nội dung LUT tại GAMMA_LUT_ADDR; đọc không làm tăng địa chỉ.
 */
#define FX1_ISP_GAMMA_LUT_RDATA_OFFSET 0x120CU
#define REG_GAMMA_LUT_RDATA FX1_ISP_GAMMA_LUT_RDATA_OFFSET
#define FX1_ISP_GAMMA_LUT_RDATA_RESET 0x00000000U
#define FX1_ISP_GAMMA_LUT_RDATA_ACCESS FX1_ISP_ACCESS_RO
#define FX1_ISP_GAMMA_LUT_RDATA_RO_MASK 0x00000FFFU
#define FX1_ISP_GAMMA_LUT_RDATA_RW_MASK 0x00000000U
#define FX1_ISP_GAMMA_LUT_RDATA_W1C_MASK 0x00000000U
#define FX1_ISP_GAMMA_LUT_RDATA_W1S_MASK 0x00000000U
#define FX1_ISP_GAMMA_LUT_RDATA_W1P_MASK 0x00000000U
#define FX1_ISP_GAMMA_LUT_RDATA_W1SC_MASK 0x00000000U
/**
 * rdata [11:0], dòng bảng 199, kiểu truy cập RO
 * Dữ liệu điểm LUT 12 bit. RAM chưa khởi tạo có nội dung chưa xác định; cột giá trị reset không khởi
 * tạo RAM.
 */
#define FX1_ISP_GAMMA_LUT_RDATA_RDATA_SHIFT 0U
#define FX1_ISP_GAMMA_LUT_RDATA_RDATA_WIDTH 12U
#define FX1_ISP_GAMMA_LUT_RDATA_RDATA_MASK 0x00000FFFU
#define FX1_ISP_GAMMA_LUT_RDATA_RDATA_RESET 0x00000000U
#define FX1_ISP_GAMMA_LUT_RDATA_RDATA_ACCESS FX1_ISP_ACCESS_RO

/**
 * GAMMA_STATUS: địa chỉ lệch 0x1210, dòng bảng 200, kiểu truy cập RO
 * Trạng thái Gamma. LUT_LOADED báo bank 0 đã xác nhận trong cả hai chế độ. Ghi bank 0, INIT bank 0,
 * thoát profile, hard reset hoặc common soft reset ở profile mode xóa cờ. Bảng chưa nạp thì bỏ qua
 * sửa.
 */
#define FX1_ISP_GAMMA_STATUS_OFFSET 0x1210U
#define REG_GAMMA_STATUS FX1_ISP_GAMMA_STATUS_OFFSET
#define FX1_ISP_GAMMA_STATUS_RESET 0x00000000U
#define FX1_ISP_GAMMA_STATUS_ACCESS FX1_ISP_ACCESS_RO
#define FX1_ISP_GAMMA_STATUS_RO_MASK 0x00000003U
#define FX1_ISP_GAMMA_STATUS_RW_MASK 0x00000000U
#define FX1_ISP_GAMMA_STATUS_W1C_MASK 0x00000000U
#define FX1_ISP_GAMMA_STATUS_W1S_MASK 0x00000000U
#define FX1_ISP_GAMMA_STATUS_W1P_MASK 0x00000000U
#define FX1_ISP_GAMMA_STATUS_W1SC_MASK 0x00000000U
/**
 * busy [0:0], dòng bảng 201, kiểu truy cập RO
 * BUSY=1 khi khối còn giữ pixel đang xử lý. Tuân thủ trình tự cấu hình/nạp LUT riêng; chỉ thấy
 * BUSY=0 chưa đủ quyền ghi đè bảng đang dùng.
 */
#define FX1_ISP_GAMMA_STATUS_BUSY_SHIFT 0U
#define FX1_ISP_GAMMA_STATUS_BUSY_WIDTH 1U
#define FX1_ISP_GAMMA_STATUS_BUSY_MASK 0x00000001U
#define FX1_ISP_GAMMA_STATUS_BUSY_RESET 0x00000000U
#define FX1_ISP_GAMMA_STATUS_BUSY_ACCESS FX1_ISP_ACCESS_RO
/**
 * lut_loaded [1:1], dòng bảng 202, kiểu truy cập RO
 * Cờ phần mềm xác nhận bank Gamma 0. Xác nhận legacy hoặc GAMMA_CONFIRM cho bank 0 đặt cờ;
 * INIT/thoát profile/hard reset/profile-mode common soft reset xóa, legacy common soft reset giữ.
 * Phải kiểm tra đủ bảng trước xác nhận; chưa nạp thì bỏ qua Gamma.
 */
#define FX1_ISP_GAMMA_STATUS_LUT_LOADED_SHIFT 1U
#define FX1_ISP_GAMMA_STATUS_LUT_LOADED_WIDTH 1U
#define FX1_ISP_GAMMA_STATUS_LUT_LOADED_MASK 0x00000002U
#define FX1_ISP_GAMMA_STATUS_LUT_LOADED_RESET 0x00000000U
#define FX1_ISP_GAMMA_STATUS_LUT_LOADED_ACCESS FX1_ISP_ACCESS_RO

/**
 * CSC_CTRL: địa chỉ lệch 0x1400, dòng bảng 203, kiểu truy cập RW
 * CSC bắt buộc RGB sang YCbCr, không bypass. STD chọn BT.601/709; FULL_RANGE chọn dải độc lập, mặc
 * định toàn dải. Điều chỉnh màu tùy chọn và trung gian số học rộng theo profile của frame.
 */
#define FX1_ISP_CSC_CTRL_OFFSET 0x1400U
#define REG_CSC_CTRL FX1_ISP_CSC_CTRL_OFFSET
#define FX1_ISP_CSC_CTRL_RESET 0x00000000U
#define FX1_ISP_CSC_CTRL_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_CSC_CTRL_RO_MASK 0x00000000U
#define FX1_ISP_CSC_CTRL_RW_MASK 0x00000001U
#define FX1_ISP_CSC_CTRL_W1C_MASK 0x00000000U
#define FX1_ISP_CSC_CTRL_W1S_MASK 0x00000000U
#define FX1_ISP_CSC_CTRL_W1P_MASK 0x00000000U
#define FX1_ISP_CSC_CTRL_W1SC_MASK 0x00000000U
/**
 * std [0:0], dòng bảng 204, kiểu truy cập RW
 * Chuẩn màu 0=BT.601 mặc định, 1=BT.709; giữ theo frame. FULL_RANGE chọn dải độc lập, mặc định toàn
 * dải. Không bypass; CSC ngược CH2 dùng đúng chuẩn/dải đã chốt của frame.
 */
#define FX1_ISP_CSC_CTRL_STD_SHIFT 0U
#define FX1_ISP_CSC_CTRL_STD_WIDTH 1U
#define FX1_ISP_CSC_CTRL_STD_MASK 0x00000001U
#define FX1_ISP_CSC_CTRL_STD_RESET 0x00000000U
#define FX1_ISP_CSC_CTRL_STD_ACCESS FX1_ISP_ACCESS_RW

/**
 * GTM_CTRL: địa chỉ lệch 0x1600, dòng bảng 205, kiểu truy cập RW
 * GTM: thích nghi tông tự động từ frame đã hoàn tất trước đó hoặc đường cong thủ công được bảo vệ.
 * Tự động trễ ít nhất một frame, còn phụ thuộc hoàn tất bảng/quyền sở hữu; thủ công giữ đường cong
 * phần mềm chọn.
 */
#define FX1_ISP_GTM_CTRL_OFFSET 0x1600U
#define REG_GTM_CTRL FX1_ISP_GTM_CTRL_OFFSET
#define FX1_ISP_GTM_CTRL_RESET 0x00000000U
#define FX1_ISP_GTM_CTRL_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_GTM_CTRL_RO_MASK 0x00000000U
#define FX1_ISP_GTM_CTRL_RW_MASK 0x00000003U
#define FX1_ISP_GTM_CTRL_W1C_MASK 0x00000000U
#define FX1_ISP_GTM_CTRL_W1S_MASK 0x00000000U
#define FX1_ISP_GTM_CTRL_W1P_MASK 0x00000000U
#define FX1_ISP_GTM_CTRL_W1SC_MASK 0x00000000U
/**
 * en [0:0], dòng bảng 206, kiểu truy cập RW
 * Bật GTM: 1 áp đường cong tông, 0 truyền nguyên.
 */
#define FX1_ISP_GTM_CTRL_EN_SHIFT 0U
#define FX1_ISP_GTM_CTRL_EN_WIDTH 1U
#define FX1_ISP_GTM_CTRL_EN_MASK 0x00000001U
#define FX1_ISP_GTM_CTRL_EN_RESET 0x00000000U
#define FX1_ISP_GTM_CTRL_EN_ACCESS FX1_ISP_ACCESS_RW
/**
 * manual [1:1], dòng bảng 207, kiểu truy cập RW
 * MANUAL=1 giữ việc tự xây đường cong. Nạp/kiểm tra khi EN=0 và MANUAL=1 rồi mới bật. MANUAL không
 * cho ghi RAM khi EN=1. Legacy chỉ dùng bank thủ công 0 khi đủ 65 điểm; chưa đủ thì truyền nguyên.
 */
#define FX1_ISP_GTM_CTRL_MANUAL_SHIFT 1U
#define FX1_ISP_GTM_CTRL_MANUAL_WIDTH 1U
#define FX1_ISP_GTM_CTRL_MANUAL_MASK 0x00000002U
#define FX1_ISP_GTM_CTRL_MANUAL_RESET 0x00000000U
#define FX1_ISP_GTM_CTRL_MANUAL_ACCESS FX1_ISP_ACCESS_RW

/**
 * GTM_LUT_ADDR: địa chỉ lệch 0x1604, dòng bảng 208, kiểu truy cập RW
 * Chỉ số LUT tỷ số GTM tự tăng sau mỗi lần ghi DATA.
 */
#define FX1_ISP_GTM_LUT_ADDR_OFFSET 0x1604U
#define REG_GTM_LUT_ADDR FX1_ISP_GTM_LUT_ADDR_OFFSET
#define FX1_ISP_GTM_LUT_ADDR_RESET 0x00000000U
#define FX1_ISP_GTM_LUT_ADDR_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_GTM_LUT_ADDR_RO_MASK 0x00000000U
#define FX1_ISP_GTM_LUT_ADDR_RW_MASK 0x0000007FU
#define FX1_ISP_GTM_LUT_ADDR_W1C_MASK 0x00000000U
#define FX1_ISP_GTM_LUT_ADDR_W1S_MASK 0x00000000U
#define FX1_ISP_GTM_LUT_ADDR_W1P_MASK 0x00000000U
#define FX1_ISP_GTM_LUT_ADDR_W1SC_MASK 0x00000000U
/**
 * addr [6:0], dòng bảng 209, kiểu truy cập RW
 * Điểm điều khiển 0..64: 65 điểm tạo 64 khoảng nội suy và điểm cuối, nên địa chỉ cần 7 bit. Điểm i
 * ứng với luma 4*i. Ghi 65..127 bị bỏ, đọc trả 0.
 */
#define FX1_ISP_GTM_LUT_ADDR_ADDR_SHIFT 0U
#define FX1_ISP_GTM_LUT_ADDR_ADDR_WIDTH 7U
#define FX1_ISP_GTM_LUT_ADDR_ADDR_MASK 0x0000007FU
#define FX1_ISP_GTM_LUT_ADDR_ADDR_RESET 0x00000000U
#define FX1_ISP_GTM_LUT_ADDR_ADDR_ACCESS FX1_ISP_ACCESS_RW

/**
 * GTM_LUT_DATA: địa chỉ lệch 0x1608, dòng bảng 210, kiểu truy cập RW
 * Cổng ghi LUT GTM chỉ cập nhật RAM khi EN=0; ghi lúc bật không đổi bảng. MANUAL=1 giữ quá trình tự
 * xây trong lúc nạp/sử dụng.
 */
#define FX1_ISP_GTM_LUT_DATA_OFFSET 0x1608U
#define REG_GTM_LUT_DATA FX1_ISP_GTM_LUT_DATA_OFFSET
#define FX1_ISP_GTM_LUT_DATA_RESET 0x00000000U
#define FX1_ISP_GTM_LUT_DATA_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_GTM_LUT_DATA_RO_MASK 0x00000000U
#define FX1_ISP_GTM_LUT_DATA_RW_MASK 0x0000FFFFU
#define FX1_ISP_GTM_LUT_DATA_W1C_MASK 0x00000000U
#define FX1_ISP_GTM_LUT_DATA_W1S_MASK 0x00000000U
#define FX1_ISP_GTM_LUT_DATA_W1P_MASK 0x00000000U
#define FX1_ISP_GTM_LUT_DATA_W1SC_MASK 0x00000000U
/**
 * data [15:0], dòng bảng 211, kiểu truy cập RW
 * Tỷ số UQ8.8 áp cho mức luma đó: 256=1x, thấp hơn làm tối, cao hơn làm sáng. Đọc DATA trả giá trị
 * đệm; dùng LUT_RDATA để đọc bảng.
 */
#define FX1_ISP_GTM_LUT_DATA_DATA_SHIFT 0U
#define FX1_ISP_GTM_LUT_DATA_DATA_WIDTH 16U
#define FX1_ISP_GTM_LUT_DATA_DATA_MASK 0x0000FFFFU
#define FX1_ISP_GTM_LUT_DATA_DATA_RESET 0x00000000U
#define FX1_ISP_GTM_LUT_DATA_DATA_ACCESS FX1_ISP_ACCESS_RW

/**
 * GTM_KEY: địa chỉ lệch 0x160C, dòng bảng 212, kiểu truy cập RW
 * Tham số key GTM nhân độ sáng cảnh khi tạo đường cong tự động, không bảo đảm trung bình số học đầu
 * ra.
 */
#define FX1_ISP_GTM_KEY_OFFSET 0x160CU
#define REG_GTM_KEY FX1_ISP_GTM_KEY_OFFSET
#define FX1_ISP_GTM_KEY_RESET 0x0000002EU
#define FX1_ISP_GTM_KEY_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_GTM_KEY_RO_MASK 0x00000000U
#define FX1_ISP_GTM_KEY_RW_MASK 0x000000FFU
#define FX1_ISP_GTM_KEY_W1C_MASK 0x00000000U
#define FX1_ISP_GTM_KEY_W1S_MASK 0x00000000U
#define FX1_ISP_GTM_KEY_W1P_MASK 0x00000000U
#define FX1_ISP_GTM_KEY_W1SC_MASK 0x00000000U
/**
 * key [7:0], dòng bảng 213, kiểu truy cập RW
 * Key UQ0.8; sau reset 0x2E=46/256=0,1796875 xấp xỉ 0,18. Là tham số phơi sáng đường cong tự động,
 * không phải trung bình đầu ra được bảo đảm.
 */
#define FX1_ISP_GTM_KEY_KEY_SHIFT 0U
#define FX1_ISP_GTM_KEY_KEY_WIDTH 8U
#define FX1_ISP_GTM_KEY_KEY_MASK 0x000000FFU
#define FX1_ISP_GTM_KEY_KEY_RESET 0x0000002EU
#define FX1_ISP_GTM_KEY_KEY_ACCESS FX1_ISP_ACCESS_RW

/**
 * GTM_LWHITE: địa chỉ lệch 0x1610, dòng bảng 214, kiểu truy cập RW
 * Mức luma ánh xạ tới trắng; phần trên bị nén về cực đại. Phần cứng tự tạo nghịch đảo.
 */
#define FX1_ISP_GTM_LWHITE_OFFSET 0x1610U
#define REG_GTM_LWHITE FX1_ISP_GTM_LWHITE_OFFSET
#define FX1_ISP_GTM_LWHITE_RESET 0x00000400U
#define FX1_ISP_GTM_LWHITE_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_GTM_LWHITE_RO_MASK 0x00000000U
#define FX1_ISP_GTM_LWHITE_RW_MASK 0x0000FFFFU
#define FX1_ISP_GTM_LWHITE_W1C_MASK 0x00000000U
#define FX1_ISP_GTM_LWHITE_W1S_MASK 0x00000000U
#define FX1_ISP_GTM_LWHITE_W1P_MASK 0x00000000U
#define FX1_ISP_GTM_LWHITE_W1SC_MASK 0x00000000U
/**
 * lwhite [15:0], dòng bảng 215, kiểu truy cập RW
 * Luma điểm trắng UQ8.8 tương đối với trung bình cảnh; sau reset 0x0400=4x.
 */
#define FX1_ISP_GTM_LWHITE_LWHITE_SHIFT 0U
#define FX1_ISP_GTM_LWHITE_LWHITE_WIDTH 16U
#define FX1_ISP_GTM_LWHITE_LWHITE_MASK 0x0000FFFFU
#define FX1_ISP_GTM_LWHITE_LWHITE_RESET 0x00000400U
#define FX1_ISP_GTM_LWHITE_LWHITE_ACCESS FX1_ISP_ACCESS_RW

/**
 * GTM_ROI_LOG2: địa chỉ lệch 0x1614, dòng bảng 216, kiểu truy cập RW
 * Cửa sổ đo sáng ở giữa, cạnh danh định 2^n. Mỗi trục tự kẹp số mũ theo floor(log2(kích thước hữu
 * dụng)), nên cửa sổ có thể chữ nhật. Diện tích lũy thừa hai cho phép chuẩn hóa bằng dịch bit.
 */
#define FX1_ISP_GTM_ROI_LOG2_OFFSET 0x1614U
#define REG_GTM_ROI_LOG2 FX1_ISP_GTM_ROI_LOG2_OFFSET
#define FX1_ISP_GTM_ROI_LOG2_RESET 0x0000000BU
#define FX1_ISP_GTM_ROI_LOG2_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_GTM_ROI_LOG2_RO_MASK 0x00000000U
#define FX1_ISP_GTM_ROI_LOG2_RW_MASK 0x0000001FU
#define FX1_ISP_GTM_ROI_LOG2_W1C_MASK 0x00000000U
#define FX1_ISP_GTM_ROI_LOG2_W1S_MASK 0x00000000U
#define FX1_ISP_GTM_ROI_LOG2_W1P_MASK 0x00000000U
#define FX1_ISP_GTM_ROI_LOG2_W1SC_MASK 0x00000000U
/**
 * roi_log2 [4:0], dòng bảng 217, kiểu truy cập RW
 * Số mũ log2 cạnh cửa sổ đo: 2^n, kẹp riêng theo chiều rộng/cao đầu vào. Sau reset n=11. Đo trên
 * luma đầu vào, dùng đường cong tự động thu được cho frame sau.
 */
#define FX1_ISP_GTM_ROI_LOG2_ROI_LOG2_SHIFT 0U
#define FX1_ISP_GTM_ROI_LOG2_ROI_LOG2_WIDTH 5U
#define FX1_ISP_GTM_ROI_LOG2_ROI_LOG2_MASK 0x0000001FU
#define FX1_ISP_GTM_ROI_LOG2_ROI_LOG2_RESET 0x0000000BU
#define FX1_ISP_GTM_ROI_LOG2_ROI_LOG2_ACCESS FX1_ISP_ACCESS_RW

/**
 * GTM_LUT_RDATA: địa chỉ lệch 0x1618, dòng bảng 218, kiểu truy cập RO
 * Đọc LUT tỷ số GTM tại GTM_LUT_ADDR, chỉ đọc.
 */
#define FX1_ISP_GTM_LUT_RDATA_OFFSET 0x1618U
#define REG_GTM_LUT_RDATA FX1_ISP_GTM_LUT_RDATA_OFFSET
#define FX1_ISP_GTM_LUT_RDATA_RESET 0x00000000U
#define FX1_ISP_GTM_LUT_RDATA_ACCESS FX1_ISP_ACCESS_RO
#define FX1_ISP_GTM_LUT_RDATA_RO_MASK 0x0000FFFFU
#define FX1_ISP_GTM_LUT_RDATA_RW_MASK 0x00000000U
#define FX1_ISP_GTM_LUT_RDATA_W1C_MASK 0x00000000U
#define FX1_ISP_GTM_LUT_RDATA_W1S_MASK 0x00000000U
#define FX1_ISP_GTM_LUT_RDATA_W1P_MASK 0x00000000U
#define FX1_ISP_GTM_LUT_RDATA_W1SC_MASK 0x00000000U
/**
 * rdata [15:0], dòng bảng 219, kiểu truy cập RO
 * Profile mode đọc bảng thủ công của bank chọn bởi TUNE_BANK; điểm chưa ghi đủ hai byte trả 0, không
 * đọc được bảng tự động. Legacy: MANUAL=1 đọc bank thủ công 0, MANUAL=0 đọc bảng tự động công bố gần
 * nhất; điểm thủ công thiếu trả 0.
 */
#define FX1_ISP_GTM_LUT_RDATA_RDATA_SHIFT 0U
#define FX1_ISP_GTM_LUT_RDATA_RDATA_WIDTH 16U
#define FX1_ISP_GTM_LUT_RDATA_RDATA_MASK 0x0000FFFFU
#define FX1_ISP_GTM_LUT_RDATA_RDATA_RESET 0x00000000U
#define FX1_ISP_GTM_LUT_RDATA_RDATA_ACCESS FX1_ISP_ACCESS_RO

/**
 * GTM_STATUS: địa chỉ lệch 0x161C, dòng bảng 220, kiểu truy cập RO
 * Trạng thái khối GTM.
 */
#define FX1_ISP_GTM_STATUS_OFFSET 0x161CU
#define REG_GTM_STATUS FX1_ISP_GTM_STATUS_OFFSET
#define FX1_ISP_GTM_STATUS_RESET 0x00000000U
#define FX1_ISP_GTM_STATUS_ACCESS FX1_ISP_ACCESS_RO
#define FX1_ISP_GTM_STATUS_RO_MASK 0x00000001U
#define FX1_ISP_GTM_STATUS_RW_MASK 0x00000000U
#define FX1_ISP_GTM_STATUS_W1C_MASK 0x00000000U
#define FX1_ISP_GTM_STATUS_W1S_MASK 0x00000000U
#define FX1_ISP_GTM_STATUS_W1P_MASK 0x00000000U
#define FX1_ISP_GTM_STATUS_W1SC_MASK 0x00000000U
/**
 * busy [0:0], dòng bảng 221, kiểu truy cập RO
 * BUSY=1 khi khối còn giữ pixel đang xử lý. Tuân thủ trình tự cấu hình/nạp LUT riêng; chỉ thấy
 * BUSY=0 chưa đủ quyền ghi đè bảng đang dùng.
 */
#define FX1_ISP_GTM_STATUS_BUSY_SHIFT 0U
#define FX1_ISP_GTM_STATUS_BUSY_WIDTH 1U
#define FX1_ISP_GTM_STATUS_BUSY_MASK 0x00000001U
#define FX1_ISP_GTM_STATUS_BUSY_RESET 0x00000000U
#define FX1_ISP_GTM_STATUS_BUSY_ACCESS FX1_ISP_ACCESS_RO

/**
 * NR_2D_CTRL: địa chỉ lệch 0x1800, dòng bảng 222, kiểu truy cập RW
 * NR_2D: khử nhiễu không gian Haar-wavelet, cường độ theo phương sai nhiễu đo ở frame trước.
 */
#define FX1_ISP_NR_2D_CTRL_OFFSET 0x1800U
#define REG_NR_2D_CTRL FX1_ISP_NR_2D_CTRL_OFFSET
#define FX1_ISP_NR_2D_CTRL_RESET 0x00000000U
#define FX1_ISP_NR_2D_CTRL_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_NR_2D_CTRL_RO_MASK 0x00000000U
#define FX1_ISP_NR_2D_CTRL_RW_MASK 0x00000001U
#define FX1_ISP_NR_2D_CTRL_W1C_MASK 0x00000000U
#define FX1_ISP_NR_2D_CTRL_W1S_MASK 0x00000000U
#define FX1_ISP_NR_2D_CTRL_W1P_MASK 0x00000000U
#define FX1_ISP_NR_2D_CTRL_W1SC_MASK 0x00000000U
/**
 * en [0:0], dòng bảng 223, kiểu truy cập RW
 * Bật 2DNR: 1 lọc, 0 truyền nguyên. Khi tắt, đường dữ liệu đi vòng động cơ nên thay đổi chỉ áp sau
 * khi động cơ xử lý hết, không đổi giữa frame.
 */
#define FX1_ISP_NR_2D_CTRL_EN_SHIFT 0U
#define FX1_ISP_NR_2D_CTRL_EN_WIDTH 1U
#define FX1_ISP_NR_2D_CTRL_EN_MASK 0x00000001U
#define FX1_ISP_NR_2D_CTRL_EN_RESET 0x00000000U
#define FX1_ISP_NR_2D_CTRL_EN_ACCESS FX1_ISP_ACCESS_RW

/**
 * NR_2D_EST_VAR: địa chỉ lệch 0x1804, dòng bảng 224, kiểu truy cập RO
 * Ước lượng phương sai nhiễu tự động từ frame đã hoàn tất, chỉ đọc. Chế độ tự động dùng kết quả
 * trước đủ điều kiện; chế độ thủ công/LUT theo luma dùng phương sai cấu hình riêng.
 */
#define FX1_ISP_NR_2D_EST_VAR_OFFSET 0x1804U
#define REG_NR_2D_EST_VAR FX1_ISP_NR_2D_EST_VAR_OFFSET
#define FX1_ISP_NR_2D_EST_VAR_RESET 0x00000000U
#define FX1_ISP_NR_2D_EST_VAR_ACCESS FX1_ISP_ACCESS_RO
#define FX1_ISP_NR_2D_EST_VAR_RO_MASK 0x0000FFFFU
#define FX1_ISP_NR_2D_EST_VAR_RW_MASK 0x00000000U
#define FX1_ISP_NR_2D_EST_VAR_W1C_MASK 0x00000000U
#define FX1_ISP_NR_2D_EST_VAR_W1S_MASK 0x00000000U
#define FX1_ISP_NR_2D_EST_VAR_W1P_MASK 0x00000000U
#define FX1_ISP_NR_2D_EST_VAR_W1SC_MASK 0x00000000U
/**
 * est_var [15:0], dòng bảng 225, kiểu truy cập RO
 * NR_EST_VAR là phương sai nguyên không dấu 16 bit, kẹp 65535, reset 0. Sigma nội bộ UQ9.8 rộng 17
 * bit trước bình phương/dịch; giá trị đọc luôn là ước lượng tự động, độc lập chế độ chọn phương sai.
 */
#define FX1_ISP_NR_2D_EST_VAR_EST_VAR_SHIFT 0U
#define FX1_ISP_NR_2D_EST_VAR_EST_VAR_WIDTH 16U
#define FX1_ISP_NR_2D_EST_VAR_EST_VAR_MASK 0x0000FFFFU
#define FX1_ISP_NR_2D_EST_VAR_EST_VAR_RESET 0x00000000U
#define FX1_ISP_NR_2D_EST_VAR_EST_VAR_ACCESS FX1_ISP_ACCESS_RO

/**
 * NR_2D_STATUS: địa chỉ lệch 0x1808, dòng bảng 226, kiểu truy cập RO
 * Trạng thái khối NR.
 */
#define FX1_ISP_NR_2D_STATUS_OFFSET 0x1808U
#define REG_NR_2D_STATUS FX1_ISP_NR_2D_STATUS_OFFSET
#define FX1_ISP_NR_2D_STATUS_RESET 0x00000000U
#define FX1_ISP_NR_2D_STATUS_ACCESS FX1_ISP_ACCESS_RO
#define FX1_ISP_NR_2D_STATUS_RO_MASK 0x00000001U
#define FX1_ISP_NR_2D_STATUS_RW_MASK 0x00000000U
#define FX1_ISP_NR_2D_STATUS_W1C_MASK 0x00000000U
#define FX1_ISP_NR_2D_STATUS_W1S_MASK 0x00000000U
#define FX1_ISP_NR_2D_STATUS_W1P_MASK 0x00000000U
#define FX1_ISP_NR_2D_STATUS_W1SC_MASK 0x00000000U
/**
 * busy [0:0], dòng bảng 227, kiểu truy cập RO
 * BUSY=1 khi khối còn giữ pixel đang xử lý. Tuân thủ trình tự cấu hình/nạp LUT riêng; chỉ thấy
 * BUSY=0 chưa đủ quyền ghi đè bảng đang dùng.
 */
#define FX1_ISP_NR_2D_STATUS_BUSY_SHIFT 0U
#define FX1_ISP_NR_2D_STATUS_BUSY_WIDTH 1U
#define FX1_ISP_NR_2D_STATUS_BUSY_MASK 0x00000001U
#define FX1_ISP_NR_2D_STATUS_BUSY_RESET 0x00000000U
#define FX1_ISP_NR_2D_STATUS_BUSY_ACCESS FX1_ISP_ACCESS_RO

/**
 * TNR_3D_CTRL: địa chỉ lệch 0x1A00, dòng bảng 228, kiểu truy cập RW
 * TNR_3D dự phòng cho khử nhiễu thời gian; có thanh ghi lưu nhưng không tác động xử lý ảnh trong đặc
 * tả này.
 */
#define FX1_ISP_TNR_3D_CTRL_OFFSET 0x1A00U
#define REG_TNR_3D_CTRL FX1_ISP_TNR_3D_CTRL_OFFSET
#define FX1_ISP_TNR_3D_CTRL_RESET 0x00000000U
#define FX1_ISP_TNR_3D_CTRL_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_TNR_3D_CTRL_RO_MASK 0x00000000U
#define FX1_ISP_TNR_3D_CTRL_RW_MASK 0x00000001U
#define FX1_ISP_TNR_3D_CTRL_W1C_MASK 0x00000000U
#define FX1_ISP_TNR_3D_CTRL_W1S_MASK 0x00000000U
#define FX1_ISP_TNR_3D_CTRL_W1P_MASK 0x00000000U
#define FX1_ISP_TNR_3D_CTRL_W1SC_MASK 0x00000000U
/**
 * en [0:0], dòng bảng 229, kiểu truy cập RW
 * Bit bật TNR chưa triển khai, không tác động xử lý.
 */
#define FX1_ISP_TNR_3D_CTRL_EN_SHIFT 0U
#define FX1_ISP_TNR_3D_CTRL_EN_WIDTH 1U
#define FX1_ISP_TNR_3D_CTRL_EN_MASK 0x00000001U
#define FX1_ISP_TNR_3D_CTRL_EN_RESET 0x00000000U
#define FX1_ISP_TNR_3D_CTRL_EN_ACCESS FX1_ISP_ACCESS_RW

/**
 * TNR_3D_MTF_SEL: địa chỉ lệch 0x1A04, dòng bảng 230, kiểu truy cập RW
 * Mã hàm truyền chuyển động TNR_3D chưa triển khai; xem TNR_3D_CTRL.
 */
#define FX1_ISP_TNR_3D_MTF_SEL_OFFSET 0x1A04U
#define REG_TNR_3D_MTF_SEL FX1_ISP_TNR_3D_MTF_SEL_OFFSET
#define FX1_ISP_TNR_3D_MTF_SEL_RESET 0x00000000U
#define FX1_ISP_TNR_3D_MTF_SEL_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_TNR_3D_MTF_SEL_RO_MASK 0x00000000U
#define FX1_ISP_TNR_3D_MTF_SEL_RW_MASK 0x00000007U
#define FX1_ISP_TNR_3D_MTF_SEL_W1C_MASK 0x00000000U
#define FX1_ISP_TNR_3D_MTF_SEL_W1S_MASK 0x00000000U
#define FX1_ISP_TNR_3D_MTF_SEL_W1P_MASK 0x00000000U
#define FX1_ISP_TNR_3D_MTF_SEL_W1SC_MASK 0x00000000U
/**
 * mtf [2:0], dòng bảng 231, kiểu truy cập RW
 * Mã chuyển động chưa triển khai: 0=không, 1=yếu, 2=vừa, 3=mạnh, 4=rất mạnh; không tác động xử lý.
 */
#define FX1_ISP_TNR_3D_MTF_SEL_MTF_SHIFT 0U
#define FX1_ISP_TNR_3D_MTF_SEL_MTF_WIDTH 3U
#define FX1_ISP_TNR_3D_MTF_SEL_MTF_MASK 0x00000007U
#define FX1_ISP_TNR_3D_MTF_SEL_MTF_RESET 0x00000000U
#define FX1_ISP_TNR_3D_MTF_SEL_MTF_ACCESS FX1_ISP_ACCESS_RW

/**
 * TNR_3D_HIST_BASE: địa chỉ lệch 0x1A08, dòng bảng 232, kiểu truy cập RW
 * Địa chỉ DDR frame lịch sử TNR_3D chưa triển khai; xem TNR_3D_CTRL.
 */
#define FX1_ISP_TNR_3D_HIST_BASE_OFFSET 0x1A08U
#define REG_TNR_3D_HIST_BASE FX1_ISP_TNR_3D_HIST_BASE_OFFSET
#define FX1_ISP_TNR_3D_HIST_BASE_RESET 0x00000000U
#define FX1_ISP_TNR_3D_HIST_BASE_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_TNR_3D_HIST_BASE_RO_MASK 0x00000000U
#define FX1_ISP_TNR_3D_HIST_BASE_RW_MASK 0xFFFFFFFFU
#define FX1_ISP_TNR_3D_HIST_BASE_W1C_MASK 0x00000000U
#define FX1_ISP_TNR_3D_HIST_BASE_W1S_MASK 0x00000000U
#define FX1_ISP_TNR_3D_HIST_BASE_W1P_MASK 0x00000000U
#define FX1_ISP_TNR_3D_HIST_BASE_W1SC_MASK 0x00000000U
/**
 * hist_base_addr [31:0], dòng bảng 233, kiểu truy cập RW
 * Địa chỉ nền frame lịch sử dự phòng, không tác động xử lý.
 */
#define FX1_ISP_TNR_3D_HIST_BASE_HIST_BASE_ADDR_SHIFT 0U
#define FX1_ISP_TNR_3D_HIST_BASE_HIST_BASE_ADDR_WIDTH 32U
#define FX1_ISP_TNR_3D_HIST_BASE_HIST_BASE_ADDR_MASK 0xFFFFFFFFU
#define FX1_ISP_TNR_3D_HIST_BASE_HIST_BASE_ADDR_RESET 0x00000000U
#define FX1_ISP_TNR_3D_HIST_BASE_HIST_BASE_ADDR_ACCESS FX1_ISP_ACCESS_RW

/**
 * EE_CTRL: địa chỉ lệch 0x1C00, dòng bảng 234, kiểu truy cập RW
 * EE: làm sắc nét bằng cộng thành phần thông cao luma ở hai mức, đã điều biến gain và kẹp biên độ.
 */
#define FX1_ISP_EE_CTRL_OFFSET 0x1C00U
#define REG_EE_CTRL FX1_ISP_EE_CTRL_OFFSET
#define FX1_ISP_EE_CTRL_RESET 0x00000000U
#define FX1_ISP_EE_CTRL_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_EE_CTRL_RO_MASK 0x00000000U
#define FX1_ISP_EE_CTRL_RW_MASK 0x0000003FU
#define FX1_ISP_EE_CTRL_W1C_MASK 0x00000000U
#define FX1_ISP_EE_CTRL_W1S_MASK 0x00000000U
#define FX1_ISP_EE_CTRL_W1P_MASK 0x00000000U
#define FX1_ISP_EE_CTRL_W1SC_MASK 0x00000000U
/**
 * en [0:0], dòng bảng 235, kiểu truy cập RW
 * Bật EE: 1 làm nét, 0 truyền nguyên có trễ; độ trễ khối không đổi.
 */
#define FX1_ISP_EE_CTRL_EN_SHIFT 0U
#define FX1_ISP_EE_CTRL_EN_WIDTH 1U
#define FX1_ISP_EE_CTRL_EN_MASK 0x00000001U
#define FX1_ISP_EE_CTRL_EN_RESET 0x00000000U
#define FX1_ISP_EE_CTRL_EN_ACCESS FX1_ISP_ACCESS_RW
/**
 * reg_update [1:1], dòng bảng 236, kiểu truy cập RW
 * Trường cập nhật chỉ giữ tương thích đọc/ghi, không có COMMIT riêng. Enable/profile/hệ số chốt ở
 * SOF tâm cửa sổ được nhận. Profile mode dùng enable/profile từ token đầu vào đã chốt, kể cả tại SOF
 * cục bộ.
 */
#define FX1_ISP_EE_CTRL_REG_UPDATE_SHIFT 1U
#define FX1_ISP_EE_CTRL_REG_UPDATE_WIDTH 1U
#define FX1_ISP_EE_CTRL_REG_UPDATE_MASK 0x00000002U
#define FX1_ISP_EE_CTRL_REG_UPDATE_RESET 0x00000000U
#define FX1_ISP_EE_CTRL_REG_UPDATE_ACCESS FX1_ISP_ACCESS_RW
/**
 * profile_sel [3:2], dòng bảng 237, kiểu truy cập RW
 * Chọn profile xử lý 0..2, chốt ở SOF tâm cửa sổ; mã 3 chọn profile 0. Độc lập PROFILE_WR_SEL.
 * Profile mode lấy enable và lựa chọn cư trú từ token đầu vào đã chốt.
 */
#define FX1_ISP_EE_CTRL_PROFILE_SEL_SHIFT 2U
#define FX1_ISP_EE_CTRL_PROFILE_SEL_WIDTH 2U
#define FX1_ISP_EE_CTRL_PROFILE_SEL_MASK 0x0000000CU
#define FX1_ISP_EE_CTRL_PROFILE_SEL_RESET 0x00000000U
#define FX1_ISP_EE_CTRL_PROFILE_SEL_ACCESS FX1_ISP_ACCESS_RW
/**
 * profile_wr_sel [5:4], dòng bảng 238, kiểu truy cập RW
 * Chọn bộ cư trú để đọc/ghi trực tiếp 0..2. Mã 3 bỏ ghi và đọc profile 0; không thuộc bank
 * TUNE_BANK.
 */
#define FX1_ISP_EE_CTRL_PROFILE_WR_SEL_SHIFT 4U
#define FX1_ISP_EE_CTRL_PROFILE_WR_SEL_WIDTH 2U
#define FX1_ISP_EE_CTRL_PROFILE_WR_SEL_MASK 0x00000030U
#define FX1_ISP_EE_CTRL_PROFILE_WR_SEL_RESET 0x00000000U
#define FX1_ISP_EE_CTRL_PROFILE_WR_SEL_ACCESS FX1_ISP_ACCESS_RW

/**
 * EE_ALPHA: địa chỉ lệch 0x1C04, dòng bảng 239, kiểu truy cập RW
 * EE: Cường độ làm nét mức nhỏ 3x3. Hệ số cư trú: PROFILE_WR_SEL chọn cả đọc/ghi, ngoài TUNE_BANK.
 * Ghi xóa xác nhận và bị chặn với đích active/đã SEAL/pending/đang được tham chiếu ở profile mode.
 */
#define FX1_ISP_EE_ALPHA_OFFSET 0x1C04U
#define REG_EE_ALPHA FX1_ISP_EE_ALPHA_OFFSET
#define FX1_ISP_EE_ALPHA_RESET 0x00004000U
#define FX1_ISP_EE_ALPHA_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_EE_ALPHA_RO_MASK 0x00000000U
#define FX1_ISP_EE_ALPHA_RW_MASK 0x0000FFFFU
#define FX1_ISP_EE_ALPHA_W1C_MASK 0x00000000U
#define FX1_ISP_EE_ALPHA_W1S_MASK 0x00000000U
#define FX1_ISP_EE_ALPHA_W1P_MASK 0x00000000U
#define FX1_ISP_EE_ALPHA_W1SC_MASK 0x00000000U
/**
 * alpha [15:0], dòng bảng 240, kiểu truy cập RW
 * Cường độ làm nét nhỏ UQ1.15, 0x8000=1x là cực đại. Phần cứng kẹp mã lớn hơn về 1x, không cuộn
 * vòng.
 */
#define FX1_ISP_EE_ALPHA_ALPHA_SHIFT 0U
#define FX1_ISP_EE_ALPHA_ALPHA_WIDTH 16U
#define FX1_ISP_EE_ALPHA_ALPHA_MASK 0x0000FFFFU
#define FX1_ISP_EE_ALPHA_ALPHA_RESET 0x00004000U
#define FX1_ISP_EE_ALPHA_ALPHA_ACCESS FX1_ISP_ACCESS_RW

/**
 * EE_BETA: địa chỉ lệch 0x1C08, dòng bảng 241, kiểu truy cập RW
 * EE: Cường độ làm nét mức lớn 5x5. Hệ số cư trú: PROFILE_WR_SEL chọn cả đọc/ghi, ngoài TUNE_BANK.
 * Ghi xóa xác nhận và bị chặn với đích active/đã SEAL/pending/đang được tham chiếu ở profile mode.
 */
#define FX1_ISP_EE_BETA_OFFSET 0x1C08U
#define REG_EE_BETA FX1_ISP_EE_BETA_OFFSET
#define FX1_ISP_EE_BETA_RESET 0x00002000U
#define FX1_ISP_EE_BETA_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_EE_BETA_RO_MASK 0x00000000U
#define FX1_ISP_EE_BETA_RW_MASK 0x0000FFFFU
#define FX1_ISP_EE_BETA_W1C_MASK 0x00000000U
#define FX1_ISP_EE_BETA_W1S_MASK 0x00000000U
#define FX1_ISP_EE_BETA_W1P_MASK 0x00000000U
#define FX1_ISP_EE_BETA_W1SC_MASK 0x00000000U
/**
 * beta [15:0], dòng bảng 242, kiểu truy cập RW
 * Cường độ làm nét lớn UQ1.15, cực đại 0x8000=1x, kẹp như ALPHA. Thành phần lớn mở rộng quầng cạnh,
 * thường đặt thấp hơn ALPHA.
 */
#define FX1_ISP_EE_BETA_BETA_SHIFT 0U
#define FX1_ISP_EE_BETA_BETA_WIDTH 16U
#define FX1_ISP_EE_BETA_BETA_MASK 0x0000FFFFU
#define FX1_ISP_EE_BETA_BETA_RESET 0x00002000U
#define FX1_ISP_EE_BETA_BETA_ACCESS FX1_ISP_ACCESS_RW

/**
 * EE_CLAMP: địa chỉ lệch 0x1C0C, dòng bảng 243, kiểu truy cập RW
 * EE: Giới hạn sửa pixel hai chiều. Quá sáng trên cạnh dễ thấy hơn thiếu sáng nên thường đặt giới
 * hạn dương nhỏ hơn âm. Hệ số cư trú: PROFILE_WR_SEL chọn cả đọc/ghi, ngoài TUNE_BANK. Ghi xóa xác
 * nhận và bị chặn với đích active/đã SEAL/pending/đang được tham chiếu ở profile mode.
 */
#define FX1_ISP_EE_CLAMP_OFFSET 0x1C0CU
#define REG_EE_CLAMP FX1_ISP_EE_CLAMP_OFFSET
#define FX1_ISP_EE_CLAMP_RESET 0x00001810U
#define FX1_ISP_EE_CLAMP_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_EE_CLAMP_RO_MASK 0x00000000U
#define FX1_ISP_EE_CLAMP_RW_MASK 0x0000FFFFU
#define FX1_ISP_EE_CLAMP_W1C_MASK 0x00000000U
#define FX1_ISP_EE_CLAMP_W1S_MASK 0x00000000U
#define FX1_ISP_EE_CLAMP_W1P_MASK 0x00000000U
#define FX1_ISP_EE_CLAMP_W1SC_MASK 0x00000000U
/**
 * clamp_pos [7:0], dòng bảng 244, kiểu truy cập RW
 * Giới hạn sửa dương lớn nhất theo đơn vị luma 8 bit.
 */
#define FX1_ISP_EE_CLAMP_CLAMP_POS_SHIFT 0U
#define FX1_ISP_EE_CLAMP_CLAMP_POS_WIDTH 8U
#define FX1_ISP_EE_CLAMP_CLAMP_POS_MASK 0x000000FFU
#define FX1_ISP_EE_CLAMP_CLAMP_POS_RESET 0x00000010U
#define FX1_ISP_EE_CLAMP_CLAMP_POS_ACCESS FX1_ISP_ACCESS_RW
/**
 * clamp_neg [15:8], dòng bảng 245, kiểu truy cập RW
 * Giới hạn độ lớn sửa âm theo đơn vị luma 8 bit.
 */
#define FX1_ISP_EE_CLAMP_CLAMP_NEG_SHIFT 8U
#define FX1_ISP_EE_CLAMP_CLAMP_NEG_WIDTH 8U
#define FX1_ISP_EE_CLAMP_CLAMP_NEG_MASK 0x0000FF00U
#define FX1_ISP_EE_CLAMP_CLAMP_NEG_RESET 0x00000018U
#define FX1_ISP_EE_CLAMP_CLAMP_NEG_ACCESS FX1_ISP_ACCESS_RW

/**
 * EE_FEATURE_EN: địa chỉ lệch 0x1C10, dòng bảng 246, kiểu truy cập RW
 * EE: Điều biến tùy chọn theo độ biến thiên, tương phản và hướng tâm. Tắt cả ba vẫn dùng ALPHA/BETA
 * và LUT luma luôn áp; thành phần tắt góp hệ số 1x. Hệ số cư trú: PROFILE_WR_SEL chọn cả đọc/ghi,
 * ngoài TUNE_BANK. Ghi xóa xác nhận và bị chặn với đích active/đã SEAL/pending/đang được tham chiếu
 * ở profile mode.
 */
#define FX1_ISP_EE_FEATURE_EN_OFFSET 0x1C10U
#define REG_EE_FEATURE_EN FX1_ISP_EE_FEATURE_EN_OFFSET
#define FX1_ISP_EE_FEATURE_EN_RESET 0x00000000U
#define FX1_ISP_EE_FEATURE_EN_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_EE_FEATURE_EN_RO_MASK 0x00000000U
#define FX1_ISP_EE_FEATURE_EN_RW_MASK 0x00000007U
#define FX1_ISP_EE_FEATURE_EN_W1C_MASK 0x00000000U
#define FX1_ISP_EE_FEATURE_EN_W1S_MASK 0x00000000U
#define FX1_ISP_EE_FEATURE_EN_W1P_MASK 0x00000000U
#define FX1_ISP_EE_FEATURE_EN_W1SC_MASK 0x00000000U
/**
 * activity_en [0:0], dòng bảng 247, kiểu truy cập RW
 * Bật điều biến theo độ biến thiên cục bộ bằng LUT ACT; giảm làm nét vùng phẳng để tránh khuếch đại
 * nhiễu.
 */
#define FX1_ISP_EE_FEATURE_EN_ACTIVITY_EN_SHIFT 0U
#define FX1_ISP_EE_FEATURE_EN_ACTIVITY_EN_WIDTH 1U
#define FX1_ISP_EE_FEATURE_EN_ACTIVITY_EN_MASK 0x00000001U
#define FX1_ISP_EE_FEATURE_EN_ACTIVITY_EN_RESET 0x00000000U
#define FX1_ISP_EE_FEATURE_EN_ACTIVITY_EN_ACCESS FX1_ISP_ACCESS_RW
/**
 * contrast_en [1:1], dòng bảng 248, kiểu truy cập RW
 * Bật điều biến theo tương phản cục bộ qua hai LUT CPOS/CNEG, tách phần dương và âm.
 */
#define FX1_ISP_EE_FEATURE_EN_CONTRAST_EN_SHIFT 1U
#define FX1_ISP_EE_FEATURE_EN_CONTRAST_EN_WIDTH 1U
#define FX1_ISP_EE_FEATURE_EN_CONTRAST_EN_MASK 0x00000002U
#define FX1_ISP_EE_FEATURE_EN_CONTRAST_EN_RESET 0x00000000U
#define FX1_ISP_EE_FEATURE_EN_CONTRAST_EN_ACCESS FX1_ISP_ACCESS_RW
/**
 * radial_en [2:2], dòng bảng 249, kiểu truy cập RW
 * Bật điều biến theo khoảng cách tới tâm ảnh, dùng ba ngưỡng R2 và bốn gain để điều chỉnh góc khác
 * tâm.
 */
#define FX1_ISP_EE_FEATURE_EN_RADIAL_EN_SHIFT 2U
#define FX1_ISP_EE_FEATURE_EN_RADIAL_EN_WIDTH 1U
#define FX1_ISP_EE_FEATURE_EN_RADIAL_EN_MASK 0x00000004U
#define FX1_ISP_EE_FEATURE_EN_RADIAL_EN_RESET 0x00000000U
#define FX1_ISP_EE_FEATURE_EN_RADIAL_EN_ACCESS FX1_ISP_ACCESS_RW

/**
 * EE_RADIAL_CENTER: địa chỉ lệch 0x1C14, dòng bảng 250, kiểu truy cập RW
 * EE: Tâm điều biến hướng tâm, lưu tọa độ nhân đôi để biểu diễn nửa pixel ở ảnh kích thước chẵn. Hệ
 * số cư trú: PROFILE_WR_SEL chọn cả đọc/ghi, ngoài TUNE_BANK. Ghi xóa xác nhận và bị chặn với đích
 * active/đã SEAL/pending/đang được tham chiếu ở profile mode.
 */
#define FX1_ISP_EE_RADIAL_CENTER_OFFSET 0x1C14U
#define REG_EE_RADIAL_CENTER FX1_ISP_EE_RADIAL_CENTER_OFFSET
#define FX1_ISP_EE_RADIAL_CENTER_RESET 0x00000000U
#define FX1_ISP_EE_RADIAL_CENTER_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_EE_RADIAL_CENTER_RO_MASK 0x00000000U
#define FX1_ISP_EE_RADIAL_CENTER_RW_MASK 0x1FFF1FFFU
#define FX1_ISP_EE_RADIAL_CENTER_W1C_MASK 0x00000000U
#define FX1_ISP_EE_RADIAL_CENTER_W1S_MASK 0x00000000U
#define FX1_ISP_EE_RADIAL_CENTER_W1P_MASK 0x00000000U
#define FX1_ISP_EE_RADIAL_CENTER_W1SC_MASK 0x00000000U
/**
 * center_x2 [12:0], dòng bảng 251, kiểu truy cập RW
 * Hai lần tọa độ cột tâm. Ảnh rộng 1920 có tâm 1919/2 nên trường này bằng 1919.
 */
#define FX1_ISP_EE_RADIAL_CENTER_CENTER_X2_SHIFT 0U
#define FX1_ISP_EE_RADIAL_CENTER_CENTER_X2_WIDTH 13U
#define FX1_ISP_EE_RADIAL_CENTER_CENTER_X2_MASK 0x00001FFFU
#define FX1_ISP_EE_RADIAL_CENTER_CENTER_X2_RESET 0x00000000U
#define FX1_ISP_EE_RADIAL_CENTER_CENTER_X2_ACCESS FX1_ISP_ACCESS_RW
/**
 * center_y2 [28:16], dòng bảng 252, kiểu truy cập RW
 * Hai lần tọa độ hàng tâm.
 */
#define FX1_ISP_EE_RADIAL_CENTER_CENTER_Y2_SHIFT 16U
#define FX1_ISP_EE_RADIAL_CENTER_CENTER_Y2_WIDTH 13U
#define FX1_ISP_EE_RADIAL_CENTER_CENTER_Y2_MASK 0x1FFF0000U
#define FX1_ISP_EE_RADIAL_CENTER_CENTER_Y2_RESET 0x00000000U
#define FX1_ISP_EE_RADIAL_CENTER_CENTER_Y2_ACCESS FX1_ISP_ACCESS_RW

/**
 * EE_RADIAL_R2_TH0: địa chỉ lệch 0x1C18, dòng bảng 253, kiểu truy cập RW
 * EE: Ngưỡng bình phương bán kính vùng 0, tránh cần căn bậc hai. Hệ số cư trú: PROFILE_WR_SEL chọn
 * cả đọc/ghi, ngoài TUNE_BANK. Ghi xóa xác nhận và bị chặn với đích active/đã SEAL/pending/đang được
 * tham chiếu ở profile mode.
 */
#define FX1_ISP_EE_RADIAL_R2_TH0_OFFSET 0x1C18U
#define REG_EE_RADIAL_R2_TH0 FX1_ISP_EE_RADIAL_R2_TH0_OFFSET
#define FX1_ISP_EE_RADIAL_R2_TH0_RESET 0x00000000U
#define FX1_ISP_EE_RADIAL_R2_TH0_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_EE_RADIAL_R2_TH0_RO_MASK 0x00000000U
#define FX1_ISP_EE_RADIAL_R2_TH0_RW_MASK 0x07FFFFFFU
#define FX1_ISP_EE_RADIAL_R2_TH0_W1C_MASK 0x00000000U
#define FX1_ISP_EE_RADIAL_R2_TH0_W1S_MASK 0x00000000U
#define FX1_ISP_EE_RADIAL_R2_TH0_W1P_MASK 0x00000000U
#define FX1_ISP_EE_RADIAL_R2_TH0_W1SC_MASK 0x00000000U
/**
 * r2_th0 [26:0], dòng bảng 254, kiểu truy cập RW
 * Bình phương bán kính kết thúc vùng 0, đơn vị (2*pixel)^2 để khớp tọa độ tâm nhân đôi.
 */
#define FX1_ISP_EE_RADIAL_R2_TH0_R2_TH0_SHIFT 0U
#define FX1_ISP_EE_RADIAL_R2_TH0_R2_TH0_WIDTH 27U
#define FX1_ISP_EE_RADIAL_R2_TH0_R2_TH0_MASK 0x07FFFFFFU
#define FX1_ISP_EE_RADIAL_R2_TH0_R2_TH0_RESET 0x00000000U
#define FX1_ISP_EE_RADIAL_R2_TH0_R2_TH0_ACCESS FX1_ISP_ACCESS_RW

/**
 * EE_RADIAL_R2_TH1: địa chỉ lệch 0x1C1C, dòng bảng 255, kiểu truy cập RW
 * EE: Ngưỡng bình phương bán kính vùng 1. Hệ số cư trú: PROFILE_WR_SEL chọn cả đọc/ghi, ngoài
 * TUNE_BANK. Ghi xóa xác nhận và bị chặn với đích active/đã SEAL/pending/đang được tham chiếu ở
 * profile mode.
 */
#define FX1_ISP_EE_RADIAL_R2_TH1_OFFSET 0x1C1CU
#define REG_EE_RADIAL_R2_TH1 FX1_ISP_EE_RADIAL_R2_TH1_OFFSET
#define FX1_ISP_EE_RADIAL_R2_TH1_RESET 0x00000000U
#define FX1_ISP_EE_RADIAL_R2_TH1_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_EE_RADIAL_R2_TH1_RO_MASK 0x00000000U
#define FX1_ISP_EE_RADIAL_R2_TH1_RW_MASK 0x07FFFFFFU
#define FX1_ISP_EE_RADIAL_R2_TH1_W1C_MASK 0x00000000U
#define FX1_ISP_EE_RADIAL_R2_TH1_W1S_MASK 0x00000000U
#define FX1_ISP_EE_RADIAL_R2_TH1_W1P_MASK 0x00000000U
#define FX1_ISP_EE_RADIAL_R2_TH1_W1SC_MASK 0x00000000U
/**
 * r2_th1 [26:0], dòng bảng 256, kiểu truy cập RW
 * Bình phương bán kính kết thúc vùng 1, đơn vị (2*pixel)^2, không nhỏ hơn R2_TH0. Kiểm tra vùng theo
 * thứ tự và lấy kết quả khớp đầu tiên. Profile mode SEAL từ chối ngưỡng giảm khi RADIAL_EN=1 bằng
 * INVALID.
 */
#define FX1_ISP_EE_RADIAL_R2_TH1_R2_TH1_SHIFT 0U
#define FX1_ISP_EE_RADIAL_R2_TH1_R2_TH1_WIDTH 27U
#define FX1_ISP_EE_RADIAL_R2_TH1_R2_TH1_MASK 0x07FFFFFFU
#define FX1_ISP_EE_RADIAL_R2_TH1_R2_TH1_RESET 0x00000000U
#define FX1_ISP_EE_RADIAL_R2_TH1_R2_TH1_ACCESS FX1_ISP_ACCESS_RW

/**
 * EE_RADIAL_R2_TH2: địa chỉ lệch 0x1C20, dòng bảng 257, kiểu truy cập RW
 * EE: Ngưỡng bình phương bán kính vùng 2. Hệ số cư trú: PROFILE_WR_SEL chọn cả đọc/ghi, ngoài
 * TUNE_BANK. Ghi xóa xác nhận và bị chặn với đích active/đã SEAL/pending/đang được tham chiếu ở
 * profile mode.
 */
#define FX1_ISP_EE_RADIAL_R2_TH2_OFFSET 0x1C20U
#define REG_EE_RADIAL_R2_TH2 FX1_ISP_EE_RADIAL_R2_TH2_OFFSET
#define FX1_ISP_EE_RADIAL_R2_TH2_RESET 0x00000000U
#define FX1_ISP_EE_RADIAL_R2_TH2_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_EE_RADIAL_R2_TH2_RO_MASK 0x00000000U
#define FX1_ISP_EE_RADIAL_R2_TH2_RW_MASK 0x07FFFFFFU
#define FX1_ISP_EE_RADIAL_R2_TH2_W1C_MASK 0x00000000U
#define FX1_ISP_EE_RADIAL_R2_TH2_W1S_MASK 0x00000000U
#define FX1_ISP_EE_RADIAL_R2_TH2_W1P_MASK 0x00000000U
#define FX1_ISP_EE_RADIAL_R2_TH2_W1SC_MASK 0x00000000U
/**
 * r2_th2 [26:0], dòng bảng 258, kiểu truy cập RW
 * Bình phương bán kính kết thúc vùng 2, đơn vị (2*pixel)^2, không nhỏ hơn R2_TH1. Lấy vùng khớp đầu
 * tiên; SEAL từ chối ngưỡng giảm khi RADIAL_EN=1 bằng INVALID.
 */
#define FX1_ISP_EE_RADIAL_R2_TH2_R2_TH2_SHIFT 0U
#define FX1_ISP_EE_RADIAL_R2_TH2_R2_TH2_WIDTH 27U
#define FX1_ISP_EE_RADIAL_R2_TH2_R2_TH2_MASK 0x07FFFFFFU
#define FX1_ISP_EE_RADIAL_R2_TH2_R2_TH2_RESET 0x00000000U
#define FX1_ISP_EE_RADIAL_R2_TH2_R2_TH2_ACCESS FX1_ISP_ACCESS_RW

/**
 * EE_RADIAL_GAIN_01: địa chỉ lệch 0x1C24, dòng bảng 259, kiểu truy cập RW
 * EE: Gain hướng tâm cho hai vùng trong. Hệ số cư trú: PROFILE_WR_SEL chọn cả đọc/ghi, ngoài
 * TUNE_BANK. Ghi xóa xác nhận và bị chặn với đích active/đã SEAL/pending/đang được tham chiếu ở
 * profile mode.
 */
#define FX1_ISP_EE_RADIAL_GAIN_01_OFFSET 0x1C24U
#define REG_EE_RADIAL_GAIN_01 FX1_ISP_EE_RADIAL_GAIN_01_OFFSET
#define FX1_ISP_EE_RADIAL_GAIN_01_RESET 0x80008000U
#define FX1_ISP_EE_RADIAL_GAIN_01_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_EE_RADIAL_GAIN_01_RO_MASK 0x00000000U
#define FX1_ISP_EE_RADIAL_GAIN_01_RW_MASK 0xFFFFFFFFU
#define FX1_ISP_EE_RADIAL_GAIN_01_W1C_MASK 0x00000000U
#define FX1_ISP_EE_RADIAL_GAIN_01_W1S_MASK 0x00000000U
#define FX1_ISP_EE_RADIAL_GAIN_01_W1P_MASK 0x00000000U
#define FX1_ISP_EE_RADIAL_GAIN_01_W1SC_MASK 0x00000000U
/**
 * gain0 [15:0], dòng bảng 260, kiểu truy cập RW
 * Gain UQ1.15 trong vùng nhỏ hơn R2_TH0; 0x8000=1x, không đổi.
 */
#define FX1_ISP_EE_RADIAL_GAIN_01_GAIN0_SHIFT 0U
#define FX1_ISP_EE_RADIAL_GAIN_01_GAIN0_WIDTH 16U
#define FX1_ISP_EE_RADIAL_GAIN_01_GAIN0_MASK 0x0000FFFFU
#define FX1_ISP_EE_RADIAL_GAIN_01_GAIN0_RESET 0x00008000U
#define FX1_ISP_EE_RADIAL_GAIN_01_GAIN0_ACCESS FX1_ISP_ACCESS_RW
/**
 * gain1 [31:16], dòng bảng 261, kiểu truy cập RW
 * Gain UQ1.15 giữa R2_TH0 và R2_TH1; 0x8000=1x.
 */
#define FX1_ISP_EE_RADIAL_GAIN_01_GAIN1_SHIFT 16U
#define FX1_ISP_EE_RADIAL_GAIN_01_GAIN1_WIDTH 16U
#define FX1_ISP_EE_RADIAL_GAIN_01_GAIN1_MASK 0xFFFF0000U
#define FX1_ISP_EE_RADIAL_GAIN_01_GAIN1_RESET 0x00008000U
#define FX1_ISP_EE_RADIAL_GAIN_01_GAIN1_ACCESS FX1_ISP_ACCESS_RW

/**
 * EE_RADIAL_GAIN_23: địa chỉ lệch 0x1C28, dòng bảng 262, kiểu truy cập RW
 * EE: Gain hướng tâm cho hai vùng ngoài. Hệ số cư trú: PROFILE_WR_SEL chọn cả đọc/ghi, ngoài
 * TUNE_BANK. Ghi xóa xác nhận và bị chặn với đích active/đã SEAL/pending/đang được tham chiếu ở
 * profile mode.
 */
#define FX1_ISP_EE_RADIAL_GAIN_23_OFFSET 0x1C28U
#define REG_EE_RADIAL_GAIN_23 FX1_ISP_EE_RADIAL_GAIN_23_OFFSET
#define FX1_ISP_EE_RADIAL_GAIN_23_RESET 0x80008000U
#define FX1_ISP_EE_RADIAL_GAIN_23_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_EE_RADIAL_GAIN_23_RO_MASK 0x00000000U
#define FX1_ISP_EE_RADIAL_GAIN_23_RW_MASK 0xFFFFFFFFU
#define FX1_ISP_EE_RADIAL_GAIN_23_W1C_MASK 0x00000000U
#define FX1_ISP_EE_RADIAL_GAIN_23_W1S_MASK 0x00000000U
#define FX1_ISP_EE_RADIAL_GAIN_23_W1P_MASK 0x00000000U
#define FX1_ISP_EE_RADIAL_GAIN_23_W1SC_MASK 0x00000000U
/**
 * gain2 [15:0], dòng bảng 263, kiểu truy cập RW
 * Gain UQ1.15 giữa R2_TH1 và R2_TH2; 0x8000=1x.
 */
#define FX1_ISP_EE_RADIAL_GAIN_23_GAIN2_SHIFT 0U
#define FX1_ISP_EE_RADIAL_GAIN_23_GAIN2_WIDTH 16U
#define FX1_ISP_EE_RADIAL_GAIN_23_GAIN2_MASK 0x0000FFFFU
#define FX1_ISP_EE_RADIAL_GAIN_23_GAIN2_RESET 0x00008000U
#define FX1_ISP_EE_RADIAL_GAIN_23_GAIN2_ACCESS FX1_ISP_ACCESS_RW
/**
 * gain3 [31:16], dòng bảng 264, kiểu truy cập RW
 * Gain UQ1.15 ngoài R2_TH2; 0x8000=1x.
 */
#define FX1_ISP_EE_RADIAL_GAIN_23_GAIN3_SHIFT 16U
#define FX1_ISP_EE_RADIAL_GAIN_23_GAIN3_WIDTH 16U
#define FX1_ISP_EE_RADIAL_GAIN_23_GAIN3_MASK 0xFFFF0000U
#define FX1_ISP_EE_RADIAL_GAIN_23_GAIN3_RESET 0x00008000U
#define FX1_ISP_EE_RADIAL_GAIN_23_GAIN3_ACCESS FX1_ISP_ACCESS_RW

/**
 * EE_LUT_CTRL: địa chỉ lệch 0x1C2C, dòng bảng 265, kiểu truy cập Mixed
 * Cổng chọn bảng EE và xác nhận bộ cư trú. Ghi hệ số/LUT làm mất hiệu lực đích. Xác nhận cần đủ hệ
 * số/bảng và đúng quyền: không bị tham chiếu ở profile mode, tắt/rỗi ở legacy.
 */
#define FX1_ISP_EE_LUT_CTRL_OFFSET 0x1C2CU
#define REG_EE_LUT_CTRL FX1_ISP_EE_LUT_CTRL_OFFSET
#define FX1_ISP_EE_LUT_CTRL_RESET 0x00000000U
#define FX1_ISP_EE_LUT_CTRL_ACCESS FX1_ISP_ACCESS_MIXED
#define FX1_ISP_EE_LUT_CTRL_RO_MASK 0x00000000U
#define FX1_ISP_EE_LUT_CTRL_RW_MASK 0x00000006U
#define FX1_ISP_EE_LUT_CTRL_W1C_MASK 0x00000000U
#define FX1_ISP_EE_LUT_CTRL_W1S_MASK 0x00000000U
#define FX1_ISP_EE_LUT_CTRL_W1P_MASK 0x00000100U
#define FX1_ISP_EE_LUT_CTRL_W1SC_MASK 0x00000000U
/**
 * lut_sel [2:1], dòng bảng 266, kiểu truy cập RW
 * Chọn bảng để ghi và đọc EE_LUT_RDATA: 0=LUMA 64 điểm, 1=ACTIVITY 64 điểm, 2=CONTRAST_POS 32 điểm,
 * 3=CONTRAST_NEG 32 điểm.
 */
#define FX1_ISP_EE_LUT_CTRL_LUT_SEL_SHIFT 1U
#define FX1_ISP_EE_LUT_CTRL_LUT_SEL_WIDTH 2U
#define FX1_ISP_EE_LUT_CTRL_LUT_SEL_MASK 0x00000006U
#define FX1_ISP_EE_LUT_CTRL_LUT_SEL_RESET 0x00000000U
#define FX1_ISP_EE_LUT_CTRL_LUT_SEL_ACCESS FX1_ISP_ACCESS_RW
/**
 * profile_load_confirm [8:8], dòng bảng 267, kiểu truy cập W1P
 * Xác nhận đủ hệ số và bốn LUT. Profile mode yêu cầu đích không được active/SEAL/pending/frame đang
 * xử lý tham chiếu; legacy yêu cầu EE tắt/rỗi. Ghi hệ số/LUT cùng chu kỳ được ưu tiên và giữ không
 * hợp lệ.
 */
#define FX1_ISP_EE_LUT_CTRL_PROFILE_LOAD_CONFIRM_SHIFT 8U
#define FX1_ISP_EE_LUT_CTRL_PROFILE_LOAD_CONFIRM_WIDTH 1U
#define FX1_ISP_EE_LUT_CTRL_PROFILE_LOAD_CONFIRM_MASK 0x00000100U
#define FX1_ISP_EE_LUT_CTRL_PROFILE_LOAD_CONFIRM_RESET 0x00000000U
#define FX1_ISP_EE_LUT_CTRL_PROFILE_LOAD_CONFIRM_ACCESS FX1_ISP_ACCESS_W1P

/**
 * EE_LUT_ADDR: địa chỉ lệch 0x1C30, dòng bảng 268, kiểu truy cập RW
 * Chỉ số bảng gain EE, tự tăng sau mỗi ghi WDATA.
 */
#define FX1_ISP_EE_LUT_ADDR_OFFSET 0x1C30U
#define REG_EE_LUT_ADDR FX1_ISP_EE_LUT_ADDR_OFFSET
#define FX1_ISP_EE_LUT_ADDR_RESET 0x00000000U
#define FX1_ISP_EE_LUT_ADDR_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_EE_LUT_ADDR_RO_MASK 0x00000000U
#define FX1_ISP_EE_LUT_ADDR_RW_MASK 0x0000003FU
#define FX1_ISP_EE_LUT_ADDR_W1C_MASK 0x00000000U
#define FX1_ISP_EE_LUT_ADDR_W1S_MASK 0x00000000U
#define FX1_ISP_EE_LUT_ADDR_W1P_MASK 0x00000000U
#define FX1_ISP_EE_LUT_ADDR_W1SC_MASK 0x00000000U
/**
 * lut_addr [5:0], dòng bảng 269, kiểu truy cập RW
 * Chỉ số 0..63 cho LUMA/ACTIVITY, 0..31 cho hai bảng tương phản; bit cao bị bỏ với bảng 32 điểm.
 */
#define FX1_ISP_EE_LUT_ADDR_LUT_ADDR_SHIFT 0U
#define FX1_ISP_EE_LUT_ADDR_LUT_ADDR_WIDTH 6U
#define FX1_ISP_EE_LUT_ADDR_LUT_ADDR_MASK 0x0000003FU
#define FX1_ISP_EE_LUT_ADDR_LUT_ADDR_RESET 0x00000000U
#define FX1_ISP_EE_LUT_ADDR_LUT_ADDR_ACCESS FX1_ISP_ACCESS_RW

/**
 * EE_LUT_WDATA: địa chỉ lệch 0x1C34, dòng bảng 270, kiểu truy cập RW
 * Cổng ghi bảng gain EE.
 */
#define FX1_ISP_EE_LUT_WDATA_OFFSET 0x1C34U
#define REG_EE_LUT_WDATA FX1_ISP_EE_LUT_WDATA_OFFSET
#define FX1_ISP_EE_LUT_WDATA_RESET 0x00000000U
#define FX1_ISP_EE_LUT_WDATA_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_EE_LUT_WDATA_RO_MASK 0x00000000U
#define FX1_ISP_EE_LUT_WDATA_RW_MASK 0x0000FFFFU
#define FX1_ISP_EE_LUT_WDATA_W1C_MASK 0x00000000U
#define FX1_ISP_EE_LUT_WDATA_W1S_MASK 0x00000000U
#define FX1_ISP_EE_LUT_WDATA_W1P_MASK 0x00000000U
#define FX1_ISP_EE_LUT_WDATA_W1SC_MASK 0x00000000U
/**
 * lut_wdata [15:0], dòng bảng 271, kiểu truy cập RW
 * Gain UQ1.15, 0x8000=1x. Mỗi profile sở hữu bảng riêng. RAM chưa xác định sau reset; nạp đủ điểm
 * cần dùng trước khi bật. Mã vượt 1x bị kẹp khi xử lý; đọc kiểm tra qua RDATA.
 */
#define FX1_ISP_EE_LUT_WDATA_LUT_WDATA_SHIFT 0U
#define FX1_ISP_EE_LUT_WDATA_LUT_WDATA_WIDTH 16U
#define FX1_ISP_EE_LUT_WDATA_LUT_WDATA_MASK 0x0000FFFFU
#define FX1_ISP_EE_LUT_WDATA_LUT_WDATA_RESET 0x00000000U
#define FX1_ISP_EE_LUT_WDATA_LUT_WDATA_ACCESS FX1_ISP_ACCESS_RW

/**
 * EE_STATUS: địa chỉ lệch 0x1C38, dòng bảng 272, kiểu truy cập Mixed
 * Trạng thái EE. LUT_VALID báo profile đã xác nhận; profile được chọn chưa hợp lệ thì bỏ qua sửa,
 * không đọc SRAM chưa khởi tạo.
 */
#define FX1_ISP_EE_STATUS_OFFSET 0x1C38U
#define REG_EE_STATUS FX1_ISP_EE_STATUS_OFFSET
#define FX1_ISP_EE_STATUS_RESET 0x00000000U
#define FX1_ISP_EE_STATUS_ACCESS FX1_ISP_ACCESS_MIXED
#define FX1_ISP_EE_STATUS_RO_MASK 0x00000771U
#define FX1_ISP_EE_STATUS_RW_MASK 0x00000000U
#define FX1_ISP_EE_STATUS_W1C_MASK 0x00000002U
#define FX1_ISP_EE_STATUS_W1S_MASK 0x00000000U
#define FX1_ISP_EE_STATUS_W1P_MASK 0x00000000U
#define FX1_ISP_EE_STATUS_W1SC_MASK 0x00000000U
/**
 * busy [0:0], dòng bảng 273, kiểu truy cập RO
 * Khối còn pixel đang xử lý. Profile mode bảo vệ bộ active/đã SEAL/pending/đang bị tham chiếu; vẫn
 * có thể nạp bộ khác không bị tham chiếu.
 */
#define FX1_ISP_EE_STATUS_BUSY_SHIFT 0U
#define FX1_ISP_EE_STATUS_BUSY_WIDTH 1U
#define FX1_ISP_EE_STATUS_BUSY_MASK 0x00000001U
#define FX1_ISP_EE_STATUS_BUSY_RESET 0x00000000U
#define FX1_ISP_EE_STATUS_BUSY_ACCESS FX1_ISP_ACCESS_RO
/**
 * error [1:1], dòng bảng 274, kiểu truy cập W1C
 * Lỗi gain hướng tâm W1C, đặt một lần mỗi frame khi EE/radial bật và gain profile đang dùng vượt 1,5
 * (0xC000). Gain kẹp 1,5 khi xử lý; sự kiện mới cùng lúc ưu tiên hơn xóa.
 */
#define FX1_ISP_EE_STATUS_ERROR_SHIFT 1U
#define FX1_ISP_EE_STATUS_ERROR_WIDTH 1U
#define FX1_ISP_EE_STATUS_ERROR_MASK 0x00000002U
#define FX1_ISP_EE_STATUS_ERROR_RESET 0x00000000U
#define FX1_ISP_EE_STATUS_ERROR_ACCESS FX1_ISP_ACCESS_W1C
/**
 * lut_valid [6:4], dòng bảng 275, kiểu truy cập RO
 * Các profile 0..2 đã xác nhận đủ hệ số/bốn LUT. Ghi hệ số/LUT hoặc đổi đầu vào xóa hợp lệ đích;
 * hard reset và common reset ở profile mode xóa tất cả. Profile không hợp lệ thì bỏ làm nét.
 */
#define FX1_ISP_EE_STATUS_LUT_VALID_SHIFT 4U
#define FX1_ISP_EE_STATUS_LUT_VALID_WIDTH 3U
#define FX1_ISP_EE_STATUS_LUT_VALID_MASK 0x00000070U
#define FX1_ISP_EE_STATUS_LUT_VALID_RESET 0x00000000U
#define FX1_ISP_EE_STATUS_LUT_VALID_ACCESS FX1_ISP_ACCESS_RO
/**
 * active_profile [9:8], dòng bảng 276, kiểu truy cập RO
 * Bộ cư trú được chốt ở SOF cục bộ EE được nhận gần nhất, không chạy theo PROFILE_WR_SEL. Giá trị
 * 0..2; hard/common reset về 0.
 */
#define FX1_ISP_EE_STATUS_ACTIVE_PROFILE_SHIFT 8U
#define FX1_ISP_EE_STATUS_ACTIVE_PROFILE_WIDTH 2U
#define FX1_ISP_EE_STATUS_ACTIVE_PROFILE_MASK 0x00000300U
#define FX1_ISP_EE_STATUS_ACTIVE_PROFILE_RESET 0x00000000U
#define FX1_ISP_EE_STATUS_ACTIVE_PROFILE_ACCESS FX1_ISP_ACCESS_RO
/**
 * active_valid [10:10], dòng bảng 277, kiểu truy cập RO
 * Đặt sau SOF cục bộ chọn bộ đã xác nhận; trước đó hoặc sau khi xóa hợp lệ thì bằng 0. Độc lập BUSY
 * và enable.
 */
#define FX1_ISP_EE_STATUS_ACTIVE_VALID_SHIFT 10U
#define FX1_ISP_EE_STATUS_ACTIVE_VALID_WIDTH 1U
#define FX1_ISP_EE_STATUS_ACTIVE_VALID_MASK 0x00000400U
#define FX1_ISP_EE_STATUS_ACTIVE_VALID_RESET 0x00000000U
#define FX1_ISP_EE_STATUS_ACTIVE_VALID_ACCESS FX1_ISP_ACCESS_RO

/**
 * EE_LUT_RDATA: địa chỉ lệch 0x1C3C, dòng bảng 278, kiểu truy cập RO
 * Đọc bảng EE theo PROFILE_WR_SEL, LUT_SEL và LUT_ADDR. Selector 3 đọc profile 0; nội dung chưa xác
 * định trước khi nạp.
 */
#define FX1_ISP_EE_LUT_RDATA_OFFSET 0x1C3CU
#define REG_EE_LUT_RDATA FX1_ISP_EE_LUT_RDATA_OFFSET
#define FX1_ISP_EE_LUT_RDATA_RESET 0x00000000U
#define FX1_ISP_EE_LUT_RDATA_ACCESS FX1_ISP_ACCESS_RO
#define FX1_ISP_EE_LUT_RDATA_RO_MASK 0x0000FFFFU
#define FX1_ISP_EE_LUT_RDATA_RW_MASK 0x00000000U
#define FX1_ISP_EE_LUT_RDATA_W1C_MASK 0x00000000U
#define FX1_ISP_EE_LUT_RDATA_W1S_MASK 0x00000000U
#define FX1_ISP_EE_LUT_RDATA_W1P_MASK 0x00000000U
#define FX1_ISP_EE_LUT_RDATA_W1SC_MASK 0x00000000U
/**
 * rdata [15:0], dòng bảng 279, kiểu truy cập RO
 * Nội dung điểm của bảng gain đang chọn.
 */
#define FX1_ISP_EE_LUT_RDATA_RDATA_SHIFT 0U
#define FX1_ISP_EE_LUT_RDATA_RDATA_WIDTH 16U
#define FX1_ISP_EE_LUT_RDATA_RDATA_MASK 0x0000FFFFU
#define FX1_ISP_EE_LUT_RDATA_RDATA_RESET 0x00000000U
#define FX1_ISP_EE_LUT_RDATA_RDATA_ACCESS FX1_ISP_ACCESS_RO

/**
 * CNF_CTRL: địa chỉ lệch 0x1E00, dòng bảng 280, kiểu truy cập RW
 * CNF: lọc màu 5x5, chỉ lấy trung bình láng giềng có luma/chroma đủ gần, làm mượt nhiễu mà hạn chế
 * loang màu qua cạnh.
 */
#define FX1_ISP_CNF_CTRL_OFFSET 0x1E00U
#define REG_CNF_CTRL FX1_ISP_CNF_CTRL_OFFSET
#define FX1_ISP_CNF_CTRL_RESET 0x00000000U
#define FX1_ISP_CNF_CTRL_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_CNF_CTRL_RO_MASK 0x00000000U
#define FX1_ISP_CNF_CTRL_RW_MASK 0x00000005U
#define FX1_ISP_CNF_CTRL_W1C_MASK 0x00000000U
#define FX1_ISP_CNF_CTRL_W1S_MASK 0x00000000U
#define FX1_ISP_CNF_CTRL_W1P_MASK 0x00000000U
#define FX1_ISP_CNF_CTRL_W1SC_MASK 0x00000000U
/**
 * en [0:0], dòng bảng 281, kiểu truy cập RW
 * Bật CNF: 1 lọc chroma, 0 truyền nguyên.
 */
#define FX1_ISP_CNF_CTRL_EN_SHIFT 0U
#define FX1_ISP_CNF_CTRL_EN_WIDTH 1U
#define FX1_ISP_CNF_CTRL_EN_MASK 0x00000001U
#define FX1_ISP_CNF_CTRL_EN_RESET 0x00000000U
#define FX1_ISP_CNF_CTRL_EN_ACCESS FX1_ISP_ACCESS_RW
/**
 * updated [2:2], dòng bảng 282, kiểu truy cập RW
 * Trường cập nhật chỉ lưu tương thích, không có tác dụng COMMIT; enable/ngưỡng được lấy tại SOF được
 * nhận.
 */
#define FX1_ISP_CNF_CTRL_UPDATED_SHIFT 2U
#define FX1_ISP_CNF_CTRL_UPDATED_WIDTH 1U
#define FX1_ISP_CNF_CTRL_UPDATED_MASK 0x00000004U
#define FX1_ISP_CNF_CTRL_UPDATED_RESET 0x00000000U
#define FX1_ISP_CNF_CTRL_UPDATED_ACCESS FX1_ISP_ACCESS_RW

/**
 * CNF_CHROMA_TH: địa chỉ lệch 0x1E04, dòng bảng 283, kiểu truy cập RW
 * Ngưỡng sai khác chroma cho phép láng giềng tham gia trung bình.
 */
#define FX1_ISP_CNF_CHROMA_TH_OFFSET 0x1E04U
#define REG_CNF_CHROMA_TH FX1_ISP_CNF_CHROMA_TH_OFFSET
#define FX1_ISP_CNF_CHROMA_TH_RESET 0x00000000U
#define FX1_ISP_CNF_CHROMA_TH_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_CNF_CHROMA_TH_RO_MASK 0x00000000U
#define FX1_ISP_CNF_CHROMA_TH_RW_MASK 0x000001FFU
#define FX1_ISP_CNF_CHROMA_TH_W1C_MASK 0x00000000U
#define FX1_ISP_CNF_CHROMA_TH_W1S_MASK 0x00000000U
#define FX1_ISP_CNF_CHROMA_TH_W1P_MASK 0x00000000U
#define FX1_ISP_CNF_CHROMA_TH_W1SC_MASK 0x00000000U
/**
 * chroma_th [8:0], dòng bảng 284, kiểu truy cập RW
 * Ngưỡng chroma hợp lệ 0..510, so với abs(U-Uc)+abs(V-Vc). Mã 511 dự phòng vẫn ghi được và xử lý như
 * 510.
 */
#define FX1_ISP_CNF_CHROMA_TH_CHROMA_TH_SHIFT 0U
#define FX1_ISP_CNF_CHROMA_TH_CHROMA_TH_WIDTH 9U
#define FX1_ISP_CNF_CHROMA_TH_CHROMA_TH_MASK 0x000001FFU
#define FX1_ISP_CNF_CHROMA_TH_CHROMA_TH_RESET 0x00000000U
#define FX1_ISP_CNF_CHROMA_TH_CHROMA_TH_ACCESS FX1_ISP_ACCESS_RW

/**
 * CNF_LUMA_TH: địa chỉ lệch 0x1E08, dòng bảng 285, kiểu truy cập RW
 * Ngưỡng sai khác luma cho phép tham gia trung bình, giúp tránh lấy mẫu qua cạnh.
 */
#define FX1_ISP_CNF_LUMA_TH_OFFSET 0x1E08U
#define REG_CNF_LUMA_TH FX1_ISP_CNF_LUMA_TH_OFFSET
#define FX1_ISP_CNF_LUMA_TH_RESET 0x00000000U
#define FX1_ISP_CNF_LUMA_TH_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_CNF_LUMA_TH_RO_MASK 0x00000000U
#define FX1_ISP_CNF_LUMA_TH_RW_MASK 0x000000FFU
#define FX1_ISP_CNF_LUMA_TH_W1C_MASK 0x00000000U
#define FX1_ISP_CNF_LUMA_TH_W1S_MASK 0x00000000U
#define FX1_ISP_CNF_LUMA_TH_W1P_MASK 0x00000000U
#define FX1_ISP_CNF_LUMA_TH_W1SC_MASK 0x00000000U
/**
 * luma_th [7:0], dòng bảng 286, kiểu truy cập RW
 * Ngưỡng tương đồng luma T_Y, 0..255 theo mã luma.
 */
#define FX1_ISP_CNF_LUMA_TH_LUMA_TH_SHIFT 0U
#define FX1_ISP_CNF_LUMA_TH_LUMA_TH_WIDTH 8U
#define FX1_ISP_CNF_LUMA_TH_LUMA_TH_MASK 0x000000FFU
#define FX1_ISP_CNF_LUMA_TH_LUMA_TH_RESET 0x00000000U
#define FX1_ISP_CNF_LUMA_TH_LUMA_TH_ACCESS FX1_ISP_ACCESS_RW

/**
 * CNF_STRENGTH: địa chỉ lệch 0x1E0C, dòng bảng 287, kiểu truy cập RW
 * STRENGTH CNF dự phòng, không tác động; bộ lọc chỉ dùng enable và ngưỡng luma/chroma.
 */
#define FX1_ISP_CNF_STRENGTH_OFFSET 0x1E0CU
#define REG_CNF_STRENGTH FX1_ISP_CNF_STRENGTH_OFFSET
#define FX1_ISP_CNF_STRENGTH_RESET 0x00000000U
#define FX1_ISP_CNF_STRENGTH_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_CNF_STRENGTH_RO_MASK 0x00000000U
#define FX1_ISP_CNF_STRENGTH_RW_MASK 0x00000003U
#define FX1_ISP_CNF_STRENGTH_W1C_MASK 0x00000000U
#define FX1_ISP_CNF_STRENGTH_W1S_MASK 0x00000000U
#define FX1_ISP_CNF_STRENGTH_W1P_MASK 0x00000000U
#define FX1_ISP_CNF_STRENGTH_W1SC_MASK 0x00000000U
/**
 * mode_strength [1:0], dòng bảng 288, kiểu truy cập RW
 * Trường cường độ chưa triển khai, không tác động xử lý.
 */
#define FX1_ISP_CNF_STRENGTH_MODE_STRENGTH_SHIFT 0U
#define FX1_ISP_CNF_STRENGTH_MODE_STRENGTH_WIDTH 2U
#define FX1_ISP_CNF_STRENGTH_MODE_STRENGTH_MASK 0x00000003U
#define FX1_ISP_CNF_STRENGTH_MODE_STRENGTH_RESET 0x00000000U
#define FX1_ISP_CNF_STRENGTH_MODE_STRENGTH_ACCESS FX1_ISP_ACCESS_RW

/**
 * CNF_STATUS: địa chỉ lệch 0x1E10, dòng bảng 289, kiểu truy cập RO
 * Trạng thái CNF, chỉ đọc.
 */
#define FX1_ISP_CNF_STATUS_OFFSET 0x1E10U
#define REG_CNF_STATUS FX1_ISP_CNF_STATUS_OFFSET
#define FX1_ISP_CNF_STATUS_RESET 0x00000000U
#define FX1_ISP_CNF_STATUS_ACCESS FX1_ISP_ACCESS_RO
#define FX1_ISP_CNF_STATUS_RO_MASK 0x00000001U
#define FX1_ISP_CNF_STATUS_RW_MASK 0x00000000U
#define FX1_ISP_CNF_STATUS_W1C_MASK 0x00000000U
#define FX1_ISP_CNF_STATUS_W1S_MASK 0x00000000U
#define FX1_ISP_CNF_STATUS_W1P_MASK 0x00000000U
#define FX1_ISP_CNF_STATUS_W1SC_MASK 0x00000000U
/**
 * busy [0:0], dòng bảng 290, kiểu truy cập RO
 * BUSY=1 khi khối còn giữ pixel đang xử lý. Tuân thủ trình tự cấu hình/nạp LUT riêng; chỉ thấy
 * BUSY=0 chưa đủ quyền ghi đè bảng đang dùng.
 */
#define FX1_ISP_CNF_STATUS_BUSY_SHIFT 0U
#define FX1_ISP_CNF_STATUS_BUSY_WIDTH 1U
#define FX1_ISP_CNF_STATUS_BUSY_MASK 0x00000001U
#define FX1_ISP_CNF_STATUS_BUSY_RESET 0x00000000U
#define FX1_ISP_CNF_STATUS_BUSY_ACCESS FX1_ISP_ACCESS_RO

/**
 * RESIZER_CTRL: địa chỉ lệch 0x2000, dòng bảng 291, kiểu truy cập RW
 * Resize CH0 NV12 tối đa 3840x2160; RESIZER_* không tiền tố thuộc CH0. Đầu vào là crop sau IQ, có
 * thể lớn hơn đích. Không phóng to/kích thước tùy ý; từ chối đích sai trước cấp quyền DMA, không tự
 * về FullRes. Chờ OUT_W/H ổn định.
 */
#define FX1_ISP_RESIZER_CTRL_OFFSET 0x2000U
#define REG_RESIZER_CTRL FX1_ISP_RESIZER_CTRL_OFFSET
#define FX1_ISP_RESIZER_CTRL_RESET 0x00000000U
#define FX1_ISP_RESIZER_CTRL_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_RESIZER_CTRL_RO_MASK 0x00000000U
#define FX1_ISP_RESIZER_CTRL_RW_MASK 0x000000FDU
#define FX1_ISP_RESIZER_CTRL_W1C_MASK 0x00000000U
#define FX1_ISP_RESIZER_CTRL_W1S_MASK 0x00000000U
#define FX1_ISP_RESIZER_CTRL_W1P_MASK 0x00000000U
#define FX1_ISP_RESIZER_CTRL_W1SC_MASK 0x00000000U
/**
 * en [0:0], dòng bảng 292, kiểu truy cập RW
 * Bật resize CH0: 1 dùng SCALE, 0 giữ kích thước crop. Cả hai phải vừa 3840x2160. Chỉ đổi hình học
 * sau dừng/xử lý hết; kiểm tra trước DMA.
 */
#define FX1_ISP_RESIZER_CTRL_EN_SHIFT 0U
#define FX1_ISP_RESIZER_CTRL_EN_WIDTH 1U
#define FX1_ISP_RESIZER_CTRL_EN_MASK 0x00000001U
#define FX1_ISP_RESIZER_CTRL_EN_RESET 0x00000000U
#define FX1_ISP_RESIZER_CTRL_EN_ACCESS FX1_ISP_ACCESS_RW
/**
 * updated [2:2], dòng bảng 293, kiểu truy cập RW
 * Trường gợi ý cập nhật legacy chỉ lưu, không COMMIT. Đổi active size/crop/enable/SCALE kích hoạt
 * tính lại hình học; kích thước tùy ý dự phòng. Chờ OUT_W/H ổn định trước DMA; chốt hình học tại
 * SOF.
 */
#define FX1_ISP_RESIZER_CTRL_UPDATED_SHIFT 2U
#define FX1_ISP_RESIZER_CTRL_UPDATED_WIDTH 1U
#define FX1_ISP_RESIZER_CTRL_UPDATED_MASK 0x00000004U
#define FX1_ISP_RESIZER_CTRL_UPDATED_RESET 0x00000000U
#define FX1_ISP_RESIZER_CTRL_UPDATED_ACCESS FX1_ISP_ACCESS_RW
/**
 * scale [7:3], dòng bảng 294, kiểu truy cập RW
 * Mã resize CH0 [7:3]: 0 giữ crop FullRes; 1..14 mã chuẩn; 16=640x640, 17=640x384, 18=640x352,
 * 19=512x512, 20=416x416, 21=320x320. Mã 15,22..31 dự phòng. Đích tối đa 3840x2160 và không lớn hơn
 * crop; không phóng to.
 */
#define FX1_ISP_RESIZER_CTRL_SCALE_SHIFT 3U
#define FX1_ISP_RESIZER_CTRL_SCALE_WIDTH 5U
#define FX1_ISP_RESIZER_CTRL_SCALE_MASK 0x000000F8U
#define FX1_ISP_RESIZER_CTRL_SCALE_RESET 0x00000000U
#define FX1_ISP_RESIZER_CTRL_SCALE_ACCESS FX1_ISP_ACCESS_RW

/**
 * RESIZER_CUSTOM_W: địa chỉ lệch 0x2004, dòng bảng 295, kiểu truy cập RO
 * Địa chỉ kích thước đích cũ, dự phòng ở ABI 1.07: đọc 0, bỏ ghi. Không hỗ trợ resize kích thước tùy
 * ý.
 */
#define FX1_ISP_RESIZER_CUSTOM_W_OFFSET 0x2004U
#define REG_RESIZER_CUSTOM_W FX1_ISP_RESIZER_CUSTOM_W_OFFSET
#define FX1_ISP_RESIZER_CUSTOM_W_RESET 0x00000000U
#define FX1_ISP_RESIZER_CUSTOM_W_ACCESS FX1_ISP_ACCESS_RO
#define FX1_ISP_RESIZER_CUSTOM_W_RO_MASK 0xFFFFFFFFU
#define FX1_ISP_RESIZER_CUSTOM_W_RW_MASK 0x00000000U
#define FX1_ISP_RESIZER_CUSTOM_W_W1C_MASK 0x00000000U
#define FX1_ISP_RESIZER_CUSTOM_W_W1S_MASK 0x00000000U
#define FX1_ISP_RESIZER_CUSTOM_W_W1P_MASK 0x00000000U
#define FX1_ISP_RESIZER_CUSTOM_W_W1SC_MASK 0x00000000U
/**
 * RESERVED [31:0], dòng bảng 296, kiểu truy cập RO
 * Đọc 0, bỏ qua ghi.
 */
#define FX1_ISP_RESIZER_CUSTOM_W_RESERVED_SHIFT 0U
#define FX1_ISP_RESIZER_CUSTOM_W_RESERVED_WIDTH 32U
#define FX1_ISP_RESIZER_CUSTOM_W_RESERVED_MASK 0xFFFFFFFFU
#define FX1_ISP_RESIZER_CUSTOM_W_RESERVED_RESET 0x00000000U
#define FX1_ISP_RESIZER_CUSTOM_W_RESERVED_ACCESS FX1_ISP_ACCESS_RO

/**
 * RESIZER_CUSTOM_H: địa chỉ lệch 0x2008, dòng bảng 297, kiểu truy cập RO
 * Địa chỉ kích thước đích cũ, dự phòng ở ABI 1.07: đọc 0, bỏ ghi. Không hỗ trợ resize kích thước tùy
 * ý.
 */
#define FX1_ISP_RESIZER_CUSTOM_H_OFFSET 0x2008U
#define REG_RESIZER_CUSTOM_H FX1_ISP_RESIZER_CUSTOM_H_OFFSET
#define FX1_ISP_RESIZER_CUSTOM_H_RESET 0x00000000U
#define FX1_ISP_RESIZER_CUSTOM_H_ACCESS FX1_ISP_ACCESS_RO
#define FX1_ISP_RESIZER_CUSTOM_H_RO_MASK 0xFFFFFFFFU
#define FX1_ISP_RESIZER_CUSTOM_H_RW_MASK 0x00000000U
#define FX1_ISP_RESIZER_CUSTOM_H_W1C_MASK 0x00000000U
#define FX1_ISP_RESIZER_CUSTOM_H_W1S_MASK 0x00000000U
#define FX1_ISP_RESIZER_CUSTOM_H_W1P_MASK 0x00000000U
#define FX1_ISP_RESIZER_CUSTOM_H_W1SC_MASK 0x00000000U
/**
 * RESERVED [31:0], dòng bảng 298, kiểu truy cập RO
 * Đọc 0, bỏ qua ghi.
 */
#define FX1_ISP_RESIZER_CUSTOM_H_RESERVED_SHIFT 0U
#define FX1_ISP_RESIZER_CUSTOM_H_RESERVED_WIDTH 32U
#define FX1_ISP_RESIZER_CUSTOM_H_RESERVED_MASK 0xFFFFFFFFU
#define FX1_ISP_RESIZER_CUSTOM_H_RESERVED_RESET 0x00000000U
#define FX1_ISP_RESIZER_CUSTOM_H_RESERVED_ACCESS FX1_ISP_ACCESS_RO

/**
 * RESIZER_OUT_W: địa chỉ lệch 0x2014, dòng bảng 299, kiểu truy cập RO
 * Kích thước đầu ra suy ra đã kiểm tra. Reset 0; có thể giữ giá trị hoàn tất trước khi hình học hiện
 * tại chưa hợp lệ. Kiểm tra GEOMETRY_STATUS của kênh trước DMA.
 */
#define FX1_ISP_RESIZER_OUT_W_OFFSET 0x2014U
#define REG_RESIZER_OUT_W FX1_ISP_RESIZER_OUT_W_OFFSET
#define FX1_ISP_RESIZER_OUT_W_RESET 0x00000000U
#define FX1_ISP_RESIZER_OUT_W_ACCESS FX1_ISP_ACCESS_RO
#define FX1_ISP_RESIZER_OUT_W_RO_MASK 0x00001FFFU
#define FX1_ISP_RESIZER_OUT_W_RW_MASK 0x00000000U
#define FX1_ISP_RESIZER_OUT_W_W1C_MASK 0x00000000U
#define FX1_ISP_RESIZER_OUT_W_W1S_MASK 0x00000000U
#define FX1_ISP_RESIZER_OUT_W_W1P_MASK 0x00000000U
#define FX1_ISP_RESIZER_OUT_W_W1SC_MASK 0x00000000U
/**
 * out_width [12:0], dòng bảng 300, kiểu truy cập RO
 * Kích thước đầu ra suy ra đã kiểm tra. Reset 0; có thể giữ giá trị hoàn tất trước khi hình học hiện
 * tại chưa hợp lệ. Kiểm tra GEOMETRY_STATUS của kênh trước DMA.
 */
#define FX1_ISP_RESIZER_OUT_W_OUT_WIDTH_SHIFT 0U
#define FX1_ISP_RESIZER_OUT_W_OUT_WIDTH_WIDTH 13U
#define FX1_ISP_RESIZER_OUT_W_OUT_WIDTH_MASK 0x00001FFFU
#define FX1_ISP_RESIZER_OUT_W_OUT_WIDTH_RESET 0x00000000U
#define FX1_ISP_RESIZER_OUT_W_OUT_WIDTH_ACCESS FX1_ISP_ACCESS_RO

/**
 * RESIZER_OUT_H: địa chỉ lệch 0x2018, dòng bảng 301, kiểu truy cập RO
 * Kích thước đầu ra suy ra đã kiểm tra. Reset 0; có thể giữ giá trị hoàn tất trước khi hình học hiện
 * tại chưa hợp lệ. Kiểm tra GEOMETRY_STATUS của kênh trước DMA.
 */
#define FX1_ISP_RESIZER_OUT_H_OFFSET 0x2018U
#define REG_RESIZER_OUT_H FX1_ISP_RESIZER_OUT_H_OFFSET
#define FX1_ISP_RESIZER_OUT_H_RESET 0x00000000U
#define FX1_ISP_RESIZER_OUT_H_ACCESS FX1_ISP_ACCESS_RO
#define FX1_ISP_RESIZER_OUT_H_RO_MASK 0x00001FFFU
#define FX1_ISP_RESIZER_OUT_H_RW_MASK 0x00000000U
#define FX1_ISP_RESIZER_OUT_H_W1C_MASK 0x00000000U
#define FX1_ISP_RESIZER_OUT_H_W1S_MASK 0x00000000U
#define FX1_ISP_RESIZER_OUT_H_W1P_MASK 0x00000000U
#define FX1_ISP_RESIZER_OUT_H_W1SC_MASK 0x00000000U
/**
 * out_height [12:0], dòng bảng 302, kiểu truy cập RO
 * Kích thước đầu ra suy ra đã kiểm tra. Reset 0; có thể giữ giá trị hoàn tất trước khi hình học hiện
 * tại chưa hợp lệ. Kiểm tra GEOMETRY_STATUS của kênh trước DMA.
 */
#define FX1_ISP_RESIZER_OUT_H_OUT_HEIGHT_SHIFT 0U
#define FX1_ISP_RESIZER_OUT_H_OUT_HEIGHT_WIDTH 13U
#define FX1_ISP_RESIZER_OUT_H_OUT_HEIGHT_MASK 0x00001FFFU
#define FX1_ISP_RESIZER_OUT_H_OUT_HEIGHT_RESET 0x00000000U
#define FX1_ISP_RESIZER_OUT_H_OUT_HEIGHT_ACCESS FX1_ISP_ACCESS_RO

/**
 * RESIZER_STATUS: địa chỉ lệch 0x201C, dòng bảng 303, kiểu truy cập RO
 * Trạng thái khối resizer.
 */
#define FX1_ISP_RESIZER_STATUS_OFFSET 0x201CU
#define REG_RESIZER_STATUS FX1_ISP_RESIZER_STATUS_OFFSET
#define FX1_ISP_RESIZER_STATUS_RESET 0x00000000U
#define FX1_ISP_RESIZER_STATUS_ACCESS FX1_ISP_ACCESS_RO
#define FX1_ISP_RESIZER_STATUS_RO_MASK 0x00000001U
#define FX1_ISP_RESIZER_STATUS_RW_MASK 0x00000000U
#define FX1_ISP_RESIZER_STATUS_W1C_MASK 0x00000000U
#define FX1_ISP_RESIZER_STATUS_W1S_MASK 0x00000000U
#define FX1_ISP_RESIZER_STATUS_W1P_MASK 0x00000000U
#define FX1_ISP_RESIZER_STATUS_W1SC_MASK 0x00000000U
/**
 * busy [0:0], dòng bảng 304, kiểu truy cập RO
 * BUSY=1 khi khối còn giữ pixel đang xử lý. Tuân thủ trình tự cấu hình/nạp LUT riêng; chỉ thấy
 * BUSY=0 chưa đủ quyền ghi đè bảng đang dùng.
 */
#define FX1_ISP_RESIZER_STATUS_BUSY_SHIFT 0U
#define FX1_ISP_RESIZER_STATUS_BUSY_WIDTH 1U
#define FX1_ISP_RESIZER_STATUS_BUSY_MASK 0x00000001U
#define FX1_ISP_RESIZER_STATUS_BUSY_RESET 0x00000000U
#define FX1_ISP_RESIZER_STATUS_BUSY_ACCESS FX1_ISP_ACCESS_RO

/**
 * OUTPUT_ENABLE: địa chỉ lệch 0x2020, dòng bảng 305, kiểu truy cập RW
 * Các kênh đầu ra bật, mặc định CH0. Bộ phân phối chốt mặt nạ tại SOF được nhận; các kênh dùng chung
 * cơ chế chặn ngược không mất dữ liệu.
 */
#define FX1_ISP_OUTPUT_ENABLE_OFFSET 0x2020U
#define REG_OUTPUT_ENABLE FX1_ISP_OUTPUT_ENABLE_OFFSET
#define FX1_ISP_OUTPUT_ENABLE_RESET 0x00000001U
#define FX1_ISP_OUTPUT_ENABLE_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_OUTPUT_ENABLE_RO_MASK 0x00000000U
#define FX1_ISP_OUTPUT_ENABLE_RW_MASK 0x00000007U
#define FX1_ISP_OUTPUT_ENABLE_W1C_MASK 0x00000000U
#define FX1_ISP_OUTPUT_ENABLE_W1S_MASK 0x00000000U
#define FX1_ISP_OUTPUT_ENABLE_W1P_MASK 0x00000000U
#define FX1_ISP_OUTPUT_ENABLE_W1SC_MASK 0x00000000U
/**
 * channels [2:0], dòng bảng 306, kiểu truy cập RW
 * Bit 0=Preview NV12, bit 1=Video NV12, bit 2=NPU RGB888. Chỉ đổi khi dừng đầu vào và mọi đường ra
 * xử lý hết.
 */
#define FX1_ISP_OUTPUT_ENABLE_CHANNELS_SHIFT 0U
#define FX1_ISP_OUTPUT_ENABLE_CHANNELS_WIDTH 3U
#define FX1_ISP_OUTPUT_ENABLE_CHANNELS_MASK 0x00000007U
#define FX1_ISP_OUTPUT_ENABLE_CHANNELS_RESET 0x00000001U
#define FX1_ISP_OUTPUT_ENABLE_CHANNELS_ACCESS FX1_ISP_ACCESS_RW

/**
 * OUTPUT_IRQ_EN: địa chỉ lệch 0x2024, dòng bảng 307, kiểu truy cập RW
 * Cho phép ngắt theo từng đầu ra.
 */
#define FX1_ISP_OUTPUT_IRQ_EN_OFFSET 0x2024U
#define REG_OUTPUT_IRQ_EN FX1_ISP_OUTPUT_IRQ_EN_OFFSET
#define FX1_ISP_OUTPUT_IRQ_EN_RESET 0x00000000U
#define FX1_ISP_OUTPUT_IRQ_EN_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_OUTPUT_IRQ_EN_RO_MASK 0x00000000U
#define FX1_ISP_OUTPUT_IRQ_EN_RW_MASK 0x00000077U
#define FX1_ISP_OUTPUT_IRQ_EN_W1C_MASK 0x00000000U
#define FX1_ISP_OUTPUT_IRQ_EN_W1S_MASK 0x00000000U
#define FX1_ISP_OUTPUT_IRQ_EN_W1P_MASK 0x00000000U
#define FX1_ISP_OUTPUT_IRQ_EN_W1SC_MASK 0x00000000U
/**
 * done [2:0], dòng bảng 308, kiểu truy cập RW
 * Mặt nạ ngắt hoàn tất: bit 0..2 tương ứng CH0..CH2; 1 cho đóng góp ngắt, 0 che.
 */
#define FX1_ISP_OUTPUT_IRQ_EN_DONE_SHIFT 0U
#define FX1_ISP_OUTPUT_IRQ_EN_DONE_WIDTH 3U
#define FX1_ISP_OUTPUT_IRQ_EN_DONE_MASK 0x00000007U
#define FX1_ISP_OUTPUT_IRQ_EN_DONE_RESET 0x00000000U
#define FX1_ISP_OUTPUT_IRQ_EN_DONE_ACCESS FX1_ISP_ACCESS_RW
/**
 * error [6:4], dòng bảng 309, kiểu truy cập RW
 * Mặt nạ ngắt lỗi: bit 0..2 tương ứng CH0..CH2; 1 cho đóng góp ngắt, 0 che.
 */
#define FX1_ISP_OUTPUT_IRQ_EN_ERROR_SHIFT 4U
#define FX1_ISP_OUTPUT_IRQ_EN_ERROR_WIDTH 3U
#define FX1_ISP_OUTPUT_IRQ_EN_ERROR_MASK 0x00000070U
#define FX1_ISP_OUTPUT_IRQ_EN_ERROR_RESET 0x00000000U
#define FX1_ISP_OUTPUT_IRQ_EN_ERROR_ACCESS FX1_ISP_ACCESS_RW

/**
 * OUTPUT_IRQ_STATUS: địa chỉ lệch 0x2028, dòng bảng 310, kiểu truy cập W1C
 * Sự kiện hoàn tất/lỗi theo kênh, sự kiện phần cứng ưu tiên hơn xóa phần mềm. Hoàn tất phải đợi phản
 * hồi ghi.
 */
#define FX1_ISP_OUTPUT_IRQ_STATUS_OFFSET 0x2028U
#define REG_OUTPUT_IRQ_STATUS FX1_ISP_OUTPUT_IRQ_STATUS_OFFSET
#define FX1_ISP_OUTPUT_IRQ_STATUS_RESET 0x00000000U
#define FX1_ISP_OUTPUT_IRQ_STATUS_ACCESS FX1_ISP_ACCESS_W1C
#define FX1_ISP_OUTPUT_IRQ_STATUS_RO_MASK 0x00000000U
#define FX1_ISP_OUTPUT_IRQ_STATUS_RW_MASK 0x00000000U
#define FX1_ISP_OUTPUT_IRQ_STATUS_W1C_MASK 0x00000077U
#define FX1_ISP_OUTPUT_IRQ_STATUS_W1S_MASK 0x00000000U
#define FX1_ISP_OUTPUT_IRQ_STATUS_W1P_MASK 0x00000000U
#define FX1_ISP_OUTPUT_IRQ_STATUS_W1SC_MASK 0x00000000U
/**
 * done [2:0], dòng bảng 311, kiểu truy cập W1C
 * Sự kiện hoàn tất W1C: bit c đặt khi kênh c công bố ODMA_BUF_DONE. Bit 0..2 ứng CH0..CH2; đặt mới
 * thắng xóa. Xóa sự kiện không giải phóng quyền sở hữu bộ đệm.
 */
#define FX1_ISP_OUTPUT_IRQ_STATUS_DONE_SHIFT 0U
#define FX1_ISP_OUTPUT_IRQ_STATUS_DONE_WIDTH 3U
#define FX1_ISP_OUTPUT_IRQ_STATUS_DONE_MASK 0x00000007U
#define FX1_ISP_OUTPUT_IRQ_STATUS_DONE_RESET 0x00000000U
#define FX1_ISP_OUTPUT_IRQ_STATUS_DONE_ACCESS FX1_ISP_ACCESS_W1C
/**
 * error [6:4], dòng bảng 312, kiểu truy cập W1C
 * Sự kiện lỗi W1C: bit c đặt khi recovery kết thúc đầu ra kênh c bật mà không có DONE. Bit 0..2 ứng
 * CH0..CH2; đặt mới thắng xóa, xóa không giải phóng bộ đệm.
 */
#define FX1_ISP_OUTPUT_IRQ_STATUS_ERROR_SHIFT 4U
#define FX1_ISP_OUTPUT_IRQ_STATUS_ERROR_WIDTH 3U
#define FX1_ISP_OUTPUT_IRQ_STATUS_ERROR_MASK 0x00000070U
#define FX1_ISP_OUTPUT_IRQ_STATUS_ERROR_RESET 0x00000000U
#define FX1_ISP_OUTPUT_IRQ_STATUS_ERROR_ACCESS FX1_ISP_ACCESS_W1C

/**
 * AEC_CTRL: địa chỉ lệch 0x2200, dòng bảng 313, kiểu truy cập Mixed
 * AEC chỉ đo thống kê, không điều khiển phơi sáng. Phần mềm đọc tổng/số mẫu/histogram rồi quyết định
 * tham số cảm biến.
 */
#define FX1_ISP_AEC_CTRL_OFFSET 0x2200U
#define REG_AEC_CTRL FX1_ISP_AEC_CTRL_OFFSET
#define FX1_ISP_AEC_CTRL_RESET 0x00000000U
#define FX1_ISP_AEC_CTRL_ACCESS FX1_ISP_ACCESS_MIXED
#define FX1_ISP_AEC_CTRL_RO_MASK 0x00000000U
#define FX1_ISP_AEC_CTRL_RW_MASK 0x00000001U
#define FX1_ISP_AEC_CTRL_W1C_MASK 0x00000000U
#define FX1_ISP_AEC_CTRL_W1S_MASK 0x00000002U
#define FX1_ISP_AEC_CTRL_W1P_MASK 0x00000000U
#define FX1_ISP_AEC_CTRL_W1SC_MASK 0x00000000U
/**
 * en [0:0], dòng bảng 314, kiểu truy cập RW
 * Bật thống kê AE: 1 tích lũy, 0 rỗi.
 */
#define FX1_ISP_AEC_CTRL_EN_SHIFT 0U
#define FX1_ISP_AEC_CTRL_EN_WIDTH 1U
#define FX1_ISP_AEC_CTRL_EN_MASK 0x00000001U
#define FX1_ISP_AEC_CTRL_EN_RESET 0x00000000U
#define FX1_ISP_AEC_CTRL_EN_ACCESS FX1_ISP_ACCESS_RW
/**
 * commit [1:1], dòng bảng 315, kiểu truy cập W1S
 * Ghi COMMIT=1 để chuyển enable/hình học zone/clip/ngưỡng từ cấu hình đệm tại SOF đầu vào được nhận
 * kế tiếp. Tự xóa khi chuyển. Chiều cao ảnh hữu dụng chốt riêng theo frame, không thuộc lệnh này.
 */
#define FX1_ISP_AEC_CTRL_COMMIT_SHIFT 1U
#define FX1_ISP_AEC_CTRL_COMMIT_WIDTH 1U
#define FX1_ISP_AEC_CTRL_COMMIT_MASK 0x00000002U
#define FX1_ISP_AEC_CTRL_COMMIT_RESET 0x00000000U
#define FX1_ISP_AEC_CTRL_COMMIT_ACCESS FX1_ISP_ACCESS_W1S

/**
 * AEC_CONTEXT_ID: địa chỉ lệch 0x2204, dòng bảng 316, kiểu truy cập RW
 * Nhãn phần mềm AEC đi cùng thống kê để nhận diện yêu cầu phơi sáng có độ trễ vài frame. Ghi ở đây,
 * đọc AEC_RESULT_CONTEXT_ID cùng frame tương ứng.
 */
#define FX1_ISP_AEC_CONTEXT_ID_OFFSET 0x2204U
#define REG_AEC_CONTEXT_ID FX1_ISP_AEC_CONTEXT_ID_OFFSET
#define FX1_ISP_AEC_CONTEXT_ID_RESET 0x00000000U
#define FX1_ISP_AEC_CONTEXT_ID_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_AEC_CONTEXT_ID_RO_MASK 0x00000000U
#define FX1_ISP_AEC_CONTEXT_ID_RW_MASK 0xFFFFFFFFU
#define FX1_ISP_AEC_CONTEXT_ID_W1C_MASK 0x00000000U
#define FX1_ISP_AEC_CONTEXT_ID_W1S_MASK 0x00000000U
#define FX1_ISP_AEC_CONTEXT_ID_W1P_MASK 0x00000000U
#define FX1_ISP_AEC_CONTEXT_ID_W1SC_MASK 0x00000000U
/**
 * context_id [31:0], dòng bảng 317, kiểu truy cập RW
 * Giá trị context phần mềm chốt tại SOF đầu vào. Ghi trước tiếp nhận frame; công bố trong
 * AEC_RESULT_CONTEXT_ID.
 */
#define FX1_ISP_AEC_CONTEXT_ID_CONTEXT_ID_SHIFT 0U
#define FX1_ISP_AEC_CONTEXT_ID_CONTEXT_ID_WIDTH 32U
#define FX1_ISP_AEC_CONTEXT_ID_CONTEXT_ID_MASK 0xFFFFFFFFU
#define FX1_ISP_AEC_CONTEXT_ID_CONTEXT_ID_RESET 0x00000000U
#define FX1_ISP_AEC_CONTEXT_ID_CONTEXT_ID_ACCESS FX1_ISP_ACCESS_RW

/**
 * AEC_ZONE_CFG: địa chỉ lệch 0x2208, dòng bảng 318, kiểu truy cập RW
 * Lưới zone AEC chia frame thành vùng tích lũy độc lập để phần mềm tự đặt trọng số đo.
 */
#define FX1_ISP_AEC_ZONE_CFG_OFFSET 0x2208U
#define REG_AEC_ZONE_CFG FX1_ISP_AEC_ZONE_CFG_OFFSET
#define FX1_ISP_AEC_ZONE_CFG_RESET 0x00001820U
#define FX1_ISP_AEC_ZONE_CFG_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_AEC_ZONE_CFG_RO_MASK 0x00000000U
#define FX1_ISP_AEC_ZONE_CFG_RW_MASK 0x00001F3FU
#define FX1_ISP_AEC_ZONE_CFG_W1C_MASK 0x00000000U
#define FX1_ISP_AEC_ZONE_CFG_W1S_MASK 0x00000000U
#define FX1_ISP_AEC_ZONE_CFG_W1P_MASK 0x00000000U
#define FX1_ISP_AEC_ZONE_CFG_W1SC_MASK 0x00000000U
/**
 * num_zone_x [5:0], dòng bảng 319, kiểu truy cập RW
 * Số zone ngang, 1..32.
 */
#define FX1_ISP_AEC_ZONE_CFG_NUM_ZONE_X_SHIFT 0U
#define FX1_ISP_AEC_ZONE_CFG_NUM_ZONE_X_WIDTH 6U
#define FX1_ISP_AEC_ZONE_CFG_NUM_ZONE_X_MASK 0x0000003FU
#define FX1_ISP_AEC_ZONE_CFG_NUM_ZONE_X_RESET 0x00000020U
#define FX1_ISP_AEC_ZONE_CFG_NUM_ZONE_X_ACCESS FX1_ISP_ACCESS_RW
/**
 * num_zone_y [12:8], dòng bảng 320, kiểu truy cập RW
 * Số zone dọc, 1..24; lưới tối đa 768 zone.
 */
#define FX1_ISP_AEC_ZONE_CFG_NUM_ZONE_Y_SHIFT 8U
#define FX1_ISP_AEC_ZONE_CFG_NUM_ZONE_Y_WIDTH 5U
#define FX1_ISP_AEC_ZONE_CFG_NUM_ZONE_Y_MASK 0x00001F00U
#define FX1_ISP_AEC_ZONE_CFG_NUM_ZONE_Y_RESET 0x00000018U
#define FX1_ISP_AEC_ZONE_CFG_NUM_ZONE_Y_ACCESS FX1_ISP_ACCESS_RW

/**
 * AEC_ZONE_SIZE: địa chỉ lệch 0x220C, dòng bảng 321, kiểu truy cập RW
 * Kích thước zone danh định khác 0, mỗi chiều 1..4095. Cần (Nx-1)*zone_width < active_width <=
 * Nx*zone_width, tương tự Y. Zone cuối phủ phần dư. Có thể dùng một zone toàn ảnh; đọc kết quả 64
 * bit đầy đủ thay cửa sổ hẹp.
 */
#define FX1_ISP_AEC_ZONE_SIZE_OFFSET 0x220CU
#define REG_AEC_ZONE_SIZE FX1_ISP_AEC_ZONE_SIZE_OFFSET
#define FX1_ISP_AEC_ZONE_SIZE_RESET 0x005A0078U
#define FX1_ISP_AEC_ZONE_SIZE_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_AEC_ZONE_SIZE_RO_MASK 0x00000000U
#define FX1_ISP_AEC_ZONE_SIZE_RW_MASK 0x0FFF0FFFU
#define FX1_ISP_AEC_ZONE_SIZE_W1C_MASK 0x00000000U
#define FX1_ISP_AEC_ZONE_SIZE_W1S_MASK 0x00000000U
#define FX1_ISP_AEC_ZONE_SIZE_W1P_MASK 0x00000000U
#define FX1_ISP_AEC_ZONE_SIZE_W1SC_MASK 0x00000000U
/**
 * zone_width [11:0], dòng bảng 322, kiểu truy cập RW
 * Chiều rộng zone danh định theo pixel.
 */
#define FX1_ISP_AEC_ZONE_SIZE_ZONE_WIDTH_SHIFT 0U
#define FX1_ISP_AEC_ZONE_SIZE_ZONE_WIDTH_WIDTH 12U
#define FX1_ISP_AEC_ZONE_SIZE_ZONE_WIDTH_MASK 0x00000FFFU
#define FX1_ISP_AEC_ZONE_SIZE_ZONE_WIDTH_RESET 0x00000078U
#define FX1_ISP_AEC_ZONE_SIZE_ZONE_WIDTH_ACCESS FX1_ISP_ACCESS_RW
/**
 * zone_height [27:16], dòng bảng 323, kiểu truy cập RW
 * Chiều cao zone danh định theo dòng.
 */
#define FX1_ISP_AEC_ZONE_SIZE_ZONE_HEIGHT_SHIFT 16U
#define FX1_ISP_AEC_ZONE_SIZE_ZONE_HEIGHT_WIDTH 12U
#define FX1_ISP_AEC_ZONE_SIZE_ZONE_HEIGHT_MASK 0x0FFF0000U
#define FX1_ISP_AEC_ZONE_SIZE_ZONE_HEIGHT_RESET 0x0000005AU
#define FX1_ISP_AEC_ZONE_SIZE_ZONE_HEIGHT_ACCESS FX1_ISP_ACCESS_RW

/**
 * AEC_SAMPLE_CLIP: địa chỉ lệch 0x2210, dòng bảng 324, kiểu truy cập RW
 * Giới hạn chấp nhận mẫu Bayer RAW12 để cộng tổng/đếm, bao gồm hai đầu mút 0..4095, cần MIN<=MAX.
 * Min/max/histogram quan sát mọi mẫu, độc lập clip.
 */
#define FX1_ISP_AEC_SAMPLE_CLIP_OFFSET 0x2210U
#define REG_AEC_SAMPLE_CLIP FX1_ISP_AEC_SAMPLE_CLIP_OFFSET
#define FX1_ISP_AEC_SAMPLE_CLIP_RESET 0x0FFF0000U
#define FX1_ISP_AEC_SAMPLE_CLIP_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_AEC_SAMPLE_CLIP_RO_MASK 0x00000000U
#define FX1_ISP_AEC_SAMPLE_CLIP_RW_MASK 0x0FFF0FFFU
#define FX1_ISP_AEC_SAMPLE_CLIP_W1C_MASK 0x00000000U
#define FX1_ISP_AEC_SAMPLE_CLIP_W1S_MASK 0x00000000U
#define FX1_ISP_AEC_SAMPLE_CLIP_W1P_MASK 0x00000000U
#define FX1_ISP_AEC_SAMPLE_CLIP_W1SC_MASK 0x00000000U
/**
 * sample_min_clip [11:0], dòng bảng 325, kiểu truy cập RW
 * Mức Bayer RAW12 nhỏ nhất được chấp nhận, gồm đầu mút; dải 0..4095. Giá trị sai làm tắt phép đo của
 * frame.
 */
#define FX1_ISP_AEC_SAMPLE_CLIP_SAMPLE_MIN_CLIP_SHIFT 0U
#define FX1_ISP_AEC_SAMPLE_CLIP_SAMPLE_MIN_CLIP_WIDTH 12U
#define FX1_ISP_AEC_SAMPLE_CLIP_SAMPLE_MIN_CLIP_MASK 0x00000FFFU
#define FX1_ISP_AEC_SAMPLE_CLIP_SAMPLE_MIN_CLIP_RESET 0x00000000U
#define FX1_ISP_AEC_SAMPLE_CLIP_SAMPLE_MIN_CLIP_ACCESS FX1_ISP_ACCESS_RW
/**
 * sample_max_clip [27:16], dòng bảng 326, kiểu truy cập RW
 * Mức Bayer RAW12 lớn nhất được chấp nhận, gồm đầu mút; dải 0..4095. Giá trị sai làm tắt phép đo của
 * frame.
 */
#define FX1_ISP_AEC_SAMPLE_CLIP_SAMPLE_MAX_CLIP_SHIFT 16U
#define FX1_ISP_AEC_SAMPLE_CLIP_SAMPLE_MAX_CLIP_WIDTH 12U
#define FX1_ISP_AEC_SAMPLE_CLIP_SAMPLE_MAX_CLIP_MASK 0x0FFF0000U
#define FX1_ISP_AEC_SAMPLE_CLIP_SAMPLE_MAX_CLIP_RESET 0x00000FFFU
#define FX1_ISP_AEC_SAMPLE_CLIP_SAMPLE_MAX_CLIP_ACCESS FX1_ISP_ACCESS_RW

/**
 * AEC_THRESH: địa chỉ lệch 0x2214, dòng bảng 327, kiểu truy cập RW
 * Ngưỡng OE/UE theo Bayer RAW12: OE khi S>TH_OE, UE khi S<TH_UE, độc lập clip tổng. Bất kỳ ngưỡng
 * nào vượt 4095 làm tắt đo frame đó.
 */
#define FX1_ISP_AEC_THRESH_OFFSET 0x2214U
#define REG_AEC_THRESH FX1_ISP_AEC_THRESH_OFFSET
#define FX1_ISP_AEC_THRESH_RESET 0x0EA00110U
#define FX1_ISP_AEC_THRESH_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_AEC_THRESH_RO_MASK 0x00000000U
#define FX1_ISP_AEC_THRESH_RW_MASK 0x0FFF0FFFU
#define FX1_ISP_AEC_THRESH_W1C_MASK 0x00000000U
#define FX1_ISP_AEC_THRESH_W1S_MASK 0x00000000U
#define FX1_ISP_AEC_THRESH_W1P_MASK 0x00000000U
#define FX1_ISP_AEC_THRESH_W1SC_MASK 0x00000000U
/**
 * th_ue [11:0], dòng bảng 328, kiểu truy cập RW
 * Phân loại thiếu sáng RAW12 nghiêm ngặt sample<threshold. Khởi đầu 272, lập trình/hiệu chuẩn theo
 * cảm biến; yêu cầu UE<OE.
 */
#define FX1_ISP_AEC_THRESH_TH_UE_SHIFT 0U
#define FX1_ISP_AEC_THRESH_TH_UE_WIDTH 12U
#define FX1_ISP_AEC_THRESH_TH_UE_MASK 0x00000FFFU
#define FX1_ISP_AEC_THRESH_TH_UE_RESET 0x00000110U
#define FX1_ISP_AEC_THRESH_TH_UE_ACCESS FX1_ISP_ACCESS_RW
/**
 * th_oe [27:16], dòng bảng 329, kiểu truy cập RW
 * Phân loại quá sáng RAW12 nghiêm ngặt sample>threshold. Khởi đầu 3744, lập trình/hiệu chuẩn theo
 * cảm biến; yêu cầu UE<OE.
 */
#define FX1_ISP_AEC_THRESH_TH_OE_SHIFT 16U
#define FX1_ISP_AEC_THRESH_TH_OE_WIDTH 12U
#define FX1_ISP_AEC_THRESH_TH_OE_MASK 0x0FFF0000U
#define FX1_ISP_AEC_THRESH_TH_OE_RESET 0x00000EA0U
#define FX1_ISP_AEC_THRESH_TH_OE_ACCESS FX1_ISP_ACCESS_RW

/**
 * AEC_STATUS: địa chỉ lệch 0x2218, dòng bảng 330, kiểu truy cập Mixed
 * Trạng thái khối AEC.
 */
#define FX1_ISP_AEC_STATUS_OFFSET 0x2218U
#define REG_AEC_STATUS FX1_ISP_AEC_STATUS_OFFSET
#define FX1_ISP_AEC_STATUS_RESET 0x00000000U
#define FX1_ISP_AEC_STATUS_ACCESS FX1_ISP_ACCESS_MIXED
#define FX1_ISP_AEC_STATUS_RO_MASK 0x00000001U
#define FX1_ISP_AEC_STATUS_RW_MASK 0x00000000U
#define FX1_ISP_AEC_STATUS_W1C_MASK 0x00000006U
#define FX1_ISP_AEC_STATUS_W1S_MASK 0x00000000U
#define FX1_ISP_AEC_STATUS_W1P_MASK 0x00000000U
#define FX1_ISP_AEC_STATUS_W1SC_MASK 0x00000000U
/**
 * busy [0:0], dòng bảng 331, kiểu truy cập RO
 * BUSY=1 khi đang tích lũy thống kê một frame.
 */
#define FX1_ISP_AEC_STATUS_BUSY_SHIFT 0U
#define FX1_ISP_AEC_STATUS_BUSY_WIDTH 1U
#define FX1_ISP_AEC_STATUS_BUSY_MASK 0x00000001U
#define FX1_ISP_AEC_STATUS_BUSY_RESET 0x00000000U
#define FX1_ISP_AEC_STATUS_BUSY_ACCESS FX1_ISP_ACCESS_RO
/**
 * stat_done [1:1], dòng bảng 332, kiểu truy cập W1C
 * Sự kiện hoàn tất W1C. Dữ liệu và nhận diện được công bố nguyên tử vào bank READY. Xin quyền trước
 * đọc, giải phóng sau sao chép. Xóa STAT_DONE chỉ xác nhận sự kiện, không giải phóng bank; hoàn tất
 * mới thắng xóa cùng lúc.
 */
#define FX1_ISP_AEC_STATUS_STAT_DONE_SHIFT 1U
#define FX1_ISP_AEC_STATUS_STAT_DONE_WIDTH 1U
#define FX1_ISP_AEC_STATUS_STAT_DONE_MASK 0x00000002U
#define FX1_ISP_AEC_STATUS_STAT_DONE_RESET 0x00000000U
#define FX1_ISP_AEC_STATUS_STAT_DONE_ACCESS FX1_ISP_ACCESS_W1C
/**
 * error [2:2], dòng bảng 333, kiểu truy cập W1C
 * Bit lỗi AE chưa triển khai, luôn đọc 0; phần mềm phát hiện frame bị mất qua FRAME_ID.
 */
#define FX1_ISP_AEC_STATUS_ERROR_SHIFT 2U
#define FX1_ISP_AEC_STATUS_ERROR_WIDTH 1U
#define FX1_ISP_AEC_STATUS_ERROR_MASK 0x00000004U
#define FX1_ISP_AEC_STATUS_ERROR_RESET 0x00000000U
#define FX1_ISP_AEC_STATUS_ERROR_ACCESS FX1_ISP_ACCESS_W1C

/**
 * AEC_FRAME_ID: địa chỉ lệch 0x221C, dòng bảng 334, kiểu truy cập RO
 * ID frame đầu vào của bank AEC đang giữ quyền đọc, chỉ đọc.
 */
#define FX1_ISP_AEC_FRAME_ID_OFFSET 0x221CU
#define REG_AEC_FRAME_ID FX1_ISP_AEC_FRAME_ID_OFFSET
#define FX1_ISP_AEC_FRAME_ID_RESET 0x00000000U
#define FX1_ISP_AEC_FRAME_ID_ACCESS FX1_ISP_ACCESS_RO
#define FX1_ISP_AEC_FRAME_ID_RO_MASK 0xFFFFFFFFU
#define FX1_ISP_AEC_FRAME_ID_RW_MASK 0x00000000U
#define FX1_ISP_AEC_FRAME_ID_W1C_MASK 0x00000000U
#define FX1_ISP_AEC_FRAME_ID_W1S_MASK 0x00000000U
#define FX1_ISP_AEC_FRAME_ID_W1P_MASK 0x00000000U
#define FX1_ISP_AEC_FRAME_ID_W1SC_MASK 0x00000000U
/**
 * frame_id [31:0], dòng bảng 335, kiểu truy cập RO
 * Số frame dùng chung tại SOF đầu vào. Frame đầu sau hard reset là 1; cuộn vòng modulo 2^32.
 * Common/DMA soft reset giữ bộ đếm. Thông tin ổn định đến RELEASE; chưa giữ quyền thì đọc 0 và báo
 * lỗi sở hữu.
 */
#define FX1_ISP_AEC_FRAME_ID_FRAME_ID_SHIFT 0U
#define FX1_ISP_AEC_FRAME_ID_FRAME_ID_WIDTH 32U
#define FX1_ISP_AEC_FRAME_ID_FRAME_ID_MASK 0xFFFFFFFFU
#define FX1_ISP_AEC_FRAME_ID_FRAME_ID_RESET 0x00000000U
#define FX1_ISP_AEC_FRAME_ID_FRAME_ID_ACCESS FX1_ISP_ACCESS_RO

/**
 * AEC_RESULT_CONTEXT_ID: địa chỉ lệch 0x2220, dòng bảng 336, kiểu truy cập RO
 * Nhãn context phần mềm của frame thống kê đang giữ quyền; ổn định đến RELEASE.
 */
#define FX1_ISP_AEC_RESULT_CONTEXT_ID_OFFSET 0x2220U
#define REG_AEC_RESULT_CONTEXT_ID FX1_ISP_AEC_RESULT_CONTEXT_ID_OFFSET
#define FX1_ISP_AEC_RESULT_CONTEXT_ID_RESET 0x00000000U
#define FX1_ISP_AEC_RESULT_CONTEXT_ID_ACCESS FX1_ISP_ACCESS_RO
#define FX1_ISP_AEC_RESULT_CONTEXT_ID_RO_MASK 0xFFFFFFFFU
#define FX1_ISP_AEC_RESULT_CONTEXT_ID_RW_MASK 0x00000000U
#define FX1_ISP_AEC_RESULT_CONTEXT_ID_W1C_MASK 0x00000000U
#define FX1_ISP_AEC_RESULT_CONTEXT_ID_W1S_MASK 0x00000000U
#define FX1_ISP_AEC_RESULT_CONTEXT_ID_W1P_MASK 0x00000000U
#define FX1_ISP_AEC_RESULT_CONTEXT_ID_W1SC_MASK 0x00000000U
/**
 * context_id [31:0], dòng bảng 337, kiểu truy cập RO
 * Mốc cấu hình đo sáng được chốt cho frame đã đo.
 */
#define FX1_ISP_AEC_RESULT_CONTEXT_ID_CONTEXT_ID_SHIFT 0U
#define FX1_ISP_AEC_RESULT_CONTEXT_ID_CONTEXT_ID_WIDTH 32U
#define FX1_ISP_AEC_RESULT_CONTEXT_ID_CONTEXT_ID_MASK 0xFFFFFFFFU
#define FX1_ISP_AEC_RESULT_CONTEXT_ID_CONTEXT_ID_RESET 0x00000000U
#define FX1_ISP_AEC_RESULT_CONTEXT_ID_CONTEXT_ID_ACCESS FX1_ISP_ACCESS_RO

/**
 * AEC_CHANNEL_SEL: địa chỉ lệch 0x2224, dòng bảng 338, kiểu truy cập RW
 * Selector đọc legacy: 0 là Bayer tổng hợp, 1..3 trả 0 thay kết quả từng màu CFA. Không phải
 * selector điểm đo; nguồn dùng TUNE_AEC_SOURCE.
 */
#define FX1_ISP_AEC_CHANNEL_SEL_OFFSET 0x2224U
#define REG_AEC_CHANNEL_SEL FX1_ISP_AEC_CHANNEL_SEL_OFFSET
#define FX1_ISP_AEC_CHANNEL_SEL_RESET 0x00000000U
#define FX1_ISP_AEC_CHANNEL_SEL_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_AEC_CHANNEL_SEL_RO_MASK 0x00000000U
#define FX1_ISP_AEC_CHANNEL_SEL_RW_MASK 0x00000003U
#define FX1_ISP_AEC_CHANNEL_SEL_W1C_MASK 0x00000000U
#define FX1_ISP_AEC_CHANNEL_SEL_W1S_MASK 0x00000000U
#define FX1_ISP_AEC_CHANNEL_SEL_W1P_MASK 0x00000000U
#define FX1_ISP_AEC_CHANNEL_SEL_W1SC_MASK 0x00000000U
/**
 * channel_sel [1:0], dòng bảng 339, kiểu truy cập RW
 * Selector đọc legacy: 0 là Bayer tổng hợp, 1..3 trả 0 thay kết quả từng màu CFA. Không phải
 * selector điểm đo; nguồn dùng TUNE_AEC_SOURCE.
 */
#define FX1_ISP_AEC_CHANNEL_SEL_CHANNEL_SEL_SHIFT 0U
#define FX1_ISP_AEC_CHANNEL_SEL_CHANNEL_SEL_WIDTH 2U
#define FX1_ISP_AEC_CHANNEL_SEL_CHANNEL_SEL_MASK 0x00000003U
#define FX1_ISP_AEC_CHANNEL_SEL_CHANNEL_SEL_RESET 0x00000000U
#define FX1_ISP_AEC_CHANNEL_SEL_CHANNEL_SEL_ACCESS FX1_ISP_ACCESS_RW

/**
 * AEC_GLOBAL_SUM_LO: địa chỉ lệch 0x2228, dòng bảng 340, kiểu truy cập RO
 * AEC: từ thấp của tổng Bayer RAW12 đủ điều kiện toàn frame. Cửa sổ hẹp chỉ chiếu các bit thấp
 * modulo từ cùng bank đang giữ quyền; dùng cổng gián tiếp 64 bit để có giá trị đầy đủ. Chưa giữ
 * quyền thì đọc 0 và đặt NO_LEASE.
 */
#define FX1_ISP_AEC_GLOBAL_SUM_LO_OFFSET 0x2228U
#define REG_AEC_GLOBAL_SUM_LO FX1_ISP_AEC_GLOBAL_SUM_LO_OFFSET
#define FX1_ISP_AEC_GLOBAL_SUM_LO_RESET 0x00000000U
#define FX1_ISP_AEC_GLOBAL_SUM_LO_ACCESS FX1_ISP_ACCESS_RO
#define FX1_ISP_AEC_GLOBAL_SUM_LO_RO_MASK 0xFFFFFFFFU
#define FX1_ISP_AEC_GLOBAL_SUM_LO_RW_MASK 0x00000000U
#define FX1_ISP_AEC_GLOBAL_SUM_LO_W1C_MASK 0x00000000U
#define FX1_ISP_AEC_GLOBAL_SUM_LO_W1S_MASK 0x00000000U
#define FX1_ISP_AEC_GLOBAL_SUM_LO_W1P_MASK 0x00000000U
#define FX1_ISP_AEC_GLOBAL_SUM_LO_W1SC_MASK 0x00000000U
/**
 * global_sum_lo [31:0], dòng bảng 341, kiểu truy cập RO
 * Các bit [31:0] của tổng mẫu đủ điều kiện toàn ảnh.
 */
#define FX1_ISP_AEC_GLOBAL_SUM_LO_GLOBAL_SUM_LO_SHIFT 0U
#define FX1_ISP_AEC_GLOBAL_SUM_LO_GLOBAL_SUM_LO_WIDTH 32U
#define FX1_ISP_AEC_GLOBAL_SUM_LO_GLOBAL_SUM_LO_MASK 0xFFFFFFFFU
#define FX1_ISP_AEC_GLOBAL_SUM_LO_GLOBAL_SUM_LO_RESET 0x00000000U
#define FX1_ISP_AEC_GLOBAL_SUM_LO_GLOBAL_SUM_LO_ACCESS FX1_ISP_ACCESS_RO

/**
 * AEC_GLOBAL_SUM_HI: địa chỉ lệch 0x222C, dòng bảng 342, kiểu truy cập RO
 * AEC: phần cao tổng Bayer RAW12 toàn frame, độ rộng giữ tương thích regmap. Cửa sổ hẹp chỉ chiếu
 * các bit thấp modulo từ cùng bank đang giữ quyền; dùng cổng gián tiếp 64 bit để có giá trị đầy đủ.
 * Chưa giữ quyền thì đọc 0 và đặt NO_LEASE.
 */
#define FX1_ISP_AEC_GLOBAL_SUM_HI_OFFSET 0x222CU
#define REG_AEC_GLOBAL_SUM_HI FX1_ISP_AEC_GLOBAL_SUM_HI_OFFSET
#define FX1_ISP_AEC_GLOBAL_SUM_HI_RESET 0x00000000U
#define FX1_ISP_AEC_GLOBAL_SUM_HI_ACCESS FX1_ISP_ACCESS_RO
#define FX1_ISP_AEC_GLOBAL_SUM_HI_RO_MASK 0x00000001U
#define FX1_ISP_AEC_GLOBAL_SUM_HI_RW_MASK 0x00000000U
#define FX1_ISP_AEC_GLOBAL_SUM_HI_W1C_MASK 0x00000000U
#define FX1_ISP_AEC_GLOBAL_SUM_HI_W1S_MASK 0x00000000U
#define FX1_ISP_AEC_GLOBAL_SUM_HI_W1P_MASK 0x00000000U
#define FX1_ISP_AEC_GLOBAL_SUM_HI_W1SC_MASK 0x00000000U
/**
 * global_sum_hi [0:0], dòng bảng 343, kiểu truy cập RO
 * Bit [32] của tổng mẫu đủ điều kiện, chỉ là cửa sổ tương thích hẹp.
 */
#define FX1_ISP_AEC_GLOBAL_SUM_HI_GLOBAL_SUM_HI_SHIFT 0U
#define FX1_ISP_AEC_GLOBAL_SUM_HI_GLOBAL_SUM_HI_WIDTH 1U
#define FX1_ISP_AEC_GLOBAL_SUM_HI_GLOBAL_SUM_HI_MASK 0x00000001U
#define FX1_ISP_AEC_GLOBAL_SUM_HI_GLOBAL_SUM_HI_RESET 0x00000000U
#define FX1_ISP_AEC_GLOBAL_SUM_HI_GLOBAL_SUM_HI_ACCESS FX1_ISP_ACCESS_RO

/**
 * AEC_GLOBAL_COUNT: địa chỉ lệch 0x2230, dòng bảng 344, kiểu truy cập RO
 * AEC: số mẫu Bayer RAW12 góp tổng toàn ảnh, loại mẫu ngoài clip gồm hai đầu mút. Cửa sổ hẹp chỉ
 * chiếu các bit thấp modulo từ cùng bank đang giữ quyền; dùng cổng gián tiếp 64 bit để có giá trị
 * đầy đủ. Chưa giữ quyền thì đọc 0 và đặt NO_LEASE.
 */
#define FX1_ISP_AEC_GLOBAL_COUNT_OFFSET 0x2230U
#define REG_AEC_GLOBAL_COUNT FX1_ISP_AEC_GLOBAL_COUNT_OFFSET
#define FX1_ISP_AEC_GLOBAL_COUNT_RESET 0x00000000U
#define FX1_ISP_AEC_GLOBAL_COUNT_ACCESS FX1_ISP_ACCESS_RO
#define FX1_ISP_AEC_GLOBAL_COUNT_RO_MASK 0x007FFFFFU
#define FX1_ISP_AEC_GLOBAL_COUNT_RW_MASK 0x00000000U
#define FX1_ISP_AEC_GLOBAL_COUNT_W1C_MASK 0x00000000U
#define FX1_ISP_AEC_GLOBAL_COUNT_W1S_MASK 0x00000000U
#define FX1_ISP_AEC_GLOBAL_COUNT_W1P_MASK 0x00000000U
#define FX1_ISP_AEC_GLOBAL_COUNT_W1SC_MASK 0x00000000U
/**
 * global_count [22:0], dòng bảng 345, kiểu truy cập RO
 * Số mẫu Bayer RAW12 đủ điều kiện toàn frame.
 */
#define FX1_ISP_AEC_GLOBAL_COUNT_GLOBAL_COUNT_SHIFT 0U
#define FX1_ISP_AEC_GLOBAL_COUNT_GLOBAL_COUNT_WIDTH 23U
#define FX1_ISP_AEC_GLOBAL_COUNT_GLOBAL_COUNT_MASK 0x007FFFFFU
#define FX1_ISP_AEC_GLOBAL_COUNT_GLOBAL_COUNT_RESET 0x00000000U
#define FX1_ISP_AEC_GLOBAL_COUNT_GLOBAL_COUNT_ACCESS FX1_ISP_ACCESS_RO

/**
 * AEC_ZONE_ADDR: địa chỉ lệch 0x2234, dòng bảng 346, kiểu truy cập RW
 * Selector zone AEC 0..767 theo thứ tự ảnh; kết quả hẹp cùng bank với cổng 64 bit. Hai bank giữ
 * READY/LEASED trong khi đo frame khác; không tái dùng trước RELEASE.
 */
#define FX1_ISP_AEC_ZONE_ADDR_OFFSET 0x2234U
#define REG_AEC_ZONE_ADDR FX1_ISP_AEC_ZONE_ADDR_OFFSET
#define FX1_ISP_AEC_ZONE_ADDR_RESET 0x00000000U
#define FX1_ISP_AEC_ZONE_ADDR_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_AEC_ZONE_ADDR_RO_MASK 0x00000000U
#define FX1_ISP_AEC_ZONE_ADDR_RW_MASK 0x000003FFU
#define FX1_ISP_AEC_ZONE_ADDR_W1C_MASK 0x00000000U
#define FX1_ISP_AEC_ZONE_ADDR_W1S_MASK 0x00000000U
#define FX1_ISP_AEC_ZONE_ADDR_W1P_MASK 0x00000000U
#define FX1_ISP_AEC_ZONE_ADDR_W1SC_MASK 0x00000000U
/**
 * zone_addr [9:0], dòng bảng 347, kiểu truy cập RW
 * Chỉ số zone 0..767 theo thứ tự y*num_zone_x+x.
 */
#define FX1_ISP_AEC_ZONE_ADDR_ZONE_ADDR_SHIFT 0U
#define FX1_ISP_AEC_ZONE_ADDR_ZONE_ADDR_WIDTH 10U
#define FX1_ISP_AEC_ZONE_ADDR_ZONE_ADDR_MASK 0x000003FFU
#define FX1_ISP_AEC_ZONE_ADDR_ZONE_ADDR_RESET 0x00000000U
#define FX1_ISP_AEC_ZONE_ADDR_ZONE_ADDR_ACCESS FX1_ISP_ACCESS_RW

/**
 * AEC_ZONE_SUM: địa chỉ lệch 0x2238, dòng bảng 348, kiểu truy cập RO
 * AEC: tổng Bayer RAW12 đủ điều kiện của zone được chọn. Cửa sổ hẹp chỉ chiếu các bit thấp modulo từ
 * cùng bank đang giữ quyền; dùng cổng gián tiếp 64 bit để có giá trị đầy đủ. Chưa giữ quyền thì đọc
 * 0 và đặt NO_LEASE.
 */
#define FX1_ISP_AEC_ZONE_SUM_OFFSET 0x2238U
#define REG_AEC_ZONE_SUM FX1_ISP_AEC_ZONE_SUM_OFFSET
#define FX1_ISP_AEC_ZONE_SUM_RESET 0x00000000U
#define FX1_ISP_AEC_ZONE_SUM_ACCESS FX1_ISP_ACCESS_RO
#define FX1_ISP_AEC_ZONE_SUM_RO_MASK 0x0FFFFFFFU
#define FX1_ISP_AEC_ZONE_SUM_RW_MASK 0x00000000U
#define FX1_ISP_AEC_ZONE_SUM_W1C_MASK 0x00000000U
#define FX1_ISP_AEC_ZONE_SUM_W1S_MASK 0x00000000U
#define FX1_ISP_AEC_ZONE_SUM_W1P_MASK 0x00000000U
#define FX1_ISP_AEC_ZONE_SUM_W1SC_MASK 0x00000000U
/**
 * zone_sum [27:0], dòng bảng 349, kiểu truy cập RO
 * Tổng mẫu đủ điều kiện của zone tại AEC_ZONE_ADDR.
 */
#define FX1_ISP_AEC_ZONE_SUM_ZONE_SUM_SHIFT 0U
#define FX1_ISP_AEC_ZONE_SUM_ZONE_SUM_WIDTH 28U
#define FX1_ISP_AEC_ZONE_SUM_ZONE_SUM_MASK 0x0FFFFFFFU
#define FX1_ISP_AEC_ZONE_SUM_ZONE_SUM_RESET 0x00000000U
#define FX1_ISP_AEC_ZONE_SUM_ZONE_SUM_ACCESS FX1_ISP_ACCESS_RO

/**
 * AEC_ZONE_COUNT: địa chỉ lệch 0x223C, dòng bảng 350, kiểu truy cập RO
 * AEC: số mẫu RAW12 đủ điều kiện của zone được chọn, cửa sổ cuộn modulo 65536. Cửa sổ hẹp chỉ chiếu
 * các bit thấp modulo từ cùng bank đang giữ quyền; dùng cổng gián tiếp 64 bit để có giá trị đầy đủ.
 * Chưa giữ quyền thì đọc 0 và đặt NO_LEASE.
 */
#define FX1_ISP_AEC_ZONE_COUNT_OFFSET 0x223CU
#define REG_AEC_ZONE_COUNT FX1_ISP_AEC_ZONE_COUNT_OFFSET
#define FX1_ISP_AEC_ZONE_COUNT_RESET 0x00000000U
#define FX1_ISP_AEC_ZONE_COUNT_ACCESS FX1_ISP_ACCESS_RO
#define FX1_ISP_AEC_ZONE_COUNT_RO_MASK 0x0000FFFFU
#define FX1_ISP_AEC_ZONE_COUNT_RW_MASK 0x00000000U
#define FX1_ISP_AEC_ZONE_COUNT_W1C_MASK 0x00000000U
#define FX1_ISP_AEC_ZONE_COUNT_W1S_MASK 0x00000000U
#define FX1_ISP_AEC_ZONE_COUNT_W1P_MASK 0x00000000U
#define FX1_ISP_AEC_ZONE_COUNT_W1SC_MASK 0x00000000U
/**
 * zone_count [15:0], dòng bảng 351, kiểu truy cập RO
 * Số mẫu RAW12 đủ điều kiện của zone; cửa sổ hẹp cuộn vòng modulo 65536.
 */
#define FX1_ISP_AEC_ZONE_COUNT_ZONE_COUNT_SHIFT 0U
#define FX1_ISP_AEC_ZONE_COUNT_ZONE_COUNT_WIDTH 16U
#define FX1_ISP_AEC_ZONE_COUNT_ZONE_COUNT_MASK 0x0000FFFFU
#define FX1_ISP_AEC_ZONE_COUNT_ZONE_COUNT_RESET 0x00000000U
#define FX1_ISP_AEC_ZONE_COUNT_ZONE_COUNT_ACCESS FX1_ISP_ACCESS_RO

/**
 * AEC_ZONE_GREEN_OE_UE: địa chỉ lệch 0x2240, dòng bảng 352, kiểu truy cập RO
 * AEC: tên thanh ghi legacy giữ lại nhưng đếm UE/OE Bayer RAW12 của zone, độc lập clip. Cửa sổ hẹp
 * chỉ chiếu các bit thấp modulo từ cùng bank đang giữ quyền; dùng cổng gián tiếp 64 bit để có giá
 * trị đầy đủ. Chưa giữ quyền thì đọc 0 và đặt NO_LEASE.
 */
#define FX1_ISP_AEC_ZONE_GREEN_OE_UE_OFFSET 0x2240U
#define REG_AEC_ZONE_GREEN_OE_UE FX1_ISP_AEC_ZONE_GREEN_OE_UE_OFFSET
#define FX1_ISP_AEC_ZONE_GREEN_OE_UE_RESET 0x00000000U
#define FX1_ISP_AEC_ZONE_GREEN_OE_UE_ACCESS FX1_ISP_ACCESS_RO
#define FX1_ISP_AEC_ZONE_GREEN_OE_UE_RO_MASK 0xFFFFFFFFU
#define FX1_ISP_AEC_ZONE_GREEN_OE_UE_RW_MASK 0x00000000U
#define FX1_ISP_AEC_ZONE_GREEN_OE_UE_W1C_MASK 0x00000000U
#define FX1_ISP_AEC_ZONE_GREEN_OE_UE_W1S_MASK 0x00000000U
#define FX1_ISP_AEC_ZONE_GREEN_OE_UE_W1P_MASK 0x00000000U
#define FX1_ISP_AEC_ZONE_GREEN_OE_UE_W1SC_MASK 0x00000000U
/**
 * green_ue_count [15:0], dòng bảng 353, kiểu truy cập RO
 * Số mẫu Bayer RAW12 dưới TH_UE trong zone, cửa sổ cuộn modulo 65536.
 */
#define FX1_ISP_AEC_ZONE_GREEN_OE_UE_GREEN_UE_COUNT_SHIFT 0U
#define FX1_ISP_AEC_ZONE_GREEN_OE_UE_GREEN_UE_COUNT_WIDTH 16U
#define FX1_ISP_AEC_ZONE_GREEN_OE_UE_GREEN_UE_COUNT_MASK 0x0000FFFFU
#define FX1_ISP_AEC_ZONE_GREEN_OE_UE_GREEN_UE_COUNT_RESET 0x00000000U
#define FX1_ISP_AEC_ZONE_GREEN_OE_UE_GREEN_UE_COUNT_ACCESS FX1_ISP_ACCESS_RO
/**
 * green_oe_count [31:16], dòng bảng 354, kiểu truy cập RO
 * Số mẫu Bayer RAW12 trên TH_OE trong zone, cửa sổ cuộn modulo 65536.
 */
#define FX1_ISP_AEC_ZONE_GREEN_OE_UE_GREEN_OE_COUNT_SHIFT 16U
#define FX1_ISP_AEC_ZONE_GREEN_OE_UE_GREEN_OE_COUNT_WIDTH 16U
#define FX1_ISP_AEC_ZONE_GREEN_OE_UE_GREEN_OE_COUNT_MASK 0xFFFF0000U
#define FX1_ISP_AEC_ZONE_GREEN_OE_UE_GREEN_OE_COUNT_RESET 0x00000000U
#define FX1_ISP_AEC_ZONE_GREEN_OE_UE_GREEN_OE_COUNT_ACCESS FX1_ISP_ACCESS_RO

/**
 * AEC_ZONE_GREEN_MIN_MAX: địa chỉ lệch 0x2244, dòng bảng 355, kiểu truy cập RO
 * Tên thanh ghi legacy được giữ; trả min/max Bayer RAW12 của zone đang giữ quyền. Zone rỗng trả
 * min=4095,max=0; chưa giữ quyền thì cả từ đọc 0 và đặt NO_LEASE.
 */
#define FX1_ISP_AEC_ZONE_GREEN_MIN_MAX_OFFSET 0x2244U
#define REG_AEC_ZONE_GREEN_MIN_MAX FX1_ISP_AEC_ZONE_GREEN_MIN_MAX_OFFSET
#define FX1_ISP_AEC_ZONE_GREEN_MIN_MAX_RESET 0x00000000U
#define FX1_ISP_AEC_ZONE_GREEN_MIN_MAX_ACCESS FX1_ISP_ACCESS_RO
#define FX1_ISP_AEC_ZONE_GREEN_MIN_MAX_RO_MASK 0x00FFFFFFU
#define FX1_ISP_AEC_ZONE_GREEN_MIN_MAX_RW_MASK 0x00000000U
#define FX1_ISP_AEC_ZONE_GREEN_MIN_MAX_W1C_MASK 0x00000000U
#define FX1_ISP_AEC_ZONE_GREEN_MIN_MAX_W1S_MASK 0x00000000U
#define FX1_ISP_AEC_ZONE_GREEN_MIN_MAX_W1P_MASK 0x00000000U
#define FX1_ISP_AEC_ZONE_GREEN_MIN_MAX_W1SC_MASK 0x00000000U
/**
 * green_min [11:0], dòng bảng 356, kiểu truy cập RO
 * Min mẫu Bayer RAW12 quan sát trong zone; zone rỗng trả 4095, chưa giữ quyền trả 0.
 */
#define FX1_ISP_AEC_ZONE_GREEN_MIN_MAX_GREEN_MIN_SHIFT 0U
#define FX1_ISP_AEC_ZONE_GREEN_MIN_MAX_GREEN_MIN_WIDTH 12U
#define FX1_ISP_AEC_ZONE_GREEN_MIN_MAX_GREEN_MIN_MASK 0x00000FFFU
#define FX1_ISP_AEC_ZONE_GREEN_MIN_MAX_GREEN_MIN_RESET 0x00000000U
#define FX1_ISP_AEC_ZONE_GREEN_MIN_MAX_GREEN_MIN_ACCESS FX1_ISP_ACCESS_RO
/**
 * green_max [23:12], dòng bảng 357, kiểu truy cập RO
 * Max mẫu Bayer RAW12 quan sát trong zone; zone rỗng trả 0.
 */
#define FX1_ISP_AEC_ZONE_GREEN_MIN_MAX_GREEN_MAX_SHIFT 12U
#define FX1_ISP_AEC_ZONE_GREEN_MIN_MAX_GREEN_MAX_WIDTH 12U
#define FX1_ISP_AEC_ZONE_GREEN_MIN_MAX_GREEN_MAX_MASK 0x00FFF000U
#define FX1_ISP_AEC_ZONE_GREEN_MIN_MAX_GREEN_MAX_RESET 0x00000000U
#define FX1_ISP_AEC_ZONE_GREEN_MIN_MAX_GREEN_MAX_ACCESS FX1_ISP_ACCESS_RO

/**
 * AEC_HIST_ADDR: địa chỉ lệch 0x2248, dòng bảng 358, kiểu truy cập RW
 * Chọn một trong 64 bin histogram RAW12: bin n chứa mức 64n..64n+63.
 */
#define FX1_ISP_AEC_HIST_ADDR_OFFSET 0x2248U
#define REG_AEC_HIST_ADDR FX1_ISP_AEC_HIST_ADDR_OFFSET
#define FX1_ISP_AEC_HIST_ADDR_RESET 0x00000000U
#define FX1_ISP_AEC_HIST_ADDR_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_AEC_HIST_ADDR_RO_MASK 0x00000000U
#define FX1_ISP_AEC_HIST_ADDR_RW_MASK 0x0000003FU
#define FX1_ISP_AEC_HIST_ADDR_W1C_MASK 0x00000000U
#define FX1_ISP_AEC_HIST_ADDR_W1S_MASK 0x00000000U
#define FX1_ISP_AEC_HIST_ADDR_W1P_MASK 0x00000000U
#define FX1_ISP_AEC_HIST_ADDR_W1SC_MASK 0x00000000U
/**
 * hist_addr [5:0], dòng bảng 359, kiểu truy cập RW
 * Chỉ số bin 0..63; bin n phủ mã Bayer RAW12 64n..64n+63.
 */
#define FX1_ISP_AEC_HIST_ADDR_HIST_ADDR_SHIFT 0U
#define FX1_ISP_AEC_HIST_ADDR_HIST_ADDR_WIDTH 6U
#define FX1_ISP_AEC_HIST_ADDR_HIST_ADDR_MASK 0x0000003FU
#define FX1_ISP_AEC_HIST_ADDR_HIST_ADDR_RESET 0x00000000U
#define FX1_ISP_AEC_HIST_ADDR_HIST_ADDR_ACCESS FX1_ISP_ACCESS_RW

/**
 * AEC_HIST_DATA: địa chỉ lệch 0x224C, dòng bảng 360, kiểu truy cập RO
 * AEC: số mẫu bin histogram được chọn trong 64 bin của mọi mẫu RAW12 quan sát. Cửa sổ hẹp chỉ chiếu
 * các bit thấp modulo từ cùng bank đang giữ quyền; dùng cổng gián tiếp 64 bit để có giá trị đầy đủ.
 * Chưa giữ quyền thì đọc 0 và đặt NO_LEASE.
 */
#define FX1_ISP_AEC_HIST_DATA_OFFSET 0x224CU
#define REG_AEC_HIST_DATA FX1_ISP_AEC_HIST_DATA_OFFSET
#define FX1_ISP_AEC_HIST_DATA_RESET 0x00000000U
#define FX1_ISP_AEC_HIST_DATA_ACCESS FX1_ISP_ACCESS_RO
#define FX1_ISP_AEC_HIST_DATA_RO_MASK 0x007FFFFFU
#define FX1_ISP_AEC_HIST_DATA_RW_MASK 0x00000000U
#define FX1_ISP_AEC_HIST_DATA_W1C_MASK 0x00000000U
#define FX1_ISP_AEC_HIST_DATA_W1S_MASK 0x00000000U
#define FX1_ISP_AEC_HIST_DATA_W1P_MASK 0x00000000U
#define FX1_ISP_AEC_HIST_DATA_W1SC_MASK 0x00000000U
/**
 * hist_count [22:0], dòng bảng 361, kiểu truy cập RO
 * Số mẫu histogram: chỉ số là sample[11:6] RAW12 chuẩn hóa (sample>>6); hỗ trợ toàn frame tới
 * 3840x2160.
 */
#define FX1_ISP_AEC_HIST_DATA_HIST_COUNT_SHIFT 0U
#define FX1_ISP_AEC_HIST_DATA_HIST_COUNT_WIDTH 23U
#define FX1_ISP_AEC_HIST_DATA_HIST_COUNT_MASK 0x007FFFFFU
#define FX1_ISP_AEC_HIST_DATA_HIST_COUNT_RESET 0x00000000U
#define FX1_ISP_AEC_HIST_DATA_HIST_COUNT_ACCESS FX1_ISP_ACCESS_RO

/**
 * AWB_CTRL: địa chỉ lệch 0x2400, dòng bảng 362, kiểu truy cập RW
 * AWB đo ô Bayer sau BPC, không chặn video. Mỗi ô RGGB 2x2 đủ điều kiện góp R, G trung bình có làm
 * tròn và B một lần lên lưới ô đều.
 */
#define FX1_ISP_AWB_CTRL_OFFSET 0x2400U
#define REG_AWB_CTRL FX1_ISP_AWB_CTRL_OFFSET
#define FX1_ISP_AWB_CTRL_RESET 0x00000000U
#define FX1_ISP_AWB_CTRL_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_AWB_CTRL_RO_MASK 0x00000000U
#define FX1_ISP_AWB_CTRL_RW_MASK 0x00003FFFU
#define FX1_ISP_AWB_CTRL_W1C_MASK 0x00000000U
#define FX1_ISP_AWB_CTRL_W1S_MASK 0x00000000U
#define FX1_ISP_AWB_CTRL_W1P_MASK 0x00000000U
#define FX1_ISP_AWB_CTRL_W1SC_MASK 0x00000000U
/**
 * en [0:0], dòng bảng 363, kiểu truy cập RW
 * Bật đo AWB ô Bayer; hình học bật không hợp lệ bị bỏ và báo cáo.
 */
#define FX1_ISP_AWB_CTRL_EN_SHIFT 0U
#define FX1_ISP_AWB_CTRL_EN_WIDTH 1U
#define FX1_ISP_AWB_CTRL_EN_MASK 0x00000001U
#define FX1_ISP_AWB_CTRL_EN_RESET 0x00000000U
#define FX1_ISP_AWB_CTRL_EN_ACCESS FX1_ISP_ACCESS_RW
/**
 * num_zone_x [7:1], dòng bảng 364, kiểu truy cập RW
 * Số zone ngang trên các cột ô hữu dụng, mỗi ô rộng hai pixel.
 */
#define FX1_ISP_AWB_CTRL_NUM_ZONE_X_SHIFT 1U
#define FX1_ISP_AWB_CTRL_NUM_ZONE_X_WIDTH 7U
#define FX1_ISP_AWB_CTRL_NUM_ZONE_X_MASK 0x000000FEU
#define FX1_ISP_AWB_CTRL_NUM_ZONE_X_RESET 0x00000000U
#define FX1_ISP_AWB_CTRL_NUM_ZONE_X_ACCESS FX1_ISP_ACCESS_RW
/**
 * num_zone_y [13:8], dòng bảng 365, kiểu truy cập RW
 * Số zone dọc trên các hàng ô hữu dụng, mỗi ô cao hai pixel.
 */
#define FX1_ISP_AWB_CTRL_NUM_ZONE_Y_SHIFT 8U
#define FX1_ISP_AWB_CTRL_NUM_ZONE_Y_WIDTH 6U
#define FX1_ISP_AWB_CTRL_NUM_ZONE_Y_MASK 0x00003F00U
#define FX1_ISP_AWB_CTRL_NUM_ZONE_Y_RESET 0x00000000U
#define FX1_ISP_AWB_CTRL_NUM_ZONE_Y_ACCESS FX1_ISP_ACCESS_RW

/**
 * AWB_X_BOUND_ADDR: địa chỉ lệch 0x2404, dòng bảng 366, kiểu truy cập RW
 * Điều khiển bảng biên AWB dự phòng. Biên đều được suy ra từ số zone AWB_CTRL và hình học hữu dụng;
 * ghi không ảnh hưởng xử lý.
 */
#define FX1_ISP_AWB_X_BOUND_ADDR_OFFSET 0x2404U
#define REG_AWB_X_BOUND_ADDR FX1_ISP_AWB_X_BOUND_ADDR_OFFSET
#define FX1_ISP_AWB_X_BOUND_ADDR_RESET 0x00000000U
#define FX1_ISP_AWB_X_BOUND_ADDR_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_AWB_X_BOUND_ADDR_RO_MASK 0x00000000U
#define FX1_ISP_AWB_X_BOUND_ADDR_RW_MASK 0x0000003FU
#define FX1_ISP_AWB_X_BOUND_ADDR_W1C_MASK 0x00000000U
#define FX1_ISP_AWB_X_BOUND_ADDR_W1S_MASK 0x00000000U
#define FX1_ISP_AWB_X_BOUND_ADDR_W1P_MASK 0x00000000U
#define FX1_ISP_AWB_X_BOUND_ADDR_W1SC_MASK 0x00000000U
/**
 * x_bound_addr [5:0], dòng bảng 367, kiểu truy cập RW
 * Trường chỉ số/giá trị/cho phép ghi biên AWB chưa triển khai; dự phòng, không tác động xử lý.
 */
#define FX1_ISP_AWB_X_BOUND_ADDR_X_BOUND_ADDR_SHIFT 0U
#define FX1_ISP_AWB_X_BOUND_ADDR_X_BOUND_ADDR_WIDTH 6U
#define FX1_ISP_AWB_X_BOUND_ADDR_X_BOUND_ADDR_MASK 0x0000003FU
#define FX1_ISP_AWB_X_BOUND_ADDR_X_BOUND_ADDR_RESET 0x00000000U
#define FX1_ISP_AWB_X_BOUND_ADDR_X_BOUND_ADDR_ACCESS FX1_ISP_ACCESS_RW

/**
 * AWB_Y_BOUND_ADDR: địa chỉ lệch 0x2408, dòng bảng 368, kiểu truy cập RW
 * Điều khiển bảng biên AWB dự phòng. Biên đều được suy ra từ số zone AWB_CTRL và hình học hữu dụng;
 * ghi không ảnh hưởng xử lý.
 */
#define FX1_ISP_AWB_Y_BOUND_ADDR_OFFSET 0x2408U
#define REG_AWB_Y_BOUND_ADDR FX1_ISP_AWB_Y_BOUND_ADDR_OFFSET
#define FX1_ISP_AWB_Y_BOUND_ADDR_RESET 0x00000000U
#define FX1_ISP_AWB_Y_BOUND_ADDR_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_AWB_Y_BOUND_ADDR_RO_MASK 0x00000000U
#define FX1_ISP_AWB_Y_BOUND_ADDR_RW_MASK 0x0000001FU
#define FX1_ISP_AWB_Y_BOUND_ADDR_W1C_MASK 0x00000000U
#define FX1_ISP_AWB_Y_BOUND_ADDR_W1S_MASK 0x00000000U
#define FX1_ISP_AWB_Y_BOUND_ADDR_W1P_MASK 0x00000000U
#define FX1_ISP_AWB_Y_BOUND_ADDR_W1SC_MASK 0x00000000U
/**
 * y_bound_addr [4:0], dòng bảng 369, kiểu truy cập RW
 * Trường chỉ số/giá trị/cho phép ghi biên AWB chưa triển khai; dự phòng, không tác động xử lý.
 */
#define FX1_ISP_AWB_Y_BOUND_ADDR_Y_BOUND_ADDR_SHIFT 0U
#define FX1_ISP_AWB_Y_BOUND_ADDR_Y_BOUND_ADDR_WIDTH 5U
#define FX1_ISP_AWB_Y_BOUND_ADDR_Y_BOUND_ADDR_MASK 0x0000001FU
#define FX1_ISP_AWB_Y_BOUND_ADDR_Y_BOUND_ADDR_RESET 0x00000000U
#define FX1_ISP_AWB_Y_BOUND_ADDR_Y_BOUND_ADDR_ACCESS FX1_ISP_ACCESS_RW

/**
 * AWB_X_BOUND_DATA: địa chỉ lệch 0x240C, dòng bảng 370, kiểu truy cập RW
 * Điều khiển bảng biên AWB dự phòng. Biên đều được suy ra từ số zone AWB_CTRL và hình học hữu dụng;
 * ghi không ảnh hưởng xử lý.
 */
#define FX1_ISP_AWB_X_BOUND_DATA_OFFSET 0x240CU
#define REG_AWB_X_BOUND_DATA FX1_ISP_AWB_X_BOUND_DATA_OFFSET
#define FX1_ISP_AWB_X_BOUND_DATA_RESET 0x00000000U
#define FX1_ISP_AWB_X_BOUND_DATA_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_AWB_X_BOUND_DATA_RO_MASK 0x00000000U
#define FX1_ISP_AWB_X_BOUND_DATA_RW_MASK 0x00001FFFU
#define FX1_ISP_AWB_X_BOUND_DATA_W1C_MASK 0x00000000U
#define FX1_ISP_AWB_X_BOUND_DATA_W1S_MASK 0x00000000U
#define FX1_ISP_AWB_X_BOUND_DATA_W1P_MASK 0x00000000U
#define FX1_ISP_AWB_X_BOUND_DATA_W1SC_MASK 0x00000000U
/**
 * x_bound_data [11:0], dòng bảng 371, kiểu truy cập RW
 * Trường chỉ số/giá trị/cho phép ghi biên AWB chưa triển khai; dự phòng, không tác động xử lý.
 */
#define FX1_ISP_AWB_X_BOUND_DATA_X_BOUND_DATA_SHIFT 0U
#define FX1_ISP_AWB_X_BOUND_DATA_X_BOUND_DATA_WIDTH 12U
#define FX1_ISP_AWB_X_BOUND_DATA_X_BOUND_DATA_MASK 0x00000FFFU
#define FX1_ISP_AWB_X_BOUND_DATA_X_BOUND_DATA_RESET 0x00000000U
#define FX1_ISP_AWB_X_BOUND_DATA_X_BOUND_DATA_ACCESS FX1_ISP_ACCESS_RW
/**
 * x_bound_we [12:12], dòng bảng 372, kiểu truy cập RW
 * Trường chỉ số/giá trị/cho phép ghi biên AWB chưa triển khai; dự phòng, không tác động xử lý.
 */
#define FX1_ISP_AWB_X_BOUND_DATA_X_BOUND_WE_SHIFT 12U
#define FX1_ISP_AWB_X_BOUND_DATA_X_BOUND_WE_WIDTH 1U
#define FX1_ISP_AWB_X_BOUND_DATA_X_BOUND_WE_MASK 0x00001000U
#define FX1_ISP_AWB_X_BOUND_DATA_X_BOUND_WE_RESET 0x00000000U
#define FX1_ISP_AWB_X_BOUND_DATA_X_BOUND_WE_ACCESS FX1_ISP_ACCESS_RW

/**
 * AWB_Y_BOUND_DATA: địa chỉ lệch 0x2410, dòng bảng 373, kiểu truy cập RW
 * Điều khiển bảng biên AWB dự phòng. Biên đều được suy ra từ số zone AWB_CTRL và hình học hữu dụng;
 * ghi không ảnh hưởng xử lý.
 */
#define FX1_ISP_AWB_Y_BOUND_DATA_OFFSET 0x2410U
#define REG_AWB_Y_BOUND_DATA FX1_ISP_AWB_Y_BOUND_DATA_OFFSET
#define FX1_ISP_AWB_Y_BOUND_DATA_RESET 0x00000000U
#define FX1_ISP_AWB_Y_BOUND_DATA_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_AWB_Y_BOUND_DATA_RO_MASK 0x00000000U
#define FX1_ISP_AWB_Y_BOUND_DATA_RW_MASK 0x00001FFFU
#define FX1_ISP_AWB_Y_BOUND_DATA_W1C_MASK 0x00000000U
#define FX1_ISP_AWB_Y_BOUND_DATA_W1S_MASK 0x00000000U
#define FX1_ISP_AWB_Y_BOUND_DATA_W1P_MASK 0x00000000U
#define FX1_ISP_AWB_Y_BOUND_DATA_W1SC_MASK 0x00000000U
/**
 * y_bound_data [11:0], dòng bảng 374, kiểu truy cập RW
 * Trường chỉ số/giá trị/cho phép ghi biên AWB chưa triển khai; dự phòng, không tác động xử lý.
 */
#define FX1_ISP_AWB_Y_BOUND_DATA_Y_BOUND_DATA_SHIFT 0U
#define FX1_ISP_AWB_Y_BOUND_DATA_Y_BOUND_DATA_WIDTH 12U
#define FX1_ISP_AWB_Y_BOUND_DATA_Y_BOUND_DATA_MASK 0x00000FFFU
#define FX1_ISP_AWB_Y_BOUND_DATA_Y_BOUND_DATA_RESET 0x00000000U
#define FX1_ISP_AWB_Y_BOUND_DATA_Y_BOUND_DATA_ACCESS FX1_ISP_ACCESS_RW
/**
 * y_bound_we [12:12], dòng bảng 375, kiểu truy cập RW
 * Trường chỉ số/giá trị/cho phép ghi biên AWB chưa triển khai; dự phòng, không tác động xử lý.
 */
#define FX1_ISP_AWB_Y_BOUND_DATA_Y_BOUND_WE_SHIFT 12U
#define FX1_ISP_AWB_Y_BOUND_DATA_Y_BOUND_WE_WIDTH 1U
#define FX1_ISP_AWB_Y_BOUND_DATA_Y_BOUND_WE_MASK 0x00001000U
#define FX1_ISP_AWB_Y_BOUND_DATA_Y_BOUND_WE_RESET 0x00000000U
#define FX1_ISP_AWB_Y_BOUND_DATA_Y_BOUND_WE_ACCESS FX1_ISP_ACCESS_RW

/**
 * AWB_GLOBAL_SUM_R_H: địa chỉ lệch 0x2414, dòng bảng 376, kiểu truy cập RO
 * Tổng Rcell toàn frame trên các ô Bayer đủ điều kiện, chỉ đọc.
 */
#define FX1_ISP_AWB_GLOBAL_SUM_R_H_OFFSET 0x2414U
#define REG_AWB_GLOBAL_SUM_R_H FX1_ISP_AWB_GLOBAL_SUM_R_H_OFFSET
#define FX1_ISP_AWB_GLOBAL_SUM_R_H_RESET 0x00000000U
#define FX1_ISP_AWB_GLOBAL_SUM_R_H_ACCESS FX1_ISP_ACCESS_RO
#define FX1_ISP_AWB_GLOBAL_SUM_R_H_RO_MASK 0x00000007U
#define FX1_ISP_AWB_GLOBAL_SUM_R_H_RW_MASK 0x00000000U
#define FX1_ISP_AWB_GLOBAL_SUM_R_H_W1C_MASK 0x00000000U
#define FX1_ISP_AWB_GLOBAL_SUM_R_H_W1S_MASK 0x00000000U
#define FX1_ISP_AWB_GLOBAL_SUM_R_H_W1P_MASK 0x00000000U
#define FX1_ISP_AWB_GLOBAL_SUM_R_H_W1SC_MASK 0x00000000U
/**
 * global_sum_r_h [2:0], dòng bảng 377, kiểu truy cập RO
 * Các bit cao [34:32] của tổng màu tương ứng trên frame/zone đang giữ quyền đọc.
 */
#define FX1_ISP_AWB_GLOBAL_SUM_R_H_GLOBAL_SUM_R_H_SHIFT 0U
#define FX1_ISP_AWB_GLOBAL_SUM_R_H_GLOBAL_SUM_R_H_WIDTH 3U
#define FX1_ISP_AWB_GLOBAL_SUM_R_H_GLOBAL_SUM_R_H_MASK 0x00000007U
#define FX1_ISP_AWB_GLOBAL_SUM_R_H_GLOBAL_SUM_R_H_RESET 0x00000000U
#define FX1_ISP_AWB_GLOBAL_SUM_R_H_GLOBAL_SUM_R_H_ACCESS FX1_ISP_ACCESS_RO

/**
 * AWB_GLOBAL_SUM_R_L: địa chỉ lệch 0x2418, dòng bảng 378, kiểu truy cập RO
 * Tổng Rcell toàn frame trên các ô Bayer đủ điều kiện, chỉ đọc.
 */
#define FX1_ISP_AWB_GLOBAL_SUM_R_L_OFFSET 0x2418U
#define REG_AWB_GLOBAL_SUM_R_L FX1_ISP_AWB_GLOBAL_SUM_R_L_OFFSET
#define FX1_ISP_AWB_GLOBAL_SUM_R_L_RESET 0x00000000U
#define FX1_ISP_AWB_GLOBAL_SUM_R_L_ACCESS FX1_ISP_ACCESS_RO
#define FX1_ISP_AWB_GLOBAL_SUM_R_L_RO_MASK 0xFFFFFFFFU
#define FX1_ISP_AWB_GLOBAL_SUM_R_L_RW_MASK 0x00000000U
#define FX1_ISP_AWB_GLOBAL_SUM_R_L_W1C_MASK 0x00000000U
#define FX1_ISP_AWB_GLOBAL_SUM_R_L_W1S_MASK 0x00000000U
#define FX1_ISP_AWB_GLOBAL_SUM_R_L_W1P_MASK 0x00000000U
#define FX1_ISP_AWB_GLOBAL_SUM_R_L_W1SC_MASK 0x00000000U
/**
 * global_sum_r_l [31:0], dòng bảng 379, kiểu truy cập RO
 * Các bit thấp [31:0] của tổng màu tương ứng trên frame/zone đang giữ quyền đọc.
 */
#define FX1_ISP_AWB_GLOBAL_SUM_R_L_GLOBAL_SUM_R_L_SHIFT 0U
#define FX1_ISP_AWB_GLOBAL_SUM_R_L_GLOBAL_SUM_R_L_WIDTH 32U
#define FX1_ISP_AWB_GLOBAL_SUM_R_L_GLOBAL_SUM_R_L_MASK 0xFFFFFFFFU
#define FX1_ISP_AWB_GLOBAL_SUM_R_L_GLOBAL_SUM_R_L_RESET 0x00000000U
#define FX1_ISP_AWB_GLOBAL_SUM_R_L_GLOBAL_SUM_R_L_ACCESS FX1_ISP_ACCESS_RO

/**
 * AWB_GLOBAL_SUM_G_H: địa chỉ lệch 0x241C, dòng bảng 380, kiểu truy cập RO
 * Tổng Gcell toàn frame trên các ô Bayer đủ điều kiện, chỉ đọc.
 */
#define FX1_ISP_AWB_GLOBAL_SUM_G_H_OFFSET 0x241CU
#define REG_AWB_GLOBAL_SUM_G_H FX1_ISP_AWB_GLOBAL_SUM_G_H_OFFSET
#define FX1_ISP_AWB_GLOBAL_SUM_G_H_RESET 0x00000000U
#define FX1_ISP_AWB_GLOBAL_SUM_G_H_ACCESS FX1_ISP_ACCESS_RO
#define FX1_ISP_AWB_GLOBAL_SUM_G_H_RO_MASK 0x00000007U
#define FX1_ISP_AWB_GLOBAL_SUM_G_H_RW_MASK 0x00000000U
#define FX1_ISP_AWB_GLOBAL_SUM_G_H_W1C_MASK 0x00000000U
#define FX1_ISP_AWB_GLOBAL_SUM_G_H_W1S_MASK 0x00000000U
#define FX1_ISP_AWB_GLOBAL_SUM_G_H_W1P_MASK 0x00000000U
#define FX1_ISP_AWB_GLOBAL_SUM_G_H_W1SC_MASK 0x00000000U
/**
 * global_sum_g_h [2:0], dòng bảng 381, kiểu truy cập RO
 * Các bit cao [34:32] của tổng màu tương ứng trên frame/zone đang giữ quyền đọc.
 */
#define FX1_ISP_AWB_GLOBAL_SUM_G_H_GLOBAL_SUM_G_H_SHIFT 0U
#define FX1_ISP_AWB_GLOBAL_SUM_G_H_GLOBAL_SUM_G_H_WIDTH 3U
#define FX1_ISP_AWB_GLOBAL_SUM_G_H_GLOBAL_SUM_G_H_MASK 0x00000007U
#define FX1_ISP_AWB_GLOBAL_SUM_G_H_GLOBAL_SUM_G_H_RESET 0x00000000U
#define FX1_ISP_AWB_GLOBAL_SUM_G_H_GLOBAL_SUM_G_H_ACCESS FX1_ISP_ACCESS_RO

/**
 * AWB_GLOBAL_SUM_G_L: địa chỉ lệch 0x2420, dòng bảng 382, kiểu truy cập RO
 * Tổng Gcell toàn frame trên các ô Bayer đủ điều kiện, chỉ đọc.
 */
#define FX1_ISP_AWB_GLOBAL_SUM_G_L_OFFSET 0x2420U
#define REG_AWB_GLOBAL_SUM_G_L FX1_ISP_AWB_GLOBAL_SUM_G_L_OFFSET
#define FX1_ISP_AWB_GLOBAL_SUM_G_L_RESET 0x00000000U
#define FX1_ISP_AWB_GLOBAL_SUM_G_L_ACCESS FX1_ISP_ACCESS_RO
#define FX1_ISP_AWB_GLOBAL_SUM_G_L_RO_MASK 0xFFFFFFFFU
#define FX1_ISP_AWB_GLOBAL_SUM_G_L_RW_MASK 0x00000000U
#define FX1_ISP_AWB_GLOBAL_SUM_G_L_W1C_MASK 0x00000000U
#define FX1_ISP_AWB_GLOBAL_SUM_G_L_W1S_MASK 0x00000000U
#define FX1_ISP_AWB_GLOBAL_SUM_G_L_W1P_MASK 0x00000000U
#define FX1_ISP_AWB_GLOBAL_SUM_G_L_W1SC_MASK 0x00000000U
/**
 * global_sum_g_l [31:0], dòng bảng 383, kiểu truy cập RO
 * Các bit thấp [31:0] của tổng màu tương ứng trên frame/zone đang giữ quyền đọc.
 */
#define FX1_ISP_AWB_GLOBAL_SUM_G_L_GLOBAL_SUM_G_L_SHIFT 0U
#define FX1_ISP_AWB_GLOBAL_SUM_G_L_GLOBAL_SUM_G_L_WIDTH 32U
#define FX1_ISP_AWB_GLOBAL_SUM_G_L_GLOBAL_SUM_G_L_MASK 0xFFFFFFFFU
#define FX1_ISP_AWB_GLOBAL_SUM_G_L_GLOBAL_SUM_G_L_RESET 0x00000000U
#define FX1_ISP_AWB_GLOBAL_SUM_G_L_GLOBAL_SUM_G_L_ACCESS FX1_ISP_ACCESS_RO

/**
 * AWB_GLOBAL_SUM_B_H: địa chỉ lệch 0x2424, dòng bảng 384, kiểu truy cập RO
 * Tổng Bcell toàn frame trên các ô Bayer đủ điều kiện, chỉ đọc.
 */
#define FX1_ISP_AWB_GLOBAL_SUM_B_H_OFFSET 0x2424U
#define REG_AWB_GLOBAL_SUM_B_H FX1_ISP_AWB_GLOBAL_SUM_B_H_OFFSET
#define FX1_ISP_AWB_GLOBAL_SUM_B_H_RESET 0x00000000U
#define FX1_ISP_AWB_GLOBAL_SUM_B_H_ACCESS FX1_ISP_ACCESS_RO
#define FX1_ISP_AWB_GLOBAL_SUM_B_H_RO_MASK 0x00000007U
#define FX1_ISP_AWB_GLOBAL_SUM_B_H_RW_MASK 0x00000000U
#define FX1_ISP_AWB_GLOBAL_SUM_B_H_W1C_MASK 0x00000000U
#define FX1_ISP_AWB_GLOBAL_SUM_B_H_W1S_MASK 0x00000000U
#define FX1_ISP_AWB_GLOBAL_SUM_B_H_W1P_MASK 0x00000000U
#define FX1_ISP_AWB_GLOBAL_SUM_B_H_W1SC_MASK 0x00000000U
/**
 * global_sum_b_h [2:0], dòng bảng 385, kiểu truy cập RO
 * Các bit cao [34:32] của tổng màu tương ứng trên frame/zone đang giữ quyền đọc.
 */
#define FX1_ISP_AWB_GLOBAL_SUM_B_H_GLOBAL_SUM_B_H_SHIFT 0U
#define FX1_ISP_AWB_GLOBAL_SUM_B_H_GLOBAL_SUM_B_H_WIDTH 3U
#define FX1_ISP_AWB_GLOBAL_SUM_B_H_GLOBAL_SUM_B_H_MASK 0x00000007U
#define FX1_ISP_AWB_GLOBAL_SUM_B_H_GLOBAL_SUM_B_H_RESET 0x00000000U
#define FX1_ISP_AWB_GLOBAL_SUM_B_H_GLOBAL_SUM_B_H_ACCESS FX1_ISP_ACCESS_RO

/**
 * AWB_GLOBAL_SUM_B_L: địa chỉ lệch 0x2428, dòng bảng 386, kiểu truy cập RO
 * Tổng Bcell toàn frame trên các ô Bayer đủ điều kiện, chỉ đọc.
 */
#define FX1_ISP_AWB_GLOBAL_SUM_B_L_OFFSET 0x2428U
#define REG_AWB_GLOBAL_SUM_B_L FX1_ISP_AWB_GLOBAL_SUM_B_L_OFFSET
#define FX1_ISP_AWB_GLOBAL_SUM_B_L_RESET 0x00000000U
#define FX1_ISP_AWB_GLOBAL_SUM_B_L_ACCESS FX1_ISP_ACCESS_RO
#define FX1_ISP_AWB_GLOBAL_SUM_B_L_RO_MASK 0xFFFFFFFFU
#define FX1_ISP_AWB_GLOBAL_SUM_B_L_RW_MASK 0x00000000U
#define FX1_ISP_AWB_GLOBAL_SUM_B_L_W1C_MASK 0x00000000U
#define FX1_ISP_AWB_GLOBAL_SUM_B_L_W1S_MASK 0x00000000U
#define FX1_ISP_AWB_GLOBAL_SUM_B_L_W1P_MASK 0x00000000U
#define FX1_ISP_AWB_GLOBAL_SUM_B_L_W1SC_MASK 0x00000000U
/**
 * global_sum_b_l [31:0], dòng bảng 387, kiểu truy cập RO
 * Các bit thấp [31:0] của tổng màu tương ứng trên frame/zone đang giữ quyền đọc.
 */
#define FX1_ISP_AWB_GLOBAL_SUM_B_L_GLOBAL_SUM_B_L_SHIFT 0U
#define FX1_ISP_AWB_GLOBAL_SUM_B_L_GLOBAL_SUM_B_L_WIDTH 32U
#define FX1_ISP_AWB_GLOBAL_SUM_B_L_GLOBAL_SUM_B_L_MASK 0xFFFFFFFFU
#define FX1_ISP_AWB_GLOBAL_SUM_B_L_GLOBAL_SUM_B_L_RESET 0x00000000U
#define FX1_ISP_AWB_GLOBAL_SUM_B_L_GLOBAL_SUM_B_L_ACCESS FX1_ISP_ACCESS_RO

/**
 * AWB_GLOBAL_COUNT: địa chỉ lệch 0x242C, dòng bảng 388, kiểu truy cập RO
 * Số ô Bayer đủ điều kiện góp vào tổng từng màu toàn frame, chỉ đọc.
 */
#define FX1_ISP_AWB_GLOBAL_COUNT_OFFSET 0x242CU
#define REG_AWB_GLOBAL_COUNT FX1_ISP_AWB_GLOBAL_COUNT_OFFSET
#define FX1_ISP_AWB_GLOBAL_COUNT_RESET 0x00000000U
#define FX1_ISP_AWB_GLOBAL_COUNT_ACCESS FX1_ISP_ACCESS_RO
#define FX1_ISP_AWB_GLOBAL_COUNT_RO_MASK 0x007FFFFFU
#define FX1_ISP_AWB_GLOBAL_COUNT_RW_MASK 0x00000000U
#define FX1_ISP_AWB_GLOBAL_COUNT_W1C_MASK 0x00000000U
#define FX1_ISP_AWB_GLOBAL_COUNT_W1S_MASK 0x00000000U
#define FX1_ISP_AWB_GLOBAL_COUNT_W1P_MASK 0x00000000U
#define FX1_ISP_AWB_GLOBAL_COUNT_W1SC_MASK 0x00000000U
/**
 * global_count [22:0], dòng bảng 389, kiểu truy cập RO
 * Số ô đủ điều kiện; tăng một lần mỗi ô 2x2 được nhận.
 */
#define FX1_ISP_AWB_GLOBAL_COUNT_GLOBAL_COUNT_SHIFT 0U
#define FX1_ISP_AWB_GLOBAL_COUNT_GLOBAL_COUNT_WIDTH 23U
#define FX1_ISP_AWB_GLOBAL_COUNT_GLOBAL_COUNT_MASK 0x007FFFFFU
#define FX1_ISP_AWB_GLOBAL_COUNT_GLOBAL_COUNT_RESET 0x00000000U
#define FX1_ISP_AWB_GLOBAL_COUNT_GLOBAL_COUNT_ACCESS FX1_ISP_ACCESS_RO

/**
 * AWB_ZONE_ADDR: địa chỉ lệch 0x2430, dòng bảng 390, kiểu truy cập RW
 * Chọn zone AWB của bank đang giữ quyền, chỉ số y*READ_NUM_ZONE_X+x dùng lưới chốt trong
 * RESULT_STATUS. Đợi một chu kỳ lõi sau chọn địa chỉ; ngoài lưới hoặc ô chưa ghi đọc 0.
 */
#define FX1_ISP_AWB_ZONE_ADDR_OFFSET 0x2430U
#define REG_AWB_ZONE_ADDR FX1_ISP_AWB_ZONE_ADDR_OFFSET
#define FX1_ISP_AWB_ZONE_ADDR_RESET 0x00000000U
#define FX1_ISP_AWB_ZONE_ADDR_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_AWB_ZONE_ADDR_RO_MASK 0x00000000U
#define FX1_ISP_AWB_ZONE_ADDR_RW_MASK 0x000007FFU
#define FX1_ISP_AWB_ZONE_ADDR_W1C_MASK 0x00000000U
#define FX1_ISP_AWB_ZONE_ADDR_W1S_MASK 0x00000000U
#define FX1_ISP_AWB_ZONE_ADDR_W1P_MASK 0x00000000U
#define FX1_ISP_AWB_ZONE_ADDR_W1SC_MASK 0x00000000U
/**
 * zone_addr [10:0], dòng bảng 391, kiểu truy cập RW
 * Chọn zone theo thứ tự ảnh, chỉ số hàng trước rồi cột.
 */
#define FX1_ISP_AWB_ZONE_ADDR_ZONE_ADDR_SHIFT 0U
#define FX1_ISP_AWB_ZONE_ADDR_ZONE_ADDR_WIDTH 11U
#define FX1_ISP_AWB_ZONE_ADDR_ZONE_ADDR_MASK 0x000007FFU
#define FX1_ISP_AWB_ZONE_ADDR_ZONE_ADDR_RESET 0x00000000U
#define FX1_ISP_AWB_ZONE_ADDR_ZONE_ADDR_ACCESS FX1_ISP_ACCESS_RW

/**
 * AWB_ZONE_SUM_R_H: địa chỉ lệch 0x2434, dòng bảng 392, kiểu truy cập RO
 * Tổng Rcell của zone đang chọn trên các ô Bayer đủ điều kiện, chỉ đọc.
 */
#define FX1_ISP_AWB_ZONE_SUM_R_H_OFFSET 0x2434U
#define REG_AWB_ZONE_SUM_R_H FX1_ISP_AWB_ZONE_SUM_R_H_OFFSET
#define FX1_ISP_AWB_ZONE_SUM_R_H_RESET 0x00000000U
#define FX1_ISP_AWB_ZONE_SUM_R_H_ACCESS FX1_ISP_ACCESS_RO
#define FX1_ISP_AWB_ZONE_SUM_R_H_RO_MASK 0x00000007U
#define FX1_ISP_AWB_ZONE_SUM_R_H_RW_MASK 0x00000000U
#define FX1_ISP_AWB_ZONE_SUM_R_H_W1C_MASK 0x00000000U
#define FX1_ISP_AWB_ZONE_SUM_R_H_W1S_MASK 0x00000000U
#define FX1_ISP_AWB_ZONE_SUM_R_H_W1P_MASK 0x00000000U
#define FX1_ISP_AWB_ZONE_SUM_R_H_W1SC_MASK 0x00000000U
/**
 * zone_sum_r_h [2:0], dòng bảng 393, kiểu truy cập RO
 * Các bit cao [34:32] của tổng màu tương ứng trên frame/zone đang giữ quyền đọc.
 */
#define FX1_ISP_AWB_ZONE_SUM_R_H_ZONE_SUM_R_H_SHIFT 0U
#define FX1_ISP_AWB_ZONE_SUM_R_H_ZONE_SUM_R_H_WIDTH 3U
#define FX1_ISP_AWB_ZONE_SUM_R_H_ZONE_SUM_R_H_MASK 0x00000007U
#define FX1_ISP_AWB_ZONE_SUM_R_H_ZONE_SUM_R_H_RESET 0x00000000U
#define FX1_ISP_AWB_ZONE_SUM_R_H_ZONE_SUM_R_H_ACCESS FX1_ISP_ACCESS_RO

/**
 * AWB_ZONE_SUM_R_L: địa chỉ lệch 0x2438, dòng bảng 394, kiểu truy cập RO
 * Tổng Rcell của zone đang chọn trên các ô Bayer đủ điều kiện, chỉ đọc.
 */
#define FX1_ISP_AWB_ZONE_SUM_R_L_OFFSET 0x2438U
#define REG_AWB_ZONE_SUM_R_L FX1_ISP_AWB_ZONE_SUM_R_L_OFFSET
#define FX1_ISP_AWB_ZONE_SUM_R_L_RESET 0x00000000U
#define FX1_ISP_AWB_ZONE_SUM_R_L_ACCESS FX1_ISP_ACCESS_RO
#define FX1_ISP_AWB_ZONE_SUM_R_L_RO_MASK 0xFFFFFFFFU
#define FX1_ISP_AWB_ZONE_SUM_R_L_RW_MASK 0x00000000U
#define FX1_ISP_AWB_ZONE_SUM_R_L_W1C_MASK 0x00000000U
#define FX1_ISP_AWB_ZONE_SUM_R_L_W1S_MASK 0x00000000U
#define FX1_ISP_AWB_ZONE_SUM_R_L_W1P_MASK 0x00000000U
#define FX1_ISP_AWB_ZONE_SUM_R_L_W1SC_MASK 0x00000000U
/**
 * zone_sum_r_l [31:0], dòng bảng 395, kiểu truy cập RO
 * Các bit thấp [31:0] của tổng màu tương ứng trên frame/zone đang giữ quyền đọc.
 */
#define FX1_ISP_AWB_ZONE_SUM_R_L_ZONE_SUM_R_L_SHIFT 0U
#define FX1_ISP_AWB_ZONE_SUM_R_L_ZONE_SUM_R_L_WIDTH 32U
#define FX1_ISP_AWB_ZONE_SUM_R_L_ZONE_SUM_R_L_MASK 0xFFFFFFFFU
#define FX1_ISP_AWB_ZONE_SUM_R_L_ZONE_SUM_R_L_RESET 0x00000000U
#define FX1_ISP_AWB_ZONE_SUM_R_L_ZONE_SUM_R_L_ACCESS FX1_ISP_ACCESS_RO

/**
 * AWB_ZONE_SUM_G_H: địa chỉ lệch 0x243C, dòng bảng 396, kiểu truy cập RO
 * Tổng Gcell của zone đang chọn trên các ô Bayer đủ điều kiện, chỉ đọc.
 */
#define FX1_ISP_AWB_ZONE_SUM_G_H_OFFSET 0x243CU
#define REG_AWB_ZONE_SUM_G_H FX1_ISP_AWB_ZONE_SUM_G_H_OFFSET
#define FX1_ISP_AWB_ZONE_SUM_G_H_RESET 0x00000000U
#define FX1_ISP_AWB_ZONE_SUM_G_H_ACCESS FX1_ISP_ACCESS_RO
#define FX1_ISP_AWB_ZONE_SUM_G_H_RO_MASK 0x00000007U
#define FX1_ISP_AWB_ZONE_SUM_G_H_RW_MASK 0x00000000U
#define FX1_ISP_AWB_ZONE_SUM_G_H_W1C_MASK 0x00000000U
#define FX1_ISP_AWB_ZONE_SUM_G_H_W1S_MASK 0x00000000U
#define FX1_ISP_AWB_ZONE_SUM_G_H_W1P_MASK 0x00000000U
#define FX1_ISP_AWB_ZONE_SUM_G_H_W1SC_MASK 0x00000000U
/**
 * zone_sum_g_h [2:0], dòng bảng 397, kiểu truy cập RO
 * Các bit cao [34:32] của tổng màu tương ứng trên frame/zone đang giữ quyền đọc.
 */
#define FX1_ISP_AWB_ZONE_SUM_G_H_ZONE_SUM_G_H_SHIFT 0U
#define FX1_ISP_AWB_ZONE_SUM_G_H_ZONE_SUM_G_H_WIDTH 3U
#define FX1_ISP_AWB_ZONE_SUM_G_H_ZONE_SUM_G_H_MASK 0x00000007U
#define FX1_ISP_AWB_ZONE_SUM_G_H_ZONE_SUM_G_H_RESET 0x00000000U
#define FX1_ISP_AWB_ZONE_SUM_G_H_ZONE_SUM_G_H_ACCESS FX1_ISP_ACCESS_RO

/**
 * AWB_ZONE_SUM_G_L: địa chỉ lệch 0x2440, dòng bảng 398, kiểu truy cập RO
 * Tổng Gcell của zone đang chọn trên các ô Bayer đủ điều kiện, chỉ đọc.
 */
#define FX1_ISP_AWB_ZONE_SUM_G_L_OFFSET 0x2440U
#define REG_AWB_ZONE_SUM_G_L FX1_ISP_AWB_ZONE_SUM_G_L_OFFSET
#define FX1_ISP_AWB_ZONE_SUM_G_L_RESET 0x00000000U
#define FX1_ISP_AWB_ZONE_SUM_G_L_ACCESS FX1_ISP_ACCESS_RO
#define FX1_ISP_AWB_ZONE_SUM_G_L_RO_MASK 0xFFFFFFFFU
#define FX1_ISP_AWB_ZONE_SUM_G_L_RW_MASK 0x00000000U
#define FX1_ISP_AWB_ZONE_SUM_G_L_W1C_MASK 0x00000000U
#define FX1_ISP_AWB_ZONE_SUM_G_L_W1S_MASK 0x00000000U
#define FX1_ISP_AWB_ZONE_SUM_G_L_W1P_MASK 0x00000000U
#define FX1_ISP_AWB_ZONE_SUM_G_L_W1SC_MASK 0x00000000U
/**
 * zone_sum_g_l [31:0], dòng bảng 399, kiểu truy cập RO
 * Các bit thấp [31:0] của tổng màu tương ứng trên frame/zone đang giữ quyền đọc.
 */
#define FX1_ISP_AWB_ZONE_SUM_G_L_ZONE_SUM_G_L_SHIFT 0U
#define FX1_ISP_AWB_ZONE_SUM_G_L_ZONE_SUM_G_L_WIDTH 32U
#define FX1_ISP_AWB_ZONE_SUM_G_L_ZONE_SUM_G_L_MASK 0xFFFFFFFFU
#define FX1_ISP_AWB_ZONE_SUM_G_L_ZONE_SUM_G_L_RESET 0x00000000U
#define FX1_ISP_AWB_ZONE_SUM_G_L_ZONE_SUM_G_L_ACCESS FX1_ISP_ACCESS_RO

/**
 * AWB_ZONE_SUM_B_H: địa chỉ lệch 0x2444, dòng bảng 400, kiểu truy cập RO
 * Tổng Bcell của zone đang chọn trên ô Bayer đủ điều kiện, chỉ đọc.
 */
#define FX1_ISP_AWB_ZONE_SUM_B_H_OFFSET 0x2444U
#define REG_AWB_ZONE_SUM_B_H FX1_ISP_AWB_ZONE_SUM_B_H_OFFSET
#define FX1_ISP_AWB_ZONE_SUM_B_H_RESET 0x00000000U
#define FX1_ISP_AWB_ZONE_SUM_B_H_ACCESS FX1_ISP_ACCESS_RO
#define FX1_ISP_AWB_ZONE_SUM_B_H_RO_MASK 0x00000007U
#define FX1_ISP_AWB_ZONE_SUM_B_H_RW_MASK 0x00000000U
#define FX1_ISP_AWB_ZONE_SUM_B_H_W1C_MASK 0x00000000U
#define FX1_ISP_AWB_ZONE_SUM_B_H_W1S_MASK 0x00000000U
#define FX1_ISP_AWB_ZONE_SUM_B_H_W1P_MASK 0x00000000U
#define FX1_ISP_AWB_ZONE_SUM_B_H_W1SC_MASK 0x00000000U
/**
 * zone_sum_b_h [2:0], dòng bảng 401, kiểu truy cập RO
 * Các bit [34:32] của tổng B trong zone đang giữ quyền.
 */
#define FX1_ISP_AWB_ZONE_SUM_B_H_ZONE_SUM_B_H_SHIFT 0U
#define FX1_ISP_AWB_ZONE_SUM_B_H_ZONE_SUM_B_H_WIDTH 3U
#define FX1_ISP_AWB_ZONE_SUM_B_H_ZONE_SUM_B_H_MASK 0x00000007U
#define FX1_ISP_AWB_ZONE_SUM_B_H_ZONE_SUM_B_H_RESET 0x00000000U
#define FX1_ISP_AWB_ZONE_SUM_B_H_ZONE_SUM_B_H_ACCESS FX1_ISP_ACCESS_RO

/**
 * AWB_ZONE_SUM_B_L: địa chỉ lệch 0x2448, dòng bảng 402, kiểu truy cập RO
 * Tổng Bcell của zone đang chọn trên ô Bayer đủ điều kiện, chỉ đọc.
 */
#define FX1_ISP_AWB_ZONE_SUM_B_L_OFFSET 0x2448U
#define REG_AWB_ZONE_SUM_B_L FX1_ISP_AWB_ZONE_SUM_B_L_OFFSET
#define FX1_ISP_AWB_ZONE_SUM_B_L_RESET 0x00000000U
#define FX1_ISP_AWB_ZONE_SUM_B_L_ACCESS FX1_ISP_ACCESS_RO
#define FX1_ISP_AWB_ZONE_SUM_B_L_RO_MASK 0xFFFFFFFFU
#define FX1_ISP_AWB_ZONE_SUM_B_L_RW_MASK 0x00000000U
#define FX1_ISP_AWB_ZONE_SUM_B_L_W1C_MASK 0x00000000U
#define FX1_ISP_AWB_ZONE_SUM_B_L_W1S_MASK 0x00000000U
#define FX1_ISP_AWB_ZONE_SUM_B_L_W1P_MASK 0x00000000U
#define FX1_ISP_AWB_ZONE_SUM_B_L_W1SC_MASK 0x00000000U
/**
 * zone_sum_b_l [31:0], dòng bảng 403, kiểu truy cập RO
 * Các bit [31:0] của tổng B trong zone đang giữ quyền.
 */
#define FX1_ISP_AWB_ZONE_SUM_B_L_ZONE_SUM_B_L_SHIFT 0U
#define FX1_ISP_AWB_ZONE_SUM_B_L_ZONE_SUM_B_L_WIDTH 32U
#define FX1_ISP_AWB_ZONE_SUM_B_L_ZONE_SUM_B_L_MASK 0xFFFFFFFFU
#define FX1_ISP_AWB_ZONE_SUM_B_L_ZONE_SUM_B_L_RESET 0x00000000U
#define FX1_ISP_AWB_ZONE_SUM_B_L_ZONE_SUM_B_L_ACCESS FX1_ISP_ACCESS_RO

/**
 * AWB_ZONE_COUNT: địa chỉ lệch 0x244C, dòng bảng 404, kiểu truy cập RO
 * Số ô Bayer đủ điều kiện của zone đang chọn, chỉ đọc.
 */
#define FX1_ISP_AWB_ZONE_COUNT_OFFSET 0x244CU
#define REG_AWB_ZONE_COUNT FX1_ISP_AWB_ZONE_COUNT_OFFSET
#define FX1_ISP_AWB_ZONE_COUNT_RESET 0x00000000U
#define FX1_ISP_AWB_ZONE_COUNT_ACCESS FX1_ISP_ACCESS_RO
#define FX1_ISP_AWB_ZONE_COUNT_RO_MASK 0x007FFFFFU
#define FX1_ISP_AWB_ZONE_COUNT_RW_MASK 0x00000000U
#define FX1_ISP_AWB_ZONE_COUNT_W1C_MASK 0x00000000U
#define FX1_ISP_AWB_ZONE_COUNT_W1S_MASK 0x00000000U
#define FX1_ISP_AWB_ZONE_COUNT_W1P_MASK 0x00000000U
#define FX1_ISP_AWB_ZONE_COUNT_W1SC_MASK 0x00000000U
/**
 * zone_count [22:0], dòng bảng 405, kiểu truy cập RO
 * Tăng một lần trên mỗi ô đủ điều kiện của zone.
 */
#define FX1_ISP_AWB_ZONE_COUNT_ZONE_COUNT_SHIFT 0U
#define FX1_ISP_AWB_ZONE_COUNT_ZONE_COUNT_WIDTH 23U
#define FX1_ISP_AWB_ZONE_COUNT_ZONE_COUNT_MASK 0x007FFFFFU
#define FX1_ISP_AWB_ZONE_COUNT_ZONE_COUNT_RESET 0x00000000U
#define FX1_ISP_AWB_ZONE_COUNT_ZONE_COUNT_ACCESS FX1_ISP_ACCESS_RO

/**
 * AWB_STATUS: địa chỉ lệch 0x2450, dòng bảng 406, kiểu truy cập W1C
 * Sự kiện hoàn tất AWB được chốt; sự kiện không cấp quyền đọc. Phải ACQUIRE bank READY bằng
 * RESULT_CTRL trước đọc kết quả.
 */
#define FX1_ISP_AWB_STATUS_OFFSET 0x2450U
#define REG_AWB_STATUS FX1_ISP_AWB_STATUS_OFFSET
#define FX1_ISP_AWB_STATUS_RESET 0x00000000U
#define FX1_ISP_AWB_STATUS_ACCESS FX1_ISP_ACCESS_W1C
#define FX1_ISP_AWB_STATUS_RO_MASK 0x00000000U
#define FX1_ISP_AWB_STATUS_RW_MASK 0x00000000U
#define FX1_ISP_AWB_STATUS_W1C_MASK 0x00000001U
#define FX1_ISP_AWB_STATUS_W1S_MASK 0x00000000U
#define FX1_ISP_AWB_STATUS_W1P_MASK 0x00000000U
#define FX1_ISP_AWB_STATUS_W1SC_MASK 0x00000000U
/**
 * stat_done [0:0], dòng bảng 407, kiểu truy cập W1C
 * Sự kiện W1C đặt khi bank kết quả đầy đủ thành READY. Xóa không xin/giải phóng quyền; hoàn tất mới
 * thắng xóa đồng thời. Thăm dò READY_COUNT để nhận biết hàng kết quả độc lập sự kiện này.
 */
#define FX1_ISP_AWB_STATUS_STAT_DONE_SHIFT 0U
#define FX1_ISP_AWB_STATUS_STAT_DONE_WIDTH 1U
#define FX1_ISP_AWB_STATUS_STAT_DONE_MASK 0x00000001U
#define FX1_ISP_AWB_STATUS_STAT_DONE_RESET 0x00000000U
#define FX1_ISP_AWB_STATUS_STAT_DONE_ACCESS FX1_ISP_ACCESS_W1C

/**
 * AWB_UNDEREXPOSED_LIMIT: địa chỉ lệch 0x2454, dòng bảng 408, kiểu truy cập RW
 * Ngưỡng dưới nghiêm ngặt theo Bayer 12 bit sau BPC. Loại ô nếu một trong bốn mẫu <= ngưỡng.
 */
#define FX1_ISP_AWB_UNDEREXPOSED_LIMIT_OFFSET 0x2454U
#define REG_AWB_UNDEREXPOSED_LIMIT FX1_ISP_AWB_UNDEREXPOSED_LIMIT_OFFSET
#define FX1_ISP_AWB_UNDEREXPOSED_LIMIT_RESET 0x00000000U
#define FX1_ISP_AWB_UNDEREXPOSED_LIMIT_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_AWB_UNDEREXPOSED_LIMIT_RO_MASK 0x00000000U
#define FX1_ISP_AWB_UNDEREXPOSED_LIMIT_RW_MASK 0x00000FFFU
#define FX1_ISP_AWB_UNDEREXPOSED_LIMIT_W1C_MASK 0x00000000U
#define FX1_ISP_AWB_UNDEREXPOSED_LIMIT_W1S_MASK 0x00000000U
#define FX1_ISP_AWB_UNDEREXPOSED_LIMIT_W1P_MASK 0x00000000U
#define FX1_ISP_AWB_UNDEREXPOSED_LIMIT_W1SC_MASK 0x00000000U
/**
 * underexposed_limit [11:0], dòng bảng 409, kiểu truy cập RW
 * Ngưỡng Bayer 12 bit; cả bốn mẫu phải lớn hơn nghiêm ngặt.
 */
#define FX1_ISP_AWB_UNDEREXPOSED_LIMIT_UNDEREXPOSED_LIMIT_SHIFT 0U
#define FX1_ISP_AWB_UNDEREXPOSED_LIMIT_UNDEREXPOSED_LIMIT_WIDTH 12U
#define FX1_ISP_AWB_UNDEREXPOSED_LIMIT_UNDEREXPOSED_LIMIT_MASK 0x00000FFFU
#define FX1_ISP_AWB_UNDEREXPOSED_LIMIT_UNDEREXPOSED_LIMIT_RESET 0x00000000U
#define FX1_ISP_AWB_UNDEREXPOSED_LIMIT_UNDEREXPOSED_LIMIT_ACCESS FX1_ISP_ACCESS_RW

/**
 * AWB_SATURATION_LIMIT: địa chỉ lệch 0x2458, dòng bảng 410, kiểu truy cập RW
 * Ngưỡng trên nghiêm ngặt theo Bayer 12 bit sau BPC. Loại ô nếu một trong bốn mẫu >= ngưỡng.
 */
#define FX1_ISP_AWB_SATURATION_LIMIT_OFFSET 0x2458U
#define REG_AWB_SATURATION_LIMIT FX1_ISP_AWB_SATURATION_LIMIT_OFFSET
#define FX1_ISP_AWB_SATURATION_LIMIT_RESET 0x00000FFFU
#define FX1_ISP_AWB_SATURATION_LIMIT_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_AWB_SATURATION_LIMIT_RO_MASK 0x00000000U
#define FX1_ISP_AWB_SATURATION_LIMIT_RW_MASK 0x00000FFFU
#define FX1_ISP_AWB_SATURATION_LIMIT_W1C_MASK 0x00000000U
#define FX1_ISP_AWB_SATURATION_LIMIT_W1S_MASK 0x00000000U
#define FX1_ISP_AWB_SATURATION_LIMIT_W1P_MASK 0x00000000U
#define FX1_ISP_AWB_SATURATION_LIMIT_W1SC_MASK 0x00000000U
/**
 * saturation_limit [11:0], dòng bảng 411, kiểu truy cập RW
 * Ngưỡng Bayer 12 bit; cả bốn mẫu phải nhỏ hơn nghiêm ngặt.
 */
#define FX1_ISP_AWB_SATURATION_LIMIT_SATURATION_LIMIT_SHIFT 0U
#define FX1_ISP_AWB_SATURATION_LIMIT_SATURATION_LIMIT_WIDTH 12U
#define FX1_ISP_AWB_SATURATION_LIMIT_SATURATION_LIMIT_MASK 0x00000FFFU
#define FX1_ISP_AWB_SATURATION_LIMIT_SATURATION_LIMIT_RESET 0x00000FFFU
#define FX1_ISP_AWB_SATURATION_LIMIT_SATURATION_LIMIT_ACCESS FX1_ISP_ACCESS_RW

/**
 * AWB_CONTEXT_ID: địa chỉ lệch 0x245C, dòng bảng 412, kiểu truy cập RW
 * Nhãn điều khiển nguồn sáng/màu do phần mềm đặt, chốt cùng cấu hình ở SOF đầu vào.
 */
#define FX1_ISP_AWB_CONTEXT_ID_OFFSET 0x245CU
#define REG_AWB_CONTEXT_ID FX1_ISP_AWB_CONTEXT_ID_OFFSET
#define FX1_ISP_AWB_CONTEXT_ID_RESET 0x00000000U
#define FX1_ISP_AWB_CONTEXT_ID_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_AWB_CONTEXT_ID_RO_MASK 0x00000000U
#define FX1_ISP_AWB_CONTEXT_ID_RW_MASK 0xFFFFFFFFU
#define FX1_ISP_AWB_CONTEXT_ID_W1C_MASK 0x00000000U
#define FX1_ISP_AWB_CONTEXT_ID_W1S_MASK 0x00000000U
#define FX1_ISP_AWB_CONTEXT_ID_W1P_MASK 0x00000000U
#define FX1_ISP_AWB_CONTEXT_ID_W1SC_MASK 0x00000000U
/**
 * context_id [31:0], dòng bảng 413, kiểu truy cập RW
 * Context phần mềm tùy ý, chốt tại SOF đầu vào và công bố cùng kết quả frame đó.
 */
#define FX1_ISP_AWB_CONTEXT_ID_CONTEXT_ID_SHIFT 0U
#define FX1_ISP_AWB_CONTEXT_ID_CONTEXT_ID_WIDTH 32U
#define FX1_ISP_AWB_CONTEXT_ID_CONTEXT_ID_MASK 0xFFFFFFFFU
#define FX1_ISP_AWB_CONTEXT_ID_CONTEXT_ID_RESET 0x00000000U
#define FX1_ISP_AWB_CONTEXT_ID_CONTEXT_ID_ACCESS FX1_ISP_ACCESS_RW

/**
 * AWB_RESULT_CONTEXT_ID: địa chỉ lệch 0x2460, dòng bảng 414, kiểu truy cập RO
 * AWB_CONTEXT_ID đã chốt với cấu hình thống kê bất biến tại SOF đầu vào, chỉ đọc.
 */
#define FX1_ISP_AWB_RESULT_CONTEXT_ID_OFFSET 0x2460U
#define REG_AWB_RESULT_CONTEXT_ID FX1_ISP_AWB_RESULT_CONTEXT_ID_OFFSET
#define FX1_ISP_AWB_RESULT_CONTEXT_ID_RESET 0x00000000U
#define FX1_ISP_AWB_RESULT_CONTEXT_ID_ACCESS FX1_ISP_ACCESS_RO
#define FX1_ISP_AWB_RESULT_CONTEXT_ID_RO_MASK 0xFFFFFFFFU
#define FX1_ISP_AWB_RESULT_CONTEXT_ID_RW_MASK 0x00000000U
#define FX1_ISP_AWB_RESULT_CONTEXT_ID_W1C_MASK 0x00000000U
#define FX1_ISP_AWB_RESULT_CONTEXT_ID_W1S_MASK 0x00000000U
#define FX1_ISP_AWB_RESULT_CONTEXT_ID_W1P_MASK 0x00000000U
#define FX1_ISP_AWB_RESULT_CONTEXT_ID_W1SC_MASK 0x00000000U
/**
 * context_id [31:0], dòng bảng 415, kiểu truy cập RO
 * AWB_CONTEXT_ID của frame đang giữ quyền đọc, chốt tại SOF đầu vào.
 */
#define FX1_ISP_AWB_RESULT_CONTEXT_ID_CONTEXT_ID_SHIFT 0U
#define FX1_ISP_AWB_RESULT_CONTEXT_ID_CONTEXT_ID_WIDTH 32U
#define FX1_ISP_AWB_RESULT_CONTEXT_ID_CONTEXT_ID_MASK 0xFFFFFFFFU
#define FX1_ISP_AWB_RESULT_CONTEXT_ID_CONTEXT_ID_RESET 0x00000000U
#define FX1_ISP_AWB_RESULT_CONTEXT_ID_CONTEXT_ID_ACCESS FX1_ISP_ACCESS_RO

/**
 * AWB_FRAME_ID: địa chỉ lệch 0x2464, dòng bảng 416, kiểu truy cập RO
 * ID frame đầu vào của bank AWB đang giữ quyền, chỉ đọc.
 */
#define FX1_ISP_AWB_FRAME_ID_OFFSET 0x2464U
#define REG_AWB_FRAME_ID FX1_ISP_AWB_FRAME_ID_OFFSET
#define FX1_ISP_AWB_FRAME_ID_RESET 0x00000000U
#define FX1_ISP_AWB_FRAME_ID_ACCESS FX1_ISP_ACCESS_RO
#define FX1_ISP_AWB_FRAME_ID_RO_MASK 0xFFFFFFFFU
#define FX1_ISP_AWB_FRAME_ID_RW_MASK 0x00000000U
#define FX1_ISP_AWB_FRAME_ID_W1C_MASK 0x00000000U
#define FX1_ISP_AWB_FRAME_ID_W1S_MASK 0x00000000U
#define FX1_ISP_AWB_FRAME_ID_W1P_MASK 0x00000000U
#define FX1_ISP_AWB_FRAME_ID_W1SC_MASK 0x00000000U
/**
 * frame_id [31:0], dòng bảng 417, kiểu truy cập RO
 * Số frame dùng chung tại SOF đầu vào. Frame đầu sau hard reset là 1; cuộn vòng modulo 2^32.
 * Common/DMA soft reset giữ bộ đếm. Thông tin ổn định đến RELEASE; chưa giữ quyền thì đọc 0 và báo
 * lỗi sở hữu.
 */
#define FX1_ISP_AWB_FRAME_ID_FRAME_ID_SHIFT 0U
#define FX1_ISP_AWB_FRAME_ID_FRAME_ID_WIDTH 32U
#define FX1_ISP_AWB_FRAME_ID_FRAME_ID_MASK 0xFFFFFFFFU
#define FX1_ISP_AWB_FRAME_ID_FRAME_ID_RESET 0x00000000U
#define FX1_ISP_AWB_FRAME_ID_FRAME_ID_ACCESS FX1_ISP_ACCESS_RO

/**
 * AWB_RESULT_CTRL: địa chỉ lệch 0x2468, dòng bảng 418, kiểu truy cập W1P
 * Lệnh AWB ghi 1 phát xung một chu kỳ, xét byte strobe; đọc 0, bỏ bit dự phòng. RELEASE thắng
 * ACQUIRE cùng lúc; sự kiện mất mới thắng CLEAR_REPORT.
 */
#define FX1_ISP_AWB_RESULT_CTRL_OFFSET 0x2468U
#define REG_AWB_RESULT_CTRL FX1_ISP_AWB_RESULT_CTRL_OFFSET
#define FX1_ISP_AWB_RESULT_CTRL_RESET 0x00000000U
#define FX1_ISP_AWB_RESULT_CTRL_ACCESS FX1_ISP_ACCESS_W1P
#define FX1_ISP_AWB_RESULT_CTRL_RO_MASK 0x00000000U
#define FX1_ISP_AWB_RESULT_CTRL_RW_MASK 0x00000000U
#define FX1_ISP_AWB_RESULT_CTRL_W1C_MASK 0x00000000U
#define FX1_ISP_AWB_RESULT_CTRL_W1S_MASK 0x00000000U
#define FX1_ISP_AWB_RESULT_CTRL_W1P_MASK 0x00000007U
#define FX1_ISP_AWB_RESULT_CTRL_W1SC_MASK 0x00000000U
/**
 * acquire [0:0], dòng bảng 419, kiểu truy cập W1P
 * Xin quyền đọc bank READY hoàn tất sớm nhất. Lệnh một chu kỳ, đọc 0; không tác dụng khi chưa có
 * READY hoặc đã giữ quyền. Xác nhận READ_VALID trước đọc, chờ một chu kỳ trước đọc zone.
 */
#define FX1_ISP_AWB_RESULT_CTRL_ACQUIRE_SHIFT 0U
#define FX1_ISP_AWB_RESULT_CTRL_ACQUIRE_WIDTH 1U
#define FX1_ISP_AWB_RESULT_CTRL_ACQUIRE_MASK 0x00000001U
#define FX1_ISP_AWB_RESULT_CTRL_ACQUIRE_RESET 0x00000000U
#define FX1_ISP_AWB_RESULT_CTRL_ACQUIRE_ACCESS FX1_ISP_ACCESS_W1P
/**
 * release [1:1], dòng bảng 420, kiểu truy cập W1P
 * Giải phóng bank LEASED về FREE. Lệnh một chu kỳ, đọc 0; chưa giữ quyền thì không tác dụng. Bank
 * vừa giải phóng có thể nhận frame từ cạnh kế tiếp.
 */
#define FX1_ISP_AWB_RESULT_CTRL_RELEASE_SHIFT 1U
#define FX1_ISP_AWB_RESULT_CTRL_RELEASE_WIDTH 1U
#define FX1_ISP_AWB_RESULT_CTRL_RELEASE_MASK 0x00000002U
#define FX1_ISP_AWB_RESULT_CTRL_RELEASE_RESET 0x00000000U
#define FX1_ISP_AWB_RESULT_CTRL_RELEASE_ACCESS FX1_ISP_ACCESS_W1P
/**
 * clear_report [2:2], dòng bảng 421, kiểu truy cập W1P
 * Chỉ xóa OVERRUN/DROP_COUNT. Lệnh một chu kỳ, đọc 0, không giải phóng kết quả hoặc xóa STAT_DONE.
 * Mất mới đồng thời đặt lại OVERRUN và tăng bộ đếm vừa xóa.
 */
#define FX1_ISP_AWB_RESULT_CTRL_CLEAR_REPORT_SHIFT 2U
#define FX1_ISP_AWB_RESULT_CTRL_CLEAR_REPORT_WIDTH 1U
#define FX1_ISP_AWB_RESULT_CTRL_CLEAR_REPORT_MASK 0x00000004U
#define FX1_ISP_AWB_RESULT_CTRL_CLEAR_REPORT_RESET 0x00000000U
#define FX1_ISP_AWB_RESULT_CTRL_CLEAR_REPORT_ACCESS FX1_ISP_ACCESS_W1P

/**
 * AWB_RESULT_STATUS: địa chỉ lệch 0x246C, dòng bảng 422, kiểu truy cập RO
 * Trạng thái sở hữu AWB: hai bank, tối đa một quyền đọc. READY/LEASED bất biến; bit dự phòng đọc 0.
 */
#define FX1_ISP_AWB_RESULT_STATUS_OFFSET 0x246CU
#define REG_AWB_RESULT_STATUS FX1_ISP_AWB_RESULT_STATUS_OFFSET
#define FX1_ISP_AWB_RESULT_STATUS_RESET 0x00000000U
#define FX1_ISP_AWB_RESULT_STATUS_ACCESS FX1_ISP_ACCESS_RO
#define FX1_ISP_AWB_RESULT_STATUS_RO_MASK 0x003F7F7FU
#define FX1_ISP_AWB_RESULT_STATUS_RW_MASK 0x00000000U
#define FX1_ISP_AWB_RESULT_STATUS_W1C_MASK 0x00000000U
#define FX1_ISP_AWB_RESULT_STATUS_W1S_MASK 0x00000000U
#define FX1_ISP_AWB_RESULT_STATUS_W1P_MASK 0x00000000U
#define FX1_ISP_AWB_RESULT_STATUS_W1SC_MASK 0x00000000U
/**
 * ready_count [1:0], dòng bảng 423, kiểu truy cập RO
 * Số bank READY đã hoàn tất, 0..2.
 */
#define FX1_ISP_AWB_RESULT_STATUS_READY_COUNT_SHIFT 0U
#define FX1_ISP_AWB_RESULT_STATUS_READY_COUNT_WIDTH 2U
#define FX1_ISP_AWB_RESULT_STATUS_READY_COUNT_MASK 0x00000003U
#define FX1_ISP_AWB_RESULT_STATUS_READY_COUNT_RESET 0x00000000U
#define FX1_ISP_AWB_RESULT_STATUS_READY_COUNT_ACCESS FX1_ISP_ACCESS_RO
/**
 * leased [2:2], dòng bảng 424, kiểu truy cập RO
 * Bằng 1 khi phần mềm giữ quyền đọc một bank.
 */
#define FX1_ISP_AWB_RESULT_STATUS_LEASED_SHIFT 2U
#define FX1_ISP_AWB_RESULT_STATUS_LEASED_WIDTH 1U
#define FX1_ISP_AWB_RESULT_STATUS_LEASED_MASK 0x00000004U
#define FX1_ISP_AWB_RESULT_STATUS_LEASED_RESET 0x00000000U
#define FX1_ISP_AWB_RESULT_STATUS_LEASED_ACCESS FX1_ISP_ACCESS_RO
/**
 * read_valid [3:3], dòng bảng 425, kiểu truy cập RO
 * READ_VALID bằng LEASED; khi 0, các cổng kết quả đọc 0.
 */
#define FX1_ISP_AWB_RESULT_STATUS_READ_VALID_SHIFT 3U
#define FX1_ISP_AWB_RESULT_STATUS_READ_VALID_WIDTH 1U
#define FX1_ISP_AWB_RESULT_STATUS_READ_VALID_MASK 0x00000008U
#define FX1_ISP_AWB_RESULT_STATUS_READ_VALID_RESET 0x00000000U
#define FX1_ISP_AWB_RESULT_STATUS_READ_VALID_ACCESS FX1_ISP_ACCESS_RO
/**
 * read_bank [4:4], dòng bảng 426, kiểu truy cập RO
 * Chỉ số bank đang giữ quyền; chưa giữ thì bằng 0.
 */
#define FX1_ISP_AWB_RESULT_STATUS_READ_BANK_SHIFT 4U
#define FX1_ISP_AWB_RESULT_STATUS_READ_BANK_WIDTH 1U
#define FX1_ISP_AWB_RESULT_STATUS_READ_BANK_MASK 0x00000010U
#define FX1_ISP_AWB_RESULT_STATUS_READ_BANK_RESET 0x00000000U
#define FX1_ISP_AWB_RESULT_STATUS_READ_BANK_ACCESS FX1_ISP_ACCESS_RO
/**
 * write_busy [5:5], dòng bảng 427, kiểu truy cập RO
 * Bằng 1 khi một bank đang ghi, gồm cả xử lý hết các phép tích lũy cuối.
 */
#define FX1_ISP_AWB_RESULT_STATUS_WRITE_BUSY_SHIFT 5U
#define FX1_ISP_AWB_RESULT_STATUS_WRITE_BUSY_WIDTH 1U
#define FX1_ISP_AWB_RESULT_STATUS_WRITE_BUSY_MASK 0x00000020U
#define FX1_ISP_AWB_RESULT_STATUS_WRITE_BUSY_RESET 0x00000000U
#define FX1_ISP_AWB_RESULT_STATUS_WRITE_BUSY_ACCESS FX1_ISP_ACCESS_RO
/**
 * overrun [6:6], dòng bảng 428, kiểu truy cập RO
 * Báo cáo chốt do hết bank, hình học sai hoặc tràn hàng context đầu vào. Không phát ngắt mới; xóa
 * bằng CLEAR_REPORT.
 */
#define FX1_ISP_AWB_RESULT_STATUS_OVERRUN_SHIFT 6U
#define FX1_ISP_AWB_RESULT_STATUS_OVERRUN_WIDTH 1U
#define FX1_ISP_AWB_RESULT_STATUS_OVERRUN_MASK 0x00000040U
#define FX1_ISP_AWB_RESULT_STATUS_OVERRUN_RESET 0x00000000U
#define FX1_ISP_AWB_RESULT_STATUS_OVERRUN_ACCESS FX1_ISP_ACCESS_RO
/**
 * read_num_zone_x [14:8], dòng bảng 429, kiểu truy cập RO
 * Số zone ngang chốt cùng bank đang giữ quyền; chưa giữ đọc 0.
 */
#define FX1_ISP_AWB_RESULT_STATUS_READ_NUM_ZONE_X_SHIFT 8U
#define FX1_ISP_AWB_RESULT_STATUS_READ_NUM_ZONE_X_WIDTH 7U
#define FX1_ISP_AWB_RESULT_STATUS_READ_NUM_ZONE_X_MASK 0x00007F00U
#define FX1_ISP_AWB_RESULT_STATUS_READ_NUM_ZONE_X_RESET 0x00000000U
#define FX1_ISP_AWB_RESULT_STATUS_READ_NUM_ZONE_X_ACCESS FX1_ISP_ACCESS_RO
/**
 * read_num_zone_y [21:16], dòng bảng 430, kiểu truy cập RO
 * Số zone dọc chốt cùng bank đang giữ quyền; chưa giữ đọc 0.
 */
#define FX1_ISP_AWB_RESULT_STATUS_READ_NUM_ZONE_Y_SHIFT 16U
#define FX1_ISP_AWB_RESULT_STATUS_READ_NUM_ZONE_Y_WIDTH 6U
#define FX1_ISP_AWB_RESULT_STATUS_READ_NUM_ZONE_Y_MASK 0x003F0000U
#define FX1_ISP_AWB_RESULT_STATUS_READ_NUM_ZONE_Y_RESET 0x00000000U
#define FX1_ISP_AWB_RESULT_STATUS_READ_NUM_ZONE_Y_ACCESS FX1_ISP_ACCESS_RO

/**
 * AWB_DROP_COUNT: địa chỉ lệch 0x2470, dòng bảng 431, kiểu truy cập RO
 * Số phép đo AWB đang bật bị mất. Frame tắt không tăng; mỗi phép đo mất chỉ đếm một lần, các mất độc
 * lập cùng lúc đều được tính.
 */
#define FX1_ISP_AWB_DROP_COUNT_OFFSET 0x2470U
#define REG_AWB_DROP_COUNT FX1_ISP_AWB_DROP_COUNT_OFFSET
#define FX1_ISP_AWB_DROP_COUNT_RESET 0x00000000U
#define FX1_ISP_AWB_DROP_COUNT_ACCESS FX1_ISP_ACCESS_RO
#define FX1_ISP_AWB_DROP_COUNT_RO_MASK 0xFFFFFFFFU
#define FX1_ISP_AWB_DROP_COUNT_RW_MASK 0x00000000U
#define FX1_ISP_AWB_DROP_COUNT_W1C_MASK 0x00000000U
#define FX1_ISP_AWB_DROP_COUNT_W1S_MASK 0x00000000U
#define FX1_ISP_AWB_DROP_COUNT_W1P_MASK 0x00000000U
#define FX1_ISP_AWB_DROP_COUNT_W1SC_MASK 0x00000000U
/**
 * drop_count [31:0], dòng bảng 432, kiểu truy cập RO
 * Bộ đếm 32 bit kẹp 0xFFFFFFFF. Hard/common soft reset hoặc CLEAR_REPORT xóa; reset riêng DMA giữ
 * báo cáo.
 */
#define FX1_ISP_AWB_DROP_COUNT_DROP_COUNT_SHIFT 0U
#define FX1_ISP_AWB_DROP_COUNT_DROP_COUNT_WIDTH 32U
#define FX1_ISP_AWB_DROP_COUNT_DROP_COUNT_MASK 0xFFFFFFFFU
#define FX1_ISP_AWB_DROP_COUNT_DROP_COUNT_RESET 0x00000000U
#define FX1_ISP_AWB_DROP_COUNT_DROP_COUNT_ACCESS FX1_ISP_ACCESS_RO

/**
 * AF_CTRL: địa chỉ lệch 0x2600, dòng bảng 433, kiểu truy cập Mixed
 * AF chỉ thống kê điểm nét theo zone; hiện thực bit enable. Các trường search/VCM phía dưới chưa có
 * phần cứng xử lý.
 */
#define FX1_ISP_AF_CTRL_OFFSET 0x2600U
#define REG_AF_CTRL FX1_ISP_AF_CTRL_OFFSET
#define FX1_ISP_AF_CTRL_RESET 0x00000000U
#define FX1_ISP_AF_CTRL_ACCESS FX1_ISP_ACCESS_MIXED
#define FX1_ISP_AF_CTRL_RO_MASK 0x00000000U
#define FX1_ISP_AF_CTRL_RW_MASK 0x000001E7U
#define FX1_ISP_AF_CTRL_W1C_MASK 0x00000000U
#define FX1_ISP_AF_CTRL_W1S_MASK 0x00000018U
#define FX1_ISP_AF_CTRL_W1P_MASK 0x00000000U
#define FX1_ISP_AF_CTRL_W1SC_MASK 0x00000000U
/**
 * en [0:0], dòng bảng 434, kiểu truy cập RW
 * Bật thống kê AF: 1 tích lũy điểm nét, 0 rỗi.
 */
#define FX1_ISP_AF_CTRL_EN_SHIFT 0U
#define FX1_ISP_AF_CTRL_EN_WIDTH 1U
#define FX1_ISP_AF_CTRL_EN_MASK 0x00000001U
#define FX1_ISP_AF_CTRL_EN_RESET 0x00000000U
#define FX1_ISP_AF_CTRL_EN_ACCESS FX1_ISP_ACCESS_RW
/**
 * bypass [1:1], dòng bảng 435, kiểu truy cập RW
 * AF bypass chưa triển khai; dùng bit EN.
 */
#define FX1_ISP_AF_CTRL_BYPASS_SHIFT 1U
#define FX1_ISP_AF_CTRL_BYPASS_WIDTH 1U
#define FX1_ISP_AF_CTRL_BYPASS_MASK 0x00000002U
#define FX1_ISP_AF_CTRL_BYPASS_RESET 0x00000000U
#define FX1_ISP_AF_CTRL_BYPASS_ACCESS FX1_ISP_ACCESS_RW
/**
 * reg_update [2:2], dòng bảng 436, kiểu truy cập RW
 * Trường AF dự phòng/chưa triển khai, không tác động xử lý trong đặc tả này.
 */
#define FX1_ISP_AF_CTRL_REG_UPDATE_SHIFT 2U
#define FX1_ISP_AF_CTRL_REG_UPDATE_WIDTH 1U
#define FX1_ISP_AF_CTRL_REG_UPDATE_MASK 0x00000004U
#define FX1_ISP_AF_CTRL_REG_UPDATE_RESET 0x00000000U
#define FX1_ISP_AF_CTRL_REG_UPDATE_ACCESS FX1_ISP_ACCESS_RW
/**
 * search_start [3:3], dòng bảng 437, kiểu truy cập W1S
 * Lệnh tìm nét chưa triển khai, không có động cơ tìm kiếm phần cứng.
 */
#define FX1_ISP_AF_CTRL_SEARCH_START_SHIFT 3U
#define FX1_ISP_AF_CTRL_SEARCH_START_WIDTH 1U
#define FX1_ISP_AF_CTRL_SEARCH_START_MASK 0x00000008U
#define FX1_ISP_AF_CTRL_SEARCH_START_RESET 0x00000000U
#define FX1_ISP_AF_CTRL_SEARCH_START_ACCESS FX1_ISP_ACCESS_W1S
/**
 * search_abort [4:4], dòng bảng 438, kiểu truy cập W1S
 * Trường AF dự phòng/chưa triển khai, không tác động xử lý trong đặc tả này.
 */
#define FX1_ISP_AF_CTRL_SEARCH_ABORT_SHIFT 4U
#define FX1_ISP_AF_CTRL_SEARCH_ABORT_WIDTH 1U
#define FX1_ISP_AF_CTRL_SEARCH_ABORT_MASK 0x00000010U
#define FX1_ISP_AF_CTRL_SEARCH_ABORT_RESET 0x00000000U
#define FX1_ISP_AF_CTRL_SEARCH_ABORT_ACCESS FX1_ISP_ACCESS_W1S
/**
 * lut_en [5:5], dòng bảng 439, kiểu truy cập RW
 * Trường AF dự phòng/chưa triển khai, không tác động xử lý trong đặc tả này.
 */
#define FX1_ISP_AF_CTRL_LUT_EN_SHIFT 5U
#define FX1_ISP_AF_CTRL_LUT_EN_WIDTH 1U
#define FX1_ISP_AF_CTRL_LUT_EN_MASK 0x00000020U
#define FX1_ISP_AF_CTRL_LUT_EN_RESET 0x00000000U
#define FX1_ISP_AF_CTRL_LUT_EN_ACCESS FX1_ISP_ACCESS_RW
/**
 * metric_sel [7:6], dòng bảng 440, kiểu truy cập RW
 * Chọn metric chưa triển khai; khối chỉ tính một loại điểm nét cố định.
 */
#define FX1_ISP_AF_CTRL_METRIC_SEL_SHIFT 6U
#define FX1_ISP_AF_CTRL_METRIC_SEL_WIDTH 2U
#define FX1_ISP_AF_CTRL_METRIC_SEL_MASK 0x000000C0U
#define FX1_ISP_AF_CTRL_METRIC_SEL_RESET 0x00000000U
#define FX1_ISP_AF_CTRL_METRIC_SEL_ACCESS FX1_ISP_ACCESS_RW
/**
 * vcm_ack [8:8], dòng bảng 441, kiểu truy cập RW
 * Trường AF dự phòng/chưa triển khai, không tác động xử lý trong đặc tả này.
 */
#define FX1_ISP_AF_CTRL_VCM_ACK_SHIFT 8U
#define FX1_ISP_AF_CTRL_VCM_ACK_WIDTH 1U
#define FX1_ISP_AF_CTRL_VCM_ACK_MASK 0x00000100U
#define FX1_ISP_AF_CTRL_VCM_ACK_RESET 0x00000000U
#define FX1_ISP_AF_CTRL_VCM_ACK_ACCESS FX1_ISP_ACCESS_RW

/**
 * AF_ZONE_EN: địa chỉ lệch 0x2604, dòng bảng 442, kiểu truy cập RW
 * Mặt nạ zone AF chưa triển khai; ảnh chia lưới 4x4 và cả 16 zone luôn được đo.
 */
#define FX1_ISP_AF_ZONE_EN_OFFSET 0x2604U
#define REG_AF_ZONE_EN FX1_ISP_AF_ZONE_EN_OFFSET
#define FX1_ISP_AF_ZONE_EN_RESET 0x0000FFFFU
#define FX1_ISP_AF_ZONE_EN_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_AF_ZONE_EN_RO_MASK 0x00000000U
#define FX1_ISP_AF_ZONE_EN_RW_MASK 0x0000FFFFU
#define FX1_ISP_AF_ZONE_EN_W1C_MASK 0x00000000U
#define FX1_ISP_AF_ZONE_EN_W1S_MASK 0x00000000U
#define FX1_ISP_AF_ZONE_EN_W1P_MASK 0x00000000U
#define FX1_ISP_AF_ZONE_EN_W1SC_MASK 0x00000000U
/**
 * zone_en [15:0], dòng bảng 443, kiểu truy cập RW
 * Mặt nạ bật zone không có tác dụng phần cứng. Mỗi bit ứng zone 4x4 theo thứ tự ảnh: bit b có hàng
 * b/4, cột b%4, b=0..15. Sau reset bật đủ; thực tế mọi zone luôn đo.
 */
#define FX1_ISP_AF_ZONE_EN_ZONE_EN_SHIFT 0U
#define FX1_ISP_AF_ZONE_EN_ZONE_EN_WIDTH 16U
#define FX1_ISP_AF_ZONE_EN_ZONE_EN_MASK 0x0000FFFFU
#define FX1_ISP_AF_ZONE_EN_ZONE_EN_RESET 0x0000FFFFU
#define FX1_ISP_AF_ZONE_EN_ZONE_EN_ACCESS FX1_ISP_ACCESS_RW

/**
 * AF_METRIC_CFG: địa chỉ lệch 0x2608, dòng bảng 444, kiểu truy cập RW
 * Trường AF tìm kiếm/điều khiển ống kính dự phòng. Phần cứng chỉ đo điểm nét theo zone; di chuyển
 * ống kính và tìm vị trí là việc của phần mềm.
 */
#define FX1_ISP_AF_METRIC_CFG_OFFSET 0x2608U
#define REG_AF_METRIC_CFG FX1_ISP_AF_METRIC_CFG_OFFSET
#define FX1_ISP_AF_METRIC_CFG_RESET 0x00000000U
#define FX1_ISP_AF_METRIC_CFG_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_AF_METRIC_CFG_RO_MASK 0x00000000U
#define FX1_ISP_AF_METRIC_CFG_RW_MASK 0x00FFFFFFU
#define FX1_ISP_AF_METRIC_CFG_W1C_MASK 0x00000000U
#define FX1_ISP_AF_METRIC_CFG_W1S_MASK 0x00000000U
#define FX1_ISP_AF_METRIC_CFG_W1P_MASK 0x00000000U
#define FX1_ISP_AF_METRIC_CFG_W1SC_MASK 0x00000000U
/**
 * min_score [15:0], dòng bảng 445, kiểu truy cập RW
 * Trường AF dự phòng/chưa triển khai, không tác động xử lý trong đặc tả này.
 */
#define FX1_ISP_AF_METRIC_CFG_MIN_SCORE_SHIFT 0U
#define FX1_ISP_AF_METRIC_CFG_MIN_SCORE_WIDTH 16U
#define FX1_ISP_AF_METRIC_CFG_MIN_SCORE_MASK 0x0000FFFFU
#define FX1_ISP_AF_METRIC_CFG_MIN_SCORE_RESET 0x00000000U
#define FX1_ISP_AF_METRIC_CFG_MIN_SCORE_ACCESS FX1_ISP_ACCESS_RW
/**
 * settle_frames [23:16], dòng bảng 446, kiểu truy cập RW
 * Trường AF dự phòng/chưa triển khai, không tác động xử lý trong đặc tả này.
 */
#define FX1_ISP_AF_METRIC_CFG_SETTLE_FRAMES_SHIFT 16U
#define FX1_ISP_AF_METRIC_CFG_SETTLE_FRAMES_WIDTH 8U
#define FX1_ISP_AF_METRIC_CFG_SETTLE_FRAMES_MASK 0x00FF0000U
#define FX1_ISP_AF_METRIC_CFG_SETTLE_FRAMES_RESET 0x00000000U
#define FX1_ISP_AF_METRIC_CFG_SETTLE_FRAMES_ACCESS FX1_ISP_ACCESS_RW

/**
 * AF_STAT_ADDR: địa chỉ lệch 0x260C, dòng bảng 447, kiểu truy cập RW
 * Chọn zone mà AF_STAT_DATA trả điểm nét.
 */
#define FX1_ISP_AF_STAT_ADDR_OFFSET 0x260CU
#define REG_AF_STAT_ADDR FX1_ISP_AF_STAT_ADDR_OFFSET
#define FX1_ISP_AF_STAT_ADDR_RESET 0x00000000U
#define FX1_ISP_AF_STAT_ADDR_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_AF_STAT_ADDR_RO_MASK 0x00000000U
#define FX1_ISP_AF_STAT_ADDR_RW_MASK 0x0000000FU
#define FX1_ISP_AF_STAT_ADDR_W1C_MASK 0x00000000U
#define FX1_ISP_AF_STAT_ADDR_W1S_MASK 0x00000000U
#define FX1_ISP_AF_STAT_ADDR_W1P_MASK 0x00000000U
#define FX1_ISP_AF_STAT_ADDR_W1SC_MASK 0x00000000U
/**
 * zone_addr [3:0], dòng bảng 448, kiểu truy cập RW
 * Chỉ số zone 0..15 theo thứ tự ảnh.
 */
#define FX1_ISP_AF_STAT_ADDR_ZONE_ADDR_SHIFT 0U
#define FX1_ISP_AF_STAT_ADDR_ZONE_ADDR_WIDTH 4U
#define FX1_ISP_AF_STAT_ADDR_ZONE_ADDR_MASK 0x0000000FU
#define FX1_ISP_AF_STAT_ADDR_ZONE_ADDR_RESET 0x00000000U
#define FX1_ISP_AF_STAT_ADDR_ZONE_ADDR_ACCESS FX1_ISP_ACCESS_RW

/**
 * AF_STAT_DATA: địa chỉ lệch 0x2610, dòng bảng 449, kiểu truy cập RO
 * AF_STAT_DATA trả 32 bit thấp của tổng điểm nét bank đang giữ quyền. Dùng cổng 64 bit để lấy đầy
 * đủ; điểm lớn cho thấy nhiều chi tiết cục bộ hơn, không phải khoảng cách lấy nét tuyệt đối.
 */
#define FX1_ISP_AF_STAT_DATA_OFFSET 0x2610U
#define REG_AF_STAT_DATA FX1_ISP_AF_STAT_DATA_OFFSET
#define FX1_ISP_AF_STAT_DATA_RESET 0x00000000U
#define FX1_ISP_AF_STAT_DATA_ACCESS FX1_ISP_ACCESS_RO
#define FX1_ISP_AF_STAT_DATA_RO_MASK 0xFFFFFFFFU
#define FX1_ISP_AF_STAT_DATA_RW_MASK 0x00000000U
#define FX1_ISP_AF_STAT_DATA_W1C_MASK 0x00000000U
#define FX1_ISP_AF_STAT_DATA_W1S_MASK 0x00000000U
#define FX1_ISP_AF_STAT_DATA_W1P_MASK 0x00000000U
#define FX1_ISP_AF_STAT_DATA_W1SC_MASK 0x00000000U
/**
 * focus_score [31:0], dòng bảng 450, kiểu truy cập RO
 * 32 bit thấp tổng điểm nét 64 bit tại AF_STAT_ADDR. ACQUIRE trước đọc, RELEASE sau sao chép; chưa
 * giữ quyền trả 0 và đặt NO_LEASE.
 */
#define FX1_ISP_AF_STAT_DATA_FOCUS_SCORE_SHIFT 0U
#define FX1_ISP_AF_STAT_DATA_FOCUS_SCORE_WIDTH 32U
#define FX1_ISP_AF_STAT_DATA_FOCUS_SCORE_MASK 0xFFFFFFFFU
#define FX1_ISP_AF_STAT_DATA_FOCUS_SCORE_RESET 0x00000000U
#define FX1_ISP_AF_STAT_DATA_FOCUS_SCORE_ACCESS FX1_ISP_ACCESS_RO

/**
 * AF_LUT_ADDR: địa chỉ lệch 0x2614, dòng bảng 451, kiểu truy cập RW
 * Trường AF tìm kiếm/điều khiển ống kính dự phòng. Phần cứng chỉ đo điểm nét theo zone; di chuyển
 * ống kính và tìm vị trí là việc của phần mềm.
 */
#define FX1_ISP_AF_LUT_ADDR_OFFSET 0x2614U
#define REG_AF_LUT_ADDR FX1_ISP_AF_LUT_ADDR_OFFSET
#define FX1_ISP_AF_LUT_ADDR_RESET 0x00000000U
#define FX1_ISP_AF_LUT_ADDR_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_AF_LUT_ADDR_RO_MASK 0x00000000U
#define FX1_ISP_AF_LUT_ADDR_RW_MASK 0x000000FFU
#define FX1_ISP_AF_LUT_ADDR_W1C_MASK 0x00000000U
#define FX1_ISP_AF_LUT_ADDR_W1S_MASK 0x00000000U
#define FX1_ISP_AF_LUT_ADDR_W1P_MASK 0x00000000U
#define FX1_ISP_AF_LUT_ADDR_W1SC_MASK 0x00000000U
/**
 * lut_addr [7:0], dòng bảng 452, kiểu truy cập RW
 * Trường AF dự phòng/chưa triển khai, không tác động xử lý trong đặc tả này.
 */
#define FX1_ISP_AF_LUT_ADDR_LUT_ADDR_SHIFT 0U
#define FX1_ISP_AF_LUT_ADDR_LUT_ADDR_WIDTH 8U
#define FX1_ISP_AF_LUT_ADDR_LUT_ADDR_MASK 0x000000FFU
#define FX1_ISP_AF_LUT_ADDR_LUT_ADDR_RESET 0x00000000U
#define FX1_ISP_AF_LUT_ADDR_LUT_ADDR_ACCESS FX1_ISP_ACCESS_RW

/**
 * AF_LUT_DATA: địa chỉ lệch 0x2618, dòng bảng 453, kiểu truy cập RW
 * Trường AF tìm kiếm/điều khiển ống kính dự phòng. Phần cứng chỉ đo điểm nét theo zone; di chuyển
 * ống kính và tìm vị trí là việc của phần mềm.
 */
#define FX1_ISP_AF_LUT_DATA_OFFSET 0x2618U
#define REG_AF_LUT_DATA FX1_ISP_AF_LUT_DATA_OFFSET
#define FX1_ISP_AF_LUT_DATA_RESET 0x00000000U
#define FX1_ISP_AF_LUT_DATA_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_AF_LUT_DATA_RO_MASK 0x00000000U
#define FX1_ISP_AF_LUT_DATA_RW_MASK 0x0000FFFFU
#define FX1_ISP_AF_LUT_DATA_W1C_MASK 0x00000000U
#define FX1_ISP_AF_LUT_DATA_W1S_MASK 0x00000000U
#define FX1_ISP_AF_LUT_DATA_W1P_MASK 0x00000000U
#define FX1_ISP_AF_LUT_DATA_W1SC_MASK 0x00000000U
/**
 * vcm_position [15:0], dòng bảng 454, kiểu truy cập RW
 * Trường AF dự phòng/chưa triển khai, không tác động xử lý trong đặc tả này.
 */
#define FX1_ISP_AF_LUT_DATA_VCM_POSITION_SHIFT 0U
#define FX1_ISP_AF_LUT_DATA_VCM_POSITION_WIDTH 16U
#define FX1_ISP_AF_LUT_DATA_VCM_POSITION_MASK 0x0000FFFFU
#define FX1_ISP_AF_LUT_DATA_VCM_POSITION_RESET 0x00000000U
#define FX1_ISP_AF_LUT_DATA_VCM_POSITION_ACCESS FX1_ISP_ACCESS_RW

/**
 * AF_VCM_CUR_POS: địa chỉ lệch 0x261C, dòng bảng 455, kiểu truy cập RW
 * Trường AF tìm kiếm/điều khiển ống kính dự phòng. Phần cứng chỉ đo điểm nét theo zone; di chuyển
 * ống kính và tìm vị trí là việc của phần mềm.
 */
#define FX1_ISP_AF_VCM_CUR_POS_OFFSET 0x261CU
#define REG_AF_VCM_CUR_POS FX1_ISP_AF_VCM_CUR_POS_OFFSET
#define FX1_ISP_AF_VCM_CUR_POS_RESET 0x00000000U
#define FX1_ISP_AF_VCM_CUR_POS_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_AF_VCM_CUR_POS_RO_MASK 0x00000000U
#define FX1_ISP_AF_VCM_CUR_POS_RW_MASK 0x0000FFFFU
#define FX1_ISP_AF_VCM_CUR_POS_W1C_MASK 0x00000000U
#define FX1_ISP_AF_VCM_CUR_POS_W1S_MASK 0x00000000U
#define FX1_ISP_AF_VCM_CUR_POS_W1P_MASK 0x00000000U
#define FX1_ISP_AF_VCM_CUR_POS_W1SC_MASK 0x00000000U
/**
 * vcm_cur_pos [15:0], dòng bảng 456, kiểu truy cập RW
 * Vị trí VCM hiện tại chưa triển khai, không tác động xử lý; phần mềm tự điều khiển VCM có thể dùng
 * làm nhãn nháp.
 */
#define FX1_ISP_AF_VCM_CUR_POS_VCM_CUR_POS_SHIFT 0U
#define FX1_ISP_AF_VCM_CUR_POS_VCM_CUR_POS_WIDTH 16U
#define FX1_ISP_AF_VCM_CUR_POS_VCM_CUR_POS_MASK 0x0000FFFFU
#define FX1_ISP_AF_VCM_CUR_POS_VCM_CUR_POS_RESET 0x00000000U
#define FX1_ISP_AF_VCM_CUR_POS_VCM_CUR_POS_ACCESS FX1_ISP_ACCESS_RW

/**
 * AF_VCM_BEST_POS: địa chỉ lệch 0x2620, dòng bảng 457, kiểu truy cập RO
 * Trường AF tìm kiếm/điều khiển ống kính dự phòng. Phần cứng chỉ đo điểm nét theo zone; di chuyển
 * ống kính và tìm vị trí là việc của phần mềm.
 */
#define FX1_ISP_AF_VCM_BEST_POS_OFFSET 0x2620U
#define REG_AF_VCM_BEST_POS FX1_ISP_AF_VCM_BEST_POS_OFFSET
#define FX1_ISP_AF_VCM_BEST_POS_RESET 0x00000000U
#define FX1_ISP_AF_VCM_BEST_POS_ACCESS FX1_ISP_ACCESS_RO
#define FX1_ISP_AF_VCM_BEST_POS_RO_MASK 0x0000FFFFU
#define FX1_ISP_AF_VCM_BEST_POS_RW_MASK 0x00000000U
#define FX1_ISP_AF_VCM_BEST_POS_W1C_MASK 0x00000000U
#define FX1_ISP_AF_VCM_BEST_POS_W1S_MASK 0x00000000U
#define FX1_ISP_AF_VCM_BEST_POS_W1P_MASK 0x00000000U
#define FX1_ISP_AF_VCM_BEST_POS_W1SC_MASK 0x00000000U
/**
 * vcm_best_pos [15:0], dòng bảng 458, kiểu truy cập RO
 * Trường AF dự phòng/chưa triển khai, không tác động xử lý trong đặc tả này.
 */
#define FX1_ISP_AF_VCM_BEST_POS_VCM_BEST_POS_SHIFT 0U
#define FX1_ISP_AF_VCM_BEST_POS_VCM_BEST_POS_WIDTH 16U
#define FX1_ISP_AF_VCM_BEST_POS_VCM_BEST_POS_MASK 0x0000FFFFU
#define FX1_ISP_AF_VCM_BEST_POS_VCM_BEST_POS_RESET 0x00000000U
#define FX1_ISP_AF_VCM_BEST_POS_VCM_BEST_POS_ACCESS FX1_ISP_ACCESS_RO

/**
 * AF_PEAK_SCORE: địa chỉ lệch 0x2624, dòng bảng 459, kiểu truy cập RO
 * Trường AF tìm kiếm/điều khiển ống kính dự phòng. Phần cứng chỉ đo điểm nét theo zone; di chuyển
 * ống kính và tìm vị trí là việc của phần mềm.
 */
#define FX1_ISP_AF_PEAK_SCORE_OFFSET 0x2624U
#define REG_AF_PEAK_SCORE FX1_ISP_AF_PEAK_SCORE_OFFSET
#define FX1_ISP_AF_PEAK_SCORE_RESET 0x00000000U
#define FX1_ISP_AF_PEAK_SCORE_ACCESS FX1_ISP_ACCESS_RO
#define FX1_ISP_AF_PEAK_SCORE_RO_MASK 0xFFFFFFFFU
#define FX1_ISP_AF_PEAK_SCORE_RW_MASK 0x00000000U
#define FX1_ISP_AF_PEAK_SCORE_W1C_MASK 0x00000000U
#define FX1_ISP_AF_PEAK_SCORE_W1S_MASK 0x00000000U
#define FX1_ISP_AF_PEAK_SCORE_W1P_MASK 0x00000000U
#define FX1_ISP_AF_PEAK_SCORE_W1SC_MASK 0x00000000U
/**
 * peak_score [31:0], dòng bảng 460, kiểu truy cập RO
 * Trường AF dự phòng/chưa triển khai, không tác động xử lý trong đặc tả này.
 */
#define FX1_ISP_AF_PEAK_SCORE_PEAK_SCORE_SHIFT 0U
#define FX1_ISP_AF_PEAK_SCORE_PEAK_SCORE_WIDTH 32U
#define FX1_ISP_AF_PEAK_SCORE_PEAK_SCORE_MASK 0xFFFFFFFFU
#define FX1_ISP_AF_PEAK_SCORE_PEAK_SCORE_RESET 0x00000000U
#define FX1_ISP_AF_PEAK_SCORE_PEAK_SCORE_ACCESS FX1_ISP_ACCESS_RO

/**
 * AF_FOCUS_RANGE: địa chỉ lệch 0x2628, dòng bảng 461, kiểu truy cập RW
 * Trường AF tìm kiếm/điều khiển ống kính dự phòng. Phần cứng chỉ đo điểm nét theo zone; di chuyển
 * ống kính và tìm vị trí là việc của phần mềm.
 */
#define FX1_ISP_AF_FOCUS_RANGE_OFFSET 0x2628U
#define REG_AF_FOCUS_RANGE FX1_ISP_AF_FOCUS_RANGE_OFFSET
#define FX1_ISP_AF_FOCUS_RANGE_RESET 0x00000000U
#define FX1_ISP_AF_FOCUS_RANGE_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_AF_FOCUS_RANGE_RO_MASK 0x00000000U
#define FX1_ISP_AF_FOCUS_RANGE_RW_MASK 0x00FFFFFFU
#define FX1_ISP_AF_FOCUS_RANGE_W1C_MASK 0x00000000U
#define FX1_ISP_AF_FOCUS_RANGE_W1S_MASK 0x00000000U
#define FX1_ISP_AF_FOCUS_RANGE_W1P_MASK 0x00000000U
#define FX1_ISP_AF_FOCUS_RANGE_W1SC_MASK 0x00000000U
/**
 * accept_delta [15:0], dòng bảng 462, kiểu truy cập RW
 * Trường AF dự phòng/chưa triển khai, không tác động xử lý trong đặc tả này.
 */
#define FX1_ISP_AF_FOCUS_RANGE_ACCEPT_DELTA_SHIFT 0U
#define FX1_ISP_AF_FOCUS_RANGE_ACCEPT_DELTA_WIDTH 16U
#define FX1_ISP_AF_FOCUS_RANGE_ACCEPT_DELTA_MASK 0x0000FFFFU
#define FX1_ISP_AF_FOCUS_RANGE_ACCEPT_DELTA_RESET 0x00000000U
#define FX1_ISP_AF_FOCUS_RANGE_ACCEPT_DELTA_ACCESS FX1_ISP_ACCESS_RW
/**
 * hof_range [23:16], dòng bảng 463, kiểu truy cập RW
 * Trường AF dự phòng/chưa triển khai, không tác động xử lý trong đặc tả này.
 */
#define FX1_ISP_AF_FOCUS_RANGE_HOF_RANGE_SHIFT 16U
#define FX1_ISP_AF_FOCUS_RANGE_HOF_RANGE_WIDTH 8U
#define FX1_ISP_AF_FOCUS_RANGE_HOF_RANGE_MASK 0x00FF0000U
#define FX1_ISP_AF_FOCUS_RANGE_HOF_RANGE_RESET 0x00000000U
#define FX1_ISP_AF_FOCUS_RANGE_HOF_RANGE_ACCESS FX1_ISP_ACCESS_RW

/**
 * AF_STATUS: địa chỉ lệch 0x262C, dòng bảng 464, kiểu truy cập Mixed
 * Trạng thái đo AF: BUSY là đang tích lũy; FRAME_DONE là sự kiện chốt W1C; SCORE_VALID cho biết điểm
 * đã công bố. Các trường tìm kiếm dự phòng.
 */
#define FX1_ISP_AF_STATUS_OFFSET 0x262CU
#define REG_AF_STATUS FX1_ISP_AF_STATUS_OFFSET
#define FX1_ISP_AF_STATUS_RESET 0x00000000U
#define FX1_ISP_AF_STATUS_ACCESS FX1_ISP_ACCESS_MIXED
#define FX1_ISP_AF_STATUS_RO_MASK 0x00000021U
#define FX1_ISP_AF_STATUS_RW_MASK 0x00000000U
#define FX1_ISP_AF_STATUS_W1C_MASK 0x0000000EU
#define FX1_ISP_AF_STATUS_W1S_MASK 0x00000000U
#define FX1_ISP_AF_STATUS_W1P_MASK 0x00000000U
#define FX1_ISP_AF_STATUS_W1SC_MASK 0x00000000U
/**
 * busy [0:0], dòng bảng 465, kiểu truy cập RO
 * AF BUSY=1 khi đang tích lũy một frame.
 */
#define FX1_ISP_AF_STATUS_BUSY_SHIFT 0U
#define FX1_ISP_AF_STATUS_BUSY_WIDTH 1U
#define FX1_ISP_AF_STATUS_BUSY_MASK 0x00000001U
#define FX1_ISP_AF_STATUS_BUSY_RESET 0x00000000U
#define FX1_ISP_AF_STATUS_BUSY_ACCESS FX1_ISP_ACCESS_RO
/**
 * frame_done [1:1], dòng bảng 466, kiểu truy cập W1C
 * Điểm nét của frame đã hoàn tất; ghi 1 để xóa sự kiện.
 */
#define FX1_ISP_AF_STATUS_FRAME_DONE_SHIFT 1U
#define FX1_ISP_AF_STATUS_FRAME_DONE_WIDTH 1U
#define FX1_ISP_AF_STATUS_FRAME_DONE_MASK 0x00000002U
#define FX1_ISP_AF_STATUS_FRAME_DONE_RESET 0x00000000U
#define FX1_ISP_AF_STATUS_FRAME_DONE_ACCESS FX1_ISP_ACCESS_W1C
/**
 * search_done [2:2], dòng bảng 467, kiểu truy cập W1C
 * Trường AF dự phòng/chưa triển khai, không tác động xử lý trong đặc tả này.
 */
#define FX1_ISP_AF_STATUS_SEARCH_DONE_SHIFT 2U
#define FX1_ISP_AF_STATUS_SEARCH_DONE_WIDTH 1U
#define FX1_ISP_AF_STATUS_SEARCH_DONE_MASK 0x00000004U
#define FX1_ISP_AF_STATUS_SEARCH_DONE_RESET 0x00000000U
#define FX1_ISP_AF_STATUS_SEARCH_DONE_ACCESS FX1_ISP_ACCESS_W1C
/**
 * vcm_pos_valid [3:3], dòng bảng 468, kiểu truy cập W1C
 * Trường AF dự phòng/chưa triển khai, không tác động xử lý trong đặc tả này.
 */
#define FX1_ISP_AF_STATUS_VCM_POS_VALID_SHIFT 3U
#define FX1_ISP_AF_STATUS_VCM_POS_VALID_WIDTH 1U
#define FX1_ISP_AF_STATUS_VCM_POS_VALID_MASK 0x00000008U
#define FX1_ISP_AF_STATUS_VCM_POS_VALID_RESET 0x00000000U
#define FX1_ISP_AF_STATUS_VCM_POS_VALID_ACCESS FX1_ISP_ACCESS_W1C
/**
 * score_valid [5:5], dòng bảng 469, kiểu truy cập RO
 * SCORE_VALID=1 khi điểm công bố thuộc một frame đã hoàn tất.
 */
#define FX1_ISP_AF_STATUS_SCORE_VALID_SHIFT 5U
#define FX1_ISP_AF_STATUS_SCORE_VALID_WIDTH 1U
#define FX1_ISP_AF_STATUS_SCORE_VALID_MASK 0x00000020U
#define FX1_ISP_AF_STATUS_SCORE_VALID_RESET 0x00000000U
#define FX1_ISP_AF_STATUS_SCORE_VALID_ACCESS FX1_ISP_ACCESS_RO

/**
 * AF_CONTEXT_ID: địa chỉ lệch 0x2630, dòng bảng 470, kiểu truy cập RW
 * Nhãn context vị trí ống kính phần mềm chốt cùng frame thống kê hoàn tất.
 */
#define FX1_ISP_AF_CONTEXT_ID_OFFSET 0x2630U
#define REG_AF_CONTEXT_ID FX1_ISP_AF_CONTEXT_ID_OFFSET
#define FX1_ISP_AF_CONTEXT_ID_RESET 0x00000000U
#define FX1_ISP_AF_CONTEXT_ID_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_AF_CONTEXT_ID_RO_MASK 0x00000000U
#define FX1_ISP_AF_CONTEXT_ID_RW_MASK 0xFFFFFFFFU
#define FX1_ISP_AF_CONTEXT_ID_W1C_MASK 0x00000000U
#define FX1_ISP_AF_CONTEXT_ID_W1S_MASK 0x00000000U
#define FX1_ISP_AF_CONTEXT_ID_W1P_MASK 0x00000000U
#define FX1_ISP_AF_CONTEXT_ID_W1SC_MASK 0x00000000U
/**
 * context_id [31:0], dòng bảng 471, kiểu truy cập RW
 * Nhãn context thống kê tùy ý, chốt với frame được đo tiếp theo. Thay đổi phơi sáng/ống kính có độ
 * trễ vài frame; ghi trước frame tương ứng và đọc AF_RESULT_CONTEXT_ID cùng thống kê để đối chiếu
 * yêu cầu.
 */
#define FX1_ISP_AF_CONTEXT_ID_CONTEXT_ID_SHIFT 0U
#define FX1_ISP_AF_CONTEXT_ID_CONTEXT_ID_WIDTH 32U
#define FX1_ISP_AF_CONTEXT_ID_CONTEXT_ID_MASK 0xFFFFFFFFU
#define FX1_ISP_AF_CONTEXT_ID_CONTEXT_ID_RESET 0x00000000U
#define FX1_ISP_AF_CONTEXT_ID_CONTEXT_ID_ACCESS FX1_ISP_ACCESS_RW

/**
 * AF_RESULT_CONTEXT_ID: địa chỉ lệch 0x2634, dòng bảng 472, kiểu truy cập RO
 * Nhãn context phần mềm AF của frame đang giữ quyền, ổn định đến RELEASE.
 */
#define FX1_ISP_AF_RESULT_CONTEXT_ID_OFFSET 0x2634U
#define REG_AF_RESULT_CONTEXT_ID FX1_ISP_AF_RESULT_CONTEXT_ID_OFFSET
#define FX1_ISP_AF_RESULT_CONTEXT_ID_RESET 0x00000000U
#define FX1_ISP_AF_RESULT_CONTEXT_ID_ACCESS FX1_ISP_ACCESS_RO
#define FX1_ISP_AF_RESULT_CONTEXT_ID_RO_MASK 0xFFFFFFFFU
#define FX1_ISP_AF_RESULT_CONTEXT_ID_RW_MASK 0x00000000U
#define FX1_ISP_AF_RESULT_CONTEXT_ID_W1C_MASK 0x00000000U
#define FX1_ISP_AF_RESULT_CONTEXT_ID_W1S_MASK 0x00000000U
#define FX1_ISP_AF_RESULT_CONTEXT_ID_W1P_MASK 0x00000000U
#define FX1_ISP_AF_RESULT_CONTEXT_ID_W1SC_MASK 0x00000000U
/**
 * context_id [31:0], dòng bảng 473, kiểu truy cập RO
 * AF_CONTEXT_ID chốt ở đầu vào của frame đang giữ quyền; ổn định đến RELEASE dù frame khác hoàn tất.
 * Chưa giữ quyền đọc 0 và đặt NO_LEASE.
 */
#define FX1_ISP_AF_RESULT_CONTEXT_ID_CONTEXT_ID_SHIFT 0U
#define FX1_ISP_AF_RESULT_CONTEXT_ID_CONTEXT_ID_WIDTH 32U
#define FX1_ISP_AF_RESULT_CONTEXT_ID_CONTEXT_ID_MASK 0xFFFFFFFFU
#define FX1_ISP_AF_RESULT_CONTEXT_ID_CONTEXT_ID_RESET 0x00000000U
#define FX1_ISP_AF_RESULT_CONTEXT_ID_CONTEXT_ID_ACCESS FX1_ISP_ACCESS_RO

/**
 * AF_FRAME_ID: địa chỉ lệch 0x2638, dòng bảng 474, kiểu truy cập RO
 * ID frame đầu vào của bank AF đang giữ quyền, chỉ đọc.
 */
#define FX1_ISP_AF_FRAME_ID_OFFSET 0x2638U
#define REG_AF_FRAME_ID FX1_ISP_AF_FRAME_ID_OFFSET
#define FX1_ISP_AF_FRAME_ID_RESET 0x00000000U
#define FX1_ISP_AF_FRAME_ID_ACCESS FX1_ISP_ACCESS_RO
#define FX1_ISP_AF_FRAME_ID_RO_MASK 0xFFFFFFFFU
#define FX1_ISP_AF_FRAME_ID_RW_MASK 0x00000000U
#define FX1_ISP_AF_FRAME_ID_W1C_MASK 0x00000000U
#define FX1_ISP_AF_FRAME_ID_W1S_MASK 0x00000000U
#define FX1_ISP_AF_FRAME_ID_W1P_MASK 0x00000000U
#define FX1_ISP_AF_FRAME_ID_W1SC_MASK 0x00000000U
/**
 * frame_id [31:0], dòng bảng 475, kiểu truy cập RO
 * Số frame dùng chung tại SOF đầu vào. Frame đầu sau hard reset là 1; cuộn vòng modulo 2^32.
 * Common/DMA soft reset giữ bộ đếm. Thông tin ổn định đến RELEASE; chưa giữ quyền thì đọc 0 và báo
 * lỗi sở hữu.
 */
#define FX1_ISP_AF_FRAME_ID_FRAME_ID_SHIFT 0U
#define FX1_ISP_AF_FRAME_ID_FRAME_ID_WIDTH 32U
#define FX1_ISP_AF_FRAME_ID_FRAME_ID_MASK 0xFFFFFFFFU
#define FX1_ISP_AF_FRAME_ID_FRAME_ID_RESET 0x00000000U
#define FX1_ISP_AF_FRAME_ID_FRAME_ID_ACCESS FX1_ISP_ACCESS_RO

/**
 * OFMT_CTRL: địa chỉ lệch 0x2800, dòng bảng 476, kiểu truy cập RW
 * Điều khiển Output Formatter. Ở lõi tích hợp, đầu vào padding nối 0:
 * STRIDE_EN/OFMT_Y_STRIDE/OFMT_UV_STRIDE chỉ lưu giá trị; chỉ ODMA áp stride dòng.
 */
#define FX1_ISP_OFMT_CTRL_OFFSET 0x2800U
#define REG_OFMT_CTRL FX1_ISP_OFMT_CTRL_OFFSET
#define FX1_ISP_OFMT_CTRL_RESET 0x00000000U
#define FX1_ISP_OFMT_CTRL_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_OFMT_CTRL_RO_MASK 0x00000000U
#define FX1_ISP_OFMT_CTRL_RW_MASK 0x00000001U
#define FX1_ISP_OFMT_CTRL_W1C_MASK 0x00000000U
#define FX1_ISP_OFMT_CTRL_W1S_MASK 0x00000000U
#define FX1_ISP_OFMT_CTRL_W1P_MASK 0x00000000U
#define FX1_ISP_OFMT_CTRL_W1SC_MASK 0x00000000U
/**
 * stride_en [0:0], dòng bảng 477, kiểu truy cập RW
 * Bật padding stride chỉ lưu ở lõi tích hợp; formatter phát dòng đóng gói sát, ODMA áp stride.
 * Padding tới OFMT_Y/UV_STRIDE là khả năng formatter độc lập.
 */
#define FX1_ISP_OFMT_CTRL_STRIDE_EN_SHIFT 0U
#define FX1_ISP_OFMT_CTRL_STRIDE_EN_WIDTH 1U
#define FX1_ISP_OFMT_CTRL_STRIDE_EN_MASK 0x00000001U
#define FX1_ISP_OFMT_CTRL_STRIDE_EN_RESET 0x00000000U
#define FX1_ISP_OFMT_CTRL_STRIDE_EN_ACCESS FX1_ISP_ACCESS_RW

/**
 * OFMT_Y_STRIDE: địa chỉ lệch 0x2804, dòng bảng 478, kiểu truy cập RW
 * Stride dòng Y theo byte của formatter; ở lõi tích hợp chỉ lưu, xem OFMT_CTRL.
 */
#define FX1_ISP_OFMT_Y_STRIDE_OFFSET 0x2804U
#define REG_OFMT_Y_STRIDE FX1_ISP_OFMT_Y_STRIDE_OFFSET
#define FX1_ISP_OFMT_Y_STRIDE_RESET 0x00000F00U
#define FX1_ISP_OFMT_Y_STRIDE_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_OFMT_Y_STRIDE_RO_MASK 0x00000000U
#define FX1_ISP_OFMT_Y_STRIDE_RW_MASK 0x0000FFFFU
#define FX1_ISP_OFMT_Y_STRIDE_W1C_MASK 0x00000000U
#define FX1_ISP_OFMT_Y_STRIDE_W1S_MASK 0x00000000U
#define FX1_ISP_OFMT_Y_STRIDE_W1P_MASK 0x00000000U
#define FX1_ISP_OFMT_Y_STRIDE_W1SC_MASK 0x00000000U
/**
 * y_stride [15:0], dòng bảng 479, kiểu truy cập RW
 * Số byte giữa hai dòng luma, một byte/pixel. Giá trị <= chiều rộng hữu dụng không chèn padding.
 */
#define FX1_ISP_OFMT_Y_STRIDE_Y_STRIDE_SHIFT 0U
#define FX1_ISP_OFMT_Y_STRIDE_Y_STRIDE_WIDTH 16U
#define FX1_ISP_OFMT_Y_STRIDE_Y_STRIDE_MASK 0x0000FFFFU
#define FX1_ISP_OFMT_Y_STRIDE_Y_STRIDE_RESET 0x00000F00U
#define FX1_ISP_OFMT_Y_STRIDE_Y_STRIDE_ACCESS FX1_ISP_ACCESS_RW

/**
 * OFMT_UV_STRIDE: địa chỉ lệch 0x2808, dòng bảng 480, kiểu truy cập RW
 * Stride dòng chroma theo byte của formatter; ở lõi tích hợp chỉ lưu, xem OFMT_CTRL.
 */
#define FX1_ISP_OFMT_UV_STRIDE_OFFSET 0x2808U
#define REG_OFMT_UV_STRIDE FX1_ISP_OFMT_UV_STRIDE_OFFSET
#define FX1_ISP_OFMT_UV_STRIDE_RESET 0x00000F00U
#define FX1_ISP_OFMT_UV_STRIDE_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_OFMT_UV_STRIDE_RO_MASK 0x00000000U
#define FX1_ISP_OFMT_UV_STRIDE_RW_MASK 0x0000FFFFU
#define FX1_ISP_OFMT_UV_STRIDE_W1C_MASK 0x00000000U
#define FX1_ISP_OFMT_UV_STRIDE_W1S_MASK 0x00000000U
#define FX1_ISP_OFMT_UV_STRIDE_W1P_MASK 0x00000000U
#define FX1_ISP_OFMT_UV_STRIDE_W1SC_MASK 0x00000000U
/**
 * uv_stride [15:0], dòng bảng 481, kiểu truy cập RW
 * Số byte giữa hai dòng chroma; padding theo nhịp 16 bit nên số byte lẻ làm tròn xuống.
 */
#define FX1_ISP_OFMT_UV_STRIDE_UV_STRIDE_SHIFT 0U
#define FX1_ISP_OFMT_UV_STRIDE_UV_STRIDE_WIDTH 16U
#define FX1_ISP_OFMT_UV_STRIDE_UV_STRIDE_MASK 0x0000FFFFU
#define FX1_ISP_OFMT_UV_STRIDE_UV_STRIDE_RESET 0x00000F00U
#define FX1_ISP_OFMT_UV_STRIDE_UV_STRIDE_ACCESS FX1_ISP_ACCESS_RW

/**
 * DMA_CTRL: địa chỉ lệch 0x3000, dòng bảng 482, kiểu truy cập Mixed
 * DMA tại 0x3000. Cấu hình/bật pipeline pixel trước khi cấp quyền/bật DMA để có hình học đầu ra hợp
 * lệ. Giao dịch DMA và CSR độc lập.
 */
#define FX1_ISP_DMA_CTRL_OFFSET 0x3000U
#define REG_DMA_CTRL FX1_ISP_DMA_CTRL_OFFSET
#define FX1_ISP_DMA_CTRL_RESET 0x00003F00U
#define FX1_ISP_DMA_CTRL_ACCESS FX1_ISP_ACCESS_MIXED
#define FX1_ISP_DMA_CTRL_RO_MASK 0x00000000U
#define FX1_ISP_DMA_CTRL_RW_MASK 0x0000FF03U
#define FX1_ISP_DMA_CTRL_W1C_MASK 0x00000000U
#define FX1_ISP_DMA_CTRL_W1S_MASK 0x00000000U
#define FX1_ISP_DMA_CTRL_W1P_MASK 0x00000000U
#define FX1_ISP_DMA_CTRL_W1SC_MASK 0x00000004U
/**
 * idma_en [0:0], dòng bảng 483, kiểu truy cập RW
 * Bật IDMA để đọc frame từ DDR.
 */
#define FX1_ISP_DMA_CTRL_IDMA_EN_SHIFT 0U
#define FX1_ISP_DMA_CTRL_IDMA_EN_WIDTH 1U
#define FX1_ISP_DMA_CTRL_IDMA_EN_MASK 0x00000001U
#define FX1_ISP_DMA_CTRL_IDMA_EN_RESET 0x00000000U
#define FX1_ISP_DMA_CTRL_IDMA_EN_ACCESS FX1_ISP_ACCESS_RW
/**
 * odma_en [1:1], dòng bảng 484, kiểu truy cập RW
 * Bật ODMA để ghi frame ra DDR.
 */
#define FX1_ISP_DMA_CTRL_ODMA_EN_SHIFT 1U
#define FX1_ISP_DMA_CTRL_ODMA_EN_WIDTH 1U
#define FX1_ISP_DMA_CTRL_ODMA_EN_MASK 0x00000002U
#define FX1_ISP_DMA_CTRL_ODMA_EN_RESET 0x00000000U
#define FX1_ISP_DMA_CTRL_ODMA_EN_ACCESS FX1_ISP_ACCESS_RW
/**
 * soft_reset [2:2], dòng bảng 485, kiểu truy cập W1SC
 * Reset riêng DMA yêu cầu khôi phục nhóm: ngừng nhận, xử lý hết hai AXI master/yêu cầu giữ/bộ tiêu
 * thụ nội bộ, rồi xung reset 15 chu kỳ; bit đọc 0. Giữ CSR, profile active đã SEAL, thống kê
 * READY/LEASED, DONE và thông tin đầu ra hoàn tất trước yêu cầu; giữ DMA_ERR/IRQ_STAT. Xóa quyền
 * VALID/FREE, đưa mọi vòng về bộ đệm 0. Thăm dò RECOVERY.BUSY/DONE, sao chép bản ghi hủy trước chạy
 * lại. Yêu cầu lúc bận bị PENDING. COMMON_CTRL.SOFT_RST có phạm vi rộng hơn, gồm pipeline/kết quả.
 */
#define FX1_ISP_DMA_CTRL_SOFT_RESET_SHIFT 2U
#define FX1_ISP_DMA_CTRL_SOFT_RESET_WIDTH 1U
#define FX1_ISP_DMA_CTRL_SOFT_RESET_MASK 0x00000004U
#define FX1_ISP_DMA_CTRL_SOFT_RESET_RESET 0x00000000U
#define FX1_ISP_DMA_CTRL_SOFT_RESET_ACCESS FX1_ISP_ACCESS_W1SC
/**
 * max_burst_m1 [15:8], dòng bảng 486, kiểu truy cập RW
 * Mã burst = số nhịp - 1, dải 0..255 cho 1..256 nhịp. Reset 63 nghĩa là 64 nhịp. MAX_BURST_BEATS
 * hiệu dụng là MAX_BURST_M1+1; burst còn tách tại cuối dòng/biên 4 KiB.
 */
#define FX1_ISP_DMA_CTRL_MAX_BURST_M1_SHIFT 8U
#define FX1_ISP_DMA_CTRL_MAX_BURST_M1_WIDTH 8U
#define FX1_ISP_DMA_CTRL_MAX_BURST_M1_MASK 0x0000FF00U
#define FX1_ISP_DMA_CTRL_MAX_BURST_M1_RESET 0x0000003FU
#define FX1_ISP_DMA_CTRL_MAX_BURST_M1_ACCESS FX1_ISP_ACCESS_RW

/**
 * DMA_STAT: địa chỉ lệch 0x3004, dòng bảng 487, kiểu truy cập RO
 * Trạng thái DMA chỉ đọc. Cả hai BUSY có thể bằng 0 khi treo lẫn khi rỗi; không coi 0 là bảo đảm
 * khỏe, cần kiểm tra bộ đếm frame tiếp tục tăng.
 */
#define FX1_ISP_DMA_STAT_OFFSET 0x3004U
#define REG_DMA_STAT FX1_ISP_DMA_STAT_OFFSET
#define FX1_ISP_DMA_STAT_RESET 0x00000000U
#define FX1_ISP_DMA_STAT_ACCESS FX1_ISP_ACCESS_RO
#define FX1_ISP_DMA_STAT_RO_MASK 0x00000003U
#define FX1_ISP_DMA_STAT_RW_MASK 0x00000000U
#define FX1_ISP_DMA_STAT_W1C_MASK 0x00000000U
#define FX1_ISP_DMA_STAT_W1S_MASK 0x00000000U
#define FX1_ISP_DMA_STAT_W1P_MASK 0x00000000U
#define FX1_ISP_DMA_STAT_W1SC_MASK 0x00000000U
/**
 * idma_busy [0:0], dòng bảng 488, kiểu truy cập RO
 * IDMA BUSY=1 khi còn giao dịch đọc chưa kết thúc.
 */
#define FX1_ISP_DMA_STAT_IDMA_BUSY_SHIFT 0U
#define FX1_ISP_DMA_STAT_IDMA_BUSY_WIDTH 1U
#define FX1_ISP_DMA_STAT_IDMA_BUSY_MASK 0x00000001U
#define FX1_ISP_DMA_STAT_IDMA_BUSY_RESET 0x00000000U
#define FX1_ISP_DMA_STAT_IDMA_BUSY_ACCESS FX1_ISP_ACCESS_RO
/**
 * odma_busy [1:1], dòng bảng 489, kiểu truy cập RO
 * ODMA BUSY=1 khi còn giao dịch ghi chưa kết thúc.
 */
#define FX1_ISP_DMA_STAT_ODMA_BUSY_SHIFT 1U
#define FX1_ISP_DMA_STAT_ODMA_BUSY_WIDTH 1U
#define FX1_ISP_DMA_STAT_ODMA_BUSY_MASK 0x00000002U
#define FX1_ISP_DMA_STAT_ODMA_BUSY_RESET 0x00000000U
#define FX1_ISP_DMA_STAT_ODMA_BUSY_ACCESS FX1_ISP_ACCESS_RO

/**
 * DMA_ERR: địa chỉ lệch 0x3008, dòng bảng 490, kiểu truy cập W1C
 * Lỗi DMA được chốt, ghi 1 để xóa để lỗi giữa frame giữ tới khi phần mềm kiểm tra.
 */
#define FX1_ISP_DMA_ERR_OFFSET 0x3008U
#define REG_DMA_ERR FX1_ISP_DMA_ERR_OFFSET
#define FX1_ISP_DMA_ERR_RESET 0x00000000U
#define FX1_ISP_DMA_ERR_ACCESS FX1_ISP_ACCESS_W1C
#define FX1_ISP_DMA_ERR_RO_MASK 0x00000000U
#define FX1_ISP_DMA_ERR_RW_MASK 0x00000000U
#define FX1_ISP_DMA_ERR_W1C_MASK 0x0000003FU
#define FX1_ISP_DMA_ERR_W1S_MASK 0x00000000U
#define FX1_ISP_DMA_ERR_W1P_MASK 0x00000000U
#define FX1_ISP_DMA_ERR_W1SC_MASK 0x00000000U
/**
 * err [5:0], dòng bảng 491, kiểu truy cập W1C
 * DMA_ERR W1C, lỗi mới thắng xóa. Recovery riêng DMA giữ lỗi; W1C/common recovery/hard reset mới
 * xóa. Bit 0 ALIGN: sai căn lề/stride/hình học, địa chỉ vượt độ rộng build hoặc byte cuối mặt phẳng
 * chạm 2^ADDR_W; kiểm tra ở đầu IDMA/khi ODMA nhận bộ đệm, bỏ dùng/xóa quyền nhưng không tiến vòng.
 * Bit 1 IDMA_UNDERRUN: IDMA bật/rỗi thiếu đầu hàng hợp lệ, chờ không mất dữ liệu. Bit 2
 * ODMA_OVERFLOW: thiếu bộ đệm ra, chặn ngược tại biên dòng. Bit 3 AXI_RD và bit 4 AXI_WR: lỗi phản
 * hồi cuối, tự kích hoạt recovery riêng DMA. Bit 5 dự phòng đọc 0.
 */
#define FX1_ISP_DMA_ERR_ERR_SHIFT 0U
#define FX1_ISP_DMA_ERR_ERR_WIDTH 6U
#define FX1_ISP_DMA_ERR_ERR_MASK 0x0000003FU
#define FX1_ISP_DMA_ERR_ERR_RESET 0x00000000U
#define FX1_ISP_DMA_ERR_ERR_ACCESS FX1_ISP_ACCESS_W1C

/**
 * DMA_IRQ_EN: địa chỉ lệch 0x300C, dòng bảng 492, kiểu truy cập RW
 * Mặt nạ ngắt DMA; chỉ chọn bit IRQ_STAT tác động chân ngắt, trạng thái vẫn đặt độc lập.
 */
#define FX1_ISP_DMA_IRQ_EN_OFFSET 0x300CU
#define REG_DMA_IRQ_EN FX1_ISP_DMA_IRQ_EN_OFFSET
#define FX1_ISP_DMA_IRQ_EN_RESET 0x00000000U
#define FX1_ISP_DMA_IRQ_EN_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_DMA_IRQ_EN_RO_MASK 0x00000000U
#define FX1_ISP_DMA_IRQ_EN_RW_MASK 0x0000007FU
#define FX1_ISP_DMA_IRQ_EN_W1C_MASK 0x00000000U
#define FX1_ISP_DMA_IRQ_EN_W1S_MASK 0x00000000U
#define FX1_ISP_DMA_IRQ_EN_W1P_MASK 0x00000000U
#define FX1_ISP_DMA_IRQ_EN_W1SC_MASK 0x00000000U
/**
 * irq_en [6:0], dòng bảng 493, kiểu truy cập RW
 * Mặt nạ chân ngắt, không che trạng thái chốt: bit 0 CFG_ERROR, 1 IDMA_START, 2 IDMA_DONE, 3
 * IDMA_UNDERRUN, 4 ODMA_DONE, 5 ODMA_OVERFLOW, 6 AXI_ERROR của một trong hai master.
 */
#define FX1_ISP_DMA_IRQ_EN_IRQ_EN_SHIFT 0U
#define FX1_ISP_DMA_IRQ_EN_IRQ_EN_WIDTH 7U
#define FX1_ISP_DMA_IRQ_EN_IRQ_EN_MASK 0x0000007FU
#define FX1_ISP_DMA_IRQ_EN_IRQ_EN_RESET 0x00000000U
#define FX1_ISP_DMA_IRQ_EN_IRQ_EN_ACCESS FX1_ISP_ACCESS_RW

/**
 * DMA_IRQ_STAT: địa chỉ lệch 0x3010, dòng bảng 494, kiểu truy cập W1C
 * Trạng thái ngắt DMA đã chốt; ghi 1 để xóa.
 */
#define FX1_ISP_DMA_IRQ_STAT_OFFSET 0x3010U
#define REG_DMA_IRQ_STAT FX1_ISP_DMA_IRQ_STAT_OFFSET
#define FX1_ISP_DMA_IRQ_STAT_RESET 0x00000000U
#define FX1_ISP_DMA_IRQ_STAT_ACCESS FX1_ISP_ACCESS_W1C
#define FX1_ISP_DMA_IRQ_STAT_RO_MASK 0x00000000U
#define FX1_ISP_DMA_IRQ_STAT_RW_MASK 0x00000000U
#define FX1_ISP_DMA_IRQ_STAT_W1C_MASK 0x0000007FU
#define FX1_ISP_DMA_IRQ_STAT_W1S_MASK 0x00000000U
#define FX1_ISP_DMA_IRQ_STAT_W1P_MASK 0x00000000U
#define FX1_ISP_DMA_IRQ_STAT_W1SC_MASK 0x00000000U
/**
 * irq [6:0], dòng bảng 495, kiểu truy cập W1C
 * IRQ_STAT W1C, sự kiện mới thắng xóa; recovery riêng DMA giữ, chỉ W1C/common recovery/hard reset
 * xóa. Bit 0 CFG_ERROR, 1 IDMA_START, 2 IDMA_DONE, 3 IDMA_UNDERRUN, 4 ODMA_DONE, 5 ODMA_OVERFLOW, 6
 * AXI_ERROR của một trong hai master.
 */
#define FX1_ISP_DMA_IRQ_STAT_IRQ_SHIFT 0U
#define FX1_ISP_DMA_IRQ_STAT_IRQ_WIDTH 7U
#define FX1_ISP_DMA_IRQ_STAT_IRQ_MASK 0x0000007FU
#define FX1_ISP_DMA_IRQ_STAT_IRQ_RESET 0x00000000U
#define FX1_ISP_DMA_IRQ_STAT_IRQ_ACCESS FX1_ISP_ACCESS_W1C

/**
 * IDMA_STRIDE: địa chỉ lệch 0x3020, dòng bảng 496, kiểu truy cập RW
 * Stride dòng đầu vào tính byte, ít nhất 2*input_width và căn theo số byte bus AXI. Địa chỉ nền cũng
 * căn theo bus; crop Bayer X là bội bus_bytes/2 pixel, Y chẵn.
 */
#define FX1_ISP_IDMA_STRIDE_OFFSET 0x3020U
#define REG_IDMA_STRIDE FX1_ISP_IDMA_STRIDE_OFFSET
#define FX1_ISP_IDMA_STRIDE_RESET 0x00001E00U
#define FX1_ISP_IDMA_STRIDE_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_IDMA_STRIDE_RO_MASK 0x00000000U
#define FX1_ISP_IDMA_STRIDE_RW_MASK 0xFFFFFFFFU
#define FX1_ISP_IDMA_STRIDE_W1C_MASK 0x00000000U
#define FX1_ISP_IDMA_STRIDE_W1S_MASK 0x00000000U
#define FX1_ISP_IDMA_STRIDE_W1P_MASK 0x00000000U
#define FX1_ISP_IDMA_STRIDE_W1SC_MASK 0x00000000U
/**
 * idma_stride [31:0], dòng bảng 497, kiểu truy cập RW
 * Số byte giữa hai dòng đầu vào. Reset 7680 byte, đủ dòng 3840 pixel x 2 byte; lớn hơn dữ liệu dòng
 * rộng 1920 sau reset.
 */
#define FX1_ISP_IDMA_STRIDE_IDMA_STRIDE_SHIFT 0U
#define FX1_ISP_IDMA_STRIDE_IDMA_STRIDE_WIDTH 32U
#define FX1_ISP_IDMA_STRIDE_IDMA_STRIDE_MASK 0xFFFFFFFFU
#define FX1_ISP_IDMA_STRIDE_IDMA_STRIDE_RESET 0x00001E00U
#define FX1_ISP_IDMA_STRIDE_IDMA_STRIDE_ACCESS FX1_ISP_ACCESS_RW

/**
 * IDMA_BUF_VALID: địa chỉ lệch 0x3024, dòng bảng 498, kiểu truy cập W1S
 * Quyền sở hữu bốn bộ đệm vào xoay vòng. Phần mềm đặt bit để giao bộ đệm đã nạp; phần cứng xóa sau
 * đọc xong, khi đó phần mềm mới được nạp lại.
 */
#define FX1_ISP_IDMA_BUF_VALID_OFFSET 0x3024U
#define REG_IDMA_BUF_VALID FX1_ISP_IDMA_BUF_VALID_OFFSET
#define FX1_ISP_IDMA_BUF_VALID_RESET 0x00000000U
#define FX1_ISP_IDMA_BUF_VALID_ACCESS FX1_ISP_ACCESS_W1S
#define FX1_ISP_IDMA_BUF_VALID_RO_MASK 0x00000000U
#define FX1_ISP_IDMA_BUF_VALID_RW_MASK 0x00000000U
#define FX1_ISP_IDMA_BUF_VALID_W1C_MASK 0x00000000U
#define FX1_ISP_IDMA_BUF_VALID_W1S_MASK 0x000000FFU
#define FX1_ISP_IDMA_BUF_VALID_W1P_MASK 0x00000000U
#define FX1_ISP_IDMA_BUF_VALID_W1SC_MASK 0x00000000U
/**
 * idma_buf_valid [7:0], dòng bảng 499, kiểu truy cập W1S
 * Cờ VALID từng bộ đệm: ghi 1 để giao dữ liệu cho IDMA; bit 0..3 ứng buffer 0..3, bit 7:4 không
 * dùng. Bit vượt số buffer lúc build đọc 0/bỏ ghi. Phần cứng xóa khi đọc hết; địa chỉ buffer 0 lấy
 * từ L0/H0, tương tự các buffer còn lại.
 */
#define FX1_ISP_IDMA_BUF_VALID_IDMA_BUF_VALID_SHIFT 0U
#define FX1_ISP_IDMA_BUF_VALID_IDMA_BUF_VALID_WIDTH 8U
#define FX1_ISP_IDMA_BUF_VALID_IDMA_BUF_VALID_MASK 0x000000FFU
#define FX1_ISP_IDMA_BUF_VALID_IDMA_BUF_VALID_RESET 0x00000000U
#define FX1_ISP_IDMA_BUF_VALID_IDMA_BUF_VALID_ACCESS FX1_ISP_ACCESS_W1S

/**
 * IDMA_FRAME_COUNT: địa chỉ lệch 0x3028, dòng bảng 500, kiểu truy cập RO
 * Số frame đã đọc từ reset, chỉ đọc. Chờ bộ đếm thay đổi để tránh bỏ lỡ mức trạng thái/xung ngắt.
 */
#define FX1_ISP_IDMA_FRAME_COUNT_OFFSET 0x3028U
#define REG_IDMA_FRAME_COUNT FX1_ISP_IDMA_FRAME_COUNT_OFFSET
#define FX1_ISP_IDMA_FRAME_COUNT_RESET 0x00000000U
#define FX1_ISP_IDMA_FRAME_COUNT_ACCESS FX1_ISP_ACCESS_RO
#define FX1_ISP_IDMA_FRAME_COUNT_RO_MASK 0xFFFFFFFFU
#define FX1_ISP_IDMA_FRAME_COUNT_RW_MASK 0x00000000U
#define FX1_ISP_IDMA_FRAME_COUNT_W1C_MASK 0x00000000U
#define FX1_ISP_IDMA_FRAME_COUNT_W1S_MASK 0x00000000U
#define FX1_ISP_IDMA_FRAME_COUNT_W1P_MASK 0x00000000U
#define FX1_ISP_IDMA_FRAME_COUNT_W1SC_MASK 0x00000000U
/**
 * idma_frame_count [31:0], dòng bảng 501, kiểu truy cập RO
 * Bộ đếm frame đầu vào, cuộn modulo 2^32.
 */
#define FX1_ISP_IDMA_FRAME_COUNT_IDMA_FRAME_COUNT_SHIFT 0U
#define FX1_ISP_IDMA_FRAME_COUNT_IDMA_FRAME_COUNT_WIDTH 32U
#define FX1_ISP_IDMA_FRAME_COUNT_IDMA_FRAME_COUNT_MASK 0xFFFFFFFFU
#define FX1_ISP_IDMA_FRAME_COUNT_IDMA_FRAME_COUNT_RESET 0x00000000U
#define FX1_ISP_IDMA_FRAME_COUNT_IDMA_FRAME_COUNT_ACCESS FX1_ISP_ACCESS_RO

/**
 * IDMA_BUF_ADDR_L0: địa chỉ lệch 0x3040, dòng bảng 502, kiểu truy cập RW
 * Địa chỉ nền DDR bộ đệm vào 0, từ thấp.
 */
#define FX1_ISP_IDMA_BUF_ADDR_L0_OFFSET 0x3040U
#define REG_IDMA_BUF_ADDR_L0 FX1_ISP_IDMA_BUF_ADDR_L0_OFFSET
#define FX1_ISP_IDMA_BUF_ADDR_L0_RESET 0x00000000U
#define FX1_ISP_IDMA_BUF_ADDR_L0_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_IDMA_BUF_ADDR_L0_RO_MASK 0x00000000U
#define FX1_ISP_IDMA_BUF_ADDR_L0_RW_MASK 0xFFFFFFFFU
#define FX1_ISP_IDMA_BUF_ADDR_L0_W1C_MASK 0x00000000U
#define FX1_ISP_IDMA_BUF_ADDR_L0_W1S_MASK 0x00000000U
#define FX1_ISP_IDMA_BUF_ADDR_L0_W1P_MASK 0x00000000U
#define FX1_ISP_IDMA_BUF_ADDR_L0_W1SC_MASK 0x00000000U
/**
 * idma_buf_addr0_lo [31:0], dòng bảng 503, kiểu truy cập RW
 * Các bit [31:0] địa chỉ pixel đầu bộ đệm vào 0. Dịch nền trong ảnh lớn kết hợp IDMA_STRIDE để biểu
 * diễn cửa sổ crop.
 */
#define FX1_ISP_IDMA_BUF_ADDR_L0_IDMA_BUF_ADDR0_LO_SHIFT 0U
#define FX1_ISP_IDMA_BUF_ADDR_L0_IDMA_BUF_ADDR0_LO_WIDTH 32U
#define FX1_ISP_IDMA_BUF_ADDR_L0_IDMA_BUF_ADDR0_LO_MASK 0xFFFFFFFFU
#define FX1_ISP_IDMA_BUF_ADDR_L0_IDMA_BUF_ADDR0_LO_RESET 0x00000000U
#define FX1_ISP_IDMA_BUF_ADDR_L0_IDMA_BUF_ADDR0_LO_ACCESS FX1_ISP_ACCESS_RW

/**
 * IDMA_BUF_ADDR_H0: địa chỉ lệch 0x3044, dòng bảng 504, kiểu truy cập RW
 * Địa chỉ nền DDR bộ đệm vào 0, từ cao.
 */
#define FX1_ISP_IDMA_BUF_ADDR_H0_OFFSET 0x3044U
#define REG_IDMA_BUF_ADDR_H0 FX1_ISP_IDMA_BUF_ADDR_H0_OFFSET
#define FX1_ISP_IDMA_BUF_ADDR_H0_RESET 0x00000000U
#define FX1_ISP_IDMA_BUF_ADDR_H0_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_IDMA_BUF_ADDR_H0_RO_MASK 0x00000000U
#define FX1_ISP_IDMA_BUF_ADDR_H0_RW_MASK 0xFFFFFFFFU
#define FX1_ISP_IDMA_BUF_ADDR_H0_W1C_MASK 0x00000000U
#define FX1_ISP_IDMA_BUF_ADDR_H0_W1S_MASK 0x00000000U
#define FX1_ISP_IDMA_BUF_ADDR_H0_W1P_MASK 0x00000000U
#define FX1_ISP_IDMA_BUF_ADDR_H0_W1SC_MASK 0x00000000U
/**
 * idma_buf_addr0_hi [31:0], dòng bảng 505, kiểu truy cập RW
 * Các bit [63:32] của địa chỉ DDR.
 */
#define FX1_ISP_IDMA_BUF_ADDR_H0_IDMA_BUF_ADDR0_HI_SHIFT 0U
#define FX1_ISP_IDMA_BUF_ADDR_H0_IDMA_BUF_ADDR0_HI_WIDTH 32U
#define FX1_ISP_IDMA_BUF_ADDR_H0_IDMA_BUF_ADDR0_HI_MASK 0xFFFFFFFFU
#define FX1_ISP_IDMA_BUF_ADDR_H0_IDMA_BUF_ADDR0_HI_RESET 0x00000000U
#define FX1_ISP_IDMA_BUF_ADDR_H0_IDMA_BUF_ADDR0_HI_ACCESS FX1_ISP_ACCESS_RW

/**
 * IDMA_BUF_ADDR_L1: địa chỉ lệch 0x3048, dòng bảng 506, kiểu truy cập RW
 * Địa chỉ nền DDR bộ đệm vào 1, từ thấp.
 */
#define FX1_ISP_IDMA_BUF_ADDR_L1_OFFSET 0x3048U
#define REG_IDMA_BUF_ADDR_L1 FX1_ISP_IDMA_BUF_ADDR_L1_OFFSET
#define FX1_ISP_IDMA_BUF_ADDR_L1_RESET 0x00000000U
#define FX1_ISP_IDMA_BUF_ADDR_L1_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_IDMA_BUF_ADDR_L1_RO_MASK 0x00000000U
#define FX1_ISP_IDMA_BUF_ADDR_L1_RW_MASK 0xFFFFFFFFU
#define FX1_ISP_IDMA_BUF_ADDR_L1_W1C_MASK 0x00000000U
#define FX1_ISP_IDMA_BUF_ADDR_L1_W1S_MASK 0x00000000U
#define FX1_ISP_IDMA_BUF_ADDR_L1_W1P_MASK 0x00000000U
#define FX1_ISP_IDMA_BUF_ADDR_L1_W1SC_MASK 0x00000000U
/**
 * idma_buf_addr1_lo [31:0], dòng bảng 507, kiểu truy cập RW
 * Các bit [31:0] địa chỉ pixel đầu bộ đệm vào 1. Dịch nền trong ảnh lớn kết hợp IDMA_STRIDE để biểu
 * diễn cửa sổ crop.
 */
#define FX1_ISP_IDMA_BUF_ADDR_L1_IDMA_BUF_ADDR1_LO_SHIFT 0U
#define FX1_ISP_IDMA_BUF_ADDR_L1_IDMA_BUF_ADDR1_LO_WIDTH 32U
#define FX1_ISP_IDMA_BUF_ADDR_L1_IDMA_BUF_ADDR1_LO_MASK 0xFFFFFFFFU
#define FX1_ISP_IDMA_BUF_ADDR_L1_IDMA_BUF_ADDR1_LO_RESET 0x00000000U
#define FX1_ISP_IDMA_BUF_ADDR_L1_IDMA_BUF_ADDR1_LO_ACCESS FX1_ISP_ACCESS_RW

/**
 * IDMA_BUF_ADDR_H1: địa chỉ lệch 0x304C, dòng bảng 508, kiểu truy cập RW
 * Địa chỉ nền DDR bộ đệm vào 1, từ cao.
 */
#define FX1_ISP_IDMA_BUF_ADDR_H1_OFFSET 0x304CU
#define REG_IDMA_BUF_ADDR_H1 FX1_ISP_IDMA_BUF_ADDR_H1_OFFSET
#define FX1_ISP_IDMA_BUF_ADDR_H1_RESET 0x00000000U
#define FX1_ISP_IDMA_BUF_ADDR_H1_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_IDMA_BUF_ADDR_H1_RO_MASK 0x00000000U
#define FX1_ISP_IDMA_BUF_ADDR_H1_RW_MASK 0xFFFFFFFFU
#define FX1_ISP_IDMA_BUF_ADDR_H1_W1C_MASK 0x00000000U
#define FX1_ISP_IDMA_BUF_ADDR_H1_W1S_MASK 0x00000000U
#define FX1_ISP_IDMA_BUF_ADDR_H1_W1P_MASK 0x00000000U
#define FX1_ISP_IDMA_BUF_ADDR_H1_W1SC_MASK 0x00000000U
/**
 * idma_buf_addr1_hi [31:0], dòng bảng 509, kiểu truy cập RW
 * Các bit [63:32] của địa chỉ DDR.
 */
#define FX1_ISP_IDMA_BUF_ADDR_H1_IDMA_BUF_ADDR1_HI_SHIFT 0U
#define FX1_ISP_IDMA_BUF_ADDR_H1_IDMA_BUF_ADDR1_HI_WIDTH 32U
#define FX1_ISP_IDMA_BUF_ADDR_H1_IDMA_BUF_ADDR1_HI_MASK 0xFFFFFFFFU
#define FX1_ISP_IDMA_BUF_ADDR_H1_IDMA_BUF_ADDR1_HI_RESET 0x00000000U
#define FX1_ISP_IDMA_BUF_ADDR_H1_IDMA_BUF_ADDR1_HI_ACCESS FX1_ISP_ACCESS_RW

/**
 * IDMA_BUF_ADDR_L2: địa chỉ lệch 0x3050, dòng bảng 510, kiểu truy cập RW
 * Địa chỉ nền DDR bộ đệm vào 2, từ thấp.
 */
#define FX1_ISP_IDMA_BUF_ADDR_L2_OFFSET 0x3050U
#define REG_IDMA_BUF_ADDR_L2 FX1_ISP_IDMA_BUF_ADDR_L2_OFFSET
#define FX1_ISP_IDMA_BUF_ADDR_L2_RESET 0x00000000U
#define FX1_ISP_IDMA_BUF_ADDR_L2_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_IDMA_BUF_ADDR_L2_RO_MASK 0x00000000U
#define FX1_ISP_IDMA_BUF_ADDR_L2_RW_MASK 0xFFFFFFFFU
#define FX1_ISP_IDMA_BUF_ADDR_L2_W1C_MASK 0x00000000U
#define FX1_ISP_IDMA_BUF_ADDR_L2_W1S_MASK 0x00000000U
#define FX1_ISP_IDMA_BUF_ADDR_L2_W1P_MASK 0x00000000U
#define FX1_ISP_IDMA_BUF_ADDR_L2_W1SC_MASK 0x00000000U
/**
 * idma_buf_addr2_lo [31:0], dòng bảng 511, kiểu truy cập RW
 * Các bit [31:0] địa chỉ pixel đầu bộ đệm vào 2. Dịch nền trong ảnh lớn kết hợp IDMA_STRIDE để biểu
 * diễn cửa sổ crop.
 */
#define FX1_ISP_IDMA_BUF_ADDR_L2_IDMA_BUF_ADDR2_LO_SHIFT 0U
#define FX1_ISP_IDMA_BUF_ADDR_L2_IDMA_BUF_ADDR2_LO_WIDTH 32U
#define FX1_ISP_IDMA_BUF_ADDR_L2_IDMA_BUF_ADDR2_LO_MASK 0xFFFFFFFFU
#define FX1_ISP_IDMA_BUF_ADDR_L2_IDMA_BUF_ADDR2_LO_RESET 0x00000000U
#define FX1_ISP_IDMA_BUF_ADDR_L2_IDMA_BUF_ADDR2_LO_ACCESS FX1_ISP_ACCESS_RW

/**
 * IDMA_BUF_ADDR_H2: địa chỉ lệch 0x3054, dòng bảng 512, kiểu truy cập RW
 * Địa chỉ nền DDR bộ đệm vào 2, từ cao.
 */
#define FX1_ISP_IDMA_BUF_ADDR_H2_OFFSET 0x3054U
#define REG_IDMA_BUF_ADDR_H2 FX1_ISP_IDMA_BUF_ADDR_H2_OFFSET
#define FX1_ISP_IDMA_BUF_ADDR_H2_RESET 0x00000000U
#define FX1_ISP_IDMA_BUF_ADDR_H2_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_IDMA_BUF_ADDR_H2_RO_MASK 0x00000000U
#define FX1_ISP_IDMA_BUF_ADDR_H2_RW_MASK 0xFFFFFFFFU
#define FX1_ISP_IDMA_BUF_ADDR_H2_W1C_MASK 0x00000000U
#define FX1_ISP_IDMA_BUF_ADDR_H2_W1S_MASK 0x00000000U
#define FX1_ISP_IDMA_BUF_ADDR_H2_W1P_MASK 0x00000000U
#define FX1_ISP_IDMA_BUF_ADDR_H2_W1SC_MASK 0x00000000U
/**
 * idma_buf_addr2_hi [31:0], dòng bảng 513, kiểu truy cập RW
 * Các bit [63:32] của địa chỉ DDR.
 */
#define FX1_ISP_IDMA_BUF_ADDR_H2_IDMA_BUF_ADDR2_HI_SHIFT 0U
#define FX1_ISP_IDMA_BUF_ADDR_H2_IDMA_BUF_ADDR2_HI_WIDTH 32U
#define FX1_ISP_IDMA_BUF_ADDR_H2_IDMA_BUF_ADDR2_HI_MASK 0xFFFFFFFFU
#define FX1_ISP_IDMA_BUF_ADDR_H2_IDMA_BUF_ADDR2_HI_RESET 0x00000000U
#define FX1_ISP_IDMA_BUF_ADDR_H2_IDMA_BUF_ADDR2_HI_ACCESS FX1_ISP_ACCESS_RW

/**
 * IDMA_BUF_ADDR_L3: địa chỉ lệch 0x3058, dòng bảng 514, kiểu truy cập RW
 * Địa chỉ nền DDR bộ đệm vào 3, từ thấp.
 */
#define FX1_ISP_IDMA_BUF_ADDR_L3_OFFSET 0x3058U
#define REG_IDMA_BUF_ADDR_L3 FX1_ISP_IDMA_BUF_ADDR_L3_OFFSET
#define FX1_ISP_IDMA_BUF_ADDR_L3_RESET 0x00000000U
#define FX1_ISP_IDMA_BUF_ADDR_L3_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_IDMA_BUF_ADDR_L3_RO_MASK 0x00000000U
#define FX1_ISP_IDMA_BUF_ADDR_L3_RW_MASK 0xFFFFFFFFU
#define FX1_ISP_IDMA_BUF_ADDR_L3_W1C_MASK 0x00000000U
#define FX1_ISP_IDMA_BUF_ADDR_L3_W1S_MASK 0x00000000U
#define FX1_ISP_IDMA_BUF_ADDR_L3_W1P_MASK 0x00000000U
#define FX1_ISP_IDMA_BUF_ADDR_L3_W1SC_MASK 0x00000000U
/**
 * idma_buf_addr3_lo [31:0], dòng bảng 515, kiểu truy cập RW
 * Các bit [31:0] địa chỉ pixel đầu bộ đệm vào 3. Dịch nền trong ảnh lớn kết hợp IDMA_STRIDE để biểu
 * diễn cửa sổ crop.
 */
#define FX1_ISP_IDMA_BUF_ADDR_L3_IDMA_BUF_ADDR3_LO_SHIFT 0U
#define FX1_ISP_IDMA_BUF_ADDR_L3_IDMA_BUF_ADDR3_LO_WIDTH 32U
#define FX1_ISP_IDMA_BUF_ADDR_L3_IDMA_BUF_ADDR3_LO_MASK 0xFFFFFFFFU
#define FX1_ISP_IDMA_BUF_ADDR_L3_IDMA_BUF_ADDR3_LO_RESET 0x00000000U
#define FX1_ISP_IDMA_BUF_ADDR_L3_IDMA_BUF_ADDR3_LO_ACCESS FX1_ISP_ACCESS_RW

/**
 * IDMA_BUF_ADDR_H3: địa chỉ lệch 0x305C, dòng bảng 516, kiểu truy cập RW
 * Địa chỉ nền DDR bộ đệm vào 3, từ cao.
 */
#define FX1_ISP_IDMA_BUF_ADDR_H3_OFFSET 0x305CU
#define REG_IDMA_BUF_ADDR_H3 FX1_ISP_IDMA_BUF_ADDR_H3_OFFSET
#define FX1_ISP_IDMA_BUF_ADDR_H3_RESET 0x00000000U
#define FX1_ISP_IDMA_BUF_ADDR_H3_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_IDMA_BUF_ADDR_H3_RO_MASK 0x00000000U
#define FX1_ISP_IDMA_BUF_ADDR_H3_RW_MASK 0xFFFFFFFFU
#define FX1_ISP_IDMA_BUF_ADDR_H3_W1C_MASK 0x00000000U
#define FX1_ISP_IDMA_BUF_ADDR_H3_W1S_MASK 0x00000000U
#define FX1_ISP_IDMA_BUF_ADDR_H3_W1P_MASK 0x00000000U
#define FX1_ISP_IDMA_BUF_ADDR_H3_W1SC_MASK 0x00000000U
/**
 * idma_buf_addr3_hi [31:0], dòng bảng 517, kiểu truy cập RW
 * Các bit [63:32] của địa chỉ DDR.
 */
#define FX1_ISP_IDMA_BUF_ADDR_H3_IDMA_BUF_ADDR3_HI_SHIFT 0U
#define FX1_ISP_IDMA_BUF_ADDR_H3_IDMA_BUF_ADDR3_HI_WIDTH 32U
#define FX1_ISP_IDMA_BUF_ADDR_H3_IDMA_BUF_ADDR3_HI_MASK 0xFFFFFFFFU
#define FX1_ISP_IDMA_BUF_ADDR_H3_IDMA_BUF_ADDR3_HI_RESET 0x00000000U
#define FX1_ISP_IDMA_BUF_ADDR_H3_IDMA_BUF_ADDR3_HI_ACCESS FX1_ISP_ACCESS_RW

/**
 * ODMA_Y_STRIDE: địa chỉ lệch 0x3080, dòng bảng 518, kiểu truy cập RW
 * Stride mặt phẳng Y đầu ra NV12; ảnh gồm Y đủ độ phân giải và UV xen kẽ có nửa số dòng.
 */
#define FX1_ISP_ODMA_Y_STRIDE_OFFSET 0x3080U
#define REG_ODMA_Y_STRIDE FX1_ISP_ODMA_Y_STRIDE_OFFSET
#define FX1_ISP_ODMA_Y_STRIDE_RESET 0x00000F00U
#define FX1_ISP_ODMA_Y_STRIDE_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_ODMA_Y_STRIDE_RO_MASK 0x00000000U
#define FX1_ISP_ODMA_Y_STRIDE_RW_MASK 0xFFFFFFFFU
#define FX1_ISP_ODMA_Y_STRIDE_W1C_MASK 0x00000000U
#define FX1_ISP_ODMA_Y_STRIDE_W1S_MASK 0x00000000U
#define FX1_ISP_ODMA_Y_STRIDE_W1P_MASK 0x00000000U
#define FX1_ISP_ODMA_Y_STRIDE_W1SC_MASK 0x00000000U
/**
 * y_stride [31:0], dòng bảng 519, kiểu truy cập RW
 * Số byte giữa hai dòng Y, một byte/pixel, làm tròn lên theo độ rộng bus.
 */
#define FX1_ISP_ODMA_Y_STRIDE_Y_STRIDE_SHIFT 0U
#define FX1_ISP_ODMA_Y_STRIDE_Y_STRIDE_WIDTH 32U
#define FX1_ISP_ODMA_Y_STRIDE_Y_STRIDE_MASK 0xFFFFFFFFU
#define FX1_ISP_ODMA_Y_STRIDE_Y_STRIDE_RESET 0x00000F00U
#define FX1_ISP_ODMA_Y_STRIDE_Y_STRIDE_ACCESS FX1_ISP_ACCESS_RW

/**
 * ODMA_UV_STRIDE: địa chỉ lệch 0x3084, dòng bảng 520, kiểu truy cập RW
 * Stride mặt phẳng UV: nửa số dòng Y nhưng cùng số byte mỗi dòng vì U/V xen kẽ ở nửa tần số ngang.
 */
#define FX1_ISP_ODMA_UV_STRIDE_OFFSET 0x3084U
#define REG_ODMA_UV_STRIDE FX1_ISP_ODMA_UV_STRIDE_OFFSET
#define FX1_ISP_ODMA_UV_STRIDE_RESET 0x00000F00U
#define FX1_ISP_ODMA_UV_STRIDE_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_ODMA_UV_STRIDE_RO_MASK 0x00000000U
#define FX1_ISP_ODMA_UV_STRIDE_RW_MASK 0xFFFFFFFFU
#define FX1_ISP_ODMA_UV_STRIDE_W1C_MASK 0x00000000U
#define FX1_ISP_ODMA_UV_STRIDE_W1S_MASK 0x00000000U
#define FX1_ISP_ODMA_UV_STRIDE_W1P_MASK 0x00000000U
#define FX1_ISP_ODMA_UV_STRIDE_W1SC_MASK 0x00000000U
/**
 * uv_stride [31:0], dòng bảng 521, kiểu truy cập RW
 * Số byte giữa hai dòng chroma.
 */
#define FX1_ISP_ODMA_UV_STRIDE_UV_STRIDE_SHIFT 0U
#define FX1_ISP_ODMA_UV_STRIDE_UV_STRIDE_WIDTH 32U
#define FX1_ISP_ODMA_UV_STRIDE_UV_STRIDE_MASK 0xFFFFFFFFU
#define FX1_ISP_ODMA_UV_STRIDE_UV_STRIDE_RESET 0x00000F00U
#define FX1_ISP_ODMA_UV_STRIDE_UV_STRIDE_ACCESS FX1_ISP_ACCESS_RW

/**
 * ODMA_BUF_FREE: địa chỉ lệch 0x3088, dòng bảng 522, kiểu truy cập W1S
 * Quyền bộ đệm ra: ghi FREE chỉ cấp sẵn, chưa chốt tham số. Khi frame ra kế tiếp bắt đầu, ODMA lấy
 * đầu vòng, chốt địa chỉ/stride/hình học frame và xóa FREE. DONE chỉ đặt sau cả hai mặt phẳng và mọi
 * phản hồi ghi đã kết thúc.
 */
#define FX1_ISP_ODMA_BUF_FREE_OFFSET 0x3088U
#define REG_ODMA_BUF_FREE FX1_ISP_ODMA_BUF_FREE_OFFSET
#define FX1_ISP_ODMA_BUF_FREE_RESET 0x00000000U
#define FX1_ISP_ODMA_BUF_FREE_ACCESS FX1_ISP_ACCESS_W1S
#define FX1_ISP_ODMA_BUF_FREE_RO_MASK 0x00000000U
#define FX1_ISP_ODMA_BUF_FREE_RW_MASK 0x00000000U
#define FX1_ISP_ODMA_BUF_FREE_W1C_MASK 0x00000000U
#define FX1_ISP_ODMA_BUF_FREE_W1S_MASK 0x000000FFU
#define FX1_ISP_ODMA_BUF_FREE_W1P_MASK 0x00000000U
#define FX1_ISP_ODMA_BUF_FREE_W1SC_MASK 0x00000000U
/**
 * odma_buf_free [7:0], dòng bảng 523, kiểu truy cập W1S
 * FREE bit 0..3 ứng bốn buffer, bit 7:4 không dùng; bit vượt số buffer build đọc 0/bỏ ghi. Ghi 1 để
 * cấp sẵn; phần cứng xóa khi nhận frame ra. Xác nhận DONE trước cấp lại FREE. Thiếu buffer làm
 * pipeline chờ và báo ODMA_OVERFLOW.
 */
#define FX1_ISP_ODMA_BUF_FREE_ODMA_BUF_FREE_SHIFT 0U
#define FX1_ISP_ODMA_BUF_FREE_ODMA_BUF_FREE_WIDTH 8U
#define FX1_ISP_ODMA_BUF_FREE_ODMA_BUF_FREE_MASK 0x000000FFU
#define FX1_ISP_ODMA_BUF_FREE_ODMA_BUF_FREE_RESET 0x00000000U
#define FX1_ISP_ODMA_BUF_FREE_ODMA_BUF_FREE_ACCESS FX1_ISP_ACCESS_W1S

/**
 * ODMA_BUF_DONE: địa chỉ lệch 0x308C, dòng bảng 524, kiểu truy cập W1C
 * DONE đầu ra W1C, chỉ đặt sau hai mặt phẳng và mọi phản hồi AXI ghi đã kết thúc. Xóa DONE trước trả
 * buffer qua FREE.
 */
#define FX1_ISP_ODMA_BUF_DONE_OFFSET 0x308CU
#define REG_ODMA_BUF_DONE FX1_ISP_ODMA_BUF_DONE_OFFSET
#define FX1_ISP_ODMA_BUF_DONE_RESET 0x00000000U
#define FX1_ISP_ODMA_BUF_DONE_ACCESS FX1_ISP_ACCESS_W1C
#define FX1_ISP_ODMA_BUF_DONE_RO_MASK 0x00000000U
#define FX1_ISP_ODMA_BUF_DONE_RW_MASK 0x00000000U
#define FX1_ISP_ODMA_BUF_DONE_W1C_MASK 0x000000FFU
#define FX1_ISP_ODMA_BUF_DONE_W1S_MASK 0x00000000U
#define FX1_ISP_ODMA_BUF_DONE_W1P_MASK 0x00000000U
#define FX1_ISP_ODMA_BUF_DONE_W1SC_MASK 0x00000000U
/**
 * odma_buf_done [7:0], dòng bảng 525, kiểu truy cập W1C
 * DONE bit 0..3 ứng buffer, 7:4 không dùng. Phần cứng đặt khi frame ra kết thúc đầy đủ; ghi 1 xác
 * nhận, rồi ghi FREE riêng để tái dùng. Hoàn tất mới thắng xóa đồng thời.
 */
#define FX1_ISP_ODMA_BUF_DONE_ODMA_BUF_DONE_SHIFT 0U
#define FX1_ISP_ODMA_BUF_DONE_ODMA_BUF_DONE_WIDTH 8U
#define FX1_ISP_ODMA_BUF_DONE_ODMA_BUF_DONE_MASK 0x000000FFU
#define FX1_ISP_ODMA_BUF_DONE_ODMA_BUF_DONE_RESET 0x00000000U
#define FX1_ISP_ODMA_BUF_DONE_ODMA_BUF_DONE_ACCESS FX1_ISP_ACCESS_W1C

/**
 * ODMA_FRAME_COUNT: địa chỉ lệch 0x3090, dòng bảng 526, kiểu truy cập RO
 * Số frame đã ghi từ reset, chỉ đọc. Ưu tiên chờ bộ đếm thay DONE vì bit DONE dùng chung với xác
 * nhận W1C của lần trước.
 */
#define FX1_ISP_ODMA_FRAME_COUNT_OFFSET 0x3090U
#define REG_ODMA_FRAME_COUNT FX1_ISP_ODMA_FRAME_COUNT_OFFSET
#define FX1_ISP_ODMA_FRAME_COUNT_RESET 0x00000000U
#define FX1_ISP_ODMA_FRAME_COUNT_ACCESS FX1_ISP_ACCESS_RO
#define FX1_ISP_ODMA_FRAME_COUNT_RO_MASK 0xFFFFFFFFU
#define FX1_ISP_ODMA_FRAME_COUNT_RW_MASK 0x00000000U
#define FX1_ISP_ODMA_FRAME_COUNT_W1C_MASK 0x00000000U
#define FX1_ISP_ODMA_FRAME_COUNT_W1S_MASK 0x00000000U
#define FX1_ISP_ODMA_FRAME_COUNT_W1P_MASK 0x00000000U
#define FX1_ISP_ODMA_FRAME_COUNT_W1SC_MASK 0x00000000U
/**
 * odma_frame_count [31:0], dòng bảng 527, kiểu truy cập RO
 * Bộ đếm frame đầu ra, cuộn modulo 2^32.
 */
#define FX1_ISP_ODMA_FRAME_COUNT_ODMA_FRAME_COUNT_SHIFT 0U
#define FX1_ISP_ODMA_FRAME_COUNT_ODMA_FRAME_COUNT_WIDTH 32U
#define FX1_ISP_ODMA_FRAME_COUNT_ODMA_FRAME_COUNT_MASK 0xFFFFFFFFU
#define FX1_ISP_ODMA_FRAME_COUNT_ODMA_FRAME_COUNT_RESET 0x00000000U
#define FX1_ISP_ODMA_FRAME_COUNT_ODMA_FRAME_COUNT_ACCESS FX1_ISP_ACCESS_RO

/**
 * ODMA_Y_ADDR_L0: địa chỉ lệch 0x30A0, dòng bảng 528, kiểu truy cập RW
 * Địa chỉ DDR mặt phẳng Y bộ đệm 0, từ thấp.
 */
#define FX1_ISP_ODMA_Y_ADDR_L0_OFFSET 0x30A0U
#define REG_ODMA_Y_ADDR_L0 FX1_ISP_ODMA_Y_ADDR_L0_OFFSET
#define FX1_ISP_ODMA_Y_ADDR_L0_RESET 0x00000000U
#define FX1_ISP_ODMA_Y_ADDR_L0_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_ODMA_Y_ADDR_L0_RO_MASK 0x00000000U
#define FX1_ISP_ODMA_Y_ADDR_L0_RW_MASK 0xFFFFFFFFU
#define FX1_ISP_ODMA_Y_ADDR_L0_W1C_MASK 0x00000000U
#define FX1_ISP_ODMA_Y_ADDR_L0_W1S_MASK 0x00000000U
#define FX1_ISP_ODMA_Y_ADDR_L0_W1P_MASK 0x00000000U
#define FX1_ISP_ODMA_Y_ADDR_L0_W1SC_MASK 0x00000000U
/**
 * odma_y_addr0_lo [31:0], dòng bảng 529, kiểu truy cập RW
 * Các bit [31:0] địa chỉ mặt phẳng Y bộ đệm 0.
 */
#define FX1_ISP_ODMA_Y_ADDR_L0_ODMA_Y_ADDR0_LO_SHIFT 0U
#define FX1_ISP_ODMA_Y_ADDR_L0_ODMA_Y_ADDR0_LO_WIDTH 32U
#define FX1_ISP_ODMA_Y_ADDR_L0_ODMA_Y_ADDR0_LO_MASK 0xFFFFFFFFU
#define FX1_ISP_ODMA_Y_ADDR_L0_ODMA_Y_ADDR0_LO_RESET 0x00000000U
#define FX1_ISP_ODMA_Y_ADDR_L0_ODMA_Y_ADDR0_LO_ACCESS FX1_ISP_ACCESS_RW

/**
 * ODMA_Y_ADDR_H0: địa chỉ lệch 0x30A4, dòng bảng 530, kiểu truy cập RW
 * Địa chỉ DDR mặt phẳng Y bộ đệm 0, từ cao.
 */
#define FX1_ISP_ODMA_Y_ADDR_H0_OFFSET 0x30A4U
#define REG_ODMA_Y_ADDR_H0 FX1_ISP_ODMA_Y_ADDR_H0_OFFSET
#define FX1_ISP_ODMA_Y_ADDR_H0_RESET 0x00000000U
#define FX1_ISP_ODMA_Y_ADDR_H0_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_ODMA_Y_ADDR_H0_RO_MASK 0x00000000U
#define FX1_ISP_ODMA_Y_ADDR_H0_RW_MASK 0xFFFFFFFFU
#define FX1_ISP_ODMA_Y_ADDR_H0_W1C_MASK 0x00000000U
#define FX1_ISP_ODMA_Y_ADDR_H0_W1S_MASK 0x00000000U
#define FX1_ISP_ODMA_Y_ADDR_H0_W1P_MASK 0x00000000U
#define FX1_ISP_ODMA_Y_ADDR_H0_W1SC_MASK 0x00000000U
/**
 * odma_y_addr0_hi [31:0], dòng bảng 531, kiểu truy cập RW
 * Các bit [63:32] của địa chỉ DDR.
 */
#define FX1_ISP_ODMA_Y_ADDR_H0_ODMA_Y_ADDR0_HI_SHIFT 0U
#define FX1_ISP_ODMA_Y_ADDR_H0_ODMA_Y_ADDR0_HI_WIDTH 32U
#define FX1_ISP_ODMA_Y_ADDR_H0_ODMA_Y_ADDR0_HI_MASK 0xFFFFFFFFU
#define FX1_ISP_ODMA_Y_ADDR_H0_ODMA_Y_ADDR0_HI_RESET 0x00000000U
#define FX1_ISP_ODMA_Y_ADDR_H0_ODMA_Y_ADDR0_HI_ACCESS FX1_ISP_ACCESS_RW

/**
 * ODMA_UV_ADDR_L0: địa chỉ lệch 0x30A8, dòng bảng 532, kiểu truy cập RW
 * Địa chỉ DDR mặt phẳng UV bộ đệm 0, từ thấp.
 */
#define FX1_ISP_ODMA_UV_ADDR_L0_OFFSET 0x30A8U
#define REG_ODMA_UV_ADDR_L0 FX1_ISP_ODMA_UV_ADDR_L0_OFFSET
#define FX1_ISP_ODMA_UV_ADDR_L0_RESET 0x00000000U
#define FX1_ISP_ODMA_UV_ADDR_L0_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_ODMA_UV_ADDR_L0_RO_MASK 0x00000000U
#define FX1_ISP_ODMA_UV_ADDR_L0_RW_MASK 0xFFFFFFFFU
#define FX1_ISP_ODMA_UV_ADDR_L0_W1C_MASK 0x00000000U
#define FX1_ISP_ODMA_UV_ADDR_L0_W1S_MASK 0x00000000U
#define FX1_ISP_ODMA_UV_ADDR_L0_W1P_MASK 0x00000000U
#define FX1_ISP_ODMA_UV_ADDR_L0_W1SC_MASK 0x00000000U
/**
 * odma_uv_addr0_lo [31:0], dòng bảng 533, kiểu truy cập RW
 * Các bit [31:0] địa chỉ mặt phẳng Y bộ đệm 0.
 */
#define FX1_ISP_ODMA_UV_ADDR_L0_ODMA_UV_ADDR0_LO_SHIFT 0U
#define FX1_ISP_ODMA_UV_ADDR_L0_ODMA_UV_ADDR0_LO_WIDTH 32U
#define FX1_ISP_ODMA_UV_ADDR_L0_ODMA_UV_ADDR0_LO_MASK 0xFFFFFFFFU
#define FX1_ISP_ODMA_UV_ADDR_L0_ODMA_UV_ADDR0_LO_RESET 0x00000000U
#define FX1_ISP_ODMA_UV_ADDR_L0_ODMA_UV_ADDR0_LO_ACCESS FX1_ISP_ACCESS_RW

/**
 * ODMA_UV_ADDR_H0: địa chỉ lệch 0x30AC, dòng bảng 534, kiểu truy cập RW
 * Địa chỉ DDR mặt phẳng UV bộ đệm 0, từ cao.
 */
#define FX1_ISP_ODMA_UV_ADDR_H0_OFFSET 0x30ACU
#define REG_ODMA_UV_ADDR_H0 FX1_ISP_ODMA_UV_ADDR_H0_OFFSET
#define FX1_ISP_ODMA_UV_ADDR_H0_RESET 0x00000000U
#define FX1_ISP_ODMA_UV_ADDR_H0_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_ODMA_UV_ADDR_H0_RO_MASK 0x00000000U
#define FX1_ISP_ODMA_UV_ADDR_H0_RW_MASK 0xFFFFFFFFU
#define FX1_ISP_ODMA_UV_ADDR_H0_W1C_MASK 0x00000000U
#define FX1_ISP_ODMA_UV_ADDR_H0_W1S_MASK 0x00000000U
#define FX1_ISP_ODMA_UV_ADDR_H0_W1P_MASK 0x00000000U
#define FX1_ISP_ODMA_UV_ADDR_H0_W1SC_MASK 0x00000000U
/**
 * odma_uv_addr0_hi [31:0], dòng bảng 535, kiểu truy cập RW
 * Các bit [63:32] của địa chỉ DDR.
 */
#define FX1_ISP_ODMA_UV_ADDR_H0_ODMA_UV_ADDR0_HI_SHIFT 0U
#define FX1_ISP_ODMA_UV_ADDR_H0_ODMA_UV_ADDR0_HI_WIDTH 32U
#define FX1_ISP_ODMA_UV_ADDR_H0_ODMA_UV_ADDR0_HI_MASK 0xFFFFFFFFU
#define FX1_ISP_ODMA_UV_ADDR_H0_ODMA_UV_ADDR0_HI_RESET 0x00000000U
#define FX1_ISP_ODMA_UV_ADDR_H0_ODMA_UV_ADDR0_HI_ACCESS FX1_ISP_ACCESS_RW

/**
 * ODMA_Y_ADDR_L1: địa chỉ lệch 0x30B0, dòng bảng 536, kiểu truy cập RW
 * Địa chỉ DDR mặt phẳng Y bộ đệm 1, từ thấp.
 */
#define FX1_ISP_ODMA_Y_ADDR_L1_OFFSET 0x30B0U
#define REG_ODMA_Y_ADDR_L1 FX1_ISP_ODMA_Y_ADDR_L1_OFFSET
#define FX1_ISP_ODMA_Y_ADDR_L1_RESET 0x00000000U
#define FX1_ISP_ODMA_Y_ADDR_L1_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_ODMA_Y_ADDR_L1_RO_MASK 0x00000000U
#define FX1_ISP_ODMA_Y_ADDR_L1_RW_MASK 0xFFFFFFFFU
#define FX1_ISP_ODMA_Y_ADDR_L1_W1C_MASK 0x00000000U
#define FX1_ISP_ODMA_Y_ADDR_L1_W1S_MASK 0x00000000U
#define FX1_ISP_ODMA_Y_ADDR_L1_W1P_MASK 0x00000000U
#define FX1_ISP_ODMA_Y_ADDR_L1_W1SC_MASK 0x00000000U
/**
 * odma_y_addr1_lo [31:0], dòng bảng 537, kiểu truy cập RW
 * Các bit [31:0] địa chỉ mặt phẳng Y bộ đệm 1.
 */
#define FX1_ISP_ODMA_Y_ADDR_L1_ODMA_Y_ADDR1_LO_SHIFT 0U
#define FX1_ISP_ODMA_Y_ADDR_L1_ODMA_Y_ADDR1_LO_WIDTH 32U
#define FX1_ISP_ODMA_Y_ADDR_L1_ODMA_Y_ADDR1_LO_MASK 0xFFFFFFFFU
#define FX1_ISP_ODMA_Y_ADDR_L1_ODMA_Y_ADDR1_LO_RESET 0x00000000U
#define FX1_ISP_ODMA_Y_ADDR_L1_ODMA_Y_ADDR1_LO_ACCESS FX1_ISP_ACCESS_RW

/**
 * ODMA_Y_ADDR_H1: địa chỉ lệch 0x30B4, dòng bảng 538, kiểu truy cập RW
 * Địa chỉ DDR mặt phẳng Y bộ đệm 1, từ cao.
 */
#define FX1_ISP_ODMA_Y_ADDR_H1_OFFSET 0x30B4U
#define REG_ODMA_Y_ADDR_H1 FX1_ISP_ODMA_Y_ADDR_H1_OFFSET
#define FX1_ISP_ODMA_Y_ADDR_H1_RESET 0x00000000U
#define FX1_ISP_ODMA_Y_ADDR_H1_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_ODMA_Y_ADDR_H1_RO_MASK 0x00000000U
#define FX1_ISP_ODMA_Y_ADDR_H1_RW_MASK 0xFFFFFFFFU
#define FX1_ISP_ODMA_Y_ADDR_H1_W1C_MASK 0x00000000U
#define FX1_ISP_ODMA_Y_ADDR_H1_W1S_MASK 0x00000000U
#define FX1_ISP_ODMA_Y_ADDR_H1_W1P_MASK 0x00000000U
#define FX1_ISP_ODMA_Y_ADDR_H1_W1SC_MASK 0x00000000U
/**
 * odma_y_addr1_hi [31:0], dòng bảng 539, kiểu truy cập RW
 * Các bit [63:32] của địa chỉ DDR.
 */
#define FX1_ISP_ODMA_Y_ADDR_H1_ODMA_Y_ADDR1_HI_SHIFT 0U
#define FX1_ISP_ODMA_Y_ADDR_H1_ODMA_Y_ADDR1_HI_WIDTH 32U
#define FX1_ISP_ODMA_Y_ADDR_H1_ODMA_Y_ADDR1_HI_MASK 0xFFFFFFFFU
#define FX1_ISP_ODMA_Y_ADDR_H1_ODMA_Y_ADDR1_HI_RESET 0x00000000U
#define FX1_ISP_ODMA_Y_ADDR_H1_ODMA_Y_ADDR1_HI_ACCESS FX1_ISP_ACCESS_RW

/**
 * ODMA_UV_ADDR_L1: địa chỉ lệch 0x30B8, dòng bảng 540, kiểu truy cập RW
 * Địa chỉ DDR mặt phẳng UV bộ đệm 1, từ thấp.
 */
#define FX1_ISP_ODMA_UV_ADDR_L1_OFFSET 0x30B8U
#define REG_ODMA_UV_ADDR_L1 FX1_ISP_ODMA_UV_ADDR_L1_OFFSET
#define FX1_ISP_ODMA_UV_ADDR_L1_RESET 0x00000000U
#define FX1_ISP_ODMA_UV_ADDR_L1_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_ODMA_UV_ADDR_L1_RO_MASK 0x00000000U
#define FX1_ISP_ODMA_UV_ADDR_L1_RW_MASK 0xFFFFFFFFU
#define FX1_ISP_ODMA_UV_ADDR_L1_W1C_MASK 0x00000000U
#define FX1_ISP_ODMA_UV_ADDR_L1_W1S_MASK 0x00000000U
#define FX1_ISP_ODMA_UV_ADDR_L1_W1P_MASK 0x00000000U
#define FX1_ISP_ODMA_UV_ADDR_L1_W1SC_MASK 0x00000000U
/**
 * odma_uv_addr1_lo [31:0], dòng bảng 541, kiểu truy cập RW
 * Các bit [31:0] địa chỉ mặt phẳng Y bộ đệm 1.
 */
#define FX1_ISP_ODMA_UV_ADDR_L1_ODMA_UV_ADDR1_LO_SHIFT 0U
#define FX1_ISP_ODMA_UV_ADDR_L1_ODMA_UV_ADDR1_LO_WIDTH 32U
#define FX1_ISP_ODMA_UV_ADDR_L1_ODMA_UV_ADDR1_LO_MASK 0xFFFFFFFFU
#define FX1_ISP_ODMA_UV_ADDR_L1_ODMA_UV_ADDR1_LO_RESET 0x00000000U
#define FX1_ISP_ODMA_UV_ADDR_L1_ODMA_UV_ADDR1_LO_ACCESS FX1_ISP_ACCESS_RW

/**
 * ODMA_UV_ADDR_H1: địa chỉ lệch 0x30BC, dòng bảng 542, kiểu truy cập RW
 * Địa chỉ DDR mặt phẳng UV bộ đệm 1, từ cao.
 */
#define FX1_ISP_ODMA_UV_ADDR_H1_OFFSET 0x30BCU
#define REG_ODMA_UV_ADDR_H1 FX1_ISP_ODMA_UV_ADDR_H1_OFFSET
#define FX1_ISP_ODMA_UV_ADDR_H1_RESET 0x00000000U
#define FX1_ISP_ODMA_UV_ADDR_H1_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_ODMA_UV_ADDR_H1_RO_MASK 0x00000000U
#define FX1_ISP_ODMA_UV_ADDR_H1_RW_MASK 0xFFFFFFFFU
#define FX1_ISP_ODMA_UV_ADDR_H1_W1C_MASK 0x00000000U
#define FX1_ISP_ODMA_UV_ADDR_H1_W1S_MASK 0x00000000U
#define FX1_ISP_ODMA_UV_ADDR_H1_W1P_MASK 0x00000000U
#define FX1_ISP_ODMA_UV_ADDR_H1_W1SC_MASK 0x00000000U
/**
 * odma_uv_addr1_hi [31:0], dòng bảng 543, kiểu truy cập RW
 * Các bit [63:32] của địa chỉ DDR.
 */
#define FX1_ISP_ODMA_UV_ADDR_H1_ODMA_UV_ADDR1_HI_SHIFT 0U
#define FX1_ISP_ODMA_UV_ADDR_H1_ODMA_UV_ADDR1_HI_WIDTH 32U
#define FX1_ISP_ODMA_UV_ADDR_H1_ODMA_UV_ADDR1_HI_MASK 0xFFFFFFFFU
#define FX1_ISP_ODMA_UV_ADDR_H1_ODMA_UV_ADDR1_HI_RESET 0x00000000U
#define FX1_ISP_ODMA_UV_ADDR_H1_ODMA_UV_ADDR1_HI_ACCESS FX1_ISP_ACCESS_RW

/**
 * ODMA_Y_ADDR_L2: địa chỉ lệch 0x30C0, dòng bảng 544, kiểu truy cập RW
 * Địa chỉ DDR mặt phẳng Y bộ đệm 2, từ thấp.
 */
#define FX1_ISP_ODMA_Y_ADDR_L2_OFFSET 0x30C0U
#define REG_ODMA_Y_ADDR_L2 FX1_ISP_ODMA_Y_ADDR_L2_OFFSET
#define FX1_ISP_ODMA_Y_ADDR_L2_RESET 0x00000000U
#define FX1_ISP_ODMA_Y_ADDR_L2_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_ODMA_Y_ADDR_L2_RO_MASK 0x00000000U
#define FX1_ISP_ODMA_Y_ADDR_L2_RW_MASK 0xFFFFFFFFU
#define FX1_ISP_ODMA_Y_ADDR_L2_W1C_MASK 0x00000000U
#define FX1_ISP_ODMA_Y_ADDR_L2_W1S_MASK 0x00000000U
#define FX1_ISP_ODMA_Y_ADDR_L2_W1P_MASK 0x00000000U
#define FX1_ISP_ODMA_Y_ADDR_L2_W1SC_MASK 0x00000000U
/**
 * odma_y_addr2_lo [31:0], dòng bảng 545, kiểu truy cập RW
 * Các bit [31:0] địa chỉ mặt phẳng Y bộ đệm 2.
 */
#define FX1_ISP_ODMA_Y_ADDR_L2_ODMA_Y_ADDR2_LO_SHIFT 0U
#define FX1_ISP_ODMA_Y_ADDR_L2_ODMA_Y_ADDR2_LO_WIDTH 32U
#define FX1_ISP_ODMA_Y_ADDR_L2_ODMA_Y_ADDR2_LO_MASK 0xFFFFFFFFU
#define FX1_ISP_ODMA_Y_ADDR_L2_ODMA_Y_ADDR2_LO_RESET 0x00000000U
#define FX1_ISP_ODMA_Y_ADDR_L2_ODMA_Y_ADDR2_LO_ACCESS FX1_ISP_ACCESS_RW

/**
 * ODMA_Y_ADDR_H2: địa chỉ lệch 0x30C4, dòng bảng 546, kiểu truy cập RW
 * Địa chỉ DDR mặt phẳng Y bộ đệm 2, từ cao.
 */
#define FX1_ISP_ODMA_Y_ADDR_H2_OFFSET 0x30C4U
#define REG_ODMA_Y_ADDR_H2 FX1_ISP_ODMA_Y_ADDR_H2_OFFSET
#define FX1_ISP_ODMA_Y_ADDR_H2_RESET 0x00000000U
#define FX1_ISP_ODMA_Y_ADDR_H2_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_ODMA_Y_ADDR_H2_RO_MASK 0x00000000U
#define FX1_ISP_ODMA_Y_ADDR_H2_RW_MASK 0xFFFFFFFFU
#define FX1_ISP_ODMA_Y_ADDR_H2_W1C_MASK 0x00000000U
#define FX1_ISP_ODMA_Y_ADDR_H2_W1S_MASK 0x00000000U
#define FX1_ISP_ODMA_Y_ADDR_H2_W1P_MASK 0x00000000U
#define FX1_ISP_ODMA_Y_ADDR_H2_W1SC_MASK 0x00000000U
/**
 * odma_y_addr2_hi [31:0], dòng bảng 547, kiểu truy cập RW
 * Các bit [63:32] của địa chỉ DDR.
 */
#define FX1_ISP_ODMA_Y_ADDR_H2_ODMA_Y_ADDR2_HI_SHIFT 0U
#define FX1_ISP_ODMA_Y_ADDR_H2_ODMA_Y_ADDR2_HI_WIDTH 32U
#define FX1_ISP_ODMA_Y_ADDR_H2_ODMA_Y_ADDR2_HI_MASK 0xFFFFFFFFU
#define FX1_ISP_ODMA_Y_ADDR_H2_ODMA_Y_ADDR2_HI_RESET 0x00000000U
#define FX1_ISP_ODMA_Y_ADDR_H2_ODMA_Y_ADDR2_HI_ACCESS FX1_ISP_ACCESS_RW

/**
 * ODMA_UV_ADDR_L2: địa chỉ lệch 0x30C8, dòng bảng 548, kiểu truy cập RW
 * Địa chỉ DDR mặt phẳng UV bộ đệm 2, từ thấp.
 */
#define FX1_ISP_ODMA_UV_ADDR_L2_OFFSET 0x30C8U
#define REG_ODMA_UV_ADDR_L2 FX1_ISP_ODMA_UV_ADDR_L2_OFFSET
#define FX1_ISP_ODMA_UV_ADDR_L2_RESET 0x00000000U
#define FX1_ISP_ODMA_UV_ADDR_L2_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_ODMA_UV_ADDR_L2_RO_MASK 0x00000000U
#define FX1_ISP_ODMA_UV_ADDR_L2_RW_MASK 0xFFFFFFFFU
#define FX1_ISP_ODMA_UV_ADDR_L2_W1C_MASK 0x00000000U
#define FX1_ISP_ODMA_UV_ADDR_L2_W1S_MASK 0x00000000U
#define FX1_ISP_ODMA_UV_ADDR_L2_W1P_MASK 0x00000000U
#define FX1_ISP_ODMA_UV_ADDR_L2_W1SC_MASK 0x00000000U
/**
 * odma_uv_addr2_lo [31:0], dòng bảng 549, kiểu truy cập RW
 * Các bit [31:0] địa chỉ mặt phẳng Y bộ đệm 2.
 */
#define FX1_ISP_ODMA_UV_ADDR_L2_ODMA_UV_ADDR2_LO_SHIFT 0U
#define FX1_ISP_ODMA_UV_ADDR_L2_ODMA_UV_ADDR2_LO_WIDTH 32U
#define FX1_ISP_ODMA_UV_ADDR_L2_ODMA_UV_ADDR2_LO_MASK 0xFFFFFFFFU
#define FX1_ISP_ODMA_UV_ADDR_L2_ODMA_UV_ADDR2_LO_RESET 0x00000000U
#define FX1_ISP_ODMA_UV_ADDR_L2_ODMA_UV_ADDR2_LO_ACCESS FX1_ISP_ACCESS_RW

/**
 * ODMA_UV_ADDR_H2: địa chỉ lệch 0x30CC, dòng bảng 550, kiểu truy cập RW
 * Địa chỉ DDR mặt phẳng UV bộ đệm 2, từ cao.
 */
#define FX1_ISP_ODMA_UV_ADDR_H2_OFFSET 0x30CCU
#define REG_ODMA_UV_ADDR_H2 FX1_ISP_ODMA_UV_ADDR_H2_OFFSET
#define FX1_ISP_ODMA_UV_ADDR_H2_RESET 0x00000000U
#define FX1_ISP_ODMA_UV_ADDR_H2_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_ODMA_UV_ADDR_H2_RO_MASK 0x00000000U
#define FX1_ISP_ODMA_UV_ADDR_H2_RW_MASK 0xFFFFFFFFU
#define FX1_ISP_ODMA_UV_ADDR_H2_W1C_MASK 0x00000000U
#define FX1_ISP_ODMA_UV_ADDR_H2_W1S_MASK 0x00000000U
#define FX1_ISP_ODMA_UV_ADDR_H2_W1P_MASK 0x00000000U
#define FX1_ISP_ODMA_UV_ADDR_H2_W1SC_MASK 0x00000000U
/**
 * odma_uv_addr2_hi [31:0], dòng bảng 551, kiểu truy cập RW
 * Các bit [63:32] của địa chỉ DDR.
 */
#define FX1_ISP_ODMA_UV_ADDR_H2_ODMA_UV_ADDR2_HI_SHIFT 0U
#define FX1_ISP_ODMA_UV_ADDR_H2_ODMA_UV_ADDR2_HI_WIDTH 32U
#define FX1_ISP_ODMA_UV_ADDR_H2_ODMA_UV_ADDR2_HI_MASK 0xFFFFFFFFU
#define FX1_ISP_ODMA_UV_ADDR_H2_ODMA_UV_ADDR2_HI_RESET 0x00000000U
#define FX1_ISP_ODMA_UV_ADDR_H2_ODMA_UV_ADDR2_HI_ACCESS FX1_ISP_ACCESS_RW

/**
 * ODMA_Y_ADDR_L3: địa chỉ lệch 0x30D0, dòng bảng 552, kiểu truy cập RW
 * Địa chỉ DDR mặt phẳng Y bộ đệm 3, từ thấp.
 */
#define FX1_ISP_ODMA_Y_ADDR_L3_OFFSET 0x30D0U
#define REG_ODMA_Y_ADDR_L3 FX1_ISP_ODMA_Y_ADDR_L3_OFFSET
#define FX1_ISP_ODMA_Y_ADDR_L3_RESET 0x00000000U
#define FX1_ISP_ODMA_Y_ADDR_L3_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_ODMA_Y_ADDR_L3_RO_MASK 0x00000000U
#define FX1_ISP_ODMA_Y_ADDR_L3_RW_MASK 0xFFFFFFFFU
#define FX1_ISP_ODMA_Y_ADDR_L3_W1C_MASK 0x00000000U
#define FX1_ISP_ODMA_Y_ADDR_L3_W1S_MASK 0x00000000U
#define FX1_ISP_ODMA_Y_ADDR_L3_W1P_MASK 0x00000000U
#define FX1_ISP_ODMA_Y_ADDR_L3_W1SC_MASK 0x00000000U
/**
 * odma_y_addr3_lo [31:0], dòng bảng 553, kiểu truy cập RW
 * Các bit [31:0] địa chỉ mặt phẳng Y bộ đệm 3.
 */
#define FX1_ISP_ODMA_Y_ADDR_L3_ODMA_Y_ADDR3_LO_SHIFT 0U
#define FX1_ISP_ODMA_Y_ADDR_L3_ODMA_Y_ADDR3_LO_WIDTH 32U
#define FX1_ISP_ODMA_Y_ADDR_L3_ODMA_Y_ADDR3_LO_MASK 0xFFFFFFFFU
#define FX1_ISP_ODMA_Y_ADDR_L3_ODMA_Y_ADDR3_LO_RESET 0x00000000U
#define FX1_ISP_ODMA_Y_ADDR_L3_ODMA_Y_ADDR3_LO_ACCESS FX1_ISP_ACCESS_RW

/**
 * ODMA_Y_ADDR_H3: địa chỉ lệch 0x30D4, dòng bảng 554, kiểu truy cập RW
 * Địa chỉ DDR mặt phẳng Y bộ đệm 3, từ cao.
 */
#define FX1_ISP_ODMA_Y_ADDR_H3_OFFSET 0x30D4U
#define REG_ODMA_Y_ADDR_H3 FX1_ISP_ODMA_Y_ADDR_H3_OFFSET
#define FX1_ISP_ODMA_Y_ADDR_H3_RESET 0x00000000U
#define FX1_ISP_ODMA_Y_ADDR_H3_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_ODMA_Y_ADDR_H3_RO_MASK 0x00000000U
#define FX1_ISP_ODMA_Y_ADDR_H3_RW_MASK 0xFFFFFFFFU
#define FX1_ISP_ODMA_Y_ADDR_H3_W1C_MASK 0x00000000U
#define FX1_ISP_ODMA_Y_ADDR_H3_W1S_MASK 0x00000000U
#define FX1_ISP_ODMA_Y_ADDR_H3_W1P_MASK 0x00000000U
#define FX1_ISP_ODMA_Y_ADDR_H3_W1SC_MASK 0x00000000U
/**
 * odma_y_addr3_hi [31:0], dòng bảng 555, kiểu truy cập RW
 * Các bit [63:32] của địa chỉ DDR.
 */
#define FX1_ISP_ODMA_Y_ADDR_H3_ODMA_Y_ADDR3_HI_SHIFT 0U
#define FX1_ISP_ODMA_Y_ADDR_H3_ODMA_Y_ADDR3_HI_WIDTH 32U
#define FX1_ISP_ODMA_Y_ADDR_H3_ODMA_Y_ADDR3_HI_MASK 0xFFFFFFFFU
#define FX1_ISP_ODMA_Y_ADDR_H3_ODMA_Y_ADDR3_HI_RESET 0x00000000U
#define FX1_ISP_ODMA_Y_ADDR_H3_ODMA_Y_ADDR3_HI_ACCESS FX1_ISP_ACCESS_RW

/**
 * ODMA_UV_ADDR_L3: địa chỉ lệch 0x30D8, dòng bảng 556, kiểu truy cập RW
 * Địa chỉ DDR mặt phẳng UV bộ đệm 3, từ thấp.
 */
#define FX1_ISP_ODMA_UV_ADDR_L3_OFFSET 0x30D8U
#define REG_ODMA_UV_ADDR_L3 FX1_ISP_ODMA_UV_ADDR_L3_OFFSET
#define FX1_ISP_ODMA_UV_ADDR_L3_RESET 0x00000000U
#define FX1_ISP_ODMA_UV_ADDR_L3_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_ODMA_UV_ADDR_L3_RO_MASK 0x00000000U
#define FX1_ISP_ODMA_UV_ADDR_L3_RW_MASK 0xFFFFFFFFU
#define FX1_ISP_ODMA_UV_ADDR_L3_W1C_MASK 0x00000000U
#define FX1_ISP_ODMA_UV_ADDR_L3_W1S_MASK 0x00000000U
#define FX1_ISP_ODMA_UV_ADDR_L3_W1P_MASK 0x00000000U
#define FX1_ISP_ODMA_UV_ADDR_L3_W1SC_MASK 0x00000000U
/**
 * odma_uv_addr3_lo [31:0], dòng bảng 557, kiểu truy cập RW
 * Các bit [31:0] địa chỉ mặt phẳng Y bộ đệm 3.
 */
#define FX1_ISP_ODMA_UV_ADDR_L3_ODMA_UV_ADDR3_LO_SHIFT 0U
#define FX1_ISP_ODMA_UV_ADDR_L3_ODMA_UV_ADDR3_LO_WIDTH 32U
#define FX1_ISP_ODMA_UV_ADDR_L3_ODMA_UV_ADDR3_LO_MASK 0xFFFFFFFFU
#define FX1_ISP_ODMA_UV_ADDR_L3_ODMA_UV_ADDR3_LO_RESET 0x00000000U
#define FX1_ISP_ODMA_UV_ADDR_L3_ODMA_UV_ADDR3_LO_ACCESS FX1_ISP_ACCESS_RW

/**
 * ODMA_UV_ADDR_H3: địa chỉ lệch 0x30DC, dòng bảng 558, kiểu truy cập RW
 * Địa chỉ DDR mặt phẳng UV bộ đệm 3, từ cao.
 */
#define FX1_ISP_ODMA_UV_ADDR_H3_OFFSET 0x30DCU
#define REG_ODMA_UV_ADDR_H3 FX1_ISP_ODMA_UV_ADDR_H3_OFFSET
#define FX1_ISP_ODMA_UV_ADDR_H3_RESET 0x00000000U
#define FX1_ISP_ODMA_UV_ADDR_H3_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_ODMA_UV_ADDR_H3_RO_MASK 0x00000000U
#define FX1_ISP_ODMA_UV_ADDR_H3_RW_MASK 0xFFFFFFFFU
#define FX1_ISP_ODMA_UV_ADDR_H3_W1C_MASK 0x00000000U
#define FX1_ISP_ODMA_UV_ADDR_H3_W1S_MASK 0x00000000U
#define FX1_ISP_ODMA_UV_ADDR_H3_W1P_MASK 0x00000000U
#define FX1_ISP_ODMA_UV_ADDR_H3_W1SC_MASK 0x00000000U
/**
 * odma_uv_addr3_hi [31:0], dòng bảng 559, kiểu truy cập RW
 * Các bit [63:32] của địa chỉ DDR.
 */
#define FX1_ISP_ODMA_UV_ADDR_H3_ODMA_UV_ADDR3_HI_SHIFT 0U
#define FX1_ISP_ODMA_UV_ADDR_H3_ODMA_UV_ADDR3_HI_WIDTH 32U
#define FX1_ISP_ODMA_UV_ADDR_H3_ODMA_UV_ADDR3_HI_MASK 0xFFFFFFFFU
#define FX1_ISP_ODMA_UV_ADDR_H3_ODMA_UV_ADDR3_HI_RESET 0x00000000U
#define FX1_ISP_ODMA_UV_ADDR_H3_ODMA_UV_ADDR3_HI_ACCESS FX1_ISP_ACCESS_RW

/**
 * IDMA_BUF_TAG0: địa chỉ lệch 0x3100, dòng bảng 560, kiểu truy cập RW
 * Thông tin frame của bộ đệm 0. Nhãn frame do phần mềm đặt trước IDMA_BUF_VALID, chốt cùng frame vào
 * và công bố với buffer ra hoàn tất.
 */
#define FX1_ISP_IDMA_BUF_TAG0_OFFSET 0x3100U
#define REG_IDMA_BUF_TAG0 FX1_ISP_IDMA_BUF_TAG0_OFFSET
#define FX1_ISP_IDMA_BUF_TAG0_RESET 0x00000000U
#define FX1_ISP_IDMA_BUF_TAG0_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_IDMA_BUF_TAG0_RO_MASK 0x00000000U
#define FX1_ISP_IDMA_BUF_TAG0_RW_MASK 0xFFFFFFFFU
#define FX1_ISP_IDMA_BUF_TAG0_W1C_MASK 0x00000000U
#define FX1_ISP_IDMA_BUF_TAG0_W1S_MASK 0x00000000U
#define FX1_ISP_IDMA_BUF_TAG0_W1P_MASK 0x00000000U
#define FX1_ISP_IDMA_BUF_TAG0_W1SC_MASK 0x00000000U
/**
 * tag [31:0], dòng bảng 561, kiểu truy cập RW
 * Thông tin frame của bộ đệm 0. Giá trị đầy đủ 32 bit, hard reset về 0. Nhãn frame do phần mềm đặt
 * trước IDMA_BUF_VALID, chốt cùng frame vào và công bố với buffer ra hoàn tất.
 */
#define FX1_ISP_IDMA_BUF_TAG0_TAG_SHIFT 0U
#define FX1_ISP_IDMA_BUF_TAG0_TAG_WIDTH 32U
#define FX1_ISP_IDMA_BUF_TAG0_TAG_MASK 0xFFFFFFFFU
#define FX1_ISP_IDMA_BUF_TAG0_TAG_RESET 0x00000000U
#define FX1_ISP_IDMA_BUF_TAG0_TAG_ACCESS FX1_ISP_ACCESS_RW

/**
 * IDMA_BUF_TAG1: địa chỉ lệch 0x3104, dòng bảng 562, kiểu truy cập RW
 * Thông tin frame của bộ đệm 1. Nhãn frame do phần mềm đặt trước IDMA_BUF_VALID, chốt cùng frame vào
 * và công bố với buffer ra hoàn tất.
 */
#define FX1_ISP_IDMA_BUF_TAG1_OFFSET 0x3104U
#define REG_IDMA_BUF_TAG1 FX1_ISP_IDMA_BUF_TAG1_OFFSET
#define FX1_ISP_IDMA_BUF_TAG1_RESET 0x00000000U
#define FX1_ISP_IDMA_BUF_TAG1_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_IDMA_BUF_TAG1_RO_MASK 0x00000000U
#define FX1_ISP_IDMA_BUF_TAG1_RW_MASK 0xFFFFFFFFU
#define FX1_ISP_IDMA_BUF_TAG1_W1C_MASK 0x00000000U
#define FX1_ISP_IDMA_BUF_TAG1_W1S_MASK 0x00000000U
#define FX1_ISP_IDMA_BUF_TAG1_W1P_MASK 0x00000000U
#define FX1_ISP_IDMA_BUF_TAG1_W1SC_MASK 0x00000000U
/**
 * tag [31:0], dòng bảng 563, kiểu truy cập RW
 * Thông tin frame của bộ đệm 1. Giá trị đầy đủ 32 bit, hard reset về 0. Nhãn frame do phần mềm đặt
 * trước IDMA_BUF_VALID, chốt cùng frame vào và công bố với buffer ra hoàn tất.
 */
#define FX1_ISP_IDMA_BUF_TAG1_TAG_SHIFT 0U
#define FX1_ISP_IDMA_BUF_TAG1_TAG_WIDTH 32U
#define FX1_ISP_IDMA_BUF_TAG1_TAG_MASK 0xFFFFFFFFU
#define FX1_ISP_IDMA_BUF_TAG1_TAG_RESET 0x00000000U
#define FX1_ISP_IDMA_BUF_TAG1_TAG_ACCESS FX1_ISP_ACCESS_RW

/**
 * IDMA_BUF_TAG2: địa chỉ lệch 0x3108, dòng bảng 564, kiểu truy cập RW
 * Thông tin frame của bộ đệm 2. Nhãn frame do phần mềm đặt trước IDMA_BUF_VALID, chốt cùng frame vào
 * và công bố với buffer ra hoàn tất.
 */
#define FX1_ISP_IDMA_BUF_TAG2_OFFSET 0x3108U
#define REG_IDMA_BUF_TAG2 FX1_ISP_IDMA_BUF_TAG2_OFFSET
#define FX1_ISP_IDMA_BUF_TAG2_RESET 0x00000000U
#define FX1_ISP_IDMA_BUF_TAG2_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_IDMA_BUF_TAG2_RO_MASK 0x00000000U
#define FX1_ISP_IDMA_BUF_TAG2_RW_MASK 0xFFFFFFFFU
#define FX1_ISP_IDMA_BUF_TAG2_W1C_MASK 0x00000000U
#define FX1_ISP_IDMA_BUF_TAG2_W1S_MASK 0x00000000U
#define FX1_ISP_IDMA_BUF_TAG2_W1P_MASK 0x00000000U
#define FX1_ISP_IDMA_BUF_TAG2_W1SC_MASK 0x00000000U
/**
 * tag [31:0], dòng bảng 565, kiểu truy cập RW
 * Thông tin frame của bộ đệm 2. Giá trị đầy đủ 32 bit, hard reset về 0. Nhãn frame do phần mềm đặt
 * trước IDMA_BUF_VALID, chốt cùng frame vào và công bố với buffer ra hoàn tất.
 */
#define FX1_ISP_IDMA_BUF_TAG2_TAG_SHIFT 0U
#define FX1_ISP_IDMA_BUF_TAG2_TAG_WIDTH 32U
#define FX1_ISP_IDMA_BUF_TAG2_TAG_MASK 0xFFFFFFFFU
#define FX1_ISP_IDMA_BUF_TAG2_TAG_RESET 0x00000000U
#define FX1_ISP_IDMA_BUF_TAG2_TAG_ACCESS FX1_ISP_ACCESS_RW

/**
 * IDMA_BUF_TAG3: địa chỉ lệch 0x310C, dòng bảng 566, kiểu truy cập RW
 * Thông tin frame của bộ đệm 3. Nhãn frame do phần mềm đặt trước IDMA_BUF_VALID, chốt cùng frame vào
 * và công bố với buffer ra hoàn tất.
 */
#define FX1_ISP_IDMA_BUF_TAG3_OFFSET 0x310CU
#define REG_IDMA_BUF_TAG3 FX1_ISP_IDMA_BUF_TAG3_OFFSET
#define FX1_ISP_IDMA_BUF_TAG3_RESET 0x00000000U
#define FX1_ISP_IDMA_BUF_TAG3_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_IDMA_BUF_TAG3_RO_MASK 0x00000000U
#define FX1_ISP_IDMA_BUF_TAG3_RW_MASK 0xFFFFFFFFU
#define FX1_ISP_IDMA_BUF_TAG3_W1C_MASK 0x00000000U
#define FX1_ISP_IDMA_BUF_TAG3_W1S_MASK 0x00000000U
#define FX1_ISP_IDMA_BUF_TAG3_W1P_MASK 0x00000000U
#define FX1_ISP_IDMA_BUF_TAG3_W1SC_MASK 0x00000000U
/**
 * tag [31:0], dòng bảng 567, kiểu truy cập RW
 * Thông tin frame của bộ đệm 3. Giá trị đầy đủ 32 bit, hard reset về 0. Nhãn frame do phần mềm đặt
 * trước IDMA_BUF_VALID, chốt cùng frame vào và công bố với buffer ra hoàn tất.
 */
#define FX1_ISP_IDMA_BUF_TAG3_TAG_SHIFT 0U
#define FX1_ISP_IDMA_BUF_TAG3_TAG_WIDTH 32U
#define FX1_ISP_IDMA_BUF_TAG3_TAG_MASK 0xFFFFFFFFU
#define FX1_ISP_IDMA_BUF_TAG3_TAG_RESET 0x00000000U
#define FX1_ISP_IDMA_BUF_TAG3_TAG_ACCESS FX1_ISP_ACCESS_RW

/**
 * IDMA_BUF_TS0: địa chỉ lệch 0x3110, dòng bảng 568, kiểu truy cập RW
 * Thông tin frame của bộ đệm 0. Mốc chụp 32 bit theo gốc thời gian FRAME_COUNTER, ghi trước
 * IDMA_BUF_VALID và đi cùng nhãn frame; xử lý cuộn modulo 2^32.
 */
#define FX1_ISP_IDMA_BUF_TS0_OFFSET 0x3110U
#define REG_IDMA_BUF_TS0 FX1_ISP_IDMA_BUF_TS0_OFFSET
#define FX1_ISP_IDMA_BUF_TS0_RESET 0x00000000U
#define FX1_ISP_IDMA_BUF_TS0_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_IDMA_BUF_TS0_RO_MASK 0x00000000U
#define FX1_ISP_IDMA_BUF_TS0_RW_MASK 0xFFFFFFFFU
#define FX1_ISP_IDMA_BUF_TS0_W1C_MASK 0x00000000U
#define FX1_ISP_IDMA_BUF_TS0_W1S_MASK 0x00000000U
#define FX1_ISP_IDMA_BUF_TS0_W1P_MASK 0x00000000U
#define FX1_ISP_IDMA_BUF_TS0_W1SC_MASK 0x00000000U
/**
 * timestamp [31:0], dòng bảng 569, kiểu truy cập RW
 * Thông tin frame của bộ đệm 0. Giá trị đầy đủ 32 bit, hard reset về 0. Mốc chụp 32 bit theo gốc
 * thời gian FRAME_COUNTER, ghi trước IDMA_BUF_VALID và đi cùng nhãn frame; xử lý cuộn modulo 2^32.
 */
#define FX1_ISP_IDMA_BUF_TS0_TIMESTAMP_SHIFT 0U
#define FX1_ISP_IDMA_BUF_TS0_TIMESTAMP_WIDTH 32U
#define FX1_ISP_IDMA_BUF_TS0_TIMESTAMP_MASK 0xFFFFFFFFU
#define FX1_ISP_IDMA_BUF_TS0_TIMESTAMP_RESET 0x00000000U
#define FX1_ISP_IDMA_BUF_TS0_TIMESTAMP_ACCESS FX1_ISP_ACCESS_RW

/**
 * IDMA_BUF_TS1: địa chỉ lệch 0x3114, dòng bảng 570, kiểu truy cập RW
 * Thông tin frame của bộ đệm 1. Mốc chụp 32 bit theo gốc thời gian FRAME_COUNTER, ghi trước
 * IDMA_BUF_VALID và đi cùng nhãn frame; xử lý cuộn modulo 2^32.
 */
#define FX1_ISP_IDMA_BUF_TS1_OFFSET 0x3114U
#define REG_IDMA_BUF_TS1 FX1_ISP_IDMA_BUF_TS1_OFFSET
#define FX1_ISP_IDMA_BUF_TS1_RESET 0x00000000U
#define FX1_ISP_IDMA_BUF_TS1_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_IDMA_BUF_TS1_RO_MASK 0x00000000U
#define FX1_ISP_IDMA_BUF_TS1_RW_MASK 0xFFFFFFFFU
#define FX1_ISP_IDMA_BUF_TS1_W1C_MASK 0x00000000U
#define FX1_ISP_IDMA_BUF_TS1_W1S_MASK 0x00000000U
#define FX1_ISP_IDMA_BUF_TS1_W1P_MASK 0x00000000U
#define FX1_ISP_IDMA_BUF_TS1_W1SC_MASK 0x00000000U
/**
 * timestamp [31:0], dòng bảng 571, kiểu truy cập RW
 * Thông tin frame của bộ đệm 1. Giá trị đầy đủ 32 bit, hard reset về 0. Mốc chụp 32 bit theo gốc
 * thời gian FRAME_COUNTER, ghi trước IDMA_BUF_VALID và đi cùng nhãn frame; xử lý cuộn modulo 2^32.
 */
#define FX1_ISP_IDMA_BUF_TS1_TIMESTAMP_SHIFT 0U
#define FX1_ISP_IDMA_BUF_TS1_TIMESTAMP_WIDTH 32U
#define FX1_ISP_IDMA_BUF_TS1_TIMESTAMP_MASK 0xFFFFFFFFU
#define FX1_ISP_IDMA_BUF_TS1_TIMESTAMP_RESET 0x00000000U
#define FX1_ISP_IDMA_BUF_TS1_TIMESTAMP_ACCESS FX1_ISP_ACCESS_RW

/**
 * IDMA_BUF_TS2: địa chỉ lệch 0x3118, dòng bảng 572, kiểu truy cập RW
 * Thông tin frame của bộ đệm 2. Mốc chụp 32 bit theo gốc thời gian FRAME_COUNTER, ghi trước
 * IDMA_BUF_VALID và đi cùng nhãn frame; xử lý cuộn modulo 2^32.
 */
#define FX1_ISP_IDMA_BUF_TS2_OFFSET 0x3118U
#define REG_IDMA_BUF_TS2 FX1_ISP_IDMA_BUF_TS2_OFFSET
#define FX1_ISP_IDMA_BUF_TS2_RESET 0x00000000U
#define FX1_ISP_IDMA_BUF_TS2_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_IDMA_BUF_TS2_RO_MASK 0x00000000U
#define FX1_ISP_IDMA_BUF_TS2_RW_MASK 0xFFFFFFFFU
#define FX1_ISP_IDMA_BUF_TS2_W1C_MASK 0x00000000U
#define FX1_ISP_IDMA_BUF_TS2_W1S_MASK 0x00000000U
#define FX1_ISP_IDMA_BUF_TS2_W1P_MASK 0x00000000U
#define FX1_ISP_IDMA_BUF_TS2_W1SC_MASK 0x00000000U
/**
 * timestamp [31:0], dòng bảng 573, kiểu truy cập RW
 * Thông tin frame của bộ đệm 2. Giá trị đầy đủ 32 bit, hard reset về 0. Mốc chụp 32 bit theo gốc
 * thời gian FRAME_COUNTER, ghi trước IDMA_BUF_VALID và đi cùng nhãn frame; xử lý cuộn modulo 2^32.
 */
#define FX1_ISP_IDMA_BUF_TS2_TIMESTAMP_SHIFT 0U
#define FX1_ISP_IDMA_BUF_TS2_TIMESTAMP_WIDTH 32U
#define FX1_ISP_IDMA_BUF_TS2_TIMESTAMP_MASK 0xFFFFFFFFU
#define FX1_ISP_IDMA_BUF_TS2_TIMESTAMP_RESET 0x00000000U
#define FX1_ISP_IDMA_BUF_TS2_TIMESTAMP_ACCESS FX1_ISP_ACCESS_RW

/**
 * IDMA_BUF_TS3: địa chỉ lệch 0x311C, dòng bảng 574, kiểu truy cập RW
 * Thông tin frame của bộ đệm 3. Mốc chụp 32 bit theo gốc thời gian FRAME_COUNTER, ghi trước
 * IDMA_BUF_VALID và đi cùng nhãn frame; xử lý cuộn modulo 2^32.
 */
#define FX1_ISP_IDMA_BUF_TS3_OFFSET 0x311CU
#define REG_IDMA_BUF_TS3 FX1_ISP_IDMA_BUF_TS3_OFFSET
#define FX1_ISP_IDMA_BUF_TS3_RESET 0x00000000U
#define FX1_ISP_IDMA_BUF_TS3_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_IDMA_BUF_TS3_RO_MASK 0x00000000U
#define FX1_ISP_IDMA_BUF_TS3_RW_MASK 0xFFFFFFFFU
#define FX1_ISP_IDMA_BUF_TS3_W1C_MASK 0x00000000U
#define FX1_ISP_IDMA_BUF_TS3_W1S_MASK 0x00000000U
#define FX1_ISP_IDMA_BUF_TS3_W1P_MASK 0x00000000U
#define FX1_ISP_IDMA_BUF_TS3_W1SC_MASK 0x00000000U
/**
 * timestamp [31:0], dòng bảng 575, kiểu truy cập RW
 * Thông tin frame của bộ đệm 3. Giá trị đầy đủ 32 bit, hard reset về 0. Mốc chụp 32 bit theo gốc
 * thời gian FRAME_COUNTER, ghi trước IDMA_BUF_VALID và đi cùng nhãn frame; xử lý cuộn modulo 2^32.
 */
#define FX1_ISP_IDMA_BUF_TS3_TIMESTAMP_SHIFT 0U
#define FX1_ISP_IDMA_BUF_TS3_TIMESTAMP_WIDTH 32U
#define FX1_ISP_IDMA_BUF_TS3_TIMESTAMP_MASK 0xFFFFFFFFU
#define FX1_ISP_IDMA_BUF_TS3_TIMESTAMP_RESET 0x00000000U
#define FX1_ISP_IDMA_BUF_TS3_TIMESTAMP_ACCESS FX1_ISP_ACCESS_RW

/**
 * ODMA_BUF_TAG0: địa chỉ lệch 0x3120, dòng bảng 576, kiểu truy cập RO
 * Thông tin frame của bộ đệm 0. Nhãn frame vào gắn với buffer ra hoàn tất; công bố khi frame ra kết
 * thúc. Chỉ số buffer vào/ra không bắt buộc khớp.
 */
#define FX1_ISP_ODMA_BUF_TAG0_OFFSET 0x3120U
#define REG_ODMA_BUF_TAG0 FX1_ISP_ODMA_BUF_TAG0_OFFSET
#define FX1_ISP_ODMA_BUF_TAG0_RESET 0x00000000U
#define FX1_ISP_ODMA_BUF_TAG0_ACCESS FX1_ISP_ACCESS_RO
#define FX1_ISP_ODMA_BUF_TAG0_RO_MASK 0xFFFFFFFFU
#define FX1_ISP_ODMA_BUF_TAG0_RW_MASK 0x00000000U
#define FX1_ISP_ODMA_BUF_TAG0_W1C_MASK 0x00000000U
#define FX1_ISP_ODMA_BUF_TAG0_W1S_MASK 0x00000000U
#define FX1_ISP_ODMA_BUF_TAG0_W1P_MASK 0x00000000U
#define FX1_ISP_ODMA_BUF_TAG0_W1SC_MASK 0x00000000U
/**
 * tag [31:0], dòng bảng 577, kiểu truy cập RO
 * Thông tin frame của bộ đệm 0. Giá trị đầy đủ 32 bit, hard reset về 0. Nhãn frame vào gắn với
 * buffer ra hoàn tất; công bố khi frame ra kết thúc. Chỉ số buffer vào/ra không bắt buộc khớp.
 */
#define FX1_ISP_ODMA_BUF_TAG0_TAG_SHIFT 0U
#define FX1_ISP_ODMA_BUF_TAG0_TAG_WIDTH 32U
#define FX1_ISP_ODMA_BUF_TAG0_TAG_MASK 0xFFFFFFFFU
#define FX1_ISP_ODMA_BUF_TAG0_TAG_RESET 0x00000000U
#define FX1_ISP_ODMA_BUF_TAG0_TAG_ACCESS FX1_ISP_ACCESS_RO

/**
 * ODMA_BUF_TAG1: địa chỉ lệch 0x3124, dòng bảng 578, kiểu truy cập RO
 * Thông tin frame của bộ đệm 1. Nhãn frame vào gắn với buffer ra hoàn tất; công bố khi frame ra kết
 * thúc. Chỉ số buffer vào/ra không bắt buộc khớp.
 */
#define FX1_ISP_ODMA_BUF_TAG1_OFFSET 0x3124U
#define REG_ODMA_BUF_TAG1 FX1_ISP_ODMA_BUF_TAG1_OFFSET
#define FX1_ISP_ODMA_BUF_TAG1_RESET 0x00000000U
#define FX1_ISP_ODMA_BUF_TAG1_ACCESS FX1_ISP_ACCESS_RO
#define FX1_ISP_ODMA_BUF_TAG1_RO_MASK 0xFFFFFFFFU
#define FX1_ISP_ODMA_BUF_TAG1_RW_MASK 0x00000000U
#define FX1_ISP_ODMA_BUF_TAG1_W1C_MASK 0x00000000U
#define FX1_ISP_ODMA_BUF_TAG1_W1S_MASK 0x00000000U
#define FX1_ISP_ODMA_BUF_TAG1_W1P_MASK 0x00000000U
#define FX1_ISP_ODMA_BUF_TAG1_W1SC_MASK 0x00000000U
/**
 * tag [31:0], dòng bảng 579, kiểu truy cập RO
 * Thông tin frame của bộ đệm 1. Giá trị đầy đủ 32 bit, hard reset về 0. Nhãn frame vào gắn với
 * buffer ra hoàn tất; công bố khi frame ra kết thúc. Chỉ số buffer vào/ra không bắt buộc khớp.
 */
#define FX1_ISP_ODMA_BUF_TAG1_TAG_SHIFT 0U
#define FX1_ISP_ODMA_BUF_TAG1_TAG_WIDTH 32U
#define FX1_ISP_ODMA_BUF_TAG1_TAG_MASK 0xFFFFFFFFU
#define FX1_ISP_ODMA_BUF_TAG1_TAG_RESET 0x00000000U
#define FX1_ISP_ODMA_BUF_TAG1_TAG_ACCESS FX1_ISP_ACCESS_RO

/**
 * ODMA_BUF_TAG2: địa chỉ lệch 0x3128, dòng bảng 580, kiểu truy cập RO
 * Thông tin frame của bộ đệm 2. Nhãn frame vào gắn với buffer ra hoàn tất; công bố khi frame ra kết
 * thúc. Chỉ số buffer vào/ra không bắt buộc khớp.
 */
#define FX1_ISP_ODMA_BUF_TAG2_OFFSET 0x3128U
#define REG_ODMA_BUF_TAG2 FX1_ISP_ODMA_BUF_TAG2_OFFSET
#define FX1_ISP_ODMA_BUF_TAG2_RESET 0x00000000U
#define FX1_ISP_ODMA_BUF_TAG2_ACCESS FX1_ISP_ACCESS_RO
#define FX1_ISP_ODMA_BUF_TAG2_RO_MASK 0xFFFFFFFFU
#define FX1_ISP_ODMA_BUF_TAG2_RW_MASK 0x00000000U
#define FX1_ISP_ODMA_BUF_TAG2_W1C_MASK 0x00000000U
#define FX1_ISP_ODMA_BUF_TAG2_W1S_MASK 0x00000000U
#define FX1_ISP_ODMA_BUF_TAG2_W1P_MASK 0x00000000U
#define FX1_ISP_ODMA_BUF_TAG2_W1SC_MASK 0x00000000U
/**
 * tag [31:0], dòng bảng 581, kiểu truy cập RO
 * Thông tin frame của bộ đệm 2. Giá trị đầy đủ 32 bit, hard reset về 0. Nhãn frame vào gắn với
 * buffer ra hoàn tất; công bố khi frame ra kết thúc. Chỉ số buffer vào/ra không bắt buộc khớp.
 */
#define FX1_ISP_ODMA_BUF_TAG2_TAG_SHIFT 0U
#define FX1_ISP_ODMA_BUF_TAG2_TAG_WIDTH 32U
#define FX1_ISP_ODMA_BUF_TAG2_TAG_MASK 0xFFFFFFFFU
#define FX1_ISP_ODMA_BUF_TAG2_TAG_RESET 0x00000000U
#define FX1_ISP_ODMA_BUF_TAG2_TAG_ACCESS FX1_ISP_ACCESS_RO

/**
 * ODMA_BUF_TAG3: địa chỉ lệch 0x312C, dòng bảng 582, kiểu truy cập RO
 * Thông tin frame của bộ đệm 3. Nhãn frame vào gắn với buffer ra hoàn tất; công bố khi frame ra kết
 * thúc. Chỉ số buffer vào/ra không bắt buộc khớp.
 */
#define FX1_ISP_ODMA_BUF_TAG3_OFFSET 0x312CU
#define REG_ODMA_BUF_TAG3 FX1_ISP_ODMA_BUF_TAG3_OFFSET
#define FX1_ISP_ODMA_BUF_TAG3_RESET 0x00000000U
#define FX1_ISP_ODMA_BUF_TAG3_ACCESS FX1_ISP_ACCESS_RO
#define FX1_ISP_ODMA_BUF_TAG3_RO_MASK 0xFFFFFFFFU
#define FX1_ISP_ODMA_BUF_TAG3_RW_MASK 0x00000000U
#define FX1_ISP_ODMA_BUF_TAG3_W1C_MASK 0x00000000U
#define FX1_ISP_ODMA_BUF_TAG3_W1S_MASK 0x00000000U
#define FX1_ISP_ODMA_BUF_TAG3_W1P_MASK 0x00000000U
#define FX1_ISP_ODMA_BUF_TAG3_W1SC_MASK 0x00000000U
/**
 * tag [31:0], dòng bảng 583, kiểu truy cập RO
 * Thông tin frame của bộ đệm 3. Giá trị đầy đủ 32 bit, hard reset về 0. Nhãn frame vào gắn với
 * buffer ra hoàn tất; công bố khi frame ra kết thúc. Chỉ số buffer vào/ra không bắt buộc khớp.
 */
#define FX1_ISP_ODMA_BUF_TAG3_TAG_SHIFT 0U
#define FX1_ISP_ODMA_BUF_TAG3_TAG_WIDTH 32U
#define FX1_ISP_ODMA_BUF_TAG3_TAG_MASK 0xFFFFFFFFU
#define FX1_ISP_ODMA_BUF_TAG3_TAG_RESET 0x00000000U
#define FX1_ISP_ODMA_BUF_TAG3_TAG_ACCESS FX1_ISP_ACCESS_RO

/**
 * ODMA_BUF_TS0: địa chỉ lệch 0x3130, dòng bảng 584, kiểu truy cập RO
 * Thông tin frame của bộ đệm 0. Mốc chụp sao chép cùng nhãn đầu vào và công bố khi đầu ra hoàn tất;
 * cùng gốc thời gian clock lõi như FRAME_COUNTER.
 */
#define FX1_ISP_ODMA_BUF_TS0_OFFSET 0x3130U
#define REG_ODMA_BUF_TS0 FX1_ISP_ODMA_BUF_TS0_OFFSET
#define FX1_ISP_ODMA_BUF_TS0_RESET 0x00000000U
#define FX1_ISP_ODMA_BUF_TS0_ACCESS FX1_ISP_ACCESS_RO
#define FX1_ISP_ODMA_BUF_TS0_RO_MASK 0xFFFFFFFFU
#define FX1_ISP_ODMA_BUF_TS0_RW_MASK 0x00000000U
#define FX1_ISP_ODMA_BUF_TS0_W1C_MASK 0x00000000U
#define FX1_ISP_ODMA_BUF_TS0_W1S_MASK 0x00000000U
#define FX1_ISP_ODMA_BUF_TS0_W1P_MASK 0x00000000U
#define FX1_ISP_ODMA_BUF_TS0_W1SC_MASK 0x00000000U
/**
 * timestamp [31:0], dòng bảng 585, kiểu truy cập RO
 * Thông tin frame của bộ đệm 0. Giá trị đầy đủ 32 bit, hard reset về 0. Mốc chụp sao chép cùng nhãn
 * đầu vào và công bố khi đầu ra hoàn tất; cùng gốc thời gian clock lõi như FRAME_COUNTER.
 */
#define FX1_ISP_ODMA_BUF_TS0_TIMESTAMP_SHIFT 0U
#define FX1_ISP_ODMA_BUF_TS0_TIMESTAMP_WIDTH 32U
#define FX1_ISP_ODMA_BUF_TS0_TIMESTAMP_MASK 0xFFFFFFFFU
#define FX1_ISP_ODMA_BUF_TS0_TIMESTAMP_RESET 0x00000000U
#define FX1_ISP_ODMA_BUF_TS0_TIMESTAMP_ACCESS FX1_ISP_ACCESS_RO

/**
 * ODMA_BUF_TS1: địa chỉ lệch 0x3134, dòng bảng 586, kiểu truy cập RO
 * Thông tin frame của bộ đệm 1. Mốc chụp sao chép cùng nhãn đầu vào và công bố khi đầu ra hoàn tất;
 * cùng gốc thời gian clock lõi như FRAME_COUNTER.
 */
#define FX1_ISP_ODMA_BUF_TS1_OFFSET 0x3134U
#define REG_ODMA_BUF_TS1 FX1_ISP_ODMA_BUF_TS1_OFFSET
#define FX1_ISP_ODMA_BUF_TS1_RESET 0x00000000U
#define FX1_ISP_ODMA_BUF_TS1_ACCESS FX1_ISP_ACCESS_RO
#define FX1_ISP_ODMA_BUF_TS1_RO_MASK 0xFFFFFFFFU
#define FX1_ISP_ODMA_BUF_TS1_RW_MASK 0x00000000U
#define FX1_ISP_ODMA_BUF_TS1_W1C_MASK 0x00000000U
#define FX1_ISP_ODMA_BUF_TS1_W1S_MASK 0x00000000U
#define FX1_ISP_ODMA_BUF_TS1_W1P_MASK 0x00000000U
#define FX1_ISP_ODMA_BUF_TS1_W1SC_MASK 0x00000000U
/**
 * timestamp [31:0], dòng bảng 587, kiểu truy cập RO
 * Thông tin frame của bộ đệm 1. Giá trị đầy đủ 32 bit, hard reset về 0. Mốc chụp sao chép cùng nhãn
 * đầu vào và công bố khi đầu ra hoàn tất; cùng gốc thời gian clock lõi như FRAME_COUNTER.
 */
#define FX1_ISP_ODMA_BUF_TS1_TIMESTAMP_SHIFT 0U
#define FX1_ISP_ODMA_BUF_TS1_TIMESTAMP_WIDTH 32U
#define FX1_ISP_ODMA_BUF_TS1_TIMESTAMP_MASK 0xFFFFFFFFU
#define FX1_ISP_ODMA_BUF_TS1_TIMESTAMP_RESET 0x00000000U
#define FX1_ISP_ODMA_BUF_TS1_TIMESTAMP_ACCESS FX1_ISP_ACCESS_RO

/**
 * ODMA_BUF_TS2: địa chỉ lệch 0x3138, dòng bảng 588, kiểu truy cập RO
 * Thông tin frame của bộ đệm 2. Mốc chụp sao chép cùng nhãn đầu vào và công bố khi đầu ra hoàn tất;
 * cùng gốc thời gian clock lõi như FRAME_COUNTER.
 */
#define FX1_ISP_ODMA_BUF_TS2_OFFSET 0x3138U
#define REG_ODMA_BUF_TS2 FX1_ISP_ODMA_BUF_TS2_OFFSET
#define FX1_ISP_ODMA_BUF_TS2_RESET 0x00000000U
#define FX1_ISP_ODMA_BUF_TS2_ACCESS FX1_ISP_ACCESS_RO
#define FX1_ISP_ODMA_BUF_TS2_RO_MASK 0xFFFFFFFFU
#define FX1_ISP_ODMA_BUF_TS2_RW_MASK 0x00000000U
#define FX1_ISP_ODMA_BUF_TS2_W1C_MASK 0x00000000U
#define FX1_ISP_ODMA_BUF_TS2_W1S_MASK 0x00000000U
#define FX1_ISP_ODMA_BUF_TS2_W1P_MASK 0x00000000U
#define FX1_ISP_ODMA_BUF_TS2_W1SC_MASK 0x00000000U
/**
 * timestamp [31:0], dòng bảng 589, kiểu truy cập RO
 * Thông tin frame của bộ đệm 2. Giá trị đầy đủ 32 bit, hard reset về 0. Mốc chụp sao chép cùng nhãn
 * đầu vào và công bố khi đầu ra hoàn tất; cùng gốc thời gian clock lõi như FRAME_COUNTER.
 */
#define FX1_ISP_ODMA_BUF_TS2_TIMESTAMP_SHIFT 0U
#define FX1_ISP_ODMA_BUF_TS2_TIMESTAMP_WIDTH 32U
#define FX1_ISP_ODMA_BUF_TS2_TIMESTAMP_MASK 0xFFFFFFFFU
#define FX1_ISP_ODMA_BUF_TS2_TIMESTAMP_RESET 0x00000000U
#define FX1_ISP_ODMA_BUF_TS2_TIMESTAMP_ACCESS FX1_ISP_ACCESS_RO

/**
 * ODMA_BUF_TS3: địa chỉ lệch 0x313C, dòng bảng 590, kiểu truy cập RO
 * Thông tin frame của bộ đệm 3. Mốc chụp sao chép cùng nhãn đầu vào và công bố khi đầu ra hoàn tất;
 * cùng gốc thời gian clock lõi như FRAME_COUNTER.
 */
#define FX1_ISP_ODMA_BUF_TS3_OFFSET 0x313CU
#define REG_ODMA_BUF_TS3 FX1_ISP_ODMA_BUF_TS3_OFFSET
#define FX1_ISP_ODMA_BUF_TS3_RESET 0x00000000U
#define FX1_ISP_ODMA_BUF_TS3_ACCESS FX1_ISP_ACCESS_RO
#define FX1_ISP_ODMA_BUF_TS3_RO_MASK 0xFFFFFFFFU
#define FX1_ISP_ODMA_BUF_TS3_RW_MASK 0x00000000U
#define FX1_ISP_ODMA_BUF_TS3_W1C_MASK 0x00000000U
#define FX1_ISP_ODMA_BUF_TS3_W1S_MASK 0x00000000U
#define FX1_ISP_ODMA_BUF_TS3_W1P_MASK 0x00000000U
#define FX1_ISP_ODMA_BUF_TS3_W1SC_MASK 0x00000000U
/**
 * timestamp [31:0], dòng bảng 591, kiểu truy cập RO
 * Thông tin frame của bộ đệm 3. Giá trị đầy đủ 32 bit, hard reset về 0. Mốc chụp sao chép cùng nhãn
 * đầu vào và công bố khi đầu ra hoàn tất; cùng gốc thời gian clock lõi như FRAME_COUNTER.
 */
#define FX1_ISP_ODMA_BUF_TS3_TIMESTAMP_SHIFT 0U
#define FX1_ISP_ODMA_BUF_TS3_TIMESTAMP_WIDTH 32U
#define FX1_ISP_ODMA_BUF_TS3_TIMESTAMP_MASK 0xFFFFFFFFU
#define FX1_ISP_ODMA_BUF_TS3_TIMESTAMP_RESET 0x00000000U
#define FX1_ISP_ODMA_BUF_TS3_TIMESTAMP_ACCESS FX1_ISP_ACCESS_RO

/**
 * ODMA_BUF_SOF_TS0: địa chỉ lệch 0x3140, dòng bảng 592, kiểu truy cập RO
 * Mốc thời gian clock lõi của buffer 0, chốt ở SOF đầu ra và công bố khi hoàn tất; kết hợp EOF_TS để
 * tính thời gian xử lý frame.
 */
#define FX1_ISP_ODMA_BUF_SOF_TS0_OFFSET 0x3140U
#define REG_ODMA_BUF_SOF_TS0 FX1_ISP_ODMA_BUF_SOF_TS0_OFFSET
#define FX1_ISP_ODMA_BUF_SOF_TS0_RESET 0x00000000U
#define FX1_ISP_ODMA_BUF_SOF_TS0_ACCESS FX1_ISP_ACCESS_RO
#define FX1_ISP_ODMA_BUF_SOF_TS0_RO_MASK 0xFFFFFFFFU
#define FX1_ISP_ODMA_BUF_SOF_TS0_RW_MASK 0x00000000U
#define FX1_ISP_ODMA_BUF_SOF_TS0_W1C_MASK 0x00000000U
#define FX1_ISP_ODMA_BUF_SOF_TS0_W1S_MASK 0x00000000U
#define FX1_ISP_ODMA_BUF_SOF_TS0_W1P_MASK 0x00000000U
#define FX1_ISP_ODMA_BUF_SOF_TS0_W1SC_MASK 0x00000000U
/**
 * timestamp [31:0], dòng bảng 593, kiểu truy cập RO
 * Mốc thời gian clock lõi của buffer 0, giá trị 32 bit hard reset 0, chốt ở SOF đầu ra và công bố
 * khi hoàn tất; kết hợp EOF_TS để tính thời gian xử lý frame.
 */
#define FX1_ISP_ODMA_BUF_SOF_TS0_TIMESTAMP_SHIFT 0U
#define FX1_ISP_ODMA_BUF_SOF_TS0_TIMESTAMP_WIDTH 32U
#define FX1_ISP_ODMA_BUF_SOF_TS0_TIMESTAMP_MASK 0xFFFFFFFFU
#define FX1_ISP_ODMA_BUF_SOF_TS0_TIMESTAMP_RESET 0x00000000U
#define FX1_ISP_ODMA_BUF_SOF_TS0_TIMESTAMP_ACCESS FX1_ISP_ACCESS_RO

/**
 * ODMA_BUF_SOF_TS1: địa chỉ lệch 0x3144, dòng bảng 594, kiểu truy cập RO
 * Mốc thời gian clock lõi của buffer 1, chốt ở SOF đầu ra và công bố khi hoàn tất; kết hợp EOF_TS để
 * tính thời gian xử lý frame.
 */
#define FX1_ISP_ODMA_BUF_SOF_TS1_OFFSET 0x3144U
#define REG_ODMA_BUF_SOF_TS1 FX1_ISP_ODMA_BUF_SOF_TS1_OFFSET
#define FX1_ISP_ODMA_BUF_SOF_TS1_RESET 0x00000000U
#define FX1_ISP_ODMA_BUF_SOF_TS1_ACCESS FX1_ISP_ACCESS_RO
#define FX1_ISP_ODMA_BUF_SOF_TS1_RO_MASK 0xFFFFFFFFU
#define FX1_ISP_ODMA_BUF_SOF_TS1_RW_MASK 0x00000000U
#define FX1_ISP_ODMA_BUF_SOF_TS1_W1C_MASK 0x00000000U
#define FX1_ISP_ODMA_BUF_SOF_TS1_W1S_MASK 0x00000000U
#define FX1_ISP_ODMA_BUF_SOF_TS1_W1P_MASK 0x00000000U
#define FX1_ISP_ODMA_BUF_SOF_TS1_W1SC_MASK 0x00000000U
/**
 * timestamp [31:0], dòng bảng 595, kiểu truy cập RO
 * Mốc thời gian clock lõi của buffer 1, giá trị 32 bit hard reset 0, chốt ở SOF đầu ra và công bố
 * khi hoàn tất; kết hợp EOF_TS để tính thời gian xử lý frame.
 */
#define FX1_ISP_ODMA_BUF_SOF_TS1_TIMESTAMP_SHIFT 0U
#define FX1_ISP_ODMA_BUF_SOF_TS1_TIMESTAMP_WIDTH 32U
#define FX1_ISP_ODMA_BUF_SOF_TS1_TIMESTAMP_MASK 0xFFFFFFFFU
#define FX1_ISP_ODMA_BUF_SOF_TS1_TIMESTAMP_RESET 0x00000000U
#define FX1_ISP_ODMA_BUF_SOF_TS1_TIMESTAMP_ACCESS FX1_ISP_ACCESS_RO

/**
 * ODMA_BUF_SOF_TS2: địa chỉ lệch 0x3148, dòng bảng 596, kiểu truy cập RO
 * Mốc thời gian clock lõi của buffer 2, chốt ở SOF đầu ra và công bố khi hoàn tất; kết hợp EOF_TS để
 * tính thời gian xử lý frame.
 */
#define FX1_ISP_ODMA_BUF_SOF_TS2_OFFSET 0x3148U
#define REG_ODMA_BUF_SOF_TS2 FX1_ISP_ODMA_BUF_SOF_TS2_OFFSET
#define FX1_ISP_ODMA_BUF_SOF_TS2_RESET 0x00000000U
#define FX1_ISP_ODMA_BUF_SOF_TS2_ACCESS FX1_ISP_ACCESS_RO
#define FX1_ISP_ODMA_BUF_SOF_TS2_RO_MASK 0xFFFFFFFFU
#define FX1_ISP_ODMA_BUF_SOF_TS2_RW_MASK 0x00000000U
#define FX1_ISP_ODMA_BUF_SOF_TS2_W1C_MASK 0x00000000U
#define FX1_ISP_ODMA_BUF_SOF_TS2_W1S_MASK 0x00000000U
#define FX1_ISP_ODMA_BUF_SOF_TS2_W1P_MASK 0x00000000U
#define FX1_ISP_ODMA_BUF_SOF_TS2_W1SC_MASK 0x00000000U
/**
 * timestamp [31:0], dòng bảng 597, kiểu truy cập RO
 * Mốc thời gian clock lõi của buffer 2, giá trị 32 bit hard reset 0, chốt ở SOF đầu ra và công bố
 * khi hoàn tất; kết hợp EOF_TS để tính thời gian xử lý frame.
 */
#define FX1_ISP_ODMA_BUF_SOF_TS2_TIMESTAMP_SHIFT 0U
#define FX1_ISP_ODMA_BUF_SOF_TS2_TIMESTAMP_WIDTH 32U
#define FX1_ISP_ODMA_BUF_SOF_TS2_TIMESTAMP_MASK 0xFFFFFFFFU
#define FX1_ISP_ODMA_BUF_SOF_TS2_TIMESTAMP_RESET 0x00000000U
#define FX1_ISP_ODMA_BUF_SOF_TS2_TIMESTAMP_ACCESS FX1_ISP_ACCESS_RO

/**
 * ODMA_BUF_SOF_TS3: địa chỉ lệch 0x314C, dòng bảng 598, kiểu truy cập RO
 * Mốc thời gian clock lõi của buffer 3, chốt ở SOF đầu ra và công bố khi hoàn tất; kết hợp EOF_TS để
 * tính thời gian xử lý frame.
 */
#define FX1_ISP_ODMA_BUF_SOF_TS3_OFFSET 0x314CU
#define REG_ODMA_BUF_SOF_TS3 FX1_ISP_ODMA_BUF_SOF_TS3_OFFSET
#define FX1_ISP_ODMA_BUF_SOF_TS3_RESET 0x00000000U
#define FX1_ISP_ODMA_BUF_SOF_TS3_ACCESS FX1_ISP_ACCESS_RO
#define FX1_ISP_ODMA_BUF_SOF_TS3_RO_MASK 0xFFFFFFFFU
#define FX1_ISP_ODMA_BUF_SOF_TS3_RW_MASK 0x00000000U
#define FX1_ISP_ODMA_BUF_SOF_TS3_W1C_MASK 0x00000000U
#define FX1_ISP_ODMA_BUF_SOF_TS3_W1S_MASK 0x00000000U
#define FX1_ISP_ODMA_BUF_SOF_TS3_W1P_MASK 0x00000000U
#define FX1_ISP_ODMA_BUF_SOF_TS3_W1SC_MASK 0x00000000U
/**
 * timestamp [31:0], dòng bảng 599, kiểu truy cập RO
 * Mốc thời gian clock lõi của buffer 3, giá trị 32 bit hard reset 0, chốt ở SOF đầu ra và công bố
 * khi hoàn tất; kết hợp EOF_TS để tính thời gian xử lý frame.
 */
#define FX1_ISP_ODMA_BUF_SOF_TS3_TIMESTAMP_SHIFT 0U
#define FX1_ISP_ODMA_BUF_SOF_TS3_TIMESTAMP_WIDTH 32U
#define FX1_ISP_ODMA_BUF_SOF_TS3_TIMESTAMP_MASK 0xFFFFFFFFU
#define FX1_ISP_ODMA_BUF_SOF_TS3_TIMESTAMP_RESET 0x00000000U
#define FX1_ISP_ODMA_BUF_SOF_TS3_TIMESTAMP_ACCESS FX1_ISP_ACCESS_RO

/**
 * ODMA_BUF_EOF_TS0: địa chỉ lệch 0x3150, dòng bảng 600, kiểu truy cập RO
 * Mốc thời gian clock lõi của buffer 0, chốt ở EOF đầu ra và công bố khi hoàn tất; trừ SOF_TS hoặc
 * mốc chụp modulo 2^32 để tính thời gian xử lý/độ trễ đầu cuối.
 */
#define FX1_ISP_ODMA_BUF_EOF_TS0_OFFSET 0x3150U
#define REG_ODMA_BUF_EOF_TS0 FX1_ISP_ODMA_BUF_EOF_TS0_OFFSET
#define FX1_ISP_ODMA_BUF_EOF_TS0_RESET 0x00000000U
#define FX1_ISP_ODMA_BUF_EOF_TS0_ACCESS FX1_ISP_ACCESS_RO
#define FX1_ISP_ODMA_BUF_EOF_TS0_RO_MASK 0xFFFFFFFFU
#define FX1_ISP_ODMA_BUF_EOF_TS0_RW_MASK 0x00000000U
#define FX1_ISP_ODMA_BUF_EOF_TS0_W1C_MASK 0x00000000U
#define FX1_ISP_ODMA_BUF_EOF_TS0_W1S_MASK 0x00000000U
#define FX1_ISP_ODMA_BUF_EOF_TS0_W1P_MASK 0x00000000U
#define FX1_ISP_ODMA_BUF_EOF_TS0_W1SC_MASK 0x00000000U
/**
 * timestamp [31:0], dòng bảng 601, kiểu truy cập RO
 * Mốc thời gian clock lõi của buffer 0, giá trị 32 bit hard reset 0, chốt ở EOF đầu ra và công bố
 * khi hoàn tất; trừ SOF_TS hoặc mốc chụp modulo 2^32 để tính thời gian xử lý/độ trễ đầu cuối.
 */
#define FX1_ISP_ODMA_BUF_EOF_TS0_TIMESTAMP_SHIFT 0U
#define FX1_ISP_ODMA_BUF_EOF_TS0_TIMESTAMP_WIDTH 32U
#define FX1_ISP_ODMA_BUF_EOF_TS0_TIMESTAMP_MASK 0xFFFFFFFFU
#define FX1_ISP_ODMA_BUF_EOF_TS0_TIMESTAMP_RESET 0x00000000U
#define FX1_ISP_ODMA_BUF_EOF_TS0_TIMESTAMP_ACCESS FX1_ISP_ACCESS_RO

/**
 * ODMA_BUF_EOF_TS1: địa chỉ lệch 0x3154, dòng bảng 602, kiểu truy cập RO
 * Mốc thời gian clock lõi của buffer 1, chốt ở EOF đầu ra và công bố khi hoàn tất; trừ SOF_TS hoặc
 * mốc chụp modulo 2^32 để tính thời gian xử lý/độ trễ đầu cuối.
 */
#define FX1_ISP_ODMA_BUF_EOF_TS1_OFFSET 0x3154U
#define REG_ODMA_BUF_EOF_TS1 FX1_ISP_ODMA_BUF_EOF_TS1_OFFSET
#define FX1_ISP_ODMA_BUF_EOF_TS1_RESET 0x00000000U
#define FX1_ISP_ODMA_BUF_EOF_TS1_ACCESS FX1_ISP_ACCESS_RO
#define FX1_ISP_ODMA_BUF_EOF_TS1_RO_MASK 0xFFFFFFFFU
#define FX1_ISP_ODMA_BUF_EOF_TS1_RW_MASK 0x00000000U
#define FX1_ISP_ODMA_BUF_EOF_TS1_W1C_MASK 0x00000000U
#define FX1_ISP_ODMA_BUF_EOF_TS1_W1S_MASK 0x00000000U
#define FX1_ISP_ODMA_BUF_EOF_TS1_W1P_MASK 0x00000000U
#define FX1_ISP_ODMA_BUF_EOF_TS1_W1SC_MASK 0x00000000U
/**
 * timestamp [31:0], dòng bảng 603, kiểu truy cập RO
 * Mốc thời gian clock lõi của buffer 1, giá trị 32 bit hard reset 0, chốt ở EOF đầu ra và công bố
 * khi hoàn tất; trừ SOF_TS hoặc mốc chụp modulo 2^32 để tính thời gian xử lý/độ trễ đầu cuối.
 */
#define FX1_ISP_ODMA_BUF_EOF_TS1_TIMESTAMP_SHIFT 0U
#define FX1_ISP_ODMA_BUF_EOF_TS1_TIMESTAMP_WIDTH 32U
#define FX1_ISP_ODMA_BUF_EOF_TS1_TIMESTAMP_MASK 0xFFFFFFFFU
#define FX1_ISP_ODMA_BUF_EOF_TS1_TIMESTAMP_RESET 0x00000000U
#define FX1_ISP_ODMA_BUF_EOF_TS1_TIMESTAMP_ACCESS FX1_ISP_ACCESS_RO

/**
 * ODMA_BUF_EOF_TS2: địa chỉ lệch 0x3158, dòng bảng 604, kiểu truy cập RO
 * Mốc thời gian clock lõi của buffer 2, chốt ở EOF đầu ra và công bố khi hoàn tất; trừ SOF_TS hoặc
 * mốc chụp modulo 2^32 để tính thời gian xử lý/độ trễ đầu cuối.
 */
#define FX1_ISP_ODMA_BUF_EOF_TS2_OFFSET 0x3158U
#define REG_ODMA_BUF_EOF_TS2 FX1_ISP_ODMA_BUF_EOF_TS2_OFFSET
#define FX1_ISP_ODMA_BUF_EOF_TS2_RESET 0x00000000U
#define FX1_ISP_ODMA_BUF_EOF_TS2_ACCESS FX1_ISP_ACCESS_RO
#define FX1_ISP_ODMA_BUF_EOF_TS2_RO_MASK 0xFFFFFFFFU
#define FX1_ISP_ODMA_BUF_EOF_TS2_RW_MASK 0x00000000U
#define FX1_ISP_ODMA_BUF_EOF_TS2_W1C_MASK 0x00000000U
#define FX1_ISP_ODMA_BUF_EOF_TS2_W1S_MASK 0x00000000U
#define FX1_ISP_ODMA_BUF_EOF_TS2_W1P_MASK 0x00000000U
#define FX1_ISP_ODMA_BUF_EOF_TS2_W1SC_MASK 0x00000000U
/**
 * timestamp [31:0], dòng bảng 605, kiểu truy cập RO
 * Mốc thời gian clock lõi của buffer 2, giá trị 32 bit hard reset 0, chốt ở EOF đầu ra và công bố
 * khi hoàn tất; trừ SOF_TS hoặc mốc chụp modulo 2^32 để tính thời gian xử lý/độ trễ đầu cuối.
 */
#define FX1_ISP_ODMA_BUF_EOF_TS2_TIMESTAMP_SHIFT 0U
#define FX1_ISP_ODMA_BUF_EOF_TS2_TIMESTAMP_WIDTH 32U
#define FX1_ISP_ODMA_BUF_EOF_TS2_TIMESTAMP_MASK 0xFFFFFFFFU
#define FX1_ISP_ODMA_BUF_EOF_TS2_TIMESTAMP_RESET 0x00000000U
#define FX1_ISP_ODMA_BUF_EOF_TS2_TIMESTAMP_ACCESS FX1_ISP_ACCESS_RO

/**
 * ODMA_BUF_EOF_TS3: địa chỉ lệch 0x315C, dòng bảng 606, kiểu truy cập RO
 * Mốc thời gian clock lõi của buffer 3, chốt ở EOF đầu ra và công bố khi hoàn tất; trừ SOF_TS hoặc
 * mốc chụp modulo 2^32 để tính thời gian xử lý/độ trễ đầu cuối.
 */
#define FX1_ISP_ODMA_BUF_EOF_TS3_OFFSET 0x315CU
#define REG_ODMA_BUF_EOF_TS3 FX1_ISP_ODMA_BUF_EOF_TS3_OFFSET
#define FX1_ISP_ODMA_BUF_EOF_TS3_RESET 0x00000000U
#define FX1_ISP_ODMA_BUF_EOF_TS3_ACCESS FX1_ISP_ACCESS_RO
#define FX1_ISP_ODMA_BUF_EOF_TS3_RO_MASK 0xFFFFFFFFU
#define FX1_ISP_ODMA_BUF_EOF_TS3_RW_MASK 0x00000000U
#define FX1_ISP_ODMA_BUF_EOF_TS3_W1C_MASK 0x00000000U
#define FX1_ISP_ODMA_BUF_EOF_TS3_W1S_MASK 0x00000000U
#define FX1_ISP_ODMA_BUF_EOF_TS3_W1P_MASK 0x00000000U
#define FX1_ISP_ODMA_BUF_EOF_TS3_W1SC_MASK 0x00000000U
/**
 * timestamp [31:0], dòng bảng 607, kiểu truy cập RO
 * Mốc thời gian clock lõi của buffer 3, giá trị 32 bit hard reset 0, chốt ở EOF đầu ra và công bố
 * khi hoàn tất; trừ SOF_TS hoặc mốc chụp modulo 2^32 để tính thời gian xử lý/độ trễ đầu cuối.
 */
#define FX1_ISP_ODMA_BUF_EOF_TS3_TIMESTAMP_SHIFT 0U
#define FX1_ISP_ODMA_BUF_EOF_TS3_TIMESTAMP_WIDTH 32U
#define FX1_ISP_ODMA_BUF_EOF_TS3_TIMESTAMP_MASK 0xFFFFFFFFU
#define FX1_ISP_ODMA_BUF_EOF_TS3_TIMESTAMP_RESET 0x00000000U
#define FX1_ISP_ODMA_BUF_EOF_TS3_TIMESTAMP_ACCESS FX1_ISP_ACCESS_RO

/**
 * FRAME_COUNTER: địa chỉ lệch 0x3160, dòng bảng 608, kiểu truy cập RO
 * Gốc thời gian metadata DMA: bộ đếm clock lõi 32 bit chạy tự do, cuộn modulo 2^32, khoảng 14,3 giây
 * ở 300 MHz.
 */
#define FX1_ISP_FRAME_COUNTER_OFFSET 0x3160U
#define REG_FRAME_COUNTER FX1_ISP_FRAME_COUNTER_OFFSET
#define FX1_ISP_FRAME_COUNTER_RESET 0x00000000U
#define FX1_ISP_FRAME_COUNTER_ACCESS FX1_ISP_ACCESS_RO
#define FX1_ISP_FRAME_COUNTER_RO_MASK 0xFFFFFFFFU
#define FX1_ISP_FRAME_COUNTER_RW_MASK 0x00000000U
#define FX1_ISP_FRAME_COUNTER_W1C_MASK 0x00000000U
#define FX1_ISP_FRAME_COUNTER_W1S_MASK 0x00000000U
#define FX1_ISP_FRAME_COUNTER_W1P_MASK 0x00000000U
#define FX1_ISP_FRAME_COUNTER_W1SC_MASK 0x00000000U
/**
 * counter [31:0], dòng bảng 609, kiểu truy cập RO
 * Bộ đếm clock lõi hard reset về 0; mốc chụp và SOF/EOF đầu ra dùng cùng gốc. Phần mềm xử lý cuộn
 * vòng và chỉ diễn giải chênh lệch trong khoảng không mơ hồ.
 */
#define FX1_ISP_FRAME_COUNTER_COUNTER_SHIFT 0U
#define FX1_ISP_FRAME_COUNTER_COUNTER_WIDTH 32U
#define FX1_ISP_FRAME_COUNTER_COUNTER_MASK 0xFFFFFFFFU
#define FX1_ISP_FRAME_COUNTER_COUNTER_RESET 0x00000000U
#define FX1_ISP_FRAME_COUNTER_COUNTER_ACCESS FX1_ISP_ACCESS_RO

/**
 * CH1_RESIZER_CTRL: địa chỉ lệch 0x3200, dòng bảng 610, kiểu truy cập RW
 * Resize CH1 NV12 tối đa 3840x2160, nhận crop sau IQ của kênh. Crop có thể lớn hơn đầu ra; không
 * phóng to/kích thước tùy ý. Từ chối đích sai trước DMA, không tự về FullRes; chờ OUT_W/H ổn định.
 */
#define FX1_ISP_CH1_RESIZER_CTRL_OFFSET 0x3200U
#define REG_CH1_RESIZER_CTRL FX1_ISP_CH1_RESIZER_CTRL_OFFSET
#define FX1_ISP_CH1_RESIZER_CTRL_RESET 0x00000000U
#define FX1_ISP_CH1_RESIZER_CTRL_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_CH1_RESIZER_CTRL_RO_MASK 0x00000000U
#define FX1_ISP_CH1_RESIZER_CTRL_RW_MASK 0x000000FDU
#define FX1_ISP_CH1_RESIZER_CTRL_W1C_MASK 0x00000000U
#define FX1_ISP_CH1_RESIZER_CTRL_W1S_MASK 0x00000000U
#define FX1_ISP_CH1_RESIZER_CTRL_W1P_MASK 0x00000000U
#define FX1_ISP_CH1_RESIZER_CTRL_W1SC_MASK 0x00000000U
/**
 * en [0:0], dòng bảng 611, kiểu truy cập RW
 * Bật resize CH1: 1 dùng SCALE, 0 giữ crop. Cả hai phải vừa 3840x2160; chỉ đổi sau dừng/xử lý hết và
 * kiểm tra trước DMA.
 */
#define FX1_ISP_CH1_RESIZER_CTRL_EN_SHIFT 0U
#define FX1_ISP_CH1_RESIZER_CTRL_EN_WIDTH 1U
#define FX1_ISP_CH1_RESIZER_CTRL_EN_MASK 0x00000001U
#define FX1_ISP_CH1_RESIZER_CTRL_EN_RESET 0x00000000U
#define FX1_ISP_CH1_RESIZER_CTRL_EN_ACCESS FX1_ISP_ACCESS_RW
/**
 * updated [2:2], dòng bảng 612, kiểu truy cập RW
 * Trường gợi ý cập nhật legacy chỉ lưu, không COMMIT. Đổi active size/crop/enable/SCALE kích hoạt
 * tính lại hình học; kích thước tùy ý dự phòng. Chờ OUT_W/H ổn định trước DMA; chốt hình học tại
 * SOF.
 */
#define FX1_ISP_CH1_RESIZER_CTRL_UPDATED_SHIFT 2U
#define FX1_ISP_CH1_RESIZER_CTRL_UPDATED_WIDTH 1U
#define FX1_ISP_CH1_RESIZER_CTRL_UPDATED_MASK 0x00000004U
#define FX1_ISP_CH1_RESIZER_CTRL_UPDATED_RESET 0x00000000U
#define FX1_ISP_CH1_RESIZER_CTRL_UPDATED_ACCESS FX1_ISP_ACCESS_RW
/**
 * scale [7:3], dòng bảng 613, kiểu truy cập RW
 * Mã CH1 NV12 [7:3]: 0 giữ crop; 1..14 mã chuẩn;
 * 16=640x640,17=640x384,18=640x352,19=512x512,20=416x416,21=320x320. 15 và 22..31 dự phòng. Tối đa
 * 3840x2160 và không vượt crop; không phóng to.
 */
#define FX1_ISP_CH1_RESIZER_CTRL_SCALE_SHIFT 3U
#define FX1_ISP_CH1_RESIZER_CTRL_SCALE_WIDTH 5U
#define FX1_ISP_CH1_RESIZER_CTRL_SCALE_MASK 0x000000F8U
#define FX1_ISP_CH1_RESIZER_CTRL_SCALE_RESET 0x00000000U
#define FX1_ISP_CH1_RESIZER_CTRL_SCALE_ACCESS FX1_ISP_ACCESS_RW

/**
 * CH1_RESIZER_CUSTOM_W: địa chỉ lệch 0x3204, dòng bảng 614, kiểu truy cập RO
 * Địa chỉ kích thước đích cũ, dự phòng ở ABI 1.07: đọc 0, bỏ ghi. Không hỗ trợ resize kích thước tùy
 * ý.
 */
#define FX1_ISP_CH1_RESIZER_CUSTOM_W_OFFSET 0x3204U
#define REG_CH1_RESIZER_CUSTOM_W FX1_ISP_CH1_RESIZER_CUSTOM_W_OFFSET
#define FX1_ISP_CH1_RESIZER_CUSTOM_W_RESET 0x00000000U
#define FX1_ISP_CH1_RESIZER_CUSTOM_W_ACCESS FX1_ISP_ACCESS_RO
#define FX1_ISP_CH1_RESIZER_CUSTOM_W_RO_MASK 0xFFFFFFFFU
#define FX1_ISP_CH1_RESIZER_CUSTOM_W_RW_MASK 0x00000000U
#define FX1_ISP_CH1_RESIZER_CUSTOM_W_W1C_MASK 0x00000000U
#define FX1_ISP_CH1_RESIZER_CUSTOM_W_W1S_MASK 0x00000000U
#define FX1_ISP_CH1_RESIZER_CUSTOM_W_W1P_MASK 0x00000000U
#define FX1_ISP_CH1_RESIZER_CUSTOM_W_W1SC_MASK 0x00000000U
/**
 * RESERVED [31:0], dòng bảng 615, kiểu truy cập RO
 * Đọc 0, bỏ qua ghi.
 */
#define FX1_ISP_CH1_RESIZER_CUSTOM_W_RESERVED_SHIFT 0U
#define FX1_ISP_CH1_RESIZER_CUSTOM_W_RESERVED_WIDTH 32U
#define FX1_ISP_CH1_RESIZER_CUSTOM_W_RESERVED_MASK 0xFFFFFFFFU
#define FX1_ISP_CH1_RESIZER_CUSTOM_W_RESERVED_RESET 0x00000000U
#define FX1_ISP_CH1_RESIZER_CUSTOM_W_RESERVED_ACCESS FX1_ISP_ACCESS_RO

/**
 * CH1_RESIZER_CUSTOM_H: địa chỉ lệch 0x3208, dòng bảng 616, kiểu truy cập RO
 * Địa chỉ kích thước đích cũ, dự phòng ở ABI 1.07: đọc 0, bỏ ghi. Không hỗ trợ resize kích thước tùy
 * ý.
 */
#define FX1_ISP_CH1_RESIZER_CUSTOM_H_OFFSET 0x3208U
#define REG_CH1_RESIZER_CUSTOM_H FX1_ISP_CH1_RESIZER_CUSTOM_H_OFFSET
#define FX1_ISP_CH1_RESIZER_CUSTOM_H_RESET 0x00000000U
#define FX1_ISP_CH1_RESIZER_CUSTOM_H_ACCESS FX1_ISP_ACCESS_RO
#define FX1_ISP_CH1_RESIZER_CUSTOM_H_RO_MASK 0xFFFFFFFFU
#define FX1_ISP_CH1_RESIZER_CUSTOM_H_RW_MASK 0x00000000U
#define FX1_ISP_CH1_RESIZER_CUSTOM_H_W1C_MASK 0x00000000U
#define FX1_ISP_CH1_RESIZER_CUSTOM_H_W1S_MASK 0x00000000U
#define FX1_ISP_CH1_RESIZER_CUSTOM_H_W1P_MASK 0x00000000U
#define FX1_ISP_CH1_RESIZER_CUSTOM_H_W1SC_MASK 0x00000000U
/**
 * RESERVED [31:0], dòng bảng 617, kiểu truy cập RO
 * Đọc 0, bỏ qua ghi.
 */
#define FX1_ISP_CH1_RESIZER_CUSTOM_H_RESERVED_SHIFT 0U
#define FX1_ISP_CH1_RESIZER_CUSTOM_H_RESERVED_WIDTH 32U
#define FX1_ISP_CH1_RESIZER_CUSTOM_H_RESERVED_MASK 0xFFFFFFFFU
#define FX1_ISP_CH1_RESIZER_CUSTOM_H_RESERVED_RESET 0x00000000U
#define FX1_ISP_CH1_RESIZER_CUSTOM_H_RESERVED_ACCESS FX1_ISP_ACCESS_RO

/**
 * CH1_RESIZER_OUT_W: địa chỉ lệch 0x3214, dòng bảng 618, kiểu truy cập RO
 * Kích thước đầu ra suy ra đã kiểm tra. Reset 0; có thể giữ giá trị hoàn tất trước khi hình học hiện
 * tại chưa hợp lệ. Kiểm tra GEOMETRY_STATUS của kênh trước DMA.
 */
#define FX1_ISP_CH1_RESIZER_OUT_W_OFFSET 0x3214U
#define REG_CH1_RESIZER_OUT_W FX1_ISP_CH1_RESIZER_OUT_W_OFFSET
#define FX1_ISP_CH1_RESIZER_OUT_W_RESET 0x00000000U
#define FX1_ISP_CH1_RESIZER_OUT_W_ACCESS FX1_ISP_ACCESS_RO
#define FX1_ISP_CH1_RESIZER_OUT_W_RO_MASK 0x00001FFFU
#define FX1_ISP_CH1_RESIZER_OUT_W_RW_MASK 0x00000000U
#define FX1_ISP_CH1_RESIZER_OUT_W_W1C_MASK 0x00000000U
#define FX1_ISP_CH1_RESIZER_OUT_W_W1S_MASK 0x00000000U
#define FX1_ISP_CH1_RESIZER_OUT_W_W1P_MASK 0x00000000U
#define FX1_ISP_CH1_RESIZER_OUT_W_W1SC_MASK 0x00000000U
/**
 * out_width [12:0], dòng bảng 619, kiểu truy cập RO
 * Kích thước đầu ra suy ra đã kiểm tra. Reset 0; có thể giữ giá trị hoàn tất trước khi hình học hiện
 * tại chưa hợp lệ. Kiểm tra GEOMETRY_STATUS của kênh trước DMA.
 */
#define FX1_ISP_CH1_RESIZER_OUT_W_OUT_WIDTH_SHIFT 0U
#define FX1_ISP_CH1_RESIZER_OUT_W_OUT_WIDTH_WIDTH 13U
#define FX1_ISP_CH1_RESIZER_OUT_W_OUT_WIDTH_MASK 0x00001FFFU
#define FX1_ISP_CH1_RESIZER_OUT_W_OUT_WIDTH_RESET 0x00000000U
#define FX1_ISP_CH1_RESIZER_OUT_W_OUT_WIDTH_ACCESS FX1_ISP_ACCESS_RO

/**
 * CH1_RESIZER_OUT_H: địa chỉ lệch 0x3218, dòng bảng 620, kiểu truy cập RO
 * Kích thước đầu ra suy ra đã kiểm tra. Reset 0; có thể giữ giá trị hoàn tất trước khi hình học hiện
 * tại chưa hợp lệ. Kiểm tra GEOMETRY_STATUS của kênh trước DMA.
 */
#define FX1_ISP_CH1_RESIZER_OUT_H_OFFSET 0x3218U
#define REG_CH1_RESIZER_OUT_H FX1_ISP_CH1_RESIZER_OUT_H_OFFSET
#define FX1_ISP_CH1_RESIZER_OUT_H_RESET 0x00000000U
#define FX1_ISP_CH1_RESIZER_OUT_H_ACCESS FX1_ISP_ACCESS_RO
#define FX1_ISP_CH1_RESIZER_OUT_H_RO_MASK 0x00001FFFU
#define FX1_ISP_CH1_RESIZER_OUT_H_RW_MASK 0x00000000U
#define FX1_ISP_CH1_RESIZER_OUT_H_W1C_MASK 0x00000000U
#define FX1_ISP_CH1_RESIZER_OUT_H_W1S_MASK 0x00000000U
#define FX1_ISP_CH1_RESIZER_OUT_H_W1P_MASK 0x00000000U
#define FX1_ISP_CH1_RESIZER_OUT_H_W1SC_MASK 0x00000000U
/**
 * out_height [12:0], dòng bảng 621, kiểu truy cập RO
 * Kích thước đầu ra suy ra đã kiểm tra. Reset 0; có thể giữ giá trị hoàn tất trước khi hình học hiện
 * tại chưa hợp lệ. Kiểm tra GEOMETRY_STATUS của kênh trước DMA.
 */
#define FX1_ISP_CH1_RESIZER_OUT_H_OUT_HEIGHT_SHIFT 0U
#define FX1_ISP_CH1_RESIZER_OUT_H_OUT_HEIGHT_WIDTH 13U
#define FX1_ISP_CH1_RESIZER_OUT_H_OUT_HEIGHT_MASK 0x00001FFFU
#define FX1_ISP_CH1_RESIZER_OUT_H_OUT_HEIGHT_RESET 0x00000000U
#define FX1_ISP_CH1_RESIZER_OUT_H_OUT_HEIGHT_ACCESS FX1_ISP_ACCESS_RO

/**
 * CH1_RESIZER_STATUS: địa chỉ lệch 0x321C, dòng bảng 622, kiểu truy cập RO
 * Trạng thái khối resizer.
 */
#define FX1_ISP_CH1_RESIZER_STATUS_OFFSET 0x321CU
#define REG_CH1_RESIZER_STATUS FX1_ISP_CH1_RESIZER_STATUS_OFFSET
#define FX1_ISP_CH1_RESIZER_STATUS_RESET 0x00000000U
#define FX1_ISP_CH1_RESIZER_STATUS_ACCESS FX1_ISP_ACCESS_RO
#define FX1_ISP_CH1_RESIZER_STATUS_RO_MASK 0x00000001U
#define FX1_ISP_CH1_RESIZER_STATUS_RW_MASK 0x00000000U
#define FX1_ISP_CH1_RESIZER_STATUS_W1C_MASK 0x00000000U
#define FX1_ISP_CH1_RESIZER_STATUS_W1S_MASK 0x00000000U
#define FX1_ISP_CH1_RESIZER_STATUS_W1P_MASK 0x00000000U
#define FX1_ISP_CH1_RESIZER_STATUS_W1SC_MASK 0x00000000U
/**
 * busy [0:0], dòng bảng 623, kiểu truy cập RO
 * Resizer BUSY=1 khi còn giữ pixel. COMMON_STATUS.BUSY tổng hợp frame trong pipeline và ODMA; thanh
 * ghi này chỉ phản ánh kênh tương ứng.
 */
#define FX1_ISP_CH1_RESIZER_STATUS_BUSY_SHIFT 0U
#define FX1_ISP_CH1_RESIZER_STATUS_BUSY_WIDTH 1U
#define FX1_ISP_CH1_RESIZER_STATUS_BUSY_MASK 0x00000001U
#define FX1_ISP_CH1_RESIZER_STATUS_BUSY_RESET 0x00000000U
#define FX1_ISP_CH1_RESIZER_STATUS_BUSY_ACCESS FX1_ISP_ACCESS_RO

/**
 * CH1_ODMA_Y_STRIDE: địa chỉ lệch 0x3280, dòng bảng 624, kiểu truy cập RW
 * Stride mặt phẳng Y đầu ra NV12; ảnh gồm Y đủ độ phân giải và UV xen kẽ có nửa số dòng.
 */
#define FX1_ISP_CH1_ODMA_Y_STRIDE_OFFSET 0x3280U
#define REG_CH1_ODMA_Y_STRIDE FX1_ISP_CH1_ODMA_Y_STRIDE_OFFSET
#define FX1_ISP_CH1_ODMA_Y_STRIDE_RESET 0x00000F00U
#define FX1_ISP_CH1_ODMA_Y_STRIDE_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_CH1_ODMA_Y_STRIDE_RO_MASK 0x00000000U
#define FX1_ISP_CH1_ODMA_Y_STRIDE_RW_MASK 0xFFFFFFFFU
#define FX1_ISP_CH1_ODMA_Y_STRIDE_W1C_MASK 0x00000000U
#define FX1_ISP_CH1_ODMA_Y_STRIDE_W1S_MASK 0x00000000U
#define FX1_ISP_CH1_ODMA_Y_STRIDE_W1P_MASK 0x00000000U
#define FX1_ISP_CH1_ODMA_Y_STRIDE_W1SC_MASK 0x00000000U
/**
 * y_stride [31:0], dòng bảng 625, kiểu truy cập RW
 * Số byte giữa hai dòng Y, một byte/pixel, làm tròn lên theo độ rộng bus.
 */
#define FX1_ISP_CH1_ODMA_Y_STRIDE_Y_STRIDE_SHIFT 0U
#define FX1_ISP_CH1_ODMA_Y_STRIDE_Y_STRIDE_WIDTH 32U
#define FX1_ISP_CH1_ODMA_Y_STRIDE_Y_STRIDE_MASK 0xFFFFFFFFU
#define FX1_ISP_CH1_ODMA_Y_STRIDE_Y_STRIDE_RESET 0x00000F00U
#define FX1_ISP_CH1_ODMA_Y_STRIDE_Y_STRIDE_ACCESS FX1_ISP_ACCESS_RW

/**
 * CH1_ODMA_UV_STRIDE: địa chỉ lệch 0x3284, dòng bảng 626, kiểu truy cập RW
 * Stride mặt phẳng UV: nửa số dòng Y nhưng cùng số byte mỗi dòng vì U/V xen kẽ ở nửa tần số ngang.
 */
#define FX1_ISP_CH1_ODMA_UV_STRIDE_OFFSET 0x3284U
#define REG_CH1_ODMA_UV_STRIDE FX1_ISP_CH1_ODMA_UV_STRIDE_OFFSET
#define FX1_ISP_CH1_ODMA_UV_STRIDE_RESET 0x00000F00U
#define FX1_ISP_CH1_ODMA_UV_STRIDE_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_CH1_ODMA_UV_STRIDE_RO_MASK 0x00000000U
#define FX1_ISP_CH1_ODMA_UV_STRIDE_RW_MASK 0xFFFFFFFFU
#define FX1_ISP_CH1_ODMA_UV_STRIDE_W1C_MASK 0x00000000U
#define FX1_ISP_CH1_ODMA_UV_STRIDE_W1S_MASK 0x00000000U
#define FX1_ISP_CH1_ODMA_UV_STRIDE_W1P_MASK 0x00000000U
#define FX1_ISP_CH1_ODMA_UV_STRIDE_W1SC_MASK 0x00000000U
/**
 * uv_stride [31:0], dòng bảng 627, kiểu truy cập RW
 * Số byte giữa hai dòng chroma.
 */
#define FX1_ISP_CH1_ODMA_UV_STRIDE_UV_STRIDE_SHIFT 0U
#define FX1_ISP_CH1_ODMA_UV_STRIDE_UV_STRIDE_WIDTH 32U
#define FX1_ISP_CH1_ODMA_UV_STRIDE_UV_STRIDE_MASK 0xFFFFFFFFU
#define FX1_ISP_CH1_ODMA_UV_STRIDE_UV_STRIDE_RESET 0x00000F00U
#define FX1_ISP_CH1_ODMA_UV_STRIDE_UV_STRIDE_ACCESS FX1_ISP_ACCESS_RW

/**
 * CH1_ODMA_BUF_FREE: địa chỉ lệch 0x3288, dòng bảng 628, kiểu truy cập W1S
 * FREE đầu ra CH1: phần mềm đặt trước, phần cứng xóa ở đầu frame ra. DONE chỉ đặt sau cả hai mặt
 * phẳng và mọi phản hồi ghi đã kết thúc.
 */
#define FX1_ISP_CH1_ODMA_BUF_FREE_OFFSET 0x3288U
#define REG_CH1_ODMA_BUF_FREE FX1_ISP_CH1_ODMA_BUF_FREE_OFFSET
#define FX1_ISP_CH1_ODMA_BUF_FREE_RESET 0x00000000U
#define FX1_ISP_CH1_ODMA_BUF_FREE_ACCESS FX1_ISP_ACCESS_W1S
#define FX1_ISP_CH1_ODMA_BUF_FREE_RO_MASK 0x00000000U
#define FX1_ISP_CH1_ODMA_BUF_FREE_RW_MASK 0x00000000U
#define FX1_ISP_CH1_ODMA_BUF_FREE_W1C_MASK 0x00000000U
#define FX1_ISP_CH1_ODMA_BUF_FREE_W1S_MASK 0x000000FFU
#define FX1_ISP_CH1_ODMA_BUF_FREE_W1P_MASK 0x00000000U
#define FX1_ISP_CH1_ODMA_BUF_FREE_W1SC_MASK 0x00000000U
/**
 * odma_buf_free [7:0], dòng bảng 629, kiểu truy cập W1S
 * FREE bit 0..3 ứng bốn buffer, 7:4 không dùng. Ghi 1 để giao buffer, phần cứng xóa ở đầu frame. Xác
 * nhận DONE trước cấp lại FREE; thiếu buffer làm chờ và báo ODMA_OVERFLOW.
 */
#define FX1_ISP_CH1_ODMA_BUF_FREE_ODMA_BUF_FREE_SHIFT 0U
#define FX1_ISP_CH1_ODMA_BUF_FREE_ODMA_BUF_FREE_WIDTH 8U
#define FX1_ISP_CH1_ODMA_BUF_FREE_ODMA_BUF_FREE_MASK 0x000000FFU
#define FX1_ISP_CH1_ODMA_BUF_FREE_ODMA_BUF_FREE_RESET 0x00000000U
#define FX1_ISP_CH1_ODMA_BUF_FREE_ODMA_BUF_FREE_ACCESS FX1_ISP_ACCESS_W1S

/**
 * CH1_ODMA_BUF_DONE: địa chỉ lệch 0x328C, dòng bảng 630, kiểu truy cập W1C
 * DONE CH1 W1C, chỉ đặt sau hai mặt phẳng và mọi phản hồi AXI ghi kết thúc. Xóa DONE trước cấp lại
 * FREE.
 */
#define FX1_ISP_CH1_ODMA_BUF_DONE_OFFSET 0x328CU
#define REG_CH1_ODMA_BUF_DONE FX1_ISP_CH1_ODMA_BUF_DONE_OFFSET
#define FX1_ISP_CH1_ODMA_BUF_DONE_RESET 0x00000000U
#define FX1_ISP_CH1_ODMA_BUF_DONE_ACCESS FX1_ISP_ACCESS_W1C
#define FX1_ISP_CH1_ODMA_BUF_DONE_RO_MASK 0x00000000U
#define FX1_ISP_CH1_ODMA_BUF_DONE_RW_MASK 0x00000000U
#define FX1_ISP_CH1_ODMA_BUF_DONE_W1C_MASK 0x000000FFU
#define FX1_ISP_CH1_ODMA_BUF_DONE_W1S_MASK 0x00000000U
#define FX1_ISP_CH1_ODMA_BUF_DONE_W1P_MASK 0x00000000U
#define FX1_ISP_CH1_ODMA_BUF_DONE_W1SC_MASK 0x00000000U
/**
 * odma_buf_done [7:0], dòng bảng 631, kiểu truy cập W1C
 * DONE bit 0..3 ứng buffer, 7:4 không dùng. Phần cứng đặt khi frame ra kết thúc đầy đủ; ghi 1 xác
 * nhận, rồi ghi FREE riêng để tái dùng. Hoàn tất mới thắng xóa đồng thời.
 */
#define FX1_ISP_CH1_ODMA_BUF_DONE_ODMA_BUF_DONE_SHIFT 0U
#define FX1_ISP_CH1_ODMA_BUF_DONE_ODMA_BUF_DONE_WIDTH 8U
#define FX1_ISP_CH1_ODMA_BUF_DONE_ODMA_BUF_DONE_MASK 0x000000FFU
#define FX1_ISP_CH1_ODMA_BUF_DONE_ODMA_BUF_DONE_RESET 0x00000000U
#define FX1_ISP_CH1_ODMA_BUF_DONE_ODMA_BUF_DONE_ACCESS FX1_ISP_ACCESS_W1C

/**
 * CH1_ODMA_FRAME_COUNT: địa chỉ lệch 0x3290, dòng bảng 632, kiểu truy cập RO
 * Số frame đã ghi từ reset, chỉ đọc. Ưu tiên chờ bộ đếm thay DONE vì bit DONE dùng chung với xác
 * nhận W1C của lần trước.
 */
#define FX1_ISP_CH1_ODMA_FRAME_COUNT_OFFSET 0x3290U
#define REG_CH1_ODMA_FRAME_COUNT FX1_ISP_CH1_ODMA_FRAME_COUNT_OFFSET
#define FX1_ISP_CH1_ODMA_FRAME_COUNT_RESET 0x00000000U
#define FX1_ISP_CH1_ODMA_FRAME_COUNT_ACCESS FX1_ISP_ACCESS_RO
#define FX1_ISP_CH1_ODMA_FRAME_COUNT_RO_MASK 0xFFFFFFFFU
#define FX1_ISP_CH1_ODMA_FRAME_COUNT_RW_MASK 0x00000000U
#define FX1_ISP_CH1_ODMA_FRAME_COUNT_W1C_MASK 0x00000000U
#define FX1_ISP_CH1_ODMA_FRAME_COUNT_W1S_MASK 0x00000000U
#define FX1_ISP_CH1_ODMA_FRAME_COUNT_W1P_MASK 0x00000000U
#define FX1_ISP_CH1_ODMA_FRAME_COUNT_W1SC_MASK 0x00000000U
/**
 * odma_frame_count [31:0], dòng bảng 633, kiểu truy cập RO
 * Bộ đếm frame đầu ra, cuộn modulo 2^32.
 */
#define FX1_ISP_CH1_ODMA_FRAME_COUNT_ODMA_FRAME_COUNT_SHIFT 0U
#define FX1_ISP_CH1_ODMA_FRAME_COUNT_ODMA_FRAME_COUNT_WIDTH 32U
#define FX1_ISP_CH1_ODMA_FRAME_COUNT_ODMA_FRAME_COUNT_MASK 0xFFFFFFFFU
#define FX1_ISP_CH1_ODMA_FRAME_COUNT_ODMA_FRAME_COUNT_RESET 0x00000000U
#define FX1_ISP_CH1_ODMA_FRAME_COUNT_ODMA_FRAME_COUNT_ACCESS FX1_ISP_ACCESS_RO

/**
 * CH1_ODMA_Y_ADDR_L0: địa chỉ lệch 0x32A0, dòng bảng 634, kiểu truy cập RW
 * Địa chỉ DDR mặt phẳng Y bộ đệm 0, từ thấp.
 */
#define FX1_ISP_CH1_ODMA_Y_ADDR_L0_OFFSET 0x32A0U
#define REG_CH1_ODMA_Y_ADDR_L0 FX1_ISP_CH1_ODMA_Y_ADDR_L0_OFFSET
#define FX1_ISP_CH1_ODMA_Y_ADDR_L0_RESET 0x00000000U
#define FX1_ISP_CH1_ODMA_Y_ADDR_L0_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_CH1_ODMA_Y_ADDR_L0_RO_MASK 0x00000000U
#define FX1_ISP_CH1_ODMA_Y_ADDR_L0_RW_MASK 0xFFFFFFFFU
#define FX1_ISP_CH1_ODMA_Y_ADDR_L0_W1C_MASK 0x00000000U
#define FX1_ISP_CH1_ODMA_Y_ADDR_L0_W1S_MASK 0x00000000U
#define FX1_ISP_CH1_ODMA_Y_ADDR_L0_W1P_MASK 0x00000000U
#define FX1_ISP_CH1_ODMA_Y_ADDR_L0_W1SC_MASK 0x00000000U
/**
 * odma_y_addr0_lo [31:0], dòng bảng 635, kiểu truy cập RW
 * Các bit [31:0] địa chỉ mặt phẳng Y bộ đệm 0.
 */
#define FX1_ISP_CH1_ODMA_Y_ADDR_L0_ODMA_Y_ADDR0_LO_SHIFT 0U
#define FX1_ISP_CH1_ODMA_Y_ADDR_L0_ODMA_Y_ADDR0_LO_WIDTH 32U
#define FX1_ISP_CH1_ODMA_Y_ADDR_L0_ODMA_Y_ADDR0_LO_MASK 0xFFFFFFFFU
#define FX1_ISP_CH1_ODMA_Y_ADDR_L0_ODMA_Y_ADDR0_LO_RESET 0x00000000U
#define FX1_ISP_CH1_ODMA_Y_ADDR_L0_ODMA_Y_ADDR0_LO_ACCESS FX1_ISP_ACCESS_RW

/**
 * CH1_ODMA_Y_ADDR_H0: địa chỉ lệch 0x32A4, dòng bảng 636, kiểu truy cập RW
 * Địa chỉ DDR mặt phẳng Y bộ đệm 0, từ cao.
 */
#define FX1_ISP_CH1_ODMA_Y_ADDR_H0_OFFSET 0x32A4U
#define REG_CH1_ODMA_Y_ADDR_H0 FX1_ISP_CH1_ODMA_Y_ADDR_H0_OFFSET
#define FX1_ISP_CH1_ODMA_Y_ADDR_H0_RESET 0x00000000U
#define FX1_ISP_CH1_ODMA_Y_ADDR_H0_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_CH1_ODMA_Y_ADDR_H0_RO_MASK 0x00000000U
#define FX1_ISP_CH1_ODMA_Y_ADDR_H0_RW_MASK 0xFFFFFFFFU
#define FX1_ISP_CH1_ODMA_Y_ADDR_H0_W1C_MASK 0x00000000U
#define FX1_ISP_CH1_ODMA_Y_ADDR_H0_W1S_MASK 0x00000000U
#define FX1_ISP_CH1_ODMA_Y_ADDR_H0_W1P_MASK 0x00000000U
#define FX1_ISP_CH1_ODMA_Y_ADDR_H0_W1SC_MASK 0x00000000U
/**
 * odma_y_addr0_hi [31:0], dòng bảng 637, kiểu truy cập RW
 * Các bit [63:32] của địa chỉ DDR.
 */
#define FX1_ISP_CH1_ODMA_Y_ADDR_H0_ODMA_Y_ADDR0_HI_SHIFT 0U
#define FX1_ISP_CH1_ODMA_Y_ADDR_H0_ODMA_Y_ADDR0_HI_WIDTH 32U
#define FX1_ISP_CH1_ODMA_Y_ADDR_H0_ODMA_Y_ADDR0_HI_MASK 0xFFFFFFFFU
#define FX1_ISP_CH1_ODMA_Y_ADDR_H0_ODMA_Y_ADDR0_HI_RESET 0x00000000U
#define FX1_ISP_CH1_ODMA_Y_ADDR_H0_ODMA_Y_ADDR0_HI_ACCESS FX1_ISP_ACCESS_RW

/**
 * CH1_ODMA_UV_ADDR_L0: địa chỉ lệch 0x32A8, dòng bảng 638, kiểu truy cập RW
 * Địa chỉ DDR mặt phẳng UV bộ đệm 0, từ thấp.
 */
#define FX1_ISP_CH1_ODMA_UV_ADDR_L0_OFFSET 0x32A8U
#define REG_CH1_ODMA_UV_ADDR_L0 FX1_ISP_CH1_ODMA_UV_ADDR_L0_OFFSET
#define FX1_ISP_CH1_ODMA_UV_ADDR_L0_RESET 0x00000000U
#define FX1_ISP_CH1_ODMA_UV_ADDR_L0_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_CH1_ODMA_UV_ADDR_L0_RO_MASK 0x00000000U
#define FX1_ISP_CH1_ODMA_UV_ADDR_L0_RW_MASK 0xFFFFFFFFU
#define FX1_ISP_CH1_ODMA_UV_ADDR_L0_W1C_MASK 0x00000000U
#define FX1_ISP_CH1_ODMA_UV_ADDR_L0_W1S_MASK 0x00000000U
#define FX1_ISP_CH1_ODMA_UV_ADDR_L0_W1P_MASK 0x00000000U
#define FX1_ISP_CH1_ODMA_UV_ADDR_L0_W1SC_MASK 0x00000000U
/**
 * odma_uv_addr0_lo [31:0], dòng bảng 639, kiểu truy cập RW
 * Các bit [31:0] địa chỉ mặt phẳng Y bộ đệm 0.
 */
#define FX1_ISP_CH1_ODMA_UV_ADDR_L0_ODMA_UV_ADDR0_LO_SHIFT 0U
#define FX1_ISP_CH1_ODMA_UV_ADDR_L0_ODMA_UV_ADDR0_LO_WIDTH 32U
#define FX1_ISP_CH1_ODMA_UV_ADDR_L0_ODMA_UV_ADDR0_LO_MASK 0xFFFFFFFFU
#define FX1_ISP_CH1_ODMA_UV_ADDR_L0_ODMA_UV_ADDR0_LO_RESET 0x00000000U
#define FX1_ISP_CH1_ODMA_UV_ADDR_L0_ODMA_UV_ADDR0_LO_ACCESS FX1_ISP_ACCESS_RW

/**
 * CH1_ODMA_UV_ADDR_H0: địa chỉ lệch 0x32AC, dòng bảng 640, kiểu truy cập RW
 * Địa chỉ DDR mặt phẳng UV bộ đệm 0, từ cao.
 */
#define FX1_ISP_CH1_ODMA_UV_ADDR_H0_OFFSET 0x32ACU
#define REG_CH1_ODMA_UV_ADDR_H0 FX1_ISP_CH1_ODMA_UV_ADDR_H0_OFFSET
#define FX1_ISP_CH1_ODMA_UV_ADDR_H0_RESET 0x00000000U
#define FX1_ISP_CH1_ODMA_UV_ADDR_H0_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_CH1_ODMA_UV_ADDR_H0_RO_MASK 0x00000000U
#define FX1_ISP_CH1_ODMA_UV_ADDR_H0_RW_MASK 0xFFFFFFFFU
#define FX1_ISP_CH1_ODMA_UV_ADDR_H0_W1C_MASK 0x00000000U
#define FX1_ISP_CH1_ODMA_UV_ADDR_H0_W1S_MASK 0x00000000U
#define FX1_ISP_CH1_ODMA_UV_ADDR_H0_W1P_MASK 0x00000000U
#define FX1_ISP_CH1_ODMA_UV_ADDR_H0_W1SC_MASK 0x00000000U
/**
 * odma_uv_addr0_hi [31:0], dòng bảng 641, kiểu truy cập RW
 * Các bit [63:32] của địa chỉ DDR.
 */
#define FX1_ISP_CH1_ODMA_UV_ADDR_H0_ODMA_UV_ADDR0_HI_SHIFT 0U
#define FX1_ISP_CH1_ODMA_UV_ADDR_H0_ODMA_UV_ADDR0_HI_WIDTH 32U
#define FX1_ISP_CH1_ODMA_UV_ADDR_H0_ODMA_UV_ADDR0_HI_MASK 0xFFFFFFFFU
#define FX1_ISP_CH1_ODMA_UV_ADDR_H0_ODMA_UV_ADDR0_HI_RESET 0x00000000U
#define FX1_ISP_CH1_ODMA_UV_ADDR_H0_ODMA_UV_ADDR0_HI_ACCESS FX1_ISP_ACCESS_RW

/**
 * CH1_ODMA_Y_ADDR_L1: địa chỉ lệch 0x32B0, dòng bảng 642, kiểu truy cập RW
 * Địa chỉ DDR mặt phẳng Y bộ đệm 1, từ thấp.
 */
#define FX1_ISP_CH1_ODMA_Y_ADDR_L1_OFFSET 0x32B0U
#define REG_CH1_ODMA_Y_ADDR_L1 FX1_ISP_CH1_ODMA_Y_ADDR_L1_OFFSET
#define FX1_ISP_CH1_ODMA_Y_ADDR_L1_RESET 0x00000000U
#define FX1_ISP_CH1_ODMA_Y_ADDR_L1_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_CH1_ODMA_Y_ADDR_L1_RO_MASK 0x00000000U
#define FX1_ISP_CH1_ODMA_Y_ADDR_L1_RW_MASK 0xFFFFFFFFU
#define FX1_ISP_CH1_ODMA_Y_ADDR_L1_W1C_MASK 0x00000000U
#define FX1_ISP_CH1_ODMA_Y_ADDR_L1_W1S_MASK 0x00000000U
#define FX1_ISP_CH1_ODMA_Y_ADDR_L1_W1P_MASK 0x00000000U
#define FX1_ISP_CH1_ODMA_Y_ADDR_L1_W1SC_MASK 0x00000000U
/**
 * odma_y_addr1_lo [31:0], dòng bảng 643, kiểu truy cập RW
 * Các bit [31:0] địa chỉ mặt phẳng Y bộ đệm 1.
 */
#define FX1_ISP_CH1_ODMA_Y_ADDR_L1_ODMA_Y_ADDR1_LO_SHIFT 0U
#define FX1_ISP_CH1_ODMA_Y_ADDR_L1_ODMA_Y_ADDR1_LO_WIDTH 32U
#define FX1_ISP_CH1_ODMA_Y_ADDR_L1_ODMA_Y_ADDR1_LO_MASK 0xFFFFFFFFU
#define FX1_ISP_CH1_ODMA_Y_ADDR_L1_ODMA_Y_ADDR1_LO_RESET 0x00000000U
#define FX1_ISP_CH1_ODMA_Y_ADDR_L1_ODMA_Y_ADDR1_LO_ACCESS FX1_ISP_ACCESS_RW

/**
 * CH1_ODMA_Y_ADDR_H1: địa chỉ lệch 0x32B4, dòng bảng 644, kiểu truy cập RW
 * Địa chỉ DDR mặt phẳng Y bộ đệm 1, từ cao.
 */
#define FX1_ISP_CH1_ODMA_Y_ADDR_H1_OFFSET 0x32B4U
#define REG_CH1_ODMA_Y_ADDR_H1 FX1_ISP_CH1_ODMA_Y_ADDR_H1_OFFSET
#define FX1_ISP_CH1_ODMA_Y_ADDR_H1_RESET 0x00000000U
#define FX1_ISP_CH1_ODMA_Y_ADDR_H1_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_CH1_ODMA_Y_ADDR_H1_RO_MASK 0x00000000U
#define FX1_ISP_CH1_ODMA_Y_ADDR_H1_RW_MASK 0xFFFFFFFFU
#define FX1_ISP_CH1_ODMA_Y_ADDR_H1_W1C_MASK 0x00000000U
#define FX1_ISP_CH1_ODMA_Y_ADDR_H1_W1S_MASK 0x00000000U
#define FX1_ISP_CH1_ODMA_Y_ADDR_H1_W1P_MASK 0x00000000U
#define FX1_ISP_CH1_ODMA_Y_ADDR_H1_W1SC_MASK 0x00000000U
/**
 * odma_y_addr1_hi [31:0], dòng bảng 645, kiểu truy cập RW
 * Các bit [63:32] của địa chỉ DDR.
 */
#define FX1_ISP_CH1_ODMA_Y_ADDR_H1_ODMA_Y_ADDR1_HI_SHIFT 0U
#define FX1_ISP_CH1_ODMA_Y_ADDR_H1_ODMA_Y_ADDR1_HI_WIDTH 32U
#define FX1_ISP_CH1_ODMA_Y_ADDR_H1_ODMA_Y_ADDR1_HI_MASK 0xFFFFFFFFU
#define FX1_ISP_CH1_ODMA_Y_ADDR_H1_ODMA_Y_ADDR1_HI_RESET 0x00000000U
#define FX1_ISP_CH1_ODMA_Y_ADDR_H1_ODMA_Y_ADDR1_HI_ACCESS FX1_ISP_ACCESS_RW

/**
 * CH1_ODMA_UV_ADDR_L1: địa chỉ lệch 0x32B8, dòng bảng 646, kiểu truy cập RW
 * Địa chỉ DDR mặt phẳng UV bộ đệm 1, từ thấp.
 */
#define FX1_ISP_CH1_ODMA_UV_ADDR_L1_OFFSET 0x32B8U
#define REG_CH1_ODMA_UV_ADDR_L1 FX1_ISP_CH1_ODMA_UV_ADDR_L1_OFFSET
#define FX1_ISP_CH1_ODMA_UV_ADDR_L1_RESET 0x00000000U
#define FX1_ISP_CH1_ODMA_UV_ADDR_L1_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_CH1_ODMA_UV_ADDR_L1_RO_MASK 0x00000000U
#define FX1_ISP_CH1_ODMA_UV_ADDR_L1_RW_MASK 0xFFFFFFFFU
#define FX1_ISP_CH1_ODMA_UV_ADDR_L1_W1C_MASK 0x00000000U
#define FX1_ISP_CH1_ODMA_UV_ADDR_L1_W1S_MASK 0x00000000U
#define FX1_ISP_CH1_ODMA_UV_ADDR_L1_W1P_MASK 0x00000000U
#define FX1_ISP_CH1_ODMA_UV_ADDR_L1_W1SC_MASK 0x00000000U
/**
 * odma_uv_addr1_lo [31:0], dòng bảng 647, kiểu truy cập RW
 * Các bit [31:0] địa chỉ mặt phẳng Y bộ đệm 1.
 */
#define FX1_ISP_CH1_ODMA_UV_ADDR_L1_ODMA_UV_ADDR1_LO_SHIFT 0U
#define FX1_ISP_CH1_ODMA_UV_ADDR_L1_ODMA_UV_ADDR1_LO_WIDTH 32U
#define FX1_ISP_CH1_ODMA_UV_ADDR_L1_ODMA_UV_ADDR1_LO_MASK 0xFFFFFFFFU
#define FX1_ISP_CH1_ODMA_UV_ADDR_L1_ODMA_UV_ADDR1_LO_RESET 0x00000000U
#define FX1_ISP_CH1_ODMA_UV_ADDR_L1_ODMA_UV_ADDR1_LO_ACCESS FX1_ISP_ACCESS_RW

/**
 * CH1_ODMA_UV_ADDR_H1: địa chỉ lệch 0x32BC, dòng bảng 648, kiểu truy cập RW
 * Địa chỉ DDR mặt phẳng UV bộ đệm 1, từ cao.
 */
#define FX1_ISP_CH1_ODMA_UV_ADDR_H1_OFFSET 0x32BCU
#define REG_CH1_ODMA_UV_ADDR_H1 FX1_ISP_CH1_ODMA_UV_ADDR_H1_OFFSET
#define FX1_ISP_CH1_ODMA_UV_ADDR_H1_RESET 0x00000000U
#define FX1_ISP_CH1_ODMA_UV_ADDR_H1_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_CH1_ODMA_UV_ADDR_H1_RO_MASK 0x00000000U
#define FX1_ISP_CH1_ODMA_UV_ADDR_H1_RW_MASK 0xFFFFFFFFU
#define FX1_ISP_CH1_ODMA_UV_ADDR_H1_W1C_MASK 0x00000000U
#define FX1_ISP_CH1_ODMA_UV_ADDR_H1_W1S_MASK 0x00000000U
#define FX1_ISP_CH1_ODMA_UV_ADDR_H1_W1P_MASK 0x00000000U
#define FX1_ISP_CH1_ODMA_UV_ADDR_H1_W1SC_MASK 0x00000000U
/**
 * odma_uv_addr1_hi [31:0], dòng bảng 649, kiểu truy cập RW
 * Các bit [63:32] của địa chỉ DDR.
 */
#define FX1_ISP_CH1_ODMA_UV_ADDR_H1_ODMA_UV_ADDR1_HI_SHIFT 0U
#define FX1_ISP_CH1_ODMA_UV_ADDR_H1_ODMA_UV_ADDR1_HI_WIDTH 32U
#define FX1_ISP_CH1_ODMA_UV_ADDR_H1_ODMA_UV_ADDR1_HI_MASK 0xFFFFFFFFU
#define FX1_ISP_CH1_ODMA_UV_ADDR_H1_ODMA_UV_ADDR1_HI_RESET 0x00000000U
#define FX1_ISP_CH1_ODMA_UV_ADDR_H1_ODMA_UV_ADDR1_HI_ACCESS FX1_ISP_ACCESS_RW

/**
 * CH1_ODMA_Y_ADDR_L2: địa chỉ lệch 0x32C0, dòng bảng 650, kiểu truy cập RW
 * Địa chỉ DDR mặt phẳng Y bộ đệm 2, từ thấp.
 */
#define FX1_ISP_CH1_ODMA_Y_ADDR_L2_OFFSET 0x32C0U
#define REG_CH1_ODMA_Y_ADDR_L2 FX1_ISP_CH1_ODMA_Y_ADDR_L2_OFFSET
#define FX1_ISP_CH1_ODMA_Y_ADDR_L2_RESET 0x00000000U
#define FX1_ISP_CH1_ODMA_Y_ADDR_L2_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_CH1_ODMA_Y_ADDR_L2_RO_MASK 0x00000000U
#define FX1_ISP_CH1_ODMA_Y_ADDR_L2_RW_MASK 0xFFFFFFFFU
#define FX1_ISP_CH1_ODMA_Y_ADDR_L2_W1C_MASK 0x00000000U
#define FX1_ISP_CH1_ODMA_Y_ADDR_L2_W1S_MASK 0x00000000U
#define FX1_ISP_CH1_ODMA_Y_ADDR_L2_W1P_MASK 0x00000000U
#define FX1_ISP_CH1_ODMA_Y_ADDR_L2_W1SC_MASK 0x00000000U
/**
 * odma_y_addr2_lo [31:0], dòng bảng 651, kiểu truy cập RW
 * Các bit [31:0] địa chỉ mặt phẳng Y bộ đệm 2.
 */
#define FX1_ISP_CH1_ODMA_Y_ADDR_L2_ODMA_Y_ADDR2_LO_SHIFT 0U
#define FX1_ISP_CH1_ODMA_Y_ADDR_L2_ODMA_Y_ADDR2_LO_WIDTH 32U
#define FX1_ISP_CH1_ODMA_Y_ADDR_L2_ODMA_Y_ADDR2_LO_MASK 0xFFFFFFFFU
#define FX1_ISP_CH1_ODMA_Y_ADDR_L2_ODMA_Y_ADDR2_LO_RESET 0x00000000U
#define FX1_ISP_CH1_ODMA_Y_ADDR_L2_ODMA_Y_ADDR2_LO_ACCESS FX1_ISP_ACCESS_RW

/**
 * CH1_ODMA_Y_ADDR_H2: địa chỉ lệch 0x32C4, dòng bảng 652, kiểu truy cập RW
 * Địa chỉ DDR mặt phẳng Y bộ đệm 2, từ cao.
 */
#define FX1_ISP_CH1_ODMA_Y_ADDR_H2_OFFSET 0x32C4U
#define REG_CH1_ODMA_Y_ADDR_H2 FX1_ISP_CH1_ODMA_Y_ADDR_H2_OFFSET
#define FX1_ISP_CH1_ODMA_Y_ADDR_H2_RESET 0x00000000U
#define FX1_ISP_CH1_ODMA_Y_ADDR_H2_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_CH1_ODMA_Y_ADDR_H2_RO_MASK 0x00000000U
#define FX1_ISP_CH1_ODMA_Y_ADDR_H2_RW_MASK 0xFFFFFFFFU
#define FX1_ISP_CH1_ODMA_Y_ADDR_H2_W1C_MASK 0x00000000U
#define FX1_ISP_CH1_ODMA_Y_ADDR_H2_W1S_MASK 0x00000000U
#define FX1_ISP_CH1_ODMA_Y_ADDR_H2_W1P_MASK 0x00000000U
#define FX1_ISP_CH1_ODMA_Y_ADDR_H2_W1SC_MASK 0x00000000U
/**
 * odma_y_addr2_hi [31:0], dòng bảng 653, kiểu truy cập RW
 * Các bit [63:32] của địa chỉ DDR.
 */
#define FX1_ISP_CH1_ODMA_Y_ADDR_H2_ODMA_Y_ADDR2_HI_SHIFT 0U
#define FX1_ISP_CH1_ODMA_Y_ADDR_H2_ODMA_Y_ADDR2_HI_WIDTH 32U
#define FX1_ISP_CH1_ODMA_Y_ADDR_H2_ODMA_Y_ADDR2_HI_MASK 0xFFFFFFFFU
#define FX1_ISP_CH1_ODMA_Y_ADDR_H2_ODMA_Y_ADDR2_HI_RESET 0x00000000U
#define FX1_ISP_CH1_ODMA_Y_ADDR_H2_ODMA_Y_ADDR2_HI_ACCESS FX1_ISP_ACCESS_RW

/**
 * CH1_ODMA_UV_ADDR_L2: địa chỉ lệch 0x32C8, dòng bảng 654, kiểu truy cập RW
 * Địa chỉ DDR mặt phẳng UV bộ đệm 2, từ thấp.
 */
#define FX1_ISP_CH1_ODMA_UV_ADDR_L2_OFFSET 0x32C8U
#define REG_CH1_ODMA_UV_ADDR_L2 FX1_ISP_CH1_ODMA_UV_ADDR_L2_OFFSET
#define FX1_ISP_CH1_ODMA_UV_ADDR_L2_RESET 0x00000000U
#define FX1_ISP_CH1_ODMA_UV_ADDR_L2_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_CH1_ODMA_UV_ADDR_L2_RO_MASK 0x00000000U
#define FX1_ISP_CH1_ODMA_UV_ADDR_L2_RW_MASK 0xFFFFFFFFU
#define FX1_ISP_CH1_ODMA_UV_ADDR_L2_W1C_MASK 0x00000000U
#define FX1_ISP_CH1_ODMA_UV_ADDR_L2_W1S_MASK 0x00000000U
#define FX1_ISP_CH1_ODMA_UV_ADDR_L2_W1P_MASK 0x00000000U
#define FX1_ISP_CH1_ODMA_UV_ADDR_L2_W1SC_MASK 0x00000000U
/**
 * odma_uv_addr2_lo [31:0], dòng bảng 655, kiểu truy cập RW
 * Các bit [31:0] địa chỉ mặt phẳng Y bộ đệm 2.
 */
#define FX1_ISP_CH1_ODMA_UV_ADDR_L2_ODMA_UV_ADDR2_LO_SHIFT 0U
#define FX1_ISP_CH1_ODMA_UV_ADDR_L2_ODMA_UV_ADDR2_LO_WIDTH 32U
#define FX1_ISP_CH1_ODMA_UV_ADDR_L2_ODMA_UV_ADDR2_LO_MASK 0xFFFFFFFFU
#define FX1_ISP_CH1_ODMA_UV_ADDR_L2_ODMA_UV_ADDR2_LO_RESET 0x00000000U
#define FX1_ISP_CH1_ODMA_UV_ADDR_L2_ODMA_UV_ADDR2_LO_ACCESS FX1_ISP_ACCESS_RW

/**
 * CH1_ODMA_UV_ADDR_H2: địa chỉ lệch 0x32CC, dòng bảng 656, kiểu truy cập RW
 * Địa chỉ DDR mặt phẳng UV bộ đệm 2, từ cao.
 */
#define FX1_ISP_CH1_ODMA_UV_ADDR_H2_OFFSET 0x32CCU
#define REG_CH1_ODMA_UV_ADDR_H2 FX1_ISP_CH1_ODMA_UV_ADDR_H2_OFFSET
#define FX1_ISP_CH1_ODMA_UV_ADDR_H2_RESET 0x00000000U
#define FX1_ISP_CH1_ODMA_UV_ADDR_H2_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_CH1_ODMA_UV_ADDR_H2_RO_MASK 0x00000000U
#define FX1_ISP_CH1_ODMA_UV_ADDR_H2_RW_MASK 0xFFFFFFFFU
#define FX1_ISP_CH1_ODMA_UV_ADDR_H2_W1C_MASK 0x00000000U
#define FX1_ISP_CH1_ODMA_UV_ADDR_H2_W1S_MASK 0x00000000U
#define FX1_ISP_CH1_ODMA_UV_ADDR_H2_W1P_MASK 0x00000000U
#define FX1_ISP_CH1_ODMA_UV_ADDR_H2_W1SC_MASK 0x00000000U
/**
 * odma_uv_addr2_hi [31:0], dòng bảng 657, kiểu truy cập RW
 * Các bit [63:32] của địa chỉ DDR.
 */
#define FX1_ISP_CH1_ODMA_UV_ADDR_H2_ODMA_UV_ADDR2_HI_SHIFT 0U
#define FX1_ISP_CH1_ODMA_UV_ADDR_H2_ODMA_UV_ADDR2_HI_WIDTH 32U
#define FX1_ISP_CH1_ODMA_UV_ADDR_H2_ODMA_UV_ADDR2_HI_MASK 0xFFFFFFFFU
#define FX1_ISP_CH1_ODMA_UV_ADDR_H2_ODMA_UV_ADDR2_HI_RESET 0x00000000U
#define FX1_ISP_CH1_ODMA_UV_ADDR_H2_ODMA_UV_ADDR2_HI_ACCESS FX1_ISP_ACCESS_RW

/**
 * CH1_ODMA_Y_ADDR_L3: địa chỉ lệch 0x32D0, dòng bảng 658, kiểu truy cập RW
 * Địa chỉ DDR mặt phẳng Y bộ đệm 3, từ thấp.
 */
#define FX1_ISP_CH1_ODMA_Y_ADDR_L3_OFFSET 0x32D0U
#define REG_CH1_ODMA_Y_ADDR_L3 FX1_ISP_CH1_ODMA_Y_ADDR_L3_OFFSET
#define FX1_ISP_CH1_ODMA_Y_ADDR_L3_RESET 0x00000000U
#define FX1_ISP_CH1_ODMA_Y_ADDR_L3_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_CH1_ODMA_Y_ADDR_L3_RO_MASK 0x00000000U
#define FX1_ISP_CH1_ODMA_Y_ADDR_L3_RW_MASK 0xFFFFFFFFU
#define FX1_ISP_CH1_ODMA_Y_ADDR_L3_W1C_MASK 0x00000000U
#define FX1_ISP_CH1_ODMA_Y_ADDR_L3_W1S_MASK 0x00000000U
#define FX1_ISP_CH1_ODMA_Y_ADDR_L3_W1P_MASK 0x00000000U
#define FX1_ISP_CH1_ODMA_Y_ADDR_L3_W1SC_MASK 0x00000000U
/**
 * odma_y_addr3_lo [31:0], dòng bảng 659, kiểu truy cập RW
 * Các bit [31:0] địa chỉ mặt phẳng Y bộ đệm 3.
 */
#define FX1_ISP_CH1_ODMA_Y_ADDR_L3_ODMA_Y_ADDR3_LO_SHIFT 0U
#define FX1_ISP_CH1_ODMA_Y_ADDR_L3_ODMA_Y_ADDR3_LO_WIDTH 32U
#define FX1_ISP_CH1_ODMA_Y_ADDR_L3_ODMA_Y_ADDR3_LO_MASK 0xFFFFFFFFU
#define FX1_ISP_CH1_ODMA_Y_ADDR_L3_ODMA_Y_ADDR3_LO_RESET 0x00000000U
#define FX1_ISP_CH1_ODMA_Y_ADDR_L3_ODMA_Y_ADDR3_LO_ACCESS FX1_ISP_ACCESS_RW

/**
 * CH1_ODMA_Y_ADDR_H3: địa chỉ lệch 0x32D4, dòng bảng 660, kiểu truy cập RW
 * Địa chỉ DDR mặt phẳng Y bộ đệm 3, từ cao.
 */
#define FX1_ISP_CH1_ODMA_Y_ADDR_H3_OFFSET 0x32D4U
#define REG_CH1_ODMA_Y_ADDR_H3 FX1_ISP_CH1_ODMA_Y_ADDR_H3_OFFSET
#define FX1_ISP_CH1_ODMA_Y_ADDR_H3_RESET 0x00000000U
#define FX1_ISP_CH1_ODMA_Y_ADDR_H3_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_CH1_ODMA_Y_ADDR_H3_RO_MASK 0x00000000U
#define FX1_ISP_CH1_ODMA_Y_ADDR_H3_RW_MASK 0xFFFFFFFFU
#define FX1_ISP_CH1_ODMA_Y_ADDR_H3_W1C_MASK 0x00000000U
#define FX1_ISP_CH1_ODMA_Y_ADDR_H3_W1S_MASK 0x00000000U
#define FX1_ISP_CH1_ODMA_Y_ADDR_H3_W1P_MASK 0x00000000U
#define FX1_ISP_CH1_ODMA_Y_ADDR_H3_W1SC_MASK 0x00000000U
/**
 * odma_y_addr3_hi [31:0], dòng bảng 661, kiểu truy cập RW
 * Các bit [63:32] của địa chỉ DDR.
 */
#define FX1_ISP_CH1_ODMA_Y_ADDR_H3_ODMA_Y_ADDR3_HI_SHIFT 0U
#define FX1_ISP_CH1_ODMA_Y_ADDR_H3_ODMA_Y_ADDR3_HI_WIDTH 32U
#define FX1_ISP_CH1_ODMA_Y_ADDR_H3_ODMA_Y_ADDR3_HI_MASK 0xFFFFFFFFU
#define FX1_ISP_CH1_ODMA_Y_ADDR_H3_ODMA_Y_ADDR3_HI_RESET 0x00000000U
#define FX1_ISP_CH1_ODMA_Y_ADDR_H3_ODMA_Y_ADDR3_HI_ACCESS FX1_ISP_ACCESS_RW

/**
 * CH1_ODMA_UV_ADDR_L3: địa chỉ lệch 0x32D8, dòng bảng 662, kiểu truy cập RW
 * Địa chỉ DDR mặt phẳng UV bộ đệm 3, từ thấp.
 */
#define FX1_ISP_CH1_ODMA_UV_ADDR_L3_OFFSET 0x32D8U
#define REG_CH1_ODMA_UV_ADDR_L3 FX1_ISP_CH1_ODMA_UV_ADDR_L3_OFFSET
#define FX1_ISP_CH1_ODMA_UV_ADDR_L3_RESET 0x00000000U
#define FX1_ISP_CH1_ODMA_UV_ADDR_L3_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_CH1_ODMA_UV_ADDR_L3_RO_MASK 0x00000000U
#define FX1_ISP_CH1_ODMA_UV_ADDR_L3_RW_MASK 0xFFFFFFFFU
#define FX1_ISP_CH1_ODMA_UV_ADDR_L3_W1C_MASK 0x00000000U
#define FX1_ISP_CH1_ODMA_UV_ADDR_L3_W1S_MASK 0x00000000U
#define FX1_ISP_CH1_ODMA_UV_ADDR_L3_W1P_MASK 0x00000000U
#define FX1_ISP_CH1_ODMA_UV_ADDR_L3_W1SC_MASK 0x00000000U
/**
 * odma_uv_addr3_lo [31:0], dòng bảng 663, kiểu truy cập RW
 * Các bit [31:0] địa chỉ mặt phẳng Y bộ đệm 3.
 */
#define FX1_ISP_CH1_ODMA_UV_ADDR_L3_ODMA_UV_ADDR3_LO_SHIFT 0U
#define FX1_ISP_CH1_ODMA_UV_ADDR_L3_ODMA_UV_ADDR3_LO_WIDTH 32U
#define FX1_ISP_CH1_ODMA_UV_ADDR_L3_ODMA_UV_ADDR3_LO_MASK 0xFFFFFFFFU
#define FX1_ISP_CH1_ODMA_UV_ADDR_L3_ODMA_UV_ADDR3_LO_RESET 0x00000000U
#define FX1_ISP_CH1_ODMA_UV_ADDR_L3_ODMA_UV_ADDR3_LO_ACCESS FX1_ISP_ACCESS_RW

/**
 * CH1_ODMA_UV_ADDR_H3: địa chỉ lệch 0x32DC, dòng bảng 664, kiểu truy cập RW
 * Địa chỉ DDR mặt phẳng UV bộ đệm 3, từ cao.
 */
#define FX1_ISP_CH1_ODMA_UV_ADDR_H3_OFFSET 0x32DCU
#define REG_CH1_ODMA_UV_ADDR_H3 FX1_ISP_CH1_ODMA_UV_ADDR_H3_OFFSET
#define FX1_ISP_CH1_ODMA_UV_ADDR_H3_RESET 0x00000000U
#define FX1_ISP_CH1_ODMA_UV_ADDR_H3_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_CH1_ODMA_UV_ADDR_H3_RO_MASK 0x00000000U
#define FX1_ISP_CH1_ODMA_UV_ADDR_H3_RW_MASK 0xFFFFFFFFU
#define FX1_ISP_CH1_ODMA_UV_ADDR_H3_W1C_MASK 0x00000000U
#define FX1_ISP_CH1_ODMA_UV_ADDR_H3_W1S_MASK 0x00000000U
#define FX1_ISP_CH1_ODMA_UV_ADDR_H3_W1P_MASK 0x00000000U
#define FX1_ISP_CH1_ODMA_UV_ADDR_H3_W1SC_MASK 0x00000000U
/**
 * odma_uv_addr3_hi [31:0], dòng bảng 665, kiểu truy cập RW
 * Các bit [63:32] của địa chỉ DDR.
 */
#define FX1_ISP_CH1_ODMA_UV_ADDR_H3_ODMA_UV_ADDR3_HI_SHIFT 0U
#define FX1_ISP_CH1_ODMA_UV_ADDR_H3_ODMA_UV_ADDR3_HI_WIDTH 32U
#define FX1_ISP_CH1_ODMA_UV_ADDR_H3_ODMA_UV_ADDR3_HI_MASK 0xFFFFFFFFU
#define FX1_ISP_CH1_ODMA_UV_ADDR_H3_ODMA_UV_ADDR3_HI_RESET 0x00000000U
#define FX1_ISP_CH1_ODMA_UV_ADDR_H3_ODMA_UV_ADDR3_HI_ACCESS FX1_ISP_ACCESS_RW

/**
 * CH1_ODMA_BUF_TAG0: địa chỉ lệch 0x3300, dòng bảng 666, kiểu truy cập RO
 * CH1, buffer 0: Nhãn frame vào công bố khi buffer ra hoàn tất; chỉ số buffer vào/ra có thể khác
 * nhau.
 */
#define FX1_ISP_CH1_ODMA_BUF_TAG0_OFFSET 0x3300U
#define REG_CH1_ODMA_BUF_TAG0 FX1_ISP_CH1_ODMA_BUF_TAG0_OFFSET
#define FX1_ISP_CH1_ODMA_BUF_TAG0_RESET 0x00000000U
#define FX1_ISP_CH1_ODMA_BUF_TAG0_ACCESS FX1_ISP_ACCESS_RO
#define FX1_ISP_CH1_ODMA_BUF_TAG0_RO_MASK 0xFFFFFFFFU
#define FX1_ISP_CH1_ODMA_BUF_TAG0_RW_MASK 0x00000000U
#define FX1_ISP_CH1_ODMA_BUF_TAG0_W1C_MASK 0x00000000U
#define FX1_ISP_CH1_ODMA_BUF_TAG0_W1S_MASK 0x00000000U
#define FX1_ISP_CH1_ODMA_BUF_TAG0_W1P_MASK 0x00000000U
#define FX1_ISP_CH1_ODMA_BUF_TAG0_W1SC_MASK 0x00000000U
/**
 * tag [31:0], dòng bảng 667, kiểu truy cập RO
 * Thông tin frame của bộ đệm 0. Giá trị đầy đủ 32 bit, hard reset về 0. Nhãn frame vào gắn với
 * buffer ra hoàn tất; công bố khi frame ra kết thúc. Chỉ số buffer vào/ra không bắt buộc khớp.
 */
#define FX1_ISP_CH1_ODMA_BUF_TAG0_TAG_SHIFT 0U
#define FX1_ISP_CH1_ODMA_BUF_TAG0_TAG_WIDTH 32U
#define FX1_ISP_CH1_ODMA_BUF_TAG0_TAG_MASK 0xFFFFFFFFU
#define FX1_ISP_CH1_ODMA_BUF_TAG0_TAG_RESET 0x00000000U
#define FX1_ISP_CH1_ODMA_BUF_TAG0_TAG_ACCESS FX1_ISP_ACCESS_RO

/**
 * CH1_ODMA_BUF_TAG1: địa chỉ lệch 0x3304, dòng bảng 668, kiểu truy cập RO
 * CH1, buffer 1: Nhãn frame vào công bố khi buffer ra hoàn tất; chỉ số buffer vào/ra có thể khác
 * nhau.
 */
#define FX1_ISP_CH1_ODMA_BUF_TAG1_OFFSET 0x3304U
#define REG_CH1_ODMA_BUF_TAG1 FX1_ISP_CH1_ODMA_BUF_TAG1_OFFSET
#define FX1_ISP_CH1_ODMA_BUF_TAG1_RESET 0x00000000U
#define FX1_ISP_CH1_ODMA_BUF_TAG1_ACCESS FX1_ISP_ACCESS_RO
#define FX1_ISP_CH1_ODMA_BUF_TAG1_RO_MASK 0xFFFFFFFFU
#define FX1_ISP_CH1_ODMA_BUF_TAG1_RW_MASK 0x00000000U
#define FX1_ISP_CH1_ODMA_BUF_TAG1_W1C_MASK 0x00000000U
#define FX1_ISP_CH1_ODMA_BUF_TAG1_W1S_MASK 0x00000000U
#define FX1_ISP_CH1_ODMA_BUF_TAG1_W1P_MASK 0x00000000U
#define FX1_ISP_CH1_ODMA_BUF_TAG1_W1SC_MASK 0x00000000U
/**
 * tag [31:0], dòng bảng 669, kiểu truy cập RO
 * Thông tin frame của bộ đệm 1. Giá trị đầy đủ 32 bit, hard reset về 0. Nhãn frame vào gắn với
 * buffer ra hoàn tất; công bố khi frame ra kết thúc. Chỉ số buffer vào/ra không bắt buộc khớp.
 */
#define FX1_ISP_CH1_ODMA_BUF_TAG1_TAG_SHIFT 0U
#define FX1_ISP_CH1_ODMA_BUF_TAG1_TAG_WIDTH 32U
#define FX1_ISP_CH1_ODMA_BUF_TAG1_TAG_MASK 0xFFFFFFFFU
#define FX1_ISP_CH1_ODMA_BUF_TAG1_TAG_RESET 0x00000000U
#define FX1_ISP_CH1_ODMA_BUF_TAG1_TAG_ACCESS FX1_ISP_ACCESS_RO

/**
 * CH1_ODMA_BUF_TAG2: địa chỉ lệch 0x3308, dòng bảng 670, kiểu truy cập RO
 * CH1, buffer 2: Nhãn frame vào công bố khi buffer ra hoàn tất; chỉ số buffer vào/ra có thể khác
 * nhau.
 */
#define FX1_ISP_CH1_ODMA_BUF_TAG2_OFFSET 0x3308U
#define REG_CH1_ODMA_BUF_TAG2 FX1_ISP_CH1_ODMA_BUF_TAG2_OFFSET
#define FX1_ISP_CH1_ODMA_BUF_TAG2_RESET 0x00000000U
#define FX1_ISP_CH1_ODMA_BUF_TAG2_ACCESS FX1_ISP_ACCESS_RO
#define FX1_ISP_CH1_ODMA_BUF_TAG2_RO_MASK 0xFFFFFFFFU
#define FX1_ISP_CH1_ODMA_BUF_TAG2_RW_MASK 0x00000000U
#define FX1_ISP_CH1_ODMA_BUF_TAG2_W1C_MASK 0x00000000U
#define FX1_ISP_CH1_ODMA_BUF_TAG2_W1S_MASK 0x00000000U
#define FX1_ISP_CH1_ODMA_BUF_TAG2_W1P_MASK 0x00000000U
#define FX1_ISP_CH1_ODMA_BUF_TAG2_W1SC_MASK 0x00000000U
/**
 * tag [31:0], dòng bảng 671, kiểu truy cập RO
 * Thông tin frame của bộ đệm 2. Giá trị đầy đủ 32 bit, hard reset về 0. Nhãn frame vào gắn với
 * buffer ra hoàn tất; công bố khi frame ra kết thúc. Chỉ số buffer vào/ra không bắt buộc khớp.
 */
#define FX1_ISP_CH1_ODMA_BUF_TAG2_TAG_SHIFT 0U
#define FX1_ISP_CH1_ODMA_BUF_TAG2_TAG_WIDTH 32U
#define FX1_ISP_CH1_ODMA_BUF_TAG2_TAG_MASK 0xFFFFFFFFU
#define FX1_ISP_CH1_ODMA_BUF_TAG2_TAG_RESET 0x00000000U
#define FX1_ISP_CH1_ODMA_BUF_TAG2_TAG_ACCESS FX1_ISP_ACCESS_RO

/**
 * CH1_ODMA_BUF_TAG3: địa chỉ lệch 0x330C, dòng bảng 672, kiểu truy cập RO
 * CH1, buffer 3: Nhãn frame vào công bố khi buffer ra hoàn tất; chỉ số buffer vào/ra có thể khác
 * nhau.
 */
#define FX1_ISP_CH1_ODMA_BUF_TAG3_OFFSET 0x330CU
#define REG_CH1_ODMA_BUF_TAG3 FX1_ISP_CH1_ODMA_BUF_TAG3_OFFSET
#define FX1_ISP_CH1_ODMA_BUF_TAG3_RESET 0x00000000U
#define FX1_ISP_CH1_ODMA_BUF_TAG3_ACCESS FX1_ISP_ACCESS_RO
#define FX1_ISP_CH1_ODMA_BUF_TAG3_RO_MASK 0xFFFFFFFFU
#define FX1_ISP_CH1_ODMA_BUF_TAG3_RW_MASK 0x00000000U
#define FX1_ISP_CH1_ODMA_BUF_TAG3_W1C_MASK 0x00000000U
#define FX1_ISP_CH1_ODMA_BUF_TAG3_W1S_MASK 0x00000000U
#define FX1_ISP_CH1_ODMA_BUF_TAG3_W1P_MASK 0x00000000U
#define FX1_ISP_CH1_ODMA_BUF_TAG3_W1SC_MASK 0x00000000U
/**
 * tag [31:0], dòng bảng 673, kiểu truy cập RO
 * Thông tin frame của bộ đệm 3. Giá trị đầy đủ 32 bit, hard reset về 0. Nhãn frame vào gắn với
 * buffer ra hoàn tất; công bố khi frame ra kết thúc. Chỉ số buffer vào/ra không bắt buộc khớp.
 */
#define FX1_ISP_CH1_ODMA_BUF_TAG3_TAG_SHIFT 0U
#define FX1_ISP_CH1_ODMA_BUF_TAG3_TAG_WIDTH 32U
#define FX1_ISP_CH1_ODMA_BUF_TAG3_TAG_MASK 0xFFFFFFFFU
#define FX1_ISP_CH1_ODMA_BUF_TAG3_TAG_RESET 0x00000000U
#define FX1_ISP_CH1_ODMA_BUF_TAG3_TAG_ACCESS FX1_ISP_ACCESS_RO

/**
 * CH1_ODMA_BUF_TS0: địa chỉ lệch 0x3310, dòng bảng 674, kiểu truy cập RO
 * CH1, buffer 0: Mốc chụp sao cùng nhãn đầu vào, công bố lúc hoàn tất, cùng gốc clock lõi
 * FRAME_COUNTER.
 */
#define FX1_ISP_CH1_ODMA_BUF_TS0_OFFSET 0x3310U
#define REG_CH1_ODMA_BUF_TS0 FX1_ISP_CH1_ODMA_BUF_TS0_OFFSET
#define FX1_ISP_CH1_ODMA_BUF_TS0_RESET 0x00000000U
#define FX1_ISP_CH1_ODMA_BUF_TS0_ACCESS FX1_ISP_ACCESS_RO
#define FX1_ISP_CH1_ODMA_BUF_TS0_RO_MASK 0xFFFFFFFFU
#define FX1_ISP_CH1_ODMA_BUF_TS0_RW_MASK 0x00000000U
#define FX1_ISP_CH1_ODMA_BUF_TS0_W1C_MASK 0x00000000U
#define FX1_ISP_CH1_ODMA_BUF_TS0_W1S_MASK 0x00000000U
#define FX1_ISP_CH1_ODMA_BUF_TS0_W1P_MASK 0x00000000U
#define FX1_ISP_CH1_ODMA_BUF_TS0_W1SC_MASK 0x00000000U
/**
 * timestamp [31:0], dòng bảng 675, kiểu truy cập RO
 * Thông tin frame của bộ đệm 0. Giá trị đầy đủ 32 bit, hard reset về 0. Mốc chụp sao chép cùng nhãn
 * đầu vào và công bố khi đầu ra hoàn tất; cùng gốc thời gian clock lõi như FRAME_COUNTER.
 */
#define FX1_ISP_CH1_ODMA_BUF_TS0_TIMESTAMP_SHIFT 0U
#define FX1_ISP_CH1_ODMA_BUF_TS0_TIMESTAMP_WIDTH 32U
#define FX1_ISP_CH1_ODMA_BUF_TS0_TIMESTAMP_MASK 0xFFFFFFFFU
#define FX1_ISP_CH1_ODMA_BUF_TS0_TIMESTAMP_RESET 0x00000000U
#define FX1_ISP_CH1_ODMA_BUF_TS0_TIMESTAMP_ACCESS FX1_ISP_ACCESS_RO

/**
 * CH1_ODMA_BUF_TS1: địa chỉ lệch 0x3314, dòng bảng 676, kiểu truy cập RO
 * CH1, buffer 1: Mốc chụp sao cùng nhãn đầu vào, công bố lúc hoàn tất, cùng gốc clock lõi
 * FRAME_COUNTER.
 */
#define FX1_ISP_CH1_ODMA_BUF_TS1_OFFSET 0x3314U
#define REG_CH1_ODMA_BUF_TS1 FX1_ISP_CH1_ODMA_BUF_TS1_OFFSET
#define FX1_ISP_CH1_ODMA_BUF_TS1_RESET 0x00000000U
#define FX1_ISP_CH1_ODMA_BUF_TS1_ACCESS FX1_ISP_ACCESS_RO
#define FX1_ISP_CH1_ODMA_BUF_TS1_RO_MASK 0xFFFFFFFFU
#define FX1_ISP_CH1_ODMA_BUF_TS1_RW_MASK 0x00000000U
#define FX1_ISP_CH1_ODMA_BUF_TS1_W1C_MASK 0x00000000U
#define FX1_ISP_CH1_ODMA_BUF_TS1_W1S_MASK 0x00000000U
#define FX1_ISP_CH1_ODMA_BUF_TS1_W1P_MASK 0x00000000U
#define FX1_ISP_CH1_ODMA_BUF_TS1_W1SC_MASK 0x00000000U
/**
 * timestamp [31:0], dòng bảng 677, kiểu truy cập RO
 * Thông tin frame của bộ đệm 1. Giá trị đầy đủ 32 bit, hard reset về 0. Mốc chụp sao chép cùng nhãn
 * đầu vào và công bố khi đầu ra hoàn tất; cùng gốc thời gian clock lõi như FRAME_COUNTER.
 */
#define FX1_ISP_CH1_ODMA_BUF_TS1_TIMESTAMP_SHIFT 0U
#define FX1_ISP_CH1_ODMA_BUF_TS1_TIMESTAMP_WIDTH 32U
#define FX1_ISP_CH1_ODMA_BUF_TS1_TIMESTAMP_MASK 0xFFFFFFFFU
#define FX1_ISP_CH1_ODMA_BUF_TS1_TIMESTAMP_RESET 0x00000000U
#define FX1_ISP_CH1_ODMA_BUF_TS1_TIMESTAMP_ACCESS FX1_ISP_ACCESS_RO

/**
 * CH1_ODMA_BUF_TS2: địa chỉ lệch 0x3318, dòng bảng 678, kiểu truy cập RO
 * CH1, buffer 2: Mốc chụp sao cùng nhãn đầu vào, công bố lúc hoàn tất, cùng gốc clock lõi
 * FRAME_COUNTER.
 */
#define FX1_ISP_CH1_ODMA_BUF_TS2_OFFSET 0x3318U
#define REG_CH1_ODMA_BUF_TS2 FX1_ISP_CH1_ODMA_BUF_TS2_OFFSET
#define FX1_ISP_CH1_ODMA_BUF_TS2_RESET 0x00000000U
#define FX1_ISP_CH1_ODMA_BUF_TS2_ACCESS FX1_ISP_ACCESS_RO
#define FX1_ISP_CH1_ODMA_BUF_TS2_RO_MASK 0xFFFFFFFFU
#define FX1_ISP_CH1_ODMA_BUF_TS2_RW_MASK 0x00000000U
#define FX1_ISP_CH1_ODMA_BUF_TS2_W1C_MASK 0x00000000U
#define FX1_ISP_CH1_ODMA_BUF_TS2_W1S_MASK 0x00000000U
#define FX1_ISP_CH1_ODMA_BUF_TS2_W1P_MASK 0x00000000U
#define FX1_ISP_CH1_ODMA_BUF_TS2_W1SC_MASK 0x00000000U
/**
 * timestamp [31:0], dòng bảng 679, kiểu truy cập RO
 * Thông tin frame của bộ đệm 2. Giá trị đầy đủ 32 bit, hard reset về 0. Mốc chụp sao chép cùng nhãn
 * đầu vào và công bố khi đầu ra hoàn tất; cùng gốc thời gian clock lõi như FRAME_COUNTER.
 */
#define FX1_ISP_CH1_ODMA_BUF_TS2_TIMESTAMP_SHIFT 0U
#define FX1_ISP_CH1_ODMA_BUF_TS2_TIMESTAMP_WIDTH 32U
#define FX1_ISP_CH1_ODMA_BUF_TS2_TIMESTAMP_MASK 0xFFFFFFFFU
#define FX1_ISP_CH1_ODMA_BUF_TS2_TIMESTAMP_RESET 0x00000000U
#define FX1_ISP_CH1_ODMA_BUF_TS2_TIMESTAMP_ACCESS FX1_ISP_ACCESS_RO

/**
 * CH1_ODMA_BUF_TS3: địa chỉ lệch 0x331C, dòng bảng 680, kiểu truy cập RO
 * CH1, buffer 3: Mốc chụp sao cùng nhãn đầu vào, công bố lúc hoàn tất, cùng gốc clock lõi
 * FRAME_COUNTER.
 */
#define FX1_ISP_CH1_ODMA_BUF_TS3_OFFSET 0x331CU
#define REG_CH1_ODMA_BUF_TS3 FX1_ISP_CH1_ODMA_BUF_TS3_OFFSET
#define FX1_ISP_CH1_ODMA_BUF_TS3_RESET 0x00000000U
#define FX1_ISP_CH1_ODMA_BUF_TS3_ACCESS FX1_ISP_ACCESS_RO
#define FX1_ISP_CH1_ODMA_BUF_TS3_RO_MASK 0xFFFFFFFFU
#define FX1_ISP_CH1_ODMA_BUF_TS3_RW_MASK 0x00000000U
#define FX1_ISP_CH1_ODMA_BUF_TS3_W1C_MASK 0x00000000U
#define FX1_ISP_CH1_ODMA_BUF_TS3_W1S_MASK 0x00000000U
#define FX1_ISP_CH1_ODMA_BUF_TS3_W1P_MASK 0x00000000U
#define FX1_ISP_CH1_ODMA_BUF_TS3_W1SC_MASK 0x00000000U
/**
 * timestamp [31:0], dòng bảng 681, kiểu truy cập RO
 * Thông tin frame của bộ đệm 3. Giá trị đầy đủ 32 bit, hard reset về 0. Mốc chụp sao chép cùng nhãn
 * đầu vào và công bố khi đầu ra hoàn tất; cùng gốc thời gian clock lõi như FRAME_COUNTER.
 */
#define FX1_ISP_CH1_ODMA_BUF_TS3_TIMESTAMP_SHIFT 0U
#define FX1_ISP_CH1_ODMA_BUF_TS3_TIMESTAMP_WIDTH 32U
#define FX1_ISP_CH1_ODMA_BUF_TS3_TIMESTAMP_MASK 0xFFFFFFFFU
#define FX1_ISP_CH1_ODMA_BUF_TS3_TIMESTAMP_RESET 0x00000000U
#define FX1_ISP_CH1_ODMA_BUF_TS3_TIMESTAMP_ACCESS FX1_ISP_ACCESS_RO

/**
 * CH1_ODMA_BUF_SOF_TS0: địa chỉ lệch 0x3320, dòng bảng 682, kiểu truy cập RO
 * CH1, buffer 0: Mốc clock lõi chốt ở SOF đầu ra, công bố lúc hoàn tất; dùng với EOF_TS tính thời
 * gian xử lý.
 */
#define FX1_ISP_CH1_ODMA_BUF_SOF_TS0_OFFSET 0x3320U
#define REG_CH1_ODMA_BUF_SOF_TS0 FX1_ISP_CH1_ODMA_BUF_SOF_TS0_OFFSET
#define FX1_ISP_CH1_ODMA_BUF_SOF_TS0_RESET 0x00000000U
#define FX1_ISP_CH1_ODMA_BUF_SOF_TS0_ACCESS FX1_ISP_ACCESS_RO
#define FX1_ISP_CH1_ODMA_BUF_SOF_TS0_RO_MASK 0xFFFFFFFFU
#define FX1_ISP_CH1_ODMA_BUF_SOF_TS0_RW_MASK 0x00000000U
#define FX1_ISP_CH1_ODMA_BUF_SOF_TS0_W1C_MASK 0x00000000U
#define FX1_ISP_CH1_ODMA_BUF_SOF_TS0_W1S_MASK 0x00000000U
#define FX1_ISP_CH1_ODMA_BUF_SOF_TS0_W1P_MASK 0x00000000U
#define FX1_ISP_CH1_ODMA_BUF_SOF_TS0_W1SC_MASK 0x00000000U
/**
 * timestamp [31:0], dòng bảng 683, kiểu truy cập RO
 * Mốc thời gian clock lõi của buffer 0, giá trị 32 bit hard reset 0, chốt ở SOF đầu ra và công bố
 * khi hoàn tất; kết hợp EOF_TS để tính thời gian xử lý frame.
 */
#define FX1_ISP_CH1_ODMA_BUF_SOF_TS0_TIMESTAMP_SHIFT 0U
#define FX1_ISP_CH1_ODMA_BUF_SOF_TS0_TIMESTAMP_WIDTH 32U
#define FX1_ISP_CH1_ODMA_BUF_SOF_TS0_TIMESTAMP_MASK 0xFFFFFFFFU
#define FX1_ISP_CH1_ODMA_BUF_SOF_TS0_TIMESTAMP_RESET 0x00000000U
#define FX1_ISP_CH1_ODMA_BUF_SOF_TS0_TIMESTAMP_ACCESS FX1_ISP_ACCESS_RO

/**
 * CH1_ODMA_BUF_SOF_TS1: địa chỉ lệch 0x3324, dòng bảng 684, kiểu truy cập RO
 * CH1, buffer 1: Mốc clock lõi chốt ở SOF đầu ra, công bố lúc hoàn tất; dùng với EOF_TS tính thời
 * gian xử lý.
 */
#define FX1_ISP_CH1_ODMA_BUF_SOF_TS1_OFFSET 0x3324U
#define REG_CH1_ODMA_BUF_SOF_TS1 FX1_ISP_CH1_ODMA_BUF_SOF_TS1_OFFSET
#define FX1_ISP_CH1_ODMA_BUF_SOF_TS1_RESET 0x00000000U
#define FX1_ISP_CH1_ODMA_BUF_SOF_TS1_ACCESS FX1_ISP_ACCESS_RO
#define FX1_ISP_CH1_ODMA_BUF_SOF_TS1_RO_MASK 0xFFFFFFFFU
#define FX1_ISP_CH1_ODMA_BUF_SOF_TS1_RW_MASK 0x00000000U
#define FX1_ISP_CH1_ODMA_BUF_SOF_TS1_W1C_MASK 0x00000000U
#define FX1_ISP_CH1_ODMA_BUF_SOF_TS1_W1S_MASK 0x00000000U
#define FX1_ISP_CH1_ODMA_BUF_SOF_TS1_W1P_MASK 0x00000000U
#define FX1_ISP_CH1_ODMA_BUF_SOF_TS1_W1SC_MASK 0x00000000U
/**
 * timestamp [31:0], dòng bảng 685, kiểu truy cập RO
 * Mốc thời gian clock lõi của buffer 1, giá trị 32 bit hard reset 0, chốt ở SOF đầu ra và công bố
 * khi hoàn tất; kết hợp EOF_TS để tính thời gian xử lý frame.
 */
#define FX1_ISP_CH1_ODMA_BUF_SOF_TS1_TIMESTAMP_SHIFT 0U
#define FX1_ISP_CH1_ODMA_BUF_SOF_TS1_TIMESTAMP_WIDTH 32U
#define FX1_ISP_CH1_ODMA_BUF_SOF_TS1_TIMESTAMP_MASK 0xFFFFFFFFU
#define FX1_ISP_CH1_ODMA_BUF_SOF_TS1_TIMESTAMP_RESET 0x00000000U
#define FX1_ISP_CH1_ODMA_BUF_SOF_TS1_TIMESTAMP_ACCESS FX1_ISP_ACCESS_RO

/**
 * CH1_ODMA_BUF_SOF_TS2: địa chỉ lệch 0x3328, dòng bảng 686, kiểu truy cập RO
 * CH1, buffer 2: Mốc clock lõi chốt ở SOF đầu ra, công bố lúc hoàn tất; dùng với EOF_TS tính thời
 * gian xử lý.
 */
#define FX1_ISP_CH1_ODMA_BUF_SOF_TS2_OFFSET 0x3328U
#define REG_CH1_ODMA_BUF_SOF_TS2 FX1_ISP_CH1_ODMA_BUF_SOF_TS2_OFFSET
#define FX1_ISP_CH1_ODMA_BUF_SOF_TS2_RESET 0x00000000U
#define FX1_ISP_CH1_ODMA_BUF_SOF_TS2_ACCESS FX1_ISP_ACCESS_RO
#define FX1_ISP_CH1_ODMA_BUF_SOF_TS2_RO_MASK 0xFFFFFFFFU
#define FX1_ISP_CH1_ODMA_BUF_SOF_TS2_RW_MASK 0x00000000U
#define FX1_ISP_CH1_ODMA_BUF_SOF_TS2_W1C_MASK 0x00000000U
#define FX1_ISP_CH1_ODMA_BUF_SOF_TS2_W1S_MASK 0x00000000U
#define FX1_ISP_CH1_ODMA_BUF_SOF_TS2_W1P_MASK 0x00000000U
#define FX1_ISP_CH1_ODMA_BUF_SOF_TS2_W1SC_MASK 0x00000000U
/**
 * timestamp [31:0], dòng bảng 687, kiểu truy cập RO
 * Mốc thời gian clock lõi của buffer 2, giá trị 32 bit hard reset 0, chốt ở SOF đầu ra và công bố
 * khi hoàn tất; kết hợp EOF_TS để tính thời gian xử lý frame.
 */
#define FX1_ISP_CH1_ODMA_BUF_SOF_TS2_TIMESTAMP_SHIFT 0U
#define FX1_ISP_CH1_ODMA_BUF_SOF_TS2_TIMESTAMP_WIDTH 32U
#define FX1_ISP_CH1_ODMA_BUF_SOF_TS2_TIMESTAMP_MASK 0xFFFFFFFFU
#define FX1_ISP_CH1_ODMA_BUF_SOF_TS2_TIMESTAMP_RESET 0x00000000U
#define FX1_ISP_CH1_ODMA_BUF_SOF_TS2_TIMESTAMP_ACCESS FX1_ISP_ACCESS_RO

/**
 * CH1_ODMA_BUF_SOF_TS3: địa chỉ lệch 0x332C, dòng bảng 688, kiểu truy cập RO
 * CH1, buffer 3: Mốc clock lõi chốt ở SOF đầu ra, công bố lúc hoàn tất; dùng với EOF_TS tính thời
 * gian xử lý.
 */
#define FX1_ISP_CH1_ODMA_BUF_SOF_TS3_OFFSET 0x332CU
#define REG_CH1_ODMA_BUF_SOF_TS3 FX1_ISP_CH1_ODMA_BUF_SOF_TS3_OFFSET
#define FX1_ISP_CH1_ODMA_BUF_SOF_TS3_RESET 0x00000000U
#define FX1_ISP_CH1_ODMA_BUF_SOF_TS3_ACCESS FX1_ISP_ACCESS_RO
#define FX1_ISP_CH1_ODMA_BUF_SOF_TS3_RO_MASK 0xFFFFFFFFU
#define FX1_ISP_CH1_ODMA_BUF_SOF_TS3_RW_MASK 0x00000000U
#define FX1_ISP_CH1_ODMA_BUF_SOF_TS3_W1C_MASK 0x00000000U
#define FX1_ISP_CH1_ODMA_BUF_SOF_TS3_W1S_MASK 0x00000000U
#define FX1_ISP_CH1_ODMA_BUF_SOF_TS3_W1P_MASK 0x00000000U
#define FX1_ISP_CH1_ODMA_BUF_SOF_TS3_W1SC_MASK 0x00000000U
/**
 * timestamp [31:0], dòng bảng 689, kiểu truy cập RO
 * Mốc thời gian clock lõi của buffer 3, giá trị 32 bit hard reset 0, chốt ở SOF đầu ra và công bố
 * khi hoàn tất; kết hợp EOF_TS để tính thời gian xử lý frame.
 */
#define FX1_ISP_CH1_ODMA_BUF_SOF_TS3_TIMESTAMP_SHIFT 0U
#define FX1_ISP_CH1_ODMA_BUF_SOF_TS3_TIMESTAMP_WIDTH 32U
#define FX1_ISP_CH1_ODMA_BUF_SOF_TS3_TIMESTAMP_MASK 0xFFFFFFFFU
#define FX1_ISP_CH1_ODMA_BUF_SOF_TS3_TIMESTAMP_RESET 0x00000000U
#define FX1_ISP_CH1_ODMA_BUF_SOF_TS3_TIMESTAMP_ACCESS FX1_ISP_ACCESS_RO

/**
 * CH1_ODMA_BUF_EOF_TS0: địa chỉ lệch 0x3330, dòng bảng 690, kiểu truy cập RO
 * CH1, buffer 0: Mốc clock lõi chốt ở EOF đầu ra, công bố lúc hoàn tất; trừ SOF_TS/mốc chụp modulo
 * 2^32 để tính thời gian xử lý/độ trễ.
 */
#define FX1_ISP_CH1_ODMA_BUF_EOF_TS0_OFFSET 0x3330U
#define REG_CH1_ODMA_BUF_EOF_TS0 FX1_ISP_CH1_ODMA_BUF_EOF_TS0_OFFSET
#define FX1_ISP_CH1_ODMA_BUF_EOF_TS0_RESET 0x00000000U
#define FX1_ISP_CH1_ODMA_BUF_EOF_TS0_ACCESS FX1_ISP_ACCESS_RO
#define FX1_ISP_CH1_ODMA_BUF_EOF_TS0_RO_MASK 0xFFFFFFFFU
#define FX1_ISP_CH1_ODMA_BUF_EOF_TS0_RW_MASK 0x00000000U
#define FX1_ISP_CH1_ODMA_BUF_EOF_TS0_W1C_MASK 0x00000000U
#define FX1_ISP_CH1_ODMA_BUF_EOF_TS0_W1S_MASK 0x00000000U
#define FX1_ISP_CH1_ODMA_BUF_EOF_TS0_W1P_MASK 0x00000000U
#define FX1_ISP_CH1_ODMA_BUF_EOF_TS0_W1SC_MASK 0x00000000U
/**
 * timestamp [31:0], dòng bảng 691, kiểu truy cập RO
 * Mốc thời gian clock lõi của buffer 0, giá trị 32 bit hard reset 0, chốt ở EOF đầu ra và công bố
 * khi hoàn tất; trừ SOF_TS hoặc mốc chụp modulo 2^32 để tính thời gian xử lý/độ trễ đầu cuối.
 */
#define FX1_ISP_CH1_ODMA_BUF_EOF_TS0_TIMESTAMP_SHIFT 0U
#define FX1_ISP_CH1_ODMA_BUF_EOF_TS0_TIMESTAMP_WIDTH 32U
#define FX1_ISP_CH1_ODMA_BUF_EOF_TS0_TIMESTAMP_MASK 0xFFFFFFFFU
#define FX1_ISP_CH1_ODMA_BUF_EOF_TS0_TIMESTAMP_RESET 0x00000000U
#define FX1_ISP_CH1_ODMA_BUF_EOF_TS0_TIMESTAMP_ACCESS FX1_ISP_ACCESS_RO

/**
 * CH1_ODMA_BUF_EOF_TS1: địa chỉ lệch 0x3334, dòng bảng 692, kiểu truy cập RO
 * CH1, buffer 1: Mốc clock lõi chốt ở EOF đầu ra, công bố lúc hoàn tất; trừ SOF_TS/mốc chụp modulo
 * 2^32 để tính thời gian xử lý/độ trễ.
 */
#define FX1_ISP_CH1_ODMA_BUF_EOF_TS1_OFFSET 0x3334U
#define REG_CH1_ODMA_BUF_EOF_TS1 FX1_ISP_CH1_ODMA_BUF_EOF_TS1_OFFSET
#define FX1_ISP_CH1_ODMA_BUF_EOF_TS1_RESET 0x00000000U
#define FX1_ISP_CH1_ODMA_BUF_EOF_TS1_ACCESS FX1_ISP_ACCESS_RO
#define FX1_ISP_CH1_ODMA_BUF_EOF_TS1_RO_MASK 0xFFFFFFFFU
#define FX1_ISP_CH1_ODMA_BUF_EOF_TS1_RW_MASK 0x00000000U
#define FX1_ISP_CH1_ODMA_BUF_EOF_TS1_W1C_MASK 0x00000000U
#define FX1_ISP_CH1_ODMA_BUF_EOF_TS1_W1S_MASK 0x00000000U
#define FX1_ISP_CH1_ODMA_BUF_EOF_TS1_W1P_MASK 0x00000000U
#define FX1_ISP_CH1_ODMA_BUF_EOF_TS1_W1SC_MASK 0x00000000U
/**
 * timestamp [31:0], dòng bảng 693, kiểu truy cập RO
 * Mốc thời gian clock lõi của buffer 1, giá trị 32 bit hard reset 0, chốt ở EOF đầu ra và công bố
 * khi hoàn tất; trừ SOF_TS hoặc mốc chụp modulo 2^32 để tính thời gian xử lý/độ trễ đầu cuối.
 */
#define FX1_ISP_CH1_ODMA_BUF_EOF_TS1_TIMESTAMP_SHIFT 0U
#define FX1_ISP_CH1_ODMA_BUF_EOF_TS1_TIMESTAMP_WIDTH 32U
#define FX1_ISP_CH1_ODMA_BUF_EOF_TS1_TIMESTAMP_MASK 0xFFFFFFFFU
#define FX1_ISP_CH1_ODMA_BUF_EOF_TS1_TIMESTAMP_RESET 0x00000000U
#define FX1_ISP_CH1_ODMA_BUF_EOF_TS1_TIMESTAMP_ACCESS FX1_ISP_ACCESS_RO

/**
 * CH1_ODMA_BUF_EOF_TS2: địa chỉ lệch 0x3338, dòng bảng 694, kiểu truy cập RO
 * CH1, buffer 2: Mốc clock lõi chốt ở EOF đầu ra, công bố lúc hoàn tất; trừ SOF_TS/mốc chụp modulo
 * 2^32 để tính thời gian xử lý/độ trễ.
 */
#define FX1_ISP_CH1_ODMA_BUF_EOF_TS2_OFFSET 0x3338U
#define REG_CH1_ODMA_BUF_EOF_TS2 FX1_ISP_CH1_ODMA_BUF_EOF_TS2_OFFSET
#define FX1_ISP_CH1_ODMA_BUF_EOF_TS2_RESET 0x00000000U
#define FX1_ISP_CH1_ODMA_BUF_EOF_TS2_ACCESS FX1_ISP_ACCESS_RO
#define FX1_ISP_CH1_ODMA_BUF_EOF_TS2_RO_MASK 0xFFFFFFFFU
#define FX1_ISP_CH1_ODMA_BUF_EOF_TS2_RW_MASK 0x00000000U
#define FX1_ISP_CH1_ODMA_BUF_EOF_TS2_W1C_MASK 0x00000000U
#define FX1_ISP_CH1_ODMA_BUF_EOF_TS2_W1S_MASK 0x00000000U
#define FX1_ISP_CH1_ODMA_BUF_EOF_TS2_W1P_MASK 0x00000000U
#define FX1_ISP_CH1_ODMA_BUF_EOF_TS2_W1SC_MASK 0x00000000U
/**
 * timestamp [31:0], dòng bảng 695, kiểu truy cập RO
 * Mốc thời gian clock lõi của buffer 2, giá trị 32 bit hard reset 0, chốt ở EOF đầu ra và công bố
 * khi hoàn tất; trừ SOF_TS hoặc mốc chụp modulo 2^32 để tính thời gian xử lý/độ trễ đầu cuối.
 */
#define FX1_ISP_CH1_ODMA_BUF_EOF_TS2_TIMESTAMP_SHIFT 0U
#define FX1_ISP_CH1_ODMA_BUF_EOF_TS2_TIMESTAMP_WIDTH 32U
#define FX1_ISP_CH1_ODMA_BUF_EOF_TS2_TIMESTAMP_MASK 0xFFFFFFFFU
#define FX1_ISP_CH1_ODMA_BUF_EOF_TS2_TIMESTAMP_RESET 0x00000000U
#define FX1_ISP_CH1_ODMA_BUF_EOF_TS2_TIMESTAMP_ACCESS FX1_ISP_ACCESS_RO

/**
 * CH1_ODMA_BUF_EOF_TS3: địa chỉ lệch 0x333C, dòng bảng 696, kiểu truy cập RO
 * CH1, buffer 3: Mốc clock lõi chốt ở EOF đầu ra, công bố lúc hoàn tất; trừ SOF_TS/mốc chụp modulo
 * 2^32 để tính thời gian xử lý/độ trễ.
 */
#define FX1_ISP_CH1_ODMA_BUF_EOF_TS3_OFFSET 0x333CU
#define REG_CH1_ODMA_BUF_EOF_TS3 FX1_ISP_CH1_ODMA_BUF_EOF_TS3_OFFSET
#define FX1_ISP_CH1_ODMA_BUF_EOF_TS3_RESET 0x00000000U
#define FX1_ISP_CH1_ODMA_BUF_EOF_TS3_ACCESS FX1_ISP_ACCESS_RO
#define FX1_ISP_CH1_ODMA_BUF_EOF_TS3_RO_MASK 0xFFFFFFFFU
#define FX1_ISP_CH1_ODMA_BUF_EOF_TS3_RW_MASK 0x00000000U
#define FX1_ISP_CH1_ODMA_BUF_EOF_TS3_W1C_MASK 0x00000000U
#define FX1_ISP_CH1_ODMA_BUF_EOF_TS3_W1S_MASK 0x00000000U
#define FX1_ISP_CH1_ODMA_BUF_EOF_TS3_W1P_MASK 0x00000000U
#define FX1_ISP_CH1_ODMA_BUF_EOF_TS3_W1SC_MASK 0x00000000U
/**
 * timestamp [31:0], dòng bảng 697, kiểu truy cập RO
 * Mốc thời gian clock lõi của buffer 3, giá trị 32 bit hard reset 0, chốt ở EOF đầu ra và công bố
 * khi hoàn tất; trừ SOF_TS hoặc mốc chụp modulo 2^32 để tính thời gian xử lý/độ trễ đầu cuối.
 */
#define FX1_ISP_CH1_ODMA_BUF_EOF_TS3_TIMESTAMP_SHIFT 0U
#define FX1_ISP_CH1_ODMA_BUF_EOF_TS3_TIMESTAMP_WIDTH 32U
#define FX1_ISP_CH1_ODMA_BUF_EOF_TS3_TIMESTAMP_MASK 0xFFFFFFFFU
#define FX1_ISP_CH1_ODMA_BUF_EOF_TS3_TIMESTAMP_RESET 0x00000000U
#define FX1_ISP_CH1_ODMA_BUF_EOF_TS3_TIMESTAMP_ACCESS FX1_ISP_ACCESS_RO

/**
 * CH2_RESIZER_CTRL: địa chỉ lệch 0x3400, dòng bảng 698, kiểu truy cập RW
 * Resize CH2 RGB888 cho AI, tối đa 640x640, nhận crop sau IQ của kênh. Crop có thể lớn hơn đầu ra;
 * không phóng to/kích thước tùy ý. Từ chối đích sai trước DMA, không tự về FullRes; chờ OUT_W/H ổn
 * định.
 */
#define FX1_ISP_CH2_RESIZER_CTRL_OFFSET 0x3400U
#define REG_CH2_RESIZER_CTRL FX1_ISP_CH2_RESIZER_CTRL_OFFSET
#define FX1_ISP_CH2_RESIZER_CTRL_RESET 0x00000000U
#define FX1_ISP_CH2_RESIZER_CTRL_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_CH2_RESIZER_CTRL_RO_MASK 0x00000000U
#define FX1_ISP_CH2_RESIZER_CTRL_RW_MASK 0x000000FDU
#define FX1_ISP_CH2_RESIZER_CTRL_W1C_MASK 0x00000000U
#define FX1_ISP_CH2_RESIZER_CTRL_W1S_MASK 0x00000000U
#define FX1_ISP_CH2_RESIZER_CTRL_W1P_MASK 0x00000000U
#define FX1_ISP_CH2_RESIZER_CTRL_W1SC_MASK 0x00000000U
/**
 * en [0:0], dòng bảng 699, kiểu truy cập RW
 * Bật resize CH2: 1 dùng SCALE, 0 giữ crop; cả hai phải vừa 640x640. Chỉ đổi sau dừng/xử lý hết và
 * kiểm tra trước DMA.
 */
#define FX1_ISP_CH2_RESIZER_CTRL_EN_SHIFT 0U
#define FX1_ISP_CH2_RESIZER_CTRL_EN_WIDTH 1U
#define FX1_ISP_CH2_RESIZER_CTRL_EN_MASK 0x00000001U
#define FX1_ISP_CH2_RESIZER_CTRL_EN_RESET 0x00000000U
#define FX1_ISP_CH2_RESIZER_CTRL_EN_ACCESS FX1_ISP_ACCESS_RW
/**
 * updated [2:2], dòng bảng 700, kiểu truy cập RW
 * Trường gợi ý cập nhật legacy chỉ lưu, không COMMIT. Đổi active size/crop/enable/SCALE kích hoạt
 * tính lại hình học; kích thước tùy ý dự phòng. Chờ OUT_W/H ổn định trước DMA; chốt hình học tại
 * SOF.
 */
#define FX1_ISP_CH2_RESIZER_CTRL_UPDATED_SHIFT 2U
#define FX1_ISP_CH2_RESIZER_CTRL_UPDATED_WIDTH 1U
#define FX1_ISP_CH2_RESIZER_CTRL_UPDATED_MASK 0x00000004U
#define FX1_ISP_CH2_RESIZER_CTRL_UPDATED_RESET 0x00000000U
#define FX1_ISP_CH2_RESIZER_CTRL_UPDATED_ACCESS FX1_ISP_ACCESS_RW
/**
 * scale [7:3], dòng bảng 701, kiểu truy cập RW
 * Mã CH2 RGB888 [7:3]: 0 giữ
 * crop,8=640x360,14=640x480,16=640x640,17=640x384,18=640x352,19=512x512,20=416x416,21=320x320. Mã
 * khác không hợp lệ. Crop FullRes phải vừa 640x640; đích không vượt crop, không phóng to.
 */
#define FX1_ISP_CH2_RESIZER_CTRL_SCALE_SHIFT 3U
#define FX1_ISP_CH2_RESIZER_CTRL_SCALE_WIDTH 5U
#define FX1_ISP_CH2_RESIZER_CTRL_SCALE_MASK 0x000000F8U
#define FX1_ISP_CH2_RESIZER_CTRL_SCALE_RESET 0x00000000U
#define FX1_ISP_CH2_RESIZER_CTRL_SCALE_ACCESS FX1_ISP_ACCESS_RW

/**
 * CH2_RESIZER_CUSTOM_W: địa chỉ lệch 0x3404, dòng bảng 702, kiểu truy cập RO
 * Địa chỉ kích thước đích cũ, dự phòng ở ABI 1.07: đọc 0, bỏ ghi. Không hỗ trợ resize kích thước tùy
 * ý.
 */
#define FX1_ISP_CH2_RESIZER_CUSTOM_W_OFFSET 0x3404U
#define REG_CH2_RESIZER_CUSTOM_W FX1_ISP_CH2_RESIZER_CUSTOM_W_OFFSET
#define FX1_ISP_CH2_RESIZER_CUSTOM_W_RESET 0x00000000U
#define FX1_ISP_CH2_RESIZER_CUSTOM_W_ACCESS FX1_ISP_ACCESS_RO
#define FX1_ISP_CH2_RESIZER_CUSTOM_W_RO_MASK 0xFFFFFFFFU
#define FX1_ISP_CH2_RESIZER_CUSTOM_W_RW_MASK 0x00000000U
#define FX1_ISP_CH2_RESIZER_CUSTOM_W_W1C_MASK 0x00000000U
#define FX1_ISP_CH2_RESIZER_CUSTOM_W_W1S_MASK 0x00000000U
#define FX1_ISP_CH2_RESIZER_CUSTOM_W_W1P_MASK 0x00000000U
#define FX1_ISP_CH2_RESIZER_CUSTOM_W_W1SC_MASK 0x00000000U
/**
 * RESERVED [31:0], dòng bảng 703, kiểu truy cập RO
 * Đọc 0, bỏ qua ghi.
 */
#define FX1_ISP_CH2_RESIZER_CUSTOM_W_RESERVED_SHIFT 0U
#define FX1_ISP_CH2_RESIZER_CUSTOM_W_RESERVED_WIDTH 32U
#define FX1_ISP_CH2_RESIZER_CUSTOM_W_RESERVED_MASK 0xFFFFFFFFU
#define FX1_ISP_CH2_RESIZER_CUSTOM_W_RESERVED_RESET 0x00000000U
#define FX1_ISP_CH2_RESIZER_CUSTOM_W_RESERVED_ACCESS FX1_ISP_ACCESS_RO

/**
 * CH2_RESIZER_CUSTOM_H: địa chỉ lệch 0x3408, dòng bảng 704, kiểu truy cập RO
 * Địa chỉ kích thước đích cũ, dự phòng ở ABI 1.07: đọc 0, bỏ ghi. Không hỗ trợ resize kích thước tùy
 * ý.
 */
#define FX1_ISP_CH2_RESIZER_CUSTOM_H_OFFSET 0x3408U
#define REG_CH2_RESIZER_CUSTOM_H FX1_ISP_CH2_RESIZER_CUSTOM_H_OFFSET
#define FX1_ISP_CH2_RESIZER_CUSTOM_H_RESET 0x00000000U
#define FX1_ISP_CH2_RESIZER_CUSTOM_H_ACCESS FX1_ISP_ACCESS_RO
#define FX1_ISP_CH2_RESIZER_CUSTOM_H_RO_MASK 0xFFFFFFFFU
#define FX1_ISP_CH2_RESIZER_CUSTOM_H_RW_MASK 0x00000000U
#define FX1_ISP_CH2_RESIZER_CUSTOM_H_W1C_MASK 0x00000000U
#define FX1_ISP_CH2_RESIZER_CUSTOM_H_W1S_MASK 0x00000000U
#define FX1_ISP_CH2_RESIZER_CUSTOM_H_W1P_MASK 0x00000000U
#define FX1_ISP_CH2_RESIZER_CUSTOM_H_W1SC_MASK 0x00000000U
/**
 * RESERVED [31:0], dòng bảng 705, kiểu truy cập RO
 * Đọc 0, bỏ qua ghi.
 */
#define FX1_ISP_CH2_RESIZER_CUSTOM_H_RESERVED_SHIFT 0U
#define FX1_ISP_CH2_RESIZER_CUSTOM_H_RESERVED_WIDTH 32U
#define FX1_ISP_CH2_RESIZER_CUSTOM_H_RESERVED_MASK 0xFFFFFFFFU
#define FX1_ISP_CH2_RESIZER_CUSTOM_H_RESERVED_RESET 0x00000000U
#define FX1_ISP_CH2_RESIZER_CUSTOM_H_RESERVED_ACCESS FX1_ISP_ACCESS_RO

/**
 * CH2_RESIZER_OUT_W: địa chỉ lệch 0x3414, dòng bảng 706, kiểu truy cập RO
 * Kích thước đầu ra suy ra đã kiểm tra. Reset 0; có thể giữ giá trị hoàn tất trước khi hình học hiện
 * tại chưa hợp lệ. Kiểm tra GEOMETRY_STATUS của kênh trước DMA.
 */
#define FX1_ISP_CH2_RESIZER_OUT_W_OFFSET 0x3414U
#define REG_CH2_RESIZER_OUT_W FX1_ISP_CH2_RESIZER_OUT_W_OFFSET
#define FX1_ISP_CH2_RESIZER_OUT_W_RESET 0x00000000U
#define FX1_ISP_CH2_RESIZER_OUT_W_ACCESS FX1_ISP_ACCESS_RO
#define FX1_ISP_CH2_RESIZER_OUT_W_RO_MASK 0x00001FFFU
#define FX1_ISP_CH2_RESIZER_OUT_W_RW_MASK 0x00000000U
#define FX1_ISP_CH2_RESIZER_OUT_W_W1C_MASK 0x00000000U
#define FX1_ISP_CH2_RESIZER_OUT_W_W1S_MASK 0x00000000U
#define FX1_ISP_CH2_RESIZER_OUT_W_W1P_MASK 0x00000000U
#define FX1_ISP_CH2_RESIZER_OUT_W_W1SC_MASK 0x00000000U
/**
 * out_width [12:0], dòng bảng 707, kiểu truy cập RO
 * Kích thước đầu ra suy ra đã kiểm tra. Reset 0; có thể giữ giá trị hoàn tất trước khi hình học hiện
 * tại chưa hợp lệ. Kiểm tra GEOMETRY_STATUS của kênh trước DMA.
 */
#define FX1_ISP_CH2_RESIZER_OUT_W_OUT_WIDTH_SHIFT 0U
#define FX1_ISP_CH2_RESIZER_OUT_W_OUT_WIDTH_WIDTH 13U
#define FX1_ISP_CH2_RESIZER_OUT_W_OUT_WIDTH_MASK 0x00001FFFU
#define FX1_ISP_CH2_RESIZER_OUT_W_OUT_WIDTH_RESET 0x00000000U
#define FX1_ISP_CH2_RESIZER_OUT_W_OUT_WIDTH_ACCESS FX1_ISP_ACCESS_RO

/**
 * CH2_RESIZER_OUT_H: địa chỉ lệch 0x3418, dòng bảng 708, kiểu truy cập RO
 * Kích thước đầu ra suy ra đã kiểm tra. Reset 0; có thể giữ giá trị hoàn tất trước khi hình học hiện
 * tại chưa hợp lệ. Kiểm tra GEOMETRY_STATUS của kênh trước DMA.
 */
#define FX1_ISP_CH2_RESIZER_OUT_H_OFFSET 0x3418U
#define REG_CH2_RESIZER_OUT_H FX1_ISP_CH2_RESIZER_OUT_H_OFFSET
#define FX1_ISP_CH2_RESIZER_OUT_H_RESET 0x00000000U
#define FX1_ISP_CH2_RESIZER_OUT_H_ACCESS FX1_ISP_ACCESS_RO
#define FX1_ISP_CH2_RESIZER_OUT_H_RO_MASK 0x00001FFFU
#define FX1_ISP_CH2_RESIZER_OUT_H_RW_MASK 0x00000000U
#define FX1_ISP_CH2_RESIZER_OUT_H_W1C_MASK 0x00000000U
#define FX1_ISP_CH2_RESIZER_OUT_H_W1S_MASK 0x00000000U
#define FX1_ISP_CH2_RESIZER_OUT_H_W1P_MASK 0x00000000U
#define FX1_ISP_CH2_RESIZER_OUT_H_W1SC_MASK 0x00000000U
/**
 * out_height [12:0], dòng bảng 709, kiểu truy cập RO
 * Kích thước đầu ra suy ra đã kiểm tra. Reset 0; có thể giữ giá trị hoàn tất trước khi hình học hiện
 * tại chưa hợp lệ. Kiểm tra GEOMETRY_STATUS của kênh trước DMA.
 */
#define FX1_ISP_CH2_RESIZER_OUT_H_OUT_HEIGHT_SHIFT 0U
#define FX1_ISP_CH2_RESIZER_OUT_H_OUT_HEIGHT_WIDTH 13U
#define FX1_ISP_CH2_RESIZER_OUT_H_OUT_HEIGHT_MASK 0x00001FFFU
#define FX1_ISP_CH2_RESIZER_OUT_H_OUT_HEIGHT_RESET 0x00000000U
#define FX1_ISP_CH2_RESIZER_OUT_H_OUT_HEIGHT_ACCESS FX1_ISP_ACCESS_RO

/**
 * CH2_RESIZER_STATUS: địa chỉ lệch 0x341C, dòng bảng 710, kiểu truy cập RO
 * Trạng thái khối resizer.
 */
#define FX1_ISP_CH2_RESIZER_STATUS_OFFSET 0x341CU
#define REG_CH2_RESIZER_STATUS FX1_ISP_CH2_RESIZER_STATUS_OFFSET
#define FX1_ISP_CH2_RESIZER_STATUS_RESET 0x00000000U
#define FX1_ISP_CH2_RESIZER_STATUS_ACCESS FX1_ISP_ACCESS_RO
#define FX1_ISP_CH2_RESIZER_STATUS_RO_MASK 0x00000001U
#define FX1_ISP_CH2_RESIZER_STATUS_RW_MASK 0x00000000U
#define FX1_ISP_CH2_RESIZER_STATUS_W1C_MASK 0x00000000U
#define FX1_ISP_CH2_RESIZER_STATUS_W1S_MASK 0x00000000U
#define FX1_ISP_CH2_RESIZER_STATUS_W1P_MASK 0x00000000U
#define FX1_ISP_CH2_RESIZER_STATUS_W1SC_MASK 0x00000000U
/**
 * busy [0:0], dòng bảng 711, kiểu truy cập RO
 * Resizer BUSY=1 khi còn giữ pixel. COMMON_STATUS.BUSY tổng hợp frame trong pipeline và ODMA; thanh
 * ghi này chỉ phản ánh kênh tương ứng.
 */
#define FX1_ISP_CH2_RESIZER_STATUS_BUSY_SHIFT 0U
#define FX1_ISP_CH2_RESIZER_STATUS_BUSY_WIDTH 1U
#define FX1_ISP_CH2_RESIZER_STATUS_BUSY_MASK 0x00000001U
#define FX1_ISP_CH2_RESIZER_STATUS_BUSY_RESET 0x00000000U
#define FX1_ISP_CH2_RESIZER_STATUS_BUSY_ACCESS FX1_ISP_ACCESS_RO

/**
 * CH2_ODMA_Y_STRIDE: địa chỉ lệch 0x3480, dòng bảng 712, kiểu truy cập RW
 * Stride dòng RGB888 đóng gói CH2 theo byte. Tên Y cũ là bí danh mặt phẳng RGB duy nhất; không có UV
 * ở CH2.
 */
#define FX1_ISP_CH2_ODMA_Y_STRIDE_OFFSET 0x3480U
#define REG_CH2_ODMA_Y_STRIDE FX1_ISP_CH2_ODMA_Y_STRIDE_OFFSET
#define FX1_ISP_CH2_ODMA_Y_STRIDE_RESET 0x00000F00U
#define FX1_ISP_CH2_ODMA_Y_STRIDE_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_CH2_ODMA_Y_STRIDE_RO_MASK 0x00000000U
#define FX1_ISP_CH2_ODMA_Y_STRIDE_RW_MASK 0xFFFFFFFFU
#define FX1_ISP_CH2_ODMA_Y_STRIDE_W1C_MASK 0x00000000U
#define FX1_ISP_CH2_ODMA_Y_STRIDE_W1S_MASK 0x00000000U
#define FX1_ISP_CH2_ODMA_Y_STRIDE_W1P_MASK 0x00000000U
#define FX1_ISP_CH2_ODMA_Y_STRIDE_W1SC_MASK 0x00000000U
/**
 * y_stride [31:0], dòng bảng 713, kiểu truy cập RW
 * Số byte giữa dòng RGB888, ít nhất 3*output_width và làm tròn lên bội số byte bus AXI. Reset 3840
 * byte; không ghi các byte padding.
 */
#define FX1_ISP_CH2_ODMA_Y_STRIDE_Y_STRIDE_SHIFT 0U
#define FX1_ISP_CH2_ODMA_Y_STRIDE_Y_STRIDE_WIDTH 32U
#define FX1_ISP_CH2_ODMA_Y_STRIDE_Y_STRIDE_MASK 0xFFFFFFFFU
#define FX1_ISP_CH2_ODMA_Y_STRIDE_Y_STRIDE_RESET 0x00000F00U
#define FX1_ISP_CH2_ODMA_Y_STRIDE_Y_STRIDE_ACCESS FX1_ISP_ACCESS_RW

/**
 * CH2_ODMA_BUF_FREE: địa chỉ lệch 0x3488, dòng bảng 714, kiểu truy cập W1S
 * FREE CH2: phần mềm đặt trước, phần cứng xóa ở đầu frame ra. DONE đặt sau mặt phẳng RGB và mọi phản
 * hồi ghi kết thúc.
 */
#define FX1_ISP_CH2_ODMA_BUF_FREE_OFFSET 0x3488U
#define REG_CH2_ODMA_BUF_FREE FX1_ISP_CH2_ODMA_BUF_FREE_OFFSET
#define FX1_ISP_CH2_ODMA_BUF_FREE_RESET 0x00000000U
#define FX1_ISP_CH2_ODMA_BUF_FREE_ACCESS FX1_ISP_ACCESS_W1S
#define FX1_ISP_CH2_ODMA_BUF_FREE_RO_MASK 0x00000000U
#define FX1_ISP_CH2_ODMA_BUF_FREE_RW_MASK 0x00000000U
#define FX1_ISP_CH2_ODMA_BUF_FREE_W1C_MASK 0x00000000U
#define FX1_ISP_CH2_ODMA_BUF_FREE_W1S_MASK 0x000000FFU
#define FX1_ISP_CH2_ODMA_BUF_FREE_W1P_MASK 0x00000000U
#define FX1_ISP_CH2_ODMA_BUF_FREE_W1SC_MASK 0x00000000U
/**
 * odma_buf_free [7:0], dòng bảng 715, kiểu truy cập W1S
 * FREE bit 0..3 ứng bốn buffer, 7:4 không dùng. Ghi 1 để giao buffer, phần cứng xóa ở đầu frame. Xác
 * nhận DONE trước cấp lại FREE; thiếu buffer làm chờ và báo ODMA_OVERFLOW.
 */
#define FX1_ISP_CH2_ODMA_BUF_FREE_ODMA_BUF_FREE_SHIFT 0U
#define FX1_ISP_CH2_ODMA_BUF_FREE_ODMA_BUF_FREE_WIDTH 8U
#define FX1_ISP_CH2_ODMA_BUF_FREE_ODMA_BUF_FREE_MASK 0x000000FFU
#define FX1_ISP_CH2_ODMA_BUF_FREE_ODMA_BUF_FREE_RESET 0x00000000U
#define FX1_ISP_CH2_ODMA_BUF_FREE_ODMA_BUF_FREE_ACCESS FX1_ISP_ACCESS_W1S

/**
 * CH2_ODMA_BUF_DONE: địa chỉ lệch 0x348C, dòng bảng 716, kiểu truy cập W1C
 * DONE CH2 W1C chỉ đặt khi mặt phẳng RGB và phản hồi AXI ghi hoàn tất; xóa DONE trước cấp lại FREE.
 */
#define FX1_ISP_CH2_ODMA_BUF_DONE_OFFSET 0x348CU
#define REG_CH2_ODMA_BUF_DONE FX1_ISP_CH2_ODMA_BUF_DONE_OFFSET
#define FX1_ISP_CH2_ODMA_BUF_DONE_RESET 0x00000000U
#define FX1_ISP_CH2_ODMA_BUF_DONE_ACCESS FX1_ISP_ACCESS_W1C
#define FX1_ISP_CH2_ODMA_BUF_DONE_RO_MASK 0x00000000U
#define FX1_ISP_CH2_ODMA_BUF_DONE_RW_MASK 0x00000000U
#define FX1_ISP_CH2_ODMA_BUF_DONE_W1C_MASK 0x000000FFU
#define FX1_ISP_CH2_ODMA_BUF_DONE_W1S_MASK 0x00000000U
#define FX1_ISP_CH2_ODMA_BUF_DONE_W1P_MASK 0x00000000U
#define FX1_ISP_CH2_ODMA_BUF_DONE_W1SC_MASK 0x00000000U
/**
 * odma_buf_done [7:0], dòng bảng 717, kiểu truy cập W1C
 * DONE bit 0..3 ứng buffer, 7:4 không dùng. Phần cứng đặt khi frame ra kết thúc đầy đủ; ghi 1 xác
 * nhận, rồi ghi FREE riêng để tái dùng. Hoàn tất mới thắng xóa đồng thời.
 */
#define FX1_ISP_CH2_ODMA_BUF_DONE_ODMA_BUF_DONE_SHIFT 0U
#define FX1_ISP_CH2_ODMA_BUF_DONE_ODMA_BUF_DONE_WIDTH 8U
#define FX1_ISP_CH2_ODMA_BUF_DONE_ODMA_BUF_DONE_MASK 0x000000FFU
#define FX1_ISP_CH2_ODMA_BUF_DONE_ODMA_BUF_DONE_RESET 0x00000000U
#define FX1_ISP_CH2_ODMA_BUF_DONE_ODMA_BUF_DONE_ACCESS FX1_ISP_ACCESS_W1C

/**
 * CH2_ODMA_FRAME_COUNT: địa chỉ lệch 0x3490, dòng bảng 718, kiểu truy cập RO
 * Bộ đếm frame RGB888 đã ghi từ reset, chỉ đọc; ưu tiên chờ bộ đếm thay DONE vì DONE dùng chung W1C
 * với lần xác nhận trước.
 */
#define FX1_ISP_CH2_ODMA_FRAME_COUNT_OFFSET 0x3490U
#define REG_CH2_ODMA_FRAME_COUNT FX1_ISP_CH2_ODMA_FRAME_COUNT_OFFSET
#define FX1_ISP_CH2_ODMA_FRAME_COUNT_RESET 0x00000000U
#define FX1_ISP_CH2_ODMA_FRAME_COUNT_ACCESS FX1_ISP_ACCESS_RO
#define FX1_ISP_CH2_ODMA_FRAME_COUNT_RO_MASK 0xFFFFFFFFU
#define FX1_ISP_CH2_ODMA_FRAME_COUNT_RW_MASK 0x00000000U
#define FX1_ISP_CH2_ODMA_FRAME_COUNT_W1C_MASK 0x00000000U
#define FX1_ISP_CH2_ODMA_FRAME_COUNT_W1S_MASK 0x00000000U
#define FX1_ISP_CH2_ODMA_FRAME_COUNT_W1P_MASK 0x00000000U
#define FX1_ISP_CH2_ODMA_FRAME_COUNT_W1SC_MASK 0x00000000U
/**
 * odma_frame_count [31:0], dòng bảng 719, kiểu truy cập RO
 * Bộ đếm frame đầu ra, cuộn modulo 2^32.
 */
#define FX1_ISP_CH2_ODMA_FRAME_COUNT_ODMA_FRAME_COUNT_SHIFT 0U
#define FX1_ISP_CH2_ODMA_FRAME_COUNT_ODMA_FRAME_COUNT_WIDTH 32U
#define FX1_ISP_CH2_ODMA_FRAME_COUNT_ODMA_FRAME_COUNT_MASK 0xFFFFFFFFU
#define FX1_ISP_CH2_ODMA_FRAME_COUNT_ODMA_FRAME_COUNT_RESET 0x00000000U
#define FX1_ISP_CH2_ODMA_FRAME_COUNT_ODMA_FRAME_COUNT_ACCESS FX1_ISP_ACCESS_RO

/**
 * CH2_ODMA_Y_ADDR_L0: địa chỉ lệch 0x34A0, dòng bảng 720, kiểu truy cập RW
 * Địa chỉ DDR RGB888 đóng gói CH2 buffer 0, từ thấp; tên Y cũ chỉ mặt phẳng RGB duy nhất.
 */
#define FX1_ISP_CH2_ODMA_Y_ADDR_L0_OFFSET 0x34A0U
#define REG_CH2_ODMA_Y_ADDR_L0 FX1_ISP_CH2_ODMA_Y_ADDR_L0_OFFSET
#define FX1_ISP_CH2_ODMA_Y_ADDR_L0_RESET 0x00000000U
#define FX1_ISP_CH2_ODMA_Y_ADDR_L0_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_CH2_ODMA_Y_ADDR_L0_RO_MASK 0x00000000U
#define FX1_ISP_CH2_ODMA_Y_ADDR_L0_RW_MASK 0xFFFFFFFFU
#define FX1_ISP_CH2_ODMA_Y_ADDR_L0_W1C_MASK 0x00000000U
#define FX1_ISP_CH2_ODMA_Y_ADDR_L0_W1S_MASK 0x00000000U
#define FX1_ISP_CH2_ODMA_Y_ADDR_L0_W1P_MASK 0x00000000U
#define FX1_ISP_CH2_ODMA_Y_ADDR_L0_W1SC_MASK 0x00000000U
/**
 * odma_y_addr0_lo [31:0], dòng bảng 721, kiểu truy cập RW
 * Địa chỉ DDR RGB888 đóng gói CH2 buffer 0, các bit [31:0].
 */
#define FX1_ISP_CH2_ODMA_Y_ADDR_L0_ODMA_Y_ADDR0_LO_SHIFT 0U
#define FX1_ISP_CH2_ODMA_Y_ADDR_L0_ODMA_Y_ADDR0_LO_WIDTH 32U
#define FX1_ISP_CH2_ODMA_Y_ADDR_L0_ODMA_Y_ADDR0_LO_MASK 0xFFFFFFFFU
#define FX1_ISP_CH2_ODMA_Y_ADDR_L0_ODMA_Y_ADDR0_LO_RESET 0x00000000U
#define FX1_ISP_CH2_ODMA_Y_ADDR_L0_ODMA_Y_ADDR0_LO_ACCESS FX1_ISP_ACCESS_RW

/**
 * CH2_ODMA_Y_ADDR_H0: địa chỉ lệch 0x34A4, dòng bảng 722, kiểu truy cập RW
 * Địa chỉ DDR RGB888 đóng gói CH2 buffer 0, từ cao; tên Y cũ chỉ mặt phẳng RGB duy nhất.
 */
#define FX1_ISP_CH2_ODMA_Y_ADDR_H0_OFFSET 0x34A4U
#define REG_CH2_ODMA_Y_ADDR_H0 FX1_ISP_CH2_ODMA_Y_ADDR_H0_OFFSET
#define FX1_ISP_CH2_ODMA_Y_ADDR_H0_RESET 0x00000000U
#define FX1_ISP_CH2_ODMA_Y_ADDR_H0_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_CH2_ODMA_Y_ADDR_H0_RO_MASK 0x00000000U
#define FX1_ISP_CH2_ODMA_Y_ADDR_H0_RW_MASK 0xFFFFFFFFU
#define FX1_ISP_CH2_ODMA_Y_ADDR_H0_W1C_MASK 0x00000000U
#define FX1_ISP_CH2_ODMA_Y_ADDR_H0_W1S_MASK 0x00000000U
#define FX1_ISP_CH2_ODMA_Y_ADDR_H0_W1P_MASK 0x00000000U
#define FX1_ISP_CH2_ODMA_Y_ADDR_H0_W1SC_MASK 0x00000000U
/**
 * odma_y_addr0_hi [31:0], dòng bảng 723, kiểu truy cập RW
 * Địa chỉ DDR RGB888 đóng gói CH2 buffer 0, các bit [63:32].
 */
#define FX1_ISP_CH2_ODMA_Y_ADDR_H0_ODMA_Y_ADDR0_HI_SHIFT 0U
#define FX1_ISP_CH2_ODMA_Y_ADDR_H0_ODMA_Y_ADDR0_HI_WIDTH 32U
#define FX1_ISP_CH2_ODMA_Y_ADDR_H0_ODMA_Y_ADDR0_HI_MASK 0xFFFFFFFFU
#define FX1_ISP_CH2_ODMA_Y_ADDR_H0_ODMA_Y_ADDR0_HI_RESET 0x00000000U
#define FX1_ISP_CH2_ODMA_Y_ADDR_H0_ODMA_Y_ADDR0_HI_ACCESS FX1_ISP_ACCESS_RW

/**
 * CH2_ODMA_Y_ADDR_L1: địa chỉ lệch 0x34B0, dòng bảng 724, kiểu truy cập RW
 * Địa chỉ DDR RGB888 đóng gói CH2 buffer 1, từ thấp; tên Y cũ chỉ mặt phẳng RGB duy nhất.
 */
#define FX1_ISP_CH2_ODMA_Y_ADDR_L1_OFFSET 0x34B0U
#define REG_CH2_ODMA_Y_ADDR_L1 FX1_ISP_CH2_ODMA_Y_ADDR_L1_OFFSET
#define FX1_ISP_CH2_ODMA_Y_ADDR_L1_RESET 0x00000000U
#define FX1_ISP_CH2_ODMA_Y_ADDR_L1_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_CH2_ODMA_Y_ADDR_L1_RO_MASK 0x00000000U
#define FX1_ISP_CH2_ODMA_Y_ADDR_L1_RW_MASK 0xFFFFFFFFU
#define FX1_ISP_CH2_ODMA_Y_ADDR_L1_W1C_MASK 0x00000000U
#define FX1_ISP_CH2_ODMA_Y_ADDR_L1_W1S_MASK 0x00000000U
#define FX1_ISP_CH2_ODMA_Y_ADDR_L1_W1P_MASK 0x00000000U
#define FX1_ISP_CH2_ODMA_Y_ADDR_L1_W1SC_MASK 0x00000000U
/**
 * odma_y_addr1_lo [31:0], dòng bảng 725, kiểu truy cập RW
 * Địa chỉ DDR RGB888 đóng gói CH2 buffer 1, các bit [31:0].
 */
#define FX1_ISP_CH2_ODMA_Y_ADDR_L1_ODMA_Y_ADDR1_LO_SHIFT 0U
#define FX1_ISP_CH2_ODMA_Y_ADDR_L1_ODMA_Y_ADDR1_LO_WIDTH 32U
#define FX1_ISP_CH2_ODMA_Y_ADDR_L1_ODMA_Y_ADDR1_LO_MASK 0xFFFFFFFFU
#define FX1_ISP_CH2_ODMA_Y_ADDR_L1_ODMA_Y_ADDR1_LO_RESET 0x00000000U
#define FX1_ISP_CH2_ODMA_Y_ADDR_L1_ODMA_Y_ADDR1_LO_ACCESS FX1_ISP_ACCESS_RW

/**
 * CH2_ODMA_Y_ADDR_H1: địa chỉ lệch 0x34B4, dòng bảng 726, kiểu truy cập RW
 * Địa chỉ DDR RGB888 đóng gói CH2 buffer 1, từ cao; tên Y cũ chỉ mặt phẳng RGB duy nhất.
 */
#define FX1_ISP_CH2_ODMA_Y_ADDR_H1_OFFSET 0x34B4U
#define REG_CH2_ODMA_Y_ADDR_H1 FX1_ISP_CH2_ODMA_Y_ADDR_H1_OFFSET
#define FX1_ISP_CH2_ODMA_Y_ADDR_H1_RESET 0x00000000U
#define FX1_ISP_CH2_ODMA_Y_ADDR_H1_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_CH2_ODMA_Y_ADDR_H1_RO_MASK 0x00000000U
#define FX1_ISP_CH2_ODMA_Y_ADDR_H1_RW_MASK 0xFFFFFFFFU
#define FX1_ISP_CH2_ODMA_Y_ADDR_H1_W1C_MASK 0x00000000U
#define FX1_ISP_CH2_ODMA_Y_ADDR_H1_W1S_MASK 0x00000000U
#define FX1_ISP_CH2_ODMA_Y_ADDR_H1_W1P_MASK 0x00000000U
#define FX1_ISP_CH2_ODMA_Y_ADDR_H1_W1SC_MASK 0x00000000U
/**
 * odma_y_addr1_hi [31:0], dòng bảng 727, kiểu truy cập RW
 * Địa chỉ DDR RGB888 đóng gói CH2 buffer 1, các bit [63:32].
 */
#define FX1_ISP_CH2_ODMA_Y_ADDR_H1_ODMA_Y_ADDR1_HI_SHIFT 0U
#define FX1_ISP_CH2_ODMA_Y_ADDR_H1_ODMA_Y_ADDR1_HI_WIDTH 32U
#define FX1_ISP_CH2_ODMA_Y_ADDR_H1_ODMA_Y_ADDR1_HI_MASK 0xFFFFFFFFU
#define FX1_ISP_CH2_ODMA_Y_ADDR_H1_ODMA_Y_ADDR1_HI_RESET 0x00000000U
#define FX1_ISP_CH2_ODMA_Y_ADDR_H1_ODMA_Y_ADDR1_HI_ACCESS FX1_ISP_ACCESS_RW

/**
 * CH2_ODMA_Y_ADDR_L2: địa chỉ lệch 0x34C0, dòng bảng 728, kiểu truy cập RW
 * Địa chỉ DDR RGB888 đóng gói CH2 buffer 2, từ thấp; tên Y cũ chỉ mặt phẳng RGB duy nhất.
 */
#define FX1_ISP_CH2_ODMA_Y_ADDR_L2_OFFSET 0x34C0U
#define REG_CH2_ODMA_Y_ADDR_L2 FX1_ISP_CH2_ODMA_Y_ADDR_L2_OFFSET
#define FX1_ISP_CH2_ODMA_Y_ADDR_L2_RESET 0x00000000U
#define FX1_ISP_CH2_ODMA_Y_ADDR_L2_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_CH2_ODMA_Y_ADDR_L2_RO_MASK 0x00000000U
#define FX1_ISP_CH2_ODMA_Y_ADDR_L2_RW_MASK 0xFFFFFFFFU
#define FX1_ISP_CH2_ODMA_Y_ADDR_L2_W1C_MASK 0x00000000U
#define FX1_ISP_CH2_ODMA_Y_ADDR_L2_W1S_MASK 0x00000000U
#define FX1_ISP_CH2_ODMA_Y_ADDR_L2_W1P_MASK 0x00000000U
#define FX1_ISP_CH2_ODMA_Y_ADDR_L2_W1SC_MASK 0x00000000U
/**
 * odma_y_addr2_lo [31:0], dòng bảng 729, kiểu truy cập RW
 * Địa chỉ DDR RGB888 đóng gói CH2 buffer 2, các bit [31:0].
 */
#define FX1_ISP_CH2_ODMA_Y_ADDR_L2_ODMA_Y_ADDR2_LO_SHIFT 0U
#define FX1_ISP_CH2_ODMA_Y_ADDR_L2_ODMA_Y_ADDR2_LO_WIDTH 32U
#define FX1_ISP_CH2_ODMA_Y_ADDR_L2_ODMA_Y_ADDR2_LO_MASK 0xFFFFFFFFU
#define FX1_ISP_CH2_ODMA_Y_ADDR_L2_ODMA_Y_ADDR2_LO_RESET 0x00000000U
#define FX1_ISP_CH2_ODMA_Y_ADDR_L2_ODMA_Y_ADDR2_LO_ACCESS FX1_ISP_ACCESS_RW

/**
 * CH2_ODMA_Y_ADDR_H2: địa chỉ lệch 0x34C4, dòng bảng 730, kiểu truy cập RW
 * Địa chỉ DDR RGB888 đóng gói CH2 buffer 2, từ cao; tên Y cũ chỉ mặt phẳng RGB duy nhất.
 */
#define FX1_ISP_CH2_ODMA_Y_ADDR_H2_OFFSET 0x34C4U
#define REG_CH2_ODMA_Y_ADDR_H2 FX1_ISP_CH2_ODMA_Y_ADDR_H2_OFFSET
#define FX1_ISP_CH2_ODMA_Y_ADDR_H2_RESET 0x00000000U
#define FX1_ISP_CH2_ODMA_Y_ADDR_H2_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_CH2_ODMA_Y_ADDR_H2_RO_MASK 0x00000000U
#define FX1_ISP_CH2_ODMA_Y_ADDR_H2_RW_MASK 0xFFFFFFFFU
#define FX1_ISP_CH2_ODMA_Y_ADDR_H2_W1C_MASK 0x00000000U
#define FX1_ISP_CH2_ODMA_Y_ADDR_H2_W1S_MASK 0x00000000U
#define FX1_ISP_CH2_ODMA_Y_ADDR_H2_W1P_MASK 0x00000000U
#define FX1_ISP_CH2_ODMA_Y_ADDR_H2_W1SC_MASK 0x00000000U
/**
 * odma_y_addr2_hi [31:0], dòng bảng 731, kiểu truy cập RW
 * Địa chỉ DDR RGB888 đóng gói CH2 buffer 2, các bit [63:32].
 */
#define FX1_ISP_CH2_ODMA_Y_ADDR_H2_ODMA_Y_ADDR2_HI_SHIFT 0U
#define FX1_ISP_CH2_ODMA_Y_ADDR_H2_ODMA_Y_ADDR2_HI_WIDTH 32U
#define FX1_ISP_CH2_ODMA_Y_ADDR_H2_ODMA_Y_ADDR2_HI_MASK 0xFFFFFFFFU
#define FX1_ISP_CH2_ODMA_Y_ADDR_H2_ODMA_Y_ADDR2_HI_RESET 0x00000000U
#define FX1_ISP_CH2_ODMA_Y_ADDR_H2_ODMA_Y_ADDR2_HI_ACCESS FX1_ISP_ACCESS_RW

/**
 * CH2_ODMA_Y_ADDR_L3: địa chỉ lệch 0x34D0, dòng bảng 732, kiểu truy cập RW
 * Địa chỉ DDR RGB888 đóng gói CH2 buffer 3, từ thấp; tên Y cũ chỉ mặt phẳng RGB duy nhất.
 */
#define FX1_ISP_CH2_ODMA_Y_ADDR_L3_OFFSET 0x34D0U
#define REG_CH2_ODMA_Y_ADDR_L3 FX1_ISP_CH2_ODMA_Y_ADDR_L3_OFFSET
#define FX1_ISP_CH2_ODMA_Y_ADDR_L3_RESET 0x00000000U
#define FX1_ISP_CH2_ODMA_Y_ADDR_L3_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_CH2_ODMA_Y_ADDR_L3_RO_MASK 0x00000000U
#define FX1_ISP_CH2_ODMA_Y_ADDR_L3_RW_MASK 0xFFFFFFFFU
#define FX1_ISP_CH2_ODMA_Y_ADDR_L3_W1C_MASK 0x00000000U
#define FX1_ISP_CH2_ODMA_Y_ADDR_L3_W1S_MASK 0x00000000U
#define FX1_ISP_CH2_ODMA_Y_ADDR_L3_W1P_MASK 0x00000000U
#define FX1_ISP_CH2_ODMA_Y_ADDR_L3_W1SC_MASK 0x00000000U
/**
 * odma_y_addr3_lo [31:0], dòng bảng 733, kiểu truy cập RW
 * Địa chỉ DDR RGB888 đóng gói CH2 buffer 3, các bit [31:0].
 */
#define FX1_ISP_CH2_ODMA_Y_ADDR_L3_ODMA_Y_ADDR3_LO_SHIFT 0U
#define FX1_ISP_CH2_ODMA_Y_ADDR_L3_ODMA_Y_ADDR3_LO_WIDTH 32U
#define FX1_ISP_CH2_ODMA_Y_ADDR_L3_ODMA_Y_ADDR3_LO_MASK 0xFFFFFFFFU
#define FX1_ISP_CH2_ODMA_Y_ADDR_L3_ODMA_Y_ADDR3_LO_RESET 0x00000000U
#define FX1_ISP_CH2_ODMA_Y_ADDR_L3_ODMA_Y_ADDR3_LO_ACCESS FX1_ISP_ACCESS_RW

/**
 * CH2_ODMA_Y_ADDR_H3: địa chỉ lệch 0x34D4, dòng bảng 734, kiểu truy cập RW
 * Địa chỉ DDR RGB888 đóng gói CH2 buffer 3, từ cao; tên Y cũ chỉ mặt phẳng RGB duy nhất.
 */
#define FX1_ISP_CH2_ODMA_Y_ADDR_H3_OFFSET 0x34D4U
#define REG_CH2_ODMA_Y_ADDR_H3 FX1_ISP_CH2_ODMA_Y_ADDR_H3_OFFSET
#define FX1_ISP_CH2_ODMA_Y_ADDR_H3_RESET 0x00000000U
#define FX1_ISP_CH2_ODMA_Y_ADDR_H3_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_CH2_ODMA_Y_ADDR_H3_RO_MASK 0x00000000U
#define FX1_ISP_CH2_ODMA_Y_ADDR_H3_RW_MASK 0xFFFFFFFFU
#define FX1_ISP_CH2_ODMA_Y_ADDR_H3_W1C_MASK 0x00000000U
#define FX1_ISP_CH2_ODMA_Y_ADDR_H3_W1S_MASK 0x00000000U
#define FX1_ISP_CH2_ODMA_Y_ADDR_H3_W1P_MASK 0x00000000U
#define FX1_ISP_CH2_ODMA_Y_ADDR_H3_W1SC_MASK 0x00000000U
/**
 * odma_y_addr3_hi [31:0], dòng bảng 735, kiểu truy cập RW
 * Địa chỉ DDR RGB888 đóng gói CH2 buffer 3, các bit [63:32].
 */
#define FX1_ISP_CH2_ODMA_Y_ADDR_H3_ODMA_Y_ADDR3_HI_SHIFT 0U
#define FX1_ISP_CH2_ODMA_Y_ADDR_H3_ODMA_Y_ADDR3_HI_WIDTH 32U
#define FX1_ISP_CH2_ODMA_Y_ADDR_H3_ODMA_Y_ADDR3_HI_MASK 0xFFFFFFFFU
#define FX1_ISP_CH2_ODMA_Y_ADDR_H3_ODMA_Y_ADDR3_HI_RESET 0x00000000U
#define FX1_ISP_CH2_ODMA_Y_ADDR_H3_ODMA_Y_ADDR3_HI_ACCESS FX1_ISP_ACCESS_RW

/**
 * CH2_ODMA_BUF_TAG0: địa chỉ lệch 0x3500, dòng bảng 736, kiểu truy cập RO
 * CH2, buffer 0: Nhãn frame vào công bố khi buffer ra hoàn tất; chỉ số buffer vào/ra có thể khác
 * nhau.
 */
#define FX1_ISP_CH2_ODMA_BUF_TAG0_OFFSET 0x3500U
#define REG_CH2_ODMA_BUF_TAG0 FX1_ISP_CH2_ODMA_BUF_TAG0_OFFSET
#define FX1_ISP_CH2_ODMA_BUF_TAG0_RESET 0x00000000U
#define FX1_ISP_CH2_ODMA_BUF_TAG0_ACCESS FX1_ISP_ACCESS_RO
#define FX1_ISP_CH2_ODMA_BUF_TAG0_RO_MASK 0xFFFFFFFFU
#define FX1_ISP_CH2_ODMA_BUF_TAG0_RW_MASK 0x00000000U
#define FX1_ISP_CH2_ODMA_BUF_TAG0_W1C_MASK 0x00000000U
#define FX1_ISP_CH2_ODMA_BUF_TAG0_W1S_MASK 0x00000000U
#define FX1_ISP_CH2_ODMA_BUF_TAG0_W1P_MASK 0x00000000U
#define FX1_ISP_CH2_ODMA_BUF_TAG0_W1SC_MASK 0x00000000U
/**
 * tag [31:0], dòng bảng 737, kiểu truy cập RO
 * Thông tin frame của bộ đệm 0. Giá trị đầy đủ 32 bit, hard reset về 0. Nhãn frame vào gắn với
 * buffer ra hoàn tất; công bố khi frame ra kết thúc. Chỉ số buffer vào/ra không bắt buộc khớp.
 */
#define FX1_ISP_CH2_ODMA_BUF_TAG0_TAG_SHIFT 0U
#define FX1_ISP_CH2_ODMA_BUF_TAG0_TAG_WIDTH 32U
#define FX1_ISP_CH2_ODMA_BUF_TAG0_TAG_MASK 0xFFFFFFFFU
#define FX1_ISP_CH2_ODMA_BUF_TAG0_TAG_RESET 0x00000000U
#define FX1_ISP_CH2_ODMA_BUF_TAG0_TAG_ACCESS FX1_ISP_ACCESS_RO

/**
 * CH2_ODMA_BUF_TAG1: địa chỉ lệch 0x3504, dòng bảng 738, kiểu truy cập RO
 * CH2, buffer 1: Nhãn frame vào công bố khi buffer ra hoàn tất; chỉ số buffer vào/ra có thể khác
 * nhau.
 */
#define FX1_ISP_CH2_ODMA_BUF_TAG1_OFFSET 0x3504U
#define REG_CH2_ODMA_BUF_TAG1 FX1_ISP_CH2_ODMA_BUF_TAG1_OFFSET
#define FX1_ISP_CH2_ODMA_BUF_TAG1_RESET 0x00000000U
#define FX1_ISP_CH2_ODMA_BUF_TAG1_ACCESS FX1_ISP_ACCESS_RO
#define FX1_ISP_CH2_ODMA_BUF_TAG1_RO_MASK 0xFFFFFFFFU
#define FX1_ISP_CH2_ODMA_BUF_TAG1_RW_MASK 0x00000000U
#define FX1_ISP_CH2_ODMA_BUF_TAG1_W1C_MASK 0x00000000U
#define FX1_ISP_CH2_ODMA_BUF_TAG1_W1S_MASK 0x00000000U
#define FX1_ISP_CH2_ODMA_BUF_TAG1_W1P_MASK 0x00000000U
#define FX1_ISP_CH2_ODMA_BUF_TAG1_W1SC_MASK 0x00000000U
/**
 * tag [31:0], dòng bảng 739, kiểu truy cập RO
 * Thông tin frame của bộ đệm 1. Giá trị đầy đủ 32 bit, hard reset về 0. Nhãn frame vào gắn với
 * buffer ra hoàn tất; công bố khi frame ra kết thúc. Chỉ số buffer vào/ra không bắt buộc khớp.
 */
#define FX1_ISP_CH2_ODMA_BUF_TAG1_TAG_SHIFT 0U
#define FX1_ISP_CH2_ODMA_BUF_TAG1_TAG_WIDTH 32U
#define FX1_ISP_CH2_ODMA_BUF_TAG1_TAG_MASK 0xFFFFFFFFU
#define FX1_ISP_CH2_ODMA_BUF_TAG1_TAG_RESET 0x00000000U
#define FX1_ISP_CH2_ODMA_BUF_TAG1_TAG_ACCESS FX1_ISP_ACCESS_RO

/**
 * CH2_ODMA_BUF_TAG2: địa chỉ lệch 0x3508, dòng bảng 740, kiểu truy cập RO
 * CH2, buffer 2: Nhãn frame vào công bố khi buffer ra hoàn tất; chỉ số buffer vào/ra có thể khác
 * nhau.
 */
#define FX1_ISP_CH2_ODMA_BUF_TAG2_OFFSET 0x3508U
#define REG_CH2_ODMA_BUF_TAG2 FX1_ISP_CH2_ODMA_BUF_TAG2_OFFSET
#define FX1_ISP_CH2_ODMA_BUF_TAG2_RESET 0x00000000U
#define FX1_ISP_CH2_ODMA_BUF_TAG2_ACCESS FX1_ISP_ACCESS_RO
#define FX1_ISP_CH2_ODMA_BUF_TAG2_RO_MASK 0xFFFFFFFFU
#define FX1_ISP_CH2_ODMA_BUF_TAG2_RW_MASK 0x00000000U
#define FX1_ISP_CH2_ODMA_BUF_TAG2_W1C_MASK 0x00000000U
#define FX1_ISP_CH2_ODMA_BUF_TAG2_W1S_MASK 0x00000000U
#define FX1_ISP_CH2_ODMA_BUF_TAG2_W1P_MASK 0x00000000U
#define FX1_ISP_CH2_ODMA_BUF_TAG2_W1SC_MASK 0x00000000U
/**
 * tag [31:0], dòng bảng 741, kiểu truy cập RO
 * Thông tin frame của bộ đệm 2. Giá trị đầy đủ 32 bit, hard reset về 0. Nhãn frame vào gắn với
 * buffer ra hoàn tất; công bố khi frame ra kết thúc. Chỉ số buffer vào/ra không bắt buộc khớp.
 */
#define FX1_ISP_CH2_ODMA_BUF_TAG2_TAG_SHIFT 0U
#define FX1_ISP_CH2_ODMA_BUF_TAG2_TAG_WIDTH 32U
#define FX1_ISP_CH2_ODMA_BUF_TAG2_TAG_MASK 0xFFFFFFFFU
#define FX1_ISP_CH2_ODMA_BUF_TAG2_TAG_RESET 0x00000000U
#define FX1_ISP_CH2_ODMA_BUF_TAG2_TAG_ACCESS FX1_ISP_ACCESS_RO

/**
 * CH2_ODMA_BUF_TAG3: địa chỉ lệch 0x350C, dòng bảng 742, kiểu truy cập RO
 * CH2, buffer 3: Nhãn frame vào công bố khi buffer ra hoàn tất; chỉ số buffer vào/ra có thể khác
 * nhau.
 */
#define FX1_ISP_CH2_ODMA_BUF_TAG3_OFFSET 0x350CU
#define REG_CH2_ODMA_BUF_TAG3 FX1_ISP_CH2_ODMA_BUF_TAG3_OFFSET
#define FX1_ISP_CH2_ODMA_BUF_TAG3_RESET 0x00000000U
#define FX1_ISP_CH2_ODMA_BUF_TAG3_ACCESS FX1_ISP_ACCESS_RO
#define FX1_ISP_CH2_ODMA_BUF_TAG3_RO_MASK 0xFFFFFFFFU
#define FX1_ISP_CH2_ODMA_BUF_TAG3_RW_MASK 0x00000000U
#define FX1_ISP_CH2_ODMA_BUF_TAG3_W1C_MASK 0x00000000U
#define FX1_ISP_CH2_ODMA_BUF_TAG3_W1S_MASK 0x00000000U
#define FX1_ISP_CH2_ODMA_BUF_TAG3_W1P_MASK 0x00000000U
#define FX1_ISP_CH2_ODMA_BUF_TAG3_W1SC_MASK 0x00000000U
/**
 * tag [31:0], dòng bảng 743, kiểu truy cập RO
 * Thông tin frame của bộ đệm 3. Giá trị đầy đủ 32 bit, hard reset về 0. Nhãn frame vào gắn với
 * buffer ra hoàn tất; công bố khi frame ra kết thúc. Chỉ số buffer vào/ra không bắt buộc khớp.
 */
#define FX1_ISP_CH2_ODMA_BUF_TAG3_TAG_SHIFT 0U
#define FX1_ISP_CH2_ODMA_BUF_TAG3_TAG_WIDTH 32U
#define FX1_ISP_CH2_ODMA_BUF_TAG3_TAG_MASK 0xFFFFFFFFU
#define FX1_ISP_CH2_ODMA_BUF_TAG3_TAG_RESET 0x00000000U
#define FX1_ISP_CH2_ODMA_BUF_TAG3_TAG_ACCESS FX1_ISP_ACCESS_RO

/**
 * CH2_ODMA_BUF_TS0: địa chỉ lệch 0x3510, dòng bảng 744, kiểu truy cập RO
 * CH2, buffer 0: Mốc chụp sao cùng nhãn đầu vào, công bố lúc hoàn tất, cùng gốc clock lõi
 * FRAME_COUNTER.
 */
#define FX1_ISP_CH2_ODMA_BUF_TS0_OFFSET 0x3510U
#define REG_CH2_ODMA_BUF_TS0 FX1_ISP_CH2_ODMA_BUF_TS0_OFFSET
#define FX1_ISP_CH2_ODMA_BUF_TS0_RESET 0x00000000U
#define FX1_ISP_CH2_ODMA_BUF_TS0_ACCESS FX1_ISP_ACCESS_RO
#define FX1_ISP_CH2_ODMA_BUF_TS0_RO_MASK 0xFFFFFFFFU
#define FX1_ISP_CH2_ODMA_BUF_TS0_RW_MASK 0x00000000U
#define FX1_ISP_CH2_ODMA_BUF_TS0_W1C_MASK 0x00000000U
#define FX1_ISP_CH2_ODMA_BUF_TS0_W1S_MASK 0x00000000U
#define FX1_ISP_CH2_ODMA_BUF_TS0_W1P_MASK 0x00000000U
#define FX1_ISP_CH2_ODMA_BUF_TS0_W1SC_MASK 0x00000000U
/**
 * timestamp [31:0], dòng bảng 745, kiểu truy cập RO
 * Thông tin frame của bộ đệm 0. Giá trị đầy đủ 32 bit, hard reset về 0. Mốc chụp sao chép cùng nhãn
 * đầu vào và công bố khi đầu ra hoàn tất; cùng gốc thời gian clock lõi như FRAME_COUNTER.
 */
#define FX1_ISP_CH2_ODMA_BUF_TS0_TIMESTAMP_SHIFT 0U
#define FX1_ISP_CH2_ODMA_BUF_TS0_TIMESTAMP_WIDTH 32U
#define FX1_ISP_CH2_ODMA_BUF_TS0_TIMESTAMP_MASK 0xFFFFFFFFU
#define FX1_ISP_CH2_ODMA_BUF_TS0_TIMESTAMP_RESET 0x00000000U
#define FX1_ISP_CH2_ODMA_BUF_TS0_TIMESTAMP_ACCESS FX1_ISP_ACCESS_RO

/**
 * CH2_ODMA_BUF_TS1: địa chỉ lệch 0x3514, dòng bảng 746, kiểu truy cập RO
 * CH2, buffer 1: Mốc chụp sao cùng nhãn đầu vào, công bố lúc hoàn tất, cùng gốc clock lõi
 * FRAME_COUNTER.
 */
#define FX1_ISP_CH2_ODMA_BUF_TS1_OFFSET 0x3514U
#define REG_CH2_ODMA_BUF_TS1 FX1_ISP_CH2_ODMA_BUF_TS1_OFFSET
#define FX1_ISP_CH2_ODMA_BUF_TS1_RESET 0x00000000U
#define FX1_ISP_CH2_ODMA_BUF_TS1_ACCESS FX1_ISP_ACCESS_RO
#define FX1_ISP_CH2_ODMA_BUF_TS1_RO_MASK 0xFFFFFFFFU
#define FX1_ISP_CH2_ODMA_BUF_TS1_RW_MASK 0x00000000U
#define FX1_ISP_CH2_ODMA_BUF_TS1_W1C_MASK 0x00000000U
#define FX1_ISP_CH2_ODMA_BUF_TS1_W1S_MASK 0x00000000U
#define FX1_ISP_CH2_ODMA_BUF_TS1_W1P_MASK 0x00000000U
#define FX1_ISP_CH2_ODMA_BUF_TS1_W1SC_MASK 0x00000000U
/**
 * timestamp [31:0], dòng bảng 747, kiểu truy cập RO
 * Thông tin frame của bộ đệm 1. Giá trị đầy đủ 32 bit, hard reset về 0. Mốc chụp sao chép cùng nhãn
 * đầu vào và công bố khi đầu ra hoàn tất; cùng gốc thời gian clock lõi như FRAME_COUNTER.
 */
#define FX1_ISP_CH2_ODMA_BUF_TS1_TIMESTAMP_SHIFT 0U
#define FX1_ISP_CH2_ODMA_BUF_TS1_TIMESTAMP_WIDTH 32U
#define FX1_ISP_CH2_ODMA_BUF_TS1_TIMESTAMP_MASK 0xFFFFFFFFU
#define FX1_ISP_CH2_ODMA_BUF_TS1_TIMESTAMP_RESET 0x00000000U
#define FX1_ISP_CH2_ODMA_BUF_TS1_TIMESTAMP_ACCESS FX1_ISP_ACCESS_RO

/**
 * CH2_ODMA_BUF_TS2: địa chỉ lệch 0x3518, dòng bảng 748, kiểu truy cập RO
 * CH2, buffer 2: Mốc chụp sao cùng nhãn đầu vào, công bố lúc hoàn tất, cùng gốc clock lõi
 * FRAME_COUNTER.
 */
#define FX1_ISP_CH2_ODMA_BUF_TS2_OFFSET 0x3518U
#define REG_CH2_ODMA_BUF_TS2 FX1_ISP_CH2_ODMA_BUF_TS2_OFFSET
#define FX1_ISP_CH2_ODMA_BUF_TS2_RESET 0x00000000U
#define FX1_ISP_CH2_ODMA_BUF_TS2_ACCESS FX1_ISP_ACCESS_RO
#define FX1_ISP_CH2_ODMA_BUF_TS2_RO_MASK 0xFFFFFFFFU
#define FX1_ISP_CH2_ODMA_BUF_TS2_RW_MASK 0x00000000U
#define FX1_ISP_CH2_ODMA_BUF_TS2_W1C_MASK 0x00000000U
#define FX1_ISP_CH2_ODMA_BUF_TS2_W1S_MASK 0x00000000U
#define FX1_ISP_CH2_ODMA_BUF_TS2_W1P_MASK 0x00000000U
#define FX1_ISP_CH2_ODMA_BUF_TS2_W1SC_MASK 0x00000000U
/**
 * timestamp [31:0], dòng bảng 749, kiểu truy cập RO
 * Thông tin frame của bộ đệm 2. Giá trị đầy đủ 32 bit, hard reset về 0. Mốc chụp sao chép cùng nhãn
 * đầu vào và công bố khi đầu ra hoàn tất; cùng gốc thời gian clock lõi như FRAME_COUNTER.
 */
#define FX1_ISP_CH2_ODMA_BUF_TS2_TIMESTAMP_SHIFT 0U
#define FX1_ISP_CH2_ODMA_BUF_TS2_TIMESTAMP_WIDTH 32U
#define FX1_ISP_CH2_ODMA_BUF_TS2_TIMESTAMP_MASK 0xFFFFFFFFU
#define FX1_ISP_CH2_ODMA_BUF_TS2_TIMESTAMP_RESET 0x00000000U
#define FX1_ISP_CH2_ODMA_BUF_TS2_TIMESTAMP_ACCESS FX1_ISP_ACCESS_RO

/**
 * CH2_ODMA_BUF_TS3: địa chỉ lệch 0x351C, dòng bảng 750, kiểu truy cập RO
 * CH2, buffer 3: Mốc chụp sao cùng nhãn đầu vào, công bố lúc hoàn tất, cùng gốc clock lõi
 * FRAME_COUNTER.
 */
#define FX1_ISP_CH2_ODMA_BUF_TS3_OFFSET 0x351CU
#define REG_CH2_ODMA_BUF_TS3 FX1_ISP_CH2_ODMA_BUF_TS3_OFFSET
#define FX1_ISP_CH2_ODMA_BUF_TS3_RESET 0x00000000U
#define FX1_ISP_CH2_ODMA_BUF_TS3_ACCESS FX1_ISP_ACCESS_RO
#define FX1_ISP_CH2_ODMA_BUF_TS3_RO_MASK 0xFFFFFFFFU
#define FX1_ISP_CH2_ODMA_BUF_TS3_RW_MASK 0x00000000U
#define FX1_ISP_CH2_ODMA_BUF_TS3_W1C_MASK 0x00000000U
#define FX1_ISP_CH2_ODMA_BUF_TS3_W1S_MASK 0x00000000U
#define FX1_ISP_CH2_ODMA_BUF_TS3_W1P_MASK 0x00000000U
#define FX1_ISP_CH2_ODMA_BUF_TS3_W1SC_MASK 0x00000000U
/**
 * timestamp [31:0], dòng bảng 751, kiểu truy cập RO
 * Thông tin frame của bộ đệm 3. Giá trị đầy đủ 32 bit, hard reset về 0. Mốc chụp sao chép cùng nhãn
 * đầu vào và công bố khi đầu ra hoàn tất; cùng gốc thời gian clock lõi như FRAME_COUNTER.
 */
#define FX1_ISP_CH2_ODMA_BUF_TS3_TIMESTAMP_SHIFT 0U
#define FX1_ISP_CH2_ODMA_BUF_TS3_TIMESTAMP_WIDTH 32U
#define FX1_ISP_CH2_ODMA_BUF_TS3_TIMESTAMP_MASK 0xFFFFFFFFU
#define FX1_ISP_CH2_ODMA_BUF_TS3_TIMESTAMP_RESET 0x00000000U
#define FX1_ISP_CH2_ODMA_BUF_TS3_TIMESTAMP_ACCESS FX1_ISP_ACCESS_RO

/**
 * CH2_ODMA_BUF_SOF_TS0: địa chỉ lệch 0x3520, dòng bảng 752, kiểu truy cập RO
 * CH2, buffer 0: Mốc clock lõi chốt ở SOF đầu ra, công bố lúc hoàn tất; dùng với EOF_TS tính thời
 * gian xử lý.
 */
#define FX1_ISP_CH2_ODMA_BUF_SOF_TS0_OFFSET 0x3520U
#define REG_CH2_ODMA_BUF_SOF_TS0 FX1_ISP_CH2_ODMA_BUF_SOF_TS0_OFFSET
#define FX1_ISP_CH2_ODMA_BUF_SOF_TS0_RESET 0x00000000U
#define FX1_ISP_CH2_ODMA_BUF_SOF_TS0_ACCESS FX1_ISP_ACCESS_RO
#define FX1_ISP_CH2_ODMA_BUF_SOF_TS0_RO_MASK 0xFFFFFFFFU
#define FX1_ISP_CH2_ODMA_BUF_SOF_TS0_RW_MASK 0x00000000U
#define FX1_ISP_CH2_ODMA_BUF_SOF_TS0_W1C_MASK 0x00000000U
#define FX1_ISP_CH2_ODMA_BUF_SOF_TS0_W1S_MASK 0x00000000U
#define FX1_ISP_CH2_ODMA_BUF_SOF_TS0_W1P_MASK 0x00000000U
#define FX1_ISP_CH2_ODMA_BUF_SOF_TS0_W1SC_MASK 0x00000000U
/**
 * timestamp [31:0], dòng bảng 753, kiểu truy cập RO
 * Mốc thời gian clock lõi của buffer 0, giá trị 32 bit hard reset 0, chốt ở SOF đầu ra và công bố
 * khi hoàn tất; kết hợp EOF_TS để tính thời gian xử lý frame.
 */
#define FX1_ISP_CH2_ODMA_BUF_SOF_TS0_TIMESTAMP_SHIFT 0U
#define FX1_ISP_CH2_ODMA_BUF_SOF_TS0_TIMESTAMP_WIDTH 32U
#define FX1_ISP_CH2_ODMA_BUF_SOF_TS0_TIMESTAMP_MASK 0xFFFFFFFFU
#define FX1_ISP_CH2_ODMA_BUF_SOF_TS0_TIMESTAMP_RESET 0x00000000U
#define FX1_ISP_CH2_ODMA_BUF_SOF_TS0_TIMESTAMP_ACCESS FX1_ISP_ACCESS_RO

/**
 * CH2_ODMA_BUF_SOF_TS1: địa chỉ lệch 0x3524, dòng bảng 754, kiểu truy cập RO
 * CH2, buffer 1: Mốc clock lõi chốt ở SOF đầu ra, công bố lúc hoàn tất; dùng với EOF_TS tính thời
 * gian xử lý.
 */
#define FX1_ISP_CH2_ODMA_BUF_SOF_TS1_OFFSET 0x3524U
#define REG_CH2_ODMA_BUF_SOF_TS1 FX1_ISP_CH2_ODMA_BUF_SOF_TS1_OFFSET
#define FX1_ISP_CH2_ODMA_BUF_SOF_TS1_RESET 0x00000000U
#define FX1_ISP_CH2_ODMA_BUF_SOF_TS1_ACCESS FX1_ISP_ACCESS_RO
#define FX1_ISP_CH2_ODMA_BUF_SOF_TS1_RO_MASK 0xFFFFFFFFU
#define FX1_ISP_CH2_ODMA_BUF_SOF_TS1_RW_MASK 0x00000000U
#define FX1_ISP_CH2_ODMA_BUF_SOF_TS1_W1C_MASK 0x00000000U
#define FX1_ISP_CH2_ODMA_BUF_SOF_TS1_W1S_MASK 0x00000000U
#define FX1_ISP_CH2_ODMA_BUF_SOF_TS1_W1P_MASK 0x00000000U
#define FX1_ISP_CH2_ODMA_BUF_SOF_TS1_W1SC_MASK 0x00000000U
/**
 * timestamp [31:0], dòng bảng 755, kiểu truy cập RO
 * Mốc thời gian clock lõi của buffer 1, giá trị 32 bit hard reset 0, chốt ở SOF đầu ra và công bố
 * khi hoàn tất; kết hợp EOF_TS để tính thời gian xử lý frame.
 */
#define FX1_ISP_CH2_ODMA_BUF_SOF_TS1_TIMESTAMP_SHIFT 0U
#define FX1_ISP_CH2_ODMA_BUF_SOF_TS1_TIMESTAMP_WIDTH 32U
#define FX1_ISP_CH2_ODMA_BUF_SOF_TS1_TIMESTAMP_MASK 0xFFFFFFFFU
#define FX1_ISP_CH2_ODMA_BUF_SOF_TS1_TIMESTAMP_RESET 0x00000000U
#define FX1_ISP_CH2_ODMA_BUF_SOF_TS1_TIMESTAMP_ACCESS FX1_ISP_ACCESS_RO

/**
 * CH2_ODMA_BUF_SOF_TS2: địa chỉ lệch 0x3528, dòng bảng 756, kiểu truy cập RO
 * CH2, buffer 2: Mốc clock lõi chốt ở SOF đầu ra, công bố lúc hoàn tất; dùng với EOF_TS tính thời
 * gian xử lý.
 */
#define FX1_ISP_CH2_ODMA_BUF_SOF_TS2_OFFSET 0x3528U
#define REG_CH2_ODMA_BUF_SOF_TS2 FX1_ISP_CH2_ODMA_BUF_SOF_TS2_OFFSET
#define FX1_ISP_CH2_ODMA_BUF_SOF_TS2_RESET 0x00000000U
#define FX1_ISP_CH2_ODMA_BUF_SOF_TS2_ACCESS FX1_ISP_ACCESS_RO
#define FX1_ISP_CH2_ODMA_BUF_SOF_TS2_RO_MASK 0xFFFFFFFFU
#define FX1_ISP_CH2_ODMA_BUF_SOF_TS2_RW_MASK 0x00000000U
#define FX1_ISP_CH2_ODMA_BUF_SOF_TS2_W1C_MASK 0x00000000U
#define FX1_ISP_CH2_ODMA_BUF_SOF_TS2_W1S_MASK 0x00000000U
#define FX1_ISP_CH2_ODMA_BUF_SOF_TS2_W1P_MASK 0x00000000U
#define FX1_ISP_CH2_ODMA_BUF_SOF_TS2_W1SC_MASK 0x00000000U
/**
 * timestamp [31:0], dòng bảng 757, kiểu truy cập RO
 * Mốc thời gian clock lõi của buffer 2, giá trị 32 bit hard reset 0, chốt ở SOF đầu ra và công bố
 * khi hoàn tất; kết hợp EOF_TS để tính thời gian xử lý frame.
 */
#define FX1_ISP_CH2_ODMA_BUF_SOF_TS2_TIMESTAMP_SHIFT 0U
#define FX1_ISP_CH2_ODMA_BUF_SOF_TS2_TIMESTAMP_WIDTH 32U
#define FX1_ISP_CH2_ODMA_BUF_SOF_TS2_TIMESTAMP_MASK 0xFFFFFFFFU
#define FX1_ISP_CH2_ODMA_BUF_SOF_TS2_TIMESTAMP_RESET 0x00000000U
#define FX1_ISP_CH2_ODMA_BUF_SOF_TS2_TIMESTAMP_ACCESS FX1_ISP_ACCESS_RO

/**
 * CH2_ODMA_BUF_SOF_TS3: địa chỉ lệch 0x352C, dòng bảng 758, kiểu truy cập RO
 * CH2, buffer 3: Mốc clock lõi chốt ở SOF đầu ra, công bố lúc hoàn tất; dùng với EOF_TS tính thời
 * gian xử lý.
 */
#define FX1_ISP_CH2_ODMA_BUF_SOF_TS3_OFFSET 0x352CU
#define REG_CH2_ODMA_BUF_SOF_TS3 FX1_ISP_CH2_ODMA_BUF_SOF_TS3_OFFSET
#define FX1_ISP_CH2_ODMA_BUF_SOF_TS3_RESET 0x00000000U
#define FX1_ISP_CH2_ODMA_BUF_SOF_TS3_ACCESS FX1_ISP_ACCESS_RO
#define FX1_ISP_CH2_ODMA_BUF_SOF_TS3_RO_MASK 0xFFFFFFFFU
#define FX1_ISP_CH2_ODMA_BUF_SOF_TS3_RW_MASK 0x00000000U
#define FX1_ISP_CH2_ODMA_BUF_SOF_TS3_W1C_MASK 0x00000000U
#define FX1_ISP_CH2_ODMA_BUF_SOF_TS3_W1S_MASK 0x00000000U
#define FX1_ISP_CH2_ODMA_BUF_SOF_TS3_W1P_MASK 0x00000000U
#define FX1_ISP_CH2_ODMA_BUF_SOF_TS3_W1SC_MASK 0x00000000U
/**
 * timestamp [31:0], dòng bảng 759, kiểu truy cập RO
 * Mốc thời gian clock lõi của buffer 3, giá trị 32 bit hard reset 0, chốt ở SOF đầu ra và công bố
 * khi hoàn tất; kết hợp EOF_TS để tính thời gian xử lý frame.
 */
#define FX1_ISP_CH2_ODMA_BUF_SOF_TS3_TIMESTAMP_SHIFT 0U
#define FX1_ISP_CH2_ODMA_BUF_SOF_TS3_TIMESTAMP_WIDTH 32U
#define FX1_ISP_CH2_ODMA_BUF_SOF_TS3_TIMESTAMP_MASK 0xFFFFFFFFU
#define FX1_ISP_CH2_ODMA_BUF_SOF_TS3_TIMESTAMP_RESET 0x00000000U
#define FX1_ISP_CH2_ODMA_BUF_SOF_TS3_TIMESTAMP_ACCESS FX1_ISP_ACCESS_RO

/**
 * CH2_ODMA_BUF_EOF_TS0: địa chỉ lệch 0x3530, dòng bảng 760, kiểu truy cập RO
 * CH2, buffer 0: Mốc clock lõi chốt ở EOF đầu ra, công bố lúc hoàn tất; trừ SOF_TS/mốc chụp modulo
 * 2^32 để tính thời gian xử lý/độ trễ.
 */
#define FX1_ISP_CH2_ODMA_BUF_EOF_TS0_OFFSET 0x3530U
#define REG_CH2_ODMA_BUF_EOF_TS0 FX1_ISP_CH2_ODMA_BUF_EOF_TS0_OFFSET
#define FX1_ISP_CH2_ODMA_BUF_EOF_TS0_RESET 0x00000000U
#define FX1_ISP_CH2_ODMA_BUF_EOF_TS0_ACCESS FX1_ISP_ACCESS_RO
#define FX1_ISP_CH2_ODMA_BUF_EOF_TS0_RO_MASK 0xFFFFFFFFU
#define FX1_ISP_CH2_ODMA_BUF_EOF_TS0_RW_MASK 0x00000000U
#define FX1_ISP_CH2_ODMA_BUF_EOF_TS0_W1C_MASK 0x00000000U
#define FX1_ISP_CH2_ODMA_BUF_EOF_TS0_W1S_MASK 0x00000000U
#define FX1_ISP_CH2_ODMA_BUF_EOF_TS0_W1P_MASK 0x00000000U
#define FX1_ISP_CH2_ODMA_BUF_EOF_TS0_W1SC_MASK 0x00000000U
/**
 * timestamp [31:0], dòng bảng 761, kiểu truy cập RO
 * Mốc thời gian clock lõi của buffer 0, giá trị 32 bit hard reset 0, chốt ở EOF đầu ra và công bố
 * khi hoàn tất; trừ SOF_TS hoặc mốc chụp modulo 2^32 để tính thời gian xử lý/độ trễ đầu cuối.
 */
#define FX1_ISP_CH2_ODMA_BUF_EOF_TS0_TIMESTAMP_SHIFT 0U
#define FX1_ISP_CH2_ODMA_BUF_EOF_TS0_TIMESTAMP_WIDTH 32U
#define FX1_ISP_CH2_ODMA_BUF_EOF_TS0_TIMESTAMP_MASK 0xFFFFFFFFU
#define FX1_ISP_CH2_ODMA_BUF_EOF_TS0_TIMESTAMP_RESET 0x00000000U
#define FX1_ISP_CH2_ODMA_BUF_EOF_TS0_TIMESTAMP_ACCESS FX1_ISP_ACCESS_RO

/**
 * CH2_ODMA_BUF_EOF_TS1: địa chỉ lệch 0x3534, dòng bảng 762, kiểu truy cập RO
 * CH2, buffer 1: Mốc clock lõi chốt ở EOF đầu ra, công bố lúc hoàn tất; trừ SOF_TS/mốc chụp modulo
 * 2^32 để tính thời gian xử lý/độ trễ.
 */
#define FX1_ISP_CH2_ODMA_BUF_EOF_TS1_OFFSET 0x3534U
#define REG_CH2_ODMA_BUF_EOF_TS1 FX1_ISP_CH2_ODMA_BUF_EOF_TS1_OFFSET
#define FX1_ISP_CH2_ODMA_BUF_EOF_TS1_RESET 0x00000000U
#define FX1_ISP_CH2_ODMA_BUF_EOF_TS1_ACCESS FX1_ISP_ACCESS_RO
#define FX1_ISP_CH2_ODMA_BUF_EOF_TS1_RO_MASK 0xFFFFFFFFU
#define FX1_ISP_CH2_ODMA_BUF_EOF_TS1_RW_MASK 0x00000000U
#define FX1_ISP_CH2_ODMA_BUF_EOF_TS1_W1C_MASK 0x00000000U
#define FX1_ISP_CH2_ODMA_BUF_EOF_TS1_W1S_MASK 0x00000000U
#define FX1_ISP_CH2_ODMA_BUF_EOF_TS1_W1P_MASK 0x00000000U
#define FX1_ISP_CH2_ODMA_BUF_EOF_TS1_W1SC_MASK 0x00000000U
/**
 * timestamp [31:0], dòng bảng 763, kiểu truy cập RO
 * Mốc thời gian clock lõi của buffer 1, giá trị 32 bit hard reset 0, chốt ở EOF đầu ra và công bố
 * khi hoàn tất; trừ SOF_TS hoặc mốc chụp modulo 2^32 để tính thời gian xử lý/độ trễ đầu cuối.
 */
#define FX1_ISP_CH2_ODMA_BUF_EOF_TS1_TIMESTAMP_SHIFT 0U
#define FX1_ISP_CH2_ODMA_BUF_EOF_TS1_TIMESTAMP_WIDTH 32U
#define FX1_ISP_CH2_ODMA_BUF_EOF_TS1_TIMESTAMP_MASK 0xFFFFFFFFU
#define FX1_ISP_CH2_ODMA_BUF_EOF_TS1_TIMESTAMP_RESET 0x00000000U
#define FX1_ISP_CH2_ODMA_BUF_EOF_TS1_TIMESTAMP_ACCESS FX1_ISP_ACCESS_RO

/**
 * CH2_ODMA_BUF_EOF_TS2: địa chỉ lệch 0x3538, dòng bảng 764, kiểu truy cập RO
 * CH2, buffer 2: Mốc clock lõi chốt ở EOF đầu ra, công bố lúc hoàn tất; trừ SOF_TS/mốc chụp modulo
 * 2^32 để tính thời gian xử lý/độ trễ.
 */
#define FX1_ISP_CH2_ODMA_BUF_EOF_TS2_OFFSET 0x3538U
#define REG_CH2_ODMA_BUF_EOF_TS2 FX1_ISP_CH2_ODMA_BUF_EOF_TS2_OFFSET
#define FX1_ISP_CH2_ODMA_BUF_EOF_TS2_RESET 0x00000000U
#define FX1_ISP_CH2_ODMA_BUF_EOF_TS2_ACCESS FX1_ISP_ACCESS_RO
#define FX1_ISP_CH2_ODMA_BUF_EOF_TS2_RO_MASK 0xFFFFFFFFU
#define FX1_ISP_CH2_ODMA_BUF_EOF_TS2_RW_MASK 0x00000000U
#define FX1_ISP_CH2_ODMA_BUF_EOF_TS2_W1C_MASK 0x00000000U
#define FX1_ISP_CH2_ODMA_BUF_EOF_TS2_W1S_MASK 0x00000000U
#define FX1_ISP_CH2_ODMA_BUF_EOF_TS2_W1P_MASK 0x00000000U
#define FX1_ISP_CH2_ODMA_BUF_EOF_TS2_W1SC_MASK 0x00000000U
/**
 * timestamp [31:0], dòng bảng 765, kiểu truy cập RO
 * Mốc thời gian clock lõi của buffer 2, giá trị 32 bit hard reset 0, chốt ở EOF đầu ra và công bố
 * khi hoàn tất; trừ SOF_TS hoặc mốc chụp modulo 2^32 để tính thời gian xử lý/độ trễ đầu cuối.
 */
#define FX1_ISP_CH2_ODMA_BUF_EOF_TS2_TIMESTAMP_SHIFT 0U
#define FX1_ISP_CH2_ODMA_BUF_EOF_TS2_TIMESTAMP_WIDTH 32U
#define FX1_ISP_CH2_ODMA_BUF_EOF_TS2_TIMESTAMP_MASK 0xFFFFFFFFU
#define FX1_ISP_CH2_ODMA_BUF_EOF_TS2_TIMESTAMP_RESET 0x00000000U
#define FX1_ISP_CH2_ODMA_BUF_EOF_TS2_TIMESTAMP_ACCESS FX1_ISP_ACCESS_RO

/**
 * CH2_ODMA_BUF_EOF_TS3: địa chỉ lệch 0x353C, dòng bảng 766, kiểu truy cập RO
 * CH2, buffer 3: Mốc clock lõi chốt ở EOF đầu ra, công bố lúc hoàn tất; trừ SOF_TS/mốc chụp modulo
 * 2^32 để tính thời gian xử lý/độ trễ.
 */
#define FX1_ISP_CH2_ODMA_BUF_EOF_TS3_OFFSET 0x353CU
#define REG_CH2_ODMA_BUF_EOF_TS3 FX1_ISP_CH2_ODMA_BUF_EOF_TS3_OFFSET
#define FX1_ISP_CH2_ODMA_BUF_EOF_TS3_RESET 0x00000000U
#define FX1_ISP_CH2_ODMA_BUF_EOF_TS3_ACCESS FX1_ISP_ACCESS_RO
#define FX1_ISP_CH2_ODMA_BUF_EOF_TS3_RO_MASK 0xFFFFFFFFU
#define FX1_ISP_CH2_ODMA_BUF_EOF_TS3_RW_MASK 0x00000000U
#define FX1_ISP_CH2_ODMA_BUF_EOF_TS3_W1C_MASK 0x00000000U
#define FX1_ISP_CH2_ODMA_BUF_EOF_TS3_W1S_MASK 0x00000000U
#define FX1_ISP_CH2_ODMA_BUF_EOF_TS3_W1P_MASK 0x00000000U
#define FX1_ISP_CH2_ODMA_BUF_EOF_TS3_W1SC_MASK 0x00000000U
/**
 * timestamp [31:0], dòng bảng 767, kiểu truy cập RO
 * Mốc thời gian clock lõi của buffer 3, giá trị 32 bit hard reset 0, chốt ở EOF đầu ra và công bố
 * khi hoàn tất; trừ SOF_TS hoặc mốc chụp modulo 2^32 để tính thời gian xử lý/độ trễ đầu cuối.
 */
#define FX1_ISP_CH2_ODMA_BUF_EOF_TS3_TIMESTAMP_SHIFT 0U
#define FX1_ISP_CH2_ODMA_BUF_EOF_TS3_TIMESTAMP_WIDTH 32U
#define FX1_ISP_CH2_ODMA_BUF_EOF_TS3_TIMESTAMP_MASK 0xFFFFFFFFU
#define FX1_ISP_CH2_ODMA_BUF_EOF_TS3_TIMESTAMP_RESET 0x00000000U
#define FX1_ISP_CH2_ODMA_BUF_EOF_TS3_TIMESTAMP_ACCESS FX1_ISP_ACCESS_RO

/**
 * TUNE_ID: địa chỉ lệch 0x8000, dòng bảng 768, kiểu truy cập RO
 * Nhận diện phần mở rộng; địa chỉ chưa ánh xạ ở v1.04 đọc 0. Cập nhật trực tiếp, theo MAS v1.07 phần
 * tuning-profiles.
 */
#define FX1_ISP_TUNE_ID_OFFSET 0x8000U
#define REG_TUNE_ID FX1_ISP_TUNE_ID_OFFSET
#define FX1_ISP_TUNE_ID_RESET 0x54554E45U
#define FX1_ISP_TUNE_ID_ACCESS FX1_ISP_ACCESS_RO
#define FX1_ISP_TUNE_ID_RO_MASK 0xFFFFFFFFU
#define FX1_ISP_TUNE_ID_RW_MASK 0x00000000U
#define FX1_ISP_TUNE_ID_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_ID_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_ID_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_ID_W1SC_MASK 0x00000000U
/**
 * MAGIC [31:0], dòng bảng 769, kiểu truy cập RO
 * Mã ASCII TUNE, chỉ phần cứng có hiện thực phần mở rộng mới trả giá trị này.
 */
#define FX1_ISP_TUNE_ID_MAGIC_SHIFT 0U
#define FX1_ISP_TUNE_ID_MAGIC_WIDTH 32U
#define FX1_ISP_TUNE_ID_MAGIC_MASK 0xFFFFFFFFU
#define FX1_ISP_TUNE_ID_MAGIC_RESET 0x54554E45U
#define FX1_ISP_TUNE_ID_MAGIC_ACCESS FX1_ISP_ACCESS_RO

/**
 * TUNE_REV: địa chỉ lệch 0x8004, dòng bảng 770, kiểu truy cập RO
 * ABI phần mở rộng 1.07, phải khớp phiên bản pipeline.
 */
#define FX1_ISP_TUNE_REV_OFFSET 0x8004U
#define REG_TUNE_REV FX1_ISP_TUNE_REV_OFFSET
#define FX1_ISP_TUNE_REV_RESET 0x00010007U
#define FX1_ISP_TUNE_REV_ACCESS FX1_ISP_ACCESS_RO
#define FX1_ISP_TUNE_REV_RO_MASK 0xFFFFFFFFU
#define FX1_ISP_TUNE_REV_RW_MASK 0x00000000U
#define FX1_ISP_TUNE_REV_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_REV_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_REV_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_REV_W1SC_MASK 0x00000000U
/**
 * VALUE [31:0], dòng bảng 771, kiểu truy cập RO
 * ABI phần mở rộng 0x00010007.
 */
#define FX1_ISP_TUNE_REV_VALUE_SHIFT 0U
#define FX1_ISP_TUNE_REV_VALUE_WIDTH 32U
#define FX1_ISP_TUNE_REV_VALUE_MASK 0xFFFFFFFFU
#define FX1_ISP_TUNE_REV_VALUE_RESET 0x00010007U
#define FX1_ISP_TUNE_REV_VALUE_ACCESS FX1_ISP_ACCESS_RO

/**
 * TUNE_CAPS: địa chỉ lệch 0x8008, dòng bảng 772, kiểu truy cập RO
 * Khả năng phần cứng chỉ đọc. Tuân thủ đầy đủ v1.07 yêu cầu bit 0..21 đều 1 (0x003FFFFF); tính năng
 * không hỗ trợ đọc 0. Phần mềm kiểm tra trước dùng.
 */
#define FX1_ISP_TUNE_CAPS_OFFSET 0x8008U
#define REG_TUNE_CAPS FX1_ISP_TUNE_CAPS_OFFSET
#define FX1_ISP_TUNE_CAPS_RESET 0x003FFFFFU
#define FX1_ISP_TUNE_CAPS_ACCESS FX1_ISP_ACCESS_RO
#define FX1_ISP_TUNE_CAPS_RO_MASK 0x003FFFFFU
#define FX1_ISP_TUNE_CAPS_RW_MASK 0x00000000U
#define FX1_ISP_TUNE_CAPS_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_CAPS_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_CAPS_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_CAPS_W1SC_MASK 0x00000000U
/**
 * PROFILE [0:0], dòng bảng 773, kiểu truy cập RO
 * Phần cứng đã hiện thực tính năng PROFILE.
 */
#define FX1_ISP_TUNE_CAPS_PROFILE_SHIFT 0U
#define FX1_ISP_TUNE_CAPS_PROFILE_WIDTH 1U
#define FX1_ISP_TUNE_CAPS_PROFILE_MASK 0x00000001U
#define FX1_ISP_TUNE_CAPS_PROFILE_RESET 0x00000001U
#define FX1_ISP_TUNE_CAPS_PROFILE_ACCESS FX1_ISP_ACCESS_RO
/**
 * NR [1:1], dòng bảng 774, kiểu truy cập RO
 * Phần cứng đã hiện thực tính năng NR.
 */
#define FX1_ISP_TUNE_CAPS_NR_SHIFT 1U
#define FX1_ISP_TUNE_CAPS_NR_WIDTH 1U
#define FX1_ISP_TUNE_CAPS_NR_MASK 0x00000002U
#define FX1_ISP_TUNE_CAPS_NR_RESET 0x00000001U
#define FX1_ISP_TUNE_CAPS_NR_ACCESS FX1_ISP_ACCESS_RO
/**
 * AWB_COLOR [2:2], dòng bảng 775, kiểu truy cập RO
 * Phần cứng đã hiện thực tính năng AWB_COLOR.
 */
#define FX1_ISP_TUNE_CAPS_AWB_COLOR_SHIFT 2U
#define FX1_ISP_TUNE_CAPS_AWB_COLOR_WIDTH 1U
#define FX1_ISP_TUNE_CAPS_AWB_COLOR_MASK 0x00000004U
#define FX1_ISP_TUNE_CAPS_AWB_COLOR_RESET 0x00000001U
#define FX1_ISP_TUNE_CAPS_AWB_COLOR_ACCESS FX1_ISP_ACCESS_RO
/**
 * AF_WINDOWS [3:3], dòng bảng 776, kiểu truy cập RO
 * Phần cứng đã hiện thực tính năng AF_WINDOWS.
 */
#define FX1_ISP_TUNE_CAPS_AF_WINDOWS_SHIFT 3U
#define FX1_ISP_TUNE_CAPS_AF_WINDOWS_WIDTH 1U
#define FX1_ISP_TUNE_CAPS_AF_WINDOWS_MASK 0x00000008U
#define FX1_ISP_TUNE_CAPS_AF_WINDOWS_RESET 0x00000001U
#define FX1_ISP_TUNE_CAPS_AF_WINDOWS_ACCESS FX1_ISP_ACCESS_RO
/**
 * HIST_WEIGHT [4:4], dòng bảng 777, kiểu truy cập RO
 * Phần cứng đã hiện thực tính năng HIST_WEIGHT.
 */
#define FX1_ISP_TUNE_CAPS_HIST_WEIGHT_SHIFT 4U
#define FX1_ISP_TUNE_CAPS_HIST_WEIGHT_WIDTH 1U
#define FX1_ISP_TUNE_CAPS_HIST_WEIGHT_MASK 0x00000010U
#define FX1_ISP_TUNE_CAPS_HIST_WEIGHT_RESET 0x00000001U
#define FX1_ISP_TUNE_CAPS_HIST_WEIGHT_ACCESS FX1_ISP_ACCESS_RO
/**
 * WB_GB [5:5], dòng bảng 778, kiểu truy cập RO
 * Phần cứng đã hiện thực tính năng WB_GB.
 */
#define FX1_ISP_TUNE_CAPS_WB_GB_SHIFT 5U
#define FX1_ISP_TUNE_CAPS_WB_GB_WIDTH 1U
#define FX1_ISP_TUNE_CAPS_WB_GB_MASK 0x00000020U
#define FX1_ISP_TUNE_CAPS_WB_GB_RESET 0x00000001U
#define FX1_ISP_TUNE_CAPS_WB_GB_ACCESS FX1_ISP_ACCESS_RO
/**
 * DEMOSAIC [6:6], dòng bảng 779, kiểu truy cập RO
 * Phần cứng đã hiện thực tính năng DEMOSAIC.
 */
#define FX1_ISP_TUNE_CAPS_DEMOSAIC_SHIFT 6U
#define FX1_ISP_TUNE_CAPS_DEMOSAIC_WIDTH 1U
#define FX1_ISP_TUNE_CAPS_DEMOSAIC_MASK 0x00000040U
#define FX1_ISP_TUNE_CAPS_DEMOSAIC_RESET 0x00000001U
#define FX1_ISP_TUNE_CAPS_DEMOSAIC_ACCESS FX1_ISP_ACCESS_RO
/**
 * CNF_BLEND [7:7], dòng bảng 780, kiểu truy cập RO
 * Phần cứng đã hiện thực tính năng CNF_BLEND.
 */
#define FX1_ISP_TUNE_CAPS_CNF_BLEND_SHIFT 7U
#define FX1_ISP_TUNE_CAPS_CNF_BLEND_WIDTH 1U
#define FX1_ISP_TUNE_CAPS_CNF_BLEND_MASK 0x00000080U
#define FX1_ISP_TUNE_CAPS_CNF_BLEND_RESET 0x00000001U
#define FX1_ISP_TUNE_CAPS_CNF_BLEND_ACCESS FX1_ISP_ACCESS_RO
/**
 * CSC_LOOK [8:8], dòng bảng 781, kiểu truy cập RO
 * Phần cứng đã hiện thực tính năng CSC_LOOK.
 */
#define FX1_ISP_TUNE_CAPS_CSC_LOOK_SHIFT 8U
#define FX1_ISP_TUNE_CAPS_CSC_LOOK_WIDTH 1U
#define FX1_ISP_TUNE_CAPS_CSC_LOOK_MASK 0x00000100U
#define FX1_ISP_TUNE_CAPS_CSC_LOOK_RESET 0x00000001U
#define FX1_ISP_TUNE_CAPS_CSC_LOOK_ACCESS FX1_ISP_ACCESS_RO
/**
 * AEC_LEASE [9:9], dòng bảng 782, kiểu truy cập RO
 * Phần cứng đã hiện thực tính năng AEC_LEASE.
 */
#define FX1_ISP_TUNE_CAPS_AEC_LEASE_SHIFT 9U
#define FX1_ISP_TUNE_CAPS_AEC_LEASE_WIDTH 1U
#define FX1_ISP_TUNE_CAPS_AEC_LEASE_MASK 0x00000200U
#define FX1_ISP_TUNE_CAPS_AEC_LEASE_RESET 0x00000001U
#define FX1_ISP_TUNE_CAPS_AEC_LEASE_ACCESS FX1_ISP_ACCESS_RO
/**
 * GAMMA_BANK [10:10], dòng bảng 783, kiểu truy cập RO
 * Phần cứng đã hiện thực tính năng GAMMA_BANK.
 */
#define FX1_ISP_TUNE_CAPS_GAMMA_BANK_SHIFT 10U
#define FX1_ISP_TUNE_CAPS_GAMMA_BANK_WIDTH 1U
#define FX1_ISP_TUNE_CAPS_GAMMA_BANK_MASK 0x00000400U
#define FX1_ISP_TUNE_CAPS_GAMMA_BANK_RESET 0x00000001U
#define FX1_ISP_TUNE_CAPS_GAMMA_BANK_ACCESS FX1_ISP_ACCESS_RO
/**
 * BPC_CAL [11:11], dòng bảng 784, kiểu truy cập RO
 * Phần cứng đã hiện thực tính năng BPC_CAL.
 */
#define FX1_ISP_TUNE_CAPS_BPC_CAL_SHIFT 11U
#define FX1_ISP_TUNE_CAPS_BPC_CAL_WIDTH 1U
#define FX1_ISP_TUNE_CAPS_BPC_CAL_MASK 0x00000800U
#define FX1_ISP_TUNE_CAPS_BPC_CAL_RESET 0x00000001U
#define FX1_ISP_TUNE_CAPS_BPC_CAL_ACCESS FX1_ISP_ACCESS_RO
/**
 * LSC_SECTORS [12:12], dòng bảng 785, kiểu truy cập RO
 * Phần cứng đã hiện thực tính năng LSC_SECTORS.
 */
#define FX1_ISP_TUNE_CAPS_LSC_SECTORS_SHIFT 12U
#define FX1_ISP_TUNE_CAPS_LSC_SECTORS_WIDTH 1U
#define FX1_ISP_TUNE_CAPS_LSC_SECTORS_MASK 0x00001000U
#define FX1_ISP_TUNE_CAPS_LSC_SECTORS_RESET 0x00000001U
#define FX1_ISP_TUNE_CAPS_LSC_SECTORS_ACCESS FX1_ISP_ACCESS_RO
/**
 * AF_LEASE [13:13], dòng bảng 786, kiểu truy cập RO
 * Phần cứng đã hiện thực tính năng AF_LEASE.
 */
#define FX1_ISP_TUNE_CAPS_AF_LEASE_SHIFT 13U
#define FX1_ISP_TUNE_CAPS_AF_LEASE_WIDTH 1U
#define FX1_ISP_TUNE_CAPS_AF_LEASE_MASK 0x00002000U
#define FX1_ISP_TUNE_CAPS_AF_LEASE_RESET 0x00000001U
#define FX1_ISP_TUNE_CAPS_AF_LEASE_ACCESS FX1_ISP_ACCESS_RO
/**
 * RAW_INPUT [14:14], dòng bảng 787, kiểu truy cập RO
 * Chuẩn hóa rõ ràng đầu vào RAW10/RAW12.
 */
#define FX1_ISP_TUNE_CAPS_RAW_INPUT_SHIFT 14U
#define FX1_ISP_TUNE_CAPS_RAW_INPUT_WIDTH 1U
#define FX1_ISP_TUNE_CAPS_RAW_INPUT_MASK 0x00004000U
#define FX1_ISP_TUNE_CAPS_RAW_INPUT_RESET 0x00000001U
#define FX1_ISP_TUNE_CAPS_RAW_INPUT_ACCESS FX1_ISP_ACCESS_RO
/**
 * BAYER_AE [15:15], dòng bảng 788, kiểu truy cập RO
 * Đo AE RAW12 tại BPC/DG, thông tin nguồn/miền đo gắn với bank đang giữ quyền.
 */
#define FX1_ISP_TUNE_CAPS_BAYER_AE_SHIFT 15U
#define FX1_ISP_TUNE_CAPS_BAYER_AE_WIDTH 1U
#define FX1_ISP_TUNE_CAPS_BAYER_AE_MASK 0x00008000U
#define FX1_ISP_TUNE_CAPS_BAYER_AE_RESET 0x00000001U
#define FX1_ISP_TUNE_CAPS_BAYER_AE_ACCESS FX1_ISP_ACCESS_RO
/**
 * CHANNEL_CROP [16:16], dòng bảng 789, kiểu truy cập RO
 * Crop độc lập và đọc lại hình học đã kiểm tra.
 */
#define FX1_ISP_TUNE_CAPS_CHANNEL_CROP_SHIFT 16U
#define FX1_ISP_TUNE_CAPS_CHANNEL_CROP_WIDTH 1U
#define FX1_ISP_TUNE_CAPS_CHANNEL_CROP_MASK 0x00010000U
#define FX1_ISP_TUNE_CAPS_CHANNEL_CROP_RESET 0x00000001U
#define FX1_ISP_TUNE_CAPS_CHANNEL_CROP_ACCESS FX1_ISP_ACCESS_RO
/**
 * CSC_WIDE_RANGE [17:17], dòng bảng 790, kiểu truy cập RO
 * Chọn dải trước một lần lượng tử hóa CSC xuống YUV8.
 */
#define FX1_ISP_TUNE_CAPS_CSC_WIDE_RANGE_SHIFT 17U
#define FX1_ISP_TUNE_CAPS_CSC_WIDE_RANGE_WIDTH 1U
#define FX1_ISP_TUNE_CAPS_CSC_WIDE_RANGE_MASK 0x00020000U
#define FX1_ISP_TUNE_CAPS_CSC_WIDE_RANGE_RESET 0x00000001U
#define FX1_ISP_TUNE_CAPS_CSC_WIDE_RANGE_ACCESS FX1_ISP_ACCESS_RO
/**
 * RECOVERY_REPORT [18:18], dòng bảng 791, kiểu truy cập RO
 * Recovery có trạng thái quan sát được và bốn bản ghi hủy.
 */
#define FX1_ISP_TUNE_CAPS_RECOVERY_REPORT_SHIFT 18U
#define FX1_ISP_TUNE_CAPS_RECOVERY_REPORT_WIDTH 1U
#define FX1_ISP_TUNE_CAPS_RECOVERY_REPORT_MASK 0x00040000U
#define FX1_ISP_TUNE_CAPS_RECOVERY_REPORT_RESET 0x00000001U
#define FX1_ISP_TUNE_CAPS_RECOVERY_REPORT_ACCESS FX1_ISP_ACCESS_RO
/**
 * FRAME_ID [19:19], dòng bảng 792, kiểu truy cập RO
 * Nhận diện frame đầu vào trên mọi buffer đầu ra thành công.
 */
#define FX1_ISP_TUNE_CAPS_FRAME_ID_SHIFT 19U
#define FX1_ISP_TUNE_CAPS_FRAME_ID_WIDTH 1U
#define FX1_ISP_TUNE_CAPS_FRAME_ID_MASK 0x00080000U
#define FX1_ISP_TUNE_CAPS_FRAME_ID_RESET 0x00000001U
#define FX1_ISP_TUNE_CAPS_FRAME_ID_ACCESS FX1_ISP_ACCESS_RO
/**
 * LEASE_GEOMETRY [20:20], dòng bảng 793, kiểu truy cập RO
 * Hình học thống kê được chốt theo kết quả.
 */
#define FX1_ISP_TUNE_CAPS_LEASE_GEOMETRY_SHIFT 20U
#define FX1_ISP_TUNE_CAPS_LEASE_GEOMETRY_WIDTH 1U
#define FX1_ISP_TUNE_CAPS_LEASE_GEOMETRY_MASK 0x00100000U
#define FX1_ISP_TUNE_CAPS_LEASE_GEOMETRY_RESET 0x00000001U
#define FX1_ISP_TUNE_CAPS_LEASE_GEOMETRY_ACCESS FX1_ISP_ACCESS_RO
/**
 * RESIDENT_OWNERSHIP [21:21], dòng bảng 794, kiểu truy cập RO
 * Đọc lại EE theo bộ cư trú được chọn và bảo vệ bộ đã SEAL.
 */
#define FX1_ISP_TUNE_CAPS_RESIDENT_OWNERSHIP_SHIFT 21U
#define FX1_ISP_TUNE_CAPS_RESIDENT_OWNERSHIP_WIDTH 1U
#define FX1_ISP_TUNE_CAPS_RESIDENT_OWNERSHIP_MASK 0x00200000U
#define FX1_ISP_TUNE_CAPS_RESIDENT_OWNERSHIP_RESET 0x00000001U
#define FX1_ISP_TUNE_CAPS_RESIDENT_OWNERSHIP_ACCESS FX1_ISP_ACCESS_RO

/**
 * TUNE_MODE: địa chỉ lệch 0x800C, dòng bảng 795, kiểu truy cập RW
 * Chọn chế độ profile khi đã dừng/xử lý hết. Legacy giữ quy tắc cập nhật cục bộ nhưng dùng miền số
 * v1.06/v1.07.
 */
#define FX1_ISP_TUNE_MODE_OFFSET 0x800CU
#define REG_TUNE_MODE FX1_ISP_TUNE_MODE_OFFSET
#define FX1_ISP_TUNE_MODE_RESET 0x00000000U
#define FX1_ISP_TUNE_MODE_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_TUNE_MODE_RO_MASK 0x00000000U
#define FX1_ISP_TUNE_MODE_RW_MASK 0x00000001U
#define FX1_ISP_TUNE_MODE_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_MODE_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_MODE_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_MODE_W1SC_MASK 0x00000000U
/**
 * ENABLE [0:0], dòng bảng 796, kiểu truy cập RW
 * 0=legacy,1=phần mở rộng. Chỉ đổi sau khi toàn pipeline xử lý hết và không giữ bank thống kê; sai
 * bị MODE. Thoát profile xóa seal hai bank, tham chiếu cư trú, trạng thái active/pending và dữ liệu
 * sẵn sàng cục bộ bank.
 */
#define FX1_ISP_TUNE_MODE_ENABLE_SHIFT 0U
#define FX1_ISP_TUNE_MODE_ENABLE_WIDTH 1U
#define FX1_ISP_TUNE_MODE_ENABLE_MASK 0x00000001U
#define FX1_ISP_TUNE_MODE_ENABLE_RESET 0x00000000U
#define FX1_ISP_TUNE_MODE_ENABLE_ACCESS FX1_ISP_ACCESS_RW

/**
 * TUNE_BANK: địa chỉ lệch 0x8010, dòng bảng 797, kiểu truy cập RW
 * Selector bank để lập trình/đọc trực tiếp. Chọn bank đã SEAL/active/pending/đang tham chiếu vẫn hợp
 * lệ; chỉ thay đổi nội dung được bảo vệ mới bị LOCKED.
 */
#define FX1_ISP_TUNE_BANK_OFFSET 0x8010U
#define REG_TUNE_BANK FX1_ISP_TUNE_BANK_OFFSET
#define FX1_ISP_TUNE_BANK_RESET 0x00000000U
#define FX1_ISP_TUNE_BANK_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_TUNE_BANK_RO_MASK 0x00000000U
#define FX1_ISP_TUNE_BANK_RW_MASK 0x00000001U
#define FX1_ISP_TUNE_BANK_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_BANK_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_BANK_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_BANK_W1SC_MASK 0x00000000U
/**
 * VALUE [0:0], dòng bảng 798, kiểu truy cập RW
 * Selector bank để lập trình/đọc trực tiếp. Chọn bank đã SEAL/active/pending/đang tham chiếu vẫn hợp
 * lệ; chỉ thay đổi nội dung được bảo vệ mới bị LOCKED.
 */
#define FX1_ISP_TUNE_BANK_VALUE_SHIFT 0U
#define FX1_ISP_TUNE_BANK_VALUE_WIDTH 1U
#define FX1_ISP_TUNE_BANK_VALUE_MASK 0x00000001U
#define FX1_ISP_TUNE_BANK_VALUE_RESET 0x00000000U
#define FX1_ISP_TUNE_BANK_VALUE_ACCESS FX1_ISP_ACCESS_RW

/**
 * TUNE_PROFILE_ID: địa chỉ lệch 0x8014, dòng bảng 799, kiểu truy cập RW
 * ID profile phần mềm lưu trong bank đang chọn, cho phép 0. Cập nhật theo profile; MAS v1.07
 * tuning-profiles.
 */
#define FX1_ISP_TUNE_PROFILE_ID_OFFSET 0x8014U
#define REG_TUNE_PROFILE_ID FX1_ISP_TUNE_PROFILE_ID_OFFSET
#define FX1_ISP_TUNE_PROFILE_ID_RESET 0x00000000U
#define FX1_ISP_TUNE_PROFILE_ID_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_TUNE_PROFILE_ID_RO_MASK 0x00000000U
#define FX1_ISP_TUNE_PROFILE_ID_RW_MASK 0xFFFFFFFFU
#define FX1_ISP_TUNE_PROFILE_ID_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_PROFILE_ID_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_PROFILE_ID_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_PROFILE_ID_W1SC_MASK 0x00000000U
/**
 * VALUE [31:0], dòng bảng 800, kiểu truy cập RW
 * ID profile phần mềm lưu trong bank đang chọn, cho phép giá trị 0.
 */
#define FX1_ISP_TUNE_PROFILE_ID_VALUE_SHIFT 0U
#define FX1_ISP_TUNE_PROFILE_ID_VALUE_WIDTH 32U
#define FX1_ISP_TUNE_PROFILE_ID_VALUE_MASK 0xFFFFFFFFU
#define FX1_ISP_TUNE_PROFILE_ID_VALUE_RESET 0x00000000U
#define FX1_ISP_TUNE_PROFILE_ID_VALUE_ACCESS FX1_ISP_ACCESS_RW

/**
 * TUNE_APPLY_FRAME: địa chỉ lệch 0x8018, dòng bảng 801, kiểu truy cập RW
 * Số thứ tự frame đầu vào đích chính xác khi EXACT=1; trường bị bỏ qua nếu không EXACT. Cập nhật
 * trực tiếp; MAS v1.07 tuning-profiles.
 */
#define FX1_ISP_TUNE_APPLY_FRAME_OFFSET 0x8018U
#define REG_TUNE_APPLY_FRAME FX1_ISP_TUNE_APPLY_FRAME_OFFSET
#define FX1_ISP_TUNE_APPLY_FRAME_RESET 0x00000000U
#define FX1_ISP_TUNE_APPLY_FRAME_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_TUNE_APPLY_FRAME_RO_MASK 0x00000000U
#define FX1_ISP_TUNE_APPLY_FRAME_RW_MASK 0xFFFFFFFFU
#define FX1_ISP_TUNE_APPLY_FRAME_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_APPLY_FRAME_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_APPLY_FRAME_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_APPLY_FRAME_W1SC_MASK 0x00000000U
/**
 * VALUE [31:0], dòng bảng 802, kiểu truy cập RW
 * Đích frame đầu vào chính xác khi EXACT=1, nếu không thì bỏ qua.
 */
#define FX1_ISP_TUNE_APPLY_FRAME_VALUE_SHIFT 0U
#define FX1_ISP_TUNE_APPLY_FRAME_VALUE_WIDTH 32U
#define FX1_ISP_TUNE_APPLY_FRAME_VALUE_MASK 0xFFFFFFFFU
#define FX1_ISP_TUNE_APPLY_FRAME_VALUE_RESET 0x00000000U
#define FX1_ISP_TUNE_APPLY_FRAME_VALUE_ACCESS FX1_ISP_ACCESS_RW

/**
 * TUNE_CMD: địa chỉ lệch 0x801C, dòng bảng 803, kiểu truy cập W1P
 * Mỗi lần ghi chỉ một lệnh, trừ EXACT đi cùng APPLY. Tổ hợp khác kể cả chỉ EXACT không chạy gì và
 * đặt INVALID; ở legacy mọi ghi khác 0 chỉ đặt MODE. SEAL kiểm tra/chốt khóa nhưng chưa kích hoạt;
 * UNSEAL chỉ sửa bank không active/pending/tham chiếu. APPLY xử lý hết đời cấu hình cũ rồi kích hoạt
 * tại SOF đủ điều kiện. Kiểm tra STATUS/ERROR sau mỗi lệnh.
 */
#define FX1_ISP_TUNE_CMD_OFFSET 0x801CU
#define REG_TUNE_CMD FX1_ISP_TUNE_CMD_OFFSET
#define FX1_ISP_TUNE_CMD_RESET 0x00000000U
#define FX1_ISP_TUNE_CMD_ACCESS FX1_ISP_ACCESS_W1P
#define FX1_ISP_TUNE_CMD_RO_MASK 0x00000000U
#define FX1_ISP_TUNE_CMD_RW_MASK 0x00000000U
#define FX1_ISP_TUNE_CMD_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_CMD_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_CMD_W1P_MASK 0x0000011FU
#define FX1_ISP_TUNE_CMD_W1SC_MASK 0x00000000U
/**
 * INIT [0:0], dòng bảng 804, kiểu truy cập W1P
 * INIT sao chép hệ số thuộc bank rỗi từ trạng thái legacy và mặc định phần mở rộng, làm mất trạng
 * thái sẵn sàng dữ liệu bank. Không sao hệ số EE cư trú, hình học LSC dùng chung hoặc cổng nạp trực
 * tiếp.
 */
#define FX1_ISP_TUNE_CMD_INIT_SHIFT 0U
#define FX1_ISP_TUNE_CMD_INIT_WIDTH 1U
#define FX1_ISP_TUNE_CMD_INIT_MASK 0x00000001U
#define FX1_ISP_TUNE_CMD_INIT_RESET 0x00000000U
#define FX1_ISP_TUNE_CMD_INIT_ACCESS FX1_ISP_ACCESS_W1P
/**
 * SEAL [1:1], dòng bảng 805, kiểu truy cập W1P
 * Kiểm tra và SEAL bank đang chọn, gồm các bảng cần thiết.
 */
#define FX1_ISP_TUNE_CMD_SEAL_SHIFT 1U
#define FX1_ISP_TUNE_CMD_SEAL_WIDTH 1U
#define FX1_ISP_TUNE_CMD_SEAL_MASK 0x00000002U
#define FX1_ISP_TUNE_CMD_SEAL_RESET 0x00000000U
#define FX1_ISP_TUNE_CMD_SEAL_ACCESS FX1_ISP_ACCESS_W1P
/**
 * APPLY [2:2], dòng bảng 806, kiểu truy cập W1P
 * APPLY chốt TUNE_BANK/ID/đích EXACT trước cạnh; yêu cầu pending sớm nhất có hiệu lực từ cạnh sau.
 * Profile đầu chỉ nhận NEXT hoặc EXACT=NEXT_FRAME; đích xa hơn INVALID. APPLY bị từ chối không đổi
 * trạng thái, đặt một lỗi đầu tiên theo thứ tự MODE,PENDING,NOT_READY,INVALID,MISSED.
 */
#define FX1_ISP_TUNE_CMD_APPLY_SHIFT 2U
#define FX1_ISP_TUNE_CMD_APPLY_WIDTH 1U
#define FX1_ISP_TUNE_CMD_APPLY_MASK 0x00000004U
#define FX1_ISP_TUNE_CMD_APPLY_RESET 0x00000000U
#define FX1_ISP_TUNE_CMD_APPLY_ACCESS FX1_ISP_ACCESS_W1P
/**
 * CANCEL [3:3], dòng bảng 807, kiểu truy cập W1P
 * Hủy yêu cầu pending trước khi đầu vào tiếp nhận. Chưa có pending thì không tác dụng, không lỗi.
 */
#define FX1_ISP_TUNE_CMD_CANCEL_SHIFT 3U
#define FX1_ISP_TUNE_CMD_CANCEL_WIDTH 1U
#define FX1_ISP_TUNE_CMD_CANCEL_MASK 0x00000008U
#define FX1_ISP_TUNE_CMD_CANCEL_RESET 0x00000000U
#define FX1_ISP_TUNE_CMD_CANCEL_ACCESS FX1_ISP_ACCESS_W1P
/**
 * UNSEAL [4:4], dòng bảng 808, kiểu truy cập W1P
 * Mở khóa bank đang chọn chỉ khi không active, không bị tham chiếu và không pending.
 */
#define FX1_ISP_TUNE_CMD_UNSEAL_SHIFT 4U
#define FX1_ISP_TUNE_CMD_UNSEAL_WIDTH 1U
#define FX1_ISP_TUNE_CMD_UNSEAL_MASK 0x00000010U
#define FX1_ISP_TUNE_CMD_UNSEAL_RESET 0x00000000U
#define FX1_ISP_TUNE_CMD_UNSEAL_ACCESS FX1_ISP_ACCESS_W1P
/**
 * EXACT [8:8], dòng bảng 809, kiểu truy cập W1P
 * EXACT bổ nghĩa APPLY: yêu cầu bằng TUNE_APPLY_FRAME; nếu không dùng SOF được nhận kế tiếp.
 */
#define FX1_ISP_TUNE_CMD_EXACT_SHIFT 8U
#define FX1_ISP_TUNE_CMD_EXACT_WIDTH 1U
#define FX1_ISP_TUNE_CMD_EXACT_MASK 0x00000100U
#define FX1_ISP_TUNE_CMD_EXACT_RESET 0x00000000U
#define FX1_ISP_TUNE_CMD_EXACT_ACCESS FX1_ISP_ACCESS_W1P

/**
 * TUNE_STATUS: địa chỉ lệch 0x8020, dòng bảng 810, kiểu truy cập RO
 * Trạng thái điều khiển trực tiếp nhất quán; MAS v1.07 tuning-profiles.
 */
#define FX1_ISP_TUNE_STATUS_OFFSET 0x8020U
#define REG_TUNE_STATUS FX1_ISP_TUNE_STATUS_OFFSET
#define FX1_ISP_TUNE_STATUS_RESET 0x00000000U
#define FX1_ISP_TUNE_STATUS_ACCESS FX1_ISP_ACCESS_RO
#define FX1_ISP_TUNE_STATUS_RO_MASK 0x000007FFU
#define FX1_ISP_TUNE_STATUS_RW_MASK 0x00000000U
#define FX1_ISP_TUNE_STATUS_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_STATUS_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_STATUS_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_STATUS_W1SC_MASK 0x00000000U
/**
 * ACTIVE_BANK [0:0], dòng bảng 811, kiểu truy cập RO
 * Bank profile hiện tại ở đầu vào.
 */
#define FX1_ISP_TUNE_STATUS_ACTIVE_BANK_SHIFT 0U
#define FX1_ISP_TUNE_STATUS_ACTIVE_BANK_WIDTH 1U
#define FX1_ISP_TUNE_STATUS_ACTIVE_BANK_MASK 0x00000001U
#define FX1_ISP_TUNE_STATUS_ACTIVE_BANK_RESET 0x00000000U
#define FX1_ISP_TUNE_STATUS_ACTIVE_BANK_ACCESS FX1_ISP_ACCESS_RO
/**
 * ACTIVE_VALID [1:1], dòng bảng 812, kiểu truy cập RO
 * Đã kích hoạt profile đầu tiên.
 */
#define FX1_ISP_TUNE_STATUS_ACTIVE_VALID_SHIFT 1U
#define FX1_ISP_TUNE_STATUS_ACTIVE_VALID_WIDTH 1U
#define FX1_ISP_TUNE_STATUS_ACTIVE_VALID_MASK 0x00000002U
#define FX1_ISP_TUNE_STATUS_ACTIVE_VALID_RESET 0x00000000U
#define FX1_ISP_TUNE_STATUS_ACTIVE_VALID_ACCESS FX1_ISP_ACCESS_RO
/**
 * PENDING [2:2], dòng bảng 813, kiểu truy cập RO
 * Đang có yêu cầu APPLY pending.
 */
#define FX1_ISP_TUNE_STATUS_PENDING_SHIFT 2U
#define FX1_ISP_TUNE_STATUS_PENDING_WIDTH 1U
#define FX1_ISP_TUNE_STATUS_PENDING_MASK 0x00000004U
#define FX1_ISP_TUNE_STATUS_PENDING_RESET 0x00000000U
#define FX1_ISP_TUNE_STATUS_PENDING_ACCESS FX1_ISP_ACCESS_RO
/**
 * BANK0_BUSY [3:3], dòng bảng 814, kiểu truy cập RO
 * Bank 0 đang active/tham chiếu/SEAL/pending.
 */
#define FX1_ISP_TUNE_STATUS_BANK0_BUSY_SHIFT 3U
#define FX1_ISP_TUNE_STATUS_BANK0_BUSY_WIDTH 1U
#define FX1_ISP_TUNE_STATUS_BANK0_BUSY_MASK 0x00000008U
#define FX1_ISP_TUNE_STATUS_BANK0_BUSY_RESET 0x00000000U
#define FX1_ISP_TUNE_STATUS_BANK0_BUSY_ACCESS FX1_ISP_ACCESS_RO
/**
 * BANK1_BUSY [4:4], dòng bảng 815, kiểu truy cập RO
 * Bank 1 đang active/tham chiếu/SEAL/pending.
 */
#define FX1_ISP_TUNE_STATUS_BANK1_BUSY_SHIFT 4U
#define FX1_ISP_TUNE_STATUS_BANK1_BUSY_WIDTH 1U
#define FX1_ISP_TUNE_STATUS_BANK1_BUSY_MASK 0x00000010U
#define FX1_ISP_TUNE_STATUS_BANK1_BUSY_RESET 0x00000000U
#define FX1_ISP_TUNE_STATUS_BANK1_BUSY_ACCESS FX1_ISP_ACCESS_RO
/**
 * BANK0_VALID [5:5], dòng bảng 816, kiểu truy cập RO
 * Bank 0 đã SEAL.
 */
#define FX1_ISP_TUNE_STATUS_BANK0_VALID_SHIFT 5U
#define FX1_ISP_TUNE_STATUS_BANK0_VALID_WIDTH 1U
#define FX1_ISP_TUNE_STATUS_BANK0_VALID_MASK 0x00000020U
#define FX1_ISP_TUNE_STATUS_BANK0_VALID_RESET 0x00000000U
#define FX1_ISP_TUNE_STATUS_BANK0_VALID_ACCESS FX1_ISP_ACCESS_RO
/**
 * BANK1_VALID [6:6], dòng bảng 817, kiểu truy cập RO
 * Bank 1 đã SEAL.
 */
#define FX1_ISP_TUNE_STATUS_BANK1_VALID_SHIFT 6U
#define FX1_ISP_TUNE_STATUS_BANK1_VALID_WIDTH 1U
#define FX1_ISP_TUNE_STATUS_BANK1_VALID_MASK 0x00000040U
#define FX1_ISP_TUNE_STATUS_BANK1_VALID_RESET 0x00000000U
#define FX1_ISP_TUNE_STATUS_BANK1_VALID_ACCESS FX1_ISP_ACCESS_RO
/**
 * BANK0_PENDING [7:7], dòng bảng 818, kiểu truy cập RO
 * Bank 0 là đích APPLY đang chờ.
 */
#define FX1_ISP_TUNE_STATUS_BANK0_PENDING_SHIFT 7U
#define FX1_ISP_TUNE_STATUS_BANK0_PENDING_WIDTH 1U
#define FX1_ISP_TUNE_STATUS_BANK0_PENDING_MASK 0x00000080U
#define FX1_ISP_TUNE_STATUS_BANK0_PENDING_RESET 0x00000000U
#define FX1_ISP_TUNE_STATUS_BANK0_PENDING_ACCESS FX1_ISP_ACCESS_RO
/**
 * BANK1_PENDING [8:8], dòng bảng 819, kiểu truy cập RO
 * Bank 1 là đích APPLY đang chờ.
 */
#define FX1_ISP_TUNE_STATUS_BANK1_PENDING_SHIFT 8U
#define FX1_ISP_TUNE_STATUS_BANK1_PENDING_WIDTH 1U
#define FX1_ISP_TUNE_STATUS_BANK1_PENDING_MASK 0x00000100U
#define FX1_ISP_TUNE_STATUS_BANK1_PENDING_RESET 0x00000000U
#define FX1_ISP_TUNE_STATUS_BANK1_PENDING_ACCESS FX1_ISP_ACCESS_RO
/**
 * BANK0_REFERENCED [9:9], dòng bảng 820, kiểu truy cập RO
 * Còn frame chưa kết thúc tham chiếu bank 0.
 */
#define FX1_ISP_TUNE_STATUS_BANK0_REFERENCED_SHIFT 9U
#define FX1_ISP_TUNE_STATUS_BANK0_REFERENCED_WIDTH 1U
#define FX1_ISP_TUNE_STATUS_BANK0_REFERENCED_MASK 0x00000200U
#define FX1_ISP_TUNE_STATUS_BANK0_REFERENCED_RESET 0x00000000U
#define FX1_ISP_TUNE_STATUS_BANK0_REFERENCED_ACCESS FX1_ISP_ACCESS_RO
/**
 * BANK1_REFERENCED [10:10], dòng bảng 821, kiểu truy cập RO
 * Còn frame chưa kết thúc tham chiếu bank 1.
 */
#define FX1_ISP_TUNE_STATUS_BANK1_REFERENCED_SHIFT 10U
#define FX1_ISP_TUNE_STATUS_BANK1_REFERENCED_WIDTH 1U
#define FX1_ISP_TUNE_STATUS_BANK1_REFERENCED_MASK 0x00000400U
#define FX1_ISP_TUNE_STATUS_BANK1_REFERENCED_RESET 0x00000000U
#define FX1_ISP_TUNE_STATUS_BANK1_REFERENCED_ACCESS FX1_ISP_ACCESS_RO

/**
 * TUNE_ACTIVE_ID: địa chỉ lệch 0x8024, dòng bảng 822, kiểu truy cập RO
 * Profile frame đầu vào được nhận gần nhất, không thay cho thông tin từng frame. Cập nhật trực tiếp;
 * MAS v1.07 tuning-profiles.
 */
#define FX1_ISP_TUNE_ACTIVE_ID_OFFSET 0x8024U
#define REG_TUNE_ACTIVE_ID FX1_ISP_TUNE_ACTIVE_ID_OFFSET
#define FX1_ISP_TUNE_ACTIVE_ID_RESET 0x00000000U
#define FX1_ISP_TUNE_ACTIVE_ID_ACCESS FX1_ISP_ACCESS_RO
#define FX1_ISP_TUNE_ACTIVE_ID_RO_MASK 0xFFFFFFFFU
#define FX1_ISP_TUNE_ACTIVE_ID_RW_MASK 0x00000000U
#define FX1_ISP_TUNE_ACTIVE_ID_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_ACTIVE_ID_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_ACTIVE_ID_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_ACTIVE_ID_W1SC_MASK 0x00000000U
/**
 * VALUE [31:0], dòng bảng 823, kiểu truy cập RO
 * Profile của frame đầu vào được nhận gần nhất, không thay thế thông tin riêng từng frame.
 */
#define FX1_ISP_TUNE_ACTIVE_ID_VALUE_SHIFT 0U
#define FX1_ISP_TUNE_ACTIVE_ID_VALUE_WIDTH 32U
#define FX1_ISP_TUNE_ACTIVE_ID_VALUE_MASK 0xFFFFFFFFU
#define FX1_ISP_TUNE_ACTIVE_ID_VALUE_RESET 0x00000000U
#define FX1_ISP_TUNE_ACTIVE_ID_VALUE_ACCESS FX1_ISP_ACCESS_RO

/**
 * TUNE_ACTIVE_FRAME: địa chỉ lệch 0x8028, dòng bảng 824, kiểu truy cập RO
 * Số frame tại đó profile hiện tại bắt đầu active; cập nhật trực tiếp, MAS v1.07 tuning-profiles.
 */
#define FX1_ISP_TUNE_ACTIVE_FRAME_OFFSET 0x8028U
#define REG_TUNE_ACTIVE_FRAME FX1_ISP_TUNE_ACTIVE_FRAME_OFFSET
#define FX1_ISP_TUNE_ACTIVE_FRAME_RESET 0x00000000U
#define FX1_ISP_TUNE_ACTIVE_FRAME_ACCESS FX1_ISP_ACCESS_RO
#define FX1_ISP_TUNE_ACTIVE_FRAME_RO_MASK 0xFFFFFFFFU
#define FX1_ISP_TUNE_ACTIVE_FRAME_RW_MASK 0x00000000U
#define FX1_ISP_TUNE_ACTIVE_FRAME_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_ACTIVE_FRAME_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_ACTIVE_FRAME_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_ACTIVE_FRAME_W1SC_MASK 0x00000000U
/**
 * VALUE [31:0], dòng bảng 825, kiểu truy cập RO
 * Số thứ tự frame mà profile hiện tại bắt đầu active.
 */
#define FX1_ISP_TUNE_ACTIVE_FRAME_VALUE_SHIFT 0U
#define FX1_ISP_TUNE_ACTIVE_FRAME_VALUE_WIDTH 32U
#define FX1_ISP_TUNE_ACTIVE_FRAME_VALUE_MASK 0xFFFFFFFFU
#define FX1_ISP_TUNE_ACTIVE_FRAME_VALUE_RESET 0x00000000U
#define FX1_ISP_TUNE_ACTIVE_FRAME_VALUE_ACCESS FX1_ISP_ACCESS_RO

/**
 * TUNE_ERROR: địa chỉ lệch 0x802C, dòng bảng 826, kiểu truy cập W1C
 * Lỗi chốt W1C; đặt mới của phần cứng thắng xóa cùng cạnh. Cập nhật trực tiếp; MAS v1.07
 * tuning-profiles.
 */
#define FX1_ISP_TUNE_ERROR_OFFSET 0x802CU
#define REG_TUNE_ERROR FX1_ISP_TUNE_ERROR_OFFSET
#define FX1_ISP_TUNE_ERROR_RESET 0x00000000U
#define FX1_ISP_TUNE_ERROR_ACCESS FX1_ISP_ACCESS_W1C
#define FX1_ISP_TUNE_ERROR_RO_MASK 0x00000000U
#define FX1_ISP_TUNE_ERROR_RW_MASK 0x00000000U
#define FX1_ISP_TUNE_ERROR_W1C_MASK 0x000000FFU
#define FX1_ISP_TUNE_ERROR_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_ERROR_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_ERROR_W1SC_MASK 0x00000000U
/**
 * LOCKED [0:0], dòng bảng 827, kiểu truy cập W1C
 * Ghi/nạp/xác nhận nhắm vào dữ liệu đang sở hữu, đã SEAL, pending hoặc đang tham chiếu.
 */
#define FX1_ISP_TUNE_ERROR_LOCKED_SHIFT 0U
#define FX1_ISP_TUNE_ERROR_LOCKED_WIDTH 1U
#define FX1_ISP_TUNE_ERROR_LOCKED_MASK 0x00000001U
#define FX1_ISP_TUNE_ERROR_LOCKED_RESET 0x00000000U
#define FX1_ISP_TUNE_ERROR_LOCKED_ACCESS FX1_ISP_ACCESS_W1C
/**
 * INVALID [1:1], dòng bảng 828, kiểu truy cập W1C
 * Trường/hình học/chỉ số bảng/lệnh không hợp lệ; gồm vi phạm SEAL, ghi nhiều lệnh hoặc chỉ EXACT,
 * đích EXACT cách đúng 2^31 và APPLY bank đã active.
 */
#define FX1_ISP_TUNE_ERROR_INVALID_SHIFT 1U
#define FX1_ISP_TUNE_ERROR_INVALID_WIDTH 1U
#define FX1_ISP_TUNE_ERROR_INVALID_MASK 0x00000002U
#define FX1_ISP_TUNE_ERROR_INVALID_RESET 0x00000000U
#define FX1_ISP_TUNE_ERROR_INVALID_ACCESS FX1_ISP_ACCESS_W1C
/**
 * NOT_READY [2:2], dòng bảng 829, kiểu truy cập W1C
 * APPLY bank chưa SEAL, SEAL thiếu bảng đã xác nhận hoặc xác nhận bảng thất bại.
 */
#define FX1_ISP_TUNE_ERROR_NOT_READY_SHIFT 2U
#define FX1_ISP_TUNE_ERROR_NOT_READY_WIDTH 1U
#define FX1_ISP_TUNE_ERROR_NOT_READY_MASK 0x00000004U
#define FX1_ISP_TUNE_ERROR_NOT_READY_RESET 0x00000000U
#define FX1_ISP_TUNE_ERROR_NOT_READY_ACCESS FX1_ISP_ACCESS_W1C
/**
 * PENDING [3:3], dòng bảng 830, kiểu truy cập W1C
 * APPLY thứ hai khi đang pending, hoặc yêu cầu recovery phần mềm khi BUSY. Giao dịch cũ tiếp tục.
 */
#define FX1_ISP_TUNE_ERROR_PENDING_SHIFT 3U
#define FX1_ISP_TUNE_ERROR_PENDING_WIDTH 1U
#define FX1_ISP_TUNE_ERROR_PENDING_MASK 0x00000008U
#define FX1_ISP_TUNE_ERROR_PENDING_RESET 0x00000000U
#define FX1_ISP_TUNE_ERROR_PENDING_ACCESS FX1_ISP_ACCESS_W1C
/**
 * MISSED [4:4], dòng bảng 831, kiểu truy cập W1C
 * Đích EXACT đã qua (d>2^31) khi nhận APPLY có profile active; chưa có active thì đặt INVALID thay
 * thế. CANCEL và recovery DMA không đặt MISSED.
 */
#define FX1_ISP_TUNE_ERROR_MISSED_SHIFT 4U
#define FX1_ISP_TUNE_ERROR_MISSED_WIDTH 1U
#define FX1_ISP_TUNE_ERROR_MISSED_MASK 0x00000010U
#define FX1_ISP_TUNE_ERROR_MISSED_RESET 0x00000000U
#define FX1_ISP_TUNE_ERROR_MISSED_ACCESS FX1_ISP_ACCESS_W1C
/**
 * MODE [5:5], dòng bảng 832, kiểu truy cập W1C
 * Lỗi MODE: đổi mode chưa xử lý hết, dùng lệnh legacy trong extension hoặc lệnh profile trong
 * legacy, ghi đầu vào/lớp IDLE khi pipeline chưa rỗi/đang profile, hoặc đổi TUNE_MODE khi còn lease
 * thống kê. Giữ nguyên trạng thái.
 */
#define FX1_ISP_TUNE_ERROR_MODE_SHIFT 5U
#define FX1_ISP_TUNE_ERROR_MODE_WIDTH 1U
#define FX1_ISP_TUNE_ERROR_MODE_MASK 0x00000020U
#define FX1_ISP_TUNE_ERROR_MODE_RESET 0x00000000U
#define FX1_ISP_TUNE_ERROR_MODE_ACCESS FX1_ISP_ACCESS_W1C
/**
 * NO_LEASE [6:6], dòng bảng 833, kiểu truy cập W1C
 * Truy cập kết quả chưa có quyền đọc hoặc thao tác xin quyền lặp.
 */
#define FX1_ISP_TUNE_ERROR_NO_LEASE_SHIFT 6U
#define FX1_ISP_TUNE_ERROR_NO_LEASE_WIDTH 1U
#define FX1_ISP_TUNE_ERROR_NO_LEASE_MASK 0x00000040U
#define FX1_ISP_TUNE_ERROR_NO_LEASE_RESET 0x00000000U
#define FX1_ISP_TUNE_ERROR_NO_LEASE_ACCESS FX1_ISP_ACCESS_W1C
/**
 * OVERFLOW [7:7], dòng bảng 834, kiểu truy cập W1C
 * Bộ tích lũy kết quả dạng kẹp đã tràn.
 */
#define FX1_ISP_TUNE_ERROR_OVERFLOW_SHIFT 7U
#define FX1_ISP_TUNE_ERROR_OVERFLOW_WIDTH 1U
#define FX1_ISP_TUNE_ERROR_OVERFLOW_MASK 0x00000080U
#define FX1_ISP_TUNE_ERROR_OVERFLOW_RESET 0x00000000U
#define FX1_ISP_TUNE_ERROR_OVERFLOW_ACCESS FX1_ISP_ACCESS_W1C

/**
 * TUNE_NEXT_FRAME: địa chỉ lệch 0x8030, dòng bảng 835, kiểu truy cập RO
 * Bộ đếm frame 3A hiện có cộng 1; frame đầu sau hard reset là 1, soft reset giữ bộ đếm. Cập nhật
 * trực tiếp; MAS v1.07 tuning-profiles.
 */
#define FX1_ISP_TUNE_NEXT_FRAME_OFFSET 0x8030U
#define REG_TUNE_NEXT_FRAME FX1_ISP_TUNE_NEXT_FRAME_OFFSET
#define FX1_ISP_TUNE_NEXT_FRAME_RESET 0x00000001U
#define FX1_ISP_TUNE_NEXT_FRAME_ACCESS FX1_ISP_ACCESS_RO
#define FX1_ISP_TUNE_NEXT_FRAME_RO_MASK 0xFFFFFFFFU
#define FX1_ISP_TUNE_NEXT_FRAME_RW_MASK 0x00000000U
#define FX1_ISP_TUNE_NEXT_FRAME_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_NEXT_FRAME_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_NEXT_FRAME_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_NEXT_FRAME_W1SC_MASK 0x00000000U
/**
 * VALUE [31:0], dòng bảng 836, kiểu truy cập RO
 * Bộ đếm frame 3A cộng 1; đầu sau hard reset là 1, soft reset giữ bộ đếm.
 */
#define FX1_ISP_TUNE_NEXT_FRAME_VALUE_SHIFT 0U
#define FX1_ISP_TUNE_NEXT_FRAME_VALUE_WIDTH 32U
#define FX1_ISP_TUNE_NEXT_FRAME_VALUE_MASK 0xFFFFFFFFU
#define FX1_ISP_TUNE_NEXT_FRAME_VALUE_RESET 0x00000001U
#define FX1_ISP_TUNE_NEXT_FRAME_VALUE_ACCESS FX1_ISP_ACCESS_RO

/**
 * TUNE_CH0_BUF0_PROFILE: địa chỉ lệch 0x8040, dòng bảng 837, kiểu truy cập RO
 * ID profile công bố cùng thông tin buffer ra đã hoàn tất, giữ đến lần buffer đó hoàn tất thành công
 * tiếp. Cập nhật theo kết quả, MAS v1.07 tuning-profiles.
 */
#define FX1_ISP_TUNE_CH0_BUF0_PROFILE_OFFSET 0x8040U
#define REG_TUNE_CH0_BUF0_PROFILE FX1_ISP_TUNE_CH0_BUF0_PROFILE_OFFSET
#define FX1_ISP_TUNE_CH0_BUF0_PROFILE_RESET 0x00000000U
#define FX1_ISP_TUNE_CH0_BUF0_PROFILE_ACCESS FX1_ISP_ACCESS_RO
#define FX1_ISP_TUNE_CH0_BUF0_PROFILE_RO_MASK 0xFFFFFFFFU
#define FX1_ISP_TUNE_CH0_BUF0_PROFILE_RW_MASK 0x00000000U
#define FX1_ISP_TUNE_CH0_BUF0_PROFILE_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_CH0_BUF0_PROFILE_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_CH0_BUF0_PROFILE_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_CH0_BUF0_PROFILE_W1SC_MASK 0x00000000U
/**
 * VALUE [31:0], dòng bảng 838, kiểu truy cập RO
 * ID profile của buffer ra hoàn tất, giữ đến lần buffer đó hoàn tất thành công tiếp.
 */
#define FX1_ISP_TUNE_CH0_BUF0_PROFILE_VALUE_SHIFT 0U
#define FX1_ISP_TUNE_CH0_BUF0_PROFILE_VALUE_WIDTH 32U
#define FX1_ISP_TUNE_CH0_BUF0_PROFILE_VALUE_MASK 0xFFFFFFFFU
#define FX1_ISP_TUNE_CH0_BUF0_PROFILE_VALUE_RESET 0x00000000U
#define FX1_ISP_TUNE_CH0_BUF0_PROFILE_VALUE_ACCESS FX1_ISP_ACCESS_RO

/**
 * TUNE_CH0_BUF1_PROFILE: địa chỉ lệch 0x8044, dòng bảng 839, kiểu truy cập RO
 * ID profile công bố cùng thông tin buffer ra đã hoàn tất, giữ đến lần buffer đó hoàn tất thành công
 * tiếp. Cập nhật theo kết quả, MAS v1.07 tuning-profiles.
 */
#define FX1_ISP_TUNE_CH0_BUF1_PROFILE_OFFSET 0x8044U
#define REG_TUNE_CH0_BUF1_PROFILE FX1_ISP_TUNE_CH0_BUF1_PROFILE_OFFSET
#define FX1_ISP_TUNE_CH0_BUF1_PROFILE_RESET 0x00000000U
#define FX1_ISP_TUNE_CH0_BUF1_PROFILE_ACCESS FX1_ISP_ACCESS_RO
#define FX1_ISP_TUNE_CH0_BUF1_PROFILE_RO_MASK 0xFFFFFFFFU
#define FX1_ISP_TUNE_CH0_BUF1_PROFILE_RW_MASK 0x00000000U
#define FX1_ISP_TUNE_CH0_BUF1_PROFILE_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_CH0_BUF1_PROFILE_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_CH0_BUF1_PROFILE_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_CH0_BUF1_PROFILE_W1SC_MASK 0x00000000U
/**
 * VALUE [31:0], dòng bảng 840, kiểu truy cập RO
 * ID profile của buffer ra hoàn tất, giữ đến lần buffer đó hoàn tất thành công tiếp.
 */
#define FX1_ISP_TUNE_CH0_BUF1_PROFILE_VALUE_SHIFT 0U
#define FX1_ISP_TUNE_CH0_BUF1_PROFILE_VALUE_WIDTH 32U
#define FX1_ISP_TUNE_CH0_BUF1_PROFILE_VALUE_MASK 0xFFFFFFFFU
#define FX1_ISP_TUNE_CH0_BUF1_PROFILE_VALUE_RESET 0x00000000U
#define FX1_ISP_TUNE_CH0_BUF1_PROFILE_VALUE_ACCESS FX1_ISP_ACCESS_RO

/**
 * TUNE_CH0_BUF2_PROFILE: địa chỉ lệch 0x8048, dòng bảng 841, kiểu truy cập RO
 * ID profile công bố cùng thông tin buffer ra đã hoàn tất, giữ đến lần buffer đó hoàn tất thành công
 * tiếp. Cập nhật theo kết quả, MAS v1.07 tuning-profiles.
 */
#define FX1_ISP_TUNE_CH0_BUF2_PROFILE_OFFSET 0x8048U
#define REG_TUNE_CH0_BUF2_PROFILE FX1_ISP_TUNE_CH0_BUF2_PROFILE_OFFSET
#define FX1_ISP_TUNE_CH0_BUF2_PROFILE_RESET 0x00000000U
#define FX1_ISP_TUNE_CH0_BUF2_PROFILE_ACCESS FX1_ISP_ACCESS_RO
#define FX1_ISP_TUNE_CH0_BUF2_PROFILE_RO_MASK 0xFFFFFFFFU
#define FX1_ISP_TUNE_CH0_BUF2_PROFILE_RW_MASK 0x00000000U
#define FX1_ISP_TUNE_CH0_BUF2_PROFILE_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_CH0_BUF2_PROFILE_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_CH0_BUF2_PROFILE_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_CH0_BUF2_PROFILE_W1SC_MASK 0x00000000U
/**
 * VALUE [31:0], dòng bảng 842, kiểu truy cập RO
 * ID profile của buffer ra hoàn tất, giữ đến lần buffer đó hoàn tất thành công tiếp.
 */
#define FX1_ISP_TUNE_CH0_BUF2_PROFILE_VALUE_SHIFT 0U
#define FX1_ISP_TUNE_CH0_BUF2_PROFILE_VALUE_WIDTH 32U
#define FX1_ISP_TUNE_CH0_BUF2_PROFILE_VALUE_MASK 0xFFFFFFFFU
#define FX1_ISP_TUNE_CH0_BUF2_PROFILE_VALUE_RESET 0x00000000U
#define FX1_ISP_TUNE_CH0_BUF2_PROFILE_VALUE_ACCESS FX1_ISP_ACCESS_RO

/**
 * TUNE_CH0_BUF3_PROFILE: địa chỉ lệch 0x804C, dòng bảng 843, kiểu truy cập RO
 * ID profile công bố cùng thông tin buffer ra đã hoàn tất, giữ đến lần buffer đó hoàn tất thành công
 * tiếp. Cập nhật theo kết quả, MAS v1.07 tuning-profiles.
 */
#define FX1_ISP_TUNE_CH0_BUF3_PROFILE_OFFSET 0x804CU
#define REG_TUNE_CH0_BUF3_PROFILE FX1_ISP_TUNE_CH0_BUF3_PROFILE_OFFSET
#define FX1_ISP_TUNE_CH0_BUF3_PROFILE_RESET 0x00000000U
#define FX1_ISP_TUNE_CH0_BUF3_PROFILE_ACCESS FX1_ISP_ACCESS_RO
#define FX1_ISP_TUNE_CH0_BUF3_PROFILE_RO_MASK 0xFFFFFFFFU
#define FX1_ISP_TUNE_CH0_BUF3_PROFILE_RW_MASK 0x00000000U
#define FX1_ISP_TUNE_CH0_BUF3_PROFILE_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_CH0_BUF3_PROFILE_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_CH0_BUF3_PROFILE_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_CH0_BUF3_PROFILE_W1SC_MASK 0x00000000U
/**
 * VALUE [31:0], dòng bảng 844, kiểu truy cập RO
 * ID profile của buffer ra hoàn tất, giữ đến lần buffer đó hoàn tất thành công tiếp.
 */
#define FX1_ISP_TUNE_CH0_BUF3_PROFILE_VALUE_SHIFT 0U
#define FX1_ISP_TUNE_CH0_BUF3_PROFILE_VALUE_WIDTH 32U
#define FX1_ISP_TUNE_CH0_BUF3_PROFILE_VALUE_MASK 0xFFFFFFFFU
#define FX1_ISP_TUNE_CH0_BUF3_PROFILE_VALUE_RESET 0x00000000U
#define FX1_ISP_TUNE_CH0_BUF3_PROFILE_VALUE_ACCESS FX1_ISP_ACCESS_RO

/**
 * TUNE_CH1_BUF0_PROFILE: địa chỉ lệch 0x8050, dòng bảng 845, kiểu truy cập RO
 * ID profile công bố cùng thông tin buffer ra đã hoàn tất, giữ đến lần buffer đó hoàn tất thành công
 * tiếp. Cập nhật theo kết quả, MAS v1.07 tuning-profiles.
 */
#define FX1_ISP_TUNE_CH1_BUF0_PROFILE_OFFSET 0x8050U
#define REG_TUNE_CH1_BUF0_PROFILE FX1_ISP_TUNE_CH1_BUF0_PROFILE_OFFSET
#define FX1_ISP_TUNE_CH1_BUF0_PROFILE_RESET 0x00000000U
#define FX1_ISP_TUNE_CH1_BUF0_PROFILE_ACCESS FX1_ISP_ACCESS_RO
#define FX1_ISP_TUNE_CH1_BUF0_PROFILE_RO_MASK 0xFFFFFFFFU
#define FX1_ISP_TUNE_CH1_BUF0_PROFILE_RW_MASK 0x00000000U
#define FX1_ISP_TUNE_CH1_BUF0_PROFILE_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_CH1_BUF0_PROFILE_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_CH1_BUF0_PROFILE_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_CH1_BUF0_PROFILE_W1SC_MASK 0x00000000U
/**
 * VALUE [31:0], dòng bảng 846, kiểu truy cập RO
 * ID profile của buffer ra hoàn tất, giữ đến lần buffer đó hoàn tất thành công tiếp.
 */
#define FX1_ISP_TUNE_CH1_BUF0_PROFILE_VALUE_SHIFT 0U
#define FX1_ISP_TUNE_CH1_BUF0_PROFILE_VALUE_WIDTH 32U
#define FX1_ISP_TUNE_CH1_BUF0_PROFILE_VALUE_MASK 0xFFFFFFFFU
#define FX1_ISP_TUNE_CH1_BUF0_PROFILE_VALUE_RESET 0x00000000U
#define FX1_ISP_TUNE_CH1_BUF0_PROFILE_VALUE_ACCESS FX1_ISP_ACCESS_RO

/**
 * TUNE_CH1_BUF1_PROFILE: địa chỉ lệch 0x8054, dòng bảng 847, kiểu truy cập RO
 * ID profile công bố cùng thông tin buffer ra đã hoàn tất, giữ đến lần buffer đó hoàn tất thành công
 * tiếp. Cập nhật theo kết quả, MAS v1.07 tuning-profiles.
 */
#define FX1_ISP_TUNE_CH1_BUF1_PROFILE_OFFSET 0x8054U
#define REG_TUNE_CH1_BUF1_PROFILE FX1_ISP_TUNE_CH1_BUF1_PROFILE_OFFSET
#define FX1_ISP_TUNE_CH1_BUF1_PROFILE_RESET 0x00000000U
#define FX1_ISP_TUNE_CH1_BUF1_PROFILE_ACCESS FX1_ISP_ACCESS_RO
#define FX1_ISP_TUNE_CH1_BUF1_PROFILE_RO_MASK 0xFFFFFFFFU
#define FX1_ISP_TUNE_CH1_BUF1_PROFILE_RW_MASK 0x00000000U
#define FX1_ISP_TUNE_CH1_BUF1_PROFILE_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_CH1_BUF1_PROFILE_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_CH1_BUF1_PROFILE_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_CH1_BUF1_PROFILE_W1SC_MASK 0x00000000U
/**
 * VALUE [31:0], dòng bảng 848, kiểu truy cập RO
 * ID profile của buffer ra hoàn tất, giữ đến lần buffer đó hoàn tất thành công tiếp.
 */
#define FX1_ISP_TUNE_CH1_BUF1_PROFILE_VALUE_SHIFT 0U
#define FX1_ISP_TUNE_CH1_BUF1_PROFILE_VALUE_WIDTH 32U
#define FX1_ISP_TUNE_CH1_BUF1_PROFILE_VALUE_MASK 0xFFFFFFFFU
#define FX1_ISP_TUNE_CH1_BUF1_PROFILE_VALUE_RESET 0x00000000U
#define FX1_ISP_TUNE_CH1_BUF1_PROFILE_VALUE_ACCESS FX1_ISP_ACCESS_RO

/**
 * TUNE_CH1_BUF2_PROFILE: địa chỉ lệch 0x8058, dòng bảng 849, kiểu truy cập RO
 * ID profile công bố cùng thông tin buffer ra đã hoàn tất, giữ đến lần buffer đó hoàn tất thành công
 * tiếp. Cập nhật theo kết quả, MAS v1.07 tuning-profiles.
 */
#define FX1_ISP_TUNE_CH1_BUF2_PROFILE_OFFSET 0x8058U
#define REG_TUNE_CH1_BUF2_PROFILE FX1_ISP_TUNE_CH1_BUF2_PROFILE_OFFSET
#define FX1_ISP_TUNE_CH1_BUF2_PROFILE_RESET 0x00000000U
#define FX1_ISP_TUNE_CH1_BUF2_PROFILE_ACCESS FX1_ISP_ACCESS_RO
#define FX1_ISP_TUNE_CH1_BUF2_PROFILE_RO_MASK 0xFFFFFFFFU
#define FX1_ISP_TUNE_CH1_BUF2_PROFILE_RW_MASK 0x00000000U
#define FX1_ISP_TUNE_CH1_BUF2_PROFILE_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_CH1_BUF2_PROFILE_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_CH1_BUF2_PROFILE_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_CH1_BUF2_PROFILE_W1SC_MASK 0x00000000U
/**
 * VALUE [31:0], dòng bảng 850, kiểu truy cập RO
 * ID profile của buffer ra hoàn tất, giữ đến lần buffer đó hoàn tất thành công tiếp.
 */
#define FX1_ISP_TUNE_CH1_BUF2_PROFILE_VALUE_SHIFT 0U
#define FX1_ISP_TUNE_CH1_BUF2_PROFILE_VALUE_WIDTH 32U
#define FX1_ISP_TUNE_CH1_BUF2_PROFILE_VALUE_MASK 0xFFFFFFFFU
#define FX1_ISP_TUNE_CH1_BUF2_PROFILE_VALUE_RESET 0x00000000U
#define FX1_ISP_TUNE_CH1_BUF2_PROFILE_VALUE_ACCESS FX1_ISP_ACCESS_RO

/**
 * TUNE_CH1_BUF3_PROFILE: địa chỉ lệch 0x805C, dòng bảng 851, kiểu truy cập RO
 * ID profile công bố cùng thông tin buffer ra đã hoàn tất, giữ đến lần buffer đó hoàn tất thành công
 * tiếp. Cập nhật theo kết quả, MAS v1.07 tuning-profiles.
 */
#define FX1_ISP_TUNE_CH1_BUF3_PROFILE_OFFSET 0x805CU
#define REG_TUNE_CH1_BUF3_PROFILE FX1_ISP_TUNE_CH1_BUF3_PROFILE_OFFSET
#define FX1_ISP_TUNE_CH1_BUF3_PROFILE_RESET 0x00000000U
#define FX1_ISP_TUNE_CH1_BUF3_PROFILE_ACCESS FX1_ISP_ACCESS_RO
#define FX1_ISP_TUNE_CH1_BUF3_PROFILE_RO_MASK 0xFFFFFFFFU
#define FX1_ISP_TUNE_CH1_BUF3_PROFILE_RW_MASK 0x00000000U
#define FX1_ISP_TUNE_CH1_BUF3_PROFILE_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_CH1_BUF3_PROFILE_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_CH1_BUF3_PROFILE_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_CH1_BUF3_PROFILE_W1SC_MASK 0x00000000U
/**
 * VALUE [31:0], dòng bảng 852, kiểu truy cập RO
 * ID profile của buffer ra hoàn tất, giữ đến lần buffer đó hoàn tất thành công tiếp.
 */
#define FX1_ISP_TUNE_CH1_BUF3_PROFILE_VALUE_SHIFT 0U
#define FX1_ISP_TUNE_CH1_BUF3_PROFILE_VALUE_WIDTH 32U
#define FX1_ISP_TUNE_CH1_BUF3_PROFILE_VALUE_MASK 0xFFFFFFFFU
#define FX1_ISP_TUNE_CH1_BUF3_PROFILE_VALUE_RESET 0x00000000U
#define FX1_ISP_TUNE_CH1_BUF3_PROFILE_VALUE_ACCESS FX1_ISP_ACCESS_RO

/**
 * TUNE_CH2_BUF0_PROFILE: địa chỉ lệch 0x8060, dòng bảng 853, kiểu truy cập RO
 * ID profile công bố cùng thông tin buffer ra đã hoàn tất, giữ đến lần buffer đó hoàn tất thành công
 * tiếp. Cập nhật theo kết quả, MAS v1.07 tuning-profiles.
 */
#define FX1_ISP_TUNE_CH2_BUF0_PROFILE_OFFSET 0x8060U
#define REG_TUNE_CH2_BUF0_PROFILE FX1_ISP_TUNE_CH2_BUF0_PROFILE_OFFSET
#define FX1_ISP_TUNE_CH2_BUF0_PROFILE_RESET 0x00000000U
#define FX1_ISP_TUNE_CH2_BUF0_PROFILE_ACCESS FX1_ISP_ACCESS_RO
#define FX1_ISP_TUNE_CH2_BUF0_PROFILE_RO_MASK 0xFFFFFFFFU
#define FX1_ISP_TUNE_CH2_BUF0_PROFILE_RW_MASK 0x00000000U
#define FX1_ISP_TUNE_CH2_BUF0_PROFILE_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_CH2_BUF0_PROFILE_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_CH2_BUF0_PROFILE_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_CH2_BUF0_PROFILE_W1SC_MASK 0x00000000U
/**
 * VALUE [31:0], dòng bảng 854, kiểu truy cập RO
 * ID profile của buffer ra hoàn tất, giữ đến lần buffer đó hoàn tất thành công tiếp.
 */
#define FX1_ISP_TUNE_CH2_BUF0_PROFILE_VALUE_SHIFT 0U
#define FX1_ISP_TUNE_CH2_BUF0_PROFILE_VALUE_WIDTH 32U
#define FX1_ISP_TUNE_CH2_BUF0_PROFILE_VALUE_MASK 0xFFFFFFFFU
#define FX1_ISP_TUNE_CH2_BUF0_PROFILE_VALUE_RESET 0x00000000U
#define FX1_ISP_TUNE_CH2_BUF0_PROFILE_VALUE_ACCESS FX1_ISP_ACCESS_RO

/**
 * TUNE_CH2_BUF1_PROFILE: địa chỉ lệch 0x8064, dòng bảng 855, kiểu truy cập RO
 * ID profile công bố cùng thông tin buffer ra đã hoàn tất, giữ đến lần buffer đó hoàn tất thành công
 * tiếp. Cập nhật theo kết quả, MAS v1.07 tuning-profiles.
 */
#define FX1_ISP_TUNE_CH2_BUF1_PROFILE_OFFSET 0x8064U
#define REG_TUNE_CH2_BUF1_PROFILE FX1_ISP_TUNE_CH2_BUF1_PROFILE_OFFSET
#define FX1_ISP_TUNE_CH2_BUF1_PROFILE_RESET 0x00000000U
#define FX1_ISP_TUNE_CH2_BUF1_PROFILE_ACCESS FX1_ISP_ACCESS_RO
#define FX1_ISP_TUNE_CH2_BUF1_PROFILE_RO_MASK 0xFFFFFFFFU
#define FX1_ISP_TUNE_CH2_BUF1_PROFILE_RW_MASK 0x00000000U
#define FX1_ISP_TUNE_CH2_BUF1_PROFILE_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_CH2_BUF1_PROFILE_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_CH2_BUF1_PROFILE_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_CH2_BUF1_PROFILE_W1SC_MASK 0x00000000U
/**
 * VALUE [31:0], dòng bảng 856, kiểu truy cập RO
 * ID profile của buffer ra hoàn tất, giữ đến lần buffer đó hoàn tất thành công tiếp.
 */
#define FX1_ISP_TUNE_CH2_BUF1_PROFILE_VALUE_SHIFT 0U
#define FX1_ISP_TUNE_CH2_BUF1_PROFILE_VALUE_WIDTH 32U
#define FX1_ISP_TUNE_CH2_BUF1_PROFILE_VALUE_MASK 0xFFFFFFFFU
#define FX1_ISP_TUNE_CH2_BUF1_PROFILE_VALUE_RESET 0x00000000U
#define FX1_ISP_TUNE_CH2_BUF1_PROFILE_VALUE_ACCESS FX1_ISP_ACCESS_RO

/**
 * TUNE_CH2_BUF2_PROFILE: địa chỉ lệch 0x8068, dòng bảng 857, kiểu truy cập RO
 * ID profile công bố cùng thông tin buffer ra đã hoàn tất, giữ đến lần buffer đó hoàn tất thành công
 * tiếp. Cập nhật theo kết quả, MAS v1.07 tuning-profiles.
 */
#define FX1_ISP_TUNE_CH2_BUF2_PROFILE_OFFSET 0x8068U
#define REG_TUNE_CH2_BUF2_PROFILE FX1_ISP_TUNE_CH2_BUF2_PROFILE_OFFSET
#define FX1_ISP_TUNE_CH2_BUF2_PROFILE_RESET 0x00000000U
#define FX1_ISP_TUNE_CH2_BUF2_PROFILE_ACCESS FX1_ISP_ACCESS_RO
#define FX1_ISP_TUNE_CH2_BUF2_PROFILE_RO_MASK 0xFFFFFFFFU
#define FX1_ISP_TUNE_CH2_BUF2_PROFILE_RW_MASK 0x00000000U
#define FX1_ISP_TUNE_CH2_BUF2_PROFILE_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_CH2_BUF2_PROFILE_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_CH2_BUF2_PROFILE_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_CH2_BUF2_PROFILE_W1SC_MASK 0x00000000U
/**
 * VALUE [31:0], dòng bảng 858, kiểu truy cập RO
 * ID profile của buffer ra hoàn tất, giữ đến lần buffer đó hoàn tất thành công tiếp.
 */
#define FX1_ISP_TUNE_CH2_BUF2_PROFILE_VALUE_SHIFT 0U
#define FX1_ISP_TUNE_CH2_BUF2_PROFILE_VALUE_WIDTH 32U
#define FX1_ISP_TUNE_CH2_BUF2_PROFILE_VALUE_MASK 0xFFFFFFFFU
#define FX1_ISP_TUNE_CH2_BUF2_PROFILE_VALUE_RESET 0x00000000U
#define FX1_ISP_TUNE_CH2_BUF2_PROFILE_VALUE_ACCESS FX1_ISP_ACCESS_RO

/**
 * TUNE_CH2_BUF3_PROFILE: địa chỉ lệch 0x806C, dòng bảng 859, kiểu truy cập RO
 * ID profile công bố cùng thông tin buffer ra đã hoàn tất, giữ đến lần buffer đó hoàn tất thành công
 * tiếp. Cập nhật theo kết quả, MAS v1.07 tuning-profiles.
 */
#define FX1_ISP_TUNE_CH2_BUF3_PROFILE_OFFSET 0x806CU
#define REG_TUNE_CH2_BUF3_PROFILE FX1_ISP_TUNE_CH2_BUF3_PROFILE_OFFSET
#define FX1_ISP_TUNE_CH2_BUF3_PROFILE_RESET 0x00000000U
#define FX1_ISP_TUNE_CH2_BUF3_PROFILE_ACCESS FX1_ISP_ACCESS_RO
#define FX1_ISP_TUNE_CH2_BUF3_PROFILE_RO_MASK 0xFFFFFFFFU
#define FX1_ISP_TUNE_CH2_BUF3_PROFILE_RW_MASK 0x00000000U
#define FX1_ISP_TUNE_CH2_BUF3_PROFILE_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_CH2_BUF3_PROFILE_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_CH2_BUF3_PROFILE_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_CH2_BUF3_PROFILE_W1SC_MASK 0x00000000U
/**
 * VALUE [31:0], dòng bảng 860, kiểu truy cập RO
 * ID profile của buffer ra hoàn tất, giữ đến lần buffer đó hoàn tất thành công tiếp.
 */
#define FX1_ISP_TUNE_CH2_BUF3_PROFILE_VALUE_SHIFT 0U
#define FX1_ISP_TUNE_CH2_BUF3_PROFILE_VALUE_WIDTH 32U
#define FX1_ISP_TUNE_CH2_BUF3_PROFILE_VALUE_MASK 0xFFFFFFFFU
#define FX1_ISP_TUNE_CH2_BUF3_PROFILE_VALUE_RESET 0x00000000U
#define FX1_ISP_TUNE_CH2_BUF3_PROFILE_VALUE_ACCESS FX1_ISP_ACCESS_RO

/**
 * TUNE_NR_CTRL: địa chỉ lệch 0x8100, dòng bảng 861, kiểu truy cập RW
 * Chọn mô hình nhiễu và bộ lọc trung vị; cập nhật theo profile, MAS v1.07 tuning-noise.
 */
#define FX1_ISP_TUNE_NR_CTRL_OFFSET 0x8100U
#define REG_TUNE_NR_CTRL FX1_ISP_TUNE_NR_CTRL_OFFSET
#define FX1_ISP_TUNE_NR_CTRL_RESET 0x00000004U
#define FX1_ISP_TUNE_NR_CTRL_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_TUNE_NR_CTRL_RO_MASK 0x00000000U
#define FX1_ISP_TUNE_NR_CTRL_RW_MASK 0x00000007U
#define FX1_ISP_TUNE_NR_CTRL_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_NR_CTRL_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_NR_CTRL_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_NR_CTRL_W1SC_MASK 0x00000000U
/**
 * MODE [1:0], dòng bảng 862, kiểu truy cập RW
 * Chọn phương sai: 0=ước lượng frame trước,1=thủ công,2=LUT theo mức sáng,3=không hợp lệ.
 */
#define FX1_ISP_TUNE_NR_CTRL_MODE_SHIFT 0U
#define FX1_ISP_TUNE_NR_CTRL_MODE_WIDTH 2U
#define FX1_ISP_TUNE_NR_CTRL_MODE_MASK 0x00000003U
#define FX1_ISP_TUNE_NR_CTRL_MODE_RESET 0x00000000U
#define FX1_ISP_TUNE_NR_CTRL_MODE_ACCESS FX1_ISP_ACCESS_RW
/**
 * MEDIAN_EN [2:2], dòng bảng 863, kiểu truy cập RW
 * Bật lọc trung vị trước; đường tắt vẫn giữ thời gian khớp.
 */
#define FX1_ISP_TUNE_NR_CTRL_MEDIAN_EN_SHIFT 2U
#define FX1_ISP_TUNE_NR_CTRL_MEDIAN_EN_WIDTH 1U
#define FX1_ISP_TUNE_NR_CTRL_MEDIAN_EN_MASK 0x00000004U
#define FX1_ISP_TUNE_NR_CTRL_MEDIAN_EN_RESET 0x00000001U
#define FX1_ISP_TUNE_NR_CTRL_MEDIAN_EN_ACCESS FX1_ISP_ACCESS_RW

/**
 * TUNE_NR_VARIANCE: địa chỉ lệch 0x8104, dòng bảng 864, kiểu truy cập RW
 * Phương sai thủ công theo đơn vị phương sai hệ số Haar hiện tại; cập nhật theo profile, MAS v1.07
 * tuning-noise.
 */
#define FX1_ISP_TUNE_NR_VARIANCE_OFFSET 0x8104U
#define REG_TUNE_NR_VARIANCE FX1_ISP_TUNE_NR_VARIANCE_OFFSET
#define FX1_ISP_TUNE_NR_VARIANCE_RESET 0x00000000U
#define FX1_ISP_TUNE_NR_VARIANCE_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_TUNE_NR_VARIANCE_RO_MASK 0x00000000U
#define FX1_ISP_TUNE_NR_VARIANCE_RW_MASK 0x0000FFFFU
#define FX1_ISP_TUNE_NR_VARIANCE_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_NR_VARIANCE_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_NR_VARIANCE_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_NR_VARIANCE_W1SC_MASK 0x00000000U
/**
 * VALUE [15:0], dòng bảng 865, kiểu truy cập RW
 * Phương sai thủ công theo đơn vị phương sai hệ số Haar.
 */
#define FX1_ISP_TUNE_NR_VARIANCE_VALUE_SHIFT 0U
#define FX1_ISP_TUNE_NR_VARIANCE_VALUE_WIDTH 16U
#define FX1_ISP_TUNE_NR_VARIANCE_VALUE_MASK 0x0000FFFFU
#define FX1_ISP_TUNE_NR_VARIANCE_VALUE_RESET 0x00000000U
#define FX1_ISP_TUNE_NR_VARIANCE_VALUE_ACCESS FX1_ISP_ACCESS_RW

/**
 * TUNE_NR_SCALE: địa chỉ lệch 0x8108, dòng bảng 866, kiểu truy cập RW
 * Hệ số phương sai không dấu Q8.8, nhân làm tròn nửa lên rồi kẹp; cập nhật theo profile, MAS v1.07
 * tuning-noise.
 */
#define FX1_ISP_TUNE_NR_SCALE_OFFSET 0x8108U
#define REG_TUNE_NR_SCALE FX1_ISP_TUNE_NR_SCALE_OFFSET
#define FX1_ISP_TUNE_NR_SCALE_RESET 0x00000100U
#define FX1_ISP_TUNE_NR_SCALE_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_TUNE_NR_SCALE_RO_MASK 0x00000000U
#define FX1_ISP_TUNE_NR_SCALE_RW_MASK 0x0000FFFFU
#define FX1_ISP_TUNE_NR_SCALE_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_NR_SCALE_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_NR_SCALE_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_NR_SCALE_W1SC_MASK 0x00000000U
/**
 * VALUE [15:0], dòng bảng 867, kiểu truy cập RW
 * Hệ số phương sai không dấu Q8.8, nhân làm tròn nửa lên rồi kẹp.
 */
#define FX1_ISP_TUNE_NR_SCALE_VALUE_SHIFT 0U
#define FX1_ISP_TUNE_NR_SCALE_VALUE_WIDTH 16U
#define FX1_ISP_TUNE_NR_SCALE_VALUE_MASK 0x0000FFFFU
#define FX1_ISP_TUNE_NR_SCALE_VALUE_RESET 0x00000100U
#define FX1_ISP_TUNE_NR_SCALE_VALUE_ACCESS FX1_ISP_ACCESS_RW

/**
 * TUNE_NR_LIMITS: địa chỉ lệch 0x810C, dòng bảng 868, kiểu truy cập RW
 * Kẹp phương sai sau nhân hệ số, cập nhật theo profile; MAS v1.07 tuning-noise.
 */
#define FX1_ISP_TUNE_NR_LIMITS_OFFSET 0x810CU
#define REG_TUNE_NR_LIMITS FX1_ISP_TUNE_NR_LIMITS_OFFSET
#define FX1_ISP_TUNE_NR_LIMITS_RESET 0xFFFF0000U
#define FX1_ISP_TUNE_NR_LIMITS_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_TUNE_NR_LIMITS_RO_MASK 0x00000000U
#define FX1_ISP_TUNE_NR_LIMITS_RW_MASK 0xFFFFFFFFU
#define FX1_ISP_TUNE_NR_LIMITS_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_NR_LIMITS_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_NR_LIMITS_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_NR_LIMITS_W1SC_MASK 0x00000000U
/**
 * MIN [15:0], dòng bảng 869, kiểu truy cập RW
 * Cận dưới phương sai.
 */
#define FX1_ISP_TUNE_NR_LIMITS_MIN_SHIFT 0U
#define FX1_ISP_TUNE_NR_LIMITS_MIN_WIDTH 16U
#define FX1_ISP_TUNE_NR_LIMITS_MIN_MASK 0x0000FFFFU
#define FX1_ISP_TUNE_NR_LIMITS_MIN_RESET 0x00000000U
#define FX1_ISP_TUNE_NR_LIMITS_MIN_ACCESS FX1_ISP_ACCESS_RW
/**
 * MAX [31:16], dòng bảng 870, kiểu truy cập RW
 * Cận trên phương sai; MIN>MAX bị SEAL từ chối INVALID.
 */
#define FX1_ISP_TUNE_NR_LIMITS_MAX_SHIFT 16U
#define FX1_ISP_TUNE_NR_LIMITS_MAX_WIDTH 16U
#define FX1_ISP_TUNE_NR_LIMITS_MAX_MASK 0xFFFF0000U
#define FX1_ISP_TUNE_NR_LIMITS_MAX_RESET 0x0000FFFFU
#define FX1_ISP_TUNE_NR_LIMITS_MAX_ACCESS FX1_ISP_ACCESS_RW

/**
 * TUNE_NR_LL_SCALE: địa chỉ lệch 0x8110, dòng bảng 871, kiểu truy cập RW
 * Hệ số không dấu Q8.8 trên phương sai hiệu dụng cho dải LL; kẹp 16 bit. Cập nhật theo profile, MAS
 * v1.07 tuning-noise.
 */
#define FX1_ISP_TUNE_NR_LL_SCALE_OFFSET 0x8110U
#define REG_TUNE_NR_LL_SCALE FX1_ISP_TUNE_NR_LL_SCALE_OFFSET
#define FX1_ISP_TUNE_NR_LL_SCALE_RESET 0x00000100U
#define FX1_ISP_TUNE_NR_LL_SCALE_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_TUNE_NR_LL_SCALE_RO_MASK 0x00000000U
#define FX1_ISP_TUNE_NR_LL_SCALE_RW_MASK 0x0000FFFFU
#define FX1_ISP_TUNE_NR_LL_SCALE_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_NR_LL_SCALE_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_NR_LL_SCALE_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_NR_LL_SCALE_W1SC_MASK 0x00000000U
/**
 * VALUE [15:0], dòng bảng 872, kiểu truy cập RW
 * Hệ số không dấu Q8.8 trên phương sai hiệu dụng cho dải LL; kẹp 16 bit.
 */
#define FX1_ISP_TUNE_NR_LL_SCALE_VALUE_SHIFT 0U
#define FX1_ISP_TUNE_NR_LL_SCALE_VALUE_WIDTH 16U
#define FX1_ISP_TUNE_NR_LL_SCALE_VALUE_MASK 0x0000FFFFU
#define FX1_ISP_TUNE_NR_LL_SCALE_VALUE_RESET 0x00000100U
#define FX1_ISP_TUNE_NR_LL_SCALE_VALUE_ACCESS FX1_ISP_ACCESS_RW

/**
 * TUNE_NR_LH_SCALE: địa chỉ lệch 0x8114, dòng bảng 873, kiểu truy cập RW
 * Hệ số không dấu Q8.8 trên phương sai hiệu dụng cho dải LH; kẹp 16 bit. Cập nhật theo profile, MAS
 * v1.07 tuning-noise.
 */
#define FX1_ISP_TUNE_NR_LH_SCALE_OFFSET 0x8114U
#define REG_TUNE_NR_LH_SCALE FX1_ISP_TUNE_NR_LH_SCALE_OFFSET
#define FX1_ISP_TUNE_NR_LH_SCALE_RESET 0x00000100U
#define FX1_ISP_TUNE_NR_LH_SCALE_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_TUNE_NR_LH_SCALE_RO_MASK 0x00000000U
#define FX1_ISP_TUNE_NR_LH_SCALE_RW_MASK 0x0000FFFFU
#define FX1_ISP_TUNE_NR_LH_SCALE_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_NR_LH_SCALE_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_NR_LH_SCALE_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_NR_LH_SCALE_W1SC_MASK 0x00000000U
/**
 * VALUE [15:0], dòng bảng 874, kiểu truy cập RW
 * Hệ số không dấu Q8.8 trên phương sai hiệu dụng cho dải LH; kẹp 16 bit.
 */
#define FX1_ISP_TUNE_NR_LH_SCALE_VALUE_SHIFT 0U
#define FX1_ISP_TUNE_NR_LH_SCALE_VALUE_WIDTH 16U
#define FX1_ISP_TUNE_NR_LH_SCALE_VALUE_MASK 0x0000FFFFU
#define FX1_ISP_TUNE_NR_LH_SCALE_VALUE_RESET 0x00000100U
#define FX1_ISP_TUNE_NR_LH_SCALE_VALUE_ACCESS FX1_ISP_ACCESS_RW

/**
 * TUNE_NR_HL_SCALE: địa chỉ lệch 0x8118, dòng bảng 875, kiểu truy cập RW
 * Hệ số không dấu Q8.8 trên phương sai hiệu dụng cho dải HL; kẹp 16 bit. Cập nhật theo profile, MAS
 * v1.07 tuning-noise.
 */
#define FX1_ISP_TUNE_NR_HL_SCALE_OFFSET 0x8118U
#define REG_TUNE_NR_HL_SCALE FX1_ISP_TUNE_NR_HL_SCALE_OFFSET
#define FX1_ISP_TUNE_NR_HL_SCALE_RESET 0x00000100U
#define FX1_ISP_TUNE_NR_HL_SCALE_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_TUNE_NR_HL_SCALE_RO_MASK 0x00000000U
#define FX1_ISP_TUNE_NR_HL_SCALE_RW_MASK 0x0000FFFFU
#define FX1_ISP_TUNE_NR_HL_SCALE_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_NR_HL_SCALE_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_NR_HL_SCALE_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_NR_HL_SCALE_W1SC_MASK 0x00000000U
/**
 * VALUE [15:0], dòng bảng 876, kiểu truy cập RW
 * Hệ số không dấu Q8.8 trên phương sai hiệu dụng cho dải HL; kẹp 16 bit.
 */
#define FX1_ISP_TUNE_NR_HL_SCALE_VALUE_SHIFT 0U
#define FX1_ISP_TUNE_NR_HL_SCALE_VALUE_WIDTH 16U
#define FX1_ISP_TUNE_NR_HL_SCALE_VALUE_MASK 0x0000FFFFU
#define FX1_ISP_TUNE_NR_HL_SCALE_VALUE_RESET 0x00000100U
#define FX1_ISP_TUNE_NR_HL_SCALE_VALUE_ACCESS FX1_ISP_ACCESS_RW

/**
 * TUNE_NR_HH_SCALE: địa chỉ lệch 0x811C, dòng bảng 877, kiểu truy cập RW
 * Hệ số không dấu Q8.8 trên phương sai hiệu dụng cho dải HH; kẹp 16 bit. Cập nhật theo profile, MAS
 * v1.07 tuning-noise.
 */
#define FX1_ISP_TUNE_NR_HH_SCALE_OFFSET 0x811CU
#define REG_TUNE_NR_HH_SCALE FX1_ISP_TUNE_NR_HH_SCALE_OFFSET
#define FX1_ISP_TUNE_NR_HH_SCALE_RESET 0x00000100U
#define FX1_ISP_TUNE_NR_HH_SCALE_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_TUNE_NR_HH_SCALE_RO_MASK 0x00000000U
#define FX1_ISP_TUNE_NR_HH_SCALE_RW_MASK 0x0000FFFFU
#define FX1_ISP_TUNE_NR_HH_SCALE_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_NR_HH_SCALE_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_NR_HH_SCALE_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_NR_HH_SCALE_W1SC_MASK 0x00000000U
/**
 * VALUE [15:0], dòng bảng 878, kiểu truy cập RW
 * Hệ số không dấu Q8.8 trên phương sai hiệu dụng cho dải HH; kẹp 16 bit.
 */
#define FX1_ISP_TUNE_NR_HH_SCALE_VALUE_SHIFT 0U
#define FX1_ISP_TUNE_NR_HH_SCALE_VALUE_WIDTH 16U
#define FX1_ISP_TUNE_NR_HH_SCALE_VALUE_MASK 0x0000FFFFU
#define FX1_ISP_TUNE_NR_HH_SCALE_VALUE_RESET 0x00000100U
#define FX1_ISP_TUNE_NR_HH_SCALE_VALUE_ACCESS FX1_ISP_ACCESS_RW

/**
 * TUNE_NR_BLEND: địa chỉ lệch 0x8120, dòng bảng 879, kiểu truy cập RW
 * Trọng số phần lọc 0..256 trong ((256-w)*original+w*filtered+128)>>8; cập nhật theo profile, MAS
 * v1.07 tuning-noise.
 */
#define FX1_ISP_TUNE_NR_BLEND_OFFSET 0x8120U
#define REG_TUNE_NR_BLEND FX1_ISP_TUNE_NR_BLEND_OFFSET
#define FX1_ISP_TUNE_NR_BLEND_RESET 0x00000100U
#define FX1_ISP_TUNE_NR_BLEND_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_TUNE_NR_BLEND_RO_MASK 0x00000000U
#define FX1_ISP_TUNE_NR_BLEND_RW_MASK 0x000001FFU
#define FX1_ISP_TUNE_NR_BLEND_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_NR_BLEND_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_NR_BLEND_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_NR_BLEND_W1SC_MASK 0x00000000U
/**
 * VALUE [8:0], dòng bảng 880, kiểu truy cập RW
 * Trọng số phần lọc 0..256 trong ((256-w)*original+w*filtered+128)>>8.
 */
#define FX1_ISP_TUNE_NR_BLEND_VALUE_SHIFT 0U
#define FX1_ISP_TUNE_NR_BLEND_VALUE_WIDTH 9U
#define FX1_ISP_TUNE_NR_BLEND_VALUE_MASK 0x000001FFU
#define FX1_ISP_TUNE_NR_BLEND_VALUE_RESET 0x00000100U
#define FX1_ISP_TUNE_NR_BLEND_VALUE_ACCESS FX1_ISP_ACCESS_RW

/**
 * TUNE_NR_LUT_ADDR: địa chỉ lệch 0x8124, dòng bảng 881, kiểu truy cập RW
 * Chỉ số lập trình 0..16, không tự tăng, thuộc bank profile đang chọn. Cập nhật qua cổng theo MAS
 * v1.07 tuning-noise.
 */
#define FX1_ISP_TUNE_NR_LUT_ADDR_OFFSET 0x8124U
#define REG_TUNE_NR_LUT_ADDR FX1_ISP_TUNE_NR_LUT_ADDR_OFFSET
#define FX1_ISP_TUNE_NR_LUT_ADDR_RESET 0x00000000U
#define FX1_ISP_TUNE_NR_LUT_ADDR_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_TUNE_NR_LUT_ADDR_RO_MASK 0x00000000U
#define FX1_ISP_TUNE_NR_LUT_ADDR_RW_MASK 0x0000001FU
#define FX1_ISP_TUNE_NR_LUT_ADDR_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_NR_LUT_ADDR_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_NR_LUT_ADDR_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_NR_LUT_ADDR_W1SC_MASK 0x00000000U
/**
 * VALUE [4:0], dòng bảng 882, kiểu truy cập RW
 * Chỉ số lập trình 0..16, không tự tăng, thuộc bank profile đang chọn.
 */
#define FX1_ISP_TUNE_NR_LUT_ADDR_VALUE_SHIFT 0U
#define FX1_ISP_TUNE_NR_LUT_ADDR_VALUE_WIDTH 5U
#define FX1_ISP_TUNE_NR_LUT_ADDR_VALUE_MASK 0x0000001FU
#define FX1_ISP_TUNE_NR_LUT_ADDR_VALUE_RESET 0x00000000U
#define FX1_ISP_TUNE_NR_LUT_ADDR_VALUE_ACCESS FX1_ISP_ACCESS_RW

/**
 * TUNE_NR_LUT_DATA: địa chỉ lệch 0x8128, dòng bảng 883, kiểu truy cập RW
 * Điểm phương sai, không phải sigma; dữ liệu không được khởi tạo bởi reset, đọc tại chỉ số chọn. Cập
 * nhật qua cổng, MAS v1.07 tuning-noise.
 */
#define FX1_ISP_TUNE_NR_LUT_DATA_OFFSET 0x8128U
#define REG_TUNE_NR_LUT_DATA FX1_ISP_TUNE_NR_LUT_DATA_OFFSET
#define FX1_ISP_TUNE_NR_LUT_DATA_RESET 0x00000000U
#define FX1_ISP_TUNE_NR_LUT_DATA_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_TUNE_NR_LUT_DATA_RO_MASK 0x00000000U
#define FX1_ISP_TUNE_NR_LUT_DATA_RW_MASK 0x0000FFFFU
#define FX1_ISP_TUNE_NR_LUT_DATA_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_NR_LUT_DATA_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_NR_LUT_DATA_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_NR_LUT_DATA_W1SC_MASK 0x00000000U
/**
 * VALUE [15:0], dòng bảng 884, kiểu truy cập RW
 * Điểm phương sai, không phải sigma; reset không bảo đảm nội dung, đọc lại theo chỉ số chọn.
 */
#define FX1_ISP_TUNE_NR_LUT_DATA_VALUE_SHIFT 0U
#define FX1_ISP_TUNE_NR_LUT_DATA_VALUE_WIDTH 16U
#define FX1_ISP_TUNE_NR_LUT_DATA_VALUE_MASK 0x0000FFFFU
#define FX1_ISP_TUNE_NR_LUT_DATA_VALUE_RESET 0x00000000U
#define FX1_ISP_TUNE_NR_LUT_DATA_VALUE_ACCESS FX1_ISP_ACCESS_RW

/**
 * TUNE_NR_SEED: địa chỉ lệch 0x812C, dòng bảng 885, kiểu truy cập RW
 * Phương sai tự động ban đầu sau reset/đổi profile; frame hoàn tất cùng profile tiếp theo thay thế.
 * Cập nhật theo profile, MAS v1.07 tuning-noise.
 */
#define FX1_ISP_TUNE_NR_SEED_OFFSET 0x812CU
#define REG_TUNE_NR_SEED FX1_ISP_TUNE_NR_SEED_OFFSET
#define FX1_ISP_TUNE_NR_SEED_RESET 0x00000000U
#define FX1_ISP_TUNE_NR_SEED_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_TUNE_NR_SEED_RO_MASK 0x00000000U
#define FX1_ISP_TUNE_NR_SEED_RW_MASK 0x0000FFFFU
#define FX1_ISP_TUNE_NR_SEED_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_NR_SEED_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_NR_SEED_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_NR_SEED_W1SC_MASK 0x00000000U
/**
 * VALUE [15:0], dòng bảng 886, kiểu truy cập RW
 * Phương sai tự động ban đầu sau reset/đổi profile; frame hoàn tất cùng profile thay thế về sau.
 */
#define FX1_ISP_TUNE_NR_SEED_VALUE_SHIFT 0U
#define FX1_ISP_TUNE_NR_SEED_VALUE_WIDTH 16U
#define FX1_ISP_TUNE_NR_SEED_VALUE_MASK 0x0000FFFFU
#define FX1_ISP_TUNE_NR_SEED_VALUE_RESET 0x00000000U
#define FX1_ISP_TUNE_NR_SEED_VALUE_ACCESS FX1_ISP_ACCESS_RW

/**
 * TUNE_WB_GB: địa chỉ lệch 0x8200, dòng bảng 887, kiểu truy cập RW
 * Gain Gb UQ4.8. Trong extension, WB_GAIN_G chỉ còn áp cho Gr. Cập nhật theo profile, MAS v1.07
 * tuning-wb.
 */
#define FX1_ISP_TUNE_WB_GB_OFFSET 0x8200U
#define REG_TUNE_WB_GB FX1_ISP_TUNE_WB_GB_OFFSET
#define FX1_ISP_TUNE_WB_GB_RESET 0x00000100U
#define FX1_ISP_TUNE_WB_GB_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_TUNE_WB_GB_RO_MASK 0x00000000U
#define FX1_ISP_TUNE_WB_GB_RW_MASK 0x00000FFFU
#define FX1_ISP_TUNE_WB_GB_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_WB_GB_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_WB_GB_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_WB_GB_W1SC_MASK 0x00000000U
/**
 * VALUE [11:0], dòng bảng 888, kiểu truy cập RW
 * Gain Gb UQ4.8; extension dùng WB_GAIN_G cho Gr riêng.
 */
#define FX1_ISP_TUNE_WB_GB_VALUE_SHIFT 0U
#define FX1_ISP_TUNE_WB_GB_VALUE_WIDTH 12U
#define FX1_ISP_TUNE_WB_GB_VALUE_MASK 0x00000FFFU
#define FX1_ISP_TUNE_WB_GB_VALUE_RESET 0x00000100U
#define FX1_ISP_TUNE_WB_GB_VALUE_ACCESS FX1_ISP_ACCESS_RW

/**
 * TUNE_DEMOSAIC_MARGIN: địa chỉ lệch 0x8204, dòng bảng 889, kiểu truy cập RW
 * Ngưỡng coi gradient gần bằng nhau, theo đơn vị gradient nguyên hiện tại; cập nhật theo profile,
 * MAS v1.07 tuning-demosaic.
 */
#define FX1_ISP_TUNE_DEMOSAIC_MARGIN_OFFSET 0x8204U
#define REG_TUNE_DEMOSAIC_MARGIN FX1_ISP_TUNE_DEMOSAIC_MARGIN_OFFSET
#define FX1_ISP_TUNE_DEMOSAIC_MARGIN_RESET 0x00000000U
#define FX1_ISP_TUNE_DEMOSAIC_MARGIN_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_TUNE_DEMOSAIC_MARGIN_RO_MASK 0x00000000U
#define FX1_ISP_TUNE_DEMOSAIC_MARGIN_RW_MASK 0xFFFFFFFFU
#define FX1_ISP_TUNE_DEMOSAIC_MARGIN_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_DEMOSAIC_MARGIN_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_DEMOSAIC_MARGIN_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_DEMOSAIC_MARGIN_W1SC_MASK 0x00000000U
/**
 * GREEN [15:0], dòng bảng 890, kiểu truy cập RW
 * Dùng nhánh gradient bằng nhau khi abs(deltaH-deltaV)<=GREEN.
 */
#define FX1_ISP_TUNE_DEMOSAIC_MARGIN_GREEN_SHIFT 0U
#define FX1_ISP_TUNE_DEMOSAIC_MARGIN_GREEN_WIDTH 16U
#define FX1_ISP_TUNE_DEMOSAIC_MARGIN_GREEN_MASK 0x0000FFFFU
#define FX1_ISP_TUNE_DEMOSAIC_MARGIN_GREEN_RESET 0x00000000U
#define FX1_ISP_TUNE_DEMOSAIC_MARGIN_GREEN_ACCESS FX1_ISP_ACCESS_RW
/**
 * RB [31:16], dòng bảng 891, kiểu truy cập RW
 * Dùng nhánh chéo gradient bằng nhau khi abs(DN-DP)<=RB.
 */
#define FX1_ISP_TUNE_DEMOSAIC_MARGIN_RB_SHIFT 16U
#define FX1_ISP_TUNE_DEMOSAIC_MARGIN_RB_WIDTH 16U
#define FX1_ISP_TUNE_DEMOSAIC_MARGIN_RB_MASK 0xFFFF0000U
#define FX1_ISP_TUNE_DEMOSAIC_MARGIN_RB_RESET 0x00000000U
#define FX1_ISP_TUNE_DEMOSAIC_MARGIN_RB_ACCESS FX1_ISP_ACCESS_RW

/**
 * TUNE_CNF_BLEND: địa chỉ lệch 0x8208, dòng bảng 892, kiểu truy cập RW
 * Trọng số chroma đã lọc 0..256: 0=ảnh gốc,256=kết quả CNF. Cập nhật theo profile, MAS v1.07
 * tuning-cnf.
 */
#define FX1_ISP_TUNE_CNF_BLEND_OFFSET 0x8208U
#define REG_TUNE_CNF_BLEND FX1_ISP_TUNE_CNF_BLEND_OFFSET
#define FX1_ISP_TUNE_CNF_BLEND_RESET 0x00000100U
#define FX1_ISP_TUNE_CNF_BLEND_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_TUNE_CNF_BLEND_RO_MASK 0x00000000U
#define FX1_ISP_TUNE_CNF_BLEND_RW_MASK 0x000001FFU
#define FX1_ISP_TUNE_CNF_BLEND_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_CNF_BLEND_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_CNF_BLEND_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_CNF_BLEND_W1SC_MASK 0x00000000U
/**
 * VALUE [8:0], dòng bảng 893, kiểu truy cập RW
 * Trọng số chroma đã lọc 0..256: 0=ảnh gốc,256=kết quả CNF.
 */
#define FX1_ISP_TUNE_CNF_BLEND_VALUE_SHIFT 0U
#define FX1_ISP_TUNE_CNF_BLEND_VALUE_WIDTH 9U
#define FX1_ISP_TUNE_CNF_BLEND_VALUE_MASK 0x000001FFU
#define FX1_ISP_TUNE_CNF_BLEND_VALUE_RESET 0x00000100U
#define FX1_ISP_TUNE_CNF_BLEND_VALUE_ACCESS FX1_ISP_ACCESS_RW

/**
 * TUNE_CSC_CTRL: địa chỉ lệch 0x8210, dòng bảng 894, kiểu truy cập RW
 * Điều khiển màu theo profile frame. Chọn hệ số dải trước lần làm tròn CSC cuối; điều chỉnh màu tùy
 * chọn đứng trước tap AF Y8 và xử lý YUV phía sau.
 */
#define FX1_ISP_TUNE_CSC_CTRL_OFFSET 0x8210U
#define REG_TUNE_CSC_CTRL FX1_ISP_TUNE_CSC_CTRL_OFFSET
#define FX1_ISP_TUNE_CSC_CTRL_RESET 0x00000002U
#define FX1_ISP_TUNE_CSC_CTRL_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_TUNE_CSC_CTRL_RO_MASK 0x00000000U
#define FX1_ISP_TUNE_CSC_CTRL_RW_MASK 0x00000003U
#define FX1_ISP_TUNE_CSC_CTRL_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_CSC_CTRL_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_CSC_CTRL_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_CSC_CTRL_W1SC_MASK 0x00000000U
/**
 * LOOK_EN [0:0], dòng bảng 895, kiểu truy cập RW
 * Bật điều chỉnh độ sáng/tương phản và ma trận chroma.
 */
#define FX1_ISP_TUNE_CSC_CTRL_LOOK_EN_SHIFT 0U
#define FX1_ISP_TUNE_CSC_CTRL_LOOK_EN_WIDTH 1U
#define FX1_ISP_TUNE_CSC_CTRL_LOOK_EN_MASK 0x00000001U
#define FX1_ISP_TUNE_CSC_CTRL_LOOK_EN_RESET 0x00000000U
#define FX1_ISP_TUNE_CSC_CTRL_LOOK_EN_ACCESS FX1_ISP_ACCESS_RW
/**
 * FULL_RANGE [1:1], dòng bảng 896, kiểu truy cập RW
 * 1: hệ số toàn dải, offset Y=0 mặc định. 0: hệ số giới hạn dải, offset Y=16. Chỉ lượng tử hóa một
 * lần sau phép nhân dải đủ rộng; độc lập CSC_CTRL.STD.
 */
#define FX1_ISP_TUNE_CSC_CTRL_FULL_RANGE_SHIFT 1U
#define FX1_ISP_TUNE_CSC_CTRL_FULL_RANGE_WIDTH 1U
#define FX1_ISP_TUNE_CSC_CTRL_FULL_RANGE_MASK 0x00000002U
#define FX1_ISP_TUNE_CSC_CTRL_FULL_RANGE_RESET 0x00000001U
#define FX1_ISP_TUNE_CSC_CTRL_FULL_RANGE_ACCESS FX1_ISP_ACCESS_RW

/**
 * TUNE_CSC_CONTRAST: địa chỉ lệch 0x8214, dòng bảng 897, kiểu truy cập RW
 * Gain luma không dấu Q4.8 quanh mức nền của dải được chọn; cập nhật theo profile, MAS v1.07
 * tuning-color.
 */
#define FX1_ISP_TUNE_CSC_CONTRAST_OFFSET 0x8214U
#define REG_TUNE_CSC_CONTRAST FX1_ISP_TUNE_CSC_CONTRAST_OFFSET
#define FX1_ISP_TUNE_CSC_CONTRAST_RESET 0x00000100U
#define FX1_ISP_TUNE_CSC_CONTRAST_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_TUNE_CSC_CONTRAST_RO_MASK 0x00000000U
#define FX1_ISP_TUNE_CSC_CONTRAST_RW_MASK 0x00000FFFU
#define FX1_ISP_TUNE_CSC_CONTRAST_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_CSC_CONTRAST_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_CSC_CONTRAST_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_CSC_CONTRAST_W1SC_MASK 0x00000000U
/**
 * VALUE [11:0], dòng bảng 898, kiểu truy cập RW
 * Gain luma không dấu Q4.8 quanh mức nền của dải được chọn.
 */
#define FX1_ISP_TUNE_CSC_CONTRAST_VALUE_SHIFT 0U
#define FX1_ISP_TUNE_CSC_CONTRAST_VALUE_WIDTH 12U
#define FX1_ISP_TUNE_CSC_CONTRAST_VALUE_MASK 0x00000FFFU
#define FX1_ISP_TUNE_CSC_CONTRAST_VALUE_RESET 0x00000100U
#define FX1_ISP_TUNE_CSC_CONTRAST_VALUE_ACCESS FX1_ISP_ACCESS_RW

/**
 * TUNE_CSC_BRIGHTNESS: địa chỉ lệch 0x8218, dòng bảng 899, kiểu truy cập RW
 * Offset luma có dấu bù hai 10 bit, đơn vị mã Y đầu ra; cập nhật theo profile, MAS v1.07
 * tuning-color.
 */
#define FX1_ISP_TUNE_CSC_BRIGHTNESS_OFFSET 0x8218U
#define REG_TUNE_CSC_BRIGHTNESS FX1_ISP_TUNE_CSC_BRIGHTNESS_OFFSET
#define FX1_ISP_TUNE_CSC_BRIGHTNESS_RESET 0x00000000U
#define FX1_ISP_TUNE_CSC_BRIGHTNESS_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_TUNE_CSC_BRIGHTNESS_RO_MASK 0x00000000U
#define FX1_ISP_TUNE_CSC_BRIGHTNESS_RW_MASK 0x000003FFU
#define FX1_ISP_TUNE_CSC_BRIGHTNESS_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_CSC_BRIGHTNESS_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_CSC_BRIGHTNESS_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_CSC_BRIGHTNESS_W1SC_MASK 0x00000000U
/**
 * VALUE [9:0], dòng bảng 900, kiểu truy cập RW
 * Offset luma có dấu bù hai 10 bit theo mã Y đầu ra.
 */
#define FX1_ISP_TUNE_CSC_BRIGHTNESS_VALUE_SHIFT 0U
#define FX1_ISP_TUNE_CSC_BRIGHTNESS_VALUE_WIDTH 10U
#define FX1_ISP_TUNE_CSC_BRIGHTNESS_VALUE_MASK 0x000003FFU
#define FX1_ISP_TUNE_CSC_BRIGHTNESS_VALUE_RESET 0x00000000U
#define FX1_ISP_TUNE_CSC_BRIGHTNESS_VALUE_ACCESS FX1_ISP_ACCESS_RW

/**
 * TUNE_CSC_UU: địa chỉ lệch 0x8220, dòng bảng 901, kiểu truy cập RW
 * Hệ số ma trận chroma có dấu Q4.14 quanh mức trung tính 128. Firmware tính hue/saturation; cập nhật
 * theo profile, MAS v1.07 tuning-color.
 */
#define FX1_ISP_TUNE_CSC_UU_OFFSET 0x8220U
#define REG_TUNE_CSC_UU FX1_ISP_TUNE_CSC_UU_OFFSET
#define FX1_ISP_TUNE_CSC_UU_RESET 0x00004000U
#define FX1_ISP_TUNE_CSC_UU_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_TUNE_CSC_UU_RO_MASK 0x00000000U
#define FX1_ISP_TUNE_CSC_UU_RW_MASK 0x0003FFFFU
#define FX1_ISP_TUNE_CSC_UU_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_CSC_UU_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_CSC_UU_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_CSC_UU_W1SC_MASK 0x00000000U
/**
 * VALUE [17:0], dòng bảng 902, kiểu truy cập RW
 * Hệ số ma trận chroma có dấu Q4.14 quanh 128; firmware tính sắc độ/độ bão hòa.
 */
#define FX1_ISP_TUNE_CSC_UU_VALUE_SHIFT 0U
#define FX1_ISP_TUNE_CSC_UU_VALUE_WIDTH 18U
#define FX1_ISP_TUNE_CSC_UU_VALUE_MASK 0x0003FFFFU
#define FX1_ISP_TUNE_CSC_UU_VALUE_RESET 0x00004000U
#define FX1_ISP_TUNE_CSC_UU_VALUE_ACCESS FX1_ISP_ACCESS_RW

/**
 * TUNE_CSC_UV: địa chỉ lệch 0x8224, dòng bảng 903, kiểu truy cập RW
 * Hệ số ma trận chroma có dấu Q4.14 quanh mức trung tính 128. Firmware tính hue/saturation; cập nhật
 * theo profile, MAS v1.07 tuning-color.
 */
#define FX1_ISP_TUNE_CSC_UV_OFFSET 0x8224U
#define REG_TUNE_CSC_UV FX1_ISP_TUNE_CSC_UV_OFFSET
#define FX1_ISP_TUNE_CSC_UV_RESET 0x00000000U
#define FX1_ISP_TUNE_CSC_UV_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_TUNE_CSC_UV_RO_MASK 0x00000000U
#define FX1_ISP_TUNE_CSC_UV_RW_MASK 0x0003FFFFU
#define FX1_ISP_TUNE_CSC_UV_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_CSC_UV_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_CSC_UV_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_CSC_UV_W1SC_MASK 0x00000000U
/**
 * VALUE [17:0], dòng bảng 904, kiểu truy cập RW
 * Hệ số ma trận chroma có dấu Q4.14 quanh 128; firmware tính sắc độ/độ bão hòa.
 */
#define FX1_ISP_TUNE_CSC_UV_VALUE_SHIFT 0U
#define FX1_ISP_TUNE_CSC_UV_VALUE_WIDTH 18U
#define FX1_ISP_TUNE_CSC_UV_VALUE_MASK 0x0003FFFFU
#define FX1_ISP_TUNE_CSC_UV_VALUE_RESET 0x00000000U
#define FX1_ISP_TUNE_CSC_UV_VALUE_ACCESS FX1_ISP_ACCESS_RW

/**
 * TUNE_CSC_VU: địa chỉ lệch 0x8228, dòng bảng 905, kiểu truy cập RW
 * Hệ số ma trận chroma có dấu Q4.14 quanh mức trung tính 128. Firmware tính hue/saturation; cập nhật
 * theo profile, MAS v1.07 tuning-color.
 */
#define FX1_ISP_TUNE_CSC_VU_OFFSET 0x8228U
#define REG_TUNE_CSC_VU FX1_ISP_TUNE_CSC_VU_OFFSET
#define FX1_ISP_TUNE_CSC_VU_RESET 0x00000000U
#define FX1_ISP_TUNE_CSC_VU_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_TUNE_CSC_VU_RO_MASK 0x00000000U
#define FX1_ISP_TUNE_CSC_VU_RW_MASK 0x0003FFFFU
#define FX1_ISP_TUNE_CSC_VU_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_CSC_VU_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_CSC_VU_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_CSC_VU_W1SC_MASK 0x00000000U
/**
 * VALUE [17:0], dòng bảng 906, kiểu truy cập RW
 * Hệ số ma trận chroma có dấu Q4.14 quanh 128; firmware tính sắc độ/độ bão hòa.
 */
#define FX1_ISP_TUNE_CSC_VU_VALUE_SHIFT 0U
#define FX1_ISP_TUNE_CSC_VU_VALUE_WIDTH 18U
#define FX1_ISP_TUNE_CSC_VU_VALUE_MASK 0x0003FFFFU
#define FX1_ISP_TUNE_CSC_VU_VALUE_RESET 0x00000000U
#define FX1_ISP_TUNE_CSC_VU_VALUE_ACCESS FX1_ISP_ACCESS_RW

/**
 * TUNE_CSC_VV: địa chỉ lệch 0x822C, dòng bảng 907, kiểu truy cập RW
 * Hệ số ma trận chroma có dấu Q4.14 quanh mức trung tính 128. Firmware tính hue/saturation; cập nhật
 * theo profile, MAS v1.07 tuning-color.
 */
#define FX1_ISP_TUNE_CSC_VV_OFFSET 0x822CU
#define REG_TUNE_CSC_VV FX1_ISP_TUNE_CSC_VV_OFFSET
#define FX1_ISP_TUNE_CSC_VV_RESET 0x00004000U
#define FX1_ISP_TUNE_CSC_VV_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_TUNE_CSC_VV_RO_MASK 0x00000000U
#define FX1_ISP_TUNE_CSC_VV_RW_MASK 0x0003FFFFU
#define FX1_ISP_TUNE_CSC_VV_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_CSC_VV_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_CSC_VV_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_CSC_VV_W1SC_MASK 0x00000000U
/**
 * VALUE [17:0], dòng bảng 908, kiểu truy cập RW
 * Hệ số ma trận chroma có dấu Q4.14 quanh 128; firmware tính sắc độ/độ bão hòa.
 */
#define FX1_ISP_TUNE_CSC_VV_VALUE_SHIFT 0U
#define FX1_ISP_TUNE_CSC_VV_VALUE_WIDTH 18U
#define FX1_ISP_TUNE_CSC_VV_VALUE_MASK 0x0003FFFFU
#define FX1_ISP_TUNE_CSC_VV_VALUE_RESET 0x00004000U
#define FX1_ISP_TUNE_CSC_VV_VALUE_ACCESS FX1_ISP_ACCESS_RW

/**
 * TUNE_AWB_CTRL: địa chỉ lệch 0x8300, dòng bảng 909, kiểu truy cập RW
 * Chọn ô Bayer sau BPC; kiểm tra clip nghiêm ngặt của cả bốn mẫu luôn áp trước. Cập nhật theo
 * profile, MAS v1.07 tuning-awb.
 */
#define FX1_ISP_TUNE_AWB_CTRL_OFFSET 0x8300U
#define REG_TUNE_AWB_CTRL FX1_ISP_TUNE_AWB_CTRL_OFFSET
#define FX1_ISP_TUNE_AWB_CTRL_RESET 0x00000000U
#define FX1_ISP_TUNE_AWB_CTRL_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_TUNE_AWB_CTRL_RO_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_CTRL_RW_MASK 0x0000FF03U
#define FX1_ISP_TUNE_AWB_CTRL_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_CTRL_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_CTRL_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_CTRL_W1SC_MASK 0x00000000U
/**
 * ROI_EN [0:0], dòng bảng 910, kiểu truy cập RW
 * ROI tính theo tọa độ ô Bayer đầy đủ.
 */
#define FX1_ISP_TUNE_AWB_CTRL_ROI_EN_SHIFT 0U
#define FX1_ISP_TUNE_AWB_CTRL_ROI_EN_WIDTH 1U
#define FX1_ISP_TUNE_AWB_CTRL_ROI_EN_MASK 0x00000001U
#define FX1_ISP_TUNE_AWB_CTRL_ROI_EN_RESET 0x00000000U
#define FX1_ISP_TUNE_AWB_CTRL_ROI_EN_ACCESS FX1_ISP_ACCESS_RW
/**
 * COLOR_EN [1:1], dòng bảng 911, kiểu truy cập RW
 * Áp hợp các ellipse được bật; mặt nạ 0 không nhận ô nào.
 */
#define FX1_ISP_TUNE_AWB_CTRL_COLOR_EN_SHIFT 1U
#define FX1_ISP_TUNE_AWB_CTRL_COLOR_EN_WIDTH 1U
#define FX1_ISP_TUNE_AWB_CTRL_COLOR_EN_MASK 0x00000002U
#define FX1_ISP_TUNE_AWB_CTRL_COLOR_EN_RESET 0x00000000U
#define FX1_ISP_TUNE_AWB_CTRL_COLOR_EN_ACCESS FX1_ISP_ACCESS_RW
/**
 * ELLIPSE_MASK [15:8], dòng bảng 912, kiểu truy cập RW
 * Tám vùng màu độc lập lấy hợp; mỗi ô chỉ đếm một lần.
 */
#define FX1_ISP_TUNE_AWB_CTRL_ELLIPSE_MASK_SHIFT 8U
#define FX1_ISP_TUNE_AWB_CTRL_ELLIPSE_MASK_WIDTH 8U
#define FX1_ISP_TUNE_AWB_CTRL_ELLIPSE_MASK_MASK 0x0000FF00U
#define FX1_ISP_TUNE_AWB_CTRL_ELLIPSE_MASK_RESET 0x00000000U
#define FX1_ISP_TUNE_AWB_CTRL_ELLIPSE_MASK_ACCESS FX1_ISP_ACCESS_RW

/**
 * TUNE_AWB_ROI_START: địa chỉ lệch 0x8304, dòng bảng 913, kiểu truy cập RW
 * Điểm bắt đầu gồm đầu mút theo tọa độ ô Bayer; cập nhật theo profile, MAS v1.07 tuning-awb.
 */
#define FX1_ISP_TUNE_AWB_ROI_START_OFFSET 0x8304U
#define REG_TUNE_AWB_ROI_START FX1_ISP_TUNE_AWB_ROI_START_OFFSET
#define FX1_ISP_TUNE_AWB_ROI_START_RESET 0x00000000U
#define FX1_ISP_TUNE_AWB_ROI_START_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_TUNE_AWB_ROI_START_RO_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_ROI_START_RW_MASK 0xFFFFFFFFU
#define FX1_ISP_TUNE_AWB_ROI_START_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_ROI_START_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_ROI_START_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_ROI_START_W1SC_MASK 0x00000000U
/**
 * X [15:0], dòng bảng 914, kiểu truy cập RW
 * Tọa độ ngang của đầu gồm đầu mút hoặc cuối không gồm đầu mút.
 */
#define FX1_ISP_TUNE_AWB_ROI_START_X_SHIFT 0U
#define FX1_ISP_TUNE_AWB_ROI_START_X_WIDTH 16U
#define FX1_ISP_TUNE_AWB_ROI_START_X_MASK 0x0000FFFFU
#define FX1_ISP_TUNE_AWB_ROI_START_X_RESET 0x00000000U
#define FX1_ISP_TUNE_AWB_ROI_START_X_ACCESS FX1_ISP_ACCESS_RW
/**
 * Y [31:16], dòng bảng 915, kiểu truy cập RW
 * Tọa độ dọc của đầu gồm đầu mút hoặc cuối không gồm đầu mút.
 */
#define FX1_ISP_TUNE_AWB_ROI_START_Y_SHIFT 16U
#define FX1_ISP_TUNE_AWB_ROI_START_Y_WIDTH 16U
#define FX1_ISP_TUNE_AWB_ROI_START_Y_MASK 0xFFFF0000U
#define FX1_ISP_TUNE_AWB_ROI_START_Y_RESET 0x00000000U
#define FX1_ISP_TUNE_AWB_ROI_START_Y_ACCESS FX1_ISP_ACCESS_RW

/**
 * TUNE_AWB_ROI_END: địa chỉ lệch 0x8308, dòng bảng 916, kiểu truy cập RW
 * Điểm kết thúc không gồm đầu mút theo ô Bayer; cập nhật theo profile, MAS v1.07 tuning-awb.
 */
#define FX1_ISP_TUNE_AWB_ROI_END_OFFSET 0x8308U
#define REG_TUNE_AWB_ROI_END FX1_ISP_TUNE_AWB_ROI_END_OFFSET
#define FX1_ISP_TUNE_AWB_ROI_END_RESET 0x00000000U
#define FX1_ISP_TUNE_AWB_ROI_END_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_TUNE_AWB_ROI_END_RO_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_ROI_END_RW_MASK 0xFFFFFFFFU
#define FX1_ISP_TUNE_AWB_ROI_END_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_ROI_END_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_ROI_END_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_ROI_END_W1SC_MASK 0x00000000U
/**
 * X [15:0], dòng bảng 917, kiểu truy cập RW
 * Tọa độ ngang của đầu gồm đầu mút hoặc cuối không gồm đầu mút.
 */
#define FX1_ISP_TUNE_AWB_ROI_END_X_SHIFT 0U
#define FX1_ISP_TUNE_AWB_ROI_END_X_WIDTH 16U
#define FX1_ISP_TUNE_AWB_ROI_END_X_MASK 0x0000FFFFU
#define FX1_ISP_TUNE_AWB_ROI_END_X_RESET 0x00000000U
#define FX1_ISP_TUNE_AWB_ROI_END_X_ACCESS FX1_ISP_ACCESS_RW
/**
 * Y [31:16], dòng bảng 918, kiểu truy cập RW
 * Tọa độ dọc của đầu gồm đầu mút hoặc cuối không gồm đầu mút.
 */
#define FX1_ISP_TUNE_AWB_ROI_END_Y_SHIFT 16U
#define FX1_ISP_TUNE_AWB_ROI_END_Y_WIDTH 16U
#define FX1_ISP_TUNE_AWB_ROI_END_Y_MASK 0xFFFF0000U
#define FX1_ISP_TUNE_AWB_ROI_END_Y_RESET 0x00000000U
#define FX1_ISP_TUNE_AWB_ROI_END_Y_ACCESS FX1_ISP_ACCESS_RW

/**
 * TUNE_AWB_R_LIMITS: địa chỉ lệch 0x8310, dòng bảng 919, kiểu truy cập RW
 * Giới hạn R bổ sung gồm hai đầu mút; G là trung bình Gr/Gb làm tròn nửa lên. Cập nhật theo profile,
 * MAS v1.07 tuning-awb.
 */
#define FX1_ISP_TUNE_AWB_R_LIMITS_OFFSET 0x8310U
#define REG_TUNE_AWB_R_LIMITS FX1_ISP_TUNE_AWB_R_LIMITS_OFFSET
#define FX1_ISP_TUNE_AWB_R_LIMITS_RESET 0x0FFF0000U
#define FX1_ISP_TUNE_AWB_R_LIMITS_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_TUNE_AWB_R_LIMITS_RO_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_R_LIMITS_RW_MASK 0x0FFF0FFFU
#define FX1_ISP_TUNE_AWB_R_LIMITS_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_R_LIMITS_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_R_LIMITS_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_R_LIMITS_W1SC_MASK 0x00000000U
/**
 * MIN [11:0], dòng bảng 920, kiểu truy cập RW
 * Cận dưới gồm đầu mút.
 */
#define FX1_ISP_TUNE_AWB_R_LIMITS_MIN_SHIFT 0U
#define FX1_ISP_TUNE_AWB_R_LIMITS_MIN_WIDTH 12U
#define FX1_ISP_TUNE_AWB_R_LIMITS_MIN_MASK 0x00000FFFU
#define FX1_ISP_TUNE_AWB_R_LIMITS_MIN_RESET 0x00000000U
#define FX1_ISP_TUNE_AWB_R_LIMITS_MIN_ACCESS FX1_ISP_ACCESS_RW
/**
 * MAX [27:16], dòng bảng 921, kiểu truy cập RW
 * Cận trên gồm đầu mút.
 */
#define FX1_ISP_TUNE_AWB_R_LIMITS_MAX_SHIFT 16U
#define FX1_ISP_TUNE_AWB_R_LIMITS_MAX_WIDTH 12U
#define FX1_ISP_TUNE_AWB_R_LIMITS_MAX_MASK 0x0FFF0000U
#define FX1_ISP_TUNE_AWB_R_LIMITS_MAX_RESET 0x00000FFFU
#define FX1_ISP_TUNE_AWB_R_LIMITS_MAX_ACCESS FX1_ISP_ACCESS_RW

/**
 * TUNE_AWB_G_LIMITS: địa chỉ lệch 0x8314, dòng bảng 922, kiểu truy cập RW
 * Giới hạn G bổ sung gồm hai đầu mút; G là trung bình Gr/Gb làm tròn nửa lên. Cập nhật theo profile,
 * MAS v1.07 tuning-awb.
 */
#define FX1_ISP_TUNE_AWB_G_LIMITS_OFFSET 0x8314U
#define REG_TUNE_AWB_G_LIMITS FX1_ISP_TUNE_AWB_G_LIMITS_OFFSET
#define FX1_ISP_TUNE_AWB_G_LIMITS_RESET 0x0FFF0000U
#define FX1_ISP_TUNE_AWB_G_LIMITS_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_TUNE_AWB_G_LIMITS_RO_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_G_LIMITS_RW_MASK 0x0FFF0FFFU
#define FX1_ISP_TUNE_AWB_G_LIMITS_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_G_LIMITS_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_G_LIMITS_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_G_LIMITS_W1SC_MASK 0x00000000U
/**
 * MIN [11:0], dòng bảng 923, kiểu truy cập RW
 * Cận dưới gồm đầu mút.
 */
#define FX1_ISP_TUNE_AWB_G_LIMITS_MIN_SHIFT 0U
#define FX1_ISP_TUNE_AWB_G_LIMITS_MIN_WIDTH 12U
#define FX1_ISP_TUNE_AWB_G_LIMITS_MIN_MASK 0x00000FFFU
#define FX1_ISP_TUNE_AWB_G_LIMITS_MIN_RESET 0x00000000U
#define FX1_ISP_TUNE_AWB_G_LIMITS_MIN_ACCESS FX1_ISP_ACCESS_RW
/**
 * MAX [27:16], dòng bảng 924, kiểu truy cập RW
 * Cận trên gồm đầu mút.
 */
#define FX1_ISP_TUNE_AWB_G_LIMITS_MAX_SHIFT 16U
#define FX1_ISP_TUNE_AWB_G_LIMITS_MAX_WIDTH 12U
#define FX1_ISP_TUNE_AWB_G_LIMITS_MAX_MASK 0x0FFF0000U
#define FX1_ISP_TUNE_AWB_G_LIMITS_MAX_RESET 0x00000FFFU
#define FX1_ISP_TUNE_AWB_G_LIMITS_MAX_ACCESS FX1_ISP_ACCESS_RW

/**
 * TUNE_AWB_B_LIMITS: địa chỉ lệch 0x8318, dòng bảng 925, kiểu truy cập RW
 * Giới hạn B bổ sung gồm hai đầu mút; G là trung bình Gr/Gb làm tròn nửa lên. Cập nhật theo profile,
 * MAS v1.07 tuning-awb.
 */
#define FX1_ISP_TUNE_AWB_B_LIMITS_OFFSET 0x8318U
#define REG_TUNE_AWB_B_LIMITS FX1_ISP_TUNE_AWB_B_LIMITS_OFFSET
#define FX1_ISP_TUNE_AWB_B_LIMITS_RESET 0x0FFF0000U
#define FX1_ISP_TUNE_AWB_B_LIMITS_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_TUNE_AWB_B_LIMITS_RO_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_B_LIMITS_RW_MASK 0x0FFF0FFFU
#define FX1_ISP_TUNE_AWB_B_LIMITS_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_B_LIMITS_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_B_LIMITS_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_B_LIMITS_W1SC_MASK 0x00000000U
/**
 * MIN [11:0], dòng bảng 926, kiểu truy cập RW
 * Cận dưới gồm đầu mút.
 */
#define FX1_ISP_TUNE_AWB_B_LIMITS_MIN_SHIFT 0U
#define FX1_ISP_TUNE_AWB_B_LIMITS_MIN_WIDTH 12U
#define FX1_ISP_TUNE_AWB_B_LIMITS_MIN_MASK 0x00000FFFU
#define FX1_ISP_TUNE_AWB_B_LIMITS_MIN_RESET 0x00000000U
#define FX1_ISP_TUNE_AWB_B_LIMITS_MIN_ACCESS FX1_ISP_ACCESS_RW
/**
 * MAX [27:16], dòng bảng 927, kiểu truy cập RW
 * Cận trên gồm đầu mút.
 */
#define FX1_ISP_TUNE_AWB_B_LIMITS_MAX_SHIFT 16U
#define FX1_ISP_TUNE_AWB_B_LIMITS_MAX_WIDTH 12U
#define FX1_ISP_TUNE_AWB_B_LIMITS_MAX_MASK 0x0FFF0000U
#define FX1_ISP_TUNE_AWB_B_LIMITS_MAX_RESET 0x00000FFFU
#define FX1_ISP_TUNE_AWB_B_LIMITS_MAX_ACCESS FX1_ISP_ACCESS_RW

/**
 * TUNE_AWB_MIN_G: địa chỉ lệch 0x831C, dòng bảng 928, kiểu truy cập RW
 * G tối thiểu để chia tỷ số màu, chỉ áp khi COLOR_EN. SEAL từ chối 0 khi COLOR_EN=1 bằng INVALID.
 * Cập nhật theo profile, MAS v1.07 tuning-awb.
 */
#define FX1_ISP_TUNE_AWB_MIN_G_OFFSET 0x831CU
#define REG_TUNE_AWB_MIN_G FX1_ISP_TUNE_AWB_MIN_G_OFFSET
#define FX1_ISP_TUNE_AWB_MIN_G_RESET 0x00000001U
#define FX1_ISP_TUNE_AWB_MIN_G_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_TUNE_AWB_MIN_G_RO_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_MIN_G_RW_MASK 0x00000FFFU
#define FX1_ISP_TUNE_AWB_MIN_G_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_MIN_G_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_MIN_G_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_MIN_G_W1SC_MASK 0x00000000U
/**
 * VALUE [11:0], dòng bảng 929, kiểu truy cập RW
 * G tối thiểu để chia tỷ số màu, chỉ áp khi COLOR_EN; SEAL từ chối 0 khi bật bằng INVALID.
 */
#define FX1_ISP_TUNE_AWB_MIN_G_VALUE_SHIFT 0U
#define FX1_ISP_TUNE_AWB_MIN_G_VALUE_WIDTH 12U
#define FX1_ISP_TUNE_AWB_MIN_G_VALUE_MASK 0x00000FFFU
#define FX1_ISP_TUNE_AWB_MIN_G_VALUE_RESET 0x00000001U
#define FX1_ISP_TUNE_AWB_MIN_G_VALUE_ACCESS FX1_ISP_ACCESS_RW

/**
 * TUNE_AWB_E0_CENTER: địa chỉ lệch 0x8340, dòng bảng 930, kiểu truy cập RW
 * Tâm ellipse theo R/G và B/G không dấu Q8.8; cập nhật theo profile, MAS v1.07 tuning-awb.
 */
#define FX1_ISP_TUNE_AWB_E0_CENTER_OFFSET 0x8340U
#define REG_TUNE_AWB_E0_CENTER FX1_ISP_TUNE_AWB_E0_CENTER_OFFSET
#define FX1_ISP_TUNE_AWB_E0_CENTER_RESET 0x01000100U
#define FX1_ISP_TUNE_AWB_E0_CENTER_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_TUNE_AWB_E0_CENTER_RO_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_E0_CENTER_RW_MASK 0xFFFFFFFFU
#define FX1_ISP_TUNE_AWB_E0_CENTER_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_E0_CENTER_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_E0_CENTER_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_E0_CENTER_W1SC_MASK 0x00000000U
/**
 * RG [15:0], dòng bảng 931, kiểu truy cập RW
 * Tọa độ tâm theo tỷ số R/G.
 */
#define FX1_ISP_TUNE_AWB_E0_CENTER_RG_SHIFT 0U
#define FX1_ISP_TUNE_AWB_E0_CENTER_RG_WIDTH 16U
#define FX1_ISP_TUNE_AWB_E0_CENTER_RG_MASK 0x0000FFFFU
#define FX1_ISP_TUNE_AWB_E0_CENTER_RG_RESET 0x00000100U
#define FX1_ISP_TUNE_AWB_E0_CENTER_RG_ACCESS FX1_ISP_ACCESS_RW
/**
 * BG [31:16], dòng bảng 932, kiểu truy cập RW
 * Tọa độ tâm theo tỷ số B/G.
 */
#define FX1_ISP_TUNE_AWB_E0_CENTER_BG_SHIFT 16U
#define FX1_ISP_TUNE_AWB_E0_CENTER_BG_WIDTH 16U
#define FX1_ISP_TUNE_AWB_E0_CENTER_BG_MASK 0xFFFF0000U
#define FX1_ISP_TUNE_AWB_E0_CENTER_BG_RESET 0x00000100U
#define FX1_ISP_TUNE_AWB_E0_CENTER_BG_ACCESS FX1_ISP_ACCESS_RW

/**
 * TUNE_AWB_E0_A: địa chỉ lệch 0x8344, dòng bảng 933, kiểu truy cập RW
 * Hệ số đường chéo không dấu Q2.14 phải dương; cập nhật theo profile, MAS v1.07 tuning-awb.
 */
#define FX1_ISP_TUNE_AWB_E0_A_OFFSET 0x8344U
#define REG_TUNE_AWB_E0_A FX1_ISP_TUNE_AWB_E0_A_OFFSET
#define FX1_ISP_TUNE_AWB_E0_A_RESET 0x00004000U
#define FX1_ISP_TUNE_AWB_E0_A_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_TUNE_AWB_E0_A_RO_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_E0_A_RW_MASK 0x0000FFFFU
#define FX1_ISP_TUNE_AWB_E0_A_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_E0_A_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_E0_A_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_E0_A_W1SC_MASK 0x00000000U
/**
 * VALUE [15:0], dòng bảng 934, kiểu truy cập RW
 * Hệ số đường chéo không dấu Q2.14 phải dương.
 */
#define FX1_ISP_TUNE_AWB_E0_A_VALUE_SHIFT 0U
#define FX1_ISP_TUNE_AWB_E0_A_VALUE_WIDTH 16U
#define FX1_ISP_TUNE_AWB_E0_A_VALUE_MASK 0x0000FFFFU
#define FX1_ISP_TUNE_AWB_E0_A_VALUE_RESET 0x00004000U
#define FX1_ISP_TUNE_AWB_E0_A_VALUE_ACCESS FX1_ISP_ACCESS_RW

/**
 * TUNE_AWB_E0_B: địa chỉ lệch 0x8348, dòng bảng 935, kiểu truy cập RW
 * Hệ số chéo có dấu Q1.14; cập nhật theo profile, MAS v1.07 tuning-awb.
 */
#define FX1_ISP_TUNE_AWB_E0_B_OFFSET 0x8348U
#define REG_TUNE_AWB_E0_B FX1_ISP_TUNE_AWB_E0_B_OFFSET
#define FX1_ISP_TUNE_AWB_E0_B_RESET 0x00000000U
#define FX1_ISP_TUNE_AWB_E0_B_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_TUNE_AWB_E0_B_RO_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_E0_B_RW_MASK 0x0000FFFFU
#define FX1_ISP_TUNE_AWB_E0_B_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_E0_B_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_E0_B_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_E0_B_W1SC_MASK 0x00000000U
/**
 * VALUE [15:0], dòng bảng 936, kiểu truy cập RW
 * Hệ số chéo có dấu Q1.14.
 */
#define FX1_ISP_TUNE_AWB_E0_B_VALUE_SHIFT 0U
#define FX1_ISP_TUNE_AWB_E0_B_VALUE_WIDTH 16U
#define FX1_ISP_TUNE_AWB_E0_B_VALUE_MASK 0x0000FFFFU
#define FX1_ISP_TUNE_AWB_E0_B_VALUE_RESET 0x00000000U
#define FX1_ISP_TUNE_AWB_E0_B_VALUE_ACCESS FX1_ISP_ACCESS_RW

/**
 * TUNE_AWB_E0_C: địa chỉ lệch 0x834C, dòng bảng 937, kiểu truy cập RW
 * Hệ số đường chéo không dấu Q2.14 phải dương; cập nhật theo profile, MAS v1.07 tuning-awb.
 */
#define FX1_ISP_TUNE_AWB_E0_C_OFFSET 0x834CU
#define REG_TUNE_AWB_E0_C FX1_ISP_TUNE_AWB_E0_C_OFFSET
#define FX1_ISP_TUNE_AWB_E0_C_RESET 0x00004000U
#define FX1_ISP_TUNE_AWB_E0_C_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_TUNE_AWB_E0_C_RO_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_E0_C_RW_MASK 0x0000FFFFU
#define FX1_ISP_TUNE_AWB_E0_C_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_E0_C_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_E0_C_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_E0_C_W1SC_MASK 0x00000000U
/**
 * VALUE [15:0], dòng bảng 938, kiểu truy cập RW
 * Hệ số đường chéo không dấu Q2.14 phải dương.
 */
#define FX1_ISP_TUNE_AWB_E0_C_VALUE_SHIFT 0U
#define FX1_ISP_TUNE_AWB_E0_C_VALUE_WIDTH 16U
#define FX1_ISP_TUNE_AWB_E0_C_VALUE_MASK 0x0000FFFFU
#define FX1_ISP_TUNE_AWB_E0_C_VALUE_RESET 0x00004000U
#define FX1_ISP_TUNE_AWB_E0_C_VALUE_ACCESS FX1_ISP_ACCESS_RW

/**
 * TUNE_AWB_E0_RADIUS2: địa chỉ lệch 0x8350, dòng bảng 939, kiểu truy cập RW
 * Bình phương bán kính không dấu Q16.16; so metric <= RADIUS2 << 14. Cập nhật theo profile, MAS
 * v1.07 tuning-awb.
 */
#define FX1_ISP_TUNE_AWB_E0_RADIUS2_OFFSET 0x8350U
#define REG_TUNE_AWB_E0_RADIUS2 FX1_ISP_TUNE_AWB_E0_RADIUS2_OFFSET
#define FX1_ISP_TUNE_AWB_E0_RADIUS2_RESET 0x00010000U
#define FX1_ISP_TUNE_AWB_E0_RADIUS2_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_TUNE_AWB_E0_RADIUS2_RO_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_E0_RADIUS2_RW_MASK 0xFFFFFFFFU
#define FX1_ISP_TUNE_AWB_E0_RADIUS2_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_E0_RADIUS2_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_E0_RADIUS2_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_E0_RADIUS2_W1SC_MASK 0x00000000U
/**
 * VALUE [31:0], dòng bảng 940, kiểu truy cập RW
 * Bình phương bán kính không dấu Q16.16; so metric <= RADIUS2 << 14.
 */
#define FX1_ISP_TUNE_AWB_E0_RADIUS2_VALUE_SHIFT 0U
#define FX1_ISP_TUNE_AWB_E0_RADIUS2_VALUE_WIDTH 32U
#define FX1_ISP_TUNE_AWB_E0_RADIUS2_VALUE_MASK 0xFFFFFFFFU
#define FX1_ISP_TUNE_AWB_E0_RADIUS2_VALUE_RESET 0x00010000U
#define FX1_ISP_TUNE_AWB_E0_RADIUS2_VALUE_ACCESS FX1_ISP_ACCESS_RW

/**
 * TUNE_AWB_E1_CENTER: địa chỉ lệch 0x8360, dòng bảng 941, kiểu truy cập RW
 * Tâm ellipse theo R/G và B/G không dấu Q8.8; cập nhật theo profile, MAS v1.07 tuning-awb.
 */
#define FX1_ISP_TUNE_AWB_E1_CENTER_OFFSET 0x8360U
#define REG_TUNE_AWB_E1_CENTER FX1_ISP_TUNE_AWB_E1_CENTER_OFFSET
#define FX1_ISP_TUNE_AWB_E1_CENTER_RESET 0x01000100U
#define FX1_ISP_TUNE_AWB_E1_CENTER_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_TUNE_AWB_E1_CENTER_RO_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_E1_CENTER_RW_MASK 0xFFFFFFFFU
#define FX1_ISP_TUNE_AWB_E1_CENTER_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_E1_CENTER_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_E1_CENTER_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_E1_CENTER_W1SC_MASK 0x00000000U
/**
 * RG [15:0], dòng bảng 942, kiểu truy cập RW
 * Tọa độ tâm theo tỷ số R/G.
 */
#define FX1_ISP_TUNE_AWB_E1_CENTER_RG_SHIFT 0U
#define FX1_ISP_TUNE_AWB_E1_CENTER_RG_WIDTH 16U
#define FX1_ISP_TUNE_AWB_E1_CENTER_RG_MASK 0x0000FFFFU
#define FX1_ISP_TUNE_AWB_E1_CENTER_RG_RESET 0x00000100U
#define FX1_ISP_TUNE_AWB_E1_CENTER_RG_ACCESS FX1_ISP_ACCESS_RW
/**
 * BG [31:16], dòng bảng 943, kiểu truy cập RW
 * Tọa độ tâm theo tỷ số B/G.
 */
#define FX1_ISP_TUNE_AWB_E1_CENTER_BG_SHIFT 16U
#define FX1_ISP_TUNE_AWB_E1_CENTER_BG_WIDTH 16U
#define FX1_ISP_TUNE_AWB_E1_CENTER_BG_MASK 0xFFFF0000U
#define FX1_ISP_TUNE_AWB_E1_CENTER_BG_RESET 0x00000100U
#define FX1_ISP_TUNE_AWB_E1_CENTER_BG_ACCESS FX1_ISP_ACCESS_RW

/**
 * TUNE_AWB_E1_A: địa chỉ lệch 0x8364, dòng bảng 944, kiểu truy cập RW
 * Hệ số đường chéo không dấu Q2.14 phải dương; cập nhật theo profile, MAS v1.07 tuning-awb.
 */
#define FX1_ISP_TUNE_AWB_E1_A_OFFSET 0x8364U
#define REG_TUNE_AWB_E1_A FX1_ISP_TUNE_AWB_E1_A_OFFSET
#define FX1_ISP_TUNE_AWB_E1_A_RESET 0x00004000U
#define FX1_ISP_TUNE_AWB_E1_A_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_TUNE_AWB_E1_A_RO_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_E1_A_RW_MASK 0x0000FFFFU
#define FX1_ISP_TUNE_AWB_E1_A_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_E1_A_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_E1_A_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_E1_A_W1SC_MASK 0x00000000U
/**
 * VALUE [15:0], dòng bảng 945, kiểu truy cập RW
 * Hệ số đường chéo không dấu Q2.14 phải dương.
 */
#define FX1_ISP_TUNE_AWB_E1_A_VALUE_SHIFT 0U
#define FX1_ISP_TUNE_AWB_E1_A_VALUE_WIDTH 16U
#define FX1_ISP_TUNE_AWB_E1_A_VALUE_MASK 0x0000FFFFU
#define FX1_ISP_TUNE_AWB_E1_A_VALUE_RESET 0x00004000U
#define FX1_ISP_TUNE_AWB_E1_A_VALUE_ACCESS FX1_ISP_ACCESS_RW

/**
 * TUNE_AWB_E1_B: địa chỉ lệch 0x8368, dòng bảng 946, kiểu truy cập RW
 * Hệ số chéo có dấu Q1.14; cập nhật theo profile, MAS v1.07 tuning-awb.
 */
#define FX1_ISP_TUNE_AWB_E1_B_OFFSET 0x8368U
#define REG_TUNE_AWB_E1_B FX1_ISP_TUNE_AWB_E1_B_OFFSET
#define FX1_ISP_TUNE_AWB_E1_B_RESET 0x00000000U
#define FX1_ISP_TUNE_AWB_E1_B_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_TUNE_AWB_E1_B_RO_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_E1_B_RW_MASK 0x0000FFFFU
#define FX1_ISP_TUNE_AWB_E1_B_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_E1_B_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_E1_B_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_E1_B_W1SC_MASK 0x00000000U
/**
 * VALUE [15:0], dòng bảng 947, kiểu truy cập RW
 * Hệ số chéo có dấu Q1.14.
 */
#define FX1_ISP_TUNE_AWB_E1_B_VALUE_SHIFT 0U
#define FX1_ISP_TUNE_AWB_E1_B_VALUE_WIDTH 16U
#define FX1_ISP_TUNE_AWB_E1_B_VALUE_MASK 0x0000FFFFU
#define FX1_ISP_TUNE_AWB_E1_B_VALUE_RESET 0x00000000U
#define FX1_ISP_TUNE_AWB_E1_B_VALUE_ACCESS FX1_ISP_ACCESS_RW

/**
 * TUNE_AWB_E1_C: địa chỉ lệch 0x836C, dòng bảng 948, kiểu truy cập RW
 * Hệ số đường chéo không dấu Q2.14 phải dương; cập nhật theo profile, MAS v1.07 tuning-awb.
 */
#define FX1_ISP_TUNE_AWB_E1_C_OFFSET 0x836CU
#define REG_TUNE_AWB_E1_C FX1_ISP_TUNE_AWB_E1_C_OFFSET
#define FX1_ISP_TUNE_AWB_E1_C_RESET 0x00004000U
#define FX1_ISP_TUNE_AWB_E1_C_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_TUNE_AWB_E1_C_RO_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_E1_C_RW_MASK 0x0000FFFFU
#define FX1_ISP_TUNE_AWB_E1_C_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_E1_C_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_E1_C_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_E1_C_W1SC_MASK 0x00000000U
/**
 * VALUE [15:0], dòng bảng 949, kiểu truy cập RW
 * Hệ số đường chéo không dấu Q2.14 phải dương.
 */
#define FX1_ISP_TUNE_AWB_E1_C_VALUE_SHIFT 0U
#define FX1_ISP_TUNE_AWB_E1_C_VALUE_WIDTH 16U
#define FX1_ISP_TUNE_AWB_E1_C_VALUE_MASK 0x0000FFFFU
#define FX1_ISP_TUNE_AWB_E1_C_VALUE_RESET 0x00004000U
#define FX1_ISP_TUNE_AWB_E1_C_VALUE_ACCESS FX1_ISP_ACCESS_RW

/**
 * TUNE_AWB_E1_RADIUS2: địa chỉ lệch 0x8370, dòng bảng 950, kiểu truy cập RW
 * Bình phương bán kính không dấu Q16.16; so metric <= RADIUS2 << 14. Cập nhật theo profile, MAS
 * v1.07 tuning-awb.
 */
#define FX1_ISP_TUNE_AWB_E1_RADIUS2_OFFSET 0x8370U
#define REG_TUNE_AWB_E1_RADIUS2 FX1_ISP_TUNE_AWB_E1_RADIUS2_OFFSET
#define FX1_ISP_TUNE_AWB_E1_RADIUS2_RESET 0x00010000U
#define FX1_ISP_TUNE_AWB_E1_RADIUS2_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_TUNE_AWB_E1_RADIUS2_RO_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_E1_RADIUS2_RW_MASK 0xFFFFFFFFU
#define FX1_ISP_TUNE_AWB_E1_RADIUS2_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_E1_RADIUS2_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_E1_RADIUS2_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_E1_RADIUS2_W1SC_MASK 0x00000000U
/**
 * VALUE [31:0], dòng bảng 951, kiểu truy cập RW
 * Bình phương bán kính không dấu Q16.16; so metric <= RADIUS2 << 14.
 */
#define FX1_ISP_TUNE_AWB_E1_RADIUS2_VALUE_SHIFT 0U
#define FX1_ISP_TUNE_AWB_E1_RADIUS2_VALUE_WIDTH 32U
#define FX1_ISP_TUNE_AWB_E1_RADIUS2_VALUE_MASK 0xFFFFFFFFU
#define FX1_ISP_TUNE_AWB_E1_RADIUS2_VALUE_RESET 0x00010000U
#define FX1_ISP_TUNE_AWB_E1_RADIUS2_VALUE_ACCESS FX1_ISP_ACCESS_RW

/**
 * TUNE_AWB_E2_CENTER: địa chỉ lệch 0x8380, dòng bảng 952, kiểu truy cập RW
 * Tâm ellipse theo R/G và B/G không dấu Q8.8; cập nhật theo profile, MAS v1.07 tuning-awb.
 */
#define FX1_ISP_TUNE_AWB_E2_CENTER_OFFSET 0x8380U
#define REG_TUNE_AWB_E2_CENTER FX1_ISP_TUNE_AWB_E2_CENTER_OFFSET
#define FX1_ISP_TUNE_AWB_E2_CENTER_RESET 0x01000100U
#define FX1_ISP_TUNE_AWB_E2_CENTER_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_TUNE_AWB_E2_CENTER_RO_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_E2_CENTER_RW_MASK 0xFFFFFFFFU
#define FX1_ISP_TUNE_AWB_E2_CENTER_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_E2_CENTER_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_E2_CENTER_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_E2_CENTER_W1SC_MASK 0x00000000U
/**
 * RG [15:0], dòng bảng 953, kiểu truy cập RW
 * Tọa độ tâm theo tỷ số R/G.
 */
#define FX1_ISP_TUNE_AWB_E2_CENTER_RG_SHIFT 0U
#define FX1_ISP_TUNE_AWB_E2_CENTER_RG_WIDTH 16U
#define FX1_ISP_TUNE_AWB_E2_CENTER_RG_MASK 0x0000FFFFU
#define FX1_ISP_TUNE_AWB_E2_CENTER_RG_RESET 0x00000100U
#define FX1_ISP_TUNE_AWB_E2_CENTER_RG_ACCESS FX1_ISP_ACCESS_RW
/**
 * BG [31:16], dòng bảng 954, kiểu truy cập RW
 * Tọa độ tâm theo tỷ số B/G.
 */
#define FX1_ISP_TUNE_AWB_E2_CENTER_BG_SHIFT 16U
#define FX1_ISP_TUNE_AWB_E2_CENTER_BG_WIDTH 16U
#define FX1_ISP_TUNE_AWB_E2_CENTER_BG_MASK 0xFFFF0000U
#define FX1_ISP_TUNE_AWB_E2_CENTER_BG_RESET 0x00000100U
#define FX1_ISP_TUNE_AWB_E2_CENTER_BG_ACCESS FX1_ISP_ACCESS_RW

/**
 * TUNE_AWB_E2_A: địa chỉ lệch 0x8384, dòng bảng 955, kiểu truy cập RW
 * Hệ số đường chéo không dấu Q2.14 phải dương; cập nhật theo profile, MAS v1.07 tuning-awb.
 */
#define FX1_ISP_TUNE_AWB_E2_A_OFFSET 0x8384U
#define REG_TUNE_AWB_E2_A FX1_ISP_TUNE_AWB_E2_A_OFFSET
#define FX1_ISP_TUNE_AWB_E2_A_RESET 0x00004000U
#define FX1_ISP_TUNE_AWB_E2_A_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_TUNE_AWB_E2_A_RO_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_E2_A_RW_MASK 0x0000FFFFU
#define FX1_ISP_TUNE_AWB_E2_A_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_E2_A_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_E2_A_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_E2_A_W1SC_MASK 0x00000000U
/**
 * VALUE [15:0], dòng bảng 956, kiểu truy cập RW
 * Hệ số đường chéo không dấu Q2.14 phải dương.
 */
#define FX1_ISP_TUNE_AWB_E2_A_VALUE_SHIFT 0U
#define FX1_ISP_TUNE_AWB_E2_A_VALUE_WIDTH 16U
#define FX1_ISP_TUNE_AWB_E2_A_VALUE_MASK 0x0000FFFFU
#define FX1_ISP_TUNE_AWB_E2_A_VALUE_RESET 0x00004000U
#define FX1_ISP_TUNE_AWB_E2_A_VALUE_ACCESS FX1_ISP_ACCESS_RW

/**
 * TUNE_AWB_E2_B: địa chỉ lệch 0x8388, dòng bảng 957, kiểu truy cập RW
 * Hệ số chéo có dấu Q1.14; cập nhật theo profile, MAS v1.07 tuning-awb.
 */
#define FX1_ISP_TUNE_AWB_E2_B_OFFSET 0x8388U
#define REG_TUNE_AWB_E2_B FX1_ISP_TUNE_AWB_E2_B_OFFSET
#define FX1_ISP_TUNE_AWB_E2_B_RESET 0x00000000U
#define FX1_ISP_TUNE_AWB_E2_B_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_TUNE_AWB_E2_B_RO_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_E2_B_RW_MASK 0x0000FFFFU
#define FX1_ISP_TUNE_AWB_E2_B_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_E2_B_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_E2_B_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_E2_B_W1SC_MASK 0x00000000U
/**
 * VALUE [15:0], dòng bảng 958, kiểu truy cập RW
 * Hệ số chéo có dấu Q1.14.
 */
#define FX1_ISP_TUNE_AWB_E2_B_VALUE_SHIFT 0U
#define FX1_ISP_TUNE_AWB_E2_B_VALUE_WIDTH 16U
#define FX1_ISP_TUNE_AWB_E2_B_VALUE_MASK 0x0000FFFFU
#define FX1_ISP_TUNE_AWB_E2_B_VALUE_RESET 0x00000000U
#define FX1_ISP_TUNE_AWB_E2_B_VALUE_ACCESS FX1_ISP_ACCESS_RW

/**
 * TUNE_AWB_E2_C: địa chỉ lệch 0x838C, dòng bảng 959, kiểu truy cập RW
 * Hệ số đường chéo không dấu Q2.14 phải dương; cập nhật theo profile, MAS v1.07 tuning-awb.
 */
#define FX1_ISP_TUNE_AWB_E2_C_OFFSET 0x838CU
#define REG_TUNE_AWB_E2_C FX1_ISP_TUNE_AWB_E2_C_OFFSET
#define FX1_ISP_TUNE_AWB_E2_C_RESET 0x00004000U
#define FX1_ISP_TUNE_AWB_E2_C_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_TUNE_AWB_E2_C_RO_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_E2_C_RW_MASK 0x0000FFFFU
#define FX1_ISP_TUNE_AWB_E2_C_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_E2_C_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_E2_C_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_E2_C_W1SC_MASK 0x00000000U
/**
 * VALUE [15:0], dòng bảng 960, kiểu truy cập RW
 * Hệ số đường chéo không dấu Q2.14 phải dương.
 */
#define FX1_ISP_TUNE_AWB_E2_C_VALUE_SHIFT 0U
#define FX1_ISP_TUNE_AWB_E2_C_VALUE_WIDTH 16U
#define FX1_ISP_TUNE_AWB_E2_C_VALUE_MASK 0x0000FFFFU
#define FX1_ISP_TUNE_AWB_E2_C_VALUE_RESET 0x00004000U
#define FX1_ISP_TUNE_AWB_E2_C_VALUE_ACCESS FX1_ISP_ACCESS_RW

/**
 * TUNE_AWB_E2_RADIUS2: địa chỉ lệch 0x8390, dòng bảng 961, kiểu truy cập RW
 * Bình phương bán kính không dấu Q16.16; so metric <= RADIUS2 << 14. Cập nhật theo profile, MAS
 * v1.07 tuning-awb.
 */
#define FX1_ISP_TUNE_AWB_E2_RADIUS2_OFFSET 0x8390U
#define REG_TUNE_AWB_E2_RADIUS2 FX1_ISP_TUNE_AWB_E2_RADIUS2_OFFSET
#define FX1_ISP_TUNE_AWB_E2_RADIUS2_RESET 0x00010000U
#define FX1_ISP_TUNE_AWB_E2_RADIUS2_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_TUNE_AWB_E2_RADIUS2_RO_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_E2_RADIUS2_RW_MASK 0xFFFFFFFFU
#define FX1_ISP_TUNE_AWB_E2_RADIUS2_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_E2_RADIUS2_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_E2_RADIUS2_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_E2_RADIUS2_W1SC_MASK 0x00000000U
/**
 * VALUE [31:0], dòng bảng 962, kiểu truy cập RW
 * Bình phương bán kính không dấu Q16.16; so metric <= RADIUS2 << 14.
 */
#define FX1_ISP_TUNE_AWB_E2_RADIUS2_VALUE_SHIFT 0U
#define FX1_ISP_TUNE_AWB_E2_RADIUS2_VALUE_WIDTH 32U
#define FX1_ISP_TUNE_AWB_E2_RADIUS2_VALUE_MASK 0xFFFFFFFFU
#define FX1_ISP_TUNE_AWB_E2_RADIUS2_VALUE_RESET 0x00010000U
#define FX1_ISP_TUNE_AWB_E2_RADIUS2_VALUE_ACCESS FX1_ISP_ACCESS_RW

/**
 * TUNE_AWB_E3_CENTER: địa chỉ lệch 0x83A0, dòng bảng 963, kiểu truy cập RW
 * Tâm ellipse theo R/G và B/G không dấu Q8.8; cập nhật theo profile, MAS v1.07 tuning-awb.
 */
#define FX1_ISP_TUNE_AWB_E3_CENTER_OFFSET 0x83A0U
#define REG_TUNE_AWB_E3_CENTER FX1_ISP_TUNE_AWB_E3_CENTER_OFFSET
#define FX1_ISP_TUNE_AWB_E3_CENTER_RESET 0x01000100U
#define FX1_ISP_TUNE_AWB_E3_CENTER_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_TUNE_AWB_E3_CENTER_RO_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_E3_CENTER_RW_MASK 0xFFFFFFFFU
#define FX1_ISP_TUNE_AWB_E3_CENTER_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_E3_CENTER_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_E3_CENTER_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_E3_CENTER_W1SC_MASK 0x00000000U
/**
 * RG [15:0], dòng bảng 964, kiểu truy cập RW
 * Tọa độ tâm theo tỷ số R/G.
 */
#define FX1_ISP_TUNE_AWB_E3_CENTER_RG_SHIFT 0U
#define FX1_ISP_TUNE_AWB_E3_CENTER_RG_WIDTH 16U
#define FX1_ISP_TUNE_AWB_E3_CENTER_RG_MASK 0x0000FFFFU
#define FX1_ISP_TUNE_AWB_E3_CENTER_RG_RESET 0x00000100U
#define FX1_ISP_TUNE_AWB_E3_CENTER_RG_ACCESS FX1_ISP_ACCESS_RW
/**
 * BG [31:16], dòng bảng 965, kiểu truy cập RW
 * Tọa độ tâm theo tỷ số B/G.
 */
#define FX1_ISP_TUNE_AWB_E3_CENTER_BG_SHIFT 16U
#define FX1_ISP_TUNE_AWB_E3_CENTER_BG_WIDTH 16U
#define FX1_ISP_TUNE_AWB_E3_CENTER_BG_MASK 0xFFFF0000U
#define FX1_ISP_TUNE_AWB_E3_CENTER_BG_RESET 0x00000100U
#define FX1_ISP_TUNE_AWB_E3_CENTER_BG_ACCESS FX1_ISP_ACCESS_RW

/**
 * TUNE_AWB_E3_A: địa chỉ lệch 0x83A4, dòng bảng 966, kiểu truy cập RW
 * Hệ số đường chéo không dấu Q2.14 phải dương; cập nhật theo profile, MAS v1.07 tuning-awb.
 */
#define FX1_ISP_TUNE_AWB_E3_A_OFFSET 0x83A4U
#define REG_TUNE_AWB_E3_A FX1_ISP_TUNE_AWB_E3_A_OFFSET
#define FX1_ISP_TUNE_AWB_E3_A_RESET 0x00004000U
#define FX1_ISP_TUNE_AWB_E3_A_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_TUNE_AWB_E3_A_RO_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_E3_A_RW_MASK 0x0000FFFFU
#define FX1_ISP_TUNE_AWB_E3_A_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_E3_A_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_E3_A_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_E3_A_W1SC_MASK 0x00000000U
/**
 * VALUE [15:0], dòng bảng 967, kiểu truy cập RW
 * Hệ số đường chéo không dấu Q2.14 phải dương.
 */
#define FX1_ISP_TUNE_AWB_E3_A_VALUE_SHIFT 0U
#define FX1_ISP_TUNE_AWB_E3_A_VALUE_WIDTH 16U
#define FX1_ISP_TUNE_AWB_E3_A_VALUE_MASK 0x0000FFFFU
#define FX1_ISP_TUNE_AWB_E3_A_VALUE_RESET 0x00004000U
#define FX1_ISP_TUNE_AWB_E3_A_VALUE_ACCESS FX1_ISP_ACCESS_RW

/**
 * TUNE_AWB_E3_B: địa chỉ lệch 0x83A8, dòng bảng 968, kiểu truy cập RW
 * Hệ số chéo có dấu Q1.14; cập nhật theo profile, MAS v1.07 tuning-awb.
 */
#define FX1_ISP_TUNE_AWB_E3_B_OFFSET 0x83A8U
#define REG_TUNE_AWB_E3_B FX1_ISP_TUNE_AWB_E3_B_OFFSET
#define FX1_ISP_TUNE_AWB_E3_B_RESET 0x00000000U
#define FX1_ISP_TUNE_AWB_E3_B_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_TUNE_AWB_E3_B_RO_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_E3_B_RW_MASK 0x0000FFFFU
#define FX1_ISP_TUNE_AWB_E3_B_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_E3_B_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_E3_B_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_E3_B_W1SC_MASK 0x00000000U
/**
 * VALUE [15:0], dòng bảng 969, kiểu truy cập RW
 * Hệ số chéo có dấu Q1.14.
 */
#define FX1_ISP_TUNE_AWB_E3_B_VALUE_SHIFT 0U
#define FX1_ISP_TUNE_AWB_E3_B_VALUE_WIDTH 16U
#define FX1_ISP_TUNE_AWB_E3_B_VALUE_MASK 0x0000FFFFU
#define FX1_ISP_TUNE_AWB_E3_B_VALUE_RESET 0x00000000U
#define FX1_ISP_TUNE_AWB_E3_B_VALUE_ACCESS FX1_ISP_ACCESS_RW

/**
 * TUNE_AWB_E3_C: địa chỉ lệch 0x83AC, dòng bảng 970, kiểu truy cập RW
 * Hệ số đường chéo không dấu Q2.14 phải dương; cập nhật theo profile, MAS v1.07 tuning-awb.
 */
#define FX1_ISP_TUNE_AWB_E3_C_OFFSET 0x83ACU
#define REG_TUNE_AWB_E3_C FX1_ISP_TUNE_AWB_E3_C_OFFSET
#define FX1_ISP_TUNE_AWB_E3_C_RESET 0x00004000U
#define FX1_ISP_TUNE_AWB_E3_C_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_TUNE_AWB_E3_C_RO_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_E3_C_RW_MASK 0x0000FFFFU
#define FX1_ISP_TUNE_AWB_E3_C_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_E3_C_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_E3_C_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_E3_C_W1SC_MASK 0x00000000U
/**
 * VALUE [15:0], dòng bảng 971, kiểu truy cập RW
 * Hệ số đường chéo không dấu Q2.14 phải dương.
 */
#define FX1_ISP_TUNE_AWB_E3_C_VALUE_SHIFT 0U
#define FX1_ISP_TUNE_AWB_E3_C_VALUE_WIDTH 16U
#define FX1_ISP_TUNE_AWB_E3_C_VALUE_MASK 0x0000FFFFU
#define FX1_ISP_TUNE_AWB_E3_C_VALUE_RESET 0x00004000U
#define FX1_ISP_TUNE_AWB_E3_C_VALUE_ACCESS FX1_ISP_ACCESS_RW

/**
 * TUNE_AWB_E3_RADIUS2: địa chỉ lệch 0x83B0, dòng bảng 972, kiểu truy cập RW
 * Bình phương bán kính không dấu Q16.16; so metric <= RADIUS2 << 14. Cập nhật theo profile, MAS
 * v1.07 tuning-awb.
 */
#define FX1_ISP_TUNE_AWB_E3_RADIUS2_OFFSET 0x83B0U
#define REG_TUNE_AWB_E3_RADIUS2 FX1_ISP_TUNE_AWB_E3_RADIUS2_OFFSET
#define FX1_ISP_TUNE_AWB_E3_RADIUS2_RESET 0x00010000U
#define FX1_ISP_TUNE_AWB_E3_RADIUS2_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_TUNE_AWB_E3_RADIUS2_RO_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_E3_RADIUS2_RW_MASK 0xFFFFFFFFU
#define FX1_ISP_TUNE_AWB_E3_RADIUS2_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_E3_RADIUS2_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_E3_RADIUS2_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_E3_RADIUS2_W1SC_MASK 0x00000000U
/**
 * VALUE [31:0], dòng bảng 973, kiểu truy cập RW
 * Bình phương bán kính không dấu Q16.16; so metric <= RADIUS2 << 14.
 */
#define FX1_ISP_TUNE_AWB_E3_RADIUS2_VALUE_SHIFT 0U
#define FX1_ISP_TUNE_AWB_E3_RADIUS2_VALUE_WIDTH 32U
#define FX1_ISP_TUNE_AWB_E3_RADIUS2_VALUE_MASK 0xFFFFFFFFU
#define FX1_ISP_TUNE_AWB_E3_RADIUS2_VALUE_RESET 0x00010000U
#define FX1_ISP_TUNE_AWB_E3_RADIUS2_VALUE_ACCESS FX1_ISP_ACCESS_RW

/**
 * TUNE_AWB_E4_CENTER: địa chỉ lệch 0x83C0, dòng bảng 974, kiểu truy cập RW
 * Tâm ellipse theo R/G và B/G không dấu Q8.8; cập nhật theo profile, MAS v1.07 tuning-awb.
 */
#define FX1_ISP_TUNE_AWB_E4_CENTER_OFFSET 0x83C0U
#define REG_TUNE_AWB_E4_CENTER FX1_ISP_TUNE_AWB_E4_CENTER_OFFSET
#define FX1_ISP_TUNE_AWB_E4_CENTER_RESET 0x01000100U
#define FX1_ISP_TUNE_AWB_E4_CENTER_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_TUNE_AWB_E4_CENTER_RO_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_E4_CENTER_RW_MASK 0xFFFFFFFFU
#define FX1_ISP_TUNE_AWB_E4_CENTER_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_E4_CENTER_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_E4_CENTER_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_E4_CENTER_W1SC_MASK 0x00000000U
/**
 * RG [15:0], dòng bảng 975, kiểu truy cập RW
 * Tọa độ tâm theo tỷ số R/G.
 */
#define FX1_ISP_TUNE_AWB_E4_CENTER_RG_SHIFT 0U
#define FX1_ISP_TUNE_AWB_E4_CENTER_RG_WIDTH 16U
#define FX1_ISP_TUNE_AWB_E4_CENTER_RG_MASK 0x0000FFFFU
#define FX1_ISP_TUNE_AWB_E4_CENTER_RG_RESET 0x00000100U
#define FX1_ISP_TUNE_AWB_E4_CENTER_RG_ACCESS FX1_ISP_ACCESS_RW
/**
 * BG [31:16], dòng bảng 976, kiểu truy cập RW
 * Tọa độ tâm theo tỷ số B/G.
 */
#define FX1_ISP_TUNE_AWB_E4_CENTER_BG_SHIFT 16U
#define FX1_ISP_TUNE_AWB_E4_CENTER_BG_WIDTH 16U
#define FX1_ISP_TUNE_AWB_E4_CENTER_BG_MASK 0xFFFF0000U
#define FX1_ISP_TUNE_AWB_E4_CENTER_BG_RESET 0x00000100U
#define FX1_ISP_TUNE_AWB_E4_CENTER_BG_ACCESS FX1_ISP_ACCESS_RW

/**
 * TUNE_AWB_E4_A: địa chỉ lệch 0x83C4, dòng bảng 977, kiểu truy cập RW
 * Hệ số đường chéo không dấu Q2.14 phải dương; cập nhật theo profile, MAS v1.07 tuning-awb.
 */
#define FX1_ISP_TUNE_AWB_E4_A_OFFSET 0x83C4U
#define REG_TUNE_AWB_E4_A FX1_ISP_TUNE_AWB_E4_A_OFFSET
#define FX1_ISP_TUNE_AWB_E4_A_RESET 0x00004000U
#define FX1_ISP_TUNE_AWB_E4_A_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_TUNE_AWB_E4_A_RO_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_E4_A_RW_MASK 0x0000FFFFU
#define FX1_ISP_TUNE_AWB_E4_A_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_E4_A_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_E4_A_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_E4_A_W1SC_MASK 0x00000000U
/**
 * VALUE [15:0], dòng bảng 978, kiểu truy cập RW
 * Hệ số đường chéo không dấu Q2.14 phải dương.
 */
#define FX1_ISP_TUNE_AWB_E4_A_VALUE_SHIFT 0U
#define FX1_ISP_TUNE_AWB_E4_A_VALUE_WIDTH 16U
#define FX1_ISP_TUNE_AWB_E4_A_VALUE_MASK 0x0000FFFFU
#define FX1_ISP_TUNE_AWB_E4_A_VALUE_RESET 0x00004000U
#define FX1_ISP_TUNE_AWB_E4_A_VALUE_ACCESS FX1_ISP_ACCESS_RW

/**
 * TUNE_AWB_E4_B: địa chỉ lệch 0x83C8, dòng bảng 979, kiểu truy cập RW
 * Hệ số chéo có dấu Q1.14; cập nhật theo profile, MAS v1.07 tuning-awb.
 */
#define FX1_ISP_TUNE_AWB_E4_B_OFFSET 0x83C8U
#define REG_TUNE_AWB_E4_B FX1_ISP_TUNE_AWB_E4_B_OFFSET
#define FX1_ISP_TUNE_AWB_E4_B_RESET 0x00000000U
#define FX1_ISP_TUNE_AWB_E4_B_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_TUNE_AWB_E4_B_RO_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_E4_B_RW_MASK 0x0000FFFFU
#define FX1_ISP_TUNE_AWB_E4_B_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_E4_B_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_E4_B_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_E4_B_W1SC_MASK 0x00000000U
/**
 * VALUE [15:0], dòng bảng 980, kiểu truy cập RW
 * Hệ số chéo có dấu Q1.14.
 */
#define FX1_ISP_TUNE_AWB_E4_B_VALUE_SHIFT 0U
#define FX1_ISP_TUNE_AWB_E4_B_VALUE_WIDTH 16U
#define FX1_ISP_TUNE_AWB_E4_B_VALUE_MASK 0x0000FFFFU
#define FX1_ISP_TUNE_AWB_E4_B_VALUE_RESET 0x00000000U
#define FX1_ISP_TUNE_AWB_E4_B_VALUE_ACCESS FX1_ISP_ACCESS_RW

/**
 * TUNE_AWB_E4_C: địa chỉ lệch 0x83CC, dòng bảng 981, kiểu truy cập RW
 * Hệ số đường chéo không dấu Q2.14 phải dương; cập nhật theo profile, MAS v1.07 tuning-awb.
 */
#define FX1_ISP_TUNE_AWB_E4_C_OFFSET 0x83CCU
#define REG_TUNE_AWB_E4_C FX1_ISP_TUNE_AWB_E4_C_OFFSET
#define FX1_ISP_TUNE_AWB_E4_C_RESET 0x00004000U
#define FX1_ISP_TUNE_AWB_E4_C_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_TUNE_AWB_E4_C_RO_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_E4_C_RW_MASK 0x0000FFFFU
#define FX1_ISP_TUNE_AWB_E4_C_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_E4_C_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_E4_C_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_E4_C_W1SC_MASK 0x00000000U
/**
 * VALUE [15:0], dòng bảng 982, kiểu truy cập RW
 * Hệ số đường chéo không dấu Q2.14 phải dương.
 */
#define FX1_ISP_TUNE_AWB_E4_C_VALUE_SHIFT 0U
#define FX1_ISP_TUNE_AWB_E4_C_VALUE_WIDTH 16U
#define FX1_ISP_TUNE_AWB_E4_C_VALUE_MASK 0x0000FFFFU
#define FX1_ISP_TUNE_AWB_E4_C_VALUE_RESET 0x00004000U
#define FX1_ISP_TUNE_AWB_E4_C_VALUE_ACCESS FX1_ISP_ACCESS_RW

/**
 * TUNE_AWB_E4_RADIUS2: địa chỉ lệch 0x83D0, dòng bảng 983, kiểu truy cập RW
 * Bình phương bán kính không dấu Q16.16; so metric <= RADIUS2 << 14. Cập nhật theo profile, MAS
 * v1.07 tuning-awb.
 */
#define FX1_ISP_TUNE_AWB_E4_RADIUS2_OFFSET 0x83D0U
#define REG_TUNE_AWB_E4_RADIUS2 FX1_ISP_TUNE_AWB_E4_RADIUS2_OFFSET
#define FX1_ISP_TUNE_AWB_E4_RADIUS2_RESET 0x00010000U
#define FX1_ISP_TUNE_AWB_E4_RADIUS2_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_TUNE_AWB_E4_RADIUS2_RO_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_E4_RADIUS2_RW_MASK 0xFFFFFFFFU
#define FX1_ISP_TUNE_AWB_E4_RADIUS2_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_E4_RADIUS2_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_E4_RADIUS2_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_E4_RADIUS2_W1SC_MASK 0x00000000U
/**
 * VALUE [31:0], dòng bảng 984, kiểu truy cập RW
 * Bình phương bán kính không dấu Q16.16; so metric <= RADIUS2 << 14.
 */
#define FX1_ISP_TUNE_AWB_E4_RADIUS2_VALUE_SHIFT 0U
#define FX1_ISP_TUNE_AWB_E4_RADIUS2_VALUE_WIDTH 32U
#define FX1_ISP_TUNE_AWB_E4_RADIUS2_VALUE_MASK 0xFFFFFFFFU
#define FX1_ISP_TUNE_AWB_E4_RADIUS2_VALUE_RESET 0x00010000U
#define FX1_ISP_TUNE_AWB_E4_RADIUS2_VALUE_ACCESS FX1_ISP_ACCESS_RW

/**
 * TUNE_AWB_E5_CENTER: địa chỉ lệch 0x83E0, dòng bảng 985, kiểu truy cập RW
 * Tâm ellipse theo R/G và B/G không dấu Q8.8; cập nhật theo profile, MAS v1.07 tuning-awb.
 */
#define FX1_ISP_TUNE_AWB_E5_CENTER_OFFSET 0x83E0U
#define REG_TUNE_AWB_E5_CENTER FX1_ISP_TUNE_AWB_E5_CENTER_OFFSET
#define FX1_ISP_TUNE_AWB_E5_CENTER_RESET 0x01000100U
#define FX1_ISP_TUNE_AWB_E5_CENTER_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_TUNE_AWB_E5_CENTER_RO_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_E5_CENTER_RW_MASK 0xFFFFFFFFU
#define FX1_ISP_TUNE_AWB_E5_CENTER_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_E5_CENTER_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_E5_CENTER_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_E5_CENTER_W1SC_MASK 0x00000000U
/**
 * RG [15:0], dòng bảng 986, kiểu truy cập RW
 * Tọa độ tâm theo tỷ số R/G.
 */
#define FX1_ISP_TUNE_AWB_E5_CENTER_RG_SHIFT 0U
#define FX1_ISP_TUNE_AWB_E5_CENTER_RG_WIDTH 16U
#define FX1_ISP_TUNE_AWB_E5_CENTER_RG_MASK 0x0000FFFFU
#define FX1_ISP_TUNE_AWB_E5_CENTER_RG_RESET 0x00000100U
#define FX1_ISP_TUNE_AWB_E5_CENTER_RG_ACCESS FX1_ISP_ACCESS_RW
/**
 * BG [31:16], dòng bảng 987, kiểu truy cập RW
 * Tọa độ tâm theo tỷ số B/G.
 */
#define FX1_ISP_TUNE_AWB_E5_CENTER_BG_SHIFT 16U
#define FX1_ISP_TUNE_AWB_E5_CENTER_BG_WIDTH 16U
#define FX1_ISP_TUNE_AWB_E5_CENTER_BG_MASK 0xFFFF0000U
#define FX1_ISP_TUNE_AWB_E5_CENTER_BG_RESET 0x00000100U
#define FX1_ISP_TUNE_AWB_E5_CENTER_BG_ACCESS FX1_ISP_ACCESS_RW

/**
 * TUNE_AWB_E5_A: địa chỉ lệch 0x83E4, dòng bảng 988, kiểu truy cập RW
 * Hệ số đường chéo không dấu Q2.14 phải dương; cập nhật theo profile, MAS v1.07 tuning-awb.
 */
#define FX1_ISP_TUNE_AWB_E5_A_OFFSET 0x83E4U
#define REG_TUNE_AWB_E5_A FX1_ISP_TUNE_AWB_E5_A_OFFSET
#define FX1_ISP_TUNE_AWB_E5_A_RESET 0x00004000U
#define FX1_ISP_TUNE_AWB_E5_A_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_TUNE_AWB_E5_A_RO_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_E5_A_RW_MASK 0x0000FFFFU
#define FX1_ISP_TUNE_AWB_E5_A_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_E5_A_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_E5_A_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_E5_A_W1SC_MASK 0x00000000U
/**
 * VALUE [15:0], dòng bảng 989, kiểu truy cập RW
 * Hệ số đường chéo không dấu Q2.14 phải dương.
 */
#define FX1_ISP_TUNE_AWB_E5_A_VALUE_SHIFT 0U
#define FX1_ISP_TUNE_AWB_E5_A_VALUE_WIDTH 16U
#define FX1_ISP_TUNE_AWB_E5_A_VALUE_MASK 0x0000FFFFU
#define FX1_ISP_TUNE_AWB_E5_A_VALUE_RESET 0x00004000U
#define FX1_ISP_TUNE_AWB_E5_A_VALUE_ACCESS FX1_ISP_ACCESS_RW

/**
 * TUNE_AWB_E5_B: địa chỉ lệch 0x83E8, dòng bảng 990, kiểu truy cập RW
 * Hệ số chéo có dấu Q1.14; cập nhật theo profile, MAS v1.07 tuning-awb.
 */
#define FX1_ISP_TUNE_AWB_E5_B_OFFSET 0x83E8U
#define REG_TUNE_AWB_E5_B FX1_ISP_TUNE_AWB_E5_B_OFFSET
#define FX1_ISP_TUNE_AWB_E5_B_RESET 0x00000000U
#define FX1_ISP_TUNE_AWB_E5_B_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_TUNE_AWB_E5_B_RO_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_E5_B_RW_MASK 0x0000FFFFU
#define FX1_ISP_TUNE_AWB_E5_B_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_E5_B_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_E5_B_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_E5_B_W1SC_MASK 0x00000000U
/**
 * VALUE [15:0], dòng bảng 991, kiểu truy cập RW
 * Hệ số chéo có dấu Q1.14.
 */
#define FX1_ISP_TUNE_AWB_E5_B_VALUE_SHIFT 0U
#define FX1_ISP_TUNE_AWB_E5_B_VALUE_WIDTH 16U
#define FX1_ISP_TUNE_AWB_E5_B_VALUE_MASK 0x0000FFFFU
#define FX1_ISP_TUNE_AWB_E5_B_VALUE_RESET 0x00000000U
#define FX1_ISP_TUNE_AWB_E5_B_VALUE_ACCESS FX1_ISP_ACCESS_RW

/**
 * TUNE_AWB_E5_C: địa chỉ lệch 0x83EC, dòng bảng 992, kiểu truy cập RW
 * Hệ số đường chéo không dấu Q2.14 phải dương; cập nhật theo profile, MAS v1.07 tuning-awb.
 */
#define FX1_ISP_TUNE_AWB_E5_C_OFFSET 0x83ECU
#define REG_TUNE_AWB_E5_C FX1_ISP_TUNE_AWB_E5_C_OFFSET
#define FX1_ISP_TUNE_AWB_E5_C_RESET 0x00004000U
#define FX1_ISP_TUNE_AWB_E5_C_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_TUNE_AWB_E5_C_RO_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_E5_C_RW_MASK 0x0000FFFFU
#define FX1_ISP_TUNE_AWB_E5_C_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_E5_C_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_E5_C_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_E5_C_W1SC_MASK 0x00000000U
/**
 * VALUE [15:0], dòng bảng 993, kiểu truy cập RW
 * Hệ số đường chéo không dấu Q2.14 phải dương.
 */
#define FX1_ISP_TUNE_AWB_E5_C_VALUE_SHIFT 0U
#define FX1_ISP_TUNE_AWB_E5_C_VALUE_WIDTH 16U
#define FX1_ISP_TUNE_AWB_E5_C_VALUE_MASK 0x0000FFFFU
#define FX1_ISP_TUNE_AWB_E5_C_VALUE_RESET 0x00004000U
#define FX1_ISP_TUNE_AWB_E5_C_VALUE_ACCESS FX1_ISP_ACCESS_RW

/**
 * TUNE_AWB_E5_RADIUS2: địa chỉ lệch 0x83F0, dòng bảng 994, kiểu truy cập RW
 * Bình phương bán kính không dấu Q16.16; so metric <= RADIUS2 << 14. Cập nhật theo profile, MAS
 * v1.07 tuning-awb.
 */
#define FX1_ISP_TUNE_AWB_E5_RADIUS2_OFFSET 0x83F0U
#define REG_TUNE_AWB_E5_RADIUS2 FX1_ISP_TUNE_AWB_E5_RADIUS2_OFFSET
#define FX1_ISP_TUNE_AWB_E5_RADIUS2_RESET 0x00010000U
#define FX1_ISP_TUNE_AWB_E5_RADIUS2_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_TUNE_AWB_E5_RADIUS2_RO_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_E5_RADIUS2_RW_MASK 0xFFFFFFFFU
#define FX1_ISP_TUNE_AWB_E5_RADIUS2_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_E5_RADIUS2_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_E5_RADIUS2_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_E5_RADIUS2_W1SC_MASK 0x00000000U
/**
 * VALUE [31:0], dòng bảng 995, kiểu truy cập RW
 * Bình phương bán kính không dấu Q16.16; so metric <= RADIUS2 << 14.
 */
#define FX1_ISP_TUNE_AWB_E5_RADIUS2_VALUE_SHIFT 0U
#define FX1_ISP_TUNE_AWB_E5_RADIUS2_VALUE_WIDTH 32U
#define FX1_ISP_TUNE_AWB_E5_RADIUS2_VALUE_MASK 0xFFFFFFFFU
#define FX1_ISP_TUNE_AWB_E5_RADIUS2_VALUE_RESET 0x00010000U
#define FX1_ISP_TUNE_AWB_E5_RADIUS2_VALUE_ACCESS FX1_ISP_ACCESS_RW

/**
 * TUNE_AWB_E6_CENTER: địa chỉ lệch 0x8400, dòng bảng 996, kiểu truy cập RW
 * Tâm ellipse theo R/G và B/G không dấu Q8.8; cập nhật theo profile, MAS v1.07 tuning-awb.
 */
#define FX1_ISP_TUNE_AWB_E6_CENTER_OFFSET 0x8400U
#define REG_TUNE_AWB_E6_CENTER FX1_ISP_TUNE_AWB_E6_CENTER_OFFSET
#define FX1_ISP_TUNE_AWB_E6_CENTER_RESET 0x01000100U
#define FX1_ISP_TUNE_AWB_E6_CENTER_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_TUNE_AWB_E6_CENTER_RO_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_E6_CENTER_RW_MASK 0xFFFFFFFFU
#define FX1_ISP_TUNE_AWB_E6_CENTER_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_E6_CENTER_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_E6_CENTER_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_E6_CENTER_W1SC_MASK 0x00000000U
/**
 * RG [15:0], dòng bảng 997, kiểu truy cập RW
 * Tọa độ tâm theo tỷ số R/G.
 */
#define FX1_ISP_TUNE_AWB_E6_CENTER_RG_SHIFT 0U
#define FX1_ISP_TUNE_AWB_E6_CENTER_RG_WIDTH 16U
#define FX1_ISP_TUNE_AWB_E6_CENTER_RG_MASK 0x0000FFFFU
#define FX1_ISP_TUNE_AWB_E6_CENTER_RG_RESET 0x00000100U
#define FX1_ISP_TUNE_AWB_E6_CENTER_RG_ACCESS FX1_ISP_ACCESS_RW
/**
 * BG [31:16], dòng bảng 998, kiểu truy cập RW
 * Tọa độ tâm theo tỷ số B/G.
 */
#define FX1_ISP_TUNE_AWB_E6_CENTER_BG_SHIFT 16U
#define FX1_ISP_TUNE_AWB_E6_CENTER_BG_WIDTH 16U
#define FX1_ISP_TUNE_AWB_E6_CENTER_BG_MASK 0xFFFF0000U
#define FX1_ISP_TUNE_AWB_E6_CENTER_BG_RESET 0x00000100U
#define FX1_ISP_TUNE_AWB_E6_CENTER_BG_ACCESS FX1_ISP_ACCESS_RW

/**
 * TUNE_AWB_E6_A: địa chỉ lệch 0x8404, dòng bảng 999, kiểu truy cập RW
 * Hệ số đường chéo không dấu Q2.14 phải dương; cập nhật theo profile, MAS v1.07 tuning-awb.
 */
#define FX1_ISP_TUNE_AWB_E6_A_OFFSET 0x8404U
#define REG_TUNE_AWB_E6_A FX1_ISP_TUNE_AWB_E6_A_OFFSET
#define FX1_ISP_TUNE_AWB_E6_A_RESET 0x00004000U
#define FX1_ISP_TUNE_AWB_E6_A_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_TUNE_AWB_E6_A_RO_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_E6_A_RW_MASK 0x0000FFFFU
#define FX1_ISP_TUNE_AWB_E6_A_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_E6_A_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_E6_A_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_E6_A_W1SC_MASK 0x00000000U
/**
 * VALUE [15:0], dòng bảng 1000, kiểu truy cập RW
 * Hệ số đường chéo không dấu Q2.14 phải dương.
 */
#define FX1_ISP_TUNE_AWB_E6_A_VALUE_SHIFT 0U
#define FX1_ISP_TUNE_AWB_E6_A_VALUE_WIDTH 16U
#define FX1_ISP_TUNE_AWB_E6_A_VALUE_MASK 0x0000FFFFU
#define FX1_ISP_TUNE_AWB_E6_A_VALUE_RESET 0x00004000U
#define FX1_ISP_TUNE_AWB_E6_A_VALUE_ACCESS FX1_ISP_ACCESS_RW

/**
 * TUNE_AWB_E6_B: địa chỉ lệch 0x8408, dòng bảng 1001, kiểu truy cập RW
 * Hệ số chéo có dấu Q1.14; cập nhật theo profile, MAS v1.07 tuning-awb.
 */
#define FX1_ISP_TUNE_AWB_E6_B_OFFSET 0x8408U
#define REG_TUNE_AWB_E6_B FX1_ISP_TUNE_AWB_E6_B_OFFSET
#define FX1_ISP_TUNE_AWB_E6_B_RESET 0x00000000U
#define FX1_ISP_TUNE_AWB_E6_B_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_TUNE_AWB_E6_B_RO_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_E6_B_RW_MASK 0x0000FFFFU
#define FX1_ISP_TUNE_AWB_E6_B_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_E6_B_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_E6_B_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_E6_B_W1SC_MASK 0x00000000U
/**
 * VALUE [15:0], dòng bảng 1002, kiểu truy cập RW
 * Hệ số chéo có dấu Q1.14.
 */
#define FX1_ISP_TUNE_AWB_E6_B_VALUE_SHIFT 0U
#define FX1_ISP_TUNE_AWB_E6_B_VALUE_WIDTH 16U
#define FX1_ISP_TUNE_AWB_E6_B_VALUE_MASK 0x0000FFFFU
#define FX1_ISP_TUNE_AWB_E6_B_VALUE_RESET 0x00000000U
#define FX1_ISP_TUNE_AWB_E6_B_VALUE_ACCESS FX1_ISP_ACCESS_RW

/**
 * TUNE_AWB_E6_C: địa chỉ lệch 0x840C, dòng bảng 1003, kiểu truy cập RW
 * Hệ số đường chéo không dấu Q2.14 phải dương; cập nhật theo profile, MAS v1.07 tuning-awb.
 */
#define FX1_ISP_TUNE_AWB_E6_C_OFFSET 0x840CU
#define REG_TUNE_AWB_E6_C FX1_ISP_TUNE_AWB_E6_C_OFFSET
#define FX1_ISP_TUNE_AWB_E6_C_RESET 0x00004000U
#define FX1_ISP_TUNE_AWB_E6_C_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_TUNE_AWB_E6_C_RO_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_E6_C_RW_MASK 0x0000FFFFU
#define FX1_ISP_TUNE_AWB_E6_C_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_E6_C_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_E6_C_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_E6_C_W1SC_MASK 0x00000000U
/**
 * VALUE [15:0], dòng bảng 1004, kiểu truy cập RW
 * Hệ số đường chéo không dấu Q2.14 phải dương.
 */
#define FX1_ISP_TUNE_AWB_E6_C_VALUE_SHIFT 0U
#define FX1_ISP_TUNE_AWB_E6_C_VALUE_WIDTH 16U
#define FX1_ISP_TUNE_AWB_E6_C_VALUE_MASK 0x0000FFFFU
#define FX1_ISP_TUNE_AWB_E6_C_VALUE_RESET 0x00004000U
#define FX1_ISP_TUNE_AWB_E6_C_VALUE_ACCESS FX1_ISP_ACCESS_RW

/**
 * TUNE_AWB_E6_RADIUS2: địa chỉ lệch 0x8410, dòng bảng 1005, kiểu truy cập RW
 * Bình phương bán kính không dấu Q16.16; so metric <= RADIUS2 << 14. Cập nhật theo profile, MAS
 * v1.07 tuning-awb.
 */
#define FX1_ISP_TUNE_AWB_E6_RADIUS2_OFFSET 0x8410U
#define REG_TUNE_AWB_E6_RADIUS2 FX1_ISP_TUNE_AWB_E6_RADIUS2_OFFSET
#define FX1_ISP_TUNE_AWB_E6_RADIUS2_RESET 0x00010000U
#define FX1_ISP_TUNE_AWB_E6_RADIUS2_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_TUNE_AWB_E6_RADIUS2_RO_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_E6_RADIUS2_RW_MASK 0xFFFFFFFFU
#define FX1_ISP_TUNE_AWB_E6_RADIUS2_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_E6_RADIUS2_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_E6_RADIUS2_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_E6_RADIUS2_W1SC_MASK 0x00000000U
/**
 * VALUE [31:0], dòng bảng 1006, kiểu truy cập RW
 * Bình phương bán kính không dấu Q16.16; so metric <= RADIUS2 << 14.
 */
#define FX1_ISP_TUNE_AWB_E6_RADIUS2_VALUE_SHIFT 0U
#define FX1_ISP_TUNE_AWB_E6_RADIUS2_VALUE_WIDTH 32U
#define FX1_ISP_TUNE_AWB_E6_RADIUS2_VALUE_MASK 0xFFFFFFFFU
#define FX1_ISP_TUNE_AWB_E6_RADIUS2_VALUE_RESET 0x00010000U
#define FX1_ISP_TUNE_AWB_E6_RADIUS2_VALUE_ACCESS FX1_ISP_ACCESS_RW

/**
 * TUNE_AWB_E7_CENTER: địa chỉ lệch 0x8420, dòng bảng 1007, kiểu truy cập RW
 * Tâm ellipse theo R/G và B/G không dấu Q8.8; cập nhật theo profile, MAS v1.07 tuning-awb.
 */
#define FX1_ISP_TUNE_AWB_E7_CENTER_OFFSET 0x8420U
#define REG_TUNE_AWB_E7_CENTER FX1_ISP_TUNE_AWB_E7_CENTER_OFFSET
#define FX1_ISP_TUNE_AWB_E7_CENTER_RESET 0x01000100U
#define FX1_ISP_TUNE_AWB_E7_CENTER_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_TUNE_AWB_E7_CENTER_RO_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_E7_CENTER_RW_MASK 0xFFFFFFFFU
#define FX1_ISP_TUNE_AWB_E7_CENTER_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_E7_CENTER_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_E7_CENTER_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_E7_CENTER_W1SC_MASK 0x00000000U
/**
 * RG [15:0], dòng bảng 1008, kiểu truy cập RW
 * Tọa độ tâm theo tỷ số R/G.
 */
#define FX1_ISP_TUNE_AWB_E7_CENTER_RG_SHIFT 0U
#define FX1_ISP_TUNE_AWB_E7_CENTER_RG_WIDTH 16U
#define FX1_ISP_TUNE_AWB_E7_CENTER_RG_MASK 0x0000FFFFU
#define FX1_ISP_TUNE_AWB_E7_CENTER_RG_RESET 0x00000100U
#define FX1_ISP_TUNE_AWB_E7_CENTER_RG_ACCESS FX1_ISP_ACCESS_RW
/**
 * BG [31:16], dòng bảng 1009, kiểu truy cập RW
 * Tọa độ tâm theo tỷ số B/G.
 */
#define FX1_ISP_TUNE_AWB_E7_CENTER_BG_SHIFT 16U
#define FX1_ISP_TUNE_AWB_E7_CENTER_BG_WIDTH 16U
#define FX1_ISP_TUNE_AWB_E7_CENTER_BG_MASK 0xFFFF0000U
#define FX1_ISP_TUNE_AWB_E7_CENTER_BG_RESET 0x00000100U
#define FX1_ISP_TUNE_AWB_E7_CENTER_BG_ACCESS FX1_ISP_ACCESS_RW

/**
 * TUNE_AWB_E7_A: địa chỉ lệch 0x8424, dòng bảng 1010, kiểu truy cập RW
 * Hệ số đường chéo không dấu Q2.14 phải dương; cập nhật theo profile, MAS v1.07 tuning-awb.
 */
#define FX1_ISP_TUNE_AWB_E7_A_OFFSET 0x8424U
#define REG_TUNE_AWB_E7_A FX1_ISP_TUNE_AWB_E7_A_OFFSET
#define FX1_ISP_TUNE_AWB_E7_A_RESET 0x00004000U
#define FX1_ISP_TUNE_AWB_E7_A_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_TUNE_AWB_E7_A_RO_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_E7_A_RW_MASK 0x0000FFFFU
#define FX1_ISP_TUNE_AWB_E7_A_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_E7_A_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_E7_A_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_E7_A_W1SC_MASK 0x00000000U
/**
 * VALUE [15:0], dòng bảng 1011, kiểu truy cập RW
 * Hệ số đường chéo không dấu Q2.14 phải dương.
 */
#define FX1_ISP_TUNE_AWB_E7_A_VALUE_SHIFT 0U
#define FX1_ISP_TUNE_AWB_E7_A_VALUE_WIDTH 16U
#define FX1_ISP_TUNE_AWB_E7_A_VALUE_MASK 0x0000FFFFU
#define FX1_ISP_TUNE_AWB_E7_A_VALUE_RESET 0x00004000U
#define FX1_ISP_TUNE_AWB_E7_A_VALUE_ACCESS FX1_ISP_ACCESS_RW

/**
 * TUNE_AWB_E7_B: địa chỉ lệch 0x8428, dòng bảng 1012, kiểu truy cập RW
 * Hệ số chéo có dấu Q1.14; cập nhật theo profile, MAS v1.07 tuning-awb.
 */
#define FX1_ISP_TUNE_AWB_E7_B_OFFSET 0x8428U
#define REG_TUNE_AWB_E7_B FX1_ISP_TUNE_AWB_E7_B_OFFSET
#define FX1_ISP_TUNE_AWB_E7_B_RESET 0x00000000U
#define FX1_ISP_TUNE_AWB_E7_B_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_TUNE_AWB_E7_B_RO_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_E7_B_RW_MASK 0x0000FFFFU
#define FX1_ISP_TUNE_AWB_E7_B_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_E7_B_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_E7_B_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_E7_B_W1SC_MASK 0x00000000U
/**
 * VALUE [15:0], dòng bảng 1013, kiểu truy cập RW
 * Hệ số chéo có dấu Q1.14.
 */
#define FX1_ISP_TUNE_AWB_E7_B_VALUE_SHIFT 0U
#define FX1_ISP_TUNE_AWB_E7_B_VALUE_WIDTH 16U
#define FX1_ISP_TUNE_AWB_E7_B_VALUE_MASK 0x0000FFFFU
#define FX1_ISP_TUNE_AWB_E7_B_VALUE_RESET 0x00000000U
#define FX1_ISP_TUNE_AWB_E7_B_VALUE_ACCESS FX1_ISP_ACCESS_RW

/**
 * TUNE_AWB_E7_C: địa chỉ lệch 0x842C, dòng bảng 1014, kiểu truy cập RW
 * Hệ số đường chéo không dấu Q2.14 phải dương; cập nhật theo profile, MAS v1.07 tuning-awb.
 */
#define FX1_ISP_TUNE_AWB_E7_C_OFFSET 0x842CU
#define REG_TUNE_AWB_E7_C FX1_ISP_TUNE_AWB_E7_C_OFFSET
#define FX1_ISP_TUNE_AWB_E7_C_RESET 0x00004000U
#define FX1_ISP_TUNE_AWB_E7_C_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_TUNE_AWB_E7_C_RO_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_E7_C_RW_MASK 0x0000FFFFU
#define FX1_ISP_TUNE_AWB_E7_C_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_E7_C_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_E7_C_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_E7_C_W1SC_MASK 0x00000000U
/**
 * VALUE [15:0], dòng bảng 1015, kiểu truy cập RW
 * Hệ số đường chéo không dấu Q2.14 phải dương.
 */
#define FX1_ISP_TUNE_AWB_E7_C_VALUE_SHIFT 0U
#define FX1_ISP_TUNE_AWB_E7_C_VALUE_WIDTH 16U
#define FX1_ISP_TUNE_AWB_E7_C_VALUE_MASK 0x0000FFFFU
#define FX1_ISP_TUNE_AWB_E7_C_VALUE_RESET 0x00004000U
#define FX1_ISP_TUNE_AWB_E7_C_VALUE_ACCESS FX1_ISP_ACCESS_RW

/**
 * TUNE_AWB_E7_RADIUS2: địa chỉ lệch 0x8430, dòng bảng 1016, kiểu truy cập RW
 * Bình phương bán kính không dấu Q16.16; so metric <= RADIUS2 << 14. Cập nhật theo profile, MAS
 * v1.07 tuning-awb.
 */
#define FX1_ISP_TUNE_AWB_E7_RADIUS2_OFFSET 0x8430U
#define REG_TUNE_AWB_E7_RADIUS2 FX1_ISP_TUNE_AWB_E7_RADIUS2_OFFSET
#define FX1_ISP_TUNE_AWB_E7_RADIUS2_RESET 0x00010000U
#define FX1_ISP_TUNE_AWB_E7_RADIUS2_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_TUNE_AWB_E7_RADIUS2_RO_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_E7_RADIUS2_RW_MASK 0xFFFFFFFFU
#define FX1_ISP_TUNE_AWB_E7_RADIUS2_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_E7_RADIUS2_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_E7_RADIUS2_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_E7_RADIUS2_W1SC_MASK 0x00000000U
/**
 * VALUE [31:0], dòng bảng 1017, kiểu truy cập RW
 * Bình phương bán kính không dấu Q16.16; so metric <= RADIUS2 << 14.
 */
#define FX1_ISP_TUNE_AWB_E7_RADIUS2_VALUE_SHIFT 0U
#define FX1_ISP_TUNE_AWB_E7_RADIUS2_VALUE_WIDTH 32U
#define FX1_ISP_TUNE_AWB_E7_RADIUS2_VALUE_MASK 0xFFFFFFFFU
#define FX1_ISP_TUNE_AWB_E7_RADIUS2_VALUE_RESET 0x00010000U
#define FX1_ISP_TUNE_AWB_E7_RADIUS2_VALUE_ACCESS FX1_ISP_ACCESS_RW

/**
 * TUNE_AF_CTRL: địa chỉ lệch 0x8500, dòng bảng 1018, kiểu truy cập RW
 * Lựa chọn AF tại tap CSC Y8; cập nhật theo profile, MAS v1.07 tuning-af.
 */
#define FX1_ISP_TUNE_AF_CTRL_OFFSET 0x8500U
#define REG_TUNE_AF_CTRL FX1_ISP_TUNE_AF_CTRL_OFFSET
#define FX1_ISP_TUNE_AF_CTRL_RESET 0xFFFF0000U
#define FX1_ISP_TUNE_AF_CTRL_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_TUNE_AF_CTRL_RO_MASK 0x00000000U
#define FX1_ISP_TUNE_AF_CTRL_RW_MASK 0xFFFF0001U
#define FX1_ISP_TUNE_AF_CTRL_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_AF_CTRL_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_AF_CTRL_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_AF_CTRL_W1SC_MASK 0x00000000U
/**
 * WINDOW_MODE [0:0], dòng bảng 1019, kiểu truy cập RW
 * 0=lưới legacy đều 4x4,1=mười sáu cửa sổ lập trình được.
 */
#define FX1_ISP_TUNE_AF_CTRL_WINDOW_MODE_SHIFT 0U
#define FX1_ISP_TUNE_AF_CTRL_WINDOW_MODE_WIDTH 1U
#define FX1_ISP_TUNE_AF_CTRL_WINDOW_MODE_MASK 0x00000001U
#define FX1_ISP_TUNE_AF_CTRL_WINDOW_MODE_RESET 0x00000000U
#define FX1_ISP_TUNE_AF_CTRL_WINDOW_MODE_ACCESS FX1_ISP_ACCESS_RW
/**
 * WINDOW_MASK [31:16], dòng bảng 1020, kiểu truy cập RW
 * Bật bộ tích lũy từng metric; các cửa sổ chồng nhau đếm độc lập.
 */
#define FX1_ISP_TUNE_AF_CTRL_WINDOW_MASK_SHIFT 16U
#define FX1_ISP_TUNE_AF_CTRL_WINDOW_MASK_WIDTH 16U
#define FX1_ISP_TUNE_AF_CTRL_WINDOW_MASK_MASK 0xFFFF0000U
#define FX1_ISP_TUNE_AF_CTRL_WINDOW_MASK_RESET 0x0000FFFFU
#define FX1_ISP_TUNE_AF_CTRL_WINDOW_MASK_ACCESS FX1_ISP_ACCESS_RW

/**
 * TUNE_AF_NOISE: địa chỉ lệch 0x8504, dòng bảng 1021, kiểu truy cập RW
 * Trừ ngưỡng từ abs(SobelX)+abs(SobelY), kẹp sàn 0 trước cộng tổng; cập nhật theo profile, MAS v1.07
 * tuning-af.
 */
#define FX1_ISP_TUNE_AF_NOISE_OFFSET 0x8504U
#define REG_TUNE_AF_NOISE FX1_ISP_TUNE_AF_NOISE_OFFSET
#define FX1_ISP_TUNE_AF_NOISE_RESET 0x00000000U
#define FX1_ISP_TUNE_AF_NOISE_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_TUNE_AF_NOISE_RO_MASK 0x00000000U
#define FX1_ISP_TUNE_AF_NOISE_RW_MASK 0x00000FFFU
#define FX1_ISP_TUNE_AF_NOISE_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_AF_NOISE_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_AF_NOISE_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_AF_NOISE_W1SC_MASK 0x00000000U
/**
 * VALUE [11:0], dòng bảng 1022, kiểu truy cập RW
 * Trừ khỏi abs(SobelX)+abs(SobelY), kẹp về 0 trước tích lũy.
 */
#define FX1_ISP_TUNE_AF_NOISE_VALUE_SHIFT 0U
#define FX1_ISP_TUNE_AF_NOISE_VALUE_WIDTH 12U
#define FX1_ISP_TUNE_AF_NOISE_VALUE_MASK 0x00000FFFU
#define FX1_ISP_TUNE_AF_NOISE_VALUE_RESET 0x00000000U
#define FX1_ISP_TUNE_AF_NOISE_VALUE_ACCESS FX1_ISP_ACCESS_RW

/**
 * TUNE_AF_W0_START: địa chỉ lệch 0x8540, dòng bảng 1023, kiểu truy cập RW
 * Điểm bắt đầu gồm đầu mút theo pixel đủ độ phân giải; cập nhật theo profile, MAS v1.07 tuning-af.
 */
#define FX1_ISP_TUNE_AF_W0_START_OFFSET 0x8540U
#define REG_TUNE_AF_W0_START FX1_ISP_TUNE_AF_W0_START_OFFSET
#define FX1_ISP_TUNE_AF_W0_START_RESET 0x00000000U
#define FX1_ISP_TUNE_AF_W0_START_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_TUNE_AF_W0_START_RO_MASK 0x00000000U
#define FX1_ISP_TUNE_AF_W0_START_RW_MASK 0xFFFFFFFFU
#define FX1_ISP_TUNE_AF_W0_START_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_AF_W0_START_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_AF_W0_START_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_AF_W0_START_W1SC_MASK 0x00000000U
/**
 * X [15:0], dòng bảng 1024, kiểu truy cập RW
 * Tọa độ ngang của đầu gồm đầu mút hoặc cuối không gồm đầu mút.
 */
#define FX1_ISP_TUNE_AF_W0_START_X_SHIFT 0U
#define FX1_ISP_TUNE_AF_W0_START_X_WIDTH 16U
#define FX1_ISP_TUNE_AF_W0_START_X_MASK 0x0000FFFFU
#define FX1_ISP_TUNE_AF_W0_START_X_RESET 0x00000000U
#define FX1_ISP_TUNE_AF_W0_START_X_ACCESS FX1_ISP_ACCESS_RW
/**
 * Y [31:16], dòng bảng 1025, kiểu truy cập RW
 * Tọa độ dọc của đầu gồm đầu mút hoặc cuối không gồm đầu mút.
 */
#define FX1_ISP_TUNE_AF_W0_START_Y_SHIFT 16U
#define FX1_ISP_TUNE_AF_W0_START_Y_WIDTH 16U
#define FX1_ISP_TUNE_AF_W0_START_Y_MASK 0xFFFF0000U
#define FX1_ISP_TUNE_AF_W0_START_Y_RESET 0x00000000U
#define FX1_ISP_TUNE_AF_W0_START_Y_ACCESS FX1_ISP_ACCESS_RW

/**
 * TUNE_AF_W0_END: địa chỉ lệch 0x8544, dòng bảng 1026, kiểu truy cập RW
 * Điểm kết thúc không gồm đầu mút theo pixel đủ độ phân giải; cập nhật theo profile, MAS v1.07
 * tuning-af.
 */
#define FX1_ISP_TUNE_AF_W0_END_OFFSET 0x8544U
#define REG_TUNE_AF_W0_END FX1_ISP_TUNE_AF_W0_END_OFFSET
#define FX1_ISP_TUNE_AF_W0_END_RESET 0x00000000U
#define FX1_ISP_TUNE_AF_W0_END_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_TUNE_AF_W0_END_RO_MASK 0x00000000U
#define FX1_ISP_TUNE_AF_W0_END_RW_MASK 0xFFFFFFFFU
#define FX1_ISP_TUNE_AF_W0_END_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_AF_W0_END_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_AF_W0_END_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_AF_W0_END_W1SC_MASK 0x00000000U
/**
 * X [15:0], dòng bảng 1027, kiểu truy cập RW
 * Tọa độ ngang của đầu gồm đầu mút hoặc cuối không gồm đầu mút.
 */
#define FX1_ISP_TUNE_AF_W0_END_X_SHIFT 0U
#define FX1_ISP_TUNE_AF_W0_END_X_WIDTH 16U
#define FX1_ISP_TUNE_AF_W0_END_X_MASK 0x0000FFFFU
#define FX1_ISP_TUNE_AF_W0_END_X_RESET 0x00000000U
#define FX1_ISP_TUNE_AF_W0_END_X_ACCESS FX1_ISP_ACCESS_RW
/**
 * Y [31:16], dòng bảng 1028, kiểu truy cập RW
 * Tọa độ dọc của đầu gồm đầu mút hoặc cuối không gồm đầu mút.
 */
#define FX1_ISP_TUNE_AF_W0_END_Y_SHIFT 16U
#define FX1_ISP_TUNE_AF_W0_END_Y_WIDTH 16U
#define FX1_ISP_TUNE_AF_W0_END_Y_MASK 0xFFFF0000U
#define FX1_ISP_TUNE_AF_W0_END_Y_RESET 0x00000000U
#define FX1_ISP_TUNE_AF_W0_END_Y_ACCESS FX1_ISP_ACCESS_RW

/**
 * TUNE_AF_W1_START: địa chỉ lệch 0x8548, dòng bảng 1029, kiểu truy cập RW
 * Điểm bắt đầu gồm đầu mút theo pixel đủ độ phân giải; cập nhật theo profile, MAS v1.07 tuning-af.
 */
#define FX1_ISP_TUNE_AF_W1_START_OFFSET 0x8548U
#define REG_TUNE_AF_W1_START FX1_ISP_TUNE_AF_W1_START_OFFSET
#define FX1_ISP_TUNE_AF_W1_START_RESET 0x00000000U
#define FX1_ISP_TUNE_AF_W1_START_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_TUNE_AF_W1_START_RO_MASK 0x00000000U
#define FX1_ISP_TUNE_AF_W1_START_RW_MASK 0xFFFFFFFFU
#define FX1_ISP_TUNE_AF_W1_START_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_AF_W1_START_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_AF_W1_START_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_AF_W1_START_W1SC_MASK 0x00000000U
/**
 * X [15:0], dòng bảng 1030, kiểu truy cập RW
 * Tọa độ ngang của đầu gồm đầu mút hoặc cuối không gồm đầu mút.
 */
#define FX1_ISP_TUNE_AF_W1_START_X_SHIFT 0U
#define FX1_ISP_TUNE_AF_W1_START_X_WIDTH 16U
#define FX1_ISP_TUNE_AF_W1_START_X_MASK 0x0000FFFFU
#define FX1_ISP_TUNE_AF_W1_START_X_RESET 0x00000000U
#define FX1_ISP_TUNE_AF_W1_START_X_ACCESS FX1_ISP_ACCESS_RW
/**
 * Y [31:16], dòng bảng 1031, kiểu truy cập RW
 * Tọa độ dọc của đầu gồm đầu mút hoặc cuối không gồm đầu mút.
 */
#define FX1_ISP_TUNE_AF_W1_START_Y_SHIFT 16U
#define FX1_ISP_TUNE_AF_W1_START_Y_WIDTH 16U
#define FX1_ISP_TUNE_AF_W1_START_Y_MASK 0xFFFF0000U
#define FX1_ISP_TUNE_AF_W1_START_Y_RESET 0x00000000U
#define FX1_ISP_TUNE_AF_W1_START_Y_ACCESS FX1_ISP_ACCESS_RW

/**
 * TUNE_AF_W1_END: địa chỉ lệch 0x854C, dòng bảng 1032, kiểu truy cập RW
 * Điểm kết thúc không gồm đầu mút theo pixel đủ độ phân giải; cập nhật theo profile, MAS v1.07
 * tuning-af.
 */
#define FX1_ISP_TUNE_AF_W1_END_OFFSET 0x854CU
#define REG_TUNE_AF_W1_END FX1_ISP_TUNE_AF_W1_END_OFFSET
#define FX1_ISP_TUNE_AF_W1_END_RESET 0x00000000U
#define FX1_ISP_TUNE_AF_W1_END_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_TUNE_AF_W1_END_RO_MASK 0x00000000U
#define FX1_ISP_TUNE_AF_W1_END_RW_MASK 0xFFFFFFFFU
#define FX1_ISP_TUNE_AF_W1_END_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_AF_W1_END_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_AF_W1_END_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_AF_W1_END_W1SC_MASK 0x00000000U
/**
 * X [15:0], dòng bảng 1033, kiểu truy cập RW
 * Tọa độ ngang của đầu gồm đầu mút hoặc cuối không gồm đầu mút.
 */
#define FX1_ISP_TUNE_AF_W1_END_X_SHIFT 0U
#define FX1_ISP_TUNE_AF_W1_END_X_WIDTH 16U
#define FX1_ISP_TUNE_AF_W1_END_X_MASK 0x0000FFFFU
#define FX1_ISP_TUNE_AF_W1_END_X_RESET 0x00000000U
#define FX1_ISP_TUNE_AF_W1_END_X_ACCESS FX1_ISP_ACCESS_RW
/**
 * Y [31:16], dòng bảng 1034, kiểu truy cập RW
 * Tọa độ dọc của đầu gồm đầu mút hoặc cuối không gồm đầu mút.
 */
#define FX1_ISP_TUNE_AF_W1_END_Y_SHIFT 16U
#define FX1_ISP_TUNE_AF_W1_END_Y_WIDTH 16U
#define FX1_ISP_TUNE_AF_W1_END_Y_MASK 0xFFFF0000U
#define FX1_ISP_TUNE_AF_W1_END_Y_RESET 0x00000000U
#define FX1_ISP_TUNE_AF_W1_END_Y_ACCESS FX1_ISP_ACCESS_RW

/**
 * TUNE_AF_W2_START: địa chỉ lệch 0x8550, dòng bảng 1035, kiểu truy cập RW
 * Điểm bắt đầu gồm đầu mút theo pixel đủ độ phân giải; cập nhật theo profile, MAS v1.07 tuning-af.
 */
#define FX1_ISP_TUNE_AF_W2_START_OFFSET 0x8550U
#define REG_TUNE_AF_W2_START FX1_ISP_TUNE_AF_W2_START_OFFSET
#define FX1_ISP_TUNE_AF_W2_START_RESET 0x00000000U
#define FX1_ISP_TUNE_AF_W2_START_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_TUNE_AF_W2_START_RO_MASK 0x00000000U
#define FX1_ISP_TUNE_AF_W2_START_RW_MASK 0xFFFFFFFFU
#define FX1_ISP_TUNE_AF_W2_START_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_AF_W2_START_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_AF_W2_START_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_AF_W2_START_W1SC_MASK 0x00000000U
/**
 * X [15:0], dòng bảng 1036, kiểu truy cập RW
 * Tọa độ ngang của đầu gồm đầu mút hoặc cuối không gồm đầu mút.
 */
#define FX1_ISP_TUNE_AF_W2_START_X_SHIFT 0U
#define FX1_ISP_TUNE_AF_W2_START_X_WIDTH 16U
#define FX1_ISP_TUNE_AF_W2_START_X_MASK 0x0000FFFFU
#define FX1_ISP_TUNE_AF_W2_START_X_RESET 0x00000000U
#define FX1_ISP_TUNE_AF_W2_START_X_ACCESS FX1_ISP_ACCESS_RW
/**
 * Y [31:16], dòng bảng 1037, kiểu truy cập RW
 * Tọa độ dọc của đầu gồm đầu mút hoặc cuối không gồm đầu mút.
 */
#define FX1_ISP_TUNE_AF_W2_START_Y_SHIFT 16U
#define FX1_ISP_TUNE_AF_W2_START_Y_WIDTH 16U
#define FX1_ISP_TUNE_AF_W2_START_Y_MASK 0xFFFF0000U
#define FX1_ISP_TUNE_AF_W2_START_Y_RESET 0x00000000U
#define FX1_ISP_TUNE_AF_W2_START_Y_ACCESS FX1_ISP_ACCESS_RW

/**
 * TUNE_AF_W2_END: địa chỉ lệch 0x8554, dòng bảng 1038, kiểu truy cập RW
 * Điểm kết thúc không gồm đầu mút theo pixel đủ độ phân giải; cập nhật theo profile, MAS v1.07
 * tuning-af.
 */
#define FX1_ISP_TUNE_AF_W2_END_OFFSET 0x8554U
#define REG_TUNE_AF_W2_END FX1_ISP_TUNE_AF_W2_END_OFFSET
#define FX1_ISP_TUNE_AF_W2_END_RESET 0x00000000U
#define FX1_ISP_TUNE_AF_W2_END_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_TUNE_AF_W2_END_RO_MASK 0x00000000U
#define FX1_ISP_TUNE_AF_W2_END_RW_MASK 0xFFFFFFFFU
#define FX1_ISP_TUNE_AF_W2_END_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_AF_W2_END_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_AF_W2_END_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_AF_W2_END_W1SC_MASK 0x00000000U
/**
 * X [15:0], dòng bảng 1039, kiểu truy cập RW
 * Tọa độ ngang của đầu gồm đầu mút hoặc cuối không gồm đầu mút.
 */
#define FX1_ISP_TUNE_AF_W2_END_X_SHIFT 0U
#define FX1_ISP_TUNE_AF_W2_END_X_WIDTH 16U
#define FX1_ISP_TUNE_AF_W2_END_X_MASK 0x0000FFFFU
#define FX1_ISP_TUNE_AF_W2_END_X_RESET 0x00000000U
#define FX1_ISP_TUNE_AF_W2_END_X_ACCESS FX1_ISP_ACCESS_RW
/**
 * Y [31:16], dòng bảng 1040, kiểu truy cập RW
 * Tọa độ dọc của đầu gồm đầu mút hoặc cuối không gồm đầu mút.
 */
#define FX1_ISP_TUNE_AF_W2_END_Y_SHIFT 16U
#define FX1_ISP_TUNE_AF_W2_END_Y_WIDTH 16U
#define FX1_ISP_TUNE_AF_W2_END_Y_MASK 0xFFFF0000U
#define FX1_ISP_TUNE_AF_W2_END_Y_RESET 0x00000000U
#define FX1_ISP_TUNE_AF_W2_END_Y_ACCESS FX1_ISP_ACCESS_RW

/**
 * TUNE_AF_W3_START: địa chỉ lệch 0x8558, dòng bảng 1041, kiểu truy cập RW
 * Điểm bắt đầu gồm đầu mút theo pixel đủ độ phân giải; cập nhật theo profile, MAS v1.07 tuning-af.
 */
#define FX1_ISP_TUNE_AF_W3_START_OFFSET 0x8558U
#define REG_TUNE_AF_W3_START FX1_ISP_TUNE_AF_W3_START_OFFSET
#define FX1_ISP_TUNE_AF_W3_START_RESET 0x00000000U
#define FX1_ISP_TUNE_AF_W3_START_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_TUNE_AF_W3_START_RO_MASK 0x00000000U
#define FX1_ISP_TUNE_AF_W3_START_RW_MASK 0xFFFFFFFFU
#define FX1_ISP_TUNE_AF_W3_START_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_AF_W3_START_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_AF_W3_START_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_AF_W3_START_W1SC_MASK 0x00000000U
/**
 * X [15:0], dòng bảng 1042, kiểu truy cập RW
 * Tọa độ ngang của đầu gồm đầu mút hoặc cuối không gồm đầu mút.
 */
#define FX1_ISP_TUNE_AF_W3_START_X_SHIFT 0U
#define FX1_ISP_TUNE_AF_W3_START_X_WIDTH 16U
#define FX1_ISP_TUNE_AF_W3_START_X_MASK 0x0000FFFFU
#define FX1_ISP_TUNE_AF_W3_START_X_RESET 0x00000000U
#define FX1_ISP_TUNE_AF_W3_START_X_ACCESS FX1_ISP_ACCESS_RW
/**
 * Y [31:16], dòng bảng 1043, kiểu truy cập RW
 * Tọa độ dọc của đầu gồm đầu mút hoặc cuối không gồm đầu mút.
 */
#define FX1_ISP_TUNE_AF_W3_START_Y_SHIFT 16U
#define FX1_ISP_TUNE_AF_W3_START_Y_WIDTH 16U
#define FX1_ISP_TUNE_AF_W3_START_Y_MASK 0xFFFF0000U
#define FX1_ISP_TUNE_AF_W3_START_Y_RESET 0x00000000U
#define FX1_ISP_TUNE_AF_W3_START_Y_ACCESS FX1_ISP_ACCESS_RW

/**
 * TUNE_AF_W3_END: địa chỉ lệch 0x855C, dòng bảng 1044, kiểu truy cập RW
 * Điểm kết thúc không gồm đầu mút theo pixel đủ độ phân giải; cập nhật theo profile, MAS v1.07
 * tuning-af.
 */
#define FX1_ISP_TUNE_AF_W3_END_OFFSET 0x855CU
#define REG_TUNE_AF_W3_END FX1_ISP_TUNE_AF_W3_END_OFFSET
#define FX1_ISP_TUNE_AF_W3_END_RESET 0x00000000U
#define FX1_ISP_TUNE_AF_W3_END_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_TUNE_AF_W3_END_RO_MASK 0x00000000U
#define FX1_ISP_TUNE_AF_W3_END_RW_MASK 0xFFFFFFFFU
#define FX1_ISP_TUNE_AF_W3_END_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_AF_W3_END_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_AF_W3_END_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_AF_W3_END_W1SC_MASK 0x00000000U
/**
 * X [15:0], dòng bảng 1045, kiểu truy cập RW
 * Tọa độ ngang của đầu gồm đầu mút hoặc cuối không gồm đầu mút.
 */
#define FX1_ISP_TUNE_AF_W3_END_X_SHIFT 0U
#define FX1_ISP_TUNE_AF_W3_END_X_WIDTH 16U
#define FX1_ISP_TUNE_AF_W3_END_X_MASK 0x0000FFFFU
#define FX1_ISP_TUNE_AF_W3_END_X_RESET 0x00000000U
#define FX1_ISP_TUNE_AF_W3_END_X_ACCESS FX1_ISP_ACCESS_RW
/**
 * Y [31:16], dòng bảng 1046, kiểu truy cập RW
 * Tọa độ dọc của đầu gồm đầu mút hoặc cuối không gồm đầu mút.
 */
#define FX1_ISP_TUNE_AF_W3_END_Y_SHIFT 16U
#define FX1_ISP_TUNE_AF_W3_END_Y_WIDTH 16U
#define FX1_ISP_TUNE_AF_W3_END_Y_MASK 0xFFFF0000U
#define FX1_ISP_TUNE_AF_W3_END_Y_RESET 0x00000000U
#define FX1_ISP_TUNE_AF_W3_END_Y_ACCESS FX1_ISP_ACCESS_RW

/**
 * TUNE_AF_W4_START: địa chỉ lệch 0x8560, dòng bảng 1047, kiểu truy cập RW
 * Điểm bắt đầu gồm đầu mút theo pixel đủ độ phân giải; cập nhật theo profile, MAS v1.07 tuning-af.
 */
#define FX1_ISP_TUNE_AF_W4_START_OFFSET 0x8560U
#define REG_TUNE_AF_W4_START FX1_ISP_TUNE_AF_W4_START_OFFSET
#define FX1_ISP_TUNE_AF_W4_START_RESET 0x00000000U
#define FX1_ISP_TUNE_AF_W4_START_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_TUNE_AF_W4_START_RO_MASK 0x00000000U
#define FX1_ISP_TUNE_AF_W4_START_RW_MASK 0xFFFFFFFFU
#define FX1_ISP_TUNE_AF_W4_START_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_AF_W4_START_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_AF_W4_START_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_AF_W4_START_W1SC_MASK 0x00000000U
/**
 * X [15:0], dòng bảng 1048, kiểu truy cập RW
 * Tọa độ ngang của đầu gồm đầu mút hoặc cuối không gồm đầu mút.
 */
#define FX1_ISP_TUNE_AF_W4_START_X_SHIFT 0U
#define FX1_ISP_TUNE_AF_W4_START_X_WIDTH 16U
#define FX1_ISP_TUNE_AF_W4_START_X_MASK 0x0000FFFFU
#define FX1_ISP_TUNE_AF_W4_START_X_RESET 0x00000000U
#define FX1_ISP_TUNE_AF_W4_START_X_ACCESS FX1_ISP_ACCESS_RW
/**
 * Y [31:16], dòng bảng 1049, kiểu truy cập RW
 * Tọa độ dọc của đầu gồm đầu mút hoặc cuối không gồm đầu mút.
 */
#define FX1_ISP_TUNE_AF_W4_START_Y_SHIFT 16U
#define FX1_ISP_TUNE_AF_W4_START_Y_WIDTH 16U
#define FX1_ISP_TUNE_AF_W4_START_Y_MASK 0xFFFF0000U
#define FX1_ISP_TUNE_AF_W4_START_Y_RESET 0x00000000U
#define FX1_ISP_TUNE_AF_W4_START_Y_ACCESS FX1_ISP_ACCESS_RW

/**
 * TUNE_AF_W4_END: địa chỉ lệch 0x8564, dòng bảng 1050, kiểu truy cập RW
 * Điểm kết thúc không gồm đầu mút theo pixel đủ độ phân giải; cập nhật theo profile, MAS v1.07
 * tuning-af.
 */
#define FX1_ISP_TUNE_AF_W4_END_OFFSET 0x8564U
#define REG_TUNE_AF_W4_END FX1_ISP_TUNE_AF_W4_END_OFFSET
#define FX1_ISP_TUNE_AF_W4_END_RESET 0x00000000U
#define FX1_ISP_TUNE_AF_W4_END_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_TUNE_AF_W4_END_RO_MASK 0x00000000U
#define FX1_ISP_TUNE_AF_W4_END_RW_MASK 0xFFFFFFFFU
#define FX1_ISP_TUNE_AF_W4_END_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_AF_W4_END_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_AF_W4_END_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_AF_W4_END_W1SC_MASK 0x00000000U
/**
 * X [15:0], dòng bảng 1051, kiểu truy cập RW
 * Tọa độ ngang của đầu gồm đầu mút hoặc cuối không gồm đầu mút.
 */
#define FX1_ISP_TUNE_AF_W4_END_X_SHIFT 0U
#define FX1_ISP_TUNE_AF_W4_END_X_WIDTH 16U
#define FX1_ISP_TUNE_AF_W4_END_X_MASK 0x0000FFFFU
#define FX1_ISP_TUNE_AF_W4_END_X_RESET 0x00000000U
#define FX1_ISP_TUNE_AF_W4_END_X_ACCESS FX1_ISP_ACCESS_RW
/**
 * Y [31:16], dòng bảng 1052, kiểu truy cập RW
 * Tọa độ dọc của đầu gồm đầu mút hoặc cuối không gồm đầu mút.
 */
#define FX1_ISP_TUNE_AF_W4_END_Y_SHIFT 16U
#define FX1_ISP_TUNE_AF_W4_END_Y_WIDTH 16U
#define FX1_ISP_TUNE_AF_W4_END_Y_MASK 0xFFFF0000U
#define FX1_ISP_TUNE_AF_W4_END_Y_RESET 0x00000000U
#define FX1_ISP_TUNE_AF_W4_END_Y_ACCESS FX1_ISP_ACCESS_RW

/**
 * TUNE_AF_W5_START: địa chỉ lệch 0x8568, dòng bảng 1053, kiểu truy cập RW
 * Điểm bắt đầu gồm đầu mút theo pixel đủ độ phân giải; cập nhật theo profile, MAS v1.07 tuning-af.
 */
#define FX1_ISP_TUNE_AF_W5_START_OFFSET 0x8568U
#define REG_TUNE_AF_W5_START FX1_ISP_TUNE_AF_W5_START_OFFSET
#define FX1_ISP_TUNE_AF_W5_START_RESET 0x00000000U
#define FX1_ISP_TUNE_AF_W5_START_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_TUNE_AF_W5_START_RO_MASK 0x00000000U
#define FX1_ISP_TUNE_AF_W5_START_RW_MASK 0xFFFFFFFFU
#define FX1_ISP_TUNE_AF_W5_START_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_AF_W5_START_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_AF_W5_START_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_AF_W5_START_W1SC_MASK 0x00000000U
/**
 * X [15:0], dòng bảng 1054, kiểu truy cập RW
 * Tọa độ ngang của đầu gồm đầu mút hoặc cuối không gồm đầu mút.
 */
#define FX1_ISP_TUNE_AF_W5_START_X_SHIFT 0U
#define FX1_ISP_TUNE_AF_W5_START_X_WIDTH 16U
#define FX1_ISP_TUNE_AF_W5_START_X_MASK 0x0000FFFFU
#define FX1_ISP_TUNE_AF_W5_START_X_RESET 0x00000000U
#define FX1_ISP_TUNE_AF_W5_START_X_ACCESS FX1_ISP_ACCESS_RW
/**
 * Y [31:16], dòng bảng 1055, kiểu truy cập RW
 * Tọa độ dọc của đầu gồm đầu mút hoặc cuối không gồm đầu mút.
 */
#define FX1_ISP_TUNE_AF_W5_START_Y_SHIFT 16U
#define FX1_ISP_TUNE_AF_W5_START_Y_WIDTH 16U
#define FX1_ISP_TUNE_AF_W5_START_Y_MASK 0xFFFF0000U
#define FX1_ISP_TUNE_AF_W5_START_Y_RESET 0x00000000U
#define FX1_ISP_TUNE_AF_W5_START_Y_ACCESS FX1_ISP_ACCESS_RW

/**
 * TUNE_AF_W5_END: địa chỉ lệch 0x856C, dòng bảng 1056, kiểu truy cập RW
 * Điểm kết thúc không gồm đầu mút theo pixel đủ độ phân giải; cập nhật theo profile, MAS v1.07
 * tuning-af.
 */
#define FX1_ISP_TUNE_AF_W5_END_OFFSET 0x856CU
#define REG_TUNE_AF_W5_END FX1_ISP_TUNE_AF_W5_END_OFFSET
#define FX1_ISP_TUNE_AF_W5_END_RESET 0x00000000U
#define FX1_ISP_TUNE_AF_W5_END_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_TUNE_AF_W5_END_RO_MASK 0x00000000U
#define FX1_ISP_TUNE_AF_W5_END_RW_MASK 0xFFFFFFFFU
#define FX1_ISP_TUNE_AF_W5_END_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_AF_W5_END_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_AF_W5_END_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_AF_W5_END_W1SC_MASK 0x00000000U
/**
 * X [15:0], dòng bảng 1057, kiểu truy cập RW
 * Tọa độ ngang của đầu gồm đầu mút hoặc cuối không gồm đầu mút.
 */
#define FX1_ISP_TUNE_AF_W5_END_X_SHIFT 0U
#define FX1_ISP_TUNE_AF_W5_END_X_WIDTH 16U
#define FX1_ISP_TUNE_AF_W5_END_X_MASK 0x0000FFFFU
#define FX1_ISP_TUNE_AF_W5_END_X_RESET 0x00000000U
#define FX1_ISP_TUNE_AF_W5_END_X_ACCESS FX1_ISP_ACCESS_RW
/**
 * Y [31:16], dòng bảng 1058, kiểu truy cập RW
 * Tọa độ dọc của đầu gồm đầu mút hoặc cuối không gồm đầu mút.
 */
#define FX1_ISP_TUNE_AF_W5_END_Y_SHIFT 16U
#define FX1_ISP_TUNE_AF_W5_END_Y_WIDTH 16U
#define FX1_ISP_TUNE_AF_W5_END_Y_MASK 0xFFFF0000U
#define FX1_ISP_TUNE_AF_W5_END_Y_RESET 0x00000000U
#define FX1_ISP_TUNE_AF_W5_END_Y_ACCESS FX1_ISP_ACCESS_RW

/**
 * TUNE_AF_W6_START: địa chỉ lệch 0x8570, dòng bảng 1059, kiểu truy cập RW
 * Điểm bắt đầu gồm đầu mút theo pixel đủ độ phân giải; cập nhật theo profile, MAS v1.07 tuning-af.
 */
#define FX1_ISP_TUNE_AF_W6_START_OFFSET 0x8570U
#define REG_TUNE_AF_W6_START FX1_ISP_TUNE_AF_W6_START_OFFSET
#define FX1_ISP_TUNE_AF_W6_START_RESET 0x00000000U
#define FX1_ISP_TUNE_AF_W6_START_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_TUNE_AF_W6_START_RO_MASK 0x00000000U
#define FX1_ISP_TUNE_AF_W6_START_RW_MASK 0xFFFFFFFFU
#define FX1_ISP_TUNE_AF_W6_START_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_AF_W6_START_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_AF_W6_START_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_AF_W6_START_W1SC_MASK 0x00000000U
/**
 * X [15:0], dòng bảng 1060, kiểu truy cập RW
 * Tọa độ ngang của đầu gồm đầu mút hoặc cuối không gồm đầu mút.
 */
#define FX1_ISP_TUNE_AF_W6_START_X_SHIFT 0U
#define FX1_ISP_TUNE_AF_W6_START_X_WIDTH 16U
#define FX1_ISP_TUNE_AF_W6_START_X_MASK 0x0000FFFFU
#define FX1_ISP_TUNE_AF_W6_START_X_RESET 0x00000000U
#define FX1_ISP_TUNE_AF_W6_START_X_ACCESS FX1_ISP_ACCESS_RW
/**
 * Y [31:16], dòng bảng 1061, kiểu truy cập RW
 * Tọa độ dọc của đầu gồm đầu mút hoặc cuối không gồm đầu mút.
 */
#define FX1_ISP_TUNE_AF_W6_START_Y_SHIFT 16U
#define FX1_ISP_TUNE_AF_W6_START_Y_WIDTH 16U
#define FX1_ISP_TUNE_AF_W6_START_Y_MASK 0xFFFF0000U
#define FX1_ISP_TUNE_AF_W6_START_Y_RESET 0x00000000U
#define FX1_ISP_TUNE_AF_W6_START_Y_ACCESS FX1_ISP_ACCESS_RW

/**
 * TUNE_AF_W6_END: địa chỉ lệch 0x8574, dòng bảng 1062, kiểu truy cập RW
 * Điểm kết thúc không gồm đầu mút theo pixel đủ độ phân giải; cập nhật theo profile, MAS v1.07
 * tuning-af.
 */
#define FX1_ISP_TUNE_AF_W6_END_OFFSET 0x8574U
#define REG_TUNE_AF_W6_END FX1_ISP_TUNE_AF_W6_END_OFFSET
#define FX1_ISP_TUNE_AF_W6_END_RESET 0x00000000U
#define FX1_ISP_TUNE_AF_W6_END_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_TUNE_AF_W6_END_RO_MASK 0x00000000U
#define FX1_ISP_TUNE_AF_W6_END_RW_MASK 0xFFFFFFFFU
#define FX1_ISP_TUNE_AF_W6_END_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_AF_W6_END_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_AF_W6_END_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_AF_W6_END_W1SC_MASK 0x00000000U
/**
 * X [15:0], dòng bảng 1063, kiểu truy cập RW
 * Tọa độ ngang của đầu gồm đầu mút hoặc cuối không gồm đầu mút.
 */
#define FX1_ISP_TUNE_AF_W6_END_X_SHIFT 0U
#define FX1_ISP_TUNE_AF_W6_END_X_WIDTH 16U
#define FX1_ISP_TUNE_AF_W6_END_X_MASK 0x0000FFFFU
#define FX1_ISP_TUNE_AF_W6_END_X_RESET 0x00000000U
#define FX1_ISP_TUNE_AF_W6_END_X_ACCESS FX1_ISP_ACCESS_RW
/**
 * Y [31:16], dòng bảng 1064, kiểu truy cập RW
 * Tọa độ dọc của đầu gồm đầu mút hoặc cuối không gồm đầu mút.
 */
#define FX1_ISP_TUNE_AF_W6_END_Y_SHIFT 16U
#define FX1_ISP_TUNE_AF_W6_END_Y_WIDTH 16U
#define FX1_ISP_TUNE_AF_W6_END_Y_MASK 0xFFFF0000U
#define FX1_ISP_TUNE_AF_W6_END_Y_RESET 0x00000000U
#define FX1_ISP_TUNE_AF_W6_END_Y_ACCESS FX1_ISP_ACCESS_RW

/**
 * TUNE_AF_W7_START: địa chỉ lệch 0x8578, dòng bảng 1065, kiểu truy cập RW
 * Điểm bắt đầu gồm đầu mút theo pixel đủ độ phân giải; cập nhật theo profile, MAS v1.07 tuning-af.
 */
#define FX1_ISP_TUNE_AF_W7_START_OFFSET 0x8578U
#define REG_TUNE_AF_W7_START FX1_ISP_TUNE_AF_W7_START_OFFSET
#define FX1_ISP_TUNE_AF_W7_START_RESET 0x00000000U
#define FX1_ISP_TUNE_AF_W7_START_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_TUNE_AF_W7_START_RO_MASK 0x00000000U
#define FX1_ISP_TUNE_AF_W7_START_RW_MASK 0xFFFFFFFFU
#define FX1_ISP_TUNE_AF_W7_START_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_AF_W7_START_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_AF_W7_START_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_AF_W7_START_W1SC_MASK 0x00000000U
/**
 * X [15:0], dòng bảng 1066, kiểu truy cập RW
 * Tọa độ ngang của đầu gồm đầu mút hoặc cuối không gồm đầu mút.
 */
#define FX1_ISP_TUNE_AF_W7_START_X_SHIFT 0U
#define FX1_ISP_TUNE_AF_W7_START_X_WIDTH 16U
#define FX1_ISP_TUNE_AF_W7_START_X_MASK 0x0000FFFFU
#define FX1_ISP_TUNE_AF_W7_START_X_RESET 0x00000000U
#define FX1_ISP_TUNE_AF_W7_START_X_ACCESS FX1_ISP_ACCESS_RW
/**
 * Y [31:16], dòng bảng 1067, kiểu truy cập RW
 * Tọa độ dọc của đầu gồm đầu mút hoặc cuối không gồm đầu mút.
 */
#define FX1_ISP_TUNE_AF_W7_START_Y_SHIFT 16U
#define FX1_ISP_TUNE_AF_W7_START_Y_WIDTH 16U
#define FX1_ISP_TUNE_AF_W7_START_Y_MASK 0xFFFF0000U
#define FX1_ISP_TUNE_AF_W7_START_Y_RESET 0x00000000U
#define FX1_ISP_TUNE_AF_W7_START_Y_ACCESS FX1_ISP_ACCESS_RW

/**
 * TUNE_AF_W7_END: địa chỉ lệch 0x857C, dòng bảng 1068, kiểu truy cập RW
 * Điểm kết thúc không gồm đầu mút theo pixel đủ độ phân giải; cập nhật theo profile, MAS v1.07
 * tuning-af.
 */
#define FX1_ISP_TUNE_AF_W7_END_OFFSET 0x857CU
#define REG_TUNE_AF_W7_END FX1_ISP_TUNE_AF_W7_END_OFFSET
#define FX1_ISP_TUNE_AF_W7_END_RESET 0x00000000U
#define FX1_ISP_TUNE_AF_W7_END_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_TUNE_AF_W7_END_RO_MASK 0x00000000U
#define FX1_ISP_TUNE_AF_W7_END_RW_MASK 0xFFFFFFFFU
#define FX1_ISP_TUNE_AF_W7_END_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_AF_W7_END_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_AF_W7_END_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_AF_W7_END_W1SC_MASK 0x00000000U
/**
 * X [15:0], dòng bảng 1069, kiểu truy cập RW
 * Tọa độ ngang của đầu gồm đầu mút hoặc cuối không gồm đầu mút.
 */
#define FX1_ISP_TUNE_AF_W7_END_X_SHIFT 0U
#define FX1_ISP_TUNE_AF_W7_END_X_WIDTH 16U
#define FX1_ISP_TUNE_AF_W7_END_X_MASK 0x0000FFFFU
#define FX1_ISP_TUNE_AF_W7_END_X_RESET 0x00000000U
#define FX1_ISP_TUNE_AF_W7_END_X_ACCESS FX1_ISP_ACCESS_RW
/**
 * Y [31:16], dòng bảng 1070, kiểu truy cập RW
 * Tọa độ dọc của đầu gồm đầu mút hoặc cuối không gồm đầu mút.
 */
#define FX1_ISP_TUNE_AF_W7_END_Y_SHIFT 16U
#define FX1_ISP_TUNE_AF_W7_END_Y_WIDTH 16U
#define FX1_ISP_TUNE_AF_W7_END_Y_MASK 0xFFFF0000U
#define FX1_ISP_TUNE_AF_W7_END_Y_RESET 0x00000000U
#define FX1_ISP_TUNE_AF_W7_END_Y_ACCESS FX1_ISP_ACCESS_RW

/**
 * TUNE_AF_W8_START: địa chỉ lệch 0x8580, dòng bảng 1071, kiểu truy cập RW
 * Điểm bắt đầu gồm đầu mút theo pixel đủ độ phân giải; cập nhật theo profile, MAS v1.07 tuning-af.
 */
#define FX1_ISP_TUNE_AF_W8_START_OFFSET 0x8580U
#define REG_TUNE_AF_W8_START FX1_ISP_TUNE_AF_W8_START_OFFSET
#define FX1_ISP_TUNE_AF_W8_START_RESET 0x00000000U
#define FX1_ISP_TUNE_AF_W8_START_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_TUNE_AF_W8_START_RO_MASK 0x00000000U
#define FX1_ISP_TUNE_AF_W8_START_RW_MASK 0xFFFFFFFFU
#define FX1_ISP_TUNE_AF_W8_START_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_AF_W8_START_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_AF_W8_START_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_AF_W8_START_W1SC_MASK 0x00000000U
/**
 * X [15:0], dòng bảng 1072, kiểu truy cập RW
 * Tọa độ ngang của đầu gồm đầu mút hoặc cuối không gồm đầu mút.
 */
#define FX1_ISP_TUNE_AF_W8_START_X_SHIFT 0U
#define FX1_ISP_TUNE_AF_W8_START_X_WIDTH 16U
#define FX1_ISP_TUNE_AF_W8_START_X_MASK 0x0000FFFFU
#define FX1_ISP_TUNE_AF_W8_START_X_RESET 0x00000000U
#define FX1_ISP_TUNE_AF_W8_START_X_ACCESS FX1_ISP_ACCESS_RW
/**
 * Y [31:16], dòng bảng 1073, kiểu truy cập RW
 * Tọa độ dọc của đầu gồm đầu mút hoặc cuối không gồm đầu mút.
 */
#define FX1_ISP_TUNE_AF_W8_START_Y_SHIFT 16U
#define FX1_ISP_TUNE_AF_W8_START_Y_WIDTH 16U
#define FX1_ISP_TUNE_AF_W8_START_Y_MASK 0xFFFF0000U
#define FX1_ISP_TUNE_AF_W8_START_Y_RESET 0x00000000U
#define FX1_ISP_TUNE_AF_W8_START_Y_ACCESS FX1_ISP_ACCESS_RW

/**
 * TUNE_AF_W8_END: địa chỉ lệch 0x8584, dòng bảng 1074, kiểu truy cập RW
 * Điểm kết thúc không gồm đầu mút theo pixel đủ độ phân giải; cập nhật theo profile, MAS v1.07
 * tuning-af.
 */
#define FX1_ISP_TUNE_AF_W8_END_OFFSET 0x8584U
#define REG_TUNE_AF_W8_END FX1_ISP_TUNE_AF_W8_END_OFFSET
#define FX1_ISP_TUNE_AF_W8_END_RESET 0x00000000U
#define FX1_ISP_TUNE_AF_W8_END_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_TUNE_AF_W8_END_RO_MASK 0x00000000U
#define FX1_ISP_TUNE_AF_W8_END_RW_MASK 0xFFFFFFFFU
#define FX1_ISP_TUNE_AF_W8_END_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_AF_W8_END_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_AF_W8_END_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_AF_W8_END_W1SC_MASK 0x00000000U
/**
 * X [15:0], dòng bảng 1075, kiểu truy cập RW
 * Tọa độ ngang của đầu gồm đầu mút hoặc cuối không gồm đầu mút.
 */
#define FX1_ISP_TUNE_AF_W8_END_X_SHIFT 0U
#define FX1_ISP_TUNE_AF_W8_END_X_WIDTH 16U
#define FX1_ISP_TUNE_AF_W8_END_X_MASK 0x0000FFFFU
#define FX1_ISP_TUNE_AF_W8_END_X_RESET 0x00000000U
#define FX1_ISP_TUNE_AF_W8_END_X_ACCESS FX1_ISP_ACCESS_RW
/**
 * Y [31:16], dòng bảng 1076, kiểu truy cập RW
 * Tọa độ dọc của đầu gồm đầu mút hoặc cuối không gồm đầu mút.
 */
#define FX1_ISP_TUNE_AF_W8_END_Y_SHIFT 16U
#define FX1_ISP_TUNE_AF_W8_END_Y_WIDTH 16U
#define FX1_ISP_TUNE_AF_W8_END_Y_MASK 0xFFFF0000U
#define FX1_ISP_TUNE_AF_W8_END_Y_RESET 0x00000000U
#define FX1_ISP_TUNE_AF_W8_END_Y_ACCESS FX1_ISP_ACCESS_RW

/**
 * TUNE_AF_W9_START: địa chỉ lệch 0x8588, dòng bảng 1077, kiểu truy cập RW
 * Điểm bắt đầu gồm đầu mút theo pixel đủ độ phân giải; cập nhật theo profile, MAS v1.07 tuning-af.
 */
#define FX1_ISP_TUNE_AF_W9_START_OFFSET 0x8588U
#define REG_TUNE_AF_W9_START FX1_ISP_TUNE_AF_W9_START_OFFSET
#define FX1_ISP_TUNE_AF_W9_START_RESET 0x00000000U
#define FX1_ISP_TUNE_AF_W9_START_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_TUNE_AF_W9_START_RO_MASK 0x00000000U
#define FX1_ISP_TUNE_AF_W9_START_RW_MASK 0xFFFFFFFFU
#define FX1_ISP_TUNE_AF_W9_START_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_AF_W9_START_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_AF_W9_START_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_AF_W9_START_W1SC_MASK 0x00000000U
/**
 * X [15:0], dòng bảng 1078, kiểu truy cập RW
 * Tọa độ ngang của đầu gồm đầu mút hoặc cuối không gồm đầu mút.
 */
#define FX1_ISP_TUNE_AF_W9_START_X_SHIFT 0U
#define FX1_ISP_TUNE_AF_W9_START_X_WIDTH 16U
#define FX1_ISP_TUNE_AF_W9_START_X_MASK 0x0000FFFFU
#define FX1_ISP_TUNE_AF_W9_START_X_RESET 0x00000000U
#define FX1_ISP_TUNE_AF_W9_START_X_ACCESS FX1_ISP_ACCESS_RW
/**
 * Y [31:16], dòng bảng 1079, kiểu truy cập RW
 * Tọa độ dọc của đầu gồm đầu mút hoặc cuối không gồm đầu mút.
 */
#define FX1_ISP_TUNE_AF_W9_START_Y_SHIFT 16U
#define FX1_ISP_TUNE_AF_W9_START_Y_WIDTH 16U
#define FX1_ISP_TUNE_AF_W9_START_Y_MASK 0xFFFF0000U
#define FX1_ISP_TUNE_AF_W9_START_Y_RESET 0x00000000U
#define FX1_ISP_TUNE_AF_W9_START_Y_ACCESS FX1_ISP_ACCESS_RW

/**
 * TUNE_AF_W9_END: địa chỉ lệch 0x858C, dòng bảng 1080, kiểu truy cập RW
 * Điểm kết thúc không gồm đầu mút theo pixel đủ độ phân giải; cập nhật theo profile, MAS v1.07
 * tuning-af.
 */
#define FX1_ISP_TUNE_AF_W9_END_OFFSET 0x858CU
#define REG_TUNE_AF_W9_END FX1_ISP_TUNE_AF_W9_END_OFFSET
#define FX1_ISP_TUNE_AF_W9_END_RESET 0x00000000U
#define FX1_ISP_TUNE_AF_W9_END_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_TUNE_AF_W9_END_RO_MASK 0x00000000U
#define FX1_ISP_TUNE_AF_W9_END_RW_MASK 0xFFFFFFFFU
#define FX1_ISP_TUNE_AF_W9_END_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_AF_W9_END_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_AF_W9_END_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_AF_W9_END_W1SC_MASK 0x00000000U
/**
 * X [15:0], dòng bảng 1081, kiểu truy cập RW
 * Tọa độ ngang của đầu gồm đầu mút hoặc cuối không gồm đầu mút.
 */
#define FX1_ISP_TUNE_AF_W9_END_X_SHIFT 0U
#define FX1_ISP_TUNE_AF_W9_END_X_WIDTH 16U
#define FX1_ISP_TUNE_AF_W9_END_X_MASK 0x0000FFFFU
#define FX1_ISP_TUNE_AF_W9_END_X_RESET 0x00000000U
#define FX1_ISP_TUNE_AF_W9_END_X_ACCESS FX1_ISP_ACCESS_RW
/**
 * Y [31:16], dòng bảng 1082, kiểu truy cập RW
 * Tọa độ dọc của đầu gồm đầu mút hoặc cuối không gồm đầu mút.
 */
#define FX1_ISP_TUNE_AF_W9_END_Y_SHIFT 16U
#define FX1_ISP_TUNE_AF_W9_END_Y_WIDTH 16U
#define FX1_ISP_TUNE_AF_W9_END_Y_MASK 0xFFFF0000U
#define FX1_ISP_TUNE_AF_W9_END_Y_RESET 0x00000000U
#define FX1_ISP_TUNE_AF_W9_END_Y_ACCESS FX1_ISP_ACCESS_RW

/**
 * TUNE_AF_W10_START: địa chỉ lệch 0x8590, dòng bảng 1083, kiểu truy cập RW
 * Điểm bắt đầu gồm đầu mút theo pixel đủ độ phân giải; cập nhật theo profile, MAS v1.07 tuning-af.
 */
#define FX1_ISP_TUNE_AF_W10_START_OFFSET 0x8590U
#define REG_TUNE_AF_W10_START FX1_ISP_TUNE_AF_W10_START_OFFSET
#define FX1_ISP_TUNE_AF_W10_START_RESET 0x00000000U
#define FX1_ISP_TUNE_AF_W10_START_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_TUNE_AF_W10_START_RO_MASK 0x00000000U
#define FX1_ISP_TUNE_AF_W10_START_RW_MASK 0xFFFFFFFFU
#define FX1_ISP_TUNE_AF_W10_START_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_AF_W10_START_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_AF_W10_START_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_AF_W10_START_W1SC_MASK 0x00000000U
/**
 * X [15:0], dòng bảng 1084, kiểu truy cập RW
 * Tọa độ ngang của đầu gồm đầu mút hoặc cuối không gồm đầu mút.
 */
#define FX1_ISP_TUNE_AF_W10_START_X_SHIFT 0U
#define FX1_ISP_TUNE_AF_W10_START_X_WIDTH 16U
#define FX1_ISP_TUNE_AF_W10_START_X_MASK 0x0000FFFFU
#define FX1_ISP_TUNE_AF_W10_START_X_RESET 0x00000000U
#define FX1_ISP_TUNE_AF_W10_START_X_ACCESS FX1_ISP_ACCESS_RW
/**
 * Y [31:16], dòng bảng 1085, kiểu truy cập RW
 * Tọa độ dọc của đầu gồm đầu mút hoặc cuối không gồm đầu mút.
 */
#define FX1_ISP_TUNE_AF_W10_START_Y_SHIFT 16U
#define FX1_ISP_TUNE_AF_W10_START_Y_WIDTH 16U
#define FX1_ISP_TUNE_AF_W10_START_Y_MASK 0xFFFF0000U
#define FX1_ISP_TUNE_AF_W10_START_Y_RESET 0x00000000U
#define FX1_ISP_TUNE_AF_W10_START_Y_ACCESS FX1_ISP_ACCESS_RW

/**
 * TUNE_AF_W10_END: địa chỉ lệch 0x8594, dòng bảng 1086, kiểu truy cập RW
 * Điểm kết thúc không gồm đầu mút theo pixel đủ độ phân giải; cập nhật theo profile, MAS v1.07
 * tuning-af.
 */
#define FX1_ISP_TUNE_AF_W10_END_OFFSET 0x8594U
#define REG_TUNE_AF_W10_END FX1_ISP_TUNE_AF_W10_END_OFFSET
#define FX1_ISP_TUNE_AF_W10_END_RESET 0x00000000U
#define FX1_ISP_TUNE_AF_W10_END_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_TUNE_AF_W10_END_RO_MASK 0x00000000U
#define FX1_ISP_TUNE_AF_W10_END_RW_MASK 0xFFFFFFFFU
#define FX1_ISP_TUNE_AF_W10_END_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_AF_W10_END_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_AF_W10_END_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_AF_W10_END_W1SC_MASK 0x00000000U
/**
 * X [15:0], dòng bảng 1087, kiểu truy cập RW
 * Tọa độ ngang của đầu gồm đầu mút hoặc cuối không gồm đầu mút.
 */
#define FX1_ISP_TUNE_AF_W10_END_X_SHIFT 0U
#define FX1_ISP_TUNE_AF_W10_END_X_WIDTH 16U
#define FX1_ISP_TUNE_AF_W10_END_X_MASK 0x0000FFFFU
#define FX1_ISP_TUNE_AF_W10_END_X_RESET 0x00000000U
#define FX1_ISP_TUNE_AF_W10_END_X_ACCESS FX1_ISP_ACCESS_RW
/**
 * Y [31:16], dòng bảng 1088, kiểu truy cập RW
 * Tọa độ dọc của đầu gồm đầu mút hoặc cuối không gồm đầu mút.
 */
#define FX1_ISP_TUNE_AF_W10_END_Y_SHIFT 16U
#define FX1_ISP_TUNE_AF_W10_END_Y_WIDTH 16U
#define FX1_ISP_TUNE_AF_W10_END_Y_MASK 0xFFFF0000U
#define FX1_ISP_TUNE_AF_W10_END_Y_RESET 0x00000000U
#define FX1_ISP_TUNE_AF_W10_END_Y_ACCESS FX1_ISP_ACCESS_RW

/**
 * TUNE_AF_W11_START: địa chỉ lệch 0x8598, dòng bảng 1089, kiểu truy cập RW
 * Điểm bắt đầu gồm đầu mút theo pixel đủ độ phân giải; cập nhật theo profile, MAS v1.07 tuning-af.
 */
#define FX1_ISP_TUNE_AF_W11_START_OFFSET 0x8598U
#define REG_TUNE_AF_W11_START FX1_ISP_TUNE_AF_W11_START_OFFSET
#define FX1_ISP_TUNE_AF_W11_START_RESET 0x00000000U
#define FX1_ISP_TUNE_AF_W11_START_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_TUNE_AF_W11_START_RO_MASK 0x00000000U
#define FX1_ISP_TUNE_AF_W11_START_RW_MASK 0xFFFFFFFFU
#define FX1_ISP_TUNE_AF_W11_START_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_AF_W11_START_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_AF_W11_START_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_AF_W11_START_W1SC_MASK 0x00000000U
/**
 * X [15:0], dòng bảng 1090, kiểu truy cập RW
 * Tọa độ ngang của đầu gồm đầu mút hoặc cuối không gồm đầu mút.
 */
#define FX1_ISP_TUNE_AF_W11_START_X_SHIFT 0U
#define FX1_ISP_TUNE_AF_W11_START_X_WIDTH 16U
#define FX1_ISP_TUNE_AF_W11_START_X_MASK 0x0000FFFFU
#define FX1_ISP_TUNE_AF_W11_START_X_RESET 0x00000000U
#define FX1_ISP_TUNE_AF_W11_START_X_ACCESS FX1_ISP_ACCESS_RW
/**
 * Y [31:16], dòng bảng 1091, kiểu truy cập RW
 * Tọa độ dọc của đầu gồm đầu mút hoặc cuối không gồm đầu mút.
 */
#define FX1_ISP_TUNE_AF_W11_START_Y_SHIFT 16U
#define FX1_ISP_TUNE_AF_W11_START_Y_WIDTH 16U
#define FX1_ISP_TUNE_AF_W11_START_Y_MASK 0xFFFF0000U
#define FX1_ISP_TUNE_AF_W11_START_Y_RESET 0x00000000U
#define FX1_ISP_TUNE_AF_W11_START_Y_ACCESS FX1_ISP_ACCESS_RW

/**
 * TUNE_AF_W11_END: địa chỉ lệch 0x859C, dòng bảng 1092, kiểu truy cập RW
 * Điểm kết thúc không gồm đầu mút theo pixel đủ độ phân giải; cập nhật theo profile, MAS v1.07
 * tuning-af.
 */
#define FX1_ISP_TUNE_AF_W11_END_OFFSET 0x859CU
#define REG_TUNE_AF_W11_END FX1_ISP_TUNE_AF_W11_END_OFFSET
#define FX1_ISP_TUNE_AF_W11_END_RESET 0x00000000U
#define FX1_ISP_TUNE_AF_W11_END_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_TUNE_AF_W11_END_RO_MASK 0x00000000U
#define FX1_ISP_TUNE_AF_W11_END_RW_MASK 0xFFFFFFFFU
#define FX1_ISP_TUNE_AF_W11_END_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_AF_W11_END_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_AF_W11_END_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_AF_W11_END_W1SC_MASK 0x00000000U
/**
 * X [15:0], dòng bảng 1093, kiểu truy cập RW
 * Tọa độ ngang của đầu gồm đầu mút hoặc cuối không gồm đầu mút.
 */
#define FX1_ISP_TUNE_AF_W11_END_X_SHIFT 0U
#define FX1_ISP_TUNE_AF_W11_END_X_WIDTH 16U
#define FX1_ISP_TUNE_AF_W11_END_X_MASK 0x0000FFFFU
#define FX1_ISP_TUNE_AF_W11_END_X_RESET 0x00000000U
#define FX1_ISP_TUNE_AF_W11_END_X_ACCESS FX1_ISP_ACCESS_RW
/**
 * Y [31:16], dòng bảng 1094, kiểu truy cập RW
 * Tọa độ dọc của đầu gồm đầu mút hoặc cuối không gồm đầu mút.
 */
#define FX1_ISP_TUNE_AF_W11_END_Y_SHIFT 16U
#define FX1_ISP_TUNE_AF_W11_END_Y_WIDTH 16U
#define FX1_ISP_TUNE_AF_W11_END_Y_MASK 0xFFFF0000U
#define FX1_ISP_TUNE_AF_W11_END_Y_RESET 0x00000000U
#define FX1_ISP_TUNE_AF_W11_END_Y_ACCESS FX1_ISP_ACCESS_RW

/**
 * TUNE_AF_W12_START: địa chỉ lệch 0x85A0, dòng bảng 1095, kiểu truy cập RW
 * Điểm bắt đầu gồm đầu mút theo pixel đủ độ phân giải; cập nhật theo profile, MAS v1.07 tuning-af.
 */
#define FX1_ISP_TUNE_AF_W12_START_OFFSET 0x85A0U
#define REG_TUNE_AF_W12_START FX1_ISP_TUNE_AF_W12_START_OFFSET
#define FX1_ISP_TUNE_AF_W12_START_RESET 0x00000000U
#define FX1_ISP_TUNE_AF_W12_START_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_TUNE_AF_W12_START_RO_MASK 0x00000000U
#define FX1_ISP_TUNE_AF_W12_START_RW_MASK 0xFFFFFFFFU
#define FX1_ISP_TUNE_AF_W12_START_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_AF_W12_START_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_AF_W12_START_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_AF_W12_START_W1SC_MASK 0x00000000U
/**
 * X [15:0], dòng bảng 1096, kiểu truy cập RW
 * Tọa độ ngang của đầu gồm đầu mút hoặc cuối không gồm đầu mút.
 */
#define FX1_ISP_TUNE_AF_W12_START_X_SHIFT 0U
#define FX1_ISP_TUNE_AF_W12_START_X_WIDTH 16U
#define FX1_ISP_TUNE_AF_W12_START_X_MASK 0x0000FFFFU
#define FX1_ISP_TUNE_AF_W12_START_X_RESET 0x00000000U
#define FX1_ISP_TUNE_AF_W12_START_X_ACCESS FX1_ISP_ACCESS_RW
/**
 * Y [31:16], dòng bảng 1097, kiểu truy cập RW
 * Tọa độ dọc của đầu gồm đầu mút hoặc cuối không gồm đầu mút.
 */
#define FX1_ISP_TUNE_AF_W12_START_Y_SHIFT 16U
#define FX1_ISP_TUNE_AF_W12_START_Y_WIDTH 16U
#define FX1_ISP_TUNE_AF_W12_START_Y_MASK 0xFFFF0000U
#define FX1_ISP_TUNE_AF_W12_START_Y_RESET 0x00000000U
#define FX1_ISP_TUNE_AF_W12_START_Y_ACCESS FX1_ISP_ACCESS_RW

/**
 * TUNE_AF_W12_END: địa chỉ lệch 0x85A4, dòng bảng 1098, kiểu truy cập RW
 * Điểm kết thúc không gồm đầu mút theo pixel đủ độ phân giải; cập nhật theo profile, MAS v1.07
 * tuning-af.
 */
#define FX1_ISP_TUNE_AF_W12_END_OFFSET 0x85A4U
#define REG_TUNE_AF_W12_END FX1_ISP_TUNE_AF_W12_END_OFFSET
#define FX1_ISP_TUNE_AF_W12_END_RESET 0x00000000U
#define FX1_ISP_TUNE_AF_W12_END_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_TUNE_AF_W12_END_RO_MASK 0x00000000U
#define FX1_ISP_TUNE_AF_W12_END_RW_MASK 0xFFFFFFFFU
#define FX1_ISP_TUNE_AF_W12_END_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_AF_W12_END_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_AF_W12_END_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_AF_W12_END_W1SC_MASK 0x00000000U
/**
 * X [15:0], dòng bảng 1099, kiểu truy cập RW
 * Tọa độ ngang của đầu gồm đầu mút hoặc cuối không gồm đầu mút.
 */
#define FX1_ISP_TUNE_AF_W12_END_X_SHIFT 0U
#define FX1_ISP_TUNE_AF_W12_END_X_WIDTH 16U
#define FX1_ISP_TUNE_AF_W12_END_X_MASK 0x0000FFFFU
#define FX1_ISP_TUNE_AF_W12_END_X_RESET 0x00000000U
#define FX1_ISP_TUNE_AF_W12_END_X_ACCESS FX1_ISP_ACCESS_RW
/**
 * Y [31:16], dòng bảng 1100, kiểu truy cập RW
 * Tọa độ dọc của đầu gồm đầu mút hoặc cuối không gồm đầu mút.
 */
#define FX1_ISP_TUNE_AF_W12_END_Y_SHIFT 16U
#define FX1_ISP_TUNE_AF_W12_END_Y_WIDTH 16U
#define FX1_ISP_TUNE_AF_W12_END_Y_MASK 0xFFFF0000U
#define FX1_ISP_TUNE_AF_W12_END_Y_RESET 0x00000000U
#define FX1_ISP_TUNE_AF_W12_END_Y_ACCESS FX1_ISP_ACCESS_RW

/**
 * TUNE_AF_W13_START: địa chỉ lệch 0x85A8, dòng bảng 1101, kiểu truy cập RW
 * Điểm bắt đầu gồm đầu mút theo pixel đủ độ phân giải; cập nhật theo profile, MAS v1.07 tuning-af.
 */
#define FX1_ISP_TUNE_AF_W13_START_OFFSET 0x85A8U
#define REG_TUNE_AF_W13_START FX1_ISP_TUNE_AF_W13_START_OFFSET
#define FX1_ISP_TUNE_AF_W13_START_RESET 0x00000000U
#define FX1_ISP_TUNE_AF_W13_START_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_TUNE_AF_W13_START_RO_MASK 0x00000000U
#define FX1_ISP_TUNE_AF_W13_START_RW_MASK 0xFFFFFFFFU
#define FX1_ISP_TUNE_AF_W13_START_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_AF_W13_START_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_AF_W13_START_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_AF_W13_START_W1SC_MASK 0x00000000U
/**
 * X [15:0], dòng bảng 1102, kiểu truy cập RW
 * Tọa độ ngang của đầu gồm đầu mút hoặc cuối không gồm đầu mút.
 */
#define FX1_ISP_TUNE_AF_W13_START_X_SHIFT 0U
#define FX1_ISP_TUNE_AF_W13_START_X_WIDTH 16U
#define FX1_ISP_TUNE_AF_W13_START_X_MASK 0x0000FFFFU
#define FX1_ISP_TUNE_AF_W13_START_X_RESET 0x00000000U
#define FX1_ISP_TUNE_AF_W13_START_X_ACCESS FX1_ISP_ACCESS_RW
/**
 * Y [31:16], dòng bảng 1103, kiểu truy cập RW
 * Tọa độ dọc của đầu gồm đầu mút hoặc cuối không gồm đầu mút.
 */
#define FX1_ISP_TUNE_AF_W13_START_Y_SHIFT 16U
#define FX1_ISP_TUNE_AF_W13_START_Y_WIDTH 16U
#define FX1_ISP_TUNE_AF_W13_START_Y_MASK 0xFFFF0000U
#define FX1_ISP_TUNE_AF_W13_START_Y_RESET 0x00000000U
#define FX1_ISP_TUNE_AF_W13_START_Y_ACCESS FX1_ISP_ACCESS_RW

/**
 * TUNE_AF_W13_END: địa chỉ lệch 0x85AC, dòng bảng 1104, kiểu truy cập RW
 * Điểm kết thúc không gồm đầu mút theo pixel đủ độ phân giải; cập nhật theo profile, MAS v1.07
 * tuning-af.
 */
#define FX1_ISP_TUNE_AF_W13_END_OFFSET 0x85ACU
#define REG_TUNE_AF_W13_END FX1_ISP_TUNE_AF_W13_END_OFFSET
#define FX1_ISP_TUNE_AF_W13_END_RESET 0x00000000U
#define FX1_ISP_TUNE_AF_W13_END_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_TUNE_AF_W13_END_RO_MASK 0x00000000U
#define FX1_ISP_TUNE_AF_W13_END_RW_MASK 0xFFFFFFFFU
#define FX1_ISP_TUNE_AF_W13_END_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_AF_W13_END_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_AF_W13_END_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_AF_W13_END_W1SC_MASK 0x00000000U
/**
 * X [15:0], dòng bảng 1105, kiểu truy cập RW
 * Tọa độ ngang của đầu gồm đầu mút hoặc cuối không gồm đầu mút.
 */
#define FX1_ISP_TUNE_AF_W13_END_X_SHIFT 0U
#define FX1_ISP_TUNE_AF_W13_END_X_WIDTH 16U
#define FX1_ISP_TUNE_AF_W13_END_X_MASK 0x0000FFFFU
#define FX1_ISP_TUNE_AF_W13_END_X_RESET 0x00000000U
#define FX1_ISP_TUNE_AF_W13_END_X_ACCESS FX1_ISP_ACCESS_RW
/**
 * Y [31:16], dòng bảng 1106, kiểu truy cập RW
 * Tọa độ dọc của đầu gồm đầu mút hoặc cuối không gồm đầu mút.
 */
#define FX1_ISP_TUNE_AF_W13_END_Y_SHIFT 16U
#define FX1_ISP_TUNE_AF_W13_END_Y_WIDTH 16U
#define FX1_ISP_TUNE_AF_W13_END_Y_MASK 0xFFFF0000U
#define FX1_ISP_TUNE_AF_W13_END_Y_RESET 0x00000000U
#define FX1_ISP_TUNE_AF_W13_END_Y_ACCESS FX1_ISP_ACCESS_RW

/**
 * TUNE_AF_W14_START: địa chỉ lệch 0x85B0, dòng bảng 1107, kiểu truy cập RW
 * Điểm bắt đầu gồm đầu mút theo pixel đủ độ phân giải; cập nhật theo profile, MAS v1.07 tuning-af.
 */
#define FX1_ISP_TUNE_AF_W14_START_OFFSET 0x85B0U
#define REG_TUNE_AF_W14_START FX1_ISP_TUNE_AF_W14_START_OFFSET
#define FX1_ISP_TUNE_AF_W14_START_RESET 0x00000000U
#define FX1_ISP_TUNE_AF_W14_START_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_TUNE_AF_W14_START_RO_MASK 0x00000000U
#define FX1_ISP_TUNE_AF_W14_START_RW_MASK 0xFFFFFFFFU
#define FX1_ISP_TUNE_AF_W14_START_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_AF_W14_START_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_AF_W14_START_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_AF_W14_START_W1SC_MASK 0x00000000U
/**
 * X [15:0], dòng bảng 1108, kiểu truy cập RW
 * Tọa độ ngang của đầu gồm đầu mút hoặc cuối không gồm đầu mút.
 */
#define FX1_ISP_TUNE_AF_W14_START_X_SHIFT 0U
#define FX1_ISP_TUNE_AF_W14_START_X_WIDTH 16U
#define FX1_ISP_TUNE_AF_W14_START_X_MASK 0x0000FFFFU
#define FX1_ISP_TUNE_AF_W14_START_X_RESET 0x00000000U
#define FX1_ISP_TUNE_AF_W14_START_X_ACCESS FX1_ISP_ACCESS_RW
/**
 * Y [31:16], dòng bảng 1109, kiểu truy cập RW
 * Tọa độ dọc của đầu gồm đầu mút hoặc cuối không gồm đầu mút.
 */
#define FX1_ISP_TUNE_AF_W14_START_Y_SHIFT 16U
#define FX1_ISP_TUNE_AF_W14_START_Y_WIDTH 16U
#define FX1_ISP_TUNE_AF_W14_START_Y_MASK 0xFFFF0000U
#define FX1_ISP_TUNE_AF_W14_START_Y_RESET 0x00000000U
#define FX1_ISP_TUNE_AF_W14_START_Y_ACCESS FX1_ISP_ACCESS_RW

/**
 * TUNE_AF_W14_END: địa chỉ lệch 0x85B4, dòng bảng 1110, kiểu truy cập RW
 * Điểm kết thúc không gồm đầu mút theo pixel đủ độ phân giải; cập nhật theo profile, MAS v1.07
 * tuning-af.
 */
#define FX1_ISP_TUNE_AF_W14_END_OFFSET 0x85B4U
#define REG_TUNE_AF_W14_END FX1_ISP_TUNE_AF_W14_END_OFFSET
#define FX1_ISP_TUNE_AF_W14_END_RESET 0x00000000U
#define FX1_ISP_TUNE_AF_W14_END_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_TUNE_AF_W14_END_RO_MASK 0x00000000U
#define FX1_ISP_TUNE_AF_W14_END_RW_MASK 0xFFFFFFFFU
#define FX1_ISP_TUNE_AF_W14_END_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_AF_W14_END_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_AF_W14_END_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_AF_W14_END_W1SC_MASK 0x00000000U
/**
 * X [15:0], dòng bảng 1111, kiểu truy cập RW
 * Tọa độ ngang của đầu gồm đầu mút hoặc cuối không gồm đầu mút.
 */
#define FX1_ISP_TUNE_AF_W14_END_X_SHIFT 0U
#define FX1_ISP_TUNE_AF_W14_END_X_WIDTH 16U
#define FX1_ISP_TUNE_AF_W14_END_X_MASK 0x0000FFFFU
#define FX1_ISP_TUNE_AF_W14_END_X_RESET 0x00000000U
#define FX1_ISP_TUNE_AF_W14_END_X_ACCESS FX1_ISP_ACCESS_RW
/**
 * Y [31:16], dòng bảng 1112, kiểu truy cập RW
 * Tọa độ dọc của đầu gồm đầu mút hoặc cuối không gồm đầu mút.
 */
#define FX1_ISP_TUNE_AF_W14_END_Y_SHIFT 16U
#define FX1_ISP_TUNE_AF_W14_END_Y_WIDTH 16U
#define FX1_ISP_TUNE_AF_W14_END_Y_MASK 0xFFFF0000U
#define FX1_ISP_TUNE_AF_W14_END_Y_RESET 0x00000000U
#define FX1_ISP_TUNE_AF_W14_END_Y_ACCESS FX1_ISP_ACCESS_RW

/**
 * TUNE_AF_W15_START: địa chỉ lệch 0x85B8, dòng bảng 1113, kiểu truy cập RW
 * Điểm bắt đầu gồm đầu mút theo pixel đủ độ phân giải; cập nhật theo profile, MAS v1.07 tuning-af.
 */
#define FX1_ISP_TUNE_AF_W15_START_OFFSET 0x85B8U
#define REG_TUNE_AF_W15_START FX1_ISP_TUNE_AF_W15_START_OFFSET
#define FX1_ISP_TUNE_AF_W15_START_RESET 0x00000000U
#define FX1_ISP_TUNE_AF_W15_START_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_TUNE_AF_W15_START_RO_MASK 0x00000000U
#define FX1_ISP_TUNE_AF_W15_START_RW_MASK 0xFFFFFFFFU
#define FX1_ISP_TUNE_AF_W15_START_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_AF_W15_START_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_AF_W15_START_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_AF_W15_START_W1SC_MASK 0x00000000U
/**
 * X [15:0], dòng bảng 1114, kiểu truy cập RW
 * Tọa độ ngang của đầu gồm đầu mút hoặc cuối không gồm đầu mút.
 */
#define FX1_ISP_TUNE_AF_W15_START_X_SHIFT 0U
#define FX1_ISP_TUNE_AF_W15_START_X_WIDTH 16U
#define FX1_ISP_TUNE_AF_W15_START_X_MASK 0x0000FFFFU
#define FX1_ISP_TUNE_AF_W15_START_X_RESET 0x00000000U
#define FX1_ISP_TUNE_AF_W15_START_X_ACCESS FX1_ISP_ACCESS_RW
/**
 * Y [31:16], dòng bảng 1115, kiểu truy cập RW
 * Tọa độ dọc của đầu gồm đầu mút hoặc cuối không gồm đầu mút.
 */
#define FX1_ISP_TUNE_AF_W15_START_Y_SHIFT 16U
#define FX1_ISP_TUNE_AF_W15_START_Y_WIDTH 16U
#define FX1_ISP_TUNE_AF_W15_START_Y_MASK 0xFFFF0000U
#define FX1_ISP_TUNE_AF_W15_START_Y_RESET 0x00000000U
#define FX1_ISP_TUNE_AF_W15_START_Y_ACCESS FX1_ISP_ACCESS_RW

/**
 * TUNE_AF_W15_END: địa chỉ lệch 0x85BC, dòng bảng 1116, kiểu truy cập RW
 * Điểm kết thúc không gồm đầu mút theo pixel đủ độ phân giải; cập nhật theo profile, MAS v1.07
 * tuning-af.
 */
#define FX1_ISP_TUNE_AF_W15_END_OFFSET 0x85BCU
#define REG_TUNE_AF_W15_END FX1_ISP_TUNE_AF_W15_END_OFFSET
#define FX1_ISP_TUNE_AF_W15_END_RESET 0x00000000U
#define FX1_ISP_TUNE_AF_W15_END_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_TUNE_AF_W15_END_RO_MASK 0x00000000U
#define FX1_ISP_TUNE_AF_W15_END_RW_MASK 0xFFFFFFFFU
#define FX1_ISP_TUNE_AF_W15_END_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_AF_W15_END_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_AF_W15_END_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_AF_W15_END_W1SC_MASK 0x00000000U
/**
 * X [15:0], dòng bảng 1117, kiểu truy cập RW
 * Tọa độ ngang của đầu gồm đầu mút hoặc cuối không gồm đầu mút.
 */
#define FX1_ISP_TUNE_AF_W15_END_X_SHIFT 0U
#define FX1_ISP_TUNE_AF_W15_END_X_WIDTH 16U
#define FX1_ISP_TUNE_AF_W15_END_X_MASK 0x0000FFFFU
#define FX1_ISP_TUNE_AF_W15_END_X_RESET 0x00000000U
#define FX1_ISP_TUNE_AF_W15_END_X_ACCESS FX1_ISP_ACCESS_RW
/**
 * Y [31:16], dòng bảng 1118, kiểu truy cập RW
 * Tọa độ dọc của đầu gồm đầu mút hoặc cuối không gồm đầu mút.
 */
#define FX1_ISP_TUNE_AF_W15_END_Y_SHIFT 16U
#define FX1_ISP_TUNE_AF_W15_END_Y_WIDTH 16U
#define FX1_ISP_TUNE_AF_W15_END_Y_MASK 0xFFFF0000U
#define FX1_ISP_TUNE_AF_W15_END_Y_RESET 0x00000000U
#define FX1_ISP_TUNE_AF_W15_END_Y_ACCESS FX1_ISP_ACCESS_RW

/**
 * TUNE_HIST_CTRL: địa chỉ lệch 0x8600, dòng bảng 1119, kiểu truy cập RW
 * Histogram trọng số 64 bin bổ sung; histogram toàn ảnh legacy không đổi. Cập nhật theo profile, MAS
 * v1.07 tuning-ae.
 */
#define FX1_ISP_TUNE_HIST_CTRL_OFFSET 0x8600U
#define REG_TUNE_HIST_CTRL FX1_ISP_TUNE_HIST_CTRL_OFFSET
#define FX1_ISP_TUNE_HIST_CTRL_RESET 0x00000000U
#define FX1_ISP_TUNE_HIST_CTRL_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_TUNE_HIST_CTRL_RO_MASK 0x00000000U
#define FX1_ISP_TUNE_HIST_CTRL_RW_MASK 0x00000003U
#define FX1_ISP_TUNE_HIST_CTRL_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_HIST_CTRL_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_HIST_CTRL_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_HIST_CTRL_W1SC_MASK 0x00000000U
/**
 * ENABLE [0:0], dòng bảng 1120, kiểu truy cập RW
 * Bật histogram có trọng số.
 */
#define FX1_ISP_TUNE_HIST_CTRL_ENABLE_SHIFT 0U
#define FX1_ISP_TUNE_HIST_CTRL_ENABLE_WIDTH 1U
#define FX1_ISP_TUNE_HIST_CTRL_ENABLE_MASK 0x00000001U
#define FX1_ISP_TUNE_HIST_CTRL_ENABLE_RESET 0x00000000U
#define FX1_ISP_TUNE_HIST_CTRL_ENABLE_ACCESS FX1_ISP_ACCESS_RW
/**
 * ROI_EN [1:1], dòng bảng 1121, kiểu truy cập RW
 * Dùng ROI đã lập trình; nếu tắt thì toàn frame.
 */
#define FX1_ISP_TUNE_HIST_CTRL_ROI_EN_SHIFT 1U
#define FX1_ISP_TUNE_HIST_CTRL_ROI_EN_WIDTH 1U
#define FX1_ISP_TUNE_HIST_CTRL_ROI_EN_MASK 0x00000002U
#define FX1_ISP_TUNE_HIST_CTRL_ROI_EN_RESET 0x00000000U
#define FX1_ISP_TUNE_HIST_CTRL_ROI_EN_ACCESS FX1_ISP_ACCESS_RW

/**
 * TUNE_HIST_START: địa chỉ lệch 0x8604, dòng bảng 1122, kiểu truy cập RW
 * Điểm bắt đầu pixel gồm đầu mút; cập nhật theo profile, MAS v1.07 tuning-ae.
 */
#define FX1_ISP_TUNE_HIST_START_OFFSET 0x8604U
#define REG_TUNE_HIST_START FX1_ISP_TUNE_HIST_START_OFFSET
#define FX1_ISP_TUNE_HIST_START_RESET 0x00000000U
#define FX1_ISP_TUNE_HIST_START_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_TUNE_HIST_START_RO_MASK 0x00000000U
#define FX1_ISP_TUNE_HIST_START_RW_MASK 0xFFFFFFFFU
#define FX1_ISP_TUNE_HIST_START_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_HIST_START_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_HIST_START_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_HIST_START_W1SC_MASK 0x00000000U
/**
 * X [15:0], dòng bảng 1123, kiểu truy cập RW
 * Tọa độ ngang của đầu gồm đầu mút hoặc cuối không gồm đầu mút.
 */
#define FX1_ISP_TUNE_HIST_START_X_SHIFT 0U
#define FX1_ISP_TUNE_HIST_START_X_WIDTH 16U
#define FX1_ISP_TUNE_HIST_START_X_MASK 0x0000FFFFU
#define FX1_ISP_TUNE_HIST_START_X_RESET 0x00000000U
#define FX1_ISP_TUNE_HIST_START_X_ACCESS FX1_ISP_ACCESS_RW
/**
 * Y [31:16], dòng bảng 1124, kiểu truy cập RW
 * Tọa độ dọc của đầu gồm đầu mút hoặc cuối không gồm đầu mút.
 */
#define FX1_ISP_TUNE_HIST_START_Y_SHIFT 16U
#define FX1_ISP_TUNE_HIST_START_Y_WIDTH 16U
#define FX1_ISP_TUNE_HIST_START_Y_MASK 0xFFFF0000U
#define FX1_ISP_TUNE_HIST_START_Y_RESET 0x00000000U
#define FX1_ISP_TUNE_HIST_START_Y_ACCESS FX1_ISP_ACCESS_RW

/**
 * TUNE_HIST_END: địa chỉ lệch 0x8608, dòng bảng 1125, kiểu truy cập RW
 * Điểm kết thúc pixel không gồm đầu mút; cập nhật theo profile, MAS v1.07 tuning-ae.
 */
#define FX1_ISP_TUNE_HIST_END_OFFSET 0x8608U
#define REG_TUNE_HIST_END FX1_ISP_TUNE_HIST_END_OFFSET
#define FX1_ISP_TUNE_HIST_END_RESET 0x00000000U
#define FX1_ISP_TUNE_HIST_END_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_TUNE_HIST_END_RO_MASK 0x00000000U
#define FX1_ISP_TUNE_HIST_END_RW_MASK 0xFFFFFFFFU
#define FX1_ISP_TUNE_HIST_END_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_HIST_END_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_HIST_END_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_HIST_END_W1SC_MASK 0x00000000U
/**
 * X [15:0], dòng bảng 1126, kiểu truy cập RW
 * Tọa độ ngang của đầu gồm đầu mút hoặc cuối không gồm đầu mút.
 */
#define FX1_ISP_TUNE_HIST_END_X_SHIFT 0U
#define FX1_ISP_TUNE_HIST_END_X_WIDTH 16U
#define FX1_ISP_TUNE_HIST_END_X_MASK 0x0000FFFFU
#define FX1_ISP_TUNE_HIST_END_X_RESET 0x00000000U
#define FX1_ISP_TUNE_HIST_END_X_ACCESS FX1_ISP_ACCESS_RW
/**
 * Y [31:16], dòng bảng 1127, kiểu truy cập RW
 * Tọa độ dọc của đầu gồm đầu mút hoặc cuối không gồm đầu mút.
 */
#define FX1_ISP_TUNE_HIST_END_Y_SHIFT 16U
#define FX1_ISP_TUNE_HIST_END_Y_WIDTH 16U
#define FX1_ISP_TUNE_HIST_END_Y_MASK 0xFFFF0000U
#define FX1_ISP_TUNE_HIST_END_Y_RESET 0x00000000U
#define FX1_ISP_TUNE_HIST_END_Y_ACCESS FX1_ISP_ACCESS_RW

/**
 * TUNE_HIST_STEP: địa chỉ lệch 0x860C, dòng bảng 1128, kiểu truy cập RW
 * Lưới lấy mẫu neo ở điểm đầu ROI; cập nhật theo profile, MAS v1.07 tuning-ae.
 */
#define FX1_ISP_TUNE_HIST_STEP_OFFSET 0x860CU
#define REG_TUNE_HIST_STEP FX1_ISP_TUNE_HIST_STEP_OFFSET
#define FX1_ISP_TUNE_HIST_STEP_RESET 0x00000101U
#define FX1_ISP_TUNE_HIST_STEP_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_TUNE_HIST_STEP_RO_MASK 0x00000000U
#define FX1_ISP_TUNE_HIST_STEP_RW_MASK 0x0000FFFFU
#define FX1_ISP_TUNE_HIST_STEP_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_HIST_STEP_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_HIST_STEP_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_HIST_STEP_W1SC_MASK 0x00000000U
/**
 * X [7:0], dòng bảng 1129, kiểu truy cập RW
 * Bước lấy mẫu ngang 1..255.
 */
#define FX1_ISP_TUNE_HIST_STEP_X_SHIFT 0U
#define FX1_ISP_TUNE_HIST_STEP_X_WIDTH 8U
#define FX1_ISP_TUNE_HIST_STEP_X_MASK 0x000000FFU
#define FX1_ISP_TUNE_HIST_STEP_X_RESET 0x00000001U
#define FX1_ISP_TUNE_HIST_STEP_X_ACCESS FX1_ISP_ACCESS_RW
/**
 * Y [15:8], dòng bảng 1130, kiểu truy cập RW
 * Bước lấy mẫu dọc 1..255.
 */
#define FX1_ISP_TUNE_HIST_STEP_Y_SHIFT 8U
#define FX1_ISP_TUNE_HIST_STEP_Y_WIDTH 8U
#define FX1_ISP_TUNE_HIST_STEP_Y_MASK 0x0000FF00U
#define FX1_ISP_TUNE_HIST_STEP_Y_RESET 0x00000001U
#define FX1_ISP_TUNE_HIST_STEP_Y_ACCESS FX1_ISP_ACCESS_RW

/**
 * TUNE_HIST_WEIGHT0: địa chỉ lệch 0x8640, dòng bảng 1131, kiểu truy cập RW
 * Trọng số không gian 5x5 theo hàng trước/cột sau. 0 loại ô, khác 0 cộng trọng số đó vào bin. Cập
 * nhật theo profile, MAS v1.07 tuning-ae.
 */
#define FX1_ISP_TUNE_HIST_WEIGHT0_OFFSET 0x8640U
#define REG_TUNE_HIST_WEIGHT0 FX1_ISP_TUNE_HIST_WEIGHT0_OFFSET
#define FX1_ISP_TUNE_HIST_WEIGHT0_RESET 0x00000001U
#define FX1_ISP_TUNE_HIST_WEIGHT0_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_TUNE_HIST_WEIGHT0_RO_MASK 0x00000000U
#define FX1_ISP_TUNE_HIST_WEIGHT0_RW_MASK 0x000000FFU
#define FX1_ISP_TUNE_HIST_WEIGHT0_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_HIST_WEIGHT0_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_HIST_WEIGHT0_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_HIST_WEIGHT0_W1SC_MASK 0x00000000U
/**
 * VALUE [7:0], dòng bảng 1132, kiểu truy cập RW
 * Trọng số không gian 5x5 theo thứ tự ảnh; 0 loại ô, giá trị khác 0 được cộng vào bin.
 */
#define FX1_ISP_TUNE_HIST_WEIGHT0_VALUE_SHIFT 0U
#define FX1_ISP_TUNE_HIST_WEIGHT0_VALUE_WIDTH 8U
#define FX1_ISP_TUNE_HIST_WEIGHT0_VALUE_MASK 0x000000FFU
#define FX1_ISP_TUNE_HIST_WEIGHT0_VALUE_RESET 0x00000001U
#define FX1_ISP_TUNE_HIST_WEIGHT0_VALUE_ACCESS FX1_ISP_ACCESS_RW

/**
 * TUNE_HIST_WEIGHT1: địa chỉ lệch 0x8644, dòng bảng 1133, kiểu truy cập RW
 * Trọng số không gian 5x5 theo hàng trước/cột sau. 0 loại ô, khác 0 cộng trọng số đó vào bin. Cập
 * nhật theo profile, MAS v1.07 tuning-ae.
 */
#define FX1_ISP_TUNE_HIST_WEIGHT1_OFFSET 0x8644U
#define REG_TUNE_HIST_WEIGHT1 FX1_ISP_TUNE_HIST_WEIGHT1_OFFSET
#define FX1_ISP_TUNE_HIST_WEIGHT1_RESET 0x00000001U
#define FX1_ISP_TUNE_HIST_WEIGHT1_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_TUNE_HIST_WEIGHT1_RO_MASK 0x00000000U
#define FX1_ISP_TUNE_HIST_WEIGHT1_RW_MASK 0x000000FFU
#define FX1_ISP_TUNE_HIST_WEIGHT1_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_HIST_WEIGHT1_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_HIST_WEIGHT1_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_HIST_WEIGHT1_W1SC_MASK 0x00000000U
/**
 * VALUE [7:0], dòng bảng 1134, kiểu truy cập RW
 * Trọng số không gian 5x5 theo thứ tự ảnh; 0 loại ô, giá trị khác 0 được cộng vào bin.
 */
#define FX1_ISP_TUNE_HIST_WEIGHT1_VALUE_SHIFT 0U
#define FX1_ISP_TUNE_HIST_WEIGHT1_VALUE_WIDTH 8U
#define FX1_ISP_TUNE_HIST_WEIGHT1_VALUE_MASK 0x000000FFU
#define FX1_ISP_TUNE_HIST_WEIGHT1_VALUE_RESET 0x00000001U
#define FX1_ISP_TUNE_HIST_WEIGHT1_VALUE_ACCESS FX1_ISP_ACCESS_RW

/**
 * TUNE_HIST_WEIGHT2: địa chỉ lệch 0x8648, dòng bảng 1135, kiểu truy cập RW
 * Trọng số không gian 5x5 theo hàng trước/cột sau. 0 loại ô, khác 0 cộng trọng số đó vào bin. Cập
 * nhật theo profile, MAS v1.07 tuning-ae.
 */
#define FX1_ISP_TUNE_HIST_WEIGHT2_OFFSET 0x8648U
#define REG_TUNE_HIST_WEIGHT2 FX1_ISP_TUNE_HIST_WEIGHT2_OFFSET
#define FX1_ISP_TUNE_HIST_WEIGHT2_RESET 0x00000001U
#define FX1_ISP_TUNE_HIST_WEIGHT2_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_TUNE_HIST_WEIGHT2_RO_MASK 0x00000000U
#define FX1_ISP_TUNE_HIST_WEIGHT2_RW_MASK 0x000000FFU
#define FX1_ISP_TUNE_HIST_WEIGHT2_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_HIST_WEIGHT2_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_HIST_WEIGHT2_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_HIST_WEIGHT2_W1SC_MASK 0x00000000U
/**
 * VALUE [7:0], dòng bảng 1136, kiểu truy cập RW
 * Trọng số không gian 5x5 theo thứ tự ảnh; 0 loại ô, giá trị khác 0 được cộng vào bin.
 */
#define FX1_ISP_TUNE_HIST_WEIGHT2_VALUE_SHIFT 0U
#define FX1_ISP_TUNE_HIST_WEIGHT2_VALUE_WIDTH 8U
#define FX1_ISP_TUNE_HIST_WEIGHT2_VALUE_MASK 0x000000FFU
#define FX1_ISP_TUNE_HIST_WEIGHT2_VALUE_RESET 0x00000001U
#define FX1_ISP_TUNE_HIST_WEIGHT2_VALUE_ACCESS FX1_ISP_ACCESS_RW

/**
 * TUNE_HIST_WEIGHT3: địa chỉ lệch 0x864C, dòng bảng 1137, kiểu truy cập RW
 * Trọng số không gian 5x5 theo hàng trước/cột sau. 0 loại ô, khác 0 cộng trọng số đó vào bin. Cập
 * nhật theo profile, MAS v1.07 tuning-ae.
 */
#define FX1_ISP_TUNE_HIST_WEIGHT3_OFFSET 0x864CU
#define REG_TUNE_HIST_WEIGHT3 FX1_ISP_TUNE_HIST_WEIGHT3_OFFSET
#define FX1_ISP_TUNE_HIST_WEIGHT3_RESET 0x00000001U
#define FX1_ISP_TUNE_HIST_WEIGHT3_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_TUNE_HIST_WEIGHT3_RO_MASK 0x00000000U
#define FX1_ISP_TUNE_HIST_WEIGHT3_RW_MASK 0x000000FFU
#define FX1_ISP_TUNE_HIST_WEIGHT3_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_HIST_WEIGHT3_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_HIST_WEIGHT3_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_HIST_WEIGHT3_W1SC_MASK 0x00000000U
/**
 * VALUE [7:0], dòng bảng 1138, kiểu truy cập RW
 * Trọng số không gian 5x5 theo thứ tự ảnh; 0 loại ô, giá trị khác 0 được cộng vào bin.
 */
#define FX1_ISP_TUNE_HIST_WEIGHT3_VALUE_SHIFT 0U
#define FX1_ISP_TUNE_HIST_WEIGHT3_VALUE_WIDTH 8U
#define FX1_ISP_TUNE_HIST_WEIGHT3_VALUE_MASK 0x000000FFU
#define FX1_ISP_TUNE_HIST_WEIGHT3_VALUE_RESET 0x00000001U
#define FX1_ISP_TUNE_HIST_WEIGHT3_VALUE_ACCESS FX1_ISP_ACCESS_RW

/**
 * TUNE_HIST_WEIGHT4: địa chỉ lệch 0x8650, dòng bảng 1139, kiểu truy cập RW
 * Trọng số không gian 5x5 theo hàng trước/cột sau. 0 loại ô, khác 0 cộng trọng số đó vào bin. Cập
 * nhật theo profile, MAS v1.07 tuning-ae.
 */
#define FX1_ISP_TUNE_HIST_WEIGHT4_OFFSET 0x8650U
#define REG_TUNE_HIST_WEIGHT4 FX1_ISP_TUNE_HIST_WEIGHT4_OFFSET
#define FX1_ISP_TUNE_HIST_WEIGHT4_RESET 0x00000001U
#define FX1_ISP_TUNE_HIST_WEIGHT4_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_TUNE_HIST_WEIGHT4_RO_MASK 0x00000000U
#define FX1_ISP_TUNE_HIST_WEIGHT4_RW_MASK 0x000000FFU
#define FX1_ISP_TUNE_HIST_WEIGHT4_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_HIST_WEIGHT4_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_HIST_WEIGHT4_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_HIST_WEIGHT4_W1SC_MASK 0x00000000U
/**
 * VALUE [7:0], dòng bảng 1140, kiểu truy cập RW
 * Trọng số không gian 5x5 theo thứ tự ảnh; 0 loại ô, giá trị khác 0 được cộng vào bin.
 */
#define FX1_ISP_TUNE_HIST_WEIGHT4_VALUE_SHIFT 0U
#define FX1_ISP_TUNE_HIST_WEIGHT4_VALUE_WIDTH 8U
#define FX1_ISP_TUNE_HIST_WEIGHT4_VALUE_MASK 0x000000FFU
#define FX1_ISP_TUNE_HIST_WEIGHT4_VALUE_RESET 0x00000001U
#define FX1_ISP_TUNE_HIST_WEIGHT4_VALUE_ACCESS FX1_ISP_ACCESS_RW

/**
 * TUNE_HIST_WEIGHT5: địa chỉ lệch 0x8654, dòng bảng 1141, kiểu truy cập RW
 * Trọng số không gian 5x5 theo hàng trước/cột sau. 0 loại ô, khác 0 cộng trọng số đó vào bin. Cập
 * nhật theo profile, MAS v1.07 tuning-ae.
 */
#define FX1_ISP_TUNE_HIST_WEIGHT5_OFFSET 0x8654U
#define REG_TUNE_HIST_WEIGHT5 FX1_ISP_TUNE_HIST_WEIGHT5_OFFSET
#define FX1_ISP_TUNE_HIST_WEIGHT5_RESET 0x00000001U
#define FX1_ISP_TUNE_HIST_WEIGHT5_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_TUNE_HIST_WEIGHT5_RO_MASK 0x00000000U
#define FX1_ISP_TUNE_HIST_WEIGHT5_RW_MASK 0x000000FFU
#define FX1_ISP_TUNE_HIST_WEIGHT5_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_HIST_WEIGHT5_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_HIST_WEIGHT5_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_HIST_WEIGHT5_W1SC_MASK 0x00000000U
/**
 * VALUE [7:0], dòng bảng 1142, kiểu truy cập RW
 * Trọng số không gian 5x5 theo thứ tự ảnh; 0 loại ô, giá trị khác 0 được cộng vào bin.
 */
#define FX1_ISP_TUNE_HIST_WEIGHT5_VALUE_SHIFT 0U
#define FX1_ISP_TUNE_HIST_WEIGHT5_VALUE_WIDTH 8U
#define FX1_ISP_TUNE_HIST_WEIGHT5_VALUE_MASK 0x000000FFU
#define FX1_ISP_TUNE_HIST_WEIGHT5_VALUE_RESET 0x00000001U
#define FX1_ISP_TUNE_HIST_WEIGHT5_VALUE_ACCESS FX1_ISP_ACCESS_RW

/**
 * TUNE_HIST_WEIGHT6: địa chỉ lệch 0x8658, dòng bảng 1143, kiểu truy cập RW
 * Trọng số không gian 5x5 theo hàng trước/cột sau. 0 loại ô, khác 0 cộng trọng số đó vào bin. Cập
 * nhật theo profile, MAS v1.07 tuning-ae.
 */
#define FX1_ISP_TUNE_HIST_WEIGHT6_OFFSET 0x8658U
#define REG_TUNE_HIST_WEIGHT6 FX1_ISP_TUNE_HIST_WEIGHT6_OFFSET
#define FX1_ISP_TUNE_HIST_WEIGHT6_RESET 0x00000001U
#define FX1_ISP_TUNE_HIST_WEIGHT6_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_TUNE_HIST_WEIGHT6_RO_MASK 0x00000000U
#define FX1_ISP_TUNE_HIST_WEIGHT6_RW_MASK 0x000000FFU
#define FX1_ISP_TUNE_HIST_WEIGHT6_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_HIST_WEIGHT6_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_HIST_WEIGHT6_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_HIST_WEIGHT6_W1SC_MASK 0x00000000U
/**
 * VALUE [7:0], dòng bảng 1144, kiểu truy cập RW
 * Trọng số không gian 5x5 theo thứ tự ảnh; 0 loại ô, giá trị khác 0 được cộng vào bin.
 */
#define FX1_ISP_TUNE_HIST_WEIGHT6_VALUE_SHIFT 0U
#define FX1_ISP_TUNE_HIST_WEIGHT6_VALUE_WIDTH 8U
#define FX1_ISP_TUNE_HIST_WEIGHT6_VALUE_MASK 0x000000FFU
#define FX1_ISP_TUNE_HIST_WEIGHT6_VALUE_RESET 0x00000001U
#define FX1_ISP_TUNE_HIST_WEIGHT6_VALUE_ACCESS FX1_ISP_ACCESS_RW

/**
 * TUNE_HIST_WEIGHT7: địa chỉ lệch 0x865C, dòng bảng 1145, kiểu truy cập RW
 * Trọng số không gian 5x5 theo hàng trước/cột sau. 0 loại ô, khác 0 cộng trọng số đó vào bin. Cập
 * nhật theo profile, MAS v1.07 tuning-ae.
 */
#define FX1_ISP_TUNE_HIST_WEIGHT7_OFFSET 0x865CU
#define REG_TUNE_HIST_WEIGHT7 FX1_ISP_TUNE_HIST_WEIGHT7_OFFSET
#define FX1_ISP_TUNE_HIST_WEIGHT7_RESET 0x00000001U
#define FX1_ISP_TUNE_HIST_WEIGHT7_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_TUNE_HIST_WEIGHT7_RO_MASK 0x00000000U
#define FX1_ISP_TUNE_HIST_WEIGHT7_RW_MASK 0x000000FFU
#define FX1_ISP_TUNE_HIST_WEIGHT7_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_HIST_WEIGHT7_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_HIST_WEIGHT7_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_HIST_WEIGHT7_W1SC_MASK 0x00000000U
/**
 * VALUE [7:0], dòng bảng 1146, kiểu truy cập RW
 * Trọng số không gian 5x5 theo thứ tự ảnh; 0 loại ô, giá trị khác 0 được cộng vào bin.
 */
#define FX1_ISP_TUNE_HIST_WEIGHT7_VALUE_SHIFT 0U
#define FX1_ISP_TUNE_HIST_WEIGHT7_VALUE_WIDTH 8U
#define FX1_ISP_TUNE_HIST_WEIGHT7_VALUE_MASK 0x000000FFU
#define FX1_ISP_TUNE_HIST_WEIGHT7_VALUE_RESET 0x00000001U
#define FX1_ISP_TUNE_HIST_WEIGHT7_VALUE_ACCESS FX1_ISP_ACCESS_RW

/**
 * TUNE_HIST_WEIGHT8: địa chỉ lệch 0x8660, dòng bảng 1147, kiểu truy cập RW
 * Trọng số không gian 5x5 theo hàng trước/cột sau. 0 loại ô, khác 0 cộng trọng số đó vào bin. Cập
 * nhật theo profile, MAS v1.07 tuning-ae.
 */
#define FX1_ISP_TUNE_HIST_WEIGHT8_OFFSET 0x8660U
#define REG_TUNE_HIST_WEIGHT8 FX1_ISP_TUNE_HIST_WEIGHT8_OFFSET
#define FX1_ISP_TUNE_HIST_WEIGHT8_RESET 0x00000001U
#define FX1_ISP_TUNE_HIST_WEIGHT8_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_TUNE_HIST_WEIGHT8_RO_MASK 0x00000000U
#define FX1_ISP_TUNE_HIST_WEIGHT8_RW_MASK 0x000000FFU
#define FX1_ISP_TUNE_HIST_WEIGHT8_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_HIST_WEIGHT8_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_HIST_WEIGHT8_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_HIST_WEIGHT8_W1SC_MASK 0x00000000U
/**
 * VALUE [7:0], dòng bảng 1148, kiểu truy cập RW
 * Trọng số không gian 5x5 theo thứ tự ảnh; 0 loại ô, giá trị khác 0 được cộng vào bin.
 */
#define FX1_ISP_TUNE_HIST_WEIGHT8_VALUE_SHIFT 0U
#define FX1_ISP_TUNE_HIST_WEIGHT8_VALUE_WIDTH 8U
#define FX1_ISP_TUNE_HIST_WEIGHT8_VALUE_MASK 0x000000FFU
#define FX1_ISP_TUNE_HIST_WEIGHT8_VALUE_RESET 0x00000001U
#define FX1_ISP_TUNE_HIST_WEIGHT8_VALUE_ACCESS FX1_ISP_ACCESS_RW

/**
 * TUNE_HIST_WEIGHT9: địa chỉ lệch 0x8664, dòng bảng 1149, kiểu truy cập RW
 * Trọng số không gian 5x5 theo hàng trước/cột sau. 0 loại ô, khác 0 cộng trọng số đó vào bin. Cập
 * nhật theo profile, MAS v1.07 tuning-ae.
 */
#define FX1_ISP_TUNE_HIST_WEIGHT9_OFFSET 0x8664U
#define REG_TUNE_HIST_WEIGHT9 FX1_ISP_TUNE_HIST_WEIGHT9_OFFSET
#define FX1_ISP_TUNE_HIST_WEIGHT9_RESET 0x00000001U
#define FX1_ISP_TUNE_HIST_WEIGHT9_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_TUNE_HIST_WEIGHT9_RO_MASK 0x00000000U
#define FX1_ISP_TUNE_HIST_WEIGHT9_RW_MASK 0x000000FFU
#define FX1_ISP_TUNE_HIST_WEIGHT9_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_HIST_WEIGHT9_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_HIST_WEIGHT9_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_HIST_WEIGHT9_W1SC_MASK 0x00000000U
/**
 * VALUE [7:0], dòng bảng 1150, kiểu truy cập RW
 * Trọng số không gian 5x5 theo thứ tự ảnh; 0 loại ô, giá trị khác 0 được cộng vào bin.
 */
#define FX1_ISP_TUNE_HIST_WEIGHT9_VALUE_SHIFT 0U
#define FX1_ISP_TUNE_HIST_WEIGHT9_VALUE_WIDTH 8U
#define FX1_ISP_TUNE_HIST_WEIGHT9_VALUE_MASK 0x000000FFU
#define FX1_ISP_TUNE_HIST_WEIGHT9_VALUE_RESET 0x00000001U
#define FX1_ISP_TUNE_HIST_WEIGHT9_VALUE_ACCESS FX1_ISP_ACCESS_RW

/**
 * TUNE_HIST_WEIGHT10: địa chỉ lệch 0x8668, dòng bảng 1151, kiểu truy cập RW
 * Trọng số không gian 5x5 theo hàng trước/cột sau. 0 loại ô, khác 0 cộng trọng số đó vào bin. Cập
 * nhật theo profile, MAS v1.07 tuning-ae.
 */
#define FX1_ISP_TUNE_HIST_WEIGHT10_OFFSET 0x8668U
#define REG_TUNE_HIST_WEIGHT10 FX1_ISP_TUNE_HIST_WEIGHT10_OFFSET
#define FX1_ISP_TUNE_HIST_WEIGHT10_RESET 0x00000001U
#define FX1_ISP_TUNE_HIST_WEIGHT10_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_TUNE_HIST_WEIGHT10_RO_MASK 0x00000000U
#define FX1_ISP_TUNE_HIST_WEIGHT10_RW_MASK 0x000000FFU
#define FX1_ISP_TUNE_HIST_WEIGHT10_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_HIST_WEIGHT10_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_HIST_WEIGHT10_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_HIST_WEIGHT10_W1SC_MASK 0x00000000U
/**
 * VALUE [7:0], dòng bảng 1152, kiểu truy cập RW
 * Trọng số không gian 5x5 theo thứ tự ảnh; 0 loại ô, giá trị khác 0 được cộng vào bin.
 */
#define FX1_ISP_TUNE_HIST_WEIGHT10_VALUE_SHIFT 0U
#define FX1_ISP_TUNE_HIST_WEIGHT10_VALUE_WIDTH 8U
#define FX1_ISP_TUNE_HIST_WEIGHT10_VALUE_MASK 0x000000FFU
#define FX1_ISP_TUNE_HIST_WEIGHT10_VALUE_RESET 0x00000001U
#define FX1_ISP_TUNE_HIST_WEIGHT10_VALUE_ACCESS FX1_ISP_ACCESS_RW

/**
 * TUNE_HIST_WEIGHT11: địa chỉ lệch 0x866C, dòng bảng 1153, kiểu truy cập RW
 * Trọng số không gian 5x5 theo hàng trước/cột sau. 0 loại ô, khác 0 cộng trọng số đó vào bin. Cập
 * nhật theo profile, MAS v1.07 tuning-ae.
 */
#define FX1_ISP_TUNE_HIST_WEIGHT11_OFFSET 0x866CU
#define REG_TUNE_HIST_WEIGHT11 FX1_ISP_TUNE_HIST_WEIGHT11_OFFSET
#define FX1_ISP_TUNE_HIST_WEIGHT11_RESET 0x00000001U
#define FX1_ISP_TUNE_HIST_WEIGHT11_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_TUNE_HIST_WEIGHT11_RO_MASK 0x00000000U
#define FX1_ISP_TUNE_HIST_WEIGHT11_RW_MASK 0x000000FFU
#define FX1_ISP_TUNE_HIST_WEIGHT11_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_HIST_WEIGHT11_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_HIST_WEIGHT11_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_HIST_WEIGHT11_W1SC_MASK 0x00000000U
/**
 * VALUE [7:0], dòng bảng 1154, kiểu truy cập RW
 * Trọng số không gian 5x5 theo thứ tự ảnh; 0 loại ô, giá trị khác 0 được cộng vào bin.
 */
#define FX1_ISP_TUNE_HIST_WEIGHT11_VALUE_SHIFT 0U
#define FX1_ISP_TUNE_HIST_WEIGHT11_VALUE_WIDTH 8U
#define FX1_ISP_TUNE_HIST_WEIGHT11_VALUE_MASK 0x000000FFU
#define FX1_ISP_TUNE_HIST_WEIGHT11_VALUE_RESET 0x00000001U
#define FX1_ISP_TUNE_HIST_WEIGHT11_VALUE_ACCESS FX1_ISP_ACCESS_RW

/**
 * TUNE_HIST_WEIGHT12: địa chỉ lệch 0x8670, dòng bảng 1155, kiểu truy cập RW
 * Trọng số không gian 5x5 theo hàng trước/cột sau. 0 loại ô, khác 0 cộng trọng số đó vào bin. Cập
 * nhật theo profile, MAS v1.07 tuning-ae.
 */
#define FX1_ISP_TUNE_HIST_WEIGHT12_OFFSET 0x8670U
#define REG_TUNE_HIST_WEIGHT12 FX1_ISP_TUNE_HIST_WEIGHT12_OFFSET
#define FX1_ISP_TUNE_HIST_WEIGHT12_RESET 0x00000001U
#define FX1_ISP_TUNE_HIST_WEIGHT12_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_TUNE_HIST_WEIGHT12_RO_MASK 0x00000000U
#define FX1_ISP_TUNE_HIST_WEIGHT12_RW_MASK 0x000000FFU
#define FX1_ISP_TUNE_HIST_WEIGHT12_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_HIST_WEIGHT12_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_HIST_WEIGHT12_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_HIST_WEIGHT12_W1SC_MASK 0x00000000U
/**
 * VALUE [7:0], dòng bảng 1156, kiểu truy cập RW
 * Trọng số không gian 5x5 theo thứ tự ảnh; 0 loại ô, giá trị khác 0 được cộng vào bin.
 */
#define FX1_ISP_TUNE_HIST_WEIGHT12_VALUE_SHIFT 0U
#define FX1_ISP_TUNE_HIST_WEIGHT12_VALUE_WIDTH 8U
#define FX1_ISP_TUNE_HIST_WEIGHT12_VALUE_MASK 0x000000FFU
#define FX1_ISP_TUNE_HIST_WEIGHT12_VALUE_RESET 0x00000001U
#define FX1_ISP_TUNE_HIST_WEIGHT12_VALUE_ACCESS FX1_ISP_ACCESS_RW

/**
 * TUNE_HIST_WEIGHT13: địa chỉ lệch 0x8674, dòng bảng 1157, kiểu truy cập RW
 * Trọng số không gian 5x5 theo hàng trước/cột sau. 0 loại ô, khác 0 cộng trọng số đó vào bin. Cập
 * nhật theo profile, MAS v1.07 tuning-ae.
 */
#define FX1_ISP_TUNE_HIST_WEIGHT13_OFFSET 0x8674U
#define REG_TUNE_HIST_WEIGHT13 FX1_ISP_TUNE_HIST_WEIGHT13_OFFSET
#define FX1_ISP_TUNE_HIST_WEIGHT13_RESET 0x00000001U
#define FX1_ISP_TUNE_HIST_WEIGHT13_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_TUNE_HIST_WEIGHT13_RO_MASK 0x00000000U
#define FX1_ISP_TUNE_HIST_WEIGHT13_RW_MASK 0x000000FFU
#define FX1_ISP_TUNE_HIST_WEIGHT13_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_HIST_WEIGHT13_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_HIST_WEIGHT13_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_HIST_WEIGHT13_W1SC_MASK 0x00000000U
/**
 * VALUE [7:0], dòng bảng 1158, kiểu truy cập RW
 * Trọng số không gian 5x5 theo thứ tự ảnh; 0 loại ô, giá trị khác 0 được cộng vào bin.
 */
#define FX1_ISP_TUNE_HIST_WEIGHT13_VALUE_SHIFT 0U
#define FX1_ISP_TUNE_HIST_WEIGHT13_VALUE_WIDTH 8U
#define FX1_ISP_TUNE_HIST_WEIGHT13_VALUE_MASK 0x000000FFU
#define FX1_ISP_TUNE_HIST_WEIGHT13_VALUE_RESET 0x00000001U
#define FX1_ISP_TUNE_HIST_WEIGHT13_VALUE_ACCESS FX1_ISP_ACCESS_RW

/**
 * TUNE_HIST_WEIGHT14: địa chỉ lệch 0x8678, dòng bảng 1159, kiểu truy cập RW
 * Trọng số không gian 5x5 theo hàng trước/cột sau. 0 loại ô, khác 0 cộng trọng số đó vào bin. Cập
 * nhật theo profile, MAS v1.07 tuning-ae.
 */
#define FX1_ISP_TUNE_HIST_WEIGHT14_OFFSET 0x8678U
#define REG_TUNE_HIST_WEIGHT14 FX1_ISP_TUNE_HIST_WEIGHT14_OFFSET
#define FX1_ISP_TUNE_HIST_WEIGHT14_RESET 0x00000001U
#define FX1_ISP_TUNE_HIST_WEIGHT14_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_TUNE_HIST_WEIGHT14_RO_MASK 0x00000000U
#define FX1_ISP_TUNE_HIST_WEIGHT14_RW_MASK 0x000000FFU
#define FX1_ISP_TUNE_HIST_WEIGHT14_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_HIST_WEIGHT14_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_HIST_WEIGHT14_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_HIST_WEIGHT14_W1SC_MASK 0x00000000U
/**
 * VALUE [7:0], dòng bảng 1160, kiểu truy cập RW
 * Trọng số không gian 5x5 theo thứ tự ảnh; 0 loại ô, giá trị khác 0 được cộng vào bin.
 */
#define FX1_ISP_TUNE_HIST_WEIGHT14_VALUE_SHIFT 0U
#define FX1_ISP_TUNE_HIST_WEIGHT14_VALUE_WIDTH 8U
#define FX1_ISP_TUNE_HIST_WEIGHT14_VALUE_MASK 0x000000FFU
#define FX1_ISP_TUNE_HIST_WEIGHT14_VALUE_RESET 0x00000001U
#define FX1_ISP_TUNE_HIST_WEIGHT14_VALUE_ACCESS FX1_ISP_ACCESS_RW

/**
 * TUNE_HIST_WEIGHT15: địa chỉ lệch 0x867C, dòng bảng 1161, kiểu truy cập RW
 * Trọng số không gian 5x5 theo hàng trước/cột sau. 0 loại ô, khác 0 cộng trọng số đó vào bin. Cập
 * nhật theo profile, MAS v1.07 tuning-ae.
 */
#define FX1_ISP_TUNE_HIST_WEIGHT15_OFFSET 0x867CU
#define REG_TUNE_HIST_WEIGHT15 FX1_ISP_TUNE_HIST_WEIGHT15_OFFSET
#define FX1_ISP_TUNE_HIST_WEIGHT15_RESET 0x00000001U
#define FX1_ISP_TUNE_HIST_WEIGHT15_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_TUNE_HIST_WEIGHT15_RO_MASK 0x00000000U
#define FX1_ISP_TUNE_HIST_WEIGHT15_RW_MASK 0x000000FFU
#define FX1_ISP_TUNE_HIST_WEIGHT15_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_HIST_WEIGHT15_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_HIST_WEIGHT15_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_HIST_WEIGHT15_W1SC_MASK 0x00000000U
/**
 * VALUE [7:0], dòng bảng 1162, kiểu truy cập RW
 * Trọng số không gian 5x5 theo thứ tự ảnh; 0 loại ô, giá trị khác 0 được cộng vào bin.
 */
#define FX1_ISP_TUNE_HIST_WEIGHT15_VALUE_SHIFT 0U
#define FX1_ISP_TUNE_HIST_WEIGHT15_VALUE_WIDTH 8U
#define FX1_ISP_TUNE_HIST_WEIGHT15_VALUE_MASK 0x000000FFU
#define FX1_ISP_TUNE_HIST_WEIGHT15_VALUE_RESET 0x00000001U
#define FX1_ISP_TUNE_HIST_WEIGHT15_VALUE_ACCESS FX1_ISP_ACCESS_RW

/**
 * TUNE_HIST_WEIGHT16: địa chỉ lệch 0x8680, dòng bảng 1163, kiểu truy cập RW
 * Trọng số không gian 5x5 theo hàng trước/cột sau. 0 loại ô, khác 0 cộng trọng số đó vào bin. Cập
 * nhật theo profile, MAS v1.07 tuning-ae.
 */
#define FX1_ISP_TUNE_HIST_WEIGHT16_OFFSET 0x8680U
#define REG_TUNE_HIST_WEIGHT16 FX1_ISP_TUNE_HIST_WEIGHT16_OFFSET
#define FX1_ISP_TUNE_HIST_WEIGHT16_RESET 0x00000001U
#define FX1_ISP_TUNE_HIST_WEIGHT16_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_TUNE_HIST_WEIGHT16_RO_MASK 0x00000000U
#define FX1_ISP_TUNE_HIST_WEIGHT16_RW_MASK 0x000000FFU
#define FX1_ISP_TUNE_HIST_WEIGHT16_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_HIST_WEIGHT16_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_HIST_WEIGHT16_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_HIST_WEIGHT16_W1SC_MASK 0x00000000U
/**
 * VALUE [7:0], dòng bảng 1164, kiểu truy cập RW
 * Trọng số không gian 5x5 theo thứ tự ảnh; 0 loại ô, giá trị khác 0 được cộng vào bin.
 */
#define FX1_ISP_TUNE_HIST_WEIGHT16_VALUE_SHIFT 0U
#define FX1_ISP_TUNE_HIST_WEIGHT16_VALUE_WIDTH 8U
#define FX1_ISP_TUNE_HIST_WEIGHT16_VALUE_MASK 0x000000FFU
#define FX1_ISP_TUNE_HIST_WEIGHT16_VALUE_RESET 0x00000001U
#define FX1_ISP_TUNE_HIST_WEIGHT16_VALUE_ACCESS FX1_ISP_ACCESS_RW

/**
 * TUNE_HIST_WEIGHT17: địa chỉ lệch 0x8684, dòng bảng 1165, kiểu truy cập RW
 * Trọng số không gian 5x5 theo hàng trước/cột sau. 0 loại ô, khác 0 cộng trọng số đó vào bin. Cập
 * nhật theo profile, MAS v1.07 tuning-ae.
 */
#define FX1_ISP_TUNE_HIST_WEIGHT17_OFFSET 0x8684U
#define REG_TUNE_HIST_WEIGHT17 FX1_ISP_TUNE_HIST_WEIGHT17_OFFSET
#define FX1_ISP_TUNE_HIST_WEIGHT17_RESET 0x00000001U
#define FX1_ISP_TUNE_HIST_WEIGHT17_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_TUNE_HIST_WEIGHT17_RO_MASK 0x00000000U
#define FX1_ISP_TUNE_HIST_WEIGHT17_RW_MASK 0x000000FFU
#define FX1_ISP_TUNE_HIST_WEIGHT17_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_HIST_WEIGHT17_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_HIST_WEIGHT17_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_HIST_WEIGHT17_W1SC_MASK 0x00000000U
/**
 * VALUE [7:0], dòng bảng 1166, kiểu truy cập RW
 * Trọng số không gian 5x5 theo thứ tự ảnh; 0 loại ô, giá trị khác 0 được cộng vào bin.
 */
#define FX1_ISP_TUNE_HIST_WEIGHT17_VALUE_SHIFT 0U
#define FX1_ISP_TUNE_HIST_WEIGHT17_VALUE_WIDTH 8U
#define FX1_ISP_TUNE_HIST_WEIGHT17_VALUE_MASK 0x000000FFU
#define FX1_ISP_TUNE_HIST_WEIGHT17_VALUE_RESET 0x00000001U
#define FX1_ISP_TUNE_HIST_WEIGHT17_VALUE_ACCESS FX1_ISP_ACCESS_RW

/**
 * TUNE_HIST_WEIGHT18: địa chỉ lệch 0x8688, dòng bảng 1167, kiểu truy cập RW
 * Trọng số không gian 5x5 theo hàng trước/cột sau. 0 loại ô, khác 0 cộng trọng số đó vào bin. Cập
 * nhật theo profile, MAS v1.07 tuning-ae.
 */
#define FX1_ISP_TUNE_HIST_WEIGHT18_OFFSET 0x8688U
#define REG_TUNE_HIST_WEIGHT18 FX1_ISP_TUNE_HIST_WEIGHT18_OFFSET
#define FX1_ISP_TUNE_HIST_WEIGHT18_RESET 0x00000001U
#define FX1_ISP_TUNE_HIST_WEIGHT18_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_TUNE_HIST_WEIGHT18_RO_MASK 0x00000000U
#define FX1_ISP_TUNE_HIST_WEIGHT18_RW_MASK 0x000000FFU
#define FX1_ISP_TUNE_HIST_WEIGHT18_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_HIST_WEIGHT18_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_HIST_WEIGHT18_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_HIST_WEIGHT18_W1SC_MASK 0x00000000U
/**
 * VALUE [7:0], dòng bảng 1168, kiểu truy cập RW
 * Trọng số không gian 5x5 theo thứ tự ảnh; 0 loại ô, giá trị khác 0 được cộng vào bin.
 */
#define FX1_ISP_TUNE_HIST_WEIGHT18_VALUE_SHIFT 0U
#define FX1_ISP_TUNE_HIST_WEIGHT18_VALUE_WIDTH 8U
#define FX1_ISP_TUNE_HIST_WEIGHT18_VALUE_MASK 0x000000FFU
#define FX1_ISP_TUNE_HIST_WEIGHT18_VALUE_RESET 0x00000001U
#define FX1_ISP_TUNE_HIST_WEIGHT18_VALUE_ACCESS FX1_ISP_ACCESS_RW

/**
 * TUNE_HIST_WEIGHT19: địa chỉ lệch 0x868C, dòng bảng 1169, kiểu truy cập RW
 * Trọng số không gian 5x5 theo hàng trước/cột sau. 0 loại ô, khác 0 cộng trọng số đó vào bin. Cập
 * nhật theo profile, MAS v1.07 tuning-ae.
 */
#define FX1_ISP_TUNE_HIST_WEIGHT19_OFFSET 0x868CU
#define REG_TUNE_HIST_WEIGHT19 FX1_ISP_TUNE_HIST_WEIGHT19_OFFSET
#define FX1_ISP_TUNE_HIST_WEIGHT19_RESET 0x00000001U
#define FX1_ISP_TUNE_HIST_WEIGHT19_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_TUNE_HIST_WEIGHT19_RO_MASK 0x00000000U
#define FX1_ISP_TUNE_HIST_WEIGHT19_RW_MASK 0x000000FFU
#define FX1_ISP_TUNE_HIST_WEIGHT19_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_HIST_WEIGHT19_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_HIST_WEIGHT19_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_HIST_WEIGHT19_W1SC_MASK 0x00000000U
/**
 * VALUE [7:0], dòng bảng 1170, kiểu truy cập RW
 * Trọng số không gian 5x5 theo thứ tự ảnh; 0 loại ô, giá trị khác 0 được cộng vào bin.
 */
#define FX1_ISP_TUNE_HIST_WEIGHT19_VALUE_SHIFT 0U
#define FX1_ISP_TUNE_HIST_WEIGHT19_VALUE_WIDTH 8U
#define FX1_ISP_TUNE_HIST_WEIGHT19_VALUE_MASK 0x000000FFU
#define FX1_ISP_TUNE_HIST_WEIGHT19_VALUE_RESET 0x00000001U
#define FX1_ISP_TUNE_HIST_WEIGHT19_VALUE_ACCESS FX1_ISP_ACCESS_RW

/**
 * TUNE_HIST_WEIGHT20: địa chỉ lệch 0x8690, dòng bảng 1171, kiểu truy cập RW
 * Trọng số không gian 5x5 theo hàng trước/cột sau. 0 loại ô, khác 0 cộng trọng số đó vào bin. Cập
 * nhật theo profile, MAS v1.07 tuning-ae.
 */
#define FX1_ISP_TUNE_HIST_WEIGHT20_OFFSET 0x8690U
#define REG_TUNE_HIST_WEIGHT20 FX1_ISP_TUNE_HIST_WEIGHT20_OFFSET
#define FX1_ISP_TUNE_HIST_WEIGHT20_RESET 0x00000001U
#define FX1_ISP_TUNE_HIST_WEIGHT20_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_TUNE_HIST_WEIGHT20_RO_MASK 0x00000000U
#define FX1_ISP_TUNE_HIST_WEIGHT20_RW_MASK 0x000000FFU
#define FX1_ISP_TUNE_HIST_WEIGHT20_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_HIST_WEIGHT20_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_HIST_WEIGHT20_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_HIST_WEIGHT20_W1SC_MASK 0x00000000U
/**
 * VALUE [7:0], dòng bảng 1172, kiểu truy cập RW
 * Trọng số không gian 5x5 theo thứ tự ảnh; 0 loại ô, giá trị khác 0 được cộng vào bin.
 */
#define FX1_ISP_TUNE_HIST_WEIGHT20_VALUE_SHIFT 0U
#define FX1_ISP_TUNE_HIST_WEIGHT20_VALUE_WIDTH 8U
#define FX1_ISP_TUNE_HIST_WEIGHT20_VALUE_MASK 0x000000FFU
#define FX1_ISP_TUNE_HIST_WEIGHT20_VALUE_RESET 0x00000001U
#define FX1_ISP_TUNE_HIST_WEIGHT20_VALUE_ACCESS FX1_ISP_ACCESS_RW

/**
 * TUNE_HIST_WEIGHT21: địa chỉ lệch 0x8694, dòng bảng 1173, kiểu truy cập RW
 * Trọng số không gian 5x5 theo hàng trước/cột sau. 0 loại ô, khác 0 cộng trọng số đó vào bin. Cập
 * nhật theo profile, MAS v1.07 tuning-ae.
 */
#define FX1_ISP_TUNE_HIST_WEIGHT21_OFFSET 0x8694U
#define REG_TUNE_HIST_WEIGHT21 FX1_ISP_TUNE_HIST_WEIGHT21_OFFSET
#define FX1_ISP_TUNE_HIST_WEIGHT21_RESET 0x00000001U
#define FX1_ISP_TUNE_HIST_WEIGHT21_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_TUNE_HIST_WEIGHT21_RO_MASK 0x00000000U
#define FX1_ISP_TUNE_HIST_WEIGHT21_RW_MASK 0x000000FFU
#define FX1_ISP_TUNE_HIST_WEIGHT21_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_HIST_WEIGHT21_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_HIST_WEIGHT21_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_HIST_WEIGHT21_W1SC_MASK 0x00000000U
/**
 * VALUE [7:0], dòng bảng 1174, kiểu truy cập RW
 * Trọng số không gian 5x5 theo thứ tự ảnh; 0 loại ô, giá trị khác 0 được cộng vào bin.
 */
#define FX1_ISP_TUNE_HIST_WEIGHT21_VALUE_SHIFT 0U
#define FX1_ISP_TUNE_HIST_WEIGHT21_VALUE_WIDTH 8U
#define FX1_ISP_TUNE_HIST_WEIGHT21_VALUE_MASK 0x000000FFU
#define FX1_ISP_TUNE_HIST_WEIGHT21_VALUE_RESET 0x00000001U
#define FX1_ISP_TUNE_HIST_WEIGHT21_VALUE_ACCESS FX1_ISP_ACCESS_RW

/**
 * TUNE_HIST_WEIGHT22: địa chỉ lệch 0x8698, dòng bảng 1175, kiểu truy cập RW
 * Trọng số không gian 5x5 theo hàng trước/cột sau. 0 loại ô, khác 0 cộng trọng số đó vào bin. Cập
 * nhật theo profile, MAS v1.07 tuning-ae.
 */
#define FX1_ISP_TUNE_HIST_WEIGHT22_OFFSET 0x8698U
#define REG_TUNE_HIST_WEIGHT22 FX1_ISP_TUNE_HIST_WEIGHT22_OFFSET
#define FX1_ISP_TUNE_HIST_WEIGHT22_RESET 0x00000001U
#define FX1_ISP_TUNE_HIST_WEIGHT22_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_TUNE_HIST_WEIGHT22_RO_MASK 0x00000000U
#define FX1_ISP_TUNE_HIST_WEIGHT22_RW_MASK 0x000000FFU
#define FX1_ISP_TUNE_HIST_WEIGHT22_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_HIST_WEIGHT22_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_HIST_WEIGHT22_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_HIST_WEIGHT22_W1SC_MASK 0x00000000U
/**
 * VALUE [7:0], dòng bảng 1176, kiểu truy cập RW
 * Trọng số không gian 5x5 theo thứ tự ảnh; 0 loại ô, giá trị khác 0 được cộng vào bin.
 */
#define FX1_ISP_TUNE_HIST_WEIGHT22_VALUE_SHIFT 0U
#define FX1_ISP_TUNE_HIST_WEIGHT22_VALUE_WIDTH 8U
#define FX1_ISP_TUNE_HIST_WEIGHT22_VALUE_MASK 0x000000FFU
#define FX1_ISP_TUNE_HIST_WEIGHT22_VALUE_RESET 0x00000001U
#define FX1_ISP_TUNE_HIST_WEIGHT22_VALUE_ACCESS FX1_ISP_ACCESS_RW

/**
 * TUNE_HIST_WEIGHT23: địa chỉ lệch 0x869C, dòng bảng 1177, kiểu truy cập RW
 * Trọng số không gian 5x5 theo hàng trước/cột sau. 0 loại ô, khác 0 cộng trọng số đó vào bin. Cập
 * nhật theo profile, MAS v1.07 tuning-ae.
 */
#define FX1_ISP_TUNE_HIST_WEIGHT23_OFFSET 0x869CU
#define REG_TUNE_HIST_WEIGHT23 FX1_ISP_TUNE_HIST_WEIGHT23_OFFSET
#define FX1_ISP_TUNE_HIST_WEIGHT23_RESET 0x00000001U
#define FX1_ISP_TUNE_HIST_WEIGHT23_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_TUNE_HIST_WEIGHT23_RO_MASK 0x00000000U
#define FX1_ISP_TUNE_HIST_WEIGHT23_RW_MASK 0x000000FFU
#define FX1_ISP_TUNE_HIST_WEIGHT23_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_HIST_WEIGHT23_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_HIST_WEIGHT23_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_HIST_WEIGHT23_W1SC_MASK 0x00000000U
/**
 * VALUE [7:0], dòng bảng 1178, kiểu truy cập RW
 * Trọng số không gian 5x5 theo thứ tự ảnh; 0 loại ô, giá trị khác 0 được cộng vào bin.
 */
#define FX1_ISP_TUNE_HIST_WEIGHT23_VALUE_SHIFT 0U
#define FX1_ISP_TUNE_HIST_WEIGHT23_VALUE_WIDTH 8U
#define FX1_ISP_TUNE_HIST_WEIGHT23_VALUE_MASK 0x000000FFU
#define FX1_ISP_TUNE_HIST_WEIGHT23_VALUE_RESET 0x00000001U
#define FX1_ISP_TUNE_HIST_WEIGHT23_VALUE_ACCESS FX1_ISP_ACCESS_RW

/**
 * TUNE_HIST_WEIGHT24: địa chỉ lệch 0x86A0, dòng bảng 1179, kiểu truy cập RW
 * Trọng số không gian 5x5 theo hàng trước/cột sau. 0 loại ô, khác 0 cộng trọng số đó vào bin. Cập
 * nhật theo profile, MAS v1.07 tuning-ae.
 */
#define FX1_ISP_TUNE_HIST_WEIGHT24_OFFSET 0x86A0U
#define REG_TUNE_HIST_WEIGHT24 FX1_ISP_TUNE_HIST_WEIGHT24_OFFSET
#define FX1_ISP_TUNE_HIST_WEIGHT24_RESET 0x00000001U
#define FX1_ISP_TUNE_HIST_WEIGHT24_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_TUNE_HIST_WEIGHT24_RO_MASK 0x00000000U
#define FX1_ISP_TUNE_HIST_WEIGHT24_RW_MASK 0x000000FFU
#define FX1_ISP_TUNE_HIST_WEIGHT24_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_HIST_WEIGHT24_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_HIST_WEIGHT24_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_HIST_WEIGHT24_W1SC_MASK 0x00000000U
/**
 * VALUE [7:0], dòng bảng 1180, kiểu truy cập RW
 * Trọng số không gian 5x5 theo thứ tự ảnh; 0 loại ô, giá trị khác 0 được cộng vào bin.
 */
#define FX1_ISP_TUNE_HIST_WEIGHT24_VALUE_SHIFT 0U
#define FX1_ISP_TUNE_HIST_WEIGHT24_VALUE_WIDTH 8U
#define FX1_ISP_TUNE_HIST_WEIGHT24_VALUE_MASK 0x000000FFU
#define FX1_ISP_TUNE_HIST_WEIGHT24_VALUE_RESET 0x00000001U
#define FX1_ISP_TUNE_HIST_WEIGHT24_VALUE_ACCESS FX1_ISP_ACCESS_RW

/**
 * TUNE_AEC_CMD: địa chỉ lệch 0x8700, dòng bảng 1181, kiểu truy cập W1P
 * Quyền đọc hai bank theo thứ tự hoàn tất; ghi cả hai thì RELEASE trước ACQUIRE. Lệnh theo MAS v1.07
 * tuning-ae.
 */
#define FX1_ISP_TUNE_AEC_CMD_OFFSET 0x8700U
#define REG_TUNE_AEC_CMD FX1_ISP_TUNE_AEC_CMD_OFFSET
#define FX1_ISP_TUNE_AEC_CMD_RESET 0x00000000U
#define FX1_ISP_TUNE_AEC_CMD_ACCESS FX1_ISP_ACCESS_W1P
#define FX1_ISP_TUNE_AEC_CMD_RO_MASK 0x00000000U
#define FX1_ISP_TUNE_AEC_CMD_RW_MASK 0x00000000U
#define FX1_ISP_TUNE_AEC_CMD_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_AEC_CMD_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_AEC_CMD_W1P_MASK 0x00000003U
#define FX1_ISP_TUNE_AEC_CMD_W1SC_MASK 0x00000000U
/**
 * ACQUIRE [0:0], dòng bảng 1182, kiểu truy cập W1P
 * Xin quyền bank READY hoàn tất sớm nhất; từ chối xin quyền lặp.
 */
#define FX1_ISP_TUNE_AEC_CMD_ACQUIRE_SHIFT 0U
#define FX1_ISP_TUNE_AEC_CMD_ACQUIRE_WIDTH 1U
#define FX1_ISP_TUNE_AEC_CMD_ACQUIRE_MASK 0x00000001U
#define FX1_ISP_TUNE_AEC_CMD_ACQUIRE_RESET 0x00000000U
#define FX1_ISP_TUNE_AEC_CMD_ACQUIRE_ACCESS FX1_ISP_ACCESS_W1P
/**
 * RELEASE [1:1], dòng bảng 1183, kiểu truy cập W1P
 * Giải phóng quyền hiện tại; chưa giữ thì đặt NO_LEASE.
 */
#define FX1_ISP_TUNE_AEC_CMD_RELEASE_SHIFT 1U
#define FX1_ISP_TUNE_AEC_CMD_RELEASE_WIDTH 1U
#define FX1_ISP_TUNE_AEC_CMD_RELEASE_MASK 0x00000002U
#define FX1_ISP_TUNE_AEC_CMD_RELEASE_RESET 0x00000000U
#define FX1_ISP_TUNE_AEC_CMD_RELEASE_ACCESS FX1_ISP_ACCESS_W1P

/**
 * TUNE_AEC_STATUS: địa chỉ lệch 0x8704, dòng bảng 1184, kiểu truy cập RO
 * Trạng thái quyền đọc trực tiếp, MAS v1.07 tuning-ae.
 */
#define FX1_ISP_TUNE_AEC_STATUS_OFFSET 0x8704U
#define REG_TUNE_AEC_STATUS FX1_ISP_TUNE_AEC_STATUS_OFFSET
#define FX1_ISP_TUNE_AEC_STATUS_RESET 0x00000000U
#define FX1_ISP_TUNE_AEC_STATUS_ACCESS FX1_ISP_ACCESS_RO
#define FX1_ISP_TUNE_AEC_STATUS_RO_MASK 0x0000000FU
#define FX1_ISP_TUNE_AEC_STATUS_RW_MASK 0x00000000U
#define FX1_ISP_TUNE_AEC_STATUS_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_AEC_STATUS_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_AEC_STATUS_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_AEC_STATUS_W1SC_MASK 0x00000000U
/**
 * LEASE_VALID [0:0], dòng bảng 1185, kiểu truy cập RO
 * Đang tồn tại quyền đọc.
 */
#define FX1_ISP_TUNE_AEC_STATUS_LEASE_VALID_SHIFT 0U
#define FX1_ISP_TUNE_AEC_STATUS_LEASE_VALID_WIDTH 1U
#define FX1_ISP_TUNE_AEC_STATUS_LEASE_VALID_MASK 0x00000001U
#define FX1_ISP_TUNE_AEC_STATUS_LEASE_VALID_RESET 0x00000000U
#define FX1_ISP_TUNE_AEC_STATUS_LEASE_VALID_ACCESS FX1_ISP_ACCESS_RO
/**
 * READY_COUNT [2:1], dòng bảng 1186, kiểu truy cập RO
 * Số bank READY chưa cấp quyền, 0..2.
 */
#define FX1_ISP_TUNE_AEC_STATUS_READY_COUNT_SHIFT 1U
#define FX1_ISP_TUNE_AEC_STATUS_READY_COUNT_WIDTH 2U
#define FX1_ISP_TUNE_AEC_STATUS_READY_COUNT_MASK 0x00000006U
#define FX1_ISP_TUNE_AEC_STATUS_READY_COUNT_RESET 0x00000000U
#define FX1_ISP_TUNE_AEC_STATUS_READY_COUNT_ACCESS FX1_ISP_ACCESS_RO
/**
 * BUSY [3:3], dòng bảng 1187, kiểu truy cập RO
 * Một bank đang nhận dữ liệu hoặc xử lý hết các cập nhật cuối.
 */
#define FX1_ISP_TUNE_AEC_STATUS_BUSY_SHIFT 3U
#define FX1_ISP_TUNE_AEC_STATUS_BUSY_WIDTH 1U
#define FX1_ISP_TUNE_AEC_STATUS_BUSY_MASK 0x00000008U
#define FX1_ISP_TUNE_AEC_STATUS_BUSY_RESET 0x00000000U
#define FX1_ISP_TUNE_AEC_STATUS_BUSY_ACCESS FX1_ISP_ACCESS_RO

/**
 * TUNE_AEC_DROPS: địa chỉ lệch 0x8708, dòng bảng 1188, kiểu truy cập W1C
 * Bộ đếm phép đo bị bỏ dạng kẹp; ghi W1C khác 0 xóa cả từ, mất mới ưu tiên hơn xóa. Cập nhật trực
 * tiếp, MAS v1.07 tuning-ae.
 */
#define FX1_ISP_TUNE_AEC_DROPS_OFFSET 0x8708U
#define REG_TUNE_AEC_DROPS FX1_ISP_TUNE_AEC_DROPS_OFFSET
#define FX1_ISP_TUNE_AEC_DROPS_RESET 0x00000000U
#define FX1_ISP_TUNE_AEC_DROPS_ACCESS FX1_ISP_ACCESS_W1C
#define FX1_ISP_TUNE_AEC_DROPS_RO_MASK 0x00000000U
#define FX1_ISP_TUNE_AEC_DROPS_RW_MASK 0x00000000U
#define FX1_ISP_TUNE_AEC_DROPS_W1C_MASK 0xFFFFFFFFU
#define FX1_ISP_TUNE_AEC_DROPS_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_AEC_DROPS_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_AEC_DROPS_W1SC_MASK 0x00000000U
/**
 * VALUE [31:0], dòng bảng 1189, kiểu truy cập W1C
 * Bộ đếm phép đo bị bỏ dạng kẹp; W1C khác 0 xóa cả từ, mất mới thắng xóa.
 */
#define FX1_ISP_TUNE_AEC_DROPS_VALUE_SHIFT 0U
#define FX1_ISP_TUNE_AEC_DROPS_VALUE_WIDTH 32U
#define FX1_ISP_TUNE_AEC_DROPS_VALUE_MASK 0xFFFFFFFFU
#define FX1_ISP_TUNE_AEC_DROPS_VALUE_RESET 0x00000000U
#define FX1_ISP_TUNE_AEC_DROPS_VALUE_ACCESS FX1_ISP_ACCESS_W1C

/**
 * TUNE_AEC_FRAME: địa chỉ lệch 0x870C, dòng bảng 1190, kiểu truy cập RO
 * ID frame đầu vào đang giữ quyền, chưa giữ đọc 0; cập nhật theo kết quả, MAS v1.07 tuning-ae.
 */
#define FX1_ISP_TUNE_AEC_FRAME_OFFSET 0x870CU
#define REG_TUNE_AEC_FRAME FX1_ISP_TUNE_AEC_FRAME_OFFSET
#define FX1_ISP_TUNE_AEC_FRAME_RESET 0x00000000U
#define FX1_ISP_TUNE_AEC_FRAME_ACCESS FX1_ISP_ACCESS_RO
#define FX1_ISP_TUNE_AEC_FRAME_RO_MASK 0xFFFFFFFFU
#define FX1_ISP_TUNE_AEC_FRAME_RW_MASK 0x00000000U
#define FX1_ISP_TUNE_AEC_FRAME_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_AEC_FRAME_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_AEC_FRAME_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_AEC_FRAME_W1SC_MASK 0x00000000U
/**
 * VALUE [31:0], dòng bảng 1191, kiểu truy cập RO
 * ID frame đầu vào đang giữ quyền; chưa giữ đọc 0.
 */
#define FX1_ISP_TUNE_AEC_FRAME_VALUE_SHIFT 0U
#define FX1_ISP_TUNE_AEC_FRAME_VALUE_WIDTH 32U
#define FX1_ISP_TUNE_AEC_FRAME_VALUE_MASK 0xFFFFFFFFU
#define FX1_ISP_TUNE_AEC_FRAME_VALUE_RESET 0x00000000U
#define FX1_ISP_TUNE_AEC_FRAME_VALUE_ACCESS FX1_ISP_ACCESS_RO

/**
 * TUNE_AEC_PROFILE: địa chỉ lệch 0x8710, dòng bảng 1192, kiểu truy cập RO
 * ID profile phần cứng đã áp cho kết quả đang giữ quyền, chưa giữ đọc 0; cập nhật theo kết quả, MAS
 * v1.07 tuning-ae.
 */
#define FX1_ISP_TUNE_AEC_PROFILE_OFFSET 0x8710U
#define REG_TUNE_AEC_PROFILE FX1_ISP_TUNE_AEC_PROFILE_OFFSET
#define FX1_ISP_TUNE_AEC_PROFILE_RESET 0x00000000U
#define FX1_ISP_TUNE_AEC_PROFILE_ACCESS FX1_ISP_ACCESS_RO
#define FX1_ISP_TUNE_AEC_PROFILE_RO_MASK 0xFFFFFFFFU
#define FX1_ISP_TUNE_AEC_PROFILE_RW_MASK 0x00000000U
#define FX1_ISP_TUNE_AEC_PROFILE_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_AEC_PROFILE_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_AEC_PROFILE_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_AEC_PROFILE_W1SC_MASK 0x00000000U
/**
 * VALUE [31:0], dòng bảng 1193, kiểu truy cập RO
 * ID profile phần cứng đã áp cho bank đang giữ quyền; chưa giữ đọc 0.
 */
#define FX1_ISP_TUNE_AEC_PROFILE_VALUE_SHIFT 0U
#define FX1_ISP_TUNE_AEC_PROFILE_VALUE_WIDTH 32U
#define FX1_ISP_TUNE_AEC_PROFILE_VALUE_MASK 0xFFFFFFFFU
#define FX1_ISP_TUNE_AEC_PROFILE_VALUE_RESET 0x00000000U
#define FX1_ISP_TUNE_AEC_PROFILE_VALUE_ACCESS FX1_ISP_ACCESS_RO

/**
 * TUNE_AEC_INDEX: địa chỉ lệch 0x8714, dòng bảng 1194, kiểu truy cập RW
 * Chỉ số kết quả không tự tăng; đọc từ thấp trước để chốt cả 64 bit. Cập nhật qua cổng, MAS v1.07
 * tuning-ae.
 */
#define FX1_ISP_TUNE_AEC_INDEX_OFFSET 0x8714U
#define REG_TUNE_AEC_INDEX FX1_ISP_TUNE_AEC_INDEX_OFFSET
#define FX1_ISP_TUNE_AEC_INDEX_RESET 0x00000000U
#define FX1_ISP_TUNE_AEC_INDEX_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_TUNE_AEC_INDEX_RO_MASK 0x00000000U
#define FX1_ISP_TUNE_AEC_INDEX_RW_MASK 0x0000FFFFU
#define FX1_ISP_TUNE_AEC_INDEX_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_AEC_INDEX_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_AEC_INDEX_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_AEC_INDEX_W1SC_MASK 0x00000000U
/**
 * VALUE [15:0], dòng bảng 1195, kiểu truy cập RW
 * Chỉ số kết quả không tự tăng; đọc từ thấp trước để chốt toàn bộ 64 bit.
 */
#define FX1_ISP_TUNE_AEC_INDEX_VALUE_SHIFT 0U
#define FX1_ISP_TUNE_AEC_INDEX_VALUE_WIDTH 16U
#define FX1_ISP_TUNE_AEC_INDEX_VALUE_MASK 0x0000FFFFU
#define FX1_ISP_TUNE_AEC_INDEX_VALUE_RESET 0x00000000U
#define FX1_ISP_TUNE_AEC_INDEX_VALUE_ACCESS FX1_ISP_ACCESS_RW

/**
 * TUNE_AEC_DATA_LO: địa chỉ lệch 0x8718, dòng bảng 1196, kiểu truy cập RO
 * 32 bit thấp kết quả đang chọn của bank giữ quyền; đồng thời chốt từ cao nguyên tử. Cập nhật theo
 * kết quả, MAS v1.07 tuning-ae.
 */
#define FX1_ISP_TUNE_AEC_DATA_LO_OFFSET 0x8718U
#define REG_TUNE_AEC_DATA_LO FX1_ISP_TUNE_AEC_DATA_LO_OFFSET
#define FX1_ISP_TUNE_AEC_DATA_LO_RESET 0x00000000U
#define FX1_ISP_TUNE_AEC_DATA_LO_ACCESS FX1_ISP_ACCESS_RO
#define FX1_ISP_TUNE_AEC_DATA_LO_RO_MASK 0xFFFFFFFFU
#define FX1_ISP_TUNE_AEC_DATA_LO_RW_MASK 0x00000000U
#define FX1_ISP_TUNE_AEC_DATA_LO_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_AEC_DATA_LO_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_AEC_DATA_LO_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_AEC_DATA_LO_W1SC_MASK 0x00000000U
/**
 * VALUE [31:0], dòng bảng 1197, kiểu truy cập RO
 * 32 bit thấp kết quả đang chọn; chốt nguyên tử từ cao.
 */
#define FX1_ISP_TUNE_AEC_DATA_LO_VALUE_SHIFT 0U
#define FX1_ISP_TUNE_AEC_DATA_LO_VALUE_WIDTH 32U
#define FX1_ISP_TUNE_AEC_DATA_LO_VALUE_MASK 0xFFFFFFFFU
#define FX1_ISP_TUNE_AEC_DATA_LO_VALUE_RESET 0x00000000U
#define FX1_ISP_TUNE_AEC_DATA_LO_VALUE_ACCESS FX1_ISP_ACCESS_RO

/**
 * TUNE_AEC_DATA_HI: địa chỉ lệch 0x871C, dòng bảng 1198, kiểu truy cập RO
 * Từ cao của lần chốt bằng đọc từ thấp trước đó. Ghi chỉ số/RELEASE/reset làm mất hiệu lực lần chốt;
 * cập nhật theo kết quả, MAS v1.07 tuning-ae.
 */
#define FX1_ISP_TUNE_AEC_DATA_HI_OFFSET 0x871CU
#define REG_TUNE_AEC_DATA_HI FX1_ISP_TUNE_AEC_DATA_HI_OFFSET
#define FX1_ISP_TUNE_AEC_DATA_HI_RESET 0x00000000U
#define FX1_ISP_TUNE_AEC_DATA_HI_ACCESS FX1_ISP_ACCESS_RO
#define FX1_ISP_TUNE_AEC_DATA_HI_RO_MASK 0xFFFFFFFFU
#define FX1_ISP_TUNE_AEC_DATA_HI_RW_MASK 0x00000000U
#define FX1_ISP_TUNE_AEC_DATA_HI_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_AEC_DATA_HI_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_AEC_DATA_HI_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_AEC_DATA_HI_W1SC_MASK 0x00000000U
/**
 * VALUE [31:0], dòng bảng 1199, kiểu truy cập RO
 * Từ cao của lần chốt bằng đọc từ thấp; mất hiệu lực khi ghi chỉ số/RELEASE/reset.
 */
#define FX1_ISP_TUNE_AEC_DATA_HI_VALUE_SHIFT 0U
#define FX1_ISP_TUNE_AEC_DATA_HI_VALUE_WIDTH 32U
#define FX1_ISP_TUNE_AEC_DATA_HI_VALUE_MASK 0xFFFFFFFFU
#define FX1_ISP_TUNE_AEC_DATA_HI_VALUE_RESET 0x00000000U
#define FX1_ISP_TUNE_AEC_DATA_HI_VALUE_ACCESS FX1_ISP_ACCESS_RO

/**
 * TUNE_AF_CMD: địa chỉ lệch 0x8740, dòng bảng 1200, kiểu truy cập W1P
 * Quyền đọc hai bank AF theo thứ tự hoàn tất, RELEASE trước ACQUIRE khi ghi cả hai; lệnh theo MAS
 * v1.07 tuning-af.
 */
#define FX1_ISP_TUNE_AF_CMD_OFFSET 0x8740U
#define REG_TUNE_AF_CMD FX1_ISP_TUNE_AF_CMD_OFFSET
#define FX1_ISP_TUNE_AF_CMD_RESET 0x00000000U
#define FX1_ISP_TUNE_AF_CMD_ACCESS FX1_ISP_ACCESS_W1P
#define FX1_ISP_TUNE_AF_CMD_RO_MASK 0x00000000U
#define FX1_ISP_TUNE_AF_CMD_RW_MASK 0x00000000U
#define FX1_ISP_TUNE_AF_CMD_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_AF_CMD_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_AF_CMD_W1P_MASK 0x00000003U
#define FX1_ISP_TUNE_AF_CMD_W1SC_MASK 0x00000000U
/**
 * ACQUIRE [0:0], dòng bảng 1201, kiểu truy cập W1P
 * Xin quyền bank READY hoàn tất sớm nhất; từ chối xin quyền lặp.
 */
#define FX1_ISP_TUNE_AF_CMD_ACQUIRE_SHIFT 0U
#define FX1_ISP_TUNE_AF_CMD_ACQUIRE_WIDTH 1U
#define FX1_ISP_TUNE_AF_CMD_ACQUIRE_MASK 0x00000001U
#define FX1_ISP_TUNE_AF_CMD_ACQUIRE_RESET 0x00000000U
#define FX1_ISP_TUNE_AF_CMD_ACQUIRE_ACCESS FX1_ISP_ACCESS_W1P
/**
 * RELEASE [1:1], dòng bảng 1202, kiểu truy cập W1P
 * Giải phóng quyền hiện tại; chưa giữ thì đặt NO_LEASE.
 */
#define FX1_ISP_TUNE_AF_CMD_RELEASE_SHIFT 1U
#define FX1_ISP_TUNE_AF_CMD_RELEASE_WIDTH 1U
#define FX1_ISP_TUNE_AF_CMD_RELEASE_MASK 0x00000002U
#define FX1_ISP_TUNE_AF_CMD_RELEASE_RESET 0x00000000U
#define FX1_ISP_TUNE_AF_CMD_RELEASE_ACCESS FX1_ISP_ACCESS_W1P

/**
 * TUNE_AF_STATUS: địa chỉ lệch 0x8744, dòng bảng 1203, kiểu truy cập RO
 * Trạng thái quyền đọc trực tiếp, MAS v1.07 tuning-af.
 */
#define FX1_ISP_TUNE_AF_STATUS_OFFSET 0x8744U
#define REG_TUNE_AF_STATUS FX1_ISP_TUNE_AF_STATUS_OFFSET
#define FX1_ISP_TUNE_AF_STATUS_RESET 0x00000000U
#define FX1_ISP_TUNE_AF_STATUS_ACCESS FX1_ISP_ACCESS_RO
#define FX1_ISP_TUNE_AF_STATUS_RO_MASK 0x0000000FU
#define FX1_ISP_TUNE_AF_STATUS_RW_MASK 0x00000000U
#define FX1_ISP_TUNE_AF_STATUS_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_AF_STATUS_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_AF_STATUS_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_AF_STATUS_W1SC_MASK 0x00000000U
/**
 * LEASE_VALID [0:0], dòng bảng 1204, kiểu truy cập RO
 * Đang tồn tại quyền đọc.
 */
#define FX1_ISP_TUNE_AF_STATUS_LEASE_VALID_SHIFT 0U
#define FX1_ISP_TUNE_AF_STATUS_LEASE_VALID_WIDTH 1U
#define FX1_ISP_TUNE_AF_STATUS_LEASE_VALID_MASK 0x00000001U
#define FX1_ISP_TUNE_AF_STATUS_LEASE_VALID_RESET 0x00000000U
#define FX1_ISP_TUNE_AF_STATUS_LEASE_VALID_ACCESS FX1_ISP_ACCESS_RO
/**
 * READY_COUNT [2:1], dòng bảng 1205, kiểu truy cập RO
 * Số bank READY chưa cấp quyền, 0..2.
 */
#define FX1_ISP_TUNE_AF_STATUS_READY_COUNT_SHIFT 1U
#define FX1_ISP_TUNE_AF_STATUS_READY_COUNT_WIDTH 2U
#define FX1_ISP_TUNE_AF_STATUS_READY_COUNT_MASK 0x00000006U
#define FX1_ISP_TUNE_AF_STATUS_READY_COUNT_RESET 0x00000000U
#define FX1_ISP_TUNE_AF_STATUS_READY_COUNT_ACCESS FX1_ISP_ACCESS_RO
/**
 * BUSY [3:3], dòng bảng 1206, kiểu truy cập RO
 * Một bank đang nhận dữ liệu hoặc xử lý hết các cập nhật cuối.
 */
#define FX1_ISP_TUNE_AF_STATUS_BUSY_SHIFT 3U
#define FX1_ISP_TUNE_AF_STATUS_BUSY_WIDTH 1U
#define FX1_ISP_TUNE_AF_STATUS_BUSY_MASK 0x00000008U
#define FX1_ISP_TUNE_AF_STATUS_BUSY_RESET 0x00000000U
#define FX1_ISP_TUNE_AF_STATUS_BUSY_ACCESS FX1_ISP_ACCESS_RO

/**
 * TUNE_AF_DROPS: địa chỉ lệch 0x8748, dòng bảng 1207, kiểu truy cập W1C
 * Bộ đếm phép đo AF bị bỏ dạng kẹp; W1C khác 0 xóa cả từ, mất mới thắng xóa. Cập nhật trực tiếp, MAS
 * v1.07 tuning-af.
 */
#define FX1_ISP_TUNE_AF_DROPS_OFFSET 0x8748U
#define REG_TUNE_AF_DROPS FX1_ISP_TUNE_AF_DROPS_OFFSET
#define FX1_ISP_TUNE_AF_DROPS_RESET 0x00000000U
#define FX1_ISP_TUNE_AF_DROPS_ACCESS FX1_ISP_ACCESS_W1C
#define FX1_ISP_TUNE_AF_DROPS_RO_MASK 0x00000000U
#define FX1_ISP_TUNE_AF_DROPS_RW_MASK 0x00000000U
#define FX1_ISP_TUNE_AF_DROPS_W1C_MASK 0xFFFFFFFFU
#define FX1_ISP_TUNE_AF_DROPS_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_AF_DROPS_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_AF_DROPS_W1SC_MASK 0x00000000U
/**
 * VALUE [31:0], dòng bảng 1208, kiểu truy cập W1C
 * Bộ đếm phép đo bị bỏ dạng kẹp; W1C khác 0 xóa cả từ, mất mới thắng xóa.
 */
#define FX1_ISP_TUNE_AF_DROPS_VALUE_SHIFT 0U
#define FX1_ISP_TUNE_AF_DROPS_VALUE_WIDTH 32U
#define FX1_ISP_TUNE_AF_DROPS_VALUE_MASK 0xFFFFFFFFU
#define FX1_ISP_TUNE_AF_DROPS_VALUE_RESET 0x00000000U
#define FX1_ISP_TUNE_AF_DROPS_VALUE_ACCESS FX1_ISP_ACCESS_W1C

/**
 * TUNE_AF_FRAME: địa chỉ lệch 0x874C, dòng bảng 1209, kiểu truy cập RO
 * ID frame đầu vào của bank AF đang giữ quyền, chưa giữ đọc 0; cập nhật theo kết quả, MAS v1.07
 * tuning-af.
 */
#define FX1_ISP_TUNE_AF_FRAME_OFFSET 0x874CU
#define REG_TUNE_AF_FRAME FX1_ISP_TUNE_AF_FRAME_OFFSET
#define FX1_ISP_TUNE_AF_FRAME_RESET 0x00000000U
#define FX1_ISP_TUNE_AF_FRAME_ACCESS FX1_ISP_ACCESS_RO
#define FX1_ISP_TUNE_AF_FRAME_RO_MASK 0xFFFFFFFFU
#define FX1_ISP_TUNE_AF_FRAME_RW_MASK 0x00000000U
#define FX1_ISP_TUNE_AF_FRAME_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_AF_FRAME_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_AF_FRAME_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_AF_FRAME_W1SC_MASK 0x00000000U
/**
 * VALUE [31:0], dòng bảng 1210, kiểu truy cập RO
 * ID frame đầu vào đang giữ quyền; chưa giữ đọc 0.
 */
#define FX1_ISP_TUNE_AF_FRAME_VALUE_SHIFT 0U
#define FX1_ISP_TUNE_AF_FRAME_VALUE_WIDTH 32U
#define FX1_ISP_TUNE_AF_FRAME_VALUE_MASK 0xFFFFFFFFU
#define FX1_ISP_TUNE_AF_FRAME_VALUE_RESET 0x00000000U
#define FX1_ISP_TUNE_AF_FRAME_VALUE_ACCESS FX1_ISP_ACCESS_RO

/**
 * TUNE_AF_PROFILE: địa chỉ lệch 0x8750, dòng bảng 1211, kiểu truy cập RO
 * ID profile phần cứng đã áp cho bank AF đang giữ quyền, chưa giữ đọc 0; cập nhật theo kết quả, MAS
 * v1.07 tuning-af.
 */
#define FX1_ISP_TUNE_AF_PROFILE_OFFSET 0x8750U
#define REG_TUNE_AF_PROFILE FX1_ISP_TUNE_AF_PROFILE_OFFSET
#define FX1_ISP_TUNE_AF_PROFILE_RESET 0x00000000U
#define FX1_ISP_TUNE_AF_PROFILE_ACCESS FX1_ISP_ACCESS_RO
#define FX1_ISP_TUNE_AF_PROFILE_RO_MASK 0xFFFFFFFFU
#define FX1_ISP_TUNE_AF_PROFILE_RW_MASK 0x00000000U
#define FX1_ISP_TUNE_AF_PROFILE_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_AF_PROFILE_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_AF_PROFILE_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_AF_PROFILE_W1SC_MASK 0x00000000U
/**
 * VALUE [31:0], dòng bảng 1212, kiểu truy cập RO
 * ID profile phần cứng đã áp cho bank đang giữ quyền; chưa giữ đọc 0.
 */
#define FX1_ISP_TUNE_AF_PROFILE_VALUE_SHIFT 0U
#define FX1_ISP_TUNE_AF_PROFILE_VALUE_WIDTH 32U
#define FX1_ISP_TUNE_AF_PROFILE_VALUE_MASK 0xFFFFFFFFU
#define FX1_ISP_TUNE_AF_PROFILE_VALUE_RESET 0x00000000U
#define FX1_ISP_TUNE_AF_PROFILE_VALUE_ACCESS FX1_ISP_ACCESS_RO

/**
 * TUNE_AF_INDEX: địa chỉ lệch 0x8754, dòng bảng 1213, kiểu truy cập RW
 * Chỉ số kết quả AF không tự tăng; đọc thấp trước để chốt 64 bit. Cập nhật qua cổng, MAS v1.07
 * tuning-af.
 */
#define FX1_ISP_TUNE_AF_INDEX_OFFSET 0x8754U
#define REG_TUNE_AF_INDEX FX1_ISP_TUNE_AF_INDEX_OFFSET
#define FX1_ISP_TUNE_AF_INDEX_RESET 0x00000000U
#define FX1_ISP_TUNE_AF_INDEX_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_TUNE_AF_INDEX_RO_MASK 0x00000000U
#define FX1_ISP_TUNE_AF_INDEX_RW_MASK 0x0000FFFFU
#define FX1_ISP_TUNE_AF_INDEX_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_AF_INDEX_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_AF_INDEX_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_AF_INDEX_W1SC_MASK 0x00000000U
/**
 * VALUE [15:0], dòng bảng 1214, kiểu truy cập RW
 * Chỉ số kết quả không tự tăng; đọc từ thấp trước để chốt toàn bộ 64 bit.
 */
#define FX1_ISP_TUNE_AF_INDEX_VALUE_SHIFT 0U
#define FX1_ISP_TUNE_AF_INDEX_VALUE_WIDTH 16U
#define FX1_ISP_TUNE_AF_INDEX_VALUE_MASK 0x0000FFFFU
#define FX1_ISP_TUNE_AF_INDEX_VALUE_RESET 0x00000000U
#define FX1_ISP_TUNE_AF_INDEX_VALUE_ACCESS FX1_ISP_ACCESS_RW

/**
 * TUNE_AF_DATA_LO: địa chỉ lệch 0x8758, dòng bảng 1215, kiểu truy cập RO
 * 32 bit thấp kết quả AF đang chọn, chốt nguyên tử từ cao; cập nhật theo kết quả, MAS v1.07
 * tuning-af.
 */
#define FX1_ISP_TUNE_AF_DATA_LO_OFFSET 0x8758U
#define REG_TUNE_AF_DATA_LO FX1_ISP_TUNE_AF_DATA_LO_OFFSET
#define FX1_ISP_TUNE_AF_DATA_LO_RESET 0x00000000U
#define FX1_ISP_TUNE_AF_DATA_LO_ACCESS FX1_ISP_ACCESS_RO
#define FX1_ISP_TUNE_AF_DATA_LO_RO_MASK 0xFFFFFFFFU
#define FX1_ISP_TUNE_AF_DATA_LO_RW_MASK 0x00000000U
#define FX1_ISP_TUNE_AF_DATA_LO_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_AF_DATA_LO_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_AF_DATA_LO_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_AF_DATA_LO_W1SC_MASK 0x00000000U
/**
 * VALUE [31:0], dòng bảng 1216, kiểu truy cập RO
 * 32 bit thấp kết quả đang chọn; chốt nguyên tử từ cao.
 */
#define FX1_ISP_TUNE_AF_DATA_LO_VALUE_SHIFT 0U
#define FX1_ISP_TUNE_AF_DATA_LO_VALUE_WIDTH 32U
#define FX1_ISP_TUNE_AF_DATA_LO_VALUE_MASK 0xFFFFFFFFU
#define FX1_ISP_TUNE_AF_DATA_LO_VALUE_RESET 0x00000000U
#define FX1_ISP_TUNE_AF_DATA_LO_VALUE_ACCESS FX1_ISP_ACCESS_RO

/**
 * TUNE_AF_DATA_HI: địa chỉ lệch 0x875C, dòng bảng 1217, kiểu truy cập RO
 * Từ cao lần chốt AF bằng đọc thấp trước; ghi chỉ số/RELEASE/reset làm mất hiệu lực. Cập nhật theo
 * kết quả, MAS v1.07 tuning-af.
 */
#define FX1_ISP_TUNE_AF_DATA_HI_OFFSET 0x875CU
#define REG_TUNE_AF_DATA_HI FX1_ISP_TUNE_AF_DATA_HI_OFFSET
#define FX1_ISP_TUNE_AF_DATA_HI_RESET 0x00000000U
#define FX1_ISP_TUNE_AF_DATA_HI_ACCESS FX1_ISP_ACCESS_RO
#define FX1_ISP_TUNE_AF_DATA_HI_RO_MASK 0xFFFFFFFFU
#define FX1_ISP_TUNE_AF_DATA_HI_RW_MASK 0x00000000U
#define FX1_ISP_TUNE_AF_DATA_HI_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_AF_DATA_HI_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_AF_DATA_HI_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_AF_DATA_HI_W1SC_MASK 0x00000000U
/**
 * VALUE [31:0], dòng bảng 1218, kiểu truy cập RO
 * Từ cao của lần chốt bằng đọc từ thấp; mất hiệu lực khi ghi chỉ số/RELEASE/reset.
 */
#define FX1_ISP_TUNE_AF_DATA_HI_VALUE_SHIFT 0U
#define FX1_ISP_TUNE_AF_DATA_HI_VALUE_WIDTH 32U
#define FX1_ISP_TUNE_AF_DATA_HI_VALUE_MASK 0xFFFFFFFFU
#define FX1_ISP_TUNE_AF_DATA_HI_VALUE_RESET 0x00000000U
#define FX1_ISP_TUNE_AF_DATA_HI_VALUE_ACCESS FX1_ISP_ACCESS_RO

/**
 * TUNE_AWB_PROFILE: địa chỉ lệch 0x8780, dòng bảng 1219, kiểu truy cập RO
 * ID profile trong bank AWB đang giữ quyền; ID frame hiện tại không đổi. Cập nhật theo kết quả, MAS
 * v1.07 tuning-awb.
 */
#define FX1_ISP_TUNE_AWB_PROFILE_OFFSET 0x8780U
#define REG_TUNE_AWB_PROFILE FX1_ISP_TUNE_AWB_PROFILE_OFFSET
#define FX1_ISP_TUNE_AWB_PROFILE_RESET 0x00000000U
#define FX1_ISP_TUNE_AWB_PROFILE_ACCESS FX1_ISP_ACCESS_RO
#define FX1_ISP_TUNE_AWB_PROFILE_RO_MASK 0xFFFFFFFFU
#define FX1_ISP_TUNE_AWB_PROFILE_RW_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_PROFILE_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_PROFILE_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_PROFILE_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_PROFILE_W1SC_MASK 0x00000000U
/**
 * VALUE [31:0], dòng bảng 1220, kiểu truy cập RO
 * ID profile của bank AWB đang giữ quyền; ID frame không đổi.
 */
#define FX1_ISP_TUNE_AWB_PROFILE_VALUE_SHIFT 0U
#define FX1_ISP_TUNE_AWB_PROFILE_VALUE_WIDTH 32U
#define FX1_ISP_TUNE_AWB_PROFILE_VALUE_MASK 0xFFFFFFFFU
#define FX1_ISP_TUNE_AWB_PROFILE_VALUE_RESET 0x00000000U
#define FX1_ISP_TUNE_AWB_PROFILE_VALUE_ACCESS FX1_ISP_ACCESS_RO

/**
 * TUNE_GAMMA_PORT: địa chỉ lệch 0x8800, dòng bảng 1221, kiểu truy cập RW
 * Đường gamma RGB dùng chung 256x12 bit theo bank; cập nhật qua cổng, MAS v1.07 tuning-gamma.
 */
#define FX1_ISP_TUNE_GAMMA_PORT_OFFSET 0x8800U
#define REG_TUNE_GAMMA_PORT FX1_ISP_TUNE_GAMMA_PORT_OFFSET
#define FX1_ISP_TUNE_GAMMA_PORT_RESET 0x00000000U
#define FX1_ISP_TUNE_GAMMA_PORT_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_TUNE_GAMMA_PORT_RO_MASK 0x00000000U
#define FX1_ISP_TUNE_GAMMA_PORT_RW_MASK 0x000000FFU
#define FX1_ISP_TUNE_GAMMA_PORT_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_GAMMA_PORT_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_GAMMA_PORT_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_GAMMA_PORT_W1SC_MASK 0x00000000U
/**
 * INDEX [7:0], dòng bảng 1222, kiểu truy cập RW
 * Chỉ số điểm, không tự tăng.
 */
#define FX1_ISP_TUNE_GAMMA_PORT_INDEX_SHIFT 0U
#define FX1_ISP_TUNE_GAMMA_PORT_INDEX_WIDTH 8U
#define FX1_ISP_TUNE_GAMMA_PORT_INDEX_MASK 0x000000FFU
#define FX1_ISP_TUNE_GAMMA_PORT_INDEX_RESET 0x00000000U
#define FX1_ISP_TUNE_GAMMA_PORT_INDEX_ACCESS FX1_ISP_ACCESS_RW

/**
 * TUNE_GAMMA_DATA: địa chỉ lệch 0x8804, dòng bảng 1223, kiểu truy cập RW
 * Điểm gamma profile đang chọn; SRAM không có nội dung reset xác định, hỗ trợ đọc kiểm tra. Cập nhật
 * qua cổng, MAS v1.07 tuning-gamma.
 */
#define FX1_ISP_TUNE_GAMMA_DATA_OFFSET 0x8804U
#define REG_TUNE_GAMMA_DATA FX1_ISP_TUNE_GAMMA_DATA_OFFSET
#define FX1_ISP_TUNE_GAMMA_DATA_RESET 0x00000000U
#define FX1_ISP_TUNE_GAMMA_DATA_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_TUNE_GAMMA_DATA_RO_MASK 0x00000000U
#define FX1_ISP_TUNE_GAMMA_DATA_RW_MASK 0x00000FFFU
#define FX1_ISP_TUNE_GAMMA_DATA_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_GAMMA_DATA_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_GAMMA_DATA_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_GAMMA_DATA_W1SC_MASK 0x00000000U
/**
 * VALUE [11:0], dòng bảng 1224, kiểu truy cập RW
 * Điểm gamma profile đang chọn; SRAM không được khởi tạo bởi reset, hỗ trợ đọc lại.
 */
#define FX1_ISP_TUNE_GAMMA_DATA_VALUE_SHIFT 0U
#define FX1_ISP_TUNE_GAMMA_DATA_VALUE_WIDTH 12U
#define FX1_ISP_TUNE_GAMMA_DATA_VALUE_MASK 0x00000FFFU
#define FX1_ISP_TUNE_GAMMA_DATA_VALUE_RESET 0x00000000U
#define FX1_ISP_TUNE_GAMMA_DATA_VALUE_ACCESS FX1_ISP_ACCESS_RW

/**
 * TUNE_TABLE_CMD: địa chỉ lệch 0x8808, dòng bảng 1225, kiểu truy cập W1P
 * Xác nhận bảng đầy đủ sau ghi/đọc kiểm tra; phần cứng kiểm tra dữ liệu theo MAS. Thất bại giữ cờ 0
 * và đặt NOT_READY; bank đã SEAL/pending/tham chiếu bị LOCKED và không đổi. Lệnh profile, legacy từ
 * chối MODE; MAS v1.07 tuning-profiles.
 */
#define FX1_ISP_TUNE_TABLE_CMD_OFFSET 0x8808U
#define REG_TUNE_TABLE_CMD FX1_ISP_TUNE_TABLE_CMD_OFFSET
#define FX1_ISP_TUNE_TABLE_CMD_RESET 0x00000000U
#define FX1_ISP_TUNE_TABLE_CMD_ACCESS FX1_ISP_ACCESS_W1P
#define FX1_ISP_TUNE_TABLE_CMD_RO_MASK 0x00000000U
#define FX1_ISP_TUNE_TABLE_CMD_RW_MASK 0x00000000U
#define FX1_ISP_TUNE_TABLE_CMD_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_TABLE_CMD_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_TABLE_CMD_W1P_MASK 0x0000000FU
#define FX1_ISP_TUNE_TABLE_CMD_W1SC_MASK 0x00000000U
/**
 * GAMMA_CONFIRM [0:0], dòng bảng 1226, kiểu truy cập W1P
 * Yêu cầu đủ 256 điểm gamma.
 */
#define FX1_ISP_TUNE_TABLE_CMD_GAMMA_CONFIRM_SHIFT 0U
#define FX1_ISP_TUNE_TABLE_CMD_GAMMA_CONFIRM_WIDTH 1U
#define FX1_ISP_TUNE_TABLE_CMD_GAMMA_CONFIRM_MASK 0x00000001U
#define FX1_ISP_TUNE_TABLE_CMD_GAMMA_CONFIRM_RESET 0x00000000U
#define FX1_ISP_TUNE_TABLE_CMD_GAMMA_CONFIRM_ACCESS FX1_ISP_ACCESS_W1P
/**
 * NR_CONFIRM [1:1], dòng bảng 1227, kiểu truy cập W1P
 * Yêu cầu đủ 17 điểm phương sai.
 */
#define FX1_ISP_TUNE_TABLE_CMD_NR_CONFIRM_SHIFT 1U
#define FX1_ISP_TUNE_TABLE_CMD_NR_CONFIRM_WIDTH 1U
#define FX1_ISP_TUNE_TABLE_CMD_NR_CONFIRM_MASK 0x00000002U
#define FX1_ISP_TUNE_TABLE_CMD_NR_CONFIRM_RESET 0x00000000U
#define FX1_ISP_TUNE_TABLE_CMD_NR_CONFIRM_ACCESS FX1_ISP_ACCESS_W1P
/**
 * BPC_CONFIRM [2:2], dòng bảng 1228, kiểu truy cập W1P
 * Yêu cầu TUNE_BPC_COUNT<=1024 điểm đã ghi, thứ tự ảnh tăng nghiêm ngặt, mọi X<W và Y<H.
 */
#define FX1_ISP_TUNE_TABLE_CMD_BPC_CONFIRM_SHIFT 2U
#define FX1_ISP_TUNE_TABLE_CMD_BPC_CONFIRM_WIDTH 1U
#define FX1_ISP_TUNE_TABLE_CMD_BPC_CONFIRM_MASK 0x00000004U
#define FX1_ISP_TUNE_TABLE_CMD_BPC_CONFIRM_RESET 0x00000000U
#define FX1_ISP_TUNE_TABLE_CMD_BPC_CONFIRM_ACCESS FX1_ISP_ACCESS_W1P
/**
 * LSC_CONFIRM [3:3], dòng bảng 1229, kiểu truy cập W1P
 * Mọi khoảng sector phải được ghi, khác 0 và phủ đủ lưới; hoàn tất sau khi tạo xong các hằng trọng
 * số từng khoảng.
 */
#define FX1_ISP_TUNE_TABLE_CMD_LSC_CONFIRM_SHIFT 3U
#define FX1_ISP_TUNE_TABLE_CMD_LSC_CONFIRM_WIDTH 1U
#define FX1_ISP_TUNE_TABLE_CMD_LSC_CONFIRM_MASK 0x00000008U
#define FX1_ISP_TUNE_TABLE_CMD_LSC_CONFIRM_RESET 0x00000000U
#define FX1_ISP_TUNE_TABLE_CMD_LSC_CONFIRM_ACCESS FX1_ISP_ACCESS_W1P

/**
 * TUNE_TABLE_STATUS: địa chỉ lệch 0x880C, dòng bảng 1230, kiểu truy cập RO
 * Các dữ liệu của bank chọn đã xác nhận; ghi dữ liệu xóa xác nhận tương ứng. Cập nhật trực tiếp, MAS
 * v1.07 tuning-profiles.
 */
#define FX1_ISP_TUNE_TABLE_STATUS_OFFSET 0x880CU
#define REG_TUNE_TABLE_STATUS FX1_ISP_TUNE_TABLE_STATUS_OFFSET
#define FX1_ISP_TUNE_TABLE_STATUS_RESET 0x00000000U
#define FX1_ISP_TUNE_TABLE_STATUS_ACCESS FX1_ISP_ACCESS_RO
#define FX1_ISP_TUNE_TABLE_STATUS_RO_MASK 0x0000000FU
#define FX1_ISP_TUNE_TABLE_STATUS_RW_MASK 0x00000000U
#define FX1_ISP_TUNE_TABLE_STATUS_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_TABLE_STATUS_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_TABLE_STATUS_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_TABLE_STATUS_W1SC_MASK 0x00000000U
/**
 * GAMMA [0:0], dòng bảng 1231, kiểu truy cập RO
 * Bảng đã xác nhận.
 */
#define FX1_ISP_TUNE_TABLE_STATUS_GAMMA_SHIFT 0U
#define FX1_ISP_TUNE_TABLE_STATUS_GAMMA_WIDTH 1U
#define FX1_ISP_TUNE_TABLE_STATUS_GAMMA_MASK 0x00000001U
#define FX1_ISP_TUNE_TABLE_STATUS_GAMMA_RESET 0x00000000U
#define FX1_ISP_TUNE_TABLE_STATUS_GAMMA_ACCESS FX1_ISP_ACCESS_RO
/**
 * NR [1:1], dòng bảng 1232, kiểu truy cập RO
 * Bảng đã xác nhận.
 */
#define FX1_ISP_TUNE_TABLE_STATUS_NR_SHIFT 1U
#define FX1_ISP_TUNE_TABLE_STATUS_NR_WIDTH 1U
#define FX1_ISP_TUNE_TABLE_STATUS_NR_MASK 0x00000002U
#define FX1_ISP_TUNE_TABLE_STATUS_NR_RESET 0x00000000U
#define FX1_ISP_TUNE_TABLE_STATUS_NR_ACCESS FX1_ISP_ACCESS_RO
/**
 * BPC [2:2], dòng bảng 1233, kiểu truy cập RO
 * Bảng đã xác nhận.
 */
#define FX1_ISP_TUNE_TABLE_STATUS_BPC_SHIFT 2U
#define FX1_ISP_TUNE_TABLE_STATUS_BPC_WIDTH 1U
#define FX1_ISP_TUNE_TABLE_STATUS_BPC_MASK 0x00000004U
#define FX1_ISP_TUNE_TABLE_STATUS_BPC_RESET 0x00000000U
#define FX1_ISP_TUNE_TABLE_STATUS_BPC_ACCESS FX1_ISP_ACCESS_RO
/**
 * LSC [3:3], dòng bảng 1234, kiểu truy cập RO
 * Bảng đã xác nhận.
 */
#define FX1_ISP_TUNE_TABLE_STATUS_LSC_SHIFT 3U
#define FX1_ISP_TUNE_TABLE_STATUS_LSC_WIDTH 1U
#define FX1_ISP_TUNE_TABLE_STATUS_LSC_MASK 0x00000008U
#define FX1_ISP_TUNE_TABLE_STATUS_LSC_RESET 0x00000000U
#define FX1_ISP_TUNE_TABLE_STATUS_LSC_ACCESS FX1_ISP_ACCESS_RO

/**
 * TUNE_BPC_CTRL: địa chỉ lệch 0x8900, dòng bảng 1235, kiểu truy cập RW
 * Giữ bộ phát hiện BPC hiện có, bổ sung ngưỡng theo màu và bản đồ lỗi nhà máy tùy chọn. Cập nhật
 * theo profile, MAS v1.07 tuning-calibration.
 */
#define FX1_ISP_TUNE_BPC_CTRL_OFFSET 0x8900U
#define REG_TUNE_BPC_CTRL FX1_ISP_TUNE_BPC_CTRL_OFFSET
#define FX1_ISP_TUNE_BPC_CTRL_RESET 0x00000000U
#define FX1_ISP_TUNE_BPC_CTRL_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_TUNE_BPC_CTRL_RO_MASK 0x00000000U
#define FX1_ISP_TUNE_BPC_CTRL_RW_MASK 0x00000003U
#define FX1_ISP_TUNE_BPC_CTRL_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_BPC_CTRL_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_BPC_CTRL_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_BPC_CTRL_W1SC_MASK 0x00000000U
/**
 * COLOR_THRESH_EN [0:0], dòng bảng 1236, kiểu truy cập RW
 * Dùng bộ ngưỡng green/RB phía dưới.
 */
#define FX1_ISP_TUNE_BPC_CTRL_COLOR_THRESH_EN_SHIFT 0U
#define FX1_ISP_TUNE_BPC_CTRL_COLOR_THRESH_EN_WIDTH 1U
#define FX1_ISP_TUNE_BPC_CTRL_COLOR_THRESH_EN_MASK 0x00000001U
#define FX1_ISP_TUNE_BPC_CTRL_COLOR_THRESH_EN_RESET 0x00000000U
#define FX1_ISP_TUNE_BPC_CTRL_COLOR_THRESH_EN_ACCESS FX1_ISP_ACCESS_RW
/**
 * FACTORY_EN [1:1], dòng bảng 1237, kiểu truy cập RW
 * Sửa tọa độ lỗi trong danh sách trước khi xét bộ phát hiện động.
 */
#define FX1_ISP_TUNE_BPC_CTRL_FACTORY_EN_SHIFT 1U
#define FX1_ISP_TUNE_BPC_CTRL_FACTORY_EN_WIDTH 1U
#define FX1_ISP_TUNE_BPC_CTRL_FACTORY_EN_MASK 0x00000002U
#define FX1_ISP_TUNE_BPC_CTRL_FACTORY_EN_RESET 0x00000000U
#define FX1_ISP_TUNE_BPC_CTRL_FACTORY_EN_ACCESS FX1_ISP_ACCESS_RW

/**
 * TUNE_BPC_G_THRESH: địa chỉ lệch 0x8904, dòng bảng 1238, kiểu truy cập RW
 * Ngưỡng theo màu max(FLOOR,(activity*K+128)>>8); cập nhật theo profile, MAS v1.07
 * tuning-calibration.
 */
#define FX1_ISP_TUNE_BPC_G_THRESH_OFFSET 0x8904U
#define REG_TUNE_BPC_G_THRESH FX1_ISP_TUNE_BPC_G_THRESH_OFFSET
#define FX1_ISP_TUNE_BPC_G_THRESH_RESET 0x00000100U
#define FX1_ISP_TUNE_BPC_G_THRESH_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_TUNE_BPC_G_THRESH_RO_MASK 0x00000000U
#define FX1_ISP_TUNE_BPC_G_THRESH_RW_MASK 0x0FFF0FFFU
#define FX1_ISP_TUNE_BPC_G_THRESH_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_BPC_G_THRESH_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_BPC_G_THRESH_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_BPC_G_THRESH_W1SC_MASK 0x00000000U
/**
 * K [11:0], dòng bảng 1239, kiểu truy cập RW
 * Hệ số nhân độ biến thiên không dấu Q4.8.
 */
#define FX1_ISP_TUNE_BPC_G_THRESH_K_SHIFT 0U
#define FX1_ISP_TUNE_BPC_G_THRESH_K_WIDTH 12U
#define FX1_ISP_TUNE_BPC_G_THRESH_K_MASK 0x00000FFFU
#define FX1_ISP_TUNE_BPC_G_THRESH_K_RESET 0x00000100U
#define FX1_ISP_TUNE_BPC_G_THRESH_K_ACCESS FX1_ISP_ACCESS_RW
/**
 * FLOOR [27:16], dòng bảng 1240, kiểu truy cập RW
 * Ngưỡng sàn theo mã RAW12.
 */
#define FX1_ISP_TUNE_BPC_G_THRESH_FLOOR_SHIFT 16U
#define FX1_ISP_TUNE_BPC_G_THRESH_FLOOR_WIDTH 12U
#define FX1_ISP_TUNE_BPC_G_THRESH_FLOOR_MASK 0x0FFF0000U
#define FX1_ISP_TUNE_BPC_G_THRESH_FLOOR_RESET 0x00000000U
#define FX1_ISP_TUNE_BPC_G_THRESH_FLOOR_ACCESS FX1_ISP_ACCESS_RW

/**
 * TUNE_BPC_RB_THRESH: địa chỉ lệch 0x8908, dòng bảng 1241, kiểu truy cập RW
 * Ngưỡng theo màu max(FLOOR,(activity*K+128)>>8); cập nhật theo profile, MAS v1.07
 * tuning-calibration.
 */
#define FX1_ISP_TUNE_BPC_RB_THRESH_OFFSET 0x8908U
#define REG_TUNE_BPC_RB_THRESH FX1_ISP_TUNE_BPC_RB_THRESH_OFFSET
#define FX1_ISP_TUNE_BPC_RB_THRESH_RESET 0x00000100U
#define FX1_ISP_TUNE_BPC_RB_THRESH_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_TUNE_BPC_RB_THRESH_RO_MASK 0x00000000U
#define FX1_ISP_TUNE_BPC_RB_THRESH_RW_MASK 0x0FFF0FFFU
#define FX1_ISP_TUNE_BPC_RB_THRESH_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_BPC_RB_THRESH_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_BPC_RB_THRESH_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_BPC_RB_THRESH_W1SC_MASK 0x00000000U
/**
 * K [11:0], dòng bảng 1242, kiểu truy cập RW
 * Hệ số nhân độ biến thiên không dấu Q4.8.
 */
#define FX1_ISP_TUNE_BPC_RB_THRESH_K_SHIFT 0U
#define FX1_ISP_TUNE_BPC_RB_THRESH_K_WIDTH 12U
#define FX1_ISP_TUNE_BPC_RB_THRESH_K_MASK 0x00000FFFU
#define FX1_ISP_TUNE_BPC_RB_THRESH_K_RESET 0x00000100U
#define FX1_ISP_TUNE_BPC_RB_THRESH_K_ACCESS FX1_ISP_ACCESS_RW
/**
 * FLOOR [27:16], dòng bảng 1243, kiểu truy cập RW
 * Ngưỡng sàn theo mã RAW12.
 */
#define FX1_ISP_TUNE_BPC_RB_THRESH_FLOOR_SHIFT 16U
#define FX1_ISP_TUNE_BPC_RB_THRESH_FLOOR_WIDTH 12U
#define FX1_ISP_TUNE_BPC_RB_THRESH_FLOOR_MASK 0x0FFF0000U
#define FX1_ISP_TUNE_BPC_RB_THRESH_FLOOR_RESET 0x00000000U
#define FX1_ISP_TUNE_BPC_RB_THRESH_FLOOR_ACCESS FX1_ISP_ACCESS_RW

/**
 * TUNE_BPC_COUNT: địa chỉ lệch 0x8910, dòng bảng 1244, kiểu truy cập RW
 * Số tọa độ lỗi nhà máy 0..1024, thứ tự ảnh tăng nghiêm ngặt. BPC_CONFIRM kiểm tra số/thứ tự/biên;
 * ghi xóa xác nhận BPC. Cập nhật theo profile, MAS v1.07 tuning-calibration.
 */
#define FX1_ISP_TUNE_BPC_COUNT_OFFSET 0x8910U
#define REG_TUNE_BPC_COUNT FX1_ISP_TUNE_BPC_COUNT_OFFSET
#define FX1_ISP_TUNE_BPC_COUNT_RESET 0x00000000U
#define FX1_ISP_TUNE_BPC_COUNT_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_TUNE_BPC_COUNT_RO_MASK 0x00000000U
#define FX1_ISP_TUNE_BPC_COUNT_RW_MASK 0x000007FFU
#define FX1_ISP_TUNE_BPC_COUNT_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_BPC_COUNT_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_BPC_COUNT_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_BPC_COUNT_W1SC_MASK 0x00000000U
/**
 * VALUE [10:0], dòng bảng 1245, kiểu truy cập RW
 * Số tọa độ lỗi nhà máy 0..1024, thứ tự ảnh tăng nghiêm ngặt; BPC_CONFIRM kiểm tra số/thứ tự/biên,
 * ghi làm mất xác nhận.
 */
#define FX1_ISP_TUNE_BPC_COUNT_VALUE_SHIFT 0U
#define FX1_ISP_TUNE_BPC_COUNT_VALUE_WIDTH 11U
#define FX1_ISP_TUNE_BPC_COUNT_VALUE_MASK 0x000007FFU
#define FX1_ISP_TUNE_BPC_COUNT_VALUE_RESET 0x00000000U
#define FX1_ISP_TUNE_BPC_COUNT_VALUE_ACCESS FX1_ISP_ACCESS_RW

/**
 * TUNE_BPC_ADDR: địa chỉ lệch 0x8914, dòng bảng 1246, kiểu truy cập RW
 * Chỉ số bảng lỗi 0..1023, không tự tăng; cập nhật qua cổng, MAS v1.07 tuning-calibration.
 */
#define FX1_ISP_TUNE_BPC_ADDR_OFFSET 0x8914U
#define REG_TUNE_BPC_ADDR FX1_ISP_TUNE_BPC_ADDR_OFFSET
#define FX1_ISP_TUNE_BPC_ADDR_RESET 0x00000000U
#define FX1_ISP_TUNE_BPC_ADDR_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_TUNE_BPC_ADDR_RO_MASK 0x00000000U
#define FX1_ISP_TUNE_BPC_ADDR_RW_MASK 0x000003FFU
#define FX1_ISP_TUNE_BPC_ADDR_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_BPC_ADDR_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_BPC_ADDR_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_BPC_ADDR_W1SC_MASK 0x00000000U
/**
 * VALUE [9:0], dòng bảng 1247, kiểu truy cập RW
 * Chỉ số bảng lỗi nhà máy 0..1023, không tự tăng.
 */
#define FX1_ISP_TUNE_BPC_ADDR_VALUE_SHIFT 0U
#define FX1_ISP_TUNE_BPC_ADDR_VALUE_WIDTH 10U
#define FX1_ISP_TUNE_BPC_ADDR_VALUE_MASK 0x000003FFU
#define FX1_ISP_TUNE_BPC_ADDR_VALUE_RESET 0x00000000U
#define FX1_ISP_TUNE_BPC_ADDR_VALUE_ACCESS FX1_ISP_ACCESS_RW

/**
 * TUNE_BPC_DATA: địa chỉ lệch 0x8918, dòng bảng 1248, kiểu truy cập RW
 * Tọa độ lỗi x/y trong Bayer hữu dụng đã chuẩn hóa, dữ liệu bank đang chọn; cập nhật qua cổng, MAS
 * v1.07 tuning-calibration.
 */
#define FX1_ISP_TUNE_BPC_DATA_OFFSET 0x8918U
#define REG_TUNE_BPC_DATA FX1_ISP_TUNE_BPC_DATA_OFFSET
#define FX1_ISP_TUNE_BPC_DATA_RESET 0x00000000U
#define FX1_ISP_TUNE_BPC_DATA_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_TUNE_BPC_DATA_RO_MASK 0x00000000U
#define FX1_ISP_TUNE_BPC_DATA_RW_MASK 0xFFFFFFFFU
#define FX1_ISP_TUNE_BPC_DATA_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_BPC_DATA_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_BPC_DATA_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_BPC_DATA_W1SC_MASK 0x00000000U
/**
 * X [15:0], dòng bảng 1249, kiểu truy cập RW
 * Tọa độ X điểm lỗi trong ảnh Bayer hữu dụng chuẩn hóa: 0<=X<W. Đây là vị trí pixel, không phải đầu
 * mút hình chữ nhật.
 */
#define FX1_ISP_TUNE_BPC_DATA_X_SHIFT 0U
#define FX1_ISP_TUNE_BPC_DATA_X_WIDTH 16U
#define FX1_ISP_TUNE_BPC_DATA_X_MASK 0x0000FFFFU
#define FX1_ISP_TUNE_BPC_DATA_X_RESET 0x00000000U
#define FX1_ISP_TUNE_BPC_DATA_X_ACCESS FX1_ISP_ACCESS_RW
/**
 * Y [31:16], dòng bảng 1250, kiểu truy cập RW
 * Tọa độ Y điểm lỗi trong ảnh Bayer hữu dụng chuẩn hóa: 0<=Y<H. Đây là vị trí pixel, không phải đầu
 * mút hình chữ nhật.
 */
#define FX1_ISP_TUNE_BPC_DATA_Y_SHIFT 16U
#define FX1_ISP_TUNE_BPC_DATA_Y_WIDTH 16U
#define FX1_ISP_TUNE_BPC_DATA_Y_MASK 0xFFFF0000U
#define FX1_ISP_TUNE_BPC_DATA_Y_RESET 0x00000000U
#define FX1_ISP_TUNE_BPC_DATA_Y_ACCESS FX1_ISP_ACCESS_RW

/**
 * TUNE_LSC_CTRL: địa chỉ lệch 0x8A00, dòng bảng 1251, kiểu truy cập RW
 * Giữ bốn mặt phẳng gain LSC, phép nội suy và độ chính xác; cập nhật theo profile, MAS v1.07
 * tuning-lsc.
 */
#define FX1_ISP_TUNE_LSC_CTRL_OFFSET 0x8A00U
#define REG_TUNE_LSC_CTRL FX1_ISP_TUNE_LSC_CTRL_OFFSET
#define FX1_ISP_TUNE_LSC_CTRL_RESET 0x00000000U
#define FX1_ISP_TUNE_LSC_CTRL_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_TUNE_LSC_CTRL_RO_MASK 0x00000000U
#define FX1_ISP_TUNE_LSC_CTRL_RW_MASK 0x00000001U
#define FX1_ISP_TUNE_LSC_CTRL_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_LSC_CTRL_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_LSC_CTRL_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_LSC_CTRL_W1SC_MASK 0x00000000U
/**
 * SECTOR_EN [0:0], dòng bảng 1252, kiểu truy cập RW
 * Dùng kích thước sector không đều thay khoảng DDA đều.
 */
#define FX1_ISP_TUNE_LSC_CTRL_SECTOR_EN_SHIFT 0U
#define FX1_ISP_TUNE_LSC_CTRL_SECTOR_EN_WIDTH 1U
#define FX1_ISP_TUNE_LSC_CTRL_SECTOR_EN_MASK 0x00000001U
#define FX1_ISP_TUNE_LSC_CTRL_SECTOR_EN_RESET 0x00000000U
#define FX1_ISP_TUNE_LSC_CTRL_SECTOR_EN_ACCESS FX1_ISP_ACCESS_RW

/**
 * TUNE_LSC_SECTOR_ADDR: địa chỉ lệch 0x8A04, dòng bảng 1253, kiểu truy cập RW
 * Cổng kích thước sector LSC; MAS v1.07 tuning-lsc.
 */
#define FX1_ISP_TUNE_LSC_SECTOR_ADDR_OFFSET 0x8A04U
#define REG_TUNE_LSC_SECTOR_ADDR FX1_ISP_TUNE_LSC_SECTOR_ADDR_OFFSET
#define FX1_ISP_TUNE_LSC_SECTOR_ADDR_RESET 0x00000000U
#define FX1_ISP_TUNE_LSC_SECTOR_ADDR_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_TUNE_LSC_SECTOR_ADDR_RO_MASK 0x00000000U
#define FX1_ISP_TUNE_LSC_SECTOR_ADDR_RW_MASK 0x0000017FU
#define FX1_ISP_TUNE_LSC_SECTOR_ADDR_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_LSC_SECTOR_ADDR_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_LSC_SECTOR_ADDR_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_LSC_SECTOR_ADDR_W1SC_MASK 0x00000000U
/**
 * INDEX [6:0], dòng bảng 1254, kiểu truy cập RW
 * Chỉ số sector 0..nodes-2, tối đa 30 khi tích hợp giới hạn 32 nút.
 */
#define FX1_ISP_TUNE_LSC_SECTOR_ADDR_INDEX_SHIFT 0U
#define FX1_ISP_TUNE_LSC_SECTOR_ADDR_INDEX_WIDTH 7U
#define FX1_ISP_TUNE_LSC_SECTOR_ADDR_INDEX_MASK 0x0000007FU
#define FX1_ISP_TUNE_LSC_SECTOR_ADDR_INDEX_RESET 0x00000000U
#define FX1_ISP_TUNE_LSC_SECTOR_ADDR_INDEX_ACCESS FX1_ISP_ACCESS_RW
/**
 * AXIS [8:8], dòng bảng 1255, kiểu truy cập RW
 * Chọn trục: 0=X,1=Y.
 */
#define FX1_ISP_TUNE_LSC_SECTOR_ADDR_AXIS_SHIFT 8U
#define FX1_ISP_TUNE_LSC_SECTOR_ADDR_AXIS_WIDTH 1U
#define FX1_ISP_TUNE_LSC_SECTOR_ADDR_AXIS_MASK 0x00000100U
#define FX1_ISP_TUNE_LSC_SECTOR_ADDR_AXIS_RESET 0x00000000U
#define FX1_ISP_TUNE_LSC_SECTOR_ADDR_AXIS_ACCESS FX1_ISP_ACCESS_RW

/**
 * TUNE_LSC_SECTOR_DATA: địa chỉ lệch 0x8A08, dòng bảng 1256, kiểu truy cập RW
 * Độ dài khoảng dương theo miền tọa độ LSC; tổng bằng tọa độ nút cuối trừ nút đầu. Cập nhật qua
 * cổng, MAS v1.07 tuning-lsc.
 */
#define FX1_ISP_TUNE_LSC_SECTOR_DATA_OFFSET 0x8A08U
#define REG_TUNE_LSC_SECTOR_DATA FX1_ISP_TUNE_LSC_SECTOR_DATA_OFFSET
#define FX1_ISP_TUNE_LSC_SECTOR_DATA_RESET 0x00000000U
#define FX1_ISP_TUNE_LSC_SECTOR_DATA_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_TUNE_LSC_SECTOR_DATA_RO_MASK 0x00000000U
#define FX1_ISP_TUNE_LSC_SECTOR_DATA_RW_MASK 0x0000FFFFU
#define FX1_ISP_TUNE_LSC_SECTOR_DATA_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_LSC_SECTOR_DATA_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_LSC_SECTOR_DATA_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_LSC_SECTOR_DATA_W1SC_MASK 0x00000000U
/**
 * VALUE [15:0], dòng bảng 1257, kiểu truy cập RW
 * Độ dài khoảng dương theo tọa độ LSC; tổng bằng tọa độ nút cuối trừ nút đầu.
 */
#define FX1_ISP_TUNE_LSC_SECTOR_DATA_VALUE_SHIFT 0U
#define FX1_ISP_TUNE_LSC_SECTOR_DATA_VALUE_WIDTH 16U
#define FX1_ISP_TUNE_LSC_SECTOR_DATA_VALUE_MASK 0x0000FFFFU
#define FX1_ISP_TUNE_LSC_SECTOR_DATA_VALUE_RESET 0x00000000U
#define FX1_ISP_TUNE_LSC_SECTOR_DATA_VALUE_ACCESS FX1_ISP_ACCESS_RW

/**
 * COMMON_INPUT_FORMAT: địa chỉ lệch 0x8B00, dòng bảng 1258, kiểu truy cập RW
 * Độ sâu gốc trước một lần chuẩn hóa RAW12. Chỉ đổi khi ngừng đầu vào và mọi đường ra/context đã xử
 * lý hết; mã độ sâu dự phòng làm từ chối tiếp nhận. Yêu cầu legacy dừng/xử lý hết, profile từ chối
 * MODE và giữ trạng thái. Thay đổi làm mất hiệu lực LSC/EE/hình học đầu ra phụ thuộc.
 */
#define FX1_ISP_COMMON_INPUT_FORMAT_OFFSET 0x8B00U
#define REG_COMMON_INPUT_FORMAT FX1_ISP_COMMON_INPUT_FORMAT_OFFSET
#define FX1_ISP_COMMON_INPUT_FORMAT_RESET 0x00000000U
#define FX1_ISP_COMMON_INPUT_FORMAT_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_COMMON_INPUT_FORMAT_RO_MASK 0x00000000U
#define FX1_ISP_COMMON_INPUT_FORMAT_RW_MASK 0x00000003U
#define FX1_ISP_COMMON_INPUT_FORMAT_W1C_MASK 0x00000000U
#define FX1_ISP_COMMON_INPUT_FORMAT_W1S_MASK 0x00000000U
#define FX1_ISP_COMMON_INPUT_FORMAT_W1P_MASK 0x00000000U
#define FX1_ISP_COMMON_INPUT_FORMAT_W1SC_MASK 0x00000000U
/**
 * DEPTH [1:0], dòng bảng 1259, kiểu truy cập RW
 * 0=RAW12 giữ nguyên,1=RAW10 dịch trái hai bit,2/3=không hợp lệ. Cả hai chứa trong từ DDR 16 bit căn
 * phải, mở rộng 0.
 */
#define FX1_ISP_COMMON_INPUT_FORMAT_DEPTH_SHIFT 0U
#define FX1_ISP_COMMON_INPUT_FORMAT_DEPTH_WIDTH 2U
#define FX1_ISP_COMMON_INPUT_FORMAT_DEPTH_MASK 0x00000003U
#define FX1_ISP_COMMON_INPUT_FORMAT_DEPTH_RESET 0x00000000U
#define FX1_ISP_COMMON_INPUT_FORMAT_DEPTH_ACCESS FX1_ISP_ACCESS_RW

/**
 * TUNE_AEC_SOURCE: địa chỉ lệch 0x8B08, dòng bảng 1260, kiểu truy cập RW
 * Selector điểm đo AE độc lập trọng số. Profile mode đọc/ghi bank chọn có thể sửa; legacy tham gia
 * AEC COMMIT. Chốt tại đầu vào và có hiệu lực ở SOF tap đã chọn.
 */
#define FX1_ISP_TUNE_AEC_SOURCE_OFFSET 0x8B08U
#define REG_TUNE_AEC_SOURCE FX1_ISP_TUNE_AEC_SOURCE_OFFSET
#define FX1_ISP_TUNE_AEC_SOURCE_RESET 0x00000000U
#define FX1_ISP_TUNE_AEC_SOURCE_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_TUNE_AEC_SOURCE_RO_MASK 0x00000000U
#define FX1_ISP_TUNE_AEC_SOURCE_RW_MASK 0x00000001U
#define FX1_ISP_TUNE_AEC_SOURCE_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_AEC_SOURCE_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_AEC_SOURCE_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_AEC_SOURCE_W1SC_MASK 0x00000000U
/**
 * SOURCE [0:0], dòng bảng 1261, kiểu truy cập RW
 * 0=BPC mặc định,1=DG sau WB; cả hai đo Bayer RAW12.
 */
#define FX1_ISP_TUNE_AEC_SOURCE_SOURCE_SHIFT 0U
#define FX1_ISP_TUNE_AEC_SOURCE_SOURCE_WIDTH 1U
#define FX1_ISP_TUNE_AEC_SOURCE_SOURCE_MASK 0x00000001U
#define FX1_ISP_TUNE_AEC_SOURCE_SOURCE_RESET 0x00000000U
#define FX1_ISP_TUNE_AEC_SOURCE_SOURCE_ACCESS FX1_ISP_ACCESS_RW

/**
 * TUNE_AEC_RESULT_META: địa chỉ lệch 0x8B0C, dòng bảng 1262, kiểu truy cập RO
 * Thông tin bất biến của kết quả AE đang giữ quyền, không phải cấu hình live. Chưa có quyền đọc 0 và
 * đặt NO_LEASE.
 */
#define FX1_ISP_TUNE_AEC_RESULT_META_OFFSET 0x8B0CU
#define REG_TUNE_AEC_RESULT_META FX1_ISP_TUNE_AEC_RESULT_META_OFFSET
#define FX1_ISP_TUNE_AEC_RESULT_META_RESET 0x00000000U
#define FX1_ISP_TUNE_AEC_RESULT_META_ACCESS FX1_ISP_ACCESS_RO
#define FX1_ISP_TUNE_AEC_RESULT_META_RO_MASK 0x800003F1U
#define FX1_ISP_TUNE_AEC_RESULT_META_RW_MASK 0x00000000U
#define FX1_ISP_TUNE_AEC_RESULT_META_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_AEC_RESULT_META_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_AEC_RESULT_META_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_AEC_RESULT_META_W1SC_MASK 0x00000000U
/**
 * SOURCE [0:0], dòng bảng 1263, kiểu truy cập RO
 * Nguồn đo sao từ frame: 0=BPC,1=DG.
 */
#define FX1_ISP_TUNE_AEC_RESULT_META_SOURCE_SHIFT 0U
#define FX1_ISP_TUNE_AEC_RESULT_META_SOURCE_WIDTH 1U
#define FX1_ISP_TUNE_AEC_RESULT_META_SOURCE_MASK 0x00000001U
#define FX1_ISP_TUNE_AEC_RESULT_META_SOURCE_RESET 0x00000000U
#define FX1_ISP_TUNE_AEC_RESULT_META_SOURCE_ACCESS FX1_ISP_ACCESS_RO
/**
 * DOMAIN [7:4], dòng bảng 1264, kiểu truy cập RO
 * Miền đo 1=Bayer RAW12 không dấu đã chuẩn hóa khi giữ quyền,0=khi chưa giữ; mã khác dự phòng.
 */
#define FX1_ISP_TUNE_AEC_RESULT_META_DOMAIN_SHIFT 4U
#define FX1_ISP_TUNE_AEC_RESULT_META_DOMAIN_WIDTH 4U
#define FX1_ISP_TUNE_AEC_RESULT_META_DOMAIN_MASK 0x000000F0U
#define FX1_ISP_TUNE_AEC_RESULT_META_DOMAIN_RESET 0x00000000U
#define FX1_ISP_TUNE_AEC_RESULT_META_DOMAIN_ACCESS FX1_ISP_ACCESS_RO
/**
 * NATIVE_DEPTH [9:8], dòng bảng 1265, kiểu truy cập RO
 * Định dạng gốc frame đã đo: 0=RAW12,1=RAW10.
 */
#define FX1_ISP_TUNE_AEC_RESULT_META_NATIVE_DEPTH_SHIFT 8U
#define FX1_ISP_TUNE_AEC_RESULT_META_NATIVE_DEPTH_WIDTH 2U
#define FX1_ISP_TUNE_AEC_RESULT_META_NATIVE_DEPTH_MASK 0x00000300U
#define FX1_ISP_TUNE_AEC_RESULT_META_NATIVE_DEPTH_RESET 0x00000000U
#define FX1_ISP_TUNE_AEC_RESULT_META_NATIVE_DEPTH_ACCESS FX1_ISP_ACCESS_RO
/**
 * LEASE_VALID [31:31], dòng bảng 1266, kiểu truy cập RO
 * Bit 1 khi thông tin thuộc kết quả AE đã được cấp quyền đọc.
 */
#define FX1_ISP_TUNE_AEC_RESULT_META_LEASE_VALID_SHIFT 31U
#define FX1_ISP_TUNE_AEC_RESULT_META_LEASE_VALID_WIDTH 1U
#define FX1_ISP_TUNE_AEC_RESULT_META_LEASE_VALID_MASK 0x80000000U
#define FX1_ISP_TUNE_AEC_RESULT_META_LEASE_VALID_RESET 0x00000000U
#define FX1_ISP_TUNE_AEC_RESULT_META_LEASE_VALID_ACCESS FX1_ISP_ACCESS_RO

/**
 * CH0_CROP_CTRL: địa chỉ lệch 0x8B40, dòng bảng 1267, kiểu truy cập RW
 * Crop YUV sau IQ dùng chung và trước resizer riêng của kênh. Thay đổi yêu cầu dừng/xử lý hết; ghi
 * hình học được nhận làm mất tính hợp lệ đến khi kiểm tra lại.
 */
#define FX1_ISP_CH0_CROP_CTRL_OFFSET 0x8B40U
#define REG_CH0_CROP_CTRL FX1_ISP_CH0_CROP_CTRL_OFFSET
#define FX1_ISP_CH0_CROP_CTRL_RESET 0x00000000U
#define FX1_ISP_CH0_CROP_CTRL_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_CH0_CROP_CTRL_RO_MASK 0x00000000U
#define FX1_ISP_CH0_CROP_CTRL_RW_MASK 0x00000001U
#define FX1_ISP_CH0_CROP_CTRL_W1C_MASK 0x00000000U
#define FX1_ISP_CH0_CROP_CTRL_W1S_MASK 0x00000000U
#define FX1_ISP_CH0_CROP_CTRL_W1P_MASK 0x00000000U
#define FX1_ISP_CH0_CROP_CTRL_W1SC_MASK 0x00000000U
/**
 * ENABLE [0:0], dòng bảng 1268, kiểu truy cập RW
 * 0=toàn ảnh hữu dụng,1=hình crop chẵn tường minh.
 */
#define FX1_ISP_CH0_CROP_CTRL_ENABLE_SHIFT 0U
#define FX1_ISP_CH0_CROP_CTRL_ENABLE_WIDTH 1U
#define FX1_ISP_CH0_CROP_CTRL_ENABLE_MASK 0x00000001U
#define FX1_ISP_CH0_CROP_CTRL_ENABLE_RESET 0x00000000U
#define FX1_ISP_CH0_CROP_CTRL_ENABLE_ACCESS FX1_ISP_ACCESS_RW

/**
 * CH0_CROP_ORIGIN: địa chỉ lệch 0x8B44, dòng bảng 1269, kiểu truy cập RW
 * Crop theo pixel ảnh hữu dụng: gốc/kích thước chẵn, không rỗng, nằm hoàn toàn trong ảnh; bật với
 * kích thước 0 là sai.
 */
#define FX1_ISP_CH0_CROP_ORIGIN_OFFSET 0x8B44U
#define REG_CH0_CROP_ORIGIN FX1_ISP_CH0_CROP_ORIGIN_OFFSET
#define FX1_ISP_CH0_CROP_ORIGIN_RESET 0x00000000U
#define FX1_ISP_CH0_CROP_ORIGIN_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_CH0_CROP_ORIGIN_RO_MASK 0x00000000U
#define FX1_ISP_CH0_CROP_ORIGIN_RW_MASK 0x1FFF1FFFU
#define FX1_ISP_CH0_CROP_ORIGIN_W1C_MASK 0x00000000U
#define FX1_ISP_CH0_CROP_ORIGIN_W1S_MASK 0x00000000U
#define FX1_ISP_CH0_CROP_ORIGIN_W1P_MASK 0x00000000U
#define FX1_ISP_CH0_CROP_ORIGIN_W1SC_MASK 0x00000000U
/**
 * X [12:0], dòng bảng 1270, kiểu truy cập RW
 * Tọa độ hoặc kích thước ngang.
 */
#define FX1_ISP_CH0_CROP_ORIGIN_X_SHIFT 0U
#define FX1_ISP_CH0_CROP_ORIGIN_X_WIDTH 13U
#define FX1_ISP_CH0_CROP_ORIGIN_X_MASK 0x00001FFFU
#define FX1_ISP_CH0_CROP_ORIGIN_X_RESET 0x00000000U
#define FX1_ISP_CH0_CROP_ORIGIN_X_ACCESS FX1_ISP_ACCESS_RW
/**
 * Y [28:16], dòng bảng 1271, kiểu truy cập RW
 * Tọa độ hoặc kích thước dọc.
 */
#define FX1_ISP_CH0_CROP_ORIGIN_Y_SHIFT 16U
#define FX1_ISP_CH0_CROP_ORIGIN_Y_WIDTH 13U
#define FX1_ISP_CH0_CROP_ORIGIN_Y_MASK 0x1FFF0000U
#define FX1_ISP_CH0_CROP_ORIGIN_Y_RESET 0x00000000U
#define FX1_ISP_CH0_CROP_ORIGIN_Y_ACCESS FX1_ISP_ACCESS_RW

/**
 * CH0_CROP_SIZE: địa chỉ lệch 0x8B48, dòng bảng 1272, kiểu truy cập RW
 * Crop theo pixel ảnh hữu dụng: gốc/kích thước chẵn, không rỗng, nằm hoàn toàn trong ảnh; bật với
 * kích thước 0 là sai.
 */
#define FX1_ISP_CH0_CROP_SIZE_OFFSET 0x8B48U
#define REG_CH0_CROP_SIZE FX1_ISP_CH0_CROP_SIZE_OFFSET
#define FX1_ISP_CH0_CROP_SIZE_RESET 0x00000000U
#define FX1_ISP_CH0_CROP_SIZE_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_CH0_CROP_SIZE_RO_MASK 0x00000000U
#define FX1_ISP_CH0_CROP_SIZE_RW_MASK 0x1FFF1FFFU
#define FX1_ISP_CH0_CROP_SIZE_W1C_MASK 0x00000000U
#define FX1_ISP_CH0_CROP_SIZE_W1S_MASK 0x00000000U
#define FX1_ISP_CH0_CROP_SIZE_W1P_MASK 0x00000000U
#define FX1_ISP_CH0_CROP_SIZE_W1SC_MASK 0x00000000U
/**
 * W [12:0], dòng bảng 1273, kiểu truy cập RW
 * Tọa độ hoặc kích thước ngang.
 */
#define FX1_ISP_CH0_CROP_SIZE_W_SHIFT 0U
#define FX1_ISP_CH0_CROP_SIZE_W_WIDTH 13U
#define FX1_ISP_CH0_CROP_SIZE_W_MASK 0x00001FFFU
#define FX1_ISP_CH0_CROP_SIZE_W_RESET 0x00000000U
#define FX1_ISP_CH0_CROP_SIZE_W_ACCESS FX1_ISP_ACCESS_RW
/**
 * H [28:16], dòng bảng 1274, kiểu truy cập RW
 * Tọa độ hoặc kích thước dọc.
 */
#define FX1_ISP_CH0_CROP_SIZE_H_SHIFT 16U
#define FX1_ISP_CH0_CROP_SIZE_H_WIDTH 13U
#define FX1_ISP_CH0_CROP_SIZE_H_MASK 0x1FFF0000U
#define FX1_ISP_CH0_CROP_SIZE_H_RESET 0x00000000U
#define FX1_ISP_CH0_CROP_SIZE_H_ACCESS FX1_ISP_ACCESS_RW

/**
 * CH0_GEOMETRY_STATUS: địa chỉ lệch 0x8B4C, dòng bảng 1275, kiểu truy cập RO
 * Trạng thái kiểm tra hình học hiện tại. Giá trị đọc cũ không cấp phép tiếp nhận khi VALID=0. ERROR
 * là mức trực tiếp, xóa khi có cấu hình hợp lệ ổn định; cấu hình sai còn đặt TUNE_ERROR.INVALID đã
 * chốt.
 */
#define FX1_ISP_CH0_GEOMETRY_STATUS_OFFSET 0x8B4CU
#define REG_CH0_GEOMETRY_STATUS FX1_ISP_CH0_GEOMETRY_STATUS_OFFSET
#define FX1_ISP_CH0_GEOMETRY_STATUS_RESET 0x00000000U
#define FX1_ISP_CH0_GEOMETRY_STATUS_ACCESS FX1_ISP_ACCESS_RO
#define FX1_ISP_CH0_GEOMETRY_STATUS_RO_MASK 0x000000F3U
#define FX1_ISP_CH0_GEOMETRY_STATUS_RW_MASK 0x00000000U
#define FX1_ISP_CH0_GEOMETRY_STATUS_W1C_MASK 0x00000000U
#define FX1_ISP_CH0_GEOMETRY_STATUS_W1S_MASK 0x00000000U
#define FX1_ISP_CH0_GEOMETRY_STATUS_W1P_MASK 0x00000000U
#define FX1_ISP_CH0_GEOMETRY_STATUS_W1SC_MASK 0x00000000U
/**
 * VALID [0:0], dòng bảng 1276, kiểu truy cập RO
 * VALID=1 chỉ khi toàn bộ hình học ổn định/hợp lệ. Reset hoặc ghi hình học liên quan xóa bit.
 */
#define FX1_ISP_CH0_GEOMETRY_STATUS_VALID_SHIFT 0U
#define FX1_ISP_CH0_GEOMETRY_STATUS_VALID_WIDTH 1U
#define FX1_ISP_CH0_GEOMETRY_STATUS_VALID_MASK 0x00000001U
#define FX1_ISP_CH0_GEOMETRY_STATUS_VALID_RESET 0x00000000U
#define FX1_ISP_CH0_GEOMETRY_STATUS_VALID_ACCESS FX1_ISP_ACCESS_RO
/**
 * BUSY [1:1], dòng bảng 1277, kiểu truy cập RO
 * Đang chờ tính toán hoặc kiểm tra hình học.
 */
#define FX1_ISP_CH0_GEOMETRY_STATUS_BUSY_SHIFT 1U
#define FX1_ISP_CH0_GEOMETRY_STATUS_BUSY_WIDTH 1U
#define FX1_ISP_CH0_GEOMETRY_STATUS_BUSY_MASK 0x00000002U
#define FX1_ISP_CH0_GEOMETRY_STATUS_BUSY_RESET 0x00000000U
#define FX1_ISP_CH0_GEOMETRY_STATUS_BUSY_ACCESS FX1_ISP_ACCESS_RO
/**
 * ERROR [7:4], dòng bảng 1278, kiểu truy cập RO
 * Mã ưu tiên: 0=không lỗi,1=định dạng/kích thước hữu dụng,2=crop rỗng/ngoài biên,3=crop lẻ,4=mã
 * resize dự phòng,5=đích vượt giới hạn kênh,6=đích vượt crop. Chọn mã khác 0 nhỏ nhất phù hợp. BUSY
 * kéo VALID=0 và ERROR=0.
 */
#define FX1_ISP_CH0_GEOMETRY_STATUS_ERROR_SHIFT 4U
#define FX1_ISP_CH0_GEOMETRY_STATUS_ERROR_WIDTH 4U
#define FX1_ISP_CH0_GEOMETRY_STATUS_ERROR_MASK 0x000000F0U
#define FX1_ISP_CH0_GEOMETRY_STATUS_ERROR_RESET 0x00000000U
#define FX1_ISP_CH0_GEOMETRY_STATUS_ERROR_ACCESS FX1_ISP_ACCESS_RO

/**
 * CH0_ACTIVE_CROP_ORIGIN: địa chỉ lệch 0x8B50, dòng bảng 1279, kiểu truy cập RO
 * Crop hợp lệ công bố nguyên tử cùng OUT_W/H và VALID. Crop tắt nghĩa là gốc 0 và toàn ảnh hữu dụng.
 * Trước lần hợp lệ đầu đọc 0; khi bận/sai giữ bản cũ nên phải xem VALID. Frame đang chạy giữ bản đã
 * chốt riêng.
 */
#define FX1_ISP_CH0_ACTIVE_CROP_ORIGIN_OFFSET 0x8B50U
#define REG_CH0_ACTIVE_CROP_ORIGIN FX1_ISP_CH0_ACTIVE_CROP_ORIGIN_OFFSET
#define FX1_ISP_CH0_ACTIVE_CROP_ORIGIN_RESET 0x00000000U
#define FX1_ISP_CH0_ACTIVE_CROP_ORIGIN_ACCESS FX1_ISP_ACCESS_RO
#define FX1_ISP_CH0_ACTIVE_CROP_ORIGIN_RO_MASK 0x1FFF1FFFU
#define FX1_ISP_CH0_ACTIVE_CROP_ORIGIN_RW_MASK 0x00000000U
#define FX1_ISP_CH0_ACTIVE_CROP_ORIGIN_W1C_MASK 0x00000000U
#define FX1_ISP_CH0_ACTIVE_CROP_ORIGIN_W1S_MASK 0x00000000U
#define FX1_ISP_CH0_ACTIVE_CROP_ORIGIN_W1P_MASK 0x00000000U
#define FX1_ISP_CH0_ACTIVE_CROP_ORIGIN_W1SC_MASK 0x00000000U
/**
 * X [12:0], dòng bảng 1280, kiểu truy cập RO
 * Tọa độ hoặc kích thước ngang đã kiểm tra.
 */
#define FX1_ISP_CH0_ACTIVE_CROP_ORIGIN_X_SHIFT 0U
#define FX1_ISP_CH0_ACTIVE_CROP_ORIGIN_X_WIDTH 13U
#define FX1_ISP_CH0_ACTIVE_CROP_ORIGIN_X_MASK 0x00001FFFU
#define FX1_ISP_CH0_ACTIVE_CROP_ORIGIN_X_RESET 0x00000000U
#define FX1_ISP_CH0_ACTIVE_CROP_ORIGIN_X_ACCESS FX1_ISP_ACCESS_RO
/**
 * Y [28:16], dòng bảng 1281, kiểu truy cập RO
 * Tọa độ hoặc kích thước dọc đã kiểm tra.
 */
#define FX1_ISP_CH0_ACTIVE_CROP_ORIGIN_Y_SHIFT 16U
#define FX1_ISP_CH0_ACTIVE_CROP_ORIGIN_Y_WIDTH 13U
#define FX1_ISP_CH0_ACTIVE_CROP_ORIGIN_Y_MASK 0x1FFF0000U
#define FX1_ISP_CH0_ACTIVE_CROP_ORIGIN_Y_RESET 0x00000000U
#define FX1_ISP_CH0_ACTIVE_CROP_ORIGIN_Y_ACCESS FX1_ISP_ACCESS_RO

/**
 * CH0_ACTIVE_CROP_SIZE: địa chỉ lệch 0x8B54, dòng bảng 1282, kiểu truy cập RO
 * Crop hợp lệ công bố nguyên tử cùng OUT_W/H và VALID. Crop tắt nghĩa là gốc 0 và toàn ảnh hữu dụng.
 * Trước lần hợp lệ đầu đọc 0; khi bận/sai giữ bản cũ nên phải xem VALID. Frame đang chạy giữ bản đã
 * chốt riêng.
 */
#define FX1_ISP_CH0_ACTIVE_CROP_SIZE_OFFSET 0x8B54U
#define REG_CH0_ACTIVE_CROP_SIZE FX1_ISP_CH0_ACTIVE_CROP_SIZE_OFFSET
#define FX1_ISP_CH0_ACTIVE_CROP_SIZE_RESET 0x00000000U
#define FX1_ISP_CH0_ACTIVE_CROP_SIZE_ACCESS FX1_ISP_ACCESS_RO
#define FX1_ISP_CH0_ACTIVE_CROP_SIZE_RO_MASK 0x1FFF1FFFU
#define FX1_ISP_CH0_ACTIVE_CROP_SIZE_RW_MASK 0x00000000U
#define FX1_ISP_CH0_ACTIVE_CROP_SIZE_W1C_MASK 0x00000000U
#define FX1_ISP_CH0_ACTIVE_CROP_SIZE_W1S_MASK 0x00000000U
#define FX1_ISP_CH0_ACTIVE_CROP_SIZE_W1P_MASK 0x00000000U
#define FX1_ISP_CH0_ACTIVE_CROP_SIZE_W1SC_MASK 0x00000000U
/**
 * W [12:0], dòng bảng 1283, kiểu truy cập RO
 * Tọa độ hoặc kích thước ngang đã kiểm tra.
 */
#define FX1_ISP_CH0_ACTIVE_CROP_SIZE_W_SHIFT 0U
#define FX1_ISP_CH0_ACTIVE_CROP_SIZE_W_WIDTH 13U
#define FX1_ISP_CH0_ACTIVE_CROP_SIZE_W_MASK 0x00001FFFU
#define FX1_ISP_CH0_ACTIVE_CROP_SIZE_W_RESET 0x00000000U
#define FX1_ISP_CH0_ACTIVE_CROP_SIZE_W_ACCESS FX1_ISP_ACCESS_RO
/**
 * H [28:16], dòng bảng 1284, kiểu truy cập RO
 * Tọa độ hoặc kích thước dọc đã kiểm tra.
 */
#define FX1_ISP_CH0_ACTIVE_CROP_SIZE_H_SHIFT 16U
#define FX1_ISP_CH0_ACTIVE_CROP_SIZE_H_WIDTH 13U
#define FX1_ISP_CH0_ACTIVE_CROP_SIZE_H_MASK 0x1FFF0000U
#define FX1_ISP_CH0_ACTIVE_CROP_SIZE_H_RESET 0x00000000U
#define FX1_ISP_CH0_ACTIVE_CROP_SIZE_H_ACCESS FX1_ISP_ACCESS_RO

/**
 * CH0_OUTPUT_LIMIT: địa chỉ lệch 0x8B58, dòng bảng 1285, kiểu truy cập RO
 * Giới hạn đầu ra cố định của kênh, không cho phép phóng to hoặc kích thước tùy ý.
 */
#define FX1_ISP_CH0_OUTPUT_LIMIT_OFFSET 0x8B58U
#define REG_CH0_OUTPUT_LIMIT FX1_ISP_CH0_OUTPUT_LIMIT_OFFSET
#define FX1_ISP_CH0_OUTPUT_LIMIT_RESET 0x08700F00U
#define FX1_ISP_CH0_OUTPUT_LIMIT_ACCESS FX1_ISP_ACCESS_RO
#define FX1_ISP_CH0_OUTPUT_LIMIT_RO_MASK 0x1FFF1FFFU
#define FX1_ISP_CH0_OUTPUT_LIMIT_RW_MASK 0x00000000U
#define FX1_ISP_CH0_OUTPUT_LIMIT_W1C_MASK 0x00000000U
#define FX1_ISP_CH0_OUTPUT_LIMIT_W1S_MASK 0x00000000U
#define FX1_ISP_CH0_OUTPUT_LIMIT_W1P_MASK 0x00000000U
#define FX1_ISP_CH0_OUTPUT_LIMIT_W1SC_MASK 0x00000000U
/**
 * MAX_W [12:0], dòng bảng 1286, kiểu truy cập RO
 * Chiều rộng đầu ra tối đa.
 */
#define FX1_ISP_CH0_OUTPUT_LIMIT_MAX_W_SHIFT 0U
#define FX1_ISP_CH0_OUTPUT_LIMIT_MAX_W_WIDTH 13U
#define FX1_ISP_CH0_OUTPUT_LIMIT_MAX_W_MASK 0x00001FFFU
#define FX1_ISP_CH0_OUTPUT_LIMIT_MAX_W_RESET 0x00000F00U
#define FX1_ISP_CH0_OUTPUT_LIMIT_MAX_W_ACCESS FX1_ISP_ACCESS_RO
/**
 * MAX_H [28:16], dòng bảng 1287, kiểu truy cập RO
 * Chiều cao đầu ra tối đa.
 */
#define FX1_ISP_CH0_OUTPUT_LIMIT_MAX_H_SHIFT 16U
#define FX1_ISP_CH0_OUTPUT_LIMIT_MAX_H_WIDTH 13U
#define FX1_ISP_CH0_OUTPUT_LIMIT_MAX_H_MASK 0x1FFF0000U
#define FX1_ISP_CH0_OUTPUT_LIMIT_MAX_H_RESET 0x00000870U
#define FX1_ISP_CH0_OUTPUT_LIMIT_MAX_H_ACCESS FX1_ISP_ACCESS_RO

/**
 * CH1_CROP_CTRL: địa chỉ lệch 0x8B80, dòng bảng 1288, kiểu truy cập RW
 * Crop YUV sau IQ dùng chung và trước resizer riêng của kênh. Thay đổi yêu cầu dừng/xử lý hết; ghi
 * hình học được nhận làm mất tính hợp lệ đến khi kiểm tra lại.
 */
#define FX1_ISP_CH1_CROP_CTRL_OFFSET 0x8B80U
#define REG_CH1_CROP_CTRL FX1_ISP_CH1_CROP_CTRL_OFFSET
#define FX1_ISP_CH1_CROP_CTRL_RESET 0x00000000U
#define FX1_ISP_CH1_CROP_CTRL_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_CH1_CROP_CTRL_RO_MASK 0x00000000U
#define FX1_ISP_CH1_CROP_CTRL_RW_MASK 0x00000001U
#define FX1_ISP_CH1_CROP_CTRL_W1C_MASK 0x00000000U
#define FX1_ISP_CH1_CROP_CTRL_W1S_MASK 0x00000000U
#define FX1_ISP_CH1_CROP_CTRL_W1P_MASK 0x00000000U
#define FX1_ISP_CH1_CROP_CTRL_W1SC_MASK 0x00000000U
/**
 * ENABLE [0:0], dòng bảng 1289, kiểu truy cập RW
 * 0=toàn ảnh hữu dụng,1=hình crop chẵn tường minh.
 */
#define FX1_ISP_CH1_CROP_CTRL_ENABLE_SHIFT 0U
#define FX1_ISP_CH1_CROP_CTRL_ENABLE_WIDTH 1U
#define FX1_ISP_CH1_CROP_CTRL_ENABLE_MASK 0x00000001U
#define FX1_ISP_CH1_CROP_CTRL_ENABLE_RESET 0x00000000U
#define FX1_ISP_CH1_CROP_CTRL_ENABLE_ACCESS FX1_ISP_ACCESS_RW

/**
 * CH1_CROP_ORIGIN: địa chỉ lệch 0x8B84, dòng bảng 1290, kiểu truy cập RW
 * Crop theo pixel ảnh hữu dụng: gốc/kích thước chẵn, không rỗng, nằm hoàn toàn trong ảnh; bật với
 * kích thước 0 là sai.
 */
#define FX1_ISP_CH1_CROP_ORIGIN_OFFSET 0x8B84U
#define REG_CH1_CROP_ORIGIN FX1_ISP_CH1_CROP_ORIGIN_OFFSET
#define FX1_ISP_CH1_CROP_ORIGIN_RESET 0x00000000U
#define FX1_ISP_CH1_CROP_ORIGIN_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_CH1_CROP_ORIGIN_RO_MASK 0x00000000U
#define FX1_ISP_CH1_CROP_ORIGIN_RW_MASK 0x1FFF1FFFU
#define FX1_ISP_CH1_CROP_ORIGIN_W1C_MASK 0x00000000U
#define FX1_ISP_CH1_CROP_ORIGIN_W1S_MASK 0x00000000U
#define FX1_ISP_CH1_CROP_ORIGIN_W1P_MASK 0x00000000U
#define FX1_ISP_CH1_CROP_ORIGIN_W1SC_MASK 0x00000000U
/**
 * X [12:0], dòng bảng 1291, kiểu truy cập RW
 * Tọa độ hoặc kích thước ngang.
 */
#define FX1_ISP_CH1_CROP_ORIGIN_X_SHIFT 0U
#define FX1_ISP_CH1_CROP_ORIGIN_X_WIDTH 13U
#define FX1_ISP_CH1_CROP_ORIGIN_X_MASK 0x00001FFFU
#define FX1_ISP_CH1_CROP_ORIGIN_X_RESET 0x00000000U
#define FX1_ISP_CH1_CROP_ORIGIN_X_ACCESS FX1_ISP_ACCESS_RW
/**
 * Y [28:16], dòng bảng 1292, kiểu truy cập RW
 * Tọa độ hoặc kích thước dọc.
 */
#define FX1_ISP_CH1_CROP_ORIGIN_Y_SHIFT 16U
#define FX1_ISP_CH1_CROP_ORIGIN_Y_WIDTH 13U
#define FX1_ISP_CH1_CROP_ORIGIN_Y_MASK 0x1FFF0000U
#define FX1_ISP_CH1_CROP_ORIGIN_Y_RESET 0x00000000U
#define FX1_ISP_CH1_CROP_ORIGIN_Y_ACCESS FX1_ISP_ACCESS_RW

/**
 * CH1_CROP_SIZE: địa chỉ lệch 0x8B88, dòng bảng 1293, kiểu truy cập RW
 * Crop theo pixel ảnh hữu dụng: gốc/kích thước chẵn, không rỗng, nằm hoàn toàn trong ảnh; bật với
 * kích thước 0 là sai.
 */
#define FX1_ISP_CH1_CROP_SIZE_OFFSET 0x8B88U
#define REG_CH1_CROP_SIZE FX1_ISP_CH1_CROP_SIZE_OFFSET
#define FX1_ISP_CH1_CROP_SIZE_RESET 0x00000000U
#define FX1_ISP_CH1_CROP_SIZE_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_CH1_CROP_SIZE_RO_MASK 0x00000000U
#define FX1_ISP_CH1_CROP_SIZE_RW_MASK 0x1FFF1FFFU
#define FX1_ISP_CH1_CROP_SIZE_W1C_MASK 0x00000000U
#define FX1_ISP_CH1_CROP_SIZE_W1S_MASK 0x00000000U
#define FX1_ISP_CH1_CROP_SIZE_W1P_MASK 0x00000000U
#define FX1_ISP_CH1_CROP_SIZE_W1SC_MASK 0x00000000U
/**
 * W [12:0], dòng bảng 1294, kiểu truy cập RW
 * Tọa độ hoặc kích thước ngang.
 */
#define FX1_ISP_CH1_CROP_SIZE_W_SHIFT 0U
#define FX1_ISP_CH1_CROP_SIZE_W_WIDTH 13U
#define FX1_ISP_CH1_CROP_SIZE_W_MASK 0x00001FFFU
#define FX1_ISP_CH1_CROP_SIZE_W_RESET 0x00000000U
#define FX1_ISP_CH1_CROP_SIZE_W_ACCESS FX1_ISP_ACCESS_RW
/**
 * H [28:16], dòng bảng 1295, kiểu truy cập RW
 * Tọa độ hoặc kích thước dọc.
 */
#define FX1_ISP_CH1_CROP_SIZE_H_SHIFT 16U
#define FX1_ISP_CH1_CROP_SIZE_H_WIDTH 13U
#define FX1_ISP_CH1_CROP_SIZE_H_MASK 0x1FFF0000U
#define FX1_ISP_CH1_CROP_SIZE_H_RESET 0x00000000U
#define FX1_ISP_CH1_CROP_SIZE_H_ACCESS FX1_ISP_ACCESS_RW

/**
 * CH1_GEOMETRY_STATUS: địa chỉ lệch 0x8B8C, dòng bảng 1296, kiểu truy cập RO
 * Trạng thái kiểm tra hình học hiện tại. Giá trị đọc cũ không cấp phép tiếp nhận khi VALID=0. ERROR
 * là mức trực tiếp, xóa khi có cấu hình hợp lệ ổn định; cấu hình sai còn đặt TUNE_ERROR.INVALID đã
 * chốt.
 */
#define FX1_ISP_CH1_GEOMETRY_STATUS_OFFSET 0x8B8CU
#define REG_CH1_GEOMETRY_STATUS FX1_ISP_CH1_GEOMETRY_STATUS_OFFSET
#define FX1_ISP_CH1_GEOMETRY_STATUS_RESET 0x00000000U
#define FX1_ISP_CH1_GEOMETRY_STATUS_ACCESS FX1_ISP_ACCESS_RO
#define FX1_ISP_CH1_GEOMETRY_STATUS_RO_MASK 0x000000F3U
#define FX1_ISP_CH1_GEOMETRY_STATUS_RW_MASK 0x00000000U
#define FX1_ISP_CH1_GEOMETRY_STATUS_W1C_MASK 0x00000000U
#define FX1_ISP_CH1_GEOMETRY_STATUS_W1S_MASK 0x00000000U
#define FX1_ISP_CH1_GEOMETRY_STATUS_W1P_MASK 0x00000000U
#define FX1_ISP_CH1_GEOMETRY_STATUS_W1SC_MASK 0x00000000U
/**
 * VALID [0:0], dòng bảng 1297, kiểu truy cập RO
 * VALID=1 chỉ khi toàn bộ hình học ổn định/hợp lệ. Reset hoặc ghi hình học liên quan xóa bit.
 */
#define FX1_ISP_CH1_GEOMETRY_STATUS_VALID_SHIFT 0U
#define FX1_ISP_CH1_GEOMETRY_STATUS_VALID_WIDTH 1U
#define FX1_ISP_CH1_GEOMETRY_STATUS_VALID_MASK 0x00000001U
#define FX1_ISP_CH1_GEOMETRY_STATUS_VALID_RESET 0x00000000U
#define FX1_ISP_CH1_GEOMETRY_STATUS_VALID_ACCESS FX1_ISP_ACCESS_RO
/**
 * BUSY [1:1], dòng bảng 1298, kiểu truy cập RO
 * Đang chờ tính toán hoặc kiểm tra hình học.
 */
#define FX1_ISP_CH1_GEOMETRY_STATUS_BUSY_SHIFT 1U
#define FX1_ISP_CH1_GEOMETRY_STATUS_BUSY_WIDTH 1U
#define FX1_ISP_CH1_GEOMETRY_STATUS_BUSY_MASK 0x00000002U
#define FX1_ISP_CH1_GEOMETRY_STATUS_BUSY_RESET 0x00000000U
#define FX1_ISP_CH1_GEOMETRY_STATUS_BUSY_ACCESS FX1_ISP_ACCESS_RO
/**
 * ERROR [7:4], dòng bảng 1299, kiểu truy cập RO
 * Mã ưu tiên: 0=không lỗi,1=định dạng/kích thước hữu dụng,2=crop rỗng/ngoài biên,3=crop lẻ,4=mã
 * resize dự phòng,5=đích vượt giới hạn kênh,6=đích vượt crop. Chọn mã khác 0 nhỏ nhất phù hợp. BUSY
 * kéo VALID=0 và ERROR=0.
 */
#define FX1_ISP_CH1_GEOMETRY_STATUS_ERROR_SHIFT 4U
#define FX1_ISP_CH1_GEOMETRY_STATUS_ERROR_WIDTH 4U
#define FX1_ISP_CH1_GEOMETRY_STATUS_ERROR_MASK 0x000000F0U
#define FX1_ISP_CH1_GEOMETRY_STATUS_ERROR_RESET 0x00000000U
#define FX1_ISP_CH1_GEOMETRY_STATUS_ERROR_ACCESS FX1_ISP_ACCESS_RO

/**
 * CH1_ACTIVE_CROP_ORIGIN: địa chỉ lệch 0x8B90, dòng bảng 1300, kiểu truy cập RO
 * Crop hợp lệ công bố nguyên tử cùng OUT_W/H và VALID. Crop tắt nghĩa là gốc 0 và toàn ảnh hữu dụng.
 * Trước lần hợp lệ đầu đọc 0; khi bận/sai giữ bản cũ nên phải xem VALID. Frame đang chạy giữ bản đã
 * chốt riêng.
 */
#define FX1_ISP_CH1_ACTIVE_CROP_ORIGIN_OFFSET 0x8B90U
#define REG_CH1_ACTIVE_CROP_ORIGIN FX1_ISP_CH1_ACTIVE_CROP_ORIGIN_OFFSET
#define FX1_ISP_CH1_ACTIVE_CROP_ORIGIN_RESET 0x00000000U
#define FX1_ISP_CH1_ACTIVE_CROP_ORIGIN_ACCESS FX1_ISP_ACCESS_RO
#define FX1_ISP_CH1_ACTIVE_CROP_ORIGIN_RO_MASK 0x1FFF1FFFU
#define FX1_ISP_CH1_ACTIVE_CROP_ORIGIN_RW_MASK 0x00000000U
#define FX1_ISP_CH1_ACTIVE_CROP_ORIGIN_W1C_MASK 0x00000000U
#define FX1_ISP_CH1_ACTIVE_CROP_ORIGIN_W1S_MASK 0x00000000U
#define FX1_ISP_CH1_ACTIVE_CROP_ORIGIN_W1P_MASK 0x00000000U
#define FX1_ISP_CH1_ACTIVE_CROP_ORIGIN_W1SC_MASK 0x00000000U
/**
 * X [12:0], dòng bảng 1301, kiểu truy cập RO
 * Tọa độ hoặc kích thước ngang đã kiểm tra.
 */
#define FX1_ISP_CH1_ACTIVE_CROP_ORIGIN_X_SHIFT 0U
#define FX1_ISP_CH1_ACTIVE_CROP_ORIGIN_X_WIDTH 13U
#define FX1_ISP_CH1_ACTIVE_CROP_ORIGIN_X_MASK 0x00001FFFU
#define FX1_ISP_CH1_ACTIVE_CROP_ORIGIN_X_RESET 0x00000000U
#define FX1_ISP_CH1_ACTIVE_CROP_ORIGIN_X_ACCESS FX1_ISP_ACCESS_RO
/**
 * Y [28:16], dòng bảng 1302, kiểu truy cập RO
 * Tọa độ hoặc kích thước dọc đã kiểm tra.
 */
#define FX1_ISP_CH1_ACTIVE_CROP_ORIGIN_Y_SHIFT 16U
#define FX1_ISP_CH1_ACTIVE_CROP_ORIGIN_Y_WIDTH 13U
#define FX1_ISP_CH1_ACTIVE_CROP_ORIGIN_Y_MASK 0x1FFF0000U
#define FX1_ISP_CH1_ACTIVE_CROP_ORIGIN_Y_RESET 0x00000000U
#define FX1_ISP_CH1_ACTIVE_CROP_ORIGIN_Y_ACCESS FX1_ISP_ACCESS_RO

/**
 * CH1_ACTIVE_CROP_SIZE: địa chỉ lệch 0x8B94, dòng bảng 1303, kiểu truy cập RO
 * Crop hợp lệ công bố nguyên tử cùng OUT_W/H và VALID. Crop tắt nghĩa là gốc 0 và toàn ảnh hữu dụng.
 * Trước lần hợp lệ đầu đọc 0; khi bận/sai giữ bản cũ nên phải xem VALID. Frame đang chạy giữ bản đã
 * chốt riêng.
 */
#define FX1_ISP_CH1_ACTIVE_CROP_SIZE_OFFSET 0x8B94U
#define REG_CH1_ACTIVE_CROP_SIZE FX1_ISP_CH1_ACTIVE_CROP_SIZE_OFFSET
#define FX1_ISP_CH1_ACTIVE_CROP_SIZE_RESET 0x00000000U
#define FX1_ISP_CH1_ACTIVE_CROP_SIZE_ACCESS FX1_ISP_ACCESS_RO
#define FX1_ISP_CH1_ACTIVE_CROP_SIZE_RO_MASK 0x1FFF1FFFU
#define FX1_ISP_CH1_ACTIVE_CROP_SIZE_RW_MASK 0x00000000U
#define FX1_ISP_CH1_ACTIVE_CROP_SIZE_W1C_MASK 0x00000000U
#define FX1_ISP_CH1_ACTIVE_CROP_SIZE_W1S_MASK 0x00000000U
#define FX1_ISP_CH1_ACTIVE_CROP_SIZE_W1P_MASK 0x00000000U
#define FX1_ISP_CH1_ACTIVE_CROP_SIZE_W1SC_MASK 0x00000000U
/**
 * W [12:0], dòng bảng 1304, kiểu truy cập RO
 * Tọa độ hoặc kích thước ngang đã kiểm tra.
 */
#define FX1_ISP_CH1_ACTIVE_CROP_SIZE_W_SHIFT 0U
#define FX1_ISP_CH1_ACTIVE_CROP_SIZE_W_WIDTH 13U
#define FX1_ISP_CH1_ACTIVE_CROP_SIZE_W_MASK 0x00001FFFU
#define FX1_ISP_CH1_ACTIVE_CROP_SIZE_W_RESET 0x00000000U
#define FX1_ISP_CH1_ACTIVE_CROP_SIZE_W_ACCESS FX1_ISP_ACCESS_RO
/**
 * H [28:16], dòng bảng 1305, kiểu truy cập RO
 * Tọa độ hoặc kích thước dọc đã kiểm tra.
 */
#define FX1_ISP_CH1_ACTIVE_CROP_SIZE_H_SHIFT 16U
#define FX1_ISP_CH1_ACTIVE_CROP_SIZE_H_WIDTH 13U
#define FX1_ISP_CH1_ACTIVE_CROP_SIZE_H_MASK 0x1FFF0000U
#define FX1_ISP_CH1_ACTIVE_CROP_SIZE_H_RESET 0x00000000U
#define FX1_ISP_CH1_ACTIVE_CROP_SIZE_H_ACCESS FX1_ISP_ACCESS_RO

/**
 * CH1_OUTPUT_LIMIT: địa chỉ lệch 0x8B98, dòng bảng 1306, kiểu truy cập RO
 * Giới hạn đầu ra cố định của kênh, không cho phép phóng to hoặc kích thước tùy ý.
 */
#define FX1_ISP_CH1_OUTPUT_LIMIT_OFFSET 0x8B98U
#define REG_CH1_OUTPUT_LIMIT FX1_ISP_CH1_OUTPUT_LIMIT_OFFSET
#define FX1_ISP_CH1_OUTPUT_LIMIT_RESET 0x08700F00U
#define FX1_ISP_CH1_OUTPUT_LIMIT_ACCESS FX1_ISP_ACCESS_RO
#define FX1_ISP_CH1_OUTPUT_LIMIT_RO_MASK 0x1FFF1FFFU
#define FX1_ISP_CH1_OUTPUT_LIMIT_RW_MASK 0x00000000U
#define FX1_ISP_CH1_OUTPUT_LIMIT_W1C_MASK 0x00000000U
#define FX1_ISP_CH1_OUTPUT_LIMIT_W1S_MASK 0x00000000U
#define FX1_ISP_CH1_OUTPUT_LIMIT_W1P_MASK 0x00000000U
#define FX1_ISP_CH1_OUTPUT_LIMIT_W1SC_MASK 0x00000000U
/**
 * MAX_W [12:0], dòng bảng 1307, kiểu truy cập RO
 * Chiều rộng đầu ra tối đa.
 */
#define FX1_ISP_CH1_OUTPUT_LIMIT_MAX_W_SHIFT 0U
#define FX1_ISP_CH1_OUTPUT_LIMIT_MAX_W_WIDTH 13U
#define FX1_ISP_CH1_OUTPUT_LIMIT_MAX_W_MASK 0x00001FFFU
#define FX1_ISP_CH1_OUTPUT_LIMIT_MAX_W_RESET 0x00000F00U
#define FX1_ISP_CH1_OUTPUT_LIMIT_MAX_W_ACCESS FX1_ISP_ACCESS_RO
/**
 * MAX_H [28:16], dòng bảng 1308, kiểu truy cập RO
 * Chiều cao đầu ra tối đa.
 */
#define FX1_ISP_CH1_OUTPUT_LIMIT_MAX_H_SHIFT 16U
#define FX1_ISP_CH1_OUTPUT_LIMIT_MAX_H_WIDTH 13U
#define FX1_ISP_CH1_OUTPUT_LIMIT_MAX_H_MASK 0x1FFF0000U
#define FX1_ISP_CH1_OUTPUT_LIMIT_MAX_H_RESET 0x00000870U
#define FX1_ISP_CH1_OUTPUT_LIMIT_MAX_H_ACCESS FX1_ISP_ACCESS_RO

/**
 * CH2_CROP_CTRL: địa chỉ lệch 0x8BC0, dòng bảng 1309, kiểu truy cập RW
 * Crop YUV sau IQ dùng chung và trước resizer riêng của kênh. Thay đổi yêu cầu dừng/xử lý hết; ghi
 * hình học được nhận làm mất tính hợp lệ đến khi kiểm tra lại.
 */
#define FX1_ISP_CH2_CROP_CTRL_OFFSET 0x8BC0U
#define REG_CH2_CROP_CTRL FX1_ISP_CH2_CROP_CTRL_OFFSET
#define FX1_ISP_CH2_CROP_CTRL_RESET 0x00000000U
#define FX1_ISP_CH2_CROP_CTRL_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_CH2_CROP_CTRL_RO_MASK 0x00000000U
#define FX1_ISP_CH2_CROP_CTRL_RW_MASK 0x00000001U
#define FX1_ISP_CH2_CROP_CTRL_W1C_MASK 0x00000000U
#define FX1_ISP_CH2_CROP_CTRL_W1S_MASK 0x00000000U
#define FX1_ISP_CH2_CROP_CTRL_W1P_MASK 0x00000000U
#define FX1_ISP_CH2_CROP_CTRL_W1SC_MASK 0x00000000U
/**
 * ENABLE [0:0], dòng bảng 1310, kiểu truy cập RW
 * 0=toàn ảnh hữu dụng,1=hình crop chẵn tường minh.
 */
#define FX1_ISP_CH2_CROP_CTRL_ENABLE_SHIFT 0U
#define FX1_ISP_CH2_CROP_CTRL_ENABLE_WIDTH 1U
#define FX1_ISP_CH2_CROP_CTRL_ENABLE_MASK 0x00000001U
#define FX1_ISP_CH2_CROP_CTRL_ENABLE_RESET 0x00000000U
#define FX1_ISP_CH2_CROP_CTRL_ENABLE_ACCESS FX1_ISP_ACCESS_RW

/**
 * CH2_CROP_ORIGIN: địa chỉ lệch 0x8BC4, dòng bảng 1311, kiểu truy cập RW
 * Crop theo pixel ảnh hữu dụng: gốc/kích thước chẵn, không rỗng, nằm hoàn toàn trong ảnh; bật với
 * kích thước 0 là sai.
 */
#define FX1_ISP_CH2_CROP_ORIGIN_OFFSET 0x8BC4U
#define REG_CH2_CROP_ORIGIN FX1_ISP_CH2_CROP_ORIGIN_OFFSET
#define FX1_ISP_CH2_CROP_ORIGIN_RESET 0x00000000U
#define FX1_ISP_CH2_CROP_ORIGIN_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_CH2_CROP_ORIGIN_RO_MASK 0x00000000U
#define FX1_ISP_CH2_CROP_ORIGIN_RW_MASK 0x1FFF1FFFU
#define FX1_ISP_CH2_CROP_ORIGIN_W1C_MASK 0x00000000U
#define FX1_ISP_CH2_CROP_ORIGIN_W1S_MASK 0x00000000U
#define FX1_ISP_CH2_CROP_ORIGIN_W1P_MASK 0x00000000U
#define FX1_ISP_CH2_CROP_ORIGIN_W1SC_MASK 0x00000000U
/**
 * X [12:0], dòng bảng 1312, kiểu truy cập RW
 * Tọa độ hoặc kích thước ngang.
 */
#define FX1_ISP_CH2_CROP_ORIGIN_X_SHIFT 0U
#define FX1_ISP_CH2_CROP_ORIGIN_X_WIDTH 13U
#define FX1_ISP_CH2_CROP_ORIGIN_X_MASK 0x00001FFFU
#define FX1_ISP_CH2_CROP_ORIGIN_X_RESET 0x00000000U
#define FX1_ISP_CH2_CROP_ORIGIN_X_ACCESS FX1_ISP_ACCESS_RW
/**
 * Y [28:16], dòng bảng 1313, kiểu truy cập RW
 * Tọa độ hoặc kích thước dọc.
 */
#define FX1_ISP_CH2_CROP_ORIGIN_Y_SHIFT 16U
#define FX1_ISP_CH2_CROP_ORIGIN_Y_WIDTH 13U
#define FX1_ISP_CH2_CROP_ORIGIN_Y_MASK 0x1FFF0000U
#define FX1_ISP_CH2_CROP_ORIGIN_Y_RESET 0x00000000U
#define FX1_ISP_CH2_CROP_ORIGIN_Y_ACCESS FX1_ISP_ACCESS_RW

/**
 * CH2_CROP_SIZE: địa chỉ lệch 0x8BC8, dòng bảng 1314, kiểu truy cập RW
 * Crop theo pixel ảnh hữu dụng: gốc/kích thước chẵn, không rỗng, nằm hoàn toàn trong ảnh; bật với
 * kích thước 0 là sai.
 */
#define FX1_ISP_CH2_CROP_SIZE_OFFSET 0x8BC8U
#define REG_CH2_CROP_SIZE FX1_ISP_CH2_CROP_SIZE_OFFSET
#define FX1_ISP_CH2_CROP_SIZE_RESET 0x00000000U
#define FX1_ISP_CH2_CROP_SIZE_ACCESS FX1_ISP_ACCESS_RW
#define FX1_ISP_CH2_CROP_SIZE_RO_MASK 0x00000000U
#define FX1_ISP_CH2_CROP_SIZE_RW_MASK 0x1FFF1FFFU
#define FX1_ISP_CH2_CROP_SIZE_W1C_MASK 0x00000000U
#define FX1_ISP_CH2_CROP_SIZE_W1S_MASK 0x00000000U
#define FX1_ISP_CH2_CROP_SIZE_W1P_MASK 0x00000000U
#define FX1_ISP_CH2_CROP_SIZE_W1SC_MASK 0x00000000U
/**
 * W [12:0], dòng bảng 1315, kiểu truy cập RW
 * Tọa độ hoặc kích thước ngang.
 */
#define FX1_ISP_CH2_CROP_SIZE_W_SHIFT 0U
#define FX1_ISP_CH2_CROP_SIZE_W_WIDTH 13U
#define FX1_ISP_CH2_CROP_SIZE_W_MASK 0x00001FFFU
#define FX1_ISP_CH2_CROP_SIZE_W_RESET 0x00000000U
#define FX1_ISP_CH2_CROP_SIZE_W_ACCESS FX1_ISP_ACCESS_RW
/**
 * H [28:16], dòng bảng 1316, kiểu truy cập RW
 * Tọa độ hoặc kích thước dọc.
 */
#define FX1_ISP_CH2_CROP_SIZE_H_SHIFT 16U
#define FX1_ISP_CH2_CROP_SIZE_H_WIDTH 13U
#define FX1_ISP_CH2_CROP_SIZE_H_MASK 0x1FFF0000U
#define FX1_ISP_CH2_CROP_SIZE_H_RESET 0x00000000U
#define FX1_ISP_CH2_CROP_SIZE_H_ACCESS FX1_ISP_ACCESS_RW

/**
 * CH2_GEOMETRY_STATUS: địa chỉ lệch 0x8BCC, dòng bảng 1317, kiểu truy cập RO
 * Trạng thái kiểm tra hình học hiện tại. Giá trị đọc cũ không cấp phép tiếp nhận khi VALID=0. ERROR
 * là mức trực tiếp, xóa khi có cấu hình hợp lệ ổn định; cấu hình sai còn đặt TUNE_ERROR.INVALID đã
 * chốt.
 */
#define FX1_ISP_CH2_GEOMETRY_STATUS_OFFSET 0x8BCCU
#define REG_CH2_GEOMETRY_STATUS FX1_ISP_CH2_GEOMETRY_STATUS_OFFSET
#define FX1_ISP_CH2_GEOMETRY_STATUS_RESET 0x00000000U
#define FX1_ISP_CH2_GEOMETRY_STATUS_ACCESS FX1_ISP_ACCESS_RO
#define FX1_ISP_CH2_GEOMETRY_STATUS_RO_MASK 0x000000F3U
#define FX1_ISP_CH2_GEOMETRY_STATUS_RW_MASK 0x00000000U
#define FX1_ISP_CH2_GEOMETRY_STATUS_W1C_MASK 0x00000000U
#define FX1_ISP_CH2_GEOMETRY_STATUS_W1S_MASK 0x00000000U
#define FX1_ISP_CH2_GEOMETRY_STATUS_W1P_MASK 0x00000000U
#define FX1_ISP_CH2_GEOMETRY_STATUS_W1SC_MASK 0x00000000U
/**
 * VALID [0:0], dòng bảng 1318, kiểu truy cập RO
 * VALID=1 chỉ khi toàn bộ hình học ổn định/hợp lệ. Reset hoặc ghi hình học liên quan xóa bit.
 */
#define FX1_ISP_CH2_GEOMETRY_STATUS_VALID_SHIFT 0U
#define FX1_ISP_CH2_GEOMETRY_STATUS_VALID_WIDTH 1U
#define FX1_ISP_CH2_GEOMETRY_STATUS_VALID_MASK 0x00000001U
#define FX1_ISP_CH2_GEOMETRY_STATUS_VALID_RESET 0x00000000U
#define FX1_ISP_CH2_GEOMETRY_STATUS_VALID_ACCESS FX1_ISP_ACCESS_RO
/**
 * BUSY [1:1], dòng bảng 1319, kiểu truy cập RO
 * Đang chờ tính toán hoặc kiểm tra hình học.
 */
#define FX1_ISP_CH2_GEOMETRY_STATUS_BUSY_SHIFT 1U
#define FX1_ISP_CH2_GEOMETRY_STATUS_BUSY_WIDTH 1U
#define FX1_ISP_CH2_GEOMETRY_STATUS_BUSY_MASK 0x00000002U
#define FX1_ISP_CH2_GEOMETRY_STATUS_BUSY_RESET 0x00000000U
#define FX1_ISP_CH2_GEOMETRY_STATUS_BUSY_ACCESS FX1_ISP_ACCESS_RO
/**
 * ERROR [7:4], dòng bảng 1320, kiểu truy cập RO
 * Mã ưu tiên: 0=không lỗi,1=định dạng/kích thước hữu dụng,2=crop rỗng/ngoài biên,3=crop lẻ,4=mã
 * resize dự phòng,5=đích vượt giới hạn kênh,6=đích vượt crop. Chọn mã khác 0 nhỏ nhất phù hợp. BUSY
 * kéo VALID=0 và ERROR=0.
 */
#define FX1_ISP_CH2_GEOMETRY_STATUS_ERROR_SHIFT 4U
#define FX1_ISP_CH2_GEOMETRY_STATUS_ERROR_WIDTH 4U
#define FX1_ISP_CH2_GEOMETRY_STATUS_ERROR_MASK 0x000000F0U
#define FX1_ISP_CH2_GEOMETRY_STATUS_ERROR_RESET 0x00000000U
#define FX1_ISP_CH2_GEOMETRY_STATUS_ERROR_ACCESS FX1_ISP_ACCESS_RO

/**
 * CH2_ACTIVE_CROP_ORIGIN: địa chỉ lệch 0x8BD0, dòng bảng 1321, kiểu truy cập RO
 * Crop hợp lệ công bố nguyên tử cùng OUT_W/H và VALID. Crop tắt nghĩa là gốc 0 và toàn ảnh hữu dụng.
 * Trước lần hợp lệ đầu đọc 0; khi bận/sai giữ bản cũ nên phải xem VALID. Frame đang chạy giữ bản đã
 * chốt riêng.
 */
#define FX1_ISP_CH2_ACTIVE_CROP_ORIGIN_OFFSET 0x8BD0U
#define REG_CH2_ACTIVE_CROP_ORIGIN FX1_ISP_CH2_ACTIVE_CROP_ORIGIN_OFFSET
#define FX1_ISP_CH2_ACTIVE_CROP_ORIGIN_RESET 0x00000000U
#define FX1_ISP_CH2_ACTIVE_CROP_ORIGIN_ACCESS FX1_ISP_ACCESS_RO
#define FX1_ISP_CH2_ACTIVE_CROP_ORIGIN_RO_MASK 0x1FFF1FFFU
#define FX1_ISP_CH2_ACTIVE_CROP_ORIGIN_RW_MASK 0x00000000U
#define FX1_ISP_CH2_ACTIVE_CROP_ORIGIN_W1C_MASK 0x00000000U
#define FX1_ISP_CH2_ACTIVE_CROP_ORIGIN_W1S_MASK 0x00000000U
#define FX1_ISP_CH2_ACTIVE_CROP_ORIGIN_W1P_MASK 0x00000000U
#define FX1_ISP_CH2_ACTIVE_CROP_ORIGIN_W1SC_MASK 0x00000000U
/**
 * X [12:0], dòng bảng 1322, kiểu truy cập RO
 * Tọa độ hoặc kích thước ngang đã kiểm tra.
 */
#define FX1_ISP_CH2_ACTIVE_CROP_ORIGIN_X_SHIFT 0U
#define FX1_ISP_CH2_ACTIVE_CROP_ORIGIN_X_WIDTH 13U
#define FX1_ISP_CH2_ACTIVE_CROP_ORIGIN_X_MASK 0x00001FFFU
#define FX1_ISP_CH2_ACTIVE_CROP_ORIGIN_X_RESET 0x00000000U
#define FX1_ISP_CH2_ACTIVE_CROP_ORIGIN_X_ACCESS FX1_ISP_ACCESS_RO
/**
 * Y [28:16], dòng bảng 1323, kiểu truy cập RO
 * Tọa độ hoặc kích thước dọc đã kiểm tra.
 */
#define FX1_ISP_CH2_ACTIVE_CROP_ORIGIN_Y_SHIFT 16U
#define FX1_ISP_CH2_ACTIVE_CROP_ORIGIN_Y_WIDTH 13U
#define FX1_ISP_CH2_ACTIVE_CROP_ORIGIN_Y_MASK 0x1FFF0000U
#define FX1_ISP_CH2_ACTIVE_CROP_ORIGIN_Y_RESET 0x00000000U
#define FX1_ISP_CH2_ACTIVE_CROP_ORIGIN_Y_ACCESS FX1_ISP_ACCESS_RO

/**
 * CH2_ACTIVE_CROP_SIZE: địa chỉ lệch 0x8BD4, dòng bảng 1324, kiểu truy cập RO
 * Crop hợp lệ công bố nguyên tử cùng OUT_W/H và VALID. Crop tắt nghĩa là gốc 0 và toàn ảnh hữu dụng.
 * Trước lần hợp lệ đầu đọc 0; khi bận/sai giữ bản cũ nên phải xem VALID. Frame đang chạy giữ bản đã
 * chốt riêng.
 */
#define FX1_ISP_CH2_ACTIVE_CROP_SIZE_OFFSET 0x8BD4U
#define REG_CH2_ACTIVE_CROP_SIZE FX1_ISP_CH2_ACTIVE_CROP_SIZE_OFFSET
#define FX1_ISP_CH2_ACTIVE_CROP_SIZE_RESET 0x00000000U
#define FX1_ISP_CH2_ACTIVE_CROP_SIZE_ACCESS FX1_ISP_ACCESS_RO
#define FX1_ISP_CH2_ACTIVE_CROP_SIZE_RO_MASK 0x1FFF1FFFU
#define FX1_ISP_CH2_ACTIVE_CROP_SIZE_RW_MASK 0x00000000U
#define FX1_ISP_CH2_ACTIVE_CROP_SIZE_W1C_MASK 0x00000000U
#define FX1_ISP_CH2_ACTIVE_CROP_SIZE_W1S_MASK 0x00000000U
#define FX1_ISP_CH2_ACTIVE_CROP_SIZE_W1P_MASK 0x00000000U
#define FX1_ISP_CH2_ACTIVE_CROP_SIZE_W1SC_MASK 0x00000000U
/**
 * W [12:0], dòng bảng 1325, kiểu truy cập RO
 * Tọa độ hoặc kích thước ngang đã kiểm tra.
 */
#define FX1_ISP_CH2_ACTIVE_CROP_SIZE_W_SHIFT 0U
#define FX1_ISP_CH2_ACTIVE_CROP_SIZE_W_WIDTH 13U
#define FX1_ISP_CH2_ACTIVE_CROP_SIZE_W_MASK 0x00001FFFU
#define FX1_ISP_CH2_ACTIVE_CROP_SIZE_W_RESET 0x00000000U
#define FX1_ISP_CH2_ACTIVE_CROP_SIZE_W_ACCESS FX1_ISP_ACCESS_RO
/**
 * H [28:16], dòng bảng 1326, kiểu truy cập RO
 * Tọa độ hoặc kích thước dọc đã kiểm tra.
 */
#define FX1_ISP_CH2_ACTIVE_CROP_SIZE_H_SHIFT 16U
#define FX1_ISP_CH2_ACTIVE_CROP_SIZE_H_WIDTH 13U
#define FX1_ISP_CH2_ACTIVE_CROP_SIZE_H_MASK 0x1FFF0000U
#define FX1_ISP_CH2_ACTIVE_CROP_SIZE_H_RESET 0x00000000U
#define FX1_ISP_CH2_ACTIVE_CROP_SIZE_H_ACCESS FX1_ISP_ACCESS_RO

/**
 * CH2_OUTPUT_LIMIT: địa chỉ lệch 0x8BD8, dòng bảng 1327, kiểu truy cập RO
 * Giới hạn đầu ra cố định của kênh, không cho phép phóng to hoặc kích thước tùy ý.
 */
#define FX1_ISP_CH2_OUTPUT_LIMIT_OFFSET 0x8BD8U
#define REG_CH2_OUTPUT_LIMIT FX1_ISP_CH2_OUTPUT_LIMIT_OFFSET
#define FX1_ISP_CH2_OUTPUT_LIMIT_RESET 0x02800280U
#define FX1_ISP_CH2_OUTPUT_LIMIT_ACCESS FX1_ISP_ACCESS_RO
#define FX1_ISP_CH2_OUTPUT_LIMIT_RO_MASK 0x1FFF1FFFU
#define FX1_ISP_CH2_OUTPUT_LIMIT_RW_MASK 0x00000000U
#define FX1_ISP_CH2_OUTPUT_LIMIT_W1C_MASK 0x00000000U
#define FX1_ISP_CH2_OUTPUT_LIMIT_W1S_MASK 0x00000000U
#define FX1_ISP_CH2_OUTPUT_LIMIT_W1P_MASK 0x00000000U
#define FX1_ISP_CH2_OUTPUT_LIMIT_W1SC_MASK 0x00000000U
/**
 * MAX_W [12:0], dòng bảng 1328, kiểu truy cập RO
 * Chiều rộng đầu ra tối đa.
 */
#define FX1_ISP_CH2_OUTPUT_LIMIT_MAX_W_SHIFT 0U
#define FX1_ISP_CH2_OUTPUT_LIMIT_MAX_W_WIDTH 13U
#define FX1_ISP_CH2_OUTPUT_LIMIT_MAX_W_MASK 0x00001FFFU
#define FX1_ISP_CH2_OUTPUT_LIMIT_MAX_W_RESET 0x00000280U
#define FX1_ISP_CH2_OUTPUT_LIMIT_MAX_W_ACCESS FX1_ISP_ACCESS_RO
/**
 * MAX_H [28:16], dòng bảng 1329, kiểu truy cập RO
 * Chiều cao đầu ra tối đa.
 */
#define FX1_ISP_CH2_OUTPUT_LIMIT_MAX_H_SHIFT 16U
#define FX1_ISP_CH2_OUTPUT_LIMIT_MAX_H_WIDTH 13U
#define FX1_ISP_CH2_OUTPUT_LIMIT_MAX_H_MASK 0x1FFF0000U
#define FX1_ISP_CH2_OUTPUT_LIMIT_MAX_H_RESET 0x00000280U
#define FX1_ISP_CH2_OUTPUT_LIMIT_MAX_H_ACCESS FX1_ISP_ACCESS_RO

/**
 * COMMON_RECOVERY_STATUS: địa chỉ lệch 0x8C00, dòng bảng 1330, kiểu truy cập Mixed
 * Trạng thái recovery trực tiếp nằm ngoài miền soft reset. Chỉ đọc bản ghi khi BUSY=0 và số lần
 * recovery ổn định.
 */
#define FX1_ISP_COMMON_RECOVERY_STATUS_OFFSET 0x8C00U
#define REG_COMMON_RECOVERY_STATUS FX1_ISP_COMMON_RECOVERY_STATUS_OFFSET
#define FX1_ISP_COMMON_RECOVERY_STATUS_RESET 0x00000000U
#define FX1_ISP_COMMON_RECOVERY_STATUS_ACCESS FX1_ISP_ACCESS_MIXED
#define FX1_ISP_COMMON_RECOVERY_STATUS_RO_MASK 0x00070331U
#define FX1_ISP_COMMON_RECOVERY_STATUS_RW_MASK 0x00000000U
#define FX1_ISP_COMMON_RECOVERY_STATUS_W1C_MASK 0x00000002U
#define FX1_ISP_COMMON_RECOVERY_STATUS_W1S_MASK 0x00000000U
#define FX1_ISP_COMMON_RECOVERY_STATUS_W1P_MASK 0x00000000U
#define FX1_ISP_COMMON_RECOVERY_STATUS_W1SC_MASK 0x00000000U
/**
 * BUSY [0:0], dòng bảng 1331, kiểu truy cập RO
 * Đang recovery; chưa cấp quyền DMA lại khi bit còn 1.
 */
#define FX1_ISP_COMMON_RECOVERY_STATUS_BUSY_SHIFT 0U
#define FX1_ISP_COMMON_RECOVERY_STATUS_BUSY_WIDTH 1U
#define FX1_ISP_COMMON_RECOVERY_STATUS_BUSY_MASK 0x00000001U
#define FX1_ISP_COMMON_RECOVERY_STATUS_BUSY_RESET 0x00000000U
#define FX1_ISP_COMMON_RECOVERY_STATUS_BUSY_ACCESS FX1_ISP_ACCESS_RO
/**
 * DONE [1:1], dòng bảng 1332, kiểu truy cập W1C
 * Recovery hoàn tất, W1C, đặt mới thắng xóa. Mỗi recovery được nhận xóa cờ trước.
 */
#define FX1_ISP_COMMON_RECOVERY_STATUS_DONE_SHIFT 1U
#define FX1_ISP_COMMON_RECOVERY_STATUS_DONE_WIDTH 1U
#define FX1_ISP_COMMON_RECOVERY_STATUS_DONE_MASK 0x00000002U
#define FX1_ISP_COMMON_RECOVERY_STATUS_DONE_RESET 0x00000000U
#define FX1_ISP_COMMON_RECOVERY_STATUS_DONE_ACCESS FX1_ISP_ACCESS_W1C
/**
 * SCOPE [5:4], dòng bảng 1333, kiểu truy cập RO
 * Phạm vi recovery: 0=không,1=DMA/lỗi tự động,2=common,3=dự phòng.
 */
#define FX1_ISP_COMMON_RECOVERY_STATUS_SCOPE_SHIFT 4U
#define FX1_ISP_COMMON_RECOVERY_STATUS_SCOPE_WIDTH 2U
#define FX1_ISP_COMMON_RECOVERY_STATUS_SCOPE_MASK 0x00000030U
#define FX1_ISP_COMMON_RECOVERY_STATUS_SCOPE_RESET 0x00000000U
#define FX1_ISP_COMMON_RECOVERY_STATUS_SCOPE_ACCESS FX1_ISP_ACCESS_RO
/**
 * PHASE [9:8], dòng bảng 1334, kiểu truy cập RO
 * Pha recovery: 0=rỗi,1=xử lý hết bus/nội bộ,2=xung reset,3=kết thúc công việc cuối.
 */
#define FX1_ISP_COMMON_RECOVERY_STATUS_PHASE_SHIFT 8U
#define FX1_ISP_COMMON_RECOVERY_STATUS_PHASE_WIDTH 2U
#define FX1_ISP_COMMON_RECOVERY_STATUS_PHASE_MASK 0x00000300U
#define FX1_ISP_COMMON_RECOVERY_STATUS_PHASE_RESET 0x00000000U
#define FX1_ISP_COMMON_RECOVERY_STATUS_PHASE_ACCESS FX1_ISP_ACCESS_RO
/**
 * COUNT [18:16], dòng bảng 1335, kiểu truy cập RO
 * Số bản ghi hủy hợp lệ đầu mảng, 0..4 theo thứ tự nhận frame đầu vào.
 */
#define FX1_ISP_COMMON_RECOVERY_STATUS_COUNT_SHIFT 16U
#define FX1_ISP_COMMON_RECOVERY_STATUS_COUNT_WIDTH 3U
#define FX1_ISP_COMMON_RECOVERY_STATUS_COUNT_MASK 0x00070000U
#define FX1_ISP_COMMON_RECOVERY_STATUS_COUNT_RESET 0x00000000U
#define FX1_ISP_COMMON_RECOVERY_STATUS_COUNT_ACCESS FX1_ISP_ACCESS_RO

/**
 * COMMON_RECOVERY_SEQUENCE: địa chỉ lệch 0x8C04, dòng bảng 1336, kiểu truy cập RO
 * Tăng modulo 2^32 mỗi recovery được nhận; chỉ hard reset xóa.
 */
#define FX1_ISP_COMMON_RECOVERY_SEQUENCE_OFFSET 0x8C04U
#define REG_COMMON_RECOVERY_SEQUENCE FX1_ISP_COMMON_RECOVERY_SEQUENCE_OFFSET
#define FX1_ISP_COMMON_RECOVERY_SEQUENCE_RESET 0x00000000U
#define FX1_ISP_COMMON_RECOVERY_SEQUENCE_ACCESS FX1_ISP_ACCESS_RO
#define FX1_ISP_COMMON_RECOVERY_SEQUENCE_RO_MASK 0xFFFFFFFFU
#define FX1_ISP_COMMON_RECOVERY_SEQUENCE_RW_MASK 0x00000000U
#define FX1_ISP_COMMON_RECOVERY_SEQUENCE_W1C_MASK 0x00000000U
#define FX1_ISP_COMMON_RECOVERY_SEQUENCE_W1S_MASK 0x00000000U
#define FX1_ISP_COMMON_RECOVERY_SEQUENCE_W1P_MASK 0x00000000U
#define FX1_ISP_COMMON_RECOVERY_SEQUENCE_W1SC_MASK 0x00000000U
/**
 * VALUE [31:0], dòng bảng 1337, kiểu truy cập RO
 * Số thế hệ recovery; kiểm tra lại trước/sau khi đọc bản ghi.
 */
#define FX1_ISP_COMMON_RECOVERY_SEQUENCE_VALUE_SHIFT 0U
#define FX1_ISP_COMMON_RECOVERY_SEQUENCE_VALUE_WIDTH 32U
#define FX1_ISP_COMMON_RECOVERY_SEQUENCE_VALUE_MASK 0xFFFFFFFFU
#define FX1_ISP_COMMON_RECOVERY_SEQUENCE_VALUE_RESET 0x00000000U
#define FX1_ISP_COMMON_RECOVERY_SEQUENCE_VALUE_ACCESS FX1_ISP_ACCESS_RO

/**
 * COMMON_ABORT0_FRAME: địa chỉ lệch 0x8C10, dòng bảng 1338, kiểu truy cập RO
 * ID frame đầu vào đã chốt; chỉ hợp lệ trong RECOVERY_STATUS.COUNT, bản ghi không dùng đọc 0.
 */
#define FX1_ISP_COMMON_ABORT0_FRAME_OFFSET 0x8C10U
#define REG_COMMON_ABORT0_FRAME FX1_ISP_COMMON_ABORT0_FRAME_OFFSET
#define FX1_ISP_COMMON_ABORT0_FRAME_RESET 0x00000000U
#define FX1_ISP_COMMON_ABORT0_FRAME_ACCESS FX1_ISP_ACCESS_RO
#define FX1_ISP_COMMON_ABORT0_FRAME_RO_MASK 0xFFFFFFFFU
#define FX1_ISP_COMMON_ABORT0_FRAME_RW_MASK 0x00000000U
#define FX1_ISP_COMMON_ABORT0_FRAME_W1C_MASK 0x00000000U
#define FX1_ISP_COMMON_ABORT0_FRAME_W1S_MASK 0x00000000U
#define FX1_ISP_COMMON_ABORT0_FRAME_W1P_MASK 0x00000000U
#define FX1_ISP_COMMON_ABORT0_FRAME_W1SC_MASK 0x00000000U
/**
 * FRAME [31:0], dòng bảng 1339, kiểu truy cập RO
 * Số thứ tự đầu vào bất biến của context chưa kết thúc, được chốt phục vụ recovery.
 */
#define FX1_ISP_COMMON_ABORT0_FRAME_FRAME_SHIFT 0U
#define FX1_ISP_COMMON_ABORT0_FRAME_FRAME_WIDTH 32U
#define FX1_ISP_COMMON_ABORT0_FRAME_FRAME_MASK 0xFFFFFFFFU
#define FX1_ISP_COMMON_ABORT0_FRAME_FRAME_RESET 0x00000000U
#define FX1_ISP_COMMON_ABORT0_FRAME_FRAME_ACCESS FX1_ISP_ACCESS_RO

/**
 * COMMON_ABORT0_OUTCOME: địa chỉ lệch 0x8C14, dòng bảng 1340, kiểu truy cập RO
 * Kết quả frame bị hủy giữ đến recovery sau; ngoài recovery chỉ hard reset xóa.
 */
#define FX1_ISP_COMMON_ABORT0_OUTCOME_OFFSET 0x8C14U
#define REG_COMMON_ABORT0_OUTCOME FX1_ISP_COMMON_ABORT0_OUTCOME_OFFSET
#define FX1_ISP_COMMON_ABORT0_OUTCOME_RESET 0x00000000U
#define FX1_ISP_COMMON_ABORT0_OUTCOME_ACCESS FX1_ISP_ACCESS_RO
#define FX1_ISP_COMMON_ABORT0_OUTCOME_RO_MASK 0x00000707U
#define FX1_ISP_COMMON_ABORT0_OUTCOME_RW_MASK 0x00000000U
#define FX1_ISP_COMMON_ABORT0_OUTCOME_W1C_MASK 0x00000000U
#define FX1_ISP_COMMON_ABORT0_OUTCOME_W1S_MASK 0x00000000U
#define FX1_ISP_COMMON_ABORT0_OUTCOME_W1P_MASK 0x00000000U
#define FX1_ISP_COMMON_ABORT0_OUTCOME_W1SC_MASK 0x00000000U
/**
 * FAILED_OUTPUTS [2:0], dòng bảng 1341, kiểu truy cập RO
 * Mặt nạ đầu ra CH0/1/2 bật nhưng chưa kết thúc thành công tại lúc chốt recovery.
 */
#define FX1_ISP_COMMON_ABORT0_OUTCOME_FAILED_OUTPUTS_SHIFT 0U
#define FX1_ISP_COMMON_ABORT0_OUTCOME_FAILED_OUTPUTS_WIDTH 3U
#define FX1_ISP_COMMON_ABORT0_OUTCOME_FAILED_OUTPUTS_MASK 0x00000007U
#define FX1_ISP_COMMON_ABORT0_OUTCOME_FAILED_OUTPUTS_RESET 0x00000000U
#define FX1_ISP_COMMON_ABORT0_OUTCOME_FAILED_OUTPUTS_ACCESS FX1_ISP_ACCESS_RO
/**
 * CAUSE [10:8], dòng bảng 1342, kiểu truy cập RO
 * Nguyên nhân hủy: 1=lỗi đọc,2=lỗi ghi,3=lệnh DMA,4=lệnh common,5=frame sai dạng. Lỗi đọc/ghi đồng
 * thời dùng mã 1, nhưng trạng thái chốt giữ cả hai lỗi.
 */
#define FX1_ISP_COMMON_ABORT0_OUTCOME_CAUSE_SHIFT 8U
#define FX1_ISP_COMMON_ABORT0_OUTCOME_CAUSE_WIDTH 3U
#define FX1_ISP_COMMON_ABORT0_OUTCOME_CAUSE_MASK 0x00000700U
#define FX1_ISP_COMMON_ABORT0_OUTCOME_CAUSE_RESET 0x00000000U
#define FX1_ISP_COMMON_ABORT0_OUTCOME_CAUSE_ACCESS FX1_ISP_ACCESS_RO

/**
 * COMMON_ABORT1_FRAME: địa chỉ lệch 0x8C18, dòng bảng 1343, kiểu truy cập RO
 * ID frame đầu vào đã chốt; chỉ hợp lệ trong RECOVERY_STATUS.COUNT, bản ghi không dùng đọc 0.
 */
#define FX1_ISP_COMMON_ABORT1_FRAME_OFFSET 0x8C18U
#define REG_COMMON_ABORT1_FRAME FX1_ISP_COMMON_ABORT1_FRAME_OFFSET
#define FX1_ISP_COMMON_ABORT1_FRAME_RESET 0x00000000U
#define FX1_ISP_COMMON_ABORT1_FRAME_ACCESS FX1_ISP_ACCESS_RO
#define FX1_ISP_COMMON_ABORT1_FRAME_RO_MASK 0xFFFFFFFFU
#define FX1_ISP_COMMON_ABORT1_FRAME_RW_MASK 0x00000000U
#define FX1_ISP_COMMON_ABORT1_FRAME_W1C_MASK 0x00000000U
#define FX1_ISP_COMMON_ABORT1_FRAME_W1S_MASK 0x00000000U
#define FX1_ISP_COMMON_ABORT1_FRAME_W1P_MASK 0x00000000U
#define FX1_ISP_COMMON_ABORT1_FRAME_W1SC_MASK 0x00000000U
/**
 * FRAME [31:0], dòng bảng 1344, kiểu truy cập RO
 * Số thứ tự đầu vào bất biến của context chưa kết thúc, được chốt phục vụ recovery.
 */
#define FX1_ISP_COMMON_ABORT1_FRAME_FRAME_SHIFT 0U
#define FX1_ISP_COMMON_ABORT1_FRAME_FRAME_WIDTH 32U
#define FX1_ISP_COMMON_ABORT1_FRAME_FRAME_MASK 0xFFFFFFFFU
#define FX1_ISP_COMMON_ABORT1_FRAME_FRAME_RESET 0x00000000U
#define FX1_ISP_COMMON_ABORT1_FRAME_FRAME_ACCESS FX1_ISP_ACCESS_RO

/**
 * COMMON_ABORT1_OUTCOME: địa chỉ lệch 0x8C1C, dòng bảng 1345, kiểu truy cập RO
 * Kết quả frame bị hủy giữ đến recovery sau; ngoài recovery chỉ hard reset xóa.
 */
#define FX1_ISP_COMMON_ABORT1_OUTCOME_OFFSET 0x8C1CU
#define REG_COMMON_ABORT1_OUTCOME FX1_ISP_COMMON_ABORT1_OUTCOME_OFFSET
#define FX1_ISP_COMMON_ABORT1_OUTCOME_RESET 0x00000000U
#define FX1_ISP_COMMON_ABORT1_OUTCOME_ACCESS FX1_ISP_ACCESS_RO
#define FX1_ISP_COMMON_ABORT1_OUTCOME_RO_MASK 0x00000707U
#define FX1_ISP_COMMON_ABORT1_OUTCOME_RW_MASK 0x00000000U
#define FX1_ISP_COMMON_ABORT1_OUTCOME_W1C_MASK 0x00000000U
#define FX1_ISP_COMMON_ABORT1_OUTCOME_W1S_MASK 0x00000000U
#define FX1_ISP_COMMON_ABORT1_OUTCOME_W1P_MASK 0x00000000U
#define FX1_ISP_COMMON_ABORT1_OUTCOME_W1SC_MASK 0x00000000U
/**
 * FAILED_OUTPUTS [2:0], dòng bảng 1346, kiểu truy cập RO
 * Mặt nạ đầu ra CH0/1/2 bật nhưng chưa kết thúc thành công tại lúc chốt recovery.
 */
#define FX1_ISP_COMMON_ABORT1_OUTCOME_FAILED_OUTPUTS_SHIFT 0U
#define FX1_ISP_COMMON_ABORT1_OUTCOME_FAILED_OUTPUTS_WIDTH 3U
#define FX1_ISP_COMMON_ABORT1_OUTCOME_FAILED_OUTPUTS_MASK 0x00000007U
#define FX1_ISP_COMMON_ABORT1_OUTCOME_FAILED_OUTPUTS_RESET 0x00000000U
#define FX1_ISP_COMMON_ABORT1_OUTCOME_FAILED_OUTPUTS_ACCESS FX1_ISP_ACCESS_RO
/**
 * CAUSE [10:8], dòng bảng 1347, kiểu truy cập RO
 * Nguyên nhân hủy: 1=lỗi đọc,2=lỗi ghi,3=lệnh DMA,4=lệnh common,5=frame sai dạng. Lỗi đọc/ghi đồng
 * thời dùng mã 1, nhưng trạng thái chốt giữ cả hai lỗi.
 */
#define FX1_ISP_COMMON_ABORT1_OUTCOME_CAUSE_SHIFT 8U
#define FX1_ISP_COMMON_ABORT1_OUTCOME_CAUSE_WIDTH 3U
#define FX1_ISP_COMMON_ABORT1_OUTCOME_CAUSE_MASK 0x00000700U
#define FX1_ISP_COMMON_ABORT1_OUTCOME_CAUSE_RESET 0x00000000U
#define FX1_ISP_COMMON_ABORT1_OUTCOME_CAUSE_ACCESS FX1_ISP_ACCESS_RO

/**
 * COMMON_ABORT2_FRAME: địa chỉ lệch 0x8C20, dòng bảng 1348, kiểu truy cập RO
 * ID frame đầu vào đã chốt; chỉ hợp lệ trong RECOVERY_STATUS.COUNT, bản ghi không dùng đọc 0.
 */
#define FX1_ISP_COMMON_ABORT2_FRAME_OFFSET 0x8C20U
#define REG_COMMON_ABORT2_FRAME FX1_ISP_COMMON_ABORT2_FRAME_OFFSET
#define FX1_ISP_COMMON_ABORT2_FRAME_RESET 0x00000000U
#define FX1_ISP_COMMON_ABORT2_FRAME_ACCESS FX1_ISP_ACCESS_RO
#define FX1_ISP_COMMON_ABORT2_FRAME_RO_MASK 0xFFFFFFFFU
#define FX1_ISP_COMMON_ABORT2_FRAME_RW_MASK 0x00000000U
#define FX1_ISP_COMMON_ABORT2_FRAME_W1C_MASK 0x00000000U
#define FX1_ISP_COMMON_ABORT2_FRAME_W1S_MASK 0x00000000U
#define FX1_ISP_COMMON_ABORT2_FRAME_W1P_MASK 0x00000000U
#define FX1_ISP_COMMON_ABORT2_FRAME_W1SC_MASK 0x00000000U
/**
 * FRAME [31:0], dòng bảng 1349, kiểu truy cập RO
 * Số thứ tự đầu vào bất biến của context chưa kết thúc, được chốt phục vụ recovery.
 */
#define FX1_ISP_COMMON_ABORT2_FRAME_FRAME_SHIFT 0U
#define FX1_ISP_COMMON_ABORT2_FRAME_FRAME_WIDTH 32U
#define FX1_ISP_COMMON_ABORT2_FRAME_FRAME_MASK 0xFFFFFFFFU
#define FX1_ISP_COMMON_ABORT2_FRAME_FRAME_RESET 0x00000000U
#define FX1_ISP_COMMON_ABORT2_FRAME_FRAME_ACCESS FX1_ISP_ACCESS_RO

/**
 * COMMON_ABORT2_OUTCOME: địa chỉ lệch 0x8C24, dòng bảng 1350, kiểu truy cập RO
 * Kết quả frame bị hủy giữ đến recovery sau; ngoài recovery chỉ hard reset xóa.
 */
#define FX1_ISP_COMMON_ABORT2_OUTCOME_OFFSET 0x8C24U
#define REG_COMMON_ABORT2_OUTCOME FX1_ISP_COMMON_ABORT2_OUTCOME_OFFSET
#define FX1_ISP_COMMON_ABORT2_OUTCOME_RESET 0x00000000U
#define FX1_ISP_COMMON_ABORT2_OUTCOME_ACCESS FX1_ISP_ACCESS_RO
#define FX1_ISP_COMMON_ABORT2_OUTCOME_RO_MASK 0x00000707U
#define FX1_ISP_COMMON_ABORT2_OUTCOME_RW_MASK 0x00000000U
#define FX1_ISP_COMMON_ABORT2_OUTCOME_W1C_MASK 0x00000000U
#define FX1_ISP_COMMON_ABORT2_OUTCOME_W1S_MASK 0x00000000U
#define FX1_ISP_COMMON_ABORT2_OUTCOME_W1P_MASK 0x00000000U
#define FX1_ISP_COMMON_ABORT2_OUTCOME_W1SC_MASK 0x00000000U
/**
 * FAILED_OUTPUTS [2:0], dòng bảng 1351, kiểu truy cập RO
 * Mặt nạ đầu ra CH0/1/2 bật nhưng chưa kết thúc thành công tại lúc chốt recovery.
 */
#define FX1_ISP_COMMON_ABORT2_OUTCOME_FAILED_OUTPUTS_SHIFT 0U
#define FX1_ISP_COMMON_ABORT2_OUTCOME_FAILED_OUTPUTS_WIDTH 3U
#define FX1_ISP_COMMON_ABORT2_OUTCOME_FAILED_OUTPUTS_MASK 0x00000007U
#define FX1_ISP_COMMON_ABORT2_OUTCOME_FAILED_OUTPUTS_RESET 0x00000000U
#define FX1_ISP_COMMON_ABORT2_OUTCOME_FAILED_OUTPUTS_ACCESS FX1_ISP_ACCESS_RO
/**
 * CAUSE [10:8], dòng bảng 1352, kiểu truy cập RO
 * Nguyên nhân hủy: 1=lỗi đọc,2=lỗi ghi,3=lệnh DMA,4=lệnh common,5=frame sai dạng. Lỗi đọc/ghi đồng
 * thời dùng mã 1, nhưng trạng thái chốt giữ cả hai lỗi.
 */
#define FX1_ISP_COMMON_ABORT2_OUTCOME_CAUSE_SHIFT 8U
#define FX1_ISP_COMMON_ABORT2_OUTCOME_CAUSE_WIDTH 3U
#define FX1_ISP_COMMON_ABORT2_OUTCOME_CAUSE_MASK 0x00000700U
#define FX1_ISP_COMMON_ABORT2_OUTCOME_CAUSE_RESET 0x00000000U
#define FX1_ISP_COMMON_ABORT2_OUTCOME_CAUSE_ACCESS FX1_ISP_ACCESS_RO

/**
 * COMMON_ABORT3_FRAME: địa chỉ lệch 0x8C28, dòng bảng 1353, kiểu truy cập RO
 * ID frame đầu vào đã chốt; chỉ hợp lệ trong RECOVERY_STATUS.COUNT, bản ghi không dùng đọc 0.
 */
#define FX1_ISP_COMMON_ABORT3_FRAME_OFFSET 0x8C28U
#define REG_COMMON_ABORT3_FRAME FX1_ISP_COMMON_ABORT3_FRAME_OFFSET
#define FX1_ISP_COMMON_ABORT3_FRAME_RESET 0x00000000U
#define FX1_ISP_COMMON_ABORT3_FRAME_ACCESS FX1_ISP_ACCESS_RO
#define FX1_ISP_COMMON_ABORT3_FRAME_RO_MASK 0xFFFFFFFFU
#define FX1_ISP_COMMON_ABORT3_FRAME_RW_MASK 0x00000000U
#define FX1_ISP_COMMON_ABORT3_FRAME_W1C_MASK 0x00000000U
#define FX1_ISP_COMMON_ABORT3_FRAME_W1S_MASK 0x00000000U
#define FX1_ISP_COMMON_ABORT3_FRAME_W1P_MASK 0x00000000U
#define FX1_ISP_COMMON_ABORT3_FRAME_W1SC_MASK 0x00000000U
/**
 * FRAME [31:0], dòng bảng 1354, kiểu truy cập RO
 * Số thứ tự đầu vào bất biến của context chưa kết thúc, được chốt phục vụ recovery.
 */
#define FX1_ISP_COMMON_ABORT3_FRAME_FRAME_SHIFT 0U
#define FX1_ISP_COMMON_ABORT3_FRAME_FRAME_WIDTH 32U
#define FX1_ISP_COMMON_ABORT3_FRAME_FRAME_MASK 0xFFFFFFFFU
#define FX1_ISP_COMMON_ABORT3_FRAME_FRAME_RESET 0x00000000U
#define FX1_ISP_COMMON_ABORT3_FRAME_FRAME_ACCESS FX1_ISP_ACCESS_RO

/**
 * COMMON_ABORT3_OUTCOME: địa chỉ lệch 0x8C2C, dòng bảng 1355, kiểu truy cập RO
 * Kết quả frame bị hủy giữ đến recovery sau; ngoài recovery chỉ hard reset xóa.
 */
#define FX1_ISP_COMMON_ABORT3_OUTCOME_OFFSET 0x8C2CU
#define REG_COMMON_ABORT3_OUTCOME FX1_ISP_COMMON_ABORT3_OUTCOME_OFFSET
#define FX1_ISP_COMMON_ABORT3_OUTCOME_RESET 0x00000000U
#define FX1_ISP_COMMON_ABORT3_OUTCOME_ACCESS FX1_ISP_ACCESS_RO
#define FX1_ISP_COMMON_ABORT3_OUTCOME_RO_MASK 0x00000707U
#define FX1_ISP_COMMON_ABORT3_OUTCOME_RW_MASK 0x00000000U
#define FX1_ISP_COMMON_ABORT3_OUTCOME_W1C_MASK 0x00000000U
#define FX1_ISP_COMMON_ABORT3_OUTCOME_W1S_MASK 0x00000000U
#define FX1_ISP_COMMON_ABORT3_OUTCOME_W1P_MASK 0x00000000U
#define FX1_ISP_COMMON_ABORT3_OUTCOME_W1SC_MASK 0x00000000U
/**
 * FAILED_OUTPUTS [2:0], dòng bảng 1356, kiểu truy cập RO
 * Mặt nạ đầu ra CH0/1/2 bật nhưng chưa kết thúc thành công tại lúc chốt recovery.
 */
#define FX1_ISP_COMMON_ABORT3_OUTCOME_FAILED_OUTPUTS_SHIFT 0U
#define FX1_ISP_COMMON_ABORT3_OUTCOME_FAILED_OUTPUTS_WIDTH 3U
#define FX1_ISP_COMMON_ABORT3_OUTCOME_FAILED_OUTPUTS_MASK 0x00000007U
#define FX1_ISP_COMMON_ABORT3_OUTCOME_FAILED_OUTPUTS_RESET 0x00000000U
#define FX1_ISP_COMMON_ABORT3_OUTCOME_FAILED_OUTPUTS_ACCESS FX1_ISP_ACCESS_RO
/**
 * CAUSE [10:8], dòng bảng 1357, kiểu truy cập RO
 * Nguyên nhân hủy: 1=lỗi đọc,2=lỗi ghi,3=lệnh DMA,4=lệnh common,5=frame sai dạng. Lỗi đọc/ghi đồng
 * thời dùng mã 1, nhưng trạng thái chốt giữ cả hai lỗi.
 */
#define FX1_ISP_COMMON_ABORT3_OUTCOME_CAUSE_SHIFT 8U
#define FX1_ISP_COMMON_ABORT3_OUTCOME_CAUSE_WIDTH 3U
#define FX1_ISP_COMMON_ABORT3_OUTCOME_CAUSE_MASK 0x00000700U
#define FX1_ISP_COMMON_ABORT3_OUTCOME_CAUSE_RESET 0x00000000U
#define FX1_ISP_COMMON_ABORT3_OUTCOME_CAUSE_ACCESS FX1_ISP_ACCESS_RO

/**
 * TUNE_CH0_BUF0_FRAME: địa chỉ lệch 0x8C40, dòng bảng 1358, kiểu truy cập RO
 * ID frame đầu vào công bố cùng DONE thành công và giữ đến lần buffer đó hoàn tất thành công tiếp.
 * Recovery riêng DMA giữ thông tin buffer đã hoàn tất trước yêu cầu; common/hard reset xóa.
 */
#define FX1_ISP_TUNE_CH0_BUF0_FRAME_OFFSET 0x8C40U
#define REG_TUNE_CH0_BUF0_FRAME FX1_ISP_TUNE_CH0_BUF0_FRAME_OFFSET
#define FX1_ISP_TUNE_CH0_BUF0_FRAME_RESET 0x00000000U
#define FX1_ISP_TUNE_CH0_BUF0_FRAME_ACCESS FX1_ISP_ACCESS_RO
#define FX1_ISP_TUNE_CH0_BUF0_FRAME_RO_MASK 0xFFFFFFFFU
#define FX1_ISP_TUNE_CH0_BUF0_FRAME_RW_MASK 0x00000000U
#define FX1_ISP_TUNE_CH0_BUF0_FRAME_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_CH0_BUF0_FRAME_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_CH0_BUF0_FRAME_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_CH0_BUF0_FRAME_W1SC_MASK 0x00000000U
/**
 * FRAME [31:0], dòng bảng 1359, kiểu truy cập RO
 * Số thứ tự frame đầu vào đã chốt, khác nhãn phần mềm tùy ý và các bộ đếm frame DMA.
 */
#define FX1_ISP_TUNE_CH0_BUF0_FRAME_FRAME_SHIFT 0U
#define FX1_ISP_TUNE_CH0_BUF0_FRAME_FRAME_WIDTH 32U
#define FX1_ISP_TUNE_CH0_BUF0_FRAME_FRAME_MASK 0xFFFFFFFFU
#define FX1_ISP_TUNE_CH0_BUF0_FRAME_FRAME_RESET 0x00000000U
#define FX1_ISP_TUNE_CH0_BUF0_FRAME_FRAME_ACCESS FX1_ISP_ACCESS_RO

/**
 * TUNE_CH0_BUF1_FRAME: địa chỉ lệch 0x8C44, dòng bảng 1360, kiểu truy cập RO
 * ID frame đầu vào công bố cùng DONE thành công và giữ đến lần buffer đó hoàn tất thành công tiếp.
 * Recovery riêng DMA giữ thông tin buffer đã hoàn tất trước yêu cầu; common/hard reset xóa.
 */
#define FX1_ISP_TUNE_CH0_BUF1_FRAME_OFFSET 0x8C44U
#define REG_TUNE_CH0_BUF1_FRAME FX1_ISP_TUNE_CH0_BUF1_FRAME_OFFSET
#define FX1_ISP_TUNE_CH0_BUF1_FRAME_RESET 0x00000000U
#define FX1_ISP_TUNE_CH0_BUF1_FRAME_ACCESS FX1_ISP_ACCESS_RO
#define FX1_ISP_TUNE_CH0_BUF1_FRAME_RO_MASK 0xFFFFFFFFU
#define FX1_ISP_TUNE_CH0_BUF1_FRAME_RW_MASK 0x00000000U
#define FX1_ISP_TUNE_CH0_BUF1_FRAME_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_CH0_BUF1_FRAME_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_CH0_BUF1_FRAME_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_CH0_BUF1_FRAME_W1SC_MASK 0x00000000U
/**
 * FRAME [31:0], dòng bảng 1361, kiểu truy cập RO
 * Số thứ tự frame đầu vào đã chốt, khác nhãn phần mềm tùy ý và các bộ đếm frame DMA.
 */
#define FX1_ISP_TUNE_CH0_BUF1_FRAME_FRAME_SHIFT 0U
#define FX1_ISP_TUNE_CH0_BUF1_FRAME_FRAME_WIDTH 32U
#define FX1_ISP_TUNE_CH0_BUF1_FRAME_FRAME_MASK 0xFFFFFFFFU
#define FX1_ISP_TUNE_CH0_BUF1_FRAME_FRAME_RESET 0x00000000U
#define FX1_ISP_TUNE_CH0_BUF1_FRAME_FRAME_ACCESS FX1_ISP_ACCESS_RO

/**
 * TUNE_CH0_BUF2_FRAME: địa chỉ lệch 0x8C48, dòng bảng 1362, kiểu truy cập RO
 * ID frame đầu vào công bố cùng DONE thành công và giữ đến lần buffer đó hoàn tất thành công tiếp.
 * Recovery riêng DMA giữ thông tin buffer đã hoàn tất trước yêu cầu; common/hard reset xóa.
 */
#define FX1_ISP_TUNE_CH0_BUF2_FRAME_OFFSET 0x8C48U
#define REG_TUNE_CH0_BUF2_FRAME FX1_ISP_TUNE_CH0_BUF2_FRAME_OFFSET
#define FX1_ISP_TUNE_CH0_BUF2_FRAME_RESET 0x00000000U
#define FX1_ISP_TUNE_CH0_BUF2_FRAME_ACCESS FX1_ISP_ACCESS_RO
#define FX1_ISP_TUNE_CH0_BUF2_FRAME_RO_MASK 0xFFFFFFFFU
#define FX1_ISP_TUNE_CH0_BUF2_FRAME_RW_MASK 0x00000000U
#define FX1_ISP_TUNE_CH0_BUF2_FRAME_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_CH0_BUF2_FRAME_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_CH0_BUF2_FRAME_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_CH0_BUF2_FRAME_W1SC_MASK 0x00000000U
/**
 * FRAME [31:0], dòng bảng 1363, kiểu truy cập RO
 * Số thứ tự frame đầu vào đã chốt, khác nhãn phần mềm tùy ý và các bộ đếm frame DMA.
 */
#define FX1_ISP_TUNE_CH0_BUF2_FRAME_FRAME_SHIFT 0U
#define FX1_ISP_TUNE_CH0_BUF2_FRAME_FRAME_WIDTH 32U
#define FX1_ISP_TUNE_CH0_BUF2_FRAME_FRAME_MASK 0xFFFFFFFFU
#define FX1_ISP_TUNE_CH0_BUF2_FRAME_FRAME_RESET 0x00000000U
#define FX1_ISP_TUNE_CH0_BUF2_FRAME_FRAME_ACCESS FX1_ISP_ACCESS_RO

/**
 * TUNE_CH0_BUF3_FRAME: địa chỉ lệch 0x8C4C, dòng bảng 1364, kiểu truy cập RO
 * ID frame đầu vào công bố cùng DONE thành công và giữ đến lần buffer đó hoàn tất thành công tiếp.
 * Recovery riêng DMA giữ thông tin buffer đã hoàn tất trước yêu cầu; common/hard reset xóa.
 */
#define FX1_ISP_TUNE_CH0_BUF3_FRAME_OFFSET 0x8C4CU
#define REG_TUNE_CH0_BUF3_FRAME FX1_ISP_TUNE_CH0_BUF3_FRAME_OFFSET
#define FX1_ISP_TUNE_CH0_BUF3_FRAME_RESET 0x00000000U
#define FX1_ISP_TUNE_CH0_BUF3_FRAME_ACCESS FX1_ISP_ACCESS_RO
#define FX1_ISP_TUNE_CH0_BUF3_FRAME_RO_MASK 0xFFFFFFFFU
#define FX1_ISP_TUNE_CH0_BUF3_FRAME_RW_MASK 0x00000000U
#define FX1_ISP_TUNE_CH0_BUF3_FRAME_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_CH0_BUF3_FRAME_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_CH0_BUF3_FRAME_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_CH0_BUF3_FRAME_W1SC_MASK 0x00000000U
/**
 * FRAME [31:0], dòng bảng 1365, kiểu truy cập RO
 * Số thứ tự frame đầu vào đã chốt, khác nhãn phần mềm tùy ý và các bộ đếm frame DMA.
 */
#define FX1_ISP_TUNE_CH0_BUF3_FRAME_FRAME_SHIFT 0U
#define FX1_ISP_TUNE_CH0_BUF3_FRAME_FRAME_WIDTH 32U
#define FX1_ISP_TUNE_CH0_BUF3_FRAME_FRAME_MASK 0xFFFFFFFFU
#define FX1_ISP_TUNE_CH0_BUF3_FRAME_FRAME_RESET 0x00000000U
#define FX1_ISP_TUNE_CH0_BUF3_FRAME_FRAME_ACCESS FX1_ISP_ACCESS_RO

/**
 * TUNE_CH1_BUF0_FRAME: địa chỉ lệch 0x8C50, dòng bảng 1366, kiểu truy cập RO
 * ID frame đầu vào công bố cùng DONE thành công và giữ đến lần buffer đó hoàn tất thành công tiếp.
 * Recovery riêng DMA giữ thông tin buffer đã hoàn tất trước yêu cầu; common/hard reset xóa.
 */
#define FX1_ISP_TUNE_CH1_BUF0_FRAME_OFFSET 0x8C50U
#define REG_TUNE_CH1_BUF0_FRAME FX1_ISP_TUNE_CH1_BUF0_FRAME_OFFSET
#define FX1_ISP_TUNE_CH1_BUF0_FRAME_RESET 0x00000000U
#define FX1_ISP_TUNE_CH1_BUF0_FRAME_ACCESS FX1_ISP_ACCESS_RO
#define FX1_ISP_TUNE_CH1_BUF0_FRAME_RO_MASK 0xFFFFFFFFU
#define FX1_ISP_TUNE_CH1_BUF0_FRAME_RW_MASK 0x00000000U
#define FX1_ISP_TUNE_CH1_BUF0_FRAME_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_CH1_BUF0_FRAME_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_CH1_BUF0_FRAME_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_CH1_BUF0_FRAME_W1SC_MASK 0x00000000U
/**
 * FRAME [31:0], dòng bảng 1367, kiểu truy cập RO
 * Số thứ tự frame đầu vào đã chốt, khác nhãn phần mềm tùy ý và các bộ đếm frame DMA.
 */
#define FX1_ISP_TUNE_CH1_BUF0_FRAME_FRAME_SHIFT 0U
#define FX1_ISP_TUNE_CH1_BUF0_FRAME_FRAME_WIDTH 32U
#define FX1_ISP_TUNE_CH1_BUF0_FRAME_FRAME_MASK 0xFFFFFFFFU
#define FX1_ISP_TUNE_CH1_BUF0_FRAME_FRAME_RESET 0x00000000U
#define FX1_ISP_TUNE_CH1_BUF0_FRAME_FRAME_ACCESS FX1_ISP_ACCESS_RO

/**
 * TUNE_CH1_BUF1_FRAME: địa chỉ lệch 0x8C54, dòng bảng 1368, kiểu truy cập RO
 * ID frame đầu vào công bố cùng DONE thành công và giữ đến lần buffer đó hoàn tất thành công tiếp.
 * Recovery riêng DMA giữ thông tin buffer đã hoàn tất trước yêu cầu; common/hard reset xóa.
 */
#define FX1_ISP_TUNE_CH1_BUF1_FRAME_OFFSET 0x8C54U
#define REG_TUNE_CH1_BUF1_FRAME FX1_ISP_TUNE_CH1_BUF1_FRAME_OFFSET
#define FX1_ISP_TUNE_CH1_BUF1_FRAME_RESET 0x00000000U
#define FX1_ISP_TUNE_CH1_BUF1_FRAME_ACCESS FX1_ISP_ACCESS_RO
#define FX1_ISP_TUNE_CH1_BUF1_FRAME_RO_MASK 0xFFFFFFFFU
#define FX1_ISP_TUNE_CH1_BUF1_FRAME_RW_MASK 0x00000000U
#define FX1_ISP_TUNE_CH1_BUF1_FRAME_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_CH1_BUF1_FRAME_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_CH1_BUF1_FRAME_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_CH1_BUF1_FRAME_W1SC_MASK 0x00000000U
/**
 * FRAME [31:0], dòng bảng 1369, kiểu truy cập RO
 * Số thứ tự frame đầu vào đã chốt, khác nhãn phần mềm tùy ý và các bộ đếm frame DMA.
 */
#define FX1_ISP_TUNE_CH1_BUF1_FRAME_FRAME_SHIFT 0U
#define FX1_ISP_TUNE_CH1_BUF1_FRAME_FRAME_WIDTH 32U
#define FX1_ISP_TUNE_CH1_BUF1_FRAME_FRAME_MASK 0xFFFFFFFFU
#define FX1_ISP_TUNE_CH1_BUF1_FRAME_FRAME_RESET 0x00000000U
#define FX1_ISP_TUNE_CH1_BUF1_FRAME_FRAME_ACCESS FX1_ISP_ACCESS_RO

/**
 * TUNE_CH1_BUF2_FRAME: địa chỉ lệch 0x8C58, dòng bảng 1370, kiểu truy cập RO
 * ID frame đầu vào công bố cùng DONE thành công và giữ đến lần buffer đó hoàn tất thành công tiếp.
 * Recovery riêng DMA giữ thông tin buffer đã hoàn tất trước yêu cầu; common/hard reset xóa.
 */
#define FX1_ISP_TUNE_CH1_BUF2_FRAME_OFFSET 0x8C58U
#define REG_TUNE_CH1_BUF2_FRAME FX1_ISP_TUNE_CH1_BUF2_FRAME_OFFSET
#define FX1_ISP_TUNE_CH1_BUF2_FRAME_RESET 0x00000000U
#define FX1_ISP_TUNE_CH1_BUF2_FRAME_ACCESS FX1_ISP_ACCESS_RO
#define FX1_ISP_TUNE_CH1_BUF2_FRAME_RO_MASK 0xFFFFFFFFU
#define FX1_ISP_TUNE_CH1_BUF2_FRAME_RW_MASK 0x00000000U
#define FX1_ISP_TUNE_CH1_BUF2_FRAME_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_CH1_BUF2_FRAME_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_CH1_BUF2_FRAME_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_CH1_BUF2_FRAME_W1SC_MASK 0x00000000U
/**
 * FRAME [31:0], dòng bảng 1371, kiểu truy cập RO
 * Số thứ tự frame đầu vào đã chốt, khác nhãn phần mềm tùy ý và các bộ đếm frame DMA.
 */
#define FX1_ISP_TUNE_CH1_BUF2_FRAME_FRAME_SHIFT 0U
#define FX1_ISP_TUNE_CH1_BUF2_FRAME_FRAME_WIDTH 32U
#define FX1_ISP_TUNE_CH1_BUF2_FRAME_FRAME_MASK 0xFFFFFFFFU
#define FX1_ISP_TUNE_CH1_BUF2_FRAME_FRAME_RESET 0x00000000U
#define FX1_ISP_TUNE_CH1_BUF2_FRAME_FRAME_ACCESS FX1_ISP_ACCESS_RO

/**
 * TUNE_CH1_BUF3_FRAME: địa chỉ lệch 0x8C5C, dòng bảng 1372, kiểu truy cập RO
 * ID frame đầu vào công bố cùng DONE thành công và giữ đến lần buffer đó hoàn tất thành công tiếp.
 * Recovery riêng DMA giữ thông tin buffer đã hoàn tất trước yêu cầu; common/hard reset xóa.
 */
#define FX1_ISP_TUNE_CH1_BUF3_FRAME_OFFSET 0x8C5CU
#define REG_TUNE_CH1_BUF3_FRAME FX1_ISP_TUNE_CH1_BUF3_FRAME_OFFSET
#define FX1_ISP_TUNE_CH1_BUF3_FRAME_RESET 0x00000000U
#define FX1_ISP_TUNE_CH1_BUF3_FRAME_ACCESS FX1_ISP_ACCESS_RO
#define FX1_ISP_TUNE_CH1_BUF3_FRAME_RO_MASK 0xFFFFFFFFU
#define FX1_ISP_TUNE_CH1_BUF3_FRAME_RW_MASK 0x00000000U
#define FX1_ISP_TUNE_CH1_BUF3_FRAME_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_CH1_BUF3_FRAME_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_CH1_BUF3_FRAME_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_CH1_BUF3_FRAME_W1SC_MASK 0x00000000U
/**
 * FRAME [31:0], dòng bảng 1373, kiểu truy cập RO
 * Số thứ tự frame đầu vào đã chốt, khác nhãn phần mềm tùy ý và các bộ đếm frame DMA.
 */
#define FX1_ISP_TUNE_CH1_BUF3_FRAME_FRAME_SHIFT 0U
#define FX1_ISP_TUNE_CH1_BUF3_FRAME_FRAME_WIDTH 32U
#define FX1_ISP_TUNE_CH1_BUF3_FRAME_FRAME_MASK 0xFFFFFFFFU
#define FX1_ISP_TUNE_CH1_BUF3_FRAME_FRAME_RESET 0x00000000U
#define FX1_ISP_TUNE_CH1_BUF3_FRAME_FRAME_ACCESS FX1_ISP_ACCESS_RO

/**
 * TUNE_CH2_BUF0_FRAME: địa chỉ lệch 0x8C60, dòng bảng 1374, kiểu truy cập RO
 * ID frame đầu vào công bố cùng DONE thành công và giữ đến lần buffer đó hoàn tất thành công tiếp.
 * Recovery riêng DMA giữ thông tin buffer đã hoàn tất trước yêu cầu; common/hard reset xóa.
 */
#define FX1_ISP_TUNE_CH2_BUF0_FRAME_OFFSET 0x8C60U
#define REG_TUNE_CH2_BUF0_FRAME FX1_ISP_TUNE_CH2_BUF0_FRAME_OFFSET
#define FX1_ISP_TUNE_CH2_BUF0_FRAME_RESET 0x00000000U
#define FX1_ISP_TUNE_CH2_BUF0_FRAME_ACCESS FX1_ISP_ACCESS_RO
#define FX1_ISP_TUNE_CH2_BUF0_FRAME_RO_MASK 0xFFFFFFFFU
#define FX1_ISP_TUNE_CH2_BUF0_FRAME_RW_MASK 0x00000000U
#define FX1_ISP_TUNE_CH2_BUF0_FRAME_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_CH2_BUF0_FRAME_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_CH2_BUF0_FRAME_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_CH2_BUF0_FRAME_W1SC_MASK 0x00000000U
/**
 * FRAME [31:0], dòng bảng 1375, kiểu truy cập RO
 * Số thứ tự frame đầu vào đã chốt, khác nhãn phần mềm tùy ý và các bộ đếm frame DMA.
 */
#define FX1_ISP_TUNE_CH2_BUF0_FRAME_FRAME_SHIFT 0U
#define FX1_ISP_TUNE_CH2_BUF0_FRAME_FRAME_WIDTH 32U
#define FX1_ISP_TUNE_CH2_BUF0_FRAME_FRAME_MASK 0xFFFFFFFFU
#define FX1_ISP_TUNE_CH2_BUF0_FRAME_FRAME_RESET 0x00000000U
#define FX1_ISP_TUNE_CH2_BUF0_FRAME_FRAME_ACCESS FX1_ISP_ACCESS_RO

/**
 * TUNE_CH2_BUF1_FRAME: địa chỉ lệch 0x8C64, dòng bảng 1376, kiểu truy cập RO
 * ID frame đầu vào công bố cùng DONE thành công và giữ đến lần buffer đó hoàn tất thành công tiếp.
 * Recovery riêng DMA giữ thông tin buffer đã hoàn tất trước yêu cầu; common/hard reset xóa.
 */
#define FX1_ISP_TUNE_CH2_BUF1_FRAME_OFFSET 0x8C64U
#define REG_TUNE_CH2_BUF1_FRAME FX1_ISP_TUNE_CH2_BUF1_FRAME_OFFSET
#define FX1_ISP_TUNE_CH2_BUF1_FRAME_RESET 0x00000000U
#define FX1_ISP_TUNE_CH2_BUF1_FRAME_ACCESS FX1_ISP_ACCESS_RO
#define FX1_ISP_TUNE_CH2_BUF1_FRAME_RO_MASK 0xFFFFFFFFU
#define FX1_ISP_TUNE_CH2_BUF1_FRAME_RW_MASK 0x00000000U
#define FX1_ISP_TUNE_CH2_BUF1_FRAME_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_CH2_BUF1_FRAME_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_CH2_BUF1_FRAME_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_CH2_BUF1_FRAME_W1SC_MASK 0x00000000U
/**
 * FRAME [31:0], dòng bảng 1377, kiểu truy cập RO
 * Số thứ tự frame đầu vào đã chốt, khác nhãn phần mềm tùy ý và các bộ đếm frame DMA.
 */
#define FX1_ISP_TUNE_CH2_BUF1_FRAME_FRAME_SHIFT 0U
#define FX1_ISP_TUNE_CH2_BUF1_FRAME_FRAME_WIDTH 32U
#define FX1_ISP_TUNE_CH2_BUF1_FRAME_FRAME_MASK 0xFFFFFFFFU
#define FX1_ISP_TUNE_CH2_BUF1_FRAME_FRAME_RESET 0x00000000U
#define FX1_ISP_TUNE_CH2_BUF1_FRAME_FRAME_ACCESS FX1_ISP_ACCESS_RO

/**
 * TUNE_CH2_BUF2_FRAME: địa chỉ lệch 0x8C68, dòng bảng 1378, kiểu truy cập RO
 * ID frame đầu vào công bố cùng DONE thành công và giữ đến lần buffer đó hoàn tất thành công tiếp.
 * Recovery riêng DMA giữ thông tin buffer đã hoàn tất trước yêu cầu; common/hard reset xóa.
 */
#define FX1_ISP_TUNE_CH2_BUF2_FRAME_OFFSET 0x8C68U
#define REG_TUNE_CH2_BUF2_FRAME FX1_ISP_TUNE_CH2_BUF2_FRAME_OFFSET
#define FX1_ISP_TUNE_CH2_BUF2_FRAME_RESET 0x00000000U
#define FX1_ISP_TUNE_CH2_BUF2_FRAME_ACCESS FX1_ISP_ACCESS_RO
#define FX1_ISP_TUNE_CH2_BUF2_FRAME_RO_MASK 0xFFFFFFFFU
#define FX1_ISP_TUNE_CH2_BUF2_FRAME_RW_MASK 0x00000000U
#define FX1_ISP_TUNE_CH2_BUF2_FRAME_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_CH2_BUF2_FRAME_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_CH2_BUF2_FRAME_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_CH2_BUF2_FRAME_W1SC_MASK 0x00000000U
/**
 * FRAME [31:0], dòng bảng 1379, kiểu truy cập RO
 * Số thứ tự frame đầu vào đã chốt, khác nhãn phần mềm tùy ý và các bộ đếm frame DMA.
 */
#define FX1_ISP_TUNE_CH2_BUF2_FRAME_FRAME_SHIFT 0U
#define FX1_ISP_TUNE_CH2_BUF2_FRAME_FRAME_WIDTH 32U
#define FX1_ISP_TUNE_CH2_BUF2_FRAME_FRAME_MASK 0xFFFFFFFFU
#define FX1_ISP_TUNE_CH2_BUF2_FRAME_FRAME_RESET 0x00000000U
#define FX1_ISP_TUNE_CH2_BUF2_FRAME_FRAME_ACCESS FX1_ISP_ACCESS_RO

/**
 * TUNE_CH2_BUF3_FRAME: địa chỉ lệch 0x8C6C, dòng bảng 1380, kiểu truy cập RO
 * ID frame đầu vào công bố cùng DONE thành công và giữ đến lần buffer đó hoàn tất thành công tiếp.
 * Recovery riêng DMA giữ thông tin buffer đã hoàn tất trước yêu cầu; common/hard reset xóa.
 */
#define FX1_ISP_TUNE_CH2_BUF3_FRAME_OFFSET 0x8C6CU
#define REG_TUNE_CH2_BUF3_FRAME FX1_ISP_TUNE_CH2_BUF3_FRAME_OFFSET
#define FX1_ISP_TUNE_CH2_BUF3_FRAME_RESET 0x00000000U
#define FX1_ISP_TUNE_CH2_BUF3_FRAME_ACCESS FX1_ISP_ACCESS_RO
#define FX1_ISP_TUNE_CH2_BUF3_FRAME_RO_MASK 0xFFFFFFFFU
#define FX1_ISP_TUNE_CH2_BUF3_FRAME_RW_MASK 0x00000000U
#define FX1_ISP_TUNE_CH2_BUF3_FRAME_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_CH2_BUF3_FRAME_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_CH2_BUF3_FRAME_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_CH2_BUF3_FRAME_W1SC_MASK 0x00000000U
/**
 * FRAME [31:0], dòng bảng 1381, kiểu truy cập RO
 * Số thứ tự frame đầu vào đã chốt, khác nhãn phần mềm tùy ý và các bộ đếm frame DMA.
 */
#define FX1_ISP_TUNE_CH2_BUF3_FRAME_FRAME_SHIFT 0U
#define FX1_ISP_TUNE_CH2_BUF3_FRAME_FRAME_WIDTH 32U
#define FX1_ISP_TUNE_CH2_BUF3_FRAME_FRAME_MASK 0xFFFFFFFFU
#define FX1_ISP_TUNE_CH2_BUF3_FRAME_FRAME_RESET 0x00000000U
#define FX1_ISP_TUNE_CH2_BUF3_FRAME_FRAME_ACCESS FX1_ISP_ACCESS_RO

/**
 * TUNE_AE_FRAME_SIZE: địa chỉ lệch 0x8C80, dòng bảng 1382, kiểu truy cập RO
 * Hình học frame thống kê đã acquire, bất biến khi giữ quyền. Chưa có quyền đọc 0 và đặt NO_LEASE.
 */
#define FX1_ISP_TUNE_AE_FRAME_SIZE_OFFSET 0x8C80U
#define REG_TUNE_AE_FRAME_SIZE FX1_ISP_TUNE_AE_FRAME_SIZE_OFFSET
#define FX1_ISP_TUNE_AE_FRAME_SIZE_RESET 0x00000000U
#define FX1_ISP_TUNE_AE_FRAME_SIZE_ACCESS FX1_ISP_ACCESS_RO
#define FX1_ISP_TUNE_AE_FRAME_SIZE_RO_MASK 0x1FFF1FFFU
#define FX1_ISP_TUNE_AE_FRAME_SIZE_RW_MASK 0x00000000U
#define FX1_ISP_TUNE_AE_FRAME_SIZE_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_AE_FRAME_SIZE_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_AE_FRAME_SIZE_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_AE_FRAME_SIZE_W1SC_MASK 0x00000000U
/**
 * WIDTH [12:0], dòng bảng 1383, kiểu truy cập RO
 * Hình học toàn ảnh hữu dụng của frame đã đo, độc lập cấu hình đang lập trình.
 */
#define FX1_ISP_TUNE_AE_FRAME_SIZE_WIDTH_SHIFT 0U
#define FX1_ISP_TUNE_AE_FRAME_SIZE_WIDTH_WIDTH 13U
#define FX1_ISP_TUNE_AE_FRAME_SIZE_WIDTH_MASK 0x00001FFFU
#define FX1_ISP_TUNE_AE_FRAME_SIZE_WIDTH_RESET 0x00000000U
#define FX1_ISP_TUNE_AE_FRAME_SIZE_WIDTH_ACCESS FX1_ISP_ACCESS_RO
/**
 * HEIGHT [28:16], dòng bảng 1384, kiểu truy cập RO
 * Hình học toàn ảnh hữu dụng của frame đã đo, độc lập cấu hình đang lập trình.
 */
#define FX1_ISP_TUNE_AE_FRAME_SIZE_HEIGHT_SHIFT 16U
#define FX1_ISP_TUNE_AE_FRAME_SIZE_HEIGHT_WIDTH 13U
#define FX1_ISP_TUNE_AE_FRAME_SIZE_HEIGHT_MASK 0x1FFF0000U
#define FX1_ISP_TUNE_AE_FRAME_SIZE_HEIGHT_RESET 0x00000000U
#define FX1_ISP_TUNE_AE_FRAME_SIZE_HEIGHT_ACCESS FX1_ISP_ACCESS_RO

/**
 * TUNE_AE_GRID: địa chỉ lệch 0x8C84, dòng bảng 1385, kiểu truy cập RO
 * Hình học frame thống kê đã acquire, bất biến khi giữ quyền. Chưa có quyền đọc 0 và đặt NO_LEASE.
 */
#define FX1_ISP_TUNE_AE_GRID_OFFSET 0x8C84U
#define REG_TUNE_AE_GRID FX1_ISP_TUNE_AE_GRID_OFFSET
#define FX1_ISP_TUNE_AE_GRID_RESET 0x00000000U
#define FX1_ISP_TUNE_AE_GRID_ACCESS FX1_ISP_ACCESS_RO
#define FX1_ISP_TUNE_AE_GRID_RO_MASK 0x001F003FU
#define FX1_ISP_TUNE_AE_GRID_RW_MASK 0x00000000U
#define FX1_ISP_TUNE_AE_GRID_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_AE_GRID_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_AE_GRID_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_AE_GRID_W1SC_MASK 0x00000000U
/**
 * NX [5:0], dòng bảng 1386, kiểu truy cập RO
 * Hình học toàn ảnh hữu dụng của frame đã đo, độc lập cấu hình đang lập trình.
 */
#define FX1_ISP_TUNE_AE_GRID_NX_SHIFT 0U
#define FX1_ISP_TUNE_AE_GRID_NX_WIDTH 6U
#define FX1_ISP_TUNE_AE_GRID_NX_MASK 0x0000003FU
#define FX1_ISP_TUNE_AE_GRID_NX_RESET 0x00000000U
#define FX1_ISP_TUNE_AE_GRID_NX_ACCESS FX1_ISP_ACCESS_RO
/**
 * NY [20:16], dòng bảng 1387, kiểu truy cập RO
 * Hình học toàn ảnh hữu dụng của frame đã đo, độc lập cấu hình đang lập trình.
 */
#define FX1_ISP_TUNE_AE_GRID_NY_SHIFT 16U
#define FX1_ISP_TUNE_AE_GRID_NY_WIDTH 5U
#define FX1_ISP_TUNE_AE_GRID_NY_MASK 0x001F0000U
#define FX1_ISP_TUNE_AE_GRID_NY_RESET 0x00000000U
#define FX1_ISP_TUNE_AE_GRID_NY_ACCESS FX1_ISP_ACCESS_RO

/**
 * TUNE_AE_ZONE_SIZE: địa chỉ lệch 0x8C88, dòng bảng 1388, kiểu truy cập RO
 * Hình học frame thống kê đã acquire, bất biến khi giữ quyền. Chưa có quyền đọc 0 và đặt NO_LEASE.
 */
#define FX1_ISP_TUNE_AE_ZONE_SIZE_OFFSET 0x8C88U
#define REG_TUNE_AE_ZONE_SIZE FX1_ISP_TUNE_AE_ZONE_SIZE_OFFSET
#define FX1_ISP_TUNE_AE_ZONE_SIZE_RESET 0x00000000U
#define FX1_ISP_TUNE_AE_ZONE_SIZE_ACCESS FX1_ISP_ACCESS_RO
#define FX1_ISP_TUNE_AE_ZONE_SIZE_RO_MASK 0xFFFFFFFFU
#define FX1_ISP_TUNE_AE_ZONE_SIZE_RW_MASK 0x00000000U
#define FX1_ISP_TUNE_AE_ZONE_SIZE_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_AE_ZONE_SIZE_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_AE_ZONE_SIZE_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_AE_ZONE_SIZE_W1SC_MASK 0x00000000U
/**
 * WIDTH [15:0], dòng bảng 1389, kiểu truy cập RO
 * Hình học toàn ảnh hữu dụng của frame đã đo, độc lập cấu hình đang lập trình.
 */
#define FX1_ISP_TUNE_AE_ZONE_SIZE_WIDTH_SHIFT 0U
#define FX1_ISP_TUNE_AE_ZONE_SIZE_WIDTH_WIDTH 16U
#define FX1_ISP_TUNE_AE_ZONE_SIZE_WIDTH_MASK 0x0000FFFFU
#define FX1_ISP_TUNE_AE_ZONE_SIZE_WIDTH_RESET 0x00000000U
#define FX1_ISP_TUNE_AE_ZONE_SIZE_WIDTH_ACCESS FX1_ISP_ACCESS_RO
/**
 * HEIGHT [31:16], dòng bảng 1390, kiểu truy cập RO
 * Hình học toàn ảnh hữu dụng của frame đã đo, độc lập cấu hình đang lập trình.
 */
#define FX1_ISP_TUNE_AE_ZONE_SIZE_HEIGHT_SHIFT 16U
#define FX1_ISP_TUNE_AE_ZONE_SIZE_HEIGHT_WIDTH 16U
#define FX1_ISP_TUNE_AE_ZONE_SIZE_HEIGHT_MASK 0xFFFF0000U
#define FX1_ISP_TUNE_AE_ZONE_SIZE_HEIGHT_RESET 0x00000000U
#define FX1_ISP_TUNE_AE_ZONE_SIZE_HEIGHT_ACCESS FX1_ISP_ACCESS_RO

/**
 * TUNE_AWB_FRAME_SIZE: địa chỉ lệch 0x8C90, dòng bảng 1391, kiểu truy cập RO
 * Hình học frame thống kê đã acquire, bất biến khi giữ quyền; chưa có quyền đọc 0.
 */
#define FX1_ISP_TUNE_AWB_FRAME_SIZE_OFFSET 0x8C90U
#define REG_TUNE_AWB_FRAME_SIZE FX1_ISP_TUNE_AWB_FRAME_SIZE_OFFSET
#define FX1_ISP_TUNE_AWB_FRAME_SIZE_RESET 0x00000000U
#define FX1_ISP_TUNE_AWB_FRAME_SIZE_ACCESS FX1_ISP_ACCESS_RO
#define FX1_ISP_TUNE_AWB_FRAME_SIZE_RO_MASK 0x1FFF1FFFU
#define FX1_ISP_TUNE_AWB_FRAME_SIZE_RW_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_FRAME_SIZE_W1C_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_FRAME_SIZE_W1S_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_FRAME_SIZE_W1P_MASK 0x00000000U
#define FX1_ISP_TUNE_AWB_FRAME_SIZE_W1SC_MASK 0x00000000U
/**
 * WIDTH [12:0], dòng bảng 1392, kiểu truy cập RO
 * Hình học toàn ảnh hữu dụng của frame đã đo, độc lập cấu hình đang lập trình.
 */
#define FX1_ISP_TUNE_AWB_FRAME_SIZE_WIDTH_SHIFT 0U
#define FX1_ISP_TUNE_AWB_FRAME_SIZE_WIDTH_WIDTH 13U
#define FX1_ISP_TUNE_AWB_FRAME_SIZE_WIDTH_MASK 0x00001FFFU
#define FX1_ISP_TUNE_AWB_FRAME_SIZE_WIDTH_RESET 0x00000000U
#define FX1_ISP_TUNE_AWB_FRAME_SIZE_WIDTH_ACCESS FX1_ISP_ACCESS_RO
/**
 * HEIGHT [28:16], dòng bảng 1393, kiểu truy cập RO
 * Hình học toàn ảnh hữu dụng của frame đã đo, độc lập cấu hình đang lập trình.
 */
#define FX1_ISP_TUNE_AWB_FRAME_SIZE_HEIGHT_SHIFT 16U
#define FX1_ISP_TUNE_AWB_FRAME_SIZE_HEIGHT_WIDTH 13U
#define FX1_ISP_TUNE_AWB_FRAME_SIZE_HEIGHT_MASK 0x1FFF0000U
#define FX1_ISP_TUNE_AWB_FRAME_SIZE_HEIGHT_RESET 0x00000000U
#define FX1_ISP_TUNE_AWB_FRAME_SIZE_HEIGHT_ACCESS FX1_ISP_ACCESS_RO

/* Cấu trúc ánh xạ bộ nhớ; mọi khoảng không được đặc tả đều được dành rõ là dự phòng. */
typedef struct FX1_ISP_Regs {
    volatile const uint32_t COMMON_VER_DATE; /* 0x0000, giá trị sau reset 0x18102026 */
    volatile const uint32_t COMMON_VER_ID; /* 0x0004, giá trị sau reset 0x00010007 */
    volatile uint32_t COMMON_CTRL; /* 0x0008, giá trị sau reset 0x00000000 */
    volatile const uint32_t COMMON_STATUS; /* 0x000C, giá trị sau reset 0x00000000 */
    volatile uint32_t COMMON_IRQ_STATUS; /* 0x0010, giá trị sau reset 0x00000000 */
    volatile uint32_t COMMON_IRQ_EN; /* 0x0014, giá trị sau reset 0x00000000 */
    volatile uint32_t COMMON_FRAME_WIDTH; /* 0x0018, giá trị sau reset 0x00000780 */
    volatile uint32_t COMMON_FRAME_HEIGHT; /* 0x001C, giá trị sau reset 0x00000438 */
    volatile uint32_t COMMON_BAYER; /* 0x0020, giá trị sau reset 0x00000000 */
    volatile uint32_t _reserved_0024[1]; /* Khoảng địa chỉ dự phòng 0x0024..0x0024 */
    volatile uint32_t COMMON_SCRATCH; /* 0x0028, giá trị sau reset 0x00000000 */
    volatile uint32_t _reserved_002C[117]; /* Khoảng địa chỉ dự phòng 0x002C..0x01FC */
    volatile uint32_t BLC_CTRL; /* 0x0200, giá trị sau reset 0x00000000 */
    volatile uint32_t BLC_GAIN_SEL; /* 0x0204, giá trị sau reset 0x00000100 */
    volatile uint32_t BLC_OFS_G0_DFT; /* 0x0208, giá trị sau reset 0x000000C8 */
    volatile uint32_t BLC_OFS_G0_R; /* 0x020C, giá trị sau reset 0x00000000 */
    volatile uint32_t BLC_OFS_G0_GR; /* 0x0210, giá trị sau reset 0x00000000 */
    volatile uint32_t BLC_OFS_G0_GB; /* 0x0214, giá trị sau reset 0x00000000 */
    volatile uint32_t BLC_OFS_G0_B; /* 0x0218, giá trị sau reset 0x00000000 */
    volatile uint32_t BLC_OFS_G1_DFT; /* 0x021C, giá trị sau reset 0x000000C8 */
    volatile uint32_t BLC_OFS_G1_R; /* 0x0220, giá trị sau reset 0x00000000 */
    volatile uint32_t BLC_OFS_G1_GR; /* 0x0224, giá trị sau reset 0x00000000 */
    volatile uint32_t BLC_OFS_G1_GB; /* 0x0228, giá trị sau reset 0x00000000 */
    volatile uint32_t BLC_OFS_G1_B; /* 0x022C, giá trị sau reset 0x00000000 */
    volatile uint32_t BLC_OFS_G2_DFT; /* 0x0230, giá trị sau reset 0x000000C8 */
    volatile uint32_t BLC_OFS_G2_R; /* 0x0234, giá trị sau reset 0x00000000 */
    volatile uint32_t BLC_OFS_G2_GR; /* 0x0238, giá trị sau reset 0x00000000 */
    volatile uint32_t BLC_OFS_G2_GB; /* 0x023C, giá trị sau reset 0x00000000 */
    volatile uint32_t BLC_OFS_G2_B; /* 0x0240, giá trị sau reset 0x00000000 */
    volatile uint32_t BLC_OFS_G3_DFT; /* 0x0244, giá trị sau reset 0x000000C8 */
    volatile uint32_t BLC_OFS_G3_R; /* 0x0248, giá trị sau reset 0x00000000 */
    volatile uint32_t BLC_OFS_G3_GR; /* 0x024C, giá trị sau reset 0x00000000 */
    volatile uint32_t BLC_OFS_G3_GB; /* 0x0250, giá trị sau reset 0x00000000 */
    volatile uint32_t BLC_OFS_G3_B; /* 0x0254, giá trị sau reset 0x00000000 */
    volatile uint32_t BLC_SCALE_G0; /* 0x0258, giá trị sau reset 0x00010000 */
    volatile uint32_t BLC_SCALE_G1; /* 0x025C, giá trị sau reset 0x00010000 */
    volatile uint32_t BLC_SCALE_G2; /* 0x0260, giá trị sau reset 0x00010000 */
    volatile uint32_t BLC_SCALE_G3; /* 0x0264, giá trị sau reset 0x00010000 */
    volatile uint32_t _reserved_0268[102]; /* Khoảng địa chỉ dự phòng 0x0268..0x03FC */
    volatile uint32_t LSC_CTRL; /* 0x0400, giá trị sau reset 0x00000000 */
    volatile uint32_t LSC_MESH_NODES; /* 0x0404, giá trị sau reset 0x00000000 */
    volatile uint32_t LSC_STRENGTH; /* 0x0408, giá trị sau reset 0x00000000 */
    volatile uint32_t LSC_PROFILE_SEL; /* 0x040C, giá trị sau reset 0x00000000 */
    volatile uint32_t LSC_LOAD_CTRL; /* 0x0410, giá trị sau reset 0x00000000 */
    volatile uint32_t LSC_COEF_DATA; /* 0x0414, giá trị sau reset 0x00000000 */
    volatile const uint32_t LSC_LOAD_STATUS; /* 0x0418, giá trị sau reset 0x00000000 */
    volatile const uint32_t LSC_PROFILE_STATUS; /* 0x041C, giá trị sau reset 0x00000000 */
    volatile uint32_t LSC_ERROR; /* 0x0420, giá trị sau reset 0x00000000 */
    volatile const uint32_t LSC_STAT_OVF_CNT_R; /* 0x0424, giá trị sau reset 0x00000000 */
    volatile const uint32_t LSC_STAT_OVF_CNT_GR; /* 0x0428, giá trị sau reset 0x00000000 */
    volatile const uint32_t LSC_STAT_OVF_CNT_GB; /* 0x042C, giá trị sau reset 0x00000000 */
    volatile const uint32_t LSC_STAT_OVF_CNT_B; /* 0x0430, giá trị sau reset 0x00000000 */
    volatile uint32_t _reserved_0434[115]; /* Khoảng địa chỉ dự phòng 0x0434..0x05FC */
    volatile uint32_t BPC_CTRL; /* 0x0600, giá trị sau reset 0x00000000 */
    volatile uint32_t BPC_MODE; /* 0x0604, giá trị sau reset 0x00000001 */
    volatile uint32_t BPC_TEMPORAL_VAR; /* 0x0608, giá trị sau reset 0x00000100 */
    volatile uint32_t BPC_THRESH; /* 0x060C, giá trị sau reset 0x004D0192 */
    volatile uint32_t BPC_PIXEL_AGE; /* 0x0610, giá trị sau reset 0x000004B0 */
    volatile const uint32_t BPC_NUM_CANDIDATES; /* 0x0614, giá trị sau reset 0x00000000 */
    volatile const uint32_t BPC_NUM_DEFECTIVE; /* 0x0618, giá trị sau reset 0x00000000 */
    volatile uint32_t BPC_STATUS; /* 0x061C, giá trị sau reset 0x00000000 */
    volatile uint32_t _reserved_0620[120]; /* Khoảng địa chỉ dự phòng 0x0620..0x07FC */
    volatile uint32_t WB_CTRL; /* 0x0800, giá trị sau reset 0x00000000 */
    volatile uint32_t WB_GAIN_R; /* 0x0804, giá trị sau reset 0x00000100 */
    volatile uint32_t WB_GAIN_G; /* 0x0808, giá trị sau reset 0x00000100 */
    volatile uint32_t WB_GAIN_B; /* 0x080C, giá trị sau reset 0x00000100 */
    volatile uint32_t _reserved_0810[124]; /* Khoảng địa chỉ dự phòng 0x0810..0x09FC */
    volatile uint32_t DG_CTRL; /* 0x0A00, giá trị sau reset 0x00000000 */
    volatile uint32_t DG_GAIN; /* 0x0A04, giá trị sau reset 0x00000100 */
    volatile uint32_t _reserved_0A08[126]; /* Khoảng địa chỉ dự phòng 0x0A08..0x0BFC */
    volatile uint32_t D_WDR_CTRL; /* 0x0C00, giá trị sau reset 0x00000000 */
    volatile uint32_t D_WDR_STRENGTH; /* 0x0C04, giá trị sau reset 0x00000000 */
    volatile uint32_t _reserved_0C08[126]; /* Khoảng địa chỉ dự phòng 0x0C08..0x0DFC */
    volatile const uint32_t DEMOSAIC_STATUS; /* 0x0E00, giá trị sau reset 0x00000000 */
    volatile uint32_t _reserved_0E04[127]; /* Khoảng địa chỉ dự phòng 0x0E04..0x0FFC */
    volatile uint32_t CCM_CTRL; /* 0x1000, giá trị sau reset 0x00000000 */
    volatile uint32_t CCM_CRR; /* 0x1004, giá trị sau reset 0x00000200 */
    volatile uint32_t CCM_CRG; /* 0x1008, giá trị sau reset 0x00000000 */
    volatile uint32_t CCM_CRB; /* 0x100C, giá trị sau reset 0x00000000 */
    volatile uint32_t CCM_CGR; /* 0x1010, giá trị sau reset 0x00000000 */
    volatile uint32_t CCM_CGG; /* 0x1014, giá trị sau reset 0x00000200 */
    volatile uint32_t CCM_CGB; /* 0x1018, giá trị sau reset 0x00000000 */
    volatile uint32_t CCM_CBR; /* 0x101C, giá trị sau reset 0x00000000 */
    volatile uint32_t CCM_CBG; /* 0x1020, giá trị sau reset 0x00000000 */
    volatile uint32_t CCM_CBB; /* 0x1024, giá trị sau reset 0x00000200 */
    volatile uint32_t CCM_OFS_R; /* 0x1028, giá trị sau reset 0x00000000 */
    volatile uint32_t CCM_OFS_G; /* 0x102C, giá trị sau reset 0x00000000 */
    volatile uint32_t CCM_OFS_B; /* 0x1030, giá trị sau reset 0x00000000 */
    volatile const uint32_t CCM_STATUS; /* 0x1034, giá trị sau reset 0x00000000 */
    volatile uint32_t _reserved_1038[114]; /* Khoảng địa chỉ dự phòng 0x1038..0x11FC */
    volatile uint32_t GAMMA_CTRL; /* 0x1200, giá trị sau reset 0x00000000 */
    volatile uint32_t GAMMA_LUT_ADDR; /* 0x1204, giá trị sau reset 0x00000000 */
    volatile uint32_t GAMMA_LUT_DATA; /* 0x1208, giá trị sau reset 0x00000000 */
    volatile const uint32_t GAMMA_LUT_RDATA; /* 0x120C, giá trị sau reset 0x00000000 */
    volatile const uint32_t GAMMA_STATUS; /* 0x1210, giá trị sau reset 0x00000000 */
    volatile uint32_t _reserved_1214[123]; /* Khoảng địa chỉ dự phòng 0x1214..0x13FC */
    volatile uint32_t CSC_CTRL; /* 0x1400, giá trị sau reset 0x00000000 */
    volatile uint32_t _reserved_1404[127]; /* Khoảng địa chỉ dự phòng 0x1404..0x15FC */
    volatile uint32_t GTM_CTRL; /* 0x1600, giá trị sau reset 0x00000000 */
    volatile uint32_t GTM_LUT_ADDR; /* 0x1604, giá trị sau reset 0x00000000 */
    volatile uint32_t GTM_LUT_DATA; /* 0x1608, giá trị sau reset 0x00000000 */
    volatile uint32_t GTM_KEY; /* 0x160C, giá trị sau reset 0x0000002E */
    volatile uint32_t GTM_LWHITE; /* 0x1610, giá trị sau reset 0x00000400 */
    volatile uint32_t GTM_ROI_LOG2; /* 0x1614, giá trị sau reset 0x0000000B */
    volatile const uint32_t GTM_LUT_RDATA; /* 0x1618, giá trị sau reset 0x00000000 */
    volatile const uint32_t GTM_STATUS; /* 0x161C, giá trị sau reset 0x00000000 */
    volatile uint32_t _reserved_1620[120]; /* Khoảng địa chỉ dự phòng 0x1620..0x17FC */
    volatile uint32_t NR_2D_CTRL; /* 0x1800, giá trị sau reset 0x00000000 */
    volatile const uint32_t NR_2D_EST_VAR; /* 0x1804, giá trị sau reset 0x00000000 */
    volatile const uint32_t NR_2D_STATUS; /* 0x1808, giá trị sau reset 0x00000000 */
    volatile uint32_t _reserved_180C[125]; /* Khoảng địa chỉ dự phòng 0x180C..0x19FC */
    volatile uint32_t TNR_3D_CTRL; /* 0x1A00, giá trị sau reset 0x00000000 */
    volatile uint32_t TNR_3D_MTF_SEL; /* 0x1A04, giá trị sau reset 0x00000000 */
    volatile uint32_t TNR_3D_HIST_BASE; /* 0x1A08, giá trị sau reset 0x00000000 */
    volatile uint32_t _reserved_1A0C[125]; /* Khoảng địa chỉ dự phòng 0x1A0C..0x1BFC */
    volatile uint32_t EE_CTRL; /* 0x1C00, giá trị sau reset 0x00000000 */
    volatile uint32_t EE_ALPHA; /* 0x1C04, giá trị sau reset 0x00004000 */
    volatile uint32_t EE_BETA; /* 0x1C08, giá trị sau reset 0x00002000 */
    volatile uint32_t EE_CLAMP; /* 0x1C0C, giá trị sau reset 0x00001810 */
    volatile uint32_t EE_FEATURE_EN; /* 0x1C10, giá trị sau reset 0x00000000 */
    volatile uint32_t EE_RADIAL_CENTER; /* 0x1C14, giá trị sau reset 0x00000000 */
    volatile uint32_t EE_RADIAL_R2_TH0; /* 0x1C18, giá trị sau reset 0x00000000 */
    volatile uint32_t EE_RADIAL_R2_TH1; /* 0x1C1C, giá trị sau reset 0x00000000 */
    volatile uint32_t EE_RADIAL_R2_TH2; /* 0x1C20, giá trị sau reset 0x00000000 */
    volatile uint32_t EE_RADIAL_GAIN_01; /* 0x1C24, giá trị sau reset 0x80008000 */
    volatile uint32_t EE_RADIAL_GAIN_23; /* 0x1C28, giá trị sau reset 0x80008000 */
    volatile uint32_t EE_LUT_CTRL; /* 0x1C2C, giá trị sau reset 0x00000000 */
    volatile uint32_t EE_LUT_ADDR; /* 0x1C30, giá trị sau reset 0x00000000 */
    volatile uint32_t EE_LUT_WDATA; /* 0x1C34, giá trị sau reset 0x00000000 */
    volatile uint32_t EE_STATUS; /* 0x1C38, giá trị sau reset 0x00000000 */
    volatile const uint32_t EE_LUT_RDATA; /* 0x1C3C, giá trị sau reset 0x00000000 */
    volatile uint32_t _reserved_1C40[112]; /* Khoảng địa chỉ dự phòng 0x1C40..0x1DFC */
    volatile uint32_t CNF_CTRL; /* 0x1E00, giá trị sau reset 0x00000000 */
    volatile uint32_t CNF_CHROMA_TH; /* 0x1E04, giá trị sau reset 0x00000000 */
    volatile uint32_t CNF_LUMA_TH; /* 0x1E08, giá trị sau reset 0x00000000 */
    volatile uint32_t CNF_STRENGTH; /* 0x1E0C, giá trị sau reset 0x00000000 */
    volatile const uint32_t CNF_STATUS; /* 0x1E10, giá trị sau reset 0x00000000 */
    volatile uint32_t _reserved_1E14[123]; /* Khoảng địa chỉ dự phòng 0x1E14..0x1FFC */
    volatile uint32_t RESIZER_CTRL; /* 0x2000, giá trị sau reset 0x00000000 */
    volatile const uint32_t RESIZER_CUSTOM_W; /* 0x2004, giá trị sau reset 0x00000000 */
    volatile const uint32_t RESIZER_CUSTOM_H; /* 0x2008, giá trị sau reset 0x00000000 */
    volatile uint32_t _reserved_200C[2]; /* Khoảng địa chỉ dự phòng 0x200C..0x2010 */
    volatile const uint32_t RESIZER_OUT_W; /* 0x2014, giá trị sau reset 0x00000000 */
    volatile const uint32_t RESIZER_OUT_H; /* 0x2018, giá trị sau reset 0x00000000 */
    volatile const uint32_t RESIZER_STATUS; /* 0x201C, giá trị sau reset 0x00000000 */
    volatile uint32_t OUTPUT_ENABLE; /* 0x2020, giá trị sau reset 0x00000001 */
    volatile uint32_t OUTPUT_IRQ_EN; /* 0x2024, giá trị sau reset 0x00000000 */
    volatile uint32_t OUTPUT_IRQ_STATUS; /* 0x2028, giá trị sau reset 0x00000000 */
    volatile uint32_t _reserved_202C[117]; /* Khoảng địa chỉ dự phòng 0x202C..0x21FC */
    volatile uint32_t AEC_CTRL; /* 0x2200, giá trị sau reset 0x00000000 */
    volatile uint32_t AEC_CONTEXT_ID; /* 0x2204, giá trị sau reset 0x00000000 */
    volatile uint32_t AEC_ZONE_CFG; /* 0x2208, giá trị sau reset 0x00001820 */
    volatile uint32_t AEC_ZONE_SIZE; /* 0x220C, giá trị sau reset 0x005A0078 */
    volatile uint32_t AEC_SAMPLE_CLIP; /* 0x2210, giá trị sau reset 0x0FFF0000 */
    volatile uint32_t AEC_THRESH; /* 0x2214, giá trị sau reset 0x0EA00110 */
    volatile uint32_t AEC_STATUS; /* 0x2218, giá trị sau reset 0x00000000 */
    volatile const uint32_t AEC_FRAME_ID; /* 0x221C, giá trị sau reset 0x00000000 */
    volatile const uint32_t AEC_RESULT_CONTEXT_ID; /* 0x2220, giá trị sau reset 0x00000000 */
    volatile uint32_t AEC_CHANNEL_SEL; /* 0x2224, giá trị sau reset 0x00000000 */
    volatile const uint32_t AEC_GLOBAL_SUM_LO; /* 0x2228, giá trị sau reset 0x00000000 */
    volatile const uint32_t AEC_GLOBAL_SUM_HI; /* 0x222C, giá trị sau reset 0x00000000 */
    volatile const uint32_t AEC_GLOBAL_COUNT; /* 0x2230, giá trị sau reset 0x00000000 */
    volatile uint32_t AEC_ZONE_ADDR; /* 0x2234, giá trị sau reset 0x00000000 */
    volatile const uint32_t AEC_ZONE_SUM; /* 0x2238, giá trị sau reset 0x00000000 */
    volatile const uint32_t AEC_ZONE_COUNT; /* 0x223C, giá trị sau reset 0x00000000 */
    volatile const uint32_t AEC_ZONE_GREEN_OE_UE; /* 0x2240, giá trị sau reset 0x00000000 */
    volatile const uint32_t AEC_ZONE_GREEN_MIN_MAX; /* 0x2244, giá trị sau reset 0x00000000 */
    volatile uint32_t AEC_HIST_ADDR; /* 0x2248, giá trị sau reset 0x00000000 */
    volatile const uint32_t AEC_HIST_DATA; /* 0x224C, giá trị sau reset 0x00000000 */
    volatile uint32_t _reserved_2250[108]; /* Khoảng địa chỉ dự phòng 0x2250..0x23FC */
    volatile uint32_t AWB_CTRL; /* 0x2400, giá trị sau reset 0x00000000 */
    volatile uint32_t AWB_X_BOUND_ADDR; /* 0x2404, giá trị sau reset 0x00000000 */
    volatile uint32_t AWB_Y_BOUND_ADDR; /* 0x2408, giá trị sau reset 0x00000000 */
    volatile uint32_t AWB_X_BOUND_DATA; /* 0x240C, giá trị sau reset 0x00000000 */
    volatile uint32_t AWB_Y_BOUND_DATA; /* 0x2410, giá trị sau reset 0x00000000 */
    volatile const uint32_t AWB_GLOBAL_SUM_R_H; /* 0x2414, giá trị sau reset 0x00000000 */
    volatile const uint32_t AWB_GLOBAL_SUM_R_L; /* 0x2418, giá trị sau reset 0x00000000 */
    volatile const uint32_t AWB_GLOBAL_SUM_G_H; /* 0x241C, giá trị sau reset 0x00000000 */
    volatile const uint32_t AWB_GLOBAL_SUM_G_L; /* 0x2420, giá trị sau reset 0x00000000 */
    volatile const uint32_t AWB_GLOBAL_SUM_B_H; /* 0x2424, giá trị sau reset 0x00000000 */
    volatile const uint32_t AWB_GLOBAL_SUM_B_L; /* 0x2428, giá trị sau reset 0x00000000 */
    volatile const uint32_t AWB_GLOBAL_COUNT; /* 0x242C, giá trị sau reset 0x00000000 */
    volatile uint32_t AWB_ZONE_ADDR; /* 0x2430, giá trị sau reset 0x00000000 */
    volatile const uint32_t AWB_ZONE_SUM_R_H; /* 0x2434, giá trị sau reset 0x00000000 */
    volatile const uint32_t AWB_ZONE_SUM_R_L; /* 0x2438, giá trị sau reset 0x00000000 */
    volatile const uint32_t AWB_ZONE_SUM_G_H; /* 0x243C, giá trị sau reset 0x00000000 */
    volatile const uint32_t AWB_ZONE_SUM_G_L; /* 0x2440, giá trị sau reset 0x00000000 */
    volatile const uint32_t AWB_ZONE_SUM_B_H; /* 0x2444, giá trị sau reset 0x00000000 */
    volatile const uint32_t AWB_ZONE_SUM_B_L; /* 0x2448, giá trị sau reset 0x00000000 */
    volatile const uint32_t AWB_ZONE_COUNT; /* 0x244C, giá trị sau reset 0x00000000 */
    volatile uint32_t AWB_STATUS; /* 0x2450, giá trị sau reset 0x00000000 */
    volatile uint32_t AWB_UNDEREXPOSED_LIMIT; /* 0x2454, giá trị sau reset 0x00000000 */
    volatile uint32_t AWB_SATURATION_LIMIT; /* 0x2458, giá trị sau reset 0x00000FFF */
    volatile uint32_t AWB_CONTEXT_ID; /* 0x245C, giá trị sau reset 0x00000000 */
    volatile const uint32_t AWB_RESULT_CONTEXT_ID; /* 0x2460, giá trị sau reset 0x00000000 */
    volatile const uint32_t AWB_FRAME_ID; /* 0x2464, giá trị sau reset 0x00000000 */
    volatile uint32_t AWB_RESULT_CTRL; /* 0x2468, giá trị sau reset 0x00000000 */
    volatile const uint32_t AWB_RESULT_STATUS; /* 0x246C, giá trị sau reset 0x00000000 */
    volatile const uint32_t AWB_DROP_COUNT; /* 0x2470, giá trị sau reset 0x00000000 */
    volatile uint32_t _reserved_2474[99]; /* Khoảng địa chỉ dự phòng 0x2474..0x25FC */
    volatile uint32_t AF_CTRL; /* 0x2600, giá trị sau reset 0x00000000 */
    volatile uint32_t AF_ZONE_EN; /* 0x2604, giá trị sau reset 0x0000FFFF */
    volatile uint32_t AF_METRIC_CFG; /* 0x2608, giá trị sau reset 0x00000000 */
    volatile uint32_t AF_STAT_ADDR; /* 0x260C, giá trị sau reset 0x00000000 */
    volatile const uint32_t AF_STAT_DATA; /* 0x2610, giá trị sau reset 0x00000000 */
    volatile uint32_t AF_LUT_ADDR; /* 0x2614, giá trị sau reset 0x00000000 */
    volatile uint32_t AF_LUT_DATA; /* 0x2618, giá trị sau reset 0x00000000 */
    volatile uint32_t AF_VCM_CUR_POS; /* 0x261C, giá trị sau reset 0x00000000 */
    volatile const uint32_t AF_VCM_BEST_POS; /* 0x2620, giá trị sau reset 0x00000000 */
    volatile const uint32_t AF_PEAK_SCORE; /* 0x2624, giá trị sau reset 0x00000000 */
    volatile uint32_t AF_FOCUS_RANGE; /* 0x2628, giá trị sau reset 0x00000000 */
    volatile uint32_t AF_STATUS; /* 0x262C, giá trị sau reset 0x00000000 */
    volatile uint32_t AF_CONTEXT_ID; /* 0x2630, giá trị sau reset 0x00000000 */
    volatile const uint32_t AF_RESULT_CONTEXT_ID; /* 0x2634, giá trị sau reset 0x00000000 */
    volatile const uint32_t AF_FRAME_ID; /* 0x2638, giá trị sau reset 0x00000000 */
    volatile uint32_t _reserved_263C[113]; /* Khoảng địa chỉ dự phòng 0x263C..0x27FC */
    volatile uint32_t OFMT_CTRL; /* 0x2800, giá trị sau reset 0x00000000 */
    volatile uint32_t OFMT_Y_STRIDE; /* 0x2804, giá trị sau reset 0x00000F00 */
    volatile uint32_t OFMT_UV_STRIDE; /* 0x2808, giá trị sau reset 0x00000F00 */
    volatile uint32_t _reserved_280C[509]; /* Khoảng địa chỉ dự phòng 0x280C..0x2FFC */
    volatile uint32_t DMA_CTRL; /* 0x3000, giá trị sau reset 0x00003F00 */
    volatile const uint32_t DMA_STAT; /* 0x3004, giá trị sau reset 0x00000000 */
    volatile uint32_t DMA_ERR; /* 0x3008, giá trị sau reset 0x00000000 */
    volatile uint32_t DMA_IRQ_EN; /* 0x300C, giá trị sau reset 0x00000000 */
    volatile uint32_t DMA_IRQ_STAT; /* 0x3010, giá trị sau reset 0x00000000 */
    volatile uint32_t _reserved_3014[3]; /* Khoảng địa chỉ dự phòng 0x3014..0x301C */
    volatile uint32_t IDMA_STRIDE; /* 0x3020, giá trị sau reset 0x00001E00 */
    volatile uint32_t IDMA_BUF_VALID; /* 0x3024, giá trị sau reset 0x00000000 */
    volatile const uint32_t IDMA_FRAME_COUNT; /* 0x3028, giá trị sau reset 0x00000000 */
    volatile uint32_t _reserved_302C[5]; /* Khoảng địa chỉ dự phòng 0x302C..0x303C */
    volatile uint32_t IDMA_BUF_ADDR_L0; /* 0x3040, giá trị sau reset 0x00000000 */
    volatile uint32_t IDMA_BUF_ADDR_H0; /* 0x3044, giá trị sau reset 0x00000000 */
    volatile uint32_t IDMA_BUF_ADDR_L1; /* 0x3048, giá trị sau reset 0x00000000 */
    volatile uint32_t IDMA_BUF_ADDR_H1; /* 0x304C, giá trị sau reset 0x00000000 */
    volatile uint32_t IDMA_BUF_ADDR_L2; /* 0x3050, giá trị sau reset 0x00000000 */
    volatile uint32_t IDMA_BUF_ADDR_H2; /* 0x3054, giá trị sau reset 0x00000000 */
    volatile uint32_t IDMA_BUF_ADDR_L3; /* 0x3058, giá trị sau reset 0x00000000 */
    volatile uint32_t IDMA_BUF_ADDR_H3; /* 0x305C, giá trị sau reset 0x00000000 */
    volatile uint32_t _reserved_3060[8]; /* Khoảng địa chỉ dự phòng 0x3060..0x307C */
    volatile uint32_t ODMA_Y_STRIDE; /* 0x3080, giá trị sau reset 0x00000F00 */
    volatile uint32_t ODMA_UV_STRIDE; /* 0x3084, giá trị sau reset 0x00000F00 */
    volatile uint32_t ODMA_BUF_FREE; /* 0x3088, giá trị sau reset 0x00000000 */
    volatile uint32_t ODMA_BUF_DONE; /* 0x308C, giá trị sau reset 0x00000000 */
    volatile const uint32_t ODMA_FRAME_COUNT; /* 0x3090, giá trị sau reset 0x00000000 */
    volatile uint32_t _reserved_3094[3]; /* Khoảng địa chỉ dự phòng 0x3094..0x309C */
    volatile uint32_t ODMA_Y_ADDR_L0; /* 0x30A0, giá trị sau reset 0x00000000 */
    volatile uint32_t ODMA_Y_ADDR_H0; /* 0x30A4, giá trị sau reset 0x00000000 */
    volatile uint32_t ODMA_UV_ADDR_L0; /* 0x30A8, giá trị sau reset 0x00000000 */
    volatile uint32_t ODMA_UV_ADDR_H0; /* 0x30AC, giá trị sau reset 0x00000000 */
    volatile uint32_t ODMA_Y_ADDR_L1; /* 0x30B0, giá trị sau reset 0x00000000 */
    volatile uint32_t ODMA_Y_ADDR_H1; /* 0x30B4, giá trị sau reset 0x00000000 */
    volatile uint32_t ODMA_UV_ADDR_L1; /* 0x30B8, giá trị sau reset 0x00000000 */
    volatile uint32_t ODMA_UV_ADDR_H1; /* 0x30BC, giá trị sau reset 0x00000000 */
    volatile uint32_t ODMA_Y_ADDR_L2; /* 0x30C0, giá trị sau reset 0x00000000 */
    volatile uint32_t ODMA_Y_ADDR_H2; /* 0x30C4, giá trị sau reset 0x00000000 */
    volatile uint32_t ODMA_UV_ADDR_L2; /* 0x30C8, giá trị sau reset 0x00000000 */
    volatile uint32_t ODMA_UV_ADDR_H2; /* 0x30CC, giá trị sau reset 0x00000000 */
    volatile uint32_t ODMA_Y_ADDR_L3; /* 0x30D0, giá trị sau reset 0x00000000 */
    volatile uint32_t ODMA_Y_ADDR_H3; /* 0x30D4, giá trị sau reset 0x00000000 */
    volatile uint32_t ODMA_UV_ADDR_L3; /* 0x30D8, giá trị sau reset 0x00000000 */
    volatile uint32_t ODMA_UV_ADDR_H3; /* 0x30DC, giá trị sau reset 0x00000000 */
    volatile uint32_t _reserved_30E0[8]; /* Khoảng địa chỉ dự phòng 0x30E0..0x30FC */
    volatile uint32_t IDMA_BUF_TAG0; /* 0x3100, giá trị sau reset 0x00000000 */
    volatile uint32_t IDMA_BUF_TAG1; /* 0x3104, giá trị sau reset 0x00000000 */
    volatile uint32_t IDMA_BUF_TAG2; /* 0x3108, giá trị sau reset 0x00000000 */
    volatile uint32_t IDMA_BUF_TAG3; /* 0x310C, giá trị sau reset 0x00000000 */
    volatile uint32_t IDMA_BUF_TS0; /* 0x3110, giá trị sau reset 0x00000000 */
    volatile uint32_t IDMA_BUF_TS1; /* 0x3114, giá trị sau reset 0x00000000 */
    volatile uint32_t IDMA_BUF_TS2; /* 0x3118, giá trị sau reset 0x00000000 */
    volatile uint32_t IDMA_BUF_TS3; /* 0x311C, giá trị sau reset 0x00000000 */
    volatile const uint32_t ODMA_BUF_TAG0; /* 0x3120, giá trị sau reset 0x00000000 */
    volatile const uint32_t ODMA_BUF_TAG1; /* 0x3124, giá trị sau reset 0x00000000 */
    volatile const uint32_t ODMA_BUF_TAG2; /* 0x3128, giá trị sau reset 0x00000000 */
    volatile const uint32_t ODMA_BUF_TAG3; /* 0x312C, giá trị sau reset 0x00000000 */
    volatile const uint32_t ODMA_BUF_TS0; /* 0x3130, giá trị sau reset 0x00000000 */
    volatile const uint32_t ODMA_BUF_TS1; /* 0x3134, giá trị sau reset 0x00000000 */
    volatile const uint32_t ODMA_BUF_TS2; /* 0x3138, giá trị sau reset 0x00000000 */
    volatile const uint32_t ODMA_BUF_TS3; /* 0x313C, giá trị sau reset 0x00000000 */
    volatile const uint32_t ODMA_BUF_SOF_TS0; /* 0x3140, giá trị sau reset 0x00000000 */
    volatile const uint32_t ODMA_BUF_SOF_TS1; /* 0x3144, giá trị sau reset 0x00000000 */
    volatile const uint32_t ODMA_BUF_SOF_TS2; /* 0x3148, giá trị sau reset 0x00000000 */
    volatile const uint32_t ODMA_BUF_SOF_TS3; /* 0x314C, giá trị sau reset 0x00000000 */
    volatile const uint32_t ODMA_BUF_EOF_TS0; /* 0x3150, giá trị sau reset 0x00000000 */
    volatile const uint32_t ODMA_BUF_EOF_TS1; /* 0x3154, giá trị sau reset 0x00000000 */
    volatile const uint32_t ODMA_BUF_EOF_TS2; /* 0x3158, giá trị sau reset 0x00000000 */
    volatile const uint32_t ODMA_BUF_EOF_TS3; /* 0x315C, giá trị sau reset 0x00000000 */
    volatile const uint32_t FRAME_COUNTER; /* 0x3160, giá trị sau reset 0x00000000 */
    volatile uint32_t _reserved_3164[39]; /* Khoảng địa chỉ dự phòng 0x3164..0x31FC */
    volatile uint32_t CH1_RESIZER_CTRL; /* 0x3200, giá trị sau reset 0x00000000 */
    volatile const uint32_t CH1_RESIZER_CUSTOM_W; /* 0x3204, giá trị sau reset 0x00000000 */
    volatile const uint32_t CH1_RESIZER_CUSTOM_H; /* 0x3208, giá trị sau reset 0x00000000 */
    volatile uint32_t _reserved_320C[2]; /* Khoảng địa chỉ dự phòng 0x320C..0x3210 */
    volatile const uint32_t CH1_RESIZER_OUT_W; /* 0x3214, giá trị sau reset 0x00000000 */
    volatile const uint32_t CH1_RESIZER_OUT_H; /* 0x3218, giá trị sau reset 0x00000000 */
    volatile const uint32_t CH1_RESIZER_STATUS; /* 0x321C, giá trị sau reset 0x00000000 */
    volatile uint32_t _reserved_3220[24]; /* Khoảng địa chỉ dự phòng 0x3220..0x327C */
    volatile uint32_t CH1_ODMA_Y_STRIDE; /* 0x3280, giá trị sau reset 0x00000F00 */
    volatile uint32_t CH1_ODMA_UV_STRIDE; /* 0x3284, giá trị sau reset 0x00000F00 */
    volatile uint32_t CH1_ODMA_BUF_FREE; /* 0x3288, giá trị sau reset 0x00000000 */
    volatile uint32_t CH1_ODMA_BUF_DONE; /* 0x328C, giá trị sau reset 0x00000000 */
    volatile const uint32_t CH1_ODMA_FRAME_COUNT; /* 0x3290, giá trị sau reset 0x00000000 */
    volatile uint32_t _reserved_3294[3]; /* Khoảng địa chỉ dự phòng 0x3294..0x329C */
    volatile uint32_t CH1_ODMA_Y_ADDR_L0; /* 0x32A0, giá trị sau reset 0x00000000 */
    volatile uint32_t CH1_ODMA_Y_ADDR_H0; /* 0x32A4, giá trị sau reset 0x00000000 */
    volatile uint32_t CH1_ODMA_UV_ADDR_L0; /* 0x32A8, giá trị sau reset 0x00000000 */
    volatile uint32_t CH1_ODMA_UV_ADDR_H0; /* 0x32AC, giá trị sau reset 0x00000000 */
    volatile uint32_t CH1_ODMA_Y_ADDR_L1; /* 0x32B0, giá trị sau reset 0x00000000 */
    volatile uint32_t CH1_ODMA_Y_ADDR_H1; /* 0x32B4, giá trị sau reset 0x00000000 */
    volatile uint32_t CH1_ODMA_UV_ADDR_L1; /* 0x32B8, giá trị sau reset 0x00000000 */
    volatile uint32_t CH1_ODMA_UV_ADDR_H1; /* 0x32BC, giá trị sau reset 0x00000000 */
    volatile uint32_t CH1_ODMA_Y_ADDR_L2; /* 0x32C0, giá trị sau reset 0x00000000 */
    volatile uint32_t CH1_ODMA_Y_ADDR_H2; /* 0x32C4, giá trị sau reset 0x00000000 */
    volatile uint32_t CH1_ODMA_UV_ADDR_L2; /* 0x32C8, giá trị sau reset 0x00000000 */
    volatile uint32_t CH1_ODMA_UV_ADDR_H2; /* 0x32CC, giá trị sau reset 0x00000000 */
    volatile uint32_t CH1_ODMA_Y_ADDR_L3; /* 0x32D0, giá trị sau reset 0x00000000 */
    volatile uint32_t CH1_ODMA_Y_ADDR_H3; /* 0x32D4, giá trị sau reset 0x00000000 */
    volatile uint32_t CH1_ODMA_UV_ADDR_L3; /* 0x32D8, giá trị sau reset 0x00000000 */
    volatile uint32_t CH1_ODMA_UV_ADDR_H3; /* 0x32DC, giá trị sau reset 0x00000000 */
    volatile uint32_t _reserved_32E0[8]; /* Khoảng địa chỉ dự phòng 0x32E0..0x32FC */
    volatile const uint32_t CH1_ODMA_BUF_TAG0; /* 0x3300, giá trị sau reset 0x00000000 */
    volatile const uint32_t CH1_ODMA_BUF_TAG1; /* 0x3304, giá trị sau reset 0x00000000 */
    volatile const uint32_t CH1_ODMA_BUF_TAG2; /* 0x3308, giá trị sau reset 0x00000000 */
    volatile const uint32_t CH1_ODMA_BUF_TAG3; /* 0x330C, giá trị sau reset 0x00000000 */
    volatile const uint32_t CH1_ODMA_BUF_TS0; /* 0x3310, giá trị sau reset 0x00000000 */
    volatile const uint32_t CH1_ODMA_BUF_TS1; /* 0x3314, giá trị sau reset 0x00000000 */
    volatile const uint32_t CH1_ODMA_BUF_TS2; /* 0x3318, giá trị sau reset 0x00000000 */
    volatile const uint32_t CH1_ODMA_BUF_TS3; /* 0x331C, giá trị sau reset 0x00000000 */
    volatile const uint32_t CH1_ODMA_BUF_SOF_TS0; /* 0x3320, giá trị sau reset 0x00000000 */
    volatile const uint32_t CH1_ODMA_BUF_SOF_TS1; /* 0x3324, giá trị sau reset 0x00000000 */
    volatile const uint32_t CH1_ODMA_BUF_SOF_TS2; /* 0x3328, giá trị sau reset 0x00000000 */
    volatile const uint32_t CH1_ODMA_BUF_SOF_TS3; /* 0x332C, giá trị sau reset 0x00000000 */
    volatile const uint32_t CH1_ODMA_BUF_EOF_TS0; /* 0x3330, giá trị sau reset 0x00000000 */
    volatile const uint32_t CH1_ODMA_BUF_EOF_TS1; /* 0x3334, giá trị sau reset 0x00000000 */
    volatile const uint32_t CH1_ODMA_BUF_EOF_TS2; /* 0x3338, giá trị sau reset 0x00000000 */
    volatile const uint32_t CH1_ODMA_BUF_EOF_TS3; /* 0x333C, giá trị sau reset 0x00000000 */
    volatile uint32_t _reserved_3340[48]; /* Khoảng địa chỉ dự phòng 0x3340..0x33FC */
    volatile uint32_t CH2_RESIZER_CTRL; /* 0x3400, giá trị sau reset 0x00000000 */
    volatile const uint32_t CH2_RESIZER_CUSTOM_W; /* 0x3404, giá trị sau reset 0x00000000 */
    volatile const uint32_t CH2_RESIZER_CUSTOM_H; /* 0x3408, giá trị sau reset 0x00000000 */
    volatile uint32_t _reserved_340C[2]; /* Khoảng địa chỉ dự phòng 0x340C..0x3410 */
    volatile const uint32_t CH2_RESIZER_OUT_W; /* 0x3414, giá trị sau reset 0x00000000 */
    volatile const uint32_t CH2_RESIZER_OUT_H; /* 0x3418, giá trị sau reset 0x00000000 */
    volatile const uint32_t CH2_RESIZER_STATUS; /* 0x341C, giá trị sau reset 0x00000000 */
    volatile uint32_t _reserved_3420[24]; /* Khoảng địa chỉ dự phòng 0x3420..0x347C */
    volatile uint32_t CH2_ODMA_Y_STRIDE; /* 0x3480, giá trị sau reset 0x00000F00 */
    volatile uint32_t _reserved_3484[1]; /* Khoảng địa chỉ dự phòng 0x3484..0x3484 */
    volatile uint32_t CH2_ODMA_BUF_FREE; /* 0x3488, giá trị sau reset 0x00000000 */
    volatile uint32_t CH2_ODMA_BUF_DONE; /* 0x348C, giá trị sau reset 0x00000000 */
    volatile const uint32_t CH2_ODMA_FRAME_COUNT; /* 0x3490, giá trị sau reset 0x00000000 */
    volatile uint32_t _reserved_3494[3]; /* Khoảng địa chỉ dự phòng 0x3494..0x349C */
    volatile uint32_t CH2_ODMA_Y_ADDR_L0; /* 0x34A0, giá trị sau reset 0x00000000 */
    volatile uint32_t CH2_ODMA_Y_ADDR_H0; /* 0x34A4, giá trị sau reset 0x00000000 */
    volatile uint32_t _reserved_34A8[2]; /* Khoảng địa chỉ dự phòng 0x34A8..0x34AC */
    volatile uint32_t CH2_ODMA_Y_ADDR_L1; /* 0x34B0, giá trị sau reset 0x00000000 */
    volatile uint32_t CH2_ODMA_Y_ADDR_H1; /* 0x34B4, giá trị sau reset 0x00000000 */
    volatile uint32_t _reserved_34B8[2]; /* Khoảng địa chỉ dự phòng 0x34B8..0x34BC */
    volatile uint32_t CH2_ODMA_Y_ADDR_L2; /* 0x34C0, giá trị sau reset 0x00000000 */
    volatile uint32_t CH2_ODMA_Y_ADDR_H2; /* 0x34C4, giá trị sau reset 0x00000000 */
    volatile uint32_t _reserved_34C8[2]; /* Khoảng địa chỉ dự phòng 0x34C8..0x34CC */
    volatile uint32_t CH2_ODMA_Y_ADDR_L3; /* 0x34D0, giá trị sau reset 0x00000000 */
    volatile uint32_t CH2_ODMA_Y_ADDR_H3; /* 0x34D4, giá trị sau reset 0x00000000 */
    volatile uint32_t _reserved_34D8[10]; /* Khoảng địa chỉ dự phòng 0x34D8..0x34FC */
    volatile const uint32_t CH2_ODMA_BUF_TAG0; /* 0x3500, giá trị sau reset 0x00000000 */
    volatile const uint32_t CH2_ODMA_BUF_TAG1; /* 0x3504, giá trị sau reset 0x00000000 */
    volatile const uint32_t CH2_ODMA_BUF_TAG2; /* 0x3508, giá trị sau reset 0x00000000 */
    volatile const uint32_t CH2_ODMA_BUF_TAG3; /* 0x350C, giá trị sau reset 0x00000000 */
    volatile const uint32_t CH2_ODMA_BUF_TS0; /* 0x3510, giá trị sau reset 0x00000000 */
    volatile const uint32_t CH2_ODMA_BUF_TS1; /* 0x3514, giá trị sau reset 0x00000000 */
    volatile const uint32_t CH2_ODMA_BUF_TS2; /* 0x3518, giá trị sau reset 0x00000000 */
    volatile const uint32_t CH2_ODMA_BUF_TS3; /* 0x351C, giá trị sau reset 0x00000000 */
    volatile const uint32_t CH2_ODMA_BUF_SOF_TS0; /* 0x3520, giá trị sau reset 0x00000000 */
    volatile const uint32_t CH2_ODMA_BUF_SOF_TS1; /* 0x3524, giá trị sau reset 0x00000000 */
    volatile const uint32_t CH2_ODMA_BUF_SOF_TS2; /* 0x3528, giá trị sau reset 0x00000000 */
    volatile const uint32_t CH2_ODMA_BUF_SOF_TS3; /* 0x352C, giá trị sau reset 0x00000000 */
    volatile const uint32_t CH2_ODMA_BUF_EOF_TS0; /* 0x3530, giá trị sau reset 0x00000000 */
    volatile const uint32_t CH2_ODMA_BUF_EOF_TS1; /* 0x3534, giá trị sau reset 0x00000000 */
    volatile const uint32_t CH2_ODMA_BUF_EOF_TS2; /* 0x3538, giá trị sau reset 0x00000000 */
    volatile const uint32_t CH2_ODMA_BUF_EOF_TS3; /* 0x353C, giá trị sau reset 0x00000000 */
    volatile uint32_t _reserved_3540[4784]; /* Khoảng địa chỉ dự phòng 0x3540..0x7FFC */
    volatile const uint32_t TUNE_ID; /* 0x8000, giá trị sau reset 0x54554E45 */
    volatile const uint32_t TUNE_REV; /* 0x8004, giá trị sau reset 0x00010007 */
    volatile const uint32_t TUNE_CAPS; /* 0x8008, giá trị sau reset 0x003FFFFF */
    volatile uint32_t TUNE_MODE; /* 0x800C, giá trị sau reset 0x00000000 */
    volatile uint32_t TUNE_BANK; /* 0x8010, giá trị sau reset 0x00000000 */
    volatile uint32_t TUNE_PROFILE_ID; /* 0x8014, giá trị sau reset 0x00000000 */
    volatile uint32_t TUNE_APPLY_FRAME; /* 0x8018, giá trị sau reset 0x00000000 */
    volatile uint32_t TUNE_CMD; /* 0x801C, giá trị sau reset 0x00000000 */
    volatile const uint32_t TUNE_STATUS; /* 0x8020, giá trị sau reset 0x00000000 */
    volatile const uint32_t TUNE_ACTIVE_ID; /* 0x8024, giá trị sau reset 0x00000000 */
    volatile const uint32_t TUNE_ACTIVE_FRAME; /* 0x8028, giá trị sau reset 0x00000000 */
    volatile uint32_t TUNE_ERROR; /* 0x802C, giá trị sau reset 0x00000000 */
    volatile const uint32_t TUNE_NEXT_FRAME; /* 0x8030, giá trị sau reset 0x00000001 */
    volatile uint32_t _reserved_8034[3]; /* Khoảng địa chỉ dự phòng 0x8034..0x803C */
    volatile const uint32_t TUNE_CH0_BUF0_PROFILE; /* 0x8040, giá trị sau reset 0x00000000 */
    volatile const uint32_t TUNE_CH0_BUF1_PROFILE; /* 0x8044, giá trị sau reset 0x00000000 */
    volatile const uint32_t TUNE_CH0_BUF2_PROFILE; /* 0x8048, giá trị sau reset 0x00000000 */
    volatile const uint32_t TUNE_CH0_BUF3_PROFILE; /* 0x804C, giá trị sau reset 0x00000000 */
    volatile const uint32_t TUNE_CH1_BUF0_PROFILE; /* 0x8050, giá trị sau reset 0x00000000 */
    volatile const uint32_t TUNE_CH1_BUF1_PROFILE; /* 0x8054, giá trị sau reset 0x00000000 */
    volatile const uint32_t TUNE_CH1_BUF2_PROFILE; /* 0x8058, giá trị sau reset 0x00000000 */
    volatile const uint32_t TUNE_CH1_BUF3_PROFILE; /* 0x805C, giá trị sau reset 0x00000000 */
    volatile const uint32_t TUNE_CH2_BUF0_PROFILE; /* 0x8060, giá trị sau reset 0x00000000 */
    volatile const uint32_t TUNE_CH2_BUF1_PROFILE; /* 0x8064, giá trị sau reset 0x00000000 */
    volatile const uint32_t TUNE_CH2_BUF2_PROFILE; /* 0x8068, giá trị sau reset 0x00000000 */
    volatile const uint32_t TUNE_CH2_BUF3_PROFILE; /* 0x806C, giá trị sau reset 0x00000000 */
    volatile uint32_t _reserved_8070[36]; /* Khoảng địa chỉ dự phòng 0x8070..0x80FC */
    volatile uint32_t TUNE_NR_CTRL; /* 0x8100, giá trị sau reset 0x00000004 */
    volatile uint32_t TUNE_NR_VARIANCE; /* 0x8104, giá trị sau reset 0x00000000 */
    volatile uint32_t TUNE_NR_SCALE; /* 0x8108, giá trị sau reset 0x00000100 */
    volatile uint32_t TUNE_NR_LIMITS; /* 0x810C, giá trị sau reset 0xFFFF0000 */
    volatile uint32_t TUNE_NR_LL_SCALE; /* 0x8110, giá trị sau reset 0x00000100 */
    volatile uint32_t TUNE_NR_LH_SCALE; /* 0x8114, giá trị sau reset 0x00000100 */
    volatile uint32_t TUNE_NR_HL_SCALE; /* 0x8118, giá trị sau reset 0x00000100 */
    volatile uint32_t TUNE_NR_HH_SCALE; /* 0x811C, giá trị sau reset 0x00000100 */
    volatile uint32_t TUNE_NR_BLEND; /* 0x8120, giá trị sau reset 0x00000100 */
    volatile uint32_t TUNE_NR_LUT_ADDR; /* 0x8124, giá trị sau reset 0x00000000 */
    volatile uint32_t TUNE_NR_LUT_DATA; /* 0x8128, giá trị sau reset 0x00000000 */
    volatile uint32_t TUNE_NR_SEED; /* 0x812C, giá trị sau reset 0x00000000 */
    volatile uint32_t _reserved_8130[52]; /* Khoảng địa chỉ dự phòng 0x8130..0x81FC */
    volatile uint32_t TUNE_WB_GB; /* 0x8200, giá trị sau reset 0x00000100 */
    volatile uint32_t TUNE_DEMOSAIC_MARGIN; /* 0x8204, giá trị sau reset 0x00000000 */
    volatile uint32_t TUNE_CNF_BLEND; /* 0x8208, giá trị sau reset 0x00000100 */
    volatile uint32_t _reserved_820C[1]; /* Khoảng địa chỉ dự phòng 0x820C..0x820C */
    volatile uint32_t TUNE_CSC_CTRL; /* 0x8210, giá trị sau reset 0x00000002 */
    volatile uint32_t TUNE_CSC_CONTRAST; /* 0x8214, giá trị sau reset 0x00000100 */
    volatile uint32_t TUNE_CSC_BRIGHTNESS; /* 0x8218, giá trị sau reset 0x00000000 */
    volatile uint32_t _reserved_821C[1]; /* Khoảng địa chỉ dự phòng 0x821C..0x821C */
    volatile uint32_t TUNE_CSC_UU; /* 0x8220, giá trị sau reset 0x00004000 */
    volatile uint32_t TUNE_CSC_UV; /* 0x8224, giá trị sau reset 0x00000000 */
    volatile uint32_t TUNE_CSC_VU; /* 0x8228, giá trị sau reset 0x00000000 */
    volatile uint32_t TUNE_CSC_VV; /* 0x822C, giá trị sau reset 0x00004000 */
    volatile uint32_t _reserved_8230[52]; /* Khoảng địa chỉ dự phòng 0x8230..0x82FC */
    volatile uint32_t TUNE_AWB_CTRL; /* 0x8300, giá trị sau reset 0x00000000 */
    volatile uint32_t TUNE_AWB_ROI_START; /* 0x8304, giá trị sau reset 0x00000000 */
    volatile uint32_t TUNE_AWB_ROI_END; /* 0x8308, giá trị sau reset 0x00000000 */
    volatile uint32_t _reserved_830C[1]; /* Khoảng địa chỉ dự phòng 0x830C..0x830C */
    volatile uint32_t TUNE_AWB_R_LIMITS; /* 0x8310, giá trị sau reset 0x0FFF0000 */
    volatile uint32_t TUNE_AWB_G_LIMITS; /* 0x8314, giá trị sau reset 0x0FFF0000 */
    volatile uint32_t TUNE_AWB_B_LIMITS; /* 0x8318, giá trị sau reset 0x0FFF0000 */
    volatile uint32_t TUNE_AWB_MIN_G; /* 0x831C, giá trị sau reset 0x00000001 */
    volatile uint32_t _reserved_8320[8]; /* Khoảng địa chỉ dự phòng 0x8320..0x833C */
    volatile uint32_t TUNE_AWB_E0_CENTER; /* 0x8340, giá trị sau reset 0x01000100 */
    volatile uint32_t TUNE_AWB_E0_A; /* 0x8344, giá trị sau reset 0x00004000 */
    volatile uint32_t TUNE_AWB_E0_B; /* 0x8348, giá trị sau reset 0x00000000 */
    volatile uint32_t TUNE_AWB_E0_C; /* 0x834C, giá trị sau reset 0x00004000 */
    volatile uint32_t TUNE_AWB_E0_RADIUS2; /* 0x8350, giá trị sau reset 0x00010000 */
    volatile uint32_t _reserved_8354[3]; /* Khoảng địa chỉ dự phòng 0x8354..0x835C */
    volatile uint32_t TUNE_AWB_E1_CENTER; /* 0x8360, giá trị sau reset 0x01000100 */
    volatile uint32_t TUNE_AWB_E1_A; /* 0x8364, giá trị sau reset 0x00004000 */
    volatile uint32_t TUNE_AWB_E1_B; /* 0x8368, giá trị sau reset 0x00000000 */
    volatile uint32_t TUNE_AWB_E1_C; /* 0x836C, giá trị sau reset 0x00004000 */
    volatile uint32_t TUNE_AWB_E1_RADIUS2; /* 0x8370, giá trị sau reset 0x00010000 */
    volatile uint32_t _reserved_8374[3]; /* Khoảng địa chỉ dự phòng 0x8374..0x837C */
    volatile uint32_t TUNE_AWB_E2_CENTER; /* 0x8380, giá trị sau reset 0x01000100 */
    volatile uint32_t TUNE_AWB_E2_A; /* 0x8384, giá trị sau reset 0x00004000 */
    volatile uint32_t TUNE_AWB_E2_B; /* 0x8388, giá trị sau reset 0x00000000 */
    volatile uint32_t TUNE_AWB_E2_C; /* 0x838C, giá trị sau reset 0x00004000 */
    volatile uint32_t TUNE_AWB_E2_RADIUS2; /* 0x8390, giá trị sau reset 0x00010000 */
    volatile uint32_t _reserved_8394[3]; /* Khoảng địa chỉ dự phòng 0x8394..0x839C */
    volatile uint32_t TUNE_AWB_E3_CENTER; /* 0x83A0, giá trị sau reset 0x01000100 */
    volatile uint32_t TUNE_AWB_E3_A; /* 0x83A4, giá trị sau reset 0x00004000 */
    volatile uint32_t TUNE_AWB_E3_B; /* 0x83A8, giá trị sau reset 0x00000000 */
    volatile uint32_t TUNE_AWB_E3_C; /* 0x83AC, giá trị sau reset 0x00004000 */
    volatile uint32_t TUNE_AWB_E3_RADIUS2; /* 0x83B0, giá trị sau reset 0x00010000 */
    volatile uint32_t _reserved_83B4[3]; /* Khoảng địa chỉ dự phòng 0x83B4..0x83BC */
    volatile uint32_t TUNE_AWB_E4_CENTER; /* 0x83C0, giá trị sau reset 0x01000100 */
    volatile uint32_t TUNE_AWB_E4_A; /* 0x83C4, giá trị sau reset 0x00004000 */
    volatile uint32_t TUNE_AWB_E4_B; /* 0x83C8, giá trị sau reset 0x00000000 */
    volatile uint32_t TUNE_AWB_E4_C; /* 0x83CC, giá trị sau reset 0x00004000 */
    volatile uint32_t TUNE_AWB_E4_RADIUS2; /* 0x83D0, giá trị sau reset 0x00010000 */
    volatile uint32_t _reserved_83D4[3]; /* Khoảng địa chỉ dự phòng 0x83D4..0x83DC */
    volatile uint32_t TUNE_AWB_E5_CENTER; /* 0x83E0, giá trị sau reset 0x01000100 */
    volatile uint32_t TUNE_AWB_E5_A; /* 0x83E4, giá trị sau reset 0x00004000 */
    volatile uint32_t TUNE_AWB_E5_B; /* 0x83E8, giá trị sau reset 0x00000000 */
    volatile uint32_t TUNE_AWB_E5_C; /* 0x83EC, giá trị sau reset 0x00004000 */
    volatile uint32_t TUNE_AWB_E5_RADIUS2; /* 0x83F0, giá trị sau reset 0x00010000 */
    volatile uint32_t _reserved_83F4[3]; /* Khoảng địa chỉ dự phòng 0x83F4..0x83FC */
    volatile uint32_t TUNE_AWB_E6_CENTER; /* 0x8400, giá trị sau reset 0x01000100 */
    volatile uint32_t TUNE_AWB_E6_A; /* 0x8404, giá trị sau reset 0x00004000 */
    volatile uint32_t TUNE_AWB_E6_B; /* 0x8408, giá trị sau reset 0x00000000 */
    volatile uint32_t TUNE_AWB_E6_C; /* 0x840C, giá trị sau reset 0x00004000 */
    volatile uint32_t TUNE_AWB_E6_RADIUS2; /* 0x8410, giá trị sau reset 0x00010000 */
    volatile uint32_t _reserved_8414[3]; /* Khoảng địa chỉ dự phòng 0x8414..0x841C */
    volatile uint32_t TUNE_AWB_E7_CENTER; /* 0x8420, giá trị sau reset 0x01000100 */
    volatile uint32_t TUNE_AWB_E7_A; /* 0x8424, giá trị sau reset 0x00004000 */
    volatile uint32_t TUNE_AWB_E7_B; /* 0x8428, giá trị sau reset 0x00000000 */
    volatile uint32_t TUNE_AWB_E7_C; /* 0x842C, giá trị sau reset 0x00004000 */
    volatile uint32_t TUNE_AWB_E7_RADIUS2; /* 0x8430, giá trị sau reset 0x00010000 */
    volatile uint32_t _reserved_8434[51]; /* Khoảng địa chỉ dự phòng 0x8434..0x84FC */
    volatile uint32_t TUNE_AF_CTRL; /* 0x8500, giá trị sau reset 0xFFFF0000 */
    volatile uint32_t TUNE_AF_NOISE; /* 0x8504, giá trị sau reset 0x00000000 */
    volatile uint32_t _reserved_8508[14]; /* Khoảng địa chỉ dự phòng 0x8508..0x853C */
    volatile uint32_t TUNE_AF_W0_START; /* 0x8540, giá trị sau reset 0x00000000 */
    volatile uint32_t TUNE_AF_W0_END; /* 0x8544, giá trị sau reset 0x00000000 */
    volatile uint32_t TUNE_AF_W1_START; /* 0x8548, giá trị sau reset 0x00000000 */
    volatile uint32_t TUNE_AF_W1_END; /* 0x854C, giá trị sau reset 0x00000000 */
    volatile uint32_t TUNE_AF_W2_START; /* 0x8550, giá trị sau reset 0x00000000 */
    volatile uint32_t TUNE_AF_W2_END; /* 0x8554, giá trị sau reset 0x00000000 */
    volatile uint32_t TUNE_AF_W3_START; /* 0x8558, giá trị sau reset 0x00000000 */
    volatile uint32_t TUNE_AF_W3_END; /* 0x855C, giá trị sau reset 0x00000000 */
    volatile uint32_t TUNE_AF_W4_START; /* 0x8560, giá trị sau reset 0x00000000 */
    volatile uint32_t TUNE_AF_W4_END; /* 0x8564, giá trị sau reset 0x00000000 */
    volatile uint32_t TUNE_AF_W5_START; /* 0x8568, giá trị sau reset 0x00000000 */
    volatile uint32_t TUNE_AF_W5_END; /* 0x856C, giá trị sau reset 0x00000000 */
    volatile uint32_t TUNE_AF_W6_START; /* 0x8570, giá trị sau reset 0x00000000 */
    volatile uint32_t TUNE_AF_W6_END; /* 0x8574, giá trị sau reset 0x00000000 */
    volatile uint32_t TUNE_AF_W7_START; /* 0x8578, giá trị sau reset 0x00000000 */
    volatile uint32_t TUNE_AF_W7_END; /* 0x857C, giá trị sau reset 0x00000000 */
    volatile uint32_t TUNE_AF_W8_START; /* 0x8580, giá trị sau reset 0x00000000 */
    volatile uint32_t TUNE_AF_W8_END; /* 0x8584, giá trị sau reset 0x00000000 */
    volatile uint32_t TUNE_AF_W9_START; /* 0x8588, giá trị sau reset 0x00000000 */
    volatile uint32_t TUNE_AF_W9_END; /* 0x858C, giá trị sau reset 0x00000000 */
    volatile uint32_t TUNE_AF_W10_START; /* 0x8590, giá trị sau reset 0x00000000 */
    volatile uint32_t TUNE_AF_W10_END; /* 0x8594, giá trị sau reset 0x00000000 */
    volatile uint32_t TUNE_AF_W11_START; /* 0x8598, giá trị sau reset 0x00000000 */
    volatile uint32_t TUNE_AF_W11_END; /* 0x859C, giá trị sau reset 0x00000000 */
    volatile uint32_t TUNE_AF_W12_START; /* 0x85A0, giá trị sau reset 0x00000000 */
    volatile uint32_t TUNE_AF_W12_END; /* 0x85A4, giá trị sau reset 0x00000000 */
    volatile uint32_t TUNE_AF_W13_START; /* 0x85A8, giá trị sau reset 0x00000000 */
    volatile uint32_t TUNE_AF_W13_END; /* 0x85AC, giá trị sau reset 0x00000000 */
    volatile uint32_t TUNE_AF_W14_START; /* 0x85B0, giá trị sau reset 0x00000000 */
    volatile uint32_t TUNE_AF_W14_END; /* 0x85B4, giá trị sau reset 0x00000000 */
    volatile uint32_t TUNE_AF_W15_START; /* 0x85B8, giá trị sau reset 0x00000000 */
    volatile uint32_t TUNE_AF_W15_END; /* 0x85BC, giá trị sau reset 0x00000000 */
    volatile uint32_t _reserved_85C0[16]; /* Khoảng địa chỉ dự phòng 0x85C0..0x85FC */
    volatile uint32_t TUNE_HIST_CTRL; /* 0x8600, giá trị sau reset 0x00000000 */
    volatile uint32_t TUNE_HIST_START; /* 0x8604, giá trị sau reset 0x00000000 */
    volatile uint32_t TUNE_HIST_END; /* 0x8608, giá trị sau reset 0x00000000 */
    volatile uint32_t TUNE_HIST_STEP; /* 0x860C, giá trị sau reset 0x00000101 */
    volatile uint32_t _reserved_8610[12]; /* Khoảng địa chỉ dự phòng 0x8610..0x863C */
    volatile uint32_t TUNE_HIST_WEIGHT0; /* 0x8640, giá trị sau reset 0x00000001 */
    volatile uint32_t TUNE_HIST_WEIGHT1; /* 0x8644, giá trị sau reset 0x00000001 */
    volatile uint32_t TUNE_HIST_WEIGHT2; /* 0x8648, giá trị sau reset 0x00000001 */
    volatile uint32_t TUNE_HIST_WEIGHT3; /* 0x864C, giá trị sau reset 0x00000001 */
    volatile uint32_t TUNE_HIST_WEIGHT4; /* 0x8650, giá trị sau reset 0x00000001 */
    volatile uint32_t TUNE_HIST_WEIGHT5; /* 0x8654, giá trị sau reset 0x00000001 */
    volatile uint32_t TUNE_HIST_WEIGHT6; /* 0x8658, giá trị sau reset 0x00000001 */
    volatile uint32_t TUNE_HIST_WEIGHT7; /* 0x865C, giá trị sau reset 0x00000001 */
    volatile uint32_t TUNE_HIST_WEIGHT8; /* 0x8660, giá trị sau reset 0x00000001 */
    volatile uint32_t TUNE_HIST_WEIGHT9; /* 0x8664, giá trị sau reset 0x00000001 */
    volatile uint32_t TUNE_HIST_WEIGHT10; /* 0x8668, giá trị sau reset 0x00000001 */
    volatile uint32_t TUNE_HIST_WEIGHT11; /* 0x866C, giá trị sau reset 0x00000001 */
    volatile uint32_t TUNE_HIST_WEIGHT12; /* 0x8670, giá trị sau reset 0x00000001 */
    volatile uint32_t TUNE_HIST_WEIGHT13; /* 0x8674, giá trị sau reset 0x00000001 */
    volatile uint32_t TUNE_HIST_WEIGHT14; /* 0x8678, giá trị sau reset 0x00000001 */
    volatile uint32_t TUNE_HIST_WEIGHT15; /* 0x867C, giá trị sau reset 0x00000001 */
    volatile uint32_t TUNE_HIST_WEIGHT16; /* 0x8680, giá trị sau reset 0x00000001 */
    volatile uint32_t TUNE_HIST_WEIGHT17; /* 0x8684, giá trị sau reset 0x00000001 */
    volatile uint32_t TUNE_HIST_WEIGHT18; /* 0x8688, giá trị sau reset 0x00000001 */
    volatile uint32_t TUNE_HIST_WEIGHT19; /* 0x868C, giá trị sau reset 0x00000001 */
    volatile uint32_t TUNE_HIST_WEIGHT20; /* 0x8690, giá trị sau reset 0x00000001 */
    volatile uint32_t TUNE_HIST_WEIGHT21; /* 0x8694, giá trị sau reset 0x00000001 */
    volatile uint32_t TUNE_HIST_WEIGHT22; /* 0x8698, giá trị sau reset 0x00000001 */
    volatile uint32_t TUNE_HIST_WEIGHT23; /* 0x869C, giá trị sau reset 0x00000001 */
    volatile uint32_t TUNE_HIST_WEIGHT24; /* 0x86A0, giá trị sau reset 0x00000001 */
    volatile uint32_t _reserved_86A4[23]; /* Khoảng địa chỉ dự phòng 0x86A4..0x86FC */
    volatile uint32_t TUNE_AEC_CMD; /* 0x8700, giá trị sau reset 0x00000000 */
    volatile const uint32_t TUNE_AEC_STATUS; /* 0x8704, giá trị sau reset 0x00000000 */
    volatile uint32_t TUNE_AEC_DROPS; /* 0x8708, giá trị sau reset 0x00000000 */
    volatile const uint32_t TUNE_AEC_FRAME; /* 0x870C, giá trị sau reset 0x00000000 */
    volatile const uint32_t TUNE_AEC_PROFILE; /* 0x8710, giá trị sau reset 0x00000000 */
    volatile uint32_t TUNE_AEC_INDEX; /* 0x8714, giá trị sau reset 0x00000000 */
    volatile const uint32_t TUNE_AEC_DATA_LO; /* 0x8718, giá trị sau reset 0x00000000 */
    volatile const uint32_t TUNE_AEC_DATA_HI; /* 0x871C, giá trị sau reset 0x00000000 */
    volatile uint32_t _reserved_8720[8]; /* Khoảng địa chỉ dự phòng 0x8720..0x873C */
    volatile uint32_t TUNE_AF_CMD; /* 0x8740, giá trị sau reset 0x00000000 */
    volatile const uint32_t TUNE_AF_STATUS; /* 0x8744, giá trị sau reset 0x00000000 */
    volatile uint32_t TUNE_AF_DROPS; /* 0x8748, giá trị sau reset 0x00000000 */
    volatile const uint32_t TUNE_AF_FRAME; /* 0x874C, giá trị sau reset 0x00000000 */
    volatile const uint32_t TUNE_AF_PROFILE; /* 0x8750, giá trị sau reset 0x00000000 */
    volatile uint32_t TUNE_AF_INDEX; /* 0x8754, giá trị sau reset 0x00000000 */
    volatile const uint32_t TUNE_AF_DATA_LO; /* 0x8758, giá trị sau reset 0x00000000 */
    volatile const uint32_t TUNE_AF_DATA_HI; /* 0x875C, giá trị sau reset 0x00000000 */
    volatile uint32_t _reserved_8760[8]; /* Khoảng địa chỉ dự phòng 0x8760..0x877C */
    volatile const uint32_t TUNE_AWB_PROFILE; /* 0x8780, giá trị sau reset 0x00000000 */
    volatile uint32_t _reserved_8784[31]; /* Khoảng địa chỉ dự phòng 0x8784..0x87FC */
    volatile uint32_t TUNE_GAMMA_PORT; /* 0x8800, giá trị sau reset 0x00000000 */
    volatile uint32_t TUNE_GAMMA_DATA; /* 0x8804, giá trị sau reset 0x00000000 */
    volatile uint32_t TUNE_TABLE_CMD; /* 0x8808, giá trị sau reset 0x00000000 */
    volatile const uint32_t TUNE_TABLE_STATUS; /* 0x880C, giá trị sau reset 0x00000000 */
    volatile uint32_t _reserved_8810[60]; /* Khoảng địa chỉ dự phòng 0x8810..0x88FC */
    volatile uint32_t TUNE_BPC_CTRL; /* 0x8900, giá trị sau reset 0x00000000 */
    volatile uint32_t TUNE_BPC_G_THRESH; /* 0x8904, giá trị sau reset 0x00000100 */
    volatile uint32_t TUNE_BPC_RB_THRESH; /* 0x8908, giá trị sau reset 0x00000100 */
    volatile uint32_t _reserved_890C[1]; /* Khoảng địa chỉ dự phòng 0x890C..0x890C */
    volatile uint32_t TUNE_BPC_COUNT; /* 0x8910, giá trị sau reset 0x00000000 */
    volatile uint32_t TUNE_BPC_ADDR; /* 0x8914, giá trị sau reset 0x00000000 */
    volatile uint32_t TUNE_BPC_DATA; /* 0x8918, giá trị sau reset 0x00000000 */
    volatile uint32_t _reserved_891C[57]; /* Khoảng địa chỉ dự phòng 0x891C..0x89FC */
    volatile uint32_t TUNE_LSC_CTRL; /* 0x8A00, giá trị sau reset 0x00000000 */
    volatile uint32_t TUNE_LSC_SECTOR_ADDR; /* 0x8A04, giá trị sau reset 0x00000000 */
    volatile uint32_t TUNE_LSC_SECTOR_DATA; /* 0x8A08, giá trị sau reset 0x00000000 */
    volatile uint32_t _reserved_8A0C[61]; /* Khoảng địa chỉ dự phòng 0x8A0C..0x8AFC */
    volatile uint32_t COMMON_INPUT_FORMAT; /* 0x8B00, giá trị sau reset 0x00000000 */
    volatile uint32_t _reserved_8B04[1]; /* Khoảng địa chỉ dự phòng 0x8B04..0x8B04 */
    volatile uint32_t TUNE_AEC_SOURCE; /* 0x8B08, giá trị sau reset 0x00000000 */
    volatile const uint32_t TUNE_AEC_RESULT_META; /* 0x8B0C, giá trị sau reset 0x00000000 */
    volatile uint32_t _reserved_8B10[12]; /* Khoảng địa chỉ dự phòng 0x8B10..0x8B3C */
    volatile uint32_t CH0_CROP_CTRL; /* 0x8B40, giá trị sau reset 0x00000000 */
    volatile uint32_t CH0_CROP_ORIGIN; /* 0x8B44, giá trị sau reset 0x00000000 */
    volatile uint32_t CH0_CROP_SIZE; /* 0x8B48, giá trị sau reset 0x00000000 */
    volatile const uint32_t CH0_GEOMETRY_STATUS; /* 0x8B4C, giá trị sau reset 0x00000000 */
    volatile const uint32_t CH0_ACTIVE_CROP_ORIGIN; /* 0x8B50, giá trị sau reset 0x00000000 */
    volatile const uint32_t CH0_ACTIVE_CROP_SIZE; /* 0x8B54, giá trị sau reset 0x00000000 */
    volatile const uint32_t CH0_OUTPUT_LIMIT; /* 0x8B58, giá trị sau reset 0x08700F00 */
    volatile uint32_t _reserved_8B5C[9]; /* Khoảng địa chỉ dự phòng 0x8B5C..0x8B7C */
    volatile uint32_t CH1_CROP_CTRL; /* 0x8B80, giá trị sau reset 0x00000000 */
    volatile uint32_t CH1_CROP_ORIGIN; /* 0x8B84, giá trị sau reset 0x00000000 */
    volatile uint32_t CH1_CROP_SIZE; /* 0x8B88, giá trị sau reset 0x00000000 */
    volatile const uint32_t CH1_GEOMETRY_STATUS; /* 0x8B8C, giá trị sau reset 0x00000000 */
    volatile const uint32_t CH1_ACTIVE_CROP_ORIGIN; /* 0x8B90, giá trị sau reset 0x00000000 */
    volatile const uint32_t CH1_ACTIVE_CROP_SIZE; /* 0x8B94, giá trị sau reset 0x00000000 */
    volatile const uint32_t CH1_OUTPUT_LIMIT; /* 0x8B98, giá trị sau reset 0x08700F00 */
    volatile uint32_t _reserved_8B9C[9]; /* Khoảng địa chỉ dự phòng 0x8B9C..0x8BBC */
    volatile uint32_t CH2_CROP_CTRL; /* 0x8BC0, giá trị sau reset 0x00000000 */
    volatile uint32_t CH2_CROP_ORIGIN; /* 0x8BC4, giá trị sau reset 0x00000000 */
    volatile uint32_t CH2_CROP_SIZE; /* 0x8BC8, giá trị sau reset 0x00000000 */
    volatile const uint32_t CH2_GEOMETRY_STATUS; /* 0x8BCC, giá trị sau reset 0x00000000 */
    volatile const uint32_t CH2_ACTIVE_CROP_ORIGIN; /* 0x8BD0, giá trị sau reset 0x00000000 */
    volatile const uint32_t CH2_ACTIVE_CROP_SIZE; /* 0x8BD4, giá trị sau reset 0x00000000 */
    volatile const uint32_t CH2_OUTPUT_LIMIT; /* 0x8BD8, giá trị sau reset 0x02800280 */
    volatile uint32_t _reserved_8BDC[9]; /* Khoảng địa chỉ dự phòng 0x8BDC..0x8BFC */
    volatile uint32_t COMMON_RECOVERY_STATUS; /* 0x8C00, giá trị sau reset 0x00000000 */
    volatile const uint32_t COMMON_RECOVERY_SEQUENCE; /* 0x8C04, giá trị sau reset 0x00000000 */
    volatile uint32_t _reserved_8C08[2]; /* Khoảng địa chỉ dự phòng 0x8C08..0x8C0C */
    volatile const uint32_t COMMON_ABORT0_FRAME; /* 0x8C10, giá trị sau reset 0x00000000 */
    volatile const uint32_t COMMON_ABORT0_OUTCOME; /* 0x8C14, giá trị sau reset 0x00000000 */
    volatile const uint32_t COMMON_ABORT1_FRAME; /* 0x8C18, giá trị sau reset 0x00000000 */
    volatile const uint32_t COMMON_ABORT1_OUTCOME; /* 0x8C1C, giá trị sau reset 0x00000000 */
    volatile const uint32_t COMMON_ABORT2_FRAME; /* 0x8C20, giá trị sau reset 0x00000000 */
    volatile const uint32_t COMMON_ABORT2_OUTCOME; /* 0x8C24, giá trị sau reset 0x00000000 */
    volatile const uint32_t COMMON_ABORT3_FRAME; /* 0x8C28, giá trị sau reset 0x00000000 */
    volatile const uint32_t COMMON_ABORT3_OUTCOME; /* 0x8C2C, giá trị sau reset 0x00000000 */
    volatile uint32_t _reserved_8C30[4]; /* Khoảng địa chỉ dự phòng 0x8C30..0x8C3C */
    volatile const uint32_t TUNE_CH0_BUF0_FRAME; /* 0x8C40, giá trị sau reset 0x00000000 */
    volatile const uint32_t TUNE_CH0_BUF1_FRAME; /* 0x8C44, giá trị sau reset 0x00000000 */
    volatile const uint32_t TUNE_CH0_BUF2_FRAME; /* 0x8C48, giá trị sau reset 0x00000000 */
    volatile const uint32_t TUNE_CH0_BUF3_FRAME; /* 0x8C4C, giá trị sau reset 0x00000000 */
    volatile const uint32_t TUNE_CH1_BUF0_FRAME; /* 0x8C50, giá trị sau reset 0x00000000 */
    volatile const uint32_t TUNE_CH1_BUF1_FRAME; /* 0x8C54, giá trị sau reset 0x00000000 */
    volatile const uint32_t TUNE_CH1_BUF2_FRAME; /* 0x8C58, giá trị sau reset 0x00000000 */
    volatile const uint32_t TUNE_CH1_BUF3_FRAME; /* 0x8C5C, giá trị sau reset 0x00000000 */
    volatile const uint32_t TUNE_CH2_BUF0_FRAME; /* 0x8C60, giá trị sau reset 0x00000000 */
    volatile const uint32_t TUNE_CH2_BUF1_FRAME; /* 0x8C64, giá trị sau reset 0x00000000 */
    volatile const uint32_t TUNE_CH2_BUF2_FRAME; /* 0x8C68, giá trị sau reset 0x00000000 */
    volatile const uint32_t TUNE_CH2_BUF3_FRAME; /* 0x8C6C, giá trị sau reset 0x00000000 */
    volatile uint32_t _reserved_8C70[4]; /* Khoảng địa chỉ dự phòng 0x8C70..0x8C7C */
    volatile const uint32_t TUNE_AE_FRAME_SIZE; /* 0x8C80, giá trị sau reset 0x00000000 */
    volatile const uint32_t TUNE_AE_GRID; /* 0x8C84, giá trị sau reset 0x00000000 */
    volatile const uint32_t TUNE_AE_ZONE_SIZE; /* 0x8C88, giá trị sau reset 0x00000000 */
    volatile uint32_t _reserved_8C8C[1]; /* Khoảng địa chỉ dự phòng 0x8C8C..0x8C8C */
    volatile const uint32_t TUNE_AWB_FRAME_SIZE; /* 0x8C90, giá trị sau reset 0x00000000 */
} FX1_ISP_Regs_t;

/* Kiểm tra C11/C++11 bảo đảm cấu trúc C có địa chỉ thành viên khớp bảng thanh ghi. */
#if defined(__cplusplus) && __cplusplus >= 201103L
#define FX1_ISP_STATIC_ASSERT(expr, msg) static_assert((expr), msg)
#elif defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
#define FX1_ISP_STATIC_ASSERT(expr, msg) _Static_assert((expr), msg)
#endif
#ifdef FX1_ISP_STATIC_ASSERT
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, COMMON_VER_DATE) == 0x0000U, "COMMON_VER_DATE offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, COMMON_VER_ID) == 0x0004U, "COMMON_VER_ID offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, COMMON_CTRL) == 0x0008U, "COMMON_CTRL offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, COMMON_STATUS) == 0x000CU, "COMMON_STATUS offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, COMMON_IRQ_STATUS) == 0x0010U, "COMMON_IRQ_STATUS offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, COMMON_IRQ_EN) == 0x0014U, "COMMON_IRQ_EN offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, COMMON_FRAME_WIDTH) == 0x0018U, "COMMON_FRAME_WIDTH offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, COMMON_FRAME_HEIGHT) == 0x001CU, "COMMON_FRAME_HEIGHT offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, COMMON_BAYER) == 0x0020U, "COMMON_BAYER offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, COMMON_SCRATCH) == 0x0028U, "COMMON_SCRATCH offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, BLC_CTRL) == 0x0200U, "BLC_CTRL offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, BLC_GAIN_SEL) == 0x0204U, "BLC_GAIN_SEL offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, BLC_OFS_G0_DFT) == 0x0208U, "BLC_OFS_G0_DFT offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, BLC_OFS_G0_R) == 0x020CU, "BLC_OFS_G0_R offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, BLC_OFS_G0_GR) == 0x0210U, "BLC_OFS_G0_GR offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, BLC_OFS_G0_GB) == 0x0214U, "BLC_OFS_G0_GB offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, BLC_OFS_G0_B) == 0x0218U, "BLC_OFS_G0_B offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, BLC_OFS_G1_DFT) == 0x021CU, "BLC_OFS_G1_DFT offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, BLC_OFS_G1_R) == 0x0220U, "BLC_OFS_G1_R offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, BLC_OFS_G1_GR) == 0x0224U, "BLC_OFS_G1_GR offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, BLC_OFS_G1_GB) == 0x0228U, "BLC_OFS_G1_GB offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, BLC_OFS_G1_B) == 0x022CU, "BLC_OFS_G1_B offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, BLC_OFS_G2_DFT) == 0x0230U, "BLC_OFS_G2_DFT offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, BLC_OFS_G2_R) == 0x0234U, "BLC_OFS_G2_R offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, BLC_OFS_G2_GR) == 0x0238U, "BLC_OFS_G2_GR offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, BLC_OFS_G2_GB) == 0x023CU, "BLC_OFS_G2_GB offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, BLC_OFS_G2_B) == 0x0240U, "BLC_OFS_G2_B offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, BLC_OFS_G3_DFT) == 0x0244U, "BLC_OFS_G3_DFT offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, BLC_OFS_G3_R) == 0x0248U, "BLC_OFS_G3_R offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, BLC_OFS_G3_GR) == 0x024CU, "BLC_OFS_G3_GR offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, BLC_OFS_G3_GB) == 0x0250U, "BLC_OFS_G3_GB offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, BLC_OFS_G3_B) == 0x0254U, "BLC_OFS_G3_B offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, BLC_SCALE_G0) == 0x0258U, "BLC_SCALE_G0 offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, BLC_SCALE_G1) == 0x025CU, "BLC_SCALE_G1 offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, BLC_SCALE_G2) == 0x0260U, "BLC_SCALE_G2 offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, BLC_SCALE_G3) == 0x0264U, "BLC_SCALE_G3 offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, LSC_CTRL) == 0x0400U, "LSC_CTRL offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, LSC_MESH_NODES) == 0x0404U, "LSC_MESH_NODES offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, LSC_STRENGTH) == 0x0408U, "LSC_STRENGTH offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, LSC_PROFILE_SEL) == 0x040CU, "LSC_PROFILE_SEL offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, LSC_LOAD_CTRL) == 0x0410U, "LSC_LOAD_CTRL offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, LSC_COEF_DATA) == 0x0414U, "LSC_COEF_DATA offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, LSC_LOAD_STATUS) == 0x0418U, "LSC_LOAD_STATUS offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, LSC_PROFILE_STATUS) == 0x041CU, "LSC_PROFILE_STATUS offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, LSC_ERROR) == 0x0420U, "LSC_ERROR offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, LSC_STAT_OVF_CNT_R) == 0x0424U, "LSC_STAT_OVF_CNT_R offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, LSC_STAT_OVF_CNT_GR) == 0x0428U, "LSC_STAT_OVF_CNT_GR offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, LSC_STAT_OVF_CNT_GB) == 0x042CU, "LSC_STAT_OVF_CNT_GB offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, LSC_STAT_OVF_CNT_B) == 0x0430U, "LSC_STAT_OVF_CNT_B offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, BPC_CTRL) == 0x0600U, "BPC_CTRL offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, BPC_MODE) == 0x0604U, "BPC_MODE offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, BPC_TEMPORAL_VAR) == 0x0608U, "BPC_TEMPORAL_VAR offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, BPC_THRESH) == 0x060CU, "BPC_THRESH offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, BPC_PIXEL_AGE) == 0x0610U, "BPC_PIXEL_AGE offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, BPC_NUM_CANDIDATES) == 0x0614U, "BPC_NUM_CANDIDATES offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, BPC_NUM_DEFECTIVE) == 0x0618U, "BPC_NUM_DEFECTIVE offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, BPC_STATUS) == 0x061CU, "BPC_STATUS offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, WB_CTRL) == 0x0800U, "WB_CTRL offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, WB_GAIN_R) == 0x0804U, "WB_GAIN_R offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, WB_GAIN_G) == 0x0808U, "WB_GAIN_G offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, WB_GAIN_B) == 0x080CU, "WB_GAIN_B offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, DG_CTRL) == 0x0A00U, "DG_CTRL offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, DG_GAIN) == 0x0A04U, "DG_GAIN offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, D_WDR_CTRL) == 0x0C00U, "D_WDR_CTRL offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, D_WDR_STRENGTH) == 0x0C04U, "D_WDR_STRENGTH offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, DEMOSAIC_STATUS) == 0x0E00U, "DEMOSAIC_STATUS offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, CCM_CTRL) == 0x1000U, "CCM_CTRL offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, CCM_CRR) == 0x1004U, "CCM_CRR offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, CCM_CRG) == 0x1008U, "CCM_CRG offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, CCM_CRB) == 0x100CU, "CCM_CRB offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, CCM_CGR) == 0x1010U, "CCM_CGR offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, CCM_CGG) == 0x1014U, "CCM_CGG offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, CCM_CGB) == 0x1018U, "CCM_CGB offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, CCM_CBR) == 0x101CU, "CCM_CBR offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, CCM_CBG) == 0x1020U, "CCM_CBG offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, CCM_CBB) == 0x1024U, "CCM_CBB offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, CCM_OFS_R) == 0x1028U, "CCM_OFS_R offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, CCM_OFS_G) == 0x102CU, "CCM_OFS_G offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, CCM_OFS_B) == 0x1030U, "CCM_OFS_B offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, CCM_STATUS) == 0x1034U, "CCM_STATUS offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, GAMMA_CTRL) == 0x1200U, "GAMMA_CTRL offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, GAMMA_LUT_ADDR) == 0x1204U, "GAMMA_LUT_ADDR offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, GAMMA_LUT_DATA) == 0x1208U, "GAMMA_LUT_DATA offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, GAMMA_LUT_RDATA) == 0x120CU, "GAMMA_LUT_RDATA offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, GAMMA_STATUS) == 0x1210U, "GAMMA_STATUS offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, CSC_CTRL) == 0x1400U, "CSC_CTRL offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, GTM_CTRL) == 0x1600U, "GTM_CTRL offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, GTM_LUT_ADDR) == 0x1604U, "GTM_LUT_ADDR offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, GTM_LUT_DATA) == 0x1608U, "GTM_LUT_DATA offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, GTM_KEY) == 0x160CU, "GTM_KEY offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, GTM_LWHITE) == 0x1610U, "GTM_LWHITE offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, GTM_ROI_LOG2) == 0x1614U, "GTM_ROI_LOG2 offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, GTM_LUT_RDATA) == 0x1618U, "GTM_LUT_RDATA offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, GTM_STATUS) == 0x161CU, "GTM_STATUS offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, NR_2D_CTRL) == 0x1800U, "NR_2D_CTRL offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, NR_2D_EST_VAR) == 0x1804U, "NR_2D_EST_VAR offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, NR_2D_STATUS) == 0x1808U, "NR_2D_STATUS offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TNR_3D_CTRL) == 0x1A00U, "TNR_3D_CTRL offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TNR_3D_MTF_SEL) == 0x1A04U, "TNR_3D_MTF_SEL offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TNR_3D_HIST_BASE) == 0x1A08U, "TNR_3D_HIST_BASE offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, EE_CTRL) == 0x1C00U, "EE_CTRL offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, EE_ALPHA) == 0x1C04U, "EE_ALPHA offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, EE_BETA) == 0x1C08U, "EE_BETA offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, EE_CLAMP) == 0x1C0CU, "EE_CLAMP offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, EE_FEATURE_EN) == 0x1C10U, "EE_FEATURE_EN offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, EE_RADIAL_CENTER) == 0x1C14U, "EE_RADIAL_CENTER offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, EE_RADIAL_R2_TH0) == 0x1C18U, "EE_RADIAL_R2_TH0 offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, EE_RADIAL_R2_TH1) == 0x1C1CU, "EE_RADIAL_R2_TH1 offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, EE_RADIAL_R2_TH2) == 0x1C20U, "EE_RADIAL_R2_TH2 offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, EE_RADIAL_GAIN_01) == 0x1C24U, "EE_RADIAL_GAIN_01 offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, EE_RADIAL_GAIN_23) == 0x1C28U, "EE_RADIAL_GAIN_23 offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, EE_LUT_CTRL) == 0x1C2CU, "EE_LUT_CTRL offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, EE_LUT_ADDR) == 0x1C30U, "EE_LUT_ADDR offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, EE_LUT_WDATA) == 0x1C34U, "EE_LUT_WDATA offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, EE_STATUS) == 0x1C38U, "EE_STATUS offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, EE_LUT_RDATA) == 0x1C3CU, "EE_LUT_RDATA offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, CNF_CTRL) == 0x1E00U, "CNF_CTRL offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, CNF_CHROMA_TH) == 0x1E04U, "CNF_CHROMA_TH offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, CNF_LUMA_TH) == 0x1E08U, "CNF_LUMA_TH offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, CNF_STRENGTH) == 0x1E0CU, "CNF_STRENGTH offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, CNF_STATUS) == 0x1E10U, "CNF_STATUS offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, RESIZER_CTRL) == 0x2000U, "RESIZER_CTRL offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, RESIZER_CUSTOM_W) == 0x2004U, "RESIZER_CUSTOM_W offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, RESIZER_CUSTOM_H) == 0x2008U, "RESIZER_CUSTOM_H offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, RESIZER_OUT_W) == 0x2014U, "RESIZER_OUT_W offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, RESIZER_OUT_H) == 0x2018U, "RESIZER_OUT_H offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, RESIZER_STATUS) == 0x201CU, "RESIZER_STATUS offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, OUTPUT_ENABLE) == 0x2020U, "OUTPUT_ENABLE offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, OUTPUT_IRQ_EN) == 0x2024U, "OUTPUT_IRQ_EN offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, OUTPUT_IRQ_STATUS) == 0x2028U, "OUTPUT_IRQ_STATUS offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, AEC_CTRL) == 0x2200U, "AEC_CTRL offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, AEC_CONTEXT_ID) == 0x2204U, "AEC_CONTEXT_ID offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, AEC_ZONE_CFG) == 0x2208U, "AEC_ZONE_CFG offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, AEC_ZONE_SIZE) == 0x220CU, "AEC_ZONE_SIZE offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, AEC_SAMPLE_CLIP) == 0x2210U, "AEC_SAMPLE_CLIP offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, AEC_THRESH) == 0x2214U, "AEC_THRESH offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, AEC_STATUS) == 0x2218U, "AEC_STATUS offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, AEC_FRAME_ID) == 0x221CU, "AEC_FRAME_ID offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, AEC_RESULT_CONTEXT_ID) == 0x2220U, "AEC_RESULT_CONTEXT_ID offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, AEC_CHANNEL_SEL) == 0x2224U, "AEC_CHANNEL_SEL offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, AEC_GLOBAL_SUM_LO) == 0x2228U, "AEC_GLOBAL_SUM_LO offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, AEC_GLOBAL_SUM_HI) == 0x222CU, "AEC_GLOBAL_SUM_HI offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, AEC_GLOBAL_COUNT) == 0x2230U, "AEC_GLOBAL_COUNT offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, AEC_ZONE_ADDR) == 0x2234U, "AEC_ZONE_ADDR offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, AEC_ZONE_SUM) == 0x2238U, "AEC_ZONE_SUM offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, AEC_ZONE_COUNT) == 0x223CU, "AEC_ZONE_COUNT offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, AEC_ZONE_GREEN_OE_UE) == 0x2240U, "AEC_ZONE_GREEN_OE_UE offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, AEC_ZONE_GREEN_MIN_MAX) == 0x2244U, "AEC_ZONE_GREEN_MIN_MAX offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, AEC_HIST_ADDR) == 0x2248U, "AEC_HIST_ADDR offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, AEC_HIST_DATA) == 0x224CU, "AEC_HIST_DATA offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, AWB_CTRL) == 0x2400U, "AWB_CTRL offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, AWB_X_BOUND_ADDR) == 0x2404U, "AWB_X_BOUND_ADDR offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, AWB_Y_BOUND_ADDR) == 0x2408U, "AWB_Y_BOUND_ADDR offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, AWB_X_BOUND_DATA) == 0x240CU, "AWB_X_BOUND_DATA offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, AWB_Y_BOUND_DATA) == 0x2410U, "AWB_Y_BOUND_DATA offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, AWB_GLOBAL_SUM_R_H) == 0x2414U, "AWB_GLOBAL_SUM_R_H offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, AWB_GLOBAL_SUM_R_L) == 0x2418U, "AWB_GLOBAL_SUM_R_L offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, AWB_GLOBAL_SUM_G_H) == 0x241CU, "AWB_GLOBAL_SUM_G_H offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, AWB_GLOBAL_SUM_G_L) == 0x2420U, "AWB_GLOBAL_SUM_G_L offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, AWB_GLOBAL_SUM_B_H) == 0x2424U, "AWB_GLOBAL_SUM_B_H offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, AWB_GLOBAL_SUM_B_L) == 0x2428U, "AWB_GLOBAL_SUM_B_L offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, AWB_GLOBAL_COUNT) == 0x242CU, "AWB_GLOBAL_COUNT offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, AWB_ZONE_ADDR) == 0x2430U, "AWB_ZONE_ADDR offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, AWB_ZONE_SUM_R_H) == 0x2434U, "AWB_ZONE_SUM_R_H offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, AWB_ZONE_SUM_R_L) == 0x2438U, "AWB_ZONE_SUM_R_L offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, AWB_ZONE_SUM_G_H) == 0x243CU, "AWB_ZONE_SUM_G_H offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, AWB_ZONE_SUM_G_L) == 0x2440U, "AWB_ZONE_SUM_G_L offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, AWB_ZONE_SUM_B_H) == 0x2444U, "AWB_ZONE_SUM_B_H offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, AWB_ZONE_SUM_B_L) == 0x2448U, "AWB_ZONE_SUM_B_L offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, AWB_ZONE_COUNT) == 0x244CU, "AWB_ZONE_COUNT offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, AWB_STATUS) == 0x2450U, "AWB_STATUS offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, AWB_UNDEREXPOSED_LIMIT) == 0x2454U, "AWB_UNDEREXPOSED_LIMIT offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, AWB_SATURATION_LIMIT) == 0x2458U, "AWB_SATURATION_LIMIT offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, AWB_CONTEXT_ID) == 0x245CU, "AWB_CONTEXT_ID offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, AWB_RESULT_CONTEXT_ID) == 0x2460U, "AWB_RESULT_CONTEXT_ID offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, AWB_FRAME_ID) == 0x2464U, "AWB_FRAME_ID offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, AWB_RESULT_CTRL) == 0x2468U, "AWB_RESULT_CTRL offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, AWB_RESULT_STATUS) == 0x246CU, "AWB_RESULT_STATUS offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, AWB_DROP_COUNT) == 0x2470U, "AWB_DROP_COUNT offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, AF_CTRL) == 0x2600U, "AF_CTRL offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, AF_ZONE_EN) == 0x2604U, "AF_ZONE_EN offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, AF_METRIC_CFG) == 0x2608U, "AF_METRIC_CFG offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, AF_STAT_ADDR) == 0x260CU, "AF_STAT_ADDR offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, AF_STAT_DATA) == 0x2610U, "AF_STAT_DATA offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, AF_LUT_ADDR) == 0x2614U, "AF_LUT_ADDR offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, AF_LUT_DATA) == 0x2618U, "AF_LUT_DATA offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, AF_VCM_CUR_POS) == 0x261CU, "AF_VCM_CUR_POS offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, AF_VCM_BEST_POS) == 0x2620U, "AF_VCM_BEST_POS offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, AF_PEAK_SCORE) == 0x2624U, "AF_PEAK_SCORE offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, AF_FOCUS_RANGE) == 0x2628U, "AF_FOCUS_RANGE offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, AF_STATUS) == 0x262CU, "AF_STATUS offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, AF_CONTEXT_ID) == 0x2630U, "AF_CONTEXT_ID offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, AF_RESULT_CONTEXT_ID) == 0x2634U, "AF_RESULT_CONTEXT_ID offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, AF_FRAME_ID) == 0x2638U, "AF_FRAME_ID offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, OFMT_CTRL) == 0x2800U, "OFMT_CTRL offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, OFMT_Y_STRIDE) == 0x2804U, "OFMT_Y_STRIDE offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, OFMT_UV_STRIDE) == 0x2808U, "OFMT_UV_STRIDE offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, DMA_CTRL) == 0x3000U, "DMA_CTRL offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, DMA_STAT) == 0x3004U, "DMA_STAT offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, DMA_ERR) == 0x3008U, "DMA_ERR offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, DMA_IRQ_EN) == 0x300CU, "DMA_IRQ_EN offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, DMA_IRQ_STAT) == 0x3010U, "DMA_IRQ_STAT offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, IDMA_STRIDE) == 0x3020U, "IDMA_STRIDE offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, IDMA_BUF_VALID) == 0x3024U, "IDMA_BUF_VALID offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, IDMA_FRAME_COUNT) == 0x3028U, "IDMA_FRAME_COUNT offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, IDMA_BUF_ADDR_L0) == 0x3040U, "IDMA_BUF_ADDR_L0 offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, IDMA_BUF_ADDR_H0) == 0x3044U, "IDMA_BUF_ADDR_H0 offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, IDMA_BUF_ADDR_L1) == 0x3048U, "IDMA_BUF_ADDR_L1 offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, IDMA_BUF_ADDR_H1) == 0x304CU, "IDMA_BUF_ADDR_H1 offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, IDMA_BUF_ADDR_L2) == 0x3050U, "IDMA_BUF_ADDR_L2 offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, IDMA_BUF_ADDR_H2) == 0x3054U, "IDMA_BUF_ADDR_H2 offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, IDMA_BUF_ADDR_L3) == 0x3058U, "IDMA_BUF_ADDR_L3 offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, IDMA_BUF_ADDR_H3) == 0x305CU, "IDMA_BUF_ADDR_H3 offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, ODMA_Y_STRIDE) == 0x3080U, "ODMA_Y_STRIDE offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, ODMA_UV_STRIDE) == 0x3084U, "ODMA_UV_STRIDE offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, ODMA_BUF_FREE) == 0x3088U, "ODMA_BUF_FREE offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, ODMA_BUF_DONE) == 0x308CU, "ODMA_BUF_DONE offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, ODMA_FRAME_COUNT) == 0x3090U, "ODMA_FRAME_COUNT offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, ODMA_Y_ADDR_L0) == 0x30A0U, "ODMA_Y_ADDR_L0 offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, ODMA_Y_ADDR_H0) == 0x30A4U, "ODMA_Y_ADDR_H0 offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, ODMA_UV_ADDR_L0) == 0x30A8U, "ODMA_UV_ADDR_L0 offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, ODMA_UV_ADDR_H0) == 0x30ACU, "ODMA_UV_ADDR_H0 offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, ODMA_Y_ADDR_L1) == 0x30B0U, "ODMA_Y_ADDR_L1 offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, ODMA_Y_ADDR_H1) == 0x30B4U, "ODMA_Y_ADDR_H1 offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, ODMA_UV_ADDR_L1) == 0x30B8U, "ODMA_UV_ADDR_L1 offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, ODMA_UV_ADDR_H1) == 0x30BCU, "ODMA_UV_ADDR_H1 offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, ODMA_Y_ADDR_L2) == 0x30C0U, "ODMA_Y_ADDR_L2 offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, ODMA_Y_ADDR_H2) == 0x30C4U, "ODMA_Y_ADDR_H2 offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, ODMA_UV_ADDR_L2) == 0x30C8U, "ODMA_UV_ADDR_L2 offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, ODMA_UV_ADDR_H2) == 0x30CCU, "ODMA_UV_ADDR_H2 offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, ODMA_Y_ADDR_L3) == 0x30D0U, "ODMA_Y_ADDR_L3 offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, ODMA_Y_ADDR_H3) == 0x30D4U, "ODMA_Y_ADDR_H3 offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, ODMA_UV_ADDR_L3) == 0x30D8U, "ODMA_UV_ADDR_L3 offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, ODMA_UV_ADDR_H3) == 0x30DCU, "ODMA_UV_ADDR_H3 offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, IDMA_BUF_TAG0) == 0x3100U, "IDMA_BUF_TAG0 offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, IDMA_BUF_TAG1) == 0x3104U, "IDMA_BUF_TAG1 offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, IDMA_BUF_TAG2) == 0x3108U, "IDMA_BUF_TAG2 offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, IDMA_BUF_TAG3) == 0x310CU, "IDMA_BUF_TAG3 offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, IDMA_BUF_TS0) == 0x3110U, "IDMA_BUF_TS0 offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, IDMA_BUF_TS1) == 0x3114U, "IDMA_BUF_TS1 offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, IDMA_BUF_TS2) == 0x3118U, "IDMA_BUF_TS2 offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, IDMA_BUF_TS3) == 0x311CU, "IDMA_BUF_TS3 offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, ODMA_BUF_TAG0) == 0x3120U, "ODMA_BUF_TAG0 offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, ODMA_BUF_TAG1) == 0x3124U, "ODMA_BUF_TAG1 offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, ODMA_BUF_TAG2) == 0x3128U, "ODMA_BUF_TAG2 offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, ODMA_BUF_TAG3) == 0x312CU, "ODMA_BUF_TAG3 offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, ODMA_BUF_TS0) == 0x3130U, "ODMA_BUF_TS0 offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, ODMA_BUF_TS1) == 0x3134U, "ODMA_BUF_TS1 offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, ODMA_BUF_TS2) == 0x3138U, "ODMA_BUF_TS2 offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, ODMA_BUF_TS3) == 0x313CU, "ODMA_BUF_TS3 offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, ODMA_BUF_SOF_TS0) == 0x3140U, "ODMA_BUF_SOF_TS0 offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, ODMA_BUF_SOF_TS1) == 0x3144U, "ODMA_BUF_SOF_TS1 offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, ODMA_BUF_SOF_TS2) == 0x3148U, "ODMA_BUF_SOF_TS2 offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, ODMA_BUF_SOF_TS3) == 0x314CU, "ODMA_BUF_SOF_TS3 offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, ODMA_BUF_EOF_TS0) == 0x3150U, "ODMA_BUF_EOF_TS0 offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, ODMA_BUF_EOF_TS1) == 0x3154U, "ODMA_BUF_EOF_TS1 offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, ODMA_BUF_EOF_TS2) == 0x3158U, "ODMA_BUF_EOF_TS2 offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, ODMA_BUF_EOF_TS3) == 0x315CU, "ODMA_BUF_EOF_TS3 offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, FRAME_COUNTER) == 0x3160U, "FRAME_COUNTER offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, CH1_RESIZER_CTRL) == 0x3200U, "CH1_RESIZER_CTRL offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, CH1_RESIZER_CUSTOM_W) == 0x3204U, "CH1_RESIZER_CUSTOM_W offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, CH1_RESIZER_CUSTOM_H) == 0x3208U, "CH1_RESIZER_CUSTOM_H offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, CH1_RESIZER_OUT_W) == 0x3214U, "CH1_RESIZER_OUT_W offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, CH1_RESIZER_OUT_H) == 0x3218U, "CH1_RESIZER_OUT_H offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, CH1_RESIZER_STATUS) == 0x321CU, "CH1_RESIZER_STATUS offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, CH1_ODMA_Y_STRIDE) == 0x3280U, "CH1_ODMA_Y_STRIDE offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, CH1_ODMA_UV_STRIDE) == 0x3284U, "CH1_ODMA_UV_STRIDE offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, CH1_ODMA_BUF_FREE) == 0x3288U, "CH1_ODMA_BUF_FREE offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, CH1_ODMA_BUF_DONE) == 0x328CU, "CH1_ODMA_BUF_DONE offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, CH1_ODMA_FRAME_COUNT) == 0x3290U, "CH1_ODMA_FRAME_COUNT offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, CH1_ODMA_Y_ADDR_L0) == 0x32A0U, "CH1_ODMA_Y_ADDR_L0 offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, CH1_ODMA_Y_ADDR_H0) == 0x32A4U, "CH1_ODMA_Y_ADDR_H0 offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, CH1_ODMA_UV_ADDR_L0) == 0x32A8U, "CH1_ODMA_UV_ADDR_L0 offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, CH1_ODMA_UV_ADDR_H0) == 0x32ACU, "CH1_ODMA_UV_ADDR_H0 offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, CH1_ODMA_Y_ADDR_L1) == 0x32B0U, "CH1_ODMA_Y_ADDR_L1 offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, CH1_ODMA_Y_ADDR_H1) == 0x32B4U, "CH1_ODMA_Y_ADDR_H1 offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, CH1_ODMA_UV_ADDR_L1) == 0x32B8U, "CH1_ODMA_UV_ADDR_L1 offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, CH1_ODMA_UV_ADDR_H1) == 0x32BCU, "CH1_ODMA_UV_ADDR_H1 offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, CH1_ODMA_Y_ADDR_L2) == 0x32C0U, "CH1_ODMA_Y_ADDR_L2 offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, CH1_ODMA_Y_ADDR_H2) == 0x32C4U, "CH1_ODMA_Y_ADDR_H2 offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, CH1_ODMA_UV_ADDR_L2) == 0x32C8U, "CH1_ODMA_UV_ADDR_L2 offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, CH1_ODMA_UV_ADDR_H2) == 0x32CCU, "CH1_ODMA_UV_ADDR_H2 offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, CH1_ODMA_Y_ADDR_L3) == 0x32D0U, "CH1_ODMA_Y_ADDR_L3 offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, CH1_ODMA_Y_ADDR_H3) == 0x32D4U, "CH1_ODMA_Y_ADDR_H3 offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, CH1_ODMA_UV_ADDR_L3) == 0x32D8U, "CH1_ODMA_UV_ADDR_L3 offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, CH1_ODMA_UV_ADDR_H3) == 0x32DCU, "CH1_ODMA_UV_ADDR_H3 offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, CH1_ODMA_BUF_TAG0) == 0x3300U, "CH1_ODMA_BUF_TAG0 offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, CH1_ODMA_BUF_TAG1) == 0x3304U, "CH1_ODMA_BUF_TAG1 offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, CH1_ODMA_BUF_TAG2) == 0x3308U, "CH1_ODMA_BUF_TAG2 offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, CH1_ODMA_BUF_TAG3) == 0x330CU, "CH1_ODMA_BUF_TAG3 offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, CH1_ODMA_BUF_TS0) == 0x3310U, "CH1_ODMA_BUF_TS0 offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, CH1_ODMA_BUF_TS1) == 0x3314U, "CH1_ODMA_BUF_TS1 offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, CH1_ODMA_BUF_TS2) == 0x3318U, "CH1_ODMA_BUF_TS2 offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, CH1_ODMA_BUF_TS3) == 0x331CU, "CH1_ODMA_BUF_TS3 offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, CH1_ODMA_BUF_SOF_TS0) == 0x3320U, "CH1_ODMA_BUF_SOF_TS0 offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, CH1_ODMA_BUF_SOF_TS1) == 0x3324U, "CH1_ODMA_BUF_SOF_TS1 offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, CH1_ODMA_BUF_SOF_TS2) == 0x3328U, "CH1_ODMA_BUF_SOF_TS2 offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, CH1_ODMA_BUF_SOF_TS3) == 0x332CU, "CH1_ODMA_BUF_SOF_TS3 offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, CH1_ODMA_BUF_EOF_TS0) == 0x3330U, "CH1_ODMA_BUF_EOF_TS0 offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, CH1_ODMA_BUF_EOF_TS1) == 0x3334U, "CH1_ODMA_BUF_EOF_TS1 offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, CH1_ODMA_BUF_EOF_TS2) == 0x3338U, "CH1_ODMA_BUF_EOF_TS2 offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, CH1_ODMA_BUF_EOF_TS3) == 0x333CU, "CH1_ODMA_BUF_EOF_TS3 offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, CH2_RESIZER_CTRL) == 0x3400U, "CH2_RESIZER_CTRL offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, CH2_RESIZER_CUSTOM_W) == 0x3404U, "CH2_RESIZER_CUSTOM_W offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, CH2_RESIZER_CUSTOM_H) == 0x3408U, "CH2_RESIZER_CUSTOM_H offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, CH2_RESIZER_OUT_W) == 0x3414U, "CH2_RESIZER_OUT_W offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, CH2_RESIZER_OUT_H) == 0x3418U, "CH2_RESIZER_OUT_H offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, CH2_RESIZER_STATUS) == 0x341CU, "CH2_RESIZER_STATUS offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, CH2_ODMA_Y_STRIDE) == 0x3480U, "CH2_ODMA_Y_STRIDE offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, CH2_ODMA_BUF_FREE) == 0x3488U, "CH2_ODMA_BUF_FREE offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, CH2_ODMA_BUF_DONE) == 0x348CU, "CH2_ODMA_BUF_DONE offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, CH2_ODMA_FRAME_COUNT) == 0x3490U, "CH2_ODMA_FRAME_COUNT offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, CH2_ODMA_Y_ADDR_L0) == 0x34A0U, "CH2_ODMA_Y_ADDR_L0 offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, CH2_ODMA_Y_ADDR_H0) == 0x34A4U, "CH2_ODMA_Y_ADDR_H0 offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, CH2_ODMA_Y_ADDR_L1) == 0x34B0U, "CH2_ODMA_Y_ADDR_L1 offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, CH2_ODMA_Y_ADDR_H1) == 0x34B4U, "CH2_ODMA_Y_ADDR_H1 offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, CH2_ODMA_Y_ADDR_L2) == 0x34C0U, "CH2_ODMA_Y_ADDR_L2 offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, CH2_ODMA_Y_ADDR_H2) == 0x34C4U, "CH2_ODMA_Y_ADDR_H2 offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, CH2_ODMA_Y_ADDR_L3) == 0x34D0U, "CH2_ODMA_Y_ADDR_L3 offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, CH2_ODMA_Y_ADDR_H3) == 0x34D4U, "CH2_ODMA_Y_ADDR_H3 offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, CH2_ODMA_BUF_TAG0) == 0x3500U, "CH2_ODMA_BUF_TAG0 offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, CH2_ODMA_BUF_TAG1) == 0x3504U, "CH2_ODMA_BUF_TAG1 offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, CH2_ODMA_BUF_TAG2) == 0x3508U, "CH2_ODMA_BUF_TAG2 offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, CH2_ODMA_BUF_TAG3) == 0x350CU, "CH2_ODMA_BUF_TAG3 offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, CH2_ODMA_BUF_TS0) == 0x3510U, "CH2_ODMA_BUF_TS0 offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, CH2_ODMA_BUF_TS1) == 0x3514U, "CH2_ODMA_BUF_TS1 offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, CH2_ODMA_BUF_TS2) == 0x3518U, "CH2_ODMA_BUF_TS2 offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, CH2_ODMA_BUF_TS3) == 0x351CU, "CH2_ODMA_BUF_TS3 offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, CH2_ODMA_BUF_SOF_TS0) == 0x3520U, "CH2_ODMA_BUF_SOF_TS0 offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, CH2_ODMA_BUF_SOF_TS1) == 0x3524U, "CH2_ODMA_BUF_SOF_TS1 offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, CH2_ODMA_BUF_SOF_TS2) == 0x3528U, "CH2_ODMA_BUF_SOF_TS2 offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, CH2_ODMA_BUF_SOF_TS3) == 0x352CU, "CH2_ODMA_BUF_SOF_TS3 offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, CH2_ODMA_BUF_EOF_TS0) == 0x3530U, "CH2_ODMA_BUF_EOF_TS0 offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, CH2_ODMA_BUF_EOF_TS1) == 0x3534U, "CH2_ODMA_BUF_EOF_TS1 offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, CH2_ODMA_BUF_EOF_TS2) == 0x3538U, "CH2_ODMA_BUF_EOF_TS2 offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, CH2_ODMA_BUF_EOF_TS3) == 0x353CU, "CH2_ODMA_BUF_EOF_TS3 offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_ID) == 0x8000U, "TUNE_ID offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_REV) == 0x8004U, "TUNE_REV offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_CAPS) == 0x8008U, "TUNE_CAPS offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_MODE) == 0x800CU, "TUNE_MODE offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_BANK) == 0x8010U, "TUNE_BANK offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_PROFILE_ID) == 0x8014U, "TUNE_PROFILE_ID offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_APPLY_FRAME) == 0x8018U, "TUNE_APPLY_FRAME offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_CMD) == 0x801CU, "TUNE_CMD offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_STATUS) == 0x8020U, "TUNE_STATUS offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_ACTIVE_ID) == 0x8024U, "TUNE_ACTIVE_ID offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_ACTIVE_FRAME) == 0x8028U, "TUNE_ACTIVE_FRAME offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_ERROR) == 0x802CU, "TUNE_ERROR offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_NEXT_FRAME) == 0x8030U, "TUNE_NEXT_FRAME offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_CH0_BUF0_PROFILE) == 0x8040U, "TUNE_CH0_BUF0_PROFILE offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_CH0_BUF1_PROFILE) == 0x8044U, "TUNE_CH0_BUF1_PROFILE offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_CH0_BUF2_PROFILE) == 0x8048U, "TUNE_CH0_BUF2_PROFILE offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_CH0_BUF3_PROFILE) == 0x804CU, "TUNE_CH0_BUF3_PROFILE offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_CH1_BUF0_PROFILE) == 0x8050U, "TUNE_CH1_BUF0_PROFILE offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_CH1_BUF1_PROFILE) == 0x8054U, "TUNE_CH1_BUF1_PROFILE offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_CH1_BUF2_PROFILE) == 0x8058U, "TUNE_CH1_BUF2_PROFILE offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_CH1_BUF3_PROFILE) == 0x805CU, "TUNE_CH1_BUF3_PROFILE offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_CH2_BUF0_PROFILE) == 0x8060U, "TUNE_CH2_BUF0_PROFILE offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_CH2_BUF1_PROFILE) == 0x8064U, "TUNE_CH2_BUF1_PROFILE offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_CH2_BUF2_PROFILE) == 0x8068U, "TUNE_CH2_BUF2_PROFILE offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_CH2_BUF3_PROFILE) == 0x806CU, "TUNE_CH2_BUF3_PROFILE offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_NR_CTRL) == 0x8100U, "TUNE_NR_CTRL offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_NR_VARIANCE) == 0x8104U, "TUNE_NR_VARIANCE offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_NR_SCALE) == 0x8108U, "TUNE_NR_SCALE offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_NR_LIMITS) == 0x810CU, "TUNE_NR_LIMITS offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_NR_LL_SCALE) == 0x8110U, "TUNE_NR_LL_SCALE offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_NR_LH_SCALE) == 0x8114U, "TUNE_NR_LH_SCALE offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_NR_HL_SCALE) == 0x8118U, "TUNE_NR_HL_SCALE offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_NR_HH_SCALE) == 0x811CU, "TUNE_NR_HH_SCALE offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_NR_BLEND) == 0x8120U, "TUNE_NR_BLEND offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_NR_LUT_ADDR) == 0x8124U, "TUNE_NR_LUT_ADDR offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_NR_LUT_DATA) == 0x8128U, "TUNE_NR_LUT_DATA offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_NR_SEED) == 0x812CU, "TUNE_NR_SEED offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_WB_GB) == 0x8200U, "TUNE_WB_GB offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_DEMOSAIC_MARGIN) == 0x8204U, "TUNE_DEMOSAIC_MARGIN offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_CNF_BLEND) == 0x8208U, "TUNE_CNF_BLEND offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_CSC_CTRL) == 0x8210U, "TUNE_CSC_CTRL offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_CSC_CONTRAST) == 0x8214U, "TUNE_CSC_CONTRAST offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_CSC_BRIGHTNESS) == 0x8218U, "TUNE_CSC_BRIGHTNESS offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_CSC_UU) == 0x8220U, "TUNE_CSC_UU offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_CSC_UV) == 0x8224U, "TUNE_CSC_UV offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_CSC_VU) == 0x8228U, "TUNE_CSC_VU offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_CSC_VV) == 0x822CU, "TUNE_CSC_VV offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_AWB_CTRL) == 0x8300U, "TUNE_AWB_CTRL offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_AWB_ROI_START) == 0x8304U, "TUNE_AWB_ROI_START offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_AWB_ROI_END) == 0x8308U, "TUNE_AWB_ROI_END offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_AWB_R_LIMITS) == 0x8310U, "TUNE_AWB_R_LIMITS offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_AWB_G_LIMITS) == 0x8314U, "TUNE_AWB_G_LIMITS offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_AWB_B_LIMITS) == 0x8318U, "TUNE_AWB_B_LIMITS offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_AWB_MIN_G) == 0x831CU, "TUNE_AWB_MIN_G offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_AWB_E0_CENTER) == 0x8340U, "TUNE_AWB_E0_CENTER offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_AWB_E0_A) == 0x8344U, "TUNE_AWB_E0_A offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_AWB_E0_B) == 0x8348U, "TUNE_AWB_E0_B offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_AWB_E0_C) == 0x834CU, "TUNE_AWB_E0_C offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_AWB_E0_RADIUS2) == 0x8350U, "TUNE_AWB_E0_RADIUS2 offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_AWB_E1_CENTER) == 0x8360U, "TUNE_AWB_E1_CENTER offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_AWB_E1_A) == 0x8364U, "TUNE_AWB_E1_A offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_AWB_E1_B) == 0x8368U, "TUNE_AWB_E1_B offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_AWB_E1_C) == 0x836CU, "TUNE_AWB_E1_C offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_AWB_E1_RADIUS2) == 0x8370U, "TUNE_AWB_E1_RADIUS2 offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_AWB_E2_CENTER) == 0x8380U, "TUNE_AWB_E2_CENTER offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_AWB_E2_A) == 0x8384U, "TUNE_AWB_E2_A offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_AWB_E2_B) == 0x8388U, "TUNE_AWB_E2_B offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_AWB_E2_C) == 0x838CU, "TUNE_AWB_E2_C offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_AWB_E2_RADIUS2) == 0x8390U, "TUNE_AWB_E2_RADIUS2 offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_AWB_E3_CENTER) == 0x83A0U, "TUNE_AWB_E3_CENTER offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_AWB_E3_A) == 0x83A4U, "TUNE_AWB_E3_A offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_AWB_E3_B) == 0x83A8U, "TUNE_AWB_E3_B offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_AWB_E3_C) == 0x83ACU, "TUNE_AWB_E3_C offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_AWB_E3_RADIUS2) == 0x83B0U, "TUNE_AWB_E3_RADIUS2 offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_AWB_E4_CENTER) == 0x83C0U, "TUNE_AWB_E4_CENTER offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_AWB_E4_A) == 0x83C4U, "TUNE_AWB_E4_A offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_AWB_E4_B) == 0x83C8U, "TUNE_AWB_E4_B offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_AWB_E4_C) == 0x83CCU, "TUNE_AWB_E4_C offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_AWB_E4_RADIUS2) == 0x83D0U, "TUNE_AWB_E4_RADIUS2 offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_AWB_E5_CENTER) == 0x83E0U, "TUNE_AWB_E5_CENTER offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_AWB_E5_A) == 0x83E4U, "TUNE_AWB_E5_A offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_AWB_E5_B) == 0x83E8U, "TUNE_AWB_E5_B offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_AWB_E5_C) == 0x83ECU, "TUNE_AWB_E5_C offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_AWB_E5_RADIUS2) == 0x83F0U, "TUNE_AWB_E5_RADIUS2 offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_AWB_E6_CENTER) == 0x8400U, "TUNE_AWB_E6_CENTER offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_AWB_E6_A) == 0x8404U, "TUNE_AWB_E6_A offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_AWB_E6_B) == 0x8408U, "TUNE_AWB_E6_B offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_AWB_E6_C) == 0x840CU, "TUNE_AWB_E6_C offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_AWB_E6_RADIUS2) == 0x8410U, "TUNE_AWB_E6_RADIUS2 offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_AWB_E7_CENTER) == 0x8420U, "TUNE_AWB_E7_CENTER offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_AWB_E7_A) == 0x8424U, "TUNE_AWB_E7_A offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_AWB_E7_B) == 0x8428U, "TUNE_AWB_E7_B offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_AWB_E7_C) == 0x842CU, "TUNE_AWB_E7_C offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_AWB_E7_RADIUS2) == 0x8430U, "TUNE_AWB_E7_RADIUS2 offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_AF_CTRL) == 0x8500U, "TUNE_AF_CTRL offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_AF_NOISE) == 0x8504U, "TUNE_AF_NOISE offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_AF_W0_START) == 0x8540U, "TUNE_AF_W0_START offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_AF_W0_END) == 0x8544U, "TUNE_AF_W0_END offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_AF_W1_START) == 0x8548U, "TUNE_AF_W1_START offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_AF_W1_END) == 0x854CU, "TUNE_AF_W1_END offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_AF_W2_START) == 0x8550U, "TUNE_AF_W2_START offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_AF_W2_END) == 0x8554U, "TUNE_AF_W2_END offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_AF_W3_START) == 0x8558U, "TUNE_AF_W3_START offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_AF_W3_END) == 0x855CU, "TUNE_AF_W3_END offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_AF_W4_START) == 0x8560U, "TUNE_AF_W4_START offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_AF_W4_END) == 0x8564U, "TUNE_AF_W4_END offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_AF_W5_START) == 0x8568U, "TUNE_AF_W5_START offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_AF_W5_END) == 0x856CU, "TUNE_AF_W5_END offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_AF_W6_START) == 0x8570U, "TUNE_AF_W6_START offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_AF_W6_END) == 0x8574U, "TUNE_AF_W6_END offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_AF_W7_START) == 0x8578U, "TUNE_AF_W7_START offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_AF_W7_END) == 0x857CU, "TUNE_AF_W7_END offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_AF_W8_START) == 0x8580U, "TUNE_AF_W8_START offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_AF_W8_END) == 0x8584U, "TUNE_AF_W8_END offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_AF_W9_START) == 0x8588U, "TUNE_AF_W9_START offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_AF_W9_END) == 0x858CU, "TUNE_AF_W9_END offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_AF_W10_START) == 0x8590U, "TUNE_AF_W10_START offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_AF_W10_END) == 0x8594U, "TUNE_AF_W10_END offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_AF_W11_START) == 0x8598U, "TUNE_AF_W11_START offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_AF_W11_END) == 0x859CU, "TUNE_AF_W11_END offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_AF_W12_START) == 0x85A0U, "TUNE_AF_W12_START offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_AF_W12_END) == 0x85A4U, "TUNE_AF_W12_END offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_AF_W13_START) == 0x85A8U, "TUNE_AF_W13_START offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_AF_W13_END) == 0x85ACU, "TUNE_AF_W13_END offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_AF_W14_START) == 0x85B0U, "TUNE_AF_W14_START offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_AF_W14_END) == 0x85B4U, "TUNE_AF_W14_END offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_AF_W15_START) == 0x85B8U, "TUNE_AF_W15_START offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_AF_W15_END) == 0x85BCU, "TUNE_AF_W15_END offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_HIST_CTRL) == 0x8600U, "TUNE_HIST_CTRL offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_HIST_START) == 0x8604U, "TUNE_HIST_START offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_HIST_END) == 0x8608U, "TUNE_HIST_END offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_HIST_STEP) == 0x860CU, "TUNE_HIST_STEP offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_HIST_WEIGHT0) == 0x8640U, "TUNE_HIST_WEIGHT0 offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_HIST_WEIGHT1) == 0x8644U, "TUNE_HIST_WEIGHT1 offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_HIST_WEIGHT2) == 0x8648U, "TUNE_HIST_WEIGHT2 offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_HIST_WEIGHT3) == 0x864CU, "TUNE_HIST_WEIGHT3 offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_HIST_WEIGHT4) == 0x8650U, "TUNE_HIST_WEIGHT4 offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_HIST_WEIGHT5) == 0x8654U, "TUNE_HIST_WEIGHT5 offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_HIST_WEIGHT6) == 0x8658U, "TUNE_HIST_WEIGHT6 offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_HIST_WEIGHT7) == 0x865CU, "TUNE_HIST_WEIGHT7 offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_HIST_WEIGHT8) == 0x8660U, "TUNE_HIST_WEIGHT8 offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_HIST_WEIGHT9) == 0x8664U, "TUNE_HIST_WEIGHT9 offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_HIST_WEIGHT10) == 0x8668U, "TUNE_HIST_WEIGHT10 offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_HIST_WEIGHT11) == 0x866CU, "TUNE_HIST_WEIGHT11 offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_HIST_WEIGHT12) == 0x8670U, "TUNE_HIST_WEIGHT12 offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_HIST_WEIGHT13) == 0x8674U, "TUNE_HIST_WEIGHT13 offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_HIST_WEIGHT14) == 0x8678U, "TUNE_HIST_WEIGHT14 offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_HIST_WEIGHT15) == 0x867CU, "TUNE_HIST_WEIGHT15 offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_HIST_WEIGHT16) == 0x8680U, "TUNE_HIST_WEIGHT16 offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_HIST_WEIGHT17) == 0x8684U, "TUNE_HIST_WEIGHT17 offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_HIST_WEIGHT18) == 0x8688U, "TUNE_HIST_WEIGHT18 offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_HIST_WEIGHT19) == 0x868CU, "TUNE_HIST_WEIGHT19 offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_HIST_WEIGHT20) == 0x8690U, "TUNE_HIST_WEIGHT20 offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_HIST_WEIGHT21) == 0x8694U, "TUNE_HIST_WEIGHT21 offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_HIST_WEIGHT22) == 0x8698U, "TUNE_HIST_WEIGHT22 offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_HIST_WEIGHT23) == 0x869CU, "TUNE_HIST_WEIGHT23 offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_HIST_WEIGHT24) == 0x86A0U, "TUNE_HIST_WEIGHT24 offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_AEC_CMD) == 0x8700U, "TUNE_AEC_CMD offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_AEC_STATUS) == 0x8704U, "TUNE_AEC_STATUS offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_AEC_DROPS) == 0x8708U, "TUNE_AEC_DROPS offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_AEC_FRAME) == 0x870CU, "TUNE_AEC_FRAME offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_AEC_PROFILE) == 0x8710U, "TUNE_AEC_PROFILE offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_AEC_INDEX) == 0x8714U, "TUNE_AEC_INDEX offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_AEC_DATA_LO) == 0x8718U, "TUNE_AEC_DATA_LO offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_AEC_DATA_HI) == 0x871CU, "TUNE_AEC_DATA_HI offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_AF_CMD) == 0x8740U, "TUNE_AF_CMD offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_AF_STATUS) == 0x8744U, "TUNE_AF_STATUS offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_AF_DROPS) == 0x8748U, "TUNE_AF_DROPS offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_AF_FRAME) == 0x874CU, "TUNE_AF_FRAME offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_AF_PROFILE) == 0x8750U, "TUNE_AF_PROFILE offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_AF_INDEX) == 0x8754U, "TUNE_AF_INDEX offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_AF_DATA_LO) == 0x8758U, "TUNE_AF_DATA_LO offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_AF_DATA_HI) == 0x875CU, "TUNE_AF_DATA_HI offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_AWB_PROFILE) == 0x8780U, "TUNE_AWB_PROFILE offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_GAMMA_PORT) == 0x8800U, "TUNE_GAMMA_PORT offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_GAMMA_DATA) == 0x8804U, "TUNE_GAMMA_DATA offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_TABLE_CMD) == 0x8808U, "TUNE_TABLE_CMD offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_TABLE_STATUS) == 0x880CU, "TUNE_TABLE_STATUS offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_BPC_CTRL) == 0x8900U, "TUNE_BPC_CTRL offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_BPC_G_THRESH) == 0x8904U, "TUNE_BPC_G_THRESH offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_BPC_RB_THRESH) == 0x8908U, "TUNE_BPC_RB_THRESH offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_BPC_COUNT) == 0x8910U, "TUNE_BPC_COUNT offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_BPC_ADDR) == 0x8914U, "TUNE_BPC_ADDR offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_BPC_DATA) == 0x8918U, "TUNE_BPC_DATA offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_LSC_CTRL) == 0x8A00U, "TUNE_LSC_CTRL offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_LSC_SECTOR_ADDR) == 0x8A04U, "TUNE_LSC_SECTOR_ADDR offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_LSC_SECTOR_DATA) == 0x8A08U, "TUNE_LSC_SECTOR_DATA offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, COMMON_INPUT_FORMAT) == 0x8B00U, "COMMON_INPUT_FORMAT offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_AEC_SOURCE) == 0x8B08U, "TUNE_AEC_SOURCE offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_AEC_RESULT_META) == 0x8B0CU, "TUNE_AEC_RESULT_META offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, CH0_CROP_CTRL) == 0x8B40U, "CH0_CROP_CTRL offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, CH0_CROP_ORIGIN) == 0x8B44U, "CH0_CROP_ORIGIN offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, CH0_CROP_SIZE) == 0x8B48U, "CH0_CROP_SIZE offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, CH0_GEOMETRY_STATUS) == 0x8B4CU, "CH0_GEOMETRY_STATUS offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, CH0_ACTIVE_CROP_ORIGIN) == 0x8B50U, "CH0_ACTIVE_CROP_ORIGIN offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, CH0_ACTIVE_CROP_SIZE) == 0x8B54U, "CH0_ACTIVE_CROP_SIZE offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, CH0_OUTPUT_LIMIT) == 0x8B58U, "CH0_OUTPUT_LIMIT offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, CH1_CROP_CTRL) == 0x8B80U, "CH1_CROP_CTRL offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, CH1_CROP_ORIGIN) == 0x8B84U, "CH1_CROP_ORIGIN offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, CH1_CROP_SIZE) == 0x8B88U, "CH1_CROP_SIZE offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, CH1_GEOMETRY_STATUS) == 0x8B8CU, "CH1_GEOMETRY_STATUS offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, CH1_ACTIVE_CROP_ORIGIN) == 0x8B90U, "CH1_ACTIVE_CROP_ORIGIN offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, CH1_ACTIVE_CROP_SIZE) == 0x8B94U, "CH1_ACTIVE_CROP_SIZE offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, CH1_OUTPUT_LIMIT) == 0x8B98U, "CH1_OUTPUT_LIMIT offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, CH2_CROP_CTRL) == 0x8BC0U, "CH2_CROP_CTRL offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, CH2_CROP_ORIGIN) == 0x8BC4U, "CH2_CROP_ORIGIN offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, CH2_CROP_SIZE) == 0x8BC8U, "CH2_CROP_SIZE offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, CH2_GEOMETRY_STATUS) == 0x8BCCU, "CH2_GEOMETRY_STATUS offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, CH2_ACTIVE_CROP_ORIGIN) == 0x8BD0U, "CH2_ACTIVE_CROP_ORIGIN offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, CH2_ACTIVE_CROP_SIZE) == 0x8BD4U, "CH2_ACTIVE_CROP_SIZE offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, CH2_OUTPUT_LIMIT) == 0x8BD8U, "CH2_OUTPUT_LIMIT offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, COMMON_RECOVERY_STATUS) == 0x8C00U, "COMMON_RECOVERY_STATUS offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, COMMON_RECOVERY_SEQUENCE) == 0x8C04U, "COMMON_RECOVERY_SEQUENCE offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, COMMON_ABORT0_FRAME) == 0x8C10U, "COMMON_ABORT0_FRAME offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, COMMON_ABORT0_OUTCOME) == 0x8C14U, "COMMON_ABORT0_OUTCOME offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, COMMON_ABORT1_FRAME) == 0x8C18U, "COMMON_ABORT1_FRAME offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, COMMON_ABORT1_OUTCOME) == 0x8C1CU, "COMMON_ABORT1_OUTCOME offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, COMMON_ABORT2_FRAME) == 0x8C20U, "COMMON_ABORT2_FRAME offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, COMMON_ABORT2_OUTCOME) == 0x8C24U, "COMMON_ABORT2_OUTCOME offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, COMMON_ABORT3_FRAME) == 0x8C28U, "COMMON_ABORT3_FRAME offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, COMMON_ABORT3_OUTCOME) == 0x8C2CU, "COMMON_ABORT3_OUTCOME offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_CH0_BUF0_FRAME) == 0x8C40U, "TUNE_CH0_BUF0_FRAME offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_CH0_BUF1_FRAME) == 0x8C44U, "TUNE_CH0_BUF1_FRAME offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_CH0_BUF2_FRAME) == 0x8C48U, "TUNE_CH0_BUF2_FRAME offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_CH0_BUF3_FRAME) == 0x8C4CU, "TUNE_CH0_BUF3_FRAME offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_CH1_BUF0_FRAME) == 0x8C50U, "TUNE_CH1_BUF0_FRAME offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_CH1_BUF1_FRAME) == 0x8C54U, "TUNE_CH1_BUF1_FRAME offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_CH1_BUF2_FRAME) == 0x8C58U, "TUNE_CH1_BUF2_FRAME offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_CH1_BUF3_FRAME) == 0x8C5CU, "TUNE_CH1_BUF3_FRAME offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_CH2_BUF0_FRAME) == 0x8C60U, "TUNE_CH2_BUF0_FRAME offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_CH2_BUF1_FRAME) == 0x8C64U, "TUNE_CH2_BUF1_FRAME offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_CH2_BUF2_FRAME) == 0x8C68U, "TUNE_CH2_BUF2_FRAME offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_CH2_BUF3_FRAME) == 0x8C6CU, "TUNE_CH2_BUF3_FRAME offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_AE_FRAME_SIZE) == 0x8C80U, "TUNE_AE_FRAME_SIZE offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_AE_GRID) == 0x8C84U, "TUNE_AE_GRID offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_AE_ZONE_SIZE) == 0x8C88U, "TUNE_AE_ZONE_SIZE offset mismatch");
FX1_ISP_STATIC_ASSERT(offsetof(FX1_ISP_Regs_t, TUNE_AWB_FRAME_SIZE) == 0x8C90U, "TUNE_AWB_FRAME_SIZE offset mismatch");
FX1_ISP_STATIC_ASSERT(sizeof(FX1_ISP_Regs_t) == 0x8C94U, "CSR map size mismatch");
#undef FX1_ISP_STATIC_ASSERT
#endif

#ifdef __cplusplus
} /* Kết thúc khối liên kết extern C. */
#endif
#endif /* Kết thúc bảo vệ header FTEL_FX1_ISP_REGMAP_V107_H. */
