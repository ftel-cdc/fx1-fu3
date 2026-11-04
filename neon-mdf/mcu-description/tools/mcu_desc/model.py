"""Intermediate data model shared by every arch backend.

Kept arch-agnostic on purpose: peripheral/register/memory shape doesn't
depend on the CPU core. Only cpu.* fields vary per architecture, and each
backend (backends/cortex_m/, backends/riscv/) is responsible for knowing
which cpu.* fields it needs.
"""
from dataclasses import dataclass, field


@dataclass
class Field:
    name: str
    bit_offset: int
    bit_width: int
    description: str = ""


@dataclass
class RegisterAlias:
    """Một view logic của register dùng chung cùng địa chỉ vật lý.

    Ví dụ UART 16550 dùng offset 0x00 cho RBR khi đọc, THR khi ghi và DLL
    khi DLAB=1. Alias giữ đúng tên/access/bitfield mà không làm sai struct C.
    """
    name: str
    access: str
    reset: int
    description: str = ""
    fields: list = field(default_factory=list)  # list[Field]


@dataclass
class Register:
    name: str
    offset: int
    size: int = 32
    access: str = "rw"
    reset: int = 0
    description: str = ""
    fields: list = field(default_factory=list)  # list[Field]
    aliases: list = field(default_factory=list)  # list[RegisterAlias]


@dataclass
class Peripheral:
    name: str
    base: int
    description: str = ""
    registers: list = field(default_factory=list)  # list[Register]
    type: str = ""  # tên struct dùng chung giữa nhiều instance (vd UART0..UART4 -> "UART").
    # Mặc định = name (peripheral tự đứng 1 mình, struct trùng tên instance).
    # loader.py điền default này khi field `type` không có trong YAML.


@dataclass
class MemoryRegion:
    name: str
    kind: str  # flash | rom | ram
    address: int
    size: int


@dataclass
class Interrupt:
    number: int
    name: str
    description: str = ""


@dataclass
class Cpu:
    core: str
    revision: str
    max_freq_hz: int
    extra: dict  # arch-specific leftover fields (nvic_priority_bits, num_irq, ...)


@dataclass
class BootloaderUart:
    peripheral: str  # ten peripherals[].name dung lam UART cho bootloader (vd "USART1")
    tx_pin: str
    rx_pin: str
    af: int
    baud: int


@dataclass
class Bootloader:
    """Khai bao TUY CHON: device co bootloader hay khong (xem
    schema/common.schema.yaml). Khi co, generator sinh THEM
    <DEVICE>_BOOTLOADER.ld + <DEVICE>_APP.ld (ngoai <DEVICE>_FLASH.ld van
    sinh binh thuong) - xem gen_linker_script.py."""
    size: int               # kich thuoc partition bootloader (byte), tinh tu dau flash
    image_header_size: int  # kich thuoc "slot" header truoc vector table cua app (byte) -
    # PHAI du lon de flash.address+size+image_header_size thoa man align
    # SCB->VTOR (xem loader.py _check_bootloader_vtor_alignment) - KHONG
    # phai chi can >= sizeof(image_header_t) that su.
    uart: BootloaderUart


@dataclass
class EraseSector:
    """1 nhom sector erase-cung-kich-thuoc, dang run-length (xem
    schema/common.schema.yaml `erase_sectors`) - vd STM32F412 co 3 nhom:
    4 sector 16KB, 1 sector 64KB, 3 sector 128KB, LIET KE THEO DUNG THU TU
    dia chi tang dan (khong duoc dao lon, tools/gen_flashmap.py doc tuan
    tu tu dau flash de tinh ranh gioi)."""
    size: int
    count: int


@dataclass
class LayoutEntry:
    name: str
    offset: int
    size: int  # Resolved byte count; "remaining" is expanded by the loader.
    usage: str  # reserved | allocatable
    description: str = ""


@dataclass
class ProfileMemory:
    region: str
    layout: list  # list[LayoutEntry]
    description: str = ""

    def entry(self, name):
        return next((entry for entry in self.layout if entry.name == name), None)


@dataclass
class ProfileParam:
    name: str = "normal"
    flash: object = None  # Optional[ProfileMemory]; None retains legacy selection.
    ram: object = None  # Optional[ProfileMemory]


@dataclass
class Device:
    device: str
    vendor: str
    family: str
    arch: str
    description: str
    cpu: Cpu
    memory: list  # list[MemoryRegion]
    peripherals: list  # list[Peripheral]
    interrupts: list  # list[Interrupt]
    bootloader: object = None  # Optional[Bootloader] - None = device khong co bootloader partition
    erase_sectors: object = None  # Optional[list[EraseSector]] - None = khong khai, tools/gen_flashmap.py
    # bo qua validate can sector khi device khong cung cap thong tin nay (KHONG bat buoc, chi de validate
    # them cho device co sector khong deu nhau - xem WEACT_F412.yaml).
    profile: ProfileParam = field(default_factory=ProfileParam)

    def memory_by_kind(self, kind: str):
        return [m for m in self.memory if m.kind == kind]

    def linker_filename(self, image: str):
        # Lowercase device IDs use fully lowercase filenames; legacy IDs keep
        # their existing names so older targets do not change paths.
        suffix = image.lower() if self.device.islower() else image.upper()
        return f"{self.device}_{suffix}.ld"
