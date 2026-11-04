# Development environment setup

Install these dependencies before using neon-mdf to build and run existing
examples, write drivers, or add targets. **Only macOS with Homebrew has been
verified in this project**: the project's recorded hardware build, flash and
debug sessions used macOS. The Linux and Windows instructions below are based
on general platform knowledge and **have not been tested in this project**.

## 1. Dependencies by task

| Task | Dependencies |
|---|---|
| Build an existing example (`examples/`) | CMake, Ninja, Python 3 with `pyyaml`/`kconfiglib`, and a compiler toolchain for the target |
| Load FU2 through UART ROMCODE | Python standard library on Linux/macOS; `pyserial` on Windows. Select boot mode 100 and reset manually; see `guide/FU2Flashing.md` |
| Flash/debug Cortex-M through a supported debug probe | Also install `openocd`; the FU2 configuration is currently a placeholder |
| Build for `CH32V003F4P6` (RISC-V) | Also install a suitable toolchain, such as `riscv64-elf-gcc` (section 4) |
| Flash physical `CH32V003F4P6` hardware | Also install `wlink` (Rust). **It is not connected to a CMake target**; see the limitations in `README.md` |
| Write drivers or add targets (`guide/AddNewMCU.md`) | The build dependencies above and a PDF reader for the chip's reference manual; this step requires reading the manual directly |
| Use VS Code (recommended) | VS Code, `cmake-tools`/`cpptools`/`cortex-debug`, and Node.js to build `neon-mdf-tools` from source |

## 2. Common tools on all platforms

- **CMake ≥ 3.25** — the main build system. `cmake/presets/targets.json` and the
  examples use CMakePresets version 6, which requires CMake 3.25 or later.
- **Ninja** — the generator used by the SDK's `CMakePresets.json` files.
- **Python 3** and these pip packages:
  ```sh
  pip3 install pyyaml kconfiglib
  ```
  - `pyyaml` is required by `mcu-description/tools/generate.py` to read YAML files.
  - `kconfiglib` is required for `menuconfig` (see `guide/MenuConfig.md`).
  - `pyserial` is required by the STM32 UART tool, automatic serial-port detection
    in the CLI, and the FU2 loader/monitor on Windows. Install it into the Python
    interpreter you use with `python3 -m pip install pyserial`. For the system
    Python on Ubuntu/Debian, `sudo apt install python3-serial` is an alternative.
    The FU2 loader/monitor uses the Python standard library on Linux/macOS when
    a port is specified explicitly, so `pyserial` is optional for that workflow.
    See [FU2 flashing preparation](guide/FU2Flashing.md#preparation).
- **Git** is optional for builds, but required to install `wlink` using
  `cargo install --git ...` (section 4).

## 3. Cortex-M toolchain (`FU2`, `WEACT_F412`)

Install `arm-none-eabi-gcc` and, for supported hardware flash/debug workflows,
`openocd`.

**macOS** (Homebrew; verified on the machine used to write this guide with
`arm-none-eabi-gcc` 10.3-2021.07):
```sh
brew install arm-none-eabi-gcc cmake ninja openocd
```

**Linux** (Debian/Ubuntu package names; untested in this project):
```sh
sudo apt install gcc-arm-none-eabi cmake ninja-build openocd python3-pip
```
If your distribution provides an old version, consider downloading the Arm GNU
Toolchain from developer.arm.com.

**Windows** (untested in this project):

- Install the official Arm GNU Toolchain from developer.arm.com, or use
  `winget install Arm.GnuArmEmbeddedToolchain`.
- Install `winget install Kitware.CMake` and `winget install Ninja-build.Ninja`,
  or install the C++ CMake tools through Visual Studio Installer.
- Download an OpenOCD binary release from openocd.org, or use
  `winget install OpenOCD.OpenOCD` if available. Add it to `PATH` manually.
- **See section 6:** two CMake targets require a Unix-style shell and cannot run
  directly under `cmd.exe` or PowerShell.

Check the installed tools:
```sh
arm-none-eabi-gcc --version
openocd --version
```

## 4. RISC-V toolchain (`CH32V003F4P6`)

**macOS** (Homebrew; verified): this SDK uses `riscv64-elf-gcc`. The filename
`cmake/toolchain/riscv-none-elf.cmake` does not indicate that the xPack
`riscv-none-elf-gcc` package is used; see the comment at the start of that file.
```sh
brew install riscv64-elf-gcc riscv64-elf-gdb
```

**Linux/Windows** (untested in this project): use a toolchain that correctly
supports **`-march=rv32ec_zicsr -mabi=ilp32e`**. The CH32V003 implements RV32EC
without the M extension. Using the more common `rv32imac` and `ilp32` settings
can emit hardware multiply/divide instructions that this chip does not support;
see section 10 of `ARCHITECTURE.md`.

One option to evaluate is the cross-platform xPack `riscv-none-elf-gcc` package:
install `npm install -g xpm`, then
`xpm install @xpack-dev-tools/riscv-none-elf-gcc`. Check its multilib support:
```sh
<compiler> -print-multi-lib | grep ilp32e
```
If there is no `ilp32e` entry, builds can still work: the SDK supplies
`__udivsi3` and related helpers in `arch/RISCV/Core/Src/riscv_soft_math.c`
without requiring an ABI-matching `-lgcc`. See section 10 of `ARCHITECTURE.md`.
Confirm that the compiler accepts `-march=rv32ec_zicsr -mabi=ilp32e` without
falling back to another ISA or ABI before using it on hardware.

**`wlink`** loads firmware onto `CH32V003F4P6` through WCH-LinkE. It is currently
**not connected to a CMake target** and is only needed for manual flashing:
```sh
# Install Rust/cargo first (https://rustup.rs).
cargo install --git https://github.com/ch32-rs/wlink
```

## 5. VS Code (recommended, optional)

1. Install VS Code.
2. Install `ms-vscode.cmake-tools`, `ms-vscode.cpptools` and
   `marus25.cortex-debug`. VS Code recommends them when an app in `examples/`
   is opened.
3. Build `neon-mdf-tools` from source; a prebuilt `.vsix` release is not provided:
   ```sh
   # Requires Node.js (18 or later recommended) and npm.
   cd vscode-extension
   npm install
   npm run typecheck && npm run package && npm run vsix
   ```
   Install the resulting `.vsix` using **Extensions: Install from VSIX...**.
   If extension changes do not appear, see the troubleshooting section in
   `VSCODE.md`: rebuild and reinstall the `.vsix` after editing the source.

## 6. Windows: two CMake targets require `bash`

`serial_monitor` and `flash_uart_stm32` in `cmake/flash_targets.cmake` use
`COMMAND bash -c "..."`, so they cannot run directly in `cmd.exe` or PowerShell.
Install one of these environments:

- **Git for Windows** (`git-scm.com`), which includes Git Bash. Ensure `bash`
  is available in `PATH`.
- **WSL** (Windows Subsystem for Linux), which provides a full Linux environment
  and is useful if you already use WSL for other work.

The other targets (`flash_jtag`, `flash_ota`, `menuconfig` and normal builds)
do not depend on `bash` and can run on native Windows.

## 7. Check the installation with an example build

```sh
cd examples/Blinky
cmake --preset WEACT_F412
cmake --build --preset WEACT_F412
```

The `Memory region .../%age Used` output for `bootloader`, `blinky` and
`blinky_ota` indicates that the required build tools are available. For driver
or target development, continue with `guide/AddNewMCU.md`,
`guide/DeviceYAML.md` and `guide/DriverImplement.md`.
