# QSPI_Flash_Read_Write

Standalone FU2 firmware for indirect NOR erase, page program and byte verification.
The application starts at the `fpga:>>` UART terminal and waits for `r`.

## Hardware and test region

Use a NOR connected to **FU2 QSPI CS0** with IO0..IO3, SCK, CS#, compatible supply and
common ground. The bitstream must route these signals to the NOR. The automatic driver
route uses the package pins below; verify their connections on the board.

| Signal | FU2 package pin |
|---|---:|
| SCK | 68 |
| CS0 | 72 |
| IO0 | 71 |
| IO1 | 70 |
| IO2 | 69 |
| IO3 | 64 |

Supported devices use the W25Q80DV-compatible command/status protocol: JEDEC
`EF 40 xx` or `C8 40 xx`, density `12h..18h`. Other IDs are rejected before array writes.
The NOR is external to FU2 and can be soldered on the board. Its presence and routing
must be confirmed; a flash used for FPGA configuration may use a different connection.

**Reserve NOR offsets `0x0003C000..0x0003CFFF` exclusively for this example.**
Every write test erases this entire 4096-byte sector, including when testing only
128 bytes, and leaves the test data there. Original data is not backed up or restored.
Initialization and entering the menu do not erase/program the NOR array. Quad mode
can update status registers to enable QE while preserving the other status bits.
Protection/suspend causes write tests to stop; the example does not remove protection.

The offset is configurable in menuconfig. It must be 4 KiB aligned, after sector zero,
and inside the first 256 KiB; reserve the chosen sector outside existing images/data.
`Flash_Read_Write` is a separate example for internal FU2 Flash.

## Build and load

```sh
cd examples/QSPI_Flash_Read_Write
cmake --preset FU2
cmake --build --preset FU2
cmake -E env NEON_MDF_SERIAL_PORT=/dev/ttyUSB0 NEON_MDF_SERIAL_BAUD=19200 \
  NEON_MDF_UART_BYTE_GAP_MS=2 NEON_MDF_UART_LOG=full \
  cmake --build --preset FU2 --target flash_uart_fu2
```

Load `build/FU2/fu2_qspi_flash_read_write_image.bin`, which includes the 128-byte FU2 header.
These apps execute from internal Flash and require the **normal** build profile.
CMake rejects `-DNEON_MDF_PROFILE=xip` because the app changes the QSPI configuration
and erases/programs the NOR. Booting/executing firmware from NOR is a separate test.
See [FU2 build and boot](../../guide/FU2.md) for UART-download and normal-boot straps.

Open UART0 at **19200 baud, 8N1**, local echo and flow control off. At `fpga:>>`, press
`r` without Enter to open the example menu. Enter prints a fresh prompt; CR, LF and
CRLF are supported. After stopping the run, press `r` at the prompt to start again.
No test executes automatically at boot. Send each command after the preceding result/menu.

## Configuration

```sh
cmake --build --preset FU2 --target menuconfig
cmake --preset FU2
cmake --build --preset FU2
```

- `CONFIG_FU2_EXAMPLE_FIXED_CLOCK_HZ`: actual FPGA clock, default 10 MHz, maximum 60 MHz.
  Setting this value does not change the physical clock.
- `CONFIG_QSPI_FLASH_READ_WRITE_TEST_OFFSET`: reserved NOR sector, default `0x3C000`.
- `CONFIG_QSPI_FLASH_READ_WRITE_DIVIDER`: encoding 0..7 for `/2,/4,...,/256`, default 4 (`/32`).
  Test SCK is `SystemCoreClock / 2^(divider + 1)`; at 10 MHz, `/32` is 312500 Hz.
- `CONFIG_QSPI_FLASH_READ_WRITE_MODE_0` / `_MODE_3`: SDR clock mode.
- `CONFIG_QSPI_FLASH_READ_WRITE_TIMEOUT_POLLS`: QSPI FIFO/completion/abort polling budget.
- `CONFIG_QSPI_FLASH_READ_WRITE_READY_POLLS`: maximum status/WREN commands per NOR wait.
  Allow for the device's maximum erase/program time at the selected clock.

Polling budgets are not milliseconds. Wake/reset uses `/32`; command-only frames
use at least `/32` to make BUSY observable, then restore the selected data clock.
The driver owns clock/reset and pin setup. Application code accesses the NOR through
NEON QSPI APIs; memory-mapped loads are used only in `QSPI_XIP_Read`.

Data buffers contain aligned `uint32_t` words, with the low byte first in serial/address
order. No DMA or peripheral interrupts are used. Abort returns the controller to
indirect mode and flushes FIFOs. A transport error ends the current run; an abort/cleanup
failure blocks future runs until FPGA reset. An incomplete transfer never reports PASS.

A successful build or software simulation does not establish physical NOR presence,
pin routing, signal timing or clock stability. Verify these on the FPGA/NOR hardware.

## Menu

| Key | Operation |
|---|---|
| `i` | Print JEDEC ID, status registers, QE and capacity |
| `s` | Select Single: program `02h` 1-1-1, read `03h` 1-1-1 |
| `d` | Select Dual read: program `02h` 1-1-1, read `3Bh` 1-1-2 with 8 dummy clocks |
| `q` | Select Quad: enable QE if needed, program `32h` 1-1-4, read `6Bh` 1-1-4 with 8 dummy clocks |
| `0..4` | Generate xorshift32 data; erase, write and verify 128/256/512/1024/2048 bytes |
| `5..9` | Same sizes with fixed patterns |
| `w` | Repeat the selected write test; random tests advance the PRNG state |
| `r` | Read and compare the last successful program; no erase/program |
| `a` | Run all ten levels in the selected mode and print a sweep summary |
| `h` / `?` | Help |
| `x` / Ctrl+C | Cancel between complete operations; at the menu return to `fpga:>>` |

Here **`q` selects Quad**; use **`x` or Ctrl+C to exit**. Default mode is Single.
Dual mode writes Single because the supported flash protocol has no Dual Page Program.
Switching mode preserves the existing test data and expected buffer, so use `r` to
compare the same pattern through Single, Dual and Quad reads. Expected data is kept
in RAM and is invalidated on a new run/reset; use a write test before `r`.

Example sequence, waiting for each result:

```text
r   open the example from fpga:>>
s   Single
0   write/verify 128 random bytes
9   write/verify 2048 fixed-pattern bytes
d   Dual read
r   read the same 2048 bytes and verify
q   Quad
r   read the same 2048 bytes and verify
w   write/verify using Quad
a   all ten levels using Quad
x   return to fpga:>>
```

## Test flow and results

1. Generate the expected buffer in RAM. Random tests start from xorshift32 seed
   `0x6D2B79F5` and advance between tests. Fixed tests repeat zeroes, ones, alternating
   bits and mixed-byte words.
2. Check protection, set/confirm WEL, erase the reserved 4 KiB sector, poll WIP and
   read back **all 4096 bytes** to confirm `FF`.
3. Program pages of at most 256 bytes. Each page has its own WREN and WIP polling;
   no page boundary is crossed.
4. Read the selected byte count through QSPI RX FIFO into RAM and compare every byte.
5. Dump only the first 64 bytes and at most eight detailed mismatches. The summary
   still counts **every** mismatching byte in the selected range.

```text
[VERIFY] bytes=2048 mismatches=0 [PASS]
```

Controller TC means the frame finished; NOR WIP polling confirms erase/program finished.
Cancellation leaves the erased/programmed portion in place and reports an incomplete test.
