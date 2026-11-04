# Create your first application

This tutorial covers creating, building, flashing and editing a new application.
Install the development environment first; see `SETUP.md`.

## 1. Create a new application

**Using VS Code** (recommended): open the repository root `neon_mdf/` or an
existing application. Press `Cmd/Ctrl+Shift+P`, select **NEON-MDF: Create New
App...**, enter a name containing letters, digits and underscores that does not
start with a digit (for example, `my_app`), then choose **Bare-metal app** or
**FreeRTOS app**. Create it in `examples/` by default. Accept the prompt to open
the new application directory in a separate window.

**Using the command line**: copy one of the two templates and replace two
tokens in the file contents:

```sh
cp -r neon-mdf/tools/templates examples/my_app   # Or tools/templates_freertos
cd examples/my_app
# Replace __APP_NAME__ with my_app and __NEON_MDF_RELATIVE_PATH__ with ../../neon-mdf
# in CMakeLists.txt, CMakePresets.json, Src/main.c and .vscode/settings.json
```

The VS Code wizard replaces these tokens automatically. When copying manually,
replace them yourself using an editor or `sed`.

## 2. Application structure

```text
my_app/
  CMakeLists.txt      # project(my_app) and neon_mdf_add_executable()/neon_mdf_add_app_executable()
  CMakePresets.json   # Includes shared cmake/presets/targets.json presets for every target
  Src/main.c         # Minimal int main(void) entry point, without a GPIO/LED demo
  Kconfig            # Empty placeholder with a commented example; present in both templates
  .vscode/           # settings.json sets NEON_MDF_PATH in cmake.environment; extensions.json
```

See `guide/MenuConfig.md` to add application-specific Kconfig options.
`Src/main.c` provides a main loop or task as the starting point for application
logic. `WEACT_F412` currently has no syscall retarget for UART `printf()`, so
template `printf()` calls do not produce UART logs on that target.

## 3. Build

```sh
cd examples/my_app
cmake --preset WEACT_F412
cmake --build --preset WEACT_F412
```

Replace `WEACT_F412` with `FU2` or `CH32V003F4P6` to build another target.
No changes to `CMakeLists.txt` or `Src/main.c` are needed unless the application
uses target-specific `#ifdef` branches. See `examples/Blinky/Src/main.c` for
one source file supporting all three targets.

Targets with `bootloader:` in their YAML, such as `WEACT_F412` and
`CH32V003F4P6`, also build a `bootloader` executable. The default application
template builds a single application through `neon_mdf_add_app_executable()`
and has no OTA support. See `guide/BootloaderOTA.md` to add OTA.

## 4. Flash and view logs on WEACT_F412 hardware

In VS Code, select `WEACT_F412` in the status bar, then run **NEON-MDF: Build +
Flash + Log**.

From the command line:

```sh
cmake --build --preset WEACT_F412 --target flash_jtag
cmake --build --preset WEACT_F412 --target serial_monitor   # Press Ctrl+] to exit
```

A repeating `"my_app is running"` message indicates that the application is
running on the board.

## 5. Edit and rebuild

Edit `Src/main.c`, for example to toggle an LED through GPIO. Use
`examples/Blinky/Src/main.c` as a reference: call `neon_gpio_clock_enable()`
before `neon_gpio_init()`. See `guide/DriverImplement.md` for the driver API.

Rebuild and flash again. Reconfiguration with `cmake --preset` is only needed
after changes to `CMakeLists.txt`, `flashmap.csv` or `Kconfig`.

## 6. Next steps

- Application-specific `menuconfig` options: `guide/MenuConfig.md`.
- OTA support: `guide/BootloaderOTA.md`.
- VS Code debugging, breakpoints and stepping: `guide/Debugging.md`.
- SDK design rationale: `ARCHITECTURE.md`.
