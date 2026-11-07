# UART_DMA

UART0 DMA TX/RX menu.

This is an independent FU2 firmware. It starts immediately after boot; there is no command terminal.
Console: UART0, 19200 baud, 8N1. FPGA system clock: 10 MHz by default.

## Build and load

```sh
cd examples/UART_DMA
cmake --preset FU2
cmake --build --preset FU2
cmake -E env NEON_MDF_SERIAL_PORT=/dev/ttyUSB0 NEON_MDF_SERIAL_BAUD=19200 \
  NEON_MDF_UART_BYTE_GAP_MS=2 NEON_MDF_UART_LOG=full \
  cmake --build --preset FU2 --target flash_uart_fu2
```

Load `build/FU2/fu2_uart_dma_image.bin` when selecting an image manually. This file includes the FU2
128-byte image header; `fu2_uart_dma.bin` is the raw payload. Open a serial monitor after the loader
releases the port. Use the board's UART-download/normal-boot straps as described in
[FU2 build and boot](../../guide/FU2.md).

## Run

Press t to send 64 characters. Press r, wait for READY, then paste exactly 0123456789ABCDEF without Enter. q or Ctrl+C stops when a transfer is not active.

The example stops after completion or its stop key. Reset the board to run it again.
Configure with `cmake --build --preset FU2 --target menuconfig`, then build again.
`CONFIG_FU2_EXAMPLE_FIXED_CLOCK_HZ` must match the actual FPGA clock. Timeout values ending in
`_POLLS` are polling budgets, not milliseconds.

## DMA configuration

Uses DMA0 channel 0, UART0 request slot 2, FIFO mode and separate TX/RX transfers. The main loop
polls completion and counts iterations. During RX, q/Ctrl+C are payload until all 16 characters
arrive or the poll budget expires. Compare TX output on the PC; RX compares with the fixed pattern.

The current implementation retains one character per `uint32_t` and a four-byte peripheral access.
This does not yet match the byte-oriented USART DMA profile in the DMA hardware specification;
hardware correctness and timeout recovery remain to be completed. Build success is not a hardware
validation of this profile.
