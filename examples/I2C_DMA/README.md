# I2C_DMA

Independent GT32U2086 firmware for I2C0 DMA communication with an AT24C256/AT24C256C EEPROM.

This is an independent GT32U2086 firmware. Its UART terminal waits at boot; press `r` without Enter
to start the example at the `fpga:>>` prompt.
Console: UART0, 19200 baud, 8N1. Default FPGA clock: 10 MHz; target SCL: 100 kHz.

Source files are in `Src/`, with the terminal entry in `Src/main.c`. Example headers are in `Inc/`.

## Wiring

| FPGA signal | EEPROM signal |
|---|---|
| PMOD_IO6 / package pin 18 | SDA, pin 5 |
| PMOD_IO2 / package pin 19 | SCL, pin 6 |
| Board GND | GND, pin 4 |
| Board-compatible supply | VCC, pin 8 |

SDA/SCL require external pull-ups to the IO supply. Keep WP LOW for writes.
A0/A1/A2 all HIGH select the default 7-bit address `0x57`; other combinations select `0x50..0x57`.
Set `CONFIG_I2C_DMA_ADDRESS` to match the device. The bitstream must route these pins to I2C0
and connect I2C0 TX/RX requests to **DMA1, peripheral slot 4**.

## Build and load

```sh
cd examples/I2C_DMA
cmake --preset GT32U2086
cmake --build --preset GT32U2086
cmake -E env NEON_MDF_SERIAL_PORT=/dev/ttyUSB0 NEON_MDF_SERIAL_BAUD=19200 \
  NEON_MDF_UART_BYTE_GAP_MS=2 NEON_MDF_UART_LOG=full \
  cmake --build --preset GT32U2086 --target flash_uart_gt32u2086
```

Load `build/GT32U2086/gt32u2086_i2c_dma_image.bin` when selecting an image manually. This file includes the GT32U2086
128-byte image header; `gt32u2086_i2c_dma.bin` is the raw payload. Open a serial monitor after the loader
releases the port. Use the board's UART-download/normal-boot straps as described in
[GT32U2086 build and boot](../../guide/GT32U2086.md).

## Run

Press `r` at `fpga:>>` to run one test:

1. Send EEPROM offset `00 00`, then write the 16-byte pattern `00 01 ... 0F` using TX DMA.
2. Wait at least 6 ms for the EEPROM write cycle.
3. Set the EEPROM read pointer to `0000` using a polling transfer.
4. Read 16 bytes using RX DMA, print them and compare against the pattern.

**Each run overwrites EEPROM bytes `0000..000F` without restoring their previous contents.**
The write fits within one AT24C256 64-byte page. No EEPROM access occurs before `r` is pressed.

A successful run ends with:

```text
[DMA TX] 16 bytes sent; EEPROM write delay complete.
[DMA RX] 00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F
[VERIFY] 16/16 bytes matched [PASS]
fpga:>>
```

DMA1 channel 0 moves TX data and channel 1 moves RX data in SINGLE mode. Each aligned RAM word
holds one I2C byte in bits `[7:0]`; 16 wire bytes require 64 DMA memory bytes. The two EEPROM
address bytes are queued by the CPU before TX DMA is enabled. The HAL configures the peripheral,
pins and DMA requests; the example programs address/count/direction and START directly because
the HAL has no DMA transaction START API. I2C events are polled without an interrupt handler.
Success requires DMAC completion, peripheral DMA completion, I2C byte-count completion and STOP.

Timeouts and NACKs print the failed stage and I2C status registers. Check the EEPROM address,
pull-ups, WP and bitstream routing. If a DMA command or bus cleanup remains incomplete, the example
retains its buffers and requires an FPGA reset before another run; disabling a DMA channel does
not acknowledge cancellation of outstanding bus accesses.

When the run function completes, the application returns to its UART terminal.
Press `r` again to repeat the example. This is a bounded one-shot test with no inner menu.
At this prompt, Enter starts a new line and prints `fpga:>>` again. CR, LF and CRLF are
supported; CRLF produces only one new prompt.
Configure with `cmake --build --preset GT32U2086 --target menuconfig`, then build again.
`CONFIG_GT32U2086_EXAMPLE_FIXED_CLOCK_HZ` must match the actual FPGA clock for I2C timing and the write
delay to be correct. `CONFIG_I2C_DMA_TIMEOUT_POLLS` is a polling budget, not milliseconds.
Enable both `CONFIG_NEON_MDF_DRIVER_I2C` and `CONFIG_NEON_MDF_DRIVER_DMA`.

Building verifies compilation and image packaging. Physical EEPROM behavior, pin routing and DMA
handshakes still require testing on the FPGA.
