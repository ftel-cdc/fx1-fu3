# CMake target dependencies

Each GT32U2086 application is a CMake project. The SDK's
`neon-mdf/CMakeLists.txt` is added as a subdirectory; do not build the SDK directory
as a standalone project.

```text
<app>
 ├── neon_mdf_driver
 │    ├── neon_mdf_device_GT32U2086
 │    └── neon_mdf_kconfig
 ├── neon_mdf_device_GT32U2086 + gcc (word-only runtime before libgcc)
 └── freertos_kernel (optional)
      ├── freertos_kernel_include → freertos_config → device
      ├── freertos_kernel_port_headers
      └── freertos_kernel_port (OBJECT, GCC_ARM_CM0)
```

| Target | Provides |
|---|---|
| `neon_mdf_device_GT32U2086` | SystemInit, startup, runtime, CMSIS/device includes, GT32U2086 macro and compiler policy |
| `neon_mdf_kconfig` | Include directory containing `sdkconfig.h` |
| `neon_mdf_driver` | Peripheral drivers and console when UART is enabled |
| `freertos_config` | Application-owned kernel configuration, linked to the device to inherit its policy |
| `freertos_kernel` | Kernel, heap_4 and Cortex-M0 port |

`neon_mdf_add_executable()` attaches the linker script and post-build image tool
to the application. The linker map corresponds to the ELF; `_image.bin` is the
BootROM binary. Programming, monitoring and menuconfig are host custom targets,
not firmware libraries.

See [BuildSystem](BuildSystem.md) for the configure/build flow and
[Architecture](Architecture.md) for the boundaries between SDK layers.
