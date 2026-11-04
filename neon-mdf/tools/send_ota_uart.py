#!/usr/bin/env python3
"""Send an app image to NEON-MDF's second-stage bootloader over UART.

Protocol implemented by components/bootloader/neon_bootloader_main.c:
  SYNC 0x55 -> READY 0x52 -> 16-byte image header -> ACK 0x06 ->
  app data in chunks of at most 256 bytes (one ACK per chunk) -> final
  CRC ACK. A NAK (0x15) aborts the transfer.

The input is the raw application .bin linked for app_ota. This client builds
the 16-byte header itself; do not pass an already padded *_blob.bin file.
"""

from __future__ import annotations

import argparse
import os
import struct
import sys
import time
import zlib
from pathlib import Path

SYNC_BYTE = 0x55
READY_BYTE = 0x52
ACK_BYTE = 0x06
NAK_BYTE = 0x15

IMAGE_MAGIC = 0x4E454F4E
IMAGE_HEADER_SIZE = 16
CHUNK_SIZE = 256

DEFAULT_PORT = "/dev/ttyACM0"
DEFAULT_BAUD = 115200
DEFAULT_SYNC_TIMEOUT_S = 15.0
DEFAULT_ERASE_TIMEOUT_S = 30.0
DEFAULT_WRITE_TIMEOUT_S = 5.0
SYNC_RETRY_INTERVAL_S = 0.20


class ProtocolError(RuntimeError):
    """The target did not follow the NEON OTA UART protocol."""


def build_header(firmware: bytes, version: int) -> bytes:
    if not firmware:
        raise ValueError("Firmware rong")
    if version < 0 or version > 0xFFFFFFFF:
        raise ValueError("Version phai nam trong khoang uint32_t")
    crc = zlib.crc32(firmware) & 0xFFFFFFFF
    return struct.pack("<IIII", IMAGE_MAGIC, version, len(firmware), crc)


def _write_all(ser, data: bytes, what: str) -> None:
    written = ser.write(data)
    ser.flush()
    if written != len(data):
        raise ProtocolError(
            f"Chi gui duoc {written}/{len(data)} byte cua {what}"
        )


def _wait_control_byte(ser, expected: int, timeout_s: float, what: str) -> None:
    deadline = time.monotonic() + timeout_s
    unexpected: list[int] = []
    while time.monotonic() < deadline:
        response = ser.read(1)
        if not response:
            continue
        value = response[0]
        if value == expected:
            return
        if value == NAK_BYTE:
            raise ProtocolError(f"Bootloader tra NAK khi {what}")
        if len(unexpected) < 8:
            unexpected.append(value)

    suffix = ""
    if unexpected:
        suffix = "; byte la da nhan: " + " ".join(
            f"0x{value:02X}" for value in unexpected
        )
    raise ProtocolError(f"Timeout {timeout_s:g}s khi {what}{suffix}")


def synchronize(ser, timeout_s: float) -> None:
    """Retry SYNC without sending a second byte after READY was observed."""
    ser.reset_input_buffer()
    deadline = time.monotonic() + timeout_s
    unexpected: list[int] = []

    while time.monotonic() < deadline:
        _write_all(ser, bytes([SYNC_BYTE]), "SYNC")

        # Read everything arriving during this retry interval. This is safer
        # than reading one byte: stale application logs could otherwise hide
        # READY and make the next SYNC corrupt the first byte of the header.
        retry_deadline = min(
            deadline, time.monotonic() + SYNC_RETRY_INTERVAL_S
        )
        while time.monotonic() < retry_deadline:
            response = ser.read(1)
            if not response:
                continue
            value = response[0]
            if value == READY_BYTE:
                return
            if len(unexpected) < 8:
                unexpected.append(value)

    suffix = ""
    if unexpected:
        suffix = "; du lieu UART gan nhat: " + " ".join(
            f"0x{value:02X}" for value in unexpected
        )
    raise ProtocolError(
        f"Khong nhan READY 0x{READY_BYTE:02X} trong {timeout_s:g}s. "
        "Hay reset board sau khi chay lenh va dong Serial Monitor"
        f"{suffix}"
    )


def send_firmware(
    ser,
    firmware: bytes,
    version: int,
    sync_timeout_s: float = DEFAULT_SYNC_TIMEOUT_S,
    erase_timeout_s: float = DEFAULT_ERASE_TIMEOUT_S,
    write_timeout_s: float = DEFAULT_WRITE_TIMEOUT_S,
) -> int:
    """Transfer firmware and return its CRC32 after the final ACK."""
    print("Dang cho bootloader; hay reset board ngay bay gio...")
    synchronize(ser, sync_timeout_s)
    print("Da nhan READY 0x52")

    return transfer_after_ready(
        ser,
        firmware,
        version,
        erase_timeout_s=erase_timeout_s,
        write_timeout_s=write_timeout_s,
    )


def transfer_after_ready(
    ser,
    firmware: bytes,
    version: int,
    erase_timeout_s: float = DEFAULT_ERASE_TIMEOUT_S,
    write_timeout_s: float = DEFAULT_WRITE_TIMEOUT_S,
) -> int:
    """Send header/data after synchronize() has consumed READY."""
    header = build_header(firmware, version)
    crc = zlib.crc32(firmware) & 0xFFFFFFFF

    # No response can exist between READY and the header, so clearing any
    # stale bytes here cannot discard a protocol ACK.
    ser.reset_input_buffer()
    _write_all(ser, header, "header")
    _wait_control_byte(ser, ACK_BYTE, erase_timeout_s, "cho erase + ACK header")
    print(
        f"Header hop le: version={version}, size={len(firmware)} byte, "
        f"crc32=0x{crc:08X}"
    )

    sent = 0
    chunk_index = 0
    total_chunks = (len(firmware) + CHUNK_SIZE - 1) // CHUNK_SIZE
    while sent < len(firmware):
        chunk = firmware[sent : sent + CHUNK_SIZE]
        _write_all(ser, chunk, f"chunk {chunk_index}")
        _wait_control_byte(
            ser, ACK_BYTE, write_timeout_s, f"cho ACK chunk {chunk_index}"
        )
        sent += len(chunk)
        chunk_index += 1
        percent = sent * 100 // len(firmware)
        print(
            f"\rDang gui {sent}/{len(firmware)} byte "
            f"({percent}%, chunk {chunk_index}/{total_chunks})",
            end="",
            flush=True,
        )

    print()
    # The chunk loop consumed the ACK for the final chunk. The bootloader then
    # calculates the whole-image CRC and emits one additional ACK/NAK.
    _wait_control_byte(ser, ACK_BYTE, write_timeout_s, "cho ACK CRC cuoi")
    print("CRC firmware hop le; bootloader dang ghi otadata va reset MCU")
    return crc


def _parse_args() -> argparse.Namespace:
    default_port = os.environ.get("NEON_MDF_SERIAL_PORT", DEFAULT_PORT)
    parser = argparse.ArgumentParser(
        description="Nap app_ota qua giao thuc UART 0x55 cua NEON-MDF bootloader"
    )
    parser.add_argument("firmware", type=Path, help="File app_ota .bin tho")
    parser.add_argument("--port", default=default_port, help=f"Cong UART (mac dinh: {default_port})")
    parser.add_argument("--baud", type=int, default=DEFAULT_BAUD)
    parser.add_argument("--version", type=lambda value: int(value, 0), default=1)
    parser.add_argument("--sync-timeout", type=float, default=DEFAULT_SYNC_TIMEOUT_S)
    parser.add_argument("--erase-timeout", type=float, default=DEFAULT_ERASE_TIMEOUT_S)
    parser.add_argument("--write-timeout", type=float, default=DEFAULT_WRITE_TIMEOUT_S)
    return parser.parse_args()


def main() -> int:
    args = _parse_args()
    try:
        firmware = args.firmware.read_bytes()
    except OSError as exc:
        print(f"Khong doc duoc firmware '{args.firmware}': {exc}", file=sys.stderr)
        return 2
    if not firmware:
        print(f"Firmware '{args.firmware}' rong", file=sys.stderr)
        return 2

    try:
        import serial
    except ImportError:
        print(
            "Thieu pyserial. Cai bang: python3 -m pip install pyserial",
            file=sys.stderr,
        )
        return 2

    print(
        f"Mo {args.port} @ {args.baud} 8N1; firmware={args.firmware} "
        f"({len(firmware)} byte)"
    )
    serial_options = dict(
        port=args.port,
        baudrate=args.baud,
        bytesize=serial.EIGHTBITS,
        parity=serial.PARITY_NONE,
        stopbits=serial.STOPBITS_ONE,
        timeout=0.02,
        write_timeout=args.write_timeout,
    )
    sync_deadline = time.monotonic() + args.sync_timeout
    last_connection_error = None

    try:
        print("Dang cho bootloader; hay bam nut RESET (khong rut cap USB)...")
        while time.monotonic() < sync_deadline:
            synchronized = False
            try:
                with serial.Serial(**serial_options) as ser:
                    remaining = sync_deadline - time.monotonic()
                    synchronize(ser, max(remaining, 0.01))
                    synchronized = True
                    print("Da nhan READY 0x52")
                    transfer_after_ready(
                        ser,
                        firmware,
                        args.version,
                        erase_timeout_s=args.erase_timeout,
                        write_timeout_s=args.write_timeout,
                    )
                    return 0
            except serial.SerialException as exc:
                if synchronized:
                    # Once READY was consumed, reconnecting and sending a new
                    # SYNC would corrupt the in-progress header/chunk stream.
                    raise ProtocolError(
                        "Mat ket noi sau khi bootloader da READY; reset board "
                        "va chay lai lenh"
                    ) from exc
                last_connection_error = exc
                print(
                    f"Cong {args.port} tam thoi ngat; dang cho no xuat hien lai...",
                    file=sys.stderr,
                )
                time.sleep(0.25)

        detail = f": {last_connection_error}" if last_connection_error else ""
        raise ProtocolError(
            f"Khong bat duoc bootloader tren {args.port} trong "
            f"{args.sync_timeout:g}s{detail}. Phai dung USB-UART noi "
            "PA9/PA10, khong phai cong USB truc tiep cua Blue Pill"
        )
    except KeyboardInterrupt:
        print("\nDa huy boi nguoi dung", file=sys.stderr)
        return 130
    except (OSError, ProtocolError, ValueError, serial.SerialException) as exc:
        print(f"Loi OTA UART: {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
