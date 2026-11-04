# Neon-MDF Architecture

This document describes the **current design** of the SDK and the reasoning behind each decision —
not a log of changes.

See more in `guide/`: `FirstApp.md` (tutorial creating a first app), `AddNewMCU.md` (step-by-step
adding a new target), `ArchitectDetail.md` (real dependency graph between blocks), `DeviceYAML.md`
(full reference to each field when initializing `mcu-description/devices/<TARGET>.yaml`),
`DriverImplement.md` (how to write + wire a new driver into CMake), `MenuConfig.md` (how to
configure `menuconfig`/`sdkconfig`), `BootloaderOTA.md` (practical guide to add OTA to an app),
`FreeRTOSPort.md` (how to wire FreeRTOS for a new CPU architecture), `Debugging.md` (debugging
mechanism via VS Code + real gotchas).

## 1. Overview of the pipeline

```
mcu-description/devices/<TARGET>.yaml
        │  (generate.py, run automatically when CMake configures)
        ▼
Header file + linker script + startup + .svd
        │
        ▼
components/driver (API arch-agnostic, 1 implementation/family)
        │
        ▼
app (Src/main.c) ──link──> neon_mdf_driver, [neon_mdf_bootloader_uart, neon_mdf_ota, freertos_kernel]
```

Core Principle: **1 YAML file describes a real chip (address/bit register, no guessing)**,
everything is generated CMSIS-style/RISC-V-style from it via Python. Driver/app only needs to know
**common API** (`NEON_GPIO_*`, `NEON_FLASH_*`) — implementation varies by family, and CMake selects
it during build. App code does not need `#ifdef` according to peripheral register.

## 2. mcu-description/ — the only source of truth

- `schema/common.schema.yaml` + `schema/arch/{cortex-m,riscv}.schema.yaml` —
  Define valid shape of a .yaml file (required/optional fields).
- `devices/<TARGET>.yaml` — 1 file/chip: `vendor`/`family`/`arch`/`cpu`,
  memory (flash/ram, real address + size), peripherals (real base address + register offset/reset
  value, with source reference manual comment), interrupts (real IRQ count), and optionally
  bootloader: (size, image_header_size, uart: — enable bootloader/OTA for that target).
- `tools/mcu_desc/loader.py`/`model.py` - parse YAML into Python object
  (`Device`/`Peripheral`/`Register`/...), validate according to schema. Peripheral with `type:`
  (e.g., `GPIO`) share a single C struct - driver writes according to `type`, not the instance name,
  and is reusable for devices with the same IP.
- `tools/backends/{cortex_m,riscv}/` - generate code TRUE from object:
  - `gen_cmsis_header.py` / `gen_riscv_header.py` — C headers (CMSIS-style register
    structures, `#define <NAME>_BASE`, IRQn enum).
  - `gen_linker_script.py` - linker script GNU ld (MEMORY/SECTIONS), and if there is `bootloader:`,
    generate two versions: 1 for bootloader partition, 1 for
    partition app (`generate_bootloader_partitions()`).
  - `gen_startup_vector.py` (Cortex-M) / `gen_startup.py` (RISC-V) — vector
    table + reset handler (copy `.data`, zero `.bss`, call `SystemInit()` then
    `main()`).
  - `gen_svd.py` - file `.svd` for debugger (common for both architectures, pure generic from list of
    peripheral/register).
- `tools/generate.py` — dispatches based on `device.arch` (`cortex-m*` → `generate_cortex_m()`,
  `riscv*` → `generate_riscv()`), runs automatically through `cmake/mcu_variants.cmake` every time
  configure (no commit of output to git — `arch/*/Device/*/Include/*.h`, `.../Source/gcc/*` are
  generated, only `system_<TARGET>.c/.h` are hand-written).

`_peripheral_struct()` (generate struct C from list of registers) is a pure function,
architecture-independent — `gen_riscv_header.py` imports directly from `gen_cmsis_header.py` rather
than rewriting it.

## 3. `arch/` — separates by CPU architecture

```
arch/
  CortexM/
    CMSIS/CMSIS_5/          # vendor ARM, only Core/Include (core_cm0.h, core_cm4.h, ...)
    CMSIS/Device/<FAMILY>/  # automatically generated (Include/, Source/gcc/) + hand-written `system_<TARGET>.c/.h`
  RISCV/
    Core/                   # CSR macro (riscv_csr.h), soft-math (__udivsi3/__mulsi3/__udivdi3/...), libc shim minimum
    Device/<VENDOR>/<FAMILY>/  # automatically generate headers
    Port/                   # PFIC and CSR mstatus: SDK_IRQ_* for RISC-V/WCH
```

Backend Cortex-M uses CMSIS directly: `NVIC_*` to control each IRQ and
`__get_PRIMASK()`/`__disable_irq()`/`__set_PRIMASK()` for critical sections.
These APIs are available via device header/CMSIS; no source IRQ wrapper is added to target Cortex-M.

RISC-V/WCH uses `arch/RISCV/Port/` with API `SDK_IRQ_*` for PFIC and CSR `mstatus`. CMake only adds
source/include of Port in the RISC-V branch.

RISC-V has no CMSIS — no need for `core_cmX.h`, `__I`/`__O`/`__IO` (CMSIS qualifier used for
register structs) are defined exactly by `gen_riscv_header.py` to use common logic
`_peripheral_struct()`.

## 4. `mcu_device.h` — fixed alias name

Each device generates a file `mcu_device.h` containing only `#include "<DEVICE>.h"` —
code arch-generic (`components/driver`, `components/bootloader`) include
`"mcu_device.h"` instead of hardcoding device names, to avoid needing `#ifdef` according to target
name when only knowing the "current device header".

## 5. CMake: 1 build = 1 target, select via `NEON_MDF_TARGET`

```
tools/cmake/project.cmake        # app include() correctly refers to this file
  └─ cmake/mcu_variants.cmake     # register target, run generate.py, set variables ARCH/FAMILY/LINKER_SCRIPT/...
       └─ add_subdirectory(neon-mdf/)
            ├─ components/driver/     # add_library(neon_mdf_driver), select .c files by FAMILY
            ├─ components/bootloader/ # (if NEON_MDF_BOOTLOADER_ENABLED) neon_mdf_crc32, neon_mdf_bootloader_uart
            ├─ components/ota/        # (if the app has flashmap.csv) neon_mdf_ota
            └─ cmake/kconfig.cmake    # generate sdkconfig.h/.cmake from Kconfig and the application sdkconfig
```

`neon_mdf_add_executable()`/`neon_mdf_add_app_executable()`/
`neon_mdf_add_bootloader_executable()` (`tools/cmake/project.cmake`) are three helpers used by applications to create
executables:

- `neon_mdf_add_executable(name src...)` — build for the entire Flash region when the target has no `bootloader:`
  (for example, `FU2`), or the application does not need to run under a
  bootloader.
- `neon_mdf_add_app_executable(name src...)` — link with linker script partition "app"
  (`NEON_MDF_APP_LINKER_SCRIPT`, generated from `generate_bootloader_partitions()`), requires target
  has `bootloader:`.
- `neon_mdf_add_bootloader_executable(name)` — no source provided, SDK handles via
  `NEON_MDF_BOOTLOADER_MAIN_SOURCES` (see section 7).

Helper functions for creating executable do not automatically add UART syscall for `WEACT_F412` or
`STM32F103CB`. `printf()` on these targets uses newlib/nosys stubs, not logging via UART. UART
bootloader still serves bootloader protocol via its own API.

CMake variable from `mcu_variants.cmake`/`generate.py` needs **2 times `PARENT_SCOPE`** to reach the
app scope: once from function in `mcu_variants.cmake` to `neon-mdf/CMakeLists.txt`, and once again
from there to app (skipping)
`add_subdirectory(${NEON_MDF_PATH} neon-mdf-build)` in
`tools/cmake/project.cmake`) — missing one of the two layers, the variable will be empty in the app
scope without clear error indication (CMake considers a variable not existing as empty).

Different from ESP-IDF in one point: ESP-IDF uses `include()` before `project()` and automatically
"shadow" `project()` with a separate macro to insert logic in between; here, the app calls
`project()` first, `include(tools/cmake/project.cmake)` later — avoid CMake Warning (dev) when
calling `project()` from within a macro via `include()`, but lose the exact order exactly like
ESP-IDF but with nearly unchanged boilerplate.

## 6. `components/driver` — arch-agnostic, 1 file/family

```
components/driver/
  neon_types.h        # neon_status_t is shared
  gpio/neon_gpio.h  # Shared API: neon_gpio_clock_enable/config/WritePin/ReadPin/TogglePin
    platform/fu2/neon_gpio.c / platform/weact_f412/neon_gpio.c / platform/ch32v003/neon_gpio.c
  flash/neon_flash.h  # FU2 main array/NVR; backend different serves bootloader
  crm/, syscsr/      # FU2 clock/reset and pinmux internal
  uart/, i2c/, spi/  # FU2 HAL; UART also has STM32F103 polling backend
  adc/, dma/, crc/, aes/, trng/, rtc/, tim/, pwm/, qspi/, wdt/
  efuse/             # Old OTP map, currently disabled on FU2
```

`components/driver/CMakeLists.txt` selects the correct `.c` file according to
`NEON_MDF_TARGET_<TARGET>_FAMILY` (not according to the exact name of the target) — many `device:`
entries with the same `family:` (if any) share a common implementation driver. GPIO must be present
in every family. FU2 always has Flash, CRM, and SYSCSR; Flash of other families is added only when
`NEON_MDF_BOOTLOADER_ENABLED`. The FU2 driver uses 32-bit aligned words for CPU and DMA. The backend
FU2's critical section directly stores/restore PRIMASK through CMSIS; the Flash helper and reset
routine are kept in SRAM until Flash is ready.

`neon_mdf_add_executable()` (`tools/cmake/project.cmake`) only links 1 target `neon_mdf_driver`
together — SDK currently lacks a system for multiple-layer component/dependency graph as ESP-IDF
really (each component has its own `CMakeLists.txt` that declares `REQUIRES` separately) because
currently only 1 component is real (`components/driver`, `components/bootloader`/`components/ota`
are linked directly when needed rather than through a graph); add this system when the number of
real components actually needs selective dependency declarations that are mutually dependent.

## 7. `components/bootloader` — 2-stage bootloader via UART

```
components/bootloader/
  neon_bootloader_main.c        # Common logic for EVERY architecture/family — NO #ifdef according to architecture
  neon_bootloader_arch.h        # interface arch-specific: SystickStart1ms/Systick1msElapsed/JumpToApp/SystemReset
    platform/weact_f412/neon_bootloader_arch.c   # CMSIS SysTick + SCB->VTOR
    platform/ch32v003/neon_bootloader_arch.c     # STK (QingKe SysTick) + mtvec
  neon_bootloader_uart.h        # interface UART: config/SendByte/RecvByte/TryRecvByte
    platform/weact_f412/neon_bootloader_uart.c   # USART1 real (STM32)
    platform/ch32v003/neon_bootloader_uart.c     # USART1 real (CH32V003, other registers in STM32 even if same name)
  image_header.h                # struct image_header_t: magic/version/size/crc32
```

`neon_bootloader_main.c` is compiled directly as the source of the executable `bootloader` (without
going through any intermediate library) — the object file from the executable is always pulled into
the linker conditionally, avoiding dependency cycles between `main.c` (requires `SystemCoreClock`
from the device) and `startup.s` (requires symbol)
`main`).

UART protocol (similar across all families, implemented by
`neon_bootloader_uart_<family>.c`):
1. Boot → wait ~3s for 1 byte SYNC (`0x55`). If not received → check if current app is valid, jump into
   if valid.
2. Receive SYNC → send `0x52` (READY) → receive 16-byte `image_header_t` → check `magic` → ACK
   (`0x06`)/NAK (`0x15`).
3. Delete region `[app_base, app_vector_base+size)` (how many sectors needed, no limit 1 fixed sector).
4. Receive data in 256-byte chunks, write flash immediately, ACK after each chunk.
5. Calculate CRC32, compare with header — match then jump into app via
   `neon_bootloader_arch_jump_to_app()`, no match then NAK + stop (never jump into wrong image).

Two models simultaneously in the same `neon_bootloader_main.c` file, selected via macro
compile-time `NEON_MDF_FLASHMAP_ENABLED`:
- No (app doesn't have `flashmap.csv`) — 1 fixed slot
  (`NEON_MDF_APP_BASE`/`APP_VECTOR_BASE`/`APP_SIZE`, calculated directly from
  `bootloader.size`/`image_header_size` in YAML) — UART overwrites the slot, no A/B/rollback.
- Yes (`components/ota/flashmap.h` generated from `flashmap.csv`) — 3 partition
  `app`/`app_ota`/`otadata`, verify+rollback (section 8).

Intentional range: no digital signature (only CRC32 — detects transmission error, no spoofing
protection), no downgrade protection, single-bank (no staging A/B symmetric).

## 8. `components/ota` + `flashmap.csv` — partition table + OTA

`flashmap.csv` (optional, set as a side `CMakeLists.txt` of app, syntax similar to `partitions.csv`
of ESP-IDF: `name,type,subtype,size`) is read by `tools/gen_flashmap.py` during configuration
(`cmake/flashmap.cmake`), calculates sequential address, validates each partition falls exactly at
the sector erase boundary (`erase_sectors` in device YAML), generates `flashmap.h`/`flashmap.cmake`
+ linker script for each partition `type=app`.

Can override according to device: `flashmap.<TARGET>.csv` is prioritized over the default
`flashmap.csv` — necessary when building an app for multiple chips with significant flash size
differences (e.g., `WEACT_F412` 512KB vs `CH32V003F4P6` 16KB), or when a target doesn't have space
for OTA (no `flashmap.<TARGET>.csv` for the device → app defaults to the "1 slot fixed" model in
section 7, no code C needs to be changed).

OTA **asymmetric** model (not A/B symmetric ping-pong):
- `app` (factory) — only log when initially loaded (JTAG/UART via bootloader), OTA never touches it →
  always fallback safe.
- `app_ota` — the only OTA slot that is written to.
- `otadata` — `neon_ota_data_t` (`active_slot`, `state`, `boot_attempts`,
  `magic`+`crc32` detect corruption): `components/ota/neon_ota.c` provides
  `neon_ota_read_data/WriteData/ConfirmValid()`, shared by the bootloader (which reads metadata to
  select a boot slot) and the application (which calls `ConfirmValid()` after running reliably).

Safety: if the application does not confirm its image and repeated resets exceed
`NEON_OTA_MAX_BOOT_ATTEMPTS` (default 3), the bootloader writes `active_slot=factory` and jumps to
the factory image automatically.

`neon_mdf_crc32` splits into separate library (not in bootloader) to allow `components/ota` to link
CRC32 without requiring `main()` of bootloader.

## 9. FreeRTOS (`rtos/FreeRTOS-Kernel/`, `cmake/freertos.cmake`)

Official FreeRTOS-Kernel vendor, using pre-existing port - no port is written for any architecture.
`neon_mdf_add_freertos(<target> <config_dir>)` selects `FREERTOS_PORT` according to `arch`

| arch | Port | Note |
|---|---|---|
| `cortex-m0*` | `GCC_ARM_CM0` | Priority PendSV/SysTick is the lowest possible, no need for `configPRIO_BITS` |
| `cortex-m4` | `GCC_ARM_CM4F` | "Direct Routing" - app must manually alias `SVC_Handler`/`PendSV_Handler`/`SysTick_Handler` to `vPortSVCHandler`/`xPortPendSVHandler`/`xPortSysTickHandler` (vector table is not automatically named the port expected) |
| `riscv*` | `GCC_RISC_V_GENERIC` + extension `RISCV_no_extensions` | No real CLINT (chip WCH uses PFIC) - app automatically configures tick timer + switches `mtvec` to Direct mode in `vPortSetupTimerInterrupt()`/`freertos_risc_v_application_interrupt_handler()` (weak, app override) |

**2 minor patches in `rtos/FreeRTOS-Kernel/portable/GCC/RISC-V/`** (clearly defined scope, comment at
place):
- `portContext.h`: 1 place uses register `t3` (x28) as scratch when restoring `mstatus` — not
  available on RV32E (only x0-x15). Patch: use `t0` instead **only when** `__riscv_32e` is defined,
  keep `t3` for RISC-V architecture not E-variant.
- `port.c`: 1 global variable uses `configCPU_CLOCK_HZ` as initializer — this SDK defines this macro
  as a runtime variable (`SystemCoreClock`, shared across all targets because clock can be
  configured differently during runtime), not a compile-time constant as most FreeRTOS demos assume
  — wrap declaration in the correct `#if` that it actually is used in (when CLINT is real).

RISC-V toolchain (`riscv64-elf-gcc`, see section 10) lacks multilib ABI `ilp32e` compatibility — it
automatically provides `__udivdi3`/`__umoddi3` (`arch/RISCV/Core/Src/riscv_soft_math.c`) to perform
64-bit division, which FreeRTOS's `pdMS_TO_TICKS()` always uses internally.

## 10. Toolchain

| Architecture | Toolchain | File | Characteristic |
|---|---|---|---|
| Cortex-M | `arm-none-eabi-gcc` | `cmake/toolchain/arm-none-eabi.cmake` | newlib nano/nosys.specs — has real libc |
| RISC-V | `riscv64-elf-gcc` (Homebrew) | `cmake/toolchain/riscv-none-elf.cmake` | `-nostdlib -ffreestanding`, no libc — everything libc-dependent must be provided by itself |

RISC-V does not have multilib matching `-march=rv32ec_zicsr -mabi=ilp32e` (chip CH32V003 actually:
RV32EC, no extension M — no hardware core / divider) → SDK provides itself:
- `arch/RISCV/Core/Src/riscv_soft_math.c` — `__udivsi3`/`__umodsi3`/
  __mulsi3/__udivdi3/__umoddi3 (bit-serial, more efficient than normal)
- `arch/RISCV/Core/Inc/libc_shim/{string,stdlib}.h` +
  `arch/RISCV/Core/Src/riscv_libc_shim.c` — `memcpy`/`memset`/`memmove`/
  strlen minimum, only wire into `freertos_kernel`'s include path (SDK code uses `__builtin_memcpy()`
  instead of `<string.h>`)

Intentionally NOT using `-march=rv32imac/ABI ilp32` (even if the toolchain has multilib available) -
will cause the compiler to emit `mul`/`div` instructions that do not exist on silicon CH32V003, the
error will only appear when running real.

## 11. Kconfig/`sdkconfig`

Similar to ESP-IDF: `neon-mdf/Kconfig` (mainmenu) → `rsource components/driver/Kconfig` (SDK option)
+ `osource "$(NEON_MDF_APP_KCONFIG)"` (app-specific Kconfig, optional - no error if app does not
exist). `cmake/kconfig.cmake` calls `tools/kconfig/gen_config_header.py` (using `kconfiglib`) to
generate `sdkconfig.h`/`sdkconfig.cmake` from `sdkconfig` of app + Kconfig merged - all merged
options are in a single `menuconfig`/single `sdkconfig`, regardless of SDK or app option.

The optional drivers have their own `CONFIG_NEON_MDF_DRIVER_*` configuration (UART, I2C, SPI, ADC,
DMA, CRC, RTC, TIM, PWM, TRNG, AES, QSPI, WDT). Disable the source type options from the build. The
CMake mapping determines which peripheral family has a backend.

## 12. VS Code extension

`vscode-extension/` — details are in `VSCODE.md`. Main design points:
- Read the target/flash target/executable name dynamically via CMake File API (`src/cmakeFileApi.ts`)
  and `CMakePresets.json` (`src/presets.ts`) — no hardcoding of target/executable names,
  automatically detect new target/app without modification
  extension.
- `dist/extension.js` is an artifact already bundled (esbuild) — change `vscode-extension/src/` to
  `npm run package && npm run vsix` and reinstall `.vsix` to take effect (no need to rebuild when
  opening VS Code).
- Create a new App by copying from `neon-mdf/tools/templates/` or `tools/templates_freertos/` (two
  folders within SDK, not bundled into extension - modify template without rebuilding extension),
  replace token `__APP_NAME__`/`__NEON_MDF_RELATIVE_PATH__` in the file content. The destination
  folder recognizes both cases when opening workspace: exactly one specific app (use the parent
  folder as `examples/`), or the original repository root folder (search for `<root>/examples/` +
  `<root>/neon-mdf/` with SDK marker real).

## 13. Add a new MCU target

Practical guide step by step (checklist, file list to modify): `guide/AddNewMCU.md`.

1. `mcu-description/devices/<TARGET>.yaml` - address/bit must be real (source from Reference Manual in
   comment), not guessed. No real TRM → clearly mark PLACEHOLDER; do not pretend register map is
   confirmed.
2. `cmake/mcu_variants.cmake` — registers `NEON_MDF_TARGET_<TARGET>_ARCH`/ `_FAMILY` (+
   `_VENDOR`/`_ABI` for RISC-V, due to the output path `arch/RISCV/Device/<Vendor>/<Family>/` being
   deeper than one level compared to Cortex-M).
3. Manually write `system_<TARGET>.c/.h` (no automatic generation — configure clock properly after
   reset, different for each chip).
4. New family (no driver): add `components/driver/gpio/neon_gpio_<family>.c` (required), and if needed
   bootloader: `flash/neon_flash_<family>.c`,
   `components/bootloader/neon_bootloader_uart_<family>.c` +
   `neon_bootloader_arch_<family>.c` — dispatch by FAMILY within
   `components/driver/CMakeLists.txt`/`components/bootloader/CMakeLists.txt`.
5. `cmake/presets/targets.json` — add a preset (`include` shared by all apps, no need to modify each
   `CMakePresets.json`).
6. To have FreeRTOS on new arch: add a branch in `cmake/freertos.cmake` (choose appropriate
   `FREERTOS_PORT`, see section 9).

Many `device:` share the same `family:` — use a common directory `arch/*/Device/<Family>/` (each
device differs only by the contents of the files) and reuse the original `components/driver` (guard
according to FAMILY, not by the exact name of the target).

## 14. Known limits and risks

- `FU2`: register map from Common Specification v8 and the BootROM v5 contract. Compilation, linking
  and MMIO emulation do not replace FPGA timing measurements. PAD indices 12..23 still lack complete
  field-level electrical configuration encodings.
- `CH32V003F4P6`: build/link verified, but not yet run on hardware because the board has not arrived.
  Runtime assumptions for bootloader UART, the FreeRTOS STK tick and GPIO are based on the actual
  Reference Manual and await hardware measurements.
- Flash/Debug via VS Code extension only works for Cortex-M (OpenOCD/SWD) - `CH32V003F4P6` requires
  `wlink` (flash) + OpenOCD fork of WCH (debug), not wire.
- STM32/CH32 bootloader OTA only checks CRC32. FU2 RTL BootROM v5 has image with numeric format via
  `fu2_image.py`; default creates non-secure image.
