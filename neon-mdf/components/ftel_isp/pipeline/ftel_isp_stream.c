/* Luồng tuần tự để bring-up bare-metal; chưa có hàng đợi nhiều frame, ISR hoặc recovery tự động. */
#include "pipeline/ftel_isp_stream.h"

typedef struct { uint64_t begin, end; } memory_span_t;

/* Dùng toàn stride của mọi dòng, kể cả padding, để bảo vệ cả mặt phẳng DMA. */
static bool add_span(memory_span_t spans[28], unsigned *count, uint64_t base,
                     uint32_t stride, uint16_t rows, uint8_t address_bits)
{
    uint64_t bytes = (uint64_t)stride * rows;
    uint64_t limit = UINT64_C(1) << address_bits;
    if (!base || base >= limit || bytes > limit - base) return false;
    uint64_t end = base + bytes;
    for (unsigned i = 0; i < *count; i++)
        if (base < spans[i].end && spans[i].begin < end) return false;
    spans[(*count)++] = (memory_span_t){base, end};
    return true;
}

static isp_status_t check_buffers(const isp_config_t *cfg, uint8_t *outputs)
{
    memory_span_t spans[28];
    unsigned count = 0;
    *outputs = 0;
    uint16_t w, h;
    (void)ifmt_active_size(&cfg->ifmt, &w, &h);
    for (unsigned i = 0; i < 4; i++)
        if (!add_span(spans, &count, cfg->dma.input[i], cfg->dma.input_stride,
                      cfg->ifmt.height, cfg->dma.address_bits)) return ISP_E_PARAM;
    for (unsigned c = 0; c < 3; c++) if (cfg->output.channel[c].enable) {
        *outputs |= 1u << c;
        output_geometry_t g;
        (void)output_compute_geometry(&cfg->output.channel[c], c, w, h, &g);
        for (unsigned i = 0; i < 4; i++) {
            if (!add_span(spans, &count, cfg->dma.output[c].y[i], cfg->dma.output[c].y_stride,
                          g.height, cfg->dma.address_bits)) return ISP_E_PARAM;
            if (c < 2 && !add_span(spans, &count, cfg->dma.output[c].uv[i], cfg->dma.output[c].uv_stride,
                                  g.height / 2u, cfg->dma.address_bits)) return ISP_E_PARAM;
        }
    }
    return ISP_OK;
}

static isp_status_t check_hardware(isp_stream_t *stream, dma_frame_status_t *s)
{
    dma_read_frame_status(s);
    /* Reset/recovery làm mất vòng slot: không được công bố DONE hay tái cấp buffer theo trạng thái cũ. */
    if ((s->recovery_status & COMMON_RECOVERY_STATUS_BUSY) ||
        s->recovery_sequence != stream->recovery_sequence) {
        stream->state = ISP_STREAM_FAULT;
        return ISP_E_STALE;
    }
    if ((s->errors & ~DMA_ERROR_INPUT_PAUSED) || (s->common_irq & 2u) ||
        (s->output_irq & 0x70u) || isp_tune_error_set()) {
        stream->state = ISP_STREAM_FAULT;
        return ISP_E_IO;
    }
    return ISP_OK;
}

static bool address_matches(uint32_t offset, uint64_t expected)
{
    return REG_RD(offset) == (uint32_t)expected && REG_RD(offset + 4u) == (uint32_t)(expected >> 32);
}

/* Không cấp quyền theo cfg trong RAM nếu thanh ghi DMA đang trỏ sang vùng nhớ khác. */
static bool dma_configuration_matches(const isp_config_t *cfg, uint8_t outputs)
{
    if (REG_RD(FX1_ISP_IDMA_STRIDE_OFFSET) != cfg->dma.input_stride ||
        (REG_RD(R_COMMON_FRAME_WIDTH) & 0x1fffu) != cfg->ifmt.width ||
        (REG_RD(R_COMMON_FRAME_HEIGHT) & 0x1fffu) != cfg->ifmt.height ||
        (REG_RD(R_COMMON_BAYER) & 3u) != (uint32_t)cfg->ifmt.bayer ||
        (REG_RD(R_COMMON_INPUT_FORMAT) & 3u) != cfg->ifmt.depth) return false;
    for (unsigned i = 0; i < 4; i++)
        if (!address_matches(FX1_ISP_IDMA_BUF_ADDR_L0_OFFSET + 8u * i, cfg->dma.input[i])) return false;
    for (unsigned c = 0; c < 3; c++) if (outputs & (1u << c)) {
        uint32_t page = 0x200u * c;
        if (REG_RD(FX1_ISP_ODMA_Y_STRIDE_OFFSET + page) != cfg->dma.output[c].y_stride ||
            (c < 2 && REG_RD(FX1_ISP_ODMA_UV_STRIDE_OFFSET + page) != cfg->dma.output[c].uv_stride))
            return false;
        for (unsigned i = 0; i < 4; i++) {
            if (!address_matches(FX1_ISP_ODMA_Y_ADDR_L0_OFFSET + page + 16u * i, cfg->dma.output[c].y[i]) ||
                (c < 2 && !address_matches(FX1_ISP_ODMA_UV_ADDR_L0_OFFSET + page + 16u * i,
                                          cfg->dma.output[c].uv[i]))) return false;
        }
    }
    return true;
}

isp_status_t isp_stream_start(isp_stream_t *stream, const isp_config_t *cfg, const isp_wait_t *wait)
{
#if defined(ISP_STREAM_ALLOW_RETAINED_COUNTS) && ISP_STREAM_ALLOW_RETAINED_COUNTS
    static bool started;
    static uint32_t last_recovery_sequence;
#endif
    if (!stream || !cfg) return ISP_E_PARAM;
    if (stream->state != ISP_STREAM_CLOSED) return ISP_E_STATE;
    isp_status_t st = isp_validate_config(cfg);
    if (st != ISP_OK) return st;
    uint8_t outputs;
    st = check_buffers(cfg, &outputs);
    if (st != ISP_OK) return st;
    st = isp_require_legacy();
    if (st != ISP_OK) return st;
    if (isp_not_idle() || (REG_RD(R_COMMON_CTRL) & COMMON_CTRL_ISP_EN) ||
        (REG_RD(R_DMA_CTRL) & DMA_CTRL_ENABLE_MASK)) return ISP_E_NOT_IDLE;
    dma_frame_status_t s;
    dma_read_frame_status(&s);
#if defined(ISP_STREAM_ALLOW_RETAINED_COUNTS) && ISP_STREAM_ALLOW_RETAINED_COUNTS
    /* FreeRTOS may submit another job after reset with retained DMA counters. */
    if (s.input_valid || (s.recovery_status & 3u) != COMMON_RECOVERY_STATUS_DONE ||
        (started && s.recovery_sequence == last_recovery_sequence)) return ISP_E_STATE;
    for (unsigned c = 0; c < 3; c++)
        if (s.output_free[c] || s.output_done[c]) return ISP_E_STATE;
#else
    /* Chỉ nhận vòng slot mới sau init/reset, không đoán head từ số DONE của một stream cũ. */
    if (s.input_count) return ISP_E_STATE;
    for (unsigned c = 0; c < 3; c++)
        if (s.output_free[c] || s.output_done[c] || s.output_count[c]) return ISP_E_STATE;
#endif
    if (s.errors || (s.common_irq & 2u) || (s.output_irq & 0x70u) || isp_tune_error_set())
        return ISP_E_IO;
    if (!dma_configuration_matches(cfg, outputs)) return ISP_E_STATE;
    st = isp_enable_pipeline();
    if (st != ISP_OK) return st;
    uint16_t w, h;
    (void)ifmt_active_size(&cfg->ifmt, &w, &h);
    st = output_wait_geometry(&cfg->output, w, h, wait);
    if (st != ISP_OK || (REG_RD(FX1_ISP_OUTPUT_ENABLE_OFFSET) & 7u) != outputs) {
        (void)isp_clear_common_ctrl();
        return st == ISP_OK ? ISP_E_IO : st;
    }
    *stream = (isp_stream_t){.state = ISP_STREAM_READY, .outputs = outputs,
                            .recovery_sequence = s.recovery_sequence};
#if defined(ISP_STREAM_ALLOW_RETAINED_COUNTS) && ISP_STREAM_ALLOW_RETAINED_COUNTS
    last_recovery_sequence = s.recovery_sequence;
    started = true;
#endif
    return ISP_OK;
}

isp_status_t isp_stream_submit(isp_stream_t *stream, uint32_t tag, uint32_t timestamp)
{
    if (!stream) return ISP_E_PARAM;
    if (stream->state != ISP_STREAM_READY) return ISP_E_STATE;
    dma_frame_status_t s;
    isp_status_t st = check_hardware(stream, &s);
    if (st != ISP_OK) return st;
    stream->input_count = s.input_count;
    for (unsigned c = 0; c < 3; c++) stream->output_count[c] = s.output_count[c];
    st = dma_submit_slot(stream->next_slot, stream->outputs, tag, timestamp);
    if (st != ISP_OK) {
        /* Có thể đã giao một phần quyền trước khi đọc lại thất bại: không tự trả READY. */
        stream->state = ISP_STREAM_FAULT;
        return st;
    }
    stream->state = ISP_STREAM_RUNNING;
    return ISP_OK;
}

isp_status_t isp_stream_poll(isp_stream_t *stream, isp_frame_result_t *result)
{
    if (!stream || !result) return ISP_E_PARAM;
    if (stream->state != ISP_STREAM_RUNNING) return ISP_E_STATE;
    dma_frame_status_t s;
    isp_status_t st = check_hardware(stream, &s);
    if (st != ISP_OK) return st;
    uint32_t bit = 1u << stream->next_slot;
    if (s.input_valid || s.busy || isp_pipeline_busy() || s.input_count == stream->input_count)
        return ISP_E_NO_DATA;
    if (s.input_count != stream->input_count + 1u) {
        stream->state = ISP_STREAM_FAULT;
        return ISP_E_STALE;
    }
    for (unsigned c = 0; c < 3; c++) if (stream->outputs & (1u << c)) {
        if (!(s.output_done[c] & bit) || s.output_count[c] == stream->output_count[c])
            return ISP_E_NO_DATA;
        if (s.output_free[c] || s.output_done[c] != bit ||
            s.output_count[c] != stream->output_count[c] + 1u) {
            stream->state = ISP_STREAM_FAULT;
            return ISP_E_STALE;
        }
    }
    st = dma_pause();
    if (st == ISP_E_NOT_IDLE) return ISP_E_NO_DATA;
    if (st != ISP_OK) { stream->state = ISP_STREAM_FAULT; return st; }

    /* MAS 6.25.3.7: thiếu RAW kế khi chủ động chạy từng frame là trạng thái pause, không mất dữ liệu.
     * Chỉ xóa bit này sau khi đã tắt DMA; giữ nguyên snapshot để app thấy đã có thông báo pause. */
    REG_WR(R_DMA_ERR, DMA_ERROR_INPUT_PAUSED);
    REG_WR(FX1_ISP_DMA_IRQ_STAT_OFFSET, 1u << 3);
    isp_barrier();
    if (REG_RD(R_DMA_ERR)) { stream->state = ISP_STREAM_FAULT; return ISP_E_IO; }
    isp_memory_barrier();
    isp_frame_result_t completed = {.sequence = stream->sequence, .slot = stream->next_slot,
                                    .outputs = stream->outputs, .dma = s};
    static const uint32_t metadata_base[3] = {
        FX1_ISP_ODMA_BUF_TAG0_OFFSET, FX1_ISP_CH1_ODMA_BUF_TAG0_OFFSET,
        FX1_ISP_CH2_ODMA_BUF_TAG0_OFFSET
    };
    for (unsigned c = 0; c < 3; c++) if (stream->outputs & (1u << c)) {
        uint32_t base = metadata_base[c] + 4u * stream->next_slot;
        completed.tag[c] = REG_RD(base);
        completed.timestamp[c] = REG_RD(base + 0x10u);
        completed.sof_timestamp[c] = REG_RD(base + 0x20u);
        completed.eof_timestamp[c] = REG_RD(base + 0x30u);
    }
    dma_frame_status_t final_status;
    st = check_hardware(stream, &final_status);
    if (st != ISP_OK) return st;
    stream->completed = completed;
    stream->state = ISP_STREAM_HELD;
    *result = completed;
    return ISP_OK;
}

isp_status_t isp_stream_wait(isp_stream_t *stream, const isp_wait_t *wait, isp_frame_result_t *result)
{
    if (!stream || !result) return ISP_E_PARAM;
    isp_wait_t fallback;
    if (!wait) { isp_get_default_wait(&fallback); wait = &fallback; }
    if (!wait->poll_limit || (wait->time_us && !wait->timeout_us)) return ISP_E_PARAM;
    uint64_t start = wait->time_us ? wait->time_us(wait->user) : 0;
    for (uint32_t i = 0; i < wait->poll_limit; i++) {
        isp_status_t st = isp_stream_poll(stream, result);
        if (st != ISP_E_NO_DATA) return st;
        if (wait->time_us && wait->time_us(wait->user) - start >= wait->timeout_us) break;
    }
    return ISP_E_TIMEOUT;
}

isp_status_t isp_stream_release(isp_stream_t *stream)
{
    if (!stream) return ISP_E_PARAM;
    if (stream->state != ISP_STREAM_HELD) return ISP_E_STATE;
    dma_frame_status_t s;
    isp_status_t st = check_hardware(stream, &s);
    if (st != ISP_OK) return st;
    st = dma_release_slot(stream->next_slot, stream->outputs);
    if (st != ISP_OK) return st;
    /* ACK đúng nguồn completion; không xóa STATS_READY do lớp 3A sở hữu. */
    REG_WR(R_COMMON_IRQ_STATUS, 1u);
    REG_WR(FX1_ISP_OUTPUT_IRQ_STATUS_OFFSET, stream->outputs);
    REG_WR(FX1_ISP_DMA_IRQ_STAT_OFFSET, (1u << 1) | (1u << 2) | (1u << 4));
    isp_barrier();
    stream->next_slot = (stream->next_slot + 1u) & 3u;
    stream->sequence++;
    stream->state = ISP_STREAM_READY;
    return ISP_OK;
}

isp_status_t isp_stream_stop(isp_stream_t *stream, uint32_t poll_limit)
{
    if (!stream) return ISP_E_PARAM;
    if (stream->state != ISP_STREAM_READY) return ISP_E_STATE;
    dma_frame_status_t s;
    isp_status_t st = check_hardware(stream, &s);
    if (st != ISP_OK) return st;
    st = isp_stop_and_wait(poll_limit);
    if (st == ISP_OK) stream->state = ISP_STREAM_STOPPED;
    return st;
}
