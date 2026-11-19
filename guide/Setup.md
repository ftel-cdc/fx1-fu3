# Development environment setup

This guide prepares the tools for building GT32U2086 firmware on Windows, Linux
and macOS. Return to the [README](../README.md) afterward to build and program
your first example. Install the **Arm bare-metal `arm-none-eabi` toolchain** with
newlib; the SDK uses `nano.specs` and `nosys.specs` when linking.

## Required tools

| Component | Requirement |
|---|---|
| CMake | 3.25 or later (CMakePresets version 6) |
| Ninja | Available in `PATH` |
| Python | 3.10 or later, with `venv` and `pip` |
| Arm GNU Toolchain | `arm-none-eabi-gcc`, `g++`, `objcopy`, `objdump`, `size`, newlib |
| Git | Clone the SDK |

CMSIS and FreeRTOS-Kernel are included in the repository. Python reads YAML for
code generation, `kconfiglib` handles configuration, and `pyserial` provides UART
access on Windows and automatic port detection. Linux/macOS UART operations use
the Python standard library when the port is specified explicitly.

## Linux · Ubuntu/Debian

```sh
sudo apt update
sudo apt install git cmake ninja-build python3 python3-venv python3-pip gcc-arm-none-eabi libnewlib-arm-none-eabi
```

Check `cmake --version`. If your distribution provides a version older than 3.25,
install a suitable version from [CMake](https://cmake.org/download/) and add it
to `PATH`. On other distributions, install equivalent packages or download
[Arm GNU Toolchain](https://developer.arm.com/downloads/-/arm-gnu-toolchain-downloads)
for a Linux host and the `arm-none-eabi` target.

If `/dev/ttyUSB0` or `/dev/ttyACM0` cannot be opened, check the device's owning
group. Ubuntu/Debian commonly require membership in `dialout`:

```sh
sudo usermod -aG dialout "$USER"
```

Log out and back in to apply the new group membership. Close other serial monitors
before programming.

## macOS

Install [Homebrew](https://brew.sh/) if needed, then run:

```sh
brew install git cmake ninja python
brew install --cask gcc-arm-embedded
```

The toolchain installation command is documented by [Homebrew Cask](https://formulae.brew.sh/cask/gcc-arm-embedded).

The cask provides Arm GNU Toolchain with its runtime libraries. If downloading
directly from Arm, select the `arm-none-eabi` package for your host architecture
and add its `bin/` directory to `PATH`. Verify both the compiler and `nano.specs`
as described below. UART ports typically appear as `/dev/cu.usbserial-*` or
`/dev/cu.usbmodem*`. Install the adapter manufacturer's USB–UART driver if the
operating system does not recognize the device.

## Windows · PowerShell or CMD

Install the following components and select the option to add them to `PATH`
where available:

1. [Git for Windows](https://git-scm.com/downloads/win).
2. [Python 3](https://www.python.org/downloads/windows/) version 3.10 or later,
   including `pip` and virtual environment support.
3. [CMake](https://cmake.org/download/) version 3.25 or later.
4. [Ninja](https://github.com/ninja-build/ninja/releases): extract `ninja.exe`
   into a directory in `PATH`.
5. [Arm GNU Toolchain](https://developer.arm.com/downloads/-/arm-gnu-toolchain-downloads):
   Windows distribution for `arm-none-eabi`, including newlib.

Open a new terminal after installation. The Python command may be `python`,
`python3` or `py -3`; use the appropriate interpreter to create the virtual
environment. Use `python` after activating `.venv`. Serial ports have names such
as `COM3`; check Device Manager. The README's `cmake -E env` commands work in both
PowerShell and CMD when entered on one line.

## Install Python packages in a virtual environment

From the repository root:

```sh
python3 -m venv .venv
```

On Windows, replace `python3` with `python` or `py -3` as needed. Activate the
environment using the command for your terminal:

```sh
# Linux/macOS
. .venv/bin/activate
```

```powershell
# Windows PowerShell
.\.venv\Scripts\Activate.ps1
```

```bat
:: Windows CMD
.venv\Scripts\activate.bat
```

If PowerShell blocks the activation script, use CMD with `activate.bat`, or invoke
`.venv\Scripts\python.exe` directly and select that interpreter in CMake as shown below.

```sh
python -m pip install -r neon-mdf/tools/requirements.txt
python -c "import yaml, kconfiglib, serial; print('Python dependencies: OK')"
```

The SDK selects its interpreter through `find_package(Python3)`. If you configured
before activating `.venv`, use `--fresh` or select Python explicitly:

```sh
# Run in examples/Printf_Log; replace the path with your .venv interpreter.
cmake --preset GT32U2086 --fresh -DPython3_EXECUTABLE=/absolute/path/to/neon_mdf/.venv/bin/python
```

On Windows, use a path such as `C:/work/neon_mdf/.venv/Scripts/python.exe`.

## Verify the environment

```sh
arm-none-eabi-gcc --version
arm-none-eabi-gcc -print-file-name=nano.specs
arm-none-eabi-gcc -print-file-name=nosys.specs
cmake --version
ninja --version
python --version
```

Both `-print-file-name` commands must return existing paths rather than merely
printing `nano.specs` or `nosys.specs`. If either file is missing, install a
toolchain distribution that includes newlib.

```sh
cd examples/Printf_Log
cmake --preset GT32U2086
cmake --build --preset GT32U2086
```

The output is `build/GT32U2086/printf_log_image.bin`. A successful build verifies
the compiler → code generation → linking → image packaging workflow. Continue
with [UART programming](GT32U2086Flashing.md) to run on a board. Adapter behavior,
timing and hardware operation must be verified through actual sessions on each
operating system.

## SDK environment variable

Bundled examples locate the SDK at `../../neon-mdf`. For a project outside the
repository, set `NEON_MDF_PATH` to the `neon-mdf/` directory:

```sh
# Bash/Zsh, from the repository root
. neon-mdf/export.sh
```

```powershell
# PowerShell, from the repository root
$env:NEON_MDF_PATH = (Resolve-Path .\neon-mdf).Path
```

You can also pass `-DNEON_MDF_PATH=/absolute/path/to/neon-mdf` when configuring.
`export.sh` only sets the SDK path; install the required tools beforehand.

## VS Code (optional)

Install [VS Code](https://code.visualstudio.com/), C/C++ and CMake Tools for editor
support. The NEON-MDF extension uses build tools from `PATH`. Node.js with `npm`
is required to build/package the extension from source; see the
[VS Code guide](VSCode.md).
