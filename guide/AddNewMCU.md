# Add new target MCU

Step-by-step instructions for adding a new target to the neon-mdf — whether it is a completely new
chip (new architecture, new vendor) or a variant of an existing family
(`FU2`/`WEACT_F412`/`CH32V003`). Specific examples in this post are real data reusability of
`CH32V003F4P6` (the first RISC-V target of the SDK, added after having 2
target Cortex-M).

## 0. Answer 3 questions in advance

1. **CPU architectures have backend yet?** There are currently `cortex-m` and `riscv`
   (`mcu-description/tools/backends/{cortex_m,riscv}/`). Other architectures (e.g. Xtensa) need to
   write new backend — outside of this guide.
2. **New family or existing family?** The new family needs to write GPIO driver +
   `system_<TARGET>.c/.h` from the beginning. The existing family (add 1 device with the same chip,
   other package/flash size variant) — re-use the entire driver, just file `.yaml` + new
   registration.
3. **Do you need bootloader/OTA?** This decision affects whether to write additional Flash driver +
   UART driver + arch glue file or not (section 7-8).

**It is mandatory to have the real Reference Manual of the chip** (not a high-level product brief)
before writing any address/bits. No real TRM → code can be written but PLACEHOLDER must be clearly
marked in YAML (see `devices/FU2.yaml` for example). — does not pretend data is real.

Full reference of each field of the YAML file (step 1 below): `guide/DeviceYAML.md`.

## 1. Write `mcu-description/devices/<TARGET>.yaml`

According to `schema/common.schema.yaml` (general field: `device`/`vendor`/`family`/
`arch`/`cpu`/`memory`/`peripherals`/`interrupts`, optional `bootloader`/ `erase_sectors`) +
`schema/arch/<arch>.schema.yaml` (private field architecture —
Cortex-M: `nvic_priority_bits`/`num_irq`/`vtor_supported`/`fpu_present`;
RISC-V: `isa`/`mtvec_mode`/`interrupt_controller`/`hart_count`, fall into `cpu.extra` automatically,
**no need to fix `model.py`/`loader.py`**).

Minimum need:
- `memory:` — at least 1 zone `kind: flash` + 1 zone `kind: ram`, real address/size.
- `peripherals:` — RCC/clock-enable + GPIO is required (GPIO driver needed). Other peripherals
  (UART/SPI/...) added when really need — do not need to declare all at the beginning, add gradually
  when writing new drivers.
- `interrupts:` — real IRQ number, at least enough so that the table vector is not empty.

Real example (`CH32V003F4P6.yaml`, shortened):

```yaml
device: CH32V003F4P6
vendor: WCH
family: CH32V003
arch: riscv-rv32ec
cpu:
  core: qingke-v2a
  max_freq_hz: 48000000
  isa: rv32ec
  mtvec_mode: vectored-address
  interrupt_controller: pfic
  hart_count: 1
memory:
  - {name: FLASH_MEM, kind: flash, address: 0x00000000, size: 0x4000}
  - {name: RAM,       kind: ram,   address: 0x20000000, size: 0x800}
peripherals:
  - name: RCC
    base: 0x40021000
    registers: [...]
  - name: GPIOA
    base: 0x40010800
    registers: &gpio_regs [...]
  - {name: GPIOC, base: 0x40011000, registers: *gpio_regs}
interrupts:
  - {number: 12, name: SysTick, description: "..."}
  - ...
```

Real traps have been encountered: if the chip has a peripheral ALSO named `FLASH` (flash controller,
for bootloader), naming the flash memory in `memory:` differently (e.g. `FLASH_MEM`) — identical to
`peripherals:[].name: FLASH` will create 2 `#define FLASH_BASE` overlapping in header, error hard to
recognize immediately.

If there is a bootloader/OTA (section 7): add `bootloader: {size, image_header_size, uart:
{peripheral, tx_pin, rx_pin, af, baud}}` and `erase_sectors:` (the real erase sector size
run-length—use to validate `flashmap.csv` later, see `WEACT_F412.yaml`/`CH32V003F4P6.yaml` as
examples of 2 different types: irregular/identical sectors).

## 2. Register in `cmake/mcu_variants.cmake`

```cmake
set(NEON_MDF_TARGET_<TARGET>_ARCH   cortex-m4)      # or riscv-rv32ec,...
set(NEON_MDF_TARGET_<TARGET>_FAMILY <FAMILY>)
```

RISC-V requires 2 additional variables (output path deeper than 1 level:
`arch/RISCV/Device/<Vendor>/<Family>/`, Cortex-M only
`arch/CortexM/CMSIS/Device/<Family>/`):

```cmake
set(NEON_MDF_TARGET_<TARGET>_VENDOR <Vendor>)
set(NEON_MDF_TARGET_<TARGET>_ABI    ilp32e)          # cpu.isa joint in YAML
```

Add `<TARGET>` to `NEON_MDF_KNOWN_TARGETS` at the top of the file.

## 3. Handwritten `system_<TARGET>.c/.h`

**Do not generate automatically** — contains `SystemInit()`/`SystemCoreClockUpdate()` +
`SystemCoreClock` variable, real configuration clock after reset (or leave HSI by default if no
frequency optimization is needed, as long as `SystemCoreClock` matches the actual value running —
everything else in the SDK, including the FreeRTOS tick, calculated according to this variable and
not hardcode Hz).

Set at `arch/{CortexM/CMSIS,RISCV/Device/<Vendor>}/Device/<Family>/Source/system_<TARGET>.c` (+
header same folder as `Include/`).

## 4. Verify codegen running clean (not need app)

```sh
cd neon-mdf/mcu-description/tools
python3 generate.py --device <TARGET>
```

Check that the generated header (`arch/*/Device/<Family>/Include/<TARGET>.h`) has the correct
address/bit written in YAML — read by eye before building app, errors at this step (e.g. wrong
offset) are much cheaper than real-time detection.

## 5. Add CMake preset

`cmake/presets/targets.json` — add 1 `configurePreset` (copy 1 target available with the same
architecture, change `name`/`NEON_MDF_TARGET`/`CMAKE_TOOLCHAIN_FILE` if different toolchain). This
file is common `include` by all — apps do not need to edit the separate `CMakePresets.json` of each
example.

## 6. New Family: write GPIO driver

Full details (API contract, how to wire CMake, add new peripheral type): `guide/DriverImplement.md`.
Summary — requires every family to have (even without bootloader). Create
`components/driver/gpio/neon_gpio_<family>.c`, implement the correct API in
`components/driver/gpio/neon_gpio.h` (`neon_gpio_clock_enable/config/WritePin/ReadPin/TogglePin`) —
use struct register generated from step 1, do not make offsets.

Add dispatch branch in `components/driver/CMakeLists.txt`:

```cmake
elseif(NEON_MDF_TARGET_${NEON_MDF_TARGET}_FAMILY STREQUAL "<FAMILY>")
  set(_neon_mdf_driver_sources gpio/neon_gpio_<family>.c)
```

## 7. Build minimum app trial

Copy `tools/templates/` (available bare-metal plate) to a temporary folder, change
target, build:

```sh
cmake --preset <TARGET>
cmake --build --preset <TARGET>
```

If the target does not have `bootloader:` in YAML, straight app build — stopped here is enough for a
new family without bootloader/OTA. **New Family REQUIRED to also add FAMILY whitelist in
`neon-mdf/CMakeLists.txt`** (line `if(... FAMILY STREQUAL "FU2" OR ... STREQUAL "WEACT_F412" OR
...)`) — missing this step, `components/driver` not `add_subdirectory()`, build error will look like
"missing header" rather than stating the real cause.

## 8. (Optional) Bootloader/OTA

Need to add 3 new files for the new family (if the family is already available, ignore this):

1. **`components/driver/flash/neon_flash_<family>.c`** — implement
   `neon_flash_erase_region`/`neon_flash_program_word` (API in `neon_flash.h`) according to the real
   flash controller (unit erase/program each chip different — eg STM32F4 writes 32-bit/time,
   CH32V003 only writes 16-bit/time although external API still receives 1 word 32-bit). Add
   dispatch branch in `components/driver/CMakeLists.txt` (block `if(NEON_MDF_BOOTLOADER_ENABLED)`).
2. **`components/bootloader/neon_bootloader_uart_<family>.c`** — implement
   `neon_bootloader_uart.h` (`config/SendByte/RecvByte/TryRecvByte`) follows the true UART declared in
   `bootloader.uart` in YAML.
3. **`components/bootloader/neon_bootloader_arch_<family>.c`** — implement
   `neon_bootloader_arch.h` (`SystickStart1ms/Systick1msElapsed/JumpToApp/SystemReset`)
   — this is the real "jump from bootloader to app" part, getting it wrong can cause the board not to
   boot the app (the most serious error in SDK). With the new architecture (not the one
   Cortex-M/RISC-V-PFIC already has), read carefully how that CPU handles the table/reset vector
   before writing `JumpToApp`.

All 3 dispatch files according to FAMILY in `components/bootloader/CMakeLists.txt`.

Verify in the correct order (don't jump):
1. `cmake --build` out both `bootloader` + app — clean link.
2. Load `bootloader` via debug probe (JTAG/SWD depending on architecture), confirm run to SYNC
   waitpoint (measured by breakpoint or temporary LED/UART log).
3. flash app via real UART protocol (not through debug probe) — confirms
   `neon_bootloader_arch_jump_to_app()` jump correctly, the app runs.
4. If you want real OTA: add `flashmap.csv`/`flashmap.<TARGET>.csv` for app, verify factory boot → OTA
   switch → auto-rollback (interrupt power midway through, confirm not to fall into a jam).

Check the RAM/flash budget before committing to OTA — if the chip is small (several dozen KB flash
or less), 2 image (factory+OTA) + bootloader may not have enough space, especially if the app uses
FreeRTOS (minimum kernel cost several KB). If not enough: don't create `flashmap.<TARGET>.csv` for
that app — bootloader falls into a 1-slot-specified model (no OTA, still verify true header/CRC32),
no C code needs to be fixed (see `ARCHITECTURE.md` item 7-8).

## 9. (Optional) FreeRTOS

If the CPU architecture already has the corresponding FreeRTOS port but does not yet wire to
`cmake/freertos.cmake`, add a branch 1 selecting `FREERTOS_PORT` according to `arch` (see
`ARCHITECTURE.md` item 9 for the current port table). If the core does not have CLINT compatible
timers (as in the case of CH32V003/PFIC), the app needs to configure itself tick through
`vPortSetupTimerInterrupt()`/port equivalent function (weak, app override) — see
`tools/templates_freertos/Src/main.c` as the full example.

## 10. Checklist before target is considered "done"

- [ ] `generate.py --device <TARGET>` running clean, header/linker/startup correct address as
      reference manual.
- [ ] Build clean for at least 1 app minimum (`tools/templates/`).
- [ ] Regression-check the targets ALSO still build clean, unchanged size unexpectedly (every change
      in step 2/6/7 is a file **for general use**, easy to accidentally destroy other targets if
      whitelist/dispatch missing `else()`/`elif()` correct
      family).
- [ ] If there is a bootloader: already flash + run on board (not just clean build) — this is the most
      risky component, do not consider "linkable" as "true".
- [ ] Update `README.md` (target table) This — SDK keeps docs describing the current state correctly,
      not the change log (see `ARCHITECTURE.md` at the beginning of the file).

## Quick reference — all files may need to edit/create

| File | Required? | Notes |
|---|---|---|
| `mcu-description/devices/<TARGET>.yaml` | Always... | Source of truth |
| `cmake/mcu_variants.cmake` | Always... | Registration ARCH/FAMILY(/VENDOR/ABI) |
| `arch/*/Device/<Family>/Source/system_<TARGET>.c/.h` | Always... | Handwritten |
| `cmake/presets/targets.json` | Always... | 1 new preset |
| `neon-mdf/CMakeLists.txt` (whitelist FAMILY) | New Family | The easiest to forget |
| `components/driver/gpio/neon_gpio_<family>.c` + dispatch | New Family | |
| `components/driver/flash/neon_flash_<family>.c` + dispatch | Bootloader | |
| `components/bootloader/neon_bootloader_uart_<family>.c` + dispatch | Bootloader | |
| `components/bootloader/neon_bootloader_arch_<family>.c` + dispatch | Bootloader | Highest risk |
| `cmake/freertos.cmake` (new port branch) | FreeRTOS on the new arch | |
| `<app>/flashmap.<TARGET>.csv` | OTA | Per-app, not per-SDK |
