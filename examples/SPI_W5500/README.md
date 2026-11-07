# SPI_W5500

SPI0 register and SRAM checks with a W5500 module.

This is an independent FU2 firmware. Its UART terminal waits at boot; press `r` without Enter
to start the example at the `fpga:>>` prompt.
Console: UART0, 19200 baud, 8N1. FPGA system clock: 10 MHz by default.

Source files are in `Src/`, with the terminal entry in `Src/main.c`. Example headers are in `Inc/`.

## Build and load

```sh
cd examples/SPI_W5500
cmake --preset FU2
cmake --build --preset FU2
cmake -E env NEON_MDF_SERIAL_PORT=/dev/ttyUSB0 NEON_MDF_SERIAL_BAUD=19200 \
  NEON_MDF_UART_BYTE_GAP_MS=2 NEON_MDF_UART_LOG=full \
  cmake --build --preset FU2 --target flash_uart_fu2
```

Load `build/FU2/fu2_spi_w5500_image.bin` when selecting an image manually. This file includes the FU2
128-byte image header; `fu2_spi_w5500.bin` is the raw payload. Open a serial monitor after the loader
releases the port. Use the board's UART-download/normal-boot straps as described in
[FU2 build and boot](../../guide/FU2.md).

## Run

Press `r` at the terminal to open the single-key menu. LAN configuration and sockets are not used. Ctrl+C or q stops.

When the run function completes or its stop key is pressed, the application returns to its
UART terminal. Press `r` again to repeat the example. While the run function is active, its own
menu/data keys apply; `r` only starts a run when the application is at `fpga:>>`.
Configure with `cmake --build --preset FU2 --target menuconfig`, then build again.
`CONFIG_FU2_EXAMPLE_FIXED_CLOCK_HZ` must match the actual FPGA clock. Timeout values ending in
`_POLLS` are polling budgets, not milliseconds.

## Wiring

| SPI0 signal | PMOD label | FU2 package pin | SYSCSR pad |
|---|---|---|---|
| SCK | IO5 | 60 | right[21] |
| MISO | IO1 | 61 | top[0] |
| MOSI | IO4 | 62 | top[1] |
| CS | IO0 | 63 | top[2] |

Confirm the PMOD routing against the actual FPGA bitstream. Connect common GND and power the
module according to its board requirements. RSTn needs its pull-up/reset circuit. The driver
configures the SPI0 mux/pads automatically; this example does not control RSTn through GPIO.

## Keys

| Key | Operation |
|---|---|
| i | Wait for startup, check VERSION and perform software reset |
| s | Stop auto and attempt software reset through MR, without a VERSION precheck |
| t | Read VERSION, MR, PHYCFGR, RTR, RCR and link state |
| p | Write/read RCR with 55/AA, then restore its previous value |
| r | Read 32 bytes of socket 0 TX SRAM |
| d | Read-only probes: modes 0/3 at 1 MHz and 100 kHz |
| b | Stop auto; B2B sweep in modes 0/3 at exact 1/3/5/10 MHz |
| 0..4 | Mode 0: 100/500/1000/2500/5000 kHz; run ping/pong and enable auto |
| 5..9 | Mode 3 at the same five speeds |
| a | Toggle automatic ping/pong every 3 seconds |
| h / ? | Help |
| q / Ctrl+C | Stop the example |

VERSIONR at 0039 must read 04. A zero MR read alone does not confirm software reset: the reset
command also requires VERSION=04. The default B2B pause is 10 complete SCK periods between
bytes, including bytes inside a 32-bit transfer. At core=10 MHz and SCK=1 MHz this is TREG1=100.
The B2B sweep checks VERSION and CRC32 repeatability of SRAM without writing it. Only exact
achievable SCK rates are tested: at core=10 MHz, 3 and 10 MHz are skipped. Repeatable SRAM data
is not proof of a known memory pattern. Pin/register debug commands and the experimental
pre-RX 1 ms delay are not part of this example.
