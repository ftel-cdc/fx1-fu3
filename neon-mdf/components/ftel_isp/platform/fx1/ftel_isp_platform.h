#ifndef FTEL_ISP_PLATFORM_FX1_H
#define FTEL_ISP_PLATFORM_FX1_H

/* Integration FX1: base CSR/MMIO, barrier và thời gian; không phải API công khai. */
#include <stdint.h>
#include "fx1/fx1_memory_map.h" /* BSP cấp base; regmap của IP chỉ cấp offset. */
#include "mcu_device.h" /* Biên dịch kiểm tra map YAML khớp BSP; offset driver vẫn lấy regmap bên trên. */
#ifndef ISP_BASE
#define ISP_BASE FX1_ISP_CSR_BASE
#endif

/*
 * BSP phải cấp địa chỉ nền CSR đã ánh xạ bằng -DISP_BASE=... hoặc header nền tảng.
 * Không có địa chỉ mặc định: regmap của IP chỉ quy định offset, không quy định địa chỉ SoC.
 */
#ifndef ISP_BASE
#error "BSP must define ISP_BASE as the mapped ISP CSR base address"
#endif

#if defined(__cplusplus)
static_assert(((uintptr_t)(ISP_BASE) & 3u) == 0u, "ISP_BASE must be 32-bit aligned");
#else
_Static_assert(((uintptr_t)(ISP_BASE) & 3u) == 0u, "ISP_BASE must be 32-bit aligned");
#endif

#ifndef REG32
#define REG32(a)         (*(volatile uint32_t *)((uintptr_t)(ISP_BASE) + (uintptr_t)(a)))
#define REG_RD(a)        REG32(a)
#define REG_WR(a, v)     (REG32(a) = (uint32_t)(v))
#define REG_SET(a, m)    (REG32(a) |=  (uint32_t)(m))
#define REG_CLR(a, m)    (REG32(a) &= ~(uint32_t)(m))
#define REG_UPD(a, m, v) (REG32(a) = (REG32(a) & ~(uint32_t)(m)) \
                                  | ((uint32_t)(v) & (uint32_t)(m)))

#endif /* REG32: dùng macro của driver chung nếu đã được khai báo. */

void ftel_isp_platform_barrier(void);
uint64_t ftel_isp_platform_time_us(void *user);

#endif
