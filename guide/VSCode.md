# GT32U2086 development in VS Code

NEON-MDF Tools provides serial port selection, an interactive UART monitor,
building, UART programming, menuconfig, debugging and project creation. Complete
[Setup](Setup.md) before running build tasks.

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

Open the repository or an application folder such as
`examples/GPIO_Input_Output`. Commands use the application containing the
active editor when possible. If several apps are available and the active
editor is outside them, use the application picker.

For an SDK outside the default location, run **NEON-MDF: Select SDK Path...**.
Use **NEON-MDF: Create New App...** to create a bare-metal or FreeRTOS project.
**Select Target** and **Select Transport** show the supported GT32U2086 preset
and UART transport. VS Code tasks need access to the toolchain and Python environment
described in [Setup](Setup.md).

## Build, program and view logs

1. Use **Select Port** in the status bar to choose the board's UART port.
2. Use **Build** to configure and build the selected application.
3. Use **Flash** to build as needed and program the image selected by CMake.
   Set boot mode `100` and press RESET when the task reports that UART is ready.
4. Use **Monitor** to open the UART terminal. Type directly into the terminal
   to send characters such as `r` and Enter to the firmware.

Flash releases an open monitor session and opens a new one after a successful
upload. **NEON-MDF: Build + Flash + Log** opens the monitor after programming
even if it was closed before the upload. To build and program signed firmware,
configure the application with the options in the [Secure Boot guide](SecureBoot.md).
The extension uses the CMake flash target, which selects the signed artifact
when signing is enabled. To see startup logs again, set boot mode `000` and reset the board with
the monitor open.

| Setting | Purpose |
|---|---|
| `neonMdf.gt32u2086FlashBaudRate` | ROMCODE UART rate, default `19200` |
| `neonMdf.baudRate` | Application UART rate, default `19200` |
| `neonMdf.sdkPath` | Optional SDK path override |

## Menuconfig

**Menuconfig** opens the SDK's terminal configuration UI. Save and exit to
regenerate the build configuration. The next build uses the saved `sdkconfig`.

## Debug

Install [Cortex-Debug](https://github.com/Marus/cortex-debug) and set up OpenOCD
and GDB for your board. The SDK ships an OpenOCD placeholder. Use adapter and
target files verified for your GT32U2086 board.

Build and program the matching firmware first. Add a configuration like this
to `.vscode/launch.json` in the open workspace folder. The executable path
below assumes you opened `examples/GPIO_Input_Output`. For the repository root,
prefix `build/` with `examples/GPIO_Input_Output/`. Adjust the OpenOCD config
path to point to your board configuration.

```json
{
  "version": "0.2.0",
  "configurations": [
    {
      "name": "GT32U2086: Attach",
      "type": "cortex-debug",
      "request": "attach",
      "servertype": "openocd",
      "cwd": "${workspaceFolder}",
      "executable": "${workspaceFolder}/build/GT32U2086/gt32u2086_gpio_input_output",
      "configFiles": ["${workspaceFolder}/board-gt32u2086.cfg"]
    }
  ]
}
```

Choose **Debug** from the NEON-MDF status bar to start the attach configuration.
The extension offers a picker if the folder has multiple Cortex-Debug attach
configurations. See the [Cortex-Debug configuration reference](https://github.com/Marus/cortex-debug/blob/master/debug_attributes.md)
for adapter and GDB settings.
