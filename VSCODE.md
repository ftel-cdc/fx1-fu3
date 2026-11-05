# Using neon-mdf with VS Code

The `neon-mdf-tools` extension (`vscode-extension/`) provides build, flash, debug,
monitor and menuconfig commands through the status bar and Command Palette.
Applications do not need separate `.vscode/tasks.json` or `launch.json` files.

## 0. Prerequisites

| Tool | Purpose |
|---|---|
| `ms-vscode.cmake-tools` | Configure and build using `CMakePresets.json` |
| `ms-vscode.cpptools` | C/C++ IntelliSense |
| `marus25.cortex-debug` | Cortex-M debugging through OpenOCD |
| `neon-mdf-tools` (`vscode-extension/`) | Build, flash, debug, monitor, menuconfig and app creation. Build a `.vsix` with `npm run package && npm run vsix` and install it |
| `arm-none-eabi-gcc` | FU2/WEACT_F412 compiler toolchain |
| `openocd` | Flash/debug through supported debug probes; FU2 UART flashing does not require it |
| `riscv64-elf-gcc` (Homebrew) | CH32V003F4P6 compiler toolchain |
| CMake ≥ 3.25, Ninja, Python 3, `pyyaml`, `kconfiglib` | Build system, code generation and menuconfig |
| `pyserial` | STM32 UART tooling and FU2 flashing on Windows; optional for FU2 on Linux/macOS with an explicit port |

## 1. Open an application

Open an application folder such as `examples/Blinky/`. Each application's
`.vscode/settings.json` supplies `cmake.environment`, so sourcing `export.sh`
is optional for this workflow. The extension also recognizes the repository
root, but opening an individual app gives CMake Tools its project directly.

## 2. Select a target

Use **NEON-MDF: <target>** in the status bar or **NEON-MDF: Select Target** in
the Command Palette. The extension reads the available targets from the open
app's `CMakePresets.json`, then asks CMake Tools to reconfigure.

## 3. Build and configure

**NEON-MDF: Build** and the status-bar Build button run
`cmake --build --preset <target>`. Depending on the target and `flashmap.csv`,
an app builds one to three executables: bootloader, factory app and OTA app.

For FU2 UART/I2C driver options, **NEON-MDF: menuconfig** opens the configuration
UI in a terminal. Save the app's `sdkconfig`, then reconfigure CMake to apply it.

## 4. Flash

Use **NEON-MDF: Select Transport (UART/JTAG)** to select the interface, then
**NEON-MDF: Flash**.

For FU2, select UART. The extension calls `flash_uart_fu2`, passing the selected
port and `neonMdf.fu2FlashBaudRate` (default 19200) at runtime. Select boot mode
100, wait until the terminal reports that UART is ready, then press **RESET**
on the board. The tool reports version 1.0 and waits for ROMCODE to confirm
completion. **NEON-MDF: Build + Flash + Log** opens the monitor after success.
The monitor uses `neonMdf.baudRate`, independently of the flashing baud rate;
FU2 defaults to 19200 when unset. See [FU2 flashing](guide/FU2Flashing.md).
The FU2 OpenOCD configuration is currently a placeholder.

For STM32 boards with a supported configuration:

- **JTAG** (default) writes bootloader and app in one OpenOCD/SWD session.
  This is supported on WEACT_F412.
- **UART** uses the chip's ROM System Memory Bootloader (AN3155). SWD is used
  only to trigger reset. On WEACT_F412, connect `PB2` to GND and hold the `B0`
  button during reset. PB2 is shared by the LED and BOOT1 on this board; make
  the required connection manually.

CH32V003F4P6 flashing is not integrated into the extension. It requires `wlink`
rather than the ARM OpenOCD/SWD workflow; see the limitations in `README.md`.

**NEON-MDF: Build + Flash + Log** runs build, flash and Serial Monitor in order,
using the selected transport.

## 5. Debug

**NEON-MDF: Debug** uses the board's OpenOCD configuration. FU2 currently has no
working configuration. For WEACT_F412, the extension first flashes through the
correct CMake target (`flash_jtag` or `flash_ota`), stops at `main()` and displays
peripheral registers using the generated
`mcu-description/generated/<TARGET>.svd`. See [debugging details](guide/Debugging.md).

CH32V003F4P6 debugging is not integrated. It needs a verified RISC-V debug setup,
such as WCH's OpenOCD fork. See the Phase 5 notes in
`project_riscv_ch32v003_plan` if those internal notes are available.

## 6. Serial Monitor

Use **NEON-MDF: Select Serial Port** to select a port and
**NEON-MDF: Serial Monitor** to open it. The extension can also detect ports
through the bootloader's SYNC/READY protocol.

## 7. Create an app

Use **NEON-MDF: Create New App...**:

1. Enter the app name, used for its folder and CMake target.
2. Select a bare-metal or FreeRTOS app. The templates are in
   `neon-mdf/tools/templates/` and `neon-mdf/tools/templates_freertos/`.
   Both are minimal proof-of-life apps without a GPIO/LED demo; the FreeRTOS
   template includes a scheduler.
3. Select a destination. The default is `examples/`, whether you opened the
   repository root or an existing app. You can also browse to another folder.

The generated app supports all three targets and includes a bootloader when
the device YAML declares `bootloader:`. To add OTA, supply a `flashmap.csv`;
see `examples/Blinky/flashmap.CH32V003F4P6.csv` and `examples/OTA_Demo/`.

## 8. Select an SDK outside the default location

Apps normally infer the SDK path as `<app>/../../neon-mdf`, which works for apps
under `examples/`. If the SDK is elsewhere, use **NEON-MDF: Select SDK Path...**.
This sets the global `neonMdf.sdkPath`, similar to `idf.espIdfPath`.

## IntelliSense

CMake Tools generates `compile_commands.json` for `cpptools`; manual include
paths are unnecessary. A missing generated header such as `mcu_device.h` in a
newly opened project usually means configuration has not run yet. Configure or
build the app once to generate it.

## Troubleshooting

| Symptom | Cause | Fix |
|---|---|---|
| No targets found in `CMakePresets.json` | Workspace is not a recognized app or repository root | Open `examples/<App>/` or the repository root |
| `core_cm0.h`/`mcu_device.h` not found | Generated files are unavailable | Configure/build the app once |
| Compiler command not found | Toolchain missing or outside `PATH` | Install the prerequisites above |
| `ModuleNotFoundError: No module named 'yaml'` | Missing `pyyaml` | Run `pip3 install pyyaml kconfiglib` |
| Flash/debug connection failure on CH32V003F4P6 | Workflow is not integrated | Use the target-specific tools described in `README.md` |
| Extension still behaves as before after editing `src/` | `dist/extension.js` and the installed `.vsix` need updating | Run `npm run package && npm run vsix`, reinstall the `.vsix`, then Reload Window |

## Further reading

- `README.md`: SDK overview, targets, bootloader, OTA and FreeRTOS.
- `ARCHITECTURE.md`: code generation and build-system design.
