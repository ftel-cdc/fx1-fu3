# FreeRTOS for GT32U2086

`FreeRTOS-Kernel/` is vendored from [FreeRTOS-Kernel](https://github.com/FreeRTOS/FreeRTOS-Kernel),
restricted to the kernel, public headers, GCC Cortex-M0 port and `heap_4`. Ports,
heaps and example projects outside the GT32U2086 scope have been removed. The root
and `portable/` CMake files are NEON-MDF build definitions for this subset. The
kernel retains GT32U2086 word-only patches for byte fields/access and static-object
layout assertions; preserve or port these patches when updating upstream sources.
The remaining kernel/port sources retain their upstream license; see
`FreeRTOS-Kernel/LICENSE.md`.

The `cmake/freertos.cmake` helper creates `freertos_config`, selects
`GCC_ARM_CM0`/`heap_4` and links the kernel with GT32U2086's word-only runtime
policy. Each application owns its `Inc/FreeRTOSConfig.h`. See the
[FreeRTOS guide](../../guide/FreeRTOSPort.md).
