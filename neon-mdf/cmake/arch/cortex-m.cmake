# Flag compile/link common to all Cortex-M targets. Call neon_mdf_apply_cortex_m_arch() after target
# creation, pass the correct core name taken from cpu.core in device .yaml (e.g. "cortex-m0") + have
# FPU please not (third parameter, optional, default OFF).
#
# neon_mdf_apply_cortex_m_arch(<target> <mcpu> [<fpu_present>])
#   fpu_present = ON -> "-mfloat-abi=hard -mfpu=fpv4-sp-d16" (Cortex-M4F REALLY uses FPU hardware - see
#                         NEON_MDF_TARGET_<X>_FPU in cmake/mcu_variants.cmake). fpu_present = OFF/no
#                         transmission -> "-mfloat-abi=soft" (M0/M0+ without FPU, or M4 without FPU)
#                         - default behavior as before.
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
