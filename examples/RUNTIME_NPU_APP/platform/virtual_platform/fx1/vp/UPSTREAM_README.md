# VP_FX1 SoC SDK (firmware handover)

> Archive of the original full upstream package description. The local MDF
> bundle has since been pruned to retain only the VP_FX1_V2.0 runtime-inference
> dependencies and the VP_FX1_V3.0 binary used by `FX1_FreeRTOS_Smoke`.
> References below to VP_FX1_SOC, VP_NEO_V1.0, `mac/`, vendor test trees, or
> removed V2/V3 firmware sources describe upstream content and are not present
> in the current trimmed bundle. See the
> [Smoke example README](../README.md) for the local layout and commands.

Virtual-platform deliveries of the **VP_FX1 Full SoC** for firmware and driver
development. Treat each VP package as the chip: run the supplied host binary
and build firmware against its fixed register/IRQ ABI.

| Package | Purpose |
|---|---|
| `VP_FX1_SOC` | Baseline SDK, driver examples, and ROM-code boot flow |
| `VP_FX1_V2.0` | NPU-enabled VP with FreeRTOS, TFLite Micro CPU/NPU demos, CLI, hardware scan, and register tests |
| `VP_NEO_V1.0` | Single-hart RV32GCV/RVV 1.0 compiler-enablement VP; not yet the complete NEO core |
| `VP_FX1_V3.0` | FX1 SoC VP: 2 RV32IMAFC harts (`ilp32f`), FX1 bus, SYS_DMA and the FX1 ISP; FreeRTOS CLI firmware with source, ISP RAW-frame samples. **Not a superset of V2.0**: different CPU, ABI and memory map, no NPU |

Sources, generated firmware outputs, test launchers, VP binaries, and documents
are separated by version. See `vp/VERSION` for the baseline build,
`vp/doc/VP_FX1_V2.0/BUILD_INFO.txt` for the V2.0 build identity and hashes, and
`vp/doc/VP_FX1_V3.0/BUILD_MANIFEST.json` for V3.0.

## Layout

```text
fx1/
  vp/
    bin/VP_FX1_SOC/                baseline VP executable + libsystemc.so*
    configs/VP_FX1_SOC/            baseline default.yaml
    doc/VP_FX1_SOC/                baseline memory map, IRQ, and boot-flow docs
    bin/VP_FX1_V2.0/               NPU-enabled VP + libsystemc.so* + default.yaml
    doc/VP_FX1_V2.0/               V2.0 guide, build identity, and SHA256SUMS
    bin/VP_NEO_V1.0/               RV32GCV VP executable + libsystemc.so*
    configs/VP_NEO_V1.0/           compiler-VP runtime configurations
    doc/VP_NEO_V1.0/               ISA/ABI/map contract, manifest, notices, hashes
    bin/VP_FX1_V3.0/               FX1 SoC VP (2 harts, ISP) + libsystemc.so.2.3
    doc/VP_FX1_V3.0/               README, memory map, manifest, licences, SHA256SUMS
    licenses/                       third-party notices
    VERSION                         baseline CDC-VP git SHA + build date + ABI
  sw/
    bsp/VP_FX1_SOC/                 baseline SoC ABI, HAL, startup, and linker file
    drivers/sources/VP_FX1_SOC/     baseline driver examples
    drivers/build/VP_FX1_SOC/       baseline driver outputs
    bootloader/
      sources/VP_FX1_SOC/           baseline ROM-code sources
      build/VP_FX1_SOC/             baseline ROM-code outputs
      test/VP_FX1_SOC/run_vp.sh     baseline VP launcher
      sources/VP_FX1_V2.0/          portable FreeRTOS/NPU/TFLM source package
      build/VP_FX1_V2.0/            generated ELF, BIN, disassembly, and map
      test/VP_FX1_V2.0/run_vp.sh    V2.0 smoke and interactive launcher
      sources/VP_NEO_V1.0/          freestanding scalar/RVV compiler examples
      build/VP_NEO_V1.0/            generated ELF, disassembly, attributes, logs
      test/VP_NEO_V1.0/run_vp.sh    scalar/vector/custom-ELF launcher
      sources/VP_FX1_V3.0/          FreeRTOS CLI firmware, BSP, ISP driver, Makefile
      build/VP_FX1_V3.0/            generated ELF, disassembly, map (not tracked)
      test/VP_FX1_V3.0/             run_vp.sh, console client, ISP samples
```

## Prerequisites

- Linux x86-64 host (AlmaLinux/RHEL 9 is the validated environment).
- GNU Make and the RISC-V bare-metal toolchain `riscv-none-elf-` on `PATH`.
  V2.0 was validated with xPack GNU RISC-V Embedded GCC 15.2.0-1.
- `nc`/Ncat is optional and only needed for the interactive UART console.
- No SystemC installation is required; each VP bundles `libsystemc.so*` beside
  its executable and uses `$ORIGIN` runtime lookup.

## Docker setup (Windows and macOS)

The VP and its bundled SystemC runtime are **Linux x86-64** executables. They
cannot run directly on Windows or macOS. The following tested flow runs the VP
inside an AlmaLinux 9 amd64 container while keeping the repository, downloaded
toolchain, and build outputs on the host.

On Apple Silicon, Docker emulates x86-64 and will be slower than an Intel/AMD
host. The same build commands also work directly on a native x86-64 AlmaLinux
9 machine; Docker is only the compatibility layer for Windows and macOS.

### 1. Install and start Docker Desktop

- Windows: enable the WSL 2 backend and Linux containers.
- macOS: enable Rosetta amd64 emulation when Docker Desktop offers the option.

Start Docker Desktop, wait for its engine to be ready, and verify it in
PowerShell or a macOS terminal:

```bash
docker version
docker run --rm --platform linux/amd64 alpine uname -m
```

The second command must print `x86_64`.

### 2. Start the AlmaLinux container

Windows PowerShell:

```powershell
docker run --rm -it --platform linux/amd64 `
  -p 5000:5000 `
  -v "${PWD}:/workspace" `
  -w /workspace `
  almalinux:9 bash
```

macOS terminal:

```bash
docker run --rm -it --platform linux/amd64 \
  -p 5000:5000 \
  -v "$PWD:/workspace" \
  -w /workspace \
  almalinux:9 bash
```

The prompt changes to `[root@... workspace]#`. Every command in the following
sections, until `exit`, runs **inside this container**. Verify the mount:

```bash
pwd
ls
```

The current directory must be `/workspace`, containing `README.md`, `sw`, and
`vp`.

### 3. Install container utilities

```bash
dnf install -y make curl tar gzip nmap-ncat procps-ng diffutils findutils
```

Because the container uses `--rm`, these RPM packages are removed when the
container exits. The toolchain and build outputs below persist because they
are written under the host-mounted `/workspace` directory.

### 4. Download the Linux x86-64 RISC-V toolchain

The firmware compiler must also be a Linux x86-64 executable. Do not download
the Windows or macOS archive.

Stand at /workspace:

```bash
mkdir -p toolchains
cd toolchains
curl -L -o xpack-riscv-none-elf-gcc-15.2.0-1-linux-x64.tar.gz \
  https://github.com/xpack-dev-tools/riscv-none-elf-gcc-xpack/releases/download/v15.2.0-1/xpack-riscv-none-elf-gcc-15.2.0-1-linux-x64.tar.gz
```
Extract it:

```bash
tar -xzf xpack-riscv-none-elf-gcc-15.2.0-1-linux-x64.tar.gz
```

### 5. Configure and verify the compiler

```bash
export PATH=/workspace/toolchains/xpack-riscv-none-elf-gcc-15.2.0-1/bin:/usr/bin:/bin:$PATH
riscv-none-elf-gcc --version | head -n 1
```

The output must report xPack GCC 15.2.0. Repeat the `export PATH=...` command
each time a new temporary container is started.

## VP_FX1_V3.0: FX1 SoC with ISP and FreeRTOS CLI

Two RV32IMAFC harts (`ilp32f`) on the FX1 bus, with CLINT, PLIC, UART, SYS_DMA
and the FX1 ISP. FreeRTOS V11.2.0 runs a command line on hart 0; hart 1 checks
every IP at boot. It is **not** a superset of V2.0: firmware for V2.0
(`rv32imac`/`ilp32`, its peripherals and NPU) does not run on it, and the
memory map is the FX1 placeholder map pending the HAS.

```bash
make -C sw/bootloader/sources/VP_FX1_V3.0 -j"$(nproc)"
sw/bootloader/test/VP_FX1_V3.0/run_vp.sh smoke            # scripted CLI session, PASS/FAIL
sw/bootloader/test/VP_FX1_V3.0/run_vp.sh isp              # D65 RAW frame through the ISP, golden check, PNG
sw/bootloader/test/VP_FX1_V3.0/run_vp.sh interactive 5000 # then, in a 2nd terminal: .../fx1_console.sh 5000
```

`run_vp.sh isp outdoor1` runs the second sample, which is **internal use
only**. In a Docker container the console listens on the container's
127.0.0.1 only, so `-p 5000:5000` does not reach it. Run the client with
`docker exec -it <container> .../fx1_console.sh 5000` instead, or use
`run_vp.sh isp d65 interactive` in the container's own terminal. See
`vp/doc/VP_FX1_V3.0/README.md` for the details:
- the CLI commands and the memory map;
- the ISP samples and the limits;
- the integrity check (`sha256sum -c vp/doc/VP_FX1_V3.0/SHA256SUMS`).

## VP_NEO_V1.0: RV32GCV compiler enablement

This delivery is one RV32GCV hart with RVV 1.0, VLEN=512 and the `ilp32d` ABI.
It exists so the compiler team can build and execute scalar/RVV ELF images before
the complete NEO core, accelerators and NoC are integrated.

```bash
make -C sw/bootloader/sources/VP_NEO_V1.0 clean
make -C sw/bootloader/sources/VP_NEO_V1.0 \
    TOOLDIR=/opt/toolchains/xpack-riscv-none-elf-gcc-15.2.0-1/bin \
    -j"$(nproc)" all verify

sw/bootloader/test/VP_NEO_V1.0/run_vp.sh smoke
```

See `vp/doc/VP_NEO_V1.0/README.md` for the frozen ISA/ABI contract, custom-ELF
runner, source provenance, licenses and integrity check.

## VP_FX1_V2.0: FreeRTOS + NPU + TFLM

Run from the `fx1` repository root. The pinned FreeRTOS source, required TFLM
headers, and RV32 TFLM archive are included, so firmware rebuilds do not require
CDC-VP, Internet access, or files copied from another workstation.

```bash
make -C sw/bootloader/sources/VP_FX1_V2.0 clean
make -C sw/bootloader/sources/VP_FX1_V2.0 \
    NPU=1 TFLM=1 \
    -j"$(nproc)"

# Deterministic boot/NPU/TFLM/hardware/register regression.
sw/bootloader/test/VP_FX1_V2.0/run_vp.sh smoke
```

The build writes only to `sw/bootloader/build/VP_FX1_V2.0/`:

```text
freertos_fx1.elf
freertos_fx1.bin
freertos_fx1.dis
freertos_fx1.map
freertos_fx1.sections.txt
```

For an interactive FreeRTOS CLI, use two terminals:

```bash
# Terminal 1
sw/bootloader/test/VP_FX1_V2.0/run_vp.sh interactive 5000

# Terminal 2
nc 127.0.0.1 5000
```

The CLI provides `tflm_hello`, `tflm_run`, `tflm_simple`, `tflm_npu`,
`tflm_demo`, `hw_scan`, and `reg_test`. The automated smoke test requires
bit-exact CPU/NPU output, `HW_SCAN PASS implemented=25 reserved=2`, and
`REG_TEST PASS 42/42`.

Validate an untouched delivery with:

```bash
sha256sum -c vp/doc/VP_FX1_V2.0/SHA256SUMS
```

See `vp/doc/VP_FX1_V2.0/README.md` and `vp/doc/VP_FX1_V2.0/BUILD_INFO.txt` for the complete package
boundary, exact dependency commits, tested compiler, and artefact hashes.

## VP_FX1_SOC baseline bring-up

Run from the `fx1` repository root:

```bash
# Toolchain on PATH (adjust to your installation).
export PATH=/opt/toolchains/xpack-riscv-none-elf-gcc-15.2.0-1/bin:$PATH

# Build the smoke-test driver.
make -C sw/drivers/sources/VP_FX1_SOC/uart_hello

# Load the default uart_hello ELF.
sw/bootloader/test/VP_FX1_SOC/run_vp.sh
```

Expected console markers:

```text
VP_FX1 SDK: UART console up
driver template OK
```

Baseline launcher usage:

```bash
run_vp.sh                       # default uart_hello ELF
run_vp.sh path/to/app.elf       # load a specific ELF
run_vp.sh app.elf --sim-ms 20   # pass extra arguments to the VP
run_vp.sh --no-fw               # elaboration smoke without firmware

# ROM-code boot flow; see vp/doc/VP_FX1_SOC/BOOTFLOW_GUIDE.md.
run_vp.sh bootrom.elf --int-flash app.bin --boot-pin low
run_vp.sh bootrom.elf --boot-pin high --uart0-socket 5577 --uart0-wait --sim-ms 60000
run_vp.sh bootrom.elf --boot-pin high --spi-flash image.bin --sim-ms 300
```

## Writing a baseline driver

1. Copy `sw/drivers/sources/VP_FX1_SOC/uart_hello/` as a starting point.
2. Include `soc_memory_map.h` and `soc_irq_map.h`; never hard-code addresses.
3. Access registers through `hal/mmio.h` (`mmio_read32`/`mmio_write32`, 32-bit
   little-endian).
4. For interrupts, enable `mstatus.MIE` and the relevant `mie` bit, install
   `mtvec`, then use CLINT or the PLIC claim/complete flow.
5. Build with `make -C <driver-dir>` and run the resulting ELF with the
   baseline `run_vp.sh`.

Each driver Makefile points `BSP` at `sw/bsp/VP_FX1_SOC` and includes `bsp.mk`,
which fixes the ABI (`-march=rv32imac -mabi=ilp32`) and supplies startup and HAL
sources. Do not change `-march`/`-mabi`; a mismatch can link successfully and
still fail at runtime.

## What is and isn't modeled

- **Shared platform IP:** UARTx2, I2Cx2, SPIx2, TIMERx2, WDT, PWM, DMA, TRNG,
  CMU, PMU, DMIC, OTP, QSPI(+NOR flash), RTC, ADC, GPIO, CLINT, and PLIC.
- **Boot hardware:** BOOTROM 64 KiB at `0x0`, IFLASH 4 MiB at `0x0400_0000`,
  GPIO0 pin 1 boot strap, NOR flash behind SPI0, and the UART0 host bridge.
- **`VP_FX1_SOC` baseline:** ISP0, VPU0, and NPU0 are reserved/unbound.
- **`VP_FX1_V2.0`:** NPU0 is functional at `0x100F_0000`, masters physical
  memory for INT8 GEMM, and signals PLIC source 17. ISP0 and VPU0 remain
  reserved. The FreeRTOS driver completes work through an ISR-to-task
  notification and the TFLM demo performs CPU-side requantization.
- **Functional, not timing-accurate:** use the VP to validate register
  semantics, interrupt ordering, and data movement, not cycle timing or WCET.
- `VP_FX1_SOC` and `VP_FX1_V2.0` are single-hart RV32 M-mode systems without MMU or caches.
- **`VP_FX1_V3.0`:** two RV32IMAFC harts in M-mode, without MMU or caches.
  - Modelled: CLINT, PLIC, one PL011-style UART, SYS_DMA and the FX1 ISP
    (functional, bit-exact to the stand-alone ISP model).
  - Not modelled: the NPU and the other V2.0 peripherals.

## VP_FX1_SOC baseline driver/register map

Two levels under `sw/bsp/VP_FX1_SOC` serve different purposes:

- `regref/<ip>_tlm/README.md` documents register and bit-field behaviour.
- `include/soc/regs/soc_regs_<ip>.h` contains the model-verified C macros used
  by firmware. Shipped clean headers include timer, I2C, and DMA; UART is
  covered by the HAL.

Read `regref/` to understand a register, then use the corresponding clean
header to program it:

```c
#include "soc_memory_map.h"
#include "regs/soc_regs_i2c.h"
#include "mmio.h"

mmio_write32(CDC_I2C0_BASE, CDC_I2C_CTRL, CDC_I2C_CTRL_ENABLEHOST);
```

IPs that only have a `regref/` document currently require a clean header from
the VP team or locally reviewed offsets in the driver.

## Third-party licenses

The VP executables statically link the Bremen riscv-vp ISS (MIT) and Berkeley
SoftFloat-3 (BSD-3-Clause), and ship the Accellera SystemC runtime
(Apache-2.0). V2.0 also includes selected pinned FreeRTOS source and a pinned
TFLite Micro RV32 archive. Keep `vp/licenses/` and the versioned documentation
with the tree whenever this SDK is passed on.
