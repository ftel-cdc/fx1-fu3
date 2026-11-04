"""Bảng tra cứu THẬT (không đoán) các đặc điểm CMSIS/kiến trúc khác nhau
giữa các biến thể lõi Cortex-M mà generator cần biết — dùng chung bởi
gen_cmsis_header.py (chọn core_cmX.h/macro *_REV) và gen_startup_vector.py
(vector table entry 4/5/6/12 có MemManage/BusFault/UsageFault/DebugMon_Handler
RIÊNG hay Reserved — CHỈ có trên ARMv7-M+/ARMv8-M mainline, KHÔNG có trên
ARMv6-M/ARMv8-M baseline).

Trước đây 2 file này tự whitelist riêng, không dùng chung: gen_cmsis_header.py
có `_CORE_HEADER` nhưng lúc tra không thấy core thì ÂM THẦM fallback về
Cortex-M0 (`.get(cpu.core, ("core_cm0.h", ...))`) — đúng bug cũ mà chính file
đó từng tự sửa 1 lần (hardcode core_cm0.h cho mọi core) nhưng vẫn còn sót lại
dạng "fallback ngầm" thay vì raise. gen_startup_vector.py thì tệ hơn: không
hề whitelist gì cả, dùng 1 template vector table cố định kiểu ARMv6-M cho MỌI
core — sai với Cortex-M4 (`WEACT_F412`, `mpu_present: true`): entry 4/5/6/12
bị ghi `.word 0` (literal, không phải `Default_Handler`) thay vì
MemManage_Handler/BusFault_Handler/UsageFault_Handler/DebugMon_Handler thật —
MemManage fault (dễ xảy ra trên chip có MPU) sẽ nhảy tới địa chỉ 0 thay vì
vòng lặp an toàn.

Gộp về 1 nguồn ở đây để 2 backend không lệch nhau lần nữa. Core chưa có trong
bảng sẽ RAISE rõ ràng lúc generate — KHÔNG đoán/fallback âm thầm.
"""

from dataclasses import dataclass


@dataclass(frozen=True)
class CortexMCoreInfo:
    core_header: str  # tên file CMSIS-Core thật (vd "core_cm4.h")
    rev_macro: str  # tên macro *_REV thật (vd "__CM4_REV")
    needs_fpu_macro: bool  # có cần định nghĩa __FPU_PRESENT không (chỉ core có FPU tuỳ chọn)
    has_extended_fault_handlers: bool
    # True = entry 4/5/6/12 của vector table là MemManage_Handler/
    # BusFault_Handler/UsageFault_Handler/DebugMon_Handler THẬT (ARMv7-M trở
    # lên: Cortex-M3/M4/M7, và ARMv8-M mainline: M33/M35P/M55/M85).
    # False = 4 slot đó là Reserved, mọi loại fault đều dùng chung
    # HardFault_Handler (ARMv6-M thật: M0/M0+/M1, và ARMv8-M baseline: M23).


# Chỉ liệt kê core đã XÁC NHẬN THẬT (đối chiếu CMSIS device header thật của
# vendor + ARM Architecture Reference Manual của đúng core đó) — không thêm
# core khác dù file core_cmX.h tương ứng đã có sẵn trong
# arch/CortexM/CMSIS/CMSIS_5/CMSIS/Core/Include/ (vendor toàn bộ CMSIS_5 gốc,
# không phải bằng chứng SDK này đã verify layout vector table cho core đó).
_CORES = {
    "cortex-m0": CortexMCoreInfo("core_cm0.h", "__CM0_REV", False, False),
    "cortex-m0plus": CortexMCoreInfo("core_cm0plus.h", "__CM0PLUS_REV", False, False),
    "cortex-m3": CortexMCoreInfo("core_cm3.h", "__CM3_REV", False, True),
    "cortex-m4": CortexMCoreInfo("core_cm4.h", "__CM4_REV", True, True),
}


def lookup(core: str) -> CortexMCoreInfo:
    info = _CORES.get(core)
    if info is None:
        raise ValueError(
            f"backends/cortex_m/core_info.py: chua xac nhan core '{core}' "
            "(can biet core_cmX.h/macro *_REV that + entry 4/5/6/12 cua "
            "vector table co MemManage/BusFault/UsageFault/DebugMon_Handler "
            "rieng hay Reserved) - them vao _CORES trong file nay SAU KHI doi "
            "chieu CMSIS device header that cua vendor (vd "
            "STMicroelectronics/cmsis_device_<family>) va ARM Architecture "
            "Reference Manual cua dung core do, khong doan."
        )
    return info
