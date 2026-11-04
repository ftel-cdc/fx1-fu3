"""Load + sanity-check a devices/*.yaml file into the model.Device dataclass.

No jsonschema dependency on purpose (see schema/*.yaml for the documented
shape) - just enough validation here to fail loudly and clearly on the
mistakes that would otherwise silently produce a broken header/linker
script (duplicate addresses, duplicate IRQ numbers, missing fields).
"""
import re

import yaml

from . import model


class DeviceDescriptionError(ValueError):
    pass


def _require(d: dict, key: str, ctx: str):
    if key not in d:
        raise DeviceDescriptionError(f"Thiếu field bắt buộc '{key}' trong {ctx}")
    return d[key]


def load_device(path: str, profile: str = "normal") -> model.Device:
    with open(path, "r", encoding="utf-8") as f:
        raw = yaml.safe_load(f)

    device = _require(raw, "device", "root")
    vendor = _require(raw, "vendor", "root")
    family = _require(raw, "family", "root")
    arch = _require(raw, "arch", "root")
    description = raw.get("description", "")

    cpu_raw = dict(_require(raw, "cpu", "root"))
    core = _require(cpu_raw, "core", "cpu")
    revision = cpu_raw.pop("revision", "unknown")
    max_freq_hz = _require(cpu_raw, "max_freq_hz", "cpu")
    cpu_raw.pop("core", None)
    cpu_raw.pop("max_freq_hz", None)
    cpu = model.Cpu(core=core, revision=revision, max_freq_hz=max_freq_hz, extra=cpu_raw)

    memory = []
    for i, m in enumerate(raw.get("memory", [])):
        ctx = f"memory[{i}]"
        memory.append(model.MemoryRegion(
            name=_require(m, "name", ctx),
            kind=_require(m, "kind", ctx),
            address=_require(m, "address", ctx),
            size=_require(m, "size", ctx),
        ))
    _check_no_overlap(memory)

    peripherals = []
    seen_periph_base = {}
    for i, p in enumerate(raw.get("peripherals", [])):
        ctx = f"peripherals[{i}] ({p.get('name', '?')})"
        name = _require(p, "name", ctx)
        base = _require(p, "base", ctx)
        if base in seen_periph_base:
            raise DeviceDescriptionError(
                f"Peripheral '{name}' trùng base address 0x{base:08X} với '{seen_periph_base[base]}'"
            )
        seen_periph_base[base] = name

        registers = []
        seen_offsets = {}
        for j, r in enumerate(p.get("registers", [])):
            rctx = f"{ctx}.registers[{j}]"
            rname = _require(r, "name", rctx)
            offset = _require(r, "offset", rctx)
            if offset in seen_offsets:
                raise DeviceDescriptionError(
                    f"Peripheral '{name}': register '{rname}' trùng offset 0x{offset:X} với '{seen_offsets[offset]}'"
                )
            seen_offsets[offset] = rname
            def load_fields(raw_fields, fields_ctx):
                fields = [
                    model.Field(
                        name=_require(fl, "name", f"{fields_ctx}[{k}]"),
                        bit_offset=_require(fl, "bit_offset", f"{fields_ctx}[{k}]"),
                        bit_width=_require(fl, "bit_width", f"{fields_ctx}[{k}]"),
                        description=fl.get("description", ""),
                    )
                    for k, fl in enumerate(raw_fields)
                ]
                field_bits = 0
                field_names = set()
                for k, fld in enumerate(fields):
                    fctx = f"{fields_ctx}[{k}] ({fld.name})"
                    if fld.name in field_names:
                        raise DeviceDescriptionError(
                            f"{fields_ctx}: field '{fld.name}' bị khai báo trùng"
                        )
                    field_names.add(fld.name)
                    if fld.bit_offset < 0 or fld.bit_width <= 0:
                        raise DeviceDescriptionError(
                            f"{fctx}: bit_offset phải >= 0 và bit_width phải > 0"
                        )
                    if fld.bit_offset + fld.bit_width > r.get("size", 32):
                        raise DeviceDescriptionError(
                            f"{fctx}: bitfield vượt quá độ rộng thanh ghi {r.get('size', 32)} bit"
                        )
                    fld_mask = ((1 << fld.bit_width) - 1) << fld.bit_offset
                    if field_bits & fld_mask:
                        raise DeviceDescriptionError(
                            f"{fctx}: bitfield chồng lấn với field đứng trước"
                        )
                    field_bits |= fld_mask
                return fields

            fields_ = load_fields(r.get("fields", []), f"{rctx}.fields")
            aliases = []
            alias_names = set()
            for k, alias in enumerate(r.get("aliases", [])):
                actx = f"{rctx}.aliases[{k}]"
                alias_name = _require(alias, "name", actx)
                if alias_name in alias_names:
                    raise DeviceDescriptionError(
                        f"{rctx}: register alias '{alias_name}' bị khai báo trùng"
                    )
                alias_names.add(alias_name)
                aliases.append(model.RegisterAlias(
                    name=alias_name,
                    access=alias.get("access", r.get("access", "rw")),
                    reset=alias.get("reset", r.get("reset", 0)),
                    description=alias.get("description", ""),
                    fields=load_fields(alias.get("fields", []), f"{actx}.fields"),
                ))
            registers.append(model.Register(
                name=rname,
                offset=offset,
                size=r.get("size", 32),
                access=r.get("access", "rw"),
                reset=r.get("reset", 0),
                description=r.get("description", ""),
                fields=fields_,
                aliases=aliases,
            ))
        registers.sort(key=lambda r: r.offset)
        peripherals.append(model.Peripheral(
            name=name, base=base, description=p.get("description", ""), registers=registers,
            type=p.get("type", name),
        ))

    _check_shared_type_register_layout(peripherals)

    interrupts = []
    seen_irq_numbers = {}
    for i, irq in enumerate(raw.get("interrupts", [])):
        ctx = f"interrupts[{i}]"
        number = _require(irq, "number", ctx)
        name = _require(irq, "name", ctx)
        if number in seen_irq_numbers:
            raise DeviceDescriptionError(
                f"IRQ number {number} trùng giữa '{name}' và '{seen_irq_numbers[number]}'"
            )
        seen_irq_numbers[number] = name
        interrupts.append(model.Interrupt(number=number, name=name, description=irq.get("description", "")))
    interrupts.sort(key=lambda i: i.number)

    num_irq = cpu.extra.get("num_irq")
    if num_irq is not None and interrupts and interrupts[-1].number >= num_irq:
        raise DeviceDescriptionError(
            f"interrupts có number={interrupts[-1].number} >= cpu.num_irq={num_irq}"
        )

    bootloader = None
    if "bootloader" in raw:
        b_raw = raw["bootloader"]
        bl_size = _require(b_raw, "size", "bootloader")
        header_size = _require(b_raw, "image_header_size", "bootloader")
        uart_raw = _require(b_raw, "uart", "bootloader")
        bootloader = model.Bootloader(
            size=bl_size,
            image_header_size=header_size,
            uart=model.BootloaderUart(
                peripheral=_require(uart_raw, "peripheral", "bootloader.uart"),
                tx_pin=_require(uart_raw, "tx_pin", "bootloader.uart"),
                rx_pin=_require(uart_raw, "rx_pin", "bootloader.uart"),
                af=_require(uart_raw, "af", "bootloader.uart"),
                baud=_require(uart_raw, "baud", "bootloader.uart"),
            ),
        )

    erase_sectors = None
    if "erase_sectors" in raw:
        erase_sectors = []
        for i, es in enumerate(raw["erase_sectors"]):
            ctx = f"erase_sectors[{i}]"
            erase_sectors.append(model.EraseSector(
                size=_require(es, "size", ctx),
                count=_require(es, "count", ctx),
            ))
        _check_erase_sectors_cover_flash(memory, erase_sectors)

    if "privice_parram" in raw:
        raise DeviceDescriptionError("privice_parram is no longer supported; use profile_param")
    profiles = _load_profiles(raw, memory)
    if profile not in profiles:
        raise DeviceDescriptionError(f"Unknown profile '{profile}'; supported: {', '.join(profiles)}")
    result = model.Device(
        device=device, vendor=vendor, family=family, arch=arch, description=description,
        cpu=cpu, memory=memory, peripherals=peripherals, interrupts=interrupts,
        bootloader=bootloader, erase_sectors=erase_sectors, profile=profiles[profile],
    )
    from dataclasses import replace
    from .memory_layout import validate_profile, pick_flash
    for params in profiles.values():
        candidate = replace(result, profile=params)
        try:
            validate_profile(candidate)
            if bootloader and not params.flash:
                selected_memory = [pick_flash(candidate)] + candidate.memory_by_kind("ram")
                _check_bootloader_vtor_alignment(selected_memory, cpu, bootloader, arch)
        except ValueError as exc:
            raise DeviceDescriptionError(f"profile '{params.name}': {exc}") from exc
    return result


def _load_profiles(raw, memory):
    # Devices without explicit profiles retain their normal memory selection.
    if "profile_param" not in raw:
        return {"normal": model.ProfileParam()}
    entries = raw["profile_param"]
    if not isinstance(entries, list) or not entries:
        raise DeviceDescriptionError("profile_param must be a nonempty list")
    profiles = {}
    for entry in entries:
        if not isinstance(entry, dict):
            raise DeviceDescriptionError("profile_param entries must be mappings")
        allowed = {"name", "flash", "ram"}
        if entry.keys() - allowed:
            raise DeviceDescriptionError(f"profile_param: unknown fields {sorted(entry.keys() - allowed)}")
        name = _require(entry, "name", "profile_param")
        if not isinstance(name, str) or name not in ("normal", "xip"):
            raise DeviceDescriptionError(f"Unknown profile '{name}'; supported: normal, xip")
        if name in profiles:
            raise DeviceDescriptionError(f"Duplicate profile '{name}'")
        regions = {}
        for kind in ("flash", "ram"):
            block = _require(entry, kind, f"profile '{name}'")
            regions[kind] = _load_profile_memory(block, kind, memory, f"profile '{name}'.{kind}")
        profiles[name] = model.ProfileParam(name=name, **regions)
    return profiles


def _load_profile_memory(block, kind, memory, ctx):
    if not isinstance(block, dict) or block.keys() - {"region", "description", "layout"}:
        raise DeviceDescriptionError(f"{ctx}: expected region, description, layout")
    region_name = _require(block, "region", ctx)
    regions = [m for m in memory if m.name == region_name and m.kind == kind]
    if not isinstance(region_name, str) or len(regions) != 1:
        raise DeviceDescriptionError(f"{ctx}.region: must name one {kind} region")
    region = regions[0]
    description = block.get("description", "")
    if not isinstance(description, str):
        raise DeviceDescriptionError(f"{ctx}.description must be a string")
    raw_entries = _require(block, "layout", ctx)
    if not isinstance(raw_entries, list) or not raw_entries:
        raise DeviceDescriptionError(f"{ctx}.layout must be a nonempty list")
    entries, names = [], set()
    for raw_entry in raw_entries:
        if not isinstance(raw_entry, dict) or raw_entry.keys() - {"name", "offset", "size", "usage", "description"}:
            raise DeviceDescriptionError(f"{ctx}.layout: expected name, offset, size, usage, description")
        name = _require(raw_entry, "name", ctx)
        if not isinstance(name, str) or not re.fullmatch(r"[a-z][a-z0-9_]*", name):
            raise DeviceDescriptionError(f"{ctx}.layout.name must be a lowercase identifier")
        if name in names:
            raise DeviceDescriptionError(f"{ctx}.layout: duplicate name '{name}'")
        names.add(name)
        entry_ctx = f"{ctx}.layout.{name}"
        offset = _require(raw_entry, "offset", entry_ctx)
        size = _require(raw_entry, "size", entry_ctx)
        usage = _require(raw_entry, "usage", entry_ctx)
        if type(offset) is not int or offset < 0:
            raise DeviceDescriptionError(f"{entry_ctx}.offset must be a nonnegative integer")
        if size == "remaining":
            size = region.size - offset
        if type(size) is not int or size <= 0:
            raise DeviceDescriptionError(f"{entry_ctx}.size must be positive bytes or 'remaining'")
        if offset + size > region.size:
            raise DeviceDescriptionError(f"{entry_ctx}: range exceeds region {region.name}")
        if usage not in ("reserved", "allocatable"):
            raise DeviceDescriptionError(f"{entry_ctx}.usage must be reserved or allocatable")
        desc = raw_entry.get("description", "")
        if not isinstance(desc, str):
            raise DeviceDescriptionError(f"{entry_ctx}.description must be a string")
        if name == "application" and usage != "allocatable":
            raise DeviceDescriptionError(f"{entry_ctx}: application must be allocatable")
        if name in ("image_header", "irq_relay"):
            expected_kind = "flash" if name == "image_header" else "ram"
            if kind != expected_kind or usage != "reserved":
                raise DeviceDescriptionError(f"{entry_ctx}: must be reserved in {expected_kind}")
        entries.append(model.LayoutEntry(name, offset, size, usage, desc))
    if "application" not in names:
        raise DeviceDescriptionError(f"{ctx}.layout must contain an allocatable 'application' entry")
    entries.sort(key=lambda entry: entry.offset)
    for left, right in zip(entries, entries[1:]):
        if left.offset + left.size > right.offset:
            raise DeviceDescriptionError(f"{ctx}.layout: '{left.name}' overlaps '{right.name}'")
    return model.ProfileMemory(region_name, entries, description)


def _check_erase_sectors_cover_flash(memory, erase_sectors):
    """`erase_sectors` la khai bao TAY (khong sinh tu `memory:`, vi map sector
    khong deu nhau cua nhieu chip khong bieu dien duoc qua 1 field don gian) -
    de bat loi go nham/quen sector som, kiem tra tong kich thuoc cong don cua
    tat ca nhom PHAI khop CHINH XAC voi kich thuoc vung flash thap nhat (quy
    uoc giong _pick_flash() trong gen_linker_script.py) - khong khop nghia la
    thieu/thua sector, chac chan sai."""
    flash_regions = [m for m in memory if m.kind == "flash"]
    if not flash_regions:
        raise DeviceDescriptionError("erase_sectors: device khong co memory region kind=flash")
    flash = sorted(flash_regions, key=lambda m: m.address)[0]

    total = sum(es.size * es.count for es in erase_sectors)
    if total != flash.size:
        raise DeviceDescriptionError(
            f"erase_sectors: tong kich thuoc cac nhom (0x{total:X}) khong khop kich thuoc "
            f"vung flash '{flash.name}' (0x{flash.size:X}) - kiem tra lai bang sector, "
            f"thieu/thua nhom nao do"
        )


def _check_bootloader_vtor_alignment(memory, cpu, bootloader, arch):
    """SCB->VTOR (ARMv7-M) - hoac tuong duong vector table dia chi cua PFIC
    tren RISC-V/WCH (mtvec[1:0]=3 "Address" mode, xem CH32V003F4P6.yaml's
    cpu.mtvec_mode) - yeu cau dia chi ghi vao PHAI la boi so cua kich thuoc
    bang vector table lam tron len luy thua 2 (toi thieu 128 byte) - phan
    cung IM LANG lam tron xuong neu ghi gia tri khong thoa (khong bao loi/
    fault) - neu image_header_size qua nho, APP_VECTOR_BASE se khong align
    dung, vector table se tro sai vi tri (alias vao vung header) va bat ky
    app nao dung ngat that (SysTick/PendSV/SVC...) se doc sai vector ->
    hardfault hoac hanh vi undefined am tham, chi lo ra khi co ngat that su
    kich hoat.

    So luong slot vector table (num_vectors) TINH KHAC NHAU that giua 2 kien
    truc - day la bug da fix (truoc day dung `16 + num_irq` vo dieu kien cho
    CA 2, xem ghi chu tu-phat-hien trong CH32V003F4P6.yaml gan
    bootloader.image_header_size):
      - Cortex-M (ARMv6-M/ARMv7-M+): 16 core exception CO DINH (giong nhau
        moi core, tu Initial SP toi SysTick_Handler) + cpu.num_irq device IRQ
        THEM VAO SAU do -> num_vectors = 16 + num_irq.
      - RISC-V/PFIC: cpu.num_irq theo QUY UOC cua SDK nay (xem
        backends/riscv/gen_startup.py) LA TONG SO SLOT THAT cua vector table,
        DA BAO GOM san phan core/reserved (PFIC chi co 12 slot co dinh dau
        bang - entry 0 "j handle_reset", 1 reserved, 2 NMI, 3 HardFault -
        KHONG phai 16 nhu ARM) -> num_vectors = num_irq truc tiep, KHONG
        cong them 16.
    Voi CH32V003F4P6 hien tai, cong thuc sai cu (16+39=55) va cong thuc dung
    (39) TINH CO deu lam tron len cung 1 nguong (256 byte) nen chua kich
    hoat bug that - nhung la trung hop so hoc, khong phai bang chung cong
    thuc dung, va rui ro that cho device RISC-V tiep theo co num_irq khac.

    So luong exception/IRQ dung de tinh PHAI la nang luc phan cung THAT cua
    silicon (cpu.extra.num_irq), khong phai so IRQ da khai o `interrupts:`
    (co the con rong/chua day du) - vi khe vector table phai du cho toan bo
    vi tri ma phan cung co the dung toi, khong chi phan SDK da wire toi nay.
    """
    num_irq = cpu.extra.get("num_irq")
    if num_irq is None:
        raise DeviceDescriptionError(
            "bootloader: yeu cau cpu.num_irq (de tinh align vector table "
            "that) - device nay chua khai cpu.num_irq"
        )
    if arch.startswith("cortex-m"):
        num_vectors = 16 + num_irq  # 16 core exception CO DINH + device IRQ
    elif arch.startswith("riscv"):
        num_vectors = num_irq  # cpu.num_irq DA LA tong so slot that (xem docstring)
    else:
        raise DeviceDescriptionError(
            f"bootloader: chua biet cach tinh so luong slot vector table cho "
            f"arch '{arch}' - them nhanh moi trong _check_bootloader_vtor_alignment() "
            "(mcu_desc/loader.py) SAU KHI xac nhan that so slot core/reserved co dinh "
            "cua kien truc do, khong doan."
        )
    required_align = 0x80
    while required_align < num_vectors * 4:
        required_align *= 2

    flash_regions = [m for m in memory if m.kind == "flash"]
    if not flash_regions:
        raise DeviceDescriptionError("bootloader: device khong co memory region kind=flash")
    flash = sorted(flash_regions, key=lambda m: m.address)[0]

    app_vector_base = flash.address + bootloader.size + bootloader.image_header_size
    if app_vector_base % required_align != 0:
        vector_formula = f"16 core + cpu.num_irq={num_irq}" if arch.startswith("cortex-m") else f"cpu.num_irq={num_irq} (da la tong so slot, xem docstring)"
        raise DeviceDescriptionError(
            f"bootloader.image_header_size=0x{bootloader.image_header_size:X} khong hop le: "
            f"APP_VECTOR_BASE=0x{app_vector_base:08X} (flash.address+bootloader.size+image_header_size) "
            f"phai la boi so cua 0x{required_align:X} (yeu cau align vector table cho {num_vectors} "
            f"vector = {vector_formula}) - chinh image_header_size de thoa dieu kien nay"
        )


def _check_no_overlap(regions):
    sorted_regions = sorted(regions, key=lambda m: m.address)
    for a, b in zip(sorted_regions, sorted_regions[1:]):
        if a.address + a.size > b.address:
            raise DeviceDescriptionError(
                f"Memory region '{a.name}' (0x{a.address:08X}+0x{a.size:X}) chồng lấn '{b.name}' (0x{b.address:08X})"
            )


def _register_signature(registers):
    """(name, offset, size, access) mỗi register - đủ để so sánh layout tương
    thích giữa các instance cùng type, bỏ qua description/reset (không ảnh
    hưởng shape của struct C)."""
    return tuple((r.name, r.offset, r.size, r.access) for r in registers)


def _check_shared_type_register_layout(peripherals):
    """Peripheral cùng `type` (vd UART0..UART4 -> type UART) sinh CHUNG 1
    struct C (xem gen_cmsis_header.py) - nếu register layout lệch nhau giữa
    các instance thì struct sinh ra sẽ sai cho ít nhất 1 instance. Chặn sớm ở
    đây thay vì để lỗi âm thầm lộ ra lúc build/chạy thật."""
    by_type = {}
    for p in peripherals:
        by_type.setdefault(p.type, []).append(p)

    for type_name, group in by_type.items():
        if len(group) < 2:
            continue
        first = group[0]
        first_sig = _register_signature(first.registers)
        for other in group[1:]:
            if _register_signature(other.registers) != first_sig:
                raise DeviceDescriptionError(
                    f"Peripheral '{other.name}' và '{first.name}' cùng type='{type_name}' "
                    f"nhưng register layout khác nhau - phải giống hệt nhau (dùng chung YAML "
                    f"anchor, xem cách UART0..UART4 dùng &uart_regs) vì cả nhóm sinh chung 1 "
                    f"struct C."
                )
