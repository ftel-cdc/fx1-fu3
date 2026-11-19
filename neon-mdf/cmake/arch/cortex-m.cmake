# GT32U2086 uses Cortex-M0 with software floating point.
function(neon_mdf_apply_cortex_m_arch target mcpu)
  if(NOT mcpu STREQUAL "cortex-m0")
    message(FATAL_ERROR "GT32U2086 requires cortex-m0")
  endif()
  target_compile_options(${target} PUBLIC -mcpu=cortex-m0 -mthumb -mfloat-abi=soft)
  target_link_options(${target} PUBLIC -mcpu=cortex-m0 -mthumb -mfloat-abi=soft)
endfunction()
