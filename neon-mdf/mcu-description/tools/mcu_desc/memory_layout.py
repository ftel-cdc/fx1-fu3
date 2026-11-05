"""Resolve profile ranges without changing the physical memory map."""
from dataclasses import dataclass


def pick_flash(device):
    regions = device.memory_by_kind("flash")
    if device.profile.flash:
        return next(m for m in regions if m.name == device.profile.flash.region)
    if not regions:
        raise ValueError(f"Device {device.device} no memory region kind=flash")
    return min(regions, key=lambda m: m.address)


def pick_ram(device):
    regions = device.memory_by_kind("ram")
    if device.profile.ram:
        return next(m for m in regions if m.name == device.profile.ram.region)
    if not regions:
        raise ValueError(f"Device {device.device} no memory region kind=ram")
    return next((m for m in regions if "USR" in m.name.upper()),
                min(regions, key=lambda m: m.address))


@dataclass(frozen=True)
class ImageLayout:
    header_base: int
    header_size: int
    flash_base: int
    flash_size: int
    ram_base: int
    ram_size: int
    relay_base: int
    relay_size: int


def profile_ranges(device):
    """Yield kind, entry, absolute base for every declared range."""
    for kind, parent in (("flash", pick_flash(device)), ("ram", pick_ram(device))):
        block = getattr(device.profile, kind)
        if block:
            for entry in block.layout:
                yield kind, entry, parent.address + entry.offset


def image_layout(device, image="standalone"):
    flash, ram = pick_flash(device), pick_ram(device)
    p = device.profile
    header_base, header_size = flash.address, 0
    flash_base, flash_size = flash.address, flash.size
    ram_base, ram_size = ram.address, ram.size
    relay_base, relay_size = 0, 0
    if p.flash:
        app = p.flash.entry("application")
        flash_base, flash_size = flash.address + app.offset, app.size
        header = p.flash.entry("image_header")
        if header:
            header_base, header_size = flash.address + header.offset, header.size
        app_ram = p.ram.entry("application")
        ram_base, ram_size = ram.address + app_ram.offset, app_ram.size
        relay = p.ram.entry("irq_relay")
        if relay:
            relay_base, relay_size = ram.address + relay.offset, relay.size
    if image == "bootloader":
        flash_base = flash.address + header_size
        flash_size = device.bootloader.size - header_size
    elif image == "app":
        header_base = flash.address + device.bootloader.size
        header_size = device.bootloader.image_header_size
        flash_base = header_base + header_size
        flash_size = flash.size - device.bootloader.size - header_size
    elif image != "standalone":
        raise ValueError(f"Unknown image kind: {image}")
    return ImageLayout(header_base, header_size, flash_base, flash_size,
                       ram_base, ram_size, relay_base, relay_size)


def validate_profile(device):
    p = device.profile
    if not p.flash:
        return  # Preserve implicit normal mode for existing device descriptions.
    if not device.arch.startswith("cortex-m"):
        raise ValueError("profile_param: explicit profiles currently require Cortex-M")
    flash = pick_flash(device)
    layout = image_layout(device)
    header = p.flash.entry("image_header")
    if header:
        if layout.header_base % 4 or header.size % 4:
            raise ValueError("image_header: base and size must be word aligned")
        if layout.header_base + header.size > layout.flash_base:
            raise ValueError("image_header must precede the application vector table")
    if layout.relay_size:
        if layout.relay_size < 8 or layout.relay_size % 8 or layout.relay_base % 8:
            raise ValueError("irq_relay: must reserve at least 8 bytes, aligned to 8")
        if device.cpu.core not in ("cortex-m0", "cortex-m0plus") or device.cpu.extra.get("vtor_supported", False):
            raise ValueError("irq_relay requires M0/M0+ without VTOR")
    if device.bootloader:
        bl = device.bootloader
        if type(bl.size) is not int or bl.size <= 0 or type(bl.image_header_size) is not int or bl.image_header_size < 0:
            raise ValueError("bootloader: size must be positive and image_header_size nonnegative")
        # Legacy bootloader partitions are relative to the entire selected flash.
        # Only a leading header + remaining application has an unambiguous split.
        app = p.flash.entry("application")
        if (any(e.name not in ("application", "image_header") for e in p.flash.layout)
                or (header and header.offset != 0)
                or app.offset != layout.header_size or app.offset + app.size != flash.size):
            raise ValueError("bootloader: flash layout must contain only an optional leading image_header and remaining application")
        if header and bl.image_header_size != header.size:
            raise ValueError("image_header.size must equal bootloader.image_header_size")
        if bl.size <= layout.header_size or bl.size + bl.image_header_size >= flash.size:
            raise ValueError("bootloader: header/partitions leave no FLASH for code")
    for kind in ("standalone", "bootloader", "app") if device.bootloader else ("standalone",):
        layout = image_layout(device, kind)
        if layout.flash_base % 4 or layout.ram_base % 8 or (layout.ram_base + layout.ram_size) % 8:
            raise ValueError("application: vector must be word aligned and RAM/stack 8-byte aligned")
        if device.cpu.extra.get("vtor_supported", device.cpu.core not in ("cortex-m0", "cortex-m0plus")):
            num_irq = device.cpu.extra.get("num_irq")
            if type(num_irq) is not int or num_irq < 0:
                raise ValueError("profile_param: cpu.num_irq required for VTOR alignment")
            alignment = max(128, 1 << ((16 + num_irq) * 4 - 1).bit_length())
            if layout.flash_base % alignment:
                raise ValueError(f"profile_param: {kind} vector base must align to 0x{alignment:X} for VTOR")
        elif layout.flash_base != flash.address and not layout.relay_size:
            raise ValueError("shifted vectors without VTOR require a reserved irq_relay entry")
