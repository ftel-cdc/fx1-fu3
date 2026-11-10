"""Nhập phần ISP vào FX1.yaml từ CSR v1.07; đối chiếu từng giá trị với regmap C.

Chỉ thay vùng có marker ISP. Phần CPU/platform do người phụ trách BSP quản lý.
Không cần openpyxl; chỉ dùng thư viện chuẩn và PyYAML như generator Neon.
"""

import argparse
import hashlib
from pathlib import Path
import re
from xml.etree import ElementTree as ET
import zipfile

import yaml

ROOT = Path(__file__).resolve().parents[1]
BEGIN = "# BEGIN ISP: sinh bởi tools/import_fx1_isp.py; sửa nguồn CSR rồi nhập lại.\n"
END = "# END ISP\n"
ACCESS = ["ro", "rw", "w1c", "w1s", "w1p", "w1sc", "mixed"]


class Hex(int):
    """Giữ offset/reset ở dạng hex để đọc YAML dễ hơn."""


class Dumper(yaml.SafeDumper):
    def increase_indent(self, flow=False, indentless=False):
        return super().increase_indent(flow, False)


Dumper.add_representer(Hex, lambda dumper, value: dumper.represent_scalar(
    "tag:yaml.org,2002:int", f"0x{value:08X}"))


def workbook_rows(path):
    ns = {"s": "http://schemas.openxmlformats.org/spreadsheetml/2006/main"}
    with zipfile.ZipFile(path) as archive:
        workbook = ET.fromstring(archive.read("xl/workbook.xml"))
        sheet = next(s for s in workbook.find("s:sheets", ns)
                     if s.get("name") == "Flat Registers")
        rel = sheet.get("{http://schemas.openxmlformats.org/officeDocument/2006/relationships}id")
        relations = ET.fromstring(archive.read("xl/_rels/workbook.xml.rels"))
        target = next(r.get("Target") for r in relations if r.get("Id") == rel)
        sheet_path = target.lstrip("/") if target.startswith("/") else "xl/" + target
        strings = []
        if "xl/sharedStrings.xml" in archive.namelist():
            strings = ["".join(t.text or "" for t in s.findall(".//s:t", ns))
                       for s in ET.fromstring(archive.read("xl/sharedStrings.xml"))]
        for row in ET.fromstring(archive.read(sheet_path)).findall("s:sheetData/s:row", ns)[1:]:
            values = {}
            for cell in row.findall("s:c", ns):
                value = cell.findtext("s:v", "", ns)
                if cell.get("t") == "s":
                    value = strings[int(value)]
                elif cell.get("t") == "inlineStr":
                    value = "".join(t.text or "" for t in cell.findall(".//s:t", ns))
                if value:
                    values[re.sub(r"\d+$", "", cell.get("r"))] = value
            yield values


def import_registers(csr, header):
    definitions = dict(re.findall(r"^#define\s+(FX1_ISP_\w+)\s+(\w+)\s*$",
                                  header.read_text(encoding="utf-8"), re.MULTILINE))

    def constant(name):
        value = definitions[name]
        return constant(value) if value in definitions else int(re.sub(r"[uUlL]+$", "", value), 0)

    def check(name, value):
        if constant(name) != value:
            raise ValueError(f"{name}: CSR={value:#x}, regmap={constant(name):#x}")

    registers = []
    for row in workbook_rows(csr):
        if row.get("A") and row.get("B"):
            register = dict(name=row["B"], offset=Hex(int(row["A"], 0)), size=32,
                            access=row["F"].lower(), reset=Hex(int(row["G"], 0)),
                            description=row.get("H", ""), fields=[])
            registers.append(register)
        if row.get("C"):
            bits = [int(b) for b in re.findall(r"\d+", row["D"])]
            high, low = bits if len(bits) == 2 else (bits[0], bits[0])
            if not 0 <= low <= high < 32 or high - low + 1 != int(row["E"]):
                raise ValueError(f"Trường bit sai: {row}")
            register["fields"].append(dict(
                name=row["C"].upper(), bit_offset=low, bit_width=high - low + 1,
                access=row["F"].lower(), reset=Hex(int(row["G"], 0)), description=row.get("H", "")))

    for reg in registers:
        prefix = "FX1_ISP_" + reg["name"]
        for suffix, value in (("OFFSET", reg["offset"]), ("RESET", reg["reset"]),
                              ("ACCESS", ACCESS.index(reg["access"]))):
            check(prefix + "_" + suffix, value)
        masks = {a: 0 for a in ACCESS if a != "mixed"}
        for field in reg["fields"]:
            mask = ((1 << field["bit_width"]) - 1) << field["bit_offset"]
            masks[field["access"]] |= mask
            for suffix, value in (("SHIFT", field["bit_offset"]), ("WIDTH", field["bit_width"]),
                                  ("MASK", mask), ("RESET", field["reset"]),
                                  ("ACCESS", ACCESS.index(field["access"]))):
                check(prefix + "_" + field["name"] + "_" + suffix, value)
        if not reg["fields"]:
            masks[reg["access"]] = 0xFFFFFFFF
        for access, mask in masks.items():
            check(prefix + "_" + access.upper() + "_MASK", mask)
    # Không chấp nhận thiếu/thừa register hoặc field khi nguồn được cập nhật.
    actual_regs = {n for n in definitions if n.endswith("_OFFSET")
                   and n != "FX1_ISP_MAX_REGISTER_OFFSET"}
    actual_fields = {n for n in definitions if n.endswith("_SHIFT")}
    if actual_regs != {"FX1_ISP_" + r["name"] + "_OFFSET" for r in registers}:
        raise ValueError("Danh sách register giữa CSR và header khác nhau")
    if actual_fields != {"FX1_ISP_" + r["name"] + "_" + f["name"] + "_SHIFT"
                         for r in registers for f in r["fields"]}:
        raise ValueError("Danh sách field giữa CSR và header khác nhau")
    return registers


def isp_block(csr, header):
    registers = import_registers(csr, header)
    preamble = BEGIN
    for label, path in (("CSR v1.07", csr), ("regmap v1.07", header)):
        preamble += f"# {label}: {path.name}; SHA256 {hashlib.sha256(path.read_bytes()).hexdigest()}\n"
    isp = dict(name="ISP", base=Hex(0x11000000),
               description="ISP FX1 v1.07; đủ register/field từ sheet Flat Registers của CSR. "
                           "Địa chỉ base theo BSP VP_FX1_V3.0; không thay thế xác nhận map silicon.",
               registers=registers)
    content = yaml.dump([isp], Dumper=Dumper, sort_keys=False, allow_unicode=True, width=110)
    # Danh sách peripheral nằm dưới root 'peripherals'.
    content = "\n".join("  " + line if line else line for line in content.splitlines()) + "\n"
    return preamble + content + END, registers


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--csr", type=Path, required=True)
    parser.add_argument("--regmap", type=Path, required=True)
    parser.add_argument("--device", type=Path, default=ROOT / "devices/FX1.yaml")
    parser.add_argument("--check", action="store_true", help="Chỉ đối chiếu; không sửa YAML")
    args = parser.parse_args()
    block, registers = isp_block(args.csr, args.regmap)
    text = args.device.read_text(encoding="utf-8")
    before, rest = text.split(BEGIN, 1)
    previous, after = rest.split(END, 1)
    if args.check:
        if BEGIN + previous + END != block:
            raise SystemExit("FAIL: phần ISP trong FX1.yaml lệch nguồn; chạy lại không có --check")
    else:
        args.device.write_text(before + block + after, encoding="utf-8")
    print(f"FX1 ISP {'CHECK' if args.check else 'IMPORT'} PASS: {len(registers)} registers, "
          f"{sum(len(r['fields']) for r in registers)} fields; CSR = regmap = YAML")


if __name__ == "__main__":
    main()
