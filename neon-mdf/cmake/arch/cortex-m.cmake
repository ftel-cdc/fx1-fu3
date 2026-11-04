# Cờ compile/link chung cho mọi target Cortex-M. Gọi neon_mdf_apply_cortex_m_arch()
# sau khi tạo target, truyền đúng tên core lấy từ cpu.core trong device .yaml
# (vd "cortex-m0") + có FPU hay không (tham số thứ 3, tuỳ chọn, mặc định OFF).
#
# neon_mdf_apply_cortex_m_arch(<target> <mcpu> [<fpu_present>])
#   fpu_present = ON  -> "-mfloat-abi=hard -mfpu=fpv4-sp-d16" (Cortex-M4F THẬT
#                         dùng FPU phần cứng - xem NEON_MDF_TARGET_<X>_FPU
#                         trong cmake/mcu_variants.cmake).
#   fpu_present = OFF/không truyền -> "-mfloat-abi=soft" (M0/M0+ không có FPU,
#                         hoặc M4 không FPU) - hành vi mặc định như cũ.
function(neon_mdf_apply_cortex_m_arch target mcpu)
  if(ARGC GREATER 2 AND ARGV2)
    set(_float_flags -mfloat-abi=hard -mfpu=fpv4-sp-d16)
  else()
    set(_float_flags -mfloat-abi=soft)
  endif()

  target_compile_options(${target} PUBLIC
    -mcpu=${mcpu}
    -mthumb
    ${_float_flags}
  )
  target_link_options(${target} PUBLIC
    -mcpu=${mcpu}
    -mthumb
    ${_float_flags}
  )
endfunction()
