#!/usr/bin/env python3
"""build_otadata_image.py - dong goi neon_ota_data_t (xem
components/ota/neon_ota.h) thanh 1 file .bin nho de ghi qua SWD - dung boi
target CMake `flash_ota` (xem cmake/flash_targets.cmake) de GIA LAP OTA that
(chua co duong mang thuc su - xem plan file) khi test tren phan cung.

Layout PHAI khop CHINH XAC voi neon_ota_data_t (20 byte, little-endian,
Cortex-M mac dinh):
    uint32_t magic; uint32_t active_slot; uint32_t state;
    uint32_t boot_attempts; uint32_t crc32;
CRC32 (zlib/PNG, khop neon_bootloader_crc32 phia C) tinh tren dung 16 byte
dau (magic/active_slot/state/boot_attempts), KHONG tinh field crc32.

Dung: build_otadata_image.py <active_slot> <state> <boot_attempts> <output.bin>
  active_slot: 0=factory, 1=ota (xem NEON_OTA_SLOT_* trong neon_ota.h)
  state:       0=VALID, 1=PENDING_VERIFY, 2=INVALID (xem NEON_OTA_STATE_*)
Vi du (gia lap vua OTA xong, cho xac nhan): build_otadata_image.py 1 1 0 otadata.bin
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
        print(f"Dung: {sys.argv[0]} <active_slot> <state> <boot_attempts> <output.bin>", file=sys.stderr)
        sys.exit(1)

    active_slot = int(sys.argv[1])
    state = int(sys.argv[2])
    boot_attempts = int(sys.argv[3])
    out_path = sys.argv[4]

    data = build_otadata(active_slot, state, boot_attempts)
    with open(out_path, "wb") as f:
        f.write(data)
    print(f"Da ghi {out_path} ({len(data)} byte): active_slot={active_slot} state={state} boot_attempts={boot_attempts}")


if __name__ == "__main__":
    main()
