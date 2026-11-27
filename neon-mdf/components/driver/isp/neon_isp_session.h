#ifndef NEON_ISP_SESSION_H
#define NEON_ISP_SESSION_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* Additive profile-mode contract. The legacy neon_isp_handle_t API cannot
 * express MAS 9.7-9.9 ownership and must not be used as an implicit adapter.
 * This header is the contract. The optional platform/fx1/fx1_isp_backend
 * implements a restricted scalar-profile subset, independently of the legacy API. */
#define NEON_ISP_SESSION_ABI 1u
#define NEON_ISP_SESSION_SLOTS 4u
#define NEON_ISP_CAP_PROFILE (1u << 0)
#define NEON_ISP_CAP_OUTPUT_IDENTITY (1u << 1)
#define NEON_ISP_CAP_SAFE_DRAIN (1u << 2)
#define NEON_ISP_SESSION_REQUIRED_CAPS (NEON_ISP_CAP_PROFILE | NEON_ISP_CAP_OUTPUT_IDENTITY | NEON_ISP_CAP_SAFE_DRAIN)
typedef enum {
    NEON_CAMERA_OK, NEON_CAMERA_PENDING, NEON_CAMERA_NO_DATA,
    NEON_CAMERA_BUSY, NEON_CAMERA_INVALID, NEON_CAMERA_UNSUPPORTED,
    NEON_CAMERA_TIMEOUT, NEON_CAMERA_HW_ERROR, NEON_CAMERA_STALE
} neon_camera_result_t;

typedef struct { uint32_t address, bytes; } neon_isp_session_span_t;
typedef struct {
    neon_isp_session_span_t raw, y, uv;
} neon_isp_session_buffer_t;
typedef struct {
    uint32_t width, height, raw_stride, y_stride, uv_stride;
    uint32_t dma_alignment; /* Backend must also validate against its actual caps. */
    uint8_t raw_depth, bayer_pattern; /* Linear RAW12 initially; Bayer 0..3. */
    neon_isp_session_buffer_t buffers[NEON_ISP_SESSION_SLOTS];
} neon_isp_session_config_t;
typedef struct {
    const void *data; size_t bytes;
    uint32_t id;
} neon_isp_session_profile_t;
typedef enum { NEON_ISP_APPLY_NEXT, NEON_ISP_APPLY_EXACT } neon_isp_apply_mode_t;
typedef struct {
    neon_isp_session_profile_t profile;
    neon_isp_apply_mode_t mode;
    uint32_t target_frame;
} neon_isp_session_transaction_t;

/* Validity is explicit. Zero is a valid wrapped ingress ID/timestamp. */
#define NEON_ISP_ID_FRAME (1u << 0)
#define NEON_ISP_ID_CONTEXT (1u << 1)
#define NEON_ISP_ID_PROFILE (1u << 2)
#define NEON_ISP_ID_OUTPUT_TIME (1u << 3)
#define NEON_ISP_ID_CAPTURE_TIME (1u << 4)
typedef struct {
    uint32_t valid, ingress_frame, context_id, profile_id, tag;
    uint32_t output_sof, output_eof, capture_timestamp, timebase;
} neon_isp_frame_identity_t;

#define NEON_ISP_DIAG_COMMON (1u << 0)
#define NEON_ISP_DIAG_DMA (1u << 1)
#define NEON_ISP_DIAG_OUTPUT (1u << 2)
#define NEON_ISP_DIAG_TUNE (1u << 3)
#define NEON_ISP_DIAG_STATS (1u << 4)
#define NEON_ISP_DIAG_RECOVERY (1u << 5) /* Stable copied records, not live BUSY. */
#define NEON_ISP_DIAG_AUX (1u << 6)
#define NEON_ISP_DIAG_RECOVERY_LIVE (1u << 7)
#define NEON_ISP_DIAG_STREAM (1u << 8)
typedef struct {
    uint32_t valid;
    uint32_t common_status, common_irq;
    uint32_t dma_status, dma_error, dma_irq;
    uint32_t output_status[3], output_geometry[3], output_irq;
    uint32_t tune_status, tune_error, active_id, active_frame, next_frame;
    uint32_t stats_status[3], stats_drop[3];
    uint32_t lsc_error, ee_status, stream_error, stream_drops;
    uint32_t recovery_live_status, recovery_live_sequence;
    uint32_t recovery_status, recovery_sequence, recovery_count;
    struct { uint32_t ingress_frame, outcome; } recovery_records[4];
} neon_isp_hw_snapshot_t;

#define NEON_ISP_OBS_APPLIED (1u << 0)
#define NEON_ISP_OBS_OUTPUT (1u << 1)
#define NEON_ISP_OBS_DRAINED (1u << 2)
#define NEON_ISP_OBS_FAULT (1u << 3)
typedef struct {
    uint32_t flags, applied_id, applied_frame;
    uint8_t slot;
    neon_isp_frame_identity_t output;
    neon_isp_hw_snapshot_t hardware;
} neon_isp_session_observation_t;

/* All calls are nonblocking and serialized by a single runtime owner.
 * claim/release use a shared device claim (also used by ftel_isp).
 * begin: stopped setup + INIT/load/confirm/SEAL/baseline APPLY, arms output
 * before ingress. A fresh begin starts the four-slot rotation at slot zero.
 * OK/PENDING only mean accepted, APPLIED must be observed.
 * UNSUPPORTED from begin guarantees no HW mutation. Other errors can retain
 * ownership; caller must drain. Never fall back to legacy profile writes.
 * poll: authoritative status drain, not IRQ-count consumption. OUTPUT is only
 * successful atomic retirement (MAS 9.9.8), including real ingress/profile IDs.
 * A NO_DATA/PENDING observation may still contain an updated hardware snapshot.
 * Backend copies diagnostic/recovery records before W1C or another recovery;
 * handles stats leases/cache synchronization; never fabricates missing metadata.
 * DRAINED proves all AXI, contexts, builders, stats leases, pending APPLY and
 * recovery reconciled; it does not invalidate a consumer-held output.
 * release_output: relinquish exactly this retired slot; re-arm only if safe.
 * apply: accepted transaction is immutable; payload stays alive until observed
 * APPLIED or authoritative drain. Busy must leave existing transaction intact.
 * stop: block ingress, resolve pending APPLY and begin safe drain. Timeout never
 * constitutes drain. No reset escalation is inferred by this interface.
 * release: called only after DRAINED and all consumer/ready outputs returned
 * (or UNSUPPORTED begin). It synchronously masks/detaches any ISR context BEFORE
 * releasing the device claim. Returning from release ends hardware/ISR lifetime.
 */
typedef struct {
    uint32_t abi, capabilities;
    neon_camera_result_t (*claim)(void *device, const void *owner);
    void (*release)(void *device, const void *owner);
    neon_camera_result_t (*begin)(void *device, const neon_isp_session_config_t *, const neon_isp_session_profile_t *);
    neon_camera_result_t (*poll)(void *device, neon_isp_session_observation_t *);
    neon_camera_result_t (*submit)(void *device, uint8_t slot, uint32_t tag);
    neon_camera_result_t (*release_output)(void *device, uint8_t slot);
    neon_camera_result_t (*apply)(void *device, const neon_isp_session_transaction_t *);
    neon_camera_result_t (*stop)(void *device);
} neon_isp_session_backend_t;
#endif
