# Cờ compile/link chung cho mọi target RISC-V. Gọi neon_mdf_apply_riscv_arch()
# sau khi tạo target, truyền đúng isa/abi lấy từ device.arch (vd
# "riscv-rv32ec" -> isa "rv32ec") - xem cmake/mcu_variants.cmake.
#
# neon_mdf_apply_riscv_arch(<target> <isa> <abi>)
#   isa - vd "rv32ec" (KHONG kem "_zicsr" - ham nay TU THEM, moi startup file
#         sinh ra deu dung lenh csrw/csrr can extension Zicsr, xem
#         mcu-description/tools/backends/riscv/gen_startup.py).
#   abi - vd "ilp32e" (khop voi ISA co extension E - register file rut gon).
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
