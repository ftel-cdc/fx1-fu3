#!/usr/bin/env python3
"""gen_flashmap.py reads <APP_DIR>/flashmap.csv, the app-specific partition table, like
partitions.csv of ESP-IDF - not device, see mcu-description/devices/*.yaml for silicon-defined
bootloader.size/image_header_size parts) - calculate sequential offsets from NEON_MDF_APP_BASE, generate:
  - flashmap.h (macro C, used by components/ota/ + bootloader)
  - flashmap.cmake (CMake variable NEON_MDF_FLASHMAP_*, used by flash_targets.cmake)
  - 1 .ld file per partition with type=app (use to create 2 separate executables, see app's
    CMakeLists.txt - e.g. examples/OTA_Demo/)

only called when the app has a flashmap.csv file (see cmake/flashmap.cmake) - app does not have this
build file in the specified 1-slot opening, do not use this script (see neon_bootloader_main.c quick
#else).

flashmap.csv format (row order = flash order, sequentially calculated offsets, no offset column):
    # name,    type, subtype, size
    app,       app,  factory, 0x34000
    app_ota,   app,  ota,     0x40000
    otadata,   data, ota,     0x4000

CSV row order is flexible (2026-08-10). flash_jtag/flash_uart_stm32 in flash_targets.cmake read the
generated NEON_MDF_FLASHMAP_APP_BASE to locate the factory app at its actual
start, without assuming that the app immediately follows the bootloader. Like ESP-IDF
partitions.csv, row order is flexible. Exactly one partition must be named "app" (type=app,
subtype=factory); flash_jtag/flash_uart_stm32 use this partition
write image factory vao.

App partitions reserve a header slot of --image-header-size bytes, just like the app
currently, image_header_t 16 bytes + VTOR align buffer) before vector table - "size" declared in CSV
is sum (header+code), not just code.

Used:
  python3 gen_flashmap.py --flashmap flashmap.csv --flash-base 0x08000000
      --app-base 0x08008000 --image-header-size 0x200
      --erase-sectors "0x4000:4,0x10000:1,0x20000:3"
      --header-out flashmap.h --cmake-out flashmap.cmake --linker-dir ./generated
"""
import argparse
import os
import sys

sys.path.insert(0, os.path.join(os.path.dirname(__file__), "..", "mcu-description", "tools"))


def _pick_write_script(arch):
    """choose to use backend according to device.arch (same as generate.py dispatch
    generate_cortex_m()/generate_riscv()) - see because import the same copy of
    cortex_m.gen_linker_script._write_script() for each architecture. During RISC-V bring-up, the ARM
    /DISCARD/ libc.a clause caused ld to fail with "cannot find libc.a" on the
    RISC-V not included
    newlib)."""
    if arch.startswith("cortex-m"):
        from backends.cortex_m.gen_linker_script import _write_script
        return _write_script
    if arch.startswith("riscv"):
        from backends.riscv.gen_linker_script import _write_script
        return _write_script
    raise FlashmapError(f"gen_flashmap.py: arch '{arch}' not accepted")


class FlashmapError(ValueError):
    pass


class Partition:
    def __init__(self, name, type_, subtype, size):
        self.name = name
        self.type = type_
        self.subtype = subtype
        self.size = size
        self.base = None
        self.vector_base = None
        self.code_size = None


def parse_flashmap(path):
    partitions = []
    seen_names = set()
    with open(path, "r", encoding="utf-8") as f:
        for lineno, raw_line in enumerate(f, start=1):
            line = raw_line.split("#", 1)[0].strip()
            if not line:
                continue
            cols = [c.strip() for c in line.split(",")]
            if len(cols) != 4:
                raise FlashmapError(
                    f"{path}:{lineno}: need to use 4 cots 'name,type,subtype,size', see that there is {len(cols)}: {raw_line.strip()}"
                )
            name, type_, subtype, size_hex = cols
            if name in seen_names:
                raise FlashmapError(f"{path}:{lineno}: partition '{name}' is declared more than once")
            seen_names.add(name)
            if type_ not in ("app", "data"):
                raise FlashmapError(f"{path}:{lineno}: type '{type_}'unmatched' (only for phep 'app' or 'data')")
            try:
                size = int(size_hex, 16)
            except ValueError:
                raise FlashmapError(f"{path}:{lineno}: size '{size_hex}' is not a hex number")
            partitions.append(Partition(name, type_, subtype, size))
    if not partitions:
        raise FlashmapError(f"{path}: no partitions found (empty file?)")
    return partitions


def validate_required_partitions(partitions):
    """flashmap.csv is only used for OTA image opening with pulse switch on (see docstring at the beginning
    of the file + ARCHITECTURE.md) - components/bootloader/neon_bootloader_main.c and
    components/ota/neon_ota.c references NEON_MDF_FLASHMAP_APP_*/APP_OTA_*/OTADATA_BASE unconditionally
    whenever
    NEON_MDF_FLASHMAP_ENABLED is defined (i.e. just have the file flashmap.csv, not important to
    use) - missing 1 of these 3 names will be the following build FAIL, then in
    neon_bootloader_main.c with an obscure compiler error ("undeclared identifier
    NEON_MDF_FLASHMAP_APP_BASE") instead of reporting the problem immediately at configure time. Validate
    true all 3, not only believe "at least 1 dynamic type=app" as before (true bug: subtype parsed
    but no reread time announcement)."""
    by_name = {p.name: p for p in partitions}

    app = by_name.get("app")
    if app is None or app.type != "app" or app.subtype != "factory":
        raise FlashmapError(
            "flashmap.csv: Required to have exactly 1 partition named 'app' with "
            "type=app,subtype=factory (the slot written by flash_jtag/flash_uart_stm32 "
            "write image factory vao, and is macro NEON_MDF_FLASHMAP_APP_BASE that "
            "components/bootloader/neon_bootloader_main.c real read) - present "
            f"{'app name partition not found' if app is None else f'app partition has type={app.type},subtype={app.subtype} (sai)'}."
        )

    app_ota = by_name.get("app_ota")
    if app_ota is None or app_ota.type != "app" or app_ota.subtype != "ota":
        raise FlashmapError(
            "flashmap.csv: Required to have exactly 1 partition named 'app_ota' with "
            "type=app,subtype=ota (the only OTA slot flash_ota is allowed to write in macro) "
            "NEON_MDF_FLASHMAP_APP_OTA_BASE read by neon_bootloader_main.c "
            f"Real) - Show {'not found' if app_ota is None else f'has type={app_ota.type},subtype={app_ota.subtype} (sai)'}."
        )

    otadata = by_name.get("otadata")
    if otadata is None or otadata.type != "data" or otadata.subtype != "ota":
        raise FlashmapError(
            "flashmap.csv: Required to have 1 partition named 'otadata' with "
            "type=data,subtype=ota (save active_slot/state/boot_attempts status, macro "
            "NEON_MDF_FLASHMAP_OTADATA_BASE ma neon_bootloader_main.c/"
            f"neon_ota.c real read) - now {'not found' if otadata is None else f'has type={otadata.type},subtype={otadata.subtype} (sai)'}."
        )


def compute_layout(partitions, app_base, image_header_size):
    offset = app_base
    for p in partitions:
        p.base = offset
        if p.type == "app":
            if p.size <= image_header_size:
                raise FlashmapError(
                    f"partition '{p.name}': size (0x{p.size:X}) must be greater than image_header_size "
                    f"(0x{image_header_size:X}) - no code allowed"
                )
            p.vector_base = p.base + image_header_size
            p.code_size = p.size - image_header_size
        offset += p.size
    return offset  # last total (use to report error if pass flash)


def parse_erase_sectors(spec):
    """'0x4000:4,0x10000:1,0x20000:3' -> [(size,count),...]. Empty string = no (ignore validate). This
    string is usually cmake/mcu_variants.cmake from YAML device already validated (not hand typed
    user) so the default rotation is thap - still shows contextual FlashmapError because let
    ValueError/IndexError tran rotation lot out if anyone traces manually the default
    `--erase-sectors` is retained for backward compatibility with debug/packaging callers."""
    if not spec:
        return None
    groups = []
    for item in spec.split(","):
        try:
            size_str, count_str = item.split(":")
            groups.append((int(size_str, 16), int(count_str)))
        except ValueError as e:
            raise FlashmapError(
                f"--erase-sectors: entry '{item}'default is (needs '<size hex>:<count number>', "
                f"vd '0x4000:4') - {e}"
            )
    return groups


def sector_boundaries(flash_base, erase_sectors):
    """The dia list only starts with each sector, from the beginning of the flash - used to check if a dia
    is available and then use the sector boundary to not."""
    boundaries = [flash_base]
    addr = flash_base
    for size, count in erase_sectors:
        for _ in range(count):
            addr += size
            boundaries.append(addr)
    return set(boundaries)


def validate_flash_capacity(flash_base, flash_size, final_offset, last_partition_name):
    """MINH check flashmap.csv does not pass real flash - read iterations with erase_sectors (field
    optional in the schema; see parse_erase_sectors()). Overflow is checked only when erase_sectors is
    declared. validate_alignment() below uses the total
    erase_sectors requires exact matching of real flash.size, see
    loader.py _check_erase_sectors_cover_flash()). A device with a bootloader:
    but failing to declare erase_sectors (in schema) will result in nothing: gen_flashmap.py runs
    "successfully", creates .ld with FLASH LENGTH = size partition from CSV declaration (not real
    flash), CMake/ld shows no error - only real flash through SWD/UART, very guesswork. This check
    ensures that FATAL_ERROR roasts as soon as configured."""
    flash_end = flash_base + flash_size
    if final_offset > flash_end:
        raise FlashmapError(
            f"flashmap: total size of partitions (executed at 0x{final_offset:08X}, "
            f"after partition cuoi '{last_partition_name}') real flash pass "
            f"(0x{flash_base:08X}-0x{flash_end:08X}, {flash_size} bytes) - reduce size 1 or "
            f"multiple partitions in flashmap.csv."
        )


def validate_alignment(partitions, flash_base, erase_sectors, final_offset):
    if erase_sectors is None:
        return
    boundaries = sector_boundaries(flash_base, erase_sectors)
    for p in partitions:
        if p.base not in boundaries:
            raise FlashmapError(
                f"partition '{p.name}`: dia only starts 0x{p.base:08X} No, then use the gasket. "
                f"real erase sector (see erase_sectors in YAML device) - this partition will erase "
                f"overlapping or erasing another partition. Adjust the size of the preceding partition "
                f"for sector alignment matching."
            )
    if final_offset not in boundaries:
        raise FlashmapError(
            f"flashmap: diem 0x{final_offset:08X} (after partition cuoi '{partitions[-1].name}') "
            f"not then use sector boundary - regain size of partition cuoi."
        )


def macro_name(partition_name):
    return partition_name.upper().replace("-", "_")


def write_header(partitions, out_path):
    lines = [
        "/* Auto-generated by tools/gen_flashmap.py - DO NOT EDIT BY HAND. */",
        "#ifndef NEON_MDF_FLASHMAP_H",
        "#define NEON_MDF_FLASHMAP_H",
        "",
        "#define NEON_MDF_FLASHMAP_ENABLED 1",
        "",
    ]
    for p in partitions:
        n = macro_name(p.name)
        lines.append(f"#define NEON_MDF_FLASHMAP_{n}_BASE   0x{p.base:08X}UL")
        lines.append(f"#define NEON_MDF_FLASHMAP_{n}_SIZE   0x{p.size:08X}UL")
        if p.type == "app":
            lines.append(f"#define NEON_MDF_FLASHMAP_{n}_VECTOR_BASE 0x{p.vector_base:08X}UL")
            lines.append(f"#define NEON_MDF_FLASHMAP_{n}_CODE_SIZE   0x{p.code_size:08X}UL")
        lines.append("")
    lines.append("#endif /* NEON_MDF_FLASHMAP_H */")
    with open(out_path, "w", encoding="utf-8") as f:
        f.write("\n".join(lines) + "\n")


def write_cmake(partitions, out_path):
    lines = [
        "# Auto-generated by tools/gen_flashmap.py - DO NOT EDIT BY HAND.",
        "set(NEON_MDF_FLASHMAP_ENABLED ON)",
    ]
    for p in partitions:
        n = macro_name(p.name)
        lines.append(f"set(NEON_MDF_FLASHMAP_{n}_BASE 0x{p.base:08X})")
        lines.append(f"set(NEON_MDF_FLASHMAP_{n}_SIZE 0x{p.size:08X})")
        if p.type == "app":
            lines.append(f"set(NEON_MDF_FLASHMAP_{n}_VECTOR_BASE 0x{p.vector_base:08X})")
            lines.append(f"set(NEON_MDF_FLASHMAP_{n}_CODE_SIZE 0x{p.code_size:08X})")
    with open(out_path, "w", encoding="utf-8") as f:
        f.write("\n".join(lines) + "\n")


class _FakeDevice:
    """gen_linker_script._write_script() only use device.device (name, just write comment at the beginning
    of .ld file) - do not use real model.Device."""
    def __init__(self, device_name):
        self.device = device_name


class _FakeRam:
    def __init__(self, address, size):
        self.address = address
        self.size = size


def write_linker_scripts(partitions, ram_addr, ram_size, device_name, out_dir, arch):
    os.makedirs(out_dir, exist_ok=True)
    write_script = _pick_write_script(arch)
    device = _FakeDevice(device_name)
    ram = _FakeRam(ram_addr, ram_size)
    written = []
    for p in partitions:
        if p.type != "app":
            continue
        out_path = os.path.join(out_dir, f"{p.name}.ld")
        write_script(device, p.vector_base, p.code_size, ram, out_path)
        written.append(out_path)
    return written


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--flashmap", required=True)
    ap.add_argument("--flash-base", required=True)
    ap.add_argument("--flash-size", required=True)
    ap.add_argument("--app-base", required=True)
    ap.add_argument("--image-header-size", required=True)
    ap.add_argument("--erase-sectors", default="")
    ap.add_argument("--header-out", required=True)
    ap.add_argument("--cmake-out", required=True)
    ap.add_argument("--linker-dir", required=True)
    ap.add_argument("--ram-base", required=True)
    ap.add_argument("--ram-size", required=True)
    ap.add_argument("--device-name", required=True)
    ap.add_argument("--arch", required=True)
    args = ap.parse_args()

    try:
        flash_base = int(args.flash_base, 16)
        flash_size = int(args.flash_size, 16)
        app_base = int(args.app_base, 16)
        image_header_size = int(args.image_header_size, 16)
        ram_base = int(args.ram_base, 16)
        ram_size = int(args.ram_size, 16)

        partitions = parse_flashmap(args.flashmap)
        validate_required_partitions(partitions)
        final_offset = compute_layout(partitions, app_base, image_header_size)
        validate_flash_capacity(flash_base, flash_size, final_offset, partitions[-1].name)
        erase_sectors = parse_erase_sectors(args.erase_sectors)
        validate_alignment(partitions, flash_base, erase_sectors, final_offset)

        _ensure_dir(args.header_out)
        _ensure_dir(args.cmake_out)
        write_header(partitions, args.header_out)
        write_cmake(partitions, args.cmake_out)
        written_ld = write_linker_scripts(partitions, ram_base, ram_size, args.device_name, args.linker_dir, args.arch)
    except FlashmapError as e:
        print(f"flashmap error: {e}", file=sys.stderr)
        sys.exit(1)

    print(f"flashmap: {len(partitions)} partition, from 0x{app_base:08X} to 0x{final_offset:08X}")
    for p in partitions:
        extra = f", vector=0x{p.vector_base:08X}, code=0x{p.code_size:X}" if p.type == "app" else ""
        print(f"  {p.name:12s} type={p.type:5s} base=0x{p.base:08X} size=0x{p.size:X}{extra}")
    print(f"already write {args.header_out}, {args.cmake_out}, {len(written_ld)} file .ld ({args.linker_dir})")


def _ensure_dir(path):
    d = os.path.dirname(os.path.abspath(path))
    os.makedirs(d, exist_ok=True)


if __name__ == "__main__":
    main()
