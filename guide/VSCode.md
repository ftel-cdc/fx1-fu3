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

The last command creates `neon-mdf-tools-<version>.vsix` using the version in
`vscode-extension/package.json`. In VS Code, open Extensions → the `…` menu →
**Install from VSIX...** and select the generated file. Press `F5`
from the extension project to test in an Extension Development Host. `.vsix`
packages and `dist/` are generated artifacts; rebuild them after extension changes.

For Remote SSH, install the VSIX from the connected SSH window so build and
UART operations use the remote host. Verify the version under **SSH: Installed**
in the Extensions view, then run **Developer: Reload Window** if prompted.
See the [VS Code Remote SSH notes](https://code.visualstudio.com/docs/remote/troubleshooting#_need-to-install-local-vsix-on-remote-endpoint).

C/C++ and CMake Tools provide IntelliSense and standard CMake editor support.
Node.js is only needed to develop/package the extension; the installed extension
runs within VS Code.

## Open an application

Open the repository or an application folder such as
`examples/GPIO_Input_Output`. Select the **NEON-MDF** icon in the left Activity
Bar to open **Board Tools**, or run **NEON-MDF: Open Board Tools** from the
Command Palette. If the icon is hidden, right-click the Activity Bar and
enable **NEON-MDF**. The **Project** section shows the selected
application, target and SDK path. Click **Application** to choose an app for
this workspace. Until you choose one, commands use the app in the active
editor when possible.

Click **SDK Path** in the sidebar to browse for an SDK on the workspace host or
return to **Auto**. Bundled applications locate the SDK relative to the checkout.
The folder selection is saved for the current workspace. The extension ignores
a default path from another machine if the SDK is missing there.
Use **NEON-MDF: Create New App...** to create a bare-metal or FreeRTOS project.
**Select Target** and **Select Transport** show the supported GT32U2086 preset
and UART transport. VS Code tasks need access to the toolchain and Python environment
described in [Setup](Setup.md).

## Build, program and view logs

1. Click **Serial Port** under **Device** and choose the board's UART port,
   such as `/dev/ttyUSB0` on Linux.
2. Under **Actions**, select **Build** to configure with `--fresh` and build the application.
3. Select **Flash via UART** to build as needed and program the image selected by CMake.
   Set boot mode `100` and press RESET when the task reports that UART is ready.
4. Select **Serial Monitor** to open the UART terminal. Type directly into it
   to send characters such as `r` and Enter to the firmware.

**Flash via UART** also configures with `--fresh`, then runs the `flash_uart_gt32u2086` CMake target with the selected
port and flash baud. It sets `NEON_MDF_UART_BYTE_GAP_MS=0` and
`NEON_MDF_UART_LOG=progress`. Follow the task terminal for the reset prompt
and upload progress. The status bar and Command Palette also expose the existing
build and flash commands. The extension carries forward the selected Python
interpreter, memory profile, firmware version and signing options when using
`--fresh`. It resolves the SDK path again on the workspace host.
When no Python interpreter was explicitly configured, the extension looks for
`.venv` in the application, workspace and SDK checkout before letting CMake pick
a system interpreter. Install the SDK dependencies into that environment.
Template `NEON_APP_FIXED_CLOCK_HZ` and `Printf_Log`’s `NEON_PRINTF_FIXED_CLOCK_HZ`
are also retained across fresh configuration.

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
