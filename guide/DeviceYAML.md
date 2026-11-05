# `mcu-description/devices/<TARGET>.yaml` declaration

Fully reference each field when writing the MCU description file — the only true source from which
all header/linker script/startup/`.svd` were created (see `ARCHITECTURE.md` section 1-2). This file
explains what field, what meaning, how valid the value is. — want to know the full process add a new
target (CMake registration, driver, bootloader ...) see `guide/AddNewMCU.md`.

The real schema source (this is an interpreting document) — is misleading with the following 2
files, the following 2 files are correct): `mcu-description/schema/common.schema.yaml` (general
field for all architectures) + `mcu-description/schema/arch/<arch>.schema.yaml` (specific field for
architectures). `loader.py` only validate mandatory fields (raise clear errors if missing) —
optional fields have default values specified in each item below.

## Minimum file size

```yaml
device: MY_CHIP
vendor: MyVendor
family: MY_CHIP
arch: cortex-m4          # or riscv-rv32ec,...
description: >
  open me short this chip.

cpu:
  core: cortex-m4
  revision: r0p1
  max_freq_hz: 100000000
  # + private field arch, see "cpu:" section below

memory:
  - {name: FLASH_MEM, kind: flash, address: 0x08000000, size: 0x80000}
  - {name: RAM,       kind: ram,   address: 0x20000000, size: 0x40000}

peripherals:
  - name: RCC
    base: 0x40023800
    registers: [...]

interrupts:
  - {number: 0, name: WWDG, description: "..."}
```

## Original Level Field

| Field | Required | Meaning |
|---|---|---|
| `device` | Yes, I... | The specific chip name, e.g. `WEACT_F412`, `CH32V003F4P6` — serves as the generated filename header/linker/startup (`<device>.h`, `startup_<device>.s`...) and the value `NEON_MDF_TARGET`. |
| `vendor` | Yes, I... | Manufacturer/team name, e.g. `STMicroelectronics`, `WCH`. Only the description of — RISC-V is used to compute the path `arch/RISCV/Device/<vendor>/<family>/` (see `cmake/mcu_variants.cmake`'s `NEON_MDF_TARGET_<TARGET>_VENDOR`), Cortex-M does not use this field for directory names. |
| `family` | Yes, I... | Chip family name, e.g. `WEACT_F412`, `CH32V003`. Many `device` and `family` share the same `arch/*/Device/<family>/` directory and reuse the same `components/driver` (dispatch by family, see `guide/AddNewMCU.md`). Usually overlap `device` if the chip has no other variants yet. |
| `arch` | Yes, I... | 1 in: `cortex-m0`, `cortex-m0plus`, `cortex-m3`, `cortex-m4`, `riscv-rv32ec` (real use, WCH QingKe V2A) — values start with `cortex-m`/`riscv` which decides which backend codegen to run (`generate_cortex_m()`/`generate_riscv()` in `generate.py`) and which schema `arch/*.schema.yaml` applies to `cpu:` below. |
| `description` | No | Short description, only for reading, no codegen affected image. |

## `cpu:`

General Field (all architecture):

| Field | Required | Meaning |
|---|---|---|
| `core` | Yes, I... | CPU core name, e.g. `cortex-m4`, `qingke-v2a`. |
| `revision` | No (default is empty) | Ex `r0p1` — write `"unknown"` if not confirmed, do not leave blank misleading that looked up. |
| `max_freq_hz` | Yes, I... | Max. frequency (Hz) — only described, not auto-pressured to `SystemInit()` (real clock configuration manual in `system_<TARGET>.c`, see `guide/AddNewMCU.md` step 3). |

Private fields according to `arch` (read in `cpu.extra` automatic dict — **no need to edit
`model.py`/`loader.py`** when adding new arch-specific field, see first comment
`riscv.schema.yaml`):

**Cortex-M** (`schema/arch/cortex-m.schema.yaml`):

| Field | Required | Meaning |
|---|---|---|
| `nvic_priority_bits` | Yes (if using FreeRTOS port CM4F) | The number of real NVIC priority bits (usually 4 on STM32). This value mismatches the entire FreeRTOS priority scheme (`configPRIO_BITS` must match) — confirms via real datasheet, no guessing. |
| `num_irq` | Yes, I... | The peripheral IRQ number (NVIC supports up to 32 on Cortex-M0/M0+, IRQ0..IRQ31) — determines the length of the "IRQ0.." section of the generated vector table. ** THIS field is also mandatory on RISC-V if the device has a `bootloader:` declaration** (see warning in `bootloader:` section below) although not included in the official field list of `riscv.schema.yaml` — `loader.py`'s `_check_bootloader_vtor_alignment()` reads `cpu.num_irq` NOT CONDITIONAL according to `arch`, raise obvious error if missing. On RISC-V, interpreted as "total actual vector table slot count" (e.g. CH32V003: highest PFIC entry used is 38 → `num_irq: 39`), not Cortex-M concept NVIC IRQ0.. |
| `vtor_supported` | Yes, I... | Pure Cortex-M0 (ARMv6-M) does not have standard VTOR — if `false`, the vector table is REQUIRED to be at address `0x00000000` (the bootloader+app layout cannot be used, because the app can't set the vector table to a nonzero address). |
| `mpu_present` | No (default `false`) | Is there a hardware MPU — confirmed via the macro `__MPU_PRESENT` in the vendor's native CMSIS header, not inferred from the core name. |
| `fpu_present` | No (default `false`) | It only makes sense for `cortex-m4` — `true` to create the macro `__FPU_PRESENT`** and** need to manually add `-mfloat-abi=hard -mfpu=fpv4-sp-d16` in `cmake/arch/cortex-m.cmake` to actually use hardware (declaring `true` does not automatically enable compile flags). |

Vector table Cortex-M **no YAML declaration required** — fixed to the architecture ARMv6-M/ARMv7-M
(SP/Reset/NMI/HardFault/.../PendSV/SysTick and then IRQ0..), `gen_startup_vector.py` generated the
correct layout, only `interrupts:` (bottom entry) filled in the name for the IRQ section.

**RISC-V** (`schema/arch/riscv.schema.yaml`, real deployment for WCH QingKe V2A — core other may need
`mtvec_mode`/`interrupt_controller` value other):

| Field | Required | Meaning |
|---|---|---|
| `isa` | Yes, I... | For example `rv32ec` (Embedded+Compressed, NOT M/A/F on CH32V003 — uses true silicon ISA, not "want-to-use" ISA; using `rv32imac` for chips with no kernel/divided hardware will cause the compiler to issue a command that says `mul`/`div` does not exist, errors are only revealed when running in real time). |
| `mtvec_mode` | Yes, I... | How CPUs use register `mtvec` for vector table positioning — WCH QingKe Used `vectored-address` (vendor-specific value, `mtvec[1:0]=3` : each entry is a function pointer, entry 0 is COMMAND `j` not address, other `"direct"` / The `"vectored"` Basic RISC-V standard `mtvec[1:0]=0/1` ). Other RISC-V core need to read carefully that core document before filling. |
| `interrupt_controller` | Yes, I... | The actual interrupt controller name, e.g. `pfic` (WCH, vendor-specific — NOT CLINT/PLIC RISC-V standard). Core-private, NOT declared in `peripherals:` (similar to Cortex-M's NVIC) — address hardcode in backend codegen (`gen_startup.py`) + `arch/RISCV/Port/`. |
| `hart_count` | Yes, I... | Hart number (RiSC - V logic core) — `1` for every single - core chip available in the SDK. |
| `num_irq` | Yes, IF there is `bootloader:` | NOT in RISC-V's "official" field, but `loader.py` is still required when the device declares `bootloader:` (see table `cpu:` above + item `bootloader:` below) The missing — raises errors as soon as `generate.py` runs, not silently. |

## `memory:`

List of memory — used to create linker script `MEMORY {}`.

```yaml
memory:
  - name: FLASH_MEM
    kind: flash
    address: 0x08000000
    size: 0x80000
  - name: RAM
    kind: ram
    address: 0x20000000
    size: 0x40000
```

| Field | Required | Meaning |
|---|---|---|
| `name` | Yes, I... | The memory name — serves as the name `#define <name>_BASE` in the generated header. |
| `kind` | Yes, I... | `flash` \| `rom` \| `ram`. Generator selects `flash`/`ram` lowest address as default for code/data (`_pick_flash()`/`_pick_ram()` in backend, see `ARCHITECTURE.md` section 2) if there are multiple zones of the same type. |
| `address` | Yes, I... | address starts real. |
| `size` | Yes, I... | real size (bytes). |

Real traps encountered: if the chip has a peripheral (bottom entry) ALSO named `FLASH` (flash
controller, needed for bootloader), set flash memory name differently (e.g. `FLASH_MEM`, used in
both `WEACT_F412.yaml` and `CH32V003F4P6.yaml`) — same name as `peripherals:[].name: FLASH` create 2
overlapping `#define FLASH_BASE`, error difficult to recognize at build time.

## `peripherals:`

List of — peripherals used to create CMSIS + `.svd` type C structs.

```yaml
peripherals:
  - name: RCC
    base: 0x40021000
    description: "Reset and Clock Control - RM chapter 3"
    registers:
      - {name: APB2PCENR, offset: 0x18, size: 32, access: rw, reset: 0x00000000, description: "..."}
```

| Field | Required | Meaning |
|---|---|---|
| `name` | Yes, I... | The peripheral name, e.g. `USART1`. Becomes `#define <name> ((<type>_TypeDef*)<name>_BASE)` in the header — this is the actual app code name used (`USART1->DATAR = ...`). |
| `base` | Yes, I... | real address base. |
| `type` | No (default = `name`) | **Group dedup struct C** — multiple peripherals with `type` generate GENERAL 1 struct (`<type>_TypeDef`), only different from `#define <name>_BASE`. Used when multiple instances are identical layout registers (e.g. `GPIOA`/`GPIOC`/`GPIOD` with `type: GPIO`) — avoid generator print identical duplicate struct N times. |
| `description` | No | Descriptive, should quote the real chapter/item Reference Manual. |
| `registers` | Yes (list, can be empty) | See below. |

**Shared layout template via YAML anchor** (when multiple peripherals are in the same `type`, do not
duplicate lists of real `registers` — see real `CH32V003F4P6.yaml`):

```yaml
peripherals:
  - name: GPIOA
    base: 0x40010800
    type: GPIO
    registers: &gpio_regs
      - {name: CFGLR, offset: 0x00, size: 32, access: rw, reset: 0x44444444, description: "..."}
      - {name: INDR,  offset: 0x08, size: 32, access: ro, reset: 0x00000000, description: "..."}
  - {name: GPIOC, base: 0x40011000, type: GPIO, description: "...", registers: *gpio_regs}
  - {name: GPIOD, base: 0x40011400, type: GPIO, description: "...", registers: *gpio_regs}
```

### `registers:`

| Field | Required | Meaning |
|---|---|---|
| `name` | Yes, I... | Register name, e.g. `CTLR1`. Becomes field struct C (`USART1->CTLR1`). |
| `offset` | Yes, I... | Offset (byte) from `base` of the peripheral. |
| `size` | No (default `32`) | `8` \| `16` \| `32` — register width (bits), C-type decision (`uint8_t`/`uint16_t`/`uint32_t`) in the generated struct. |
| `access` | No (default `rw`) | `ro` \| `wo` \| `rw` — is for `.svd`/document only, generator does NOT block write to register `ro` at level C (no automatic `const`). |
| `reset` | No (default `0x0`) | Real reset value — only for `.svd` (debugger shows default value), no image affected runtime. |
| `description` | No | It is recommended to quote the actual Reference Manual chapter/section. |
| `fields` | No (default is empty) | A detailed bitfield list (`name`/`bit_offset`/`bit_width`/`description`) — is required only when there is a detailed bitfield document and indeed needed (most drivers in this SDK define `#define <REG>_<BIT> (1u << N)` themselves right in the file `.c` see because relying on `fields:`, seeing `platform/ch32v003/neon_gpio.c`/`platform/ch32v003/neon_flash.c` as an example — to be empty is normal, not omission). |

## `interrupts:`

```yaml
interrupts:
  - {number: 12, name: SysTick, description: "System timer interrupt - THAT, RM 6.3"}
```

| Field | Required | Meaning |
|---|---|---|
| `number` | Yes, I... | The real IRQ number. Cortex-M: `0..(cpu.num_irq-1)`, corresponding to `IRQ0..IRQ(num_irq-1)` in the standard vector table ARMv6-M/v7-M (see fixed table in `cpu:` Cortex-M above). RISC-V/PFIC: the real slot number in the vendor vector table (e.g. CH32V003: 2-3=NMI/HardFault fixed according to architecture not declared here, 4 or more is the real number of each IRQ, see `gen_startup.py`). |
| `name` | Yes, I... | generate `<name>_IRQn` (enum `IRQn_Type`) + `<name>_IRQHandler` (default weak handler in startup, override by defining function of the same name). |
| `description` | No | It is recommended to quote the actual Reference Manual chapter/section. |

No need to declare any IRQ of the chip with —, only declare which IRQ SDK/app actually uses
(STK/SysTick, USART for bootloader...); the remaining slot generator fills `Reserved`/`0` in vector
table by itself, no error.

## `bootloader:` (optional)

Only declare when device build according to bootloader+app model (see `ARCHITECTURE.md` item 7).
There is this field → generator generates additional `<DEVICE>_BOOTLOADER.ld` +
`<DEVICE>_APP.ld` + macro `NEON_MDF_APP_BASE`/`APP_VECTOR_BASE`/`APP_SIZE`,
and `NEON_MDF_BOOTLOADER_ENABLED=ON` in CMake.

```yaml
bootloader:
  size: 0x8000
  image_header_size: 0x200
  uart:
    peripheral: USART1
    tx_pin: PA9
    rx_pin: PA10
    af: 7
    baud: 115200
```

| Field | Required | Meaning |
|---|---|---|
| `size` | Yes, I... | size partition bootloader (bytes), from the beginning of the lowest flash area. |
| `image_header_size` | Yes, I... | size "slot" header before app vector table (byte) — MUST be large enough to satisfy alignment requirements: multiple of 2^n ≥ `4*(16+cpu.num_irq)`, minimum `0x80` — `loader.py` manual calculation and raise obvious error if not satisfied (`_check_bootloader_vtor_alignment()`), no manual calculation required. Check now applies NO CONDITIONS under `arch` (running for all devices with `bootloader:`, including RISC-V without real VTOR — full-borrowed formula from ARMv7-M but still used to ensure that `image_header_size` is large enough in general) → `cpu.num_irq` MUST declare even though writing YAML for RISC-V (see item `cpu:` above). DO NOT set the correct `image_header_size` to equal the true `sizeof(image_header_t)` (16 bytes) — is always much larger for alignment reasons. |
| `uart.peripheral` | Yes, I... | Name in `peripherals:[].name` used as firmware receiver UART. |
| `uart.tx_pin`/`rx_pin` | Yes, I... | E.g. `PA9`/`PA10` — only describes, the real UART driver (`neon_bootloader_uart_<family>.c`) manually configures the GPIO by hand, does not read this field to generate code. |
| `uart.af` | Yes, I... | Alternate function number (Cortex-M) — `0` if the chip does not need AFIO remap (e.g. CH32V003 uses default mapping). |
| `uart.baud` | Yes, I... | Vd `115200`. |

## `erase_sectors:` (optional)

Just if the app of this device uses `flashmap.csv` (OTA, see `ARCHITECTURE.md` section 8) — is used
to validate each partition that falls within true sector erase boundaries. Do not declare =
generator ignores this validate (safe if the app does not use flashmap.csv).

```yaml
erase_sectors:
  - {size: 0x4000, count: 4}    # 4 sector 16KB
  - {size: 0x10000, count: 1}   # 1 sector 64KB
  - {size: 0x20000, count: 3}   # 3 sector 128KB
```

Run-length (group of sectors the same size in a row), listing the correct order of addresses
ascending from the beginning of the flash area. **Total `size*count` of all groups MUST match
EXACTLY the lowest flash area size in `memory:`** — `loader.py` raise obvious errors if deviated
(catch mistaken typing/missing sector early, before image affects real flash).

Chips with evenly spaced sectors (e.g. CH32V003: 16 1KB sectors) only need 1 line
`{size: 0x400, count: 16}`.

## PLACEHOLDER Rules vs Real Figures

If only product brief (no Technical Reference Manual register level) as `FU2.yaml` currently — still
write YAML to pipeline run out (build real `.elf`), but ** MUST clearly mark in comment** where is
the guess / provisional data (see `FU2.yaml` as a sample) — not to be confused with true confirmed
data that readers later mistakenly believed to have looked up. Do not flash firmware build from YAML
and placeholder onto real silicon.

All fields with "THAT"/"THAT" in the comment of existing `.yaml` files (`WEACT_F412.yaml`,
`CH32V003F4P6.yaml`) means that they have been directly compared with vendor's official Reference
Manual/datasheet — keep this comment convention when adding new fields, to distinguish clearly from
guesses.
