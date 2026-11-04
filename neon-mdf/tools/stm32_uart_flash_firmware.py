#!/usr/bin/env python3
"""
stm32_uart_flash_firmware.py - client TU VIET cho protocol UART cua System
Memory Bootloader THAT cua STM32 (ROM, AN3155/AN4221 cua ST - KHONG PHAI
protocol tu viet cua SDK trong components/bootloader/neon_bootloader_main.c).
Thay the stm32flash - cong
cu chuan nhung xac nhan KHONG DANG TIN CAY tren combo board/driver macOS cu
the nay: no chi thu bat tay (gui byte 0x7F) DUNG 1 LAN, trong khi thuc te
can retry (thu nhieu lan) moi bat duoc dung luc bootloader san sang - da xac
nhan qua test tay (Python retry loop luon thanh cong, stm32flash luon that
bai "Failed to init device").

Yeu cau BOOT0=1 VA BOOT1=0 luc reset. Voi STM32F103CB, UART cua ROM
bootloader la USART1: PA9=TX, PA10=RX. Voi board WeAct STM32F412, BOOT1
(PB2, trung voi chan LED) bi tha noi nen phai noi PB2->GND tam thoi.

Script doc danh sach command bang GET (0x00) va tu chon lenh xoa:
  - Erase 0x43 cho bootloader cu nhu STM32F103 medium-density USART V2.2.
  - Extended Erase 0x44 cho bootloader moi nhu STM32F412.

Dung: stm32_uart_flash_firmware.py <serial_port> <file1.bin> <addr1_hex> [<file2.bin> <addr2_hex> ...]

Ghi NHIEU vung KHONG LIEN TUC trong CUNG 1 phien (1 lan dong bo + 1 lan mass
erase, nhieu vong ghi Write Memory noi tiep) - THAY vi ghep tat ca thanh 1
file .bin lien tuc gia dinh cac vung nam sat nhau (bo di 2026 - dia chi tung
vung lay THAT tu flashmap.csv qua cmake/flash_targets.cmake, khong con gia
dinh "app nam ngay sau bootloader"). Vi du that (2 vung: bootloader + app):
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
# KHONG con hang so FLASH_BASE co dinh - dia chi tung vung truyen THAT qua
# tham so dong lenh (xem _parse_args()/main()), lay tu flashmap.csv qua
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
        raise RuntimeError(f"Timeout doi ACK cho '{what}' (khong nhan duoc gi)")
    if b[0] == NACK_BYTE:
        raise RuntimeError(f"Bootloader tra ve NACK cho '{what}'")
    if b[0] != ACK_BYTE:
        raise RuntimeError(f"Byte la (0x{b[0]:02X}) thay vi ACK cho '{what}'")


def sync(ser) -> None:
    """
    Gui SYNC_BYTE lap lai (nghi SYNC_POLL_S giua moi lan) toi khi nhan duoc
    BAT KY phan hoi nao - DUNG NGAY khi co phan hoi (dung hay sai), KHONG
    gui them SYNC sau do (bootloader khong co co che resync, gui thua se
    lam lech giao thuc - cung bai hoc da gap voi protocol tu viet cua SDK).
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
                    # DUNG NGAY khi thay ACK/NACK HOP LE - day moi la phan
                    # hoi THAT cua bootloader, gui them SYNC sau do se lam
                    # lech giao thuc (giong bai hoc voi protocol tu viet).
                    resp = b
                    break
                # Byte LA (khong phai ACK/NACK) - BO QUA, KHONG dung lai:
                # da xac nhan THAT tren phan cung day la NHIEU THOANG QUA
                # ngay luc UART/clock moi on dinh sau reset (khac han
                # protocol tu viet cua SDK, System Bootloader KHONG bi lech
                # giao thuc khi nhan them SYNC byte - no chi tra loi ACK/NACK
                # cho tung lenh don le). Tiep tuc gui lai SYNC cho toi khi
                # thay dung ACK/NACK hoac het gio.
                last_unexpected = b
    finally:
        ser.timeout = old_timeout

    if resp is None:
        extra = f" (byte la cuoi cung nhan duoc: 0x{last_unexpected:02X})" if last_unexpected is not None else ""
        raise RuntimeError(
            f"Khong nhan duoc ACK/NACK tu System Bootloader trong {SYNC_RETRY_S}s{extra} - "
            "kiem tra: da noi PB2->GND, giu B0, RESET dung luc chua?"
        )
    if resp != ACK_BYTE:
        raise RuntimeError(f"Bootloader tra ve NACK (0x{NACK_BYTE:02X}) cho SYNC")


def get_version(ser) -> int:
    send_command(ser, CMD_GET_VERSION)
    recv_ack(ser, "Get Version (cmd)")
    data = ser.read(3)
    if len(data) != 3:
        raise RuntimeError("Khong doc du 3 byte tra ve tu Get Version")
    recv_ack(ser, "Get Version (data)")
    return data[0]


def get_supported_commands(ser):
    """Tra ve (bootloader_version, set(command)) tu lenh GET 0x00."""
    send_command(ser, CMD_GET)
    recv_ack(ser, "Get (cmd)")
    n = ser.read(1)
    if len(n) != 1:
        raise RuntimeError("Khong doc duoc do dai danh sach command tu Get")
    payload = ser.read(n[0] + 1)
    if len(payload) != n[0] + 1:
        raise RuntimeError("Khong doc du danh sach command tu Get")
    recv_ack(ser, "Get (data)")
    if len(payload) < 2:
        raise RuntimeError("Payload Get khong co danh sach command hop le")
    return payload[0], set(payload[1:])


def get_id(ser) -> int:
    send_command(ser, CMD_GET_ID)
    recv_ack(ser, "Get ID (cmd)")
    n = ser.read(1)
    if len(n) != 1:
        raise RuntimeError("Khong doc duoc do dai tu Get ID")
    pid_bytes = ser.read(n[0] + 1)
    if len(pid_bytes) != n[0] + 1:
        raise RuntimeError("Khong doc du byte PID tu Get ID")
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
        # AN3155 quy dinh rieng global erase cua command 0x43 la 0xFF, 0x00.
        ser.write(bytes([0xFF, 0x00]))
        recv_ack(ser, "Erase (mass erase)", timeout_s=ERASE_TIMEOUT_S)
        return CMD_ERASE

    advertised = ", ".join(f"0x{cmd:02X}" for cmd in sorted(supported_commands))
    raise RuntimeError(
        "System Bootloader khong cong bo Erase 0x43 hoac Extended Erase 0x44 "
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
    recv_ack(ser, "Go (cmd)")
    addr_bytes = struct.pack(">I", address)
    ser.write(addr_bytes + bytes([checksum_xor(addr_bytes)]))
    recv_ack(ser, "Go (address)")


def _parse_args(argv):
    # argv[0] = serial_port, phan con lai la CAP (file, dia_chi_hex) lien
    # tiep - can >=1 cap, so phan tu con lai PHAI chan.
    if len(argv) < 3 or (len(argv) - 1) % 2 != 0:
        print(
            f"Dung: {sys.argv[0]} <serial_port> <file1.bin> <addr1_hex> [<file2.bin> <addr2_hex> ...]",
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
            print(f"Dia chi '{addr_hex}' khong phai so hex hop le", file=sys.stderr)
            sys.exit(1)
        with open(path, "rb") as f:
            data = f.read()
        if len(data) == 0:
            print(f"File '{path}' rong", file=sys.stderr)
            sys.exit(1)
        regions.append((path, addr, data))
    return port, regions


def main():
    port, regions = _parse_args(sys.argv[1:])

    try:
        import serial
    except ImportError:
        print("Thieu pyserial - chay: pip install pyserial", file=sys.stderr)
        sys.exit(1)

    ser = serial.Serial(port, baudrate=57600, parity=serial.PARITY_EVEN, stopbits=1, bytesize=8, timeout=5.0)
    try:
        print("Dang dong bo voi System Bootloader (gui 0x7F lap lai)...")
        sync(ser)
        print("Da dong bo (ACK). Dang doc Get + Get Version + Get ID de xac nhan ket noi...")

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
        print(f"{len(regions)} vung, {total_bytes} byte tong. Dang xoa toan bo flash (mass erase, co the mat vai giay)...")
        erase_command = mass_erase(ser, supported_commands)
        print(f"Da xoa xong bang command 0x{erase_command:02X}.")

        # Mass erase da xoa TOAN BO chip 1 lan - ghi tung vung THEO DUNG dia
        # chi that cua no, KHONG can gia dinh cac vung nam sat nhau (khac
        # het cach lam cu ghep 1 file .bin lien tuc dai dien toan bo flash -
        # bo di vi buoc do bat cac vung PHAI dung 1 thu tu co dinh trong
        # flashmap.csv, xem cmake/flash_targets.cmake).
        for path, base_addr, data in regions:
            print(f"Dang ghi '{path}' ({len(data)} byte) tai 0x{base_addr:08X}...")
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

        print("Ghi xong. THANH CONG - nho THA nut B0 + rut day PB2->GND, roi reset lai de chay app that.")
    finally:
        ser.close()


if __name__ == "__main__":
    main()
