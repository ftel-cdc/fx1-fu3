# Declare and implement driver (`components/driver/`)

Instructions for writing new drivers in neon-mdf: add implementation for an unknown family (e.g.
GPIO/Flash for a new chip), or add a new peripheral (e.g. SPI, not yet included in the SDK) — and
how to wire into CMake to make the app work.

See `ARCHITECTURE.md` section 6 for a design overview, `guide/ArchitectDetail.md` for the full CMake
dependency graph. This file goes deep into how to write it, not repeating the reason for the design.

## Source format applied to Neon MDF

Apply [NEON Coding Convention V1.3](NEON_Coding_Convention_V1.3.md) and
[Neon integration guide MDF](NEON_MDF_Source_Format_Guideline.md).

- Public HAL API uses `neon_<module>_<action>()`, private helper uses `static ...
  <module>_<action>()`, type uses `_t`, field uses `snake_case`.
- Use 4 spaces, same line open brackets and parentheses for all conditional/loop blocks.
- Header guard uses `__NEON_<MODULE>_H__`; keep C++ guard for external function.
- Common header in module directory, backend in `platform/<family>/neon_<module>.c`.
- The generic type in `neon_types.h`; `neon_config.h` provides module switches 0/1 based on
  Kconfig/sdkconfig. `neon.h` is the optional entry header.
- UART/I2C separates `init(handle, instance)` and `set_config(handle, config)`. The driver owns the
  field handle and keeps a copy of the configuration in private storage.
- Holds register type/device symbol, ISR and startup/runtime hook. Cortex-M uses CMSIS directly; API
  `SDK_IRQ_*` belongs to Port RISC-V.
- Fix header/startup/linker generated from YAML via source description or generator.
- Keep FU2 word-access ABI, compiler policy and `.ramfunc` of Flash/reset.
- CMSIS/FreeRTOS keep existing platform upstream and patch convention.

## Common model: 1 header, N implementation

Every peripheral in `components/driver/` strictly according to 1 mold:

```
components/driver/<peripheral>/
  neon_<peripheral>.h          # GENERAL API - app include() correct this file
  platform/<family>/neon_<peripheral>.c # implementation REAL, 1 file/family
```

App calls `neon_gpio_init()`/`neon_flash_erase_region()`... **identical** on all targets — CMake
selects the correct file `.c` by family at build (`components/driver/CMakeLists.txt`), app never
`#ifdef` by family to call driver.

## 1. Write generic API header (`neon_<peripheral>.h`)

Required rules:

- **Use type-based names, not instance names**: use `GPIO_TypeDef*` rather than
  `GPIOA_TypeDef*`/`GPIO0_TypeDef*`. The type is generated from `mcu-description`
  (`peripherals:[].type`; see `guide/DeviceYAML.md`) and included through the fixed `#include
  "mcu_device.h"` alias, without hardcoding the device name. A driver can therefore accept every
  instance with the same register layout (`GPIOA`/`GPIOC`/`GPIOD`) through one function and support
  future devices in the same family.
- The Cortex-M backend controls IRQ via CMSIS `NVIC_*`; the critical section saves PRIMASK and
  restores it to its previous state. The RISC-V backend uses `SDK_IRQ_*` from
  `arch/RISCV/Port/Inc/sdk_irq.h` to control PFIC/CSR.
- Returning `neon_status_t` (`neon_types.h`: `NEON_OK`/`NEON_ERROR`/`NEON_BUSY`/ `NEON_TIMEOUT`) for
  the function may fail; optional — for the function is definitely error-free (e.g.
  `WritePin`/`TogglePin` now returns `void`).
- Function name `neon_<peripheral>_<action>` (snake_case), macro `NEON_<PERIPHERAL>_*`, type
  `neon_<peripheral>_<name>_t` — see `neon_gpio.h`/ `neon_flash.h` as exact pattern.

Minimum frame example (new peripheral, e.g. SPI):

```c
#ifndef __NEON_SPI_H__
#define __NEON_SPI_H__

#include "neon_types.h"
#include "mcu_device.h"

typedef struct {
    uint32_t clock_speed_hz;
    /* ... */
} neon_spi_config_t;

neon_status_t neon_spi_init(SPI_TypeDef *instance, const neon_spi_config_t *init);
neon_status_t neon_spi_transmit(SPI_TypeDef *instance, const uint8_t *data, uint32_t size_bytes,
                                uint32_t timeout_polls);

#endif /* __NEON_SPI_H__ */
```

`SPI_TypeDef` only exists if the YAML device in the build has a peripheral declaration of `type:
SPI` (see `guide/DeviceYAML.md` entry `peripherals:`) — if the build for the family does not have an
SPI declaration in YAML, the code app that calls `neon_spi_*` will give a clear build error (missing
type), not a hidden error at runtime.

## 2. Write implementation for 1 family (`platform/<family>/neon_<peripheral>.c`)

Straightforward operation through the real struct register (not offset — reference Reference Manual,
see `guide/DeviceYAML.md`). Real example, shortened from
`components/driver/gpio/platform/ch32v003/neon_gpio.c`:

```c
#include "gpio/neon_gpio.h"

neon_status_t neon_gpio_clock_enable(GPIO_TypeDef *instance) {
    /* RCC->APB2PCENR bit2=IOPAEN, bit4=IOPCEN, bit5=IOPDEN - THAT, RM 3.4.7. */
    if (instance == GPIOA) {
        RCC->APB2PCENR |= (1U << 2);
    } else if (instance == GPIOC) {
        RCC->APB2PCENR |= (1U << 4);
    } else if (instance == GPIOD) {
        RCC->APB2PCENR |= (1U << 5);
    } else {
        return NEON_INVALID_PARAM;
    }

    return NEON_OK;
}
```

Notes:
- `neon_gpio_clock_enable()` MUST be pre-called by the app `neon_gpio_init()` on a chip with
  clock-gating (most real chips) — itself does not enable other functions implicitly, even if a
  family does not need to enable clock (no-op), it keeps the API so that the app shares a flow for
  all targets.
- This `.c` file doesn't have its own header. — implements only the interface declared in
  `neon_<peripheral>.h`, without adding a new public function outside of that interface (want to add
  a new function, fix the common header first, so EVERY family knows the interface has changed,
  avoid API deflection between families).

## 3. Wire to CMake

`components/driver` creates a target `neon_mdf_driver`. Each peripheral has its own `CMakeLists.txt`
and selects the source via helper by family:

```cmake
if(CONFIG_NEON_MDF_DRIVER_SPI)
  neon_mdf_driver_select_family_source(
    TARGET neon_mdf_driver
    PERIPHERAL spi
    SOURCES
      FU2 ${CMAKE_CURRENT_LIST_DIR}/platform/fu2/neon_spi.c
      MY_MCU ${CMAKE_CURRENT_LIST_DIR}/neon_spi_my_mcu.c
  )
endif()
```

Add a new folder to the `foreach(... add_subdirectory(...))` loop at
`components/driver/CMakeLists.txt`, and then add Kconfig if the driver can be disabled. Add family
to an existing driver by simply adding the `FAMILY source` pair in the helper. GPIO uses `REQUIRED`
to configure error messages when the backend is missing. FU2 always builds CRM/SYSCSR/Flash; Flash
of another family is only needed when bootloader is enabled.

General target has public include root `components/driver` and link device, Kconfig. No need to add
include path for each peripheral. eFuse still has its own folder; Kconfig keeps it off and audit
ignored because the current FU2 memory map is not supported.

## FU2 Own Rule

- All data access SRAM/MMIO must be 32-bit word aligned. Field logic 8/16-bit in struct uses
  `neon_field8_t`/`neon_field16_t`; buffer bytes have all aligned words and are accessed via
  `<string.h>` of the FU2 runtime.
- DMA must also keep address, byte count and burst divisible by four. Disassembly only checks CPU
  instructions, not proving the DMA configuration is correct.
- The private helper uses `<module>_<action>` and the linkage `static`, for example
  `uart_wait_tx_ready()`. The internal interface between sources has a family namespace like
  `fu2_syscsr_*`.
- Critical section of FU2 backend using PRIMASK save static helper, call `__disable_irq()` and restore
  with `__set_PRIMASK()`. The function Flash/reset must keep the entire call chain running at Flash
  unavailable in SRAM.
- API polling receives timeout according to the number of waiting rounds, specifying zero behavior in
  the header. ADC/CRC zero only checks readiness; DMA zero is explicit infinite waiting.
- UART/I2C has the route option AUTO/KEEP; board configuration pinmux for SPI/PWM/QSPI.

## 4. Link to app — no need to do anything more

App calls `neon_mdf_add_executable()`/`neon_mdf_add_app_executable()` (`tools/cmake/project.cmake`)
already automatically `target_link_libraries(... PRIVATE neon_mdf_driver)` — new driver added to
`components/driver/CMakeLists.txt` automatically available for EVERY app, no need to edit the
`CMakeLists.txt` of each example.
`#include "gpio/neon_gpio.h"` (hay `"<peripheral>/neon_<peripheral>.h"`)
work now because include root PUBLIC is `components/driver/` (last step in section 3).

## 5. Checklist adds a brand new type of peripheral (e.g. SPI, unprecedented)

1. Peripheral declared in `mcu-description/devices/<TARGET>.yaml` not yet (`peripherals:[].type: SPI`,
   see `guide/DeviceYAML.md`) — not yet available, add first, address/register must be TRUE.
2. Create `components/driver/spi/neon_spi.h` (generic API, as per section 1 rules).
3. Create `components/driver/spi/neon_spi_<family>.c` for EVERY family that needs SPI (not required
   every family must have — immediately only the real family).
4. Add the folder to `components/driver/CMakeLists.txt` and create your own CMake using helper in
   section 3; add Kconfig if peripheral is optional.
5. Build a test app called `neon_spi_*` for family just wrote — verify clean link.
6. OTHER regression-check family still builds clean (new dispatch branch must not accidentally
   block/change the old behavior family — see real error: `list(APPEND)` missing previously set
   source).

## 6. Checklist add new family for peripheral YES (e.g. add GPIO for 4th chip)

1. Reference Manual comparisons of new chips — offset/bit CFGLR (or equivalent) may differ from the old
   family despite having the same API function name.
2. Write `platform/<family_moi>/neon_<peripheral>.c` implement CORRECT interface existing in
   `neon_<peripheral>.h` — do not add/remove public function (want to change interface, edit common
   header first, image affect EVERY family, see item 2).
3. Add `FAMILY source` pair to helper in `components/driver/<peripheral>/CMakeLists.txt` (section 3).
4. Build and test the app for the new family — see `guide/AddNewMCU.md` for the entire process of
   adding targets (YAML devices, CMake subscriptions ...) if this is also a completely new target,
   not just the new family for peripheral.
