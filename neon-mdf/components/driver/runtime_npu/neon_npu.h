/**
 * @file neon_npu.h
 * @brief Public C++ entry point for the NEON NPU driver.
 *
 * The current backend is the FX1 V2.0 Virtual Platform. The backend implements
 * the runtime's NPUDriver interface, so applications pass neon_npu_t to the
 * runtime platform and graph APIs.
 */
#ifndef NEON_NPU_H
#define NEON_NPU_H

#ifndef __cplusplus
#error "neon_npu.h is a C++ interface"
#endif

#include "platform/FX1_V2.0/NPUV4Runtime.h"

using neon_npu_t = NPUV4Runtime;

#endif /* NEON_NPU_H */
