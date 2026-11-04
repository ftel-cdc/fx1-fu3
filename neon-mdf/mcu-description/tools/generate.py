#!/usr/bin/env python3
"""CLI: sinh CMSIS device header + linker script + startup file + .svd từ
mcu-description/devices/<DEVICE>.yaml.

Usage:
  python3 mcu-description/tools/generate.py --device FU2
  python3 mcu-description/tools/generate.py --all

Được gọi tự động bởi CMake (xem cmake/mcu_variants.cmake) mỗi lần configure,
nhưng cũng chạy tay được để xem trước output.
"""
import argparse
import glob
import os
import sys

sys.path.insert(0, os.path.dirname(__file__))

from mcu_desc.loader import load_device, DeviceDescriptionError  # noqa: E402

REPO_ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
DEVICES_DIR = os.path.join(REPO_ROOT, "mcu-description", "devices")
GENERATED_DIR = os.path.join(REPO_ROOT, "mcu-description", "generated")


def _ensure_dir(path):
    os.makedirs(os.path.dirname(path), exist_ok=True)


def generate_cortex_m(device):
    from backends.cortex_m import gen_cmsis_header, gen_linker_script, gen_startup_vector, gen_svd

    cmsis_device_root = os.path.join(
        REPO_ROOT, "arch", "CortexM", "CMSIS", "Device", device.family
    )
    include_dir = os.path.join(cmsis_device_root, "Include")
    gcc_dir = os.path.join(cmsis_device_root, "Source", "gcc")
    linker_dir = os.path.join(gcc_dir, "linker")

    header_name = f"{device.device}.h"
    header_path = os.path.join(include_dir, header_name)
    alias_header_path = os.path.join(include_dir, "mcu_device.h")
    linker_path = os.path.join(linker_dir, device.linker_filename("flash"))
    startup_path = os.path.join(gcc_dir, f"startup_{device.device}.s")
    svd_path = os.path.join(GENERATED_DIR, f"{device.device}.svd")

    outputs = [header_path, alias_header_path, linker_path, startup_path, svd_path]
    for p in outputs:
        _ensure_dir(p)

    gen_cmsis_header.generate(device, header_path)
    gen_linker_script.generate(device, linker_path)
    gen_startup_vector.generate(device, startup_path)
    gen_svd.generate(device, svd_path)

    if device.device == "FU2":
        # Use the same resolved profile for linker placement and image packaging.
        from mcu_desc.memory_layout import image_layout, pick_flash
        layout = image_layout(device)
        flash = pick_flash(device)
        image_fragment = os.path.join(GENERATED_DIR, "FU2_image.cmake")
        with open(image_fragment, "w", encoding="utf-8") as f:
            f.write("# Generated from FU2.yaml; do not edit.\n")
            for name, value in (
                ("BASE", layout.header_base),
                ("SIZE", flash.size),
                ("HEADER_SIZE", layout.header_size),
                ("VECTOR_BASE", layout.flash_base),
            ):
                f.write(f"set(NEON_MDF_FU2_IMAGE_{name} 0x{value:X})\n")
        outputs.append(image_fragment)

    with open(alias_header_path, "w", encoding="utf-8") as f:
        f.write(f'#include "{header_name}"\n')

    # bootloader: TUY CHON (xem loader.py) - chi sinh THEM 2 linker script +
    # 1 cmake fragment khi device co khai bao "bootloader:" trong YAML. File
    # cmake fragment la cach duy nhat CMake (khong tu doc YAML/Python) biet
    # duoc device nay co bootloader hay khong - xem cmake/mcu_variants.cmake,
    # cung pattern voi cmake/kconfig.cmake (Python sinh fragment, CMake chi
    # include()).
    if device.bootloader is not None:
        bootloader_linker_path = os.path.join(linker_dir, device.linker_filename("bootloader"))
        app_linker_path = os.path.join(linker_dir, device.linker_filename("app"))
        cmake_fragment_path = os.path.join(GENERATED_DIR, f"{device.device}_bootloader.cmake")
        for p in (bootloader_linker_path, app_linker_path, cmake_fragment_path):
            _ensure_dir(p)

        gen_linker_script.generate_bootloader_partitions(device, bootloader_linker_path, app_linker_path)

        # NEON_MDF_APP_BASE: tinh GIONG HET cach gen_cmsis_header.py tinh macro C
        # cung ten (flash.address + bootloader.size) - khong duoc suy ra khac di,
        # tranh 2 noi tinh dia chi partition ra 2 gia tri lech nhau. Can gia tri
        # nay o dang CMake variable (khong chi C macro) de flash_targets.cmake
        # dung lam dia chi ghi combined-image qua SWD (xem flash_jtag target).
        from mcu_desc.memory_layout import pick_flash
        flash_regions = [pick_flash(device)]
        app_base = flash_regions[0].address + device.bootloader.size

        # NEON_MDF_RAM_BASE/SIZE + NEON_MDF_ERASE_SECTORS: chi can cho
        # tools/gen_flashmap.py (partition table CUA RIENG project, xem
        # cmake/flashmap.cmake) - dung LAI dung 1 vung RAM voi
        # gen_linker_script._pick_ram() (uu tien ten chua "USR") de app
        # partition linker script khop VOI <DEVICE>_APP.ld hien co, khong
        # tinh khac di. NEON_MDF_ERASE_SECTORS rong (chuoi "") neu device
        # khong khai erase_sectors - gen_flashmap.py tu bo qua validate.
        from mcu_desc.memory_layout import image_layout
        app_layout = image_layout(device, "app")
        if device.erase_sectors:
            erase_sectors_str = ",".join(f"0x{es.size:X}:{es.count}" for es in device.erase_sectors)
        else:
            erase_sectors_str = ""

        with open(cmake_fragment_path, "w", encoding="utf-8") as f:
            f.write(
                "# Auto-generated by mcu-description/tools/generate.py - DO NOT EDIT BY HAND.\n"
                f"# Source: mcu-description/devices/{device.device}.yaml (bootloader: section)\n"
                "set(NEON_MDF_BOOTLOADER_ENABLED ON)\n"
                f"set(NEON_MDF_BOOTLOADER_SIZE 0x{device.bootloader.size:X})\n"
                f"set(NEON_MDF_IMAGE_HEADER_SIZE 0x{device.bootloader.image_header_size:X})\n"
                f"set(NEON_MDF_APP_BASE 0x{app_base:X})\n"
                # NEON_MDF_FLASH_SIZE: tong kich thuoc vung flash THAT (khong
                # phai kich thuoc con lai sau bootloader) - dung boi
                # tools/gen_flashmap.py de validate flashmap.csv khong vuot
                # qua flash that (doc lap voi erase_sectors, xem comment
                # trong gen_flashmap.py's validate_flash_capacity()).
                f"set(NEON_MDF_FLASH_SIZE 0x{flash_regions[0].size:X})\n"
                f"set(NEON_MDF_RAM_BASE 0x{app_layout.ram_base:X})\n"
                f"set(NEON_MDF_RAM_SIZE 0x{app_layout.ram_size:X})\n"
                f'set(NEON_MDF_ERASE_SECTORS "{erase_sectors_str}")\n'
            )

        outputs += [bootloader_linker_path, app_linker_path, cmake_fragment_path]

    return outputs


def generate_riscv(device):
    """Mirror generate_cortex_m() ve hinh dang, khac duong dan (KHONG dung
    ten "CMSIS" - dac thu ARM, khong ap dung cho RISC-V) va dung 3 backend
    o backends/riscv/ (gen_riscv_header/gen_linker_script/gen_startup) thay
    vi backends/cortex_m/. gen_svd.py DUNG CHUNG nguyen xi (thuan tuy
    generic, xem docstring cua no)."""
    from backends.riscv import gen_riscv_header, gen_linker_script, gen_startup
    from backends.cortex_m import gen_svd

    device_root = os.path.join(
        REPO_ROOT, "arch", "RISCV", "Device", device.vendor, device.family
    )
    include_dir = os.path.join(device_root, "Include")
    gcc_dir = os.path.join(device_root, "Source", "gcc")
    linker_dir = os.path.join(gcc_dir, "linker")

    header_path = os.path.join(include_dir, f"{device.device}.h")
    alias_header_path = os.path.join(include_dir, "mcu_device.h")
    linker_path = os.path.join(linker_dir, device.linker_filename("flash"))
    startup_path = os.path.join(gcc_dir, f"startup_{device.device}.s")
    svd_path = os.path.join(GENERATED_DIR, f"{device.device}.svd")

    outputs = [header_path, alias_header_path, linker_path, startup_path, svd_path]
    for p in outputs:
        _ensure_dir(p)

    gen_riscv_header.generate(device, header_path)
    gen_linker_script.generate(device, linker_path)
    gen_startup.generate(device, startup_path)
    gen_svd.generate(device, svd_path)

    with open(alias_header_path, "w", encoding="utf-8") as f:
        f.write(
            "/* Auto-generated - DO NOT EDIT BY HAND.\n"
            " * Alias co dinh ten de code arch-generic (vd arch/RISCV/Port/)\n"
            f" * include duoc dung device dang active ma khong hardcode ten '{device.device}'.\n"
            " */\n"
            f'#include "{device.device}.h"\n'
        )

    # bootloader: Phase 6 (xem plan) - mirror y het khoi bootloader cua
    # generate_cortex_m(), chi khac duong dan (arch/RISCV/Device/<Vendor>/<Family>/
    # thay vi arch/CortexM/CMSIS/Device/<Family>/).
    if device.bootloader is not None:
        bootloader_linker_path = os.path.join(linker_dir, device.linker_filename("bootloader"))
        app_linker_path = os.path.join(linker_dir, device.linker_filename("app"))
        cmake_fragment_path = os.path.join(GENERATED_DIR, f"{device.device}_bootloader.cmake")
        for p in (bootloader_linker_path, app_linker_path, cmake_fragment_path):
            _ensure_dir(p)

        gen_linker_script.generate_bootloader_partitions(device, bootloader_linker_path, app_linker_path)

        from mcu_desc.memory_layout import pick_flash
        flash_regions = [pick_flash(device)]
        app_base = flash_regions[0].address + device.bootloader.size

        ram = gen_linker_script._pick_ram(device)
        if device.erase_sectors:
            erase_sectors_str = ",".join(f"0x{es.size:X}:{es.count}" for es in device.erase_sectors)
        else:
            erase_sectors_str = ""

        with open(cmake_fragment_path, "w", encoding="utf-8") as f:
            f.write(
                "# Auto-generated by mcu-description/tools/generate.py - DO NOT EDIT BY HAND.\n"
                f"# Source: mcu-description/devices/{device.device}.yaml (bootloader: section)\n"
                "set(NEON_MDF_BOOTLOADER_ENABLED ON)\n"
                f"set(NEON_MDF_BOOTLOADER_SIZE 0x{device.bootloader.size:X})\n"
                f"set(NEON_MDF_IMAGE_HEADER_SIZE 0x{device.bootloader.image_header_size:X})\n"
                f"set(NEON_MDF_APP_BASE 0x{app_base:X})\n"
                # NEON_MDF_FLASH_SIZE: tong kich thuoc vung flash THAT (khong
                # phai kich thuoc con lai sau bootloader) - dung boi
                # tools/gen_flashmap.py de validate flashmap.csv khong vuot
                # qua flash that (doc lap voi erase_sectors, xem comment
                # trong gen_flashmap.py's validate_flash_capacity()).
                f"set(NEON_MDF_FLASH_SIZE 0x{flash_regions[0].size:X})\n"
                f"set(NEON_MDF_RAM_BASE 0x{ram.address:X})\n"
                f"set(NEON_MDF_RAM_SIZE 0x{ram.size:X})\n"
                f'set(NEON_MDF_ERASE_SECTORS "{erase_sectors_str}")\n'
            )

        outputs += [bootloader_linker_path, app_linker_path, cmake_fragment_path]

    return outputs


def generate_one(device_name, profile="normal"):
    yaml_path = os.path.join(DEVICES_DIR, f"{device_name}.yaml")
    if not os.path.isfile(yaml_path):
        print(f"error: khong tim thay {yaml_path}", file=sys.stderr)
        sys.exit(1)

    try:
        device = load_device(yaml_path, profile=profile)
    except DeviceDescriptionError as e:
        print(f"error: {yaml_path}: {e}", file=sys.stderr)
        sys.exit(1)

    if device.arch.startswith("cortex-m"):
        outputs = generate_cortex_m(device)
    elif device.arch.startswith("riscv"):
        outputs = generate_riscv(device)
    else:
        print(f"error: arch '{device.arch}' khong duoc nhan dien", file=sys.stderr)
        sys.exit(1)

    print(f"[{device_name}] generated:")
    for o in outputs:
        print(f"  - {os.path.relpath(o, REPO_ROOT)}")


def main():
    ap = argparse.ArgumentParser()
    group = ap.add_mutually_exclusive_group(required=True)
    group.add_argument("--device", help="Ten device, vd FU2 (khop file mcu-description/devices/FU2.yaml)")
    group.add_argument("--all", action="store_true", help="Sinh cho toan bo device trong mcu-description/devices/")
    ap.add_argument("--profile", default="normal", help="Memory profile: normal (default) or xip")
    args = ap.parse_args()

    if args.all:
        for yaml_path in sorted(glob.glob(os.path.join(DEVICES_DIR, "*.yaml"))):
            generate_one(os.path.splitext(os.path.basename(yaml_path))[0], args.profile)
    else:
        generate_one(args.device, args.profile)


if __name__ == "__main__":
    main()
