# RISC-V backend — RESERVED, not yet implemented

This directory reserves space for a code generation backend (headers, linker
scripts and startup code) when the SDK adds RISC-V support. See
`mcu-description/schema/arch/riscv.schema.yaml` and `Core/Arch/RISCV/README.md`.

The planned files mirror `backends/cortex_m/`:

- `gen_riscv_header.py` generates peripheral register structures. Peripheral
  definitions share the common schema; core/CSR definitions differ from Cortex-M.
- `gen_linker_script.py` generates RISC-V linker scripts, with some differences
  in MEMORY/SECTIONS due to the ABI and relocation model.
- `gen_startup.py` generates startup code and trap vectors (PLIC/CLINT rather
  than NVIC; see the notes in `riscv.schema.yaml`).

The parent `generate.py` already dispatches by `device.arch`. Add an
`elif device.arch.startswith("riscv"):` branch pointing to this backend when
it is ready.
