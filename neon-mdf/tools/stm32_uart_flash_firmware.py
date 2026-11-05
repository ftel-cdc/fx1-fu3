#!/usr/bin/env python3
"""
stm32_uart_flash_firmware.py implements the STM32 ROM System Memory Bootloader UART protocol (ST
AN3155/AN4221), rather than the SDK protocol in
components/bootloader/neon_bootloader_main.c). Replace the stm32flash - tool with a long
previously. It was unreliable with the tested board/macOS driver combination: it sent
synchronization byte 0x7F once, although synchronization should be retried until the bootloader is
run through a manual test (Python retry loop creates a tool, stm32flash creates that "Failed
to init device").

Requires BOOT0=1 and BOOT1=0 at reset. On STM32F103CB, the ROM bootloader UART is USART1:
PA9=TX, PA10=RX. On the WeAct STM32F412 board, BOOT1/PB2 is shared with the LED and can float, so
connect PB2 temporarily to GND.

Script read the command list using GET (0x00) and select erase command:
  - Erase 0x43 for old bootloader like STM32F103 medium-density USART V2.2.
  - Extended Erase 0x44 for each bootloader as STM32F412.

Use: stm32_uart_flash_firmware.py <serial_port> <file1.bin> <addr1_hex> [<file2.bin> <addr2_hex>...]

write multiple discontiguous regions in one session: synchronize once, mass erase once, then issue
consecutive Write Memory operations, instead of combining both regions into one contiguous .bin file
assume adjacent regions. Removed in 2026: each region address now comes directly from flashmap.csv
through cmake/flash_targets.cmake, not children assume "app male right after bootloader"). Vi true
enough (2 regions: bootloader + app):
  stm32_uart_flash_firmware.py /dev/tty.usbmodemXXXX \\
      bootloader.bin 0x08000000 blinky_blob.bin 0x08008000
"""
import struct
import sys
import time

SYNC_BYTE = 0x7F
ACK_BYTE = 0x79
NACK_BYTE = 0x1F

CMD_GET = 0x00
CMD_GET_VERSION = 0x01
CMD_GET_ID = 0x02
CMD_ERASE = 0x43
CMD_EXTENDED_ERASE = 0x44
CMD_WRITE_MEMORY = 0x31
CMD_GO = 0x21

WRITE_CHUNK_SIZE = 256
# no FLASH_BASE numbers with specified - dia indicating real regions via instruction parameter (see
# _parse_args()/main()), derived from flashmap.csv via
# cmake/flash_targets.cmake.

SYNC_RETRY_S = 9.5
SYNC_POLL_S = 0.1
ERASE_TIMEOUT_S = 30.0
WRITE_TIMEOUT_S = 5.0


def checksum_xor(data: bytes) -> int:
    x = 0
    for b in data:
        x ^= b
    return x


def send_command(ser, cmd: int) -> None:
    ser.write(bytes([cmd, cmd ^ 0xFF]))


def recv_ack(ser, what: str, timeout_s: float = WRITE_TIMEOUT_S) -> None:
    old_timeout = ser.timeout
    ser.timeout = timeout_s
    try:
        b = ser.read(1)
    finally:
        ser.timeout = old_timeout
    if len(b) != 1:
        raise RuntimeError(f"Timeout exchanges ACK for '{what}' (received nothing)")
    if b[0] == NACK_BYTE:
        raise RuntimeError(f"Bootloader look up NACK for '{what}'")
    if b[0] != ACK_BYTE:
        raise RuntimeError(f"Unexpected byte (0x{b[0]:02X}) see because ACK for '{what}'")


def sync(ser) -> None:
    """
    Repeat SYNC_BYTE with SYNC_POLL_S between attempts until the first response. Stop sending SYNC as
    soon as a reply arrives: the bootloader has no resynchronization mechanism, and extra SYNC bytes
    would corrupt the transaction.
    """
    ser.reset_input_buffer()
    old_timeout = ser.timeout
    ser.timeout = SYNC_POLL_S
    deadline = time.time() + SYNC_RETRY_S
    resp = None
    last_unexpected = None
    try:
        while time.time() < deadline:
            ser.write(bytes([SYNC_BYTE]))
            r = ser.read(1)
            if len(r) == 1:
                b = r[0]
                if b == ACK_BYTE or b == NACK_BYTE:
                    # Return immediately on a valid ACK/NACK. Further SYNC bytes would disrupt the bootloader protocol.
                    resp = b
                    break
                # Ignore bytes other than ACK/NACK. Hardware testing showed transient bytes immediately
                # after UART/clock each on specified after reset (unlike the protocol from the SDK
                # writing. The System Bootloader remains synchronized when receiving
                # additional SYNC bytes - no only check error ACK/NACK for each command don le).
                # retry SYNC until ACK/NACK arrives or the timeout expires.
                last_unexpected = b
    finally:
        ser.timeout = old_timeout

    if resp is None:
        extra = f" (byte is ultimately received: 0x{last_unexpected:02X})" if last_unexpected is not None else ""
        raise RuntimeError(
            f"not getting ACK/NACK from System Bootloader in {SYNC_RETRY_S}s{extra} - "
            "test: have we PB2->GND, keep B0, RESET used?"
        )
    if resp != ACK_BYTE:
        raise RuntimeError(f"Bootloader returned NACK (0x{NACK_BYTE:02X}) for SYNC")


def get_version(ser) -> int:
    send_command(ser, CMD_GET_VERSION)
    recv_ack(ser, "Get Version (cmd)")
    data = ser.read(3)
    if len(data) != 3:
        raise RuntimeError("unable to read 3 bytes from Get Version")
    recv_ack(ser, "Get Version (data)")
    return data[0]


def get_supported_commands(ser):
    """look up (bootloader_version, set(command)) from GET command 0x00."""
    send_command(ser, CMD_GET)
    recv_ack(ser, "Get (cmd)")
    n = ser.read(1)
    if len(n) != 1:
        raise RuntimeError("can't read that long command list from Get")
    payload = ser.read(n[0] + 1)
    if len(payload) != n[0] + 1:
        raise RuntimeError("cannot read full command list from Get")
    recv_ack(ser, "Get (data)")
    if len(payload) < 2:
        raise RuntimeError("Payload Get has no logical command list")
    return payload[0], set(payload[1:])


def get_id(ser) -> int:
    send_command(ser, CMD_GET_ID)
    recv_ack(ser, "Get ID (cmd)")
    n = ser.read(1)
    if len(n) != 1:
        raise RuntimeError("cannot read that long from Get ID")
    pid_bytes = ser.read(n[0] + 1)
    if len(pid_bytes) != n[0] + 1:
        raise RuntimeError("cannot read enough PID bytes from Get ID")
    recv_ack(ser, "Get ID (data)")
    return int.from_bytes(pid_bytes, "big")


def mass_erase(ser, supported_commands) -> int:
    if CMD_EXTENDED_ERASE in supported_commands:
        send_command(ser, CMD_EXTENDED_ERASE)
        recv_ack(ser, "Extended Erase (cmd)")
        payload = bytes([0xFF, 0xFF])
        ser.write(payload + bytes([checksum_xor(payload)]))
        recv_ack(ser, "Extended Erase (mass erase)", timeout_s=ERASE_TIMEOUT_S)
        return CMD_EXTENDED_ERASE

    if CMD_ERASE in supported_commands:
        send_command(ser, CMD_ERASE)
        recv_ack(ser, "Erase (cmd)")
        # AN3155 separately specifies the global erase of the 0x43 command as 0xFF, 0x00.
        ser.write(bytes([0xFF, 0x00]))
        recv_ack(ser, "Erase (mass erase)", timeout_s=ERASE_TIMEOUT_S)
        return CMD_ERASE

    advertised = ", ".join(f"0x{cmd:02X}" for cmd in sorted(supported_commands))
    raise RuntimeError(
        "System Bootloader does not support Erase 0x43 or Extended Erase 0x44 "
        f"(commands: {advertised})"
    )


def write_memory_chunk(ser, address: int, chunk: bytes) -> None:
    assert 1 <= len(chunk) <= 256
    send_command(ser, CMD_WRITE_MEMORY)
    recv_ack(ser, f"Write Memory @0x{address:08X} (cmd)")

    addr_bytes = struct.pack(">I", address)
    ser.write(addr_bytes + bytes([checksum_xor(addr_bytes)]))
    recv_ack(ser, f"Write Memory @0x{address:08X} (address)")

    n_minus_1 = len(chunk) - 1
    body = bytes([n_minus_1]) + chunk
    ser.write(body + bytes([checksum_xor(body)]))
    recv_ack(ser, f"Write Memory @0x{address:08X} (data)", timeout_s=WRITE_TIMEOUT_S)


def go(ser, address: int) -> None:
    send_command(ser, CMD_GO)
    recv_ack(ser, "Command")
    addr_bytes = struct.pack(">I", address)
    ser.write(addr_bytes + bytes([checksum_xor(addr_bytes)]))
    recv_ack(ser, "Address")


def _parse_args(argv):
    # argv[0] = serial_port, subsection is level (file, dia_chi_hex) in a row - need >=1 level, number of
    # sections from subsection to foot.
    if len(argv) < 3 or (len(argv) - 1) % 2 != 0:
        print(
            f"Used: {sys.argv[0]} <serial_port> <file1.bin> <addr1_hex> [<file2.bin> <addr2_hex> ...]",
            file=sys.stderr,
        )
        sys.exit(1)
    port = argv[0]
    regions = []
    rest = argv[1:]
    for i in range(0, len(rest), 2):
        path, addr_hex = rest[i], rest[i + 1]
        try:
            addr = int(addr_hex, 16)
        except ValueError:
            print(f"Dia just '{addr_hex}' is not a hex number", file=sys.stderr)
            sys.exit(1)
        with open(path, "rb") as f:
            data = f.read()
        if len(data) == 0:
            print(f"File '{path}' Empty", file=sys.stderr)
            sys.exit(1)
        regions.append((path, addr, data))
    return port, regions


def main():
    port, regions = _parse_args(sys.argv[1:])

    try:
        import serial
    except ImportError:
        print("pyserial - run missing: pip install pyserial", file=sys.stderr)
        sys.exit(1)

    ser = serial.Serial(port, baudrate=57600, parity=serial.PARITY_EVEN, stopbits=1, bytesize=8, timeout=5.0)
    try:
        print("Synchronizing with System Bootloader by retrying 0x7F...")
        sync(ser)
        print("ACK has been activated. Read Get + Get Version + Get ID to confirm the connection...")

        get_version_value, supported_commands = get_supported_commands(ser)
        command_text = " ".join(f"0x{cmd:02X}" for cmd in sorted(supported_commands))
        print(
            f"  GET version: {get_version_value >> 4}.{get_version_value & 0xF}; "
            f"commands: {command_text}"
        )

        version = get_version(ser)
        print(f"  Bootloader version: {version >> 4}.{version & 0xF}")

        pid = get_id(ser)
        print(f"  Product ID: 0x{pid:04X}")

        total_bytes = sum(len(data) for _, _, data in regions)
        print(f"{len(regions)} regions, {total_bytes} total bytes. is erasing the entire flash (mass erase, may take a few seconds)...")
        erase_command = mass_erase(ser, supported_commands)
        print(f"Erase completed using command 0x{erase_command:02X}.")

        # Mass erase has erased the entire chip once - write each region ACCORDING to the real dia of no,
        # without assuming adjacent regions or combining them into one long
        # continuous .bin file fills the whole flash - microprocessor step that turns on the regions
        # must be in the correct order specified in flashmap.csv, see cmake/flash_targets.cmake).
        for path, base_addr, data in regions:
            print(f"writing '{path}' ({len(data)} bytes) at 0x{base_addr:08X}...")
            total_chunks = (len(data) + WRITE_CHUNK_SIZE - 1) // WRITE_CHUNK_SIZE
            offset = 0
            chunk_index = 0
            while offset < len(data):
                chunk = data[offset:offset + WRITE_CHUNK_SIZE]
                write_memory_chunk(ser, base_addr + offset, chunk)
                chunk_index += 1
                offset += len(chunk)
                print(f"  chunk {chunk_index}/{total_chunks} OK", end="\r")
            print()

        print("Flashing complete. Release B0, disconnect PB2 from GND, then reset to run the app.")
    finally:
        ser.close()


if __name__ == "__main__":
    main()
