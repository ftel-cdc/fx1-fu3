"""THAT lookup table (do not guess) CMSIS/architecture characteristics differ between core Cortex-M
variants that generator needs to know — shared by gen_cmsis_header.py (choose core_cmX.h/macro
*_REV) and gen_startup_vector.py (vector table entry 4/5/6/12 has
MemManage/BusFault/UsageFault/DebugMon_Handler OWN Please Reserved — ONLY available on
ARMv7-M+/ARMv8-M mainline, NOT on
ARMv6-M/ARMv8-M baseline).

Previously, these two files had their own separate whitelists, not shared: gen_cmsis_header.py has
`_CORE_HEADER` but when looking up the core is not found, it METHODALLY fallsback on Cortex-M0
(`.get(cpu.core, ("core_cm0.h", ...))`) — correct old bug that the file itself once fixed (hardcode
core_cm0.h for all cores) but still remains a "hidden fallback" seen because of raise.
gen_startup_vector.py is worse: no whitelist at all, using a fixed vector table template ARMv6-M
type for EVERY core — wrong with Cortex-M4 (`WEACT_F412`, `mpu_present: true`): entry 4/5/6/12 was
written `.word 0` (literal, not `Default_Handler`) seen because
MemManage_Handler/BusFault_Handler/UsageFault_Handler/DebugMon_Handler real — MemManage fault
(easily occurring on MPU chips) will jump to address 0 seen because loop safety.

Combine 1 source here so that the 2 backends don't deviate again. Core not in the table will RAISE
obviously at generate — NO guess/fallback silently.
"""

from dataclasses import dataclass


@dataclass(frozen=True)
class CortexMCoreInfo:
    core_header: str  # real CMSIS-Core file name (e.g. "core_cm4.h")
    rev_macro: str  # real macro name *_REV (e.g. "__CM4_REV")
    needs_fpu_macro: bool  # need to define __FPU_PRESENT (only core with optional FPU)
    has_extended_fault_handlers: bool
    # True = entry 4/5/6/12 of the table vector is MemManage_Handler/
    # BusFault_Handler/UsageFault_Handler/DebugMon_Handler TRUE (ARMv7-M or higher: Cortex-M3/M4/M7,
    # and ARMv8-M mainline: M33/M35P/M55/M85). False = 4 slots that are Reserved, all types of
    # faults share HardFault_Handler (true ARMv6-M: M0/M0+/M1, and ARMv8-M baseline: M23).


# List only the core that has been VERIFIED (compare vendor's real CMSIS device header + ARM
# Architecture Reference Manual of the correct core) — does not add another core even though the
# corresponding core_cmX.h file is already available in
# arch/CortexM/CMSIS/CMSIS_5/CMSIS/Core/Include/ (vendor all original CMSIS_5, not proof this SDK
# has verified layout vector table for that core).
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
            f"backends/cortex_m/core_info.py: core not confirmed '{core}' "
            "(need to know core_cmX.h/macro *_real REV + entry 4/5/6/12's "
            "the vector table includes MemManage/BusFault/UsageFault/DebugMon_Handler "
            "private please Reserved) - add vao _CORES in this file after changing "
            "real CMSIS device header of vendor (e.g. "
            "STMicroelectronics/cmsis_device_<family>) and the ARM Architecture "
            "Reference Manual's use that core, not guessing."
        )
    return info
