# GT32U2086 device description in YAML

The device description is in `neon-mdf/mcu-description/devices/GT32U2086.yaml`.
The loader validates it and the backend generates CMSIS headers, linker scripts,
startup code, SVD and an image CMake fragment. Reference schemas are in
`mcu-description/schema/`; executable validation is implemented in
`tools/mcu_desc/loader.py` and `memory_layout.py`.

## Device and CPU information

```yaml
device: GT32U2086
vendor: FPTTelecom
family: GT32U2086
arch: cortex-m0
cpu:
  core: cortex-m0
  revision: r0p0
  max_freq_hz: 60000000
  nvic_priority_bits: 2
  num_irq: 32
  vtor_supported: false
```

`device`, `family` and `arch` identify the supported target. `max_freq_hz`
describes the CPU limit; it does not configure the board clock. Handwritten
system code determines the runtime clock. Other CPU fields control CMSIS macros
and vector generation.

## Memory map and profiles

`memory` lists physical regions with `name`, `kind` (`flash`, `ram`, `rom`),
`address` and `size`. To relocate an image, use `profile_param` to select regions
and allocate application ranges rather than changing physical addresses.

Each `normal`/`xip` profile has `flash` and `ram` mappings containing `region`
and `layout`. Layout entries have `name`, `offset`, `size`, `usage` and an
optional description. Offsets are relative to the parent region;
`size: remaining` uses the rest of the region from that offset. Entries must fit
within the parent without overlap. Each mapping needs an `application` entry
with `usage: allocatable`.

| Profile | Image base | Vector base | Application RAM |
|---|---|---|---|
| `normal` | `0x00020000` | `0x00020080` | `0x01001000` to `0x0101FFFF` |
| `xip` | `0x30000000` | `0x30000080` | Same as normal |

`image_header` in Flash and `irq_relay` in SRAM are ROM reservations. GT32U2086
profiles reserve a 128-byte header and the first 4 KB of SRAM. Startup copies
and clears only application data/bss; the relay must remain intact. See
[GT32U2086](GT32U2086.md).

## Peripherals and registers

Each peripheral has `name`, `base`, `registers`, and optionally `type` and a
description. Registers declare `name`, `offset`, `size`, `access`, reset values,
and optional `fields`/`aliases`. Access permissions are `ro`, `wo` or `rw`.

```yaml
registers:
  - name: CTRL
    offset: 0x00
    size: 32
    access: rw
    reset: 0x00000000
    fields:
      - {name: ENABLE, bit_offset: 0, bit_width: 1}
```

This illustrates syntax; actual addresses/bits must come from the device YAML
and hardware documentation. `aliases` describe multiple views of one address,
such as mode-dependent UART registers. Instances sharing a `type` must have
compatible register layouts. Generated headers use structs, reserved ranges and
bitfield macros derived from this data.

## Interrupts

`interrupts` entries contain `number`, `name` and `description`. `number` is the
NVIC IRQ index from 0 to `cpu.num_irq - 1`; names generate `*_IRQn` and
`*_IRQHandler`. Entries are placed at **16 + number**, regardless of declaration
order. Undeclared IRQ slots are reserved. The loader rejects duplicate or
out-of-range IRQ numbers.

## Regenerate and verify

From the repository root:

```sh
python neon-mdf/mcu-description/tools/generate.py --device GT32U2086 --profile normal
```

Then configure/build an application to check headers, vectors, layout and image:

```sh
cd examples/Printf_Log
cmake --preset GT32U2086
cmake --build --preset GT32U2086
```

CMake writes outputs under each build directory’s `generated/GT32U2086/`.
The manual command defaults to `build/generated/GT32U2086/normal` below the
current directory; use `--output-dir /path/to/output` to override it.
Use separate build directories for concurrent profiles. See [build system](BuildSystem.md).
