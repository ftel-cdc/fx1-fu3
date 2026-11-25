/*
 * Cấu hình và đọc thống kê màu AWB: quyền đọc bank, tổng R/G/B và từng vùng.
 * Hiện thực và luồng xử lý.
 */
#include "stats/awb/ftel_isp_awb.h"

void awb_get_default_config(awb_stat_config_t *cfg)
{
    if (!cfg) return;

    cfg->enable             = false;          /* AWB_CTRL có giá trị sau reset 0 */
    cfg->num_zone_x         = 0u;             /* AWB_CTRL có giá trị sau reset 0 (chưa hợp lệ) */
    cfg->num_zone_y         = 0u;
    cfg->underexposed_limit = 0u;             /* AWB_UNDEREXPOSED_LIMIT có giá trị sau reset 0 */
    cfg->saturation_limit   = AEC_SAMPLE_MAX; /* AWB_SATURATION_LIMIT có giá trị sau reset 0xFFF */
    cfg->context_id         = 0u;             /* AWB_CONTEXT_ID có giá trị sau reset 0 */
}

/*
 * MAS 6.22.3.7: lưới bật phải thỏa 1 <= NX <= W/2 và 1 <= NY <= H/2 theo số ô Bayer, trong giới
 * hạn 64x32. Lưới sai làm bỏ phép đo, đặt OVERRUN và tăng DROP_COUNT dù giao dịch ghi AXI vẫn
 * thành công.
 */
isp_status_t awb_validate_config(const awb_stat_config_t *cfg,
                        uint16_t active_w, uint16_t active_h)
{
    if (!cfg)
        return ISP_E_PARAM;
    if (cfg->num_zone_x < 1u || cfg->num_zone_x > AWB_NUM_ZONE_X_MAX)
        return ISP_E_PARAM;
    if (cfg->num_zone_y < 1u || cfg->num_zone_y > AWB_NUM_ZONE_Y_MAX)
        return ISP_E_PARAM;
    if (cfg->underexposed_limit > AEC_SAMPLE_MAX ||
        cfg->saturation_limit   > AEC_SAMPLE_MAX)
        return ISP_E_PARAM;
    if (cfg->underexposed_limit >= cfg->saturation_limit)
        return ISP_E_PARAM;

    /* Lưới chỉ được có tối đa 2048 zone có thể đọc. */
    uint32_t zones = (uint32_t)cfg->num_zone_x * (uint32_t)cfg->num_zone_y;
    if (zones > AWB_NUM_ZONE_MAX)
        return ISP_E_PARAM;

    if (active_w != 0u) {
        uint32_t cells_x = (uint32_t)active_w / 2u;
        if ((uint32_t)cfg->num_zone_x > cells_x)
            return ISP_E_PARAM;
    }
    if (active_h != 0u) {
        uint32_t cells_y = (uint32_t)active_h / 2u;
        if ((uint32_t)cfg->num_zone_y > cells_y)
            return ISP_E_PARAM;
    }

    return ISP_OK;
}

isp_status_t awb_configure(const awb_stat_config_t *cfg,
                           uint16_t active_w, uint16_t active_h)
{
    if (awb_validate_config(cfg, active_w, active_h) != ISP_OK)
        return ISP_E_PARAM;

    isp_status_t st = isp_prepare_config();
    if (st != ISP_OK) return st;

    st = isp_check_tune_cap(TUNE_CAP_AWB_COLOR);
    if (st != ISP_OK) return st;

    REG_UPD(R_AWB_UNDEREXPOSED_LIMIT, AWB_LIMIT_MASK, cfg->underexposed_limit);
    REG_UPD(R_AWB_SATURATION_LIMIT,   AWB_LIMIT_MASK, cfg->saturation_limit);
    REG_WR(R_AWB_CONTEXT_ID, cfg->context_id);

    uint32_t ctrl = AWB_CTRL_NX(cfg->num_zone_x) | AWB_CTRL_NY(cfg->num_zone_y);
    if (cfg->enable) ctrl |= AWB_CTRL_EN;
    REG_UPD(R_AWB_CTRL, 0x3FFFu, ctrl);

    isp_barrier();
    st = isp_config_status();
    if (st != ISP_OK) return st;
    if (!isp_reg_matches(R_AWB_UNDEREXPOSED_LIMIT, 0xfffu, cfg->underexposed_limit) ||
        !isp_reg_matches(R_AWB_SATURATION_LIMIT, 0xfffu, cfg->saturation_limit) ||
        REG_RD(R_AWB_CONTEXT_ID) != cfg->context_id ||
        !isp_reg_matches(R_AWB_CTRL, 0x3fffu, ctrl)) return ISP_E_IO;
    return ISP_OK;
}

void awb_get_lease_status(awb_lease_status_t *st)
{
    if (!st) return;

    uint32_t status = REG_RD(R_AWB_RESULT_STATUS);
    st->ready_count = (uint8_t)(status & AWB_RESULT_STATUS_READY_COUNT);
    st->leased      = (status & AWB_RESULT_STATUS_LEASED) != 0u;
    st->overrun     = (status & AWB_RESULT_STATUS_OVERRUN) != 0u;
    st->num_zone_x  = (uint8_t)((status & AWB_RESULT_STATUS_RD_NX) >> 8);
    st->num_zone_y  = (uint8_t)((status & AWB_RESULT_STATUS_RD_NY) >> 16);
    st->drop_count  = REG_RD(R_AWB_DROP_COUNT);
}

isp_status_t awb_acquire(void)
{
    isp_status_t st = isp_check_tune_cap(TUNE_CAP_AWB_COLOR);
    if (st != ISP_OK) return st;

    if (REG_RD(R_AWB_RESULT_STATUS) & AWB_RESULT_STATUS_LEASED) return ISP_E_NOT_IDLE;

    /* Lệnh kéo dài một chu kỳ; không tác dụng nếu chưa có READY hoặc đã giữ quyền đọc. */
    REG_WR(R_AWB_RESULT_CTRL, AWB_RESULT_CTRL_ACQUIRE);
    isp_memory_barrier();          /* Lệnh phải tới phần cứng trước khi đọc trạng thái. */

    return (REG_RD(R_AWB_RESULT_STATUS) & AWB_RESULT_STATUS_READ_VALID)
           ? ISP_OK : ISP_E_NOT_IDLE;
}

isp_status_t awb_release(void)
{
    isp_status_t rc = isp_check_tune_cap(TUNE_CAP_AWB_COLOR);
    if (rc != ISP_OK) return rc;
    if (!(REG_RD(R_AWB_RESULT_STATUS) & AWB_RESULT_STATUS_LEASED))
        return ISP_E_NOT_IDLE;

    REG_WR(R_AWB_RESULT_CTRL, AWB_RESULT_CTRL_RELEASE);
    isp_memory_barrier();
    return (REG_RD(R_AWB_RESULT_STATUS) & AWB_RESULT_STATUS_LEASED) ? ISP_E_IO : ISP_OK;
}

/* Ghép từ cao/thấp; thanh ghi cao chứa các bit [34:32]. */
static uint64_t awb_join(uint32_t addr_h, uint32_t addr_l)
{
    uint64_t hi = REG_RD(addr_h) & 0x7u;
    uint64_t lo = REG_RD(addr_l);
    return (hi << 32) | lo;
}

isp_status_t awb_read_global(awb_global_stats_t *st)
{
    if (!st)
        return ISP_E_PARAM;

    isp_status_t rc = isp_check_tune_cap(TUNE_CAP_AWB_COLOR);
    if (rc != ISP_OK) return rc;

    if (!(REG_RD(R_AWB_RESULT_STATUS) & AWB_RESULT_STATUS_READ_VALID))
        return ISP_E_NOT_IDLE;

    st->frame_id   = REG_RD(R_AWB_FRAME_ID);
    st->context_id = REG_RD(R_AWB_RESULT_CONTEXT_ID);

    st->global.sum_r = awb_join(R_AWB_GLOBAL_SUM_R_H, R_AWB_GLOBAL_SUM_R_L);
    st->global.sum_g = awb_join(R_AWB_GLOBAL_SUM_G_H, R_AWB_GLOBAL_SUM_G_L);
    st->global.sum_b = awb_join(R_AWB_GLOBAL_SUM_B_H, R_AWB_GLOBAL_SUM_B_L);
    st->global.count = REG_RD(R_AWB_GLOBAL_COUNT) & 0x007FFFFFu;

    return (REG_RD(R_AWB_RESULT_STATUS) & AWB_RESULT_STATUS_READ_VALID) ? ISP_OK : ISP_E_NOT_IDLE;
}

isp_status_t awb_read_zone(uint16_t zone_index, awb_zone_stats_t *st)
{
    if (!st)
        return ISP_E_PARAM;

    isp_status_t rc = isp_check_tune_cap(TUNE_CAP_AWB_COLOR);
    if (rc != ISP_OK) return rc;

    uint32_t status = REG_RD(R_AWB_RESULT_STATUS);
    if (!(status & AWB_RESULT_STATUS_READ_VALID))
        return ISP_E_NOT_IDLE;

    /*
     * Dùng lưới đã chốt cùng bank đang giữ quyền đọc, không dùng cấu hình đang lập trình hoặc giới hạn
     * kiến trúc.
     */
    uint32_t nx = (status & AWB_RESULT_STATUS_RD_NX) >> 8;
    uint32_t ny = (status & AWB_RESULT_STATUS_RD_NY) >> 16;
    uint32_t grid_zones = nx * ny;

    if (grid_zones == 0u || nx > AWB_NUM_ZONE_X_MAX || ny > AWB_NUM_ZONE_Y_MAX)
        return ISP_E_NOT_IDLE;
    if ((uint32_t)zone_index >= grid_zones)
        return ISP_E_PARAM;
    /* zone_addr chỉ có 11 bit; chỉ số lớn hơn sẽ ánh xạ nhầm sang zone khác. */
    if ((uint32_t)zone_index > AWB_ZONE_INDEX_MAX)
        return ISP_E_PARAM;

    REG_UPD(R_AWB_ZONE_ADDR, AWB_ZONE_ADDR_MASK, zone_index);
    isp_barrier();

    /*
     * Đặc tả yêu cầu đợi một chu kỳ lõi sau khi chọn địa chỉ. Đọc lại địa chỉ tạo giao dịch bus kế
     * tiếp để bộ chọn dữ liệu kịp chuyển và kiểm tra giá trị đã ghi.
     */
    if (!isp_reg_matches(R_AWB_ZONE_ADDR, AWB_ZONE_ADDR_MASK, zone_index)) return ISP_E_IO;

    st->sum_r = awb_join(R_AWB_ZONE_SUM_R_H, R_AWB_ZONE_SUM_R_L);
    st->sum_g = awb_join(R_AWB_ZONE_SUM_G_H, R_AWB_ZONE_SUM_G_L);
    st->sum_b = awb_join(R_AWB_ZONE_SUM_B_H, R_AWB_ZONE_SUM_B_L);
    st->count = REG_RD(R_AWB_ZONE_COUNT) & 0x007FFFFFu;

    return (REG_RD(R_AWB_RESULT_STATUS) & AWB_RESULT_STATUS_READ_VALID) ? ISP_OK : ISP_E_NOT_IDLE;
}

void awb_clear_report(void)
{
    if (isp_check_tune_cap(TUNE_CAP_AWB_COLOR) != ISP_OK) return;
    REG_WR(R_AWB_RESULT_CTRL, AWB_RESULT_CTRL_CLEAR_REPORT);
    isp_memory_barrier();
}

isp_status_t awb_get_stats(awb_frame_stats_t *stats)
{
    if (!stats) return ISP_E_PARAM;
    isp_status_t rc = isp_check_tune_cap(TUNE_CAP_AWB_COLOR);
    if (rc != ISP_OK) return rc;
    uint32_t status = REG_RD(R_AWB_RESULT_STATUS);
    if (status & AWB_RESULT_STATUS_LEASED) return ISP_E_NOT_IDLE;
    if (!(status & AWB_RESULT_STATUS_READY_COUNT)) return ISP_E_NO_DATA;
    rc = awb_acquire();
    if (rc != ISP_OK) {
        /*
         * Trước ACQUIRE chưa có quyền đọc. Nếu lệnh đã có tác dụng nhưng READ_VALID đọc lại lỗi, quyền vừa
         * cấp thuộc lời gọi này và phải được giải phóng.
         */
        if (REG_RD(R_AWB_RESULT_STATUS) & AWB_RESULT_STATUS_LEASED) {
            isp_status_t released = awb_release();
            if (released != ISP_OK) return released;
        }
        return rc;
    }
    awb_frame_stats_t local = {0};
    rc = awb_read_global(&local.global);
    status = REG_RD(R_AWB_RESULT_STATUS);
    local.overrun = (status & AWB_RESULT_STATUS_OVERRUN) != 0;
    local.drop_count = REG_RD(R_AWB_DROP_COUNT);
    if (rc == ISP_OK && (!(status & AWB_RESULT_STATUS_READ_VALID) ||
        REG_RD(R_AWB_FRAME_ID) != local.global.frame_id)) rc = ISP_E_IO;
    isp_status_t released = awb_release();
    if (rc == ISP_OK) rc = released;
    if (rc == ISP_OK) *stats = local;
    return rc;
}
