# SPI_DMA

SPI0 master DMA TX/RX checks using 32 bytes of W5500 socket 0 TX SRAM.

This is an independent GT32U2086 firmware. Its UART terminal waits at boot; press `r` without Enter
to start the example at the `fpga:>>` prompt.
Console: UART0, 19200 baud, 8N1. FPGA system clock: 10 MHz by default.

Source files are in `Src/`, with the terminal entry in `Src/main.c`. Example headers are in `Inc/`.

## Build and load

```sh
cd examples/SPI_DMA
cmake --preset GT32U2086
cmake --build --preset GT32U2086
cmake -E env NEON_MDF_SERIAL_PORT=/dev/ttyUSB0 NEON_MDF_SERIAL_BAUD=19200 \
  NEON_MDF_UART_BYTE_GAP_MS=2 NEON_MDF_UART_LOG=full \
  cmake --build --preset GT32U2086 --target flash_uart_gt32u2086
```

Load `build/GT32U2086/gt32u2086_spi_dma_image.bin` when selecting an image manually. This file includes the GT32U2086
128-byte image header; `gt32u2086_spi_dma.bin` is the raw payload. Open a serial monitor after the loader
releases the port. Use the board's UART-download/normal-boot straps as described in
[GT32U2086 build and boot](../../guide/GT32U2086.md).

## Run

Connect W5500 to the SPI0 PMOD route used by the FPGA bitstream:

| FPGA pin | W5500 signal |
|---|---|
| PMOD_IO5 | SCK |
| PMOD_IO4 | MOSI |
| PMOD_IO1 | MISO |
| PMOD_IO0 | SCSn / CS |
| GND | GND |

Power the module according to its board requirements and release RSTn. The FPGA bitstream must
route these pins to SPI0. No Ethernet cable or IP configuration is required. Use this example
with socket 0 closed and its TX buffer allocated; the power-on default allocation is sufficient.
The test overwrites bytes 0..31 in socket 0 TX SRAM and leaves the pattern there.

Press `r` at `fpga:>>`. Each run does the following:

1. Configure mode 3, MSB first, hardware SS0, requested SCK 1 MHz and the driver default B2B
   pause of 10 SCK periods. Allow a conservative power-on wait of at least 560 ms.
2. Read `VERSIONR` using polling. Stop unless its low byte is `04`.
3. Write `00 01 ... 1F` using DMA TX, then read by polling and compare all 32 bytes.
4. Fill the receive buffer with `A5`, then read the same SRAM using DMA RX. A separate DMA TX
   channel sends dummy words to generate SCK. Compare all 32 received bytes with the pattern.
5. Release SPI0 and return to the prompt. Press `r` to repeat.

Example successful output, with iteration counts omitted:

```text
[W5500] VERSION=04 [PASS]
[DMA TX] Write 32 bytes to socket 0 TX SRAM: 00 01 ... 1F.
[DMA TX] Polling readback: 32/32 matched [PASS]
[DMA RX] Read 32 bytes; DMA TX sends dummy words to generate SCK.
[DMA RX] Compare: 32/32 matched [PASS]
[SPI DMA] TX and RX [PASS]
fpga:>>
```

Data mismatches report the first bad byte, its expected value and its received value. A failed
VERSION read stops before SRAM writes or DMA payload transfers.

## Transfer flow

DMA0 channel 0 handles RAM-to-SPI0 TX; channel 1 handles SPI0 RX-to-RAM. Both use peripheral
request slot 1. SINGLE requests service one 32-bit FIFO word (four serial bytes), so each
32-byte payload occupies eight aligned `uint32_t` elements. RAM addresses increment; SPI
data-register addresses stay fixed. Peripheral burst ceilings are four bytes.

The CPU sends the three-byte W5500 VDM header with a 24-bit polling transfer. Payload transfers
use 32-bit words. CS stays LOW across header and payload. Receive capture is off for writes;
RX DMA is armed before TX for reads. The CPU polls completion and increments a work counter,
without moving payload words through the SPI data registers during DMA.

Completion requires DMAC done/idle with no outstanding bus transactions, plus the SPI DMA
completion flags. `neon_spi_wait_tx_complete()` also waits for TX FIFO empty and a conservative
final-frame guard before CS is released. A second guard allows CS release timing to finish.

SPI/DMA errors stop the current run. If a DMAC command has not retired, the example disables
SPI DMA requests, parks its two channels and retains the static buffers and SPI configuration.
It refuses another run until the FPGA is reset. Parking is not an acknowledged abort, so the
example does not reset DMA0 or reuse buffers after an incomplete command.

When the run function completes or its stop key is pressed, the application returns to its
UART terminal. Press `r` again to repeat the example. While the run function is active, its own
menu/data keys apply; `r` only starts a run when the application is at `fpga:>>`.
At this prompt, Enter starts a new line and prints `fpga:>>` again. CR, LF and CRLF are
supported; CRLF produces only one new prompt.
Configure with `cmake --build --preset GT32U2086 --target menuconfig`, then build again.
`CONFIG_GT32U2086_EXAMPLE_FIXED_CLOCK_HZ` must match the actual FPGA clock. Timeout values ending in
`_POLLS` are polling budgets, not milliseconds.

`CONFIG_SPI_DMA_SCK_HZ` selects the requested SCK; the divider rounds the actual frequency down.
`CONFIG_SPI_DMA_TIMEOUT_POLLS` bounds SPI/DMA waits. B2B uses the SPI driver default.
A successful build or software simulation does not verify the physical DMA request routing,
MISO signal, CS timing or peripheral behavior; test the image on the FPGA and W5500.
