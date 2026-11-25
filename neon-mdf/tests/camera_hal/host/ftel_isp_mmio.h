#ifndef TEST_FTEL_ISP_MMIO_H
#define TEST_FTEL_ISP_MMIO_H
#include <stdint.h>
extern uint32_t ftel_isp_test_registers[0x10000 / 4];
#define REG32(offset) ftel_isp_test_registers[(offset) / 4u]
#endif
