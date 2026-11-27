#ifndef FX1_ISP_PROFILE_TXN_H
#define FX1_ISP_PROFILE_TXN_H
#include "fx1_isp_backend.h"
enum { FX1_PROFILE_IDLE, FX1_SETUP_RECOVERY, FX1_SETUP_WIDTH, FX1_SETUP_HEIGHT, FX1_SETUP_REST,
       FX1_PROFILE_LOAD, FX1_PROFILE_SEAL, FX1_PROFILE_APPLY, FX1_PROFILE_WAIT };
bool fx1_isp_profile_valid(const neon_isp_session_profile_t *);
neon_camera_result_t fx1_isp_profile_queue(fx1_isp_backend_t *, const neon_isp_session_transaction_t *);
neon_camera_result_t fx1_isp_profile_progress(fx1_isp_backend_t *, neon_isp_session_observation_t *);
#endif
