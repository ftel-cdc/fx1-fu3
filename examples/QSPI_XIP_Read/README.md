# QSPI_XIP_Read

Standalone FU2 firmware that programs a known NOR pattern using indirect Quad commands,
then verifies it through the **memory-mapped AHB window**. The application itself executes
from internal Flash. Press `r` at the `fpga:>>` terminal to open its menu.

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
cd examples/QSPI_XIP_Read
cmake --preset FU2
cmake --build --preset FU2
cmake -E env NEON_MDF_SERIAL_PORT=/dev/ttyUSB0 NEON_MDF_SERIAL_BAUD=19200 \
  NEON_MDF_UART_BYTE_GAP_MS=2 NEON_MDF_UART_LOG=full \
  cmake --build --preset FU2 --target flash_uart_fu2
```

Load `build/FU2/fu2_qspi_xip_read_image.bin`, which includes the 128-byte FU2 header.
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
- `CONFIG_QSPI_XIP_READ_TEST_OFFSET`: reserved NOR sector, default `0x3C000`.
- `CONFIG_QSPI_XIP_READ_DIVIDER`: encoding 0..7 for `/2,/4,...,/256`, default 4 (`/32`).
  Test SCK is `SystemCoreClock / 2^(divider + 1)`; at 10 MHz, `/32` is 312500 Hz.
- `CONFIG_QSPI_XIP_READ_MODE_0` / `_MODE_3`: SDR clock mode.
- `CONFIG_QSPI_XIP_READ_TIMEOUT_POLLS`: QSPI FIFO/completion/abort polling budget.
- `CONFIG_QSPI_XIP_READ_READY_POLLS`: maximum status/WREN commands per NOR wait.
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
| `0..4` | Erase/program 128/256/512/1024/2048 xorshift32 bytes, then verify by mapped reads |
| `5..9` | Same sizes with fixed patterns |
| `w` | Repeat the selected write test |
| `r` | Capture and verify the last pattern once through XIP |
| `t` | Repeat fresh XIP captures, compare every byte and report CRC/good/bad counts |
| `a` | Run all ten write/verify levels |
| `+` | Increase SCK by decreasing divider encoding, down to `/2` |
| `-` | Decrease SCK by increasing divider encoding, up to `/256` |
| `h` / `?` | Help |
| `q` / Ctrl+C | Cancel between complete operations; at the menu return to `fpga:>>` |

`CONFIG_QSPI_XIP_READ_TRIES` sets the number of captures for `t`, default **100**.
Clock changes preserve the original expected buffer. SCK logs are calculated from the
configured input clock/divider, rather than measured. The initial divider returns to the
menuconfig value on a new run. Respect the NOR and board clock limits.

Example sequence:

```text
r   open the example from fpga:>>
9   program/verify 2048 fixed-pattern bytes
t   read and compare the same region 100 times
+   increase SCK
r   one mapped read with the same expected pattern
t   repeat at the new SCK
-   decrease SCK
q   return to fpga:>>
```

Use `0..9` or `w` before `r`/`t` in each session. Expected data is stored in RAM and is
lost on reset or a new run. A read test does not use its first capture as the reference:
every capture is compared against the **original programmed buffer**.

## Test flow and results

Erase verification and page programming use indirect QSPI transfers with the same WEL,
WIP, page-boundary and protection checks as the read/write example. Quad Enable is
checked/enabled before the menu opens. The array is erased/programmed only on a write command.

Each mapped capture does the following:

1. Configure `EBh` **1-4-4 SDR**, 24-bit address, one alternate byte `FFh`, four dummy
   SCK periods, instruction sent on every frame.
2. Read aligned words from `QSPI_XIP_BASE + TEST_OFFSET`; the default address is
   **`0x3003C000`**. The FU2 mapping window is 256 KiB, independently of NOR capacity.
3. Abort the mapped transfer to release CS, flush prefetch and return to indirect mode.
   The next capture creates a fresh mapping. Printing/verification uses the captured RAM.
4. Compare every selected byte with the original expected pattern. CRC-32/ISO-HDLC is
   printed as a summary; exact byte comparison determines PASS/FAIL.

```text
[XIP] bytes=2048 CRC=... expected=... mismatches=0 [PASS]
[REPEAT] good=100 bad=0 completed=100/100 [PASS]
```

The repeated test logs the first/last capture and at most eight failed-capture summaries,
plus up to eight byte differences from the first bad capture. All captures and all bytes
are still checked. Cancellation, mismatches or a transport error prevent an overall PASS.
The simple write/verify result dumps at most 64 sample bytes.

This tests the NOR **data-read mapping** and does not jump to a NOR application or
validate execution/boot from NOR. The normal internal-Flash build profile is required.
