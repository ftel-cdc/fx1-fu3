#ifndef NEON_CAMERA_STATUS_H
#define NEON_CAMERA_STATUS_H
#include "isp/neon_isp_session.h"
/* Software lifecycle only. RUNNING does not imply COMMON_STATUS.BUSY. */
typedef enum {
    NEON_CAMERA_UNINITIALIZED, NEON_CAMERA_CONFIGURED, NEON_CAMERA_STARTING,
    NEON_CAMERA_RUNNING, NEON_CAMERA_STOPPING, NEON_CAMERA_DRAINING, NEON_CAMERA_FAULTED
} neon_camera_state_t;
typedef struct {
    neon_camera_state_t state;
    neon_camera_result_t last_error;
    uint64_t generation;
    uint32_t submitted, retired;
    uint8_t consumer_held, ready;
    bool claimed, drained, profile_pending;
    neon_isp_hw_snapshot_t hardware;
} neon_camera_status_t;
#endif
