#!/usr/bin/env python3
"""
neon_mdf.py - CLI build/flash/monitor cho neon-mdf, giong idf.py cua ESP-IDF.

Chay TU THU MUC PROJECT (co CMakePresets.json, vd examples/Blinky/),
giong idf.py:
    cd examples/Blinky
    python3 /duong/dan/toi/neon-mdf/tools/neon_mdf.py -t WEACT_F412 build flash-jtag monitor

De khoi phai go duong dan day moi lan, them neon-mdf/tools vao PATH 1 lan
(vd trong .zshrc):
    export PATH="$PATH:/duong/dan/toi/neon-mdf/tools"
roi chi can: neon_mdf.py build flash-jtag monitor  (nho chmod +x file nay truoc)

Hanh dong (co the ghi NHIEU hanh dong trong 1 lenh, chay TUAN TU, dung ngay
neu 1 buoc that bai). FU2 dung ROMCODE va image header + app; cac target
co bootloader SDK dung quy trinh bootloader + header + app rieng:
    build            - configure (neu build dir chua co) + build (CA bootloader
                       lan app - incremental, chi compile lai file thuc su doi,
                       hanh vi goc cua CMake/Ninja)
    flash-jtag       - build + nap TRON BO [bootloader+header+app] qua SWD
                       trong 1 PHIEN openocd (khong can UART/bootloader dang
                       chay/cong serial gi ca) - khong can thao tac tay - dung
                       mac dinh khi da co dau SWD cam san
    flash-uart-stm32 - build + nap TRON BO [bootloader+header+app] qua System
                       Memory Bootloader THAT cua chip (ROM cua STM32, KHONG
                       PHAI bootloader tu viet) qua UART - BAT BUOC THAO TAC
                       TAY: noi day PB2->GND + giu nut B0 + reset TRUOC khi
                       tiep tuc (script se cho Enter) - dung khi khong co dau
                       SWD, chi co UART
    flash-uart-fu2   - build + nap <app>_image.bin qua ROMCODE USART update.
                       Chon boot mode 100, doi tool san sang roi bam RESET.
                       Port/baud truyen luc chay: -p /dev/ttyUSB0 -b 19200,
                       hoac NEON_MDF_SERIAL_PORT / NEON_MDF_SERIAL_BAUD.
    flash-ota        - GIA LAP OTA qua SWD (ghi anh vao slot "ota" + otadata
                       pending, KHONG dung UART/mang that - xem
                       cmake/flash_targets.cmake, neon_mdf_add_ota_flash_target())
                       - CHI dung duoc voi app CO flashmap.csv (xem
                       examples/OTA_Demo/), loi ro rang neu khong co
    monitor          - mo serial monitor doc log UART (Ctrl+] de thoat)
    menuconfig       - mo UI chon tinh nang (Kconfig)
    set-target       - luu target mac dinh cho project nay (ghi vao
                       build/.neon_mdf_target) - can di kem -t, lan sau khong
                       can -t nua

Chon target: -t/--target > build/.neon_mdf_target (tu 'set-target') > tu
dong neu CMakePresets.json chi co DUNG 1 target.

Chon cong serial: -p/--port > bien moi truong NEON_MDF_SERIAL_PORT > tu dong
do (ap dung cho cac action UART va monitor).
"""
import argparse
import json
import os
import subprocess
import sys
from pathlib import Path

TARGET_STATE_FILE = "build/.neon_mdf_target"


def project_root() -> Path:
    return Path.cwd()


def load_presets() -> list:
    presets_file = project_root() / "CMakePresets.json"
    if not presets_file.exists():
        print(
            f"Loi: khong tim thay {presets_file} - hay chay lenh nay TU THU MUC PROJECT "
            "(vd examples/Blinky/), khong phai thu muc goc repo.",
            file=sys.stderr,
        )
        sys.exit(1)

    with open(presets_file) as f:
        root = json.load(f)

    names = [p["name"] for p in root.get("configurePresets", []) if not p.get("hidden")]
    for rel in root.get("include", []):
        include_file = project_root() / rel
        if include_file.exists():
            with open(include_file) as f:
                frag = json.load(f)
            for p in frag.get("configurePresets", []):
                if not p.get("hidden") and p["name"] not in names:
                    names.append(p["name"])
    return names


def resolve_target(cli_target) -> str:
    if cli_target:
        return cli_target

    state_file = project_root() / TARGET_STATE_FILE
    if state_file.exists():
        return state_file.read_text().strip()

    presets = load_presets()
    if len(presets) == 1:
        return presets[0]

    print(
        f"Loi: chua chon target - dung -t/--target (cac target co: {', '.join(presets)}) "
        "hoac chay 'neon_mdf.py -t <ten> set-target' truoc.",
        file=sys.stderr,
    )
    sys.exit(1)


def save_target(target: str) -> None:
    presets = load_presets()
    if target not in presets:
        print(f"Loi: target '{target}' khong co trong CMakePresets.json (cac target co: {', '.join(presets)})", file=sys.stderr)
        sys.exit(1)
    state_file = project_root() / TARGET_STATE_FILE
    state_file.parent.mkdir(parents=True, exist_ok=True)
    state_file.write_text(target)
    print(f"Da luu target mac dinh: {target}")


def run(cmd, **kwargs) -> None:
    print(f"$ {' '.join(cmd)}")
    result = subprocess.run(cmd, **kwargs)
    if result.returncode != 0:
        sys.exit(result.returncode)


def ensure_configured(target: str) -> None:
    build_dir = project_root() / "build" / target
    if not (build_dir / "CMakeCache.txt").exists():
        run(["cmake", "--preset", target])


def detect_port_simple() -> str:
    """
    Dung cho ca 'flash-uart-stm32' lan 'monitor' - KHONG dung giao thuc SYNC/READY
    o day (do la protocol rieng cua bootloader TU VIET cua SDK, khong con
    target CMake nao goi qua UART nua - xem flash_targets.cmake). Chi don
    gian: neu DUNG 1 cong USB-serial ton tai thi dung cong do.
    """
    try:
        import serial.tools.list_ports
    except ImportError:
        # Explicit -p/NEON_MDF_SERIAL_PORT remains dependency-free for FU2 POSIX.
        return None

    candidates = [p.device for p in serial.tools.list_ports.comports() if p.vid is not None]
    if len(candidates) == 1:
        return candidates[0]
    return None


def resolve_port(cli_port) -> str:
    if cli_port:
        return cli_port
    if os.environ.get("NEON_MDF_SERIAL_PORT"):
        return os.environ["NEON_MDF_SERIAL_PORT"]

    port = detect_port_simple()

    if not port:
        print(
            "Loi: khong tu dong do duoc cong nao - chi dinh bang -p/--port hoac bien moi truong NEON_MDF_SERIAL_PORT.",
            file=sys.stderr,
        )
        sys.exit(1)
    print(f"Dung cong serial: {port}")
    return port


def action_build(target: str, _resolved_port) -> None:
    ensure_configured(target)
    run(["cmake", "--build", "--preset", target])


def action_flash_jtag(target: str, _resolved_port) -> None:
    """
    Nap CA bootloader lan app qua SWD trong 1 PHIEN openocd duy nhat (2 lenh
    `program` rieng, dia chi that - xem cmake/flash_targets.cmake) - KHONG
    can UART/bootloader dang chay/cong serial gi ca. Goi target CMake dung
    chung flash_jtag.
    """
    ensure_configured(target)
    run(["cmake", "--build", "--preset", target, "--target", "flash_jtag"])


def action_flash_stm32(target: str, resolved_port: str) -> None:
    """
    Nap CA bootloader LAN app qua System Memory Bootloader THAT cua chip
    (ROM cua STM32, KHONG PHAI bootloader tu viet cua SDK) qua UART - dung
    client tu viet tools/stm32_uart_flash_firmware.py (KHONG dung stm32flash,
    da xac nhan khong dang tin cay tren macOS - xem comment trong
    flash_targets.cmake). Goi target CMake dung chung flash_uart_stm32.

    CHI 1 THAO TAC TAY (khong can bam nut Reset - target CMake TU RESET qua
    SWD, da xac nhan dang tin cay hon han bam nut Reset vat ly): noi day
    PB2->GND + giu nut B0 tren board TRUOC khi tiep tuc - action nay CHO
    NGUOI DUNG XAC NHAN qua input() truoc khi chay, vi khong the tu dong
    hoa thao tac tay nay.
    """
    ensure_configured(target)
    input(
        "\n=== NOI DAY PB2->GND + GIU NUT B0 TREN BOARD (KHONG can bam nut Reset - "
        "tu dong reset qua SWD) ===\nBam Enter khi da san sang..."
    )
    env = dict(os.environ, NEON_MDF_SERIAL_PORT=resolved_port)
    run(["cmake", "--build", "--preset", target, "--target", "flash_uart_stm32"], env=env)
    print(
        "\nNap xong - nho THA nut B0 + RUT day PB2->GND, roi reset lai de chay app that."
    )


def action_flash_ota(target: str, _resolved_port) -> None:
    """
    GIA LAP OTA qua SWD (ghi anh + otadata pending vao slot "ota") - goi
    target CMake dung chung flash_ota (chi ton tai voi app CO flashmap.csv,
    xem cmake/flash_targets.cmake neon_mdf_add_ota_flash_target() +
    examples/OTA_Demo/). Khong can port serial (ghi qua SWD, giong flash_jtag).
    """
    ensure_configured(target)
    run(["cmake", "--build", "--preset", target, "--target", "flash_ota"])


def action_flash_fu2(target: str, resolved_port: str, baud: int) -> None:
    if target != "FU2":
        print("Loi: flash-uart-fu2 chi dung cho target FU2.", file=sys.stderr)
        sys.exit(1)
    ensure_configured(target)
    env = dict(os.environ, NEON_MDF_SERIAL_PORT=resolved_port,
               NEON_MDF_SERIAL_BAUD=str(baud))
    run(["cmake", "--build", "--preset", target, "--target", "flash_uart_fu2"], env=env)


def action_monitor(target: str, resolved_port: str, monitor_baud=None) -> None:
    ensure_configured(target)
    env = dict(os.environ, NEON_MDF_SERIAL_PORT=resolved_port)
    if monitor_baud is not None:
        env["NEON_MDF_MONITOR_BAUD"] = str(monitor_baud)
    run(["cmake", "--build", "--preset", target, "--target", "serial_monitor"], env=env)


def action_menuconfig(target: str, _resolved_port) -> None:
    ensure_configured(target)
    run(["cmake", "--build", "--preset", target, "--target", "menuconfig"])


ALL_ACTIONS = ["build", "flash-jtag", "flash-uart-stm32", "flash-uart-fu2", "flash-ota", "monitor", "menuconfig", "set-target"]


def main() -> None:
    parser = argparse.ArgumentParser(
        description="neon-mdf CLI (giong idf.py) - build/flash/monitor/menuconfig.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__,
    )
    parser.add_argument("-t", "--target", help="Target (vd WEACT_F412).")
    parser.add_argument("-p", "--port", help="Cong serial (vd /dev/cu.usbmodem102).")
    parser.add_argument("-b", "--baud", type=int, help="Baud ROMCODE FU2; hoac NEON_MDF_SERIAL_BAUD. Khong thay doi baud tren chip.")
    parser.add_argument("--monitor-baud", type=int, help="Baud UART ung dung FU2 (mac dinh 19200), doc lap voi baud nap.")
    parser.add_argument("actions", nargs="+", choices=ALL_ACTIONS, help="Hanh dong (co the ghi nhieu, chay tuan tu).")
    args = parser.parse_args()

    if "set-target" in args.actions:
        if not args.target:
            print("Loi: 'set-target' can -t/--target di kem, vd: neon_mdf.py -t WEACT_F412 set-target", file=sys.stderr)
            sys.exit(1)
        save_target(args.target)
        args.actions = [a for a in args.actions if a != "set-target"]
        if not args.actions:
            return

    target = resolve_target(args.target)

    baud = args.baud
    if "flash-uart-fu2" in args.actions:
        if target != "FU2":
            parser.error("flash-uart-fu2 chi dung cho target FU2")
        try:
            baud = baud if baud is not None else int(os.environ.get("NEON_MDF_SERIAL_BAUD", ""))
        except ValueError:
            parser.error("truyen baud ROMCODE FU2 bang -b/--baud hoac NEON_MDF_SERIAL_BAUD")
        if baud <= 0:
            parser.error("baud phai la so nguyen duong")
    if args.monitor_baud is not None and args.monitor_baud <= 0:
        parser.error("monitor-baud phai la so nguyen duong")

    needs_port = any(a in args.actions for a in ("flash-uart-stm32", "flash-uart-fu2", "monitor"))
    resolved_port = None
    if needs_port:
        resolved_port = resolve_port(args.port)

    for action in args.actions:
        if action == "build":
            action_build(target, resolved_port)
        elif action == "flash-jtag":
            action_flash_jtag(target, resolved_port)
        elif action == "flash-uart-stm32":
            action_flash_stm32(target, resolved_port)
        elif action == "flash-uart-fu2":
            action_flash_fu2(target, resolved_port, baud)
        elif action == "flash-ota":
            action_flash_ota(target, resolved_port)
        elif action == "monitor":
            action_monitor(target, resolved_port, args.monitor_baud)
        elif action == "menuconfig":
            action_menuconfig(target, resolved_port)


if __name__ == "__main__":
    main()
