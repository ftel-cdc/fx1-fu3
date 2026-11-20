#!/usr/bin/env python3
"""CLI: create CMSIS device header + linker script + startup file + .svd from
mcu-description/devices/<DEVICE>.yaml.

Usage:
  python3 mcu-description/tools/generate.py --device GT32U2086
  python3 mcu-description/tools/generate.py --all

Called automatically by CMake (see cmake/mcu_variants.cmake) each time configured, but also run
manually to preview output.
"""
import argparse
import glob
import os
import sys

sys.path.insert(0, os.path.dirname(__file__))

from mcu_desc.loader import load_device, DeviceDescriptionError  # noqa: E402

REPO_ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
DEVICES_DIR = os.path.join(REPO_ROOT, "mcu-description", "devices")


def _ensure_dir(path):
    os.makedirs(os.path.dirname(path), exist_ok=True)


def generate_cortex_m(device, output_dir):
    from backends.cortex_m import gen_cmsis_header, gen_linker_script, gen_startup_vector, gen_svd

    include_dir = os.path.join(output_dir, "include")

    header_name = f"{device.device}.h"
    header_path = os.path.join(include_dir, header_name)
    alias_header_path = os.path.join(include_dir, "mcu_device.h")
    linker_path = os.path.join(output_dir, device.linker_filename("flash"))
    startup_path = os.path.join(output_dir, f"startup_{device.device}.s")
    svd_path = os.path.join(output_dir, f"{device.device}.svd")

    outputs = [header_path, alias_header_path, linker_path, startup_path, svd_path]
    for p in outputs:
        _ensure_dir(p)

    gen_cmsis_header.generate(device, header_path)
    gen_linker_script.generate(device, linker_path)
    gen_startup_vector.generate(device, startup_path)
    gen_svd.generate(device, svd_path)

    if device.device == "GT32U2086":
        # Use the same resolved profile for linker placement and image packaging.
        from mcu_desc.memory_layout import image_layout, pick_flash
        layout = image_layout(device)
        flash = pick_flash(device)
        image_fragment = os.path.join(output_dir, "GT32U2086_image.cmake")
        with open(image_fragment, "w", encoding="utf-8") as f:
            f.write("# Generated from GT32U2086.yaml; do not edit.\n")
            for name, value in (
                ("BASE", layout.header_base),
                ("SIZE", flash.size),
                ("HEADER_SIZE", layout.header_size),
                ("VECTOR_BASE", layout.flash_base),
            ):
                f.write(f"set(NEON_MDF_GT32U2086_IMAGE_{name} 0x{value:X})\n")
        outputs.append(image_fragment)

    with open(alias_header_path, "w", encoding="utf-8") as f:
        f.write(f'#include "{header_name}"\n')

    return outputs


def generate_one(device_name, profile="normal", output_dir=None):
    yaml_path = os.path.join(DEVICES_DIR, f"{device_name}.yaml")
    if not os.path.isfile(yaml_path):
        print(f"error: not found {yaml_path}", file=sys.stderr)
        sys.exit(1)

    try:
        device = load_device(yaml_path, profile=profile)
    except DeviceDescriptionError as e:
        print(f"error: {yaml_path}: {e}", file=sys.stderr)
        sys.exit(1)

    if device.device == "GT32U2086" and device.arch == "cortex-m0":
        destination = output_dir or os.path.join(os.getcwd(), "build", "generated", device_name, profile)
        outputs = generate_cortex_m(device, os.path.abspath(destination))
    else:
        print(f"error: arch '{device.arch}' not accepted", file=sys.stderr)
        sys.exit(1)

    print(f"[{device_name}] generated:")
    for o in outputs:
        print(f"  - {os.path.relpath(o, REPO_ROOT)}")


def main():
    ap = argparse.ArgumentParser()
    group = ap.add_mutually_exclusive_group(required=True)
    group.add_argument("--device", help="device name, e.g. GT32U2086 (mcu-description/devices/GT32U2086.yaml file matching)")
    group.add_argument("--all", action="store_true", help="create for the whole device in mcu-description/devices/")
    ap.add_argument("--profile", default="normal", help="Memory profile: normal (default) or xip")
    ap.add_argument("--output-dir", help="Output directory; defaults to build/generated/<device>/<profile> in the current directory")
    args = ap.parse_args()

    if args.all:
        for yaml_path in sorted(glob.glob(os.path.join(DEVICES_DIR, "*.yaml"))):
            generate_one(os.path.splitext(os.path.basename(yaml_path))[0], args.profile, args.output_dir)
    else:
        generate_one(args.device, args.profile, args.output_dir)


if __name__ == "__main__":
    main()
