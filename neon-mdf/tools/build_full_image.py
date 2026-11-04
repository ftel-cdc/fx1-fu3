#!/usr/bin/env python3
"""
build_full_image.py - ghep [bootloader.bin][dem][header 16 byte][dem][app.bin]
thanh 1 file .bin DUY NHAT dai dien TOAN BO noi dung flash tu dia chi bat dau
flash - dung CHUNG cho CA 2 duong flash trong flash_targets.cmake:
flash_jtag (SWD, ghi qua 1 lenh `program` cua openocd) VA flash_uart_stm32
(UART, ghi qua System Memory Bootloader THAT cua chip bang client tu viet
tools/stm32_uart_flash_firmware.py) - ca 2 deu nap TRON BO 1 anh flash duy
nhat nen dung chung 1 tool build anh nay, chi khac nhau o CACH GHI file .bin
ra phan cung.

Header 16 byte (magic/version/size/crc32, little-endian - Cortex-M mac dinh
little-endian) khop CHINH XAC voi components/bootloader/image_header.h +
neon_bootloader_main.c (bootloader tu viet cua SDK doc header nay de xac
minh app truoc khi nhay vao, bat ke duong nao ghi vao flash - JTAG hay
UART). CRC32 dung `zlib.crc32()` (chuan zlib/PNG, poly 0xEDB88320) - khop
voi components/bootloader/neon_bootloader_crc32.c.

Dung: build_full_image.py <bootloader.bin> <bootloader_size_hex> <app.bin> <image_header_size_hex> <output.bin>
Vi du: build_full_image.py bootloader.bin 0x8000 blinky.bin 0x200 blinky_full.bin
"""
import struct
import sys
import zlib
from pathlib import Path

IMAGE_MAGIC = 0x4E454F4E
IMAGE_VERSION = 1


def build_header(image: bytes) -> bytes:
    crc = zlib.crc32(image) & 0xFFFFFFFF
    return struct.pack("<IIII", IMAGE_MAGIC, IMAGE_VERSION, len(image), crc)


def build_app_blob(image: bytes, header_slot_size: int) -> bytes:
    """
    Ghep [header 16 byte][dem 0xFF den header_slot_size][image]. header_slot_size
    (vd 0x200 tren WEACT_F412) la "khe" du align SCB->VTOR, KHONG PHAI kich
    thuoc header that (16 byte) - xem NEON_MDF_IMAGE_HEADER_SIZE,
    mcu-description/schema.
    """
    header = build_header(image)
    if len(header) > header_slot_size:
        raise ValueError(
            f"header ({len(header)} byte) lon hon header_slot_size ({header_slot_size} byte)"
        )
    padding = b"\xFF" * (header_slot_size - len(header))
    return header + padding + image


def main():
    if len(sys.argv) != 6:
        print(
            f"Dung: {sys.argv[0]} <bootloader.bin> <bootloader_size_hex> <app.bin> <image_header_size_hex> <output.bin>",
            file=sys.stderr,
        )
        sys.exit(1)

    bootloader_bin_path, bootloader_size_hex, app_bin_path, header_slot_size_hex, out_path = sys.argv[1:6]
    bootloader_size = int(bootloader_size_hex, 16)
    header_slot_size = int(header_slot_size_hex, 16)

    bootloader_image = Path(bootloader_bin_path).read_bytes()
    if len(bootloader_image) == 0:
        print(f"File '{bootloader_bin_path}' rong", file=sys.stderr)
        sys.exit(1)
    if len(bootloader_image) > bootloader_size:
        print(
            f"Loi: bootloader.bin ({len(bootloader_image)} byte) lon hon bootloader_size "
            f"({bootloader_size} byte) - vuot qua partition bootloader duoc cap",
            file=sys.stderr,
        )
        sys.exit(1)

    app_image = Path(app_bin_path).read_bytes()
    if len(app_image) == 0:
        print(f"File '{app_bin_path}' rong", file=sys.stderr)
        sys.exit(1)

    bootloader_padding = b"\xFF" * (bootloader_size - len(bootloader_image))
    try:
        app_blob = build_app_blob(app_image, header_slot_size)
    except ValueError as e:
        print(f"Loi: {e} - kiem tra lai gia tri image_header_size truyen vao", file=sys.stderr)
        sys.exit(1)

    full_image = bootloader_image + bootloader_padding + app_blob

    Path(out_path).write_bytes(full_image)
    print(
        f"Da ghi {out_path}: {len(bootloader_image)} byte bootloader + {len(bootloader_padding)} byte dem "
        f"+ {header_slot_size} byte header+dem + {len(app_image)} byte app = {len(full_image)} byte tong"
    )


if __name__ == "__main__":
    main()
