#ifndef CAMERA_INTERNAL_H
#define CAMERA_INTERNAL_H
#include "neon_camera_runtime.h"
#include <string.h>
neon_camera_result_t camera_fail(neon_camera_session_t *, neon_camera_result_t);
neon_camera_result_t camera_finish_stop(neon_camera_session_t *);
neon_camera_result_t camera_observe(neon_camera_session_t *, const neon_isp_session_observation_t *);
void camera_history_add(neon_camera_session_t *, const neon_camera_frame_t *);
#endif
