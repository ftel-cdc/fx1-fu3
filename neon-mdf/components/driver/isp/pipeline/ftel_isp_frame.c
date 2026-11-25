/* Một frame slot 0; lõi submit/collect dùng chung cho polling và adapter OS. */
#include "ftel_isp_private.h"
#include "io/dma/ftel_isp_dma.h"
#include "platform/fx1/ftel_isp_platform.h"

typedef struct { uint64_t begin, end; } buffer_span_t;

static bool buffer_span(const ftel_isp_buffer_t *buffer, uint32_t row_bytes,
                         uint16_t rows, buffer_span_t *span)
{
    uint64_t bytes = (uint64_t)buffer->stride * rows;
    uint64_t begin = buffer->address;
    uint64_t end = begin + bytes;
    if ((begin & 15u) || (buffer->stride & 15u) || buffer->stride < row_bytes ||
        !bytes || bytes > buffer->bytes || begin < FX1_FW_RAW_BUF_BASE ||
        end > (uint64_t)FX1_DDR_BASE + FX1_DDR_SIZE) return false;
    /* DDR trước RAW_BUF_BASE là vùng ELF/stack/shared của BSP VP. */
    *span = (buffer_span_t){begin, end};
    return true;
}

static bool buffers_valid(const ftel_isp_frame_t *frame, const ftel_isp_fx1_session_t *s)
{
    buffer_span_t span[3];
    if (!buffer_span(&frame->raw, 2u * s->config.bayer.input.width,
                      s->config.bayer.input.height, &span[0]) ||
        !buffer_span(&frame->y, s->active_width, s->active_height, &span[1]) ||
        !buffer_span(&frame->uv, s->active_width, s->active_height / 2u, &span[2])) return false;
    for (unsigned i = 0; i < 3; ++i)
        for (unsigned j = 0; j < i; ++j)
            if (span[i].begin < span[j].end && span[j].begin < span[i].end) return false;
    return true;
}

ftel_isp_status_t ftel_isp_fx1_submit(const ftel_isp_frame_t *frame, bool irq,
                                       uint32_t prepare_timeout_us)
{
    if (!ftel_isp_fx1_access_allowed()) return FTEL_ISP_E_BUSY;
    if (frame == NULL || !prepare_timeout_us) return FTEL_ISP_E_CONFIG;
    ftel_isp_fx1_session_t *s = &ftel_isp_fx1_session;
    if (!s->ready || s->submitted || !s->config.aec.enable) return FTEL_ISP_E_STATE;
    if (!buffers_valid(frame, s)) return FTEL_ISP_E_CONFIG;
    if (REG_RD(R_COMMON_RECOVERY_SEQUENCE) != s->recovery_sequence) return FTEL_ISP_E_STALE;
    isp_status_t status = isp_require_legacy();
    if (status != ISP_OK) return ftel_isp_fx1_status(status);
    if (isp_not_idle() || (REG_RD(R_DMA_CTRL) & DMA_CTRL_ENABLE_MASK) ||
        (REG_RD(R_COMMON_CTRL) & COMMON_CTRL_ISP_EN) || aec_lease_valid()) return FTEL_ISP_E_BUSY;
    /* Soft reset trả vòng slot về 0 (MAS 9.9.8), không lấy giá trị đếm tuyệt
     * đối bằng 0 làm điều kiện. Chờ delta +1 từ snapshot trước submit. */
    uint32_t input_before = REG_RD(FX1_ISP_IDMA_FRAME_COUNT_OFFSET);
    uint32_t output_before = REG_RD(FX1_ISP_ODMA_FRAME_COUNT_OFFSET);

    output_config_t output;
    output_geometry_t geometry[3];
    dma_config_t dma;
    output_get_default_config(&output, s->active_width, s->active_height);
    for (unsigned c = 0; c < 3; ++c) {
        status = output_compute_geometry(&output.channel[c], c, s->active_width, s->active_height, &geometry[c]);
        if (status != ISP_OK) return ftel_isp_fx1_status(status);
    }
    dma_get_default_config(&dma, s->config.bayer.input.width, geometry);
    dma.input_stride = frame->raw.stride;
    dma.output[0].y_stride = frame->y.stride;
    dma.output[0].uv_stride = frame->uv.stride;
    /* Chỉ slot 0 được cấp VALID/FREE. Các slot không hoạt động dùng cùng địa
     * chỉ giữ chỗ để không cấu hình DMA tới địa chỉ 0; không cấp quyền cho chúng. */
    for (unsigned i = 0; i < 4; ++i) {
        dma.input[i] = frame->raw.address;
        dma.output[0].y[i] = frame->y.address;
        dma.output[0].uv[i] = frame->uv.address;
    }
    status = dma_validate_config(&dma, s->config.bayer.input.width, s->config.bayer.input.height, geometry);
    if (status != ISP_OK) return ftel_isp_fx1_status(status);
    isp_wait_t wait = {ftel_isp_platform_time_us, NULL, prepare_timeout_us, 1000000u};
    /* Từ đây có thể đã ghi cấu hình một phần; cần init lại trước lần chạy khác. */
    s->submitted = true;
    status = output_configure(&output, s->active_width, s->active_height);
    if (status != ISP_OK) goto failed;
    status = dma_configure(&dma, s->config.bayer.input.width, s->config.bayer.input.height, geometry);
    if (status != ISP_OK) goto failed;
    status = isp_enable_pipeline();
    if (status != ISP_OK) goto failed;
    status = output_wait_geometry(&output, s->active_width, s->active_height, &wait);
    if (status != ISP_OK) goto failed;
    /* Chân PLIC do adapter giữ masked trong lúc cấu hình. */
    REG_WR(R_COMMON_IRQ_EN, irq ? 3u : 0u);
    REG_WR(FX1_ISP_OUTPUT_IRQ_EN_OFFSET, irq ? 0x71u : 0u);
    REG_WR(R_DMA_IRQ_EN, irq ? 0x71u : 0u); /* CFG/ODMA/overflow/AXI. */
    s->input_before = input_before;
    s->output_before = output_before;
    s->tag = frame->tag;
    s->timestamp = frame->timestamp;
    /* BSP hiện không có D-cache; barrier không thay clean/invalidate silicon. */
    status = dma_submit_slot(0, 1u, frame->tag, frame->timestamp);
    if (status != ISP_OK) goto failed;
    return FTEL_ISP_OK;
failed:
    return ftel_isp_fx1_status(status);
}

ftel_isp_status_t ftel_isp_fx1_collect(ftel_isp_frame_result_t *result)
{
    if (!ftel_isp_fx1_access_allowed()) return FTEL_ISP_E_BUSY;
    if (result == NULL) return FTEL_ISP_E_CONFIG;
    ftel_isp_fx1_session_t *s = &ftel_isp_fx1_session;
    if (!s->ready || !s->submitted || s->complete || s->recovering)
        return FTEL_ISP_E_STATE;
    isp_status_t status;
    dma_frame_status_t state;
    dma_read_frame_status(&state);
    if (state.recovery_sequence != s->recovery_sequence) return FTEL_ISP_E_STALE;
    if ((state.errors & ~DMA_ERROR_INPUT_PAUSED) || (state.common_irq & 2u) ||
        (state.output_irq & 0x70u) || isp_tune_error_set()) return FTEL_ISP_E_IO;
    if (state.input_valid || state.busy || isp_pipeline_busy() ||
        state.input_count != s->input_before + 1u ||
        state.output_count[0] != s->output_before + 1u || state.output_done[0] != 1u)
        return FTEL_ISP_E_NO_DATA;
    status = dma_pause();
    if (status != ISP_OK) goto failed;
    /* Chỉ bỏ cờ pause thiếu frame kế sau khi xác nhận frame hiện tại xong và
     * DMA đã dừng. Lưu snapshot lỗi gốc trong result; không bỏ qua lỗi khác. */
    REG_WR(R_DMA_ERR, DMA_ERROR_INPUT_PAUSED);
    REG_WR(FX1_ISP_DMA_IRQ_STAT_OFFSET, 1u << 3);
    isp_barrier();
    if (REG_RD(R_DMA_ERR)) { status = ISP_E_IO; goto failed; }
    ftel_isp_frame_result_t completed = {
        .input_count = state.input_count, .output_count = state.output_count[0],
        .frame_id = REG_RD(FX1_ISP_TUNE_CH0_BUF0_FRAME_OFFSET),
        .tag = REG_RD(FX1_ISP_ODMA_BUF_TAG0_OFFSET),
        .timestamp = REG_RD(FX1_ISP_ODMA_BUF_TS0_OFFSET), .dma_errors = state.errors,
    };
    if (completed.tag != s->tag || completed.timestamp != s->timestamp) {
        status = ISP_E_STALE;
        goto failed;
    }
    status = dma_release_slot(0, 1u);
    if (status != ISP_OK) goto failed;
    status = isp_clear_common_ctrl();
    if (status != ISP_OK) goto failed;
    isp_memory_barrier();
    REG_WR(R_COMMON_IRQ_EN, 0);
    REG_WR(R_DMA_IRQ_EN, 0);
    REG_WR(FX1_ISP_OUTPUT_IRQ_EN_OFFSET, 0);
    s->complete = true;
    s->frame_id = completed.frame_id;
    *result = completed;
    return FTEL_ISP_OK;
failed:
    /* Không thu hồi buffer đang chạy chỉ vì software timeout. */
    return ftel_isp_fx1_status(status);
}

/* Bản bare-metal giữ hợp đồng một frame sau mỗi init, polling có timeout. */
ftel_isp_status_t ftel_isp_run_frame(const ftel_isp_frame_t *frame, uint32_t timeout_us,
                                    ftel_isp_frame_result_t *result)
{
    if (frame == NULL || result == NULL || !timeout_us) return FTEL_ISP_E_CONFIG;
    ftel_isp_status_t status = ftel_isp_fx1_submit(frame, false, timeout_us);
    if (status != FTEL_ISP_OK) return status;
    uint64_t begin = ftel_isp_platform_time_us(NULL);
    for (uint32_t poll = 0; poll < 1000000u; ++poll) {
        status = ftel_isp_fx1_collect(result);
        if (status != FTEL_ISP_E_NO_DATA) return status;
        if (ftel_isp_platform_time_us(NULL) - begin >= timeout_us) break;
    }
    return FTEL_ISP_E_TIMEOUT;
}

/* Bắt đầu recovery, không chờ trong critical section hoặc ISR. */
ftel_isp_status_t ftel_isp_fx1_recovery_begin(void)
{
    if (!ftel_isp_fx1_access_allowed()) return FTEL_ISP_E_BUSY;
    if (isp_check_abi() != ISP_OK) return FTEL_ISP_E_ABI;
    REG_WR(R_COMMON_IRQ_EN, 0);
    REG_WR(R_DMA_IRQ_EN, 0);
    REG_WR(FX1_ISP_OUTPUT_IRQ_EN_OFFSET, 0);
    REG_WR(R_DMA_CTRL, REG_RD(R_DMA_CTRL) & DMA_CTRL_BURST_MASK);
    isp_barrier();
    if (!(REG_RD(R_COMMON_RECOVERY_STATUS) & COMMON_RECOVERY_STATUS_BUSY)) {
        REG_WR(R_COMMON_RECOVERY_STATUS, COMMON_RECOVERY_STATUS_DONE);
        isp_barrier();
        REG_WR(R_COMMON_CTRL, COMMON_CTRL_SOFT_RST);
        isp_barrier();
    }
    ftel_isp_fx1_session.recovering = true;
    return FTEL_ISP_OK;
}

ftel_isp_status_t ftel_isp_fx1_recovery_poll(void)
{
    if (!ftel_isp_fx1_access_allowed()) return FTEL_ISP_E_BUSY;
    if (!ftel_isp_fx1_session.recovering) return FTEL_ISP_E_STATE;
    uint32_t recovery = REG_RD(R_COMMON_RECOVERY_STATUS);
    if ((recovery & 3u) != COMMON_RECOVERY_STATUS_DONE) return FTEL_ISP_E_NO_DATA;
    dma_frame_status_t state;
    dma_read_frame_status(&state);
    if (state.busy || state.input_valid || isp_pipeline_busy() ||
        (REG_RD(R_DMA_CTRL) & DMA_CTRL_ENABLE_MASK)) return FTEL_ISP_E_NO_DATA;
    for (unsigned c = 0; c < 3; ++c)
        if (state.output_free[c] || state.output_done[c]) return FTEL_ISP_E_NO_DATA;
    if (isp_clear_common_ctrl() != ISP_OK) return FTEL_ISP_E_IO;
    isp_memory_barrier();
    ftel_isp_fx1_session = (ftel_isp_fx1_session_t){0};
    return FTEL_ISP_OK;
}
