# AES_Known_Vector

AES Known Vector example skeleton.

This is an independent FU2 firmware. It starts immediately after boot; there is no command terminal.
Console: UART0, 19200 baud, 8N1. FPGA system clock: 10 MHz by default.

## Build and load

```sh
cd examples/AES_Known_Vector
cmake --preset FU2
cmake --build --preset FU2
cmake -E env NEON_MDF_SERIAL_PORT=/dev/ttyUSB0 NEON_MDF_SERIAL_BAUD=19200 \
  NEON_MDF_UART_BYTE_GAP_MS=2 NEON_MDF_UART_LOG=full \
  cmake --build --preset FU2 --target flash_uart_fu2
```

Load `build/FU2/fu2_aes_known_vector_image.bin` when selecting an image manually. This file includes the FU2
128-byte image header; `fu2_aes_known_vector.bin` is the raw payload. Open a serial monitor after the loader
releases the port. Use the board's UART-download/normal-boot straps as described in
[FU2 build and boot](../../guide/FU2.md).

## Run

The firmware prints [TODO] and stops. Peripheral behavior has not been implemented.

The example stops after completion or its stop key. Reset the board to run it again.
Configure with `cmake --build --preset FU2 --target menuconfig`, then build again.
`CONFIG_FU2_EXAMPLE_FIXED_CLOCK_HZ` must match the actual FPGA clock. Timeout values ending in
`_POLLS` are polling budgets, not milliseconds.
