"""create device header (<DEVICE>.h) for RISC-V (WCH QingKe V2A) from model.Device.

Mirror gen_cmsis_header.py (Cortex-M) about the output image (banner, include guard, peripheral
struct, base address define, memory map) - but other "core config" is safe: no negative
core_cmX.h/CMSIS, no negative IRQn_Type (-14..-1 for core exception) RISC-V uses a different trap
mask (mcause CSR, not the same number of negative IRQn/path as ARM) - IRQn here is just the actual
PFIC number (>=0), uses the receiver blue with 4 to calculate address in vector table (see
gen_startup.py).

_peripheral_struct() IMPORT directly from backends.cortex_m.gen_cmsis_header (do not copy logic -
create C struct from list Register as generic, do not depend on CPU directivity, see model.py's
docstring).
"""
import os
import sys

sys.path.insert(0, os.path.join(os.path.dirname(__file__), "..", "cortex_m"))
from gen_cmsis_header import _peripheral_struct, GENERATED_BANNER  # noqa: E402


def generate(device, out_path: str):
    cpu = device.cpu
    isa = cpu.extra.get("isa", "rv32ec")

    lines = []
    lines.append(GENERATED_BANNER.format(device=device.device))
    guard = f"{device.device.upper()}_H"
    lines.append(f"#ifndef {guard}")
    lines.append(f"#define {guard}")
    lines.append("")
    lines.append('#ifdef __cplusplus')
    lines.append('extern "C" {')
    lines.append('#endif')
    lines.append("")
    lines.append("#include <stdint.h>")
    lines.append("")

    # ---- IRQn (PFIC) ----
    # Unlike ARM, RISC-V has no negative core-exception numbers (-14..-1). mcause identifies
    # traps/exceptions, with the MSB set for interrupts. This list contains device IRQ numbers only
    # (>=0), used as offsets in the PFIC vector table; see gen_startup.py. SysTick/SW are
    # core-related IRQs but still use ordinary positive numbers on this architecture.
    lines.append("/* =========================== Interrupt Number Definition (PFIC) ===================== */")
    lines.append(f"/* {device.device} - see mcu-description/devices/{device.device}.yaml */")
    lines.append("typedef enum {")
    for irq in device.interrupts:
        lines.append(f"  {irq.name}_IRQn = {irq.number},")
    lines.append("} IRQn_Type;")
    lines.append("")

    lines.append(f"#define NEON_RISCV_ISA \"{isa}\"")
    # riscv_csr.h serves the role of Cortex-M core_cmX.h with CSR macros and __NOP()
    # - include it so apps can use __NOP()/RISCV_CSR_* through just
    # #include "mcu_device.h" as usual, no need to know the proper file name.
    lines.append('#include "riscv_csr.h"')
    lines.append(f'#include "system_{device.device}.h"')
    lines.append("")

    # __IO/__I/__O - Cortex-M takes these three macros from core_cmX.h (CMSIS) when _peripheral_struct()
    # creates the struct. RISC-V has no core_cmX.h definition, so define it locally using
    # 3 original CMSIS values so that _peripheral_struct() shares the same logic).
    lines.append("/* ========================= Access qualifier ( CMSIS __IO/__I/__O) ===== */")
    lines.append("#define __I     volatile const")
    lines.append("#define __O     volatile")
    lines.append("#define __IO    volatile")
    lines.append("")

    # ---- Peripheral structs (shared logic with Cortex-M, see docstring at the beginning of the file)----
    lines.append("/* =========================== Peripheral register structures ======================== */")
    seen_types = set()
    for p in device.peripherals:
        if p.type in seen_types:
            continue
        seen_types.add(p.type)
        if p.description:
            lines.append(f"/* {p.description} */")
        lines.append(_peripheral_struct(p.type, p.registers))
        lines.append("")

    lines.append("/* =========================== Peripheral base address / instance ==================== */")
    for p in device.peripherals:
        lines.append(f"#define {p.name}_BASE   0x{p.base:08X}UL")
    lines.append("")
    for p in device.peripherals:
        lines.append(f"#define {p.name}   (({p.type}_TypeDef *) {p.name}_BASE)")
    lines.append("")

    lines.append("/* =========================== Memory map ============================================ */")
    for m in device.memory:
        lines.append(f"#define {m.name}_BASE   0x{m.address:08X}UL   /* {m.kind}, 0x{m.size:X} bytes */")
    lines.append("")

    if device.bootloader is not None:
        flash = sorted([m for m in device.memory if m.kind == "flash"], key=lambda m: m.address)[0]
        bl = device.bootloader
        app_base = flash.address + bl.size
        app_vector_base = app_base + bl.image_header_size
        app_size = flash.size - bl.size - bl.image_header_size
        lines.append("/* ========================= Bootloader partition layout (see YAML devices, item `bootloader:`) === */")
        lines.append(f"#define NEON_MDF_BOOTLOADER_SIZE     0x{bl.size:08X}UL")
        lines.append(f"#define NEON_MDF_IMAGE_HEADER_SIZE   0x{bl.image_header_size:08X}UL")
        lines.append(f"#define NEON_MDF_APP_BASE            0x{app_base:08X}UL")
        lines.append(f"#define NEON_MDF_APP_VECTOR_BASE     0x{app_vector_base:08X}UL")
        lines.append(f"#define NEON_MDF_APP_SIZE            0x{app_size:08X}UL")
        lines.append("")

    lines.append('#ifdef __cplusplus')
    lines.append('}')
    lines.append('#endif')
    lines.append("")
    lines.append(f"#endif /* {guard} */")

    with open(out_path, "w", encoding="utf-8") as f:
        f.write("\n".join(lines) + "\n")
