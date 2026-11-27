#ifndef NEON_CAMERA_3A_H
#define NEON_CAMERA_3A_H

#include <stdint.h>
#include "isp/neon_isp.h"
#include "neon_camera_iq.h"

/* Future algorithm-only boundary: each packet is copied coherently under its
 * driver lease, then released before process_snapshot runs. Engine packets must
 * match ingress/context/profile; absence is explicit, never latest-value merge.
 * No binary adapter is implemented by this declaration. */
typedef struct {
    bool valid;
    neon_isp_frame_identity_t identity;
    const void *payload;
    size_t bytes;
} neon_camera_stats_packet_t;
typedef struct {
    neon_camera_stats_packet_t aec, awb, af;
    neon_camera_controls_t effective;
} neon_camera_stats_snapshot_t;
typedef struct {
    uint32_t abi;
    size_t workspace_bytes, workspace_alignment;
    neon_camera_result_t (*process_snapshot)(void *workspace,
        const neon_camera_stats_snapshot_t *, neon_camera_iq_input_t *proposal);
} neon_camera_3a_snapshot_ops_t;

/* Compatibility reference algorithm below uses the legacy statistics API. */
typedef struct {
    uint32_t target_frame_id, exposure_lines;
    uint16_t analogue_gain_q8, digital_gain_q8, wb_red_gain_q8, wb_blue_gain_q8, lens_position;
    uint8_t apply_exposure, apply_white_balance, apply_focus;
} neon_camera_3a_controls_t;
typedef struct {
    uint8_t target_luma, ae_speed_percent, awb_speed_percent;
    uint32_t min_exposure_lines, max_exposure_lines;
    uint16_t min_analogue_gain_q8, max_analogue_gain_q8;
    uint32_t control_latency_frames;
} neon_camera_3a_profile_t;
typedef struct { neon_camera_3a_profile_t profile; neon_camera_3a_controls_t current; uint8_t initialized; } neon_camera_3a_t;
typedef struct {
    int (*load_profile)(neon_camera_3a_t *, const void *, uint32_t);
    int (*process)(neon_camera_3a_t *, const neon_isp_statistics_t *, neon_camera_3a_controls_t *);
} neon_camera_3a_ops_t;
void neon_camera_3a_init(neon_camera_3a_t *algorithm, const neon_camera_3a_profile_t *profile);
void neon_camera_3a_profile_default(neon_camera_3a_profile_t *profile);
const neon_camera_3a_ops_t *neon_camera_3a_reference_ops(void);

#endif
