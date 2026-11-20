#pragma once
// C++ definition of the opaque NeonPlatform from <neon/c_runtime_api.h>.
// Device glue fills it (e.g. VP_FX1_V2.0: NPUV4Runtime implements all three).
#include "../../core/DeviceTarget.h"
#include "../../core/NPUComponent.h"
#include "../driver/NPUDriver.h"

struct NeonPlatform {
  const DeviceTarget* target;
  const NPUComponent* component;
  NPUDriver*          driver;
};
