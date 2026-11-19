# Your first GT32U2086 application

Complete [environment setup](Setup.md) and activate `.venv` before starting.
The quickest approach is to create a project from a template or copy a suitable example.

## Create from VS Code

Open an example, run **NEON-MDF: Create New App...**, name the project and select
bare-metal or FreeRTOS. The wizard copies the SDK template and replaces the
application name and SDK path. See [VS Code](VSCode.md) for extension installation
and SDK selection.

## Create from a terminal

Run the following cross-platform script from the repository root using Python
in `.venv`. It creates `examples/MyApp` from the bare-metal template:

```python
from pathlib import Path
import shutil

source = Path("neon-mdf/tools/templates")
destination = Path("examples/MyApp")
shutil.copytree(source, destination)
for path in destination.rglob("*"):
    if path.is_file():
        text = path.read_text(encoding="utf-8")
        text = text.replace("__APP_NAME__", "MyApp")
        text = text.replace("__NEON_MDF_RELATIVE_PATH__", "../../neon-mdf")
        path.write_text(text, encoding="utf-8")
```

Save this as a temporary script and run it with `python`, or execute it in the
Python REPL. `copytree` reports an error if the destination already exists.
For a FreeRTOS application, replace `templates` with `templates_freertos`;
kernel configuration is in `Inc/FreeRTOSConfig.h`. For applications outside the
repository, replace `../../neon-mdf` with the correct relative SDK path, or use
the VS Code wizard to calculate it automatically.

## Project structure

```text
MyApp/
├── CMakeLists.txt       # Application name, SDK path and source list
├── CMakePresets.json    # GT32U2086 preset, Arm GNU, Ninja, build/GT32U2086
├── Kconfig             # Optional application-specific settings
├── Src/main.c          # Application entry point
└── Inc/FreeRTOSConfig.h # FreeRTOS template only
```

After the first configuration, `sdkconfig` stores application settings and
`build/` contains generated artifacts. Keep `sdkconfig` in version control;
do not commit `build/`.

The application calls `project()` before including the SDK:

```cmake
cmake_minimum_required(VERSION 3.25)
project(MyApp C ASM)
include($ENV{NEON_MDF_PATH}/tools/cmake/project.cmake)
neon_mdf_add_executable(MyApp Src/main.c)
```

This minimal version requires `NEON_MDF_PATH`; templates already provide
SDK path discovery. Add `.c` files to `neon_mdf_add_executable()` and register
include directories with `target_include_directories(MyApp PRIVATE Inc)` as needed.

## Configure and build

```sh
cd examples/MyApp
cmake --preset GT32U2086
cmake --build --preset GT32U2086
```

The template enables UART through the default driver configuration. Check
`CONFIG_NEON_MDF_DRIVER_UART=y` in `sdkconfig` if the application calls `printf`.
The main output is `build/GT32U2086/MyApp_image.bin`; a `.bin` without `_image`
is the payload without its header. `MyApp` is the ELF and `MyApp.map` is the linker map.

## Program, run and edit

```sh
cmake -E env NEON_MDF_SERIAL_PORT=/dev/ttyUSB0 NEON_MDF_SERIAL_BAUD=19200 cmake --build --preset GT32U2086 --target flash_uart_gt32u2086
cmake -E env NEON_MDF_SERIAL_PORT=/dev/ttyUSB0 NEON_MDF_MONITOR_BAUD=19200 cmake --build --preset GT32U2086 --target serial_monitor
```

Set the port and baud for your board. Select boot mode `100`, wait for the loader
to be ready, then press RESET. The template prints `MyApp is running` through
UART0. If initial logs are missed, keep the monitor open and reset in boot mode
`000`. See [UART programming](GT32U2086Flashing.md).

Edit `Src/main.c`, build and program again. For a bare-metal application using
interrupts, finish peripheral/NVIC configuration before calling `__enable_irq()`;
BootROM hands off with global interrupts masked. The FreeRTOS port enables
interrupts when starting the scheduler; see [FreeRTOS](FreeRTOSPort.md).

## Configuration and CLI

```sh
cmake --build --preset GT32U2086 --target menuconfig
cmake --preset GT32U2086
cmake --build --preset GT32U2086
```

The CLI provides a shorter way to invoke the same CMake targets. From the
application directory, after setting `NEON_MDF_PATH`:

```sh
python "$NEON_MDF_PATH/tools/neon_mdf.py" -p /dev/ttyUSB0 -b 19200 build flash-uart-gt32u2086 monitor
```

Available actions are `build`, `flash-uart-gt32u2086`, `monitor`, `menuconfig`
and `set-target`. `GT32U2086` is the only target; see `--help` for parameters.
