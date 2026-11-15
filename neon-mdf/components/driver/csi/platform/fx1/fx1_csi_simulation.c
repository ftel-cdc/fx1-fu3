/**
 * @file fx1_csi_simulation.c
 * @brief In-RAM backend of the CSI-2 receiver.
 */

/*==================================================================================================
*                                          INCLUDE FILES                                          
==================================================================================================*/
#include "csi/platform/fx1/fx1_csi_simulation.h"
#include "csi/platform/common/neon_csi_hw.h"
#include <string.h>

/*==================================================================================================
*                                        DEFINES AND MACROS                                       
==================================================================================================*/
#define SIM_DATA_TYPE_RAW10 0x2BU
#define SIM_DATA_TYPE_RAW12 0x2CU
#define SIM_CORE_STATE_RUN 1U

/*==================================================================================================
*                                  STRUCTURES AND OTHER TYPEDEFS                                  
==================================================================================================*/
typedef struct {
    uint32_t frame_id;
    uint64_t timestamp_ns;
    uint32_t record_id;
    uint32_t last_valid_line;
    neon_csi_vc_info_t vc[NEON_CSI_MAX_VCS];
} sim_context_t;

typedef struct {
    bool configured;
    bool running;
    bool stop_fails;
    bool short_packet_valid;
    neon_csi_error_policy_t policy;
    neon_csi_buffer_state_t ownership[NEON_CSI_MAX_BUFFERS];
    uint8_t write_side;
    int8_t last_completed;       /* -1 until the first capture completes. */
    uint32_t next_frame_id;
    uint32_t record_id;
    uint32_t events;
    uint32_t event_mask;
    uint32_t packet_count;
    uint32_t frame_count;
    uint32_t ecc_error_count;
    uint32_t crc_error_count;
    uint32_t frame_drop_count;
    bool extract_enable;
    neon_csi_extract_t extract[NEON_CSI_MAX_EXTRACT_SLOTS];
    sim_context_t context[NEON_CSI_MAX_BUFFERS];
    neon_csi_short_packet_t short_packet;
    fx1_csi_sim_counters_t counters;
} csi_sim_t;

_Static_assert(sizeof(csi_sim_t) <= sizeof(((neon_csi_handle_t *)0)->backend_words),
               "csi_sim_t does not fit in the handle");

/*==================================================================================================
*                                         LOCAL FUNCTIONS                                         
==================================================================================================*/
static csi_sim_t *csi_sim(neon_csi_handle_t *handle)
{
    return (csi_sim_t *)(void *)handle->backend_words;
}

static const csi_sim_t *csi_sim_const(const neon_csi_handle_t *handle)
{
    return (const csi_sim_t *)(const void *)handle->backend_words;
}

static void sim_arm_all(neon_csi_handle_t *handle)
{
    csi_sim_t *sim = csi_sim(handle);
    uint32_t i;

    for (i = 0U; i < handle->caps.buffer_count; i++) {
        sim->ownership[i] = NEON_CSI_BUFFER_HW_OWNED;
    }
    sim->write_side = 0U;
}

/* The side the DMA writes next: the first hardware-owned one from write_side on, or -1 when software holds all. */
static int sim_pick_side(const neon_csi_handle_t *handle)
{
    const csi_sim_t *sim = csi_sim_const(handle);
    const uint8_t count = handle->caps.buffer_count;
    uint8_t step;

    for (step = 0U; step < count; step++) {
        uint8_t side = (uint8_t)((sim->write_side + step) % count);

        if (sim->ownership[side] == NEON_CSI_BUFFER_HW_OWNED) {
            return (int)side;
        }
    }
    return -1;
}

static uint32_t sim_frame_id_mask(const neon_csi_handle_t *handle)
{
    return (handle->caps.frame_id_bits >= 32U) ? UINT32_MAX : (uint32_t)((1UL << handle->caps.frame_id_bits) - 1UL);
}

static void sim_fill_context(neon_csi_handle_t *handle, sim_context_t *context, uint32_t frame_id,
                             const fx1_csi_sim_frame_t *frame)
{
    csi_sim_t *sim = csi_sim(handle);
    uint8_t vc_mask = (frame->vc_mask == 0U) ? 1U : frame->vc_mask;
    uint32_t vc;
    uint32_t slot;

    (void)memset(context, 0, sizeof(*context));
    context->frame_id = frame_id;
    context->timestamp_ns = frame->timestamp_ns;
    context->record_id = sim->record_id;
    context->last_valid_line = frame->fault ? frame->last_valid_line : handle->layout.height;
    for (vc = 0U; vc < handle->caps.vc_count; vc++) {
        neon_csi_vc_info_t *info = &context->vc[vc];

        if ((vc_mask & (1U << vc)) == 0U) {
            continue;
        }
        info->present = true;
        info->byte_count = (uint16_t)handle->layout.bytes_per_line;
        info->line_count = (uint16_t)context->last_valid_line;
        info->data_type = (handle->layout.raw_format == NEON_CSI_RAW10) ? SIM_DATA_TYPE_RAW10 : SIM_DATA_TYPE_RAW12;
        info->err_summary = frame->fault ? 1U : 0U;
        if (sim->extract_enable) {
            for (slot = 0U; slot < handle->caps.extract_slots; slot++) {
                if (sim->extract[slot].length_bytes != 0U) {
                    info->extracted[slot] = frame->extracted[vc][slot];
                }
            }
        }
    }
}

/*==================================================================================================
*                                         GLOBAL FUNCTIONS                                        
==================================================================================================*/
neon_status_t neon_csi_hw_init(neon_csi_handle_t *handle)
{
    csi_sim_t *sim = csi_sim(handle);

    (void)memset(handle->backend_words, 0, sizeof(handle->backend_words));
    sim->last_completed = -1;
    return NEON_OK;
}

neon_status_t neon_csi_hw_configure(neon_csi_handle_t *handle, const neon_csi_config_t *config, const neon_csi_layout_t *layout)
{
    csi_sim_t *sim = csi_sim(handle);
    fx1_csi_sim_counters_t counters = sim->counters;
    uint32_t i;

    (void)layout; /* The generic part stores it in the handle. */
    (void)memset(sim, 0, sizeof(*sim));
    sim->counters = counters;
    sim->counters.configures++;
    sim->configured = true;
    sim->last_completed = -1;
    sim->policy = config->dma.error_policy;
    sim->record_id = config->metadata.record_id;
    sim->event_mask = config->event_mask;
    sim->extract_enable = config->metadata.enable && config->metadata.extract_enable;
    for (i = 0U; i < NEON_CSI_MAX_EXTRACT_SLOTS; i++) {
        sim->extract[i] = config->metadata.extract[i];
    }
    return NEON_OK;
}

neon_status_t neon_csi_hw_start(neon_csi_handle_t *handle)
{
    csi_sim_t *sim = csi_sim(handle);

    if (!sim->configured) {
        return NEON_ERROR;
    }
    sim_arm_all(handle);
    sim->last_completed = -1;
    sim->running = true;
    sim->counters.starts++;
    return NEON_OK;
}

neon_status_t neon_csi_hw_stop(neon_csi_handle_t *handle)
{
    csi_sim_t *sim = csi_sim(handle);

    if (sim->stop_fails) {
        return NEON_TIMEOUT;
    }
    sim->running = false;
    sim->counters.stops++;
    return NEON_OK;
}

neon_status_t neon_csi_hw_soft_reset(neon_csi_handle_t *handle)
{
    csi_sim_t *sim = csi_sim(handle);

    /* The datapath restarts and both buffers are released: every capture in flight or held is lost. */
    sim_arm_all(handle);
    sim->last_completed = -1;
    sim->events = 0UL;
    sim->counters.soft_resets++;
    return NEON_OK;
}

neon_status_t neon_csi_hw_read_events(neon_csi_handle_t *handle, uint32_t *events)
{
    *events = csi_sim(handle)->events;
    return NEON_OK;
}

neon_status_t neon_csi_hw_clear_events(neon_csi_handle_t *handle, uint32_t events)
{
    csi_sim(handle)->events &= ~events; /* Write one to clear. */
    return NEON_OK;
}

neon_status_t neon_csi_hw_set_event_mask(neon_csi_handle_t *handle, uint32_t event_mask)
{
    csi_sim(handle)->event_mask = event_mask & handle->caps.supported_events;
    return NEON_OK;
}

neon_status_t neon_csi_hw_read_buffer_state(neon_csi_handle_t *handle, neon_csi_buffer_state_t state[NEON_CSI_MAX_BUFFERS])
{
    const csi_sim_t *sim = csi_sim(handle);
    uint32_t i;

    for (i = 0U; i < handle->caps.buffer_count; i++) {
        state[i] = sim->ownership[i];
    }
    return NEON_OK;
}

neon_status_t neon_csi_hw_read_frame(neon_csi_handle_t *handle, uint8_t buffer_index, neon_csi_frame_t *frame)
{
    const csi_sim_t *sim = csi_sim(handle);
    const sim_context_t *context;

    if (buffer_index >= handle->caps.buffer_count || sim->ownership[buffer_index] == NEON_CSI_BUFFER_HW_OWNED) {
        return NEON_ERROR;
    }
    /*
     * With the metadata record the context of this very buffer is in DDR. Without it the registers
     * hold only the last completed capture, which is not this buffer once a newer one has finished.
     */
    frame->context_valid = handle->meta_enabled || sim->last_completed == (int8_t)buffer_index;
    context = frame->context_valid ? &sim->context[buffer_index] : &sim->context[sim->last_completed];
    frame->frame_id = context->frame_id;
    frame->timestamp_ns = context->timestamp_ns;
    frame->record_id = context->record_id;
    frame->last_valid_line = context->last_valid_line;
    (void)memcpy(frame->vc, context->vc, sizeof(frame->vc));
    return NEON_OK;
}

neon_status_t neon_csi_hw_release_buffer(neon_csi_handle_t *handle, uint8_t buffer_index)
{
    csi_sim_t *sim = csi_sim(handle);

    if (buffer_index >= handle->caps.buffer_count || sim->ownership[buffer_index] == NEON_CSI_BUFFER_HW_OWNED) {
        return NEON_ERROR;
    }
    sim->ownership[buffer_index] = NEON_CSI_BUFFER_HW_OWNED;
    return NEON_OK;
}

neon_status_t neon_csi_hw_read_counters(neon_csi_handle_t *handle, neon_csi_status_t *status)
{
    const csi_sim_t *sim = csi_sim(handle);

    status->packet_count = sim->packet_count;
    status->frame_count = sim->frame_count;
    status->ecc_error_count = sim->ecc_error_count;
    status->crc_error_count = sim->crc_error_count;
    status->frame_drop_count = sim->frame_drop_count;
    status->core_state = sim->running ? (uint8_t)SIM_CORE_STATE_RUN : 0U;
    return NEON_OK;
}

neon_status_t neon_csi_hw_set_record_id(neon_csi_handle_t *handle, uint32_t record_id)
{
    csi_sim(handle)->record_id = record_id;
    return NEON_OK;
}

neon_status_t neon_csi_hw_read_short_packet(neon_csi_handle_t *handle, neon_csi_short_packet_t *packet)
{
    csi_sim_t *sim = csi_sim(handle);

    if (!sim->short_packet_valid) {
        return NEON_BUSY; /* Nothing pending. */
    }
    *packet = sim->short_packet;
    sim->short_packet_valid = false;
    return NEON_OK;
}

neon_status_t fx1_csi_sim_push_frame(neon_csi_handle_t *handle, const fx1_csi_sim_frame_t *frame)
{
    csi_sim_t *sim;
    uint32_t frame_id;
    int side;

    if (handle == NULL || frame == NULL) {
        return NEON_INVALID_PARAM;
    }
    sim = csi_sim(handle);
    if (!sim->running) {
        return NEON_ERROR;
    }
    frame_id = sim->next_frame_id & sim_frame_id_mask(handle);
    sim->next_frame_id = (sim->next_frame_id + 1U) & sim_frame_id_mask(handle);
    sim->packet_count += handle->layout.height;
    sim->ecc_error_count += frame->ecc_errors;
    sim->crc_error_count += frame->crc_errors;
    sim->events |= frame->extra_events & handle->caps.supported_events;

    side = sim_pick_side(handle);
    if (side < 0) {
        /* The incoming frame is dropped before it reaches DDR: both sides are held by software. */
        sim->frame_drop_count++;
        sim->events |= NEON_CSI_EVENT_FRAME_DROPPED;
        return NEON_BUSY;
    }
    if (frame->fault && sim->policy == NEON_CSI_ERROR_POLICY_DISCARD_AND_REWIND) {
        /* The side stays hardware owned and is written again from its first line. */
        sim->frame_drop_count++;
        sim->events |= NEON_CSI_EVENT_BUFFER_ERROR | NEON_CSI_EVENT_FRAME_DROPPED;
        return NEON_OK;
    }
    sim_fill_context(handle, &sim->context[side], frame_id, frame);
    sim->last_completed = (int8_t)side;
    sim->frame_count++;
    sim->write_side = (uint8_t)((unsigned)(side + 1) % handle->caps.buffer_count);
    if (frame->fault) {
        sim->ownership[side] = NEON_CSI_BUFFER_SW_ERROR;
        sim->events |= NEON_CSI_EVENT_BUFFER_ERROR | NEON_CSI_EVENT_FRAME_RECEIVED;
    } else {
        sim->ownership[side] = NEON_CSI_BUFFER_SW_VALID;
        sim->events |= NEON_CSI_EVENT_BUFFER_VALID | NEON_CSI_EVENT_FRAME_RECEIVED;
    }
    return NEON_OK;
}

void fx1_csi_sim_set_next_frame_id(neon_csi_handle_t *handle, uint32_t frame_id)
{
    csi_sim(handle)->next_frame_id = frame_id & sim_frame_id_mask(handle);
}

void fx1_csi_sim_raise_events(neon_csi_handle_t *handle, uint32_t events)
{
    csi_sim(handle)->events |= events & handle->caps.supported_events;
}

void fx1_csi_sim_push_short_packet(neon_csi_handle_t *handle, const neon_csi_short_packet_t *packet)
{
    csi_sim_t *sim = csi_sim(handle);

    sim->short_packet = *packet;
    sim->short_packet_valid = true;
}

void fx1_csi_sim_set_stop_fails(neon_csi_handle_t *handle, bool fails)
{
    csi_sim(handle)->stop_fails = fails;
}

void fx1_csi_sim_get_counters(const neon_csi_handle_t *handle, fx1_csi_sim_counters_t *counters)
{
    *counters = csi_sim_const(handle)->counters;
}

uint32_t fx1_csi_sim_get_record_id(const neon_csi_handle_t *handle)
{
    return csi_sim_const(handle)->record_id;
}

uint32_t fx1_csi_sim_get_event_mask(const neon_csi_handle_t *handle)
{
    return csi_sim_const(handle)->event_mask;
}
