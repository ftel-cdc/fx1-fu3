"""Sinh file CMSIS-SVD (.svd) từ model.Device - dùng cho debugger (Cortex-Debug
trong VSCode, Ozone, pyOCD gdbserver...) hiển thị cây thanh ghi peripheral lúc
debug. Không cần cho việc build firmware - chỉ để trải nghiệm debug tốt hơn.
"""
from xml.sax.saxutils import escape

_ACCESS_SVD = {"rw": "read-write", "ro": "read-only", "wo": "write-only"}


def generate(device, out_path: str):
    lines = ['<?xml version="1.0" encoding="utf-8"?>']
    lines.append('<device schemaVersion="1.1" xmlns:xs="http://www.w3.org/2001/XMLSchema-instance" xs:noNamespaceSchemaLocation="CMSIS-SVD.xsd">')
    lines.append(f"  <vendor>{escape(device.vendor)}</vendor>")
    lines.append(f"  <name>{escape(device.device)}</name>")
    lines.append(f"  <description>{escape(device.description.strip())}</description>")
    lines.append("  <addressUnitBits>8</addressUnitBits>")
    lines.append("  <width>32</width>")
    lines.append("  <size>32</size>")
    lines.append("  <access>read-write</access>")
    lines.append("  <resetValue>0x00000000</resetValue>")
    lines.append("  <resetMask>0xFFFFFFFF</resetMask>")
    lines.append("  <peripherals>")

    for p in device.peripherals:
        lines.append("    <peripheral>")
        lines.append(f"      <name>{escape(p.name)}</name>")
        if p.description:
            lines.append(f"      <description>{escape(p.description)}</description>")
        lines.append(f"      <baseAddress>0x{p.base:08X}</baseAddress>")
        lines.append("      <addressBlock>")
        lines.append("        <offset>0</offset>")
        # Preserve the minimum window, but cover sparse maps such as DMA-330.
        block_size = max([0x400] + [r.offset + r.size // 8 for r in p.registers])
        lines.append(f"        <size>0x{block_size:X}</size>")
        lines.append("        <usage>registers</usage>")
        lines.append("      </addressBlock>")
        lines.append("      <registers>")
        for physical_register in p.registers:
            register_views = (physical_register.aliases if
                              physical_register.aliases else [physical_register])
            for r in register_views:
                lines.append("        <register>")
                lines.append(f"          <name>{escape(r.name)}</name>")
                if r.description:
                    lines.append(f"          <description>{escape(r.description)}</description>")
                if physical_register.aliases:
                    lines.append(f"          <alternateGroup>{escape(physical_register.name)}</alternateGroup>")
                lines.append(f"          <addressOffset>0x{physical_register.offset:X}</addressOffset>")
                lines.append(f"          <size>{physical_register.size}</size>")
                lines.append(f"          <access>{_ACCESS_SVD.get(r.access, 'read-write')}</access>")
                lines.append(f"          <resetValue>0x{r.reset:08X}</resetValue>")
                if r.fields:
                    lines.append("          <fields>")
                    for fld in r.fields:
                        lines.append("            <field>")
                        lines.append(f"              <name>{escape(fld.name)}</name>")
                        if fld.description:
                            lines.append(f"              <description>{escape(fld.description)}</description>")
                        lines.append(f"              <bitOffset>{fld.bit_offset}</bitOffset>")
                        lines.append(f"              <bitWidth>{fld.bit_width}</bitWidth>")
                        lines.append("            </field>")
                    lines.append("          </fields>")
                lines.append("        </register>")
        lines.append("      </registers>")
        lines.append("    </peripheral>")

    lines.append("  </peripherals>")
    lines.append("</device>")

    with open(out_path, "w", encoding="utf-8") as f:
        f.write("\n".join(lines) + "\n")
