#!/usr/bin/env python3
"""GT32U2086 image and NVR record tool (image format v1.0).

Sub-commands:

  genkey  -o key.pem                        new ECDSA P-256 private key (PEM)
  image   -i app.bin -o app.img [...]       build a secure/non-secure image
  info    app.img [--key key.pem]           decode and check an image
  nvr     --secure-mode 0|1 [--key ...]     NVR slot 0 / slot 1 records

The application binary must be linked for vector_base = flash_addr + 0x80
and start with its 48-word vector table.

Copyright (c) 2026 FPT Telecom. All rights reserved.
"""

import argparse
import binascii
import hashlib
import re
import struct
import subprocess
import sys

IMAGE_MAGIC = 0x46505455
IMAGE_VERSION = 0x0001
HEADER_SIZE = 0x80
SIG_SIZE = 0x40
DOMAIN = b"FU2-IMAGE-V1"  # BootROM v5 protocol constant.
PAYLOAD_MIN = 48 * 4

FLASH_IMAGE_BASE = 0x00020000
XIP_BASE = 0x30000000
FLASH_CAPACITY = {FLASH_IMAGE_BASE: 0x80000, XIP_BASE: 0x40000}
APP_RAM_BASE = 0x01001000
APP_RAM_END = 0x01020000
ROM_MSP_MIN = 0x01000028  # Leave space for relay metadata and an exception frame.

NVR_POLICY_MAGIC = 0x53325546   # "FU2S"
NVR_KEY_MAGIC = 0x4B325546      # "FU2K"
NVR_SLOT_SIZE = 0x200
# Word0 metadata: bit0 PROGRAMMED_N, bit1 LOCK_N (active low).
NVR_WORD0_LOCKED = 0xFFFFFFFC


def crc32(data):
    """CRC-32/ISO-HDLC (zlib)."""
    return binascii.crc32(data) & 0xFFFFFFFF


def parse_version(text):
    """'main.sub1.sub2' -> app_version word."""
    main, sub1, sub2 = (int(x, 0) for x in text.split("."))
    if not (0 <= main <= 0xFFFF and 0 <= sub1 <= 0xFF and 0 <= sub2 <= 0xFF):
        raise ValueError("app version out of range: " + text)
    return main << 16 | sub1 << 8 | sub2


def load_private_key(path):
    from cryptography.hazmat.primitives import serialization
    with open(path, "rb") as f:
        return serialization.load_pem_private_key(f.read(), password=None)


def public_key_xy(key):
    """64-byte raw X || Y of a private or public key object."""
    from cryptography.hazmat.primitives.asymmetric import ec
    if isinstance(key, ec.EllipticCurvePrivateKey):
        key = key.public_key()
    nums = key.public_numbers()
    return nums.x.to_bytes(32, "big") + nums.y.to_bytes(32, "big")


def sign_digest_input(key, message):
    """Raw r || s signature of SHA-256(message)."""
    from cryptography.hazmat.primitives import hashes
    from cryptography.hazmat.primitives.asymmetric import ec, utils
    der = key.sign(message, ec.ECDSA(hashes.SHA256()))
    r, s = utils.decode_dss_signature(der)
    return r.to_bytes(32, "big") + s.to_bytes(32, "big")


def build_image(payload, flash_addr=FLASH_IMAGE_BASE, app_version=0,
                key=None, crc_bypass=False, signature=None):
    """Return header || payload.

    key:       private key object -> secure image; None -> non-secure image.
    signature: force these 64 signature bytes (tests only).
    """
    if len(payload) % 4:
        payload += b"\xff" * (4 - len(payload) % 4)
    if len(payload) < PAYLOAD_MIN:
        raise ValueError("payload must hold the 192-byte vector table")
    if flash_addr not in FLASH_CAPACITY:
        raise ValueError("GT32U2086 image base must be 0x00020000 or 0x30000000")
    if len(payload) + HEADER_SIZE > FLASH_CAPACITY[flash_addr]:
        raise ValueError("image exceeds the selected GT32U2086 Flash region")
    msp, reset = struct.unpack_from("<II", payload)
    vector_base = flash_addr + HEADER_SIZE
    if msp % 8 or not APP_RAM_BASE <= msp <= APP_RAM_END:
        raise ValueError("initial MSP must be aligned within GT32U2086 application RAM")
    if not reset & 1 or not (vector_base + PAYLOAD_MIN <= (reset & ~1) <
                            vector_base + len(payload)):
        raise ValueError("Reset_Handler does not point into the linked GT32U2086 payload")

    secure = key is not None or signature is not None
    first = struct.pack("<IHHIIIIIHH",
                        IMAGE_MAGIC, IMAGE_VERSION, HEADER_SIZE,
                        len(payload), flash_addr,
                        flash_addr + HEADER_SIZE + 4, app_version,
                        crc32(payload), 1 if crc_bypass else 0,
                        SIG_SIZE if secure else 0)
    assert len(first) == 0x20
    if signature is None:
        signature = (sign_digest_input(key, DOMAIN + first + payload)
                     if key is not None else b"\xff" * SIG_SIZE)
    body = first + signature + b"\xff" * 28
    header = body + struct.pack("<I", crc32(body))
    assert len(header) == HEADER_SIZE
    return header + payload


def image_digest(image):
    return hashlib.sha256(DOMAIN + image[:0x20] + image[HEADER_SIZE:]).digest()


def describe(image, pubkey=None):
    """Decode an image; return (fields, list of problems)."""
    problems = []
    if len(image) < HEADER_SIZE:
        return {}, ["image is shorter than its 128-byte header"]
    (magic, version, hsize, size, addr, entry, appver, pcrc, bypass,
     sigsize) = struct.unpack_from("<IHHIIIIIHH", image)
    f = dict(magic=magic, version=version, header_size=hsize,
             payload_size=size, flash_addr=addr, entry_point=entry,
             app_version=appver, payload_crc=pcrc, crc32_bypass=bypass,
             signature_size=sigsize,
             header_crc=struct.unpack_from("<I", image, 0x7C)[0])
    payload = image[HEADER_SIZE:HEADER_SIZE + size]
    if magic != IMAGE_MAGIC:
        problems.append("bad magic")
    if version != IMAGE_VERSION or hsize != HEADER_SIZE:
        problems.append("bad version/header_size")
    if sigsize not in (0, SIG_SIZE):
        problems.append("bad signature_size")
    if bypass > 1:
        problems.append("bad crc32_bypass")
    if size < PAYLOAD_MIN or size % 4 or len(payload) != size:
        problems.append("bad payload_size")
    if len(image) != HEADER_SIZE + size:
        problems.append("file size does not match header + payload")
    if entry != addr + HEADER_SIZE + 4:
        problems.append("entry_point != flash_addr + 0x84")
    if addr not in FLASH_CAPACITY or size + HEADER_SIZE > FLASH_CAPACITY.get(addr, 0):
        problems.append("image outside the selected GT32U2086 Flash region")
    if crc32(image[:0x7C]) != f["header_crc"]:
        problems.append("header CRC mismatch")
    if bypass == 0 and crc32(payload) != pcrc:
        problems.append("payload CRC mismatch")
    if image[0x60:0x7C] != b"\xff" * 28:
        problems.append("reserved not erased")
    if sigsize == 0 and image[0x20:0x60] != b"\xff" * 64:
        problems.append("non-secure image with a signature")
    if len(payload) >= 8:
        msp, reset = struct.unpack_from("<II", payload)
        if msp % 8 or not ROM_MSP_MIN <= msp <= APP_RAM_END:
            problems.append("initial MSP is unaligned or outside the ROMCODE RAM range")
        if not reset & 1 or not addr + HEADER_SIZE <= (reset & ~1) < addr + HEADER_SIZE + size:
            problems.append("Reset_Handler is not a Thumb address inside the payload")
    if sigsize and pubkey is not None:
        from cryptography.exceptions import InvalidSignature
        from cryptography.hazmat.primitives import hashes
        from cryptography.hazmat.primitives.asymmetric import ec, utils
        r = int.from_bytes(image[0x20:0x40], "big")
        s = int.from_bytes(image[0x40:0x60], "big")
        try:
            pubkey.verify(utils.encode_dss_signature(r, s),
                          DOMAIN + image[:0x20] + payload,
                          ec.ECDSA(hashes.SHA256()))
        except InvalidSignature:
            problems.append("signature does not verify")
    return f, problems


def nvr_policy_slot(secure_mode, key_slot=1, word0=NVR_WORD0_LOCKED):
    """512-byte slot 0 image (word0 = controller metadata)."""
    rec = struct.pack("<IIHBBI", word0, NVR_POLICY_MAGIC, 1, secure_mode,
                      0xFF, 0xFFFFFF00 | key_slot)
    return rec + b"\xff" * (NVR_SLOT_SIZE - len(rec))


def nvr_key_slot(xy, word0=NVR_WORD0_LOCKED):
    """512-byte slot 1 image holding the raw P-256 public key."""
    rec = struct.pack("<IIHH", word0, NVR_KEY_MAGIC, 1, 1) + xy
    return rec + b"\xff" * (NVR_SLOT_SIZE - len(rec))


def cmd_genkey(args):
    from cryptography.hazmat.primitives import serialization
    from cryptography.hazmat.primitives.asymmetric import ec
    key = ec.generate_private_key(ec.SECP256R1())
    with open(args.output, "wb") as f:
        f.write(key.private_bytes(serialization.Encoding.PEM,
                                  serialization.PrivateFormat.PKCS8,
                                  serialization.NoEncryption()))
    print("public key X||Y:", public_key_xy(key).hex())


def cmd_image(args):
    flash_addr = int(args.flash_addr, 0)
    if args.elf:
        check_elf_layout(args.elf, flash_addr, args.objdump)
    with open(args.input, "rb") as f:
        payload = f.read()
    key = load_private_key(args.key) if args.key else None
    img = build_image(payload, flash_addr,
                      parse_version(args.app_version), key, args.crc_bypass)
    with open(args.output, "wb") as f:
        f.write(img)
    print("%s: %s image, payload %d bytes, flash_addr 0x%08x" %
          (args.output, "secure" if key else "non-secure",
           len(img) - HEADER_SIZE, int(args.flash_addr, 0)))


def check_elf_layout(path, flash_addr, objdump="arm-none-eabi-objdump"):
    """Reject a stale linker/profile before producing an apparently valid image."""
    result = subprocess.run([objdump, "-h", str(path)], capture_output=True,
                            text=True, check=True)
    sections = {}
    for line in result.stdout.splitlines():
        match = re.match(r"\s*\d+\s+(\S+)\s+([0-9a-fA-F]+)\s+([0-9a-fA-F]+)", line)
        if match:
            sections[match[1]] = (int(match[2], 16), int(match[3], 16))
    if sections.get(".isr_vector") != (PAYLOAD_MIN, flash_addr + HEADER_SIZE):
        raise ValueError("ELF must have 48 vectors at image base + 0x80")
    for name in (".data", ".bss", "._user_heap_stack"):
        size, address = sections.get(name, (0, APP_RAM_BASE))
        if size and not (APP_RAM_BASE <= address and address + size <= APP_RAM_END):
            raise ValueError(f"ELF {name} overlaps reserved/out-of-range SRAM")


def cmd_info(args):
    with open(args.image, "rb") as f:
        img = f.read()
    pub = load_private_key(args.key).public_key() if args.key else None
    fields, problems = describe(img, pub)
    for k, v in fields.items():
        print("%-15s 0x%08x" % (k, v))
    print("image_digest    " + image_digest(img).hex())
    for p in problems:
        print("ERROR:", p)
    return 1 if problems else 0


def cmd_nvr(args):
    slot0 = nvr_policy_slot(args.secure_mode)
    xy = public_key_xy(load_private_key(args.key)) if args.key else \
        b"\xff" * 64
    slot1 = nvr_key_slot(xy)
    with open(args.output, "wb") as f:
        f.write(slot0 + slot1)
    print("%s: slot 0 (policy, secure_mode=%d) + slot 1 (key), 1024 bytes"
          % (args.output, args.secure_mode))
    print("records start at slot offset 0x04; word0 is controller metadata")


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    sub = ap.add_subparsers(dest="cmd", required=True)

    p = sub.add_parser("genkey")
    p.add_argument("-o", "--output", required=True)
    p.set_defaults(func=cmd_genkey)

    p = sub.add_parser("image")
    p.add_argument("-i", "--input", required=True, help="application .bin")
    p.add_argument("-o", "--output", required=True)
    p.add_argument("--flash-addr", default="0x20000",
                   help="0x20000 (internal Flash) or 0x30000000 (XIP)")
    p.add_argument("--app-version", default="0.0.0", help="main.sub1.sub2")
    p.add_argument("--elf", help="check the linked vector and RAM layout")
    p.add_argument("--objdump", default="arm-none-eabi-objdump")
    p.add_argument("--key", help="PEM private key -> secure image")
    p.add_argument("--crc-bypass", action="store_true")
    p.set_defaults(func=cmd_image)

    p = sub.add_parser("info")
    p.add_argument("image")
    p.add_argument("--key", help="PEM private key to check the signature")
    p.set_defaults(func=cmd_info)

    p = sub.add_parser("nvr")
    p.add_argument("--secure-mode", type=int, choices=(0, 1), required=True)
    p.add_argument("--key", help="PEM key whose public part goes to slot 1")
    p.add_argument("-o", "--output", required=True)
    p.set_defaults(func=cmd_nvr)

    args = ap.parse_args()
    try:
        return args.func(args) or 0
    except (ValueError, OSError, struct.error, subprocess.CalledProcessError) as error:
        ap.exit(1, f"ERROR: {error}\n")


if __name__ == "__main__":
    sys.exit(main())
