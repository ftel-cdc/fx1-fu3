/**
 * @file neon_csi.h
 * @brief Public declarations for the CSI-2 receiver.
 * @details
 * CSI-2 receiver: packet and pixel datapath, ping-pong DMA to DDR, frame context, metadata, test pattern
 * and event reporting. The D-PHY and the lanes belong to neon_mipi.h; a receiver is bound to one link and
 * takes a lock on it while it runs.
 *
 * This header holds the contract only. Every number that belongs to a chip (buffer count, burst and
 * outstanding limits, alignment, frame id width, which events exist) is reported by neon_csi_get_caps() and
 * enforced from there, so that another chip changes its backend and not this file. The first chip is the
 * FX1 FTEL MIPI CSI-2 controller (MAS V1.1).
 *
 * Only the DMA output path is exposed. The FX1 ISP MAS fixes memory-to-memory as its default topology
 * and its AXI4-Stream input is not compatible with the 64-bit stream of this core.
 * @note Blocking calls are intended for task context. neon_csi_get_events() and
 * neon_csi_clear_events() are the only calls meant for an interrupt handler.
 */

#ifndef __NEON_CSI_H__
#define __NEON_CSI_H__

/*==================================================================================================
*                                          INCLUDE FILES                                          
==================================================================================================*/
#include <stdbool.h>
#include <stdint.h>
#include "neon_types.h"
#include "mipi/neon_mipi.h"
#include "csi/neon_csi_events.h"
#ifdef __cplusplus
extern "C" {
#endif

/*==================================================================================================
*                                        DEFINES AND MACROS                                       
==================================================================================================*/
/*
 * Capacity of the structures below: the most that any supported chip needs. What a chip really has is in
 * neon_csi_caps_t and is never larger. A chip that needs more raises these.
 */
#define NEON_CSI_MAX_BUFFERS 2U       /* DMA buffers that rotate between hardware and software. */
#define NEON_CSI_MAX_VCS 2U           /* Virtual channels that carry frame context. */
#define NEON_CSI_MAX_EXTRACT_SLOTS 2U /* Embedded-data extraction slots per virtual channel. */
#define NEON_CSI_BACKEND_WORDS 48U    /* Words of backend-private storage inside the handle. */

/*==================================================================================================
*                                  STRUCTURES AND OTHER TYPEDEFS                                  
==================================================================================================*/
typedef enum {
    NEON_CSI_RAW10 = 10,
    NEON_CSI_RAW12 = 12
} neon_csi_raw_format_t;

typedef enum {
    NEON_CSI_OUTPUT_PACKED = 0, /* Native CSI-2 packing, re-packed to line boundaries. DMA_CONTROL.dma_format = bypass. */
    NEON_CSI_OUTPUT_RAW16 = 1   /* One 16-bit little-endian halfword per pixel. */
} neon_csi_output_format_t;

typedef enum {
    NEON_CSI_ALIGN_NONE = 0,  /* Packed output: no sample container. */
    NEON_CSI_ALIGN_RIGHT = 1, /* Pixel in the least-significant bits of the halfword. What the FX1 produces. */
    NEON_CSI_ALIGN_LEFT = 2   /* {pixel, zero fill} in bits 11:0: the ISP input contract. Not produced yet. */
} neon_csi_sample_align_t;

typedef enum {
    NEON_CSI_ERROR_POLICY_RELEASE_WITH_ERROR = 0, /* A faulted buffer is handed to software marked as error. */
    NEON_CSI_ERROR_POLICY_DISCARD_AND_REWIND = 1  /* A faulted buffer is rewound and written again. */
} neon_csi_error_policy_t;

typedef enum {
    NEON_CSI_BUFFER_HW_OWNED = 0, /* The DMA may write this side. */
    NEON_CSI_BUFFER_SW_VALID = 1, /* Complete and correct; software owns it. */
    NEON_CSI_BUFFER_SW_ERROR = 2  /* Incomplete because of a fault; software owns it. */
} neon_csi_buffer_state_t;

typedef enum {
    NEON_CSI_STATE_IDLE = 0,        /* Bound to a link, not configured. */
    NEON_CSI_STATE_CONFIGURED = 1,
    NEON_CSI_STATE_RUNNING = 2,
    NEON_CSI_STATE_CONFIG_LOST = 3  /* The link was fully reset; configure again. */
} neon_csi_state_t;

typedef enum {
    NEON_CSI_RECOVER_SOFT = 0 /* Datapath reset, register configuration kept (CORE_CONFIG.soft_reset). */
} neon_csi_recover_level_t;

/** What one chip provides. The core validates and schedules from these values only. */
typedef struct {
    uint8_t buffer_count;             /* DMA buffers, at most NEON_CSI_MAX_BUFFERS. */
    uint8_t vc_count;                 /* Virtual channels with frame context, at most NEON_CSI_MAX_VCS. */
    uint8_t extract_slots;            /* Extraction slots per virtual channel, at most NEON_CSI_MAX_EXTRACT_SLOTS. */
    uint8_t frame_id_bits;            /* Width of frame_id; it wraps modulo 2^frame_id_bits. */
    uint8_t max_outstanding;          /* Outstanding write transactions. */
    uint8_t max_qos;
    uint8_t max_prot;
    uint16_t max_burst_beats;
    uint16_t max_line_padding_bytes;
    uint16_t min_tpg_dimension;       /* Smallest test pattern width and height. */
    uint8_t tpg_width_granularity;    /* The generator rounds the width down to a multiple of this. */
    bool test_pattern;                /* A test pattern generator exists. */
    uint32_t max_bytes_per_line;
    /*
     * Base address and line pitch of every buffer are multiples of this, so that a consumer can read a
     * buffer in place. A policy of the SoC integration (FX1: the ISP input DMA, ISP MAS table 154), not a
     * limit of the receiver itself.
     */
    uint32_t dma_align_bytes;
    uint32_t meta_record_min_bytes;   /* Smallest metadata area; 0 when there is no metadata record. */
    uint32_t supported_events;        /* NEON_CSI_EVENT_* bits that the chip can raise. */
} neon_csi_caps_t;

typedef struct {
    uint8_t length_bytes;  /* 0 leaves the slot unused, otherwise 1 to 4. */
    uint16_t byte_offset;  /* Offset inside the embedded line. */
    uint8_t line_index;    /* Embedded line the field is read from. */
} neon_csi_extract_t;

typedef struct {
    bool enable;                 /* Capture embedded data into a per-capture metadata record. */
    uint8_t data_type;           /* 6-bit CSI-2 Data Type of the embedded line. */
    bool frame_number_check;
    bool extract_enable;
    neon_csi_extract_t extract[NEON_CSI_MAX_EXTRACT_SLOTS];
    uint32_t record_id;          /* metadata_record_id; may be changed while running. */
    uint32_t base_addr[NEON_CSI_MAX_BUFFERS]; /* Provisional: not yet in the register map. */
    uint32_t area_size_bytes;    /* Provisional: at least caps.meta_record_min_bytes plus the largest embedded payload. */
} neon_csi_metadata_config_t;

typedef struct {
    bool enable;                 /* Replace the sensor path with the test pattern generator. */
    uint8_t pattern;
    neon_csi_raw_format_t pixel_format;
    uint16_t width;              /* The generator rounds it down to a multiple of caps.tpg_width_granularity. */
    uint16_t height;
    uint16_t hblank;
    uint16_t vblank;
} neon_csi_tpg_config_t;

typedef struct {
    uint32_t base_addr[NEON_CSI_MAX_BUFFERS]; /* Ping and pong RAW buffers. */
    uint32_t buffer_size_bytes;
    uint16_t line_padding_bytes;
    uint16_t max_burst_beats;    /* 1 to caps.max_burst_beats. */
    uint8_t max_outstanding;     /* 1 to caps.max_outstanding write transactions. */
    uint8_t qos;
    uint8_t prot;
    neon_csi_error_policy_t error_policy;
} neon_csi_dma_config_t;

typedef struct {
    neon_csi_raw_format_t raw_format;
    uint16_t width;              /* Active pixels per line expected from the sensor. */
    uint16_t height;
    neon_csi_output_format_t output_format;
    neon_csi_dma_config_t dma;
    neon_csi_metadata_config_t metadata;
    neon_csi_tpg_config_t test_pattern;
    uint32_t event_mask;         /* Events that may assert the interrupt line. */
} neon_csi_config_t;

typedef struct {
    uint16_t width;              /* Geometry that reaches DDR: the test pattern's when it is enabled. */
    uint16_t height;
    uint32_t bytes_per_line;
    uint32_t line_pitch_bytes;
    uint32_t frame_bytes;        /* line_pitch_bytes * height. */
    neon_csi_output_format_t output_format;
    neon_csi_raw_format_t raw_format;
    neon_csi_sample_align_t sample_align;
} neon_csi_layout_t;

typedef struct {
    bool present;                /* The virtual channel took part in the capture. */
    uint16_t byte_count;
    uint16_t line_count;
    uint8_t data_type;
    bool width_mismatch;
    bool height_mismatch;
    uint8_t err_summary;
    uint32_t extracted[NEON_CSI_MAX_EXTRACT_SLOTS];
} neon_csi_vc_info_t;

typedef struct {
    uint8_t buffer_index;
    neon_csi_buffer_state_t state;
    uint32_t base_addr;
    uint32_t frame_id;           /* caps.frame_id_bits wide, wraps. */
    uint64_t timestamp_ns;
    uint32_t record_id;
    uint32_t last_valid_line;    /* Meaningful only when state is NEON_CSI_BUFFER_SW_ERROR. */
    neon_csi_layout_t layout;
    neon_csi_vc_info_t vc[NEON_CSI_MAX_VCS];
    /*
     * False when this buffer's context is not known: the registers describe only the last completed
     * capture and a newer one has completed since. frame_id, timestamp_ns, record_id, last_valid_line and
     * vc are then zero. Enable the metadata record to get an exact context for every buffer.
     */
    bool context_valid;
} neon_csi_frame_t;

typedef struct {
    uint8_t virtual_channel;
    uint8_t data_type;
    uint16_t data;
} neon_csi_short_packet_t;

typedef struct {
    neon_csi_state_t state;
    neon_csi_buffer_state_t buffer_state[NEON_CSI_MAX_BUFFERS];
    uint32_t packet_count;
    uint32_t frame_count;
    uint32_t ecc_error_count;
    uint32_t crc_error_count;
    uint32_t frame_drop_count;
    uint8_t core_state;          /* Raw Control FSM state, diagnostic only. */
    bool irq_pending;            /* A set event is enabled in event_mask. */
} neon_csi_status_t;

typedef struct neon_csi_handle_s {
    neon_mipi_handle_t *mipi;
    neon_csi_caps_t caps;        /* Copied from the backend by neon_csi_init(). */
    neon_csi_state_t state;
    bool link_locked;
    uint8_t acquired_mask;       /* Buffers held by software through neon_csi_acquire_frame(). */
    uint8_t order_hint;          /* Side holding the older frame when both are complete and frame ids are unknown. */
    uint32_t config_epoch;
    uint32_t phy_epoch;
    uint32_t event_mask;
    uint32_t software_events;
    neon_csi_layout_t layout;
    uint32_t raw_base_addr[NEON_CSI_MAX_BUFFERS];
    uint32_t meta_base_addr[NEON_CSI_MAX_BUFFERS];
    uint32_t meta_area_size_bytes;
    bool meta_enabled;
    uint64_t backend_words[NEON_CSI_BACKEND_WORDS]; /* Private to the selected backend. */
} neon_csi_handle_t;

/*==================================================================================================
*                                       FUNCTION PROTOTYPES                                       
==================================================================================================*/
/**
 * @brief Bind a receiver to a link.
 * @param[out] handle Receiver handle, caller allocated.
 * @param[in] mipi Link handle, initialized.
 * @return NEON_OK on success, NEON_INVALID_PARAM for a NULL pointer.
 */
neon_status_t neon_csi_init(neon_csi_handle_t *handle, neon_mipi_handle_t *mipi);

/**
 * @brief Report what the chip provides. Needs no handle and no hardware.
 * @param[out] caps Capabilities.
 * @return NEON_OK, or NEON_INVALID_PARAM for a NULL pointer.
 */
neon_status_t neon_csi_get_caps(neon_csi_caps_t *caps);

/**
 * @brief Release the binding.
 * @param[in] handle Receiver handle.
 * @return NEON_OK, or NEON_BUSY while the receiver is running.
 */
neon_status_t neon_csi_deinit(neon_csi_handle_t *handle);

/**
 * @brief Fill a configuration with defaults for the ISP memory-to-memory path: RAW12, RAW16 output,
 * release-with-error policy, 16-beat bursts and the frame and error events enabled. The DDR buffers and
 * the geometry are left at zero for the caller.
 * @param[out] config Configuration.
 */
void neon_csi_get_default_config(neon_csi_config_t *config);

/**
 * @brief Compute the buffer layout that a configuration produces. Needs no hardware.
 * @param[in] config Configuration.
 * @param[out] layout Layout the DMA writes and the consumer reads.
 * @return NEON_OK, NEON_INVALID_PARAM for an invalid geometry or format, or NEON_NOT_SUPPORTED.
 */
neon_status_t neon_csi_calculate_layout(const neon_csi_config_t *config, neon_csi_layout_t *layout);

/**
 * @brief Validate and program a configuration. The core stays disabled.
 * @param[in,out] handle Receiver handle, not running.
 * @param[in] config Configuration.
 * @return NEON_OK on success, NEON_BUSY while running, NEON_ERROR if the link is released,
 * NEON_INVALID_PARAM for an invalid configuration, or a backend status.
 */
neon_status_t neon_csi_configure(neon_csi_handle_t *handle, const neon_csi_config_t *config);

/**
 * @brief Arm both buffers, enable the DMA and then the core (MAS 9.1 step 6). Start the sensor after it.
 * @param[in,out] handle Receiver handle, configured.
 * @return NEON_OK on success, NEON_BUSY if running, NEON_ERROR if not configured, the configuration was
 * lost or the link is not ready.
 */
neon_status_t neon_csi_start(neon_csi_handle_t *handle);

/**
 * @brief Abort the DMA, wait for outstanding writes, disable the core and unlock the link. Stop the
 * sensor first. Buffers held by software become invalid.
 * @param[in,out] handle Receiver handle, running.
 * @return NEON_OK on success, NEON_ERROR if the receiver is not running.
 */
neon_status_t neon_csi_stop(neon_csi_handle_t *handle);

/**
 * @brief Reset the datapath and keep the register configuration. Frames held by software are lost: both
 * buffers become hardware owned again.
 * @param[in,out] handle Receiver handle, running.
 * @param[in] level Recovery scope.
 * @return NEON_OK, or NEON_ERROR when the clock lane is not in High-Speed mode: soft_reset has no effect
 * there, so escalate to neon_mipi_recover() with NEON_MIPI_RECOVER_FULL and configure again.
 */
neon_status_t neon_csi_recover(neon_csi_handle_t *handle, neon_csi_recover_level_t level);

/**
 * @brief Read the state, the buffer ownership and the counters.
 * @param[in,out] handle Receiver handle.
 * @param[out] status Snapshot.
 * @return NEON_OK on success, or a backend status.
 */
neon_status_t neon_csi_get_status(neon_csi_handle_t *handle, neon_csi_status_t *status);

/**
 * @brief Read the sticky event flags, including events that are masked.
 * @param[in,out] handle Receiver handle.
 * @param[out] events NEON_CSI_EVENT_* bits.
 * @return NEON_OK on success, or a backend status.
 */
neon_status_t neon_csi_get_events(neon_csi_handle_t *handle, uint32_t *events);

/**
 * @brief Clear exactly the events that were serviced. Never clear more: an event that arrived while
 * the handler ran would be lost.
 * @param[in,out] handle Receiver handle.
 * @param[in] events Bits to clear.
 * @return NEON_OK on success, or a backend status.
 */
neon_status_t neon_csi_clear_events(neon_csi_handle_t *handle, uint32_t events);

/**
 * @brief Change the events that may assert the interrupt line.
 * @param[in,out] handle Receiver handle.
 * @param[in] event_mask NEON_CSI_EVENT_* bits.
 * @return NEON_OK on success, or a backend status.
 */
neon_status_t neon_csi_set_event_mask(neon_csi_handle_t *handle, uint32_t event_mask);

/**
 * @brief Take the oldest completed buffer that software does not yet hold.
 * @details When both buffers are complete the older one is picked by modular frame_id, which needs
 * the metadata record. Without it the per-buffer frame_id is unknown and the HAL assumes that the DMA
 * alternates between the two sides starting from side 0; that holds while software releases buffers in
 * the order it took them. To be confirmed on the VP.
 * @param[in,out] handle Receiver handle, running.
 * @param[out] frame Buffer, layout and frame context. state tells whether the data is usable: a
 * completion event alone is not proof.
 * @return NEON_OK, NEON_BUSY if no buffer has completed, NEON_ERROR if not running.
 */
neon_status_t neon_csi_acquire_frame(neon_csi_handle_t *handle, neon_csi_frame_t *frame);

/**
 * @brief Return a buffer to the DMA.
 * @param[in,out] handle Receiver handle, running.
 * @param[in] buffer_index Buffer returned by neon_csi_acquire_frame().
 * @return NEON_OK, NEON_INVALID_PARAM if software does not hold that buffer, NEON_ERROR if not running.
 */
neon_status_t neon_csi_release_frame(neon_csi_handle_t *handle, uint8_t buffer_index);

/**
 * @brief Set the tag that the next captures carry in their metadata record.
 * @param[in,out] handle Receiver handle, configured.
 * @param[in] record_id Software tag.
 * @return NEON_OK on success, or a backend status.
 */
neon_status_t neon_csi_set_record_id(neon_csi_handle_t *handle, uint32_t record_id);

/**
 * @brief Pop one generic short packet (Data Types 0x08 to 0x0F).
 * @param[in,out] handle Receiver handle, running.
 * @param[out] packet Packet.
 * @return NEON_OK, NEON_BUSY if the FIFO is empty, NEON_ERROR if not running.
 */
neon_status_t neon_csi_read_short_packet(neon_csi_handle_t *handle, neon_csi_short_packet_t *packet);

#ifdef __cplusplus
}
#endif

#endif /* __NEON_CSI_H__ */
