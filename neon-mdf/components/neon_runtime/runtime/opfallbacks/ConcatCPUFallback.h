#pragma once

#include "core/NPU_ISA.h"
#include "runtime/loader/ModelLoader.h"

#ifdef __cplusplus
extern "C" {
#endif

bool concatCPUFallback(const RuntimeOp& op, const NPUModel& model);

#ifdef __cplusplus
}
#endif
