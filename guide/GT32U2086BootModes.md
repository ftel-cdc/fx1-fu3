# Boot mode and firmware GT32U2086 running procedure

## Starting principle

After Reset or power, GT32U2086 runs ROMCODE first. ROMCODE reads `SYSCSR_STATUS[2:0]` to select firmware
source, check image and switch to application if conditions are valid. User sets boot mode by
pin/jumper/switch according to board design; PC-side software parameter does not manually see
changing these pins.

The code in the documentation is the logical value of the SoC, not the jumper number or switch order
on a specific board. Use the board diagram to determine how the logic levels are set, keeping the
configuration stable during Reset.

| `STATUS[2:0]` | `MODE[1:0]` | `BOOT` | Modes | Image source | Application Running Location |
|---|---|---|---|---|---|
| `000` | `00` | `0` | Internal Flash | Image flashed in Internal Flash | Internal Flash |
| `100` | `10` | `0` | USART update | PC send image via UART | Internal Flash after update |
| `101` | `10` | `1` | QSPI indirect update | Image from external QSPI source | Internal Flash after copying |
| `111` | `11` | `1` | QSPI XIP | Image Flashed in External QSPI Flash | Direct from External QSPI Flash |

The codes `001`, `010`, `011`, `110` failed to select the application boot path. ROMCODE returned to
Reset processing and reread the status. For consistent operation, set a valid mode and then press
Reset.

## Prepare the right image

The SDK packages the image in the following structure:

```text
[header 128 bytes][vector table of application][code/data of application]
```

| The contents | Profile `normal` | Profile `xip` |
|---|---|---|
| Used for mode | `000`, `100`, `101` | `111` |
| address image viewed from CPU | `0x00020000` | `0x30000000` |
| address vector table | `0x00020080` | `0x30000080` |
| maximum size, including headers | `0x80000` byte (512 KB) | `0x40000` bytes (256 KB XIP window) |
| File used to flash | `<app>_image.bin` | `<app>_image_xip.bin` |

The program's associated address must match the profile and header. Do not change the address in the
header of image `normal` to turn it into an XIP image.

Build default profile from application directory:

```sh
cmake --preset GT32U2086
cmake --build --preset GT32U2086
```

If the project has previously selected XIP, switch back with:

```sh
cmake --preset GT32U2086 --fresh -DNEON_MDF_PROFILE=normal
cmake --build --preset GT32U2086
```

Build image XIP:

```sh
cmake --preset GT32U2086 --fresh -DNEON_MDF_PROFILE=xip
cmake --build --preset GT32U2086
```

Profile is the image parameter affecting linker and code should be selected in the configure step.
Port/baud is a communication parameter when flashing, only transmitted at the target flash run. The
files device/linker created automatically are now shared: configure and build each profile
sequentially, do not build simultaneously normal/XIP in the same SDK.

Check the image before flashing, from the root repository:

```sh
python3 neon-mdf/tools/gt32u2086_flash.py \
  --image /path/to/my_app_image.bin --check-only
```

## Mode 000 — Running from Internal Flash

1. Prepare image profile `normal` and flash to Internal Flash first, for example according to the USART
   update process in the next entry.
2. Select the boot code `000` on your board.
3. Press Reset or power it back on.
4. ROMCODE checks the existing image at `0x00020000`, including header, CRC, vector and requires
   authentication according to NVR policy. If valid, ROMCODE switches to the application.
5. Check the result of application: LED, UART or written peripheral function.

This mode does not wait for the PC to send firmware. Do not use the UART tool to request image
burning while the board is in `000` mode.

## Mode 100 — flash via UART and run from Internal Flash

1. Build application according to `normal` profile. Board must have valid NVR policy, compatible with
   image Secure/Non-secure used.
2. Connect the UART flash correctly according to the board and select boot mode `100`. Close the other
   Serial Monitor holding the port.
3. From the application folder, run:

   ```sh
   cmake -E env \
     NEON_MDF_SERIAL_PORT=/dev/ttyUSB0 \
     NEON_MDF_SERIAL_BAUD=19200 \
     NEON_MDF_UART_BYTE_GAP_MS=2 \
     NEON_MDF_UART_LOG=full \
     cmake --build --preset GT32U2086 --target flash_uart_gt32u2086
   ```

   Replace port and baud by actual board; UART ROMCODE uses 8N1, no flow control. No need to add
   port/baud to configure.
4. When the tool displays `[WAIT] UART is ready`, press RESET on GT32U2086 board.
5. ROMCODE requires each image offset, receive data, prepare Flash, burn payload, test image and then
   write the last header. Keep connection and stable power during flash.
6. Wait for `[PASS] Image transfer completed`. The tool indicates this result after receiving
   `0xFFFFFFFF` from ROMCODE and has transmitted enough image. ROMCODE continues to switch to the
   application right in this session.
7. Switch to `000` mode before the next Reset/power up to run the installed image, avoid returning to
   update pending.

The tool displays the required boot mode; it does not read or directly confirm the boot pin value on
the board. Parameter details and error handling are in
[flash firmware GT32U2086 via UART](GT32U2086Flashing.md).

## Mode 101 — Copy from QSPI to Internal Flash

1. Build image profile **`normal`**, as the final application will run in
   Internal Flash.
2. Prepare image at external QSPI source. With Flash NOR which ROMCODE reads from the beginning of
   device, set all `<app>_image.bin` at `0` offset. Use programmer or loader in accordance with
   board design and re-read to confirm data.
3. Connect the QSPI power to controller/CS by board, select boot mode `101`, and then press Reset.
4. ROMCODE reads the header from QSPI source, checks the `0x00020000` image address, clears the
   necessary Internal Flash area and copies the payload to it.
5. ROMCODE checks the CRC, asks for authentication and vector, writes the final header and then
   switches to running the application in Internal Flash.
6. Check the application operation. Select mode `000` for subsequent startups if you do not want to
   repeat the update step from QSPI.

The `101` mode is not a QSPI Flash write service and does not require an XIP image. The UART 1.0
tool does not yet have a backend for preparing this QSPI source. ROMCODE does not broadcast the
`0xFFFFFFFF` UART completion message to the copy QSPI path.

## Mode 111 — Running directly from QSPI Flash

1. Build image profile `xip`, use file `<app>_image_xip.bin` and check header base `0x30000000`. File
   `<app>_xip.bin` only contains payload raw.
2. Use programmer or firmware loader UART–QSPI to flash the entire image into external Flash, at `0`
   offset with image layout at the top of device. Read again to confirm contents. `0x30000000` is
   the CPU address mapped to external Flash, not the offset that must be passed to the programmer of
   the Flash chip.
3. Connect external Flash according to the board, select boot mode `111`, and then press Reset.
4. ROMCODE initializes QSPI, enables memory-mapped window and checks header, CRC configuration image,
   and application vector in XIP area.
5. If valid, ROMCODE switches to the application at QSPI Flash and keeps the XIP configuration active.
   Check LED/UART or the functionality of the application.
6. Keep `111` mode for next Reset/power-up attempts if XIP continues to run.

In the current ROMCODE, XIP lines do not perform NVR policy and signature checks like Internal lines
Flash/update. It is not inferred that the image has been authenticated with a signature only because
the XIP boot was successful.

The UART 1.0 tool does not yet provide a loader firmware for writing QSPI Flash. ROMCODE's USART
update service always writes Internal Flash and rejects XIP images. External Flash flash via UART
requires both the GT32U2086 side loader and the PC interface; adding the `--boot-mode 111` option does not
create this capability on its own.

## Check the running firmware

For applications with UART logs, open the baud-based monitor that configures the application:

```sh
cmake -E env \
  NEON_MDF_SERIAL_PORT=/dev/ttyUSB0 \
  NEON_MDF_MONITOR_BAUD=19200 \
  cmake --build --preset GT32U2086 --target serial_monitor
```

Application baud may differ from baud flash. The first log may appear before the monitor opens; with
normal boot mode, the monitor can be opened first and then Reset for re-observation. Do not open the
monitor simultaneously with the flashing tool on the same port.

| Phenomenon | Inspection Direction |
|---|---|
| Mode `100` without UART requirement | Boot code, Reset operation, UART flash, baud, wire TX/RX/GND |
| Mode `000` not running | Image already in Internal Flash; header, CRC, vector, NVR policy |
| Mode `101` not running | QSPI source has `normal` image at right offset; connect, image format and NVR policy |
| Mode `111` not running | Image `xip`, offset External Flash, Flash type, configuration QSPI reader and vector |
| Invalid code selected | Select one of `000`, `100`, `101`, `111` and Reset |

ROMCODE does not automatically switch to another boot source when the selected image is invalid and
does not send error code/NACK only. When a boot error occurs, check that the selected boot path is
correct because it waits for an automatic fallback mechanism.
