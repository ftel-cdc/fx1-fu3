# I2C_EEPROM

Independent FU2 firmware for I2C0 master communication with an AT24C256/AT24C256C EEPROM.
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
cmake --preset FU2
cmake --build --preset FU2
cmake -E env NEON_MDF_SERIAL_PORT=/dev/ttyUSB0 NEON_MDF_SERIAL_BAUD=19200 \
  NEON_MDF_UART_BYTE_GAP_MS=2 NEON_MDF_UART_LOG=full \
  cmake --build --preset FU2 --target flash_uart_fu2
```

The packaged image is `build/FU2/fu2_i2c_eeprom_image.bin`, including the 128-byte FU2 header.
Use the UART-download/boot straps described in [FU2 build and boot](../../guide/FU2.md).
Open the serial monitor after the loader releases the port, with local echo and flow control off.

## Menu

Press `r` at `fpga:>>`, then choose one key:

| Key | Operation |
|---|---|
| `s` | Scan all 128 7-bit addresses, `00..7F`; display ACK/NACK map |
| `a` | Write `2A 00` at EEPROM offset `0000`, wait 6 ms, read and compare |
| `r` | Read 64-byte blocks over `0000..7FFF`, dump 16 bytes per row, wrap and repeat |
| `t` | Read the original contents, overwrite all 32 KiB with a PRNG pattern, read and verify every byte |
| `q` / Ctrl+C | Stop a long operation after its current block; at the menu, return to `fpga:>>` |

`a` overwrites EEPROM bytes `0000..0001`. **`t` overwrites all 32768 bytes and does not restore
old contents.** The initial dump is for inspection and is not a backup. Stopping `t` leaves the
bytes already written in place. Neither operation runs automatically at boot or menu entry.

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

The full test reads the original contents in 64-byte blocks, then writes 32-byte blocks without
crossing a 64-byte page boundary. A xorshift32 generator provides a new pattern per run. Each
write is followed by a 6 ms delay. Verification restarts the generator with the same seed and
compares all 32768 bytes; mismatches report the EEPROM address, expected byte and received byte.
The result includes written/checked byte counts and the mismatch count.

The continuous read stops on an I2C error and returns to the menu. Transport errors report the
HAL status and the I2C error bitmask. A long operation checks `q`/Ctrl+C between blocks; other
received characters are ignored there. UART hex output makes full-device dumps take time.

## Configuration and implementation

Open `cmake --build --preset FU2 --target menuconfig`, save, then configure/build again.
`CONFIG_FU2_EXAMPLE_FIXED_CLOCK_HZ` must match the actual FPGA clock. Set the EEPROM address
with `CONFIG_I2C_EEPROM_ADDRESS`; `CONFIG_I2C_EEPROM_TIMEOUT_POLLS` is an iteration budget,
not milliseconds. The driver default calculates standard-mode 100 kHz timing from the
current FU2 clock; the example does not duplicate the timing calculation.

The example calls NEON I2C APIs for all bus transfers. The driver configures clock/reset and
SDA/SCL pin routing. EEPROM addressed reads use a two-byte pointer write with STOP, followed
by a read with another START/STOP. The last read byte is NACKed by the master. It uses polling;
no I2C ISR or DMA is needed.

If error bit `NEON_I2C_ERROR_RECOVERY_REQUIRED` (`0x100`) is set, STOP/bus cleanup
did not finish. Further transfers are blocked. Fix the wiring/device holding the bus,
press `q` to close the menu, then `r` to configure I2C0 again.

Application byte values are stored in `uint32_t` elements. Small aligned buffers pack/unpack
these values at the HAL byte-buffer boundary, respecting the FU2 word-only RAM contract.
SysTick runs from the core clock without interrupts for the EEPROM write delay and PRNG seed.
The example requires SysTick to be unused at entry and restores its disabled configuration on exit.

Build/software verification does not establish physical ACKs, clock timing or pin routing.
Run the image with the connected EEPROM to check these on the FPGA.
