#!/usr/bin/env python3
"""build_otadata_image.py - dynamically calls neon_ota_data_t (see components/ota/neon_ota.h) into a
small .bin file to write via SWD - used by target CMake `flash_ota` (see cmake/flash_targets.cmake)
for real OTA iterations (without actual network connection - see plan file) when testing on
hardware.

Layout must match exactly with neon_ota_data_t (20 bytes, little-endian, default Cortex-M):
    uint32_t magic; uint32_t active_slot; uint32_t state;
    uint32_t boot_attempts; uint32_t crc32;
CRC32 (zlib/PNG, matching neon_bootloader_crc32 phia C) is used for the first 16 bytes
(magic/active_slot/state/boot_attempts), not the crc32 field.

use: build_otadata_image.py <active_slot> <state> <boot_attempts> <output.bin> active_slot:
0=factory, 1=ota (see NEON_OTA_SLOT_* in neon_ota.h) state: 0=VALID, 1=PENDING_VERIFY, 2=INVALID
(see NEON_OTA_STATE_*) Micro enough (duplicate price just finished OTA, for confirmation):
build_otadata_image.py 1 1 0 otadata.bin
"""
import struct
import sys
import zlib

NEON_OTA_MAGIC = 0x4154414F


def build_otadata(active_slot: int, state: int, boot_attempts: int) -> bytes:
    header = struct.pack("<IIII", NEON_OTA_MAGIC, active_slot, state, boot_attempts)
    crc = zlib.crc32(header) & 0xFFFFFFFF
    return header + struct.pack("<I", crc)


def main():
    if len(sys.argv) != 5:
        print(f"Used: {sys.argv[0]} <active_slot> <state> <boot_attempts> <output.bin>", file=sys.stderr)
        sys.exit(1)

    active_slot = int(sys.argv[1])
    state = int(sys.argv[2])
    boot_attempts = int(sys.argv[3])
    out_path = sys.argv[4]

    data = build_otadata(active_slot, state, boot_attempts)
    with open(out_path, "wb") as f:
        f.write(data)
    print(f"already write {out_path} ({len(data)} byte): active_slot={active_slot} state={state} boot_attempts={boot_attempts}")


if __name__ == "__main__":
    main()
