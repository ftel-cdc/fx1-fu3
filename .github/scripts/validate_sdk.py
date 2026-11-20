#!/usr/bin/env python3
"""Build and validate the SDK without changing example configurations or using a board."""
import argparse
from concurrent.futures import ThreadPoolExecutor
import json
import re
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile

REPO = Path(__file__).resolve().parents[2]
SDK = REPO / "neon-mdf"

NARROW = re.compile(r"^\s*[0-9a-f]+:\s+(ldr[bh]|ldrs[bh]|str[bh])(?:\.\w+)?\s", re.I)


def audit_object(path, objdump):
    result = subprocess.run([objdump, "-d", "--no-show-raw-insn", str(path)],
                            capture_output=True, text=True, check=True)
    function = "<unknown>"
    findings = []
    for line in result.stdout.splitlines():
        if re.match(r"^[0-9a-f]+ <.+>:$", line):
            function = line.split("<", 1)[1].rsplit(">", 1)[0]
        if NARROW.match(line):
            findings.append(f"{path}: {function}: {line.strip()}")
    return findings




def run(command, cwd, log, expect=0):
    result = subprocess.run([str(x) for x in command], cwd=cwd, text=True,
                            stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    with log.open("a") as out:
        out.write("$ " + " ".join(str(x) for x in command) + "\n" + result.stdout)
    if result.returncode != expect:
        raise RuntimeError(f"{command[0]} exited {result.returncode}, expected {expect}: {log}\n{result.stdout[-4000:]}")
    return result.stdout


def prepare(source, destination):
    shutil.copytree(source, destination, ignore=shutil.ignore_patterns("build", "sdkconfig.old", "__pycache__"))
    (destination / "CMakePresets.json").write_text(json.dumps({
        "version": 6, "include": [str(SDK / "cmake/presets/targets.json")]
    }))


def build(app, output, profile="normal", build_type="Debug", signed=False):
    log = output.parent / (output.name + ".log")
    run(["cmake", "--preset", "GT32U2086", "-B", output,
         f"-DNEON_MDF_PATH={SDK}", f"-DPython3_EXECUTABLE={sys.executable}",
         f"-DNEON_MDF_PROFILE={profile}", f"-DCMAKE_BUILD_TYPE={build_type}",
         f"-DNEON_MDF_SECURE_BOOT={'ON' if signed else 'OFF'}"], app, log)
    run(["cmake", "--build", output, "-j", "4"], app, log)
    objects = sorted(set(output.rglob("*.obj")) | set(output.rglob("*.o")))
    objects = [p for p in objects if any(part.endswith(".dir") for part in p.parts)]
    if not objects:
        raise RuntimeError(f"No firmware objects found in {output}")
    findings = [finding for obj in objects for finding in audit_object(obj, "arm-none-eabi-objdump")]
    with log.open("a") as out:
        out.write("\n".join(findings) if findings else f"[PASS] word-access audit: {len(objects)} objects\n")
    if findings:
        raise RuntimeError("Narrow-access instruction candidates:\n" + "\n".join(findings))
    images = sorted(output.glob("*_image_signed.bin" if signed else "*_image_xip.bin" if profile == "xip" else "*_image.bin"))
    if len(images) != 1:
        raise RuntimeError(f"Expected one boot image in {output}, found {images}")
    run([sys.executable, SDK / "tools/gt32u2086_flash.py", "--image", images[0], "--check-only"], app, log)
    print(f"[PASS] {app.name}: {build_type}/{profile}{'/signed' if signed else ''}", flush=True)
    return images[0]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--work-dir", type=Path, help="Keep build outputs/logs here (must be empty)")
    args = parser.parse_args()
    root = args.work_dir.resolve() if args.work_dir else Path(tempfile.mkdtemp(prefix="neon-sdk-check-"))
    root.mkdir(parents=True, exist_ok=True)
    if any(root.iterdir()):
        parser.error("--work-dir must be empty")
    print(f"Validation outputs: {root}", flush=True)
    # Prove the guard detects an actual narrow SRAM access.
    probe = root / "narrow_probe.c"
    probe.write_text("#include <stdint.h>\nvolatile uint16_t sample;\nuint32_t read_sample(void) { return sample; }\n")
    probe_obj = root / "narrow_probe.o"
    run(["arm-none-eabi-gcc", "-mcpu=cortex-m0", "-mthumb", "-O2", "-c", probe, "-o", probe_obj], root, root / "audit-probe.log")
    if not audit_object(probe_obj, "arm-none-eabi-objdump"):
        raise RuntimeError("Word-access audit missed the negative probe")
    print("[PASS] audit rejects a known halfword SRAM load", flush=True)
    apps = {}
    for source in sorted((REPO / "examples").iterdir()):
        if (source / "CMakePresets.json").exists():
            app = root / source.name
            prepare(source, app)
            apps[source.name] = app
            build(app, app / "build/GT32U2086")

    # Regression: CLI and direct CMake builds must both see changed sdkconfig.
    app = apps["Printf_Log"]
    config = app / "sdkconfig"
    config.write_text(config.read_text().replace("CONFIG_NEON_MDF_DRIVER_WDT=y",
                                                "# CONFIG_NEON_MDF_DRIVER_WDT is not set"))
    log = root / "config-refresh.log"
    run([sys.executable, SDK / "tools/neon_mdf.py", "build"], app, log)
    header = app / "build/GT32U2086/config/sdkconfig.h"
    if "#define CONFIG_NEON_MDF_DRIVER_WDT 1" in header.read_text():
        raise RuntimeError("CLI build retained a stale sdkconfig.h")
    config.write_text(config.read_text().replace("# CONFIG_NEON_MDF_DRIVER_WDT is not set",
                                                "CONFIG_NEON_MDF_DRIVER_WDT=y"))
    run(["cmake", "--build", "--preset", "GT32U2086"], app, log)
    if "#define CONFIG_NEON_MDF_DRIVER_WDT 1" not in header.read_text():
        raise RuntimeError("CMake build retained a stale sdkconfig.h")
    print("[PASS] CLI and CMake apply sdkconfig edits", flush=True)

    # Separate build directories may configure/build profiles concurrently.
    with ThreadPoolExecutor(max_workers=2) as pool:
        normal = pool.submit(build, app, app / "build/normal-isolated", "normal")
        xip = pool.submit(build, app, app / "build/xip-isolated", "xip")
        normal.result()
        xip.result()
    build(app, app / "build/signed", signed=True)
    for name in ("ADC_Sampling", "CRC_Known_Vector", "RTC_Calendar", "FreeRTOS_Blinky"):
        build(apps[name], apps[name] / "build/release", build_type="Release")

    # Both project templates must build after normal token replacement.
    for template in ("templates", "templates_freertos"):
        app = root / template
        prepare(SDK / "tools" / template, app)
        for path in app.rglob("*"):
            if path.is_file():
                path.write_text(path.read_text().replace("__APP_NAME__", template)
                                .replace("__NEON_MDF_RELATIVE_PATH__", str(SDK)))
        build(app, app / "build/GT32U2086")
    print(f"[PASS] {len(apps)} examples, templates, release audits, configuration refresh, concurrent profiles and signing.")


if __name__ == "__main__":
    main()
