#!/usr/bin/env python3
"""
build_full_image.py combines [bootloader.bin][padding][16-byte header][padding][app.bin]
to 1 long single .bin file that fills the entire flash content from the flash start address - shared
by both 2 flash path in flash_targets.cmake: flash_jtag (SWD, write via 1 openocd's `program`
command) and flash_uart_stm32 (UART, write via real chip System Memory Bootloader with client from
tools/stm32_uart_flash_firmware.py). Both transports flash complete images, so
share this 1 build image tool, differing only in how to write the .bin file to hardware.

Header 16 byte (magic/version/size/crc32, little-endian - Cortex-M default little-endian) matches
exactly with components/bootloader/image_header.h + neon_bootloader_main.c (bootloader from SDK read
this header to verify app before jumping, enable write flash - JTAG let's UART). CRC32 uses
`zlib.crc32()` (standard zlib/PNG, poly 0xEDB88320) - matches
components/bootloader/neon_bootloader_crc32.c.

Use: build_full_image.py <bootloader.bin> <bootloader_size_hex> <app.bin> <image_header_size_hex> <output.bin>
Example: build_full_image.py bootloader.bin 0x8000 blinky.bin 0x200 blinky_full.bin
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
    Combine [16-byte header][0xFF padding to header_slot_size][image]. header_slot_size
    (e.g. 0x200 on WEACT_F412) is "slot" sufficient to align SCB->VTOR, not true size header (16 bytes)
    - see NEON_MDF_IMAGE_HEADER_SIZE,
    mcu-description/schema.
    """
    header = build_header(image)
    if len(header) > header_slot_size:
        raise ValueError(
            f"header ({len(header)} bytes) exceeds header_slot_size ({header_slot_size} byte)"
        )
    padding = b"\xFF" * (header_slot_size - len(header))
    return header + padding + image


def main():
    if len(sys.argv) != 6:
        print(
            f"Used: {sys.argv[0]} <bootloader.bin> <bootloader_size_hex> <app.bin> <image_header_size_hex> <output.bin>",
            file=sys.stderr,
        )
        sys.exit(1)

    bootloader_bin_path, bootloader_size_hex, app_bin_path, header_slot_size_hex, out_path = sys.argv[1:6]
    bootloader_size = int(bootloader_size_hex, 16)
    header_slot_size = int(header_slot_size_hex, 16)

    bootloader_image = Path(bootloader_bin_path).read_bytes()
    if len(bootloader_image) == 0:
        print(f"File '{bootloader_bin_path}' Empty", file=sys.stderr)
        sys.exit(1)
    if len(bootloader_image) > bootloader_size:
        print(
            f"error: bootloader.bin ({len(bootloader_image)} bytes) exceeds bootloader_size "
            f"({bootloader_size} byte) - pass granted bootloader partition",
            file=sys.stderr,
        )
        sys.exit(1)

    app_image = Path(app_bin_path).read_bytes()
    if len(app_image) == 0:
        print(f"File '{app_bin_path}' Empty", file=sys.stderr)
        sys.exit(1)

    bootloader_padding = b"\xFF" * (bootloader_size - len(bootloader_image))
    try:
        app_blob = build_app_blob(app_image, header_slot_size)
    except ValueError as e:
        print(f"error: {e} - recheck the validity of image_header_size", file=sys.stderr)
        sys.exit(1)

    full_image = bootloader_image + bootloader_padding + app_blob

    Path(out_path).write_bytes(full_image)
    print(
        f"already write {out_path}: {len(bootloader_image)} byte bootloader + {len(bootloader_padding)} padding bytes "
        f"+ {header_slot_size} header+padding bytes + {len(app_image)} byte app = {len(full_image)} bytes total"
    )


if __name__ == "__main__":
    main()
