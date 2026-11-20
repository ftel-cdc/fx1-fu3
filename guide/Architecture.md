# NEON-MDF architecture

NEON-MDF targets GT32U2086. Each application owns its CMake project and
configuration; the SDK provides device support, drivers, a console,
configuration tools and firmware packaging.

## Layers

```text
Application (Src/, Inc/, Kconfig, sdkconfig)
        ↓
HAL drivers + console       FreeRTOS (optional)
        ↓                         ↓
GT32U2086 device: CMSIS, startup, SystemInit, word-only runtime
        ↓
GT32U2086 / BootROM v5
```

| Directory | Purpose |
|---|---|
| `neon-mdf/mcu-description/devices/GT32U2086.yaml` | Memory map, registers, IRQs and profiles |
| `neon-mdf/mcu-description/tools/` | Loader/model and Cortex-M0 code generation backend |
| `neon-mdf/arch/CortexM/CMSIS/Device/GT32U2086/` | Handwritten system/runtime code and generated reference snapshots |
| `<application>/build/GT32U2086/generated/GT32U2086/` | Build-local device headers, startup, linker, SVD and image metadata |
| `neon-mdf/arch/CortexM/CMSIS/CMSIS_5/` | CMSIS-Core, restricted to Cortex-M0 core and compiler headers |
| `neon-mdf/components/driver/` | Public APIs and GT32U2086 peripheral backends |
| `neon-mdf/components/console/` | `printf` formatter, `_write` and UART connection |
| `neon-mdf/rtos/` | FreeRTOS kernel, GCC Cortex-M0 port and heap_4 |
| `neon-mdf/cmake/` | Toolchain, device target, Kconfig and image/programming helpers |
| `neon-mdf/tools/` | CLI, image/UART tools and application templates |
| `examples/` | Independent firmware projects |
| `vscode-extension/` | Build/UART programming/monitor UI |

## One device description

CMake runs `generate.py` during configuration. The YAML description generates
`GT32U2086.h`, the `mcu_device.h` alias, linker script, startup vectors, SVD and
an image CMake fragment. Generated files carry a banner; edit the YAML or
generator instead of their output. `system_GT32U2086.c/.h` and
`gt32u2086_runtime.c` are handwritten.

`profile_param` selects internal Flash (`normal`) or QSPI XIP (`xip`). The
physical memory map stays fixed; profiles define the header, IRQ relay and
application memory ranges. Each build directory owns its generated device
files, allowing concurrent normal/XIP builds in separate directories. See [Device YAML](DeviceYAML.md).

## Drivers and runtime

Public headers are in `driver/<peripheral>/neon_<peripheral>.h`; backends are
in `driver/<peripheral>/platform/gt32u2086/`. `neon_mdf_driver` collects the
sources and propagates include paths, the device macro, compiler flags and
configuration to the application.

GT32U2086 SRAM requires word access. The device target propagates compiler
flags and provides word-only memory routines; applications, drivers and
FreeRTOS must share this policy. Access MMIO through driver APIs using the
required register widths.

## Startup and images

For internal Flash, application vectors are linked at `0x00020080`. The image
tool places a 128-byte header at `0x00020000`. BootROM verifies the image and
hands off to the application, forwarding exceptions through the IRQ relay
because Cortex-M0 has no VTOR.

The SDK currently builds an application and its BootROM image. See
[GT32U2086](GT32U2086.md) and [Boot modes](GT32U2086BootModes.md) for the full contract.

## Related documentation

[Build system](BuildSystem.md) explains CMake, [target dependencies](ArchitectDetail.md)
shows how dependencies propagate through libraries, and [SDK internals](SDKInternals.md)
provides a source-reading sequence.
