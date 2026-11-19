# I2C_EEPROM

Independent GT32U2086 firmware for I2C0 master communication with an AT24C256/AT24C256C EEPROM.
Boot opens the UART terminal at `fpga:>>`. Press `r` without Enter to open the I2C menu.
Console: UART0, 19200 baud, 8N1. Default FPGA clock: 10 MHz; target SCL: 100 kHz.

## Wiring

| FPGA signal | EEPROM signal |
|---|---|
| PMOD_IO6 / package pin 18 | SDA, pin 5 |
| PMOD_IO2 / package pin 19 | SCL, pin 6 |
| Board GND | GND, pin 4 |
| Board-compatible supply | VCC, pin 8 |

The bitstream must route PMOD_IO6/IO2 to I2C0. SDA/SCL need external pull-ups to the IO supply.
A0/A1/A2 all HIGH select the default address `0x57`; these pins select addresses `0x50..0x57`. Keep WP LOW
for writes. This example assumes a 32 KiB AT24C256 with a 16-bit memory address and 64-byte pages.

## Build and load

```sh
cd examples/I2C_EEPROM
cmake --preset GT32U2086
cmake --build --preset GT32U2086
cmake -E env NEON_MDF_SERIAL_PORT=/dev/ttyUSB0 NEON_MDF_SERIAL_BAUD=19200 \
  NEON_MDF_UART_BYTE_GAP_MS=2 NEON_MDF_UART_LOG=full \
  cmake --build --preset GT32U2086 --target flash_uart_gt32u2086
```

The packaged image is `build/GT32U2086/gt32u2086_i2c_eeprom_image.bin`, including the 128-byte GT32U2086 header.
Use the UART-download/boot straps described in [GT32U2086 build and boot](../../guide/GT32U2086.md).
Open the serial monitor after the loader releases the port, with local echo and flow control off.

## Menu

Press `r` at `fpga:>>`, then choose one key:

| Key | Operation |
|---|---|
| `s` | Scan all 128 7-bit addresses, `00..7F`; display ACK/NACK map |
| `a` | Write `2A 00` at EEPROM offset `0000`, wait 6 ms, read and compare |
| `r` | Read `0000..00FF` once in four 64-byte blocks, dump 16 bytes per row, return to the menu |
| `t` | Overwrite and verify the first 1 KiB (`0000..03FF`) with a PRNG pattern |
| `q` / Ctrl+C | Stop a long operation after its current block; at the menu, return to `fpga:>>` |

`a` overwrites EEPROM bytes `0000..0001`. **`t` overwrites bytes `0000..03FF` and does not restore
old contents.** Stopping `t` leaves the bytes already written in place. Neither operation runs
automatically at boot or menu entry. The EEPROM capacity remains 32 KiB; these quick tests use
only its beginning.

Scan uses one-byte reads and includes reserved addresses. A scan can advance a device's pointer
or trigger read side effects. ACK identifies an address response and does not identify EEPROM
model/capacity. Read/write tests always use `CONFIG_I2C_EEPROM_ADDRESS`, default `0x57`.
The configured address is not changed by scanning.

A successful two-byte test prints:

```text
[WRITE] mem=0000 data=2A 00; wait 6 ms, then read.
[READ] 2A 00 [PASS]
[MENU] s/a/r/t or q to exit.
```

The write/read test performs 32 writes of 32 bytes without crossing a 64-byte page boundary.
A xorshift32 generator provides a new pattern per run. Each write is followed by a 6 ms delay.
Verification restarts the generator with the same seed and compares all 1024 bytes in sixteen
64-byte reads. It prints the first 64 bytes as a sample and reports the address, expected byte
and received byte for at most eight mismatches. Further mismatches are still counted. The
result includes written/checked byte counts and the total mismatch count; cancellation or a
transport error cannot produce a successful result.

The read command returns to the menu after 256 bytes, cancellation or an I2C error. Transport
errors report the HAL status and the I2C error bitmask. Both tests check `q`/Ctrl+C between
blocks; other received characters are ignored there. The smaller test ranges and limited
UART output keep these examples quick to run.

## Configuration and implementation

Open `cmake --build --preset GT32U2086 --target menuconfig`, save, then configure/build again.
`CONFIG_GT32U2086_EXAMPLE_FIXED_CLOCK_HZ` must match the actual FPGA clock. Set the EEPROM address
with `CONFIG_I2C_EEPROM_ADDRESS`; `CONFIG_I2C_EEPROM_TIMEOUT_POLLS` is an iteration budget,
not milliseconds. The driver default calculates standard-mode 100 kHz timing from the
current GT32U2086 clock; the example does not duplicate the timing calculation.

The example calls NEON I2C APIs for all bus transfers. The driver configures clock/reset and
SDA/SCL pin routing. TX preloads up to 32 bytes into the FIFO before START, then streams
the remaining bytes. EEPROM addressed reads use a two-byte pointer write with STOP, followed
by a read with another START/STOP. The last read byte is NACKed by the master. It uses polling;
no I2C ISR or DMA is needed.

During RX, the driver sends STOP when hardware reports transfer DONE, even if the CPU
has not yet copied every FIFO byte to RAM. STOP preserves unread RX data. The driver
then drains the remaining bytes without keeping SCL LOW in the software-end state.

If error bit `NEON_I2C_ERROR_RECOVERY_REQUIRED` (`0x100`) is set, STOP/bus cleanup
did not finish. Further transfers are blocked. Fix the wiring/device holding the bus,
press `q` to close the menu, then `r` to configure I2C0 again.

Application byte values are stored in `uint32_t` elements. Small aligned buffers pack/unpack
these values at the HAL byte-buffer boundary, respecting the GT32U2086 word-only RAM contract.
SysTick runs from the core clock without interrupts for the EEPROM write delay and PRNG seed.
The example requires SysTick to be unused at entry and restores its disabled configuration on exit.

Build/software verification does not establish physical ACKs, clock timing or pin routing.
Run the image with the connected EEPROM to check these on the FPGA.
