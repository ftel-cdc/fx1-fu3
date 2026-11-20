# VP_FX1_V2.0 package

> **Historical upstream delivery notes.** The source tree was curated for the
> neon_mdf SDK. The vendor firmware, test runner, TFLM assets, and standalone
> BSP described below are no longer included. For the supported SDK build and
> run commands, see [FX1 FreeRTOS Smoke README](../../../README.md); the
> retained model-inference path is documented in
> [neon_runtime README](../../../../../neon-mdf/components/neon_runtime/README.md).

This directory set is the internal handover of the CDC-VP FX1 virtual platform,
its NPU-enabled FreeRTOS/TFLM firmware, and the sources directly changed for
the NPU integration.

## Package layout

| Path | Contents |
|---|---|
| `sw/bootloader/sources/VP_FX1_V2.0` | Rebuildable FreeRTOS/NPU/TFLM firmware source, selected pinned FreeRTOS source, TFLM application headers plus its pinned prebuilt archive, and VP NPU integration reference source |
| `sw/bootloader/build/VP_FX1_V2.0` | Generated RV32 ELF, raw BIN, disassembly, linker map, and section/symbol report |
| `sw/bootloader/test/VP_FX1_V2.0` | Relocatable VP smoke/interactive runner and deterministic CLI input |
| `vp/bin/VP_FX1_V2.0` | NPU-enabled x86-64 VP, SystemC runtime, and default configuration |
| `vp/doc/VP_FX1_V2.0` | Package metadata, source notes, integration documentation, and SHA-256 manifest |

## Make and Run

```bash
export CC=/usr/bin/gcc
export CXX=/usr/bin/g++
export PATH=/usr/bin:/bin:/opt/toolchains/riscv-none-elf/bin:$PATH

$CC -dumpfullversion
$CXX --version | head -n 1
riscv-none-elf-gcc --version | head -n 1

make -C sw/bootloader/sources/VP_FX1_V2.0 clean
make -C sw/bootloader/sources/VP_FX1_V2.0 \
    NPU=1 TFLM=1 \
    -j"$(nproc)"

cd /fx1/sw/bootloader/test/VP_FX1_V2.0
./run_vp.sh interactive 5000

connect after run_vp.sh:  nc 127.0.0.1 5000
```

