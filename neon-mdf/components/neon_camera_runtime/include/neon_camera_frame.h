#ifndef NEON_CAMERA_FRAME_H
#define NEON_CAMERA_FRAME_H
#include "isp/neon_isp_session.h"
#include "neon_camera_controls.h"
typedef struct {
    uint64_t generation, lease;
    uint8_t slot;
    neon_isp_frame_identity_t identity;
    neon_camera_controls_t effective, proposed;
    uint32_t requested_target, observed_applied_frame, recovery_sequence;
    bool requested_target_valid, observed_applied_valid, recovery_sequence_valid;
    bool replay;
} neon_camera_frame_t;
/* Arithmetic modulo 2^32. This span measures output processing, not capture latency. */
static inline uint32_t neon_camera_timestamp_span(uint32_t sof, uint32_t eof)
{ return eof - sof; }
#endif
