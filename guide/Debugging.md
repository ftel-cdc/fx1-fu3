# Testing and debugging GT32U2086

The current workflow is **build → program through UART → inspect UART output and
hardware measurements**. `neon-mdf/cmake/openocd/GT32U2086_openocd.cfg` remains a
placeholder; the SDK does not provide a complete JTAG/OpenOCD configuration for
programming or debugging GT32U2086.

## Validate before running

```sh
cmake --preset GT32U2086
cmake --build --preset GT32U2086
python ../../neon-mdf/tools/gt32u2086_image.py info build/GT32U2086/printf_log_image.bin
```

These commands use `Printf_Log`; substitute the image filename for other
applications. Check the normal image base `0x00020000`, 128-byte header and
application vector base `0x00020080`. Inspect the linker map for incorrect
Flash/RAM placement. `gt32u2086_flash.py --check-only` validates images without
opening a UART port; see [UART programming](GT32U2086Flashing.md).

## Logs and measurements

Open the monitor at the application baud (19200, 8N1 by default). If no logs
appear, check the actual clock, UART mux, TX/RX/GND, baud and
`CONFIG_NEON_MDF_DRIVER_UART=y`. Examples with the `fpga:>>` terminal typically
wait for `r` before running. Use a logic analyzer/oscilloscope to measure GPIO,
UART, SPI, I2C and PWM timing.

BootROM hands off with global interrupts masked. If polling works but ISRs do
not run, check peripheral configuration, NVIC and `__enable_irq()` in bare-metal
applications. See the IRQ relay mechanism in [GT32U2086](GT32U2086.md).

## Preparing JTAG integration

Identify the adapter, JTAG transport, target script, Flash programming algorithm
and board reset behavior before replacing the placeholder and validating a
GDB/OpenOCD session. The extension's Debug command currently reports this
integration status.

BootROM firmware requires a header outside the raw ELF payload. When implementing
a debugger workflow, keep the header and code consistent: verify that the probe's
erase/program operations preserve the complete image. See the
[image format](GT32U2086.md).
