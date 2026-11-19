# Flashing GT32U2086 firmware through UART

GT32U2086 Flash Tool **1.0** uses ROMCODE's USART update service to burn images to Internal Flash. The
tool runs on a computer; OpenOCD is not required for this flash path.

Process for full boot lines located in
[GT32U2086 boot modes and firmware workflow](GT32U2086BootModes.md).

## Preparation

**Linux/macOS:** UART flash and GT32U2086 monitor use `termios`, `select` and `os` in Python standard
library, similar to the original `uart_boot_loader.py` tool. No need to install `pyserial` or
`python3-serial` to run GT32U2086 tool when port is specified. You can go straight to connecting boards
and running flash commands.

**Windows:** tool uses `pyserial` to access COM ports. Install dependencies from the repository root
directory:

```sh
python3 -m pip install -r neon-mdf/tools/requirements.txt
```

Install in the exact Python that CMake uses to run the tool; test it on Windows with the
corresponding interpreter, for example:

```sh
python3 -c "import serial; print(serial.__version__)"
```

`pyserial` is used for the CLI SDK's automatic port detection. When
using GT32U2086 on Linux/macOS, pass the port with `--port`, `-p` or `NEON_MDF_SERIAL_PORT` as the
examples below so that this library is not needed.

Connect the UART flash of the board: TX of the adapter connects the RX of the board, RX connects the
TX and connects the GND together. Use the voltage level suitable for the board. Select boot mode 100
— USART update according to the foot/jumper diagram of the board. Close the other Serial Monitor
program that is holding the port before flashing.

Board needs valid NVR policy, compatible with image Secure/Non-secure. This tool does not perform
provisioning NVR. Signatures and security policies are checked by ROMCODE on the device; CRC
checking on PC does not replace that step.

## Flash through CMake

Run in the application folder, for example `examples/Printf_Log/`:

```sh
cmake --preset GT32U2086
```

Transfer port and baud at target flash run:

```sh
cmake -E env \
  NEON_MDF_SERIAL_PORT=/dev/ttyUSB0 \
  NEON_MDF_SERIAL_BAUD=19200 \
  NEON_MDF_UART_BYTE_GAP_MS=2 \
  NEON_MDF_UART_LOG=full \
  cmake --build --preset GT32U2086 --target flash_uart_gt32u2086
```

With PowerShell or CMD, write the same command on one line and replace the port with `COM3`, for
example. `cmake -E env` passes environment variables to build and tool processes, does not store
port/baud in CMake cache. Changing these values requires no re-configuration. `cmake --build` does
not directly accept `--port`/`--baud` options.

Target builds the application itself and gets `<app>_image.bin` correctly. Do not select a `.elf` or
`.bin` file that contains only payloads. Target `flash_uart_gt32u2086` points to the first packaged
application; if the project has multiple images, select a specific `<app>_flash_uart_gt32u2086`. Projects
using `neon_mdf_add_executable()` or `neon_mdf_add_gt32u2086_image()` are registered with these targets
automatically.

The tool displays the banner including version, required boot mode, image profile, application
version, address image, port, baud and reset method. When it appears:

```text
[WAIT] Select boot mode 100 - USART update, then press RESET on the GT32U2086 board.
[WAIT] UART is ready. Press Ctrl+C to cancel.
```

Press RESET on GT32U2086 board. At this point the UART port is open and the tool is ready to reply to
ROMCODE. By default the tool waits for the first Reset until a request is received or the user
presses Ctrl+C. After transmission starts, each request has a 300 second timeout, which indicates
about how long ROMCODE pauses communication for preparing/erase Flash.

When finished:

```text
[PASS] Image transfer completed in ... seconds; total runtime ... seconds.
[INFO] ROMCODE verified and committed the image; application handoff follows.
```

The tool indicates success when ROMCODE has requested enough images and returns `0xFFFFFFFF`.
ROMCODE then switches to the application; no reset is required again to start this session.
Re-select boot mode 000 — Internal Flash before subsequent resets/powerups to run the installed
firmware.

Baud flash must match the actual ROMCODE on board. `19200` is an example; parameter indicates
PC-side UART configuration, does not change baud of ROMCODE. Transmission frame is
**8 data bits, no parity, 1 stop bit (8N1)**, no flow control.

## Application monitoring

Baud of application independent of baud flash:

```sh
cmake -E env \
  NEON_MDF_SERIAL_PORT=/dev/ttyUSB0 \
  NEON_MDF_MONITOR_BAUD=19200 \
  cmake --build --preset GT32U2086 --target serial_monitor
```

Default GT32U2086 monitor uses 19200 baud; press Ctrl+C to quit. Log `[WAIT]`, `[TX]`, `[PASS]` belong to
PC tool. The content after the application runs depends on the application itself; there is no
mandatory boot log for all firmwares. Opening the monitor after flash may miss the first log of the
application.

## Using live tool and SDK CLI

From root repository:

```sh
python3 neon-mdf/tools/gt32u2086_flash.py --version
python3 neon-mdf/tools/gt32u2086_flash.py --list-boot-modes
python3 neon-mdf/tools/gt32u2086_flash.py --image /path/to/my_app_image.bin --check-only
python3 neon-mdf/tools/gt32u2086_flash.py \
  --port /dev/ttyUSB0 --baud 19200 \
  --image /path/to/my_app_image.bin \
  --monitor --monitor-baud 19200
```

The tool receives `.bin` or `.hex`/`.mem` tokens as byte/word; this is not an Intel HEX parser. Use
`--hex-format byte|word` if you need to specify the interpretation. `--byte-gap-ms` defaults to 2 ms
for reply bytes, just like the original `uart_boot_loader.py` tool. Log shows each request and
response by default:

```text
[RX #1] 00 00 00 00  request offset=0x00000000
[TX] 55 54 50 46  image[0x00000000:0x00000004]  0.0%
[RX #2] 04 00 00 00  request offset=0x00000004
[TX] ...  image[0x00000004:0x00000008]  ...%
```

The tool returns all 4 bytes in offset, write log with separate stream after response so that the
terminal display does not block the transmission loop. Do not check/cancel TX between bytes of the
same word. If ROMCODE requests to re-offset, the tool sends back the correct word. You can choose
`--log-mode progress` or `--quiet` to view only percentages; `--verbose` is equivalent to
`--log-mode full`.

`--timeout` replaces the requested timeout; `--wait-timeout` limits the first Reset timeout. By
default, when the flash is finished, exit, only open the monitor if there is `--monitor`.

### Adjustable transmission speed

Parameters read at runtime, no need to configure again:

| Environment variable | Optional live tool | Default value |
|---|---|---|
| `NEON_MDF_UART_BYTE_GAP_MS` | `--byte-gap-ms` | `2` ms |
| `NEON_MDF_UART_LOG` | `--log-mode` | `full` (each RX/TX); or `progress` |

Each 4 byte word has 3 gaps. With 31.476 bytes and 2 ms gap image, the rest time on the PC combined
was about 47 seconds, not counting UART transmission, USB and Flash operation of ROMCODE. This is
also how to stretch bytes of the original tool.

To try to reduce downtime, it is possible to pass `NEON_MDF_UART_BYTE_GAP_MS=0` in CMake command, or
`--byte-gap-ms 0` when calling the tool directly. When the gap is zero, the tool sends the whole
word with one write. The appropriate value depends on FIFO/clock and the actual board; if a retry or
CRC appears that does not match, return to the 2 ms gap in use stably. Flash speed also depends on
the time of writing Flash and ROMCODE requires each word, so it is necessary to measure the
completion time on the board.

From the application directory, after having set `NEON_MDF_PATH`:

```sh
python3 "$NEON_MDF_PATH/tools/neon_mdf.py" \
  -t GT32U2086 -p /dev/ttyUSB0 -b 19200 flash-uart-gt32u2086
```

CLI calls the same CMake target; can append `monitor` and `--monitor-baud 19200` actions before the
action list.

## Boot modes and XIP images

| Boot mode | ROMCODE Activity | The role of the UART 1.0 tool |
|---|---|---|
| `000` | Check and run image already in Internal Flash | No UART image receiving service |
| `100` | Get image via UART and record Internal Flash | flash image profile `normal` |
| `101` | Read image from external QSPI source and update Internal Flash | Need to prepare QSPI source by corresponding tool |
| `111` | Check and run image in QSPI XIP area | Need flash External Flash by programmer or loader UART–QSPI |

The XIP image has a base `0x30000000`, while the Internal Flash image has a base `0x00020000`. The
tool can check both profiles with `--check-only`, but will refuse to transmit the XIP image using
USART update service before opening UART port. Replacing `--boot-mode` only describes the required
mode, does not control the boot pin and does not add QSPI recording service to ROMCODE.

## Using extension VS Code

1. Select target GT32U2086, transport UART and the serial port of your board.
2. Set `neonMdf.gt32u2086FlashBaudRate` to ROMCODE baud matching; default of setting is
   19200. `neonMdf.baudRate` is the application baud, independent of flash baud; when not set, monitor GT32U2086
   uses 19200.
3. Select NEON-MDF: Flash or Build + Flash + Log. The extension closes the open monitor, passes
   port/baud when CMake is running and displays the tool in terminal.
4. Select boot mode 100, wait for the UART message to appear, and then press RESET.
5. With Build + Flash + Log, the app monitor is opened after a successful flash.

Select the GT32U2086 boot straps and reset manually when the loader is ready.

## Troubleshooting

| Phenomenon | Inspection |
|---|---|
| Missing port/baud | Transmit `NEON_MDF_SERIAL_PORT` and `NEON_MDF_SERIAL_BAUD` in flash command |
| Cannot open port | Port Name, Access and Other Serial Monitor Programs |
| Waiting for Reset but no data | Boot mode 100, connect TX/RX/GND, baud and Reset operation |
| `[RETRY]` | ROMCODE asks for resubmission; if prolonged, check connection, baud and byte spacing |
| Timeout after header | ROMCODE may be preparing Flash or has rejected image; check Flash timeout and configuration |
| Full byte transmission but no `[PASS]` | ROMCODE not confirmed complete; check CRC, signature, NVR policy and connection |
| Image XIP rejected | Use external QSPI Flash path; USART update only write Internal Flash |

ROMCODE does not send NACK/error codes only. Tool will report timeout, not deduce specific security
reasons or report success based on transmission percentage only.
