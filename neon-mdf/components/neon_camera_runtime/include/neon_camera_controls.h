#ifndef NEON_CAMERA_CONTROLS_H
#define NEON_CAMERA_CONTROLS_H
#include <stdint.h>
/* Software/sensor policy, not MAS register fields. Applied validity can only be
 * supplied by sensor evidence; RAW replay exposure/gain remain proposals. */
typedef struct {
    uint32_t valid, exposure_lines, analogue_gain_q8, digital_gain_q8;
    uint32_t wb_red_q8, wb_blue_q8;
} neon_camera_controls_t;
#endif
