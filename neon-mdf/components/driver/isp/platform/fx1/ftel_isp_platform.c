/* Adapter nền tảng FX1; BSP được build local trong Device/FTEL/FX1. */
#include "platform/fx1/ftel_isp_platform.h"
#include "fx1_fw.h"

void ftel_isp_platform_barrier(void)
{
    fx1_mb();
}

uint64_t ftel_isp_platform_time_us(void *user)
{
    (void)user;
    return fx1_mtime() / (FX1_MTIME_HZ / 1000000u);
}
