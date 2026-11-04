/*
 * Cấu hình RAW10/RAW12, pha Bayer và kích thước hữu dụng sau chuẩn hóa về RGGB.
 * Hiện thực và luồng xử lý.
 */
#include "bayer domain/ifmt/ftel_isp_ifmt.h"

/*
 * Đổi pha Bayer thành số cột/hàng đầu phải bỏ, theo Bảng 20 của MAS để chuẩn hóa đầu ra RGGB.
 */
static void ifmt_bayer_offsets(bayer_pattern_t bayer, uint8_t *skip_columns, uint8_t *skip_rows)
{
    *skip_columns = ((uint8_t)bayer) & 0x1u;
    *skip_rows = (((uint8_t)bayer) >> 1) & 0x1u;
}

int ifmt_active_size(const ifmt_config_t *cfg,
                     uint16_t *active_w, uint16_t *active_h)
{
    if (!cfg || (cfg->width & 1u) || cfg->width < IFMT_W_MIN || cfg->width > IFMT_W_MAX ||
        (cfg->height & 1u) || cfg->height < IFMT_H_MIN || cfg->height > IFMT_H_MAX ||
        (cfg->bayer != BAYER_RGGB && cfg->bayer != BAYER_GRBG &&
         cfg->bayer != BAYER_GBRG && cfg->bayer != BAYER_BGGR) ||
        cfg->depth > INPUT_DEPTH_RAW10) return -1;

    uint8_t skip_columns;
    uint8_t skip_rows;
    ifmt_bayer_offsets(cfg->bayer, &skip_columns, &skip_rows);

    /*
     * Kích thước ngoài đã kiểm tra chẵn và đúng giới hạn, nên phép trừ độ lệch không bị âm/tràn.
     */
    uint16_t active_width = (uint16_t)((cfg->width  - skip_columns) & ~1u);
    uint16_t active_height = (uint16_t)((cfg->height - skip_rows) & ~1u);
    if (active_width < IFMT_W_MIN || active_width > IFMT_ACTIVE_W_MAX ||
        active_height < IFMT_H_MIN || active_height > IFMT_ACTIVE_H_MAX) return -1;

    if (active_w != NULL) {
        *active_w = active_width;
    }
    if (active_h != NULL) {
        *active_h = active_height;
    }
    return 0;
}

isp_status_t ifmt_validate_config(const ifmt_config_t *c)
{
    return ifmt_active_size(c, 0, 0) == 0 ? ISP_OK : ISP_E_PARAM;
}

void ifmt_get_default_config(ifmt_config_t *cfg)
{
    if (cfg == NULL) {
        return;
    }
    cfg->width  = 1920;                /* COMMON_FRAME_WIDTH có giá trị sau reset 0x780 */
    cfg->height = 1080;                /* COMMON_FRAME_HEIGHT có giá trị sau reset 0x438 */
    cfg->bayer  = BAYER_RGGB;          /* COMMON_BAYER có giá trị sau reset 0x0 */
    cfg->depth  = INPUT_DEPTH_RAW12;   /* COMMON_INPUT_FORMAT có giá trị sau reset 0x0 */
}

isp_status_t ifmt_configure(const ifmt_config_t *cfg)
{
    if (ifmt_validate_config(cfg) != ISP_OK)
        return ISP_E_PARAM;

    isp_status_t st = isp_require_legacy();
    if (st != ISP_OK) return st;

    /*
     * Chỉ đổi miền đầu vào khi legacy đã dừng và xử lý hết dữ liệu; chế độ profile từ chối bằng
     * TUNE_ERROR.MODE.
     */
    if (isp_in_profile_mode())
        return ISP_E_NOT_IDLE;

    /* Đổi miền đầu vào: bắt buộc dừng xếp dữ liệu và chờ xử lý hết. */
    st = isp_stop_and_wait(ISP_IDLE_DRAIN_TIMEOUT);
    if (st != ISP_OK) return st;

    /*
     * Bốn thanh ghi này thuộc lớp IDLE (MAS 9.9.3). Khi chưa rỗi, phần cứng giữ giá trị cũ và ghi
     * TUNE_ERROR dù AXI vẫn trả OKAY. Xóa lỗi đã chốt trước, ghi rồi kiểm tra lỗi và đọc lại mọi
     * trường.
     */
    st = isp_tune_error_clear();
    if (st != ISP_OK) return st;

    /* Thứ tự theo đặc tả: định dạng, pha Bayer, chiều rộng, chiều cao. */
    REG_UPD(R_COMMON_INPUT_FORMAT, 0x3u,    (uint32_t)cfg->depth);
    REG_UPD(R_COMMON_BAYER,        0x3u,    (uint32_t)cfg->bayer);
    REG_UPD(R_COMMON_FRAME_WIDTH,  0xFFFFu, (uint32_t)cfg->width);
    REG_UPD(R_COMMON_FRAME_HEIGHT, 0xFFFFu, (uint32_t)cfg->height);

    isp_barrier();

    if (isp_tune_error_set())
        return ISP_E_NOT_IDLE;

    if (!isp_reg_matches(R_COMMON_INPUT_FORMAT, 0x3u,    (uint32_t)cfg->depth)  ||
        !isp_reg_matches(R_COMMON_BAYER,        0x3u,    (uint32_t)cfg->bayer)  ||
        !isp_reg_matches(R_COMMON_FRAME_WIDTH,  0xFFFFu, (uint32_t)cfg->width)  ||
        !isp_reg_matches(R_COMMON_FRAME_HEIGHT, 0xFFFFu, (uint32_t)cfg->height))
        return ISP_E_NOT_IDLE;

    /*
     * Đổi đầu vào làm mất hiệu lực các bộ LSC/EE và kích thước đầu ra phụ thuộc. Bên gọi phải nạp/xác
     * nhận lại dữ liệu, kiểm tra hình học rồi mới cấp quyền DMA. SEAL/APPLY chỉ áp dụng khi triển khai
     * luồng profile riêng.
     */
    return ISP_OK;
}
