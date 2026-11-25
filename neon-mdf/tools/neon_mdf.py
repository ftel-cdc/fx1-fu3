#!/usr/bin/env python3
"""Build, flash and monitor GT32U2086 applications.

Run from a directory containing CMakePresets.json:
    python3 /path/to/neon-mdf/tools/neon_mdf.py build
    python3 /path/to/neon-mdf/tools/neon_mdf.py -p /dev/ttyUSB0 -b 19200 flash-uart-gt32u2086 monitor

Select boot mode 100 and press RESET after the UART loader is ready.
Programming baud must match ROMCODE. Monitor baud defaults to 19200 and
can be set separately with --monitor-baud. Ctrl+C exits the monitor.
"""
import argparse
import json
import os
import subprocess
import sys
from pathlib import Path

TARGET_STATE_FILE = "build/.neon_mdf_target"


def sdk_version() -> str:
    version_file = Path(__file__).resolve().parents[1] / "VERSION"
    try:
        return version_file.read_text(encoding="utf-8").strip() or "unknown"
    except (OSError, UnicodeError):
        return "unknown"


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
    """Select the only USB serial port, without sending probe bytes."""
    try:
        import serial.tools.list_ports
    except ImportError:
        # Explicit -p/NEON_MDF_SERIAL_PORT remains dependency-free for GT32U2086 POSIX.
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
            "error: cannot select a unique serial port; pass -p/--port or NEON_MDF_SERIAL_PORT.",
            file=sys.stderr,
        )
        sys.exit(1)
    print(f"use serial port: {port}")
    return port


def action_build(target: str, _resolved_port) -> None:
    ensure_configured(target)
    run(["cmake", "--build", "--preset", target])


def action_flash_gt32u2086(target: str, resolved_port: str, baud: int) -> None:
    if target != "GT32U2086":
        print("error: flash-uart-gt32u2086 only for GT32U2086 target.", file=sys.stderr)
        sys.exit(1)
    ensure_configured(target)
    env = dict(os.environ, NEON_MDF_SERIAL_PORT=resolved_port,
               NEON_MDF_SERIAL_BAUD=str(baud))
    run(["cmake", "--build", "--preset", target, "--target", "flash_uart_gt32u2086"], env=env)


def action_monitor(target: str, resolved_port: str, monitor_baud=None) -> None:
    ensure_configured(target)
    env = dict(os.environ, NEON_MDF_SERIAL_PORT=resolved_port)
    if monitor_baud is not None:
        env["NEON_MDF_MONITOR_BAUD"] = str(monitor_baud)
    run(["cmake", "--build", "--preset", target, "--target", "serial_monitor"], env=env)


def action_menuconfig(target: str, _resolved_port) -> None:
    ensure_configured(target)
    run(["cmake", "--build", "--preset", target, "--target", "menuconfig"])


ALL_ACTIONS = ["build", "flash-uart-gt32u2086", "monitor", "menuconfig", "set-target"]


def main() -> None:
    parser = argparse.ArgumentParser(
        description="neon-mdf CLI, similar to idf.py: build/flash/monitor/menuconfig.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__,
    )
    parser.add_argument("--version", action="version", version=f"NEON-MDF SDK {sdk_version()}")
    parser.add_argument("-t", "--target", choices=["GT32U2086"], help="Device preset (GT32U2086).")
    parser.add_argument("-p", "--port", help="Serial port, e.g. /dev/cu.usbmodem102.")
    parser.add_argument("-b", "--baud", type=int, help="Programming baud matching ROMCODE; alternatively NEON_MDF_SERIAL_BAUD.")
    parser.add_argument("--monitor-baud", type=int, help="Application UART baud (default 19200), independent of programming baud.")
    parser.add_argument("actions", nargs="+", choices=ALL_ACTIONS, help="action (can write multiple, run sequentially).")
    args = parser.parse_args()

    if "set-target" in args.actions:
        if not args.target:
            print("error: 'set-target' requires -t/--target to be included, e.g. neon_mdf.py -t GT32U2086 set-target", file=sys.stderr)
            sys.exit(1)
        save_target(args.target)
        args.actions = [a for a in args.actions if a != "set-target"]
        if not args.actions:
            return

    target = resolve_target(args.target)
    if target != "GT32U2086":
        parser.error("Only GT32U2086 is supported; select -t GT32U2086 to replace a stale target")

    baud = args.baud
    if "flash-uart-gt32u2086" in args.actions:
        if target != "GT32U2086":
            parser.error("flash-uart-gt32u2086 for target GT32U2086 only")
        try:
            baud = baud if baud is not None else int(os.environ.get("NEON_MDF_SERIAL_BAUD", ""))
        except ValueError:
            parser.error("baud rate ROMCODE GT32U2086 by -b/--baud or NEON_MDF_SERIAL_BAUD")
        if baud <= 0:
            parser.error("baud must be a positive integer")
    if args.monitor_baud is not None and args.monitor_baud <= 0:
        parser.error("monitor-baud must be a positive integer")

    needs_port = any(a in args.actions for a in ("flash-uart-gt32u2086", "monitor"))
    resolved_port = None
    if needs_port:
        resolved_port = resolve_port(args.port)

    for action in args.actions:
        if action == "build":
            action_build(target, resolved_port)
        elif action == "flash-uart-gt32u2086":
            action_flash_gt32u2086(target, resolved_port, baud)
        elif action == "monitor":
            action_monitor(target, resolved_port, args.monitor_baud)
        elif action == "menuconfig":
            action_menuconfig(target, resolved_port)


if __name__ == "__main__":
    main()
