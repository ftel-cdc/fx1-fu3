# GT32U2086 development in VS Code

The NEON-MDF extension provides building, UART programming, serial monitoring,
menuconfig and project creation. It invokes the same CMake presets/targets as
the terminal; complete [Setup](Setup.md) first.

## Build and install the extension

If a `.vsix` package built for the current version is available, install it
through **Install from VSIX...**. To build from source, install Node.js with
`npm`, then run from the repository root:

```sh
cd vscode-extension
npm ci
npm run typecheck
npm run compile
npm run vsix
```

The last command creates `neon-mdf-tools-0.1.0.vsix`. In VS Code, open Extensions →
the `…` menu → **Install from VSIX...** and select the generated file. Press `F5`
from the extension project to test in an Extension Development Host. `.vsix`
packages and `dist/` are generated artifacts; rebuild them after extension changes.

C/C++ and CMake Tools provide IntelliSense and standard CMake editor support.
Node.js is only needed to develop/package the extension; the installed extension
runs within VS Code.

## Open an application

Open the **application directory**, such as `examples/GPIO_Input_Output`, using
**File → Open Folder**. The status bar displays GT32U2086, UART and the serial port.

Examples locate the SDK at `../../neon-mdf`. For projects outside the repository,
run **NEON-MDF: Select SDK Path...** and choose the `neon-mdf/` directory containing
`tools/cmake/project.cmake`. Ensure VS Code terminals/tasks can access the same
toolchain and Python dependencies. You can launch `code` from a terminal with
`.venv` activated; if needed, configure first with `-DPython3_EXECUTABLE=...` as
shown in [Setup](Setup.md).

## Build, program and view logs

1. Select target **GT32U2086** and the board's serial port.
2. Run **NEON-MDF: Build** to configure/build.
3. Set `neonMdf.gt32u2086FlashBaudRate` to match ROMCODE (19200 by default).
4. Run **NEON-MDF: Flash** or **NEON-MDF: Build + Flash + Log**.
5. Select boot mode `100`, wait for the tool to report UART ready, then press RESET.

The extension closes its monitor before programming to release the port.
Build + Flash + Log reopens the monitor after successful programming. If the
application prints logs before the monitor opens, reset in boot mode `000` to
view them again.

| Setting | Value / meaning |
|---|---|
| `neonMdf.target` | `GT32U2086` |
| `neonMdf.transport` | `uart` |
| `neonMdf.gt32u2086FlashBaudRate` | ROMCODE programming baud; default `19200` |
| `neonMdf.baudRate` | Application/monitor baud; default `19200` |
| `neonMdf.sdkPath` | SDK path when using a non-default location |

Run **NEON-MDF: Serial Monitor** to open the UART terminal. Only one program
should hold the port at a time. Application baud is independent of ROMCODE baud.

## Configure and create applications

**NEON-MDF: menuconfig** opens driver/application settings. Reconfigure after
saving to apply changes. **NEON-MDF: Create New App...** creates a bare-metal or
FreeRTOS project from a template, replacing the application name and SDK path;
see [FirstApp](FirstApp.md).

## Debugging

**NEON-MDF: Debug** reports the current support status. GT32U2086 does not yet
have complete OpenOCD adapter/target/Flash configuration; see [Debugging](Debugging.md).
Use UART output and board measurements to verify example behavior.
