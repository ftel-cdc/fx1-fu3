# ADC_Sampling

ADC batch sampling and range checks.

This is an independent GT32U2086 firmware. Its UART terminal waits at boot; press `r` without Enter
to start the example at the `fpga:>>` prompt.
Console: UART0, 19200 baud, 8N1. FPGA system clock: 10 MHz by default.

Source files are in `Src/`, with the terminal entry in `Src/main.c`. Example headers are in `Inc/`.

## Build and load

```sh
cd examples/ADC_Sampling
cmake --preset GT32U2086
cmake --build --preset GT32U2086
cmake -E env NEON_MDF_SERIAL_PORT=/dev/ttyUSB0 NEON_MDF_SERIAL_BAUD=19200 \
  NEON_MDF_UART_BYTE_GAP_MS=2 NEON_MDF_UART_LOG=full \
  cmake --build --preset GT32U2086 --target flash_uart_gt32u2086
```

Load `build/GT32U2086/gt32u2086_adc_sampling_image.bin` when selecting an image manually. This file includes the GT32U2086
128-byte image header; `gt32u2086_adc_sampling.bin` is the raw payload. Open a serial monitor after the loader
releases the port. Use the board's UART-download/normal-boot straps as described in
[GT32U2086 build and boot](../../guide/GT32U2086.md).

## Run

Press `r` to read batches of 1, 4 and 16 samples from channel 0. Each batch reports
its values, minimum, maximum and average, checks the 12-bit range, then deinitializes
the ADC. Successful completion prints `[ADC][PASS] all sample batches completed.`
Set `ADC_SAMPLING_CHANNEL` for the connected analog input. The CPU extracts sample
slots from aligned `uint32_t` storage; do not dereference a `uint16_t *` in application code.

When the run function completes or its stop key is pressed, the application returns to its
UART terminal. Press `r` again to repeat the example. While the run function is active, its own
menu/data keys apply; `r` only starts a run when the application is at `fpga:>>`.
At this prompt, Enter starts a new line and prints `fpga:>>` again. CR, LF and CRLF are
supported; CRLF produces only one new prompt.
Configure with `cmake --build --preset GT32U2086 --target menuconfig`, then build again; changes to `sdkconfig` are applied automatically.
`CONFIG_GT32U2086_EXAMPLE_FIXED_CLOCK_HZ` must match the actual FPGA clock. Timeout values ending in
`_POLLS` are polling budgets, not milliseconds.
