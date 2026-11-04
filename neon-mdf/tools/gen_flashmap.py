#!/usr/bin/env python3
"""gen_flashmap.py - doc <APP_DIR>/flashmap.csv (partition table CUA RIENG
project, giong het partitions.csv that cua ESP-IDF - KHONG phai cua device,
xem mcu-description/devices/*.yaml cho phan bootloader.size/image_header_size
CO DINH theo silicon) - tinh offset tuan tu tu NEON_MDF_APP_BASE, sinh:
  - flashmap.h   (macro C, dung boi components/ota/ + bootloader)
  - flashmap.cmake (bien CMake NEON_MDF_FLASHMAP_*, dung boi flash_targets.cmake)
  - 1 file .ld MOI cho MOI partition co type=app (dung tao 2 executable
    rieng, xem CMakeLists.txt cua app - vd examples/OTA_Demo/)

CHI duoc goi khi app CO file flashmap.csv (xem cmake/flashmap.cmake) - app
khong co file nay build theo mo hinh 1-slot-co-dinh, khong dung script nay
(xem neon_bootloader_main.c nhanh #else).

flashmap.csv format (thu tu dong = thu tu tren flash, offset TU TINH tuan
tu, khong co cot offset):
    # name,    type, subtype, size
    app,       app,  factory, 0x34000
    app_ota,   app,  ota,     0x40000
    otadata,   data, ota,     0x4000

Thu tu dong TRONG FILE KHONG con bat buoc partition nao phai dung truoc
(2026-08-10) - flash_jtag/flash_uart_stm32 (flash_targets.cmake) doc THAT
dia chi NEON_MDF_FLASHMAP_APP_BASE sinh ra tu day de biet ghi "app" (factory)
vao dau, khong con gia dinh "app luon nam ngay sau bootloader" nua - giong
cach partitions.csv that cua ESP-IDF khong quan tam thu tu dong. Yeu cau DUY
NHAT con lai: co dung 1 partition TEN "app" (type=app, subtype=factory) -
day la partition flash_jtag/flash_uart_stm32 ghi anh factory vao.

Partition type=app tu co "khe" header = --image-header-size (giong het app
hien tai, image_header_t 16 byte + dem align VTOR) truoc vector table -
"size" khai trong CSV la TONG (header+code), khong phai chi code.

Dung:
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
    """Chon dung backend theo device.arch (giong cach generate.py dispatch
    generate_cortex_m()/generate_riscv()) - THAY vi import cung 1 ban
    cortex_m.gen_linker_script._write_script() cho MOI arch (bug that gap
    luc lam RISC-V: linker script sinh ra dinh /DISCARD/ libc.a cua ARM,
    lam ld that bai "cannot find libc.a" tren toolchain RISC-V khong kem
    newlib)."""
    if arch.startswith("cortex-m"):
        from backends.cortex_m.gen_linker_script import _write_script
        return _write_script
    if arch.startswith("riscv"):
        from backends.riscv.gen_linker_script import _write_script
        return _write_script
    raise FlashmapError(f"gen_flashmap.py: arch '{arch}' khong duoc nhan dien")


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
                    f"{path}:{lineno}: can dung 4 cot 'name,type,subtype,size', thay vao do co {len(cols)}: {raw_line.strip()}"
                )
            name, type_, subtype, size_hex = cols
            if name in seen_names:
                raise FlashmapError(f"{path}:{lineno}: partition '{name}' bi khai trung ten")
            seen_names.add(name)
            if type_ not in ("app", "data"):
                raise FlashmapError(f"{path}:{lineno}: type '{type_}' khong hop le (chi cho phep 'app' hoac 'data')")
            try:
                size = int(size_hex, 16)
            except ValueError:
                raise FlashmapError(f"{path}:{lineno}: size '{size_hex}' khong phai so hex hop le")
            partitions.append(Partition(name, type_, subtype, size))
    if not partitions:
        raise FlashmapError(f"{path}: khong co partition nao (file rong?)")
    return partitions


def validate_required_partitions(partitions):
    """flashmap.csv CHI duoc dung cho mo hinh OTA bat doi xung (xem docstring
    dau file + ARCHITECTURE.md) - components/bootloader/neon_bootloader_main.c
    va components/ota/neon_ota.c THAM CHIEU CUNG THAT toi 3 macro
    NEON_MDF_FLASHMAP_APP_*/APP_OTA_*/OTADATA_BASE vo dieu kien mien la
    NEON_MDF_FLASHMAP_ENABLED duoc dinh nghia (tuc la CHI CAN co file
    flashmap.csv, khong quan tam noi dung) - thieu 1 trong 3 ten nay se lam
    build FAIL sau, sau trong neon_bootloader_main.c voi thong bao kho hieu
    ("undeclared identifier NEON_MDF_FLASHMAP_APP_BASE") thay vi bao loi ro
    rang tai day, ngay luc configure. Validate that ca 3, khong chi tin
    "co it nhat 1 dong type=app" nhu truoc (bug that: subtype duoc parse
    nhung khong bao gio doc lai)."""
    by_name = {p.name: p for p in partitions}

    app = by_name.get("app")
    if app is None or app.type != "app" or app.subtype != "factory":
        raise FlashmapError(
            "flashmap.csv: BAT BUOC co dung 1 partition ten 'app' voi "
            "type=app,subtype=factory (day la slot flash_jtag/flash_uart_stm32 "
            "ghi anh factory vao, va la macro NEON_MDF_FLASHMAP_APP_BASE ma "
            "components/bootloader/neon_bootloader_main.c doc THAT) - hien "
            f"{'khong tim thay partition ten app' if app is None else f'partition app co type={app.type},subtype={app.subtype} (sai)'}."
        )

    app_ota = by_name.get("app_ota")
    if app_ota is None or app_ota.type != "app" or app_ota.subtype != "ota":
        raise FlashmapError(
            "flashmap.csv: BAT BUOC co dung 1 partition ten 'app_ota' voi "
            "type=app,subtype=ota (slot OTA duy nhat flash_ota duoc phep ghi vao, macro "
            "NEON_MDF_FLASHMAP_APP_OTA_BASE ma neon_bootloader_main.c doc "
            f"THAT) - hien {'khong tim thay' if app_ota is None else f'co type={app_ota.type},subtype={app_ota.subtype} (sai)'}."
        )

    otadata = by_name.get("otadata")
    if otadata is None or otadata.type != "data" or otadata.subtype != "ota":
        raise FlashmapError(
            "flashmap.csv: BAT BUOC co dung 1 partition ten 'otadata' voi "
            "type=data,subtype=ota (luu trang thai active_slot/state/boot_attempts, macro "
            "NEON_MDF_FLASHMAP_OTADATA_BASE ma neon_bootloader_main.c/"
            f"neon_ota.c doc THAT) - hien {'khong tim thay' if otadata is None else f'co type={otadata.type},subtype={otadata.subtype} (sai)'}."
        )


def compute_layout(partitions, app_base, image_header_size):
    offset = app_base
    for p in partitions:
        p.base = offset
        if p.type == "app":
            if p.size <= image_header_size:
                raise FlashmapError(
                    f"partition '{p.name}': size (0x{p.size:X}) phai LON HON image_header_size "
                    f"(0x{image_header_size:X}) - khong con cho cho code"
                )
            p.vector_base = p.base + image_header_size
            p.code_size = p.size - image_header_size
        offset += p.size
    return offset  # tong cuoi cung (dung de bao loi neu vuot qua flash)


def parse_erase_sectors(spec):
    """'0x4000:4,0x10000:1,0x20000:3' -> [(size,count), ...]. Chuoi rong = khong co (bo qua validate).
    Chuoi nay THUONG do cmake/mcu_variants.cmake tu sinh tu device YAML da
    validate (khong phai nguoi dung go tay) nen rui ro sai dinh dang thap -
    van bao FlashmapError co ngu canh thay vi de ValueError/IndexError tran
    ro lot ra ngoai neu co ai truyen tay `--erase-sectors` sai dinh dang luc
    debug/goi thu cong."""
    if not spec:
        return None
    groups = []
    for item in spec.split(","):
        try:
            size_str, count_str = item.split(":")
            groups.append((int(size_str, 16), int(count_str)))
        except ValueError as e:
            raise FlashmapError(
                f"--erase-sectors: muc '{item}' sai dinh dang (can '<size hex>:<count so>', "
                f"vd '0x4000:4') - {e}"
            )
    return groups


def sector_boundaries(flash_base, erase_sectors):
    """Danh sach dia chi BAT DAU cua tung sector, tu dau flash - dung de
    kiem tra 1 dia chi co ROI DUNG ranh gioi sector hay khong."""
    boundaries = [flash_base]
    addr = flash_base
    for size, count in erase_sectors:
        for _ in range(count):
            addr += size
            boundaries.append(addr)
    return set(boundaries)


def validate_flash_capacity(flash_base, flash_size, final_offset, last_partition_name):
    """Check TUONG MINH flashmap.csv khong vuot qua flash THAT - doc lap voi
    erase_sectors (field TUY CHON theo schema, xem parse_erase_sectors()).
    Truoc day khong co check nay - overflow chi vo tinh bi bat khi
    erase_sectors duoc khai (validate_alignment() duoi day, tac dung phu vi
    tong erase_sectors bat buoc khop CHINH XAC flash.size that, xem
    loader.py's _check_erase_sectors_cover_flash()) - device co bootloader:
    nhung KHONG khai erase_sectors (hop le theo schema) se khong duoc bao ve
    gi ca: gen_flashmap.py chay "thanh cong", sinh .ld voi FLASH LENGTH =
    kich thuoc partition tu khai trong CSV (khong phai flash that), CMake/ld
    khong bao loi gi - chi lo ra khi flash that qua SWD/UART, rat kho chan
    doan. Check nay dam bao FATAL_ERROR ro rang ngay luc configure."""
    flash_end = flash_base + flash_size
    if final_offset > flash_end:
        raise FlashmapError(
            f"flashmap: tong kich thuoc cac partition (ket thuc tai 0x{final_offset:08X}, "
            f"sau partition cuoi '{last_partition_name}') VUOT QUA flash that "
            f"(0x{flash_base:08X}-0x{flash_end:08X}, {flash_size} byte) - giam size 1 hoac "
            f"nhieu partition trong flashmap.csv."
        )


def validate_alignment(partitions, flash_base, erase_sectors, final_offset):
    if erase_sectors is None:
        return
    boundaries = sector_boundaries(flash_base, erase_sectors)
    for p in partitions:
        if p.base not in boundaries:
            raise FlashmapError(
                f"partition '{p.name}': dia chi bat dau 0x{p.base:08X} KHONG roi dung ranh gioi "
                f"sector erase that (xem erase_sectors trong device YAML) - erase partition nay se "
                f"dung/xoa nham du lieu partition lan can. Chinh lai size cua partition TRUOC no "
                f"cho khop ranh gioi sector."
            )
    if final_offset not in boundaries:
        raise FlashmapError(
            f"flashmap: diem ket thuc 0x{final_offset:08X} (sau partition cuoi '{partitions[-1].name}') "
            f"KHONG roi dung ranh gioi sector - chinh lai size cua partition cuoi."
        )


def macro_name(partition_name):
    return partition_name.upper().replace("-", "_")


def write_header(partitions, out_path):
    lines = [
        "/* Auto-generated boi tools/gen_flashmap.py - DO NOT EDIT BY HAND. */",
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
        "# Auto-generated boi tools/gen_flashmap.py - DO NOT EDIT BY HAND.",
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
    """gen_linker_script._write_script() chi dung device.device (ten, chi de
    ghi vao comment dau file .ld) - khong can dung ca model.Device that."""
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
        print(f"Loi flashmap: {e}", file=sys.stderr)
        sys.exit(1)

    print(f"flashmap: {len(partitions)} partition, tu 0x{app_base:08X} den 0x{final_offset:08X}")
    for p in partitions:
        extra = f", vector=0x{p.vector_base:08X}, code=0x{p.code_size:X}" if p.type == "app" else ""
        print(f"  {p.name:12s} type={p.type:5s} base=0x{p.base:08X} size=0x{p.size:X}{extra}")
    print(f"Da ghi {args.header_out}, {args.cmake_out}, {len(written_ld)} file .ld ({args.linker_dir})")


def _ensure_dir(path):
    d = os.path.dirname(os.path.abspath(path))
    os.makedirs(d, exist_ok=True)


if __name__ == "__main__":
    main()
