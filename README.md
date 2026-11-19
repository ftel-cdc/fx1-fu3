# NEON-MDF · GT32U2086 SDK

NEON-MDF is a firmware development kit for **GT32U2086**, providing peripheral
drivers, CMSIS, FreeRTOS and application examples. The SDK uses CMake/Ninja and
Arm GNU Toolchain, with terminal and VS Code workflows on Windows, Linux and macOS.

Each application is an independent project. The SDK generates device headers,
startup code, linker scripts and SVD from the GT32U2086 hardware description,
then packages firmware for programming through the UART BootROM.

[Environment setup](guide/Setup.md) · [Your first application](guide/FirstApp.md) ·
[Examples](examples/README.md) · [Documentation](guide/README.md)

## What is included?

- **Peripheral drivers:** GPIO, UART, I2C, SPI, DMA, ADC, timer/PWM, RTC, watchdog,
  Flash/QSPI, CRC, TRNG and AES, plus clock/reset management and SYSCSR.
- **Application configuration:** select drivers and parameters with `menuconfig`/`sdkconfig`.
- **UART console:** `printf` for strings and integers, a programming tool and a serial monitor.
- **FreeRTOS:** Cortex-M0 port integration with application-specific heap/stack settings.
- **Examples and templates:** button inputs, GPIO outputs, interrupts, DMA and RTOS queues.

GT32U2086 has an Arm Cortex-M0 core running at up to 60 MHz, 512 KB instruction
Flash and 128 KB SRAM. FPGA examples typically configure a 10 MHz clock; set the
clock to match your hardware. Hardware details and firmware formats are covered
in the [GT32U2086 guide](guide/GT32U2086.md).

## Getting started

### 1. Prepare the tools

| Tool | Requirement / purpose |
|---|---|
| Git | Obtain the SDK source |
| Arm GNU Toolchain | `arm-none-eabi` distribution with GCC, binutils and newlib |
| CMake | **3.25 or later**, for the SDK presets |
| Ninja | Build firmware |
| Python | **3.10 or later**, with packages from `neon-mdf/tools/requirements.txt` |
| USB–UART adapter and GT32U2086 board | Program and run firmware; no board is needed to build |

Install the tools using the [Windows, Linux or macOS setup guide](guide/Setup.md),
then verify them in your terminal:

```sh
arm-none-eabi-gcc --version
cmake --version
ninja --version
python3 --version
```

On Windows, use `python` if your Python installation does not provide `python3`.
VS Code and Node.js are only needed to use the editor or build/package its extension.

### 2. Obtain the SDK and install Python packages

```sh
git clone https://github.com/ftel-cdc/neon_mdf.git
cd neon_mdf
python3 -m venv .venv
```

Activate the Python environment:

| Terminal | Command from `neon_mdf/` |
|---|---|
| Linux/macOS (Bash, Zsh) | `. .venv/bin/activate` |
| Windows PowerShell | `.\.venv\Scripts\Activate.ps1` |
| Windows CMD | `.venv\Scripts\activate.bat` |

```sh
python -m pip install -r neon-mdf/tools/requirements.txt
```

Keep this environment activated when configuring and building. Bundled examples
locate the SDK relative to their repository directory; see [creating an application](guide/FirstApp.md)
for projects outside the repository.

### 3. Build your first firmware

Use `Printf_Log` to verify your environment and observe the firmware over UART:

```sh
cd examples/Printf_Log
cmake --preset GT32U2086
cmake --build --preset GT32U2086
```

The complete firmware image is **`build/GT32U2086/printf_log_image.bin`**. It
includes the BootROM header; select this file when programming an image manually.

### 4. Program the board and view output

Connect the adapter TX to board RX, adapter RX to board TX, and share GND according
to your board schematic. Close any programs holding the serial port. Replace
`/dev/ttyUSB0` with your actual port, such as `COM3` on Windows or
`/dev/cu.usbserial-…` on macOS.

Run this command on **one line** from `examples/Printf_Log/`:

```sh
cmake -E env NEON_MDF_SERIAL_PORT=/dev/ttyUSB0 NEON_MDF_SERIAL_BAUD=19200 cmake --build --preset GT32U2086 --target flash_uart_gt32u2086
```

Select **boot mode `100`**, wait for the tool to report that UART is ready, then
press **RESET**. Programming baud must match ROMCODE on your board; `19200` is the
example configuration. After successful programming, open the monitor:

```sh
cmake -E env NEON_MDF_SERIAL_PORT=/dev/ttyUSB0 NEON_MDF_MONITOR_BAUD=19200 cmake --build --preset GT32U2086 --target serial_monitor
```

You should see `[INFO] counter=…` lines. Exit the monitor with `Ctrl+C`.
Select boot mode **`000`** to run the installed firmware after subsequent resets
or power cycles. See [UART programming](guide/GT32U2086Flashing.md) for connection
checks and troubleshooting.

## Next steps

| Task | Start here |
|---|---|
| Read a button and control GPIO | [GPIO_Input_Output](examples/GPIO_Input_Output/README.md) |
| Find a peripheral example | [Example index](examples/README.md) |
| Create your own project | [Your first application](guide/FirstApp.md) |
| Build, program and view logs in VS Code | [VS Code](guide/VSCode.md) |
| Enable drivers or change parameters | [menuconfig](guide/MenuConfig.md) |
| Use tasks and queues | [FreeRTOS](guide/FreeRTOSPort.md) |
| Explore drivers, memory and the build system | [Documentation index](guide/README.md) |

UART is the SDK's current getting-started workflow. GT32U2086 does not yet have a
complete JTAG/OpenOCD configuration; see [Debugging](guide/Debugging.md) for the
current integration status.
