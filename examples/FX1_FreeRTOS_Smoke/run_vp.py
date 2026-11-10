#!/usr/bin/env python3
"""Chạy ELF nền tảng Neon trên VP FX1; lưu log và verdict có thể đọc lại."""

import argparse
import hashlib
import json
from pathlib import Path
import re
import shlex
import subprocess
import sys


def sha256(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--vp", type=Path, required=True)
    parser.add_argument("--elf", type=Path, required=True)
    parser.add_argument("--readelf", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--timeout-ms", type=int, default=1000)
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    result = {"status": "FAIL", "scope": "FX1_VP_PLATFORM_ONLY", "errors": []}
    try:
        if args.timeout_ms <= 0:
            raise ValueError("timeout-ms phai lon hon 0")
        if not args.vp.is_file():
            raise ValueError("Chua chon VP hop le: configure -DNEON_FX1_VP_EXECUTABLE=/path/to/fx1_soc; build ELF khong can VP")
        for name in ("vp", "elf", "readelf"):
            path = getattr(args, name).resolve(strict=True)
            setattr(args, name, path)
            result[name] = str(path)
        result["elf_sha256"] = sha256(args.elf)
        result["vp_sha256"] = sha256(args.vp)
        header = subprocess.check_output([str(args.readelf), "-h", str(args.elf)], text=True)
        attributes = subprocess.check_output([str(args.readelf), "-A", str(args.elf)], text=True)
        segments = subprocess.check_output([str(args.readelf), "-W", "-l", str(args.elf)], text=True)
        (args.output / "elf_inspect.txt").write_text(header + attributes + segments)
        if not ("ELF32" in header and "RISC-V" in header and "single-float ABI" in header
                and "little endian" in header):
            raise ValueError("ELF khong dung RV32 little-endian / ilp32f")
        if not re.search(r"Entry point address:\s*0x80000000\b", header):
            raise ValueError("Entry ELF khong khop FX1 VP DDR startup")
        # Linker BSP loại .riscv.attributes khỏi ELF cuối. Kiểm tra ISA trên
        # từng object thật và cờ compile, không suy ra ISA từ tên toolchain.
        compilation = json.loads((args.elf.parent / "compile_commands.json").read_text())
        object_checks = []
        for entry in compilation:
            flags = entry.get("arguments") or shlex.split(entry["command"])
            if "-march=rv32imafc_zicsr_zifencei" not in flags or "-mabi=ilp32f" not in flags:
                raise ValueError(f"Co ISA/ABI sai: {entry['file']}")
            obj = Path(entry.get("output") or flags[flags.index("-o") + 1])
            if not obj.is_absolute():
                obj = Path(entry["directory"]) / obj
            obj_attributes = subprocess.check_output([str(args.readelf), "-A", str(obj)], text=True)
            for extension in ("rv32i", "_m", "_a", "_f", "_c", "_zicsr", "_zifencei"):
                if extension not in obj_attributes:
                    raise ValueError(f"Object thieu ISA {extension}: {obj}")
            object_checks.append(str(obj))
        if not object_checks:
            raise ValueError("Khong co object de kiem tra ISA")
        result["objects_checked"] = object_checks
        result["elf_contract"] = "PASS"
        command = [str(args.vp), "--fw", str(args.elf), "--timeout-ms", str(args.timeout_ms)]
        result["command"] = command
        try:
            proc = subprocess.run(command, text=True, stdout=subprocess.PIPE,
                                  stderr=subprocess.STDOUT, timeout=60, check=False)
            output = proc.stdout
            result["exit_code"] = proc.returncode
        except subprocess.TimeoutExpired as exc:
            partial = exc.stdout or b""
            output = partial.decode(errors="replace") if isinstance(partial, bytes) else partial
            (args.output / "platform.log").write_text(output)
            raise RuntimeError("VP vuot qua timeout wall-clock 60 giay") from exc
        (args.output / "platform.log").write_text(output)
        checks = re.findall(r"\[FX1_VP\] PASS (\d+) (.+)", output)
        summary = re.search(r"\[FX1_VP\] SUMMARY checks=(\d+) failed=(\d+)", output)
        result["checks"] = [{"id": int(i), "name": name} for i, name in checks]
        if proc.returncode != 0 or "[fx1_soc] RESULT PASS" not in output:
            raise RuntimeError(f"VP khong PASS, exit={proc.returncode}; xem platform.log")
        if summary is None or int(summary[2]) != 0 or int(summary[1]) < 25:
            raise RuntimeError("Thieu summary hoac chua du so check cua smoke")
        count = int(summary[1])
        if [int(i) for i, _ in checks] != list(range(1, count + 1)) or "[FX1_VP] FAIL" in output:
            raise RuntimeError("Danh sach PASS/summary khong khop")
        result["status"] = "PASS"
        result["passed"] = count
        kernel = re.search(r"\[FX1_VP\] kernel=(.+)", output)
        result["kernel"] = kernel[1] if kernel else "unknown"
    except (OSError, ValueError, RuntimeError, subprocess.CalledProcessError) as exc:
        result["errors"].append(str(exc))
    (args.output / "result.json").write_text(json.dumps(result, indent=2, ensure_ascii=False) + "\n")
    if result["status"] != "PASS":
        print("FX1_VP FAIL: " + "; ".join(result["errors"]), file=sys.stderr)
        return 1
    print(f"FX1_VP: {result['passed']} PASS, 0 FAIL; {args.output / 'platform.log'}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
