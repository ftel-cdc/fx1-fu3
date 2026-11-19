# Printf_Log — UART logging on GT32U2086

This bare-metal example calls `printf` directly through the SDK's built-in
word-only formatter and `_write`. The application only includes `<stdio.h>`;
it does not need to configure or attach a UART explicitly.
Enable `CONFIG_NEON_MDF_DRIVER_UART=y`; there is no separate printf flag.
The first printf call initializes UART0 automatically.

UART0 defaults to **19200 baud, 8N1**. The FPGA uses a fixed **10 MHz** clock,
following the Test_VP policy. `NEON_PRINTF_FIXED_CLOCK_HZ` applies to both
startup/device code and the application. UART is enabled in `sdkconfig`.
Disabling it excludes both the UART backend and console; an application that
calls printf will then fail to link.

## Build

```sh
cd examples/Printf_Log
cmake --preset GT32U2086
cmake --build --preset GT32U2086
```

Outputs:

- `build/GT32U2086/printf_log`: ELF for the debugger or emulator.
- `build/GT32U2086/printf_log.bin`: raw payload.
- `build/GT32U2086/printf_log_image.bin`: 128-byte ROM v5 header and payload, programmed
  at `0x00020000` with the existing GT32U2086 tools.

Use the complete image for ROM boot. See [GT32U2086.md](../../guide/GT32U2086.md) for the
programming and XIP contract. This example does not add a separate GT32U2086 flashing
tool or bootloader.

Reconfigure for a board with a different clock:

```sh
# Board supplying a fixed 20 MHz clock:
cmake --preset GT32U2086 -DNEON_PRINTF_FIXED_CLOCK_HZ=20000000
# Or use the SDK's normal SystemInit policy:
cmake --preset GT32U2086 -DNEON_PRINTF_FIXED_CLOCK_HZ=0
```

Choose one clock configuration for the board and rebuild. The console defaults
to 19200 baud. The optional AttachUART API described in the guide allows a
different UART or baud rate.

## View logs

Connect UART0 TX to USB-UART RX and share GND. Check routing and voltage against
the board map. Open the extension's Serial Monitor at 19200 baud or run:

```sh
python3 -m serial.tools.miniterm /dev/ttyUSB0 19200
```

Reset the board after opening the monitor. Expected output with the default
configuration:

```text
[INFO] NEON MDF printf ready
[INFO] printf via _write
[DEBUG] signed=-42 hex=0x1234ABCD text=hello
[INFO] counter=0
[INFO] counter=1
...
```

The counter uses a busy loop rather than a precise time interval. The example
stops in `halt()` if printf fails, including automatic UART initialization
failures; inspect the status in the debugger. INFO/DEBUG prefixes are strings
written by the application; there is no log-level filtering system yet.

For the API, supported formats and use in other applications, see
[PrintfLogging.md](../../guide/PrintfLogging.md).
