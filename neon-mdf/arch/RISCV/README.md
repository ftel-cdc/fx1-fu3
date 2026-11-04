# Architecture support for RISC-V

Neon MDF provides a backend for WCH's `CH32V003F4P6`, which uses the QingKe V2A
(RV32EC) core. This directory is organized as follows:

```text
arch/RISCV/
  Core/                     # CSR helpers, software arithmetic and libc shim
  Device/<Vendor>/<Family>/ # YAML-generated device header/startup/linker; handwritten system code
  Port/                     # SDK_IRQ_* for PFIC and mstatus critical sections
```

`Port/Src/sdk_irq.c` controls WCH's PFIC and the interrupt-enable bit in the
`mstatus` CSR. This is a WCH-specific implementation rather than standard
PLIC/CLINT support. `examples/FreeRTOS_Blinky` and the FreeRTOS template use
`SDK_IRQ_Enable()` to enable the CH32V003 tick interrupt.

CMake adds the Port sources and include directories for RISC-V targets. The
Cortex-M backend uses CMSIS NVIC/PRIMASK directly. See
[FreeRTOSPort.md](../../../guide/FreeRTOSPort.md) for FreeRTOS tick and trap
handler integration.
