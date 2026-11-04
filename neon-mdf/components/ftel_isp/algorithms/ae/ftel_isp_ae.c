/*
 * AE tham chiếu chạy CPU: kiểm tra thống kê, tính sáng trung bình và đề xuất
 * một tham số cho frame sau. Hàm không ghi MMIO và không tự cập nhật current.
 */
#include "algorithms/ae/ftel_isp_ae.h"

void ae_get_default_config(ae_config_t *cfg)
{
    if (cfg == NULL) {
        return;
    }

    /* Phơi sáng theo micro giây; gain Q8 dùng 256 để biểu diễn 1x. */
    cfg->initial = (ae_params_t){
        .exposure_us = 10000,
        .analog_gain_q8 = 256,
        .digital_gain_q8 = 256,
    };
    cfg->minimum = (ae_params_t){
        .exposure_us = 100,
        .analog_gain_q8 = 256,
        .digital_gain_q8 = 256,
    };
    cfg->maximum = (ae_params_t){
        .exposure_us = 33000,
        .analog_gain_q8 = 4096,
        .digital_gain_q8 = 4095,
    };
    cfg->control = AE_CONTROL_DIGITAL_GAIN;
    cfg->target = 1024;
    cfg->tolerance = 16;
    cfg->max_step_q8 = 512;
}

isp_status_t ae_validate_params(const ae_config_t *cfg, const ae_params_t *params)
{
    if (cfg == NULL || params == NULL ||
        params->exposure_us < cfg->minimum.exposure_us ||
        params->exposure_us > cfg->maximum.exposure_us ||
        params->analog_gain_q8 < cfg->minimum.analog_gain_q8 ||
        params->analog_gain_q8 > cfg->maximum.analog_gain_q8 ||
        params->digital_gain_q8 < cfg->minimum.digital_gain_q8 ||
        params->digital_gain_q8 > cfg->maximum.digital_gain_q8) {
        return ISP_E_PARAM;
    }
    return ISP_OK;
}

isp_status_t ae_validate_config(const ae_config_t *cfg)
{
    if (cfg == NULL ||
        cfg->control < AE_CONTROL_DIGITAL_GAIN ||
        cfg->control > AE_CONTROL_ANALOG_GAIN ||
        cfg->minimum.exposure_us == 0 ||
        cfg->minimum.analog_gain_q8 < 256 ||
        cfg->minimum.digital_gain_q8 < 256 ||
        cfg->maximum.digital_gain_q8 > 4095 ||
        cfg->minimum.exposure_us > cfg->maximum.exposure_us ||
        cfg->minimum.analog_gain_q8 > cfg->maximum.analog_gain_q8 ||
        cfg->minimum.digital_gain_q8 > cfg->maximum.digital_gain_q8 ||
        cfg->target == 0 || cfg->target > 4095 ||
        cfg->tolerance > cfg->target ||
        cfg->max_step_q8 < 256 || cfg->max_step_q8 > 4096) {
        return ISP_E_PARAM;
    }
    return ae_validate_params(cfg, &cfg->initial);
}

isp_status_t ae_init(ae_context_t *context, const ae_config_t *cfg)
{
    if (context == NULL) {
        return ISP_E_PARAM;
    }

    isp_status_t status = ae_validate_config(cfg);
    if (status != ISP_OK) {
        return status;
    }

    context->config = *cfg;
    context->current = cfg->initial;
    return ISP_OK;
}

isp_status_t ae_statistics_mean(const aec_global_stats_t *stats, uint16_t *mean)
{
    if (stats == NULL || mean == NULL) {
        return ISP_E_PARAM;
    }
    if (stats->overflow) {
        return ISP_E_OVERFLOW;
    }
    if (stats->global_count == 0) {
        if (stats->global_sum != 0) {
            return ISP_E_IO;
        }
        return ISP_E_NO_DATA;
    }

    /* Kiểm tra count trước phép nhân để số liệu sai không gây tràn 64 bit. */
    if (stats->global_count > UINT64_C(3840) * 2160 ||
        stats->global_sum > stats->global_count * 4095u ||
        stats->oe_count > UINT64_C(3840) * 2160 ||
        stats->ue_count > UINT64_C(3840) * 2160) {
        return ISP_E_IO;
    }

    /* Chia nguyên và làm tròn nửa lên; chỉ ghi đầu ra khi số đo hợp lệ. */
    *mean = (uint16_t)((stats->global_sum + stats->global_count / 2u) /
                       stats->global_count);
    return ISP_OK;
}

isp_status_t ae_process(const ae_context_t *context, const aec_global_stats_t *stats,
                        ae_params_t *proposal)
{
    if (context == NULL || proposal == NULL) {
        return ISP_E_PARAM;
    }

    /* 1. Kiểm tra cấu hình, bộ đã áp và số đo trước khi tính đề xuất. */
    isp_status_t status = ae_validate_config(&context->config);
    if (status != ISP_OK) {
        return status;
    }
    status = ae_validate_params(&context->config, &context->current);
    if (status != ISP_OK) {
        return status;
    }

    uint16_t mean;
    status = ae_statistics_mean(stats, &mean);
    if (status != ISP_OK) {
        return status;
    }

    const ae_config_t *cfg = &context->config;
    ae_params_t next_params = context->current;

    /* 2. Sáng đã nằm trong dải mục tiêu thì giữ nguyên bộ tham số. */
    if ((uint32_t)mean + cfg->tolerance >= cfg->target &&
        mean <= (uint32_t)cfg->target + cfg->tolerance) {
        *proposal = next_params;
        return ISP_OK;
    }

    /* 3. Tỷ số mục tiêu/mean dạng Q8; dùng mẫu số 1 khi mean bằng 0. */
    uint32_t ratio_q8 = (uint32_t)cfg->target * 256u / (mean ? mean : 1u);
    uint32_t minimum_ratio_q8 = (65536u + cfg->max_step_q8 - 1u) / cfg->max_step_q8;
    if (ratio_q8 < minimum_ratio_q8) {
        ratio_q8 = minimum_ratio_q8;
    }
    if (ratio_q8 > cfg->max_step_q8) {
        ratio_q8 = cfg->max_step_q8;
    }

    /* 4. Chọn đúng một tham số để điều khiển và lấy giới hạn của tham số đó. */
    uint32_t current_value;
    uint32_t minimum_value;
    uint32_t maximum_value;
    switch (cfg->control) {
    case AE_CONTROL_DIGITAL_GAIN:
        current_value = next_params.digital_gain_q8;
        minimum_value = cfg->minimum.digital_gain_q8;
        maximum_value = cfg->maximum.digital_gain_q8;
        break;
    case AE_CONTROL_EXPOSURE:
        current_value = next_params.exposure_us;
        minimum_value = cfg->minimum.exposure_us;
        maximum_value = cfg->maximum.exposure_us;
        break;
    case AE_CONTROL_ANALOG_GAIN:
        current_value = next_params.analog_gain_q8;
        minimum_value = cfg->minimum.analog_gain_q8;
        maximum_value = cfg->maximum.analog_gain_q8;
        break;
    default:
        return ISP_E_PARAM;
    }

    /* 5. Nhân bằng trung gian 64 bit, làm tròn rồi kẹp bước và miền tham số. */
    uint64_t next_value = ((uint64_t)current_value * ratio_q8 + 128u) / 256u;
    uint64_t step_minimum = ((uint64_t)current_value * 256u + cfg->max_step_q8 - 1u) /
                            cfg->max_step_q8;
    uint64_t step_maximum = (uint64_t)current_value * cfg->max_step_q8 / 256u;
    if (next_value < step_minimum) {
        next_value = step_minimum;
    }
    if (next_value > step_maximum) {
        next_value = step_maximum;
    }
    if (next_value < minimum_value) {
        next_value = minimum_value;
    }
    if (next_value > maximum_value) {
        next_value = maximum_value;
    }

    /* Tiến một đơn vị khi lượng tử hóa giữ nguyên, nếu vẫn nằm trong giới hạn bước. */
    if (next_value == current_value && cfg->max_step_q8 > 256) {
        if (mean < cfg->target && current_value < maximum_value && next_value < step_maximum) {
            next_value++;
        }
        if (mean > cfg->target && current_value > minimum_value && next_value > step_minimum) {
            next_value--;
        }
    }

    /* 6. Chỉ sửa tham số đã chọn; current vẫn thuộc bộ đã áp của context. */
    if (cfg->control == AE_CONTROL_DIGITAL_GAIN) {
        next_params.digital_gain_q8 = (uint16_t)next_value;
    }
    if (cfg->control == AE_CONTROL_EXPOSURE) {
        next_params.exposure_us = (uint32_t)next_value;
    }
    if (cfg->control == AE_CONTROL_ANALOG_GAIN) {
        next_params.analog_gain_q8 = (uint16_t)next_value;
    }
    *proposal = next_params;
    return ISP_OK;
}
