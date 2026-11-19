# GT32U2086 build system

Run CMake from the application directory. `CMakePresets.json` selects Ninja,
Arm GNU Toolchain, `NEON_MDF_TARGET=GT32U2086` and `build/GT32U2086` as the output directory.

## Configuration

1. `project(<name> C ASM)` initializes the project with the preset's toolchain.
2. `tools/cmake/project.cmake` locates the SDK, validates the target and adds the SDK with `add_subdirectory`.
3. `cmake/mcu_variants.cmake` runs the generator for the `normal` or `xip` profile and creates the device library.
4. `cmake/kconfig.cmake` combines SDK/application Kconfig and generates `sdkconfig.h`/`.cmake`.
5. Drivers and console add sources according to configuration. The application registers its executable and optionally FreeRTOS.

Python reads the YAML; CMake receives the memory layout through
`mcu-description/generated/GT32U2086_image.cmake`. Reconfigure after changing
YAML. CMake uses `Python3_EXECUTABLE` for device generation, Kconfig and
packaging; select the `.venv` interpreter as described in [Setup](Setup.md).

## Building and linking

`neon_mdf_add_executable()` links the application with the driver, device and
libgcc, and selects the generated linker script. `-u,__clzsi2` selects the
word-only runtime before libgcc helpers; `-nostdlib` keeps runtime and entry
point selection under SDK control.

After linking, `cmake/gt32u2086_image.cmake` runs objcopy, reports size and
invokes `tools/gt32u2086_image.py`. The tool validates ELF placement and packages
the header/payload.

| Artifact | Purpose |
|---|---|
| `<app>` | ELF with symbols for analysis/debugging |
| `<app>.map` | Linker map and memory ranges |
| `<app>.bin` | Payload starting at the vector base |
| `<app>_image.bin` | BootROM header + payload for internal Flash programming |

XIP profiles add `_xip` to binary names; see [GT32U2086](GT32U2086.md).

## Common operation targets

```sh
cmake --preset GT32U2086
cmake --build --preset GT32U2086
cmake --build --preset GT32U2086 --target menuconfig
cmake --build --preset GT32U2086 --target flash_uart_gt32u2086
cmake --build --preset GT32U2086 --target serial_monitor
```

Programming/monitoring read `NEON_MDF_SERIAL_PORT`, baud and UART parameters at
**execution time**. Port/baud changes do not require reconfiguration. Programming
targets depend on the application so firmware is packaged before transmission.
For projects with multiple executables, use `<app>_flash_uart_gt32u2086` to select
the image; `flash_uart_gt32u2086` uses the first packaged executable.

## Application configuration

`CONFIG_NEON_MDF_DRIVER_*` controls optional drivers. GPIO, Flash, CRM and SYSCSR
are always-built infrastructure. UART also enables the console; applications
calling `printf` must enable UART. Each project owns its `sdkconfig`. See
[menuconfig](MenuConfig.md).

FreeRTOS is added explicitly with `neon_mdf_add_freertos()`, using the kernel,
GCC ARM_CM0 and heap_4. Other ports have been removed from the vendor subset.
See [FreeRTOS](FreeRTOSPort.md).

## Start from a fresh configuration

```sh
cmake --preset GT32U2086 --fresh
cmake --build --preset GT32U2086
```

Use `--fresh` after changing the toolchain, Python or checkout path. The new
preset uses `build/GT32U2086`; old binaries are not outputs of the current source.
Generated sources are shared between applications/profiles, so configure and
build sequentially, especially when changing `NEON_MDF_PROFILE`.
