# Dependencies between blocks in neon-mdf

This document plots real dependencies between blocks (CMake target + include-dependency at source
level). — is based on real `target_link_libraries()`/ `add_subdirectory()`/`#include` in the repo,
not a theoretical diagram. Use when you need to know if "fixing block X has image affecting block Y"
or "want to use API of Z, link more".

## 1. Layer diagram (low → high)

```
Layer 0 mcu-description/tools/ (Python, run at CMake configure — not runtime dependency)
              │ generated
              ▼
Layer 1  arch/{CortexM,RISCV}/Device/<Family>/   → target neon_mdf_device_<TARGET>
         arch/{CortexM,RISCV}/Core/ (RISC-V: soft-math, libc shim, CSR macro)
              │
              ▼
Layer 2 Cortex-M: CMSIS-Core (NVIC/PRIMASK used directly from device header) arch/RISCV/Port/ (PFIC
         — SDK_IRQ_*, commonly compile to target device RISC-V)
         cmake/kconfig.cmake             → target neon_mdf_kconfig (INTERFACE)
              │
              ▼
Layer 3  components/driver/              → target neon_mdf_driver
              │
              ├──────────────┬───────────────────┐
              ▼               ▼                    ▼
Layer 4 components/ cmake/flashmap.cmake(straight self-app
         bootloader/       → neon_mdf_flashmap      neon_mdf_driver, vd
         → neon_mdf_ (INTERFACE) app no bootloader)
           bootloader_uart,      │
           neon_mdf_crc32        ▼
              │            components/ota/
              │            → neon_mdf_ota
              │            (link: neon_mdf_driver,
              │             neon_mdf_crc32, neon_mdf_flashmap)
              │                  │
              └──────┬───────────┘
                     ▼
Layer 5 executable "bootloader" (compile directly neon_bootloader_main.c — without intermediate
         CMake target) — link neon_mdf_bootloader_uart, neon_mdf_driver, neon_mdf_crc32,
         [neon_mdf_ota if available flashmap.csv]

Layer 6 app executable (Src/main.c of each example) — link neon_mdf_driver,
         [neon_mdf_bootloader_uart, neon_mdf_ota if app has OTA],
         [freertos_kernel if using FreeRTOS]
```

`rtos/FreeRTOS-Kernel/` (target `freertos_kernel`/`freertos_kernel_port`) and
`vscode-extension/`/`tools/neon_mdf.py` **not in the link graph** — FreeRTOS only receives
architectural flags (`-march`/`-mcpu`) via `neon_mdf_apply_{cortex_m,riscv}_arch()`, not
`neon_mdf_device_<TARGET>` links; extension/CLI is a dev-time tool that calls CMake from outside,
not generic compile code into firmware.

## 2. Table of dependencies for each CMake target (confirmed in source, not speculative)

| Target | Type | `target_link_libraries(... PUBLIC ...)` | Definition at |
|---|---|---|---|
| `neon_mdf_device_<TARGET>` | STATIC | *(not depend on any other target SDK — lowest platform)* | `cmake/mcu_variants.cmake` |
| `neon_mdf_kconfig` | INTERFACE | *(include dir to `sdkconfig.h` only)* | `cmake/kconfig.cmake` |
| `neon_mdf_driver` | STATIC | `neon_mdf_device_<TARGET>`, `neon_mdf_kconfig` | `components/driver/CMakeLists.txt` |
| `neon_mdf_crc32` | STATIC | `neon_mdf_device_<TARGET>` | `components/bootloader/CMakeLists.txt` |
| `neon_mdf_bootloader_uart` | STATIC | `neon_mdf_device_<TARGET>` | `components/bootloader/CMakeLists.txt` |
| `neon_mdf_flashmap` | INTERFACE | *(include dir only to generated `flashmap.h`)* | `cmake/flashmap.cmake` |
| `neon_mdf_ota` | STATIC | `neon_mdf_driver`, `neon_mdf_crc32`, `neon_mdf_flashmap` | `components/ota/CMakeLists.txt` |
| `freertos_config` | INTERFACE | *(include dir to app's `FreeRTOSConfig.h` only)* | `cmake/freertos.cmake` |
| `freertos_kernel`, `freertos_kernel_port` | STATIC/OBJECT | *(from `rtos/FreeRTOS-Kernel/`, no link `neon_mdf_device_<TARGET>` — only receive flags `-march`/`-mcpu` via `neon_mdf_apply_*_arch()`)* | `cmake/freertos.cmake` |
| executable `bootloader` | EXE | `neon_mdf_bootloader_uart`, `neon_mdf_driver`, `neon_mdf_crc32`, + `neon_mdf_ota` if app has `flashmap.csv` | `tools/cmake/project.cmake` (`neon_mdf_add_bootloader_executable`) |
| app executable | EXE | `neon_mdf_driver` (+ customized app: `neon_mdf_bootloader_uart`, `neon_mdf_ota`, `freertos_kernel`) | `CMakeLists.txt` of each app |

**`PUBLIC`** in all above targets is the purpose — 1 target link `neon_mdf_driver` automatically
include dir + compile flags (`-mcpu`/`-march`) of `neon_mdf_device_<TARGET>` without manual link 2
times. This is why `components/driver/CMakeLists.txt` only needs
`target_link_libraries(neon_mdf_driver PUBLIC ${NEON_MDF_DEVICE_TARGET} ...)` 1 line, not repeat
`target_compile_options` everywhere else.

## 3. Include-dependency at source level (verified grep, no speculation)

Layering immutability: Low block never `#include` higher block than it.

- `components/driver/` — include nothing from `components/bootloader/` or `components/ota/`. Only
  include `mcu_device.h` (Layer 1) +
  `sdkconfig.h` (Layer 2).
- `arch/RISCV/Port/` — includes nothing from `components/driver/`; provides the `SDK_IRQ_*` API for
  PFIC/CSR. Backend Cortex-M includes the device/CMSIS header and calls directly to NVIC/PRIMASK,
  with no Port dependencies.
- `components/bootloader/neon_bootloader_main.c` — is the ONLY include point in the SDK upside down to
  `components/ota/` (`flashmap.h`, `neon_ota.h`), and ONLY when `NEON_MDF_FLASHMAP_ENABLED` is
  defined (app has `flashmap.csv`) — is not unconditionally included. This is a deliberate
  exception: the bootloader needs to read/write otadata to decide which boot slot, that logic
  belongs to `components/ota` (shared with app, not rewritten).
- `components/ota/neon_ota.c` — include `flash/neon_flash.h` (Layer 3, same level as
  `components/driver`) + `neon_bootloader_crc32.h` (Layer 4, `components/bootloader`) — **not**
  include `neon_bootloader_main.c` or any of the bootloader's `_main`/`_arch` files (only requires
  CRC32, not required
  logic main loop).

## 4. Order `add_subdirectory()` real (`neon-mdf/CMakeLists.txt`)

```
1. cmake/mcu_variants.cmake → neon_mdf_device_<TARGET>, variable ARCH/FAMILY/LINKER_SCRIPT/...
2. cmake/flashmap.cmake → (IF app has flashmap.csv) neon_mdf_flashmap,
                                     add_subdirectory(components/ota) → neon_mdf_ota
                                     -- MUST run BEFORE step 4, because components/bootloader needs neon_mdf_ota link if flashmap is
                                        available
3. cmake/kconfig.cmake            → neon_mdf_kconfig, target menuconfig
4. add_subdirectory(components/driver)     → neon_mdf_driver
5. add_subdirectory(components/bootloader) → (IF NEON_MDF_BOOTLOADER_ENABLED)
                                     neon_mdf_crc32, neon_mdf_bootloader_uart,
                                     Variable NEON_MDF_BOOTLOADER_MAIN_SOURCES
```

This whole `add_subdirectory(neon-mdf/)` is itself in `tools/cmake/project.cmake`, which is
`include()` by `CMakeLists.txt`'s
**each app** — each app re-runs the entire sequence independently (without a single SDK build sharing
multiple apps, in exactly the same way that each example `esp-idf/examples/` re-builds the entire
IDF by itself).

## 5. Specific example: app with OTA (e.g. `OTA_Demo`, target `WEACT_F412`)

```
OTA_Demo/CMakeLists.txt
  └─ include(tools/cmake/project.cmake)
       └─ add_subdirectory(neon-mdf/)
            ├─ neon_mdf_device_WEACT_F412        [Layer 1]
            ├─ neon_mdf_flashmap + neon_mdf_ota [Layer 3-4, as there is flashmap.csv]
            ├─ neon_mdf_kconfig                  [Layer 2]
            ├─ neon_mdf_driver                   [Layer 3]
            └─ neon_mdf_crc32, neon_mdf_bootloader_uart  [Layer 4]
  ├─ neon_mdf_add_bootloader_executable(bootloader)
  │     link: neon_mdf_bootloader_uart, neon_mdf_driver, neon_mdf_crc32, neon_mdf_ota
  └─ 3× add_executable(ota_demo[_ota][_ota_bad] Src/main.c)
        link: neon_mdf_driver, neon_mdf_bootloader_uart, neon_mdf_ota
```

The last 4 executable (`bootloader`, `ota_demo`, `ota_demo_ota`, `ota_demo_ota_bad`) all link
`neon_mdf_ota`+`neon_mdf_driver` — but
**different linker script** (bootloader uses `NEON_MDF_BOOTLOADER_LINKER_SCRIPT`, 3 app uses 3
different `.ld` files created from `flashmap.csv`, 1 file for each partition) so there are still 4
independent `.bin` files despite the same CMake "dependent tree".

## 6. Why this layering is important when modifying code

- Edit `components/driver/gpio/platform/<family>/neon_gpio.c` — only image affects executable link
  `neon_mdf_driver` or more (everything). Safe to edit without worrying about breaking
  `arch/RISCV/Port/`.
- Fixing the `arch/RISCV/Port/Src/sdk_irq.c` — image affects RISC-V builds that use this PFIC/CSR
  class; source is compiled into `neon_mdf_device_<TARGET>`. Check for RISC-V applications calling
  `SDK_IRQ_*` when fixing this file.
- Fixed `components/ota/neon_ota.c` — image affecting both bootloaders (read otadata at boot) REFIX
  app (write otadata at confirm) — 2 where use COMMON 1 implementation, not 2 separate instances, so
  fix logic here is automatically consistent between the two sides (this is why `neon_ota.c` exists
  as a separate component instead of its own code in `neon_bootloader_main.c`).
- Fix `components/bootloader/neon_bootloader_main.c` — **no** image affects
  `components/driver`/`components/ota` (one-direction dependency), but is the most risky file to fix
  (only run before any other app can run — error here may cause the board not boot at all).
