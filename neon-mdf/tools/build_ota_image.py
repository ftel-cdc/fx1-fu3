#!/usr/bin/env python3
"""
build_ota_image.py - ghep [header 16 byte][dem][app.bin] thanh 1 file .bin
CHO SLOT OTA (khong co bootloader trong blob nay - khac han
build_full_image.py, vi flash_ota (xem cmake/flash_targets.cmake,
neon_mdf_add_ota_flash_target()) CHI ghi vao slot "app_ota" qua SWD, KHONG
dung toi bootloader/slot "app" (factory) dang chay san).

Dung LAI build_app_blob() cua build_full_image.py (import truc tiep, khong
copy logic) - CUNG dinh dang header/CRC voi duong flash_jtag/flash_uart_stm32,
chi khac o cho KHONG co bootloader dung truoc trong blob.

Dung: build_ota_image.py <app.bin> <image_header_size_hex> <output.bin>
Vi du: build_ota_image.py ota_demo_ota.bin 0x200 ota_demo_ota_blob.bin
"""
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from build_full_image import build_app_blob  # noqa: E402


def main():
    if len(sys.argv) != 4:
        print(f"Dung: {sys.argv[0]} <app.bin> <image_header_size_hex> <output.bin>", file=sys.stderr)
        sys.exit(1)

    app_bin_path, header_slot_size_hex, out_path = sys.argv[1:4]
    header_slot_size = int(header_slot_size_hex, 16)

    app_image = Path(app_bin_path).read_bytes()
    if len(app_image) == 0:
        print(f"File '{app_bin_path}' rong", file=sys.stderr)
        sys.exit(1)

    try:
        blob = build_app_blob(app_image, header_slot_size)
    except ValueError as e:
        print(f"Loi: {e} - kiem tra lai gia tri image_header_size truyen vao", file=sys.stderr)
        sys.exit(1)

    Path(out_path).write_bytes(blob)
    print(f"Da ghi {out_path}: {header_slot_size} byte header+dem + {len(app_image)} byte app = {len(blob)} byte tong")


if __name__ == "__main__":
    main()
