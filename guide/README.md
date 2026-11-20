# NEON-MDF documentation

Start with the [README](../README.md) to build your first GT32U2086 firmware,
then choose a guide for your next task.

## Setup and application development

| Guide | Contents |
|---|---|
| [Environment setup](Setup.md) | Windows/Linux/macOS tools, Python venv, PATH and toolchain verification |
| [Board configuration](BoardConfiguration.md) | Actual clocks, connector pins, bitstreams and storage reservations |
| [Your first application](FirstApp.md) | Create bare-metal or FreeRTOS projects, understand files, build and run |
| [VS Code](VSCode.md) | Install the extension, select SDK/UART port, build, program and monitor |
| [Example index](../examples/README.md) | Peripheral examples and connection requirements |
| [menuconfig](MenuConfig.md) | Select drivers, configure applications and use sdkconfig in C/CMake |
| [Printf / logging](PrintfLogging.md) | UART console, formatter, baud and formatting limitations |
| [FreeRTOS](FreeRTOSPort.md) | Cortex-M0 port, heap/stack, interrupt handlers and queues |
| [Debugging](Debugging.md) | Image validation, UART logging and JTAG/OpenOCD integration status |
| [Hardware acceptance](HardwareValidation.md) | Support limits, board evidence and automated regressions |

## Hardware and firmware images

| Guide | Contents |
|---|---|
| [GT32U2086](GT32U2086.md) | Memory, BootROM v5, word-only runtime, normal/XIP profiles |
| [UART programming](GT32U2086Flashing.md) | Wiring, tool parameters, programming/monitoring and troubleshooting |
| [Boot modes](GT32U2086BootModes.md) | Boot selection, image preparation, internal Flash and QSPI |
| [Secure boot](SecureBoot.md) | Build and program signed internal Flash images |
| [Device YAML](DeviceYAML.md) | Registers, IRQs and memory profiles |

## SDK development

| Guide | Contents |
|---|---|
| [Architecture](Architecture.md) | SDK layers, device descriptions and processing flow |
| [Build system](BuildSystem.md) | CMake presets, targets, code generation, packaging and host tools |
| [Target dependencies](ArchitectDetail.md) | Application, device, driver, Kconfig and FreeRTOS dependencies |
| [Driver development](DriverImplement.md) | APIs, GT32U2086 backends, configuration and verification |
| [Reading the SDK source](SDKInternals.md) | Source walkthrough from startup to peripherals |
| [Coding convention](NEON_Coding_Convention_V1.3.md) | Naming and coding style |
| [Applying the convention](NEON_MDF_Source_Format_Guideline.md) | Source/header layout in this repository |

## Documentation organization

The README introduces the SDK, lists prerequisites and provides one initial
build/program workflow. Procedures, architecture explanations and references
are kept in separate guides. This organization draws on how vendors separate
getting-started material from detailed documentation:
[ST · SDK README](https://github.com/STMicroelectronics/STM32CubeF4/blob/master/README.md),
[NXP · Getting Started / User Guide](https://mcuxpresso.nxp.com/mcuxsdk/latest/),
[Microchip · Harmony](https://www.microchip.com/en-us/tools-resources/configure/mplab-harmony)
and [Espressif · README](https://github.com/espressif/esp-idf/blob/master/README.md).
