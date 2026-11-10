"""Sinh header FX1 từ YAML, đối chiếu map với BSP local, giữ startup FX1 đã port."""

from pathlib import Path

from mcu_desc.loader import DeviceDescriptionError
from . import gen_fx1_registers


def generate(device, repo_root):
    if (device.device != "FX1" or device.family != "FX1" or device.vendor != "FTEL"
            or device.arch != "riscv-rv32imafc_zicsr_zifencei"
            or device.cpu.extra.get("isa") != "rv32imafc_zicsr_zifencei"
            or device.cpu.extra.get("abi") != "ilp32f"
            or device.cpu.extra.get("interrupt_controller") != "plic"
            or device.cpu.extra.get("mtvec_mode") != "direct"):
        raise DeviceDescriptionError("FX1 phai dung RV32IMAFC/ilp32f, PLIC va mtvec direct")
    for field in ("hart_count", "mtime_hz"):
        value = device.cpu.extra.get(field)
        if not isinstance(value, int) or value <= 0:
            raise DeviceDescriptionError(f"FX1 thieu cpu.{field} hop le")
    checks = {
        "FX1_NUM_HARTS": device.cpu.extra["hart_count"],
        "FX1_MTIME_HZ": device.cpu.extra["mtime_hz"],
        "FX1_PLIC_NUM_SOURCES": device.cpu.extra["num_irq"],
    }
    for region in device.memory:
        checks[f"FX1_{region.name}_BASE"] = region.address
        checks[f"FX1_{region.name}_SIZE"] = region.size
    gen_fx1_registers.validate(device)
    peripheral_macros = gen_fx1_registers.BASE_MACROS
    for peripheral in device.peripherals:
        if peripheral.name not in peripheral_macros:
            raise DeviceDescriptionError(f"FX1: instance chua duoc ho tro: {peripheral.name}")
        checks[f"FX1_{peripheral_macros[peripheral.name]}_BASE"] = peripheral.base
    for irq in device.interrupts:
        checks[f"FX1_IRQ_{irq.name}"] = irq.number
    directory = Path(repo_root) / "arch/RISCV/Device/FTEL/FX1/Include"
    directory.mkdir(parents=True, exist_ok=True)
    lines = ["/* Sinh tu mcu-description/devices/FX1.yaml; khong sua truc tiep. */",
             "#ifndef NEON_DEVICE_FX1_H", "#define NEON_DEVICE_FX1_H",
             '#include "fx1/fx1_memory_map.h"', '#define NEON_MDF_DEVICE_NAME "FX1"']
    for macro, value in checks.items():
        lines.extend([f"#if {macro} != 0x{value:X}",
                      f'#error "FX1.yaml khong khop BSP: {macro}"', "#endif"])
    lines.extend(["#ifndef __ASSEMBLER__", '#include "fx1_registers.h"', "#endif", "#endif", ""])
    header = directory / "FX1.h"
    alias = directory / "mcu_device.h"
    header.write_text("\n".join(lines), encoding="utf-8")
    alias.write_text('/* Sinh tu FX1.yaml. */\n#include "FX1.h"\n', encoding="utf-8")
    return [str(header), str(alias)] + gen_fx1_registers.generate(device, repo_root)
