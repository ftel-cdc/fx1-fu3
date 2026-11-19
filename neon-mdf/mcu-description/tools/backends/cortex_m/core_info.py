"""CMSIS and vector-table characteristics of GT32U2086's Cortex-M0 core."""
from dataclasses import dataclass

@dataclass(frozen=True)
class CortexMCoreInfo:
    core_header: str
    rev_macro: str
    needs_fpu_macro: bool
    has_extended_fault_handlers: bool

def lookup(core: str) -> CortexMCoreInfo:
    if core != "cortex-m0":
        raise ValueError(f"Unsupported core '{core}'; GT32U2086 requires cortex-m0")
    return CortexMCoreInfo("core_cm0.h", "__CM0_REV", False, False)
