# UART_DMA

UART0 DMA TX/RX menu.

This is an independent GT32U2086 firmware. Its UART terminal waits at boot; press `r` without Enter
to start the example at the `fpga:>>` prompt.
Console: UART0, 19200 baud, 8N1. FPGA system clock: 10 MHz by default.

Source files are in `Src/`, with the terminal entry in `Src/main.c`. Example headers are in `Inc/`.

## Build and load

```sh
cd examples/UART_DMA
cmake --preset GT32U2086
cmake --build --preset GT32U2086
cmake -E env NEON_MDF_SERIAL_PORT=/dev/ttyUSB0 NEON_MDF_SERIAL_BAUD=19200 \
  NEON_MDF_UART_BYTE_GAP_MS=2 NEON_MDF_UART_LOG=full \
  cmake --build --preset GT32U2086 --target flash_uart_gt32u2086
```

Load `build/GT32U2086/gt32u2086_uart_dma_image.bin` when selecting an image manually. This file includes the GT32U2086
128-byte image header; `gt32u2086_uart_dma.bin` is the raw payload. Open a serial monitor after the loader
releases the port. Use the board's UART-download/normal-boot straps as described in
[GT32U2086 build and boot](../../guide/GT32U2086.md).

## Run

Press t to send 64 characters. Press r, wait for READY, then paste exactly 0123456789ABCDEF without Enter. q or Ctrl+C stops when a transfer is not active.

When the run function completes or its stop key is pressed, the application returns to its
UART terminal. Press `r` again to repeat the example. While the run function is active, its own
menu/data keys apply; `r` only starts a run when the application is at `fpga:>>`.
At this prompt, Enter starts a new line and prints `fpga:>>` again. CR, LF and CRLF are
supported; CRLF produces only one new prompt.
Configure with `cmake --build --preset GT32U2086 --target menuconfig`, then build again.
`CONFIG_GT32U2086_EXAMPLE_FIXED_CLOCK_HZ` must match the actual FPGA clock. Timeout values ending in
`_POLLS` are polling budgets, not milliseconds.

## DMA configuration

Uses DMA0 channel 0, UART0 request slot 2, FIFO mode and separate TX/RX transfers. The main loop
polls completion and counts iterations. During RX, q/Ctrl+C are payload until all 16 characters
arrive or the poll budget expires. Compare TX output on the PC; RX compares with the fixed pattern.

The current implementation retains one character per `uint32_t` and a four-byte peripheral access.
This does not yet match the byte-oriented USART DMA profile in the DMA hardware specification;
hardware correctness and timeout recovery remain to be completed. Build success is not a hardware
validation of this profile.
