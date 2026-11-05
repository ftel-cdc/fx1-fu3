#!/usr/bin/env python3
"""
build_ota_image.py - ghep [header 16 bytes][buffer][app.bin] into a .bin file FOR SLOT OTA (no
bootloader in this blob - different from build_full_image.py, since flash_ota (see
cmake/flash_targets.cmake, neon_mdf_add_ota_flash_target()) only writes to slot "app_ota" via SWD,
does not touch the bootloader or the running factory app slot.

re-use build_app_blob() of build_full_image.py (import directly, not copy logic) - same as
header/CRC with path flash_jtag/flash_uart_stm32, only different for no previous bootloader in blob.

Use: build_ota_image.py <app.bin> <image_header_size_hex> <output.bin>
Example: build_ota_image.py ota_demo_ota.bin 0x200 ota_demo_ota_blob.bin
"""
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from build_full_image import build_app_blob  # noqa: E402


def main():
    if len(sys.argv) != 4:
        print(f"Used: {sys.argv[0]} <app.bin> <image_header_size_hex> <output.bin>", file=sys.stderr)
        sys.exit(1)

    app_bin_path, header_slot_size_hex, out_path = sys.argv[1:4]
    header_slot_size = int(header_slot_size_hex, 16)

    app_image = Path(app_bin_path).read_bytes()
    if len(app_image) == 0:
        print(f"File '{app_bin_path}' Empty", file=sys.stderr)
        sys.exit(1)

    try:
        blob = build_app_blob(app_image, header_slot_size)
    except ValueError as e:
        print(f"error: {e} - recheck the validity of image_header_size", file=sys.stderr)
        sys.exit(1)

    Path(out_path).write_bytes(blob)
    print(f"already write {out_path}: {header_slot_size} header+padding bytes + {len(app_image)} byte app = {len(blob)} bytes total")


if __name__ == "__main__":
    main()
