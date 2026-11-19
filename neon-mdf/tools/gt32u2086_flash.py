#!/usr/bin/env python3
"""GT32U2086 ROMCODE UART flashing and monitoring for Neon MDF.

Select USART update (100), start the tool, then press RESET on the board.
Port and baud are read at execution time, including when launched by CMake.
Copyright (c) 2026 FPT Telecom. All rights reserved.
"""

import argparse
import math
import os
from pathlib import Path
import queue
import struct
import sys
import threading
import time

import gt32u2086_image
from gt32u2086_uart import open_uart

VERSION = "1.0"
DONE = 0xFFFFFFFF
WORD_SIZE = 4
BOOT_MODES = {
    "000": ("Internal Flash", "Run the image already installed in Internal Flash."),
    "100": ("USART update", "Receive an image over UART and program Internal Flash."),
    "101": ("QSPI indirect update", "Copy an image from the external QSPI source into Internal Flash."),
    "111": ("QSPI XIP", "Run an image already programmed in external QSPI Flash."),
}


class FlashError(Exception):
    """An image, protocol or transport failure with an actionable diagnostic."""


class AsyncLogger:
    """Keep verbose terminal output out of the ROMCODE response deadline."""

    def __init__(self):
        self.messages = queue.Queue()
        self.thread = threading.Thread(target=self._run, daemon=True)
        self.thread.start()

    def _run(self):
        while True:
            message = self.messages.get()
            try:
                if message is None:
                    return
                print(message, flush=True)
            finally:
                self.messages.task_done()

    def log(self, message):
        self.messages.put(message)

    def close(self):
        self.messages.put(None)
        self.messages.join()
        self.thread.join()


def load_image(path, hex_format="auto"):
    if path.suffix.lower() not in (".hex", ".mem"):
        return path.read_bytes()

    records = []
    data_tokens = []
    lines = path.read_text(encoding="ascii").splitlines()

    for line_number, raw_line in enumerate(lines, start=1):
        line = raw_line.split("//", 1)[0].split("#", 1)[0].strip()
        if not line:
            continue

        for token in line.split():
            token = token.replace("_", "")
            if token.startswith("@"):
                address_text = token[1:]
                try:
                    address = int(address_text, 16)
                except ValueError as error:
                    raise ValueError(
                        f"{path}:{line_number}: invalid address directive: {token}"
                    ) from error
                if not address_text or address < 0:
                    raise ValueError(
                        f"{path}:{line_number}: invalid address directive: {token}"
                    )
                records.append(("address", line_number, address))
                continue
            if len(token) > 8:
                raise ValueError(
                    f"{path}:{line_number}: word is wider than 32 bits: {token}"
                )
            records.append(("data", line_number, token))
            data_tokens.append((line_number, token))

    # GT32U2086 project images exist in byte-stream and numeric 32-bit-word forms.
    # One-byte-per-token files are unambiguous.  For wider tokens, use the GT32U2086
    # magic to distinguish textual byte order (55545046) from a numeric
    # little-endian word (46505455).  The explicit CLI setting remains useful
    # for diagnosing a malformed image whose magic is already damaged.
    if hex_format == "auto":
        if data_tokens and all(len(token) <= 2 for _, token in data_tokens):
            actual_format = "byte"
        else:
            actual_format = "word"
            byte_prefix = bytearray()
            for _, token in data_tokens:
                if len(token) % 2:
                    break
                try:
                    byte_prefix.extend(bytes.fromhex(token))
                except ValueError:
                    break
                if len(byte_prefix) >= 4:
                    break
            magic_bytes = gt32u2086_image.IMAGE_MAGIC.to_bytes(4, byteorder="little")
            if byte_prefix.startswith(magic_bytes):
                actual_format = "byte"
    else:
        actual_format = hex_format

    image = bytearray()
    address_base = None
    for kind, line_number, value in records:
        if kind == "address":
            if actual_format != "byte":
                raise ValueError(
                    f"{path}:{line_number}: address directives require byte format"
                )

            address = value
            if address_base is None:
                address_base = address
            relative_address = address - address_base
            if relative_address < len(image):
                raise ValueError(
                    f"{path}:{line_number}: address directive moves backward or "
                    f"overlaps data: 0x{address:08X}"
                )
            if relative_address > gt32u2086_image.FLASH_CAPACITY[gt32u2086_image.FLASH_IMAGE_BASE]:
                raise ValueError(
                    f"{path}:{line_number}: address span exceeds the BootROM "
                    f"image region: 0x{relative_address:X} bytes"
                )
            image.extend(b"\xFF" * (relative_address - len(image)))
            continue

        token = value
        if actual_format == "byte":
            if len(token) % 2:
                raise ValueError(
                    f"{path}:{line_number}: byte-stream token must contain "
                    f"an even number of hex digits: {token}"
                )
            try:
                image.extend(bytes.fromhex(token))
            except ValueError as error:
                raise ValueError(
                    f"{path}:{line_number}: invalid hex value: {token}"
                ) from error
            if len(image) > gt32u2086_image.FLASH_CAPACITY[gt32u2086_image.FLASH_IMAGE_BASE]:
                raise ValueError(
                    f"{path}:{line_number}: image exceeds the BootROM image region"
                )
            continue

        try:
            value = int(token, 16)
        except ValueError as error:
            raise ValueError(
                f"{path}:{line_number}: invalid hex value: {token}"
            ) from error

        try:
            image.extend(value.to_bytes(4, byteorder="little"))
        except OverflowError as error:
            raise ValueError(
                f"{path}:{line_number}: word is wider than 32 bits: {token}"
            ) from error
        if len(image) > gt32u2086_image.FLASH_CAPACITY[gt32u2086_image.FLASH_IMAGE_BASE]:
            raise ValueError(
                f"{path}:{line_number}: image exceeds the BootROM image region"
            )

    return bytes(image)


def extract_request(buffer, expected, last, image_size):
    """Resynchronize fragmented little-endian requests, retries and resets."""
    candidates = [("next", expected)] if expected < image_size else []
    if last is not None:
        candidates.append(("retry", last))
    candidates.append(("done" if expected == image_size else "premature_done", DONE))
    if expected:
        candidates.append(("restart", 0))
    matches = []
    seen = set()
    for priority, (kind, offset) in enumerate(candidates):
        pattern = struct.pack("<I", offset)
        if pattern in seen:
            continue
        seen.add(pattern)
        position = buffer.find(pattern)
        if position >= 0:
            matches.append((position, priority, kind, offset))
    if not matches:
        if len(buffer) > 64:
            del buffer[:-3]
        return None
    position, _, kind, offset = min(matches)
    del buffer[:position + WORD_SIZE]
    return kind, offset, position


def write_all(port, data):
    view = memoryview(data)
    while view:
        count = port.write(view)
        if not count:
            raise FlashError("UART write timed out.")
        view = view[count:]


def send_response(port, data, gap):
    # Match uart_boot_loader.py: finish the requested word, then handle the
    # next request/retry. With no pacing, submit the entire word in one write.
    if gap == 0:
        write_all(port, data)
        port.flush()
        return
    for index, value in enumerate(data):
        write_all(port, bytes([value]))
        if index + 1 < len(data):
            time.sleep(gap)
    port.flush()


def read_uart(port, timeout):
    if hasattr(port, "read_available"):
        return port.read_available(timeout)
    # pyserial on Windows: wait for the first byte, then drain the burst.
    # Bound an idle wait so Ctrl+C remains responsive while awaiting RESET.
    previous_timeout = port.timeout
    port.timeout = min(timeout, 0.5) if timeout is not None else 0.5
    try:
        data = port.read(1)
        if data:
            data += port.read(min(port.in_waiting, 4095))
        return data
    finally:
        port.timeout = previous_timeout


def transfer_image(port, image, timeout=300.0, wait_timeout=0.0,
                   byte_gap=0.002, verbose=True, log=print):
    expected = 0
    last = None
    buffer = bytearray()
    waiting_since = time.monotonic()
    transfer_start = None
    deadline = waiting_since + wait_timeout if wait_timeout else None
    progress = -1
    request_count = 0
    log("[WAIT] Waiting for offset 0x00000000.")
    log("[WAIT] Select boot mode 100 - USART update, then press RESET on the GT32U2086 board.")
    log("[WAIT] UART is ready. Press Ctrl+C to cancel.")
    while True:
        request = extract_request(buffer, expected, last, len(image))
        if request is None:
            if deadline is not None and time.monotonic() >= deadline:
                if transfer_start is None:
                    raise FlashError("No ROMCODE request: check boot mode 100, RESET, UART wiring and baud.")
                raise FlashError(
                    f"No recognized ROMCODE request for {timeout:g} seconds "
                    f"(next offset 0x{expected:08X}). Check the connection, baud, "
                    "image and device NVR policy. ROMCODE does not send a failure code.")
            remaining = max(0.0, deadline - time.monotonic()) if deadline is not None else None
            buffer.extend(read_uart(port, remaining))
            continue
        kind, offset, discarded = request
        request_count += 1
        if discarded:
            log(f"[SYNC] Discarded {discarded} unaligned byte(s).")
        if kind == "premature_done":
            raise FlashError("ROMCODE reported completion before requesting the complete image.")
        if kind == "done":
            now = time.monotonic()
            log(f"[RX #{request_count}] ff ff ff ff  boot complete")
            log(f"[PASS] Image transfer completed in {now - transfer_start:.2f} seconds; "
                f"total runtime {now - waiting_since:.2f} seconds.")
            log("[INFO] ROMCODE verified and committed the image; application handoff follows.")
            log("[INFO] Select boot mode 000 for subsequent resets from Internal Flash.")
            return bytes(buffer)
        if transfer_start is None:
            transfer_start = time.monotonic()
        if kind == "restart":
            log("[RESET] New offset-zero sequence detected.")
            expected, last, progress = 0, None, -1
        response = image[offset:offset + WORD_SIZE]
        # Send before enqueuing logs, as in the original FPGA loader.
        send_response(port, response, byte_gap)
        if verbose:
            log(f"[RX #{request_count}] {struct.pack('<I', offset).hex(' ')}  "
                f"request offset=0x{offset:08X}")
        if kind == "retry":
            log(f"[RETRY] Resent offset 0x{offset:08X}")
        else:
            last, expected = offset, offset + WORD_SIZE
        next_offset = offset + WORD_SIZE
        if verbose:
            log(f"[TX] {response.hex(' ')}  "
                f"image[0x{offset:08X}:0x{next_offset:08X}]  {100.0 * next_offset / len(image):.1f}%")
        else:
            percent = expected * 100 // len(image)
            if percent > progress:
                log(f"[TX] {expected}/{len(image)} image bytes sent ({percent}%).")
                progress = percent
        if expected == gt32u2086_image.HEADER_SIZE and kind != "retry":
            log("[WAIT] Header sent; ROMCODE may pause while preparing/erasing Internal Flash.")
        deadline = time.monotonic() + timeout


def positive_int(text):
    value = int(text)
    if value <= 0:
        raise argparse.ArgumentTypeError("must be a positive integer")
    return value


def finite_time(text):
    value = float(text)
    if not math.isfinite(value) or value < 0:
        raise argparse.ArgumentTypeError("must be finite and non-negative")
    return value


def parse_args(argv=None):
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--version", action="version", version=f"GT32U2086 Flash Tool {VERSION}")
    parser.add_argument("--list-boot-modes", action="store_true", help="describe ROMCODE boot modes without opening UART")
    parser.add_argument("--image", type=Path, help="packaged GT32U2086 image (.bin or byte/word .hex/.mem)")
    parser.add_argument("--hex-format", choices=("auto", "byte", "word"), default="auto")
    parser.add_argument("--profile", choices=("auto", "normal", "xip"), default="auto", help="expected build profile")
    parser.add_argument("--boot-mode", choices=tuple(BOOT_MODES), default="100", help="required hardware boot mode; does not change board pins (default: 100)")
    parser.add_argument("--port", default=os.environ.get("NEON_MDF_SERIAL_PORT"), help="UART port, or NEON_MDF_SERIAL_PORT at execution time")
    parser.add_argument("--baud", type=positive_int, default=os.environ.get("NEON_MDF_SERIAL_BAUD"), help="ROMCODE baud, or NEON_MDF_SERIAL_BAUD at execution time")
    parser.add_argument("--byte-gap-ms", type=finite_time, default=os.environ.get("NEON_MDF_UART_BYTE_GAP_MS", "2"), help="gap between response bytes, or NEON_MDF_UART_BYTE_GAP_MS (default: 2 ms; 0 sends a complete word)")
    parser.add_argument("--timeout", type=finite_time, default=300.0, help="per-request timeout, including Flash erase (default: 300 s)")
    parser.add_argument("--wait-timeout", type=finite_time, default=0.0, help="initial wait for RESET (default: 0, wait until Ctrl+C)")
    parser.add_argument("--check-only", action="store_true", help="validate an image without opening UART")
    parser.add_argument("--log-mode", choices=("full", "progress"), default=os.environ.get("NEON_MDF_UART_LOG", "full"), help="RX/TX log like uart_boot_loader.py, or percentage-only; NEON_MDF_UART_LOG (default: full)")
    parser.add_argument("--verbose", dest="log_mode", action="store_const", const="full", help="alias for --log-mode full (default)")
    parser.add_argument("--quiet", dest="log_mode", action="store_const", const="progress", help="alias for --log-mode progress")
    monitor = parser.add_mutually_exclusive_group()
    monitor.add_argument("--monitor", action="store_true", help="monitor the application after successful programming")
    monitor.add_argument("--no-monitor", action="store_false", dest="monitor", help="exit after programming (default)")
    parser.set_defaults(monitor=False)
    parser.add_argument("--monitor-only", action="store_true", help="open the UART monitor without programming")
    parser.add_argument("--monitor-baud", type=positive_int, default=os.environ.get("NEON_MDF_MONITOR_BAUD", "19200"), help="application UART baud, independent of programming (default: 19200)")
    args = parser.parse_args(argv)
    if args.timeout == 0:
        parser.error("--timeout must be positive")
    if args.byte_gap_ms * 3 >= 100:
        parser.error("inter-byte gaps must total less than the ROMCODE's approximate 100 ms receive window")
    if args.log_mode not in ("full", "progress"):
        parser.error("NEON_MDF_UART_LOG must be full or progress")
    return parser, args


def banner(args, fields=None):
    print(f"NEON MDF - GT32U2086 Flash Tool v{VERSION}")
    print("Operation          : " + ("UART monitor" if args.monitor_only else
                                      "Check image" if args.check_only else "Program image"))
    if not args.monitor_only:
        name = BOOT_MODES[args.boot_mode][0]
        print(f"Required boot mode : {args.boot_mode} - {name} (select on board)")
    if fields:
        profile = "xip" if fields["flash_addr"] == gt32u2086_image.XIP_BASE else "normal"
        app_version = fields["app_version"]
        print(f"Image profile      : {profile}")
        print(f"Image              : {args.image}")
        print(f"Application version: {app_version >> 16}.{(app_version >> 8) & 255}.{app_version & 255}")
        print(f"Image base         : 0x{fields['flash_addr']:08X}")
    if not args.check_only:
        baud = args.monitor_baud if args.monitor_only else args.baud
        print(f"Port               : {args.port or '(not specified)'}")
        print(f"UART               : {baud or '(not specified)'} baud, 8N1, no flow control")
        if not args.monitor_only:
            print("Reset              : Manual")
            print(f"Byte gap           : {args.byte_gap_ms:g} ms")
            print(f"Transfer log       : {args.log_mode}")
    print(flush=True)


def monitor_uart(port, baud, remaining=b""):
    port.baudrate = baud
    print(f"[MONITOR] Application UART at {baud} baud, 8N1. Press Ctrl+C to exit.", flush=True)
    if remaining:
        print(remaining.decode("utf-8", errors="replace"), end="", flush=True)
    while True:
        data = port.read(min(port.in_waiting, 4096) or 1)
        if data:
            print(data.decode("utf-8", errors="replace"), end="", flush=True)


def main(argv=None):
    parser, args = parse_args(argv)
    if args.list_boot_modes:
        print(f"GT32U2086 Flash Tool {VERSION} - ROMCODE boot modes")
        for code, (name, description) in BOOT_MODES.items():
            print(f"{code}  {name}: {description}")
        print("UART programming is available only for 100; boot-mode selection is a board operation.")
        return 0
    connection = None
    try:
        image, fields = None, None
        if not args.monitor_only:
            if not args.image:
                parser.error("--image is required for image checking/programming")
            image = load_image(args.image, args.hex_format)
            fields, problems = gt32u2086_image.describe(image)
            banner(args, fields)
            if problems:
                raise FlashError("Invalid GT32U2086 image: " + "; ".join(problems))
            profile = "xip" if fields["flash_addr"] == gt32u2086_image.XIP_BASE else "normal"
            if args.profile not in ("auto", profile):
                raise FlashError(f"Build profile {args.profile} does not match image profile {profile}.")
            print("[PASS] Image header, size, CRC and vectors checked locally.", flush=True)
            if fields["crc32_bypass"]:
                print("[INFO] Payload CRC is bypassed by the image header.", flush=True)
            if fields["signature_size"]:
                print("[INFO] Signature present; device ROMCODE performs authentication using the NVR key.", flush=True)
            if args.check_only:
                return 0
            if profile == "xip":
                raise FlashError(
                    "XIP image targets external QSPI Flash at 0x30000000. USART update "
                    "writes Internal Flash only. Program external Flash with a QSPI programmer "
                    "or a UART-to-QSPI loader, then select boot mode 111 and RESET.")
            if args.boot_mode != "100":
                raise FlashError(
                    f"Boot mode {args.boot_mode} ({BOOT_MODES[args.boot_mode][0]}) has no "
                    "UART programming service. Use --list-boot-modes for its required workflow.")
        else:
            banner(args)
        if not args.port:
            parser.error("specify --port or NEON_MDF_SERIAL_PORT when running the tool/build target")
        if not args.monitor_only and args.baud is None:
            parser.error("specify --baud or NEON_MDF_SERIAL_BAUD matching the board ROMCODE")
        if not args.monitor_only:
            print("[INFO] ROMCODE will enforce the device NVR policy and verify the installed image.", flush=True)
        connection = open_uart(args.port, args.monitor_baud if args.monitor_only else args.baud)
        if args.monitor_only:
            monitor_uart(connection, args.monitor_baud)
        else:
            logger = AsyncLogger()
            try:
                remaining = transfer_image(connection, image, args.timeout, args.wait_timeout,
                                           args.byte_gap_ms / 1000.0, args.log_mode == "full", logger.log)
            finally:
                logger.close()
            if args.monitor:
                monitor_uart(connection, args.monitor_baud, remaining)
        return 0
    except KeyboardInterrupt:
        print("\n[STOP] Interrupted by user.", flush=True)
        return 130
    except (FlashError, OSError, ValueError) as error:
        print(f"[FAIL] {error}", file=sys.stderr, flush=True)
        return 1
    finally:
        if connection is not None:
            connection.close()


if __name__ == "__main__":
    sys.exit(main())
