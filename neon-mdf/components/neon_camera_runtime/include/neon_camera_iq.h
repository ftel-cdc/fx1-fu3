#ifndef NEON_CAMERA_IQ_H
#define NEON_CAMERA_IQ_H
#include "neon_camera_frame.h"
/* Integration boundary only; no shipped IQ ABI/archive is claimed by this header.
 * Caller owns tuning/workspace/output storage for their whole use. No MMIO/tasks.
 * Output is a complete immutable profile, including table/resident requirements,
 * never a partial WB update. Backend owns packing/confirmation and bank locks. */
typedef struct {
    uint32_t valid; /* Lux/CCT validity belongs to the algorithm, not AE index. */
    int32_t lux_index, cct;
    neon_camera_controls_t controls;
} neon_camera_iq_input_t;
typedef struct {
    uint32_t abi;
    size_t workspace_bytes, workspace_alignment;
    neon_camera_result_t (*resolve)(void *workspace, const void *tuning, size_t tuning_bytes,
        const neon_camera_iq_input_t *, void *profile, size_t capacity, size_t *written);
} neon_camera_iq_ops_t;
#endif
