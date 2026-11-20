#!/usr/bin/env python3
"""Run the VP until the firmware reports inference PASS or FAIL."""
from __future__ import annotations

import os
from pathlib import Path
import re
import signal
import subprocess
import sys

here = Path(__file__).resolve().parent
app_root = here
vp_dir = app_root / "platform/virtual_platform/fx1/vp/bin/VP_FX1_V2.0"
vp_bin = vp_dir / "vp_fx1_full_soc"
model = (app_root / "models/"
         "mobilenet_nano_qat_int8_nchw_native/mobilenet_nano_qat_int8.nsm")
out_dir = Path(os.getenv("NEON_FX1_V2_OUT_DIR",
                         str(app_root / "build/FX1_V2_RUNTIME")))
firmware = out_dir / "neon_graph_vp_v2.elf"
log_path = out_dir / "vp_neon_graph.log"
output_path = out_dir / "output.bin"
output_values = None
output_hash = None
fragment_ends: list[tuple[int, int]] = []

if not vp_bin.is_file() or not model.is_file() or not firmware.is_file():
    print(f"Missing run input: vp={vp_bin} model={model} fw={firmware}", file=sys.stderr)
    sys.exit(2)

env = os.environ.copy()
env["LD_LIBRARY_PATH"] = str(vp_dir) + os.pathsep + env.get("LD_LIBRARY_PATH", "")
cmd = [str(vp_bin), "-c", "default.yaml", "--fw", str(firmware),
       "--int-flash", str(model), "--quantum", os.getenv("VP_QUANTUM", "5000"),
       "--sim-ms", os.getenv("VP_SIM_MS", "20000000")]

log_path.parent.mkdir(parents=True, exist_ok=True)
proc = subprocess.Popen(cmd, cwd=vp_dir, env=env, stdout=subprocess.PIPE,
                        stderr=subprocess.STDOUT, text=True, bufsize=1)
verdict = None
with log_path.open("w", encoding="utf-8") as log:
    assert proc.stdout is not None
    for line in proc.stdout:
        print(line, end="", flush=True)
        log.write(line)
        log.flush()
        match = re.search(r"\[NEON_E2E\] FRAGMENT_END index=(\d+) status=(\d+) bytes=(\d+)", line)
        if match:
            fragment_ends.append((int(match.group(1)), int(match.group(2))))
        match = re.search(
            r"\[NEON_E2E\] OUTPUT_OK addr=0x82000000 bytes=2 values=(-?\d+) (-?\d+) hash=(\d+)",
            line,
        )
        if match:
            output_values = (int(match.group(1)), int(match.group(2)))
            output_hash = int(match.group(3))
        if "[NEON_E2E] PASS " in line:
            verdict = "PASS"
            break
        if "[NEON_E2E] FAIL " in line:
            verdict = "FAIL"
            break

    if verdict is not None and proc.poll() is None:
        proc.send_signal(signal.SIGTERM)
        try:
            proc.wait(timeout=5)
        except subprocess.TimeoutExpired:
            proc.kill()
            proc.wait()

    if verdict is None:
        proc.wait()

if verdict == "PASS":
    expected_fragments = [(i, 0) for i in range(10)]
    if fragment_ends != expected_fragments:
        print(f"Fragment completion sequence mismatch: {fragment_ends}", file=sys.stderr)
        sys.exit(1)
    if output_values is None or output_hash is None:
        print("VP reported PASS without a parseable output marker", file=sys.stderr)
        sys.exit(1)
    output_bytes = bytes(value & 0xFF for value in output_values)
    actual_hash = 2166136261
    for byte in output_bytes:
        actual_hash = ((actual_hash ^ byte) * 16777619) & 0xFFFFFFFF
    if actual_hash != output_hash:
        print(f"Output hash mismatch: UART={output_hash} host={actual_hash}", file=sys.stderr)
        sys.exit(1)
    output_path.write_bytes(output_bytes)
    values_path = out_dir / "output_int8.txt"
    values_path.write_text(
        f"address=0x82000000\nshape=[1,2]\ndtype=int8\nvalues={output_values[0]},{output_values[1]}\n"
        f"fnv1a=0x{output_hash:08x}\n",
        encoding="utf-8",
    )
    print(f"Saved output bytes from DDR 0x82000000 to {output_path}")
    print(f"Decoded int8 output: {output_values}; details: {values_path}")
    print(f"All 10 fragments completed; VP log: {log_path}")
    sys.exit(0)
print(f"VP did not report inference PASS (verdict={verdict}, exit={proc.returncode}); log: {log_path}",
      file=sys.stderr)
sys.exit(1)
