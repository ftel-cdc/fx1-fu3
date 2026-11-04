/*
 * Cấu hình bù sáng ống kính và nạp/xác nhận các bảng hệ số lưới lưu trú.
 * Hiện thực và luồng xử lý.
 */
#include "bayer domain/lsc/ftel_isp_lsc.h"

/*
 * active_w/active_h = 0 bỏ kiểm tra lưới không vượt kích thước ảnh; chỉ dùng nội bộ khi nạp lại
 * lưới đã cấu hình.
 */
static int lsc_validate(const lsc_mesh_config_t *cfg,
                        uint16_t active_w, uint16_t active_h)
{
    if (!cfg) return -1;
    if (cfg->nx < LSC_MESH_N_MIN || cfg->nx > LSC_MESH_N_MAX) return -1;
    if (cfg->ny < LSC_MESH_N_MIN || cfg->ny > LSC_MESH_N_MAX) return -1;
    /* Lưới không được vượt kích thước ảnh hữu dụng. */
    if (active_w != 0u && cfg->nx > active_w) return -1;
    if (active_h != 0u && cfg->ny > active_h) return -1;
    if (cfg->strength > LSC_STRENGTH_UNITY) return -1;
    return 0;
}

static void lsc_abort_load(uint8_t profile)
{
    REG_WR(R_LSC_LOAD_CTRL,
           LSC_LOAD_CTRL_PROFILE(profile) | LSC_LOAD_CTRL_ABORT);
    isp_barrier();
}

static bool lsc_wait_valid(const isp_wait_t *wait)
{
    return wait == NULL || (wait->poll_limit != 0u &&
           (wait->time_us == NULL || wait->timeout_us != 0u));
}

static isp_status_t lsc_capture(lsc_result_t *result, isp_status_t status)
{
    result->status = status;
    result->load_status = REG_RD(R_LSC_LOAD_STATUS);
    result->profile_status = REG_RD(R_LSC_PROFILE_STATUS);
    result->errors = REG_RD(R_LSC_ERROR) & LSC_ERROR_STICKY_MASK;
    return status;
}

/* Mỗi bước có giới hạn riêng; dừng ngay khi phần cứng báo lỗi thay vì chờ hết hạn. */
static isp_status_t lsc_wait_load(uint32_t mask, uint32_t value,
                                 const isp_wait_t *wait)
{
    uint64_t start = wait->time_us ? wait->time_us(wait->user) : 0;
    for (uint32_t poll = 0; poll < wait->poll_limit; poll++) {
        if (REG_RD(R_LSC_ERROR) & LSC_ERROR_STICKY_MASK)
            return ISP_E_IO;
        isp_status_t status = isp_config_status();
        if (status != ISP_OK) return status;
        if (isp_reg_matches(R_LSC_LOAD_STATUS, mask, value)) return ISP_OK;
        if (wait->time_us && wait->time_us(wait->user) - start >= wait->timeout_us)
            break;
    }
    return ISP_E_TIMEOUT;
}

/* Trả true khi giá trị đọc về dưới mặt nạ mask khớp field. */
static bool lsc_field_is(uint32_t addr, uint32_t mask, uint32_t field)
{
    return (REG_RD(addr) & mask) == (field & mask);
}

void lsc_get_default_config(lsc_mesh_config_t *cfg)
{
    if (!cfg) return;
    cfg->nx       = 0;                      /* LSC_MESH_NODES có giá trị sau reset 0 (chưa hợp lệ) */
    cfg->ny       = 0;
    cfg->strength = 0;                      /* LSC_STRENGTH có giá trị sau reset 0 */
}

isp_status_t lsc_validate_config(const lsc_mesh_config_t *cfg,
                                 uint16_t active_w, uint16_t active_h)
{
    if (!active_w || !active_h) return ISP_E_PARAM;
    return lsc_validate(cfg, active_w, active_h) == 0 ? ISP_OK : ISP_E_PARAM;
}

static isp_status_t lsc_configure_wait(const lsc_mesh_config_t *cfg,
                                      uint16_t active_w, uint16_t active_h,
                                      const isp_wait_t *wait)
{
    if (lsc_validate_config(cfg, active_w, active_h) != ISP_OK)
        return ISP_E_PARAM;

    isp_status_t st = isp_require_legacy();
    if (st != ISP_OK) return st;

    /*
     * Hình học lưới dùng chung ngoài bank tuning; chỉ ghi sau khi thoát profile và xử lý hết dữ liệu.
     */
    if (isp_in_profile_mode())
        return ISP_E_NOT_IDLE;

    /* Chờ xử lý hết dữ liệu trước khi tắt, dùng cùng clock/giới hạn do bên gọi cấp. */
    uint64_t start = wait->time_us ? wait->time_us(wait->user) : 0;
    uint32_t poll = 0;
    while (isp_not_idle()) {
        if (poll++ >= wait->poll_limit ||
            (wait->time_us && wait->time_us(wait->user) - start >= wait->timeout_us))
            return ISP_E_TIMEOUT;
    }
    REG_WR(R_DMA_CTRL, REG_RD(R_DMA_CTRL) & DMA_CTRL_BURST_MASK);
    isp_barrier();
    st = isp_clear_common_ctrl();
    if (st != ISP_OK) return st;

    /*
     * LSC_MESH_NODES thuộc lớp IDLE (MAS 9.9.3). Ghi bị từ chối giữ hình học cũ và chỉ đặt TUNE_ERROR;
     * cần xóa lỗi, ghi rồi kiểm tra lỗi và đọc lại.
     */
    st = isp_tune_error_clear();
    if (st != ISP_OK) return st;

    REG_UPD(R_LSC_STRENGTH,   LSC_STRENGTH_MASK,
            cfg->strength & LSC_STRENGTH_MASK);
    REG_UPD(R_LSC_MESH_NODES, 0xFFFFu,
            LSC_MESH_NX(cfg->nx) | LSC_MESH_NY(cfg->ny));

    isp_barrier();

    if (isp_tune_error_set())
        return ISP_E_NOT_IDLE;

    if (!lsc_field_is(R_LSC_MESH_NODES, 0xFFFFu,
                      LSC_MESH_NX(cfg->nx) | LSC_MESH_NY(cfg->ny)) ||
        !lsc_field_is(R_LSC_STRENGTH, LSC_STRENGTH_MASK, cfg->strength))
        return ISP_E_NOT_IDLE;

    return isp_wait_register(R_LSC_PROFILE_STATUS, LSC_PROFILE_STATUS_GEOM_OK,
                             LSC_PROFILE_STATUS_GEOM_OK, wait);
}

isp_status_t lsc_configure(const lsc_mesh_config_t *cfg,
                           uint16_t active_w, uint16_t active_h)
{
    isp_wait_t wait;
    isp_get_default_wait(&wait);
    return lsc_configure_wait(cfg, active_w, active_h, &wait);
}

uint32_t lsc_coef_count(const lsc_mesh_config_t *cfg)
{
    if (!cfg) return 0u;
    return (uint32_t)cfg->nx * (uint32_t)cfg->ny * LSC_COEFS_PER_NODE;
}

/* Đọc kích thước lưới đã lập trình từ phần cứng. */
static int lsc_programmed_mesh(uint8_t *nx, uint8_t *ny)
{
    uint32_t word = REG_RD(R_LSC_MESH_NODES);
    *nx = (uint8_t)(word & 0xFFu);
    *ny = (uint8_t)((word >> 8) & 0xFFu);
    if (*nx < LSC_MESH_N_MIN || *nx > LSC_MESH_N_MAX) return -1;
    if (*ny < LSC_MESH_N_MIN || *ny > LSC_MESH_N_MAX) return -1;
    return 0;
}

isp_status_t lsc_load_profile_ex(uint8_t profile, const lsc_mesh_config_t *cfg,
                                 const uint32_t *coefs, const isp_wait_t *wait,
                                 lsc_result_t *result)
{
    lsc_result_t local = {0};
    if (result == NULL) result = &local;
    *result = local;
    result->stage = LSC_STAGE_PREPARE;
    result->profile = profile;
    result->status = ISP_E_PARAM;
    if (!lsc_wait_valid(wait) || lsc_validate(cfg, 0u, 0u) != 0 || !coefs ||
        profile > LSC_PROFILE_MAX)
        return ISP_E_PARAM;

    isp_wait_t fallback;
    if (wait == NULL) {
        isp_get_default_wait(&fallback);
        wait = &fallback;
    }

    isp_status_t st = isp_require_legacy();
    if (st != ISP_OK) { result->status = st; return st; }

    /*
     * Số hệ số yêu cầu lấy từ hình học dùng chung đã ghi vào LSC_MESH_NODES. Bộ nạp so số đã ghi với
     * giá trị đó; khác nhau đặt COEF_COUNT.
     */
    uint8_t mesh_nx;
    uint8_t mesh_ny;
    if (lsc_programmed_mesh(&mesh_nx, &mesh_ny) != 0)
        return lsc_capture(result, ISP_E_NOT_IDLE);
    if (mesh_nx != cfg->nx || mesh_ny != cfg->ny)
        return ISP_E_PARAM;
    if (lsc_coef_count(cfg) != (uint32_t)mesh_nx * mesh_ny * LSC_COEFS_PER_NODE)
        return ISP_E_PARAM;

    uint32_t count = lsc_coef_count(cfg);
    result->expected_count = count;
    /*
     * Kiểm tra mọi hệ số trước BEGIN. Thoát giữa lần nạp làm chuỗi còn mở và BEGIN sau lỗi
     * LOAD_PROTOCOL. Phần cứng vẫn đếm/báo hệ số ngoài dải, nhưng driver từ chối trước để tránh nạp
     * dở.
     */
    for (uint32_t i = 0; i < count; i++) {
        if (coefs[i] > LSC_COEF_MAX)
            return ISP_E_PARAM;
    }

    /*
     * Nếu lần nạp trước còn mở, BEGIN mới sẽ bị từ chối với LOAD_PROTOCOL. Phát ABORT và chờ về rỗi
     * trước khi bắt đầu lại.
     */
    if (REG_RD(R_LSC_LOAD_STATUS) & LSC_LOAD_STATUS_BUSY) {
        lsc_abort_load(profile);
        st = isp_wait_register(R_LSC_LOAD_STATUS, LSC_LOAD_STATUS_BUSY, 0, wait);
        if (st != ISP_OK) return lsc_capture(result, st);
    }

    /*
     * Ở legacy, profile đang hoạt động là LSC_PROFILE_SEL khi LSC_CTRL.EN và ACTIVE_VALID cùng bật.
     * Sau reset, SEL=0 nhưng chưa có profile hợp lệ; profile được chọn nhưng chưa hợp lệ vẫn có thể
     * nạp.
     */
    bool lsc_en       = (REG_RD(R_LSC_CTRL) & LSC_CTRL_EN) != 0u;
    uint8_t act_sel   = (uint8_t)(REG_RD(R_LSC_PROFILE_SEL) &
                                  LSC_PROFILE_SEL_PROFILE);
    bool active_valid = (REG_RD(R_LSC_PROFILE_STATUS) & LSC_PROFILE_STATUS_ACTIVE_OK) != 0u;

    if (lsc_en && active_valid && profile == act_sel)
        return lsc_capture(result, ISP_E_NOT_IDLE);


    /*
     * LSC_ERROR là lỗi chốt W1C; xóa lỗi cũ trước để không nhầm một lần nạp tốt là thất bại.
     */
    REG_WR(R_LSC_ERROR, LSC_ERROR_STICKY_MASK);
    isp_barrier();
    st = isp_tune_error_clear();
    if (st != ISP_OK) return lsc_capture(result, st);

    result->stage = LSC_STAGE_BEGIN;
    REG_WR(R_LSC_LOAD_CTRL,
           LSC_LOAD_CTRL_PROFILE(profile) | LSC_LOAD_CTRL_BEGIN);
    isp_barrier();

    /* BEGIN bị từ chối thì không ghi DATA, tránh phát sinh thêm LOAD_PROTOCOL. */
    st = lsc_wait_load(LSC_LOAD_STATUS_BUSY | LSC_LOAD_STATUS_PROFILE,
                        LSC_LOAD_STATUS_BUSY | ((uint32_t)profile << 18), wait);
    if (st != ISP_OK) {
        lsc_capture(result, st);
        if (result->load_status & LSC_LOAD_STATUS_BUSY) lsc_abort_load(profile);
        return st;
    }

    result->stage = LSC_STAGE_DATA;
    for (uint32_t i = 0; i < count; i++)
        REG_WR(R_LSC_COEF_DATA, coefs[i] & LSC_COEF_DATA_MASK);
    isp_barrier();

    /*
     * LOAD_FULL bật khi đủ số hệ số, trước VALIDATE. Thiếu FULL nghĩa là số được nhận chưa đúng; hủy
     * thay vì xác nhận bảng thiếu.
     */
    result->stage = LSC_STAGE_FULL;
    st = lsc_wait_load(LSC_LOAD_STATUS_FULL, LSC_LOAD_STATUS_FULL, wait);
    if (st == ISP_OK && (REG_RD(R_LSC_LOAD_STATUS) & LSC_LOAD_STATUS_ADDR) != count)
        st = ISP_E_IO;
    if (st != ISP_OK) {
        lsc_capture(result, st);
        lsc_abort_load(profile);
        return st;
    }

    result->stage = LSC_STAGE_VALIDATE;
    REG_WR(R_LSC_LOAD_CTRL,
           LSC_LOAD_CTRL_PROFILE(profile) | LSC_LOAD_CTRL_VALIDATE);
    isp_barrier();

    /*
     * Chờ VALIDATE hoàn tất; LOAD_BUSY bao phủ cả chuỗi, chỉ kết thúc khi bit này hạ.
     */
    st = lsc_wait_load(LSC_LOAD_STATUS_BUSY, 0, wait);
    if (st != ISP_OK) {
        lsc_capture(result, st);
        lsc_abort_load(profile);
        return st;
    }

    /*
     * Kết quả quyết định: VALIDATE chỉ đặt bit VALID của profile khi đủ hệ số và không có lỗi nạp.
     */
    if (!(REG_RD(R_LSC_PROFILE_STATUS) & LSC_PROFILE_STATUS_VALID(profile)))
        return lsc_capture(result, ISP_E_IO);

    st = isp_config_status();
    if (st == ISP_OK) result->stage = LSC_STAGE_READY;
    return lsc_capture(result, st);
}

isp_status_t lsc_load_profile(uint8_t profile, const lsc_mesh_config_t *cfg,
                              const uint32_t *coefs)
{
    return lsc_load_profile_ex(profile, cfg, coefs, NULL, NULL);
}

isp_status_t lsc_set_profile(uint8_t profile, bool enable)
{
    if (profile > LSC_PROFILE_MAX)
        return ISP_E_PARAM;

    isp_status_t st = isp_prepare_config();
    if (st != ISP_OK) return st;

    /* Legacy từ chối chọn profile chưa nạp và đặt profile_sel_reject. */
    if (!(REG_RD(R_LSC_PROFILE_STATUS) & LSC_PROFILE_STATUS_VALID(profile)))
        return ISP_E_NOT_IDLE;

    REG_WR(R_LSC_ERROR, LSC_ERROR_PROFILE_SEL_REJECT);
    isp_barrier();
    REG_UPD(R_LSC_PROFILE_SEL, LSC_PROFILE_SEL_PROFILE, (uint32_t)profile);

    isp_barrier();
    if (REG_RD(R_LSC_ERROR) & LSC_ERROR_PROFILE_SEL_REJECT) return ISP_E_IO;
    if (!isp_reg_matches(R_LSC_PROFILE_SEL, LSC_PROFILE_SEL_PROFILE, profile)) return ISP_E_IO;
    if (enable) REG_SET(R_LSC_CTRL, LSC_CTRL_EN);
    else        REG_CLR(R_LSC_CTRL, LSC_CTRL_EN);

    isp_barrier();
    st = isp_config_status();
    if (st != ISP_OK) return st;
    if (!isp_reg_matches(R_LSC_PROFILE_SEL, 3u, profile) ||
        !isp_reg_matches(R_LSC_CTRL, 1u, enable ? 1u : 0u)) return ISP_E_IO;
    return ISP_OK;
}

uint32_t lsc_get_errors(bool write_one_clear)
{
    uint32_t err = REG_RD(R_LSC_ERROR) & LSC_ERROR_STICKY_MASK;
    if (write_one_clear && err)
        REG_WR(R_LSC_ERROR, err);
    return err;
}

/* Mỗi nút chứa R/Gr/Gb/B; mặc định có 7*5 nút với hệ số 1x. */
enum {
    LSC_DEFAULT_NX = 7,
    LSC_DEFAULT_NY = 5,
    LSC_DEFAULT_COEFFICIENT_COUNT = LSC_DEFAULT_NX * LSC_DEFAULT_NY * LSC_COEFS_PER_NODE,
};

static const uint32_t default_mesh[LSC_DEFAULT_COEFFICIENT_COUNT] = {
    0x40000u, 0x40000u, 0x40000u, 0x40000u, 0x40000u, 0x40000u, 0x40000u, 0x40000u, 0x40000u, 0x40000u,
    0x40000u, 0x40000u, 0x40000u, 0x40000u, 0x40000u, 0x40000u, 0x40000u, 0x40000u, 0x40000u, 0x40000u,
    0x40000u, 0x40000u, 0x40000u, 0x40000u, 0x40000u, 0x40000u, 0x40000u, 0x40000u, 0x40000u, 0x40000u,
    0x40000u, 0x40000u, 0x40000u, 0x40000u, 0x40000u, 0x40000u, 0x40000u, 0x40000u, 0x40000u, 0x40000u,
    0x40000u, 0x40000u, 0x40000u, 0x40000u, 0x40000u, 0x40000u, 0x40000u, 0x40000u, 0x40000u, 0x40000u,
    0x40000u, 0x40000u, 0x40000u, 0x40000u, 0x40000u, 0x40000u, 0x40000u, 0x40000u, 0x40000u, 0x40000u,
    0x40000u, 0x40000u, 0x40000u, 0x40000u, 0x40000u, 0x40000u, 0x40000u, 0x40000u, 0x40000u, 0x40000u,
    0x40000u, 0x40000u, 0x40000u, 0x40000u, 0x40000u, 0x40000u, 0x40000u, 0x40000u, 0x40000u, 0x40000u,
    0x40000u, 0x40000u, 0x40000u, 0x40000u, 0x40000u, 0x40000u, 0x40000u, 0x40000u, 0x40000u, 0x40000u,
    0x40000u, 0x40000u, 0x40000u, 0x40000u, 0x40000u, 0x40000u, 0x40000u, 0x40000u, 0x40000u, 0x40000u,
    0x40000u, 0x40000u, 0x40000u, 0x40000u, 0x40000u, 0x40000u, 0x40000u, 0x40000u, 0x40000u, 0x40000u,
    0x40000u, 0x40000u, 0x40000u, 0x40000u, 0x40000u, 0x40000u, 0x40000u, 0x40000u, 0x40000u, 0x40000u,
    0x40000u, 0x40000u, 0x40000u, 0x40000u, 0x40000u, 0x40000u, 0x40000u, 0x40000u, 0x40000u, 0x40000u,
    0x40000u, 0x40000u, 0x40000u, 0x40000u, 0x40000u, 0x40000u, 0x40000u, 0x40000u, 0x40000u, 0x40000u
};

void lsc_get_default_init_config(lsc_init_config_t *cfg)
{
    if (cfg == NULL) {
        return;
    }

    cfg->mesh.nx = LSC_DEFAULT_NX;
    cfg->mesh.ny = LSC_DEFAULT_NY;
    cfg->mesh.strength = LSC_STRENGTH_UNITY;
    cfg->enable = false;
    cfg->profile = 0;

    /* Cả ba slot dùng chung bảng nguồn trong RAM, nhưng được nạp riêng vào IP. */
    for (unsigned profile = 0; profile < LSC_NUM_PROFILES; profile++) {
        cfg->coefficients[profile] = default_mesh;
        cfg->coefficient_count[profile] = LSC_DEFAULT_COEFFICIENT_COUNT;
    }
}

isp_status_t lsc_validate_init_config(const lsc_init_config_t *cfg,
                                     uint16_t active_width, uint16_t active_height)
{
    if (cfg == NULL || cfg->profile > LSC_PROFILE_MAX ||
        lsc_validate_config(&cfg->mesh, active_width, active_height) != ISP_OK) {
        return ISP_E_PARAM;
    }

    uint32_t expected_count = lsc_coef_count(&cfg->mesh);
    for (unsigned profile = 0; profile < LSC_NUM_PROFILES; profile++) {
        if (cfg->coefficients[profile] == NULL) {
            /* Cho phép bỏ nạp slot, trừ slot đang được chọn khi bật LSC. */
            if (cfg->coefficient_count[profile] != 0 ||
                (cfg->enable && cfg->profile == profile)) {
                return ISP_E_PARAM;
            }
            continue;
        }
        if (cfg->coefficient_count[profile] != expected_count) {
            return ISP_E_PARAM;
        }
        for (uint32_t index = 0; index < expected_count; index++) {
            if (cfg->coefficients[profile][index] > LSC_COEF_MAX) {
                return ISP_E_PARAM;
            }
        }
    }
    return ISP_OK;
}

isp_status_t lsc_initialize_ex(const lsc_init_config_t *cfg,
                               uint16_t active_width, uint16_t active_height,
                               const isp_wait_t *wait, lsc_result_t *result)
{
    lsc_result_t local = {0};
    if (result == NULL) result = &local;
    *result = local;
    result->stage = LSC_STAGE_PREPARE;
    /* 1. Kiểm tra toàn bộ bảng trước khi ghi bất kỳ thanh ghi nào. */
    isp_status_t status = lsc_validate_init_config(cfg, active_width, active_height);
    if (!lsc_wait_valid(wait)) status = ISP_E_PARAM;
    if (status != ISP_OK) {
        result->status = status;
        return status;
    }
    isp_wait_t fallback;
    if (wait == NULL) {
        isp_get_default_wait(&fallback);
        wait = &fallback;
    }
    status = isp_require_legacy();
    if (status != ISP_OK) {
        result->status = status;
        return status;
    }
    if (isp_not_idle()) {
        return lsc_capture(result, ISP_E_NOT_IDLE);
    }
    status = isp_prepare_config();
    if (status != ISP_OK) {
        return lsc_capture(result, status);
    }

    /* 2. Tắt LSC rồi thiết lập hình học dùng chung của các bảng. */
    result->stage = LSC_STAGE_CONFIGURE;
    status = isp_write_checked(R_LSC_CTRL, 1u, 0);
    if (status != ISP_OK) {
        return lsc_capture(result, status);
    }
    status = lsc_configure_wait(&cfg->mesh, active_width, active_height, wait);
    if (status != ISP_OK) {
        return lsc_capture(result, status);
    }

    /*
     * 3. Nếu cần nạp slot đang chọn, nạp một bảng khác trước rồi chọn tạm bảng
     * đã VALID đó. Luồng này hợp MAS và tránh nạp trực tiếp slot đang chọn.
     * Nếu model giữ ACTIVE_PROFILE đến SOF, đổi selector chưa giải phóng slot
     * active cũ; lỗi BEGIN vẫn được trả nguyên vẹn. Không tạo bảng cho NULL.
     */
    uint8_t selected = (uint8_t)(REG_RD(R_LSC_PROFILE_SEL) & LSC_PROFILE_SEL_PROFILE);
    unsigned loaded = 0;
    if (selected < LSC_NUM_PROFILES && cfg->coefficients[selected] != NULL) {
        for (unsigned profile = 0; profile < LSC_NUM_PROFILES; profile++) {
            if (profile == selected || cfg->coefficients[profile] == NULL) continue;
            status = lsc_load_profile_ex((uint8_t)profile, &cfg->mesh,
                                         cfg->coefficients[profile], wait, result);
            if (status != ISP_OK) return status;
            loaded |= 1u << profile;
            result->stage = LSC_STAGE_SELECT;
            status = lsc_set_profile((uint8_t)profile, false);
            if (status != ISP_OK) return lsc_capture(result, status);
            break;
        }
    }

    /* 4. Nạp các bảng còn lại; bảng dùng chuyển tạm đã được nạp đúng một lần. */
    for (unsigned profile = 0; profile < LSC_NUM_PROFILES; profile++) {
        if (cfg->coefficients[profile] == NULL || (loaded & (1u << profile))) {
            continue;
        }
        status = lsc_load_profile_ex((uint8_t)profile, &cfg->mesh,
                                     cfg->coefficients[profile], wait, result);
        if (status != ISP_OK) {
            return status;
        }
    }

    /* 5. Chọn bảng yêu cầu và bật theo cấu hình; không có bảng thì giữ tắt. */
    result->stage = LSC_STAGE_SELECT;
    result->profile = cfg->profile;
    if (cfg->coefficients[cfg->profile] != NULL) {
        status = lsc_set_profile(cfg->profile, cfg->enable);
    } else {
        status = isp_write_checked(R_LSC_CTRL, 1u, 0);
    }
    if (status == ISP_OK) result->stage = LSC_STAGE_READY;
    return lsc_capture(result, status);
}

isp_status_t lsc_initialize(const lsc_init_config_t *cfg,
                            uint16_t active_width, uint16_t active_height)
{
    return lsc_initialize_ex(cfg, active_width, active_height, NULL, NULL);
}
