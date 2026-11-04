#!/usr/bin/env python3
"""
neon_mdf.py - build, flash, monitor and configure neon-mdf applications.

Run from an application folder containing CMakePresets.json, for example:
    cd examples/Blinky
    python3 /path/to/neon-mdf/tools/neon_mdf.py -t WEACT_F412 build flash-jtag monitor

Add neon-mdf/tools to PATH and make this script executable to invoke it directly.
Multiple actions run in order and stop when an action fails:

    build             Configure if needed, then build incrementally with CMake/Ninja.
    flash-jtag        Build and flash bootloader/header/app in one SWD session.
    flash-uart-stm32  Use the STM32 ROM System Memory Bootloader. Connect PB2 to
                      GND and hold B0; the CMake target resets through SWD.
    flash-uart-fu2    Load <app>_image.bin through ROMCODE USART update. Select
                      boot mode 100 and press RESET when the tool is ready.
    flash-ota         Simulate OTA over SWD, writing the OTA image and pending
                      otadata. Requires flashmap.csv; no network transport is used.
    monitor           Open UART logs; Ctrl+] exits.
    menuconfig        Configure Kconfig features.
    set-target        Save -t in build/.neon_mdf_target for later invocations.

FU2 uses its ROMCODE image header and app. Targets with the SDK bootloader use
separate bootloader/header/app packaging. Flash targets are shared SDK CMake
targets; see cmake/flash_targets.cmake and examples/OTA_Demo/.

Target precedence: -t/--target, saved set-target value, then the only available
CMake preset. Serial-port precedence: -p/--port, NEON_MDF_SERIAL_PORT, then
automatic detection. FU2 example: -t FU2 -p /dev/ttyUSB0 -b 19200 flash-uart-fu2.
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
            f"error: not found {presets_file} - please run this command from the PROJECT folder "
            "(e.g. examples/Blinky/), not the repository root directory.",
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
        f"error: no target selected - use -t/--target (targets with: {', '.join(presets)}) "
        "or run 'neon_mdf.py -t <name> set-target' first.",
        file=sys.stderr,
    )
    sys.exit(1)


def save_target(target: str) -> None:
    presets = load_presets()
    if target not in presets:
        print(f"error: target '{target}' is not in CMakePresets.json (targets have: {', '.join(presets)})", file=sys.stderr)
        sys.exit(1)
    state_file = project_root() / TARGET_STATE_FILE
    state_file.parent.mkdir(parents=True, exist_ok=True)
    state_file.write_text(target)
    print(f"saved default target: {target}")


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
    used by both flash-uart-stm32 and monitor, without SYNC/READY probing here
    (the SDK bootloader protocol has no corresponding CMake flash target anymore; see
    flash_targets.cmake). only don't time: if a USB-serial port exists, use it.
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
            "error: not auto that is warped - specified by -p/--port or variable each field NEON_MDF_SERIAL_PORT.",
            file=sys.stderr,
        )
        sys.exit(1)
    print(f"use serial port: {port}")
    return port


def action_build(target: str, _resolved_port) -> None:
    ensure_configured(target)
    run(["cmake", "--build", "--preset", target])


def action_flash_jtag(target: str, _resolved_port) -> None:
    """
    flash both bootloader and app through SWD in one OpenOCD session, using two `program` commands at
    their actual addresses (cmake/flash_targets.cmake). No UART, running bootloader or serial port
    at all. call target CMake shared flash_jtag.
    """
    ensure_configured(target)
    run(["cmake", "--build", "--preset", target, "--target", "flash_jtag"])


def action_flash_stm32(target: str, resolved_port: str) -> None:
    """
    flash both app bootloaders via the actual chip's System Memory Bootloader (ROM of STM32, not
    bootloader from SDK script) via UART - use client from tools/stm32_uart_flash_firmware.py script
    (not stm32flash, confirmed not to be malicious on macOS - see comment in flash_targets.cmake).
    call CMake target for shared flash_uart_stm32.

    Manual preparation: connect PB2 to GND and hold B0 before continuing. The CMake target resets
    through SWD, which was more reliable than the physical Reset button during testing. input()
    waits for the user to complete these physical steps because they cannot be automated.
    cannot automatically execute this hand operation.
    """
    ensure_configured(target)
    input(
        "\n=== CONNECT PB2->GND + HOLD B0 ON THE BOARD (no Reset button needed - "
        "automatic reset via SWD) ===\nPress Enter when ready..."
    )
    env = dict(os.environ, NEON_MDF_SERIAL_PORT=resolved_port)
    run(["cmake", "--build", "--preset", target, "--target", "flash_uart_stm32"], env=env)
    print(
        "\nFlashing complete. Release B0, disconnect PB2 from GND, then reset to run the app."
    )


def action_flash_ota(target: str, _resolved_port) -> None:
    """
    OTA iteration price via SWD (write image + otadata pending on slot "ota") - calls the shared CMake
    target flash_ota (exists only with app with flashmap.csv, see cmake/flash_targets.cmake
    neon_mdf_add_ota_flash_target() + examples/OTA_Demo/). No serial port required (write via SWD,
    same as flash_jtag).
    """
    ensure_configured(target)
    run(["cmake", "--build", "--preset", target, "--target", "flash_ota"])


def action_flash_fu2(target: str, resolved_port: str, baud: int) -> None:
    if target != "FU2":
        print("error: flash-uart-fu2 only for FU2 target.", file=sys.stderr)
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
        description="neon-mdf CLI, similar to idf.py: build/flash/monitor/menuconfig.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__,
    )
    parser.add_argument("-t", "--target", help="Target (vd WEACT_F412).")
    parser.add_argument("-p", "--port", help="Serial port, e.g. /dev/cu.usbmodem102.")
    parser.add_argument("-b", "--baud", type=int, help="Baud ROMCODE FU2; or NEON_MDF_SERIAL_BAUD. no baud change seen on the chip.")
    parser.add_argument("--monitor-baud", type=int, help="Baud UART uses FU2 (default 19200), read repeatedly with baud flash.")
    parser.add_argument("actions", nargs="+", choices=ALL_ACTIONS, help="action (can write multiple, run sequentially).")
    args = parser.parse_args()

    if "set-target" in args.actions:
        if not args.target:
            print("error: 'set-target' requires -t/--target to be included, e.g. neon_mdf.py -t WEACT_F412 set-target", file=sys.stderr)
            sys.exit(1)
        save_target(args.target)
        args.actions = [a for a in args.actions if a != "set-target"]
        if not args.actions:
            return

    target = resolve_target(args.target)

    baud = args.baud
    if "flash-uart-fu2" in args.actions:
        if target != "FU2":
            parser.error("flash-uart-fu2 for target FU2 only")
        try:
            baud = baud if baud is not None else int(os.environ.get("NEON_MDF_SERIAL_BAUD", ""))
        except ValueError:
            parser.error("baud rate ROMCODE FU2 by -b/--baud or NEON_MDF_SERIAL_BAUD")
        if baud <= 0:
            parser.error("baud must be a sugar integer")
    if args.monitor_baud is not None and args.monitor_baud <= 0:
        parser.error("monitor-baud must be a line integer")

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
