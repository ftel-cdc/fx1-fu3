/*
 * Cấu hình và đọc thống kê sáng AEC: quyền đọc bank, tổng/count, vùng và histogram.
 * Hiện thực và luồng xử lý.
 */
#include "stats/aec/ftel_isp_aec.h"

/*
 * Thứ tự các trường trong bản ghi zone AEC tại 0x200+8*z (MAS, Bảng 194): tổng, số mẫu, OE, UE,
 * min, max, 0, 0.
 */
#define AEC_ZONE_REC_SUM    0u
#define AEC_ZONE_REC_COUNT  1u
#define AEC_ZONE_REC_OE     2u
#define AEC_ZONE_REC_UE     3u
#define AEC_ZONE_REC_MIN    4u
#define AEC_ZONE_REC_MAX    5u

/*
 * Kích thước zone danh định = ceil(active/n). Với n >= 1, phải thỏa (n-1)*dim < active <= n*dim.
 */
static uint16_t aec_zone_dim(uint16_t active, uint16_t n)
{
    if (n == 0u || active == 0u)
        return 0u;
    uint32_t dim = ((uint32_t)active + (uint32_t)n - 1u) / (uint32_t)n;
    if (dim == 0u)
        dim = 1u;
    return (uint16_t)dim;
}

/*
 * Trả true khi n zone kích thước dim phủ đủ active pixel, theo quy tắc tiếp nhận hình học của phần
 * cứng.
 */
static bool aec_grid_covers(uint32_t n, uint32_t dim, uint32_t active)
{
    return (n - 1u) * dim < active && active <= n * dim;
}

/*
 * Tìm số zone lớn nhất <= max_n sao cho kích thước suy ra phủ đủ active và vừa trường ZONE_SIZE 12
 * bit. active == 0 trả 0 và sẽ bị hàm kiểm tra cấu hình từ chối.
 */
static uint16_t aec_grid_for(uint16_t active, uint16_t max_n)
{
    uint32_t best = 0u;
    for (uint32_t n = 1u; n <= max_n; n++) {
        uint32_t dim = ((uint32_t)active + n - 1u) / n;
        if (dim == 0u)
            dim = 1u;
        if (dim > AEC_ZONE_DIM_MAX)
            continue;
        if (aec_grid_covers(n, dim, active))
            best = n;
    }
    return (uint16_t)best;
}

isp_status_t aec_validate_config(const aec_config_t *cfg,
                        uint16_t active_w, uint16_t active_h)
{
    if (!cfg) return ISP_E_PARAM;

    const aec_zone_config_t *z = &cfg->zones;
    if (z->num_zone_x < 1u || z->num_zone_x > AEC_NUM_ZONE_X_MAX) return ISP_E_PARAM;
    if (z->num_zone_y < 1u || z->num_zone_y > AEC_NUM_ZONE_Y_MAX) return ISP_E_PARAM;
    if (z->zone_width  < 1u || z->zone_width  > AEC_ZONE_DIM_MAX) return ISP_E_PARAM;
    if (z->zone_height < 1u || z->zone_height > AEC_ZONE_DIM_MAX) return ISP_E_PARAM;

    /* Lưới phải phủ đủ ảnh hữu dụng: (N-1)*dim < active <= N*dim. */
    uint32_t x_lo = (uint32_t)(z->num_zone_x - 1u) * z->zone_width;
    uint32_t x_hi = (uint32_t)z->num_zone_x * z->zone_width;
    uint32_t y_lo = (uint32_t)(z->num_zone_y - 1u) * z->zone_height;
    uint32_t y_hi = (uint32_t)z->num_zone_y * z->zone_height;

    if (!(x_lo < active_w && active_w <= x_hi)) return ISP_E_PARAM;
    if (!(y_lo < active_h && active_h <= y_hi)) return ISP_E_PARAM;

    if (cfg->sample_min_clip > AEC_SAMPLE_MAX ||
        cfg->sample_max_clip > AEC_SAMPLE_MAX) return ISP_E_PARAM;
    if (cfg->sample_min_clip > cfg->sample_max_clip) return ISP_E_PARAM;

    if (cfg->thresh_ue > AEC_SAMPLE_MAX ||
        cfg->thresh_oe > AEC_SAMPLE_MAX) return ISP_E_PARAM;
    if (cfg->thresh_ue >= cfg->thresh_oe) return ISP_E_PARAM;

    if (cfg->source != AEC_SOURCE_BPC && cfg->source != AEC_SOURCE_DG) return ISP_E_PARAM;

    return ISP_OK;
}

void aec_get_default_config(aec_config_t *cfg,
                            uint16_t active_w, uint16_t active_h)
{
    if (!cfg) return;

    cfg->enable          = false;      /* AEC_CTRL có giá trị sau reset 0 */
    cfg->source          = AEC_SOURCE_BPC;  /* TUNE_AEC_SOURCE có giá trị sau reset 0 */
    cfg->context_id      = 0u;         /* AEC_CONTEXT_ID có giá trị sau reset 0 */
    cfg->zones.num_zone_x = 32u;       /* AEC_ZONE_CFG có giá trị sau reset 0x20 */
    cfg->zones.num_zone_y = 24u;       /* AEC_ZONE_CFG có giá trị sau reset 0x18 */
    cfg->sample_min_clip = 0u;         /* AEC_SAMPLE_CLIP có giá trị sau reset 0x0 */
    cfg->sample_max_clip = 4095u;      /* AEC_SAMPLE_CLIP có giá trị sau reset 0xFFF */
    cfg->thresh_ue       = AEC_THRESH_UE_DEFAULT;   /* 272  */
    cfg->thresh_oe       = AEC_THRESH_OE_DEFAULT;   /* 3744 */

    /*
     * Suy ra kích thước và thu nhỏ lưới đến khi phủ ảnh hợp lệ. Chỉ ceil(W/32) chưa đủ: W=930 cho
     * dim=30, nhưng 31*30 == 930 vi phạm biên cuối; vì vậy phải giảm số zone.
     */
    cfg->zones.num_zone_x = (uint8_t)aec_grid_for(active_w, AEC_NUM_ZONE_X_MAX);
    cfg->zones.num_zone_y = (uint8_t)aec_grid_for(active_h, AEC_NUM_ZONE_Y_MAX);
    cfg->zones.zone_width  = aec_zone_dim(active_w, cfg->zones.num_zone_x);
    cfg->zones.zone_height = aec_zone_dim(active_h, cfg->zones.num_zone_y);
}

isp_status_t aec_configure(const aec_config_t *cfg,
                           uint16_t active_w, uint16_t active_h)
{
    if (aec_validate_config(cfg, active_w, active_h) != ISP_OK)
        return ISP_E_PARAM;

    isp_status_t st = isp_prepare_config();
    if (st != ISP_OK) return st;

    st = isp_check_tune_cap(TUNE_CAP_AEC_LEASE);
    if (st != ISP_OK) return st;

    const aec_zone_config_t *z = &cfg->zones;

    /*
     * Ở legacy, TUNE_AEC_SOURCE nằm trong tập cấu hình đệm của AEC; ghi nguồn đo tại đây rồi phát
     * COMMIT ở dưới để chốt cho SOF tiếp theo.
     */
    REG_UPD(R_TUNE_AEC_SOURCE, TUNE_AEC_SOURCE_MASK, (uint32_t)cfg->source);
    REG_WR(R_AEC_CONTEXT_ID, cfg->context_id);
    REG_UPD(R_AEC_ZONE_CFG, 0x1F3Fu,
            AEC_ZONE_CFG_NX(z->num_zone_x) | AEC_ZONE_CFG_NY(z->num_zone_y));
    REG_UPD(R_AEC_ZONE_SIZE, 0x0FFF0FFFu,
            AEC_ZONE_SIZE_W(z->zone_width) | AEC_ZONE_SIZE_H(z->zone_height));
    REG_UPD(R_AEC_SAMPLE_CLIP, 0x0FFF0FFFu,
            AEC_SAMPLE_CLIP_MIN(cfg->sample_min_clip) |
            AEC_SAMPLE_CLIP_MAX(cfg->sample_max_clip));
    REG_UPD(R_AEC_THRESH, 0x0FFF0FFFu,
            AEC_THRESH_UE(cfg->thresh_ue) | AEC_THRESH_OE(cfg->thresh_oe));

    /*
     * Ghi một lần để đặt bit bật và COMMIT cùng lúc. Cấu hình đệm được chuyển sang cấu hình xử lý ở
     * SOF đầu vào được tiếp nhận kế tiếp.
     */
    REG_WR(R_AEC_CTRL, (cfg->enable ? AEC_CTRL_EN : 0u) | AEC_CTRL_COMMIT);
    isp_memory_barrier();
    st = isp_config_status();
    if (st != ISP_OK) return st;
    if (!isp_reg_matches(R_TUNE_AEC_SOURCE, 1u, cfg->source) ||
        REG_RD(R_AEC_CONTEXT_ID) != cfg->context_id ||
        !isp_reg_matches(R_AEC_ZONE_CFG, 0x1f3fu, z->num_zone_x | ((uint32_t)z->num_zone_y << 8)) ||
        !isp_reg_matches(R_AEC_ZONE_SIZE, 0x0fff0fffu, z->zone_width | ((uint32_t)z->zone_height << 16)) ||
        !isp_reg_matches(R_AEC_SAMPLE_CLIP, 0x0fff0fffu, cfg->sample_min_clip | ((uint32_t)cfg->sample_max_clip << 16)) ||
        !isp_reg_matches(R_AEC_THRESH, 0x0fff0fffu, cfg->thresh_ue | ((uint32_t)cfg->thresh_oe << 16)) ||
        !isp_reg_matches(R_AEC_CTRL, 1u, cfg->enable ? 1u : 0u)) return ISP_E_IO;
    return ISP_OK;
}

isp_status_t aec_acquire(void)
{
    isp_status_t st = isp_check_tune_cap(TUNE_CAP_AEC_LEASE);
    if (st != ISP_OK) return st;

    if (aec_lease_valid()) return ISP_E_NOT_IDLE;

    /* Chỉ xin quyền đọc khi bên gọi chưa giữ một bank kết quả. */
    REG_WR(R_TUNE_AEC_CMD, TUNE_AEC_CMD_ACQUIRE);
    isp_memory_barrier();          /* Lệnh phải tới phần cứng trước khi đọc trạng thái. */

    return (REG_RD(R_TUNE_AEC_STATUS) & TUNE_AEC_STATUS_LEASE_VALID)
           ? ISP_OK : ISP_E_NOT_IDLE;
}

isp_status_t aec_release(void)
{
    isp_status_t rc = isp_check_tune_cap(TUNE_CAP_AEC_LEASE);
    if (rc != ISP_OK) return rc;
    if (!aec_lease_valid())
        return ISP_E_NOT_IDLE;

    REG_WR(R_TUNE_AEC_CMD, TUNE_AEC_CMD_RELEASE);
    isp_memory_barrier();
    return aec_lease_valid() ? ISP_E_IO : ISP_OK;
}

bool aec_lease_valid(void)
{
    return (REG_RD(R_TUNE_AEC_STATUS) & TUNE_AEC_STATUS_LEASE_VALID) != 0u;
}

/* Chốt và đọc một kết quả 64 bit từ bank đang giữ quyền đọc; đọc từ thấp trước. */
static isp_status_t aec_read_u64(uint16_t index, uint64_t *value)
{
    REG_WR(R_TUNE_AEC_INDEX, index);
    isp_barrier();      /* Phải ghi chỉ số trước khi đọc từ thấp. */
    if (!isp_reg_matches(R_TUNE_AEC_INDEX, 0xffffu, index)) return ISP_E_IO;
    uint32_t lo = REG_RD(R_TUNE_AEC_DATA_LO);
    uint32_t hi = REG_RD(R_TUNE_AEC_DATA_HI);
    if (!aec_lease_valid()) return ISP_E_NOT_IDLE;
    *value = ((uint64_t)hi << 32) | lo;
    return ISP_OK;
}
#define AEC_READ(index_, value_) do { \
    isp_status_t rc_ = aec_read_u64((index_), (value_)); \
    if (rc_ != ISP_OK) return rc_; \
} while (0)

/* Lưới dùng để tạo bank kết quả đang được giữ quyền đọc (TUNE_AE_GRID). */
static void aec_leased_grid(uint16_t *nx, uint16_t *ny)
{
    uint32_t grid = REG_RD(R_TUNE_AE_GRID);
    *nx = (uint16_t)(grid & 0x3Fu);
    *ny = (uint16_t)((grid >> 16) & 0x1Fu);
}

/* Cờ tràn được chốt theo từng kết quả: chỉ số 8, bit 0. */
static isp_status_t aec_result_overflow(bool *overflow)
{
    uint64_t flags = 0u;
    AEC_READ(AEC_IDX_OVERFLOW, &flags);
    *overflow = (flags & 1u) != 0;
    return ISP_OK;
}

isp_status_t aec_read_global(aec_global_stats_t *st)
{
    if (!st)
        return ISP_E_PARAM;

    isp_status_t rc = isp_check_tune_cap(TUNE_CAP_AEC_LEASE);
    if (rc != ISP_OK) return rc;

    if (!aec_lease_valid())
        return ISP_E_NOT_IDLE;

    st->frame_id   = REG_RD(R_TUNE_AEC_FRAME);
    st->context_id = REG_RD(R_AEC_RESULT_CONTEXT_ID);

    AEC_READ(AEC_IDX_GLOBAL_SUM, &st->global_sum);
    AEC_READ(AEC_IDX_GLOBAL_COUNT, &st->global_count);
    AEC_READ(AEC_IDX_GLOBAL_OE, &st->oe_count);
    AEC_READ(AEC_IDX_GLOBAL_UE, &st->ue_count);
    return aec_result_overflow(&st->overflow);
}

isp_status_t aec_read_zone(uint16_t zone_index, aec_zone_stats_t *st)
{
    if (!st)
        return ISP_E_PARAM;
    isp_status_t rc = isp_check_tune_cap(TUNE_CAP_AEC_LEASE);
    if (rc != ISP_OK) return rc;
    if (!aec_lease_valid())
        return ISP_E_NOT_IDLE;

    /*
     * Giới hạn chỉ số theo lưới đã tạo bank kết quả, thay vì chỉ dựa vào giới hạn kiến trúc.
     */
    uint16_t nx;
    uint16_t ny;
    aec_leased_grid(&nx, &ny);
    uint32_t grid_zones = (uint32_t)nx * (uint32_t)ny;
    if (grid_zones == 0u || grid_zones > AEC_NUM_ZONE_MAX ||
        nx > AEC_NUM_ZONE_X_MAX || ny > AEC_NUM_ZONE_Y_MAX)
        return ISP_E_NOT_IDLE;
    if ((uint32_t)zone_index >= grid_zones)
        return ISP_E_PARAM;

    /*
     * MAS 6.21.3.3: cửa sổ zone hẹp chỉ trả phép chiếu modulo (sum lấy 28 bit thấp, count/OE/UE lấy 16
     * bit thấp). Một zone có thể vượt 65.535 mẫu, nên phải đọc bản ghi 64 bit đầy đủ tại 0x200 + 8*z.
     */
    uint16_t base = (uint16_t)(AEC_IDX_ZONE_BASE + 8u * zone_index);

    uint64_t sum = 0u;
    uint64_t count = 0u;
    uint64_t oe = 0u;
    uint64_t ue = 0u;
    uint64_t min = 0u;
    uint64_t max = 0u;

    AEC_READ((uint16_t)(base + AEC_ZONE_REC_SUM),   &sum);
    AEC_READ((uint16_t)(base + AEC_ZONE_REC_COUNT), &count);
    AEC_READ((uint16_t)(base + AEC_ZONE_REC_OE),    &oe);
    AEC_READ((uint16_t)(base + AEC_ZONE_REC_UE),    &ue);
    AEC_READ((uint16_t)(base + AEC_ZONE_REC_MIN),   &min);
    AEC_READ((uint16_t)(base + AEC_ZONE_REC_MAX),   &max);

    st->sum        = sum;
    st->count      = count;
    st->oe_count   = oe;
    st->ue_count   = ue;
    st->min        = (uint16_t)min;
    st->max        = (uint16_t)max;
    st->empty      = (count == 0u);
    return aec_result_overflow(&st->overflow);
}

isp_status_t aec_set_source(aec_source_t source)
{
    if (source != AEC_SOURCE_BPC && source != AEC_SOURCE_DG)
        return ISP_E_PARAM;

    isp_status_t st = isp_prepare_config();
    if (st != ISP_OK) return st;

    /*
     * TUNE_AEC_SOURCE thuộc tập cấu hình đệm AEC ở legacy (MAS, Bảng 196). Nếu thiếu COMMIT, đường xử
     * lý chưa dùng nguồn đo vừa ghi.
     */
    REG_UPD(R_TUNE_AEC_SOURCE, TUNE_AEC_SOURCE_MASK, (uint32_t)source);
    /* Chỉ giữ EN; không phát lại các bit lệnh đọc về từ thanh ghi. */
    REG_WR(R_AEC_CTRL, (REG_RD(R_AEC_CTRL) & AEC_CTRL_EN) | AEC_CTRL_COMMIT);
    isp_memory_barrier();
    st = isp_config_status();
    if (st != ISP_OK) return st;
    return isp_reg_matches(R_TUNE_AEC_SOURCE, TUNE_AEC_SOURCE_MASK, (uint32_t)source)
           ? ISP_OK : ISP_E_IO;
}

isp_status_t aec_get_result_source(aec_source_t *source)
{
    if (!source)
        return ISP_E_PARAM;
    isp_status_t rc = isp_check_tune_cap(TUNE_CAP_AEC_LEASE);
    if (rc != ISP_OK) return rc;
    if (!aec_lease_valid())
        return ISP_E_NOT_IDLE;

    uint32_t meta = REG_RD(R_TUNE_AEC_RESULT_META);
    if (!(meta & 0x80000000u) || (meta & TUNE_AEC_META_DOMAIN) != 0x10u ||
        ((meta >> 8) & 3u) > 1u) return ISP_E_IO;
    *source = (meta & TUNE_AEC_META_SOURCE)
              ? AEC_SOURCE_DG : AEC_SOURCE_BPC;
    return ISP_OK;
}

isp_status_t aec_read_hist64(uint8_t bin, uint64_t *count)
{
    if (!count || bin >= AEC_HIST_BINS) return ISP_E_PARAM;
    isp_status_t rc = isp_check_tune_cap(TUNE_CAP_AEC_LEASE);
    if (rc != ISP_OK) return rc;
    if (!aec_lease_valid()) return ISP_E_NOT_IDLE;
    return aec_read_u64((uint16_t)(0x100u + bin), count);
}

isp_status_t aec_read_hist(uint8_t bin, uint32_t *count)
{
    if (!count) return ISP_E_PARAM;
    uint64_t value;
    isp_status_t rc = aec_read_hist64(bin, &value);
    if (rc == ISP_OK) *count = value > UINT32_MAX ? UINT32_MAX : (uint32_t)value;
    return rc;
}

isp_status_t aec_get_stats(aec_frame_stats_t *stats)
{
    if (!stats) return ISP_E_PARAM;
    isp_status_t rc = isp_check_tune_cap(TUNE_CAP_AEC_LEASE);
    if (rc != ISP_OK) return rc;
    uint32_t status = REG_RD(R_TUNE_AEC_STATUS);
    if (status & TUNE_AEC_STATUS_LEASE_VALID) return ISP_E_NOT_IDLE;
    if (!(status & TUNE_AEC_STATUS_READY_COUNT)) return ISP_E_NO_DATA;
    rc = aec_acquire();
    if (rc != ISP_OK) return rc;
    aec_frame_stats_t local = {0};
    rc = aec_read_global(&local.global);
    if (rc == ISP_OK) rc = aec_get_result_source(&local.source);
    for (unsigned i = 0; rc == ISP_OK && i < AEC_HIST_BINS; i++)
        rc = aec_read_hist64((uint8_t)i, &local.histogram[i]);
    if (rc == ISP_OK && (!aec_lease_valid() || REG_RD(R_TUNE_AEC_FRAME) != local.global.frame_id))
        rc = ISP_E_IO;
    isp_status_t released = aec_release();
    if (rc == ISP_OK) rc = released;
    if (rc == ISP_OK) *stats = local;
    return rc;
}
