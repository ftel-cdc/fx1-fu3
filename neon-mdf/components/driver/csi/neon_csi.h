/**
 * @file neon_csi.h
 * @brief Public declarations for the CSI-2 receiver.
 * @details
 * CSI-2 receiver: D-PHY link bring-up, packet decoding, DMA of the frames to memory, frame context,
 * metadata, test pattern and event reporting. The receiver owns its link: it is bound to one PHY
 * (phy/neon_phy.h) and drives it, together with the reset, lane enables and Stop State of its own
 * registers, from neon_csi_configure(). config.link describes the lanes.
 *
 * This header is common to every chip. What a chip provides is reported by neon_csi_get_caps(); settings
 * that only one chip has are in csi/platform/<family>/neon_csi_<family>.h.
 * @note Blocking calls are intended for task context. neon_csi_get_events(), neon_csi_take_sub_events()
 * and neon_csi_get_sub_events() are the only calls meant for an interrupt handler.
 */

#ifndef __NEON_CSI_H__
#define __NEON_CSI_H__

/*==================================================================================================
*                                          INCLUDE FILES                                          
==================================================================================================*/
#include <stdbool.h>
#include <stdint.h>
#include "neon_types.h"
#include "phy/neon_phy.h"
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
#define NEON_CSI_MAX_BUFFERS 4U       /* DMA buffers that rotate between hardware and software. */
#define NEON_CSI_MAX_VCS 4U           /* Virtual channels that carry frame context. */

/*
 * Events and sub events. neon_csi_get_events() tells which events have sub events set; each event then
 * has its own 32 sub event bits, read with neon_csi_take_sub_events(). NEON_CSI_SUB_EVENT_<EVENT>_<NAME>
 * belongs to NEON_CSI_EVENT_<EVENT>. Sub events of NEON_CSI_EVENT_PLATFORM are in
 * csi/platform/<family>/neon_csi_<family>.h. A chip raises only those in caps.supported_sub_events.
 * Every event has at least one sub event: what happens is always a sub event, the event only groups them.
 * An event with a single condition gets one sub event at bit 0.
 */
/* Event bit in the result of neon_csi_get_events(). */
#define NEON_CSI_EVENT_BIT(event)                   (1UL << (uint32_t)(event))

/* Sub events of NEON_CSI_EVENT_BUFFER. */
#define NEON_CSI_SUB_EVENT_BUFFER_VALID             (1UL << 0)                      /* A buffer completed correctly; software owns it. */
#define NEON_CSI_SUB_EVENT_BUFFER_ERROR             (1UL << 1)                      /* A buffer completed with a fault; software owns it. */
#define NEON_CSI_SUB_EVENT_BUFFER_DROPPED           (1UL << 2)                      /* A frame was lost, e.g. no buffer was free. */

/* Sub events of NEON_CSI_EVENT_FRAME. vc is 0 to 7. */
#define NEON_CSI_SUB_EVENT_FRAME_START              (1UL << 0)                      /* A Frame Start was received. */
#define NEON_CSI_SUB_EVENT_FRAME_ERROR_VC(vc)       (1UL << (8U + (uint32_t)(vc)))  /* A frame of channel vc had a packet error. */
#define NEON_CSI_SUB_EVENT_FRAME_SYNC_ERROR_VC(vc)  (1UL << (16U + (uint32_t)(vc))) /* Frame Start and Frame End of channel vc did not pair up. */

/* Sub events of NEON_CSI_EVENT_PACKET. */
#define NEON_CSI_SUB_EVENT_PACKET_ECC_CORRECTED     (1UL << 0)                      /* A packet header had a single-bit error, corrected. */
#define NEON_CSI_SUB_EVENT_PACKET_ECC_UNCORRECTABLE (1UL << 1)                      /* A packet header had an uncorrectable error. */
#define NEON_CSI_SUB_EVENT_PACKET_CRC_ERROR         (1UL << 2)                      /* A long packet payload failed its CRC. */
#define NEON_CSI_SUB_EVENT_PACKET_UNSUPPORTED_DT    (1UL << 3)                      /* A long packet carried a Data Type the receiver does not take. */
#define NEON_CSI_SUB_EVENT_PACKET_WC_CORRUPTION     (1UL << 4)                      /* A long packet ended before its Word Count. */
#define NEON_CSI_SUB_EVENT_PACKET_SHORT             (1UL << 5)                      /* A generic short packet waits for neon_csi_read_short_packet(). */

/* Sub events of NEON_CSI_EVENT_PHY. */
#define NEON_CSI_SUB_EVENT_PHY_SOT_ERROR            (1UL << 0)                      /* Start-of-Transmission error on a data lane. */
#define NEON_CSI_SUB_EVENT_PHY_SOT_SYNC_ERROR       (1UL << 1)                      /* Start-of-Transmission sync could not be found. */

/* Sub events of NEON_CSI_EVENT_SYSTEM. Bit 0 is free: it was the link reset of the former separate link driver. */
#define NEON_CSI_SUB_EVENT_SYSTEM_OVERFLOW          (1UL << 1)                      /* An internal buffer or FIFO overflowed; data was lost. */
#define NEON_CSI_SUB_EVENT_SYSTEM_BUS_ERROR         (1UL << 2)                      /* A DMA write to memory was refused by the bus. */

/*==================================================================================================
*                                  STRUCTURES AND OTHER TYPEDEFS                                  
==================================================================================================*/
/** Events. Each one has its own set of sub events, NEON_CSI_SUB_EVENT_<EVENT>_*. */
typedef enum {
    NEON_CSI_EVENT_BUFFER = 0,    /* Buffers completed or lost. */
    NEON_CSI_EVENT_FRAME = 1,     /* Frame Start and frame errors, per virtual channel. */
    NEON_CSI_EVENT_PACKET = 2,    /* Packet header and payload errors, generic short packets. */
    NEON_CSI_EVENT_PHY = 3,       /* D-PHY errors. */
    NEON_CSI_EVENT_SYSTEM = 4,    /* Overflow, bus error. */
    NEON_CSI_EVENT_PLATFORM = 5,  /* Chip-specific; sub events in csi/platform/<family>/neon_csi_<family>.h. */
    NEON_CSI_EVENT_COUNT = 6      /* Number of events. */
} neon_csi_event_t;

/** RAW pixel format sent by the sensor, valued by its bit depth. Which ones a chip accepts is caps.supported_raw_formats. */
typedef enum {
    NEON_CSI_RAW6 = 6,   /* Data Type 0x28. */
    NEON_CSI_RAW7 = 7,   /* Data Type 0x29. */
    NEON_CSI_RAW8 = 8,   /* Data Type 0x2A. */
    NEON_CSI_RAW10 = 10, /* Data Type 0x2B. */
    NEON_CSI_RAW12 = 12, /* Data Type 0x2C. */
    NEON_CSI_RAW14 = 14, /* Data Type 0x2D. */
    NEON_CSI_RAW16 = 16, /* Data Type 0x2E, CSI-2 v2.0. */
    NEON_CSI_RAW20 = 20, /* Data Type 0x2F, CSI-2 v2.0. */
    NEON_CSI_RAW24 = 24  /* Data Type 0x27, CSI-2 v2.0. */
} neon_csi_raw_format_t;

/** Bit of a neon_csi_raw_format_t in caps.supported_raw_formats. */
#define NEON_CSI_RAW_FORMAT_BIT(format) (1UL << (uint32_t)(format))

/** How the receiver writes the pixels to memory, whatever the sensor sent (neon_csi_raw_format_t). */
typedef enum {
    NEON_CSI_PACKING_NATIVE = 0,     /* CSI-2 packing as received, e.g. RAW10: 4 pixels in 5 bytes. */
    NEON_CSI_PACKING_UNPACKED16 = 1  /* One 16-bit little-endian container per pixel. */
} neon_csi_pixel_packing_t;

/** Position of the pixel inside its 16-bit container. */
typedef enum {
    NEON_CSI_ALIGN_NONE = 0,  /* NEON_CSI_PACKING_NATIVE: no container. */
    NEON_CSI_ALIGN_RIGHT = 1, /* Pixel in the least-significant bits, zero above. */
    NEON_CSI_ALIGN_LEFT = 2   /* Pixel in the most-significant bits, zero below. */
} neon_csi_sample_align_t;

typedef enum {
    NEON_CSI_BUFFER_HW_OWNED = 0, /* The DMA may write this side. */
    NEON_CSI_BUFFER_SW_VALID = 1, /* Complete and correct; software owns it. */
    NEON_CSI_BUFFER_SW_ERROR = 2  /* Incomplete because of a fault; software owns it. */
} neon_csi_buffer_state_t;

typedef enum {
    NEON_CSI_STATE_RESET = 0,       /* Not initialized, or deinitialized: bound to no PHY. */
    NEON_CSI_STATE_IDLE = 1,        /* Bound to a PHY, not configured. */
    NEON_CSI_STATE_CONFIGURED = 2,  /* Configuration programmed and link up (Stop State seen), not receiving. */
    NEON_CSI_STATE_RUNNING = 3      /* Receiving. */
} neon_csi_state_t;

/** Generic short packet (Data Types 0x08 to 0x0F). */
typedef struct {
    uint16_t data;                   /* 16-bit data field of the packet. */
    uint8_t data_type;               /* 6-bit CSI-2 Data Type, 0x08 to 0x0F. */
    uint8_t virtual_channel;         /* Virtual channel the packet was sent on. */
} neon_csi_short_packet_t;

/** Header of the last image long packet on one virtual channel. */
typedef struct {
    uint16_t line_count;             /* Image long packets received in the current or last frame. */
    uint16_t byte_count;             /* Word Count of the last one. */
    uint8_t data_type;               /* 6-bit CSI-2 Data Type of the last one. */
    bool error;                      /* This channel's part of the frame had an error; false when the chip cannot tell. */
} neon_csi_vc_info_t;

/** What the chip provides, fixed when the hardware is built. */
typedef struct {
    uint8_t buffer_count;            /* DMA buffers, at most NEON_CSI_MAX_BUFFERS. */
    uint8_t vc_count;                /* Virtual channels with frame context, at most NEON_CSI_MAX_VCS. */
    bool has_test_pattern;           /* A test pattern generator can replace the sensor. */
    bool has_metadata;               /* Embedded data can be captured with each frame. */
    uint32_t dma_align_bytes;        /* Buffer addresses, buffer sizes and the line pitch are multiples of this; 0 means any. */
    uint32_t supported_raw_formats;  /* NEON_CSI_RAW_FORMAT_BIT() of each accepted neon_csi_raw_format_t. */
    uint32_t supported_sub_events[NEON_CSI_EVENT_COUNT]; /* Per event: the sub event bits the chip can raise. */
} neon_csi_caps_t;

/** Memory the receiver writes the frames to. A platform may extend it, with this struct as first member. */
typedef struct {
    uint32_t base_addr[NEON_CSI_MAX_BUFFERS];  /* Start of each buffer in memory. */
    uint32_t buffer_size_bytes;                /* Room for the pixels of one virtual channel in each buffer. */
    uint32_t line_pitch_bytes;                 /* Distance between two lines; 0 packs them back to back. */
} neon_csi_dma_config_t;

/** Test pattern generator, in place of the sensor. Its pixel format is config.raw_format. */
typedef struct {
    bool enable;                             /* Take the frames from the generator instead of the sensor. */
    uint8_t pattern;                         /* Chip-specific pattern number. */
    uint16_t width;                          /* Pixels per line of the generated frame; a chip may round it (see its platform header). */
    uint16_t height;                         /* Lines of the generated frame. */
} neon_csi_tpg_config_t;

/** Capture of the embedded data lines, next to the pixels of each virtual channel. */
typedef struct {
    bool enable;                             /* Capture the embedded data lines into the metadata area. */
    uint8_t data_type;                       /* 6-bit CSI-2 Data Type of the embedded lines, e.g. 0x12. */
    uint32_t area_size_bytes;                /* Room for the metadata of one virtual channel in each buffer. */
} neon_csi_metadata_config_t;

/** Everything neon_csi_configure() programs. */
typedef struct {
    neon_phy_mipi_dphy_opts_t link;          /* D-PHY lanes, line rate and board wiring; not used while the test pattern is enabled. */
    neon_csi_raw_format_t raw_format;        /* Format the sensor sends. */
    uint16_t width;                          /* Active pixels per line expected from the sensor. */
    uint16_t height;                         /* Lines per frame expected from the sensor. */
    neon_csi_pixel_packing_t pixel_packing;  /* How the pixels are written to memory. */
    neon_csi_dma_config_t dma;               /* Where the frames are written. */
    neon_csi_metadata_config_t metadata;     /* Ignored when caps.has_metadata is false. */
    neon_csi_tpg_config_t test_pattern;      /* Ignored when caps.has_test_pattern is false. */
    uint32_t sub_event_mask[NEON_CSI_EVENT_COUNT]; /* Per event: the sub events that may assert the interrupt line. */
} neon_csi_config_t;

/** How one frame lies in a buffer, from neon_csi_calculate_layout(). */
typedef struct {
    uint16_t width;                  /* Pixels per line that reach memory: the test pattern's when it is enabled. */
    uint16_t height;                 /* Lines per frame that reach memory. */
    uint32_t bytes_per_line;         /* Bytes of pixels in one line, from width, raw_format and pixel_packing. */
    uint32_t line_pitch_bytes;       /* Distance between two lines; at least bytes_per_line. */
    uint32_t frame_bytes;            /* line_pitch_bytes * height; must fit in dma.buffer_size_bytes. */
    uint32_t buffer_bytes;           /* Memory to allocate at each dma.base_addr[n]: pixels and metadata of every virtual channel. */
    neon_csi_sample_align_t sample_align; /* Where each pixel sits in its container; chosen by the chip. */
} neon_csi_layout_t;

/** One completed buffer, from neon_csi_acquire_frame(). */
typedef struct {
    uint8_t buffer_index;                    /* Buffer to pass back to neon_csi_release_frame(). */
    neon_csi_buffer_state_t state;           /* NEON_CSI_BUFFER_SW_VALID, or SW_ERROR if the frame is incomplete. */
    uint32_t base_addr;                      /* Start of the buffer in memory. */
    uint32_t pixel_addr[NEON_CSI_MAX_VCS];   /* Start of the pixels of virtual channel n; valid when vc_mask bit n is set. */
    uint32_t meta_addr[NEON_CSI_MAX_VCS];    /* Start of its metadata area; 0 when metadata is disabled. */
    uint32_t frame_id;                       /* Capture number; wraps, never 0. */
    uint64_t timestamp_ns;                   /* Time of the capture's Frame Start. */
    uint32_t last_valid_line;                /* Last line fully written, UINT32_MAX if none; meaningful only when state is SW_ERROR. */
    uint8_t vc_mask;                         /* Bit n set: virtual channel n took part, vc[n] is valid. */
    neon_csi_vc_info_t vc[NEON_CSI_MAX_VCS]; /* Long packet summary of each virtual channel. */
} neon_csi_frame_t;

/** Snapshot of the receiver, from neon_csi_get_status(). */
typedef struct {
    neon_csi_state_t state;                                     /* Driver state. */
    neon_csi_buffer_state_t buffer_state[NEON_CSI_MAX_BUFFERS]; /* Owner of each buffer; caps.buffer_count are valid. */
    uint32_t packet_count;                                      /* Long packets received. */
    uint32_t frame_count;                                       /* Frames received. */
    uint32_t ecc_error_count;                                   /* Packet headers with an ECC error. */
    uint32_t crc_error_count;                                   /* Long packets with a CRC error. */
    uint32_t frame_drop_count;                                  /* Frames lost because no buffer was free. */
    bool clock_stop_state;                                      /* The clock lane is in Stop State. */
    bool clock_hs_active;                                       /* The clock lane is in High-Speed mode: the byte clock runs. */
    uint8_t lane_stop_mask;                                     /* Bit n set: data lane n is in Stop State. */
    neon_phy_mipi_dphy_status_t phy_errors;                     /* D-PHY errors since the previous get_status, read and cleared. */
} neon_csi_status_t;

/** Receiver instance, caller allocated. Fields are private to the driver. */
typedef struct neon_csi_handle_s {
    neon_phy_t *phy;                 /* D-PHY of the link, initialized by neon_csi_init(). */
    neon_csi_caps_t caps;            /* Copy of neon_csi_get_caps(), taken by neon_csi_init(). */
    neon_csi_state_t state;          /* Driver state. */
    neon_csi_config_t config;        /* Last configuration accepted by neon_csi_configure(). */
    bool link_up;                    /* The PHY is powered and the lanes are enabled, from configure() to deinit(). */
    uint8_t acquired_mask;           /* Bit n set: software holds buffer n. */
} neon_csi_handle_t;

/*==================================================================================================
*                                       FUNCTION PROTOTYPES                                       
==================================================================================================*/
/**
 * @brief Bind a receiver to its D-PHY and initialize the PHY (neon_phy_init()).
 * @param[out] handle Receiver handle, caller allocated.
 * @param[in,out] phy PHY handle from neon_phy_get(): a MIPI D-PHY with a receive side. It is set to
 * NEON_PHY_MIPI_DPHY_SUBMODE_RX and stays in use until neon_csi_deinit().
 * @return NEON_OK on success, NEON_INVALID_PARAM for a NULL pointer, NEON_NOT_SUPPORTED if the hardware is
 * not a receiver this driver supports, or a status of neon_phy_init() or neon_phy_set_mode().
 */
neon_status_t neon_csi_init(neon_csi_handle_t *handle, neon_phy_t *phy);

/**
 * @brief Report what the chip provides. Needs no handle and no hardware.
 * @param[out] caps Capabilities.
 * @return NEON_OK, or NEON_INVALID_PARAM for a NULL pointer.
 */
neon_status_t neon_csi_get_caps(neon_csi_caps_t *caps);

/**
 * @brief Disable the lanes, power the PHY off, release it (neon_phy_exit()) and the binding.
 * @param[in] handle Receiver handle.
 * @return NEON_OK, or NEON_BUSY while the receiver is running.
 */
neon_status_t neon_csi_deinit(neon_csi_handle_t *handle);

/**
 * @brief Fill a configuration with defaults: RAW12 in NEON_CSI_PACKING_UNPACKED16, test pattern off, the
 * frame and error events enabled, and a link with a continuous clock and a straight lane map. The buffers,
 * the geometry, link.lane_count and link.lane_rate_bps are left at zero for the caller.
 * @param[out] config Configuration.
 */
void neon_csi_get_default_config(neon_csi_config_t *config);

/**
 * @brief Compute the buffer layout that a configuration produces. Needs no hardware. With
 * dma.buffer_size_bytes or dma.line_pitch_bytes left at 0, the smallest valid value is used.
 * @param[in] config Configuration.
 * @param[out] layout Layout the DMA writes and the consumer reads.
 * @return NEON_OK, NEON_INVALID_PARAM for an invalid geometry or format, or NEON_NOT_SUPPORTED.
 */
neon_status_t neon_csi_calculate_layout(const neon_csi_config_t *config, neon_csi_layout_t *layout);

/**
 * @brief Validate and program a configuration, and bring the link up. The core stays disabled.
 * @details The link is reset and reprogrammed every time: lanes disabled, D-PHY reset, PHY configured
 * from config.link, lanes enabled, then a wait for Stop State on the clock lane and every used data lane.
 * The sensor must already drive LP-11, for at least T_INIT (100 us). With the test pattern enabled only
 * the D-PHY reset is done.
 * @param[in,out] handle Receiver handle, not running.
 * @param[in] config Configuration.
 * @return NEON_OK on success, NEON_BUSY while running, NEON_ERROR if not initialized, NEON_INVALID_PARAM
 * for an invalid configuration, NEON_NOT_SUPPORTED for a lane count or line rate the hardware cannot take
 * (including more lanes than the receiver was built with),
 * NEON_TIMEOUT if Stop State was not seen (the receiver stays IDLE; call again once the sensor drives
 * LP-11), or a platform error status.
 */
neon_status_t neon_csi_configure(neon_csi_handle_t *handle, const neon_csi_config_t *config);

/**
 * @brief Arm the buffers, enable the DMA and then the receiver. Start the sensor after it.
 * @param[in,out] handle Receiver handle, configured.
 * @return NEON_OK on success, NEON_BUSY if running, NEON_ERROR if not configured.
 */
neon_status_t neon_csi_start(neon_csi_handle_t *handle);

/**
 * @brief Stop at once: abort the DMA, wait only for the AXI writes already issued, and disable the core.
 * No Frame End is waited for: a frame in progress is dropped. Stop the sensor first. Buffers held by
 * software become invalid. The link stays up, so start() may follow without configure(); the first
 * capture after that start may then report one frame sync error (see neon_phy_fx1.h, clock after the
 * last burst).
 * @param[in,out] handle Receiver handle, running.
 * @return NEON_OK on success, NEON_ERROR if the receiver is not running, or a platform error status
 * (the receiver then stays running and stop may be called again).
 */
neon_status_t neon_csi_stop(neon_csi_handle_t *handle);

/**
 * @brief Reset the datapath and keep the register configuration. Frames held by software are lost: both
 * buffers become hardware owned again.
 * The deeper reset is neon_csi_stop() then neon_csi_configure(), which resets the D-PHY and brings the
 * link up again.
 * @param[in,out] handle Receiver handle, running.
 * @return NEON_OK, NEON_ERROR if not running or when the clock lane is not in High-Speed mode (the
 * datapath reset needs the byte clock: escalate to stop() and configure()), or a platform error status.
 */
neon_status_t neon_csi_recover(neon_csi_handle_t *handle);

/**
 * @brief Read the state, the buffer ownership, the counters and the link, and take the D-PHY errors.
 * @param[in,out] handle Receiver handle.
 * @param[out] status Snapshot.
 * @return NEON_OK on success, or a platform error status.
 */
neon_status_t neon_csi_get_status(neon_csi_handle_t *handle, neon_csi_status_t *status);

/**
 * @brief Tell which events have sub events set, masked ones included.
 * @param[in,out] handle Receiver handle.
 * @param[out] events NEON_CSI_EVENT_BIT() of each such event.
 * @return NEON_OK on success, NEON_INVALID_PARAM for a NULL pointer, or a platform error status.
 */
neon_status_t neon_csi_get_events(neon_csi_handle_t *handle, uint32_t *events);

/**
 * @brief Read the sticky sub events of one event and clear exactly those read. A sub event that arrives
 * after the read stays set for the next call. Masked sub events are included.
 * @param[in,out] handle Receiver handle.
 * @param[in] event Event.
 * @param[out] sub_events NEON_CSI_SUB_EVENT_<EVENT>_* bits that were set.
 * @return NEON_OK on success, NEON_INVALID_PARAM for a NULL pointer or an unknown event, or a platform
 * error status.
 */
neon_status_t neon_csi_take_sub_events(neon_csi_handle_t *handle, neon_csi_event_t event, uint32_t *sub_events);

/**
 * @brief Read the sticky sub events of one event without clearing them. Masked sub events are included.
 * @param[in,out] handle Receiver handle.
 * @param[in] event Event.
 * @param[out] sub_events NEON_CSI_SUB_EVENT_<EVENT>_* bits.
 * @return NEON_OK on success, NEON_INVALID_PARAM for a NULL pointer or an unknown event, or a platform
 * error status.
 */
neon_status_t neon_csi_get_sub_events(neon_csi_handle_t *handle, neon_csi_event_t event, uint32_t *sub_events);

/**
 * @brief Change the sub events of one event that may assert the interrupt line.
 * @param[in,out] handle Receiver handle.
 * @param[in] event Event.
 * @param[in] mask NEON_CSI_SUB_EVENT_<EVENT>_* bits; bits not in caps.supported_sub_events are ignored.
 * @return NEON_OK on success, NEON_INVALID_PARAM for an unknown event, or a platform error status.
 */
neon_status_t neon_csi_set_sub_event_mask(neon_csi_handle_t *handle, neon_csi_event_t event, uint32_t mask);

/**
 * @brief Take the oldest completed buffer that software does not yet hold.
 * @details When several buffers are complete, the one with the oldest frame_id is returned.
 * @param[in,out] handle Receiver handle, running.
 * @param[out] frame Buffer and frame context. state tells whether the data is usable: a
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
