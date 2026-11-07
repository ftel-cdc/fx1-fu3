# Neon MDF — Source Format & Convention V1.3

## 1. Purpose and scope

Neon MDF applies the [NEON Coding Convention V1.3](NEON_Coding_Convention_V1.3.md) to handwritten
C/header in SDK, templates and examples. This file describes how conventions are applied to existing
Neon MDF structure, build and hardware.

Main convention: public HAL API `neon_module_action()`, private helper `module_action()`, type `_t`,
field `snake_case`, indent 4 spaces and open brackets same line. The private helper keeps the
keyword `static`.

Device register type, name register, startup/linker symbol, `main`, ISR, `SystemInit`,
`SystemCoreClock`, syscall/runtime and FreeRTOS hook keep the platform contract. `SDK_IRQ_*` is
RISC-V Port interface, not HAL API. Cortex-M uses CMSIS directly. CMSIS/FreeRTOS and output
generator are not changed only for style sync.

## 2. Repository structure

```text
<repository>/
├── .clang-format
├── .clang-format-ignore
├── neon-mdf/
│   ├── components/
│   │   ├── driver/                 # HAL and peripheral backend
│   │   ├── console/                # printf/console FU2
│   │   ├── bootloader/
│   │   └── ota/
│   ├── arch/                       # CPU, device, IRQ, runtime
│   ├── mcu-description/            # YAML, schema, generator
│   ├── cmake/                      # Toolchain and build integration
│   ├── tools/                      # CLI, templates, audit and test
│   └── rtos/FreeRTOS-Kernel/        # Upstream and patch platforms
├── examples/<App>/                 # Independent CMake Project
└── guide/
```

SDK/app structure and dimensional dependency are further described in
[ARCHITECTURE.md](../ARCHITECTURE.md).

## 3. Architecture and dependencies

App calls component/HAL; backends API mapping to register, clock, reset and pinmux of family. Driver
does not include `main.h` or source app dependencies. Header uses `mcu_device.h` or header
`neon_<module>_device.h` respectively to get type/instance of selected device.

CMake selects backend according to `NEON_MDF_TARGET_<TARGET>_FAMILY`. Cortex-M enables/disables IRQ
and sets priority via CMSIS `NVIC_*`, critical section saves/restore PRIMASK using CMSIS. RISC-V
uses `SDK_IRQ_*` from `arch/RISCV/Port/`. The required Flash/reset operation on FU2 still runs from
SRAM.

## 4. Directory and file source conventions

| Ingredients | Current path/template |
|---|---|
| Entry header optional | `components/driver/neon.h` |
| Common Type | `components/driver/neon_types.h` |
| Module switches | `components/driver/neon_config.h` |
| Header HAL | `components/driver/<module>/neon_<module>.h` |
| Header instance | `<module>/neon_<module>_device.h`, when API needs alias instance |
| Backend | `<module>/platform/<family>/neon_<module>.c` |
| Internal header by platform | `syscsr/platform/fu2/neon_syscsr_internal.h` |
| FU2 internal memory utility | `components/driver/neon_utils_memory.h` |
| Source/header app | `examples/<App>/Src/`, `Inc/`, or app-root files for standalone FPGA examples |
| Standalone FPGA peripheral example | `examples/<Example>/` |

For example:

```text
components/driver/uart/
├── CMakeLists.txt
├── neon_uart.h
├── neon_uart_device.h
└── platform/
    ├── fu2/neon_uart.c
    └── stm32f103/neon_uart.c
```

The file name uses lowercase letters and underscores. Existing example directory names such
as `UART_RX_Interrupt` and `CRC_Known_Vector` are retained. Each FU2 example has its own `main.c`
and executable. Add implementation files to CMake sources; do not include a `.c` file directly.

## 5. Naming convention

| Ingredients | Format/example |
|---|---|
| Public HAL function | `neon_uart_transmit()`, `neon_flash_erase_sector()` |
| Private function | `static ... uart_wait_tx_ready(...)` |
| Handle | `neon_uart_handle_t` |
| Configuration | `neon_uart_config_t` |
| Struct/enum Other | `neon_uart_state_t`, `neon_spi_fifo_config_t` |
| Common Type | `neon_status_t`, `neon_field8_t`, `neon_field16_t` |
| Named struct tag | `neon_uart_handle_s` if need to put tag |
| Enum value/public macro | `NEON_UART_STATE_READY`, `NEON_GPIO_PIN_7` |
| Private macro | `UART_CONTEXT_PIN_AUTO`, `FLASH_RAMFUNC` |
| Field/parameter/local | `baud_rate`, `clock_frequency_hz`, `timeout_polls` |
| Header guard | `__NEON_UART_H__` by choice of V1.3 |

Private helper does not add `static_` to the name. HAL's type uses `_t`; type register like
`UART_TypeDef` remains device generator of the same level. Internal interface by platform can
network namespace family, for example `fu2_syscsr_*`; application does not use internal headers.

Public enum with explicit value, preserving existing encoding. Use unsigned macro for combining
bitmasks. Keep literal `U`, `UL`, `ULL` by width. Input pointer is not modified using `const`.
Failable operations usually return `neon_status_t`; keep meaning of existing statuses, including
`NEON_NOT_SUPPORTED`, `NEON_LOCKED`, `NEON_NOT_MAPPED`.

## 6. Format header

Order: Doxygen file → include guard → include necessary → type/macro → public prototype → close C++
guard and include guard. Use banner section as current files; only create section with content.

The header itself provides dependencies for its declaration and can be used independently of the
family that the module supports. Wrap external declarations with conditional `extern "C"`. Private
variables/helpers are in `.c`; internal write header `@internal`.

```c
#ifndef __NEON_EXAMPLE_H__
#define __NEON_EXAMPLE_H__

#include "neon_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize the example module.
 * @return NEON_OK on success, otherwise a module error status.
 */
neon_status_t neon_example_init(void);

#ifdef __cplusplus
}
#endif

#endif /* __NEON_EXAMPLE_H__ */
```

## 7. Format source

Source includes the corresponding public header first; followed by dependency private/platform and
standard library. The file was moved to `platform/` include header using a module path like
`"uart/neon_uart.h"`.

Section order:

1. `INCLUDE FILES`
2. `DEFINES AND MACROS` when available
3. `STRUCTURES AND OTHER TYPEDEFS` when available
4. `LOCAL VARIABLES` when available
5. `PRIVATE FUNCTION PROTOTYPES` when available
6. `GLOBAL FUNCTIONS`
7. `PRIVATE FUNCTIONS` when available

Use 4 spaces, open brackets on the same line, with brackets for all `if/else/for/while`. Do not
merge functions, loops or conditional branches into a single line. Declare helper before caller.
File `.c` compiled in C does not need `extern "C"` blocks.

`.clang-format` at root encodes the presentation rules, including `InsertBraces` and recommendation
column 120. `.clang-format-ignore` removes build, upstream and output hardware generator from batch
format operations.

## 8. Comment and Doxygen

The file has `@file`, `@brief`, `@details`. The public API is described at the header with `@brief`,
`@param[in/out/in,out]`, `@return` when non-void. Use `@details`, `@note` when interpreting
blocking, ISR, thread safety, ownership and side effect of registers. Implementation uses `@copydoc`
to reuse contracts.

Do not add `@param None`. Comment focuses on reasons and hardware contracts, specifically timeout,
read-to-clear/W1C, buffer, pinmux and reset order.

## 9. configuration and include

`neon_config.h` offers the `NEON_<MODULE>_MODULE_ENABLED` switch as **0/1**. The default switches
reflect the family and `sdkconfig` of the build; it is possible to override the switch before
including the header. `neon.h` is an optional entry header for including on modules. Each module's
include header remains independent.

```c
#include "neon_config.h"

#if NEON_UART_MODULE_ENABLED == 1
#include "uart/neon_uart.h"
#endif
```

Kconfig/menuconfig is still the source of SDK build selection. CMake reads
`CONFIG_NEON_MDF_DRIVER_<MODULE>` to add a source; `neon_config.h` passes that selection onto the C
switch 0/1. Override switch in source does not automatically add disabled drivers to the library:
when changing features, update the corresponding Kconfig/build. HAL headers do not require the app's
own configuration just to parse declarations.

Details: [MenuConfig.md](MenuConfig.md).

## 10. Handle, configuration and platform

UART/I2C has its own initialization runtime handle and configuration:

```c
neon_uart_handle_t uart;
neon_uart_config_t config;
neon_uart_get_default_config(&config);
config.baud_rate = 19200U;

neon_status_t status = neon_uart_init(&uart, NEON_UART0);
if (status == NEON_OK) {
    status = neon_uart_set_config(&uart, &config);
}
```

Handle is provided by the caller, field is managed by the driver. `init()` binds an instance and
creates a `INITIALIZED` status; `set_config()` copies the configuration to the backend's private
storage and sets up hardware. An instance has one owner after successful configuration, until
`deinit()`.

read information using `get_config()`, `get_instance()`, `get_state()`, `get_error()`. Caller
serialize configuration/teardown operations with other task/ISR using the same peripheral. The API
polling/configuration can block in task context; do not call blocking wait in ISR.

FU2 constraints continue to apply:

- SRAM/MMIO uses aligned word 32-bit. `neon_field8_t` and `neon_field16_t` remain `uint32_t` on FU2;
  the other backend keeps 8/16-bit. width
- Buffer byte/string owns the full word containing data and terminator. Use `<string.h>` for normal
  RAM; runtime associates implementation word-only.
- `neon_utils_memory.h` is only the internal backend of `fu2_runtime.c`. MMIO uses register word
  `volatile`, does not include MMIO in memory/string. function
- Keep DMA's alignment/count/burst and `.ramfunc`'s Flash/reset. requirements
- Keep FU2 compiler policy and check word-access on firmware/archive.
- `timeout_polls` is the number of poll rings. The value 0 makes sense for each API; do not manually
  convert timeout to milliseconds just because the convention example uses ms.

Rebuild the entire object after changing API/type/handle. See [FU2.md](FU2.md).

## 11. Checklist and Inspection

- Select correct component/module and backend family.
- Use the name file/API/type/field and guard according to V1.3.
- Test independent headers, C++ guard and Doxygen of API.
- Update all calls, CMake, templates and tests when changing interface/path.
- Check owner, input `const`, unit, bounds, timeout, ISR and side effect.
- Handwritten format source; retains register/runtime/upstream contract.
- Build related targets, run meaningful regression for changes.
- With FU2 test word-access, SRAM placement and emulation; emulation does not validate FPGA timing or
  actual hardware.

```sh
. neon-mdf/export.sh
cd examples/SPI_W5500
cmake --preset FU2
cmake --build --preset FU2
```

How to add driver: [DriverImplement.md](DriverImplement.md).

## 12. Sample module

This is the minimum layout template; `EXAMPLE` is not a driver available in the SDK. Add header
files according to section 6 and fully describe public prototypes.

```c
/**
 * @file neon_example.c
 * @brief Example source layout.
 * @details Demonstrates V1.3 private naming and function organization.
 */

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include "neon_example.h"

/*==================================================================================================
*                                   PRIVATE FUNCTION PROTOTYPES
==================================================================================================*/
static neon_status_t example_reset(void);

/*==================================================================================================
*                                         GLOBAL FUNCTIONS
==================================================================================================*/
/** @copydoc neon_example_init */
neon_status_t neon_example_init(void) {
    return example_reset();
}

/*==================================================================================================
*                                        PRIVATE FUNCTIONS
==================================================================================================*/
static neon_status_t example_reset(void) {
    return NEON_OK;
}
```

When adding real backend, set source in `platform/<family>/`, declare via
`neon_mdf_driver_select_family_source()` and add Kconfig when optional module.
