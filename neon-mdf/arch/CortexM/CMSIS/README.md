# CMSIS for GT32U2086

`CMSIS_5/` contains CMSIS-Core files from [Arm CMSIS_5](https://github.com/ARM-software/CMSIS_5),
restricted to `core_cm0.h`, version/compiler headers and the upstream license.
Core headers outside the GT32U2086 scope have been removed. When updating the
vendor sources, keep Cortex-M0 dependency headers at the same version and verify the build.

`Device/GT32U2086/` contains headers, startup code and linker scripts generated
from `mcu-description/devices/GT32U2086.yaml`. The system files and
`gt32u2086_runtime.c` are handwritten. See the [SDK architecture](../../../../guide/Architecture.md).
