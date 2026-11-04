# Common compile/link flag for all RISC-V targets. Call neon_mdf_apply_riscv_arch() after target
# creation, pass correctly isa/abi taken from device.arch (e.g. "riscv-rv32ec" -> isa "rv32ec") -
# see cmake/mcu_variants.cmake.
#
# neon_mdf_apply_riscv_arch(<target> <isa> <abi>)
#   isa - e.g. "rv32ec" (without "_zicsr" - this function is from add, every startup file generated uses
#         the csrw/csrr command that requires the Zicsr extension, see
#         mcu-description/tools/backends/riscv/gen_startup.py).
#   abi, e.g. "ilp32e", matching ISA extension E and its reduced register file.
function(neon_mdf_apply_riscv_arch target isa abi)
  target_compile_options(${target} PUBLIC
    -march=${isa}_zicsr
    -mabi=${abi}
  )
  target_link_options(${target} PUBLIC
    -march=${isa}_zicsr
    -mabi=${abi}
  )
endfunction()
