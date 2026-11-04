/*
 * Khung 3A tuần tự: chuẩn bị frame -> thu thập AEC/AWB -> tính bộ kế tiếp.
 * Chỉ prepare áp tham số; poll giữ đề xuất để frame sau sử dụng.
 */
#include "pipeline/ftel_isp_3a.h"
#include "bayer domain/dg/ftel_isp_dg.h"

void isp_3a_get_default_config(isp_3a_config_t *cfg,
                              uint16_t active_width, uint16_t active_height)
{
    if (cfg == NULL) {
        return;
    }

    ae_get_default_config(&cfg->ae);
    aec_get_default_config(&cfg->aec, active_width, active_height);
    cfg->aec.enable = true;
    cfg->aec.source = AEC_SOURCE_DG;

    awb_get_default_config(&cfg->awb);
    cfg->awb.enable = true;
    cfg->awb.num_zone_x = (uint8_t)(active_width / 2u < 32u ? active_width / 2u : 32u);
    cfg->awb.num_zone_y = (uint8_t)(active_height / 2u < 24u ? active_height / 2u : 24u);

    cfg->active_width = active_width;
    cfg->active_height = active_height;
    cfg->collect_awb = true;
    cfg->context_seed = 0x3a000000;
}

isp_status_t isp_3a_apply_digital_gain(void *user, uint32_t frame_id,
                                      uint32_t context_id, uint32_t gain_q8)
{
    (void)user;
    (void)frame_id;
    (void)context_id;

    if (gain_q8 > 4095) {
        return ISP_E_PARAM;
    }
    dg_config_t cfg = {
        .enable = true,
        .gain = (uint16_t)gain_q8,
    };
    return dg_configure(&cfg);
}

void isp_3a_get_default_ops(isp_3a_ops_t *ops)
{
    if (ops == NULL) {
        return;
    }
    *ops = (isp_3a_ops_t){
        .digital_gain = isp_3a_apply_digital_gain,
        .exposure = NULL,
        .analog_gain = NULL,
        .process_ae = NULL,
    };
}

static isp_status_t validate_3a_config(const isp_3a_config_t *cfg,
                                      const isp_3a_ops_t *ops)
{
    if (cfg == NULL || ops == NULL ||
        cfg->active_width < 32 || cfg->active_height < 32 ||
        cfg->active_width > 3840 || cfg->active_height > 2160 ||
        ((cfg->active_width | cfg->active_height) & 1u) || !cfg->aec.enable) {
        return ISP_E_PARAM;
    }

    isp_status_t status = ae_validate_config(&cfg->ae);
    if (status != ISP_OK) {
        return status;
    }
    status = aec_validate_config(&cfg->aec, cfg->active_width, cfg->active_height);
    if (status != ISP_OK) {
        return status;
    }
    if (cfg->collect_awb) {
        if (!cfg->awb.enable) {
            return ISP_E_PARAM;
        }
        status = awb_validate_config(&cfg->awb, cfg->active_width, cfg->active_height);
        if (status != ISP_OK) {
            return status;
        }
    }

    if (ops->digital_gain == NULL ||
        (cfg->ae.control == AE_CONTROL_EXPOSURE && ops->exposure == NULL) ||
        (cfg->ae.control == AE_CONTROL_ANALOG_GAIN && ops->analog_gain == NULL)) {
        return ISP_E_UNSUPPORTED;
    }

    /* Điều khiển DG cần phép đo sau DG để thấy tác động của gain vừa áp. */
    if (cfg->ae.control == AE_CONTROL_DIGITAL_GAIN && cfg->aec.source != AEC_SOURCE_DG) {
        return ISP_E_PARAM;
    }
    return ISP_OK;
}

isp_status_t isp_3a_init(isp_3a_context_t *context, const isp_3a_config_t *cfg,
                        const isp_3a_ops_t *ops, void *user)
{
    if (context == NULL) {
        return ISP_E_PARAM;
    }

    isp_status_t status = validate_3a_config(cfg, ops);
    if (status != ISP_OK) {
        return status;
    }

    /* Dựng bản tạm để context của bên gọi chỉ đổi khi khởi tạo thành công. */
    isp_3a_context_t initial = {0};
    initial.config = *cfg;
    initial.ops = *ops;
    initial.user = user;
    status = ae_init(&initial.ae, &cfg->ae);
    if (status != ISP_OK) {
        return status;
    }

    initial.pending = cfg->ae.initial;
    initial.token = cfg->context_seed;
    initial.state = ISP_3A_READY;
    *context = initial;
    return ISP_OK;
}

static bool frame_id_is_newer(uint32_t frame_id, uint32_t previous_frame_id)
{
    uint32_t distance = frame_id - previous_frame_id;
    return distance != 0 && distance < 0x80000000u;
}

/* Ghi cùng nhãn cho AEC/AWB trước SOF; chưa gửi frame hoặc áp tham số cảm biến. */
static isp_status_t configure_frame_statistics(const isp_3a_context_t *context,
                                               uint32_t context_id)
{
    aec_config_t aec = context->config.aec;
    aec.context_id = context_id;
    isp_status_t status = aec_configure(&aec, context->config.active_width,
                                       context->config.active_height);
    if (status != ISP_OK) {
        return status;
    }

    if (context->config.collect_awb) {
        awb_stat_config_t awb = context->config.awb;
        awb.context_id = context_id;
        return awb_configure(&awb, context->config.active_width,
                             context->config.active_height);
    }
    return isp_write_checked(R_AWB_CTRL, AWB_CTRL_EN, 0);
}

/* Thứ tự áp giữ nguyên: phơi sáng -> gain tương tự -> gain số. */
static isp_status_t apply_pending_params(const isp_3a_context_t *context,
                                         uint32_t frame_id, uint32_t context_id)
{
    if ((context->ops.exposure == NULL &&
         context->pending.exposure_us != context->ae.current.exposure_us) ||
        (context->ops.analog_gain == NULL &&
         context->pending.analog_gain_q8 != context->ae.current.analog_gain_q8)) {
        return ISP_E_UNSUPPORTED;
    }

    isp_status_t status;
    if (context->ops.exposure != NULL) {
        status = context->ops.exposure(context->user, frame_id, context_id,
                                       context->pending.exposure_us);
        if (status != ISP_OK) {
            return status;
        }
    }
    if (context->ops.analog_gain != NULL) {
        status = context->ops.analog_gain(context->user, frame_id, context_id,
                                          context->pending.analog_gain_q8);
        if (status != ISP_OK) {
            return status;
        }
    }
    return context->ops.digital_gain(context->user, frame_id, context_id,
                                     context->pending.digital_gain_q8);
}

isp_status_t isp_3a_prepare_frame(isp_3a_context_t *context, uint32_t frame_id)
{
    if (context == NULL) {
        return ISP_E_PARAM;
    }
    if (context->state != ISP_3A_READY) {
        return ISP_E_STATE;
    }
    if (context->have_frame && !frame_id_is_newer(frame_id, context->result.frame_id)) {
        return ISP_E_STALE;
    }

    /* 1. Kiểm tra trước khi bắt đầu ghi cấu hình hoặc gọi hàm cảm biến. */
    isp_status_t status = validate_3a_config(&context->config, &context->ops);
    if (status != ISP_OK) {
        return status;
    }
    status = ae_validate_params(&context->config.ae, &context->pending);
    if (status != ISP_OK) {
        return status;
    }
    status = isp_require_legacy();
    if (status != ISP_OK) {
        return status;
    }
    if (isp_not_idle() || (REG_RD(R_TUNE_AEC_STATUS) & 1u) ||
        (REG_RD(R_AWB_RESULT_STATUS) & AWB_RESULT_STATUS_LEASED)) {
        return ISP_E_NOT_IDLE;
    }

    /* 2. Ghi nhãn thống kê rồi áp bộ ban đầu/đề xuất cho đúng frame. */
    uint32_t context_id = context->token + 1u;
    status = configure_frame_statistics(context, context_id);
    if (status != ISP_OK) {
        goto fault;
    }
    status = apply_pending_params(context, frame_id, context_id);
    if (status != ISP_OK) {
        goto fault;
    }

    /* 3. Chỉ xác nhận current sau khi tất cả hàm áp dụng thành công. */
    context->ae.current = context->pending;
    context->token = context_id;
    context->have_frame = true;
    context->aec_done = false;
    context->awb_done = !context->config.collect_awb;
    context->result = (isp_3a_result_t){0};
    context->result.frame_id = frame_id;
    context->result.context_id = context_id;
    context->result.applied = context->pending;
    context->result.next = context->pending;
    context->result.ae_status = ISP_E_NO_DATA;
    context->result.awb_status = ISP_E_NO_DATA;
    context->state = ISP_3A_WAIT_STATS;
    return ISP_OK;

fault:
    /* Có thể đã ghi/áp một phần: bên gọi phải đồng bộ lại trước khi tạo context mới. */
    context->state = ISP_3A_FAULT;
    return status;
}

static isp_status_t validate_awb_measurement(const awb_global_stats_t *stats)
{
    const awb_zone_stats_t *global = &stats->global;
    uint64_t maximum_sum = (uint64_t)global->count * 4095u;
    if (global->count > 3840u * 2160u / 4u ||
        global->sum_r > maximum_sum || global->sum_g > maximum_sum ||
        global->sum_b > maximum_sum) {
        return ISP_E_IO;
    }
    return global->count != 0 ? ISP_OK : ISP_E_NO_DATA;
}

/* Helper get_stats đã giải phóng lease trước khi trả bản sao để ghép vào context. */
static isp_status_t collect_aec_snapshot(isp_3a_context_t *context)
{
    aec_frame_stats_t snapshot;
    isp_status_t status = aec_get_stats(&snapshot);
    if (status != ISP_OK && status != ISP_E_NO_DATA) {
        context->state = ISP_3A_FAULT;
        return status;
    }
    if (status == ISP_OK) {
        if (snapshot.global.frame_id != context->result.frame_id ||
            snapshot.global.context_id != context->result.context_id ||
            snapshot.source != context->config.aec.source) {
            return ISP_E_STALE;
        }
        context->result.aec = snapshot;
        context->result.aec_valid = true;
        context->aec_done = true;
    }
    return status;
}

static isp_status_t collect_awb_snapshot(isp_3a_context_t *context)
{
    awb_frame_stats_t snapshot;
    isp_status_t status = awb_get_stats(&snapshot);
    if (status != ISP_OK && status != ISP_E_NO_DATA) {
        context->state = ISP_3A_FAULT;
        return status;
    }
    if (status == ISP_OK) {
        if (snapshot.global.frame_id != context->result.frame_id ||
            snapshot.global.context_id != context->result.context_id) {
            return ISP_E_STALE;
        }
        context->result.awb = snapshot;
        context->result.awb_valid = true;
        context->awb_done = true;
        context->result.awb_status = validate_awb_measurement(&snapshot.global);
    }
    return status;
}

/* Kiểm tra số đo trước thuật toán, rồi kiểm tra giới hạn/khả năng áp của đề xuất. */
static isp_status_t compute_next_params(const isp_3a_context_t *context,
                                        ae_params_t *proposal)
{
    uint16_t mean;
    isp_status_t status = ae_statistics_mean(&context->result.aec.global, &mean);
    if (status != ISP_OK) {
        return status;
    }

    if (context->ops.process_ae != NULL) {
        status = context->ops.process_ae(context->user, &context->ae,
                                         &context->result.aec.global, proposal);
    } else {
        status = ae_process(&context->ae, &context->result.aec.global, proposal);
    }
    if (status != ISP_OK) {
        return status;
    }
    status = ae_validate_params(&context->config.ae, proposal);
    if (status != ISP_OK) {
        return status;
    }
    if ((context->ops.exposure == NULL &&
         proposal->exposure_us != context->ae.current.exposure_us) ||
        (context->ops.analog_gain == NULL &&
         proposal->analog_gain_q8 != context->ae.current.analog_gain_q8)) {
        return ISP_E_UNSUPPORTED;
    }
    if (proposal->digital_gain_q8 != context->ae.current.digital_gain_q8 &&
        context->config.aec.source != AEC_SOURCE_DG) {
        return ISP_E_UNSUPPORTED;
    }
    return ISP_OK;
}

isp_status_t isp_3a_poll(isp_3a_context_t *context, isp_3a_result_t *result)
{
    if (context == NULL || result == NULL) {
        return ISP_E_PARAM;
    }
    if (context->state != ISP_3A_WAIT_STATS) {
        return ISP_E_STATE;
    }

    /* 1. Thu phần còn thiếu; AEC/AWB có thể READY ở hai lần gọi khác nhau. */
    isp_status_t status;
    if (!context->aec_done) {
        status = collect_aec_snapshot(context);
        if (status != ISP_OK && status != ISP_E_NO_DATA) {
            return status;
        }
    }
    if (!context->awb_done) {
        status = collect_awb_snapshot(context);
        if (status != ISP_OK && status != ISP_E_NO_DATA) {
            return status;
        }
    }

    *result = context->result;
    if (!context->aec_done || !context->awb_done) {
        return ISP_E_NO_DATA;
    }

    /* 2. Đủ thống kê: chạy AE một lần. Đề xuất lỗi/rỗng/tràn thì giữ bộ cũ. */
    ae_params_t proposal = context->ae.current;
    status = compute_next_params(context, &proposal);
    context->result.ae_status = status;
    if (status == ISP_OK) {
        context->pending = proposal;
    } else {
        context->pending = context->ae.current;
    }

    /* 3. Frame đã thu thập xong; tham số chỉ được áp ở prepare kế tiếp. */
    context->result.next = context->pending;
    context->state = ISP_3A_READY;
    *result = context->result;
    return ISP_OK;
}

isp_status_t isp_3a_skip_statistics(isp_3a_context_t *context)
{
    if (context == NULL) {
        return ISP_E_PARAM;
    }
    if (context->state != ISP_3A_WAIT_STATS) {
        return ISP_E_STATE;
    }

    isp_status_t status = isp_require_legacy();
    if (status != ISP_OK) {
        return status;
    }
    if (isp_not_idle()) {
        return ISP_E_NOT_IDLE;
    }

    context->pending = context->ae.current;
    context->state = ISP_3A_READY;
    return ISP_OK;
}
