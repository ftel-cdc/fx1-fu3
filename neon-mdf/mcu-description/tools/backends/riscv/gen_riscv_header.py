"""Sinh device header (<DEVICE>.h) cho RISC-V (WCH QingKe V2A) tu model.Device.

Mirror gen_cmsis_header.py (Cortex-M) ve HINH DANG output (banner, include
guard, peripheral struct, base address define, memory map) - nhung phan
"core config" khac hoan toan: KHONG co core_cmX.h/CMSIS, KHONG co IRQn_Type
kieu am (-14..-1 cho core exception) vi RISC-V dung co che trap khac han
(mcause CSR, khong phai bang so IRQn am/duong nhu ARM) - IRQn o day CHI la
so PFIC THAT (>=0), dung lam chi so nhan voi 4 de tinh dia chi trong vector
table (xem gen_startup.py).

_peripheral_struct() IMPORT TRUC TIEP tu backends.cortex_m.gen_cmsis_header
(khong copy logic - sinh struct C tu list Register la thuan tuy generic,
khong phu thuoc kien truc CPU, xem model.py's docstring).
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
    # KHAC HAN quy uoc ARM: KHONG co core exception am (-14..-1) - RISC-V
    # dung mcause CSR de phan biet trap/exception (bit MSB=1 = interrupt),
    # con day CHI la so IRQ THIET BI (>=0, dung nhan 4 lam offset trong
    # vector table PFIC - xem gen_startup.py). SysTick/SW la 2 "core-ish"
    # IRQ nhung van danh so duong binh thuong tren kien truc nay (khac ARM).
    lines.append("/* =========================== Interrupt Number Definition (PFIC) ===================== */")
    lines.append(f"/* {device.device} - xem mcu-description/devices/{device.device}.yaml */")
    lines.append("typedef enum {")
    for irq in device.interrupts:
        lines.append(f"  {irq.name}_IRQn = {irq.number},")
    lines.append("} IRQn_Type;")
    lines.append("")

    lines.append(f"#define NEON_RISCV_ISA \"{isa}\"")
    # riscv_csr.h - tuong duong core_cmX.h ben Cortex-M (CSR macro + __NOP())
    # - include o day de code app dung __NOP()/RISCV_CSR_* ma chi can
    # #include "mcu_device.h" nhu binh thuong, khong can biet ten file rieng.
    lines.append('#include "riscv_csr.h"')
    lines.append(f'#include "system_{device.device}.h"')
    lines.append("")

    # __IO/__I/__O - Cortex-M lay 3 macro nay tu core_cmX.h (CMSIS) khi
    # _peripheral_struct() sinh struct - RISC-V KHONG include core_cmX.h nen
    # phai tu dinh nghia o day (dung DUNG 3 gia tri CMSIS goc de _peripheral_struct()
    # dung chung khong doi logic).
    lines.append("/* =========================== Access qualifier (tuong duong CMSIS __IO/__I/__O) ===== */")
    lines.append("#define __I     volatile const")
    lines.append("#define __O     volatile")
    lines.append("#define __IO    volatile")
    lines.append("")

    # ---- Peripheral structs (DUNG CHUNG logic voi Cortex-M, xem docstring dau file) ----
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
        lines.append("/* =========================== Bootloader partition layout (xem devices YAML, muc `bootloader:`) === */")
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
