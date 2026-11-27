#ifndef NEON_ISP_CLAIM_H
#define NEON_ISP_CLAIM_H
#include <stdbool.h>
/* One physical FX1 ISP. Atomic across callers; token must be stable and non-NULL.
 * Claim is deliberately non-recursive. It serializes sessions, not MMIO calls. */
bool neon_isp_device_claim(const void *owner);
bool neon_isp_device_release(const void *owner);
bool neon_isp_device_available_to(const void *owner);
#endif
