# GT32U2086 driver development

Drivers expose public APIs in
`components/driver/<peripheral>/neon_<peripheral>.h` with implementations in
`platform/gt32u2086/neon_<peripheral>.c`. All sources are collected in the
`neon_mdf_driver` CMake target.

## Add or update a module

1. Compare bases, offsets, access permissions and bitfields in `GT32U2086.yaml`
   against hardware documentation. Update the YAML and regenerate when register data changes.
2. Define configuration/handle types and APIs in the public header, following the
   [coding convention](NEON_Coding_Convention_V1.3.md).
3. Implement the GT32U2086 backend: clock/reset, PAD mux, MMIO, errors and timeouts.
4. Add sources to the peripheral's CMake file; optional modules have Kconfig switches.
5. Add or update an application example to verify the changed behavior on hardware.

## CMake and configuration integration

GPIO, Flash, CRM and SYSCSR are always-built infrastructure. Other peripherals
are controlled by `CONFIG_NEON_MDF_DRIVER_*` in `components/driver/Kconfig`.
UART also enables the console. Example conditional source registration:

```cmake
if(CONFIG_NEON_MDF_DRIVER_UART)
  target_sources(neon_mdf_driver PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/platform/gt32u2086/neon_uart.c)
endif()
```

Some peripheral CMakeLists also use
`components/cmake/driver_helpers.cmake` to select the GT32U2086 backend. When
adding a module, register its subdirectory in `components/driver/CMakeLists.txt`
and update `neon_config.h` if the public header needs a module guard. See
[menuconfig](MenuConfig.md).

## Hardware rules

- Use aligned 32-bit word accesses for SRAM; buffers must own every word accessed by the runtime.
- Follow register widths and ordering for MMIO; do not use memory routines on registers.
- Save and restore PRIMASK in critical sections; do not unconditionally enable interrupts on exit.
- `timeout_polls` counts polling iterations, not milliseconds.
- Calculate timing from actual clocks; do not assume maximum CPU frequency.
- Initialize clocks and PAD mux before enabling peripherals; handle W1C/RO/WO status correctly.

## Verification

Build the module's example with appropriate enabled/disabled configurations.
Inspect images/maps after runtime or memory-layout changes. Use a board to verify
IRQs, clocks, FIFOs, DMA and error/timeout behavior; a successful build does not
verify physical timing.

[GPIO_Input_Output](../examples/GPIO_Input_Output/README.md),
[UART_RX_Interrupt](../examples/UART_RX_Interrupt/README.md) and
[SPI_DMA](../examples/SPI_DMA/README.md) provide starting points for driver verification.
