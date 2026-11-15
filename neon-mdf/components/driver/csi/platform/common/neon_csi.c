/**
 * @file neon_csi.c
 * @brief CSI-2 receiver: validation, state machine and ownership rules, independent of the backend.
 */

/*==================================================================================================
*                                          INCLUDE FILES                                          
==================================================================================================*/
#include "csi/neon_csi.h"
#include "csi/platform/common/neon_csi_hw.h"
#include <stddef.h>
#include <string.h>

/*==================================================================================================
*                                        DEFINES AND MACROS                                       
==================================================================================================*/
#define CSI_DATA_TYPE_MAX 0x3FU
#define CSI_PACKED_RAW10_BYTES 5U
#define CSI_PACKED_RAW10_PIXELS 4U
#define CSI_PACKED_RAW12_BYTES 3U
#define CSI_PACKED_RAW12_PIXELS 2U
/* Portable starting values. configure() checks them against the caps of the chip. */
#define CSI_DEFAULT_BURST_BEATS 16U
#define CSI_DEFAULT_OUTSTANDING 4U
#define CSI_DEFAULT_EVENT_MASK (NEON_CSI_EVENT_FRAME_RECEIVED | NEON_CSI_EVENT_FRAME_DROPPED | \
                                NEON_CSI_EVENT_BUFFER_ERROR | NEON_CSI_EVENT_AXI_ERROR | NEON_CSI_EVENT_LANE_ABORT)

/*==================================================================================================
*                                         LOCAL FUNCTIONS                                         
==================================================================================================*/
static bool csi_raw_format_valid(neon_csi_raw_format_t format)
{
    return format == NEON_CSI_RAW10 || format == NEON_CSI_RAW12;
}

static bool csi_aligned(const neon_csi_caps_t *caps, uint32_t value)
{
    return caps->dma_align_bytes == 0U || (value % caps->dma_align_bytes) == 0U;
}

static bool csi_overlap(uint32_t a_base, uint64_t a_size, uint32_t b_base, uint64_t b_size)
{
    return (uint64_t)a_base < (uint64_t)b_base + b_size && (uint64_t)b_base < (uint64_t)a_base + a_size;
}

static bool csi_range_bad(const neon_csi_caps_t *caps, uint32_t base, uint64_t size)
{
    return base == 0U || !csi_aligned(caps, base) || size == 0U ||
           (uint64_t)base + size > (uint64_t)UINT32_MAX + 1U;
}

/* Frame ids are frame_id_bits wide and wrap. */
static uint32_t csi_frame_id_mask(const neon_csi_caps_t *caps)
{
    return (caps->frame_id_bits >= 32U) ? UINT32_MAX : (uint32_t)((1UL << caps->frame_id_bits) - 1UL);
}

/* Detect that the link under a configured receiver was reset. */
static neon_status_t csi_check_link(neon_csi_handle_t *handle)
{
    if (handle->state != NEON_CSI_STATE_CONFIGURED && handle->state != NEON_CSI_STATE_RUNNING) {
        return NEON_OK;
    }
    if (handle->mipi->config_epoch != handle->config_epoch) {
        handle->state = NEON_CSI_STATE_CONFIG_LOST;
        return NEON_ERROR;
    }
    if (handle->state == NEON_CSI_STATE_RUNNING && handle->mipi->phy_epoch != handle->phy_epoch) {
        handle->phy_epoch = handle->mipi->phy_epoch;
        handle->software_events |= NEON_CSI_EVENT_LINK_RESET;
    }
    return NEON_OK;
}

static neon_status_t csi_validate_config(const neon_csi_caps_t *caps, const neon_csi_config_t *config,
                                         const neon_csi_layout_t *layout)
{
    const neon_csi_dma_config_t *dma = &config->dma;
    const neon_csi_metadata_config_t *meta = &config->metadata;
    uint32_t i;
    uint32_t j;
    uint64_t meta_side_bytes;

    if (dma->error_policy != NEON_CSI_ERROR_POLICY_RELEASE_WITH_ERROR &&
        dma->error_policy != NEON_CSI_ERROR_POLICY_DISCARD_AND_REWIND) {
        return NEON_INVALID_PARAM;
    }
    if (dma->max_burst_beats == 0U || dma->max_burst_beats > caps->max_burst_beats ||
        dma->max_outstanding == 0U || dma->max_outstanding > caps->max_outstanding ||
        dma->qos > caps->max_qos || dma->prot > caps->max_prot ||
        dma->line_padding_bytes > caps->max_line_padding_bytes) {
        return NEON_INVALID_PARAM;
    }
    if (!csi_aligned(caps, layout->line_pitch_bytes) || dma->buffer_size_bytes < layout->frame_bytes) {
        return NEON_INVALID_PARAM;
    }
    for (i = 0U; i < caps->buffer_count; i++) {
        if (csi_range_bad(caps, dma->base_addr[i], dma->buffer_size_bytes)) {
            return NEON_INVALID_PARAM;
        }
        for (j = i + 1U; j < caps->buffer_count; j++) {
            if (csi_overlap(dma->base_addr[i], dma->buffer_size_bytes, dma->base_addr[j], dma->buffer_size_bytes)) {
                return NEON_INVALID_PARAM;
            }
        }
    }
    if ((config->event_mask & ~(caps->supported_events | NEON_CSI_EVENT_LINK_RESET)) != 0UL) {
        return NEON_INVALID_PARAM;
    }
    if (meta->extract_enable && !meta->enable) {
        return NEON_INVALID_PARAM;
    }
    for (i = 0U; i < NEON_CSI_MAX_EXTRACT_SLOTS; i++) {
        if (meta->extract[i].length_bytes > 4U) {
            return NEON_INVALID_PARAM;
        }
        if (i >= caps->extract_slots && meta->extract[i].length_bytes != 0U) {
            return NEON_NOT_SUPPORTED; /* The chip has fewer slots than the HAL structure can hold. */
        }
    }
    if (meta->enable) {
        if (caps->meta_record_min_bytes == 0U) {
            return NEON_NOT_SUPPORTED;
        }
        if (meta->data_type > CSI_DATA_TYPE_MAX || meta->area_size_bytes < caps->meta_record_min_bytes) {
            return NEON_INVALID_PARAM;
        }
        /* One side holds one area per virtual channel. */
        meta_side_bytes = (uint64_t)caps->vc_count * meta->area_size_bytes;
        for (i = 0U; i < caps->buffer_count; i++) {
            if (csi_range_bad(caps, meta->base_addr[i], meta_side_bytes)) {
                return NEON_INVALID_PARAM;
            }
            for (j = 0U; j < caps->buffer_count; j++) {
                if (csi_overlap(meta->base_addr[i], meta_side_bytes, dma->base_addr[j], dma->buffer_size_bytes) ||
                    (j > i && csi_overlap(meta->base_addr[i], meta_side_bytes, meta->base_addr[j], meta_side_bytes))) {
                    return NEON_INVALID_PARAM;
                }
            }
        }
    }
    return NEON_OK;
}

static neon_status_t csi_calculate_layout(const neon_csi_caps_t *caps, const neon_csi_config_t *config,
                                         neon_csi_layout_t *layout)
{
    neon_csi_raw_format_t raw_format;
    uint32_t width;
    uint32_t height;
    uint32_t bytes_per_line;
    uint64_t frame_bytes;

    if (config == NULL || layout == NULL) {
        return NEON_INVALID_PARAM;
    }
    if (config->output_format != NEON_CSI_OUTPUT_PACKED && config->output_format != NEON_CSI_OUTPUT_RAW16) {
        return NEON_INVALID_PARAM;
    }
    if (config->test_pattern.enable) {
        if (!caps->test_pattern) {
            return NEON_NOT_SUPPORTED;
        }
        raw_format = config->test_pattern.pixel_format;
        width = config->test_pattern.width;
        if (caps->tpg_width_granularity > 1U) {
            width -= width % caps->tpg_width_granularity; /* The generator truncates the width. */
        }
        height = config->test_pattern.height;
        if (width < caps->min_tpg_dimension || height < caps->min_tpg_dimension) {
            return NEON_INVALID_PARAM;
        }
    } else {
        raw_format = config->raw_format;
        width = config->width;
        height = config->height;
    }
    if (!csi_raw_format_valid(raw_format) || width == 0U || height == 0U) {
        return NEON_INVALID_PARAM;
    }
    /* A line is a whole number of packing units: 4 pixels in 5 bytes, 2 pixels in 3 bytes. */
    if (raw_format == NEON_CSI_RAW10 && (width % CSI_PACKED_RAW10_PIXELS) != 0U) {
        return NEON_INVALID_PARAM;
    }
    if (raw_format == NEON_CSI_RAW12 && (width % CSI_PACKED_RAW12_PIXELS) != 0U) {
        return NEON_INVALID_PARAM;
    }
    if (config->output_format == NEON_CSI_OUTPUT_RAW16) {
        bytes_per_line = width * 2U;
    } else if (raw_format == NEON_CSI_RAW10) {
        bytes_per_line = (width / CSI_PACKED_RAW10_PIXELS) * CSI_PACKED_RAW10_BYTES;
    } else {
        bytes_per_line = (width / CSI_PACKED_RAW12_PIXELS) * CSI_PACKED_RAW12_BYTES;
    }
    if (bytes_per_line > caps->max_bytes_per_line) {
        return NEON_NOT_SUPPORTED;
    }
    frame_bytes = (uint64_t)(bytes_per_line + config->dma.line_padding_bytes) * height;
    if (frame_bytes > UINT32_MAX) {
        return NEON_NOT_SUPPORTED;
    }
    layout->width = (uint16_t)width;
    layout->height = (uint16_t)height;
    layout->bytes_per_line = bytes_per_line;
    layout->line_pitch_bytes = bytes_per_line + config->dma.line_padding_bytes;
    layout->frame_bytes = (uint32_t)frame_bytes;
    layout->output_format = config->output_format;
    layout->raw_format = raw_format;
    layout->sample_align = (config->output_format == NEON_CSI_OUTPUT_RAW16) ? NEON_CSI_ALIGN_RIGHT : NEON_CSI_ALIGN_NONE;
    return NEON_OK;
}

/*
 * Oldest of the buffers that software may take. When every candidate has an exact context the frame ids
 * decide, as modular differences, since they wrap. Otherwise the per-buffer frame id is unknown and the DMA
 * is assumed to cycle through the buffers, so the first ready one from order_hint on is the oldest.
 */
static uint8_t csi_pick_oldest(const neon_csi_handle_t *handle, const neon_csi_frame_t candidate[], const bool ready[])
{
    const uint8_t count = handle->caps.buffer_count;
    const uint32_t mask = csi_frame_id_mask(&handle->caps);
    bool exact = true;
    uint8_t i;
    uint8_t j;

    for (i = 0U; i < count; i++) {
        if (ready[i] && !candidate[i].context_valid) {
            exact = false;
        }
    }
    for (i = 0U; i < count; i++) {
        bool oldest = ready[i] && exact;

        for (j = 0U; oldest && j < count; j++) {
            /* j is not older than i when it is at most half the id range ahead. */
            if (j != i && ready[j] && ((candidate[j].frame_id - candidate[i].frame_id) & mask) > mask / 2U) {
                oldest = false;
            }
        }
        if (oldest) {
            return i;
        }
    }
    for (j = 0U; j < count; j++) {
        i = (uint8_t)((handle->order_hint + j) % count);
        if (ready[i]) {
            return i;
        }
    }
    return 0U;
}

static neon_status_t csi_fill_frame(neon_csi_handle_t *handle, uint8_t index, neon_csi_buffer_state_t state,
                                    neon_csi_frame_t *frame)
{
    neon_status_t status;

    (void)memset(frame, 0, sizeof(*frame));
    frame->buffer_index = index;
    frame->state = state;
    frame->base_addr = handle->raw_base_addr[index];
    frame->layout = handle->layout;
    status = neon_csi_hw_read_frame(handle, index, frame);
    if (status == NEON_OK && !frame->context_valid) {
        /* What the registers show belongs to a newer capture: do not let it pass for this buffer's. */
        frame->frame_id = 0U;
        frame->timestamp_ns = 0U;
        frame->record_id = 0U;
        frame->last_valid_line = 0U;
        (void)memset(frame->vc, 0, sizeof(frame->vc));
    }
    return status;
}

/*==================================================================================================
*                                         GLOBAL FUNCTIONS                                        
==================================================================================================*/
neon_status_t neon_csi_init(neon_csi_handle_t *handle, neon_mipi_handle_t *mipi)
{
    if (handle == NULL || mipi == NULL) {
        return NEON_INVALID_PARAM;
    }
    (void)memset(handle, 0, sizeof(*handle));
    handle->mipi = mipi;
    handle->state = NEON_CSI_STATE_IDLE;
    if (neon_csi_hw_get_caps(&handle->caps) != NEON_OK) {
        return NEON_ERROR;
    }
    return neon_csi_hw_init(handle);
}

neon_status_t neon_csi_deinit(neon_csi_handle_t *handle)
{
    if (handle == NULL) {
        return NEON_INVALID_PARAM;
    }
    if (handle->state == NEON_CSI_STATE_RUNNING || handle->link_locked) {
        return NEON_BUSY;
    }
    (void)memset(handle, 0, sizeof(*handle));
    return NEON_OK;
}

void neon_csi_get_default_config(neon_csi_config_t *config)
{
    if (config == NULL) {
        return;
    }
    (void)memset(config, 0, sizeof(*config));
    config->raw_format = NEON_CSI_RAW12;
    config->output_format = NEON_CSI_OUTPUT_RAW16;
    config->dma.max_burst_beats = CSI_DEFAULT_BURST_BEATS;
    config->dma.max_outstanding = CSI_DEFAULT_OUTSTANDING;
    config->dma.error_policy = NEON_CSI_ERROR_POLICY_RELEASE_WITH_ERROR;
    config->test_pattern.pixel_format = NEON_CSI_RAW12;
    config->event_mask = CSI_DEFAULT_EVENT_MASK;
}


neon_status_t neon_csi_calculate_layout(const neon_csi_config_t *config, neon_csi_layout_t *layout)
{
    neon_csi_caps_t caps;

    if (neon_csi_hw_get_caps(&caps) != NEON_OK) {
        return NEON_ERROR;
    }
    return csi_calculate_layout(&caps, config, layout);
}

neon_status_t neon_csi_get_caps(neon_csi_caps_t *caps)
{
    if (caps == NULL) {
        return NEON_INVALID_PARAM;
    }
    return neon_csi_hw_get_caps(caps);
}

neon_status_t neon_csi_configure(neon_csi_handle_t *handle, const neon_csi_config_t *config)
{
    neon_csi_layout_t layout;
    neon_status_t status;
    uint32_t i;

    if (handle == NULL || config == NULL) {
        return NEON_INVALID_PARAM;
    }
    if (handle->state == NEON_CSI_STATE_RUNNING || handle->link_locked) {
        return NEON_BUSY;
    }
    if (handle->mipi->state == NEON_MIPI_STATE_RELEASED) {
        return NEON_ERROR;
    }
    status = csi_calculate_layout(&handle->caps, config, &layout);
    if (status == NEON_OK) {
        status = csi_validate_config(&handle->caps, config, &layout);
    }
    if (status == NEON_OK) {
        status = neon_csi_hw_configure(handle, config, &layout);
    }
    if (status != NEON_OK) {
        return status;
    }
    handle->layout = layout;
    handle->event_mask = config->event_mask;
    handle->meta_enabled = config->metadata.enable;
    handle->meta_area_size_bytes = config->metadata.area_size_bytes;
    for (i = 0U; i < handle->caps.buffer_count; i++) {
        handle->raw_base_addr[i] = config->dma.base_addr[i];
        handle->meta_base_addr[i] = config->metadata.base_addr[i];
    }
    handle->acquired_mask = 0U;
    handle->order_hint = 0U;
    handle->software_events = 0UL;
    handle->config_epoch = handle->mipi->config_epoch;
    handle->state = NEON_CSI_STATE_CONFIGURED;
    return NEON_OK;
}

neon_status_t neon_csi_start(neon_csi_handle_t *handle)
{
    neon_status_t status;

    if (handle == NULL) {
        return NEON_INVALID_PARAM;
    }
    if (handle->state == NEON_CSI_STATE_RUNNING) {
        return NEON_BUSY;
    }
    if (csi_check_link(handle) != NEON_OK || handle->state != NEON_CSI_STATE_CONFIGURED) {
        return NEON_ERROR;
    }
    status = neon_mipi_lock(handle->mipi); /* Fails unless the link is ready: MAS 9.1 steps 1 to 5 come first. */
    if (status != NEON_OK) {
        return status;
    }
    status = neon_csi_hw_start(handle);
    if (status != NEON_OK) {
        (void)neon_mipi_unlock(handle->mipi);
        return status;
    }
    handle->link_locked = true;
    handle->phy_epoch = handle->mipi->phy_epoch;
    handle->acquired_mask = 0U;
    handle->order_hint = 0U;
    handle->state = NEON_CSI_STATE_RUNNING;
    return NEON_OK;
}

neon_status_t neon_csi_stop(neon_csi_handle_t *handle)
{
    neon_status_t status;

    if (handle == NULL) {
        return NEON_INVALID_PARAM;
    }
    (void)csi_check_link(handle);
    if (handle->state != NEON_CSI_STATE_RUNNING && !(handle->state == NEON_CSI_STATE_CONFIG_LOST && handle->link_locked)) {
        return NEON_ERROR;
    }
    status = neon_csi_hw_stop(handle);
    if (status != NEON_OK) {
        return status; /* The DMA may still run: keep the lock and let the caller retry or recover. */
    }
    if (handle->link_locked) {
        (void)neon_mipi_unlock(handle->mipi);
        handle->link_locked = false;
    }
    handle->acquired_mask = 0U;
    if (handle->state == NEON_CSI_STATE_RUNNING) {
        handle->state = NEON_CSI_STATE_CONFIGURED;
    }
    return NEON_OK;
}

neon_status_t neon_csi_recover(neon_csi_handle_t *handle, neon_csi_recover_level_t level)
{
    neon_mipi_status_t link;
    neon_status_t status;

    if (handle == NULL || level != NEON_CSI_RECOVER_SOFT) {
        return NEON_INVALID_PARAM;
    }
    if (csi_check_link(handle) != NEON_OK || handle->state != NEON_CSI_STATE_RUNNING) {
        return NEON_ERROR;
    }
    status = neon_mipi_get_status(handle->mipi, &link);
    if (status != NEON_OK) {
        return status;
    }
    if (!link.clock_hs_active) {
        return NEON_ERROR; /* soft_reset is synchronised into rxbyteclkhs and does nothing while it is stopped. */
    }
    status = neon_csi_hw_soft_reset(handle);
    if (status == NEON_OK) {
        handle->software_events &= ~(uint32_t)NEON_CSI_EVENT_LINK_RESET;
        handle->acquired_mask = 0U; /* Both sides are hardware owned again. */
        handle->order_hint = 0U;
    }
    return status;
}

neon_status_t neon_csi_get_status(neon_csi_handle_t *handle, neon_csi_status_t *status)
{
    uint32_t events;
    neon_status_t result;

    if (handle == NULL || status == NULL) {
        return NEON_INVALID_PARAM;
    }
    (void)csi_check_link(handle);
    (void)memset(status, 0, sizeof(*status));
    status->state = handle->state;
    if (handle->state != NEON_CSI_STATE_CONFIGURED && handle->state != NEON_CSI_STATE_RUNNING) {
        return NEON_OK;
    }
    result = neon_csi_hw_read_buffer_state(handle, status->buffer_state);
    if (result == NEON_OK) {
        result = neon_csi_hw_read_counters(handle, status);
    }
    if (result == NEON_OK) {
        result = neon_csi_hw_read_events(handle, &events);
    }
    if (result == NEON_OK) {
        status->irq_pending = ((events | handle->software_events) & handle->event_mask) != 0UL;
    }
    return result;
}

neon_status_t neon_csi_get_events(neon_csi_handle_t *handle, uint32_t *events)
{
    neon_status_t status;

    if (handle == NULL || events == NULL) {
        return NEON_INVALID_PARAM;
    }
    (void)csi_check_link(handle);
    if (handle->state != NEON_CSI_STATE_CONFIGURED && handle->state != NEON_CSI_STATE_RUNNING) {
        *events = handle->software_events;
        return NEON_OK;
    }
    status = neon_csi_hw_read_events(handle, events);
    if (status == NEON_OK) {
        *events |= handle->software_events;
    }
    return status;
}

neon_status_t neon_csi_clear_events(neon_csi_handle_t *handle, uint32_t events)
{
    neon_status_t status = NEON_OK;

    if (handle == NULL) {
        return NEON_INVALID_PARAM;
    }
    handle->software_events &= ~events;
    if ((events & handle->caps.supported_events) != 0UL &&
        (handle->state == NEON_CSI_STATE_CONFIGURED || handle->state == NEON_CSI_STATE_RUNNING)) {
        status = neon_csi_hw_clear_events(handle, events & handle->caps.supported_events);
    }
    return status;
}

neon_status_t neon_csi_set_event_mask(neon_csi_handle_t *handle, uint32_t event_mask)
{
    neon_status_t status;

    if (handle == NULL || (event_mask & ~(handle->caps.supported_events | NEON_CSI_EVENT_LINK_RESET)) != 0UL) {
        return NEON_INVALID_PARAM;
    }
    if (handle->state != NEON_CSI_STATE_CONFIGURED && handle->state != NEON_CSI_STATE_RUNNING) {
        return NEON_ERROR;
    }
    status = neon_csi_hw_set_event_mask(handle, event_mask);
    if (status == NEON_OK) {
        handle->event_mask = event_mask;
    }
    return status;
}

neon_status_t neon_csi_acquire_frame(neon_csi_handle_t *handle, neon_csi_frame_t *frame)
{
    neon_csi_buffer_state_t state[NEON_CSI_MAX_BUFFERS];
    neon_csi_frame_t candidate[NEON_CSI_MAX_BUFFERS];
    bool ready[NEON_CSI_MAX_BUFFERS];
    uint8_t count;
    uint8_t chosen;
    uint8_t ready_count = 0U;
    uint8_t i;
    neon_status_t status;

    if (handle == NULL || frame == NULL) {
        return NEON_INVALID_PARAM;
    }
    (void)csi_check_link(handle);
    if (handle->state != NEON_CSI_STATE_RUNNING) {
        return NEON_ERROR;
    }
    count = handle->caps.buffer_count;
    (void)memset(state, 0, sizeof(state));
    status = neon_csi_hw_read_buffer_state(handle, state);
    if (status != NEON_OK) {
        return status;
    }
    for (i = 0U; i < count; i++) {
        ready[i] = state[i] != NEON_CSI_BUFFER_HW_OWNED && (handle->acquired_mask & (1U << i)) == 0U;
        if (ready[i]) {
            status = csi_fill_frame(handle, i, state[i], &candidate[i]);
            if (status != NEON_OK) {
                return status;
            }
            ready_count++;
        }
    }
    if (ready_count == 0U) {
        return NEON_BUSY;
    }
    chosen = csi_pick_oldest(handle, candidate, ready);
    *frame = candidate[chosen];
    handle->acquired_mask |= (uint8_t)(1U << chosen);
    handle->order_hint = (uint8_t)((chosen + 1U) % count);
    return NEON_OK;
}

neon_status_t neon_csi_release_frame(neon_csi_handle_t *handle, uint8_t buffer_index)
{
    neon_status_t status;

    if (handle == NULL || buffer_index >= handle->caps.buffer_count) {
        return NEON_INVALID_PARAM;
    }
    (void)csi_check_link(handle);
    if (handle->state != NEON_CSI_STATE_RUNNING) {
        return NEON_ERROR;
    }
    if ((handle->acquired_mask & (1U << buffer_index)) == 0U) {
        return NEON_INVALID_PARAM;
    }
    status = neon_csi_hw_release_buffer(handle, buffer_index);
    if (status == NEON_OK) {
        handle->acquired_mask &= (uint8_t)~(1U << buffer_index);
    }
    return status;
}

neon_status_t neon_csi_set_record_id(neon_csi_handle_t *handle, uint32_t record_id)
{
    if (handle == NULL) {
        return NEON_INVALID_PARAM;
    }
    (void)csi_check_link(handle);
    if (handle->state != NEON_CSI_STATE_CONFIGURED && handle->state != NEON_CSI_STATE_RUNNING) {
        return NEON_ERROR;
    }
    return neon_csi_hw_set_record_id(handle, record_id);
}

neon_status_t neon_csi_read_short_packet(neon_csi_handle_t *handle, neon_csi_short_packet_t *packet)
{
    if (handle == NULL || packet == NULL) {
        return NEON_INVALID_PARAM;
    }
    (void)csi_check_link(handle);
    if (handle->state != NEON_CSI_STATE_RUNNING) {
        return NEON_ERROR;
    }
    return neon_csi_hw_read_short_packet(handle, packet);
}
