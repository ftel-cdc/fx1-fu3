# Reading the NEON-MDF source

Use a small application such as `examples/Printf_Log` or
`examples/GPIO_Input_Output` to follow the GT32U2086 firmware flow.

## From application to hardware

1. Read the application's `Src/main.c`: initialization, terminal, main loop and driver calls.
2. Open the public header in `neon-mdf/components/driver/<peripheral>/` for APIs,
   handles/configuration, return codes and polling budgets.
3. Follow `platform/gt32u2086/neon_<peripheral>.c` for mux, clock/reset, MMIO and critical sections.
4. Compare `mcu-description/devices/GT32U2086.yaml` with the generated header for offsets/bitfields.
5. Use hardware documentation and board measurements to verify timing/IRQ behavior.

## From reset to main

`startup_GT32U2086.s` provides vectors and Reset_Handler, initializes `.data`/`.bss`
and calls SystemInit and main. `system_GT32U2086.c` manages clocks and
`SystemCoreClock`. `gt32u2086_runtime.c` provides memory operations suitable for
word-only SRAM. BootROM processes the header and publishes vector information
through the SRAM relay before entering the application.

Generated startup/linker files are in
the application's `build/GT32U2086/generated/GT32U2086/`, with device headers in
its `include/` subdirectory. Source-tree generated files are reference snapshots;
CMake uses the build-local outputs. Edit the YAML/generator to change the output.
See [GT32U2086](GT32U2086.md).

## From build to programming

Read `tools/cmake/project.cmake` → `cmake/mcu_variants.cmake` →
`mcu-description/tools/generate.py` → `cmake/kconfig.cmake` →
`cmake/gt32u2086_image.cmake` → `tools/gt32u2086_image.py` →
`cmake/gt32u2086_flash.cmake` → `tools/gt32u2086_flash.py`/`gt32u2086_uart.py`.

[BuildSystem](BuildSystem.md) describes intermediate artifacts and configuration
variables. The CLI/extension call these same targets, so failures can be
reproduced from a terminal.

## Checks when changing the SDK

- Match APIs/headers and implementations in types, register widths and lifetime rules.
- Keep linker/startup data and stack allocation out of header/relay reservations.
- Align SRAM buffers and ensure ownership of every accessed word.
- Match actual clocks to UART, timer, I2C and RTOS tick configuration.
- Treat polling budgets as iteration counts, not milliseconds.
- Distinguish host build/link verification from behavior requiring a physical board.
